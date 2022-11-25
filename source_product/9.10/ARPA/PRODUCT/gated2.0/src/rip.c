/*
 *  $Header: rip.c,v 1.1.109.6 92/03/04 08:28:20 ash Exp $
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


#define	RIPCMDS
#include "include.h"
#include "routed.h"
#include "rip.h"
#include "icmp.h"

#ifdef	PROTO_RIP

#define	RIP_TIMER_UPDATE	0
#define	RIP_TIMER_FLASH		1

static task *rip_task = (task *) 0;
static time_t rip_next_flash = (time_t) 0;

static struct servent rip_sp;

static char rip_packet[RIPPACKETSIZE + 1];
static struct rip *ripmsg = (struct rip *) rip_packet;

int rip_pointopoint = FALSE;		/* Are we ONLY doing pointopoint RIP? */
int rip_supplier = -1;			/* Are we broadcasting RIP protocols? */
int doing_rip = TRUE;			/* Are we running RIP protocols? */
metric_t rip_default_metric;		/* Default metric to use when propogating */
pref_t rip_preference;			/* Preference for RIP routes */

int rip_n_trusted = 0;			/* Number of trusted gateways */
int rip_n_source = 0;			/* Number of source gateways */
gw_entry *rip_gw_list = NULL;		/* List of RIP gateways */
adv_entry *rip_accept_list = NULL;	/* List of nets to accept from RIP */
adv_entry *rip_propagate_list = NULL;	/* List of sources to propagates routes to RIP */
adv_entry **rip_int_accept = NULL;	/* List of accept lists per interface */
adv_entry **rip_int_propagate = NULL;	/* List of propagate lists per interface */

#ifdef	RTM_ADD
#define	dst2sock(addr, dst)	memset ((addr)->sa_data, 0, sizeof (addr)->sa_data); \
		    (addr)->sa_family = ntohs((dst)->rip_family); \
    		    (addr)->sa_len = (sizeof (struct sockaddr_in)); \
		    socktype_in((addr))->sin_addr = (dst)->rip_addr
#else	/* RTM_ADD */
#define	dst2sock(addr, dst)	memset ((addr)->sa_data, 0, sizeof (addr)->sa_data); \
		    (addr)->sa_family = ntohs((dst)->rip_family); \
		    socktype_in((addr))->sin_addr = (dst)->rip_addr
#endif	/* RTM_ADD */

#define	sock2dst(dst, addr)	memset ((dst), 0, sizeof *(dst)); \
    		    (dst)->rip_family = htons((addr)->sa_family); \
    		    (dst)->rip_addr = socktype_in((addr))->sin_addr

/*
 *	Trace RIP packets
 */
static void
rip_trace(dir, who, cp, size)
struct sockaddr_in *who;		/* should be sockaddr */
char *dir, *cp;
register int size;
{
    register struct rip *rpmsg = (struct rip *) cp;
    register struct netinfo *n;
    register const char *cmd = "Invalid";
    struct sockaddr addr;

    if (rpmsg->rip_cmd && rpmsg->rip_cmd < RIPCMD_MAX) {
	cmd = ripcmds[rpmsg->rip_cmd];
    }
    tracef("RIP %s %#A vers %d, cmd %s, length %d",
	   dir,
	   who,
	   rpmsg->rip_vers,
	   cmd, size);
    switch (rpmsg->rip_cmd) {
#ifdef	RIPCMD_POLL
	case RIPCMD_POLL:
#endif				/* RIPCMD_POLL */
	case RIPCMD_REQUEST:
	case RIPCMD_RESPONSE:
	    trace(TR_RIP, 0, NULL);
	    if (trace_flags & TR_UPDATE) {
		size -= 4 * sizeof(char);
		n = rpmsg->rip_nets;
		for (; size > 0; n++, size -= sizeof(struct netinfo)) {
		    if (size < sizeof(struct netinfo)) {
			break;
		    }
		    dst2sock(&addr, &n->rip_dst);
		    if (addr.sa_family == AF_UNSPEC &&
			!socktype_in(&addr)->sin_addr.s_addr &&
			ntohl(n->rip_metric) == RIPHOPCNT_INFINITY) {
			trace(TR_RIP | TR_NOSTAMP, 0, "\trouting table request");
		    } else {
			trace(TR_RIP | TR_NOSTAMP, 0, "\tnet %-15A  metric %2d",
			      &addr,
			      ntohl(n->rip_metric));
		    }
		}
		trace(TR_RIP, 0, "RIP %s end of packet", dir);
	    }
	    break;
	case RIPCMD_TRACEON:
	    trace(TR_RIP, 0, ", file %*s", size, rpmsg->rip_tracefile);
	    break;
#ifdef	RIPCMD_POLLENTRY
	case RIPCMD_POLLENTRY:
	    n = rpmsg->rip_nets;
	    dst2sock(&addr, &n->rip_dst);
	    trace(TR_RIP, 0, ", net %A",
		  &addr);
	    break;
#endif				/* RIPCMD_POLLENTRY */
	default:
	    trace(TR_RIP, 0, NULL);
	    break;
    }
    trace(TR_RIP, 0, NULL);
}


static void
rip_send(tp, flags, sin, size)
task *tp;
flag_t flags;
struct sockaddr_in *sin;
int size;
{
    struct sockaddr_in dst;

    dst = *sin;
    sin = &dst;
    if (sin->sin_port == 0) {
	sin->sin_port = rip_sp.s_port;
    }
    (void) task_send_packet(tp, rip_packet, size, flags, (sockaddr_un *) sin);
    if (trace_flags & TR_RIP) {
	rip_trace("SENT", sin, rip_packet, size);
    }
}


/*
 * Supply dst with the contents of the routing tables.
 * If this won't fit in one packet, chop it up into several.
 */
/*ARGSUSED*/
static void
rip_supply(tp, dst, src, ifp, gwp, do_split_horizon, flash_update)
task *tp;
struct sockaddr_in *dst, *src;
if_entry *ifp;
gw_entry *gwp;
int do_split_horizon;
int flash_update;
{
    rt_entry *rt;
    struct netinfo *n;
    int size;
    metric_t metric = 0, split_horizon;

    ripmsg->rip_cmd = RIPCMD_RESPONSE;
    ripmsg->rip_vers = RIPVERSION;
    n = ripmsg->rip_nets;

    RT_TABLE(rt) {
	if (rt->rt_state & RTS_NOADVISE) {
	    continue;
	}
	if (flash_update && (rip_task->task_rtrevision >= rt->rt_revision)) {
	    continue;
	}
	/* Do not send interface routes back to the same interface */
	if ((rt->rt_ifp == ifp) && (rt->rt_proto & RTPROTO_DIRECT)) {
	    continue;
	}
	/* Subnets and host routes do not go everywhere */
	if (rt->rt_state & RTS_HOSTROUTE) {
	    if (((ifp->int_net.in.sin_addr.s_addr ^ rt->rt_dest.in.sin_addr.s_addr) &
		 ifp->int_netmask.in.sin_addr.s_addr) &&
		!((rt->rt_ifp->int_net.in.sin_addr.s_addr ^ rt->rt_dest.in.sin_addr.s_addr) &
		  rt->rt_ifp->int_netmask.in.sin_addr.s_addr)) {
		/* Host is being sent to another network and we learned it via it's home network */
		/* XXX - This assumes we are announcing the network route */
		continue;
	    }
	} else if (rt->rt_state & RTS_SUBNET) {
	    if ((rt->rt_dest.in.sin_addr.s_addr & ifp->int_netmask.in.sin_addr.s_addr) != ifp->int_net.in.sin_addr.s_addr) {
		/* Only send subnets to interfaces of the same network */
		continue;
	    }
	} else {
	    if (rt->rt_dest.in.sin_addr.s_addr == ifp->int_net.in.sin_addr.s_addr) {
		/* Do not send the whole net to a subnet */
		continue;
	    }
	}

	if ((rt->rt_proto & rip_task->task_rtproto) &&
	    (ifp == rt->rt_ifp) &&
	    do_split_horizon &&
	    (ifp->int_state & (IFS_BROADCAST|IFS_POINTOPOINT) || equal_in(rt->rt_router.in.sin_addr, dst->sin_addr))) {
	    split_horizon = RIPHOPCNT_INFINITY;
	} else {
	    split_horizon = 0;
	}

	if (rt->rt_ifp->int_state & IFS_LOOPBACK) {
	    /* Routes via the loopback interface must have an explicit metric */
	    metric = RIPHOPCNT_INFINITY;
	}

	metric = (rt->rt_proto & RTPROTO_DIRECT) ? RIP_HOP : rip_default_metric;
	if (!propagate(rt,
		       rip_task->task_rtproto,
		       rip_propagate_list,
		       INT_CONTROL(rip_int_propagate, ifp),
		       gwp ? gwp->gw_propagate : NULL,
		       &metric)) {
	    continue;
	}
	/* Add the interface mteric */
	metric += ifp->int_metric;

	if (split_horizon) {
	    metric = split_horizon;
	}
	if (!(rt->rt_ifp->int_state & IFS_UP) || (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE))) {
	    metric = RIPHOPCNT_INFINITY;
	}
	if (flash_update && (metric >= RIPHOPCNT_INFINITY) && (rt->rt_ifp == ifp) && (rt->rt_proto & rip_task->task_rtproto)) {
	    /* Don't flash deleted or unreachable routes back to their source */
	    continue;
	}
	size = (char *) n - rip_packet;
	if (size > (RIPPACKETSIZE - sizeof(struct netinfo))) {
	    rip_send(tp, 0, dst, size);
	    n = ripmsg->rip_nets;
	}
	sock2dst(&n->rip_dst, &rt->rt_dest.a);

	/* Make sure metric is valid */
	if (metric > RIPHOPCNT_INFINITY) {
	    metric = RIPHOPCNT_INFINITY;
	}
	n->rip_metric = metric;
	n->rip_metric = htonl(n->rip_metric);
	n++;
    } RT_TABLEEND;

    if ((n != ripmsg->rip_nets) || !src) {	/* OK to reply to a RIPQUERY with an empty packet */
	size = (char *) n - rip_packet;
	rip_send(tp, 0, dst, size);
    }
}


/*
 * 	Check out a newly received RIP packet.
 */

void
rip_recv(tp)
task *tp;
{
    int size;
    register rt_entry *rt;
    register struct netinfo *n;
    register if_entry *ifp, *ifpc;
    gw_entry *gwp = (gw_entry *) 0;
    register int OK = TRUE;
    struct rip *inripmsg = (struct rip *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    int change = FALSE;
#ifdef	RIP_CHECK_ZERO
    int check_zero = FALSE;
#endif	/* RIP_CHECK_ZERO */
    int newsize;
    flag_t rte_table;
    u_short src_port;
    metric_t metric;
    const char *reject_msg = (char *) 0;
    char type[MAXHOSTNAMELENGTH];
    int answer = FALSE;
    int split_horizon = TRUE;
    struct sockaddr dest;
		

    if (task_receive_packet(tp, &size)) {
	return;
    }
    if (recv_addr.in.sin_family != AF_INET) {
	reject_msg = "protocol not INET";
	goto Reject;
    }
    src_port = recv_addr.in.sin_port;
    recv_addr.in.sin_port = 0;		/* For comparisons */

    switch (inripmsg->rip_vers) {
	case 0:
	    reject_msg = "ignoring version 0 packets";
	    goto Reject;
	case 1:
#ifdef	RIP_CHECK_ZERO
	    check_zero++;
#endif	/* RIP_CHECK_ZERO */
	    break;
    }

    /* Locate or create a gateway structure for this gateway */
    gwp = gw_timestamp(&rip_gw_list, rip_task->task_rtproto, (sockaddr_un *) & recv_addr);

    /* If we have a list of trusted gateways, verify that this gateway is trusted */
    if (rip_n_trusted && !(gwp->gw_flags & GWF_TRUSTED)) {
	OK = FALSE;
    }
    if (trace_flags & TR_RIP) {
	rip_trace("RECV", &recv_addr.in, (caddr_t) inripmsg, size);
    }
#ifdef	RIP_CHECK_ZERO
    if (check_zero && inripmsg->rip_res) {
	/* XXX - Should check later on that the sin_zero fields are zero */
	reject_msg = "reserved fields not zero";
	goto Reject;
    }
#endif				/* RIP_CHECK_ZERO */

    switch (inripmsg->rip_cmd) {
#ifdef	RIPCMD_POLL
	case RIPCMD_POLL:
	    answer = TRUE;
	    split_horizon = FALSE;
#endif				/* RIPCMD_POLL */
	case RIPCMD_REQUEST:
	    if ((src_port != rip_sp.s_port) || answer) {
		recv_addr.in.sin_port = src_port;

		if ((ifp = if_withdst((sockaddr_un *) & recv_addr)) <= (if_entry *) 0) {
		    struct sockaddr_in dst;

		    dst = recv_addr.in;	/* struct copy */

		    dst.sin_addr.s_addr = htonl(gd_inet_netof(dst.sin_addr));
		    if (!(rt = rt_lookup(RTS_NETROUTE, (sockaddr_un *) & dst))) {
			if (!(rt = rt_lookup(RTS_NETROUTE, (sockaddr_un *) & default_net))) {
			    reject_msg = "can not find interface for route";
			    goto Reject;
			}
		    }
		    ifp = rt->rt_ifp;
		}
	    } else {
		ifpc = if_withaddr((sockaddr_un *) & recv_addr);
		if (ifpc) {
		    return;
		}
		if (!OK) {
		    reject_msg = "not on trustedripgateways list";
		    goto Reject;
		}
		if ((ifp = if_withdst((sockaddr_un *) & recv_addr)) <= (if_entry *) 0) {
		    reject_msg = "not on same net";
		    goto Reject;
		}
		if (ifp->int_state & (IFS_NORIPIN | IFS_NORIPOUT)) {
		    reject_msg = "interface marked for no RIP in/out";
		    goto Reject;
		}
		if (!rip_supplier) {
		    reject_msg = "not supplying RIP";
		    goto Reject;
		}
	    }

	    gwp->gw_flags |= GWF_QUERY | GWF_ACCEPT;
	    newsize = 0;
	    size -= 4 * sizeof(char);
	    n = inripmsg->rip_nets;
	    while (size > 0) {
		if (size < sizeof(struct netinfo)) {
		    break;
		}
		size -= sizeof(struct netinfo);
		dst2sock(&dest, &n->rip_dst);
		n->rip_metric = ntohl(n->rip_metric);
		if (dest.sa_family == AF_UNSPEC &&
		    n->rip_metric == RIPHOPCNT_INFINITY &&
		    !size) {
		    recv_addr.in.sin_port = src_port;

		    rip_supply(tp, &recv_addr.in, (struct sockaddr_in *) 0, ifp, gwp, split_horizon, FALSE);
		    return;
		}
		rt = rt_lookup(RTS_INTERIOR, (sockaddr_un *) &dest);
		n->rip_metric = htonl(!rt ? RIPHOPCNT_INFINITY : min(rt->rt_metric + ifp->int_metric, RIPHOPCNT_INFINITY));
		n++;
		newsize += sizeof(struct netinfo);
	    }

	    if (newsize > 0) {
		recv_addr.in.sin_port = src_port;

		inripmsg->rip_cmd = RIPCMD_RESPONSE;
		newsize += sizeof(int);
		memcpy((char *) ripmsg, (char *) inripmsg, newsize);
		rip_send(tp, 0, &recv_addr.in, newsize);
	    }
	    return;
	case RIPCMD_TRACEON:
	case RIPCMD_TRACEOFF:
	    if (!OK) {
		reject_msg = "not on trustedripgateways list";
		goto Reject;
	    }
	    if (ntohs(src_port) > IPPORT_RESERVED) {
		reject_msg = "not from a trusted port";
		goto Reject;
	    }
	    if ((ifp = if_withdst((sockaddr_un *) & recv_addr)) <= (if_entry *) 0) {
		reject_msg = "not on same net";
		goto Reject;
	    }
	    if (ifp->int_state & IFS_NORIPIN) {
		reject_msg = "not listening to RIP on this interface";
		goto Reject;
	    }
	    reject_msg = "TRACE packets not supported";
	    goto Reject;
#ifdef	RIPCMD_POLLENTRY
	case RIPCMD_POLLENTRY:
	    n = inripmsg->rip_nets;
	    newsize = sizeof(struct entryinfo);
	    dst2sock(&dest, &n->rip_dst);
	    switch (dest.sa_family) {
	    case AF_INET:
		rt = rt_lookup(RTS_INTERIOR, (sockaddr_un *) &dest);
		break;

	    default:
		rt = 0;
	    }
	    if (rt) {			/* don't bother to check rip_vers */
		struct entryinfo *e = (struct entryinfo *) n;

		sock2dst(&e->rtu_dst, &rt->rt_dest.a);
		sock2dst(&e->rtu_router, &rt->rt_router.a);
		e->rtu_flags = htons((unsigned short) rt->rt_flags);
		e->rtu_state = htons((unsigned short) rt->rt_state);
		e->rtu_timer = htonl((unsigned long) rt->rt_timer);
		e->rtu_metric = rt->rt_metric;
		e->rtu_metric = htonl((u_long) e->rtu_metric);
		ifp = rt->rt_ifp;
		if (ifp) {
		    e->int_flags = htonl((unsigned long) ifp->int_state);
		    (void) strncpy(e->int_name, rt->rt_ifp->int_name, sizeof(e->int_name));
		} else {
		    e->int_flags = 0;
		    (void) strcpy(e->int_name, "(none)");
		}
	    } else {
		memset((char *) n, (char) 0, newsize);
	    }

	    newsize += sizeof (struct rip) - sizeof (struct netinfo);
	    memcpy((char *) ripmsg, (char *) inripmsg, newsize);
	    gwp->gw_flags |= GWF_QUERY | GWF_ACCEPT;
	    recv_addr.in.sin_port = src_port;
	    rip_send(tp, 0, &recv_addr.in, newsize);
	    return;
#endif				/* RIPCMD_POLLENTRY */
	case RIPCMD_RESPONSE:
	    /*
             *  Are we talking to ourselves???
             *
             *  if_withaddr() handles PTP's also.  If from a
             *  dst of a PTP link, let it through for further processing.
             *  you shouldn't receive your own RIPs on a PTP.
             */

	    ifpc = if_withaddr((sockaddr_un *) & recv_addr);
	    if (ifpc) {
		if_rtupdate(ifpc);
		if ((ifpc->int_state & IFS_POINTOPOINT) == 0) {
		    return;
		}
	    }
	    if (!OK) {
#ifndef	notdef
		reject_msg = "not on trustedripgateways list";
		goto Reject;
#else				/* notdef */
		return;
#endif				/* notdef */
	    }
	    if (src_port != rip_sp.s_port) {
		reject_msg = "not from a trusted port";
		goto Reject;
	    }
	    if ((ifp = if_withdst((sockaddr_un *) & recv_addr)) <= (if_entry *) 0) {
		reject_msg = "not on same net";
		goto Reject;
	    }
	    if (ifp->int_state & IFS_NORIPIN) {
		reject_msg = "interface marked for no RIP in";
		goto Reject;
	    }
	    gwp->gw_flags |= GWF_ACCEPT;

	    /*
             * update interface timer on interface that packet came in on.
             */
	    if_rtupdate(ifp);

	    rt_open(tp);

	    size -= 4 * sizeof(char);
	    n = inripmsg->rip_nets;
	    for (; size >= sizeof(struct netinfo); size -= sizeof(struct netinfo), n++) {
	        pref_t preference = rip_preference;

		dst2sock(&dest, &n->rip_dst);
		switch (dest.sa_family) {
		case AF_INET:
		    if (gd_inet_checkhost((struct sockaddr_in *) &dest)) {
			break;
		    }
		    /* Fall through */

		default:
		    continue;
		}

		/* Verify that this is a valid metric */
		if (!is_valid_in((sockaddr_un *) &dest,
				 rip_accept_list,
				 INT_CONTROL(rip_int_accept, ifp),
				 gwp->gw_accept,
				 &preference)) {
		    continue;
		}
		/*
	         *  Convert metric to host byte order.  If metric is zero, ignore this network
	         */
		if (!(metric = ntohl(n->rip_metric))) {
		    continue;
		}
		/* Now add hop count to metric */
		metric += ifp->int_metric + RIP_HOP;

		/* Determine routing table based on host bits */
		rte_table = gd_inet_ishost((struct sockaddr_in *) &dest) ? RTS_HOSTROUTE : RTS_INTERIOR;

		rt = rt_locate(rte_table, (sockaddr_un *) &dest, rip_task->task_rtproto);
		if (!rt) {
		    /* new route */

		    if (metric >= RIPHOPCNT_INFINITY) {
			continue;
		    }
		    (void) rt_add((sockaddr_un *) &dest,
				  (sockaddr_un *) 0,
				  (sockaddr_un *) & recv_addr,
				  gwp,
				  metric,
				  rte_table,
				  rip_task->task_rtproto,
				  0,
				  (time_t) 0,
				  preference);
		    change = TRUE;
		} else {
		    /* Existing route */
		    if ((rt->rt_flags & RTF_GATEWAY) == 0) {
			continue;
		    }
		    if (equal(&rt->rt_router, &recv_addr)) {
			if (metric >= RIPHOPCNT_INFINITY) {
			    change += rt_unreach(rt);
			    continue;
			}
			if ((metric != rt->rt_metric) || (rt->rt_state & RTS_HOLDDOWN)) {
			    if (rt_change(rt,
					  (sockaddr_un *) & recv_addr,
					  metric,
					  (time_t) 0,
					  preference)) {
				change = TRUE;
			    }
			}
			rt_refresh(rt);
		    } else {
			if ((metric >= RIPHOPCNT_INFINITY) || (rt->rt_state & RTS_HOLDDOWN)) {
			    continue;
			}
			if ((metric < rt->rt_metric) ||
			    ((rt->rt_timer > (rt->rt_timer_max / 2)) &&
			     (rt->rt_metric == metric) && !(rt->rt_state & (RTS_CHANGED | RTS_REFRESH)))) {
			    if (rt_change(rt,
					  (sockaddr_un *) & recv_addr,
					  metric,
					  (time_t) 0,
					  preference)) {
				change = TRUE;
			    }
			}
		    }
		}
	    }				/*  for each route */
	    if (rt_close(tp, gwp, change)) {
		task_flash(rip_task);
	    }
	    break;
	default:
	    reject_msg = "invalid or not implemented command";
	    goto Reject;
    }
    return;

  Reject:
    if (inripmsg->rip_cmd < RIPCMD_MAX) {
	(void) strcpy(type, ripcmds[inripmsg->rip_cmd]);
    } else {
	(void) sprintf(type, "#%d", inripmsg->rip_cmd);
    }
    trace(TR_RIP, 0, "rip_recv: ignoring RIP %s packet from %#A - %s",
	  type,
	  &recv_addr,
	  reject_msg);
    trace(TR_RIP, 0, NULL);
    if (gwp) {
	gwp->gw_flags |= GWF_REJECT;
    }
    return;
}


/*
 * Output a preformed RIP packet.
 */

/*ARGSUSED*/
static void
rip_out(tp, dst, src, ifp, gwp, do_split_horizon, flash_update)
task *tp;
struct sockaddr_in *dst, *src;
if_entry *ifp;
gw_entry *gwp;
int do_split_horizon;
int flash_update;
{
    metric_t tmp = ntohl(ripmsg->rip_nets[0].rip_metric);
    rt_entry *rt;
    struct netinfo *n = ripmsg->rip_nets;
    struct sockaddr dest;

    dst2sock(&dest, &n->rip_dst);

    if (dst->sin_family != AF_INET) {
	return;
    }
    /*
     * Check to see if we are sending the initial RIP request to other
     * gateways.  That request has no restrictions other than whether RIP
     * is allowed on that interface or not.  This restriction is handled
     * in toall().
     */

    if (dest.sa_family != AF_UNSPEC ||
	ntohl(n->rip_metric) != RIPHOPCNT_INFINITY) {

	rt = rt_lookup(RTS_INTERIOR, (sockaddr_un *) &dest);
	if (rt == NULL) {
	    rt = rt_lookup(RTS_HOSTROUTE, (sockaddr_un *) &dest);
	    if (rt == NULL) {
		trace(TR_ALL, LOG_ERR, "rip_out: bad route %A",
		      &dest);
		return;
	    }
	}

	/*
         * XXX - make sure this route can be announced via this interface/proto.
        if (!is_valid(rt, rip_task->task_rtproto, ifp)) {
          return;
        }
         */
	/*
         * since we are only sending out this one packet, we can add the
         * interface metric here.  Don't forget Split Horizon.
         */
	if ((rt->rt_proto & rip_task->task_rtproto) && (ifp == rt->rt_ifp) && do_split_horizon &&
	    (ifp->int_state & (IFS_BROADCAST|IFS_POINTOPOINT) || equal_in(rt->rt_router.in.sin_addr, dst->sin_addr))) {
	    tmp = ntohl(n->rip_metric);
	    n->rip_metric = htonl(RIPHOPCNT_INFINITY);
	} else if ((tmp = ntohl(n->rip_metric)) != RIPHOPCNT_INFINITY) {
	    if ((tmp + ifp->int_metric) >= RIPHOPCNT_INFINITY) {
		n->rip_metric = htonl(RIPHOPCNT_INFINITY);
	    } else {
		n->rip_metric = htonl((tmp + ifp->int_metric));
	    }
	}
    }
    rip_send(rip_task, 0, dst, sizeof(struct rip));
    n->rip_metric = tmp;
    n->rip_metric = htonl(n->rip_metric);
}


/*
 *	send RIP packets
 */
/*ARGSUSED*/
static void
rip_job(tip, interval)
timer *tip;
time_t interval;
{
    task_toall(tip->timer_task,
	       rip_supply,
	       rip_pointopoint,
	       IFS_NORIPOUT,
	       rip_n_source ? rip_gw_list : NULL,
	       FALSE);
}


/*
 *	send a flash update packet
 */
/*ARGSUSED*/
static void
rip_do_flash(tip, interval)
timer *tip;
time_t interval;
{
    trace(TR_TASK, 0, "rip_do_flash: Doing flash update for RIP");
    task_toall(rip_task,
	       rip_supply,
	       rip_pointopoint,
	       IFS_NORIPOUT,
	       rip_n_source ? rip_gw_list : NULL,
	       TRUE);
    rip_next_flash = (time_t) (random() % 4 + 1) + time_sec;
    trace(TR_TASK, 0, "rip_do_flash: Flash update done, none before %T", rip_next_flash);
}


/*
 *	Check to see if a flash update packet is allowed and send or schedule it
 */
static void
rip_flash(tp)
task *tp;
{
    if (time_sec >= rip_next_flash) {
	/* A flash update can be sent now, do it */
	rip_do_flash(tp->task_timer[RIP_TIMER_FLASH], (time_t) 0);
    } else if (!tp->task_timer[RIP_TIMER_FLASH] && (tp->task_timer[RIP_TIMER_UPDATE]->timer_next_time > rip_next_flash)) {
	/* A flash update can't be sent and one is not yet scheduled */
	(void) timer_create(tp,
			    RIP_TIMER_FLASH,
			    "Flash",
			    TIMERF_DELETE | TIMERF_ABSOLUTE,
			    rip_next_flash - time_sec,
			    rip_do_flash);
    }
}


/*
 *	Cleanup before re-init
 */
/*ARGSUSED*/
static void
rip_cleanup(tp)
task *tp;
{
    adv_cleanup(&rip_n_trusted, &rip_n_source, rip_gw_list,
		&rip_accept_list, &rip_propagate_list,
		&rip_int_accept, &rip_int_propagate);
}


/*
 *	Dump info about RIP
 */
static void
rip_dump(fd)
FILE *fd;
{
    (void) fprintf(fd, "RIP:\n");
    (void) fprintf(fd, "\tDefault metric: %d\t\tDefault preference: %d\n",
		   rip_default_metric,
		   rip_preference);
    if (rip_gw_list) {
	(void) fprintf(fd, "\tActive gateways:\n");
	gw_dump(fd, "\t\t", rip_gw_list);
    }
    control_accept_dump(fd, 1, rip_accept_list, rip_int_accept, rip_gw_list);
    control_propagate_dump(fd, 1, rip_propagate_list, rip_int_propagate, rip_gw_list);
    (void) fprintf(fd, "\n\n");
}


/*
 * initialize RIP socket and RIP task
 */

/*ARGSUSED*/
void
rip_init()
{
    static struct servent *sp;
    struct sockaddr_in addr;
    if_entry *ifp;
    void (*flash) () = rip_flash;	/* Hack for UTX/32 and Ultrix */

    if (doing_rip) {
	if (!rip_task) {
	    if (rip_supplier < 0) {
		if (n_interfaces > 1) {
		    rip_supplier = TRUE;
		    trace(TR_ALL, LOG_NOTICE, "rip_init: Acting as RIP supplier to our direct nets");
		}
		IF_LIST(ifp) {
		    if (ifp->int_state & IFS_POINTOPOINT) {
			rip_supplier = TRUE;
			trace(TR_INT, 0, "init_if: PointoPoint RIP supplier to: %s", ifp->int_name);
		    }
		} IF_LISTEND(ifp) ;
	    }
	    if (rip_supplier < 0) {
		rip_supplier = FALSE;
	    }
	    if ((sp = getservbyname("router", "udp")) == NULL) {
		trace(TR_ALL, LOG_ERR, "No service for router available, using %d",
		      RIP_PORT);
		memset((caddr_t) & rip_sp, (char) 0, sizeof(rip_sp));
		rip_sp.s_port = htons(RIP_PORT);
	    } else {
		memcpy((char *) &rip_sp, (char *) sp, sizeof(rip_sp));
	    }

	    sockclear_in(&addr);
	    addr.sin_port = rip_sp.s_port;
	    addr.sin_addr.s_addr = INADDR_ANY;

	    rip_task = task_alloc("RIP");
	    sockcopy(&addr, &rip_task->task_addr);
	    rip_task->task_rtproto = RTPROTO_RIP;
	    rip_task->task_recv = rip_recv;
	    rip_task->task_cleanup = rip_cleanup;
	    rip_task->task_dump = rip_dump;

	    if ((rip_task->task_socket = task_get_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		quit(errno);
	    }
	    if (!test_flag) {
		if (task_set_option(rip_task, TASKOPTION_BROADCAST, (caddr_t) TRUE) < 0) {
		    quit(errno);
		}
		if (task_set_option(rip_task, TASKOPTION_RECVBUF, (caddr_t) (32 * 1024)) < 0) {
		    quit(errno);
		}
		if (task_set_option(rip_task, TASKOPTION_DONTROUTE, (caddr_t) TRUE) < 0) {
		    quit(errno);
		}
		if (bind(rip_task->task_socket, (struct sockaddr *) & addr, socksize(&addr)) < 0) {
		    trace(TR_ALL, LOG_ERR, "rip_init: bind: %m");
		    (void) close(rip_task->task_socket);
		    quit(errno);
		}
	    }
	    if (rip_supplier) {
		rip_task->task_flash = flash;
		(void) timer_create(rip_task,
				    RIP_TIMER_UPDATE,
				    "Update",
				    0,
				    (time_t) RIP_INTERVAL,
				    rip_job);
	    }
	    if (!task_create(rip_task, RIPPACKETSIZE)) {
		quit(EINVAL);
	    }
	    if (RIP_INTERVAL < rt_timer->timer_interval) {
		timer_interval(rt_timer, (time_t) RIP_INTERVAL);
	    }
	    if (!test_flag) {
		/* Generate a RIP REQUEST packet asking for all known RIP routes */
		ripmsg->rip_cmd = RIPCMD_REQUEST;
		ripmsg->rip_vers = RIPVERSION;
		ripmsg->rip_nets[0].rip_dst.rip_family = htons(AF_UNSPEC);
		ripmsg->rip_nets[0].rip_metric = htonl(RIPHOPCNT_INFINITY);
		task_toall(rip_task,
			   rip_out,
			   rip_pointopoint,
			   IFS_NORIPOUT,
			   rip_n_source ? rip_gw_list : NULL,
			   FALSE);
	    }
	}
	if (rip_supplier) {
	    if_rtactive = TRUE;		/* Indicate we are broadcasting */
	    ignore_redirects = TRUE;	/* Gateways don't listen to redirects */
	}
    } else {
	rip_cleanup((task *) 0);
	if (rip_task) {
	    task_delete(rip_task);
	    rip_task = (task *) 0;
	}
    }
}


#endif				/* PROTO_RIP */
