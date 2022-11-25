/*
 *  $Header: bgp_rt.c,v 1.1.109.5 92/02/28 14:01:25 ash Exp $
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
#include "bgp.h"

#ifdef	PROTO_BGP

static rt_data as_base =
{&as_base, &as_base};

static bits asorigin_bits[] =
{
    {0, NULL},
    {ASO_IGP, "IGP"},
    {ASO_EGP, "EGP"},
    {ASO_INCOMPLETE, "Incomplete"},
    {0}
};

/*
 *	Dump the AS paths
 */
void
bgp_as_dump(fd)
FILE *fd;
{
    int i;
    rt_data *rtd;
    as_path *asp;

    /* Dump AS path information */
    (void) fprintf(fd,
		   "\n\nAS Paths:\n");

    RTDATA_LIST(rtd, &as_base) {
	asp = (as_path *) rtd->rtd_data;
	(void) fprintf(fd,
		       "\tRefcount %4d  Length %3d:",
		       rtd->rtd_refcount,
		       asp->as_count);
	for (i = 0; i < asp->as_count; i++) {
	    (void) fprintf(fd,
			   " %5u",
			   asp->as_number[i]);
	}
	(void) fprintf(fd,
		       " %5s\n",
		       trace_state(asorigin_bits, asp->as_origin));

    } RTDATA_LIST_END(rtd, &as_base);
    (void) fprintf(fd, "\n");

}


/*
 *	Dump an AS path
 */
static void
bgp_rt_dump(fd, rt)
FILE *fd;
rt_entry *rt;
{
    int i;
    as_path *asp;

    if (rt->rt_data && rt->rt_data->rtd_data) {
	asp = (as_path *) rt->rt_data->rtd_data;

	if (asp) {
	    (void) fprintf(fd, "\t\t\tPath:");
	    for (i = 0; i < asp->as_count; i++) {
		(void) fprintf(fd,
			       " %u",
			       asp->as_number[i]);
	    }
	    (void) fprintf(fd, " %s\n",
			   trace_state(asorigin_bits, asp->as_origin));
	}
    }
}


/*
 *	Allocate an AS entry given it's size
 */
rt_data *
bgp_as_alloc(pairs)
int pairs;
{
    rt_data *rtd;

    rtd = rtd_alloc(sizeof(as_path) + (sizeof(as_t) * (pairs - 1)));
    rtd->rtd_dump = bgp_rt_dump;

    return (rtd);
}


/*
 *	Build a one or two entry AS path, locate preexisting path and increment reference count
 */
as_path *
#ifdef	USE_PROTOTYPES
bgp_as_build(int count, int origin, as_t AS)
#else				/* USE_PROTOTYPES */
bgp_as_build(count, origin, AS)
int count;
int origin;
as_t AS;

#endif				/* USE_PROTOTYPES */
{
    static as_path asp;

    if (count) {
	asp.as_count = count;
	asp.as_number[0] = AS;
    }
    asp.as_origin = origin;

    return (&asp);
}


void
bgp_recv_Update(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{
    u_char *cp, *lp;
    int i, j;
    u_char asCount;
    u_char asDirection;
    as_t asNumber;
    u_short netCount;
    u_short netMetric;
    int error = 0;
    pref_t preference;
    struct sockaddr_in gateway;
    struct in_addr bgp_gateway;
    struct sockaddr_in netNumber;
    metric_t metric = 0;
    time_t rt_maxage = 0;		/* BGP routes don't time out */
    rt_entry *rt;
    rt_data *rtd = (rt_data *) 0;
    as_path *asp;

    sockclear_in(&gateway);
    sockclear_in(&netNumber);

    cp = (u_char *) PDU;
    lp = cp + length;
    cp += sizeof(pduHeader);

    rt_open(bnp->bgp_task);

    /* Parse the gateway */
    if ((cp + sizeof(bgp_gateway)) >= lp) {
	/* This should probably be an invalid length error */
	error = BGPUPDERR_GATEWAY;
	goto Error;
    }
    PickUp(cp, bgp_gateway);
    if (bnp->bgp_options & BGPO_GATEWAY) {
	gateway = bnp->bgp_gateway;	/* struct copy */
    } else {
	gateway.sin_addr = bgp_gateway;	/* struct copy */
    }

    /* Parse the AS path */
    PickUp(cp, asCount);
    if (asCount == 0) {
	error = BGPUPDERR_ASCOUNT;
	goto Error;
    }
    if ((cp + asCount * (sizeof(asDirection) + sizeof(asNumber))) >= lp) {
	error = BGPUPDERR_ASCOUNT;
	goto Error;
    }
    rtd = bgp_as_alloc(i = asCount);
    asp = (as_path *) rtd->rtd_data;
    asp->as_count = asCount;
    for (i = 0; i < asCount; i++) {
	/* Pickup direction and As Number */
	PickUp(cp, asDirection);
	PickUp(cp, asNumber);
	asNumber = ntohs(asNumber);

	/* Make sure direction is valid */
	if (!asDirection || (asDirection > asDirMax)) {
	    error = BGPUPDERR_DIRECTION;
	    goto Error;
	}
	/* Make sure EGP and Incompelete types only occur at the end of the list */
	if (((asDirection == asDirEgp) || (asDirection == asDirIncomplete)) && (i != asCount - 1)) {
	    error = BGPUPDERR_ORDER;
	    goto Error;
	}
	/* Check for loops */
	for (j = 0; j < i; j++) {
	    if ((asNumber == asp->as_number[j]) || (asNumber == bnp->bgp_asout)) {
		error = BGPUPDERR_LOOP;
		goto Error;
	    }
	}

	/* Looks OK, store this AS in the path structure */
	asp->as_number[i] = asNumber;
    }
    if (asDirection == asDirEgp) {
	asp->as_origin = ASO_EGP;
    } else if (asDirection == asDirIncomplete) {
	asp->as_origin = ASO_INCOMPLETE;
    } else {
	asp->as_origin = ASO_IGP;
    }

    /* Two phase rule violation check goes here */

    /* Lookup this path, if duplicate return pointer to old path */
    rtd = rtd_insert(rtd, &as_base);
    asp = (as_path *) rtd->rtd_data;

    /* Parse the networks */
    PickUp(cp, netCount);
    netCount = ntohs(netCount);
    if (netCount == 0) {
	error = BGPUPDERR_NETCOUNT;
	goto Error;
    }
    if ((cp + netCount * (sizeof(netMetric) + sizeof(netNumber.sin_addr))) != lp) {
	error = BGPUPDERR_NETCOUNT;
	goto Error;
    }
    for (i = 0; i < netCount; i++) {
	PickUp(cp, netNumber.sin_addr);
	PickUp(cp, netMetric);
	netMetric = ntohs(netMetric);
	metric = netMetric;

	/* Set default value of preference */
	preference = bnp->bgp_preference;

	/* Check if this network is valid from this peer */
	if (!is_valid_in((sockaddr_un *) & netNumber, bnp->bgp_accept, (adv_entry *) 0, (adv_entry *) 0, &preference)) {
	    IF_BGPUPD {
		trace(TR_BGP, 0, "bgp_recv_Update: net %-15A not valid from AS %5d",
		      &netNumber,
		      bnp->bgp_asin);
	    }
	    continue;
	}
	rt = rt_locate_gw(RTS_NETROUTE,
			  (sockaddr_un *) & netNumber,
			  bnp->bgp_task->task_rtproto,
			  &bnp->bgp_gw);
	if (!rt) {
	    /* New route */
	    if (netMetric == bgpMetricInfinity) {
		continue;
	    }
	    rt = rt_add((sockaddr_un *) & netNumber,
			(sockaddr_un *) 0,
			(sockaddr_un *) & gateway,
			&bnp->bgp_gw,
			metric,
			(flag_t) (RTS_NOAGE |
			       ((bnp->bgp_linktype == openLinkInternal) ?
				(RTS_INTERIOR | RTS_NOTINSTALL | RTS_NOADVISE) : (RTS_EXTERIOR))),
			bnp->bgp_task->task_rtproto,
			bnp->bgp_asin,
			rt_maxage,
			preference);
	    if (rt) {
		rt->rt_data = rtd;
		rtd->rtd_refcount++;
	    } else {
		error = BGPUPDERR_NETWORK;
		goto Error;
	    }
	} else {
	    /* Existing route */
	    if (netMetric == bgpMetricInfinity) {
		/* Route is to be deleted */
		(void) rt_delete(rt);
	    } else {
		/* Change metric/gateway */
		if (rt_change(rt,
			      (sockaddr_un *) & gateway,
			      metric,
			      rt_maxage,
			      preference)) {
		    rtd_unlink(rt->rt_data);
		    rt->rt_data = rtd;
		    rtd->rtd_refcount++;
		}
	    }
	}
    }

  Error:
    /* Send error and close connection if necessary */
    if (error) {
	bgp_send_NotifyUpdate(bnp, error, PDU, length);
    }
    /* Free the as_path pointer if necessary */
    if (rtd) {
	rtd_unlink(rtd);
    }
    i = netCount;
    if (!(bnp->bgp_options & BGPO_NOGENDEFAULT) && !(bnp->bgp_flags & BGPF_GENDEFAULT) && i) {
	if (rt_default_add()) {
	    bnp->bgp_flags |= BGPF_GENDEFAULT;
	}
    }
    if (rt_close(bnp->bgp_task, &bnp->bgp_gw, i)) {
	task_flash(bnp->bgp_task);
    }
    return;
}


struct update_packet {
    struct update_packet *up_next;	/* Pointer to next packet on chain */
    struct in_addr up_gateway;		/* Gateway */
    as_path *up_asp;			/* Pointer to AS path */
    u_char *up_ncp;			/* Pointer to netCount field */
    u_char *up_np;			/* Network/Metric fill field */
    u_short up_net_count;		/* Number of networks in this update */
};

void
bgp_send_update_packet(bnp, packet)
bgpPeer *bnp;
struct update_packet *packet;
{
    int length;
    u_short uns_short;
    bgpPdu *PDU;
    u_char *cp;

    cp = (u_char *) packet + sizeof(struct update_packet);	/* Point to PDU */
    PDU = (bgpPdu *) cp;
    length = packet->up_np - cp - sizeof(pduHeader);	/* Calculate length */

    /* Set packet type */
    PDU->header.type = bgpPduUpdate;

    /* Add Network Count field to packet */
    uns_short = htons(packet->up_net_count);
    packet->up_net_count = 0;
    packet->up_np = packet->up_ncp;
    PutDown(packet->up_np, uns_short);

    bgp_send(bnp, PDU, length);

}


void
bgp_send_update(bnp, flash_flag)
bgpPeer *bnp;
int flash_flag;				/* Flash update */
{
    int i;
    u_short uns_short;
    u_short net_Metric;
    metric_t metric;
    u_char *cp;
    u_char uns_char;
    as_path *asp;
    struct update_packet *up, *upn;
    struct update_packet *up_base = NULL;
    struct sockaddr_in *gateway;
    rt_entry *rt;

    trace(TR_TASK, 0, "bgp_send_update: sending updates to %s",
	  bnp->bgp_name);

    RT_TABLE(rt) {
	metric = bgp_default_metric;
	if (rt->rt_state & (RTS_HOSTROUTE | RTS_SUBNET | RTS_NOADVISE)) {
	    /* Subnets and hostroutes not allowed - also catch routes not to be announced */
	    continue;
	}
	if (flash_flag && (bnp->bgp_task->task_rtrevision >= rt->rt_revision)) {
	    /* Processing a flash update and this route has not been modified */
	    continue;
	}
	if (!propagate(rt,
		       (proto_t) 0,
		       bnp->bgp_propagate,
		       (adv_entry *) 0,
		       (adv_entry *) 0,
		       &metric)) {
	    continue;
	}
	/* Get pointer to an asp path for this route */
	switch (rt->rt_proto) {
	    case RTPROTO_BGP:
		asp = (as_path *) rt->rt_data->rtd_data;
		if (!asp->as_count && (bnp->bgp_linktype == openLinkInternal)) {
		    /* Don't propogate internal routes over Internal links */
		    continue;
		}
		break;

	    case RTPROTO_EGP:
		asp = bgp_as_build(1, ASO_EGP, rt->rt_as);
		break;
		
	    default:
		if (bnp->bgp_linktype == openLinkInternal) {
		  /* Do not propagate interior routes over Internal links */
		  continue;
		}
		
		asp = bgp_as_build(0, ASO_IGP, 0);
		break;
	}

	/* Check for split horizon */
	for (i = 0; i < asp->as_count; i++) {
	    if (asp->as_number[i] == bnp->bgp_asin) {
		/* Don't send route back to this peer, it would cause a loop */
		break;
	    }
	}
	if (i != asp->as_count) {
	    continue;
	}
	/* Determine gateway for this path */
	if (gd_inet_wholenetof(rt->rt_router.in.sin_addr) !=gd_inet_wholenetof(bnp->bgp_interface->int_addr.in.sin_addr)) {
	    /* Gateway is my address */
	    gateway = &bnp->bgp_interface->int_addr.in;
	} else {
	    /* Gateway is next hop */
	    gateway = &rt->rt_router.in;
	}

	/* Locate or allocate a packet for this path */
	for (up = up_base; up; up = up->up_next) {
	    if ((asp == up->up_asp) && equal_in(gateway->sin_addr, up->up_gateway)) {
		/* Send packet if already full */
		if ((up->up_np - (u_char *) up - sizeof(struct update_packet) + 6 /* XXX */ ) > BGPMAXPACKETSIZE) {
		    bgp_send_update_packet(bnp, up);
		}
		break;
	    }
	}
	if (!up) {
	    up = (struct update_packet *) calloc(1, BGPMAXPACKETSIZE + sizeof(struct update_packet));
	    if (!up) {
		trace(TR_ALL, LOG_ERR, "bgp_send_update: calloc %m");
		quit(errno);
	    }
	    /* Add to chain */
	    up->up_next = up_base;
	    up_base = up;
	    /* Save gateway and as_path pointers */
	    up->up_gateway = gateway->sin_addr;
	    up->up_asp = asp;
	    /* Point to packet */
	    cp = (u_char *) up + sizeof(struct update_packet) + sizeof(pduHeader);
	    /* Add gateway */
	    PutDown(cp, gateway->sin_addr);
	    /* AS path */
	    if (bnp->bgp_linktype == openLinkInternal) {
		/* Don't prepend my AS and link type if Internal */
		uns_char = asp->as_count;
		PutDown(cp, uns_char);	/* Save AS count */
	    } else {
		/* Prepend my AS and link type if not Internal */
		uns_char = asp->as_count + 1;
		PutDown(cp, uns_char);	/* Save AS count */
		uns_char = bnp->bgp_linktype;
		PutDown(cp, uns_char);	/* Put down direction */
		uns_short = htons(bnp->bgp_asout);
		PutDown(cp, uns_short);	/* Put down AS number */
	    }
	    /* Add AS path */
	    for (i = 0; i < asp->as_count; i++) {
		uns_char = asDirHorizontal;
		PutDown(cp, uns_char);
		uns_short = htons(asp->as_number[i]);
		PutDown(cp, uns_short);
	    }

	    up->up_ncp = cp;		/* Save pointer to network count */
	    up->up_np = cp + sizeof(u_short);	/* Save pointer to where to save network */

	    /* XXX - The following is for compatibility with BGP */
	    /* version 1 and will hopefully be removed soon */
	    cp -= sizeof(as_t) + sizeof(uns_char);
	    if (asp->as_origin == ASO_IGP) {
		uns_char = asDirHorizontal;
	    } else if (asp->as_origin == ASO_EGP) {
		uns_char = asDirEgp;
	    } else {
		uns_char = asDirIncomplete;
	    }
	    PutDown(cp, uns_char);
	    /* XXX - end of compatibility*/

	}
	up->up_net_count++;
	/* Add networks and metrics */

	/* Store the network */
	PutDown(up->up_np, rt->rt_dest.in.sin_addr);

	/* Metric is already set.  Check here for metricout or unreachable */
	if (!(rt->rt_ifp->int_state & IFS_UP) || (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE))) {
	    metric = bgpMetricInfinity;
	} else if (bnp->bgp_options & BGPO_METRICOUT) {
	    metric = bnp->bgp_metricout;
	}
	net_Metric = metric;
	PutDown(up->up_np, net_Metric);

    } RT_TABLEEND;

    /* Send and free any packets */
    for (up = up_base; up; up = upn) {
	upn = up->up_next;
	bgp_send_update_packet(bnp, up);
	(void) free((caddr_t) up);
    }

    bnp->bgp_task->task_rtrevision = rt_revision;
}

#endif				/* PROTO_BGP */
