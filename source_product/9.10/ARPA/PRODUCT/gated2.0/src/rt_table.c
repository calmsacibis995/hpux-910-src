/*
 *  $Header: rt_table.c,v 1.1.109.6 92/03/04 08:36:02 ash Exp $
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


#include "include.h"
#include "routed.h"

bits rt_flag_bits[] =
{
    {RTF_UP, "UP"},
    {RTF_GATEWAY, "GW"},
    {RTF_HOST, "HOST"},
#ifdef	RTF_DYNAMIC
    {RTF_DYNAMIC, "DYN"},
#endif				/* RTF_DYNAMIC */
#ifdef	RTF_MODIFIED
    {RTF_MODIFIED, "MOD"},
#endif				/* RTF_MODIFIED */
#ifdef	RTF_DONE
    {RTF_DONE, "DONE"},
#endif				/* RTF_DONE */
#ifdef	RTF_MASK
    {RTF_MASK, "MASK"},
#endif				/* RTF_MASK */
#ifdef	RTF_CLONING
    {RTF_CLONING, "CLONING"},
#endif				/* RTF_CLONING */
#ifdef	RTF_XRESOLVE
    {RTF_XRESOLVE, "XRESOLVE"},
#endif				/* RTF_XRESOLVE */
    {0}
};

bits rt_state_bits[] =
{
    {RTS_NOAGE, "NoAge"},
    {RTS_REMOTE, "Remote"},
    {RTS_CHANGED, "Changed"},
    {RTS_NOTINSTALL, "NotInstall"},
    {RTS_NOADVISE, "NoAdvise"},
    {RTS_SUBNET, "Subnet"},
    {RTS_POINTOPOINT, "P2P"},
    {RTS_HOSTROUTE, "Host"},
    {RTS_INTERIOR, "Int"},
    {RTS_EXTERIOR, "Ext"},
    {RTS_HOLDDOWN, "HoldDown"},
    {RTS_DELETE, "Delete"},
    {RTS_REFRESH, "Refresh"},
    {0}
};

bits rt_proto_bits[] =
{
    {RTPROTO_DIRECT, "Direct"},
    {RTPROTO_KERNEL, "Kernel"},
    {RTPROTO_REDIRECT, "Redirect"},
    {RTPROTO_DEFAULT, "Default"},
    {RTPROTO_IGP, "IGP"},
    {RTPROTO_OSPF, "OSPF"},
    {RTPROTO_IGRP, "IGRP"},
    {RTPROTO_HELLO, "HELLO"},
    {RTPROTO_RIP, "RIP"},
    {RTPROTO_BGP, "BGP"},
    {RTPROTO_EGP, "EGP"},
    {RTPROTO_STATIC, "Static"},
    {RTPROTO_SNMP, "SNMP"},
    {RTPROTO_KRT, "KRT"},
    {0}
};


rt_head *rt_inet_hash[ROUTEHASHSIZ + 1];/* Network route table */

task *rt_task = (task *) 0;
timer *rt_timer = (timer *) 0;
u_int rt_net_routes = 0;		/* # nets in routing table */
u_int rt_host_routes = 0;		/* # hosts in routing table */
int rt_default_active = 0;		/* Number of requests to install default */
int rt_default_needed = FALSE;		/* TRUE if we need to generate a default */
static rt_entry *rt_default_rt;		/* Pointer to internal default if installed */
u_long rt_revision = 0;			/* Revision level of routing table */
gw_entry *rt_gw_list;			/* List of gateways for static routes */

#if	defined(AGENT_SNMP)
static int rt_table_changed = TRUE;	/* Routing table has been changed */

#endif				/* defined(AGENT_SNMP) */

static int rt_changes = 0;		/* Number of changes to routing table */
static task *rt_opentask = (task *) 0;	/* Protocol that has table open */

#define	rt_check_open(rt, name)	if (!rt_opentask) { trace(TR_ALL, LOG_ERR, "%s: table not open - proto %s", \
							  name, trace_bits(rt_proto_bits, rt->rt_proto)); \
							      quit(EBADF); }

/*
 * rt_trace() traces changes to the routing tables
 */
 /* static*/ void
rt_trace(action, rt)
char *action;
rt_entry *rt;
{

    tracef("%-8s %-15A gw %-15A  %-8s  pref %3d  metric %d",
	   action,
	   &rt->rt_dest,
	   &rt->rt_router,
	   trace_bits(rt_proto_bits, rt->rt_proto),
	   rt->rt_preference,
	   rt->rt_metric);
    tracef("  <%s>", trace_bits(rt_state_bits, rt->rt_state));
    if (rt->rt_as) {
	tracef("  as %d", rt->rt_as);
    }
    /* XXX - Format protocol specific information? */
    trace(TR_RT | TR_NOSTAMP, 0, NULL);

}


/**/

/*
 *	Locate the rt_head pointer for this destination.  Create one if it does not exist.
 */
static rt_head *
rth_locate(dst, mask, state)
sockaddr_un *dst;
sockaddr_un *mask;
flag_t *state;
{
    rt_head *rth = (rt_head *) 0;
    rt_head *xrth;

    RT_HASH(dst);

    RT_BUCKET(rth) {
	if ((rth->rth_hash == hash) && (equal(&rth->rth_dest, dst))) {
	    break;
	}
    } RT_BUCKET_END(rth);

    if (!rth) {
	rth = (rt_head *) calloc(1, sizeof(*rth));
	if (!rth) {
	    trace(TR_ALL, LOG_ERR, "rth_locate: calloc: %m");
	    quit(errno);
	}
	/* Copy destination */
	sockcopy(dst, &rth->rth_dest);

	/* Set the mask */
	if (mask) {
	    sockcopy(mask, &rth->rth_dest_mask);
	} else {
	    sockcopy(dst, &rth->rth_dest_mask);

	    if (rth->rth_dest.in.sin_addr.s_addr == htonl(DEFAULTNET)) {
		rth->rth_dest_mask.in.sin_addr.s_addr = htonl(INADDR_ANY);
	    } else if (*state & RTS_HOSTROUTE) {
		rth->rth_state |= RTS_HOSTROUTE;
		rth->rth_dest_mask.in.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	    } else {
		u_long subnet_mask, natural_mask;

		if (gd_inet_lnaof(rth->rth_dest.in.sin_addr)) {
		    /* Host bits must be zero for a network route */
		    rth->rth_dest.in.sin_addr =
		    gd_inet_makeaddr(gd_inet_netof(rth->rth_dest.in.sin_addr), 0, TRUE);
		}
		natural_mask = htonl(gd_inet_netmask(ntohl(rth->rth_dest.in.sin_addr.s_addr)));
		subnet_mask = htonl(if_subnetmask(rth->rth_dest.in.sin_addr));

		if (subnet_mask &&
		    ((rth->rth_dest.in.sin_addr.s_addr & natural_mask) !=rth->rth_dest.in.sin_addr.s_addr)) {
		    /* This is a subnet - set subnet mask */
		    *state |= RTS_SUBNET;
		    rth->rth_state |= RTS_SUBNET;
		    rth->rth_dest_mask.in.sin_addr.s_addr = subnet_mask;
		} else {
		    /* This is not a subnet - set the natural mask */
		    rth->rth_dest_mask.in.sin_addr.s_addr = natural_mask;
		}
	    }
	}

	rth->rth_hash = hash;
	rth->rt_forw = rth->rt_back = (rt_entry *) & rth->rt_forw;
	rth->rt_head = rth;

	/* Insert this rt_head structure at end of doubly linked list */
	if (*rtb) {
	    insque((struct qelem *) rth, (struct qelem *) (*rtb)->rth_back);
	} else {
	    *rtb = rth;
	    rth->rth_forw = rth->rth_back = rth;
	}
    }
    return (rth);
}


/**/
/*
 *	Remove an rt_entry structure from the doubly linked list
 *	pointed to by it's rt_head
 */

static void
rt_remove(rt)
rt_entry *rt;
{
    if (!--rt->rt_head->rth_entries) {
	if (rt->rt_state & RTS_HOSTROUTE) {
	    rt_host_routes--;
	} else {
	    rt_net_routes--;
	}
    }
    remque((struct qelem *) rt);
}


/*	Insert an rt_entry structure in preference order in the doubly linked	*/
/*	list pointed to by it's rt_head.  If two routes with identical		*/
/*	preference are found, the one witht he shorter as path length is used.	*/
/*	If the as path lengths are the same, the route with the lower next-hop	*/
/*	IP address is prefered. This insures that the selection of the prefered	*/
/*	route is deterministic.							*/
static void
rt_insert(rt)
rt_entry *rt;
{
    rt_entry *rt1;
    rt_head *rth = rt->rt_head;

    RT_ALLRT(rt1, rth) {
	if ((rt->rt_state & RTS_DELETE) && !(rt1->rt_state & RTS_DELETE)) {
	    /* Deleted routes go behind non-deleted routes */
	    continue;
	}
	if ((rt1->rt_state & RTS_DELETE) && !(rt->rt_state & RTS_DELETE)) {
	    /* non-deleted routes preceed deleted routes */
	    break;
	}
	if (rt->rt_preference < rt1->rt_preference) {
	    /* This preference is better */
	    break;
	} else if (rt->rt_preference == rt1->rt_preference) {
	    /* Same preference */

	    if (rt->rt_proto == rt1->rt_proto &&
		rt->rt_as == rt1->rt_as) {
		/* Same protocol and AS */

		if (rt->rt_metric < rt1->rt_metric) {
		    /* Use lower metric */
		    break;
		}
	    }

	    /* lastly try router address */
	    if (rt->rt_router.in.sin_addr.s_addr < rt1->rt_router.in.sin_addr.s_addr) {
		/* This router address is lower, use it */
		break;
	    }
	}
    } RT_ALLRT_END(rt1, rth);

    /* Insert prior to element if found, or behind the element at the end of a list. */
    /* For an empty list this ends up being behind the first element. */
    insque((struct qelem *) rt, (struct qelem *) (rt1 ? rt1->rt_back : rth->rt_back));

    if (!rth->rth_entries++) {
	if (rt->rt_state & RTS_HOSTROUTE) {
	    rt_host_routes++;
	} else {
	    rt_net_routes++;
	}
    }
}


/**/
/*
 *	rt_alloc - allocate an rt_entry
 */

static rt_entry *
rt_alloc(dst, mask, state)
sockaddr_un *dst;
sockaddr_un *mask;
flag_t *state;
{
    rt_entry *rt;

    /* No free list entries, allocate a new one */
    rt = (rt_entry *) calloc(1, sizeof(*rt));
    if (!rt) {
	trace(TR_ALL, LOG_ERR, "rt_alloc: calloc: %m");
    }
    /* Set pointer to head */
    rt->rt_head = rth_locate(dst, mask, state);
    return (rt);
}


/*
 * Delete a route from the routing table.
 */
static void
rt_release(rt)
rt_entry *rt;
{

    rt_check_open(rt, "rt_release");

    if (!(rt->rt_state & RTS_DELETE)) {
	/* If this route is active the kernel's routing table needs to be	*/
	/* updated.  If this is the only route for this destination we only	*/
	/* need to delete it from the kernel.  If there is another route	*/
	/* then we need to change to this new route.			*/
	if (rt->rt_active == rt) {
	    rt_entry *rt1;
	    rt_head *rth = rt->rt_head;

	    RT_ALLRT(rt1, rth) {
		if ((rt1 != rt) && !(rt1->rt_state & RTS_HOLDDOWN)) {
		    break;
		}
	    } RT_ALLRT_END(rt1, rth);

	    krt_change(rt, rt1);	/* krt_change does a change or a delete */
	    rt->rt_active = rt1;
	}
    }
    TRACE_ACTION((rt->rt_state & RTS_DELETE) ? "RELEASE" : "DELETE", rt);

    rt_changes++;

    if (rt->rt_data) {
	rtd_unlink(rt->rt_data);
    }
    rt_remove(rt);

    (void) free((caddr_t) rt);
#if	defined(AGENT_SNMP)
    rt_table_changed = TRUE;
#endif				/* defined(AGENT_SNMP) */
}

/**/
/*
 *	rt_open: Make table available for updating
 */
void
rt_open(tp)
task *tp;
{
    if (rt_opentask) {
	tracef("rt_open: open attempt by %s",
	       task_name(tp));
	trace(TR_ALL, LOG_ERR, " already open by %s",
	      task_name(rt_opentask));
	quit(EPERM);
    }
    rt_opentask = tp;
    rt_revision++;
    rt_changes = 0;
}


/*
 *	rt_close: Clean up after table updates
 */
int
rt_close(tp, gwp, changes)
task *tp;
gw_entry *gwp;
int changes;
{
    int rtchanges = rt_changes;

    if (!rt_opentask) {
	trace(TR_ALL, LOG_ERR, "rt_close: close attempt by %s when table not open",
	      task_name(tp));
	quit(EBADF);
    }
    if (rt_opentask != tp) {
	tracef("rt_close: close attempt by %s",
	       task_name(tp));
	trace(TR_ALL, LOG_ERR, " when opened by %s",
	      task_name(rt_opentask));
	quit(EBADF);
    }
    rt_opentask = (task *) 0;
    if (rt_changes) {
	tracef("rt_close: %d",
	       rt_changes);
	if (changes) {
	    tracef("/%d", changes);
	}
	tracef(" route%s proto %s",
	       rt_changes > 1 ? "s" : "",
	       task_name(tp));
	if (gwp) {
	    tracef(" from %A",
		   &gwp->gw_addr);
	}
	trace(TR_RT, 0, " table revision %ld",
	      rt_revision);
	trace(TR_RT, 0, NULL);
	rt_changes = 0;
    } else {
	rt_revision--;
    }

    return (rtchanges);
}

/**/
 /*  Add a route to the routing table after some checking.  The route	*/
 /*  is added in preference order.  If the active route changes, the	*/
 /*  kernel routing table is updated.					*/
rt_entry *
#ifdef	USE_PROTOTYPES
rt_add(sockaddr_un * dst,
       sockaddr_un * mask,
       sockaddr_un * gate,
       gw_entry * sourcegw,
       metric_t metric,
       flag_t state,
       proto_t proto,
       as_t as,
       time_t timer_max,
       pref_t preference)
#else				/* USE_PROTOTYPES */
rt_add(dst, mask, gate, sourcegw, metric, state, proto, as, timer_max, preference)
sockaddr_un *dst, *mask, *gate;
gw_entry *sourcegw;
metric_t metric;
flag_t state;
proto_t proto;
as_t as;
time_t timer_max;
pref_t preference;

#endif				/* USE_PROTOTYPES */
{
    rt_entry *rt = (rt_entry *) 0;

    rt_check_open(rt, "rt_add");

    rt = rt_alloc(dst, mask, &state);
    if (!rt) {
	return (rt);
    }
    if (rt->rt_head->rth_entries) {
	rt_entry *rt1;

	RT_ALLRT(rt1, rt->rt_head) {
	    if ((rt1->rt_proto & proto) && (rt1->rt_sourcegw == sourcegw) && (rt1->rt_state & RTS_DELETE)) {
		rt1 = rt1->rt_back;
		rt_release(rt1->rt_forw);
	    }
	} RT_ALLRT_END(rt1, rt->rt_head);
    }
    /* XXX - need to support multiple next-hops */
    rt->rt_router = *gate;
    rt->rt_sourcegw = sourcegw;
    rt->rt_metric = metric;
    rt->rt_timer = 0;
    rt->rt_timer_max = timer_max ? timer_max : RT_T_EXPIRE;
    rt->rt_flags = 0;
    rt->rt_preference = preference;
#ifdef	RTM_ADD
    rt->rt_mtu = 0;			/* Need code to figure out a good mtu by the time we port to BSD 4.4 */
#endif				/* RTM_ADD */
    rt->rt_state = state | RTS_CHANGED | rt->rt_head->rth_state;

    if ((rt->rt_state & rt->rt_head->rth_state) != rt->rt_head->rth_state) {
	/* XXX - this route does not match */
    }
    /* Set RTF_HOST flag if appropriate */
    if (rt->rt_state & RTS_HOSTROUTE) {
	rt->rt_flags |= RTF_HOST;
    }
    rt->rt_proto = proto;
    rt->rt_as = as;
    rt->rt_ifp = if_withdst(&rt->rt_router);
    if (rt->rt_ifp == (if_entry *) NULL) {
	trace(TR_ALL, LOG_WARNING, "rt_add: interface not found for net %-15A gateway %A",
	      &rt->rt_dest,
	      &rt->rt_router);
	(void) free((caddr_t) rt);
	return ((rt_entry *) 0);
    }
#ifdef		notdef
    if (rt->rt_ifp->int_state & IFS_LOOPBACK && rt->rt_proto != RTPROTO_DEFAULT) {
	rt->rt_state |= RTS_NOADVISE;
    }
#endif		/* notdef */
#ifdef	RTF_DYNAMIC
    if (rt->rt_proto == RTPROTO_REDIRECT) {
	rt->rt_flags |= RTF_DYNAMIC;
    }
#endif				/* RTF_DYNAMIC */

    /* If this is a martian net it doesn't get into MY tables */
    /* The exception is the loopback host address on the loopback */
    /* interface */
    if (is_martian(&rt->rt_dest)) {
	struct in_addr addr;

	addr.s_addr = htonl(INADDR_LOOPBACK);
	if (!((rt->rt_ifp->int_state & IFS_LOOPBACK) &&
	      equal_in(rt->rt_dest.in.sin_addr, addr))) {
	    tracef("rt_add: ignoring martian network %A",
		   &rt->rt_dest);
	    if (rt->rt_sourcegw) {
		tracef(" from gateway %A",
		       &rt->rt_sourcegw->gw_addr);
	    }
	    tracef(" protocol %s AS %d",
		   trace_bits(rt_proto_bits, rt->rt_proto),
		   rt->rt_as);
	    trace(TR_INT, LOG_WARNING, NULL);
	    (void) free((caddr_t) rt);
	    return ((rt_entry *) 0);
	}
    }
    if (rt->rt_state & RTS_EXTERIOR) {
	rt->rt_flags |= RTF_GATEWAY;
    } else {
	switch (rt->rt_proto) {
	    case RTPROTO_KERNEL:
		if (rt->rt_flags & RTF_HOST) {
		    if (!if_withaddr(&rt->rt_dest)) {
			rt->rt_flags |= RTF_GATEWAY;
		    }
		} else {
		    if (!if_withdst(&rt->rt_dest)) {
			rt->rt_flags |= RTF_GATEWAY;
		    }
		}
		break;
	    case RTPROTO_STATIC:
		if (!if_withaddr(&rt->rt_dest)) {
		    rt->rt_flags |= RTF_GATEWAY;
		}
		break;
	    case RTPROTO_DIRECT:
		break;
	    default:
		rt->rt_flags |= RTF_GATEWAY;
		break;
	}
    }
    rt_changes++;
    rt->rt_revision = rt_revision;
    TRACE_ACTION("ADD", rt);

    /* Insert this route into the table */
    rt_insert(rt);

    /* If the new route is the first in the chain and there is no active	*/
    /* route, or the active route is not in Holddown, this route will	*/
    /* become the active route and the kernel should be updated. */
    if ((rt == rt->rt_head->rt_forw) && !(rt->rt_active && (rt->rt_active->rt_state & RTS_HOLDDOWN))) {
	krt_change(rt->rt_active, rt);	/* krt_change does an add or a change */
	rt->rt_active = rt;		/* This route is now the active route */
    }
#if	defined(AGENT_SNMP)
    rt_table_changed = TRUE;
#endif				/* defined(AGENT_SNMP) */

    return (rt);
}


 /* rt_change() changes a route &/or notes that an update was received.	*/
 /* returns 1 if change made.  Updates the kernel's routing table if	*/
 /* the router has changed, or a preference change has made another	*/
 /* route active							*/
int
#ifdef	USE_PROTOTYPES
rt_change(rt_entry * rt,
	  sockaddr_un * gate,
	  metric_t metric,
	  time_t timer_max,
	  pref_t preference)
#else				/* USE_PROTOTYPES */
rt_change(rt, gate, metric, timer_max, preference)
rt_entry *rt;
sockaddr_un *gate;
metric_t metric;
time_t timer_max;
pref_t preference;

#endif				/* USE_PROTOTYPES */
{
    rt_entry orig_rt;
    int krt_changed = FALSE;		/* Kernel needs to be changed */
    int pref_changed = TRUE;		/* Active route may need to be updated */
    int changed = FALSE;
    if_entry *t_ifp;

    orig_rt = *rt;			/* Save a copy of the original route */

    rt_check_open(rt, "rt_change");

    if (rt->rt_ifp != (t_ifp = if_withdst(gate))) {
	if (rt->rt_ifp == (if_entry *) NULL) {
	    trace(TR_ALL, LOG_WARNING, "rt_change: interface not found for net %-15A gateway %A",
		  &rt->rt_dest,
		  &rt->rt_router);
	    return (FALSE);
	}
	rt->rt_ifp = t_ifp;
	changed = TRUE;
    }
    rt->rt_timer_max = timer_max ? timer_max : RT_T_EXPIRE;
    rt->rt_state |= RTS_CHANGED;	/* ensures route age reset */
    rt->rt_state &= ~RTS_HOLDDOWN;	/* If route changed, reset hold down */

    /* XXX - need to support multiple next hops */
    if (!equal(&rt->rt_router, gate)) {
	krt_changed = TRUE;
	pref_changed = TRUE;
	changed = TRUE;
	rt->rt_router = *gate;
    }
    if (metric != rt->rt_metric) {
	changed = TRUE;
	rt->rt_metric = metric;
    }
    if (preference != rt->rt_preference) {
	changed = TRUE;
	pref_changed = TRUE;
	rt->rt_preference = preference;
    }
    if (pref_changed) {
	/* Put this route in order in the queue by deleting it and	*/
	/* re-inserting it						*/
	rt_remove(rt);
	rt_insert(rt);
	krt_changed = TRUE;
    }
    if (krt_changed) {
	rt_entry *new_rt, *old_rt;

	new_rt = old_rt = rt->rt_active;/* Default is not to change active route */

	if (rt->rt_head->rt_forw == rt) {
	    /* This route is eligble to become the active route */
	    if (rt->rt_active == rt) {
		/* We are still the active route, but the gateway or mtu may have changed */
		old_rt = &orig_rt;
		new_rt = rt;
	    } else if (rt->rt_active) {
		/* Another route is active */
		if (!rt->rt_active->rt_state & RTS_HOLDDOWN) {
		    /* Other route is not in holddown, OK to switch */
		    new_rt = rt;
		}
	    } else {
		/* No route active, this is the new active route */
		new_rt = rt;
	    }
	} else if (rt->rt_active == rt) {
	    /* We were active, now find a new one */
	    rt_head *rth = rt->rt_head;

	    RT_ALLRT(new_rt, rth) {
		if ((new_rt != rt) && !(new_rt->rt_state & RTS_HOLDDOWN)) {
		    break;
		}
	    } RT_ALLRT_END(new_rt, rth);
	}
	(void) krt_change(old_rt, new_rt);	/* krt_change figures out what needs changing */

	rt->rt_active = new_rt;		/* Set the (maybe) new active route */
    }
    if (changed) {
	rt_changes++;
	rt->rt_revision = rt_revision;
	TRACE_ACTION("CHANGE", rt);
#if	defined(AGENT_SNMP)
	rt_table_changed = TRUE;
#endif				/* defined(AGENT_SNMP) */
    }
    return (TRUE);
}


#ifndef	rt_refresh
/*
 * rt_refresh() flags the route as being refreshed so the route timer will be reset the next time rt_time is run.
 */
rt_refresh(rt)
rt_entry *rt;
{

    rt_check_open(rt, "rt_refresh");

    rt->rt_state |= RTS_REFRESH;
}

#endif				/* rt_refresh */


/*
 *
 * rt_unreach() does processing on a route that has been
 * indicated as unreachable in a routing update.  The metric
 * is set to infinity and the timer is set so the route will expire
 * within RT_HOLDDOWN seconds.
 */
int
rt_unreach(rt)
rt_entry *rt;
{

    rt_check_open(rt, "rt_unreach");

    if (!(rt->rt_state & RTS_HOLDDOWN)) {
	rt->rt_state |= RTS_HOLDDOWN;
	rt->rt_timer = 0;
	rt->rt_timer_max = RT_T_HOLDDOWN;
	rt_changes++;
	rt->rt_revision = rt_revision;
	TRACE_ACTION("CHANGE", rt);
#if	defined(AGENT_SNMP)
	rt_table_changed = TRUE;
#endif				/* defined(AGENT_SNMP) */
	return (1);
    } else {
	return (0);
    }
}


/*
 * rt_delete() does processing on a route that has been indicated as
 * deleted in a routing update.  The timer is set so the route will expire
 * within RT_DELETE seconds.
 */
int
rt_delete(rt)
rt_entry *rt;
{

    rt_check_open(rt, "rt_delete");

    if (!(rt->rt_state & RTS_DELETE)) {
	rt->rt_state |= RTS_DELETE;
	rt->rt_timer = 0;
	rt->rt_timer_max = RT_T_DELETE;
	rt_changes++;
	rt->rt_revision = rt_revision;
	if (rt->rt_active == rt) {
	    rt_entry *rt1;
	    rt_head *rth = rt->rt_head;

	    rt_remove(rt);
	    rt_insert(rt);

	    RT_ALLRT(rt1, rth) {
		if ((rt1 != rt) && !(rt1->rt_state & RTS_HOLDDOWN)) {
		    break;
		}
	    } RT_ALLRT_END(rt1, rth);

	    krt_change(rt, rt1);
	    rt->rt_active = rt1;
	}
	rt->rt_state |= RTS_NOTINSTALL;
	TRACE_ACTION("DELETE", rt);
#if	defined(AGENT_SNMP)
	rt_table_changed = TRUE;
#endif				/* defined(AGENT_SNMP) */
	return (1);
    } else {
	return (0);
    }
}


/**/
/*
 *	Routines to handle route specific data
 */
rt_data *
rtd_alloc(length)
int length;
{
    rt_data *rtd;

    rtd = (rt_data *) calloc(1, sizeof(*rtd) + length);
    if (!rtd) {
	trace(TR_ALL, LOG_ERR, "rtd_alloc: calloc: %m");
	quit(errno);
    }
    rtd->rtd_data = (caddr_t) rtd + sizeof(*rtd);
    rtd->rtd_length = length;

    return (rtd);
}


rt_data *
rtd_locate(data, length, head)
caddr_t data;
int length;
rt_data *head;
{
    rt_data *rtd;

    RTDATA_LIST(rtd, head) {
	if ((rtd->rtd_length == length) &&
	    !memcmp(rtd->rtd_data, data, length)) {
	    break;
	}
    } RTDATA_LIST_END(rtd, head);

    if (!rtd) {
	rtd = rtd_alloc(length);

	memcpy(rtd->rtd_data, data, length);
	insque((struct qelem *) rtd, (struct qelem *) head);
    }
    rtd->rtd_refcount++;

    return (rtd);
}


rt_data *
rtd_insert(rtd, head)
rt_data *rtd;
rt_data *head;
{
    rt_data *rtd1;

    RTDATA_LIST(rtd1, head) {
	if ((rtd->rtd_length == rtd1->rtd_length) &&
	 !memcmp(rtd->rtd_data, rtd1->rtd_data, (int) rtd->rtd_length)) {
	    break;
	}
    } RTDATA_LIST_END(rtd1, head);

    if (rtd1) {
	(void) free((caddr_t) rtd);
	rtd = rtd1;
    } else {
	insque((struct qelem *) rtd, (struct qelem *) head);
    }

    rtd->rtd_refcount++;

    return (rtd);
}


void
rtd_unlink(rtd)
rt_data *rtd;
{
    if (!--rtd->rtd_refcount) {
	remque((struct qelem *) rtd);
	(void) free((caddr_t) rtd);
    }
}


/**/

/*
 * Handle the internally generated default route.
 */

/*
 * This is only a pseudo route so use the loopback interface,
 * it should not go down and produce
 * undesirable side effects.
 */  
#define	RT_DEFAULT_ADD	rt_default_rt = rt_add((sockaddr_un *) &default_net, \
					       (sockaddr_un *) 0, \
					       &loopback, \
					       (gw_entry *) 0, \
					       0, \
					       RTS_INTERIOR | RTS_NOAGE | RTS_NOTINSTALL, \
					       RTPROTO_DEFAULT, \
					       (as_t) 0, \
					       (time_t) 0, \
					       RTPREF_DEFAULT)

#define	RT_DEFAULT_DELETE	(void) rt_delete(rt_default_rt); \
    				rt_default_rt = (rt_entry *) 0

static void
rt_default_reinit()
{
    if (rt_default_needed) {
	/* Default is enabled */

	if (rt_default_active && !rt_default_rt) {
	    /* Should be installed, but is not */
	    sockaddr_un loopback;

	    sockclear_in(&loopback.in);
	    loopback.in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	    RT_DEFAULT_ADD;
	}
    } else {
	/* Default is disabled */

	if (rt_default_rt) {
	    /* Get rid of current default */

	    RT_DEFAULT_DELETE;
	}
    }
}


int
rt_default_add()
{
    if (!rt_default_active++ && rt_default_needed) {
	/* First request to add and default is enabled, add it */
	sockaddr_un loopback;

	sockclear_in(&loopback.in);
	loopback.in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	RT_DEFAULT_ADD;

	return 1;
    }

    return 0;
}


int
rt_default_delete()
{

    if (--rt_default_active && rt_default_rt) {
	/* Last request to delete and default is installed, remove it */

	rt_open(rt_task);
	RT_DEFAULT_DELETE;
	rt_close(rt_task, (gw_entry *) 0, 0);

	return 1;
    }

    return 0;
}


static void
rt_default_cleanup()
{
    rt_default_needed = FALSE;
}


#undef	RT_DEFAULT_ADD
#undef	RT_DEFAULT_DELETE

/**/
/*
 * rt_time() increments the age of all routes in the routing table
 */

/*ARGSUSED*/
static void
rt_time(tip, interval)
timer *tip;
time_t interval;
{
    int old_routes = 0, hold_routes = 0;
    rt_entry *rt = (rt_entry *) 0;

    rt_open(rt_task);
    RT_WHOLE(rt) {
	if (rt->rt_state & RTS_REFRESH) {	/* Route was refreshed */
	    rt->rt_state &= ~RTS_REFRESH;
	    rt->rt_timer = 0;
	}
	if (rt->rt_state & RTS_CHANGED) {	/* recently updated */
	    rt->rt_state &= ~RTS_CHANGED;
	    rt->rt_timer = 0;
	} else if (!(rt->rt_state & RTS_NOAGE) || (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE))) {
	    rt->rt_timer += interval;
	}
	/*
         *  is route too old?
         */
	if (rt->rt_timer >= rt->rt_timer_max) {
	    if (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE)) {
		if (rt->rt_proto == RTPROTO_DIRECT) {
		    trace(TR_INT, LOG_ERR, "interface timeout - deleting route to %A",
			  &rt->rt_dest);
		}
		old_routes++;
		rt = rt->rt_back;
		rt_release(rt->rt_forw);
	    } else {
		hold_routes += rt_unreach(rt);
	    }
	}
    } RT_WHOLEEND(rt);
    (void) rt_close(rt_task, (gw_entry *) 0, old_routes + hold_routes);
    if (old_routes + hold_routes) {
	trace(TR_RT, 0, "rt_time: above %d routes deleted and %d routes helddown", old_routes, hold_routes);
	if (hold_routes) {
	    /* Only do flashing for helddown routes, deleted routes are already history */
	    task_flash(rt_task);
	}
    }
    return;
}


/*
 * rt_gwunreach() deletes all exterior routes from the routing table for a
 * specified gateway
 */
int
rt_gwunreach(tp, gwp)
task *tp;
gw_entry *gwp;
{
    int changes = 0;
    rt_entry *rt;

    rt_open(tp);

    RT_TABLE(rt) {
	if (rt->rt_sourcegw == gwp) {
	    changes += rt_delete(rt);
	}
    } RT_TABLEEND;

    (void) rt_close(tp, gwp, changes);

    return (changes);
}


/*
 * Looks up a destination network route with a specific protocol mask.
 * Specifying a protocol of zero will match all protocols.
 */

rt_entry *
rt_locate(state, dst, proto)
flag_t state;
sockaddr_un *dst;
proto_t proto;
{
    rt_entry *rt;
    rt_head *rth = (rt_head *) 0;

    RT_HASH(dst);

    RT_BUCKET(rth) {
	if ((rth->rth_hash == hash) && equal(&rth->rth_dest, dst)) {
	    RT_ALLRT(rt, rth) {
		if (!(rt->rt_state & RTS_DELETE) && (rt->rt_state & state) && (rt->rt_proto & proto)) {
		    return (rt);
		}
	    } RT_ALLRT_END(rt, rth);
	}
    } RT_BUCKET_END(rth);

    return ((rt_entry *) 0);
}


/* Look up a route with a destination address, protocol and source gateway */

rt_entry *
rt_locate_gw(state, dst, proto, gwp)
flag_t state;
sockaddr_un *dst;
proto_t proto;
gw_entry *gwp;
{
    rt_entry *rt;
    rt_head *rth = (rt_head *) 0;

    RT_HASH(dst);

    RT_BUCKET(rth) {
	if ((rth->rth_hash == hash) && equal(&rth->rth_dest, dst)) {
	    RT_ALLRT(rt, rth) {
		if (!(rt->rt_state & RTS_DELETE) && (rt->rt_state & state) && (rt->rt_proto & proto) && (rt->rt_sourcegw == gwp)) {
		    return (rt);
		}
	    } RT_ALLRT_END(rt, rth);
	}
    } RT_BUCKET_END(rth);

    return ((rt_entry *) 0);
}


/*
 * rt_redirect() changes the routing tables in response to a redirect
 * message or indication from the kernel
 */

int ignore_redirects = FALSE;
int redirect_n_trusted = 0;		/* Number of trusted ICMP gateways */
pref_t redirect_preference = RTPREF_REDIRECT;	/* Preference for ICMP redirects */
gw_entry *redirect_gw_list;		/* Active ICMP gateways */
adv_entry *redirect_accept_list = NULL;	/* List of nets to accept from ICMP */
adv_entry **redirect_int_accept = NULL;	/* List of accept lists per interface */

void
rt_redirect(tp, dst, gateway, src, host_redirect)
task *tp;
sockaddr_un *dst, *gateway, *src;
int host_redirect;
{
    int saveinstall = install;
    rt_entry *rt;
    int interior = 0;
    pref_t preference = redirect_preference;
    register if_entry *ifp;
    gw_entry *gwp = 0;
    const char *redirect_type;
    flag_t table;

    /* XXX - How about installing all ICMP routes, then delete the ones we don't want */
    /* XXX - This will remove need for special interface to kernel delete routines */
    /* XXX - Maybe a flag to indicate that a delete failure is OK */

    rt_open(tp);

    if (host_redirect) {
	redirect_type = "host";
	table = RTS_HOSTROUTE;
    } else {
	redirect_type = "net";
	table = RTS_NETROUTE;
    }

    /* check gateway directly reachable */
    if (!if_withdst((sockaddr_un *) gateway)) {
	goto do_log;
    }
    /* Ignore if we are the source of this packet */
    if (src && if_withaddr((sockaddr_un *) src)) {
	goto do_log;
    }
    if (if_withaddr((sockaddr_un *) gateway) || if_withaddr((sockaddr_un *) dst)) {	/* a routing loop? */
	/*	XXX - What do to here?	*/
#ifdef	notdef
	tracef("rt_redirect: Routing loop, %A via %A",
	       dst,
	       &gateway);
	if (src) {
	    tracef(" from %A",
		   &src);
	}
	trace(TR_ALL, LOG_ERR, NULL);
#endif				/* notdef */
	goto do_log;
    }
    install = FALSE;			/* route already in kernel */

    tracef("REDIRECT: %s redirect", redirect_type);
    if (src != NULL) {
	tracef(" from %A",
	       src);
    }
    tracef(": %A via %A: ",
	   dst,
	   gateway);

    if (ignore_redirects) {
	trace(TR_RT, 0, "redirects not allowed");
	goto Delete;
    }
    if (ifp = if_withaddr((sockaddr_un *) gateway)) {
	trace(TR_RT, 0, "cannot redirect to myself");
	goto Delete;
    }
    if (!host_redirect) {
	IF_LIST(ifp) {
	    if (gd_inet_wholenetof(dst->in.sin_addr) == gd_inet_wholenetof(ifp->int_addr.in.sin_addr)) {
		interior++;
		break;
	    }
	} IF_LISTEND(ifp) ;
    }
    rt = rt_locate(table, (sockaddr_un *) dst, (flag_t) RTPROTO_ANY);
    if (src && rt && !equal(src, &rt->rt_router)) {
	trace(TR_RT, 0, "not from router in use");
	if (!equal(gateway, &rt->rt_router)) {
	    goto Delete;
	} else {
	    goto Invalid;
	}
    }

    gwp = gw_timestamp(&redirect_gw_list, RTPROTO_REDIRECT, src);
    /* If we have a list of trusted gateways, verify that this gateway is trusted */
    if (redirect_n_trusted && !(gwp->gw_flags & GWF_TRUSTED)) {
	trace(TR_ALL, LOG_ERR, "not from a trusted gateway");
	goto Delete;
    }

    ifp = if_withdst((sockaddr_un *) gateway);
    if (!ifp) {
	trace(TR_ALL, LOG_ERR, "can not find interface for gateway");
	goto Delete;
    }
    if (!is_valid_in((sockaddr_un *) dst,
		     redirect_accept_list,
		     INT_CONTROL(redirect_int_accept, ifp),
		     gwp->gw_accept,
		     &preference)) {
	trace(TR_RT, 0, "not valid");
	goto Delete;
    }

    trace(TR_RT, 0, NULL);

    rt = rt_locate(table, (sockaddr_un *) dst, RTPROTO_REDIRECT);
    /* XXX - what if a route with a lower preference exists?  Need to fix the kernel */
    if (rt) {
	if (rt_change(rt,
		      (sockaddr_un *) gateway,
		      rt->rt_metric,
		      (time_t) 0,
		      preference) == 0) {
	    trace(TR_RT, 0, "rt_redirect: error from rt_change");
	    goto Invalid;
	}
    } else {
	table = (host_redirect) ? RTS_HOSTROUTE : interior ? RTS_INTERIOR : RTS_EXTERIOR;
	if (!(rt = rt_add((sockaddr_un *) dst,
			  (sockaddr_un *) 0,
			  (sockaddr_un *) gateway,
			  gwp,
			  0,
			  table,
			  RTPROTO_REDIRECT,
			  0,
			  (time_t) 0,
			  preference))) {
	    trace(TR_RT, 0, "rt_redirect: error from rt_add");
	    goto Delete;
	}
    }
    goto do_log;

  Delete:
    /*
     *  Delete the entry from the kernel
     */

    install = saveinstall;
    (void) krt_delete_dst(tp,
			  dst,
			  (sockaddr_un *) 0,
			  gateway,
	  (flag_t) ((host_redirect ? RTF_HOST : 0) | RTF_UP | RTF_GATEWAY
#ifdef	RTF_DYNAMIC
		    | RTF_DYNAMIC
#endif				/* RTF_DYNAMIC */
			  )
	);

    /* If we have a route for this net installed, we had better reinstall it */
    rt = rt_locate(table, (sockaddr_un *) dst, (flag_t) RTPROTO_ANY);
    if (rt) {
	(void) krt_add(rt);
    }
    if (!(trace_flags & TR_KRT)) {
	goto Invalid;
    }
  do_log:

  Invalid:
    install = saveinstall;
    (void) rt_close(tp, gwp, 0);
    return;
}


#if	defined(AGENT_SNMP)
/*
 *	Routine to compare to routine table entries, used by rt_next
 */
int
rt_next_compare(rt1, rt2)
rt_entry **rt1, **rt2;
{
    u_long dst1 = ntohl((*rt1)->rt_dest.in.sin_addr.s_addr);
    u_long dst2 = ntohl((*rt2)->rt_dest.in.sin_addr.s_addr);
    int compare;

    if (dst1 < dst2) {
	compare = -1;
    } else if (dst1 > dst2) {
	compare = 1;
    } else {
	compare = 0;
    }
    return (compare);
}


/*
 *	Lookup next routing table entry, used by SNMP
 */
rt_entry *
rt_next(dst)
sockaddr_un *dst;
{
    int i;
    static int numb_routes = 0;
    static u_int n_routes = 0;
    rt_entry *rt;
    static rt_entry **rt_table_sort;
    rt_entry **rtp = rt_table_sort;

    if (rt_table_changed) {
	rt_table_changed = FALSE;
	n_routes = rt_net_routes + rt_host_routes;
	if ((n_routes > numb_routes)) {
	    if (rt_table_sort) {
		free((char *) rt_table_sort);
	    }
	    trace(TR_INT, 0, "rt_next: allocating routing table for %d routes", n_routes);
	    /* Allocate for one extra so the list is null terminated */
	    rt_table_sort = (rt_entry **) calloc(n_routes + 1, sizeof(rt_entry *));
	    if (rt_table_sort == NULL) {
		trace(TR_ALL, LOG_ERR, "rt_next: malloc: %m");
		return (NULL);
	    }
	    rtp = rt_table_sort;
	    numb_routes = n_routes;
	}
	trace(TR_INT, 0, "rt_next: copying and sorting table for %d routes", n_routes);
	i = 0;
	RT_TABLE(rt) {
	    rt_table_sort[i++] = rt;
	    if (i > n_routes) {
		trace(TR_ALL, LOG_ERR, "rt_next: n_routes = %d is too small", n_routes);
		return (NULL);
	    }
	} RT_TABLEEND;

	if (i != n_routes) {
	    trace(TR_ALL, LOG_ERR, "rt_next n_routes = %d, i = %d",
		  n_routes,
		  i);
	    return (NULL);
	}
	n_routes = i;

	qsort((char *) rt_table_sort, (int) n_routes, sizeof(rt_entry *), rt_next_compare);

    }
    if (dst) {
	u_long dest = ntohl(dst->in.sin_addr.s_addr);

	/* Really need some sort of binary search */
	do {
	    if (dest < (u_long) ntohl((*rtp)->rt_dest.in.sin_addr.s_addr)) {
		break;
	    }
	} while (*(++rtp));
    }
    return (*rtp);
}

#endif				/* defined(AGENT_SNMP) */


/*
 *	Dump routing table to dump file
 */
static void
rt_dump(fd)
FILE *fd;
{
    rt_entry *rt;
    rt_head *rth;

    /* Dump the control info */
    control_dump(fd);

    /*
     * Dump the static gateways
     */
    if (rt_gw_list) {
	(void) fprintf(fd,
		       "Gateways referenced by static routes:\n");
	gw_dump(fd,
		"\t\t",
		rt_gw_list);
    }
    (void) fprintf(fd, "Redirects: %s\n",
		   ignore_redirects ? "off" : "on");
    (void) fprintf(fd, "\tPreference: %d\n",
		   redirect_preference);
    if (redirect_gw_list) {
	(void) fprintf(fd, "\tActive gateways:\n");
	gw_dump(fd, "\t\t", redirect_gw_list);
    }
    control_accept_dump(fd, 1, redirect_accept_list, redirect_int_accept, redirect_gw_list);
    (void) fprintf(fd, "\n\n");

    /* Print our AS */
    if (my_system) {
	(void) fprintf(fd, "Autonomous system:\t%u\n",
		       my_system);
    }
    /*
     * Dump all the routing information
     */
    (void) fprintf(fd,
	    "\n\nRouting Tables:\n\tInstall: %s\tGenerate Default: %s\n",
		   install ? "yes" : "no",
		   rt_default_needed ? "yes" : "no");
    (void) fprintf(fd,
		   "\tRevision: %lu\n",
		   rt_revision);
    (void) fprintf(fd,
		   "\tHashsize: %d\t\tHashmask: %04x\n",
		   ROUTEHASHSIZ,
		   ROUTEHASHMASK);
    (void) fprintf(fd,
		   "\tEntries:\t%u nets\t%u hosts\n\n",
		   rt_net_routes,
		   rt_host_routes);

    RT_SCTBL RT_BUCKET(rth) {
	(void) fprintf(fd,
		  "\t%-15A\tmask %-15A\thash %u\tentries %d\tstate %s\n",
		       &rth->rth_dest,
		       &rth->rth_dest_mask,
		       rth->rth_hash,
		       rth->rth_entries,
		       trace_bits(rt_state_bits, rth->rth_state));
	RT_ALLRT(rt, rth) {
	    (void) fprintf(fd,
			   "\t\t%c%s\tPreference: %3d\n",
			   (rth->rt_active == rt) ? '*' : ' ',
			   trace_bits(rt_proto_bits, rt->rt_proto),
			   rt->rt_preference);

	    (void) fprintf(fd,
			   "\t\t\tGateway: %-15A\tInterface: %s\n",
			   &rt->rt_router,
			   rt->rt_ifp->int_name);

	    (void) fprintf(fd,
			   "\t\t\tFlags: <%s>",
			   trace_bits(rt_flag_bits, rt->rt_flags));
	    (void) fprintf(fd,
			   "\tState: <%s>\n",
			   trace_bits(rt_state_bits, rt->rt_state));

	    if (rt->rt_as) {
		(void) fprintf(fd,
			       "\t\t\tAS: %5u\n",
			       rt->rt_as);
	    }
	    if (!(rt->rt_state & RTS_NOAGE)) {
		(void) fprintf(fd,
			       "\t\t\tAge: %#T\tMax Age: %#T\n",
			       rt->rt_timer,
			       rt->rt_timer_max);
	    }
	    (void) fprintf(fd,
			   "\t\t\tMetric: %d",
			   rt->rt_metric);
	    (void) fprintf(fd,
			   "\tRevision: %lu\n",
			   rt->rt_revision);
	    /* Format protocol specific data */
	    if (rt->rt_data && rt->rt_data->rtd_dump) {
		rt->rt_data->rtd_dump(fd, rt);
	    }
	    (void) fprintf(fd, "\n");
	} RT_ALLRT_END(rt, rth);
	(void) fprintf(fd, "\n");
    } RT_BUCKET_END(rth) RT_SCTBL_END;
}


/*
 *	In preparation for a re-parse, reset the NOAGE flags on static routes
 *	so they will be deleted if they are not refreshed.
 */
/*ARGSUSED*/
static void
rt_cleanup(tp)
task *tp;
{
    int changes = 0;
    rt_entry *rt = (rt_entry *) 0;

    rt_open(rt_task);

    RT_WHOLE(rt) {
	if (rt->rt_proto & RTPROTO_STATIC) {
	    rt->rt_state &= ~RTS_NOAGE;
	    changes++;
	}
    } RT_WHOLEEND(rt);

    rt_default_cleanup();

    /* Reset defaults */
    ignore_redirects = FALSE;
    
    (void) rt_close(rt_task, (gw_entry *) 0, changes);

    adv_cleanup(&redirect_n_trusted, (int *) 0, redirect_gw_list,
		&redirect_accept_list, (adv_entry **) 0,
		&redirect_int_accept, (adv_entry ***) 0);
}


/*
 *	Delete any static routes that do not have the NOAGE flag.
 */
/*ARGSUSED*/
static void
rt_reinit(tp)
task *tp;
{
    int changes = 0;
    rt_entry *rt = (rt_entry *) 0;

    rt_open(rt_task);

    RT_WHOLE(rt) {
	if ((rt->rt_proto & RTPROTO_STATIC) && !(rt->rt_state & RTS_NOAGE)) {
	    (void) rt_unreach(rt);
	}
    } RT_WHOLEEND(rt);

    rt_default_reinit();
    
    (void) rt_close(rt_task, (gw_entry *) 0, changes);
}


 /*  Initialize the routing table.  The hash buckets are initilized	*/
 /*  with empty doubly linked list.  The last+1 entry is initilized to	*/
 /*  zero so the end of the list can be detected easily.		*/
 /*									*/
 /*  Also creates a timer and task for the job of aging the routing	*/
 /*  table 								*/
void
rt_init()
{
#ifdef	__HIGHC__
    rt_head *temp = (rt_head *) rt_inet_hash;

    rt_inet_hash[ROUTEHASHSIZ] = temp;
#else	/* __HIGHC__ */
    rt_inet_hash[ROUTEHASHSIZ] = (rt_head *) rt_inet_hash;
#endif	/* __HIGHC__ */

    rt_task = task_alloc("RT");
    rt_task->task_cleanup = rt_cleanup;
    rt_task->task_reinit = rt_reinit;
    rt_task->task_dump = rt_dump;
    if (!task_create(rt_task, 0)) {
	quit(EINVAL);
    }
    rt_timer = timer_create(rt_task, 0, "Age", 0, (time_t) RT_T_AGE, rt_time);
}
