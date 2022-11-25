/* $Header: route.h,v 1.14.83.4 93/09/17 19:01:41 kcs Exp $ */

#ifndef _SYS_ROUTE_INCLUDED
#define _SYS_ROUTE_INCLUDED
/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
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
 *	@(#)route.h	7.4 (Berkeley) 6/27/88
 */

/*
 * Kernel resident routing tables.
 * 
 * The routing tables are initialized when interface addresses
 * are set by making entries for all directly connected interfaces.
 */

/*
 * A route consists of a destination address and a reference
 * to a routing entry.  These are often held by protocols
 * in their control blocks, e.g. inpcb.
 */
struct route {
	struct	rtentry *ro_rt;
	struct	sockaddr ro_dst;
	u_long	ro_flags;
	struct	socket	*ro_socket;
};

#define	RF_PRNOCKSUM	0x00010000 /* the protocol is not doing checksumming */
#define	RF_IFCKO	IFF_CKO	   /* the driver supports checksum offloading */
#define	RF_IFNOACC	IFF_NOACC  /* driver will not access data outbound */


struct routeaddrs {
	unsigned char *rthost;	/* Returns address of _rthost here */
	unsigned char *rtnet;	/* Returns address of _rthost here */
	unsigned char *rthashsize;	/* Returns address of _rthost here */
};

/*
 * We distinguish between routes to hosts and routes to networks,
 * preferring the former if available.  For each route we infer
 * the interface to use from the gateway address supplied when
 * the route was entered.  Routes that forward packets through
 * gateways are marked so that the output routines know to address the
 * gateway rather than the ultimate destination.
 */
struct rtentry {
	u_long	rt_hash;		/* to speed lookups */
	struct	sockaddr rt_dst;	/* key */
	struct	sockaddr rt_gateway;	/* value */
	u_short	rt_flags;		/* up/down?, host/net */
	u_short	rt_refcnt;		/* # held references */
	u_long	rt_use;			/* raw # packets forwarded */
	struct	ifnet *rt_ifp;		/* the answer: interface to use */
	struct	rtentry *rt_next;	/* next rtentry on hash chain */
	int	rt_proto;		/* netwk mgmt routing mechanism */
	int	rt_upd;			/* time route added/modified	*/
};

#define	RTF_UP		0x1		/* route useable */
#define	RTF_GATEWAY	0x2		/* destination is a gateway */
#define	RTF_HOST	0x4		/* host entry (net otherwise) */
#define	RTF_DYNAMIC	0x10		/* created dynamically (by redirect) */
#define	RTF_MODIFIED	0x20		/* modified dynamically (by redirect)*/
#define	RTF_DOUBTFUL	0x80		/* can we reliably reach the gateway?*/

#define	RTN_DOUBTFUL	0x01		/* reachability of gateway in doubt */
#define	RTN_NUKEREDIRS	0x02		/* as walking table, kill redirects */

/*
 * Routing statistics.
 */
struct	rtstat {
	short	rts_badredirect;	/* bogus redirect calls */
	short	rts_dynamic;		/* routes created by redirects */
	short	rts_newgateway;		/* routes modified by redirects */
	short	rts_unreach;		/* lookups which failed */
	short	rts_wildcard;		/* lookups satisfied by a wildcard */
	short	rts_setdoubtful;	/* doubltful flag set on a route */
	short	rts_cleardoubtful;	/* doubltful flag cleared on a route */
	short	rts_nukedredirects;	/* redirects nuked as advised */
};

#ifdef _KERNEL

/*
 * NIKE - define the macro which determines if we can support copy on write
 */
#define	COW_SUPP(ro) \
	(((ro)->ro_flags & RF_IFNOACC) && \
	 (((ro)->ro_flags & RF_IFCKO) || ((ro)->ro_flags & RF_PRNOCKSUM)))

#define	RTFREE(rt) \
	if ((rt)->rt_refcnt == 1) \
		rtfree(rt); \
	else \
		(rt)->rt_refcnt--;

#ifdef	GATEWAY
#define	RTHASHSIZ	64
#else
#define	RTHASHSIZ	8
#endif
#if	(RTHASHSIZ & (RTHASHSIZ - 1)) == 0
#define RTHASHMOD(h)	((h) & (RTHASHSIZ - 1))
#else
#define RTHASHMOD(h)	((h) % RTHASHSIZ)
#endif
extern struct rtentry *rthost[RTHASHSIZ];
extern struct rtentry *rtnet[RTHASHSIZ];
extern struct rtstat rtstat;
#endif	/* _KERNEL */
#endif  /* _SYS_ROUTE_INCLUDED */
