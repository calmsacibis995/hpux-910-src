/*
 * $Header: tcp_usrreq.c,v 1.60.83.5 93/10/28 10:14:44 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/tcp_usrreq.c,v $
 * $Revision: 1.60.83.5 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/28 10:14:44 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) tcp_usrreq.c $Revision: 1.60.83.5 $ $Date: 93/10/28 10:14:44 $";
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
 *	@(#)tcp_usrreq.c	7.10 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#ifdef __hp9000s800
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#endif /* __hp9000s800 */
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"
#include "../h/stat.h"

#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"
#include "../netinet/tcp_debug.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../net/netmp.h"

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
/*
 * TCP protocol interface to socket abstraction.
 */
extern	char *tcpstates[];
struct	tcpcb *tcp_newtcpcb();
void	tcp_acceptconn();
void	tcp_rejectconn();

extern	int tcpcksum;

/*
 * Process a TCP user request for TCP tb.  If this is a send request
 * then m is the mbuf chain of send data.  If this is a timer expiration
 * (called from the software clock routine), then timertype tells which timer.
 */
/*ARGSUSED*/
tcp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct inpcb *inp;
	register struct tcpcb *tp;
	int s;
	int error = 0;
	int ostate;
	int	indx;			/* Used by ENQ_RXMT */
	struct tcpcb *timqp;

	if (req == PRU_CONTROL)
		return (in_control(so, (int)m, (caddr_t)nam,
			(struct ifnet *)rights));
	if (rights && rights->m_len)
		return (EINVAL);

	NET_SPLNET(s);
	inp = sotoinpcb(so);
	/*
	 * When a TCP is attached to a socket, then there will be
	 * a (struct inpcb) pointed at by the socket, and this
	 * structure will point at a subsidary (struct tcpcb).
	 */
	if (inp == 0 && req != PRU_ATTACH) {
		NET_SPLX(s);
		return (EINVAL);		/* XXX */
	}
	if (inp) {
		tp = intotcpcb(inp);
		/* WHAT IF TP IS 0? */
#ifdef KPROF
		tcp_acounts[tp->t_state][req]++;
#endif
		ostate = tp->t_state;
	} else
		ostate = 0;
	switch (req) {

	/*
	 * TCP attaches to socket via PRU_ATTACH, reserving space,
	 * and an internet control block.
	 */
	case PRU_ATTACH:
		if (inp) {
			error = EISCONN;
			break;
		}
		error = tcp_attach(so);
		if (error)
			break;
		if ((so->so_options & SO_LINGER) && so->so_linger == 0)
			so->so_linger = TCP_LINGERTIME;
		tp = sototcpcb(so);
		break;

	/*
	 * PRU_DETACH detaches the TCP protocol from the socket.
	 * If the protocol state is non-embryonic, then can't
	 * do this directly: have to initiate a PRU_DISCONNECT,
	 * which may finish later; embryonic TCB's can just
	 * be discarded here.
	 */
	case PRU_DETACH:
		if (tp->t_state > TCPS_LISTEN)
			tp = tcp_disconnect(tp);
		else
			tp = tcp_close(tp);
		break;

	/*
	 * Give the socket an address.
	 */
	case PRU_BIND:
		error = in_pcbbind(inp, nam);
		if (error)
			break;
		break;

#ifdef	HPXTI
	/*
	 * unbind a connection
	 */
	case PRU_UNBIND:
		error = in_pcbunbind(inp);
		break;
#endif

	/*
	 * Prepare to accept connections.
	 */
	case PRU_LISTEN:
		if (inp->inp_lport == 0)
			error = in_pcbbind(inp, (struct mbuf *)0);
		if (error == 0) {
			tp->t_state = TCPS_LISTEN;
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP,
    				0, 5, tp->t_state, TCPS_LISTEN,
    				tp->t_inpcb->inp_lport,
    				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
		}
		break;

	/*
	 * Initiate connection to peer.
	 * Create a template for use in transmissions on this connection.
	 * Enter SYN_SENT state, and mark socket as connecting.
	 * Start keep-alive timer, and seed output sequence space.
	 * Send initial segment on connection.
	 */
	case PRU_CONNECT:
		if (inp->inp_lport == 0) {
			error = in_pcbbind(inp, (struct mbuf *)0);
			if (error)
				break;
		}
		error = in_pcbconnect(inp, nam);
		if (error)
			break;
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			in_pcbdisconnect(inp);
			error = ENOBUFS;
			break;
		}
		soisconnecting(so);
		tcpstat.tcps_connattempt++;
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
			NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_SYN_SENT,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_SYN_SENT;
		MIB_tcpIncrCounter(ID_tcpActiveOpens);
		/*
		 * Mark connection as active
		 * to allow proper handling of SYNs
		 * in tcp_input.c.
		 * - Subbu, smh
		 */
		tp->t_flags |= TF_ACTIVE;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
		tp->iss = tcp_iss; tcp_iss += TCP_ISSINCR/2;
		tcp_sendseqinit(tp);
		error = tcp_output(tp);
		/* Return ENOBUFS to user if no mbuf for TCP header */
		break;

	/*
	 * Create a TCP connection between two sockets.
	 */
	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Accept a connection
	 */
	case PRU_ACCEPTCONN:
		tcp_acceptconn(so);
		break;

	/*
	 * Reject a connection
	 */
	case PRU_REJECTCONN:
		tcp_rejectconn(tp);
		break;

	/*
	 * Do message mode (for NetIPC)
	 */
	case PRU_MESSAGE_MODE:
		tp->t_flags |= TF_MSGMODE;
		break;

	/*
	 * Initiate disconnect from peer.
	 * If connection never passed embryonic stage, just drop;
	 * else if don't need to let data drain, then can just drop anyways,
	 * else have to begin TCP shutdown process: mark socket disconnecting,
	 * drain unread data, state switch to reflect user close, and
	 * send segment (e.g. FIN) to peer.  Socket will be really disconnected
	 * when peer sends FIN and acks ours.
	 *
	 * SHOULD IMPLEMENT LATER PRU_CONNECT VIA REALLOC TCPCB.
	 */
	case PRU_DISCONNECT:
		tp = tcp_disconnect(tp);
		break;

	/*
	 * Accept a connection.  Essentially all the work is
	 * done at higher levels; just return the address
	 * of the peer, storing through addr.
	 */
	case PRU_ACCEPT: {
		struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

		nam->m_len = sizeof (struct sockaddr_in);
		sin->sin_family = AF_INET;
		sin->sin_port = inp->inp_fport;
		sin->sin_addr = inp->inp_faddr;
		break;
		}

	/*
	 * Mark the connection as being incapable of further output.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		tp = tcp_usrclosed(tp);
		if (tp) {
			error = tcp_output(tp);
			if (error == ENOBUFS) {
				if ((tp->t_timer[TCPT_REXMT] == 0) &&
					(tp->t_timer[TCPT_PERSIST] == 0)) {
					tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
					ENQ_REXMT(tp);
					tp->t_rxtshift = 0;
				}
				error = 0;
			} 
		}
		break;

	/*
	 * After a receive, possibly send window update to peer.
	 */
	case PRU_RCVD:
		(void) tcp_window_update(tp);
		break;

	/*
	 * Do a send by putting data in output queue and updating urgent
	 * marker if URG set.  Possibly send more data.
	 */
	case PRU_SEND:
		sbappend(&so->so_snd, m);
		error = tcp_output(tp);
		if (error == ENOBUFS) {
			if ((tp->t_timer[TCPT_REXMT] == 0) &&
				(tp->t_timer[TCPT_PERSIST] == 0)) {
				tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
				ENQ_REXMT(tp);
				tp->t_rxtshift = 0;
			}
			error = 0;
		} 
		break;

	/*
	 * Abort the TCP.
	 */
	case PRU_ABORT:
		tp = tcp_drop(tp, ECONNABORTED);
		break;

	case PRU_SENSE:
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
		NET_SPLX(s);
		return (0);

	case PRU_RCVOOB:
		if ((so->so_oobmark == 0 &&
		    (so->so_state & SS_RCVATMARK) == 0) ||
		    so->so_options & SO_OOBINLINE ||
		    tp->t_oobflags & TCPOOB_HADDATA) {
			error = EINVAL;
			break;
		}
		if ((tp->t_oobflags & TCPOOB_HAVEDATA) == 0) {
			error = EWOULDBLOCK;
			break;
		}
		m->m_len = 1;
		*mtod(m, caddr_t) = tp->t_iobc;
		if (((int)nam & MSG_PEEK) == 0)
			tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		break;

	case PRU_SENDOOB:
  		if (SEQ_GT(tp->snd_up, tp->snd_una)) {
			m_freem(m);
			error = ENOBUFS;
			break;
		}
		if (sbspace(&so->so_snd) < -((MCLBYTES-1)+512)) {
			m_freem(m);
			error = ENOBUFS;
			break;
		}
		/*
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section.
		 * Otherwise, snd_up should be one lower.
		 */
		sbappend(&so->so_snd, m);
		tp->snd_up = tp->snd_una + so->so_snd.sb_cc;
		tp->t_flags |= TF_FORCE;
		error = tcp_output(tp);
		if (error == ENOBUFS) {
			if ((tp->t_timer[TCPT_REXMT] == 0) &&
				(tp->t_timer[TCPT_PERSIST] == 0)) {
				tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
				ENQ_REXMT(tp);
				tp->t_rxtshift = 0;
			} 
			error = 0;
		} 
		tp->t_flags &= ~TF_FORCE;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	/*
	 * TCP slow timer went off; going through this
	 * routine for tracing's sake.
	 */
	case PRU_SLOWTIMO:
		tp = tcp_timers(tp, (int)nam);
		req |= (int)nam << 8;		/* for debug's sake */
		break;

	/* 
	 * NIKE:  return zero if the protocol/interface supports copy on write.
	 */
	case PRU_COPYAVOID:
		if (inp->inp_route.ro_rt == 0) 
			return (EINVAL);
		if (!tcpcksum)
			inp->inp_route.ro_flags |= RF_PRNOCKSUM;
		if (COW_SUPP(&inp->inp_route))
			return(0);
		else
			return(EOPNOTSUPP);	

	default:
		panic("tcp_usrreq");
	}
	if (tp && (so->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (struct tcpiphdr *)0, req);
	NET_SPLX(s);
	return (error);
}

#ifdef __hp9000s800
tcp_usrsend(so, nam, uio, flags, rights, kdatap)
	struct socket *so;
	struct mbuf *nam;
	struct uio *uio;
	int flags;
	struct mbuf *rights;
	struct mbuf *kdatap;
{
	struct inpcb *inp;
	struct tcpcb *tp;
	struct mbuf *top;
	int space;
	int first;
	int indx;				  /* Used by ENQ_RXMT */
	struct tcpcb *timqp;			  /* Used by ENQ_RXMT */
	sv_sema_t savestate;			  /*MPNET: MP prot state */
	int savelevel;
	int savespl;
	int error;

	if (flags) {
		error = sosend(so, nam, uio, flags, rights, kdatap);
		goto exit;
	}

	if (rights && rights->m_len) {
		error = EINVAL;
		goto exit;
	}
	if ((inp = sotoinpcb(so)) == NULL) {
		error = EINVAL;
		goto exit;
	}
	tp = intotcpcb(inp);

#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgsnd++;	/* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgsnd++;
#endif /* Series 800 */

	NETMP_GO_EXCLUSIVE(savelevel, savestate);/*MPNET: MP prot on */
	first = 1;

restart:sblock(&so->so_snd);
	do {
		if (so->so_state & SS_CANTSENDMORE) {
			error = EPIPE;
			goto unlock;
		}
		if (error = so->so_error) {
			so->so_error = 0;			/* ??? */
			goto unlock;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			error = ENOTCONN;
			goto unlock;
		}
		space = sbspace(&so->so_snd);
		if ((space <= 0) ||
		    (uio->uio_resid >= MCLBYTES && space < MCLBYTES &&
			    so->so_snd.sb_cc >= MCLBYTES)) {
			if (uio->uio_fpflags & O_NDELAY) {
				if (space > 0)
					goto send;
				error = 0;
				goto unlock;
			}
			if (uio->uio_fpflags & O_NONBLOCK) {
				if (space > 0)
					goto send;
				error = EAGAIN;
				goto unlock;
			}
			if (so->so_state & SS_NBIO) {
				if (space > 0)
					goto send;
				error = EWOULDBLOCK;
				goto unlock;
			}
			sbunlock(&so->so_snd);
			error = sbwait(&so->so_snd);
			if (error == 0)
				goto restart;
			error = EINTR;
			goto unexcl;
		}

	send:	error = sobuildsnd(&top, uio, space, so->so_snd.sb_flags);
		if (error)
			goto free;

		NET_SPLNET(s);
		sbappend(&so->so_snd, top);
		error = tcp_output(tp);
		if (error) {
			if (error != ENOBUFS) {
				NET_SPLX(s);
				goto unlock;
			}
			if ((tp->t_timer[TCPT_REXMT] == 0) &&
				(tp->t_timer[TCPT_PERSIST] == 0)) {
				tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
				ENQ_REXMT(tp);
				tp->t_rxtshift = 0;
			}
		} 
		NET_SPLX(s);
		first = 0;

	} while (uio->uio_resid);

	error = 0;
	goto unlock;

free:	m_freem(top);
unlock: sbunlock(&so->so_snd);
unexcl:	NETMP_GO_UNEXCLUSIVE(savelevel, savestate);/*MPNET: prot off */
	if (error == EPIPE)
		psignal(u.u_procp, SIGPIPE);
	if (first == 0) 
		error = 0;
exit:	return (error);
}
#endif /* __hp9000s800 */

tcp_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);
	register struct tcpcb *tp;
	register struct mbuf *m;
	register struct mbuf *m1 = mp;
        struct inpcb *inpcb;
        struct tcpcb *tcpcb;
        short tcp_st;
        u_long new_size, hiwat;


	if (level != IPPROTO_TCP)
		return (ip_ctloutput(op, so, level, optname, mp));
	/*
	 * Nothing in the socket layer prevents a user from executing
	 * a socket option on a dead socket.  Once the connection is
	 * blown away the inp is zero'd, but this code path is still
	 * legal.  We prevent a panic by checking first.
	 */
	if (inp == NULL)
		return(EINVAL);
	tp = intotcpcb(inp);

	switch (op) {

	case PRCO_SETOPT:
		m = *mp;
		switch (optname) {

		case TCP_NODELAY:
			if (m == NULL || m->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(m, int *))
				tp->t_flags |= TF_NODELAY;
			else
				tp->t_flags &= ~TF_NODELAY;

			break;


		case TCP_MAXSEG:	/* not yet */
		default:
			error = EINVAL;
			break;
		}
		if (m)
		        (void) m_free(m);
		break;

	case PRCO_GETOPT:
		*mp = m = m_get((so->so_state & SS_NOWAIT)
		                ? M_DONTWAIT : M_WAIT, MT_SOOPTS);
		if (!m) {
		   error = ENOBUFS;
		   break;
		}

		m->m_len = sizeof(int);

		switch (optname) {
		case TCP_NODELAY:
			*mtod(m, int *) = tp->t_flags & TF_NODELAY;
			break;
		case TCP_MAXSEG:
			*mtod(m, int *) = tp->t_maxseg;
			break;
		default:
			error = EINVAL;
			break;
		}
		break;

	case PRCO_SHRINKBUFFER:
               /* Instead of inventing new defines, we simply use the socket
                  layers defines. They are used to check the shrinking buffer
                  problem  . NOTE: Don't free the mbuf yet, socket layer needs
                  the mbuf for sbreserve. */

		switch (optname) {
                case SO_SNDBUF:
                case SO_RCVBUF:

		        m = *mp;
   			inpcb = (struct inpcb *)so->so_pcb;
		        tcpcb = (struct tcpcb *)inpcb->inp_ppcb;
		        tcp_st = tcpcb->t_state;

                       /* Watch out the states, they may not complete yet */ 

                        if ((tcp_st != TCPS_CLOSED) && (tcp_st != TCPS_LISTEN))                         {
	              /* neither closed nor listening, need to check */
                      new_size = ((u_long) *mtod(m, int *));
                      hiwat = ((optname == SO_SNDBUF) ? 
                              so->so_snd.sb_hiwat : so->so_rcv.sb_hiwat); 
                      if (new_size < hiwat) { /* not allowed to shrink 
                                                     buffer size */
                                error = EINVAL;
                                break;
	                  }
                      } /* no checking needed */
                      break; 
                  default: 
                      error = EINVAL;
                      break;
                    
                 } /* switch optname */
                 break; /* PRCO_SHRINKBUFFER*/
     }
     return (error);
}

u_long	tcp_sendspace = 1024*8;
u_long	tcp_recvspace = 1024*8;
/*
 * Attach TCP protocol to socket, allocating
 * internet protocol control block, tcp control block,
 * buffer space, and entering LISTEN state if to accept connections.
 */
tcp_attach(so)
	struct socket *so;
{
	register struct tcpcb *tp;
	struct inpcb *inp;
	int error;

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, tcp_sendspace, tcp_recvspace);
		if (error)
			return (error);
	}
	error = in_pcballoc(so, &tcb);
	if (error)
		return (error);
	inp = sotoinpcb(so);
	inp->inp_ttl = tcpDefaultTTL;	/* not initialized by in_pcballoc */
	tp = tcp_newtcpcb(inp);
	if (tp == 0) {
		int nofd = so->so_state & SS_NOFDREF;	/* XXX */

		so->so_state &= ~SS_NOFDREF;	/* don't free the socket yet */
		in_pcbdetach(inp);
		so->so_state |= nofd;
		return (ENOBUFS);
	}
	NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    		0, 5, tp->t_state, TCPS_CLOSED,
    		tp->t_inpcb->inp_lport,
    		tp->t_inpcb->inp_fport,
    		tp->t_inpcb->inp_faddr.s_addr);
	tp->t_state = TCPS_CLOSED;
	MIB_tcpIncrConnNumEnt;
	return (0);
}

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
struct tcpcb *
tcp_disconnect(tp)
	register struct tcpcb *tp;
{
	int error;
	struct socket *so = tp->t_inpcb->inp_socket;
	int	indx;			/* Used by ENQ_RXMT */
	struct tcpcb *timqp;

	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_close(tp);
	else if ((so->so_options & SO_LINGER) && so->so_linger == 0)
		tp = tcp_drop(tp, 0);
	else {
		soisdisconnecting(so);
		sbflush(&so->so_rcv);
		tp = tcp_usrclosed(tp);
		if (tp){
			error = tcp_output(tp);
			if (error == ENOBUFS) {
				if ((tp->t_timer[TCPT_REXMT] == 0) &&
					(tp->t_timer[TCPT_PERSIST] == 0)) {
					tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
					ENQ_REXMT(tp);
					tp->t_rxtshift = 0;
				} 
				error = 0;
			} 
		}
	}
	return (tp);
}

/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
struct tcpcb *
tcp_usrclosed(tp)
	register struct tcpcb *tp;
{

	switch (tp->t_state) {

	case TCPS_SYN_SENT:
		MIB_tcpIncrCounter(ID_tcpAttemptFails);
	case TCPS_CLOSED:
	case TCPS_LISTEN:
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_CLOSED,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_CLOSED;
		tp = tcp_close(tp);
		break;

	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_FIN_WAIT_1,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_FIN_WAIT_1;
		break;

	case TCPS_CLOSE_WAIT:
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_LAST_ACK,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_LAST_ACK;
		break;
	}
	if (tp && tp->t_state >= TCPS_FIN_WAIT_2)
		soisdisconnected(tp->t_inpcb->inp_socket);
	return (tp);
}

/*
 * Accept a connection.
 * Set state to SYN_RECEIVED, output SYN and ACK:
 * if in CLOSED state, indirectly, via tcp_input();
 * if in SYN_HOLD state, directly, via tcp_output().
 */
void
tcp_acceptconn(so)
	struct socket *so;
{
	struct inpcb *inp, *inp_head;
	register struct tcpcb *tp, *tp_head;
	struct socket *so_head;

	inp = (struct inpcb *)so->so_pcb;
	tp = intotcpcb(inp);
	tp->t_flags |= TF_ACCEPTCONN;

	so_head = so->so_head;
	if (so_head) {
	   inp_head = (struct inpcb *)so_head->so_pcb;
	   tp_head = intotcpcb(inp_head);
	   tp->t_flags |= tp_head->t_flags;
        }

	if (tp->t_state == TCPS_SYN_HOLD) {
		tp->t_flags |= TF_ACKNOW;
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_SYN_RECEIVED,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
		tcpstat.tcps_accepts++;
		(void) tcp_output(tp);
		/* Dont bother about ENOBUFS here */
	} else if (tp->t_state != TCPS_CLOSED) {
		panic("tcp_acceptconn");
	}
}

/*
 * Reject a connection.
 * Just say no.
 */
void
tcp_rejectconn(tp)
	register struct tcpcb *tp;
{
	if (tp->t_state == TCPS_SYN_HOLD) {
		tcp_drop(tp, 0);
		tcpstat.tcps_rejects++;
	} else {
		panic("tcp_rejectconn");
	}
}

/*
 * update our peer's window information; taken from tcp_output as
 * a performance optimization (i.e. the only time its called is when
 * the user recieves data -- no need to call tcp_output unless the
 * window really needs to be updated.  We save some instructions in
 * the case where its not.
 */

tcp_window_update(tp)
struct tcpcb *tp;
{
	struct socket *so = tp->t_inpcb->inp_socket;
	int	win;

	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;
	win = sbspace(&so->so_rcv);
	/*
	 * Compare available window to amount of window
	 * known to peer (as advertised window less
	 * next expected input).  If the difference is at least two
	 * max size segments or at least 35% of the maximum possible
	 * window, then want to send a window update to peer.
	 */
	if (win > 0) {
		int adv = win - (tp->rcv_adv - tp->rcv_nxt);

		if( adv > 0 ){ /* for the problem found in 8.0 */
			if (so->so_rcv.sb_cc == 0 && adv >= (tp->t_maxseg << 1))
				goto sendwin;
		/*
		 * Changing the following if statement from
		 * 100*a/b >= 35 to 100*a >= 35*b wins us 
		 * 69 instructions! -- Subbu 6/19/89.
		 * Changing 35 into 33 and 1/3 wins us 
		 * another 4 instructions. Worth it? XXX
		 */
/***************************************************************
		if ((100 * adv) >= (35 * so->so_rcv.sb_hiwat))
			goto sendwin;
***************************************************************/

			if ((adv + (adv << 1)) >= so->so_rcv.sb_hiwat)
				goto sendwin;
		} /* if adv */
	}

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are:
	 *	idle			not doing retransmits or persists
	 *	persisting		to move a small or zero window
	 *	(re)transmitting	and thereby not persisting
	 *
	 * tp->t_timer[TCPT_PERSIST]
	 *	is set when we are in persist state.
	 * tp->t_flags & TF_FORCE
	 *	is set when we are called to send a persist packet.
	 * tp->t_timer[TCPT_REXMT]
	 *	is set when we are retransmitting
	 * The output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state.
	 * If nothing happens soon, send when timer expires:
	 * if window is nonzero, transmit what we can,
	 * otherwise force out a byte.
	 */
	if (so->so_snd.sb_cc && tp->t_timer[TCPT_REXMT] == 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}
	return;
sendwin:
	tp->t_flags |= TF_WINUPDATE;
send:
	(void)tcp_output(tp);
}
