/*
 * $Header: tcp_input.c,v 1.81.83.8 94/01/06 10:53:23 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/tcp_input.c,v $
 * $Revision: 1.81.83.8 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 94/01/06 10:53:23 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) tcp_input.c $Revision: 1.81.83.8 $ $Date: 94/01/06 10:53:23 $";
#endif

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)tcp_input.c	7.19 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/cko.h"

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

#include "../h/subsys_id.h"
#include "../h/net_diag.h"

#undef insque
#undef remque

int	tcpprintfs = 0;
int	tcpcksum = 1;
int	tcprexmtthresh = 3;
int	tcppreddat = 0;
int	tcppredack = 0;
struct	tcpiphdr tcp_saveti;

struct	tcpcb *tcp_newtcpcb();

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 * Set DELACK for segments received in order, but ack immediately
 * when segments are out of order (so fast retransmit can work).
 *
 * HP INDaa09596: add check of SS_CANTRCVMORE flag
 */
#define	TCP_REASS(tp, ti, m, so, flags) { 			\
	if ((ti)->ti_seq == (tp)->rcv_nxt && 			\
	    (tp)->seg_next == (struct tcpiphdr *)(tp) && 	\
	    (tp)->t_state == TCPS_ESTABLISHED && 		\
	    (((so)->so_state & SS_CANTRCVMORE) == 0) && 	\
	    (!((tp)->t_flags & TF_MSGMODE))) { 			\
		(tp)->t_flags |= TF_DELACK; 			\
		if (!(tp)->acknext) 				\
			enq((tp), ackqp, acknext, ackprev);	\
		(tp)->rcv_nxt += (ti)->ti_len;  		\
		flags = (ti)->ti_flags & TH_FIN; 		\
		tcpstat.tcps_rcvpack++;				\
		tcpstat.tcps_rcvbyte += (ti)->ti_len;		\
		sbappend(&(so)->so_rcv, (m)); 			\
		sorwakeup(so); 					\
	} else { 						\
		(flags) = tcp_reass((tp), (ti)); 		\
		tp->t_flags |= TF_ACKNOW; 			\
	} 							\
}

tcp_reass(tp, ti)
	register struct tcpcb *tp;
	register struct tcpiphdr *ti;
{
	register struct tcpiphdr *q;
	struct socket *so = tp->t_inpcb->inp_socket;
	struct mbuf *m;
	int flags;
	void	nipc_sbappendmsg();

	/*
	 * Call with ti==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (ti == 0)
		goto present;

	/*
	 * Find a segment which begins after this one does.
	 */
	for (q = tp->seg_next; q != (struct tcpiphdr *)tp;
	    q = (struct tcpiphdr *)q->ti_next)
		if (SEQ_GT(q->ti_seq, ti->ti_seq))
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if ((struct tcpiphdr *)q->ti_prev != (struct tcpiphdr *)tp) {
		register int i, j;
		q = (struct tcpiphdr *)q->ti_prev;
		/* conversion to int (in i) handles seq wraparound */
		i = q->ti_seq + q->ti_len - ti->ti_seq;
		j = i;

		/* If there is OOB data in the dropped area, 
		 * take care to reduce the above count by 1
		 * because the urgent byte has already been pulled
		 * off. Also, mark this segment as not having URG
		 * flag set. -- HP 3/27/89.
		 */
		if ((ti->ti_flags & TH_URG) &&
		    (ti->ti_urp) &&
		    (ti->ti_urp <= ti->ti_len) &&
		    (ti->ti_urp <= i)) {
			j = i-1;	/* j should be used to remove bytes */
			ti->ti_flags &= ~TH_URG;
			ti->ti_urp = 0;
		}
		if (i > 0) {
			if (i >= ti->ti_len) {
				tcpstat.tcps_rcvduppack++;
				tcpstat.tcps_rcvdupbyte += ti->ti_len;
				goto drop;
			}
			m_adj(dtom(ti), j);
			ti->ti_len -= i;
			ti->ti_seq += i;
		}
		q = (struct tcpiphdr *)(q->ti_next);
	}
	tcpstat.tcps_rcvoopack++;
	tcpstat.tcps_rcvoobyte += ti->ti_len;

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q != (struct tcpiphdr *)tp) {
 		register int i,j;

		i = j = (ti->ti_seq + ti->ti_len) - q->ti_seq;
		if (i <= 0)
			break;
		if (i < q->ti_len) {
			/* If there is OOB data in the dropped area, 
		 	 * take care to reduce the above count by 1
		 	 * because the urgent byte has already been pulled
		 	 * off. Also, mark this segment as not having URG
		 	 * flag set. -- HP 3/27/89.
		 	 */
			if ((q->ti_flags & TH_URG) &&
			    (q->ti_urp) &&
			    (q->ti_urp <= q->ti_len) &&
			    (q->ti_urp <= i)) {
				j = i - 1;
				q->ti_flags &= ~TH_URG;
				q->ti_urp = 0;
			}
			q->ti_seq += i;
			q->ti_len -= i;
			m_adj(dtom(q), j);
			break;
		}
		q = (struct tcpiphdr *)q->ti_next;
		m = dtom(q->ti_prev);
		remque(q->ti_prev);
		m_freem(m);
	}

	/*
	 * Stick new segment in its place.
	 */
	insque(ti, q->ti_prev);

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVERCVDSYN(tp->t_state) == 0)
		return (0);
	ti = tp->seg_next;
	if (ti == (struct tcpiphdr *)tp || ti->ti_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && ti->ti_len)
		return (0);
	do {
		tcpstat.tcps_rcvpack++;
		tcpstat.tcps_rcvbyte += ti->ti_len;
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags;
		remque(ti);
		m = dtom(ti);
		ti = (struct tcpiphdr *)ti->ti_next;
		if (so->so_state & SS_CANTRCVMORE)
			m_freem(m);
		else {
			if (tp->t_flags & TF_MSGMODE)
				nipc_sbappendmsg(&so->so_rcv, m,
					flags & TH_PUSH);
			else
				sbappend(&so->so_rcv, m);
		}
	} while (ti != (struct tcpiphdr *)tp && ti->ti_seq == tp->rcv_nxt);
	sorwakeup(so);
	return (flags & TH_FIN);
drop:
	m_freem(dtom(ti));
	return (0);
}

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
tcp_input(m0)
	struct mbuf *m0;
{
	register struct tcpiphdr *ti;
	struct inpcb *inp;
	register struct mbuf *m;
	struct mbuf *om = 0;
	struct mbuf *m1 = 0;
	int len, tlen, off, error;
	register struct tcpcb *tp = 0;
	register int tiflags;
	struct socket *so;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate;
	struct in_addr laddr;
	int dropsocket = 0;
	int iss = 0;
 	int nvs_return = 0;
	u_short rcvdcksum;	/* for netipc optional checksumming */
	int	hash;		/* HP - hash inpcb's for TCP */
	struct inpcb *hashqp;
	struct tcpcb *ackqp=&tcpackq;
	int	indx;
	struct tcpcb *timqp;
	unsigned int	comp_sum;

	tcpstat.tcps_rcvtotal++;
	MIB_tcpIncrCounter(ID_tcpInSegs);
	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */
	m = m0;
	ti = mtod(m, struct tcpiphdr *);
	if (((struct ip *)ti)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ti, (struct mbuf *)0);
	if (m->m_off > MMAXOFF || m->m_len < sizeof (struct tcpiphdr)) {
		if ((m = m_pullup(m, sizeof (struct tcpiphdr))) == 0) {
			tcpstat.tcps_rcvshort++;
			MIB_tcpIncrCounter(ID_tcpInErrs);
			return;
		}
		ti = mtod(m, struct tcpiphdr *);
	}

	/*
	 * Checksum extended TCP header and data.
	 * Allow optional checksum for SYN packets only.
	 */
	tlen = ((struct ip *)ti)->ip_len;
	len = sizeof (struct ip) + tlen;
	if (tcpcksum) {
		ti->ti_next = ti->ti_prev = 0;
		ti->ti_x1 = 0;
		ti->ti_len = (u_short)tlen;
		ti->ti_len = htons((u_short)ti->ti_len);
		/* HP : checksum assist */
		if (m->m_flags & MF_CKO_IN) {
			CKO_PSEUDO(comp_sum, m, ti);
		 } else {
			comp_sum = in_cksum(m, len);
			for (m1 = m; m1; m1 = m1->m_next)
				m1->m_flags &= ~MF_NOACC;
		}

		if (comp_sum) {
			if ((ti->ti_flags & TH_SYN) &&
			    (!(ti->ti_flags & TH_ACK))) {
				rcvdcksum = ti->ti_sum;
				ti->ti_sum = 0;
				ti->ti_sum = in_cksum(m, len);
				if ((ti->ti_sum != 0 && rcvdcksum == 0) ||
			     	    (ti->ti_sum == 0 && rcvdcksum == 0xffff)) {
					if (m->m_flags & MF_CKO_IN) {
					   for (m1 = m; m1; m1 = m1->m_next)
						   m1->m_flags &= ~MF_NOACC;
                                        } 
					goto passed;
                                }
			}
			if (tcpprintfs)
				printf("tcp sum: src %x\n", ti->ti_src);
			tcpstat.tcps_rcvbadsum++;
			MIB_tcpIncrCounter(ID_tcpInErrs);
			goto drop;
		}
	}

passed:

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.
	 */
	off = ti->ti_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		if (tcpprintfs)
			printf("tcp off: src %x off %d\n", ti->ti_src, off);
		tcpstat.tcps_rcvbadoff++;
		MIB_tcpIncrCounter(ID_tcpInErrs);
		goto drop;
	}
	tlen -= off;
	ti->ti_len = tlen;
	if (off > sizeof (struct tcphdr)) {
		if (m->m_len < sizeof(struct ip) + off) {
			if ((m = m_pullup(m, sizeof (struct ip) + off)) == 0) {
				tcpstat.tcps_rcvshort++;
				MIB_tcpIncrCounter(ID_tcpInErrs);
				return;
			}
			ti = mtod(m, struct tcpiphdr *);
		}
		om = m_get(M_DONTWAIT, MT_DATA);
		if (om == 0)
			goto drop;
		om->m_len = off - sizeof (struct tcphdr);
		{ caddr_t op = mtod(m, caddr_t) + sizeof (struct tcpiphdr);
		  bcopy(op, mtod(om, caddr_t), (unsigned)om->m_len);
		  m->m_len -= om->m_len;
		  bcopy(op+om->m_len, op,
		   (unsigned)(m->m_len-sizeof (struct tcpiphdr)));
		}
	}
	tiflags = ti->ti_flags;


	/*
	 * Locate pcb for segment.
	 */
findpcb:
	/* 
	 * HP : hashed inpcb's for TCP.  
	 * 	If connected, we will find it in the hashed list.  
	 * 	If the inpcb is not found in the hashed list,
	 *    		then search the regular list of inpcb's.
	 */
	PCB_LOOK(tcphash, ti->ti_src, ti->ti_sport, ti->ti_dst, ti->ti_dport, 
		 hash, inp);
	if (inp ==0)
		inp = in_pcblookup (&tcb, ti->ti_src, ti->ti_sport, 
			ti->ti_dst, ti->ti_dport, INPLOOKUP_WILDCARD);
	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	if (inp == 0)
		goto dropwithreset;
	tp = intotcpcb(inp);
	if (tp == 0)
		goto dropwithreset;
	so = inp->inp_socket;

	/*
	 * Header prediction: check for the two common cases
	 * of a uni-directional data xfer.  If the packet has
	 * no control flags, is in-sequence, the window didn't
	 * change and we're not retransmitting, it's a
	 * candidate.  If the length is zero and the ack moved
	 * forward, we're the sender side of the xfer.  Just
	 * free the data acked & wake any higher level process
	 * that was blocked waiting for space.  If the length
	 * is non-zero and the ack didn't move, we're the
	 * receiver side.  If we're getting packets in-order
	 * (the reassembly queue is empty), add the data to
	 * the socket buffer and note that we need a delayed ack.
	 *
	 * HP INDaa09596: add check of SS_CANTRCVMORE flag
	 */
/* goto bypass; */
	if (tp->t_state == TCPS_ESTABLISHED &&
	   (tiflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ACK)) == TH_ACK &&
	    ti->ti_seq == tp->rcv_nxt && ti->ti_win == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max &&
	    so->so_ipc != SO_NETIPC ) {
		if (ti->ti_len == 0) {
			if (SEQ_GT(ti->ti_ack, tp->snd_una) &&
			    SEQ_LEQ(ti->ti_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd) {

			    /*
			     * this is a pure ack for outstanding data
			     * and we're not in the middle of slow-start
			     * or congestion avoidance.
			     */
				++tcppredack;
				if (tp->t_rtt && SEQ_GT(ti->ti_ack,tp->t_rtseq))
					tcp_xmit_timer (tp);
				sbdrop (&so->so_snd, ti->ti_ack - tp->snd_una);
				tp->snd_una = ti->ti_ack;
				m_freem (m);
				/*
				 * If all outstanding data is acked, stop the
				 * retransmit timer.  If there's no one
				 * waiting to output, let tcp_output decide
				 * between more output or persist.  Otherwise
				 * give the user a crack at the new space,
				 * assuming he'll call tcp_output when it's
				 * filled.  If there is more data to be acked,
				 * restart retransmit timer, using current
				 * (possibly backed-off) value.
				 */
				if (tp->snd_una == tp->snd_max)
					tp->t_timer[TCPT_REXMT] = 0;
				else if (tp->t_timer[TCPT_PERSIST] == 0) {
					tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
					ENQ_REXMT(tp);
				}

				sowwakeup(so);

				/*HP*
				 * call nvs_output only under certain dubious
				 * conditions, retained from earlier releases.
				 */
				if ((so->so_snd.sb_flags & SB_WAIT) == 0 && 
				    !so->so_snd.sb_sel && so->so_snd.sb_cc &&
				    (so->so_snd.sb_flags & SB_NVS_WAIT) &&
				    sbspace(&so->so_snd) >= MLEN) {
				   nvs_return = nvs_output(so->so_nvs_index);
				}

				/*HP*
				 * call tcp_output if there is outbound data,
				 * independent of SB_WAIT and sb_sel (cf.
				 * earlier releases).
				 *
				 * we must do this if we reset REXMT, above,
				 * because no timers are running now.
				 * probably also desirable to do when PERSIST
				 * is running because tcp_output might
				 * determine that send conditions have changed.
				 *
				 * in any case, calling tcp_output 
				 * unconditionally is consistent with BSD
				 * implementations, contrary to original VJ
				 * algorithm.
				 */
				if (nvs_return == 0  &&  so->so_snd.sb_cc) {
				   error = tcp_output (tp);
				   if (error == ENOBUFS) {
				      if ((tp->t_timer[TCPT_REXMT] == 0) &&
				          (tp->t_timer[TCPT_PERSIST] == 0)) {
				         tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
				         ENQ_REXMT(tp);
				         tp->t_rxtshift = 0;
				      } 
				   } 
				}

	 			/* Reset idle time and keep-alive timer. */
				tp->t_idle = 0;
				tp->t_timer[TCPT_KEEP] = tcp_keepidle;
				return;
			}
		}
		else if (ti->ti_ack == tp->snd_una &&
			 tp->seg_next == (struct tcpiphdr *)tp &&
			 ti->ti_len <= sbspace(&so->so_rcv) &&
			 ((so->so_state & SS_CANTRCVMORE) == 0)) {
				/*
				 * this is a pure, in-sequence data packet
				 * with nothing on the reassembly queue and
				 * we have enough buffer space to take it.
				 */
				++tcppreddat;
				tcpstat.tcps_rcvpack++;
				tcpstat.tcps_rcvbyte += ti->ti_len;
				tp->rcv_nxt += ti->ti_len;
				/*
				 * Drop TCP and IP headers then add data
				 * to socket buffer
				 */
				m->m_off += sizeof(struct tcpiphdr);
				m->m_len -= sizeof(struct tcpiphdr);
				sbappend(&so->so_rcv, m);
				sorwakeup(so);
				tp->t_flags |= TF_DELACK;
				/* 
				 * HP : Place in DELACK queue only once
				 */
				if (!tp->acknext) 
			        	enq(tp, ackqp, acknext, ackprev);
	 			/* Reset idle time and keep-alive timer. */
				tp->t_idle = 0;
				tp->t_timer[TCPT_KEEP] = tcp_keepidle;
				return;
			}
	}
bypass:

	/*
	 * Prediction failed:  do things one step at a time.
	 */


	if (tp->t_state == TCPS_CLOSED)
		goto drop;

	/*
	 * Drop TCP and IP headers; TCP options were dropped above.
	 */

	m->m_off += sizeof(struct tcpiphdr);
	m->m_len -= sizeof(struct tcpiphdr);

	if (so->so_options & SO_DEBUG) {
		ostate = tp->t_state;
		tcp_saveti = *ti;
	}
	if (so->so_options & SO_ACCEPTCONN) {
		so = (struct socket *) (*so->so_connindication)(so);
		if (so == 0)
			goto drop;
		/*
		 * This is ugly, but ....
		 *
		 * Mark socket as temporary until we're
		 * committed to keeping it.  The code at
		 * ``drop'' and ``dropwithreset'' check the
		 * flag dropsocket to see if the temporary
		 * socket created here should be discarded.
		 * We mark the socket as discardable until
		 * we're committed to it below in TCPS_LISTEN.
		 */
		dropsocket++;
		inp = (struct inpcb *)so->so_pcb;
		inp->inp_laddr = ti->ti_dst;
		inp->inp_lport = ti->ti_dport;
		inp->inp_options = ip_srcroute();
		tp = intotcpcb(inp);
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_LISTEN,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_LISTEN;
	}

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = tcp_keepidle;

	/*
	 * Process options if not in LISTEN state,
	 * else do it below (after getting remote address).
	 */
	if (om && tp->t_state != TCPS_LISTEN) {
		tcp_dooptions(tp, om, ti);
		om = 0;
	}

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	{ int win;

	win = sbspace(&so->so_rcv);
	if (win < 0)
		win = 0;
	tp->rcv_wnd = MAX(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}
	/****************************************************************
	 *	SYN Processing					 	*
	 ***************************************************************/
	switch (tp->t_state) {

	/*
	 * If the state is LISTEN then ignore segment if it contains an RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * If the ACCEPTCONN flag is set, enter RECEIVED state
	 * and set up to acknowledge and set keepalive timer,
	 * else enter SYN_HOLD state and drop packet.
	 */
	case TCPS_LISTEN: {
		struct mbuf *am;
		register struct sockaddr_in *sin;

		if (tiflags & TH_RST)
			goto drop;
		if (tiflags & TH_ACK)
			goto dropwithreset;
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (in_broadcast(ti->ti_dst))
			goto drop;
		am = m_get(M_DONTWAIT, MT_SONAME);
		if (am == NULL)
			goto drop;
		am->m_len = sizeof (struct sockaddr_in);
		sin = mtod(am, struct sockaddr_in *);
		sin->sin_family = AF_INET;
		sin->sin_addr = ti->ti_src;
		sin->sin_port = ti->ti_sport;
		laddr = inp->inp_laddr;
		if (inp->inp_laddr.s_addr == INADDR_ANY)
			inp->inp_laddr = ti->ti_dst;
		if (in_pcbconnect(inp, am)) {
			inp->inp_laddr = laddr;
			(void) m_free(am);
			goto drop;
		}
		(void) m_free(am);
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			tp = tcp_drop(tp, ENOBUFS);
			dropsocket = 0;		/* socket is already gone */
			goto drop;
		}
		if (om) {
			tcp_dooptions(tp, om, ti);
			om = 0;
		}
		if (iss)
			tp->iss = iss;
		else
			tp->iss = tcp_iss;
		tcp_iss += TCP_ISSINCR/2;
		tp->irs = ti->ti_seq;
		tcp_sendseqinit(tp);
		tcp_rcvseqinit(tp);
		dropsocket = 0;		/* committed to socket */
		if (tp->t_flags & TF_ACCEPTCONN) {
			tp->t_flags |= TF_ACKNOW;
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_SYN_RECEIVED,
    				tp->t_inpcb->inp_lport,
    				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_SYN_RECEIVED;
			tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
			tcpstat.tcps_accepts++;
			MIB_tcpIncrCounter(ID_tcpPassiveOpens);
			goto trimthenstep6;
		} else {
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_SYN_HOLD,
    				tp->t_inpcb->inp_lport,
    				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_SYN_HOLD;
			goto drop;
		}
		}

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK) {
				tp = tcp_drop(tp, ECONNREFUSED);
				MIB_tcpIncrCounter(ID_tcpAttemptFails);
			}
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = ti->ti_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[TCPT_REXMT] = 0;
		tp->irs = ti->ti_seq;
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			tcpstat.tcps_connects++;
			soisconnected(so);
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_ESTABLISHED,
    				tp->t_inpcb->inp_lport,
    				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_ESTABLISHED;
			/* 
			 * HP : hashed inpcb's for TCP 
			 *	Insert into hashed list.
			 */
			PCB_HASH(inp->inp_faddr,inp->inp_fport, 
				 inp->inp_laddr,inp->inp_lport,hash);
			hashqp = &tcphash[hash];
			enq(inp, hashqp, inp_nexth, inp_prevh);
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			(void) tcp_reass(tp, (struct tcpiphdr *)0);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt) {
				/*
	 			 * HP : Use global_rtt to avoid incrementing 
				 * 	t_rtt of each individual tcpcb.  
				 *	See comment in tcp_xmit_timer().
	 			 */
				tp->t_rtt = global_rtt - tp->t_rtt + 1;
				if (tp->t_rtt < 0)
					tp->t_rtt += MAX_SHORT;

				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
				TCPT_RANGESET(tp->t_rxtcur, 
				    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
				    TCPTV_MIN, TCPTV_REXMTMAX);
				tp->t_rtt = 0;
			}
		} else  {
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_SYN_RECEIVED,
    				tp->t_inpcb->inp_lport,
    				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_SYN_RECEIVED;
		}

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			m_adj(m, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			tcpstat.tcps_rcvpackafterwin++;
			tcpstat.tcps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;
	}
	/****************************************************************
	 *	End of SYN Processing					*
	 ***************************************************************/
	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1) 
				ti->ti_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
			/*
			 * HP:
			 * Don't fall into the code below...
			 * else we will get into an infinite loop
			 * on active connect,
			 * but only if tcp is active...
			 * (test probably should be: if ACTIVE
			 * && ti->ti_len - 1 == tp->irs
			 * just in case of bogus SYN,ACK packets)
			 */
			if (tp->t_flags & TF_ACTIVE) {
				tcpstat.tcps_rcvduppack++;
				tcpstat.tcps_rcvdupbyte += ti->ti_len;
				tp->t_flags |= TF_ACKNOW;
				goto dropthenestab;
			}
		}
		if (todrop > ti->ti_len ||
		    todrop == ti->ti_len && (tiflags&TH_FIN) == 0) {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += ti->ti_len;
			/*
			 * If segment is just one to the left of the window,
			 * check two special cases:
			 * 1. Don't toss RST in response to 4.2-style keepalive.
			 * 2. If the only thing to drop is a FIN, we can drop
			 *    it, but check the ACK or we will get into FIN
			 *    wars if our FINs crossed (both CLOSING).
			 * In either case, send ACK to resynchronize,
			 * but keep on processing for RST or ACK.
			 */
			if ((tiflags & TH_FIN && todrop == ti->ti_len + 1)
#ifdef TCP_COMPAT_42
			  || (tiflags & TH_RST && ti->ti_seq == tp->rcv_nxt - 1)
#endif
			   ) {
				todrop = ti->ti_len;
				tiflags &= ~TH_FIN;
				tp->t_flags |= TF_ACKNOW;
                        } else
			        goto dropafterack;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}
dropthenestab:
		m_adj(m, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;
		if (ti->ti_urp > todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && ti->ti_len) {
		tp = tcp_close(tp);
		tcpstat.tcps_rcvafterclose++;
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (ti->ti_seq+ti->ti_len) - (tp->rcv_nxt+tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= ti->ti_len) {
			tcpstat.tcps_rcvbyteafterwin += ti->ti_len;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {
				iss = tp->rcv_nxt + TCP_ISSINCR;
				(void) tcp_close(tp);
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat.tcps_rcvbyteafterwin += todrop;
		m_adj(m, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}
	/****************************************************************
	 *	RST Processing					 	*
	 ***************************************************************/
	/*
	 * If the RST bit is set examine the state:
	 *    SYN_HOLD STATE:
	 *	Inform user that connection was refused,
	 *	return to listen state.
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */

	if (tiflags&TH_RST) switch (tp->t_state) {

	case TCPS_SYN_HOLD:
	case TCPS_SYN_RECEIVED:
		so->so_error = ECONNREFUSED;
#ifdef _WSIO      
		sohaspollexception(so);
#endif
		MIB_tcpIncrCounter(ID_tcpAttemptFails);
		goto close;

	case TCPS_ESTABLISHED:
	case TCPS_CLOSE_WAIT:
		MIB_tcpIncrCounter(ID_tcpEstabResets);
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
		so->so_error = ECONNRESET;
#ifdef _WSIO      
		sohaspollexception(so);
#endif
	close:
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_CLOSED,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_CLOSED;
		tcpstat.tcps_drops++;
		tp = tcp_close(tp);
		goto drop;

	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		tp = tcp_close(tp);
		goto drop;
	}
	/****************************************************************
	 *	End of RST Processing					*
	 ***************************************************************/
	/*
	 * If a SYN is in the window, then this is an
	 * error and we send an RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;
	
	/****************************************************************
	 *	ACK Processing					 	*
	 ***************************************************************/
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send an RST.
	 */
	case TCPS_SYN_RECEIVED:
		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max))
			goto dropwithreset;
		tcpstat.tcps_connects++;
		soisconnected(so);
		NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG, NS_LS_TCP,
    			0, 5, tp->t_state, TCPS_ESTABLISHED,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_ESTABLISHED;
		/* 
		 * HP : hashed inpcb's for TCP 
		 *	Insert into hashed list.
		 */
		PCB_HASH(inp->inp_faddr,inp->inp_fport, 
			 inp->inp_laddr,inp->inp_lport,hash);
			hashqp = &tcphash[hash];
			enq(inp, hashqp, inp_nexth, inp_prevh);
		tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
		(void) tcp_reass(tp, (struct tcpiphdr *)0);
		tp->snd_wl1 = ti->ti_seq - 1;
		/* fall into ... */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && ti->ti_win == tp->snd_wnd) {
				tcpstat.tcps_rcvdupack++;
				/*
				 * If we have outstanding data (not a
				 * window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.  If this packet fills the
				 * only hole in the receiver's seq.
				 * space, the next real ack will fully
				 * open our window.  This means we
				 * have to do the usual slow-start to
				 * not overwhelm an intermediate gateway
				 * with a burst of packets.  Leave
				 * here with the congestion window set
				 * to allow 2 packets on the next real
				 * ack and the exp-to-linear thresh
				 * set for half the current window
				 * size (since we know we're losing at
				 * the current window size).
				 */
				if (tp->t_timer[TCPT_REXMT] == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					u_int win =
					    MIN(tp->snd_wnd, tp->snd_cwnd) / 2 /
						tp->t_maxseg;

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_maxseg;
					tp->t_timer[TCPT_REXMT] = 0;
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_maxseg;
					error = tcp_output(tp);
					if (error == ENOBUFS) {
						/* Retransmit just reset above */
						if (tp->t_timer[TCPT_PERSIST] == 0) {
							tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
							ENQ_REXMT(tp);
							tp->t_rxtshift = 0;
						}
					} 
					/* 
				   	 * HP : Ported from 4.3 Reno.  
					 *
				   	 * Increase cwnd to reflect other 
				   	 * side's cached packets.
					*/
					tp->snd_cwnd = tp->snd_ssthresh +
					       tp->t_maxseg * tp->t_dupacks;
					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				} else if (tp->t_dupacks > tcprexmtthresh) {
					tp->snd_cwnd += tp->t_maxseg;
					error = tcp_output(tp);
					if (error == ENOBUFS) {
						/* Retransmit just reset above */
						if (tp->t_timer[TCPT_PERSIST] == 0) {
							tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
							ENQ_REXMT(tp);
							tp->t_rxtshift = 0;
						}
					} 
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		/* 
		 * HP : Ported from 4.3 Reno.  
		 *
		 * If the congestion window was inflated to account
		 * for the other side's cached packets, retract it.
		 */
		if (tp->t_dupacks > tcprexmtthresh &&
		    tp->snd_cwnd > tp->snd_ssthresh)
			tp->snd_cwnd = tp->snd_ssthresh;
		tp->t_dupacks = 0;
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		tcpstat.tcps_rcvackpack++;
		tcpstat.tcps_rcvackbyte += acked;

		/*
		 * If transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq))
			tcp_xmit_timer(tp);

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			ENQ_REXMT(tp);
		}
		/*
		 * HP : Ported from 4.3 Reno.
		 *
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly: maxseg per window
		 * (maxseg^2 / cwnd per packet), plus a constant
		 * fraction of a packet (maxseg/8) to help larger windows
		 * open quickly enough.
		 */
		{
		register u_int cw = tp->snd_cwnd;
		register u_int incr = tp->t_maxseg;

		if (cw > tp->snd_ssthresh)
			incr = incr * incr / cw + incr / 8;
		tp->snd_cwnd = min(cw + incr, IP_MAXPACKET);
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
		} else {
			sbdrop(&so->so_snd, acked);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		sowwakeup(so);
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 */
				if (so->so_state & SS_CANTRCVMORE) {
					soisdisconnected(so);
					tp->t_timer[TCPT_2MSL] = tcp_maxidle;
				}
				NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
					NS_LS_TCP, 0, 5,
					tp->t_state, TCPS_FIN_WAIT_2,
    					tp->t_inpcb->inp_lport,
    					tp->t_inpcb->inp_fport,
    					tp->t_inpcb->inp_faddr.s_addr);
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

	 	/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
					NS_LS_TCP, 0, 5,
					tp->t_state, TCPS_TIME_WAIT,
    					tp->t_inpcb->inp_lport,
    					tp->t_inpcb->inp_fport,
    					tp->t_inpcb->inp_faddr.s_addr);
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				soisdisconnected(so);
			}
			break;

		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_close(tp);
				goto drop;
			}
			break;

		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			goto dropafterack;
		}
	}
	/****************************************************************
	 *	End of ACK Processing					*
	 ***************************************************************/
step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 *
	 * HP: allow for shrinking windows also, by changing
	 *   ti_win > snd_wnd
	 * to
	 *   ti_win != snd_wnd
	 * in comparison below.
	 */
	if ((tiflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq &&
	    (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
		/* Allow for shrinking windows also !*/
	     tp->snd_wl2 == ti->ti_ack && ti->ti_win != tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd)
			tcpstat.tcps_rcvwinupd++;
		tp->snd_wnd = ti->ti_win;
		/*
		 * HP: allow for shrinking windows also,
		 * by resetting transmit timer.
		 */
		if (ti->ti_win == 0)
			tp->t_timer[TCPT_REXMT] = 0;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}
	/****************************************************************
	 *	URG Processing					 	*
	 ***************************************************************/
	if ((tiflags & TH_URG) && ti->ti_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (ti->ti_urp + so->so_rcv.sb_cc > SB_MAX) {
			ti->ti_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section
		 * as the original spec states.
		 */
		if (SEQ_GT(ti->ti_seq+ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;

			/* \*HP*\
			 * If we're at OOB data, set SS_RCVATMARK.  Otherwise,
			 * we must explicitly reset SS_RCVATMARK in case we
			 * were at previous OOB data.
			 *
			 * 4.2BSD and 4.3BSD don't reset mark explicitly.
			 * Thus, SS_RCVATMARK would remain set if we were at
			 * old OOB data, but we will point to new OOB data.
			 * This is inconsistent with the case when we were not
			 * at old OOB data, in which case SS_RCVATMARK is not
			 * already set, and it is correctly not set until we
			 * get to new OOB data.
			 *
			 * Moreover, the BSD behavior will see SS_RCVATMARK
			 * set twice, now (at old OOB data) and later (at new
			 * OOB data).  This might confuse loops that flush data
			 * before the mark.
			 *
			 * This behavior was corrected in 7.0 HP-UX.
			 */

			if (so->so_oobmark == 0)
				so->so_state |= SS_RCVATMARK;
                        else                                     /*HP*/
                                so->so_state &= ~SS_RCVATMARK;   /*HP*/
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (ti->ti_urp <= ti->ti_len &&
		    (so->so_options & SO_OOBINLINE) == 0)
			tcp_pulloutofband(so, ti);
	} else
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
	/****************************************************************
	 *	Process Data					 	*
	 ***************************************************************/
dodata:							/* XXX */

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((ti->ti_len || (tiflags&TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, m, so, tiflags);
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		if (len > tp->max_rcvd)
			tp->max_rcvd = len;
	} else {
		m_freem(m);
		tiflags &= ~TH_FIN;
	}
	/****************************************************************
	 *	FIN Processing					 	*
	 ***************************************************************/
	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.
	 */
	if (tiflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {

	 	/*
		 * In SYN_RECEIVED and ESTABLISHED STATES
		 * enter the CLOSE_WAIT state.
		 */
		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_CLOSE_WAIT,
    				tp->t_inpcb->inp_lport,
				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

	 	/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */
		case TCPS_FIN_WAIT_1:
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_CLOSING,
    				tp->t_inpcb->inp_lport,
				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_CLOSING;
			break;

	 	/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other 
		 * standard timers.
		 */
		case TCPS_FIN_WAIT_2:
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
				NS_LS_TCP, 0, 5, tp->t_state, TCPS_TIME_WAIT,
    				tp->t_inpcb->inp_lport,
				tp->t_inpcb->inp_fport,
    				tp->t_inpcb->inp_faddr.s_addr);
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			break;
		}
	}
	/****************************************************************
	 *	End of FIN Processing					*
	 ***************************************************************/
	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*  nvs_output can be waiting for one reason:
	 *  - there was no space in the output buffer
	 *  so if there now is sufficient space, and the
	 *  wait flag is set, call nvs_output.
	 */
	if (so->so_snd.sb_flags & SB_NVS_WAIT &&
            sbspace(&so->so_snd) >= MLEN)
        	nvs_return = nvs_output(so->so_nvs_index);
	/*
	 * Return any desired output. Optimized for kernel telnet.
	 */
	if ((nvs_return == 0) && 
	    (needoutput || (tp->t_flags & TF_ACKNOW))) {
		error = tcp_output(tp);
		if ((error == ENOBUFS) && (so->so_snd.sb_cc)) {
			if ((tp->t_timer[TCPT_REXMT] == 0) &&
				(tp->t_timer[TCPT_PERSIST] == 0)) {
				tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
				ENQ_REXMT(tp);
				tp->t_rxtshift = 0;
			} 
		} 
	}
	return;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	error = tcp_output(tp);
	if ((error == ENOBUFS) && (so->so_snd.sb_cc)) {
		if ((tp->t_timer[TCPT_REXMT] == 0) &&
			(tp->t_timer[TCPT_PERSIST] == 0)) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			ENQ_REXMT(tp);
			tp->t_rxtshift = 0;
		}
	} 
	return;

dropwithreset:
	if (om) {
		(void) m_free(om);
		om = 0;
	}
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast.
	 */
	if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst))
		goto drop;
	if (tiflags & TH_ACK)
		tcp_respond(tp, ti, (tcp_seq)0, ti->ti_ack, TH_RST);
	else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		tcp_respond(tp, ti, ti->ti_seq+ti->ti_len, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	MIB_tcpIncrCounter(ID_tcpOutRsts);
	/* destroy temporarily created socket */
	if (dropsocket)
		(void) soabort(so);
	return;

drop:
	if (om)
		(void) m_free(om);
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	m_freem(m);
	/* destroy temporarily created socket */
	if (dropsocket)
		(void) soabort(so);
	return;
}

tcp_dooptions(tp, om, ti)
	struct tcpcb *tp;
	struct mbuf *om;
	struct tcpiphdr *ti;
{
	register u_char *cp;
	int opt, optlen, cnt;

	cp = mtod(om, u_char *);
	cnt = om->m_len;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case TCPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			/*
			 * HP: the following line is corrected
			 * to allow for unaligned short option length
			 * and prevent kernel panics
			 *
			 * tp->t_maxseg = *(u_short *)(cp + 2);
			 */
			tp->t_maxseg = (cp[2] << 8) + cp[3];
			tp->t_maxseg = ntohs((u_short)tp->t_maxseg);
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			break;
		}
	}
	(void) m_free(om);
}

/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
tcp_pulloutofband(so, ti)
	struct socket *so;
	struct tcpiphdr *ti;
{
	register struct mbuf *m;
	int cnt = ti->ti_urp - 1;
	
	m = dtom(ti);
	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			bcopy(cp+1, cp, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == 0)
			break;
	}
	panic("tcp_pulloutofband");
}

/*
 *  Determine a reasonable value for maxseg size.
 *  If the route is known, use one that can be handled
 *  on the given interface without forcing IP to fragment.
 *  If bigger than an mbuf cluster (MCLBYTES), round down to nearest size
 *  to utilize large mbufs.
 *  If interface pointer is unavailable, or the destination isn't local,
 *  use a conservative size (512 or the default IP max size, but no more
 *  than the mtu of the interface through which we route),
 *  as we can't discover anything about intervening gateways or networks.
 *  We also initialize the congestion/slow start window to be a single
 *  segment if the destination isn't local; this information should
 *  probably all be saved with the routing entry at the transport level.
 *
 *  This is ugly, and doesn't belong at this level, but has to happen somehow.
 */
tcp_mss(tp)
	register struct tcpcb *tp;
{
	struct route *ro;
	struct ifnet *ifp;
	int mss;
	struct inpcb *inp;

	inp = tp->t_inpcb;
	ro = &inp->inp_route;
	if ((ro->ro_rt == (struct rtentry *)0) ||
	    (ifp = ro->ro_rt->rt_ifp) == (struct ifnet *)0) {
		/* No route yet, so try to acquire one */
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				inp->inp_faddr;
			rtalloc(ro);
		}
		if ((ro->ro_rt == 0) || (ifp = ro->ro_rt->rt_ifp) == 0)
			return (TCP_MSS);
	}

	mss = ifp->if_mtu - sizeof(struct tcpiphdr);
#if	(MCLBYTES & (MCLBYTES - 1)) == 0
	if (mss > MCLBYTES)
		mss &= ~(MCLBYTES-1);
#else
	if (mss > MCLBYTES)
		mss = mss / MCLBYTES * MCLBYTES;
#endif
	if (in_localaddr(inp->inp_faddr))
		return (mss);

	mss = MIN(mss, TCP_MSS);
	tp->snd_cwnd = mss;
	return (mss);
}

/* Routine added for header prediction code */

tcp_xmit_timer(tp)
register struct tcpcb *tp;
{
	tcpstat.tcps_rttupdated++;
	/*
	 * HP : Use global_rtt to avoid incrementing t_rtt of each
	 *	individual tcpcb.  
	 *
	 *	When we start rtt timing, store the value of global_rtt 
	 *	in t_rtt.  When we need to know the real t_rtt, we compute 
	 *	the time lapse between the current global_rtt and what was 
	 *	stored in t_rtt.
	 */

	if (tp->t_rtt) {
		tp->t_rtt = global_rtt - tp->t_rtt + 1;
		/* case when global_rtt has wrapped around */
		if (tp->t_rtt < 0)
			tp->t_rtt += MAX_SHORT;
	}

	if (tp->t_srtt != 0) {
		register short delta;

		/*
		 * srtt is stored as fixed point with 3 bits
		 * after the binary point (i.e., scaled by 8).
		 * The following magic is equivalent
		 * to the smoothing algorithm in rfc793
		 * with an alpha of .875
		 * (srtt = rtt/8 + srtt*7/8 in fixed point).
		 * Adjust t_rtt to origin 0.
		 */
		delta = tp->t_rtt - 1 - (tp->t_srtt >> 3);
		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1;
		/*
		 * We accumulate a smoothed rtt variance
		 * (actually, a smoothed mean difference),
		 * then set the retransmit timer to smoothed
		 * rtt + 2 times the smoothed variance.
		 * rttvar is stored as fixed point
		 * with 2 bits after the binary point
		 * (scaled by 4).  The following is equivalent
		 * to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).
		 * This replaces rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (tp->t_rttvar >> 2);
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1;
	} else {
		/* 
		 * No rtt measurement yet - use the
		 * unsmoothed rtt.  Set the variance
		 * to half the rtt (so our first
		 * retransmit happens at 2*rtt)
		 */
		tp->t_srtt = tp->t_rtt << 3;
		tp->t_rttvar = tp->t_rtt << 1;
	}
	tp->t_rtt = 0;
	tp->t_rxtshift = 0;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
}
