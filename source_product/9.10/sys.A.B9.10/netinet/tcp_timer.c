/* $Header: tcp_timer.c,v 1.22.83.4 93/09/17 19:06:04 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/tcp_timer.c,v $
 * $Revision: 1.22.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:06:04 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) tcp_timer.c $Revision: 1.22.83.4 $";
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
 *	@(#)tcp_timer.c	7.14 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"

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
#include "../net/netmp.h"

int	tcp_keepidle = TCPTV_KEEP_IDLE;
int	tcp_keepintvl = TCPTV_KEEPINTVL;
int	tcp_maxidle;
/*
 * Fast timeout routine for processing delayed acks
 */
tcp_fasttimo()
{
	register struct inpcb *inp;
	register struct tcpcb *tp,*next;
	int s;

	NET_SPLNET(s);
	tp = tcpackq.acknext;
	if (tp != &tcpackq) {
		for (; tp != &tcpackq; tp = next) {
			/* 
			 * Save acknext.  acknext zeroed by tcp_output().
			 */
			next = tp->acknext;		
			tp->t_flags &= ~TF_DELACK;
			tp->t_flags |= TF_ACKNOW;
			tcpstat.tcps_delack++;
			(void) tcp_output(tp);
			/* Don't bother with ENOBUFS here */
		}
	}
	NET_SPLX(s);
}

/*
 * HP : Split BSD's tcp_slowtimo() into 2 routines : 
 *		tcp_slowtimo() & tcp_slowtimo2().
 *
 *	tcp_slowtimo() will take care of timers < TIMQLEN and is called
 *		every 500 ms.
 *	tcp_slowtimo2() will take care of timers >= TIMQLEN and is called
 *		by tcp_slowtimo() each time tq_bk wraps around to 0.
 *              ie, each time we are servicing bucket 0 of Timer Queue.
 *              This is equivalent to every TIMQLEN * 500 ms.
 *
 * Other Changes :
 *
 *	Maintain a global_rtt instead of incrementing t_rtt of
 *	individual tcpcb's.
 */
tcp_slowtimo()
{
	register struct inpcb *inp, *inpnxt;
	register struct tcpcb *next;
	register struct tcpcb *tp;
	register int i;
	int s;

	global_rtt ++;
	if (global_rtt < 0)
		global_rtt = 0;
	/*
	 * HP : Timer Queue for slowtimo 
	 * 	Process Timer Queue in round-robin fashion
	 */
	tq_bk = ++tq_bk & (TIMQLEN-1);

	NET_SPLNET(s);
	tp = tcptimq[tq_bk].tim_next;
	if (tp == (struct tcpcb *) &tcptimq[tq_bk]) {
		NET_SPLX(s);
		if (tq_bk == 0) 		
			tcp_slowtimo2();
		return;
	}
	for (; tp != (struct tcpcb *) &tcptimq[tq_bk]; tp = next) {
		next 	= tp->tim_next;
		inp  	= tp->t_inpcb;
		inpnxt	= inp->inp_next;

		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] && tp->t_timer[i]< TIMQLEN) {
				(void) tcp_usrreq(tp->t_inpcb->inp_socket,
				    PRU_SLOWTIMO, (struct mbuf *)0,
				    (struct mbuf *)i, (struct mbuf *)0);
				if (inpnxt->inp_prev != inp)
					goto tpnone;
			}
		}
		deq(tp, tim_next, tim_prev);
tpnone:
		;
	}
	tcp_iss += TCP_ISSINCR/PR_SLOWHZ;		/* increment iss */
#ifdef TCP_COMPAT_42
	if ((int)tcp_iss < 0)
		tcp_iss = 0;				/* XXX */
#endif
	NET_SPLX(s);
	if (tq_bk == 0) 		
		tcp_slowtimo2();
	return;
}
/*
 * Tcp protocol timeout routine called every TIMQLEN * 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */
tcp_slowtimo2()
{
	register struct inpcb *ip, *ipnxt;
	register struct tcpcb *tp;
	register int i;
	int s;
	int	indx;
	struct tcpcb *timqp;

	NET_SPLNET(s);
	/*
	 * Search through tcb's and update active timers.
	 */
	ip = tcb.inp_next;
	if (ip == 0) {
		NET_SPLX(s);
		return;
	}
	for (; ip != &tcb; ip = ipnxt) {
	    ipnxt = ip->inp_next;
	    tp = intotcpcb(ip);
	    if (tp == 0)
		continue;
	    /*
	     * If timer already queued, wait for tcp_slowtimo() handle it.
	     */
	    for (i = 0; i < TCPT_NTIMERS; i++) {
		if (tp->t_timer[i]) {
			if (tp->t_timer[i] <= TIMQLEN) {
			    if (tp->tim_next == 0) {
				(void) tcp_usrreq(tp->t_inpcb->inp_socket,
				    PRU_SLOWTIMO, (struct mbuf *)0,
				    (struct mbuf *)i, (struct mbuf *)0);
				if (ipnxt->inp_prev != ip)
					goto tpgone;
			    }
			}
			else {
			    tp->t_timer[i] -= TIMQLEN;
			    if ((tp->t_timer[i] < TIMQLEN) && (tp->tim_next==0)) {
				indx = (tq_bk + tp->t_timer[i]) & (TIMQLEN-1);
				timqp = (struct tcpcb *) &tcptimq[indx];
				enq(tp, timqp, tim_next, tim_prev);
			    }
			}
		}
	    }
		tp->t_idle += TIMQLEN;
tpgone:
		;
	}
	NET_SPLX(s);
}

/*
 * Cancel all timers for TCP tp.
 */
tcp_canceltimers(tp)
	struct tcpcb *tp;
{
	register int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

int	tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };

/*
 * TCP timer processing.
 */
struct tcpcb *
tcp_timers(tp, timer)
	register struct tcpcb *tp;
	int timer;
{
	register int rexmt;
	int	indx;
	struct tcpcb *timqp;

	switch (timer) {

	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 *
	 * HP:  We set 2MSL to tcp_maxidle in tcp_input when we move into
	 * FINWAIT-2 after application calls close (not shutdown).  The
	 * following code assumes that tcp_maxidle >= 2MSL; otherwise, we
	 * fail to enter TIME-WAIT for 2MSL, per RFC.  This assumption might
	 * be violated if/when we support configurable keepalive variables
	 * (and it can be violated with adb; not so uncommon).
	 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT &&
		    tp->t_idle <= tcp_maxidle)
			tp->t_timer[TCPT_2MSL] = tcp_keepintvl;
		else
			tp = tcp_close(tp);
		break;

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	case TCPT_REXMT:
		if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp->t_rxtshift = TCP_MAXRXTSHIFT;
			tcpstat.tcps_timeoutdrop++;
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		}
		tcpstat.tcps_rexmttimeo++;
		rexmt = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
		rexmt *= tcp_backoff[tp->t_rxtshift];
		TCPT_RANGESET(tp->t_rxtcur, rexmt, TCPTV_MIN, TCPTV_REXMTMAX);
		tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		ENQ_REXMT(tp);
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
			in_losing(tp->t_inpcb);
			tp->t_rttvar += (tp->t_srtt >> 2);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
		u_int win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		}
		(void) tcp_output(tp);
		/* Retransmit timer is already running. Dont need to check
		   for ENOBUFS */
		break;

	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
	case TCPT_PERSIST:
		tcpstat.tcps_persisttimeo++;
		tcp_setpersist(tp);
		tp->t_flags |= TF_FORCE;
		(void) tcp_output(tp);
		/* Persist timer is already running. Dont need to check
		   for ENOBUFS */
		tp->t_flags &= ~TF_FORCE;
		break;

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
	case TCPT_KEEP:
		tcpstat.tcps_keeptimeo++;
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;

		/* HP
		 * Send keepalives through FINWAIT-2, not just CLOSE-WAIT.
		 * This avoids keeping endpoint in FINWAIT-1 when route
		 * breaks before application calls close.  Also avoids
		 * keeping endpoint in FINWAIT-2 when route breaks after
		 * shutdown(how==1[send-only]).
		 *
		 * Some risk in sending probes in these states because we
		 * differ from de facto std (BSD).  However, they are
		 * important in shutdown case because application is trying
		 * to effect verifiable grace close (e.g., call shutdown,
		 * then select).
		 */

		if (tp->t_inpcb->inp_socket->so_options & SO_KEEPALIVE &&
		    tp->t_state <= TCPS_FIN_WAIT_2) {
		    	if (tp->t_idle >= tcp_keepidle + tcp_maxidle)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			tcpstat.tcps_keepprobe++;
#ifdef TCP_COMPAT_42
			/*
			 * The keepalive packet must have nonzero length
			 * to get a 4.2 host to respond.
			 */
			tcp_respond(tp, tp->t_template,
			    tp->rcv_nxt - 1, tp->snd_una - 1, 0);
#else
			tcp_respond(tp, tp->t_template,
			    tp->rcv_nxt, tp->snd_una - 1, 0);
#endif
			tp->t_timer[TCPT_KEEP] = tcp_keepintvl;
			/* 
			 * HP : Timer Queue.  REXMT timer has precedence over
			 *	KEEP timer.
			 */
			if ((tp->t_timer[TCPT_KEEP] < TIMQLEN) && (tp->tim_next==0)) {
				indx = (tq_bk + tp->t_timer[TCPT_KEEP]) & (TIMQLEN-1);
				timqp = (struct tcpcb *) &tcptimq[indx];
				enq(tp, timqp, tim_next, tim_prev);
			}
		} else
			tp->t_timer[TCPT_KEEP] = tcp_keepidle;
		break;
	dropit:
		tcpstat.tcps_keepdrops++;
		tp = tcp_drop(tp, ETIMEDOUT);
		break;
	}
	return (tp);
}
