/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/tty_nvs.c,v $
 * $Revision: 1.8.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:13:01 $
 */

/*
 * Network virtual terminals via sockets - server end
 *
 * Based on code written by Charles Hedrick (at Rutgers) and Marshall T. Rose
 * (at Northrop?).  Rewritten by Hewlett-Packard Company for use in their HP-UX
 * operating system.
 */

/*
 * The code in this module supports the NVS, or remote end of the connection
 * (i.e., the link between the network and the pseudo-terminal).
 * The purpose of this code is to emulate efficiently the character-
 * shuffling functions performed by /etc/telnetd.  The telnet on the other
 * host will believe it is talking to telnetd, when it is in reality
 * talking to the NVS kernel software herein.
 *
 * The NVS kernel software achieves performance improvements by handling
 * incoming character traffic at interrupt level, eliminating the overhead
 * and delays resulting from scheduling a user-mode process (telnetd).
 * Outgoing character traffic is dumped directly into the socket by
 * nvs_output(), further eliminating the need for service
 * by a user-mode process.
 *
 * The TELNET portion of this code handles the "easy" part of the TELNET
 * server FSM.  That is, it byte-stuffs IACs on output.  On input, when
 * an IAC is detected the user-mode process (telnetd) is called to process
 * the TELNET negotiation.  Upon completion of the task, telnetd will
 * call the SIOCJNVS ioctl() to resume kernel-mode handling of the connection.
 *
 * To differentiate between the different server protocols, the third argument
 * to the SIOCJNVS ioctl is encoded as follows:
 *
 *	+--------+--------+--------+--------+
 *	| input  | output |    pty   fd     |
 *	+--------+--------+--------+--------+
 *	  8 bits   8 bits       16 bits
 *
 * where
 *	input	=	4 for RLOGIN
 *			8 for "normal" TELNET
 *			9 for "binary" TELNET
 *	output	=	1 for "normal" TELNET
 *			2 for "normal" TELNET temporary output-only join
 *			4 for RLOGIN
 *			9 for "binary" TELNET
 *		       10 for "binary" TELNET temporary output-only join
 *
 *
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"		/* for lint */
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ld_dvr.h"
#include "../h/uio.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/nvs.h"
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */


/*
** Turning on NVS_QA enables tracing for tty_nvs.c.  Trace info is
** placed in the circular buffer,  nvs_pbuf, when nvs_debug is also non-
** zero.  #undef'ing NVS_QA causes this tracing code not to be compiled into
** the kernel.
**/
#ifdef NVS_QA
int nvs_debug = 0;
#define nvs_emsg(st,pa)		if (nvs_debug) nvs_printbuf(st,pa)
#else /* !NVS_QA */	/* Should be off in released customer kernels */
#define nvs_emsg(st,pa)
#endif /* !NVS_QA */


/* in modes */
#define IN_RLOGIN	0x004		/* rlogin mode */
#define IN_TELNET	0x008		/* telnet mode */
#define IN_BINARY	0x001		/* telnet binary modifier*/
#define IN_CR		0x100		/* cr seen */
#define IN_IS_SCHED	0x400		/* nvs_input scheduled via timeout */
/* out modes */
#define OUT_TELNET	0x001		/* special IAC processing */
#define OUT_TELNET_TMP	0x002		/* temporary output-only join */
#define OUT_RLOGIN	0x004		/* special urgent mode stuff */
#define OUT_BINARY	0x008		/* telnet binary modifier*/
#define OUT_IS_SCHED	0x100		/* nvs_output scheduled via timeout */
#define IAC		255
#define FALSE		0
#define	TRUE		1
#define SEND_SLACK	14	/* wasted space allowed in send mbufs */

#ifdef hp9000s800
#define ptym_open       pty0_open
#define ptym_control    pty0_option1
#endif
extern int ptym_open(), ptym_control();


/*
 * pty-to-socket correspondence table.
 *
 * This table is searched linearly, but only for creating
 * connections.  The socket structure keeps an index into this table.
 * The table is indexed by pty minor number.
 * You must be careful to make sure that an index still
 * represents a valid connection.
 */
extern struct nvsj nvsj[];
extern long npty;
int	nvsichz = 0;
extern int	sowakeup();
extern struct tty  *pt_line[];
void nvs_callout(), nvs_input_t(), nvs_output_t();


/*
 * Mate a tty and a socket
 * Returns UNIX error code
 * Must be called with MP protection on.  Returns with the socket buffers locked.
 * We lock the buffers to lock out the
 * normal read and write code from using this socket while it
 * is assigned to us.
 */
nvs_add(dev, so, anvs, inmode, outmode)
	dev_t dev;
	struct socket *so;
	struct nvsj **anvs;	/* return addr of NVS context here */
	int inmode, outmode;
{
	register struct nvsj *nvs, *nx;
#ifdef hp9000s800
	extern int hz;
#endif
	int error = 0;

	nvs_emsg("nvs_add: socket:0x%x\n",(int)so);
	nvs_emsg("nvs_add: outmode:0x%x\n",outmode);
	if ((so->so_snd.sb_flags & SB_LOCK) || (so->so_rcv.sb_flags & SB_LOCK)){
	    nvs_emsg("nvs_add: socket locked:0x%x\n",(int)so);
	    return(EACCES);
	}
/*
 * We have to lock these before setting up the link.
 */
	sblock(&so->so_snd);
	sblock(&so->so_rcv);

	nvs = &nvsj[minor(dev)];
	if (nvs->nvs_so_int) {
		error = EBUSY;
	    nvs_emsg("nvs_add: EBUSY:%d\n",minor(dev));
		goto error_out;
		/* pty already an NVS */
	}
	/* All is clear, mate them */
	*anvs = nvs;			/* pass context pointer to caller */
	nvs->nvs_so_int = (long)so;
	nvs->nvs_in_mode = inmode;
	nvs->nvs_out_mode = outmode;
	if ((outmode & OUT_TELNET_TMP) == 0) {
	       nvs_emsg("setting SB_NVS bit\n",0);
		ptym_control(dev, SDC_CALLOUT, nvs_callout);
		so->so_rcv.sb_flags |= SB_NVS;
		so->so_nvs_index = minor(dev);
	}
	nvs_emsg("Index is %d\n",minor(dev));
/*
 * The call to nvs_input depends upon the fact that we are called with
 * MP protection.
 */
	if ((outmode & OUT_TELNET_TMP) == 0)
		nvs_input(minor(dev)); /* get characters flowing */
	(void) nvs_output(minor(dev));	/* ditto */
	nvs_emsg("nvs_add: index: %d\n", minor(dev)); 
	return 0;

error_out:
	sbunlock(&so->so_snd);
	sbunlock(&so->so_rcv);
	nvs_emsg("error rtn from nvs_add, error= %d\n",dev);
	return(error);
}


/*
 * Undo a tty/socket correspondence
 * Must be called with MP protection on.
 * Since this routine is only called
 * from ioc_join, and it is only called once, the connection is
 * guaranteed to still be there.
 */
nvs_del(dev)
	dev_t dev;
{
	register struct nvsj *nvs = &nvsj[minor(dev)];
	register struct socket *so;

	nvs_emsg("nvs_del: socket: 0x%x\n",nvs->nvs_so_int);
	so = (struct socket *)(nvs->nvs_so_int);
	so->so_rcv.sb_flags &= ~(SB_NVS | SB_NVS_WAIT);
	so->so_snd.sb_flags &= ~(SB_NVS | SB_NVS_WAIT);
	ptym_control(dev, SDC_CALLOUT, NULL);
	sbunlock(&so->so_rcv);
	sbunlock(&so->so_snd);
	/*
	 * Delete this entry
	 */
	nvs->nvs_so_int = 0;		/* mark entry as free */
	nvs_emsg("nvs_del: %s\n","leaving");
}


/*
 * Handle ioctl request on socket to join socket and pty, called
 * from protocol ioctl logic.  Protocol-specific validation is
 * done before you get here.
 *
 * If the connection was successfully established, sleep and wait
 * until:
 *	it is broken at the remote end
 * or
 *	a signal is received on the local end
 * or
 *	an IAC is received in the input stream (TELNET service only)
 *
 * Returns
 *	0 = remote NVT closed the connection
 *	EINTR = interrupted by signal
 *	Anything else = an error establishing the connection
 */
nvs_ioc_join(so, ptyfd)
	register struct socket *so;
	int ptyfd;		/* pty file descriptor */
{
	register struct file *fp;
	register struct inode *ip;
	int error, s;
	struct nvsj *nvsp;
	int inmode, outmode;
	label_t oldqsave;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	nvs_emsg("Entering nvs_ioc_join 0x%x\n", (int)so);
	outmode = (ptyfd >> 16) & 0xFF;
	inmode = ptyfd >> 24;
	ptyfd = ptyfd & 0xFFFF;
	nvs_emsg("ptyfd %d\n", ptyfd);

	/*
	 * If this is the first time, initialize some globals
	 */
	if (nvsichz == 0) {
		nvsichz = hz / 3;
	}

	/*
	 * Validate modes:  we only support telnet for now.
	 */
	if ((outmode & ~OUT_BINARY) != OUT_TELNET &&
	    (outmode & ~OUT_BINARY) != OUT_TELNET_TMP) {
	    return EINVAL;
	}
	if ((inmode & ~IN_BINARY) != IN_TELNET) {
	    return EINVAL;
	}
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/*
	 * Validate file descriptor and ensure it references a pty
	 */
	if ((fp = getf(ptyfd)) == 0) {
	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	    return EBADF;
	}
	{
		register struct vnode *vp;

		vp = (struct vnode *)fp->f_data;
		if (fp->f_type != DTYPE_VNODE) {
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		    return ENOTTY;
		}
		ip = VTOI(vp);
		if ((ip->i_mode & IFMT) != IFCHR ||
		    cdevsw[major(ip->i_rdev)].d_open != ptym_open)  {
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		    return ENOTTY;		/* really ENOTPTY :-) */
		}
	}
	/* Because the minor number of the pty is stored as a quick
	 * reference in the socket structure as a short, we need to
	 * check to make sure that the minor number will fit.  This limits
	 * the nvs module to only using the first 32K defined ptys, which
	 * is not considered a limitation for the forseeable future.
	 */
	if (minor(ip->i_rdev) > 0x7fff) {
	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	    return ENFILE;
	}

	/*
	 * Argument socket and pty are valid, now join them and wait
	 */
	if (error = nvs_add(ip->i_rdev, so, &nvsp, 
			    inmode, outmode)) {
	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	    return error;
	}
/*  41 - it is safest to save qsave.  Actually, it would be more elegant to
 *  use the new errset version of sleep, but this is portable.
 */
	oldqsave = u.u_qsave;
	if (setjmp(&u.u_qsave)) {
		/* The process received a signal.
		** MPNET: turn MP protection on.  We have to re-acquire
		** the semaphore.  If the previous NETMP_GO_UNEXCLUSIVE
		** gave up a semaphore from another empire to gain the n/w
		** semaphore, that fact will be stored in "savestate"
		** and we will re-acquire it upon return (via the 
		** macro)
		 */
		NET_PSEMA();
		u.u_qsave = oldqsave;
		nvs_emsg("longjmp into kernel socket:0x%x\n",(int)so);
		nvs_del(ip->i_rdev);		/* disassociate pty&socket */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate); /* MPNET */
		return EINTR;
	}
	while (so->so_rcv.sb_flags & SB_NVS)
	  {
	    nvs_emsg("ioc_join:sleep,nvsp :%d\n",nvsp);
	        /* wait for signal or disconnect */
	    sleep((caddr_t)nvsp, PZERO + 1);
	        /* Don't care whether we came out of the sleep via wakeup()
	        ** or via signal.
	        */
	  }

	/* The remote disconnected */
/* 41 - safest to save qsave */
	u.u_qsave = oldqsave;
	nvs_emsg("ioc_join:wakeup from sleep,nvsp:0x%x\n",nvsp);
	nvs_del(ip->i_rdev);			/* disassociate pty&socket */
	nvs_emsg("Leaving nvs_ioc_join so 0x%x\n", (int) so);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate); /* MPNET */
	return 0;
}


/*
 * Move characters from pty outq to network
 * Must be called with MP protection.  We make no assumptions
 * about whether there is really an open connection behind all of this.
 * The whole thing could have vanished, or the index could even 
 * represent a completely different connection than when it was first
 * formed.  I doesn't really matter.
 * Return value indicates whether any data was actually transfered.
 */
/*ARGSUSED*/
int
nvs_output(nvsindex)
	register int		nvsindex;
{
  	register struct mbuf	*m;
	register int		space;
	register u_char		*dp;
	register int		iacs;
	register int		count;
	register int		nakcrs;
	register int		outmode;
	struct mbuf		*mhead, *m_curr;
	struct socket		*so;
	struct nvsj		*nvs;
	int			junk;
	struct iovec		buffer;
	int			more_data;

	nvs_emsg("Entering nvs_output\n",0);
	mhead = NULL;

	/* If the connection is no longer valid, do nothing. */
	nvs = &nvsj[nvsindex];
	if ((so=(struct socket *)(nvs->nvs_so_int)) == 0) {
		nvs_emsg("nothing in nvs_so_int %x\n", nvs->nvs_so_int);
		return 0;
	}
	so->so_snd.sb_flags &= ~SB_NVS_WAIT;

	/* If socket is dead, drain the data buffered in the pty. */
	if ((so->so_state & SS_CANTSENDMORE) || so->so_error) {
		while (ptym_control(nvsindex, SDC_GETC, &junk) != ENOBUFS) 
			;
		return 0;
	}

	/*
	 * See how much space we have so that we do not
	 * overflow the socket buffer.
	 */
	space = sbspace(&so->so_snd);
	nvs_emsg("nvs_output:space= %d\n", space);
	if (space <= MLEN ) {
		nvs_emsg("nvs_output:insuff. space %d\n", space);
		so->so_snd.sb_flags |= SB_NVS_WAIT; /* wait till space */
		return 0; /* don't resched. TCP code will call us when space */
	}

	/* Assume up front that the pty has data, which it usually will. */
	more_data = TRUE;
	/* junk is used to remember if I've left an unprocessed \r
	 * at the end of an mbuf.
	 */
	junk = 0;
	outmode = nvs->nvs_out_mode;
	do {
		/*
		 * Send a chunk of character traffic that may be pending.
		 * Each pass through this loop fills one more mbuf and
		 * hangs it on the end of the mbuf chain.
		 */
		m_curr = m;	/* m is junk 1st time thru, not a problem */
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			/*
			 * For some reason, there are no mbufs
			 * available, so I reschedule myself to
			 * run again in a little while to retry.
			 */
			if ((nvs->nvs_out_mode &
			      (OUT_TELNET_TMP|OUT_IS_SCHED)) == 0) {
				nvs_emsg("no mbufs, try later\n", 0);
				nvs->nvs_out_mode |= OUT_IS_SCHED;
				net_timeout(nvs_output_t, nvsindex, nvsichz);
			}
			break;
		}
		if (mhead == NULL) 
			mhead = m;
		else
			m_curr->m_next = m;
		count = m->m_len = MLEN;
		nvs_emsg("nvs_output:count= %d\n", count);
		buffer.iov_base = mtod (m, caddr_t);

		/*
		 * This is an optimization.  If there are no IAC's, we can
		 * just copy the data from the pty directly into the tcp
		 * mbuf.  If there are IAC's, we have to turn each single
		 * IAC into two IAC's.  Since we do not know
		 * beforehand how much data we will get from the pty, we get
		 * only as much data as half our available buffer space,
		 * so we can handle the worst case of all IAC's.  Then we
		 * parse this data counting IAC's.  If there were IAC's,
		 * then we shift the data, inserting second IAC
		 * where needed.  We keep doing this until our mbuf is
		 * sufficiently full.  If, at any time, the pty gives us
		 * less data than we asked it for, we know for future
		 * reference that we have emptied the pty's t_outq.
		 */
		if (outmode & OUT_BINARY)
			while (count >= SEND_SLACK && more_data) {
				buffer.iov_len = count>>1;
				dp = (u_char *) buffer.iov_base;
				ptym_control(nvsindex, SDC_GETB, &buffer);
				nvs_emsg("SDC_GETB 1, buffer.iov_len=%d\n", 
				    buffer.iov_len);
				if (buffer.iov_len < (count>>1))
				    more_data = FALSE;/*pty buffer was emptied*/
				count -= buffer.iov_len;
				for(iacs=0;((caddr_t)dp)<buffer.iov_base;dp++)
				    if (*dp == IAC) iacs++;
				buffer.iov_base += iacs;
				count -= iacs;
				while (iacs) {
					dp--;
					if ((*(dp+iacs) = *dp) == IAC) {
						iacs--;
						*(dp+iacs) = IAC;
					}
				}
			}
		/*
		 * The loop below is like the one above, but it is
		 * used in the case of non-binary output modes.
		 * Here, we not only do the IAC processing, but we
		 * also look for any \r that is not followed by a
		 * \n.  We need to pad these with a \0.  For the
		 * sake of efficiency, we do not worry about the
		 * case where a \r is the last byte in an mbuf here.
		 * All we do is remember that it exists, and check
		 * for it outside the loop just before sending
		 * the data to TCP.
		 */
                /*
                 * The following changes in the algorithm for
                 * transfering data from pty to the socket mbuf
                 * were added to sufficiently handle the worse
                 * case scenario.  The algorithm retrieves half
                 * of the available space in the mbuf from the
                 * pty.  This was done to handle the previous and wrong
                 * worse case of all "Naked CRs", all "IAC", or a
                 * combination of both.   The actual worse case is
                 * the previous data gotten from the pty ending with
                 * a "Naked CR" and then have the next buffer full of
                 * all "Naked CR" and/or IAC.  This will require that
                 * 2 * (sizeof buffer) + 1 bytes to be added to the mbuf.
                 * This will cause the last extra bytes to be written
                 * past the data area of the mbuf and one byte
                 * into the next field of the mbuf structure.  This
                 * will panic the system if the altered next field
                 * points to an invalid area of memory.  This worse case
                 * can be prevented if the maximum amount of data
                 * gotten from the pty is (half of the available
                 * space in the mbuf) minus 1.  brd 12/12/91
                 */
		else  {
			nakcrs = 0;
			nvs_emsg("non-binary mode,count=%d\n", count);
			while (count >= SEND_SLACK && more_data) {
				buffer.iov_len = (count>>1) - 1;
				dp = (u_char *) buffer.iov_base;
				nvs_emsg("calling ptym_control\n", 0);
				ptym_control(nvsindex, SDC_GETB, &buffer);
				nvs_emsg("SDC_GETB 2, buffer.iov_len=%d\n", 
				    buffer.iov_len);
				if (buffer.iov_len < ((count>>1) - 1))
				    more_data = FALSE;
				count -= buffer.iov_len;
				/* Put in phantom \n to handle trailing
				 * naked \r case -- we know that there will
				 * always be room for it.
				 */
				*(buffer.iov_base) = '\n';
				if (nakcrs)
				    dp--;
				for (iacs= nakcrs=0;
				      (caddr_t)dp < buffer.iov_base;dp++){
				    if (*dp == '\r' &&
					*(dp+1) != '\n') nakcrs++;
				    else if (*dp == IAC) iacs++;
				}
				iacs += nakcrs;
				buffer.iov_base += iacs;
				count -= iacs;
				while (iacs) {
					if (nakcrs) {
					    if (*(dp--) != '\n' &&
						*(dp) == '\r') {
						    *(dp+iacs) = 0;
						    nakcrs--;
						    iacs--;
					    }
					}
					else dp--;
					if ((*(dp+iacs) = *dp) == IAC) {
					    iacs--;
					    *(dp+iacs) = IAC;
					}
				}
				if (buffer.iov_len &&
				    *(buffer.iov_base-1) == '\r')
					nakcrs++;
			}
			/* If the mbuf has \r as its last character,
			 * remember that we have not checked it
			 * for padding so that we can do it later.
			 * This hopefully should be an infrequent case.
			 */
			if (nakcrs) {
				junk++;
				space--; /* save space for \0 */
			}
		}
		space -= m->m_len;
		m->m_len -= count;
	} while (space > 0 && more_data);

	/* Strip trailing mbuf if it is empty.
	 * This should be a rare situation.
	 */
	if (m && m->m_len == 0) {
		if (m == mhead)
			mhead = NULL;
		else
			m_curr->m_next = NULL;
		m_free(m);
	}

	/* Send data if there is any. */
	if (mhead) {
		if (junk) {
			/* We get here only if we are in non-binary mode
			 * and one or more of the mbufs in the chain
			 * ends in \r.  In this case, we need to make
			 * a check of each one to see if a \n is following
			 * it and pad it with \0 if not.  We are guaranteed
			 * enough space to stuff each \0.  This is a kludge
			 * but needs to be done to be correct.  Feel free
			 * to replace it with some more efficient code.
			 * Fortunately, execution of this loop should be
			 * relatively rare.
			 */
			for (m = mhead; m; m = m->m_next) {
				if (*(mtod(m, caddr_t)+m->m_len-1) == '\r') {
					if (m->m_next) {
						if (*mtod(m->m_next, caddr_t)
						    == '\n')
							continue;
					}
					/* I should really set a flag here
					 * if there is a final \r
					 * so I can check next time through.
					 * However, 4.3BSD gets away with
					 * not doing so, and the code will
					 * be much simpler.
					 */
					*(mtod(m, caddr_t)+m->m_len) = 0;
					m->m_len++;
				}
			}
		}
		junk = 0;
		nvs_emsg("calling tcp PRU_SEND,m= %x\n", mhead);
		nvs_emsg("len= %d\n", nvs_mlen(mhead));
		nvs_emsg("so->sosnd.sb_cc=%d\n", so->so_snd.sb_cc);
		(*so->so_proto->pr_usrreq)(so, PRU_SEND, mhead, 0, 0);
		nvs_emsg("now so->sosnd.sb_cc=%d\n", so->so_snd.sb_cc);
	}

	/* Tell underlying protocol to wake me up if I exited the
	 * loop due to lack of socket buffer space.
	 */
	if (space <= MLEN && more_data &&
	    (nvs->nvs_out_mode & OUT_TELNET_TMP) == 0)
		so->so_snd.sb_flags |= SB_NVS_WAIT; /* wait till space */

       	nvs_emsg("Leaving nvs_output,more_data=%d\n",more_data);
	/*
	 * Indicate whether we actually sent any data into the socket.
	 * Even though it is now no longer allowed to dereference mhead,
	 * we can still check it to see if we called PRU_SEND.
	 */
	if (mhead)
		return 1;
	else
		return 0;
}


/*
 * Called from sowakeup() when NVS has incoming characters from the net,
 * to transfer those characters from the socket to the tty's input queue.
 * Also called from nvs_add() when join is first made, and called from
 * net_timeout() when triggered by pty or in event of naked CR.
 * Logic here is similar to that of soreceive() in so_socket.c
 * Must be called with MP protection.
 */
nvs_input(nvsindex)
	register int nvsindex;
{
	struct sockbuf *sb;		/* so_rcv */
	register struct nvsj *nvs;
	register struct mbuf *m;
	register struct socket *so;
	register unsigned char *cp;
	register int n;
	register int nvsmode;
	unsigned char inchar;
	int local_flags = 0;

	nvs_emsg("Entering nvs_input, index= %d\n", nvsindex);
	nvs = &nvsj[nvsindex];
	if ((so = (struct socket *)(nvs->nvs_so_int)) == NULL) {
		nvs_emsg("Leaving nvs_input, no skt\n", 0);
		return;
	}
	sb = &(so->so_rcv);
	nvsmode = nvs->nvs_in_mode;  /* flags. use a register for speed */
	/*
	 * If IN_IS_SCHED, then do an un-timeout.  We must have gotten
	 * here because data came in before the timeout expired.
	 */
	if (nvsmode & IN_IS_SCHED) {
		net_untimeout(nvs_input_t, nvsindex);
		nvsmode &= ~IN_IS_SCHED;
		nvs->nvs_in_mode &= ~IN_IS_SCHED;
	}

	/*
	 * Have located the pty, now pass it the data
	 */
	if (sb->sb_cc <= 0) {
		/*
		 * If the socket is dead, wake up telnetd.  See more
		 * explanation at end of procedure.
		 */
		if ((so->so_state & (SS_CANTRCVMORE | SS_CANTSENDMORE)) ||
		    so-> so_error) {
			sb->sb_flags &= ~SB_NVS;
		        nvs_emsg("nvs_input:wake up,nvsp :%x\n",nvs);
			wakeup((caddr_t)nvs);
		}
		nvs_emsg("Leaving nvs_input, ckt died\n", 0);
		return;
	}
	/*
	 * Process each mbuf on the socket's rcv queue
	 */
	nvs_emsg(" so->sorcv.sb_cc=%d\n", so->so_rcv.sb_cc);
	while (sb->sb_cc > 0) {
		m = sb->sb_mb;
		cp = mtod(m, unsigned char *);
		n = m->m_len;
		while (n) {
		  	inchar = *cp;
			if (nvsmode & IN_CR) {
				if (inchar == '\0' || inchar == '\n') {
					n--;
					cp++;
				}
				nvsmode &= ~IN_CR;
			}

			else {	/* not in IN_CR mode */
				struct iovec buffer;

				/*
				 * IAC causes us to wake up telnetd to handle
				 * it.  We don't worry about PRU_RCVD here
				 * because telnetd will immediately do a recv
				 * on the socket which will take care of things
				 * for us.
				 */
				if (inchar == IAC) {
					sbdrop(sb, m->m_len - n);
					sb->sb_flags &= ~SB_NVS;
		nvs_emsg("nvs_input: IAC encountered, wake up,nvsp :%d\n",nvs);
					wakeup((caddr_t)nvs);
					return;
				}

				buffer.iov_base = (caddr_t) cp;
				do {
					inchar = *cp;
					if (inchar == IAC)
						break;
					n--;
					cp++;
					if (inchar == '\r' &&
					    (nvsmode & IN_BINARY) == 0) {
						nvsmode |= IN_CR;
						break;
					}
				} while (n);
				buffer.iov_len = (caddr_t) cp - buffer.iov_base;
		        nvs_emsg("nvs_input:calling ptym_control for input\n",0);
				ptym_control(nvsindex,
					     SDC_PUTB, &buffer);
		nvs_emsg("rtn ptym_control SDC_PUTB 1, buffer.iov_len=%d\n ", 
				buffer.iov_len);
				if (buffer.iov_len) {
					sbdrop(sb, m->m_len-
					       (n+buffer.iov_len));
					nvsmode &= ~IN_CR;
					goto out;
				}
			}	/* if (nvsmode&IN_CR) else */
		}		/* while (n) */
		sbdrop(sb,m->m_len);
	}			/* while  (sb->sb_cc > 0) */
out:
	nvs->nvs_in_mode &= IN_IS_SCHED; /* save sched flag incase pty set it */
	nvs->nvs_in_mode |= nvsmode;  /* put back possibly changed flags */
	/*
	 * Notify protocol that more space is available in the socket
	 */
	if ((so->so_state & SS_CANTRCVMORE) == 0 &&
	    so->so_proto->pr_flags & PR_WANTRCVD &&
	    so->so_pcb) 
		(*so->so_proto->pr_usrreq)
		    (so, PRU_RCVD,0,0,&local_flags);
	/*
	 * If socket closes at remote end first (not the most common case),
	 * wake up the agent process so it can exit.
	 * Only do this if there is no data left in the socket buffer.
	 * If there is data, then we must be blocked on flow control into
	 * the pty, and the pty will wake us up later.  We want to
	 * process all inbound data before waking up telnetd so it does
	 * not become confused.
	 */
	if (sb->sb_cc <= 0 &&
	    ((so->so_state & (SS_CANTRCVMORE | SS_CANTSENDMORE)) ||
	    so-> so_error)) {
		sb->sb_flags &= ~SB_NVS;
		        nvs_emsg("nvs_input: skt died,wake up,nvsp :%d\n",nvs);
		wakeup((caddr_t)nvs);
	}
	nvs_emsg("Leaving nvs_input\n",0);
}


/*
 * nvs_callout
 * This entrypoint is passed into the pty so that it can be
 * called by the pty when it wants to cause us to do something.
 * The first parameter which the pty will pass us is the pty minor
 * number.  We use this as an index into the nvsj table.  The
 * second parameter is a set of flags which tell us what action
 * to take.  The FREAD flag will tell us that the master pty has data
 * available for us to read and it has not been suspended by a ^S.
 * The FWRITE flag will tell us that flow control has enabled us to
 * do further writes into the master pty.  We assume that we may be
 * called at any priority level.
 * We simply cause ourselves to be called again on netisr via net_timeout.
 * This is to avoid problems where we might have been called at high
 * spl levels by kernel printf.
 */

void
nvs_callout (nvsindex, which)
int nvsindex;
int which;
{
	nvsindex = minor(nvsindex);
	if ((which & FREAD) &&
	    (nvsj[nvsindex].nvs_out_mode & OUT_IS_SCHED) == 0)  {
		nvsj[nvsindex].nvs_out_mode |= OUT_IS_SCHED;
		net_timeout (nvs_output_t, nvsindex, 1);
	}
	if ((which & FWRITE) &&
	    (nvsj[nvsindex].nvs_in_mode & IN_IS_SCHED) == 0)  {
		nvsj[nvsindex].nvs_in_mode |= IN_IS_SCHED;
		net_timeout (nvs_input_t, nvsindex, 0);
	}
}

/*
 * Call nvs_output() after clearing OUT_IS_SCHED flag.
 * This is how nvs_output() is called from net_timout.
 */
void
nvs_output_t(nvsindex)
	int nvsindex;
{
       	nvs_emsg("nvs_output_t,nvsindex %d\n", nvsindex);
	nvsj[nvsindex].nvs_out_mode &= ~OUT_IS_SCHED;
	nvs_output(nvsindex);
}

/*
 * Call nvs_input() after clearing IN_IS_SCHED flag.
 * This is how nvs_input() is called from net_timout.
 */
void
nvs_input_t(nvsindex)
	int nvsindex;
{
	nvsj[nvsindex].nvs_in_mode &= ~IN_IS_SCHED;
	nvs_input(nvsindex);
}
#ifdef NVS_QA
nvs_mlen(mhead)
struct mbuf *mhead;
{
   int len = 0;
   struct mbuf *ptr = mhead;
   while (ptr) {
	len += ptr->m_len;
	ptr= ptr->m_next;
   }
   return(len);
}
/*
 * nvs_printbuf(fmt, val)
 *
 *	fmt = printf-style format specification
 *	val = one 32-bit quantity, to be converted to ASCII representation
 *		according to fmt
 *
 * DESCRIPTION
 *
 * This routine allows one to have kernel printf-style printouts of what's
 * going on, without taking all of the time it takes to get that data out
 * to the terminal.  It also can hold more information;  the buffer is 4K
 * bytes, and it's circular.  When a message is formatted that takes more
 * space than remains in the buffer, the remainder is zeroed (so you don't
 * get misled by old inforamtion) and the new message wraps around to
 * the front.  Obviously, it will be more readable if you make all of your
 * messages the same number of characters, and making them a sub-multiple
 * of 4K is even more efficient.
 *
 * 
 */
#define NVSPBUFSIZE 8192
char nvs_pbuf[NVSPBUFSIZE];
static int char_index = 0;
nvs_printbuf(fmt, val)
char *fmt;
int val;
{
    char msg_buf[80];
    char *ptr;
    int n_chars;

     sprintf(msg_buf, 80, fmt, val);
     n_chars = strlen(msg_buf);
     ptr = &nvs_pbuf[char_index];
     if ((n_chars + char_index) > NVSPBUFSIZE) {
	for (; char_index < NVSPBUFSIZE; char_index++)
	    nvs_pbuf[char_index] = ' ';
	char_index = 0;
	ptr = nvs_pbuf;
     }
     bcopy(msg_buf, ptr, n_chars);
     char_index += n_chars;
     ptr = &nvs_pbuf[char_index];
     *ptr = '^';
}
#endif NVS_QA
