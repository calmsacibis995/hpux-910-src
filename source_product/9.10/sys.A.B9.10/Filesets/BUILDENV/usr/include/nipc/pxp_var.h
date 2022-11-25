/*
 * @(#)pxp_var.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 19:12:21 $
 * $Locker:  $
 */

#ifndef NIPC_VAR.H
#define NIPC_VAR.H

/*
 *  PXP variables 
 */

/*
 * Timeout values are in tenths of seconds.
 */
#define PXPVER		0	/* current pxp version number */
#define PXPCLS		1	/* current pxp class number */
#define PXPINITTO	30	/* initial timeout value           */
#define PXPMAXRETRY	10	/* default retry limit */
#define PXPMAXDATA	1400	/* maximum message data size in bytes */
#define PXPMAXTOCNT	10	/* timeout backoff value */
#define PXPQUEUELIMIT	10	/* maximum number of messages 
			           allowed to queue on a port  */
extern struct inpcb xpb;
extern int pxp_sndseq, pxp_rcvseq;

/* PXP Control Block, one for each message */

struct pxpcb {
       struct pxpcb	*xp_prev;	/* previous control block */	
       struct pxpcb	*xp_next;       /* next pxp control block */
       struct inpcb	*xp_inpcb;	/* back pointer to Internet pcb */
       short		xp_type;	/* pxp cb type; request or reply */
       struct in_addr	xp_sin_faddr;	/* message ip foreign address SJH */
       struct in_addr	xp_sin_laddr;	/* message ip local address SJH */
       u_short		xp_sin_port;	/* message port address */
       short		xp_timer;	/* pxp timers */
       short		xp_rxtshift;	/* rexmt. exp. backoff */
       short		xp_retries;	/* user specified retry number */
       short		xp_num_req;	/* current number of requests */
       struct mbuf	*xp_msg;	/* request message mbuf chain */
       int		xp_snd_msgid;	/* source message id */
       int		xp_rcv_msgid;	/* destination message id */
};             

#define intopxpcb(inp) \
    ((struct pxpcb *) ((struct pxpcb *)(inp->inp_ppcb))->xp_next)

#define PXPCB_NULL      ((struct pxpcb *)0)  /* pxpcb null pointer */
#define INPCB_NULL	((struct inpcb *)0)  /* inpcb null pointer */

#define E_MAXUNREPLIEDREQS	1000
#define E_SEQNUM		1001
#define	E_RETRYEXHAUSTED	1002

struct pxpstat {
	int pxp_hdrops;		/* bad header (used by netstat) */
	int pxp_badsum;		/* bad checksum (used by netstat) */
	int pxp_badlen;		/* bad length   (used by netstat) */
};

#endif /* NIPC_VAR.H */
