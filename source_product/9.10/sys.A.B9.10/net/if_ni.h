/*
 * @(#)if_ni.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 19:00:17 $
 * $Locker:  $
 */

#ifndef	_SYS_IF_NI_INCLUDED
#define	_SYS_IF_NI_INCLUDED

#ifdef	_KERNEL

#define NI_MAX		10			/* XXX - # of "ni" interfaces */

struct ni_cb {
	struct ifnet	ni_if;			/* Interface structure */
	u_short		ni_flags;		/* Network interface flags */
	short		ni_pgrp;		/* Network interface pgrp */
	struct proc *	ni_sel;			/* Selecting process */
	mib_ifEntry	ni_mib;			/* Mib stats */
	u_char		ni_bound[AF_MAX];	/* protocols bound */
};

#define NIF_INUSE	0x0001			/* Interface is "in use" */
#define NIF_WAIT	0x0002			/* Driver is waiting */
#define NIF_COLL	0x0004			/* Select "collision" */
#define NIF_ASYNC	0x0008			/* ASYNC I/O enabled */
#define	NIF_NBIO	0x0010			/* Non-blocking I/O enabled */

#define NIM_TYPE	1			/* "Other" */
#define NIM_SPEED	1000000			/* what the h... */

struct ni_loginfo {
	int		nli_event;		/* Netisr event */
	struct ifqueue *nli_queue;		/* Protocol queue */
};

#endif /* _KERNEL */

struct ni_desc {
	char nd_name[64];
};

#define	NIOCBIND	_IOW('n',  1, int)	/* bind protocol */
#define	NIOCUNBIND	_IOW('n',  2, int)	/* unbind protocol */
#define	NIOCBOUND	_IOW('n',  3, int)	/* protocol bound */
#define	NIOCGFLAGS	_IOR('n',  4, int)	/* get interface flags */
#define	NIOCSFLAGS	_IOW('n',  5, int)	/* set interface flags */
#define	NIOCGMTU	_IOR('n',  6, int)	/* get interface MTU */
#define	NIOCSMTU	_IOW('n',  7, int)	/* set interface MTU */
#define	NIOCGPGRP	_IOR('n',  8, int)	/* get ASYNC I/O pgrp id */
#define	NIOCSPGRP	_IOW('n',  9, int)	/* set ASYNC I/O pgrp id */
#define	NIOCGQLEN	_IOR('n', 10, int)	/* get if_snd queue length */
#define	NIOCSQLEN	_IOW('n', 11, int)	/* set if_snd queue length */
#define	NIOCGUNIT	_IOR('n', 12, int)	/* get interface unit number */
#define	NIOCSMDESCR	_IOW('n', 13, struct ni_desc) /* set mib if desc */
#define	NIOCSMTYPE	_IOW('n', 14, int)	/* set mib if type */
#define	NIOCSMSPEED	_IOW('n', 15, int)	/* set mib if speed */

#endif /* _SYS_IF_NI_INCLUDED */
