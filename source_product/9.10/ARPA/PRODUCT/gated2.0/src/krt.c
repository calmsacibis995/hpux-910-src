/*
 *  $Header: krt.c,v 1.1.109.6 92/02/28 15:56:36 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


/* krt.c
 *
 * Kernel routing table interface routines
 */

#define ROUTE_KERNEL
#include "include.h"
#include <nlist.h>
#ifdef	SYSV
#include <sys/sioctl.h>
#include <sys/stropts.h>
#else	/* SYSV */
#include <sys/ioctl.h>
#endif	/* SYSV */
#include <sys/mbuf.h>
#ifndef vax11c
#include <sys/file.h>
#endif				/* vax11c */
#ifdef	RTM_ADD
#include <sys/kinfo.h>
#endif				/* RTM_ADD */

#if	RT_N_MULTIPATH != 1
Fatal error - the BSD Unix kernel can only support one next hop !
#endif

extern off_t lseek();

task *krt_task;				/* Task for kernel routing table */

#ifndef	RTM_ADD
static struct nlist *krt_rt[2];

#define	KRT_RTHOST	0
#define	KRT_RTNET	1
static struct nlist *krt_rthashsize;

#endif				/* RTM_ADD */
#ifndef	vax11c
static struct nlist *krt_version;

#else				/* vax11c */
static struct nlist *krt_multinet_version;
static struct nlist *krt_multinet_product_name;

#define read(kmem,buf,size)	klread(buf,size)
#define lseek(kmem,offset,how)	klseek(offset)
#endif				/* vax11c */

static struct {
    const char *nl_name;
    struct nlist **nl_ptr;
} nl_names[] = {

#ifndef	RTM_ADD
    {
	"_rthost", &krt_rt[KRT_RTHOST]
    },
    {
	"_rtnet", &krt_rt[KRT_RTNET]
    },
    {
	"_rthashsize", &krt_rthashsize
    },
#endif				/* RTM_ADD */
#ifndef	vax11c
    {
	"_version", &krt_version
    },
#else				/* vax11c */
    {
	"_multinet_version", &krt_multinet_version
    },
    {
	"_multinet_product_name", &krt_multinet_product_name
    },
#endif				/* vax11c */
    {
	NULL, NULL
    }
};

#define	NL_SIZE	(sizeof (nl_names)/sizeof (nl_names[0]))

#if	defined(ULTRIX3_X) || defined(ULTRIX4_X) || defined(hpux)
typedef struct rtentry krt_type;

#define	krt_next	rt_next
#define	krt_size	sizeof(krt_type)
#define	krt_conv(ptr)	(&ptr)
#endif

#if	defined(SYSV)
typedef	struct msgb	krt_type;

#define	krt_next	b_next;
#define	krt_size	sizeof(krt_type)
#endif

#if	!defined(krt_next) && !defined(RTM_ADD)
typedef struct mbuf krt_type;

#define	krt_next	m_next
#define	krt_size	(MMINOFF + sizeof(struct rtentry))
#define krt_conv(ptr)	mtod(&ptr, struct rtentry *)
#endif				/* !defined(krt_next) && !defined(RTM_ADD) */


 /*	Delete a route given dest, gateway and flags	*/
/*ARGSUSED*/
int
krt_delete_dst(tp, dest, mask, gate, flags)
task *tp;
sockaddr_un *dest;
sockaddr_un *mask;
sockaddr_un *gate;
flag_t flags;
{
    int do_ioctl = !test_flag && install;
    struct rtentry krt;

    memset((caddr_t) & krt, (char) 0, sizeof(krt));
    sockcopy(dest, &krt.rt_dst);
    sockcopy(gate, &krt.rt_gateway);
    krt.rt_flags = flags;

    if (do_ioctl && (task_ioctl(tp->task_socket, SIOCDELRT, (caddr_t) & krt, sizeof (krt)) == -1)) {
	trace(TR_ALL, LOG_ERR, "krt_delete_dst: task: %s: SIOCDELRT %A via %A flags <%s>: %m",
	      task_name(tp),
	      &krt.rt_dst,
	      &krt.rt_gateway,
	      trace_bits(rt_flag_bits, krt.rt_flags));
	return (1);
    }
    return (0);
}


#if	defined(krt_next)
 /*  Read the kernel's routing table.			*/
static void
krt_rtread(kmem)
int kmem;
{
    int saveinstall = install;
    int i, hashsize = 0, rtbufsize, krt_table;
    flag_t table;
    register if_entry *ifp;
    struct rtentry *krt;
    krt_type *next, m_buf, **base;

    if (kmem < 0) {
	return;
    }

    /* Make sure we have the addresses of the hash tables */
    if ((krt_rt[KRT_RTHOST]->n_value == 0) || (krt_rt[KRT_RTNET]->n_value == 0)) {
	trace(TR_ALL, LOG_ERR, "krt_rtread: rthost and/or rtnet not in namelist");
	quit(errno);
    }

    /* Get the number of hash buckets */
    if (krt_rthashsize->n_value != 0) {
	(void) lseek(kmem, (off_t) krt_rthashsize->n_value, 0);
	(void) read(kmem, (caddr_t) & hashsize, sizeof(hashsize));
    }
    if (!hashsize) {
#ifdef	RTHASHSIZ
	trace(TR_ALL, 0, "krt_rtread: defaulting rthashsize to RTHASHSIZ(%d)",
	      hashsize = RTHASHSIZ);
#else				/* RTHASHSIZ */
	trace(TR_ALL, LOG_ERR, "krt_rtread: rthashsize not in namelist");
	quit(ENOENT);
#endif				/* RTHASHSIZ */
    }

    /* set up to read the table of hash chains */
    rtbufsize = hashsize * sizeof(krt_type *);
    base = (krt_type **) malloc((unsigned int) rtbufsize);
    if (base == NULL) {
	trace(TR_ALL, LOG_ERR, "krt_rtread: malloc: %m");
	quit(errno);
    }
    for (krt_table = KRT_RTHOST; krt_table <= KRT_RTNET; krt_table++) {
#ifdef	_IBMR2
	off_t root;

	if (lseek(kmem, (off_t) krt_rt[krt_table]->n_value, 0) == -1) {
	    trace(TR_ALL, LOG_ERR, "krt_rtread: lseek rthash: %m");
	    quit(errno);
	}
	if (read(kmem, (caddr_t) &root, sizeof (root)) != sizeof (root)) {
	    trace(TR_ALL, LOG_ERR, "krt_rtread: read rthash: %m");
	    quit(errno);
	}
#else	/* _IBMR2 */
#define	root	krt_rt[krt_table]->n_value
#endif	/* _IBMR2 */

	if (lseek(kmem, (off_t) root, 0) == -1) {
	    trace(TR_ALL, LOG_ERR, "krt_rtread: lseek rthash: %m");
	    quit(errno);
	}
	if (read(kmem, (caddr_t) base, rtbufsize) != rtbufsize) {
	    trace(TR_ALL, LOG_ERR, "krt_rtread: read rthash: %m");
	    quit(errno);
	}
	for (i = 0; i < hashsize; i++) {
	    for (next = base[i]; next != NULL; next = m_buf.krt_next) {
#ifdef	SYSV
		struct rtentry krt_rtentry;
#endif	/* SYSV */

		if (lseek(kmem, (off_t) next, 0) == -1) {
		    trace(TR_ALL, LOG_ERR, "krt_rtread: lseek rtentry: %m");
		    quit(errno);
		}
		if (read(kmem, (caddr_t) & m_buf, krt_size) != krt_size) {
		    trace(TR_ALL, LOG_ERR, "krt_rtread: read rtentry: %m");
		    quit(errno);
		}
#ifdef	SYSV
		if (lseek(kmem, (off_t) m_buf.b_rptr, 0) == -1) {
		    trace(TR_ALL, LOG_ERR, "krt_rtread: lseek msg->rtentry: %m");
		    quit(errno);
		}
		if (read(kmem, (caddr_t) & krt_rtentry, sizeof (krt_rtentry)) != sizeof (krt_rtentry)) {
		    trace(TR_ALL, LOG_ERR, "krt_rtread: read msg->rtentry: %m");
		    quit(errno);
		}
		krt = &krt_rtentry;
#else	/* SYSV */
		krt = krt_conv(m_buf);
#endif	/* SYSV */

		if (krt->rt_gateway.sa_family != AF_INET) {
		    continue;
		}
		install = FALSE;	/* don't install routes in kernel */

		if (krt->rt_flags & RTF_HOST) {
		    table = RTS_HOSTROUTE;
		} else {
		    /*
	             *	Route is interior if we have an interface to it or a subnet of it
	             */
		    table = RTS_EXTERIOR;
		    IF_LIST(ifp) {
			if (gd_inet_wholenetof(socktype_in(&krt->rt_dst)->sin_addr)
			    == gd_inet_wholenetof(ifp->int_addr.in.sin_addr)) {
			    table = RTS_INTERIOR;
			    break;
			}
		    } IF_LISTEND(ifp) ;
		}

		/*
	         *	If Kernel route already exists, delete this one, the kernel uses the
	         *	first one
	         */
		if (rt_locate(table, (sockaddr_un *) & krt->rt_dst, RTPROTO_KERNEL)) {
		    goto Delete;
		}
		/*
	         *	If there was a problem adding the route, delete the kernel route
	         */
		if (!rt_add((sockaddr_un *) & krt->rt_dst,
			    (sockaddr_un *) 0,
			    (sockaddr_un *) & krt->rt_gateway,
			    (gw_entry *) 0,
			    0,
			    table,
			    RTPROTO_KERNEL,
			    0,
			    (time_t) 0,
			    RTPREF_KERNEL)) {
		    goto Delete;
		}
		continue;

	      Delete:
		install = saveinstall;
		krt_delete_dst(krt_task,
			       (sockaddr_un *) & krt->rt_dst,
			       (sockaddr_un *) 0,
			       (sockaddr_un *) & krt->rt_gateway,
			       (flag_t) krt->rt_flags);
	    }
	}
    }
    free((caddr_t) base);

    install = saveinstall;
    
    return;
}

#endif				/* defined(krt_next) */


#ifdef	RTM_ADD

/* Support for BSD4.4 route socket. */

#define	KRT_TIMEOUT	2		/* Length of time before a response is overdue */

#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))

static bits rtm_type_bits[] =
{
    {RTM_ADD, "ADD"},
    {RTM_DELETE, "DELETE"},
    {RTM_CHANGE, "CHANGE"},
    {RTM_GET, "GET"},
    {RTM_LOSING, "LOSING"},
    {RTM_REDIRECT, "REDIRECT"},
    {RTM_MISS, "MISS"},
    {RTM_LOCK, "LOCK"},
    {RTM_OLDADD, "OLDADD"},
    {RTM_OLDDEL, "OLDDEL"},
#ifdef	RTM_RESOLVE
    {RTM_RESOLVE, "RESOLVE"},
#endif				/* RTM_RESOLVE */
};

static bits rtm_lock_bits[] =
{
    {RTV_MTU, "MTU"},
    {RTV_HOPCOUNT, "HOPCOUNT"},
    {RTV_EXPIRE, "EXPIRE"},
    {RTV_RPIPE, "RPIPE"},
    {RTV_SPIPE, "SPIPE"},
    {RTV_SSTHRESH, "SSTHRESH"},
    {RTV_RTT, "RTT"},
    {RTV_RTTVAR, "RTTVAR"},
};

static bits rtm_sock_bits[] =
{
    {RTA_DST, "DST"},
    {RTA_GATEWAY, "GATEWAY"},
    {RTA_NETMASK, "NETMASK"},
    {RTA_GENMASK, "GENMASK"},
    {RTA_IFP, "IFP"},
    {RTA_IFA, "IFA"},
    {RTA_AUTHOR, "AUTHOR"}
};

struct rtm_msg {
    struct rtm_msg *rtm_forw;
    struct rtm_msg *rtm_back;
    /* How about some statistics */
    struct rt_msghdr msghdr;
};

static struct rtm_msg rtm_head =
{&rtm_head, &rtm_head};			/* Head of message queue */

/* Trace a route socket packet */
/*ARGSUSED*/
static void
krt_trace(tp, direction, rtp)
task *tp;
char *direction;
struct rt_msghdr *rtp;
{
    sockaddr_un *ap;

    /* XXX - print minimal information, more if TR_UPDATE enabled */
    tracef("KRT %s  length %d  version %d  type %s(%d)  addrs %s(%x)  pid %d  seq %d  error %d",
	   direction,
	   rtp->rtm_msglen,
	   rtp->rtm_version,
	   trace_state(rtm_type_bits, rtp->rtm_type - 1),
	   rtp->rtm_type,
	   trace_bits(rtm_sock_bits, rtp->rtm_addrs),
	   rtp->rtm_addrs,
	   rtp->rtm_pid,
	   rtp->rtm_seq,
	   rtp->rtm_errno);
    if (rtp->rtm_errno) {
	errno = rtp->rtm_errno;
	trace(TR_KRT, 0, "%m");
    } else {
	trace(TR_KRT, 0, NULL);
    }

    tracef("KRT %s  flags %s(%x)",
	   direction,
	   trace_bits(rt_flag_bits, rtp->rtm_flags),
	   rtp->rtm_flags);
    tracef("  locks %s(%x)",
	   trace_bits(rtm_lock_bits, rtp->rtm_rmx.rmx_locks),
	   rtp->rtm_rmx.rmx_locks);
    trace(TR_KRT, 0, "  inits %s(%x)",
	  trace_bits(rtm_lock_bits, rtp->rtm_inits),
	  rtp->rtm_inits);

    /* Display metrics */
    switch (rtp->rtm_type) {
	case RTM_ADD:
	case RTM_CHANGE:
	case RTM_GET:
	    trace(TR_KRT, 0, "KRT %s  mtu %d  hopcount %d  expire %d  ssthresh %d",
		  direction,
		  rtp->rtm_rmx.rmx_mtu,
		  rtp->rtm_rmx.rmx_hopcount,
		  rtp->rtm_rmx.rmx_expire,
		  rtp->rtm_rmx.rmx_ssthresh);
	    trace(TR_KRT, 0, "KRT %s  recvpipe %d  sendpipe %d  rtt %d  rttvar %d",
		  direction,
		  rtp->rtm_rmx.rmx_recvpipe,
		  rtp->rtm_rmx.rmx_sendpipe,
		  rtp->rtm_rmx.rmx_rtt,
		  rtp->rtm_rmx.rmx_rttvar);
	    break;
    }
    ap = (sockaddr_un *) (rtp + 1);

    /* Display addresses */
    if (rtp->rtm_addrs & RTA_DST) {
	tracef("KRT %s  dest %A",
	       direction,
	       ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_GATEWAY) {
	tracef("  next hop %A",
	       ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_NETMASK) {
	tracef("  mask %A",
	       ap);
	if (ap->a.sa_len) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	} else {
	    ap = (sockaddr_un *) ((caddr_t) ap + sizeof(u_long));
	}
    }
    if (rtp->rtm_addrs & RTA_GENMASK) {
	tracef("  genmask %A", ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_IFP) {
	tracef("  ifp %A", ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_IFA) {
	tracef("  ifa %A", ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_AUTHOR) {
	tracef("  author %A",
	       ap);
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    trace(TR_KRT, 0, NULL);

    trace(TR_KRT, 0, NULL);
}


/* Use the getkinfo() system call to read the routing table(s) */
/*ARGSUSED*/
static void
krt_rtread(kmem)
int kmem;
{
    int saveinstall = install;
    int size;
    caddr_t kbuf, cp, limit;
    struct rt_msghdr *rtp;
    flag_t table;
    if_entry *ifp;
    sockaddr_un *ap;
    sockaddr_un *dest = (sockaddr_un *) 0;
    sockaddr_un *mask = (sockaddr_un *) 0;
    sockaddr_un *gate = (sockaddr_un *) 0;

    if ((size = getkerninfo(KINFO_RT_DUMP, (caddr_t) 0, (int *) 0, 0)) < 0) {
	trace(TR_ALL, LOG_ERR, "krt_rtread: getkerninfo(KINFO_RT_DUMP) estimate: %m");
	quit(errno);
    }
    if (trace_flags & TR_PROTOCOL) {
	trace(TR_KRT, 0, "krt_rtread: getkerninfo(KINFO_RT_DUMP) estimates %d bytes needed",
	      size);
    }
    kbuf = (caddr_t) malloc(size);
    if (!kbuf) {
	trace(TR_ALL, LOG_ERR, "krt_rtread: malloc(%d) failed",
	      size);
	quit(ENOMEM);
    }
    if (getkerninfo(KINFO_RT_DUMP, kbuf, &size, 0) < 0) {
	trace(TR_ALL, LOG_ERR, "krt_rtread: getkerninfo(KINFO_RT_DUMP): %m");
	quit(errno);
    }
    limit = kbuf + size;

    for (cp = kbuf; cp < limit; cp += rtp->rtm_msglen) {
	sockaddr_un addr;

	rtp = (struct rt_msghdr *) cp;

	if (rtp->rtm_version != RTM_VERSION) {
	    trace(TR_ALL, LOG_ERR, "krt_rtread: version mismatch!  Expected %d, received %d",
		  RTM_VERSION,
		  rtp->rtm_version);
	    quit(EPROTONOSUPPORT);
	}
	krt_trace(krt_task, "KINFO", rtp);

	ap = (sockaddr_un *) (rtp + 1);

	if (rtp->rtm_addrs & RTA_DST) {
	    dest = ap;
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->rtm_addrs & RTA_GATEWAY) {
	    gate = ap;
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->rtm_addrs & RTA_NETMASK) {
	    sockcopy(ap, &addr);
	    mask = &addr;
	    mask->a.sa_family = dest->a.sa_family;
	    if (ap->a.sa_len) {
		ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	    } else {
		ap = (sockaddr_un *) ((caddr_t) ap + sizeof(u_long));
	    }
	} else {
	    mask = (sockaddr_un *) 0;
	}

	if (rtp->rtm_addrs & RTA_GENMASK) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->rtm_addrs & RTA_IFP) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->rtm_addrs & RTA_IFA) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->rtm_addrs & RTA_AUTHOR) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (dest->a.sa_family != AF_INET) {
	    continue;
	}
	install = FALSE;		/* don't install routes in kernel */

	if (rtp->rtm_flags & RTF_HOST) {
	    table = RTS_HOSTROUTE;
	} else {
	    /*
	     *	Route is interior if we have an interface to it or a subnet of it
	     */
	    table = RTS_EXTERIOR;
	    IF_LIST(ifp) {
		if (gd_inet_wholenetof(dest->in.sin_addr) == gd_inet_wholenetof(ifp->int_addr.in.sin_addr)) {
		    table = RTS_INTERIOR;
		    break;
		}
	    } IF_LISTEND(ifp) ;
	}

	/*
	 *	If Kernel route already exists, delete this one, the kernel uses the
	 *	first one
	 */
	if (rt_locate(table, dest, RTPROTO_KERNEL)) {
	    goto Delete;
	}
	/*
	 *	If there was a problem adding the route, delete the kernel route
	 */
	if (!rt_add(dest,
		    mask,
		    gate,
		    (gw_entry *) 0,
		    0,
		    table,
		    RTPROTO_KERNEL,
		    0,
		    (time_t) 0,
		    RTPREF_KERNEL)) {
	    goto Delete;
	}
	continue;

      Delete:
	install = saveinstall;
	krt_delete_dst(krt_task,
		       dest,
		       mask,
		       gate,
		       (flag_t) rtp->rtm_flags);

    }

    (void) free(kbuf);

    install = saveinstall;
}

static void krt_remqueue();

/* Issue a request */
static void
krt_send(tp, rtp)
task *tp;
struct rtm_msg *rtp;
{
    int error = 0;
    const char *sent = "SENT";

    if (!rtp->msghdr.rtm_seq) {
	rtp->msghdr.rtm_seq = ++rtm_head.msghdr.rtm_seq;
	rtp->msghdr.rtm_version = RTM_VERSION;
    }
    rtp->msghdr.rtm_pid = my_pid;

    if (!test_flag && install && tp->task_socket != -1) {
	if (write(tp->task_socket, (caddr_t) &rtp->msghdr, rtp->msghdr.rtm_msglen) < 0) {
	    error = errno;
	    trace(TR_ALL, LOG_ERR, "krt_send: write: %m");
	    sent = "*NOT SENT*";
	}

    }
    if (trace_flags & TR_KRT) {
	krt_trace(tp, sent, &rtp->msghdr);
    }

    switch (error) {
    case EWOULDBLOCK:
    case ENOBUFS:
	/* Indicate request should be retried if the error is not fatal */
	rtp->msghdr.rtm_pid = 0;
	timer_set(tp->task_timer[0], (time_t) KRT_TIMEOUT);
	break;

    default:
	krt_remqueue(tp, rtp);
    }
}


/* Insert at the end of the request queue.  If this is the first element, */
/* call krt_send() to initiate the action. */
static void
krt_addqueue(tp, rtp)
task *tp;
struct rtm_msg *rtp;
{
    /* XXX - Should have logic to consolidate duplicates in the queue */

    /* Insert at the end of the queue */
    insque((struct qelem *) rtp, (struct qelem *) rtm_head.rtm_back);

    if (rtm_head.rtm_forw == rtp) {
	krt_send(tp, rtp);
    }
}


/* Dequeue a successful response.  If there are more on the queue, call */
/* krt_send() to initiate the action */
static void
krt_remqueue(tp, rtp)
task *tp;
struct rtm_msg *rtp;
{
    /* Remove this element from the queue and free it */
    remque((struct qelem *) rtp);
    (void) free((caddr_t) rtp);

    if (rtm_head.rtm_forw != &rtm_head) {
	/* Issue the next request */
	krt_send(tp, rtm_head.rtm_forw);
    } else {
	/* No more requests, reset the timer */
	timer_reset(tp->task_timer[0]);
    }

}


/* Fill in a request and enqueue it */
/* XXX - should allocate large chunks */
static void
krt_request(type, rt)
int type;
rt_entry *rt;
{
    int size;
    struct rtm_msg *rtp;
    struct sockaddr *ap;

    if (!install) {
	return;
    }
    size = sizeof(struct rtm_msg);
    /* Hack to make sure socket lengths are set correctly */
    rt->rt_dest.a.sa_len = sizeof (struct sockaddr_in);
    rt->rt_dest_mask.a.sa_len = sizeof (struct sockaddr_in);
    rt->rt_router.a.sa_len = sizeof (struct sockaddr_in);
    size += socksize(&rt->rt_dest) + socksize(&rt->rt_router);
    size += socksize(&rt->rt_dest_mask) ? socksize(&rt->rt_dest_mask) : sizeof(u_long);

    rtp = (struct rtm_msg *) calloc(1, size);
    if (!rtp) {
	trace(TR_ALL, LOG_ERR, "krt_request: calloc: %m");
	quit(errno);
    }
    rtp->msghdr.rtm_type = type;
    rtp->msghdr.rtm_flags = rt->rt_flags;
    if (rt->rt_ifp->int_state & IFS_UP) {
	rtp->msghdr.rtm_flags |= RTF_UP;
    }
    rtp->msghdr.rtm_msglen = size - (sizeof (struct rtm_msg) - sizeof (struct rt_msghdr));

    /* XXX - set metrics */

    ap = (struct sockaddr *) (rtp + 1);

    sockcopy(&rt->rt_dest, ap);
    ap = (struct sockaddr *) ((caddr_t) ap + ROUNDUP(rt->rt_dest.a.sa_len));
    rtp->msghdr.rtm_addrs |= RTA_DST;

    sockcopy(&rt->rt_router, ap);
    ap = (struct sockaddr *) ((caddr_t) ap + ROUNDUP(rt->rt_router.a.sa_len));
    rtp->msghdr.rtm_addrs |= RTA_GATEWAY;

    if (socksize(&rt->rt_dest_mask)) {
	sockcopy(&rt->rt_dest_mask, ap);
	ap = (struct sockaddr *) ((caddr_t) ap + ROUNDUP(rt->rt_dest_mask.a.sa_len));
    } else {
	memset((caddr_t) ap, 0, sizeof(u_long));
	ap = (struct sockaddr *) ((caddr_t) ap + sizeof(u_long));
    }
    rtp->msghdr.rtm_addrs |= RTA_NETMASK;

    krt_addqueue(krt_task, rtp);
}


/* Process a route socket response from the kernel */
static void
krt_recv(tp)
task *tp;
{
    int size;
    struct rt_msghdr *rtp = (struct rt_msghdr *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    sockaddr_un *ap;
    sockaddr_un *ap1 = (sockaddr_un *) 0;
    sockaddr_un *ap2 = (sockaddr_un *) 0;
    sockaddr_un *ap3 = (sockaddr_un *) 0;
    sockaddr_un *ap4 = (sockaddr_un *) 0;
    rt_entry *rt;

    if (task_receive_packet(tp, &size)) {
	return;
    }
    if (trace_flags & TR_KRT) {
	krt_trace(tp, "RECV", rtp);
    }
    ap = (sockaddr_un *) (rtp + 1);

    if (rtp->rtm_addrs & RTA_DST) {
	ap1 = ap;
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	if (ap1->a.sa_family != AF_INET) {
	    return;
	}
    }
    if (rtp->rtm_addrs & RTA_GATEWAY) {
	ap2 = ap;
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_NETMASK) {
	ap3 = ap;
	if (ap->a.sa_len) {
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	} else {
	    ap = (sockaddr_un *) ((caddr_t) ap + sizeof(u_long));
	}
    }
    if (rtp->rtm_addrs & RTA_GENMASK) {
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_IFP) {
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_IFA) {
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_addrs & RTA_AUTHOR) {
	ap4 = ap;
	ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
    }
    if (rtp->rtm_pid == my_pid) {
	/* XXX - to enable the use of multiple requests outstanding the queue */
	/* XXX - needs to be scanned for this sequence */
	if (rtp->rtm_seq != rtm_head.rtm_forw->msghdr.rtm_seq) {
	    trace(TR_ALL, 0, "krt_recv: invalid message sequence %d, expected %d",
		  rtp->rtm_seq,
		  rtm_head.rtm_forw->msghdr.rtm_seq);
	    return;
	}
	switch (rtp->rtm_type) {
	    case RTM_ADD:
	    case RTM_OLDADD:
	    case RTM_DELETE:
	    case RTM_OLDDEL:
	    case RTM_CHANGE:
	    case RTM_LOCK:
		if (!(rtp->rtm_flags & RTF_DONE)) {
		    errno = rtp->rtm_errno;
		    trace(TR_KRT, LOG_ERR, "krt_recv: %s request for %A/%A via %A: %m",
			  trace_state(rtm_type_bits, rtp->rtm_type - 1),
			  ap1,
			  ap3,
			  ap2);
		} else if (trace_flags & TR_PROTOCOL) {
		    trace(TR_KRT, 0, "krt_recv: sequence %d acknowledged",
			  rtp->rtm_seq);
		}
		krt_remqueue(tp, rtm_head.rtm_forw);
		break;
	    case RTM_GET:
		/* XXX - check for verification of a route change here */
		trace(TR_KRT, 0, "krt_recv: GET response of unknown origin");
		break;
	    default:
		trace(TR_ALL, LOG_ERR, "krt_recv: invalid message type %d",
		      rtp->rtm_type);
		break;
	}
    } else {
	/* Ignore incomplete messages */
	if (!(rtp->rtm_flags & RTF_DONE)) {
	    return;
	}
	switch (rtp->rtm_type) {
	    case RTM_CHANGE:
		if (rt = rt_locate(RTS_HOSTROUTE | RTS_NETROUTE, ap1, RTPROTO_KRT)) {
		    if (!rt_change(rt,
				   ap2,
				   (metric_t) 0,
				   (time_t) 0,	/* XXX - rtm_expire?? */
				   RTPREF_KRT)) {
			trace(TR_ALL, LOG_ERR, "krt_recv: error changing route to %A/%A via %A",
			      ap1,
			      ap3,
			      ap2);
		    }
		} else {
		    /* Route did not exist, add new one */
		    goto add;
		}
		break;
	    case RTM_ADD:
	    case RTM_OLDADD:
	    case RTM_DELETE:
	    case RTM_OLDDEL:
		/* Delete existing route */
		rt_open(tp);
		if (rt = rt_locate(RTS_HOSTROUTE | RTS_NETROUTE, ap1, RTPROTO_KRT)) {
		    (void) rt_delete(rt);
		}
		if (rtp->rtm_type == RTM_DELETE || rtp->rtm_type == RTM_OLDDEL) {
		    rt_close(tp, 0, 1);
		    break;
		}
		/* Add new route */
	      add:
		if (!rt_add(ap1,
			    ap3,
			    ap2,
			    (gw_entry *) 0,
			    (metric_t) 0,
			    RTS_NOAGE | RTS_NOADVISE,
			    RTPROTO_KRT,
			    (as_t) 0,
			    (time_t) 0,	/* XXX - rtm_expire?? */
			    RTPREF_KRT)) {
		    trace(TR_ALL, LOG_ERR, "krt_recv: error adding route to %A/%A via %A",
			  ap1,
			  ap3,
			  ap2);
		}
		rt_close(tp, 0, 1);
		break;
	    case RTM_GET:
		/* ignore */
		break;
	    case RTM_LOSING:
		trace(TR_KRT, 0, "krt_recv: kernel reports TCP lossage on route to %A/%A via %A",
		      ap1,
		      ap3,
		      ap2);
		break;
	    case RTM_REDIRECT:
		trace(TR_KRT, 0, "krt_recv: redirect to %A/%A via %A from %A",
		      ap1,
		      ap3,
		      ap2,
		      ap4);
		/* XXX - rt_redirect() needs to support a netmask */
		rt_redirect(tp, ap1, ap2, ap4, (rtp->rtm_flags & RTF_HOST) ? TRUE : FALSE);
		break;
	    case RTM_MISS:
		trace(TR_KRT, 0, "krt_recv: kernel can not find route to %A/%A via %A",
		      ap1,
		      ap3,
		      ap2);
		break;
	    case RTM_LOCK:
		/* XXX - ignore */
		break;
#ifdef	RTM_RESOLVE
	    case RTM_RESOLVE:
		/* XXX - ignore */
		break;
#endif				/* RTM_RESOLVE */
	}
    }
}


/* Deal with a timeout of a route socket response from the kernel */
/*ARGSUSED*/
static void
krt_timeout(tip, interval)
timer *tip;
time_t interval;
{
    if (rtm_head.rtm_forw->msghdr.rtm_pid) {
	/* No response during timeout period */
	if (rtm_head.rtm_forw->msghdr.rtm_type != RTM_GET) {
	    /* XXX - request timed out, issue a get to determine if it succeded */
	} else {
	    /* Reset pid to indicate a retry */
	    rtm_head.rtm_forw->msghdr.rtm_pid = 0;
	    krt_send(tip->timer_task, rtm_head.rtm_forw);
	}
    } else {
	/* Write failed last time, retry */
	krt_send(tip->timer_task, rtm_head.rtm_forw);
    }
}

#endif				/* RTM_ADD */


int
krt_add(new_rt)
rt_entry *new_rt;
{
    int error = 0;

#ifndef	RTM_ADD
    int do_ioctl = !test_flag && install;
    struct rtentry krt;

#endif				/* RTM_ADD */

    if (new_rt->rt_state & RTS_NOTINSTALL) {
	return (error);
    }
    tracef("KERNEL ADD    %-15A mask %-15A gateway %-15A flags <%s>",
	   &new_rt->rt_dest,
	   &new_rt->rt_dest_mask,
	   &new_rt->rt_router,
	   trace_bits(rt_flag_bits, new_rt->rt_flags));

#ifdef	RTM_ADD
    trace(TR_KRT, 0, NULL);
    krt_request(RTM_ADD, new_rt);
#else				/* RTM_ADD */
    memset((caddr_t) & krt, (char) 0, sizeof(krt));
    krt.rt_dst = new_rt->rt_dest.a;	/* struct copy */
    krt.rt_gateway = new_rt->rt_router.a;	/* struct copy */
    krt.rt_flags = new_rt->rt_flags;
    if (new_rt->rt_ifp->int_state & IFS_UP) {
	krt.rt_flags |= RTF_UP;
    }
    if (do_ioctl && (task_ioctl(krt_task->task_socket, SIOCADDRT, (caddr_t) & krt, sizeof (krt)) < 0)) {
	error = errno;
	trace(TR_ALL | TR_NOSTAMP, LOG_ERR, " SIOCADDRT: %m");
    } else {
	trace(TR_KRT | TR_NOSTAMP, 0, NULL);
    }
#endif				/* RTM_ADD */

    return (error);
}


int
krt_delete(old_rt)
rt_entry *old_rt;
{
    int error = 0;

#ifndef	RTM_ADD
    int do_ioctl = !test_flag && install;
    struct rtentry krt;

#endif				/* RTM_ADD */

    if (old_rt->rt_state & RTS_NOTINSTALL) {
	return (error);
    }
    tracef("KERNEL DELETE %-15A mask %-15A gateway %-15A flags <%s>",
	   &old_rt->rt_dest,
	   &old_rt->rt_dest_mask,
	   &old_rt->rt_router,
	   trace_bits(rt_flag_bits, old_rt->rt_flags));

#ifdef	RTM_ADD
    trace(TR_KRT, 0, NULL);
    krt_request(RTM_DELETE, old_rt);
#else				/* RTM_ADD */
    memset((caddr_t) & krt, (char) 0, sizeof(krt));
    krt.rt_dst = old_rt->rt_dest.a;	/* struct copy */
    krt.rt_gateway = old_rt->rt_router.a;	/* struct copy */
    krt.rt_flags = old_rt->rt_flags;
    if (old_rt->rt_ifp->int_state & IFS_UP) {
	krt.rt_flags |= RTF_UP;
    }
    if (do_ioctl && (task_ioctl(krt_task->task_socket, SIOCDELRT, (caddr_t) & krt, sizeof (krt)) < 0)) {
	error = errno;
	trace(TR_ALL | TR_NOSTAMP, LOG_ERR, " SIOCDELRT: %m");
    } else {
	trace(TR_KRT | TR_NOSTAMP, 0, NULL);
    }
#endif				/* RTM_ADD */

    return (error);
}


int
krt_change(old_rt, new_rt)
rt_entry *old_rt, *new_rt;
{
    int error = 0;

    if (old_rt && new_rt && ((old_rt->rt_state & RTS_NOTINSTALL) == (new_rt->rt_state & RTS_NOTINSTALL))) {
	if ((old_rt->rt_state & RTS_NOTINSTALL) ||
	    equal(&old_rt->rt_router, &new_rt->rt_router) &&
	    (old_rt->rt_flags == new_rt->rt_flags)) {
	    return (error);
#ifdef	RTM_ADD
	} else {
	    trace(TR_KRT, 0, "KERNEL CHANGE %-15A mask %-15A old: gateway %-15A flags <%s> new: gateway %-15A flags <%s>",
		  &old_rt->rt_dest,
		  &old_rt->rt_dest_mask,
		  &old_rt->rt_router,
		  trace_bits(rt_flag_bits, old_rt->rt_flags),
		  &new_rt->rt_router,
		  trace_bits(rt_flag_bits, new_rt->rt_flags));

	    krt_request(RTM_CHANGE, new_rt);
	    return (error);
#endif				/* RTM_ADD */
	}
    }
    if (new_rt && !(new_rt->rt_state & RTS_NOTINSTALL)) {
	error = krt_add(new_rt);
    }
    if (!error && old_rt && !(old_rt->rt_state & RTS_NOTINSTALL)) {
	error = krt_delete(old_rt);
    }
    return (error);
}



#ifdef	RTM_ADD
static void
krt_dump(fp)
FILE *fp;
{
    struct rtm_msg *rtp;
    sockaddr_un *ap;

    (void) fprintf(fp, "Route socket:\n");

    (void) fprintf(fp, "\tSequence:\t%d\n",
		   rtm_head.msghdr.rtm_seq);

    for (rtp = rtm_head.rtm_forw; rtp != &rtm_head; rtp = rtp->rtm_forw) {
	(void) fprintf(fp, "\t\tlength %u  version %u  type %s(%u)  addrs %s(%x)  pid %d  seq %d  error %d",
		       rtp->msghdr.rtm_msglen,
		       rtp->msghdr.rtm_version,
		    trace_state(rtm_type_bits, rtp->msghdr.rtm_type - 1),
		       rtp->msghdr.rtm_type,
		       trace_bits(rtm_sock_bits, rtp->msghdr.rtm_addrs),
		       rtp->msghdr.rtm_addrs,
		       rtp->msghdr.rtm_pid,
		       rtp->msghdr.rtm_seq,
		       rtp->msghdr.rtm_errno);
	if (rtp->msghdr.rtm_errno) {
	    errno = rtp->msghdr.rtm_errno;
	    (void) fprintf(fp, " %m\n");
	} else {
	    (void) fprintf(fp, "\n");
	}

	(void) fprintf(fp, "\t\tflags %s(%x)",
		       trace_bits(rt_flag_bits, rtp->msghdr.rtm_flags),
		       rtp->msghdr.rtm_flags);
	(void) fprintf(fp, "  locks %s(%x)",
		trace_bits(rtm_lock_bits, rtp->msghdr.rtm_rmx.rmx_locks),
		       rtp->msghdr.rtm_rmx.rmx_locks);
	(void) fprintf(fp, "  inits %s(%x)\n",
		       trace_bits(rtm_lock_bits, rtp->msghdr.rtm_inits),
		       rtp->msghdr.rtm_inits);

	/* Display metrics */
	switch (rtp->msghdr.rtm_type) {
	    case RTM_ADD:
	    case RTM_CHANGE:
	    case RTM_GET:
		(void) fprintf(fp, "\t\tmtu %u  hopcount %u  expire %u  ssthresh %u\n",
			       rtp->msghdr.rtm_rmx.rmx_mtu,
			       rtp->msghdr.rtm_rmx.rmx_hopcount,
			       rtp->msghdr.rtm_rmx.rmx_expire,
			       rtp->msghdr.rtm_rmx.rmx_ssthresh);
		(void) fprintf(fp, "\t\trecvpipe %u  sendpipe %u  rtt %u  rttvar %u\n",
			       rtp->msghdr.rtm_rmx.rmx_recvpipe,
			       rtp->msghdr.rtm_rmx.rmx_sendpipe,
			       rtp->msghdr.rtm_rmx.rmx_rtt,
			       rtp->msghdr.rtm_rmx.rmx_rttvar);
		break;
	}
	ap = (sockaddr_un *) (rtp + 1);

	/* Display addresses */
	if (rtp->msghdr.rtm_addrs & RTA_DST) {
	    (void) fprintf(fp, "\t\tdest %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->msghdr.rtm_addrs & RTA_GATEWAY) {
	    (void) fprintf(fp, "  next hop %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->msghdr.rtm_addrs & RTA_NETMASK) {
	    (void) fprintf(fp, "  mask %A",
			   ap);
	    if (ap->a.sa_len) {
		ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	    } else {
		ap = (sockaddr_un *) ((caddr_t) ap + sizeof(u_long));
	    }
	}
	if (rtp->msghdr.rtm_addrs & RTA_GENMASK) {
	    (void) fprintf(fp, "  genmask %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->msghdr.rtm_addrs & RTA_IFP) {
	    (void) fprintf(fp, "  ifp %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->msghdr.rtm_addrs & RTA_IFA) {
	    (void) fprintf(fp, "  ifa %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	if (rtp->msghdr.rtm_addrs & RTA_AUTHOR) {
	    (void) fprintf(fp, "  author %A",
			   ap);
	    ap = (sockaddr_un *) ((caddr_t) ap + ROUNDUP(ap->a.sa_len));
	}
	(void) fprintf(fp, "\n\n");
    }
}

#endif				/* RTM_ADD */


 /*  Initilize the kernel routing table function.  First, create a	*/
 /*  task to hold the socket used in manipulating the kernel routing	*/
 /*  table.  Second, read the initial kernel routing table into		*/
 /*  gated's routing table.						*/
void
krt_init()
{
    int i, kmem;
    int krt_socket;
    struct nlist *nl = (struct nlist *) 0;

#ifndef	AF_ROUTE
#define	AF_ROUTE	AF_INET
#endif				/* AF_ROUTE */
    if ((krt_socket = task_get_socket(AF_ROUTE, SOCK_RAW, 0)) < 0) {
	quit(errno);
    }
    krt_task = task_alloc("KRT");
    krt_task->task_proto = IPPROTO_RAW;
    krt_task->task_socket = krt_socket;
#ifdef	RTM_ADD
    krt_task->task_dump = krt_dump;
    krt_task->task_recv = krt_recv;
#endif				/* RTM_ADD */
    krt_task->task_rtproto = RTPROTO_KERNEL;
    if (!task_create(krt_task, 0)) {
	quit(EINVAL);
    }
#ifdef	RTM_ADD
    (void) timer_create(krt_task,
			0,
			"Timeout",
			TIMERF_ABSOLUTE,
			(time_t) 0,
			krt_timeout);

    if (task_set_option(krt_task,
			TASKOPTION_NONBLOCKING,
			(caddr_t) TRUE) < 0) {
	quit(errno);
    }
    /* Indicate we do not want to see our packets */
    if (task_set_option(krt_task,
			TASKOPTION_USELOOPBACK,
			(caddr_t) FALSE) < 0) {
	quit(errno);
    }
    
#endif				/* RTM_ADD */


    /* Build nlist from symbol names */
    nl = (struct nlist *) calloc(NL_SIZE, sizeof(struct nlist));
    for (i = NL_SIZE; i--;) {
	/* Use memcpy to avoid warning about const char * */
	memcpy((caddr_t) & nl[i].n_name, (caddr_t) & nl_names[i].nl_name, sizeof(char *));
#ifdef	NLIST_NOUNDER
	if (nl[i].n_name) {
	    nl[i].n_name++;
	}
#endif				/* NLIST_NOUNDER */
	if (nl_names[i].nl_ptr) {
	    *nl_names[i].nl_ptr = &nl[i];
	}
    }

#ifndef vax11c
    if (nlist(UNIX_NAME, nl) < 0) {
	trace(TR_ALL, LOG_ERR, "krt_init: nlist(\"%s\"): %m",
	      UNIX_NAME);
	if (test_flag) {
	    return;
	} else {
	    quit(errno);
	}
    }
    kmem = open("/dev/kmem", O_RDONLY, 0);
    if (kmem < 0) {
	trace(TR_ALL, LOG_ERR, "krt_init: open(\"/dev/kmem\"): %m");
	if (test_flag) {
	    kmem = -1;
	} else {
	    quit(errno);
	}
    }
    if (krt_version->n_value && (kmem >= 0)) {
	char *p;

	if ((version_kernel = (char *) calloc(1, BUFSIZ)) == 0) {
	    trace(TR_ALL, LOG_ERR, "krt_init: calloc: %m");
	    quit(errno);
	}
	(void) lseek(kmem, (off_t) krt_version->n_value, 0);
	(void) read(kmem, version_kernel, BUFSIZ - 1);
	if (p = (char *) index(version_kernel, '\n')) {
	    *p = NULL;
	}
	if ((p = (char *) malloc((unsigned int) strlen(version_kernel)+1)) == 0) {
	    trace(TR_ALL, LOG_ERR, "krt_init: malloc: %m");
	    quit(errno);
	}
	(void) strcpy(p, version_kernel);
	free(version_kernel);
	version_kernel = p;
	trace(TR_INT, 0, NULL);
	trace(TR_INT, 0, "krt_init: %s = %s",
	      krt_version->n_name,
	      version_kernel);
    } else {
	version_kernel = NULL;
    }
#else				/* vax11c */
    {
	extern char *Network_Image_File;

	(void) multinet_kernel_nlist(Network_Image_File, nl);
    }
    if (krt_multinet_version->n_value && krt_multinet_product_name->n_value && (kmem >= 0)) {
	char *p;

	if ((version_kernel = (char *) calloc(1, BUFSIZ)) == 0) {
	    trace(TR_ALL, LOG_ERR, "krt_init: calloc: %m");
	    quit(errno);
	}
	(void) lseek(kmem, (off_t) krt_multinet_product_name->n_value, 0);
	(void) read(kmem, version_kernel, BUFSIZ - 2);
	(void) strcat(version_kernel, " ");
	(void) lseek(kmem, (off_t) krt_multinet_version->n_value, 0);
	(void) read(kmem, version_kernel + strlen(version_kernel),
		    BUFSIZ - 1 - strlen(version_kernel));
	if ((p = (char *) malloc(strlen(version_kernel)+1)) == 0) {
	    trace(TR_ALL, LOG_ERR, "krt_init: malloc: %m");
	    quit(errno);
	}
	(void) strcpy(p, version_kernel);
	free(version_kernel);
	version_kernel = p;
	trace(TR_INT, 0, "krt_init: %s %s = %s",
	      krt_multinet_product_name->n_name,
	      krt_multinet_version->n_name,
	      version_kernel);
    } else {
	version_kernel = NULL;
    }
#endif				/* vax11c */

    rt_open(krt_task);

    trace(TR_RT, 0, "krt_init: Initial routes read from kernel:");

    krt_rtread(kmem);

    rt_close(krt_task, (gw_entry *) 0, 0);

    if (nl) {
	(void) free((caddr_t) nl);
    }
#ifndef vax11c
    if (kmem >= 0) {
	(void) close(kmem);
    }
#endif				/* vax11c */
}
