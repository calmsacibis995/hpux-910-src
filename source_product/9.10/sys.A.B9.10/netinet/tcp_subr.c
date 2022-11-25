/*
 * $Header: tcp_subr.c,v 1.34.83.7 93/10/27 15:31:15 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/tcp_subr.c,v $
 * $Revision: 1.34.83.7 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 15:31:15 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) tcp_subr.c $Revision: 1.34.83.7 $ $Date: 93/10/27 15:31:15 $";
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
 *	@(#)tcp_subr.c	7.14 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"

#include "../net/route.h"
#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"

#include "../h/subsys_id.h"
#include "../h/net_diag.h"

#undef remque
u_char	tcpDefaultTTL = TCP_TTL;

/*
 * Tcp initialization
 */
tcp_init()
{
	int	i;

	tcp_iss = 1;		/* wrong */
	tcb.inp_next = tcb.inp_prev = &tcb;

	/* HP : Delayed Ack Queue */
	tcpackq.acknext = tcpackq.ackprev = &tcpackq;
	/* 
	 * HP : hashed inpcb's for TCP 
	 *	Initialize the circular hashed list.
	 */

	for (i=0; i<PCB_NB; i++) {
		tcphash[i].inp_nexth = (struct inpcb *) &tcphash[i];
		tcphash[i].inp_prevh = (struct inpcb *) &tcphash[i];
	}
	/*
	 * HP : 'Soon To Expire' Timer Queue.
	 * 	Initialize tcp_maxidle.  
	 *	Update it only if we reconfigure tcp_keepintvl.
	 */
	tcp_maxidle = TCPTV_KEEPCNT * tcp_keepintvl;
	tq_bk	   = 0;	
	global_rtt = 0;
	for (i=0; i<TIMQLEN; i++) {
		tcptimq[i].tim_next = (struct tcpcb *) &tcptimq[i];
		tcptimq[i].tim_prev = (struct tcpcb *) &tcptimq[i];
	}

	/* initialization for network management */

#define	vanj	4	/* Van Jacobson's RTO algorithm */

	/* Convert tcpRtoMin to milliseconds.  Multiply by 500 instead 
	   of 1000 because TCPTV_MIN has a factor of PR_SLOWHZ=2	*/

	MIB_tcpcounter[ID_tcpRtoAlgorithm & INDX_MASK] = vanj;
	MIB_tcpcounter[ID_tcpRtoMin & INDX_MASK] = TCPTV_MIN*500;
	MIB_tcpcounter[ID_tcpRtoMax & INDX_MASK] = TCPTV_REXMTMAX*500;
	MIB_tcpcounter[ID_tcpMaxConn & INDX_MASK] = (counter) -1;

	/* register get/set routines for network management */
	NMREG (GP_tcp, nmget_tcp, mib_unsupp, mib_unsupp, mib_unsupp);
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct tcpiphdr *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct mbuf *m;
	register struct tcpiphdr *n;

	if ((n = tp->t_template) == 0) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			return (0);
		m->m_off = MMAXOFF - sizeof (struct tcpiphdr);
		m->m_len = sizeof (struct tcpiphdr);
		n = mtod(m, struct tcpiphdr *);
	}
	n->ti_next = n->ti_prev = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (n);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If flags==0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
tcp_respond(tp, ti, ack, seq, flags)
	struct tcpcb *tp;
	register struct tcpiphdr *ti;
	tcp_seq ack, seq;
	int flags;
{
	register struct mbuf *m;
	int win = 0, tlen;
	struct route *ro = 0;
	int ip_ttl;

	if (tp) {
		win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		ro = &tp->t_inpcb->inp_route;
		ip_ttl = tp->t_inpcb->inp_ttl;
	} else
		ip_ttl = tcpDefaultTTL;
	if (flags == 0) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			return;
#ifdef TCP_COMPAT_42
		tlen = 1;
#else
		tlen = 0;
#endif
		m->m_len = sizeof (struct tcpiphdr) + tlen;
		*mtod(m, struct tcpiphdr *) = *ti;
		ti = mtod(m, struct tcpiphdr *);
		flags = TH_ACK;
	} else {
		m = dtom(ti);
		m_freem(m->m_next);
		m->m_next = 0;
		m->m_off = (int)ti - (int)m;
		tlen = 0;
		m->m_len = sizeof (struct tcpiphdr);
		m->m_flags &= ~(MF_CKO_IN | MF_CKO_OUT);
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_long);
		xchg(ti->ti_dport, ti->ti_sport, u_short);
#undef xchg
	}
	ti->ti_next = ti->ti_prev = 0;
	ti->ti_x1 = 0;
	ti->ti_len = htons((u_short)(sizeof (struct tcphdr) + tlen));
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof (struct tcphdr) >> 2;
	ti->ti_flags = flags;
	ti->ti_win = htons((u_short)win);
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(m, sizeof (struct tcpiphdr) + tlen);
	((struct ip *)ti)->ip_len = sizeof (struct tcpiphdr) + tlen;
	((struct ip *)ti)->ip_ttl = ip_ttl;
	(void) ip_output(m, (struct mbuf *)0, ro, 0);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
struct tcpcb *
tcp_newtcpcb(inp)
	struct inpcb *inp;
{
	register struct tcpcb *tp;

	MALLOC(tp, struct tcpcb *, sizeof (struct tcpcb), M_PCB, M_NOWAIT);
	if (tp == NULL)
		return ((struct tcpcb *)0);
	bzero((caddr_t)tp, sizeof (struct tcpcb));
	mbstat.m_mtypes[MT_PCB]++;
	tp->seg_next = tp->seg_prev = (struct tcpiphdr *)tp;
	tp->acknext  = 0;
	tp->tim_next = 0;
	tp->t_maxseg = TCP_MSS;
	tp->t_flags = 0;		/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = TCPTV_SRTTDFLT << 2;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = sbspace(&inp->inp_socket->so_snd);
	tp->snd_ssthresh = 65535;		/* XXX */
	inp->inp_ppcb = (caddr_t)tp;
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, errno)
	register struct tcpcb *tp;
	int errno;
{
	struct socket *so = tp->t_inpcb->inp_socket;

	if (TCPS_HAVEHELDSYN(tp->t_state)) {
			NS_LOG_INFO5(LE_TCP_CHANGE_STATE, NS_LC_PROLOG,
			NS_LS_TCP, 0, 5, tp->t_state, TCPS_CLOSED,
    			tp->t_inpcb->inp_lport,
    			tp->t_inpcb->inp_fport,
    			tp->t_inpcb->inp_faddr.s_addr);
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		/* Dont need to check for ENOBUFS here */
		tcpstat.tcps_drops++;
	} else
		tcpstat.tcps_conndrops++;
	so->so_error = errno;
#ifdef _WSIO      
	sohaspollexception(so);
#endif
	return (tcp_close(tp));
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct tcpcb *
tcp_close(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;
	int	hash;

	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = dtom(t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void) m_free(dtom(tp->t_template));
	/* 
	 * HP : Hashed PCB's for TCP.  
	 *	Delayed Ack Queue for tcp_fasttimo replacement.
	 *	Timer Queue for tcp_slowtimo replacement.
	 *
	 *	If inpcb is in hashed or DELACK or Timer queue, delink it.  
	 *	Clear next pointer so we won't try to delink again.  
	 *	This is needed because tcp_close can be entered through 
	 *	different paths, eg: RST, timeout, and user issue 
	 *	close() after connection was timed out.
	 */
	if (inp->inp_nexth) 
		deq(inp, inp_nexth, inp_prevh);
	if (tp->acknext) 
		deq(tp, acknext, ackprev);
	if (tp->tim_next) 
		deq(tp, tim_next, tim_prev);
	FREE(tp, M_PCB);
	MIB_tcpDecrConnNumEnt;
	mbstat.m_mtypes[MT_PCB]--;
	inp->inp_ppcb = 0;
	soisdisconnected(so);
	in_pcbdetach(inp);
	tcpstat.tcps_closed++;
	return ((struct tcpcb *)0);
}

tcp_drain()
{

}

/*
 * Notify a tcp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
tcp_notify(inp)
	register struct inpcb *inp;
{

	wakeup((caddr_t) &inp->inp_socket->so_timeo);
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}
tcp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int tcp_quench(), in_rtchange();

	if ((unsigned)cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;

	switch (cmd) {

	case PRC_QUENCH:
		in_pcbnotify(&tcb, sin, 0, tcp_quench);
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		in_pcbnotify(&tcb, sin, 0, in_rtchange);
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
		in_pcbnotify(&tcb, sin, (int)inetctlerrmap[cmd],
			tcp_notify);
	}
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
tcp_quench(inp)
	struct inpcb *inp;
{
	struct tcpcb *tp = intotcpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_maxseg;
}
