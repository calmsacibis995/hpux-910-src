/*
 * $Header: tcp_output.c,v 1.52.83.5 93/10/27 13:40:22 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/tcp_output.c,v $
 * $Revision: 1.52.83.5 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 13:40:22 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) tcp_output.c $Revision: 1.52.83.5 $ $Date: 93/10/27 13:40:22 $";
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
 *	@(#)tcp_output.c	7.17 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/tcp.h"
#define	TCPOUTFLAGS
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"
#include "../netinet/tcp_debug.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../net/cko.h"
/*
 * Initial options.
 */
u_char	tcp_initopt[4] = { TCPOPT_MAXSEG, 4, 0x0, 0x0, };

/*
 * Tcp output routine: figure out what should be sent and send it.
 */
tcp_output(tp)
	register struct tcpcb *tp;
{
	register struct socket *so = tp->t_inpcb->inp_socket;
	register long win;
	long len;
	struct mbuf *m0;
	int off, flags, error;
	int forced_zero_len = 0;
	register struct mbuf *m;
	register struct tcpiphdr *ti;
	u_char *opt;
	unsigned optlen = 0;
	int idle, sendalot;
	int	eom=0;
	void	nipc_sbmsgsize();
	struct mbuf	*nipc_m_copy();
	int	indx;
	struct tcpcb *timqp;
	int	iplen;
	struct inpcb *inp = tp->t_inpcb;

	/*
	 * Determine length of data that should be transmitted,
	 * and flags that will be used.
	 * If there is some data or critical controls (SYN, RST)
	 * to send, then transmit; otherwise, investigate further.
	 */
	idle = (tp->snd_max == tp->snd_una);
again:
	sendalot = 0;
	off = tp->snd_nxt - tp->snd_una;
	win = MIN(tp->snd_wnd, tp->snd_cwnd);

	/*
	 * If in persist timeout with window of 0, send 1 byte.
	 * Otherwise, if window is small but nonzero
	 * and timer expired, we will send what we can
	 * and go to transmit state.
	 */
	if (tp->t_flags & TF_FORCE) {
		if (win == 0)
			win = 1;
		else {
			tp->t_timer[TCPT_PERSIST] = 0;
			tp->t_rxtshift = 0;
		}
	}

	len = MIN(so->so_snd.sb_cc, win) - off;
	flags = tcp_outflags[tp->t_state];

	/* possible shrink the len if in message mode */
	if ((tp->t_flags & TF_MSGMODE) && (len > 0))
		nipc_sbmsgsize(&so->so_snd, off, (int)len, (int *)&len, &eom);

	if (len < 0) {
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be -1.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit and pull snd_nxt
		 * back to (closed) window.  We will enter persist
		 * state below.  If the window didn't close completely,
		 * just wait for an ACK.
		 */
		len = 0;
		if (win == 0) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->snd_nxt = tp->snd_una;
		}
	}
	if (len > tp->t_maxseg) {
		len = tp->t_maxseg;
		sendalot = 1;
		eom=0;	/* in case we're shrinking a message mode message*/
	}

	/*
	 * If FIN is set, we want to reset it if we are retransmitting
	 * a segment before FIN. An optimization by checking for the FIN
	 * flag instead of computing the expression -- Subbu 6/6/89.
	 */
	if ((flags & TH_FIN) &&
	     SEQ_LT(tp->snd_nxt + len, tp->snd_una + so->so_snd.sb_cc))
		flags &= ~TH_FIN;

	/*
	 * Send if we owe peer an ACK.
	 */
	if (tp->t_flags & (TF_ACKNOW|TF_WINUPDATE))
		goto send;

	/*
	 * Sender silly window avoidance.  If connection is idle
	 * and can send all data, a maximum segment,
	 * at least a maximum default-size segment do it,
	 * or are forced, do it; otherwise don't bother.
	 * If peer's buffer is tiny, then send
	 * when window is at least half open.
	 * If retransmitting (possibly after persist timer forced us
	 * to send into a small window), then must resend.
	 * An optimization by moving this ahead of other checks.
	 * - Subbu (6/6/89).
	 * If in message mode then send messages regardless of size.
	 */
	if (len) {
		if (len == tp->t_maxseg)
			goto send;
		if ((idle || tp->t_flags & TF_NODELAY) &&
		    len + off >= so->so_snd.sb_cc)
			goto send;
		if (tp->t_flags & TF_FORCE)
			goto send;
		if (len >= (tp->max_sndwnd >> 1))
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
		if (tp->t_flags & TF_MSGMODE)
			goto send;
	}

	/*
	 * If our state indicates that FIN should be sent
	 * and we have not yet done so, or we're retransmitting the FIN,
	 * then we need to send. An optimization by comparing the FIN
	 * flag first instead of computing the expression. Moreover,
	 * this is not a common case. So we move it down. -- Subbu 6/13/89.
	 */
	if (flags & TH_FIN &&
	    ((tp->t_flags & TF_SENTFIN) == 0 || tp->snd_nxt == tp->snd_una))
		goto send;

	if (flags & (TH_SYN|TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;

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

	/*
	 * No reason to send a segment, just return.
	 */
	return (0);

send:
	win = sbspace(&so->so_rcv);
	/*
	 * Grab a header mbuf, attaching a copy of data to
	 * be transmitted, and initialize the header from
	 * the template for sends on this connection.
	 */
	MGET(m, M_DONTWAIT, MT_HEADER);
	/* If the mbuf for the header could not be obtained let
	   the caller handle the error */
	if (m == NULL) return(ENOBUFS);
#define	DATASPACE  (MLEN - (IN_MAXLINKHDR + sizeof(struct tcpiphdr)))
	m->m_off = MMINOFF + IN_MAXLINKHDR;
	m->m_len = sizeof (struct tcpiphdr);
	/*
	 * The common case of sending a packet is being reached after
	 * the two IFs here. -- Subbu.
	 */
	if (len) {
		if ((tp->t_flags & TF_FORCE) && len == 1)
			tcpstat.tcps_sndprobe++;
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			tcpstat.tcps_sndrexmitpack++;
			tcpstat.tcps_sndrexmitbyte += len;
			MIB_tcpIncrCounter(ID_tcpRetransSegs);
		} else {
			tcpstat.tcps_sndpack++;
			tcpstat.tcps_sndbyte += len;
			MIB_tcpIncrCounter(ID_tcpOutSegs);
		}
		if (tp->t_flags & TF_MSGMODE) {
			if (len <= DATASPACE) {
				nipc_m_copydata(so->so_snd.sb_mb,off, (int) len,
				    mtod(m, caddr_t) + sizeof(struct tcpiphdr));
				m->m_len += len;
			} else {
				m->m_next = nipc_m_copy(so->so_snd.sb_mb, 
							off, (int) len);
				if (m->m_next == 0) {
					forced_zero_len = 1;
					len = 0;
					sendalot = 0; /* dont loop back, etc */
				}
			}
		}
		else {
			if (len <= DATASPACE) {
				m_copydata(so->so_snd.sb_mb, off, (int) len,
				    mtod(m, caddr_t) + sizeof(struct tcpiphdr));
				m->m_len += len;
			} else {
				m->m_next = m_copy(so->so_snd.sb_mb, 
							off, (int) len);
				if (m->m_next == 0) {
					forced_zero_len = 1;
					len = 0;
					sendalot = 0; /* dont loop back, etc */
				}
			}
		}
	} else if (tp->t_flags & TF_ACKNOW)
		tcpstat.tcps_sndacks++;
	else if (flags & (TH_SYN|TH_FIN|TH_RST))
		tcpstat.tcps_sndctrl++;
	else if (SEQ_GT(tp->snd_up, tp->snd_una))
		tcpstat.tcps_sndurg++;
	else
		tcpstat.tcps_sndwinup++;

	ti = mtod(m, struct tcpiphdr *);
	if (tp->t_template == 0)
		panic("tcp_output");
	bcopy((caddr_t)tp->t_template, (caddr_t)ti, sizeof (struct tcpiphdr));

	/*
	 * Fill in fields, remembering maximum advertised
	 * window for use in delaying messages about window sizes.
	 * If resending a FIN, be sure not to use a new sequence number.
	 */
	if (flags & TH_FIN && tp->t_flags & TF_SENTFIN && 
	    tp->snd_nxt == tp->snd_max)
		tp->snd_nxt--;
	ti->ti_seq = htonl(tp->snd_nxt);
	ti->ti_ack = htonl(tp->rcv_nxt);
	/*
	 * Before ESTABLISHED, force sending of initial options
	 * unless TCP set to not do any options.
	 */
	opt = NULL;
	if (flags & TH_SYN && (tp->t_flags & TF_NOOPT) == 0) {
		u_short mss;

		mss = MIN((so->so_rcv.sb_hiwat >> 1), tcp_mss(tp));
		if (mss > IP_MSS - sizeof(struct tcpiphdr)) {
			opt = tcp_initopt;
			optlen = sizeof (tcp_initopt);
			*(u_short *)(opt + 2) = htons(mss);
		}
	}
	if (opt) {
		m0 = m->m_next;
		m->m_next = m_get(M_DONTWAIT, MT_DATA);
		if (m->m_next == 0) {
			(void) m_free(m);
			m_freem(m0);
			return (ENOBUFS);
		}
		m->m_next->m_next = m0;
		m0 = m->m_next;
		m0->m_len = optlen;
		bcopy((caddr_t)opt, mtod(m0, caddr_t), optlen);
		opt = (u_char *)(mtod(m0, caddr_t) + optlen);
		while (m0->m_len & 0x3) {
			*opt++ = TCPOPT_EOL;
			m0->m_len++;
		}
		optlen = m0->m_len;
		ti->ti_off = (sizeof (struct tcphdr) + optlen) >> 2;
	}
	ti->ti_flags = flags;
	/*
	 * Calculate receive window.  Don't shrink window,
	 * but avoid silly window syndrome.
	 */
	if (win < (long)(so->so_rcv.sb_hiwat >> 2) && win < (long)tp->t_maxseg)
		win = 0;
	if (win > IP_MAXPACKET)
		win = IP_MAXPACKET;
	if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
		win = (long)(tp->rcv_adv - tp->rcv_nxt);
	ti->ti_win = htons((u_short)win);
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		ti->ti_urp = htons((u_short)(tp->snd_up - tp->snd_nxt));
		ti->ti_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull
		 * the urgent pointer to the left edge of the send window
		 * so that it doesn't drift into the send window on sequence
		 * number wraparound.
		 */
 		if (SEQ_GT(tp->snd_una, tp->snd_up))
 			tp->snd_up = tp->snd_una;	/* drag it along */
	/*
	 * If anything to send and we can send it all, set PUSH.
	 * (This will keep happy those implementations which only
	 * give data to the user when a buffer fills or a PUSH comes in.)
	 */
	if (tp->t_flags & TF_MSGMODE) {
		if (eom)
			ti->ti_flags |= TH_PUSH;
		else
			ti->ti_flags &= ~TH_PUSH;
	}
	else if (len && off+len == so->so_snd.sb_cc)
		ti->ti_flags |= TH_PUSH;

	/*
	 * Put TCP length in extended header, and then
	 * checksum extended header and data.
	 */
	if (len + optlen)
		ti->ti_len = htons((u_short)(sizeof(struct tcphdr) +
		    optlen + len));
	/* HP : checksum assist
	 *
	 *	The casting (struct tcpiphdr *)0 will generate the
	 *	proper offset for ti_sum.  This is an HP-PA feature
	 *	used in genassym.c.
	 */
	iplen = sizeof (struct tcpiphdr) + (int)optlen + len;
	cko_cksum(m, iplen, inp, &((struct tcpiphdr *)0)->ti_sum, 
			CKO_ALGO_TCP|CKO_INSERT);
	/*
	 * In transmit state, time the transmission and arrange for
	 * the retransmit.  In persist state, just set snd_max.
	 */
	if ((tp->t_flags & TF_FORCE) == 0 || tp->t_timer[TCPT_PERSIST] == 0) {
		tcp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment.
		 */
		if (flags & TH_SYN)
			tp->snd_nxt++;
		if (flags & TH_FIN) {
			tp->snd_nxt++;
			tp->t_flags |= TF_SENTFIN;
		}
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (tp->t_rtt == 0) {
				/*
	 			 * HP : Use global_rtt to avoid incrementing 
				 * 	t_rtt of each individual tcpcb.  
				 *	See comment in tcp_xmit_timer().
	 			 */
				tp->t_rtt = global_rtt;
				tp->t_rtseq = startseq;
				tcpstat.tcps_segstimed++;
			}
		}

		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if ((tp->t_timer[TCPT_REXMT] == 0) &&
		    ((tp->snd_nxt != tp->snd_una) || (forced_zero_len))) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			ENQ_REXMT(tp);
			if (tp->t_timer[TCPT_PERSIST]) {
				tp->t_timer[TCPT_PERSIST] = 0;
				tp->t_rxtshift = 0;
			}
		}
	} else
		if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
			tp->snd_max = tp->snd_nxt + len;

	/*
	 * Trace.
	 */
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_OUTPUT, tp->t_state, tp, ti, 0);

	/*
	 * Fill in IP length and desired time to live and
	 * send to IP level.
	 */
	/* HP : checksum assist */
	((struct ip *)ti)->ip_len = iplen;
	((struct ip *)ti)->ip_ttl = tp->t_inpcb->inp_ttl;
	error = ip_output(m, tp->t_inpcb->inp_options, &tp->t_inpcb->inp_route,
	    so->so_options & SO_DONTROUTE);
	if (error) {
		if (error == ENOBUFS) {
			tcp_quench(tp->t_inpcb);
			return (0);
		}
		return (error);
	}
	tcpstat.tcps_sndtotal++;

	/*
	 * Data sent (as far as we can tell).
	 * If this advertises a larger window than any other segment,
	 * then remember the size of the advertised window.
	 * Any pending ACK has now been sent.
	 */
	if (win > 0 && SEQ_GT(tp->rcv_nxt+win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->t_flags &= ~(TF_ACKNOW|TF_DELACK|TF_WINUPDATE);
	if (sendalot)
		goto again;
	/* 
	 * HP : If in delayed ack queue, delink and clear next pointer.
	 */
	if (tp->acknext) 
		deq(tp, acknext, ackprev);
	return (0);
}

tcp_setpersist(tp)
	register struct tcpcb *tp;
{
	register t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
	int	indx;
	struct tcpcb *timqp;

	if (tp->t_timer[TCPT_REXMT])
		panic("tcp_output REXMT");
	/*
	 * Start/restart persistance timer.
	 */
	TCPT_RANGESET(tp->t_timer[TCPT_PERSIST],
	    t * tcp_backoff[tp->t_rxtshift],
	    TCPTV_PERSMIN, TCPTV_PERSMAX);
	/*
	 * HP : Timer Queue.  REXMT/PERSIST timer have precedence over
	 *	KEEP, 2MSL timer.
	 */
	if (tp->t_timer[TCPT_PERSIST] < TIMQLEN) {
		indx = (tq_bk + tp->t_timer[TCPT_PERSIST]) & (TIMQLEN-1);
		timqp = (struct tcpcb *) &tcptimq[indx];
		if (tp->tim_next)
			deq(tp, tim_next, tim_prev);
		enq(tp, timqp, tim_next, tim_prev);
	}
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}
