/*
 * $Header: rt_table.h,v 1.1.109.5 92/02/28 16:01:43 ash Exp $
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


/*
 * rt_table.h
 *
 * Routing table data and parameter definitions.
 *
 */

/*
 *	The routing table consists of an array of pointers to hash
 *	buckets.  Each hash bucket is a doubly linked list of rt_head
 *	entries.  Each rt_head entry contains a destination address and
 *	the root entry of a doubly linked lists of type rt_entry.  Each
 *	rt_entry contains information about how this occurance of a
 *	destination address was learned, next hop, ...
 *
 *	The rt_entry structure contains a pointer back to it's rt_head
 *	structure.
 */

/*
 *	Define link field as a macro.  These three fields must be in the same
 *	relative order in the rt_head and rt_entry structures.
 */
#define rt_link struct _rt_entry *rt_forw, *rt_back; struct _rt_head *rt_head

struct _rt_head {
    struct _rt_head *rth_forw;		/* Forward chain pointer */
    struct _rt_head *rth_back;		/* Backward chain pointer */
    sockaddr_un rth_dest;		/* The destination */
    sockaddr_un rth_dest_mask;		/* Subnet mask for this route */
    hash_t rth_hash;			/* Hash bucket index */
    flag_t rth_state;			/* Global state bits */
    int rth_entries;			/* Number of routes for this destintation */
    struct _rt_entry *rth_active;	/* Pointer to the active route */
     rt_link;				/* Routing table chain */
};


/* Prefix of protocol independent data */
typedef struct _rt_data {
    struct _rt_data *rtd_forw;		/* Chain pointers */
    struct _rt_data *rtd_back;		/* ... */
    int rtd_refcount;			/* Reference count */
    u_int rtd_length;			/* Length of data (after this head) */
    void (*rtd_dump) ();		/* Routine to format data */
    caddr_t rtd_data;			/* Pointer to data (which follows this structure) */
} rt_data;


struct _rt_entry {
    rt_link;				/* Chain and head pointers */
#define	rt_dest		rt_head->rth_dest	/* Route resides in rt_head */
#define	rt_dest_mask	rt_head->rth_dest_mask	/* Mask resides in rt_head */
#define	rt_active rt_head->rth_active	/* Pointer to the active route */
    sockaddr_un rt_router;		/* Next hop */
    if_entry *rt_ifp;			/* Interface to send said packets to */
    gw_entry *rt_sourcegw;		/* Gateway we learned this route from */
    task *rt_task;			/* Pointer to task that entered this route */
    time_t rt_timer;			/* Age of this route */
    time_t rt_timer_max;		/* Maximum allowed age of this route */
    metric_t rt_metric;			/* Interior metric of this route */
    flag_t rt_state;			/* Gated flags for this route */
    proto_t rt_proto;			/* Protocol for this route */
#ifdef	RTM_ADD
    mtu_t rt_mtu;			/* MTU to use for this destination (for 4.4 BSD) */
#endif				/* RTM_ADD */
    pref_t rt_preference;		/* Preference for this route */
    u_long rt_revision;			/* Revision of the routing table when route was changed */
    as_t rt_as;				/* AS from which this route was learned */
    flag_t rt_flags;			/* Kernel flags for this route */
    rt_data *rt_data;			/* Protocol specific data */
};

 /* Define the size of the routing hash table in number of buckets.	*/
 /* The larger the number the more buckets there are and the less	*/
 /* compares needed to find a particular route.  The more buckets	*/
 /* used, the more memory used.						*/
#define	ROUTEHASHSIZ	64
#define	ROUTEHASHMASK	(ROUTEHASHSIZ - 1)

 /* Defines for use in referencing routing tables via the routing	*/
 /* table index rt_tables.						*/
#define	RTI_HOST	0		/* Index into rt_tables for host table */
#define	RTI_NET		1		/* Index into rt_tables for net table */

/*
 * "State" of routing table entry.
 */
#define	RTS_POINTOPOINT	0x01		/* route is point-to-point */
#define RTS_SUBNET	0x02		/* is this a subnet route? */
#define	RTS_NOAGE	0x04		/* don't time out route */
#define	RTS_REMOTE	0x08		/* route is for ``remote'' entity */
#define	RTS_CHANGED	0x10		/* route has been altered recently */
#define	RTS_REFRESH	0x20		/* Route was refreshed, reset timer */
#define RTS_NOTINSTALL  0x40		/* don't install this route in kernel */
#define RTS_NOADVISE	0x80		/* This route not to be advised */
#define RTS_HOSTROUTE	0x0100		/* a host route */
#define RTS_INTERIOR    0x0200		/* an interior route */
#define RTS_EXTERIOR    0x0400		/* an exterior route */
#define	RTS_NETROUTE	RTS_INTERIOR|RTS_EXTERIOR
#define	RTS_HOLDDOWN	0x0800		/* Route is held down */
#define	RTS_DELETE	0x1000		/* Route is deleted */

#define DEFAULTNET	0x00000000	/* net # for default route */

#define RT_T_AGE	60		/* maximum time in seconds between route age increments. */
#define RT_T_EXPIRE	180		/* Age at which an interior route goes into holddown */
#define RT_T_HOLDDOWN	120		/* Holddown of a route in seconds */
#define	RT_T_DELETE	120		/* Time to keep a deleted route */
#define	RT_T_FREE	300		/* Length of time to keep an entry on the free list */

#define	RT_N_MULTIPATH	1		/* Number of multipath routes supported by forwarding engine */

#define RTPROTO_DIRECT		0x01	/* route is directly connected */
#define RTPROTO_KERNEL		0x02	/* route was received via KERNEL */
#define RTPROTO_REDIRECT	0x04	/* route was received via a redirect */
#define RTPROTO_DEFAULT		0x08	/* route is GATEWAY default */
#define RTPROTO_IGP		0x10	/* NSFnet backbone SPF */
#define	RTPROTO_OSPF		0x20	/* Open SPF */
#define	RTPROTO_IGRP		0x40	/* cisco IGRP */
#define RTPROTO_HELLO		0x80	/* DCN HELLO */
#define RTPROTO_RIP		0x0100	/* Berkeley RIP */
#define	RTPROTO_BGP		0x0200	/* Border gateway protocol */
#define RTPROTO_EGP		0x0400	/* route was received via EGP */
#define	RTPROTO_STATIC		0x8000	/* route is static */
#define	RTPROTO_SNMP		0x1000	/* route was installed by SNMP - also needed for parsing */
#define	RTPROTO_KRT		0x2000	/* route was learned via route socket */
#define	RTPROTO_ANY		0xffffffff	/* Matches any protocol */

/*
 *	Preferences of the various route types
 */
#define	RTPREF_DIRECT		0	/* Routes to interfaces */
#define	RTPREF_DEFAULT		10	/* defaultgateway and EGP default */
#define	RTPREF_REDIRECT		20	/* redirects */
#define	RTPREF_KRT		30	/* learned via route socket */
#define	RTPREF_SNMP		40	/* route installed by SNMP */
#define	RTPREF_STATIC		50	/* Static routes */
#define	RTPREF_IGP		60	/* NSFnet backbone SPF */
#define	RTPREF_OSPF		70	/* Open SPF IGP */
#define	RTPREF_IGRP		80	/* Cisco IGRP */
#define	RTPREF_HELLO		90	/* DCN Hello */
#define	RTPREF_RIP		100	/* Berkeley RIP */
#define	RTPREF_BGP		150	/* Border Gateway Protocol - external peer */
#define	RTPREF_EGP		200	/* Exterior Gateway Protocol */
#define RTPREF_IBGP		255	/* Border Gateway Protocol - internal peer */
#define	RTPREF_KERNEL		255	/* Routes in kernel at startup */


#define	TRACE_ACTION(action, route) { \
	    if (trace_flags & TR_RT) \
		rt_trace(action, route); \
	}

/*
 *	Defines for use by the various tables following
 */

 /* Hash into a table */
#define	RT_HASH(dst)	rt_head **rtb;\
    			u_long hash = gd_inet_hash(dst) & ROUTEHASHMASK; \
    				rtb = &rt_inet_hash[hash]

 /* Scan all buckets in a table */
#define	RT_SCTBL	{\
			   rt_head **rtb;\
			   for (rtb = rt_inet_hash; *rtb != (rt_head *) rt_inet_hash; rtb++)
#define	RT_SCTBL_END	}

 /* Scan all entries in this bucket */
#define	RT_BUCKET(rth)	{\
			   rt_head *rths = (rt_head *) 0;\
			   for (rth = *rtb; rth != rths; rth = rth->rth_forw, rths = *rtb)
#define	RT_BUCKET_END(rth)	if (rth == rths) rth = (rt_head *) 0; }

 /* Scan all routes for this destination */
#define	RT_ALLRT(rt, rth)	for (rt = rth->rt_forw; rt != (rt_entry *) &rth->rt_forw; rt = rt->rt_forw)
#define	RT_ALLRT_END(rt, rth)	if (rt == (rt_entry *) &rth->rt_forw) rt = (rt_entry *) 0;

 /* Only route in use for this destination */
#define	RT_IFRT(rt, rth)	if (rt = rth->rth_active)
#define	RT_IFRT_END

/*
 *	Macro to scan through entire active routing table
 */
#define RT_WHOLE(rt)	{ rt_head *rth; RT_SCTBL RT_BUCKET(rth) RT_ALLRT(rt, rth)

#define	RT_WHOLEEND(rt)	RT_ALLRT_END(rt, rth) RT_BUCKET_END(rth) RT_SCTBL_END }

/*
 *	Macro to scan through entire active routing table
 */
#define RT_TABLE(rt)	{ rt_head *rth;	RT_SCTBL RT_BUCKET(rth) RT_IFRT(rt, rth)

#define	RT_TABLEEND	RT_IFRT_END RT_BUCKET_END(rth) RT_SCTBL_END }

/*
 *	Macro to scan through a list of protocol specific data blocks
 */
#define	RTDATA_LIST(rtd, head) for (rtd = (head)->rtd_forw; rtd != (head); rtd = rtd->rtd_forw)

#define	RTDATA_LIST_END(rtd, head) if (rtd == (head)) rtd = (rt_data *) 0

 /*  Macro implementation of rt_refresh.  If this is not defined, a	*/
 /*  function will be used.						*/
#define	rt_refresh(rt)	rt->rt_state |= RTS_REFRESH

 /* Macro implementation of obsolete rt_lookup.		*/
#define	rt_lookup(state, dst)	rt_locate(state, dst, (flag_t) RTPROTO_ANY)

extern rt_head *rt_inet_hash[];		/* ip routing table */

extern bits rt_flag_bits[];		/* Route flag bits */
extern bits rt_state_bits[];		/* Route state bits */
extern bits rt_proto_bits[];		/* Protocol types */
extern bits rt_asorigin_bits[];		/* AS origins */
extern u_int rt_net_routes;		/* # networks in routing tables */
extern u_int rt_host_routes;		/* # hosts in routing tables */
extern int rt_default_active;		/* TRUE if gateway default is active */
extern int rt_default_needed;		/* TRUE if gateway default is needed */
extern u_long rt_revision;		/* Current revision of the routing table */
extern struct _task *rt_task;
extern struct _timer *rt_timer;
extern gw_entry *rt_gw_list;		/* List of gateways for static routes */

extern int ignore_redirects;		/* TRUE if ICMP redirects should be ignored */
extern int redirect_n_trusted;		/* Number of trusted ICMP gateways */
extern pref_t redirect_preference;	/* Preference for ICMP redirects */
extern gw_entry *redirect_gw_list;	/* List of learned and defined ICMP gateways */
extern adv_entry *redirect_accept_list;	/* List of routes that we can accept */
extern adv_entry **redirect_int_accept;	/* List of accept lists per interface */


/* Delete pseudo default at termination time */
#define	rt_default_reset()	rt_default_active = 1; (void) rt_default_delete()

#ifdef	USE_PROTOTYPES
extern void rt_init(void);		/* Initialize routing table */
extern void rt_open(task * tp);		/* Open routing table for updating */
extern int
rt_close(task * tp,
	 gw_entry * gwp,
	 int changes);			/* Signal completion of updates */
extern rt_entry *
rt_add(sockaddr_un * dst,
       sockaddr_un * mask,
       sockaddr_un * gate,
       gw_entry * sourcegw,
       int metric,
       flag_t state,
       proto_t proto,
       as_t as,
       time_t timer_max,
       pref_t preference);		/* Add a route to the routing table */
extern int
rt_change(rt_entry * rt,
	  sockaddr_un * gate,
	  metric_t metric,
	  time_t timer_max,
	  pref_t preference);		/* Change an entry in the routing table */
extern int rt_unreach(rt_entry * rt);	/* Make a route unreachable */
extern int rt_delete(rt_entry * rt);	/* Delete a route from the routing table */
extern int rt_gwunreach(task * tp, gw_entry * gwp);	/* Process an unreachable gateway */
extern void
rt_redirect(task * tp,
	    sockaddr_un * dest,
	    sockaddr_un * mask,
	    sockaddr_un * gate,
	    int host_redirect);		/* Process a redirect */

extern rt_data *rtd_alloc(int length);	/* Allocate protocol specific data block */
extern rt_data *rtd_insert(rt_data * rtd, rt_data * head);	/* Same as locate given an rtd */
extern rt_data *rtd_locate(caddr_t data, int length, rt_data * head);	/* Locate or allocate a data block */
extern void rtd_unlink(rt_data * rtd);	/* Dereference a block */

#if	defined(AGENT_SNMP)
extern rt_entry *rt_next(sockaddr_un * dst);	/* Locate next route in lexigraphic order */

#endif				/* defined(AGENT_SNMP) */
extern rt_entry *
rt_locate(flag_t state,
	  sockaddr_un * dst,
	  proto_t proto);		/* Locate a route given dst, table and proto */
extern rt_entry *
rt_locate_gw(flag_t state,
	     sockaddr_un * dst,
	     proto_t proto,
	     gw_entry * gwp);		/* Locate a route given dst, table, proto and gwp */

extern int rt_default_add();		/* Request installation of gateway default */
extern int rt_default_delete();		/* Request deletion of gateway default */
#else				/* USE_PROTOTYPES */
extern void rt_init();			/* Initialize routing table */
extern void rt_open();			/* Open routing table for updating */
extern int rt_close();			/* Signal completion of updates */
extern rt_entry *rt_add();		/* Add a route to the routing table */
extern int rt_change();			/* Change an entry in the routing table */
extern int rt_unreach();		/* Make a route unreachable */
extern int rt_delete();			/* Delete a route from the routing table */
extern int rt_gwunreach();		/* Process an unreachable gateway */
extern void rt_redirect();		/* Process a redirect */

extern rt_data *rtd_alloc();		/* Allocate protocol specific data block */
extern rt_data *rtd_locate();		/* Locate or insert a data block */
extern rt_data *rtd_insert();		/* Locate or allocate a data block */
extern void rtd_unlink();		/* Dereference a block */

#if	defined(AGENT_SNMP)
extern rt_entry *rt_next();		/* Locate next route in lexigraphic order */

#endif				/* defined(AGENT_SNMP) */
#ifdef	notdef
extern rt_entry *rt_find();		/* ?? */

#endif				/* notdef */
extern rt_entry *rt_locate();		/* Locate a route given dst, table and proto */
extern rt_entry *rt_locate_gw();	/* Locate a route given dst, table, proto and gwp */

extern int rt_default_add();		/* Request installation of gateway default */
extern int rt_default_delete();		/* Request deletion of gateway default */

#endif				/* USE_PROTOTYPES */
