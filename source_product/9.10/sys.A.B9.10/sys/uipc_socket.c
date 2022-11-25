/*
 * $Header: uipc_socket.c,v 1.11.83.3 93/09/17 20:14:25 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_socket.c,v $
 * $Revision: 1.11.83.3 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 20:14:25 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_socket.c $Revision: 1.11.83.3 $ $Date: 93/09/17 20:14:25 $";
#endif

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)uipc_socket.c	7.10 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"

#include "../net/netmp.h"      	   /* MPNET */

/* HP: for the privilege of #include'g tcp_debug.h */
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip_var.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"
#include "../netinet/tcp_debug.h"  /* HP */

extern int soconnindication();

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	test socketpair
 *	clean up async
 *	out-of-band is a kludge
 */
/*ARGSUSED*/
int
socreate(dom, aso, type, proto, flags)
	struct socket **aso;
	register int type;
	int proto;
	unsigned int flags;  /*HP*/
{
	register struct protosw *prp;
	register struct socket *so;
	int error, oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	error = 0;
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);  /*MPNET: MP prot on */

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0) {
		/*MPNET: prot off */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (EPROTONOSUPPORT);
	}
	if (prp->pr_type != type) {
		/*MPNET: prot off */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (EPROTOTYPE);
	}

	MALLOC(so, struct socket *, sizeof (struct socket), M_SOCKET,
               (flags & SS_NOWAIT) ? M_NOWAIT : M_WAITOK);
        if (!so) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (ENOBUFS);
	}

	bzero((caddr_t)so, sizeof (struct socket));
	mbstat.m_mtypes[MT_SOCKET]++;
	so->so_rcv.sb_so = so;
	so->so_snd.sb_so = so;
	so->so_options = 0;
	so->so_state = 0;
	so->so_type = type;
	/*HP* if SS_NOUSER, "u." might not be real user context
	 *
	 * NB: we permit SS_PRIV without SS_NOUSER for flexibility.
	 * however, currently SS_PRIV is used only when SS_NOUSER is
	 * also set.
	 */
	if (u.u_uid == 0 && !(flags & SS_NOUSER))
		so->so_state = SS_PRIV;
        so->so_state |= flags & (SS_NOWAIT | SS_NOUSER | SS_PRIV);
	so->so_proto = prp;
	so->so_ipc = SO_BSDIPC;
	so->so_connindication = soconnindication;
	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)proto, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		/*MPNET: prot off */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (error);
	}
	*aso = so;

	/*MPNET: prot off */
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return (error);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int oldlevel;
	int error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate); /*MPNET: MP prot on */

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return (error);
}

solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int oldlevel, error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate); /*MPNET: MP prot on */

	/* return error if socket is connected or being connected */
	if (so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return (EISCONN);
	}

	/* return error if socket has been shut down */
	if (so->so_state & (SS_CANTSENDMORE | SS_CANTRCVMORE)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return (EINVAL);
	}

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, SOMAXCONN);

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return (0);
}

/* MPNET: This routine derives its MP protection from its caller. */
sofree(so)
	register struct socket *so;
{
	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		so->so_head = 0;
	}
	sbrelease(&so->so_snd);
	sorflush(so);

	/* NOTE:  The following exists for tcp connections that are dropped */
	/* BEFORE they have been accepted by the user.  In this case,       */
	/* sofree must free the netipc control block.  The assumption made  */
	/* here is that NO other structures are attached to the ipccb       */

	if ((so->so_ipc == SO_NETIPC) && (so->so_ipccb)) {
		FREE(so->so_ipccb, M_NIPCCB);
		mbstat.m_mtypes[MT_IPCCB]--;
	}

	FREE(so, M_SOCKET);
	mbstat.m_mtypes[MT_SOCKET]--;
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
soclose(so)
	register struct socket *so;
{
	int oldlevel;
	int error = 0;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);	/*MPNET: MP prot on */

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0 != so)
			(void) soabort(so->so_q0);
		while (so->so_q != so)
			(void) soabort(so->so_q);
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & (SS_NBIO | SS_NOWAIT)))
				goto drop;
			if ((so->so_state & (SS_ISCONNECTED | SS_NOWAIT))
			    == (SS_ISCONNECTED | SS_NOWAIT)) {
			   /* HP *
			    * we assume that SS_NOWAIT user does not have fd
			    * for socket or that fd is not disposed of if 
			    * soclose fails.  currently, this is not the
			    * case with closef (!).  But current SS_NOWAIT
			    * user does not call closef; also, moot point
			    * because current SS_NOWAIT user does not set
			    * SO_LINGER.
			    */
			   error = EWOULDBLOCK;
			   goto out;
			}
			while (so->so_state & SS_ISCONNECTED)
				/* MPNET: we lose MP protection on sleep,
				** but it is regained automatically on
				** wakeup.
				*/
				sleep((caddr_t)&so->so_timeo, PZERO+1);
		}
	}
drop:
	if (so->so_pcb) {
		int error2 =
		    (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0)
			error = error2;
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
out:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return (error);
}

/*
 * Must be called at splnet...
 * MPNET: This routine derives its MP protection from its caller.
 */
soabort(so)
	struct socket *so;
{
	return (
	    (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
}

soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int error;

	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
	return (error);
}

soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int oldlevel;
	int error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	if (so->so_options & SO_ACCEPTCONN)
		return (EOPNOTSUPP);
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);	/*MPNET: MP prot on */

	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnect(so))))
		error = EISCONN;
	else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		    (struct mbuf *)0, nam, (struct mbuf *)0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return (error);
}

soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int oldlevel;
	int error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate); /*MPNET: MP prot on */
	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return (error);
}

sodisconnect(so)
	register struct socket *so;
{
	int oldlevel;
	int error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate); /*MPNET: MP prot on */

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
bad:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return (error);
}

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 */

sosend(so, nam, uio, flags, rights, kdatap)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
	struct mbuf *kdatap;  /*HP* inbound mbuf chain if MSG_MBUF */
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp;
	register int space;
	int len, rlen = 0, error = 0, oldlevel, dontroute, first = 1;
	int mgetwait = (so->so_state & SS_NOWAIT) ? M_DONTWAIT : M_WAIT;
	int resid;
	sv_sema_t savestate;		/* MPNET: MP save state */


	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat)
		return (EMSGSIZE);
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);

#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgsnd++;	/* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgsnd++;
#endif /* Series 800 */

	if (rights)
		rlen = rights->m_len;

#define	snderr(errno)	{ error = errno; goto release; }

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */

restart:
	error = sbmaylock(&so->so_snd);
	if (error)
	   goto out;

	do {
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;			/* ??? */
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);
			if (nam == 0)
				snderr(EDESTADDRREQ);
		}
                resid = (flags & MSG_MBUF) ? m_len(kdatap) : uio->uio_resid;

		if (flags & MSG_OOB)
			space = 1024;
		else {
			/* defer if:
			 *    (1) not enough space to send rights; OR
			 *    (2) not enough space to send all at once, if
			 *        required; OR
			 *    (3) sending >= cluster, but avail < cluster,
			 *        and socket >= cluster, and not nonblocking
			 *        I/O. (why?)
			 */
			space = sbspace(&so->so_snd);

			if (space <= rlen ||
			   (sosendallatonce(so) && space < resid + rlen) ||
                           (resid >= MCLBYTES && space < MCLBYTES &&
			    so->so_snd.sb_cc >= MCLBYTES &&
                            (so->so_state & (SS_NBIO | SS_NOWAIT)) == 0 &&
			    (uio->uio_fpflags & (O_NONBLOCK | O_NDELAY)) == 0)){
				if (uio->uio_fpflags & O_NDELAY) {
					if (first)
						error = 0;
					goto release;
				}
				if (uio->uio_fpflags & O_NONBLOCK) {
					if (first)
						error = EAGAIN;
					goto release;
				}
				if (so->so_state & (SS_NBIO | SS_NOWAIT)) {
					if (first)
						error = EWOULDBLOCK;
					goto release;
				}
				sbunlock(&so->so_snd);
				error = sbwait(&so->so_snd);
				if (error) {
					if (top) 
						m_freem(top);
					if (first)  {
					    NETMP_GO_UNEXCLUSIVE(oldlevel,
						savestate);/*MPNET: prot off */
						return(EINTR);
					}
					else {
					    NETMP_GO_UNEXCLUSIVE(oldlevel,
						savestate);/*MPNET: prot off */
					    return(0);
					}
				} else goto restart;
			}
		}

		mp = &top;
		space -= rlen;
		if (flags & MSG_MBUF) {
		   top = kdatap;
                   kdatap = 0;
                   uio->uio_resid = resid = 0;
                }
		else {
		   while (space > 0) {
 		      MGET(m, mgetwait, MT_DATA);
		      if (!m) {
		         error = ENOBUFS;
		         break;
		      }
		      /* HP: performance optimization: use clusters rather
		       * than mbufs for larger data transfers.  Allow send to
		       * exceed sockbuf by MCLBYTES-1.  The test used to be :
		       *    if (uio->uio_resid >= MCLBYTES / 2 &&
		       *        space >= MCLBYTES)
		       */
		      if (uio->uio_resid >= MCLBYTES>>1) {
 			    MCLGET(m);
 			    if (m->m_len != MCLBYTES)
 				    goto nopages;
 			    len = MIN(MCLBYTES, uio->uio_resid);
 		      } else {
nopages:
			    len = MIN(MIN(MLEN, uio->uio_resid), space);
		      }

		      space -= len;
		      error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
		      m->m_len = len;
		      *mp = m;
		      if (error)
				break;
		      mp = &m->m_next;
		      if (uio->uio_resid <= 0)
				break;
		   }
                }

		if (error)
			break;

		if (dontroute)
			so->so_options |= SO_DONTROUTE;
		error = (*so->so_proto->pr_usrreq)(so,
		    (flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
		    top, (caddr_t)nam, rights);
		if (dontroute)
			so->so_options &= ~SO_DONTROUTE;

		rights = 0;
		rlen = 0;
		top = 0;
		if (error)
			break;
		first = 0;
	} while (uio->uio_resid);

release:
	sbunlock(&so->so_snd);
out:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	if (top)
		m_freem(top);
	if (error == EPIPE)
		psignal(u.u_procp, SIGPIPE);
	if (!first) error = 0;
	return (error);
}


/*
 * Implement receive operations on a socket.
 * We depend on the way that records are added to the sockbuf
 * by sbappend*.  In particular, each record (mbufs linked through m_next)
 * must begin with an address if the protocol so specifies,
 * followed by an optional mbuf containing access rights if supported
 * by the protocol, and then zero or more mbufs of data.
 * In order to avoid blocking network interrupts for the entire time here,
 * we give up MP exclusion while doing the actual copy to user space.
 * Although the sockbuf is locked, new data may still be appended,
 * and thus we must maintain consistency of the sockbuf during that time.
 */
soreceive(so, aname, uio, flags, rightsp, kdatap)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int *flags;
	struct mbuf **rightsp;
	struct mbuf **kdatap;   /*HP* outbound mbuf chain if MSG_MBUF */
{
	int local_flags = *flags;
	register struct mbuf *m;
	int len, error = 0, oldlevel, offset;
	struct protosw *pr = so->so_proto;
	struct mbuf *nextrecord;
	int moff;
	int copied_mbuf;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */

	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;
	if (local_flags & MSG_OOB) {
		m = m_get((so->so_state & SS_NOWAIT) ? M_DONTWAIT : M_WAIT,
                          MT_DATA);
		if (!m)
		   error = ENOBUFS;
		else {
		   error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		              m, (struct mbuf *)(local_flags & MSG_PEEK),
		              (struct mbuf *)0);
		}

		if (error)
			goto bad;

		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;

			if (local_flags & MSG_MBUF) {
			   /*HP*
			    * return entire mbuf directly, even if more than 
			    * uio_resid.
			    *
			    * note that this is different from the uiomove
			    * case, which truncates the OOB data (!), but it
			    * is consistent with MSG_MBUF handling of normal
			    * data (below); that is, the caller must tolerate
			    * this.  since the OOB data has been taken off
			    * the "socket", ideally we should return it
			    * __all__ to the user.  in the uiomove case, we
			    * have no choice but to truncate.  (perhaps
			    * better:  we could have uio_resid to PRU_RCVOOB.)
			    * in the mbuf xfer case, we defer the decision to
			    * the caller (e.g., tpiso).
			    *
			    * of course, the issue is purely pedantic since
			    * TCP OOB data is only one octet.
			    */
			   *kdatap = m;
			   uio->uio_resid -= len;
			   m = m->m_next;
			   kdatap = &(*kdatap)->m_next;
			   *kdatap = 0;            /* m_next = 0 */
			}
			else { 
			   error = uiomove(mtod(m, caddr_t), (int)len,
			                   UIO_READ, uio);
			   m = m_free(m);
			}
		} while (uio->uio_resid > 0  &&  error == 0  &&  m);
bad:
		if (m)
			m_freem(m);
		/*MPNET: prot off */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (error);
	}

restart:
	error = sbmaylock(&so->so_rcv);
	if (error)
	   goto out;

	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE)
			goto release;
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if (uio->uio_fpflags & O_NONBLOCK) {
			error = EAGAIN;
			goto release;
		}
		if (uio->uio_fpflags & O_NDELAY) {
			error = 0;
			goto release;
		}
		if (so->so_state & (SS_NBIO | SS_NOWAIT)) {
			error = EWOULDBLOCK;
			goto release;
		}
		sbunlock(&so->so_rcv);
		error = sbwait(&so->so_rcv);
		if (error) {
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET prot off*/
		    return(EINTR);
		}
		goto restart;
	}

#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgrcv++;	/* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgrcv++;
#endif /* Series 800 */

	m = so->so_rcv.sb_mb;
	if (m == 0)
		panic("receive 1");
	nextrecord = m->m_act;

	if (pr->pr_flags & PR_ADDR) {
		if (m->m_type != MT_SONAME)
			panic("receive 1a");
		if (local_flags & MSG_PEEK) {
			if (aname)
				*aname = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (aname) {
				*aname = m;
				m = m->m_next;
				(*aname)->m_next = 0;
				so->so_rcv.sb_mb = m;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
			if (m)
				m->m_act = nextrecord;
		}
	}

	if (m && m->m_type == MT_RIGHTS) {
		if ((pr->pr_flags & PR_RIGHTS) == 0)
			panic("receive 2");
		if (local_flags & MSG_PEEK) {
			if (rightsp)
				*rightsp = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (rightsp) {
				*rightsp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
			if (m)
				m->m_act = nextrecord;
		}
	}

	moff = 0;      /* changed only if MSG_PEEK */
	offset = 0;    /* changed only if MSG_PEEK */
	while (m  &&  uio->uio_resid > 0  &&  error == 0) {
		if (m->m_type != MT_DATA && m->m_type != MT_HEADER)
			panic("receive 3");

		len = uio->uio_resid;
		so->so_state &= ~SS_RCVATMARK;

		/* if OOB mark, only copy up to mark */
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;

		/* if beyond mbuf, copy only to end of mbuf */
		if (len > m->m_len - moff)
			len = m->m_len - moff;

		/*HP*
		 * if MSG_MBUF, return mbuf chain instead of copying into a
		 * flat buffer space.  avoid copying mbufs for performance
		 * reasons.
		 */
		copied_mbuf = 1;
		if (local_flags & MSG_MBUF) {
		   /* if PEEK, we must copy mbuf.  if OOB mark, we copy mbuf
		    * if len is at OOB mark and OOB mark is within mbuf.
		    */
		   if ((local_flags & MSG_PEEK)  ||
		       (so->so_oobmark  &&  len == so->so_oobmark - offset  &&
		        len < m->m_len - moff) ) {
		      *kdatap = m_copy(m, moff, len);
		      if (!*kdatap)
		         error = ENOBUFS;
		   }
		   else {
		      /* otherwise, simply return entire mbuf directly.
		       *
		       * if len < m_len, we will still return the entire
		       * mbuf, which may be more than uio_resid.  caller
		       * must tolerate this.  it is necessary because
		       * caller cannot know the size of the mbuf (cluster) 
		       * a priori.  if we did not return the entire mbuf,
		       * data might never be read from the socket.  thus,
		       * uio_resid is advisory in this case.
		       *
		       * the alternative might be to copy part of mbuf.
		       * we prefer not to do this for tpiso for performance
		       * reasons.
		       *
		       * hindsight:  might have been better to invent
		       * MSG_ADVISORY (?) to control this decision.
		       */
		      VASSERT(moff == 0);
		      *kdatap = m;
		      len = m->m_len;
		      copied_mbuf = 0;
		   }

		   if (!error) {
		      kdatap = &(*kdatap)->m_next;
		      uio->uio_resid -= len;
		   }
		}
		else { 
		   error = uiomove(mtod(m, caddr_t) + moff, (int)len, 
		                   ((m->m_flags & MF_NOACC) && 
		                    (so->so_proto->pr_flags & PR_COPYAVOID))
		                   ? UIO_READ|UIO_AVOID_COPY : UIO_READ,
		                   uio);
		}  

		if (len == m->m_len - moff) {          /* copied all */
			if (local_flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				/* OSI needs eom flag *HP*/
				*flags = m->m_flags;
				nextrecord = m->m_act;
				sbfree(&so->so_rcv, m);
                                if (!copied_mbuf) {
				   /*HP*
				    * MSG_MBUF is set, and conditions
				    * permitted us to return mbuf directly
				    * rather than copy data.  don't m_free.
				    */
				   so->so_rcv.sb_mb = m->m_next;
				   m->m_next = 0;
				}
				else
				   MFREE(m, so->so_rcv.sb_mb);

				m = so->so_rcv.sb_mb;
				if (m)
					m->m_act = nextrecord;
			}
		} else {                             /* copyied less */
			if (local_flags & MSG_PEEK)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}

		if (so->so_oobmark) {
			if ((local_flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else
				offset += len;
		}
	}  /* while */

	if ((local_flags & MSG_PEEK) == 0) {
		if (m == 0)
			so->so_rcv.sb_mb = nextrecord;
		else if (pr->pr_flags & PR_ATOMIC && so->so_ipc == SO_BSDIPC)
			(void) sbdroprecord(&so->so_rcv);
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
			    (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0 && rightsp && *rightsp &&
		    pr->pr_domain->dom_externalize)
			error = (*pr->pr_domain->dom_externalize)(*rightsp);
	}

release:
	sbunlock(&so->so_rcv);
out:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return (error);
}


soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr = so->so_proto;
	int result, oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	how++;
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */

	if (how & FREAD) {
		result = somayrflush(so);
		if (result)
		   goto out;
	}

	if (how & FWRITE)
		result = (*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	else result = 0;

out:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return(result);
}


int
somayrflush(so)                      /*HP*/
	register struct socket *so;
{
	/* NOTE:  changes here must copied into sorflush() */

	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	int s, error;
	struct sockbuf asb;

        /* sblock conditionally */
	error = sbmaylock(sb);
	if (error)
	   return (error);

	s = splimp();
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof(*sb));
	sb->sb_so = asb.sb_so;
	splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
	return (0);
}


sorflush(so)
	register struct socket *so;
{
	/* NOTE:  changes here must copied into somayrflush() */

	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	int s, error;
	struct sockbuf asb;

	/* sblock unconditionally */
	sblock(sb);

	s = splimp();
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof(*sb));
	sb->sb_so = asb.sb_so;
	splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}


sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error = 0, oldlevel;
	register struct mbuf *m = m0;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
		    error = ((*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, &m0));
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		    return(error);
		}
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

		case SO_LINGER:
			if (m == NULL || m->m_len != sizeof (struct linger)) {
				error = EINVAL;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			/* fall thru... */

		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			if (*mtod(m, int *))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;


		case SO_DEBUG:
			if (m == NULL || m->m_len != sizeof (int)) {
				error = EINVAL;
				goto bad;
			}

			if (*mtod(m, int *)) {
				if (!tcp_debug) {
				   /* HP: MALLOC tcp_debug[TCP_NDEBUG] to
				    * conserve memory.  never bother to
				    * dealloc; might be in use by another
				    * socket later.  init to zero, just as if
				    * statically alloc'd; only way to disting
				    * unused entries.  reset tcp_debx
				    * defensively.
				    *
				    * we alloc here instead of in tcp_trace
				    * because we can M_WAITOK.  we borrow
				    * M_SOOPTS.  currently not used elsewhere,
				    * and it's not worth asking for another
				    * MALLOC type.  if net mem alloc is ever
				    * constrained (netmemmax), M_SOOPTS would
				    * properly fall under constraint.  (we
				    * really need an M_NETMISC type.)
				    */
	                           MALLOC(tcp_debug, struct tcp_debug*,
                                          TCP_NDEBUG*sizeof(struct tcp_debug),
                                          M_SOOPTS, 
				          (so->so_state & SS_NOWAIT)
				             ? M_NOWAIT : M_WAITOK);
				   if (!tcp_debug) {
				      error = ENOBUFS;
				      goto bad;
				      }
				   bzero((caddr_t)tcp_debug,
				         TCP_NDEBUG*sizeof(struct tcp_debug));
				   tcp_debx = 0;
				}

				/* don't set state until MALLOC is okay */
				so->so_options |= SO_DEBUG;
			}
			else
				so->so_options &= ~SO_DEBUG;
			break;


		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		case SO_SND_COPYAVOID:
		case SO_RCV_COPYAVOID:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:

			    /* HP: check for shrinking buffers for AF_INET and
			     * X.25 , AF_UNIX does not need the checking
			     */

                            if (so->so_proto && so->so_proto->pr_ctloutput) {
                              if ((error = ((*so->so_proto->pr_ctloutput)
                                (PRCO_SHRINKBUFFER, 
                                 so, 
                                 so->so_proto->pr_protocol,
                                 optname, 
                                 &m0))) != 0) { 
                                    goto bad; 
                                } 
                             }
				if (sbreserve_sw(so, optname == SO_SNDBUF ?
				    &so->so_snd : &so->so_rcv,
				    (u_long) *mtod(m, int *)) == 0) {
					error = ENOBUFS;
					goto bad;
				}
			    /* If the protocol has set the PR_WANTSETSOCKOPT
			    ** flag in the protosw entry, then this means it
			    ** wants to be informed of changes in message
			    ** sizes, e.g., for purposes of adjusting
			    ** flow-control windows.
			    */
			    if (so->so_proto && 
				(so->so_proto->pr_flags & PR_WANTSETSOCKOPT) &&
				so->so_proto->pr_ctloutput) {
				    error = ((*so->so_proto->pr_ctloutput)
				      (PRCO_SETOPT, so, level, optname, &m0));
			    }
				break;

			case SO_SNDLOWAT:
				so->so_snd.sb_lowat = *mtod(m, int *);
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat = *mtod(m, int *);
				break;
			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = *mtod(m, int *);
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = *mtod(m, int *);
				break;
			case SO_SND_COPYAVOID:
				if (*mtod(m, int *))
				    so->so_snd.sb_flags |= SB_COPYAVOID_REQ;
				else
				    so->so_snd.sb_flags &= ~SB_COPYAVOID_REQ;
				break;
			case SO_RCV_COPYAVOID:
				if (*mtod(m, int *))
				    so->so_rcv.sb_flags |= SB_COPYAVOID_REQ;
				else
				    so->so_rcv.sb_flags &= ~SB_COPYAVOID_REQ;
				break;
			}
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
	}
bad:
	if (m)
		(void) m_free(m);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return (error);
}

sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct mbuf *m;
	int oldlevel, error;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/* MP prot on */
	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			error = ((*so->so_proto->pr_ctloutput)
				  (PRCO_GETOPT, so, level, optname, mp));
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			return(error);
		} else  {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			return (ENOPROTOOPT);
		}
	} else {
		m = m_get((so->so_state & SS_NOWAIT) ? M_DONTWAIT : M_WAIT,
		          MT_SOOPTS);
		if (!m) {
		   NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		   return (ENOBUFS);
		}
		m->m_len = sizeof (int);

		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof (struct linger);
			mtod(m, struct linger *)->l_onoff =
				so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
			*mtod(m, int *) = so->so_snd.sb_timeo;
			break;

		case SO_RCVTIMEO:
			*mtod(m, int *) = so->so_rcv.sb_timeo;
			break;

                case SO_SND_COPYAVOID:
                        *mtod(m, int *) = 
				so->so_snd.sb_flags & SB_COPYAVOID_REQ;
			break;

                case SO_RCV_COPYAVOID:
                        *mtod(m, int *) = 
				so->so_rcv.sb_flags & SB_COPYAVOID_REQ;
			break;

		default:
			(void)m_free(m);
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			return (ENOPROTOOPT);
		}
		*mp = m;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return (0);
	}
}

sohasoutofband(so)
	register struct socket *so;
{
	struct proc *p;

	if (so->so_pgrp < 0)
		gsignal(-so->so_pgrp, SIGURG);
	else if (so->so_pgrp > 0 && (p = pfind(so->so_pgrp)) != 0)
		psignal(p, SIGURG);

	if (so->so_rcv.sb_sel) {
		selwakeup(so->so_rcv.sb_sel, so->so_rcv.sb_flags & SB_COLL);
		so->so_rcv.sb_sel = 0;
		so->so_rcv.sb_flags &= ~SB_COLL;
	}
}


#ifdef _WSIO
/* This routine was added to support POLLHUP and POLLERR conditions on
 * a socket. This routine is called when the above "poll" conditions are
 * met.
 */
sohaspollexception(so)
        register struct socket *so;
{
        struct proc *p;

        if (so->so_rcv.sb_sel) {
                selwakeup(so->so_rcv.sb_sel, so->so_rcv.sb_flags & SB_COLL);
                so->so_rcv.sb_sel = 0;
                so->so_rcv.sb_flags &= ~SB_COLL;
        }

        if (so->so_state & SS_ASYNC) {
                if (so->so_pgrp < 0)
                        gsignal(-so->so_pgrp, SIGIO);
                else if (so->so_pgrp > 0 && (p = pfind(so->so_pgrp)) != 0)
                        psignal(p, SIGIO);
        }
}
#endif


sogetaddr(so, nam, which)
	struct socket *so;
	struct mbuf **nam;
	int    which;
{
	int error;
	struct mbuf *m = 0;

	*nam = 0;
	if (which && (so->so_state & (SS_ISCONNECTED)) == 0) {
                                          /* |SS_ISCONFIRMING */
		error = ENOTCONN;
		goto bad;
	}
	m = m_getclr((so->so_state & SS_NOWAIT) ? M_DONTWAIT : M_WAIT, MT_SONAME);
	if (m == NULL) {
		error = ENOBUFS;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so,
				which ? PRU_PEERADDR : PRU_SOCKADDR,
				(struct mbuf *)0, m, (struct mbuf *)0);
	if (error == 0)
		*nam = m;
bad:
	if (error && m)
		m_freem(m);
	return error;
}


/*
 * "Accept" the first queued connection.
 */
sodequeue(head, so, nam)
	struct socket *head, **so;
	struct mbuf **nam;
{
	int error = 0;
	struct mbuf *m;
	struct socket *aso;

	if (nam)
		*nam = 0;
	*so = 0;
	if ((head->so_options & SO_ACCEPTCONN) == 0) {
		error = EINVAL;
		goto bad;
	}
again:
	if (head->so_qlen == 0) {
		error = ENOTCONN;
		goto bad;
	}

	if (head->so_error) {
		error = head->so_error;
		head->so_error = 0;
		goto bad;
	}

	aso = head->so_q;
	if (aso != head->so_q) {		/* Didn't win race */
		aso = 0;
	}
	if (aso == 0)				/* Back to starting block */
		goto again;

	if (soqremque(aso, 1) == 0)
		panic("sodequeue");
	aso->so_state |= SS_NOFDREF;

	m = m_getclr((aso->so_state & SS_NOWAIT) ? M_DONTWAIT : M_WAIT, MT_SONAME);
	(void) soaccept(aso, m);
	*so = aso;
	if (nam)
		*nam = m;
	else
		m_freem(m);
bad:
	return error;
}
