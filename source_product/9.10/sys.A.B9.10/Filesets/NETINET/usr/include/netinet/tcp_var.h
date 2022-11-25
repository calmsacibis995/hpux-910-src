/* $Header: tcp_var.h,v 1.27.83.5 93/12/06 16:41:46 marshall Exp $Revision: $ $Date: 93/12/06 16:41:46 $ */

#ifndef	_SYS_TCP_VAR_INCLUDED
#define	_SYS_TCP_VAR_INCLUDED
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
 *	@(#)tcp_var.h	7.8 (Berkeley) 6/29/88
 */

/*
 * TCP configuration:  This is a half-assed attempt to make TCP
 * self-configure for a few varieties of 4.2 and 4.3-based unixes.
 * If you don't have a) a 4.3bsd vax or b) a 3.x Sun (x<6), check
 * this carefully (it's probably not right).  Please send me mail
 * if you run into configuration problems.
 *  - Van Jacobson (van@lbl-csam.arpa)
 */

#define BSD 43
#define TCP_COMPAT_42	/* set if we have to interop w/4.2 systems */

#ifndef SB_MAX
#ifdef	SB_MAXCOUNT
#define	SB_MAX	SB_MAXCOUNT	/* Sun has to be a little bit different... */
#else
#define SB_MAX	32767		/* XXX */
#endif	/* SB_MAXCOUNT */
#endif	/* SB_MAX */

#ifndef IP_MAXPACKET
#define	IP_MAXPACKET	65535		/* maximum packet size */
#endif

/*
 * Kernel variables for tcp.
 */

/*
 * Tcp control block, one per tcp; fields:
 *
 * NOTE : The location for next and previous pointers should not moved
 * because routines in machine/queue.s depends on their current offsets.
 */
struct tcpcb {
	struct	tcpiphdr *seg_next;	/* sequencing queue */
	struct	tcpiphdr *seg_prev;
	short	t_state;		/* state of this connection */
	short	t_timer[TCPT_NTIMERS];	/* tcp timers */
	short	t_rxtshift;		/* log(2) of rexmt exp. backoff */
	short	t_rxtcur;		/* current retransmit value */
	short	t_dupacks;		/* consecutive dup acks recd */
	u_short	t_maxseg;		/* maximum segment size */
	u_short	t_flags;
#define	TF_ACKNOW	0x0001		/* ack peer immediately */
#define	TF_DELACK	0x0002		/* ack, but try to delay it */
#define	TF_NODELAY	0x0004		/* don't delay packets to coalesce */
#define	TF_NOOPT	0x0008		/* don't use tcp options */
#define	TF_SENTFIN	0x0010		/* have sent FIN */
#define	TF_FORCE	0x0800		/* 1 if forcing out a byte */
#define	TF_MSGMODE	0x1000		/* in Netipc message mode */
#define	TF_ACTIVE	0x2000		/* active connection */
#define	TF_ACCEPTCONN	0x4000		/* accept connection */
#define TF_WINUPDATE	0x8000		/* send window update to peer */
	struct	tcpiphdr *t_template;	/* skeletal packet for transmit */
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq	snd_una;		/* send unacknowledged */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	snd_up;			/* send urgent pointer */
	tcp_seq	snd_wl1;		/* window update seg seq number */
	tcp_seq	snd_wl2;		/* window update seg ack number */
	tcp_seq	iss;			/* initial send sequence number */
	u_short	snd_wnd;		/* send window */
/* receive sequence variables */
	u_short	rcv_wnd;		/* receive window */
	tcp_seq	rcv_nxt;		/* receive next */
	tcp_seq	rcv_up;			/* receive urgent pointer */
	tcp_seq	irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq	rcv_adv;		/* advertised window */
/* retransmit variables */
	tcp_seq	snd_max;		/* highest sequence number sent
					 * used to recognize retransmits
					 */
/* congestion control (for slow start, source quench, retransmit after loss) */
	u_short	snd_cwnd;		/* congestion-controlled window */
	u_short snd_ssthresh;		/* snd_cwnd size threshhold for
					 * for slow start exponential to
					 * linear switch */
/*
 * transmit timing stuff.
 * srtt and rttvar are stored as fixed point; for convenience in smoothing,
 * srtt has 3 bits to the right of the binary point, rttvar has 2.
 * "Variance" is actually smoothed difference.
 */
	short	t_idle;			/* inactivity time */
	short	t_rtt;			/* round trip time */
	tcp_seq	t_rtseq;		/* sequence number being timed */
	short	t_srtt;			/* smoothed round-trip time */
	short	t_rttvar;		/* variance in round-trip time */
	u_short max_rcvd;		/* most peer has sent into window */
	u_short	max_sndwnd;		/* largest window peer has offered */
/* out-of-band data */
	char	t_oobflags;		/* have some */
	char	t_iobc;			/* input character */
#define	TCPOOB_HAVEDATA	0x01
#define	TCPOOB_HADDATA	0x02
	struct  tcpcb	 *acknext;	/* Delayed ACK queue */
	struct  tcpcb	 *ackprev;
	struct  tcpcb	 *tim_next;	/* Slow Timer queue */
	struct  tcpcb	 *tim_prev;
};

#define	intotcpcb(ip)	((struct tcpcb *)(ip)->inp_ppcb)
#define	sototcpcb(so)	(intotcpcb(sotoinpcb(so)))

/*
 * TCP statistics.
 * Many of these should be kept per connection,
 * but that's inconvenient at the moment.
 */
struct	tcpstat {
	u_long	tcps_connattempt;	/* connections initiated */
	u_long	tcps_accepts;		/* connections accepted */
	u_long	tcps_rejects;		/* connections rejected */
	u_long	tcps_connects;		/* connections established */
	u_long	tcps_drops;		/* connections dropped */
	u_long	tcps_conndrops;		/* embryonic connections dropped */
	u_long	tcps_closed;		/* conn. closed (includes drops) */
	u_long	tcps_segstimed;		/* segs where we tried to get rtt */
	u_long	tcps_rttupdated;	/* times we succeeded */
	u_long	tcps_delack;		/* delayed acks sent */
	u_long	tcps_timeoutdrop;	/* conn. dropped in rxmt timeout */
	u_long	tcps_rexmttimeo;	/* retransmit timeouts */
	u_long	tcps_persisttimeo;	/* persist timeouts */
	u_long	tcps_keeptimeo;		/* keepalive timeouts */
	u_long	tcps_keepprobe;		/* keepalive probes sent */
	u_long	tcps_keepdrops;		/* connections dropped in keepalive */

	u_long	tcps_sndtotal;		/* total packets sent */
	u_long	tcps_sndpack;		/* data packets sent */
	u_long	tcps_sndbyte;		/* data bytes sent */
	u_long	tcps_sndrexmitpack;	/* data packets retransmitted */
	u_long	tcps_sndrexmitbyte;	/* data bytes retransmitted */
	u_long	tcps_sndacks;		/* ack-only packets sent */
	u_long	tcps_sndprobe;		/* window probes sent */
	u_long	tcps_sndurg;		/* packets sent with URG only */
	u_long	tcps_sndwinup;		/* window update-only packets sent */
	u_long	tcps_sndctrl;		/* control (SYN|FIN|RST) packets sent */

	u_long	tcps_rcvtotal;		/* total packets received */
	u_long	tcps_rcvpack;		/* packets received in sequence */
	u_long	tcps_rcvbyte;		/* bytes received in sequence */
	u_long	tcps_rcvbadsum;		/* packets received with ccksum errs */
	u_long	tcps_rcvbadoff;		/* packets received with bad offset */
	u_long	tcps_rcvshort;		/* packets received too short */
	u_long	tcps_rcvduppack;	/* duplicate-only packets received */
	u_long	tcps_rcvdupbyte;	/* duplicate-only bytes received */
	u_long	tcps_rcvpartduppack;	/* packets with some duplicate data */
	u_long	tcps_rcvpartdupbyte;	/* dup. bytes in part-dup. packets */
	u_long	tcps_rcvoopack;		/* out-of-order packets received */
	u_long	tcps_rcvoobyte;		/* out-of-order bytes received */
	u_long	tcps_rcvpackafterwin;	/* packets with data after window */
	u_long	tcps_rcvbyteafterwin;	/* bytes rcvd after window */
	u_long	tcps_rcvafterclose;	/* packets rcvd after "close" */
	u_long	tcps_rcvwinprobe;	/* rcvd window probe packets */
	u_long	tcps_rcvdupack;		/* rcvd duplicate acks */
	u_long	tcps_rcvacktoomuch;	/* rcvd acks for unsent data */
	u_long	tcps_rcvackpack;	/* rcvd ack packets */
	u_long	tcps_rcvackbyte;	/* bytes acked by rcvd acks */
	u_long	tcps_rcvwinupd;		/* rcvd window update packets */
};

#ifdef _KERNEL
struct	inpcb tcb;		/* head of queue of active tcpcb's */
struct	tcpstat tcpstat;	/* tcp statistics */
struct	inpcb tcphash[PCB_NB];	/* Hashed queue for TCP inpcb's */
struct	tcpcb tcpackq;		/* Delayed ACK queue */
#undef enq
#undef deq

/* HP : Macros to enqueue and dequeue from a doubly-linked list
 *
 *		p = pointer to element being inserted or deleted
 *		prev = enqueue p after element prev 
 *		nextp = next pointer in structure
 *		prevp = previous pointer in structure
 */

#define enq(p, prev, nextp, prevp) {				\
	(p)->prevp = (prev);					\
	(p)->nextp = (prev)->nextp;				\
	(prev)->nextp->prevp = (p);				\
	(prev)->nextp = (p);					\
}

#define deq(p, nextp, prevp) {					\
	(p)->prevp->nextp = (p)->nextp;				\
	(p)->nextp->prevp = (p)->prevp;				\
	(p)->nextp = 0;						\
}

int 	tq_bk;			/* Current bucket in Timer Queue */
short 	global_rtt;		/* Global count for t_rtt 	 */

#define MAX_SHORT 32768		/* Max positive value in a short */
#define	TIMQLEN	8		/* Array size for Slow Timer Queue*/

struct	tcpcb tcptimq[TIMQLEN];	/* Slow Timer queue */
	/* 
	 * HP : Timer Queue.  REXMT timer value will take precedence
	 *	over KEEP and 2MSL timers.
	 */
#define ENQ_REXMT(tp) {						\
	if ((tp)->t_timer[TCPT_REXMT] < TIMQLEN) {		\
		indx = (tq_bk + (tp)->t_timer[TCPT_REXMT]) & (TIMQLEN-1);  \
		timqp = (struct tcpcb *) &tcptimq[indx];	\
		if ((tp)->tim_next)				\
			deq((tp), tim_next, tim_prev);		\
		enq((tp), timqp, tim_next, tim_prev);		\
	}							\
}

struct	tcpiphdr *tcp_template();
struct	tcpcb *tcp_close(), *tcp_drop();
struct	tcpcb *tcp_timers(), *tcp_disconnect(), *tcp_usrclosed();
#endif
#endif	/* _SYS_TCP_VAR_INCLUDED */
