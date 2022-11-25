/* $Header: socketvar.h,v 1.32.83.4 93/09/17 18:34:59 kcs Exp $ */

#ifndef	_SYS_SOCKETVAR_INCLUDED
#define	_SYS_SOCKETVAR_INCLUDED
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
 *	@(#)socketvar.h	7.4 (Berkeley) 6/27/88
 */

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 */
struct socket {
	short	so_type;		/* generic type, see socket.h */
	short	so_options;		/* from socket call, see socket.h */
	short	so_linger;		/* time to linger while closing */
	short	so_state;		/* internal state flags SS_*, below */
	caddr_t	so_pcb;			/* protocol control block */
	struct	protosw *so_proto;	/* protocol handle */
/*
 * Variables for connection queueing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct	socket *so_head;	/* back pointer to accept socket */
	struct	socket *so_q0;		/* queue of partial connections */
	struct	socket *so_q;		/* queue of incoming connections */
	short	so_q0len;		/* partials on so_q0 */
	short	so_qlen;		/* number of connections on so_q */
	short	so_qlimit;		/* max number queued connections */
	short	so_timeo;		/* connection timeout */
	u_short	so_error;		/* error affecting connection */
	short	so_pgrp;		/* pgrp for signals */
	u_long	so_oobmark;		/* chars to oob mark */
/*
 * Variables for socket buffering.
 */
	struct	sockbuf {
		u_long	sb_cc;		/* actual chars in buffer */
		u_long	sb_hiwat;	/* max actual char count */
		u_long	sb_mbcnt;	/* chars of mbufs used */
		u_long	sb_mbmax;	/* max chars of mbufs to use */
		u_long	sb_lowat;	/* low water mark (not used yet) */
		struct	mbuf *sb_mb;	/* the mbuf chain */
		struct	proc *sb_sel;	/* process selecting read/write */
		short	sb_timeo;	/* timeout (not used yet) */
		u_short	sb_flags;	/* flags, see below */
		struct socket *sb_so;   /* ptr to containing socket struct *HP*/
		void	(*sb_wakeup)();	/* upcall instead of sowakeup *HP*/
		caddr_t	sb_wakearg;	/* (*sb_wakeup)(sb_wakearg) *HP*/
	} so_rcv, so_snd;

#define	SB_MAX		(64*1024)	/* max chars in sockbuf */
#define	SB_LOCK		0x01		/* lock on data queue (so_rcv only) */
#define	SB_WANT		0x02		/* someone is waiting to lock */
#define	SB_WAIT		0x04		/* someone is waiting for data/space */
#define	SB_SEL		0x08		/* buffer is selected */
#define	SB_COLL		0x10		/* collision selecting */
#define	SB_MSGCOMPLETE	0x20		/* last message in sockbuf complete */
#define SB_COPYAVOID_REQ    0x1000  	/* user requested copy-on-write (COW)*/
#define SB_COPYAVOID_SUPP   0x2000 	/* lower layers support COW */
#define SB_NVS          0x4000   	/* socket linked to NVS pty */
#define SB_NVS_WAIT     0x8000  	/* waiting for output buffer space */
#define so_nvs_index    so_q0len

	int	so_ipc;			/* type of IPC using this struct */

#define SO_BSDIPC	1		/* BSD IPC */
#define SO_NETIPC 	2		/* HP Net IPC */
#define SO_ULIPC 	3		/* HP UL IPC */

	int	(*so_connindication)();	/* connection indication routine */
	caddr_t	so_ipccb;		/* pointer to IPC specific info */
};


/*
 * Socket state bits.
 */
#define	SS_NOFDREF		0x0001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x0002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x0004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x0008	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x0010	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x0020	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x0040	/* at mark on input */
#define	SS_PRIV			0x0080	/* privileged for broadcast, raw... */
#define	SS_NBIO			0x0100	/* non-blocking ops */
#define	SS_ASYNC		0x0200	/* async i/o notify */
#define SS_INTERRUPTED		0x0400  /* connect() was interrupted by signal*/
#define SS_NOWAIT		0x0800  /* no sleeps for any reason *HP*/
#define SS_NOUSER               0x1000  /* no user context (u.) *HP*/


/*HP* Bits for network events to sb_wakeup */
#define SE_ERROR	0x0001	/* so_error non-0 */
#define SE_HAVEDATA	0x0002	/* data in send or recv q */
#define SE_HAVEOOB	0x0004	/* oob data in recv q */
#define SE_DATAFULL	0x0008	/* send or recv q is full */
#define SE_CONNOUT	0x0010	/* outgoing connect complete (connect) */
#define SE_CONNIN	0x0020	/* incoming connect complete (listen)  */
#define SE_SENDCONN	0x0040	/* connected for send */
#define SE_RECVCONN	0x0080	/* connected for recv */
#define SE_POLL		0x4000	/* wakeup is synchronous poll */
#define SE_STATUS	0x8000	/* above status bits valid */


/*
 * Macros for sockets and socket buffering.
 */

/* how much space is there in a socket buffer (so->so_snd or so->so_rcv) */
#define	sbspace(sb) \
    (MIN((long)((sb)->sb_hiwat - (sb)->sb_cc),\
	 (long)((sb)->sb_mbmax - (sb)->sb_mbcnt)))

/* do we have to send all at once on a socket? */
#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)

/* can we read something from so? */
#define	soreadable(so) \
    ((so)->so_rcv.sb_cc || ((so)->so_state & SS_CANTRCVMORE) || \
	(so)->so_qlen || (so)->so_error)

/* can we write something to so? */
#define	sowriteable(so) \
    (sbspace(&(so)->so_snd) > 0 && \
	(((so)->so_state&SS_ISCONNECTED) || \
	  ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0) || \
     ((so)->so_state & SS_CANTSENDMORE) || \
     (so)->so_error)

/* adjust counters in sb reflecting allocation of m */
#define	sballoc(sb, m) { \
	(sb)->sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt += (m)->m_clsize; \
}

/* adjust counters in sb reflecting freeing of m */
#define	sbfree(sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt -= (m)->m_clsize; \
}


/* set lock on sockbuf sb */
/* we should not use sleep for SS_NOWAIT sockets */
#define sblock(sb) { \
	while ((sb)->sb_flags & SB_LOCK) { \
		VASSERT(!((sb)->sb_so->so_state & SS_NOWAIT)); \
		(sb)->sb_flags |= SB_WANT; \
		sleep((caddr_t)&(sb)->sb_flags, PZERO+1); \
	} \
	(sb)->sb_flags |= SB_LOCK; \
}


/* release lock on sockbuf sb */
#define	sbunlock(sb) { \
	(sb)->sb_flags &= ~SB_LOCK; \
	if ((sb)->sb_flags & SB_WANT) { \
		(sb)->sb_flags &= ~SB_WANT; \
		if ((sb)->sb_wakeup) \
			(void) sewakeup((sb)->sb_so, sb, 1); \
		else \
			wakeup((caddr_t)&(sb)->sb_flags); \
	} \
}

#define	sorwakeup(so)	sowakeup((so), &(so)->so_rcv)
#define	sowwakeup(so)	sowakeup((so), &(so)->so_snd)

#ifdef _KERNEL
struct	socket *sonewconn();
#endif
#endif	/* _SYS_SOCKETVAR_INCLUDED */
