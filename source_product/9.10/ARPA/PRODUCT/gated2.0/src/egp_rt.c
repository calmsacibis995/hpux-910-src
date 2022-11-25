/*
 *  $Header: egp_rt.c,v 1.1.109.5 92/02/28 14:02:06 ash Exp $
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
 * rt_egp.c
 *
 * EGP route update and processing and preparation functions.
 *
 * Functions: egp_rt_send, egp_rt_recv
 */

#include "include.h"
#include "egp.h"

#ifdef	PROTO_EGP

/*
 * egp_rt_send() prepares the network part of the EGP Network Reachability
 * update message with respect to the shared network of the EGP peer.
 * This only includes the networks in the interior routing table (direct
 * networks, and remote networks of non-routing gateways of this autonomous
 * system) other than the net shared by the EGP peer. If the user has
 * specified that only certain networks are allowed to be advised all others
 * are excluded from outgoing NR update messages.
 * If the interior routing table includes other interior gateways on the
 * network shared with the EGP peer (i.e. indirect neighbors), they are
 * included in updates as the appropriate first hop to their attached
 * networks.
 * This function checks the status of routes and if down sets the
 * distance as unreachable.
 *
 * Returns the length of the EGP NR packet in octets or ERROR if an error
 * has occurred.
 */

int
egp_rt_send(nrpkt, ngp)
struct egpnr *nrpkt;			/* start of NR message */
struct egpngh *ngp;			/* Pointer to entry in neighbor table */
{
    rt_entry *rt;
    int n_bytes;
    struct in_addr current_gw;
    register u_char *nrp;		/* next octet of NR message */
    u_char *n_distance = NULL, *distance = NULL, *n_nets = NULL;
    metric_t this_metric = 0;
    u_long current_net;
    struct sockaddr_in addr;
    struct net_order {			/* temporary linked list for ordering nets */
	struct net_order *next;		/* Next entry */
	struct in_addr net;		/* Network */
	struct in_addr gateway;		/* Gateway */
	u_char exterior;		/* True if this is an exterior gateway */
	u_char distance;		/* Distance to advertize */
    } *start_net, *free_net, *last_net;
    register struct net_order *net_pt, *this_net;	/* current search point */
    rt_entry *pollnet_rt;

    if (!(pollnet_rt = rt_locate(RTS_INTERIOR, (sockaddr_un *) & ngp->ng_paddr, RTPROTO_DIRECT))) {
	trace(TR_INT, LOG_ERR, "egp_rt_send: no route to polled net, %A wanted",
	      &ngp->ng_paddr);
	return (ERROR);
    }
    sockclear_in(&addr);

    /*
     * Reorder the interior routes as required for the NR message with respect to
     * the given shared net. Uses a temporary linked list terminated by NULL
     * pointer. The first element of the list is a dummy so insertions can be done
     * before the first true entry. The route status is checked and if down the
     * distance is set as unreachable.  The required order groups nets by gateway
     * and in order of increasing metric. This gateway is listed first (with all
     * nets not reached by gateways on the shared net) and then neighbor gateways
     * on the shared net, in any order. As there are few nets to be reported by a
     * stub gateway, each route is copied from the interior routing table and
     * inserted in the temporary reordered list using a linear search.
     *
     * Use the total number of networks to be sure we allocate a large enough
     * buffer.
     */

    /* XXX - need an indication of whether this is an interior or exterior route, list the interior ones first and set */
    /* XXX - the number of interior and exterior gateways correctly. */

    start_net = (struct net_order *) malloc((unsigned) (rt_net_routes + 1) * sizeof (struct net_order));
    if (start_net == NULL) {
	trace(TR_ALL, LOG_ERR, "egp_rt_send: malloc: %m");
	return (ERROR);
    }
    last_net = start_net + rt_net_routes + 1;
    start_net->next = NULL;

    /*
     * ensures first gateway listed is self
     */
    start_net->gateway = pollnet_rt->rt_router.in.sin_addr;	/* struct copy */

    start_net->exterior = 0;
    free_net = start_net + 1;		/* first element dummy to ease insertion code */

    /*
     * check all interior routes of route table
     */
    RT_TABLE(rt) {
	if (rt->rt_state & (RTS_HOSTROUTE | RTS_SUBNET | RTS_NOADVISE)) {
	    /* Don't send subnets or networks not to be announces */
	    continue;
	}
	IF_EGPUPD tracef("EGP UPDATE\tnet %-15A AS %5d - ",
			 &rt->rt_dest,
			  rt->rt_as);

	/*
         *  Don't allow the DEFAULT net through, unless we are allowed to
         *  send DEFAULT.
         */
	if ((rt->rt_dest.in.sin_addr.s_addr == DEFAULTNET) &&
	    !(ngp->ng_options & NGO_DEFAULTOUT)) {
	    IF_EGPUPD trace(TR_EGP | TR_NOSTAMP, 0, "not propogating default.");

	    continue;
	}
	/*
         * ignore nets that are not Class A, B or C
         */
	current_net = rt->rt_dest.in.sin_addr.s_addr;

	if (gd_inet_class((u_char *) & current_net) == 0) {
	    IF_EGPUPD trace(TR_EGP | TR_NOSTAMP, 0, "not Class A, B or C");

	    trace(0, LOG_ERR, "egp_rt_send: net not class A, B or C: %A",
		  &rt->rt_dest);
	    continue;
	}
	this_metric = egp_default_metric;

	if (!propagate(rt,
		       (proto_t) 0,
		       ngp->ng_propagate,
		       (adv_entry *) 0,
		       (adv_entry *) 0,
		       &this_metric)) {
	    IF_EGPUPD trace(TR_EGP | TR_NOSTAMP, 0, "restrictions prohibit announcement");

	    continue;
	}
	/* Check for metric of infinity and metricout */
	if ((!rt->rt_ifp->int_state & IFS_UP) || (rt->rt_state & (RTS_HOLDDOWN | RTS_DELETE))) {
	    this_metric = EGP_INFINITY;
	} else if (ngp->ng_options & NGO_METRICOUT) {
	    this_metric = ngp->ng_metricout;
	}
	/*
         * committed to advertising net
         */
	if ((this_net = free_net++) >= last_net) {
	    trace(TR_ALL, LOG_ERR, "egp_rt_send: rt_net_routes too low, overflowed allocated network list");
	    return (ERROR);
	}
	this_net->net = rt->rt_dest.in.sin_addr;	/* struct copy */

	this_net->distance = this_metric;
	this_net->exterior = 0;
	/*
         * assign gw on shared net
         */
	if (gd_inet_wholenetof(rt->rt_router.in.sin_addr) !=gd_inet_wholenetof(ngp->ng_paddr.sin_addr)) {
	    this_net->gateway = pollnet_rt->rt_router.in.sin_addr;
	} else {
	    /* gw is neighbor */
	    this_net->gateway = rt->rt_router.in.sin_addr;

	    if (rt->rt_as && (rt->rt_as != my_system)) {
		this_net->exterior++;
	    }
	}
	/*
         * If this is the DEFAULT net, set the specified metric, we are
         * the gateway.
         */
	if (this_net->net.s_addr == DEFAULTNET) {
	    this_net->gateway = pollnet_rt->rt_router.in.sin_addr;

	    this_net->distance = ngp->ng_defaultmetric;
	    IF_EGPUPD tracef(" DEFAULT - ");
	}
	IF_EGPUPD tracef("metric %3d ", this_net->distance);

	/*
         * insert net in ordered list
         */
	for (net_pt = start_net; net_pt->next; net_pt = net_pt->next) {
	    if (this_net->exterior) {
		if (!net_pt->next->exterior) {
		    continue;
		}
	    } else {
		if (net_pt->next->exterior) {
		    break;
		}
	    }
	    if (equal_in(this_net->gateway, net_pt->next->gateway)) {
		if (this_net->distance <= net_pt->next->distance)
		    break;
	    } else {
		if (equal_in(this_net->gateway, net_pt->gateway)) {
		    break;
		}
	    }
	}				/* for (all nets to be announced) */
	/*
         * insert this net after search net
         */
	this_net->next = net_pt->next;
	net_pt->next = this_net;
	addr.sin_addr = this_net->gateway;	/* sstruct copy */
	IF_EGPUPD trace(TR_EGP | TR_NOSTAMP, 0, "added to update distance %3d gateway %A%s",
			 this_net->distance,
			&addr,
			 this_net->exterior ? " exterior" : "");
    } RT_TABLEEND;

    IF_EGPUPD trace(TR_EGP, 0, NULL);

    /*
     * copy nets into NR message
     */
    nrpkt->en_igw = 0;			/* init # interior gateways */
    nrpkt->en_egw = 0;			/* init # exterior gateways */
    nrp = (u_char *) (nrpkt + 1);	/* start nets part NR msg */
    current_gw.s_addr = 0;		/* ensure first gateway addr copied */
    for (net_pt = start_net->next; net_pt != NULL; net_pt = net_pt->next) {
	if (!equal_in(net_pt->gateway, current_gw)) {
	    /* new gateway */
	    current_gw = net_pt->gateway;
	    current_net = current_gw.s_addr;
	    n_bytes = 4 - gd_inet_class((u_char *) & current_net);
	    memcpy((char *) nrp, (char *) &current_net + 4 - n_bytes, n_bytes);
	    nrp += n_bytes;
	    if (net_pt->exterior) {
		nrpkt->en_egw++;
	    } else {
		nrpkt->en_igw++;
	    }
	    n_distance = nrp++;
	    *n_distance = 1;
	    distance = nrp++;
	    *distance = net_pt->distance;
	    n_nets = nrp++;
	    *n_nets = 1;
	} else if ((net_pt->distance != *distance) || (*n_nets == 255)) {
	    /* New distance or this distance if ull */
	    (*n_distance)++;
	    distance = nrp++;
	    *distance = net_pt->distance;
	    n_nets = nrp++;
	    *n_nets = 1;
	} else {
	    (*n_nets)++;
	}

	current_net = net_pt->net.s_addr;
	n_bytes = gd_inet_class((u_char *) & current_net);
	memcpy((char *) nrp, (char *) &current_net, n_bytes);
	nrp += n_bytes;
    }					/* end for each net */
    free((char *) start_net);
    return (nrp - (u_char *) nrpkt);	/* length of NR message */
}

/*
 * egp_rt_recv() updates the exterior routing tables on receipt of an NR
 * update message from an EGP neighbor. It first checks for valid NR counts
 * before updating the routing tables.
 *
 * EGP Updates are used to update the exterior routing table if one of the
 * following is satisfied:
 *   - No routing table entry exists for the destination network and the
 *     metric indicates the route is reachable (< 255).
 *   - The advised gateway is the same as the current route.
 *   - The advised distance metric is less than the current metric.
 *   - The current route is older (plus a margin) than the maximum poll
 *     interval for all acquired EGP neighbors. That is, the route was
 *     omitted from the last Update.
 *
 * Returns 1 if there is an error in NR message data, 0 otherwise.
 */

int
egp_rt_recv(ngp, pkt, egplen)
struct egpngh *ngp;			/* pointer to neighbor state table */
struct egppkt *pkt;
int egplen;				/* length EGP NR packet */
{
    register u_char *nrb;
    struct egpnr *nrp = (struct egpnr *) pkt;
    struct sockaddr_in destination, gateway;
    u_char gw[4];			/* gateway internet address */
    int gw_class, net_class, ng, nd, nn, n_gw, n_dist = 0, n_net = 0, checkingNR = TRUE,
     NR_nets = 0;
    pref_t preference;
    u_int state;
    metric_t distance;
    rt_entry *rt;
    as_t pkt_system;
    task *tp = ngp->ng_task;

    sockclear_in(&destination);
    sockclear_in(&gateway);

    /*
     * check class of shared net
     */
    *(u_long *) gw = nrp->en_net.s_addr;/* set net part of gateways */
    if ((gw_class = gd_inet_class((u_char *) & gw[0])) == 0) {
	return (EBADDATA);		/* NR message error */
    }
    pkt_system = htons(pkt->egp_system);

    n_gw = nrp->en_igw + nrp->en_egw;

    /*
     * First check NR message for valid counts, then repeat and update routing
     * tables
     */
  repeat:
    if (!checkingNR) {
	rt_open(tp);
    }
    nrb = (u_char *) nrp + sizeof(struct egpnr);	/* start first gw */

    /* XXX - Need to keep track of interior vs exterior gateways and install routes with interior vs exterior flag set */
    /* XXX - so we don't compare the metric of routes that are not really from the same AS */

    for (ng = 0; ng < n_gw; ng++) {	/* all gateways */
	switch (gw_class) {		/* fill gateway local address */
	    case CLAA:
		gw[1] = *nrb++;
	    case CLAB:
		gw[2] = *nrb++;
	    case CLAC:
		gw[3] = *nrb++;
	}
	gateway.sin_addr.s_addr = (ngp->ng_options & NGO_GATEWAY) ? ngp->ng_gateway.sin_addr.s_addr : *(u_long *) gw;
	n_dist = *nrb++;

	for (nd = 0; nd < n_dist; nd++) {	/* all distances this gateway */
	    distance = (u_short) (*nrb++);

	    n_net = *nrb++;
	    if (!checkingNR) {
		NR_nets += n_net;
	    }
	    for (nn = 0; nn < n_net; nn++) {	/* all nets this distance */
		preference = ngp->ng_preference;
		if ((net_class = gd_inet_class(nrb)) == 0) {
		    net_class = 3;
		}
		destination.sin_addr.s_addr = 0;	/* zero unused bytes*/
		memcpy((char *) &destination.sin_addr.s_addr, (char *) nrb, net_class);
		nrb += net_class;
		if (!gd_inet_class((u_char *) & destination.sin_addr.s_addr)) {
		    if (checkingNR) {
			trace(TR_EXT, LOG_ERR, "egp_rt_recv: net %A not class A, B or C from %s via %A",
			      &destination,
			      ngp->ng_name,
			      &gateway);
		    }
#ifdef	notdef
		    return (EBADDATA);	/* Ignore complete NR packet */
#else				/* notdef */
		    continue;		/* Ignore only this route */
#endif				/* notdef */
		}
		if ((destination.sin_addr.s_addr == DEFAULTNET) && !(ngp->ng_options & NGO_DEFAULTIN)) {
		    if (checkingNR) {
			trace(TR_EXT, LOG_WARNING, "egp_rt_recv: ignoring net %A from %s via %A",
			      &destination,
			      ngp->ng_name,
			      &gateway);
		    }
		    continue;
		}
		if (checkingNR) {	/* first check counts only */
		    if (nrb > (u_char *) nrp + egplen + 1)
			return (EBADDATA);	/* erroneous counts in NR */
		} else {		/* update routing table */
		    if (equal_in(gateway.sin_addr, ngp->ng_interface->int_addr.in.sin_addr)) {
			continue;
		    }
		    /* Check of this network is valid from this AS */
		    if (!is_valid_in((sockaddr_un *) & destination,
				     ngp->ng_accept,
				     (adv_entry *) 0,
				     (adv_entry *) 0,
				     &preference)) {
			if (trace_flags & TR_EGP) {
			    trace(TR_EGP, 0, "egp_rt_recv: net %-15A not valid from AS %5d",
				  &destination,
				  pkt_system);
			}
			continue;
		    }
		    /*
	             * check for an existing route
	             */
		    state = 0;
		    rt = rt_locate_gw(RTS_EXTERIOR, (sockaddr_un *) & destination, RTPROTO_EGP, &ngp->ng_gw);
		    if (!rt) {		/* new route */
			if (distance >= EGP_INFINITY) {
			    continue;
			}
			if (rt = rt_add((sockaddr_un *) & destination,
					(sockaddr_un *) 0,
					(sockaddr_un *) & gateway,
					&ngp->ng_gw,
					distance,
					RTS_EXTERIOR | state,
					RTPROTO_EGP,
					ngp->ng_asin,
					ngp->ng_rtage,
					preference)) {
			}
		    } else {		/* existing route */
			if (equal(&rt->rt_router, &gateway) && (rt->rt_as == ngp->ng_asin)) {	/* same gw */
			    if (distance < EGP_INFINITY) {
				if (distance < rt->rt_metric) {
				    if (rt_change(rt,
					       (sockaddr_un *) & gateway,
						  distance,
						  ngp->ng_rtage,
						  preference)) {
				    }
				}
				rt_refresh(rt);
			    } else {
				(void) rt_delete(rt);
			    }
			} else {	/* different gateway */
			    if (rt->rt_as == ngp->ng_asin) {
				/* Same Autonotmous system */
				if (distance < rt->rt_metric) {
				    if (rt_change(rt,
					       (sockaddr_un *) & gateway,
						  distance,
						  ngp->ng_rtage,
						  preference)) {
				    }
				}
			    } else if ((distance < rt->rt_metric) ||
				       ((distance >= rt->rt_metric) && (rt->rt_timer > (ngp->ng_T2 * (EGP_N_POLLAGE - 1))) &&
					!(rt->rt_state & RTS_CHANGED | RTS_REFRESH))) {
				/* Override route if metric is better, or this one has almost expired (and has not been refreshed) */
				if (rt_change(rt,
					      (sockaddr_un *) & gateway,
					      distance,
					      ngp->ng_rtage,
					      preference)) {
				}
			    }
			}
		    }			/* end else existing route */
		}			/* end else update routing table */
	    }				/* end for all nets */
	}				/* end for all distances */
    }					/* end for all gateways */
    if (checkingNR) {
	if (nrb > (u_char *) nrp + egplen) {
	    return (EBADDATA);		/* erroneous counts */
	} else {
	    checkingNR = FALSE;
	}
	goto repeat;
    }
    /*
     * Generate default if not prohibited and the NR packet
     * contains more than one route
     */
    if (!(ngp->ng_options & NGO_NOGENDEFAULT) && !(ngp->ng_flags & NGF_GENDEFAULT) && NR_nets) {
	if (rt_default_add()) {
	    ngp->ng_flags |= NGF_GENDEFAULT;
	}
    }
    if (rt_close(tp, &ngp->ng_gw, NR_nets)) {
	task_flash(tp);
    }
    return (NOERROR);
}

#endif				/* PROTO_EGP */
