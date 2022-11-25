/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tty_pty.c,v $
 * $Revision: 1.9.84.7 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/02/15 16:22:51 $
 */

/* HPUX_ID: @(#)tty_pty.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/param.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/tty.h"
#include "../h/termios.h"
#include "../h/ttold.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/signal.h"
#include "../h/pty.h"
#include "../h/ld_dvr.h"
#include "../h/malloc.h"

#ifndef tIOC
#define	tIOC	('t' << 8)
#endif

#ifndef TIOC
#define	TIOC	('T' << 8)
#endif

#ifndef fIOC
#define	fIOC	('f' << 8)
#endif

/* driver is in the middle of a last close on this pty */
#define LAST_CLOSE_IN_PROGRESS       WAIT_FOR_RING

/*
 * Pseudo-teletype Driver
 * (Actually two drivers, requiring two entries in 'cdevsw')
 */


extern int npty;
int max_ptys ;		/* npty fluctuates. max_ptys is set to NTPY */
extern struct tty *pt_line[];


extern dev_t cons_out_dev;
extern struct tty *cons_out_tty;

/*
 * pts == /dev/tty[pqr]* slave side
 * ptm == /dev/pty[pqr]* master side
 */

/* Temporary until it can be defined in tty.h */
#define	T_VHANGUP	-1

/* open for slave side of pty */
ptys_open(dev, flag)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Open for slave side of pty.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 23 Apr 90	jph	Fix for DTS INDaa07273.
 | 			Add a flag saying slave is present.  
 |
 | 1 Apr 92     vh      Fix for DTS INDaa10171.
 |			Recode the FNDELAY and FNBLOCK processing to 
 |			return EAGAIN when master is not present and 
 |			an open is made to the slave with O_NONBLOCK.
 |
 | 11 May 92	jlau	Restructured the code to ensure that the open count
 |			is maintained correctly even in the case when the 
 |			open is aborted by a signal.  This could happen when
 |			the slave sleeps waiting for the master's handshake
 |			for trapped request.  This code change was leveraged 
 |			off from a similar effort on the S800 side.
 |
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 | 22 May 92	dmurray	Added call to clear vhangup flag on new open.
 |
 | 15 Oct 93	rpc	Modifications to support dynamic tty/pty_info
 |			allocation rather than static allocation.
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp;
	register struct pty_info *pi;
	register struct proc *pp;
	int mnum = minor(dev);

	int retval ;
	int pty_allocated ;

	if (mnum >= max_ptys)
		return(ENXIO);
	
	pty_allocated = 0 ;

	tp = pt_line[mnum];

	if (tp == (struct tty *) 0) {
		retval = allocate_ptys (mnum) ;
		if (retval != 0)
			return (retval) ;
		pty_allocated = 1 ;	/* allocated on this open */
		tp = pt_line[mnum];

	}
	pi = (struct pty_info *)tp->utility;

	/* Clear vhangup flag. */
	pty_proc( tp, T_VHANGUP, 0 );

	/*
	 * No vhangup check is necessary here because the slave would
	 * already have been opened for the vhangup to have been issued.
	 */
	while (tp->t_state & LAST_CLOSE_IN_PROGRESS)
		if (sleep((caddr_t)&tp->utility, TTIPRI | PCATCH)) {
			/* 
			 *	This "can't happen"... I left it here
			 *	for posterity. (rpc)
			 */
			if (pty_allocated)
				free_ptys (mnum) ;
			return (EINTR);
		}

	if ((tp->t_state & ISOPEN) == 0)
		ttinit(tp);		/* Set up default chars */

	if (!(flag&(FNDELAY|FNBLOCK)))
        	while (!(pi->pty_state & PMPRES)) {
			/* Wait indefinitely for a ptm server to exist */
			tp->t_state |= WOPEN;
			if (sleep((caddr_t)&tp->t_rawq, TTIPRI | PCATCH))  {
				if (pty_allocated)
					free_ptys (mnum) ;
				return (EINTR);
			}
		}
	else if (!(pi->pty_state & PMPRES)) {
		if (pty_allocated)
			free_ptys (mnum) ;
		return(EAGAIN);
	}

	/* Open the line discipline on the pty slave */
	pi->r_info.errno_error =
		(*linesw[tp->t_line].l_open)(dev,tp);

	/* Tell pty master server about this open. */
	if (pi->r_info.errno_error == 0)
		pi->r_info.errno_error = tell_ptm(tp,TIOCOPEN,0,TRAPRW);

	/* Acquire the controlling tty and update open count */
	if ((retval = pi->r_info.errno_error) == 0) {
		pp = u.u_procp;
		if (!(flag&FNOCTTY)
			&& (pp->p_pid == pp->p_pgrp) 
			&& (u.u_procp->p_ttyp == NULL)
			&& (pp->p_pid == pp->p_sid) 
			&& (tp->t_cproc == NULL)
			&& (tp->t_pgrp == 0)) {

			u.u_procp->p_ttyp = tp;
			pp->p_ttyd = tp->t_dev;
			tp->t_pgrp = pp->p_pgrp;
			tp->t_cproc = pp;
		}


		/* Indicate pty slave is present */
		pi->pty_state |= PSPRES; 


		/* Keep track of open count */
		tp->tty_count++;
	}
	else if (pty_allocated)
		free_ptys (mnum) ;
	return (retval) ;
}


tell_ptm(tp,request,arg,mode)
register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Routine executed by pts only that communicates 
 |			between	pty and pts process to handshake 
 |			ioctl/open/close to ptm.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 07 Jun 90	jph	Fix for DTS XXXxxXXXXX.
 | 			When we return from sleeps, restore the u.u_qsave
 |			from the local qsave. 
 |
 | 15 Oct 91	dmurray	Fix for DTS INDaa10234.
 |			Don't clear pi->r_info.request on non-zero setjmp
 |			return when waiting for trap to become available.
 |			The process doing the handshake currently needs
 |			that field.
 |
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 | 22 May 92	dmurray	Added checks after sleeps for vhangup.
 |
 ---------------------------------------------------------------------------*/

	register struct pty_info *pi = (struct pty_info *)tp->utility;
	register sizefield;
	int sigmask_save;

#ifdef OSDEBUG
	if (pi == (struct pty_info *) 0)
		panic ("tell_ptm: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	/* Handshake only if TIOCTRAP or TIOCMONITOR is enabled */
	if (pi->tioctrap || (pi->tiocmonitor && arg!=0)) {
		/*
		** Sleep until trap is available, unless server is
		** already gone.
		*/
		while (pi->trapbusy && (pi->pty_state & PMPRES)) {
			pi->trapwait = TRUE;
			sigmask_save = u.u_procp->p_sigmask;
			if (pi->tiocsigmode == TIOCSIGBLOCK) {
				u.u_procp->p_sigmask |= u.u_procp->p_sigcatch;
			}
			if (sleep((caddr_t)&pi->trapwait, TTIPRI | PCATCH) ||
			    pi->vhangup					 ) {
				/* 
			        ** Slave has recieved a signal - cleanup 
			        ** and return. 
			        */
				pi->trapwait = FALSE;
				wakeup((caddr_t)&pi->trapwait);
				if (pi->tiocsigmode == TIOCSIGABORT)
					u.u_eosys = EOSYS_NORESTART;
				u.u_procp->p_sigmask = sigmask_save;
				return(pi->vhangup ? EBADF : EINTR);
			}
			u.u_procp->p_sigmask = sigmask_save; 
		}

		pi->trapbusy = TRUE;
		/*
		** Trap request (i.e., record pts information).
		*/
		pi->r_info.request = request;
		/* Compute arg get */
		sizefield = pi->r_info.request&IOCSIZE_MASK;
		if (arg && ((pi->r_info.request&IOC_IN) || mode==TRAPTERMIO)) {
			if (sizefield)
				pi->r_info.argget = TIOCARGGET | sizefield;
			else
				pi->r_info.argget =
					TIOCARGGET | (sizeof(int)<<16);
		} else
			pi->r_info.argget = 0;
		/* Compute arg set */
		if (arg && ((pi->r_info.request&IOC_OUT) || mode==TRAPTERMIO)){
			if (sizefield)
				pi->r_info.argset = TIOCARGSET | sizefield;
			else
				pi->r_info.argset =
					TIOCARGSET | (sizeof(int)<<16);
		} else
			pi->r_info.argset = 0;
		pi->r_info.pid = u.u_procp->p_pid;
		pi->r_info.pgrp = u.u_procp->p_pgrp;
		/* The error may already be set on these. */
		if (pi->r_info.request!=TIOCOPEN &&
			pi->r_info.request!=TIOCCLOSE && mode!=TRAPTERMIO)
				pi->r_info.errno_error = 0;
		/* The return value may already be set on this. */
		if (mode!=TRAPTERMIO)
			pi->r_info.return_value = 0;
		/* Copy in structure if something to copy in */
		if (pi->r_info.argget)
			bcopy(arg, pi->ioctl_buf,
				((pi->r_info.argget&IOCSIZE_MASK)>>16));
		/* Wakeup selects waiting on this */
		ptyselwakeup(&pi->pty_sele);
		/*
		** if master is sleeping until something is trapped, 
		** wake him up 
		*/
		wakeup((caddr_t)&pi->trapbusy);

		/* if this is a close don't wait for the handshake */
		if (pi->r_info.request == TIOCCLOSE)
			return(pi->r_info.errno_error);
		/*
		** Sleep until master side handshakes trapped request,
		** unless server is already gone.
		*/
		pi->trapcomplete = TRUE;
		while (pi->trapcomplete && (pi->pty_state & PMPRES)) {

			sigmask_save = u.u_procp->p_sigmask;
			if (pi->tiocsigmode == TIOCSIGBLOCK) 
				u.u_procp->p_sigmask |= u.u_procp->p_sigcatch;
			
			if (sleep((caddr_t)&pi->trapcomplete,TTIPRI | PCATCH) ||
			    pi->vhangup					    ) {
				/* 
				** Slave has recieved a signal cleanup 
				** and return. 
				*/
				pi->trapcomplete = FALSE;
				pi->r_info.request = 0;
				pi->trapbusy = FALSE;
				if (pi->trapwait) {
					pi->trapwait = FALSE;
					wakeup((caddr_t)&pi->trapwait);
				}
				if (pi->tiocsigmode == TIOCSIGABORT)
					u.u_eosys = EOSYS_NORESTART;
				u.u_procp->p_sigmask = sigmask_save; 
				return(pi->vhangup ? EBADF : EINTR);
			}
			u.u_procp->p_sigmask = sigmask_save; 
		}

		if (request == TIOCOPEN)
			pi->r_info.errno_error = 0;
		/*
		** Complete pts request.
		*/
		/* Copy out structure if something to copy out.
		 * WARNING! Do Not Remove the check for arg!
		 * Must recheck value of arg here because of a
		 * possible race condition between an open waiting
		 * to be handshaked and a ioctl waiting for the trap.
		 * If the master gets closed while this condition
		 * exist and the ioctl process runs before the open
		 * process the system will panic on the bcopy.	This
		 * will happen because the ioctl will reset the value
		 * of argset underneath the open.
		 */
		if (pi->r_info.argset && arg)
			bcopy(pi->ioctl_buf, arg,
				((pi->r_info.argset&IOCSIZE_MASK)>>16));

		pi->trapbusy = FALSE;
		if (pi->trapwait) {
			pi->trapwait = FALSE;
			wakeup((caddr_t)&pi->trapwait);
		}
	}
	return(pi->r_info.errno_error);
}


/* close for slave side of pty */
ptys_close(dev)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Close for slave side of pty.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 23 Apr 90	jph	Fix for DTS INDaa07273.
 | 			Add a check for presence of master.  Clear flag 
 | 			saying slave is present.
 |
 | 29 May 92	marcel  Added call to clear_ctty_from_session to clear any
 |			dangling pointers to this devices as a ctty. Fix for
 |			DTS INDaa08508 and part of DTS INDaa11774
 |
 | 15 Oct 93	rpc	9.03 changes for dynamic tty and pty_info allocation.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;

	/*
	 *	Supposedly this can happen due to a problem in the system.
	 *	It was here as a "bandaid" in the previous implementation,
	 *	so I left it here. (rpc)
	 */
	if (tp == (struct tty *) 0)
		return ;
	 
	if (!(tp->t_state&ISOPEN) && (tp->tty_count == 0))
		panic ("ptys_close: Invalid state/count") ;


	/* if this isn't the last close then get out of here */
	if (tp->tty_count > 1) {
		tp->tty_count--;
		return;
	}
	tp->t_state |= LAST_CLOSE_IN_PROGRESS;
	tp->tty_count = 0;
	
	if (CONTROL_PROC(tp)) {
		clear_ctty_from_session(CONTROL_PROC(tp));
	}

	tp->t_pgrp = 0;         /* not a controlling terminal anymore */
	tp->t_cproc = NULL;

	if (!(pi->pty_state & PMPRES)) {
		(*linesw[tp->t_line].l_close)(tp);
	}

	pi->pty_state &= ~PSPRES;
	pi->r_info.errno_error = u.u_error;

	/* Tell ptm about this close if it wants to know */
	u.u_error = tell_ptm(tp,TIOCCLOSE,0,TRAPRW);
	tp->t_state &= ~LAST_CLOSE_IN_PROGRESS;
	wakeup((caddr_t)&tp->utility);
	if (!(pi->pty_state & PMPRES)) {
		free_ptys (minor(dev)) ;
	}


}


/* read for slave side of pty */
ptys_read(dev,uio)
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Read for slave side of pty.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 |
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 | 22 May 92	dmurray	Added checks after sleeps for vhangup.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
	register struct clist *tq;
	int retval = 0;

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptys_read: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptys_read: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	/* If the master has gone away, return 0.  This should stop hangs */
	if((tp->t_state & CARR_ON) == 0) {
		if (tp->t_cproc != NULL)
			/* SIGHUP controlling process */
			psignal(tp->t_cproc, SIGHUP);	
		goto return_retval;
	}

	if (pi->tioctty)
		if (pi->tiocremote) {
			/* sleep until there is somthing to read */
			tq = &tp->t_rawq;
			while (tq->c_cc == 0 && (pi->pty_state & PMPRES)) {
				wakeup((caddr_t)&pi->tioctty);
				if (uio->uio_fpflags&FNBLOCK)
					return(EAGAIN);
				if (uio->uio_fpflags&FNDELAY)
					return(0);
				if (sleep((caddr_t)&pi->pktbyte, TTIPRI | PCATCH)) {
					retval = EINTR;
					goto return_retval;
				}
				if (pi->vhangup) {
					retval = EBADF;
					goto return_retval;
				}
			}
			while (tq->c_cc > 1 && uio->uio_resid!=0 && retval==0) 
{
				register c;
				if ((c = getc(tq)) < 0)
					break;
				retval = ureadc(c, uio); 
			}
			/* throw away the last char in the buffer */
			if (tq->c_cc == 1)
				getc(tq); 
			if (tq->c_cc)
				return(retval);
			/* the entire record has been read - wakeup master */
			wakeup((caddr_t)&pi->tioctty);
		} else {
			retval = ttread(tp,uio);
			/* wakeup master */
			wakeup((caddr_t)&pi->tioctty);
	} else { /* Bypass line discipline */

		/* sleep until there is somthing to read */
		while (tp->t_rawq.c_cc == 0) {
			wakeup((caddr_t)&pi->tioctty);
			if (uio->uio_fpflags&FNBLOCK)
				return(EAGAIN);
			if (uio->uio_fpflags&FNDELAY)
				return(0);
			if (sleep((caddr_t)&pi->pktbyte, TTIPRI | PCATCH)) {
				retval = EINTR;
				goto return_retval;
			}
			if (pi->vhangup) {
				retval = EBADF;
				goto return_retval;
			}
		}
		tq = &tp->t_rawq;
		while (uio->uio_resid!=0 && retval==0) {
			if (uio->uio_resid >= CBSIZE) {
				register n;
				register struct cblock *cp;
				register char *bufp;
				cp = getcb(tq);
				if (cp == NULL)
					break;
				n = min(uio->uio_resid, cp->c_last - cp->c_first);
				retval = uiomove(&cp->c_data[cp->c_first], n, UIO_READ, uio);
				putcf(cp);
			} else {
				register c;
				if ((c = getc(tq)) < 0)
					break;
				retval = ureadc(c, uio); 
			}
		}
		/* if someone is waiting for room to write then wake them up */
		wakeup((caddr_t)&pi->tioctty);
		if (tp->t_rawq.c_cc == 0)
			tp->t_delct = 0;
	}
	ptyselwakeup(&pi->pty_selw);
return_retval:
	return(retval);
}

ptys_putchar(c)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Print a character - used by kernel printf.
 |			Note: This function is only used when 
 |			Console output has been redirected via
 |			TIOCCONS.
 |
 | MODIFICATION HISTORY
 |
 | Date    User		Changes
 | 18 Jul  garth	Fix for SR 4700-802116 (aka, DTS IND05536).
 |			Removed code that previously went through 
 |			ptys_write and added code to output character
 | 			directly to output queue.  Before fix, panics
 |			would occur in ptys_write because it would
 |			try to sleep, occasionally, when called off the
 |			ICS.
 |
 ---------------------------------------------------------------------------*/
	extern struct tty *cons_tty;
	register struct tty *tp = cons_tty;
	struct uio uio;
	struct iovec iovec;
	char charbuf[1];

	if (cons_out_dev)
		tp = cons_out_tty;	/* Someone has grabbed the console */

	/* Put character directly on output
	 * queue and start output.
	 */
	ttxput(tp, c, 0);	
	(*tp->t_proc)(tp, T_OUTPUT); 	 
				
}

/* write for slave side of pty */
ptys_write(dev,uio)
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Write for slave side of PTY. 
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 18 Apr 90	jph	Fix for DTS INDaa07273.
 |			Added interlock mechanism to driver to prevent
 |			slave from writing unless master is present. 
 |
 |  5 Nov 90  	dmurray	Fix for DTS INDaa07424. (aka SR 1650-135921)
 |                    	Changed code so that a new cblock isn't taken from
 |                    	the free list every time this procedure is entered
 |                    	in non-tty mode.  The worst case is where one byte
 |                    	writes are being done and the cblocks are exhausted
 |                    	before TTYHOG is exceeded in the out queue.
 |
 |  6 Dec 91	dmurray	Fix for INDaa10655.  Protect clist manipulation
 |			with spl.
 |
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 | 22 May 92	dmurray	Added checks after sleeps for vhangup.
 |
 | 12 Jun 92	marcel	Added check to see if master leaves  while
 |			we are sleeping on high_water, in not tioctty mode
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
	int error = 0;
	extern ttwrite();

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptys_write: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptys_write: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	if (!(pi->pty_state & PMPRES)) {
		error = EIO;
		return(error);
		}
	if (pi->tioctty)
		error = ttwrite(tp,uio);
	else { /* Bypass line discipline */
		register cc;
		register n;
		register struct cblock *cb;
		int x;

		while (uio->uio_resid) {
			if ((tp->t_state&CARR_ON)==0)
				return(0);
			/* insure that the buffer is not zero */
			/* length and is contigious. */
			cc = uio->uio_iov->iov_len;
			if (cc == 0) { 
				uio->uio_iovcnt--;
				uio->uio_iov++;
				if (uio->uio_iovcnt < 0)
					panic("ptyswrite");
				continue;
			}

			/* sleep until there is room in the buffer to write */
			while (tp->t_outq.c_cc >= TTYHOG) {
				/* if someone is waiting for data to read then 
wake them up */
				x = splsx(tp->t_int_lvl);
				(*tp->t_proc)(tp, T_OUTPUT);
				splsx(x);

				if ((tp->t_state&CARR_ON)==0)
					return(0);

				tp->t_state |= OASLP;
				if (sleep((caddr_t)&tp->t_outq, TTOPRI | PCATCH))
					return(EINTR);
				if (pi->vhangup)
					return(EBADF);
			}
			/*just in case the master is now gone*/
			if ((tp->t_state&CARR_ON)==0)
					return(0);


			/*
			 * If the last cblock in the out queue clist is full
			 * or the clist is empty, then lets go get another one.
			 * Otherwise use the one that is there.
			 */
			x = splsx(tp->t_int_lvl);
			if ((tp->t_outq.c_cl == NULL) ||
			    (tp->t_outq.c_cl->c_last == CBSIZE)) {
				while ((cb = getcf()) == NULL) {
					if (sleep((caddr_t)&cfreelist,
							     TTIPRI | PCATCH))
						return(EINTR);
					if (pi->vhangup)
						return(EBADF);
					/*just in case the master is gone*/
					if ((tp->t_state&CARR_ON)==0)
					  return(0);

				}
				cb->c_last = 0;
			} else
				cb = tp->t_outq.c_cl;

			n = min(cc, CBSIZE - cb->c_last);

			/*
			 * Move the user data into the cblock and update
			 * the indices and pointers.
			 */
			if (error = uiomove(&cb->c_data[cb->c_last], n,
					    UIO_WRITE, uio)) {
				if (cb->c_last == 0)     /* If we got a new */
					putcf(cb);       /* cblock then put */
				splsx(x);		 /* it back.        */
				break;
			}

			if (cb->c_last == 0) {
				cb->c_last = n;
				putcb(cb, &tp->t_outq);
			} else {
				cb->c_last += n;
				tp->t_outq.c_cc += n;
			}
			tp->t_col += n;
			splsx(x);

		} /* while */

		/* if someone is waiting for data to read then wake them up */
		x = splsx(tp->t_int_lvl);
		(*tp->t_proc)(tp, T_OUTPUT);
		splsx(x);
	} /* else */
	return(error);
}


/* open for master side of pty */
ptym_open(dev, flag)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Open for master side of pty.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 23 Apr 90	jph	Fix for DTS INDaa07273.
 | 			Check for presence of slave along with checking
 |			for presence of master.  Add a flag saying master 
 |			is present.  
 |
 | 01 Apr 92    vh      Fix for DTS INDaa11646.
 |			Clear WOPEN after waking up slave pty waiting on
 |			master open.
 |
 | 15 Oct 93	rpc	Changes for dynamically allocating tty and pty_info
 |			structs.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp;
	register struct pty_info *pi;

	int retval ;

	dev = minor(dev);

	/*
	 *	9.03 change. npty fluctuates now. Must use max_ptys
	 */
	if (dev >= max_ptys)
		return(ENXIO);

	tp = pt_line[dev];


	if (tp == (struct tty *) 0) {
		retval = allocate_ptys (dev) ;
		if (retval != 0)
			return (retval) ;
		tp = pt_line[dev];
	}
	/*
	 *	9.03. tp->utility gets initialized in allocate_ptys
	 */

	pi = (struct pty_info *)tp->utility;

	/* Interlock check, if either master or slave exists then return
	 * EBUSY
	 */
	if ((pi->pty_state & PMPRES) || (pi->pty_state & PSPRES)) {
		/*
		 *	9.03. It is "impossible" to be in this state
		 *	and have allocated the tty/pty_info structs here. No
		 *	need to free them in this case. (rpc)
		 */

		return(EBUSY);
	}

	if (tp->t_state & WOPEN)
		wakeup((caddr_t)&tp->t_rawq);
	tp->t_state &= ~WOPEN;
	tp->t_state |= CARR_ON;
	pi_init (pi) ;
	pi->pty_state |= PMPRES; 
	pi->u_procp = u.u_procp;
	return(0);
}


/* close for master side of pty */
ptym_close(dev)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Close for master side of pty.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 23 Apr 90	jph	Fix for DTS INDaa07273.
 | 			Add a check for presence of slave and do a line 
 |			discipline close if there is no slave present.   
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_close: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_close: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	tp->t_state &= ~CARR_ON;	/* virtual carrier gone */
	pi->callout = NULL;
	/* Need to flush this channel also */
	ttyflush(tp, FWRITE|FREAD);
	/* Don't let any one be stuck. */
	tp->t_state &= ~(BUSY|TIMEOUT);
	wakeup((caddr_t)&pi->tiocpkt);
	wakeup((caddr_t)&pi->pktbyte);
	if (pi->trapwait)
		wakeup((caddr_t)&pi->trapwait);
	if (pi->trapcomplete)
		wakeup((caddr_t)&pi->trapcomplete);
	if ((tp->t_state & ISOPEN) && (tp->t_cproc != NULL))
		/* SIGHUP controlling process */
		psignal(tp->t_cproc, SIGHUP);	


	if (!(pi->pty_state & PSPRES)) {
		(*linesw[tp->t_line].l_close)(tp);
		free_ptys (minor(dev)) ;
	}
	else
		pi->pty_state &= ~PMPRES;

}


/* read for master side of pty */
ptym_read(dev,uio)
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Read for Master side of PTY.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 23 Feb 89  garth	Fix for SR 4700-683599 (aka, DTS INDaa03140).
 |			Added checks for stopped/restarted input.  When
 |			input is stopped a CSTOP character is returned.
 |			When input is restarted a CSTART character is
 |			returned.
 |
 | 23 Feb 89  garth	Fix for SR 4700-679142 (aka, DTS INDaa03071).
 |			Added check for stopped output.  
 |	
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
	register struct clist *tq = &tp->t_outq;
	register c;
	int error = 0;

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_read: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_read: pty_info list is corrupt") ;
#endif /* OSDEBUG */


	if (tp->t_state&BUSY) {
		tp->t_state &= ~BUSY;
		wakeup((caddr_t)&tp->t_state);
	}
	if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
		tp->t_state &= ~TTIOW;
		wakeup((caddr_t)&tp->t_oflag);
	}
	if ((tp->t_state&OASLP) && tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD])
 {
		tp->t_state &= ~OASLP;
		tp->t_int_flag &= ~PEND_TXINT;
		wakeup((caddr_t)&tp->t_outq);
	}
	/* Stopped input? */
	if (pi->pty_state & PBLOCK ) {
		error = ureadc(CSTOP, uio);
		if (!error)
			/* stop character sent */
			pi->pty_state &= ~PBLOCK;
		return(error);
	}
	/* Restarted input? */
	if (pi->pty_state & PUNBLOCK) {
		error = ureadc(CSTART, uio);
		if (!error)
			/* start character sent */
			pi->pty_state &= ~PUNBLOCK;
		return(error);
	}
	while ((tq->c_cc == 0) && ((!pi->tiocpkt) ||
		(pi->tiocpkt && (!pi->sendpktbyte)))) {
		wakeup((caddr_t)&pi->tiocpkt);
		if (uio->uio_fpflags&FNBLOCK)
			return(EAGAIN);
		if (uio->uio_fpflags&FNDELAY)
			return(0);
		pi->ptmsleep = TRUE;
		if (sleep((caddr_t)&pi->ptmsleep, TTIPRI | PCATCH)) {
			pi->ptmsleep = FALSE;
			return(EINTR);
		}
	}
	if (pi->tiocpkt && pi->tioctty) {
		if (pi->sendpktbyte) {
			pi->sendpktbyte = FALSE;
			c = pi->pktbyte;
			pi->pktbyte = 0;
			return(ureadc(c,uio));
		} else
			error = ureadc(0, uio);
	}
	while (tp->t_state & TTSTOP) {
		wakeup((caddr_t)&pi->tiocpkt);
		return(error);
	}
	while (uio->uio_resid!=0 && error==0) {
		if (uio->uio_resid >= CBSIZE) {
			register struct cblock *cp;
			register char *bufp;

			if ((cp = getcb(tq)) == NULL)
				break;
			error = uiomove(&cp->c_data[cp->c_first], 
				       cp->c_last - cp->c_first, UIO_READ, uio);
			putcf(cp);

		} else {
			if ((c = getc(tq)) < 0)
				break;
			error = ureadc(c, uio);
		}
	}
	/* if someone is waiting for room to write then wake them up */
	wakeup((caddr_t)&pi->tiocpkt);
	if (tp->t_wsel) {
		selwakeup(tp->t_wsel, tp->t_state & WCOLL);
		tp->t_wsel = 0;
		tp->t_state &= ~WCOLL;
	}
	if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
		tp->t_state &= ~TTIOW;
		wakeup((caddr_t)&tp->t_oflag);
	}
	if ((tp->t_state&OASLP) && tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD]) {
		tp->t_state &= ~OASLP;
		wakeup((caddr_t)&tp->t_outq);
	}
	if (tp->t_state&BUSY) {
		tp->t_state &= ~BUSY;
		wakeup((caddr_t)&tp->t_state);
	}
	return(error);
}


/* write for master side of pty */
ptym_write(dev,uio)
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Write for Master side of PTY.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 01 Nov 88  garth	Fix for SR 4700-691964 (aka, DTS INDaa03213).
 |			Added call to ptyselwakeup when remote mode is
 |			enabled and when termio(7) processing is disabled.
 |			Before fix, slave read selects were not being
 |			woken up when writes completed in remote mode or
 |			when writes completed while termio(7) processing
 |			was disabled.
 |
 | 31 May 89  garth	Fix for INDaa05064.  Removed code that blocks
 |			if rawq is about to exceed TTYHOG.  Only removed
 |			where line discipline processes the data. 
 |
 | 15 Jan 90  garth     Fix for SR 4700-796698 (aka, DTS INDaa05356).
 |			Added check for stopped input (TTXOFF) when in TIOCTTY
 |			mode.  If input has been stopped then no more data
 |			can be written until input has been restarted.
 |
 |  5 Nov 90  dmurray 	Fix for DTS INDaa07424. (aka SR 1650-135921)
 |                    	Changed code so that a new cblock isn't taken from
 |                    	the free list every time this procedure is entered
 |                    	in non-tty mode.  The worst case is where one byte
 |                    	writes are being done and the cblocks are exhausted
 |                    	before TTYHOG is exceeded in the raw queue.
 |
 |  8 Nov 90  dmurray 	Fix for DTS INDaa08340.
 |                    	Added call to ptyselwakeup() instead of checking
 |                    	tp->t_rsel and doing a selwakeup().  tp->t_rsel
 |                   	doesn't get set so the process was never being woken up.
 |
 |  6 Dec 91  dmurray	Fix for INDaa10655.  Protect clist manipulation
 |			with spl.
 | 
 | 13 May 92	jlau	Added pcatch to sleep.
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *) tp->utility;
	register cc, error = 0;
	unsigned char obuf[CBSIZE];
	register unsigned char *c;
	register n;
	register struct cblock *cp;
	extern ttin();
#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_write: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_write: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	if (pi->tioctty)
		if (pi->tiocremote) {
			/* sleep until slave has read the last record */
			while (tp->t_rawq.c_cc) {
				wakeup((caddr_t)&pi->pktbyte);
				if (sleep((caddr_t)&pi->tioctty, TTIPRI | PCATCH))
					return(EINTR);
			}
			while (uio->uio_resid) {
				cc = uio->uio_iov->iov_len;
				n = min(cc, CBSIZE);
				/* sleep if this write will overflow the buffer
*/
				if ((tp->t_rawq.c_cc + n) > (TTYHOG - 1)) {
					putc(0, &tp->t_rawq);
					wakeup((caddr_t)&pi->pktbyte);
					if (sleep((caddr_t)&pi->tioctty,
							TTIPRI | PCATCH))
						return(EINTR);
					continue;
				}
				if ((cp = getcf()) == NULL)
					break;
				if ((error = uiomove(cp->c_data, n, UIO_WRITE,  uio))) {
					putcf(cp);
					break;
				}
				cp->c_last = n;
				putcb(cp, &tp->t_rawq);
			}
			putc(0, &tp->t_rawq);
			wakeup((caddr_t)&pi->pktbyte);
			ptyselwakeup(&pi->pty_selr);
		} else {
			while (uio->uio_resid) {
				/* If flow control is turned on then don't
			 	 * write any more data.
				 */
				if (tp->t_state & TTXOFF)
					break;
				c = obuf;
				cc = uio->uio_iov->iov_len;
				n = min(cc, CBSIZE);
				if ((error = uiomove(obuf, n, UIO_WRITE, uio)))
					break;
				while (n--)
					ttin(tp, *c++, 0, n);
			}
			wakeup((caddr_t)&tp->t_rawq);
			wakeup((caddr_t)&pi->pktbyte);
		}
	else {  /* bypass line discipline */

		int x;

		while (uio->uio_resid) {
			cc = uio->uio_iov->iov_len;

			/* sleep if this write will overflow the buffer */
			x = splsx(tp->t_int_lvl);
			while (tp->t_rawq.c_cc >= TTYHOG) {
				/* if a reader is waiting for data then wake he r up */
				wakeup((caddr_t)&pi->pktbyte);
				if (sleep((caddr_t)&pi->tioctty, TTIPRI | PCATCH))
					return(EINTR);
			}

			/*
			 * If the last cblock in the out queue clist is full
			 * or the clist is empty, then lets go get another one.
			 * Otherwise use the one that is there.
			 */
			if ((tp->t_rawq.c_cl == NULL) ||
                            (tp->t_rawq.c_cl->c_last == CBSIZE)) {
                                while ((cp = getcf()) == NULL)
                                        if (sleep((caddr_t)&cfreelist,
							      TTIPRI | PCATCH))
						return(EINTR);
                                cp->c_last = 0;
                        } else
                                cp = tp->t_rawq.c_cl;

			n = min(cc, CBSIZE - cp->c_last);

			/*
			 * Move the user data into the cblock and update
			 * the indices and pointers.
			 */
			if (error = uiomove(&cp->c_data[cp->c_last], n,
                                             UIO_WRITE, uio)) {
                                if (cp->c_last == 0)     /* If we got a new */
                                        putcf(cp);       /* cblock then put */
				splsx(x);		 /* it back.        */
                                break;
			}

			if (cp->c_last == 0) {
                                cp->c_last = n;
                                putcb(cp, &tp->t_rawq);
                        } else {
                                cp->c_last += n;
                                tp->t_rawq.c_cc += n;
                        }

			splsx(x);
			/* if a reader is waiting for data then wake her up */
			wakeup((caddr_t)&pi->pktbyte);

			/* wakeup slaves blocked on select */
			ptyselwakeup(&pi->pty_selr);

		} /* while */
	} /* else */
	return(error);
}


/* ioctl for slave side of pty */
ptys_ioctl(dev, cmd, arg, mode)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Ioctl for slave side of PTY.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 01 Oct 89	garth	Added another check (fIOC) for the FIO* ioctls.  
 |			Before fix, none of the FIO* ioctls were being
 |			sent to the line discipline to be processed.
 |
 | 23 Apr 90	jph	Fix for DTS INDaa07273. 
 |			Added check for presence of master.      
 |
 | 12 Nov 90	dmurray	Fix for DTS INDaa08338.
 |			ptys_ioctl() was not returning EINVAL for ioctls in
 |	 		non-TTY mode which should result in EINVAL.
 |			Uncommented code in the default case of the
 |			switch (cmd&0x0ff00) to set return code to EINVAL which
 |			according to the comments should have been uncommented
 |			in 6.2.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
	int err;
	extern ttiocom();
	unsigned char do_stop = (unsigned char) ((tp->t_iflag & IXON) >> 8);

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptys_ioctl: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptys_ioctl: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	if (!(pi->pty_state & PMPRES)) {
		return(EIO);
	}

	if (cmd == TIOCCLOSE || cmd == TIOCOPEN)
		return(EINVAL);
	switch (cmd&0x0ff00) {
	case tIOC:
	case fIOC:
	case TIOC:
	case LDIOC:
		if (pi->tioctty) {
			pi->r_info.errno_error = 0;
			if ((err=ttiocom(tp, cmd, arg, mode))!=-1)
				pi->r_info.errno_error = err;
			if (pi->tiocmonitor) {
				int save_errno;
				int save_retval;

				/* Master should not be able to change return */
				/* value and errno for tty(4) ioctl's if      */
				/* pi->tioctty and pi->tiocmonitor are true   */

				save_errno  = pi->r_info.errno_error;
				save_retval = pi->r_info.return_value = u.u_r.r_val1;
				(void) tell_ptm(tp,cmd,arg,TRAPTERMIO);
				u.u_r.r_val1 = pi->r_info.return_value = save_retval;
				pi->r_info.errno_error = save_errno;
			}
			break;
		}
		/* fall through */
	default:
		/* Pass Non-TTY(4) Ioctl's thru */

		pi->r_info.errno_error = EINVAL;
		pi->r_info.return_value = -1;

		if (!tell_ptm(tp,cmd,arg,TRAPRW))
			u.u_r.r_val1 = pi->r_info.return_value;
		break;
	}

	if (pi->tiocpkt && pi->tioctty) {
		if (do_stop) {
			if ((tp->t_iflag & IXON) == 0) {
				pi->pktbyte |= TIOCPKT_NOSTOP;
				pi->pktbyte &= ~TIOCPKT_DOSTOP;
				pi->sendpktbyte = TRUE;
				if (pi->ptmsleep) {
					pi->ptmsleep = FALSE;
					wakeup((caddr_t)&pi->ptmsleep);
				}
				ptyselwakeup(&pi->pty_selr);
			}
		} else {
			if (tp->t_iflag & IXON) {
				pi->pktbyte |= TIOCPKT_DOSTOP;
				pi->pktbyte &= ~TIOCPKT_NOSTOP;
				pi->sendpktbyte = TRUE;
				if (pi->ptmsleep) {
					pi->ptmsleep = FALSE;
					wakeup((caddr_t)&pi->ptmsleep);
				}
				ptyselwakeup(&pi->pty_selr);
			}
		}
	}

	return(pi->r_info.errno_error);
}


/* ioctl for master side of pty */
ptym_ioctl(dev, cmd, arg, mode)
{
/*---------------------------------------------------------------------------
 | FUNCTION -           Ioctl for Master side of PTY.
 |
 | MODIFICATION HISTORY
 |
 | Date       User      Changes
 |  8 Oct 90  marcel    Fix for SAPEX Hotsite (SR XXXXX-XXXXX, aka DTS
 |			 INDaa08339)
 |                      Added another check (fIOC) for the FIO* ioctls.
 |                      Before fix, none of the FIO* ioctls were being
 |                      sent to the line discipline to be processed.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
	register int *bool;
	register struct request_info *r;
	int err;
	extern ttin();

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_ioctl: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_ioctl: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	switch (cmd) {
	case TIOCSIGMODE:
		bool = (int *)arg;
		if (*bool < 0 || *bool > 2)
			return(EINVAL);
		pi->tiocsigmode = *bool;
		break;
	case TIOCSIGSEND:
		bool = (int *)arg;
		/* is this a valid signal? */
		if (*bool < 1 || *bool >= NSIG)
			return(EINVAL);
		/* Send signal to tty process group */
		gsignal(tp->t_pgrp, *bool);
		break;
	case TIOCBREAK:
		if (!pi->tioctty)
			return(EINVAL);
		/* Send Break */
		ttin(tp, FRERROR, 0, 0);
		break;
	case TIOCSTOP:
		if (!pi->tioctty)
			return(EINVAL);
		/* Stop data flowing from the pts to ptm. */
		(*tp->t_proc)(tp, T_SUSPEND);
		break;
	case TIOCSTART:
		if (!pi->tioctty)
			return(EINVAL);
		/* Restart data flowing from the pts to ptm. */
		(*tp->t_proc)(tp, T_RESUME);
		break;
	case TIOCPKT:
		/* Enable/Disable TIOCPKT Mode */
		bool = (int *)arg;
		pi->tiocpkt = (*bool ? TRUE : FALSE);
		break;
	case TIOCREMOTE:
		/* Enable/Disable remote processing */
		bool = (int *)arg;
		if (pi->tiocremote != *bool)
			ttyflush(tp, FWRITE|FREAD);
		pi->tiocremote = (*bool ? TRUE : FALSE);
		break;
	case TIOCTTY:
		/* Enable/Disable termio processing */
		bool = (int *)arg;
		if (pi->tioctty != *bool)
			ttyflush(tp, FWRITE|FREAD);
		pi->tioctty = (*bool ? TRUE : FALSE);
		break;
	case TIOCTRAPSTATUS:
		/* Indicate if an open/close/ioctl has been trapped */
		bool = (int *)arg;
		*bool = (pi->r_info.request ? TRUE : FALSE);
		break;
	case TIOCTRAP:
		/* Enable/Disable Trapping of all pts open/close/ioctls */
		bool = (int *)arg;
		pi->tioctrap = (*bool ? TRUE : FALSE);
		break;
	case TIOCMONITOR:
		/* Enable/Disable Monitoring of all pts termio ioctls */
		bool = (int *)arg;
		pi->tiocmonitor = (*bool ? TRUE : FALSE);
		break;
	case TIOCREQGET:
		/* sleep until something is trapped */
		while (pi->trapbusy == FALSE)
			if (sleep((caddr_t)&pi->trapbusy, TTIPRI | PCATCH))
				return(EINTR);
		/* Read Trapped open/close/ioctl header information */
		bcopy(&pi->r_info, arg, sizeof(pi->r_info));
		break;
	case TIOCREQCHECK:
		if (pi->trapbusy == FALSE) {
			return(EINVAL);
		}
		/* Read Trapped open/close/ioctl header information */
		bcopy(&pi->r_info, arg, sizeof(pi->r_info));
		break;
	case TIOCREQSET:
		r = (struct request_info *)arg;
		if (pi->r_info.request == r->request && pi->trapbusy) {
			/* server may not force an error on open or close */
			if (r->request != TIOCCLOSE && r->request != TIOCOPEN) {
				pi->r_info.errno_error = r->errno_error;
				pi->r_info.return_value = r->return_value;
			} else 
				pi->r_info.errno_error = 0;
			/* Indicate that there is no longer any ptm
			   ioctl pending. */
			pi->r_info.request = 0;
			/* if this was a close release the trap */
			if (r->request == TIOCCLOSE) {
				pi->trapbusy = FALSE;
				if (pi->trapwait) {
					pi->trapwait = FALSE;
					wakeup((caddr_t)&pi->trapwait);
				}
				return(0);
			}
			/* Wakeup pts side */
			if (pi->trapcomplete) {
				pi->trapcomplete = FALSE;
				wakeup((caddr_t)&pi->trapcomplete);
			}
		} else
			return(EINVAL);
		break;
	default:
		switch (cmd&(~IOCSIZE_MASK)) {
		case TIOCARGGET:
			if (!pi->r_info.request)
				return(EINVAL);
			bcopy(pi->ioctl_buf, arg, (cmd&IOCSIZE_MASK)>>16);
			return(0);
		case TIOCARGSET:
			if (!pi->r_info.request)
				return(EINVAL);
			bcopy(arg, pi->ioctl_buf, (cmd&IOCSIZE_MASK)>>16);
			return(0);
		}
		switch (cmd&0x0ff00) {
		case tIOC:
		case fIOC:
		case TIOC:
		case LDIOC:
			if (pi->tioctty) {
				if ((err=ttiocom(tp, cmd, arg, mode))==-1)
					err = 0;
				return(err);
			}
		default:
			return(EINVAL);
		}
	}
		if (pi->r_info.request == TIOCOPEN)
		if (pi->r_info.errno_error)
			pi->r_info.errno_error = 0;
	return(0);
}


/* Line discipline proc routine */
pty_proc(tp, cmd, data)

register struct tty	*	tp;
int				cmd;
int				data;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Pty driver control routine.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 23 Feb 89	garth	Fix for SR 4700-683599 (aka, DTS INDaa03140).
 |			Changed ptyselwakeup in T_BLOCK and T_UNBLOCK to
 |			wakeup write selects instead of read selects.  Also,
 |			removed packet mode information.
 |
 | 23 Feb 89	garth	Fix for SR 4700-683599 (aka, DTS INDaa03140).
 |			Added code to set pty state when input is
 |			stopped or restarted.
 |
 | 11 Apr 91	dmurray	Fix for DTS INDaa09308.
 |			Changed the T_UNBLOCK code so that it makes the
 |			call to the kernel telnet callout routine after
 |			everything else is done.  Previously we made the
 |			call before setting all the flags and the telnet
 |			code was trying to do an SDC_PUTB before the XOFF
 |			bit was clear.  This caused a hang in the inbound
 |			data flow.
 |
 | 22 May 92	dmurray	Added T_VHANGUP.
 |
 ----------------------------------------------------------------------*/

	register struct pty_info *pi = (struct pty_info *)tp->utility;
	register void (*callout)() = pi->callout;
	char didwakeup = FALSE;

	switch(cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
		goto start;

	case T_WFLUSH:
		if (pi->tiocpkt && pi->tioctty)
			pi->pktbyte |= TIOCPKT_FLUSHWRITE;
	case T_RESUME:
		if (pi->tiocpkt && pi->tioctty) {
			pi->pktbyte |= TIOCPKT_START;
			pi->pktbyte &= ~TIOCPKT_STOP;
			pi->sendpktbyte = TRUE;
			if (pi->ptmsleep) {
				pi->ptmsleep = FALSE;
				wakeup((caddr_t)&pi->ptmsleep);
			}
			ptyselwakeup(&pi->pty_selr);
			didwakeup = TRUE;
		}
		tp->t_state &= ~TTSTOP;
		goto start;

	case T_OUTPUT:
	start:
		if (callout)
			(*callout)(minor(tp->t_dev), FREAD);
		/* If this is a ptm read process don't do this */
		if (tp->t_state&(TIMEOUT|TTSTOP)) /* | BUSY ??? */
			break;
		if (tp->t_outq.c_cc) {
			if (didwakeup == FALSE) {
				if (pi->ptmsleep) {
					pi->ptmsleep = FALSE;
					wakeup((caddr_t)&pi->ptmsleep);
				}
				ptyselwakeup(&pi->pty_selr);
			}
			tp->t_state |= BUSY;
			tp->t_int_flag |= PEND_TXINT;
		} else {
			tp->t_state &= ~BUSY;
			tp->t_int_flag &= ~PEND_TXINT;
		}
		if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
			tp->t_state &= ~TTIOW;
			wakeup((caddr_t)&tp->t_oflag);
		}
		if ((tp->t_state&OASLP) && tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD]) {
			tp->t_state &= ~OASLP;
			wakeup((caddr_t)&tp->t_outq);
		}
		break;

	case T_SUSPEND:
		if (pi->tiocpkt && pi->tioctty) {
			pi->pktbyte |= TIOCPKT_STOP;
			pi->pktbyte &= ~TIOCPKT_START;
			pi->sendpktbyte = TRUE;
			if (pi->ptmsleep) {
				pi->ptmsleep = FALSE;
				wakeup((caddr_t)&pi->ptmsleep);
			}
			ptyselwakeup(&pi->pty_selr);
		}
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:
		/* Input has been stopped, send a 
		 * CSTOP character.
		 */
		pi->pty_state |= PBLOCK;
		if (tp->t_state & TTXON) {
			if (pi->ptmsleep) {
				pi->ptmsleep = FALSE;
				wakeup((caddr_t)&pi->ptmsleep);
			}
			ptyselwakeup(&pi->pty_selw);
		}
		tp->t_state &= ~TTXON;
		tp->t_state |= TTXOFF|TBLOCK;
		break;

	case T_RFLUSH:
		if (pi->tiocpkt && pi->tioctty) {
			pi->pktbyte |= TIOCPKT_FLUSHREAD;
			pi->sendpktbyte = TRUE;
			if (pi->ptmsleep) {
				pi->ptmsleep = FALSE;
				wakeup((caddr_t)&pi->ptmsleep);
			}
			ptyselwakeup(&pi->pty_selr);
		}
		ptyselwakeup(&pi->pty_selw);
		if (!(tp->t_state&TBLOCK))
			break;

	case T_UNBLOCK:
		/* Input has been restarted, send a 
		 * CSTART character.
		 */
		pi->pty_state |= PUNBLOCK;
		if ((tp->t_state & TTXON) == 0) {
			if (pi->ptmsleep) {
				pi->ptmsleep = FALSE;
				wakeup((caddr_t)&pi->ptmsleep);
			}
			ptyselwakeup(&pi->pty_selw);
		}
		tp->t_state &= ~(TTXOFF|TBLOCK);
		tp->t_state |= TTXON;
		/*
		 * Make this call after everything else is done so that
		 * the telnet code will not try to write data before the
		 * appropriate bits are set or cleared.
		 */
		if (callout)
			(*callout)(minor(tp->t_dev), FWRITE);
		break;

	case T_BREAK:
		break;

	case T_VHANGUP:
		pi->vhangup     = data;
		ttcontrol( tp, LDC_VHANGUP, data );
		if (data == 0)
			break;
		wakeup( (caddr_t)&tp->utility );
		wakeup( (caddr_t)&tp->t_rawq ) ;
		wakeup( (caddr_t)&tp->t_outq );
		wakeup( (caddr_t)&pi->trapwait );
		wakeup( (caddr_t)&pi->trapcomplete );
		wakeup( (caddr_t)&pi->pktbyte );
		wakeup( (caddr_t)&cfreelist );
		wakeup( (caddr_t)&pi->ptmsleep );
		wakeup( (caddr_t)&pi->tioctty );
		wakeup( (caddr_t)&pi->trapbusy );
		break;
	}
}


pty_nop()
{
	return(0);
}


struct tty *pty_nop2()
{
	return(0);
}

int ptys_select();		/* forward reference in pty_driver */

struct tty_driver pty_driver = {
	SIONULL,		/* Driver Number */
	ptys_open,
	ptys_close,
	ptys_read,
	ptys_write,
	ptys_ioctl,
	ptys_select,
	ptys_putchar,		/* pseudo-putchar routine */
	pty_nop,		/* wait routine */
	pty_nop2,		/* pwr_init routine */
	pty_nop,		/* who_init routine */
	0			/* next tty in the list */
	};


ptys_link()
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Pty init routine to be called at power up. 
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 |  6 Dec 91	dmurray	Fix for INDaa10655. Changed t_int_lvl to protect
 |			against kernel telnet access when netisr is on ICS.
 |
 |  15 Oct 93   rpc	The tty structs and pty_info structs are now
 |			dynamically allocated, rather than being allocated
 |			at compile time. Now, just initialize the arrays
 |			of pointers to NULL and set npty to 0. npty now
 |			represents the maximum number of open ptys that
 |			can be on the system. This was a 9.03 change.
 |
 |
 ---------------------------------------------------------------------------*/
	register i;
	for (i=0; i<npty; i++)
		pt_line[i] = (struct tty *) 0 ;
	max_ptys = npty ;
	npty = 0 ;
}


pty_tty_init(index)
int index ;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Pty init routine to be called when a tty/pty_info
 |			struct is allocated at open time. Called by
 |			allocate_ptys
 |
 | MODIFICATION HISTORY
 |
 |  15 Oct 93   rpc	Only the pt_line and pty_info structs specified by
 |			"index" are initialized. The tty structs and pty_info
 |			structs are now dynamically allocated, rather than
 |			being allocated at compile time.
 |
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[index];

	if (tp == (struct tty *) 0)
		panic ("pty_tty_init: pt_line is corrupt") ;
	
	tp->t_proc = pty_proc;
	tp->t_drvtype = &pty_driver;
	tp->t_int_lvl = 2;	/*Protect against kernel telnet on ICS*/
	tp->t_cproc = NULL;
	pi_init(tp->utility);
}

pi_init(pi)
register struct pty_info *pi;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Initialize a PTY_info structure.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 01 Nov 88  garth	Fix for SR 4700-683599 (aka, DTS INDaa03140).
 | 			Initialize pty flow control states to off.
 |
 | 16 May 90  jph	Fix for DTS INDaa07273.
 |			Initialize pty flow control states to off, *BUT*
 |			DON'T set other bits (i.e. don't do "=", do "&=").
 |
 | 18 Oct 90	rpc	The pty_info struct is bzeroed when malloced. No need
 |			to initialize all fields to zero now. I left the
 |			FALSE statements in incase someone changes FALSE
 |			to non-zero.
 |
 ---------------------------------------------------------------------------*/
	pi->ptmsleep = FALSE;
	pi->tiocsigmode = TIOCSIGNORMAL;
	pi->tioctrap = FALSE;
	pi->trapbusy = FALSE;
	pi->trapwait = FALSE;
	pi->trapcomplete = FALSE;
	pi->tioctty = TRUE;
	pi->tiocpkt = FALSE;
	pi->tiocmonitor = FALSE;
	pi->tiocremote = FALSE;
	pi->sendpktbyte = FALSE;
	pi->callout = NULL;
	/* Reset pty flow control states */
	pi->pty_state &= ~(PBLOCK|PUNBLOCK);
}

/* Slave side select routine */
ptys_select(dev, which)
{
/*---------------------------------------------------------------------------
 | FUNCTION -	 	Select for slave side of PTY.	
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 23 Feb 89  garth	For SR 4700-691964 (aka, DTS INDaa03213).
 |			Added several checks for read selects so that
 |			the line discipline gets bypassed when in remote
 |			mode and termio(7) processing is disabled.  Before
 |			fix, read selects returned incorrect values when
 |			in remote mode or when termio(7) processing was
 |			disabled.
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;
#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptys_select: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptys_select: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	if ((tp->t_state & CARR_ON) == 0)
		return(1);
	if (which == FREAD) {
		if (pi->tioctty) {
			if (pi->tiocremote) {
			       if (tp->t_rawq.c_cc || !(pi->pty_state & PMPRES))
			       		return(1);
			} else
				return(ttselect(tp,which));
		} else {
			if (tp->t_rawq.c_cc)
				return(1);
		}
		ptyselqueue(&pi->pty_selr);
		return(0);
	}
	return(ttselect(tp,which));
}

/* Master side select routine */
ptym_select(dev, which)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Select for Master side of PTY.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 23 Feb 89  garth	Fix for SR 4700-679142 (aka, DTS INDaa03140).
 |			Added check for stopped input.  
 |
 | 24 Feb 89  garth	For SR 4700-739706 (aka, DTS INDaa04271).
 |			Added code to prevent master write selects 
 |			from blocking when ICANON is enabled.  Before
 |			fix, a deadlock situation would occur if a newline 
 |			character (\n) needed to be written to complete
 |			a read and there was data on the rawq.  The
 |			select would block and the newline would never
 |			be written.
 |
 | 15 Jan 90  garth     Fix for SR 4700-796698 (aka, DTS INDaa05356).
 |			Modified write select so that it returns true if
 |			Output flow control has not been turned on.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *)tp->utility;

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_select: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_select: pty_info list is corrupt") ;
#endif /* OSDEBUG */

	switch (which) {
	case FREAD:
		if ((tp->t_outq.c_cc && !(tp->t_state&TTSTOP)) || (pi->tiocpkt && pi->sendpktbyte)) 
			return(1);
		ptyselqueue(&pi->pty_selr);
		break;
	case FWRITE:
		if (!(tp->t_state & TTXOFF))
			return(1);
		ptyselqueue(&pi->pty_selw);
		break;
	case 0:
		if (pi->r_info.request)
			return(1);
		ptyselqueue(&pi->pty_sele);
		break;
	}
	return(0);
}


ptyselqueue(ptysel)
struct pty_select *ptysel;
{
	register struct proc *p;

	if ((p = ptysel->pty_selp) && (p->p_wchan == (caddr_t) &selwait))
		ptysel->pty_selflag |= PSEL_COLL;
	else
		ptysel->pty_selp = u.u_procp;
}


ptyselwakeup(ptysel)
struct pty_select *ptysel;
{
	if (ptysel->pty_selp) {
		selwakeup(ptysel->pty_selp, ptysel->pty_selflag & PSEL_COLL);
		ptysel->pty_selp = 0;
		ptysel->pty_selflag &= ~PSEL_COLL;
	}
}

ptym_control(dev,cmd,arg)
dev_t dev;
enum sdc_func cmd;
caddr_t arg;
{ 
/*---------------------------------------------------------------------------
 | FUNCTION -		Performs miscellaneous PTY master functionality.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 20 Mar 90  garth 	Fix for problem with kernel telnet and international
 |			character sets.  Changed data passed to line discipline
 |			with SDC_PUTC to be an unsigned character.  This 
 |			prevents the character from being sign extended when
 |			all 8 bits are used.
 |
 ---------------------------------------------------------------------------*/
	register struct tty *tp = pt_line[minor(dev)];
	register struct pty_info *pi = (struct pty_info *) tp->utility;
	register struct clist *tq = &tp->t_outq;
	register struct cblock *cp;
	register int nchar, maxchar;
	register ci;
	int c;

#ifdef OSDEBUG
	if (tp == (struct tty *) 0)
		panic ("ptym_control: pt_line list is corrupt") ;
	if (pi == (struct pty_info *) 0)
		panic ("ptym_control: pty_info list is corrupt") ;
#endif /* OSDEBUG */

#define BUFF	((struct iovec *)arg)

	switch(cmd) {
	case SDC_PUTC:
		if (tp->t_state &TTXOFF)
			return(ENOBUFS);
		ttin(tp,(u_char)arg,0,0);
		break;
	case SDC_GETC:
		if ((tp->t_state & TTSTOP) || (tp->t_outq.c_cc == 0))
			return(ENOBUFS);
		*(u_char *)arg = getc(tq);
		/* 
		 * if someone is waiting for room to write then wake
		 * them up
		 * 
		 */
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~WCOLL;
		}
		if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
			tp->t_state &= ~TTIOW;
			wakeup((caddr_t)&tp->t_oflag);
		}
		if ((tp->t_state&OASLP) && tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD]) {
			tp->t_state &= ~OASLP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_state&BUSY) {
			tp->t_state &= ~BUSY;
			wakeup((caddr_t)&tp->t_state);
		}
		break;
	case SDC_PUTB:
		while (BUFF->iov_len) {
			if (tp->t_state & TTXOFF)
				break;
			(BUFF->iov_len)--;
			ttin(tp,(u_char)*(BUFF->iov_base)++,0,0);
		}
		break;
	case SDC_GETB:
		maxchar = BUFF->iov_len;
		BUFF->iov_len = 0;
		while (maxchar > 0) {
			if (tp->t_state & TTSTOP)
				return(0);
			if (tp->t_outq.c_cc == 0)
				break;
			nchar = tq.c_cf->c_last - tq.c_cf->c_first;
			if (maxchar >= nchar) {
				BUFF->iov_len += nchar;
				maxchar -= nchar;
				cp = getcb(tq);
				for (ci = cp->c_first;ci < cp->c_last;ci++)
				   *(BUFF->iov_base)++ = (u_char)cp->c_data[ci];
				putcf(cp);
			} else {
				BUFF->iov_len += maxchar;
				while (maxchar--)
					*(BUFF->iov_base)++ = getc(tq); 
			}
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~WCOLL;
		}
		if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
			tp->t_state &= ~TTIOW;
			wakeup((caddr_t)&tp->t_oflag);
		}
		if ((tp->t_state&OASLP) && tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD]) {
			tp->t_state &= ~OASLP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_state&BUSY) {
			tp->t_state &= ~BUSY;
			wakeup((caddr_t)&tp->t_state);
		}
		break;
	case SDC_CALLOUT:
		pi->callout = (void (*)())arg;
		break;
	}
	return(0);
}


free_ptys (index)
int	index ;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Frees the tty struct pointed to by tp, and frees
 |			the pty_info struct pointed to by tp->utility
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp = pt_line[index];
	register struct pty_info *pi = (struct pty_info *)tp->utility;

	(void) kfree (pi, M_DEVBUF) ;
	(void) kfree (tp, M_DEVBUF) ;
	pt_line[index] = (struct tty *) 0 ;
	--npty ;
	return ;
}



allocate_ptys (index)
int index ;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Allocates the tty struct pointed to by tp, and frees
 |			the pty_info struct pointed to by tp->utility. Also
 |			calls pty_tty_init to initialize the structs.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 ---------------------------------------------------------------------------*/

	register struct tty *tp  ;
	register struct pty_info *pi  ;

	tp = (struct tty *) kmalloc (sizeof (struct tty),M_DEVBUF,M_WAITOK) ;
	if (tp == (struct tty *) NULL)
		return (EAGAIN) ; /* Wrong error, but already documented */
	pi = (struct pty_info *) kmalloc(sizeof (struct pty_info),
				         M_DEVBUF,M_WAITOK);
	if (pi == (struct pty_info *) NULL) {
		kfree (tp,M_DEVBUF) ;
		return (EAGAIN) ; /* Wrong error, but already documented */
	}
	bzero ((char *)tp,sizeof (struct tty)) ;
	bzero ((char *)pi,sizeof (struct pty_info)) ;
	pt_line[index] = tp ;
	tp->utility = (int *)pi;

	npty++ ;
	pty_tty_init(index) ;
	return (0) ;
}

