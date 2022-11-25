/*
 *  $Header: egp_init.c,v 1.1.109.5 92/02/28 14:01:53 ash Exp $
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
#include "egp.h"

#ifdef	PROTO_EGP

int egp_neighbors;
struct egpngh *egp_neighbor_head;

#if	defined(AGENT_SNMP)
struct egpngh **egp_sort;		/* Sorted list of pointers to neighbors */

#endif				/* defined(AGENT_SNMP) */
u_short egprid_h;
int egp_pktsize;			/* EGP max packet size */
pref_t egp_preference;			/* Preference for EGP routes */
metric_t egp_default_metric;
int doing_egp = FALSE;
adv_entry *egp_accept_list = NULL;	/* List of EGP advise entries */
adv_entry *egp_propagate_list = NULL;	/* List of EGP propagate entries */

u_int egp_reachability[1 << REACH_RATIO];

struct egpstats_t egp_stats =
{0, 0, 0, 0};

bits egp_states[] =
{
    {NGS_IDLE, "Idle"},
    {NGS_ACQUISITION, "Acquisition"},
    {NGS_DOWN, "Down"},
    {NGS_UP, "Up"},
    {NGS_CEASE, "Cease"},
    {-1}
};

bits egp_flags[] =
{
    {NGF_SENT_UNSOL, "SentUnsol"},
    {NGF_SENT_POLL, "SentPoll"},
    {NGF_SENT_REPOLL, "SentRepoll"},
    {NGF_RECV_REPOLL, "RecvRepoll"},
    {NGF_RECV_UNSOL, "RecvUnsol"},
    {NGF_PROC_POLL, "ProcPoll"},
    {NGF_DELETE, "Delete"},
    {NGF_WAIT, "Wait"},
    {NGF_GENDEFAULT, "GenDefault"},
    {0, 0}
};

bits egp_options[] =
{
    {NGO_METRICOUT, "MetricOut"},
    {NGO_ASIN, "AsIn"},
    {NGO_ASOUT, "AsOut"},
    {NGO_NOGENDEFAULT, "NoGenDefault"},
    {NGO_DEFAULTIN, "AcceptDefault"},
    {NGO_DEFAULTOUT, "PropagateDefault"},
    {NGO_INTERFACE, "Interface"},
    {NGO_SADDR, "Saddr"},
    {NGO_GATEWAY, "Gateway"},
    {NGO_MAXACQUIRE, "MaxAcquire"},
    {NGO_VERSION, "Version"},
    {NGO_P1, "P1"},
    {NGO_P2, "P2"},
    {0, 0}
};

const char *egp_acq_codes[5] =
{
    "Request",
    "Confirm",
    "Refuse",
    "Cease",
    "CeaseAck"};

const char *egp_reach_codes[2] =
{
    "Hello",
    "I-H-U"};

const char *egp_nr_status[3] =
{
    "Indeterminate",
    "Up",
    "Down"};

const char *egp_acq_status[8] =
{
    "Unspecified",
    "ActiveMode",
    "PassiveMode",
    "Insufficient Resources",
    "Prohibited",
    "Going Down",
    "Parameter Problem",
    "Protocol Violation"};

const char *egp_reasons[7] =
{
    "Unspecified",
    "Bad EGP header format",
    "Bad EGP data field format",
    "Reachability info unavailable",
    "Excessive polling rate",
    "No response",
    "Unsupported version"};


#if	defined(AGENT_SNMP)
/*
 *	Routine to compare to routine table entries, used by egp_init
 */
int
egp_sort_compare(ngp1, ngp2)
struct egpngh **ngp1, **ngp2;
{
    u_long dst1 = ntohl((*ngp1)->ng_addr.sin_addr.s_addr);
    u_long dst2 = ntohl((*ngp2)->ng_addr.sin_addr.s_addr);
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


void
egp_sort_neighbors()
{
    int i;
    struct egpngh *ngp;

    /* Build a sorted list of neighbors for network monitoring */
    if (egp_sort) {
	(void) free((caddr_t) egp_sort);
    }
    if (egp_neighbors) {
	egp_sort = (struct egpngh **) calloc((u_int) (egp_neighbors + 1), sizeof(struct egpngh *));
	if (!egp_sort) {
	    trace(TR_ALL, LOG_ERR, "egp_sort_neighbors: calloc: %m");
	    quit(errno);
	}
	i = 0;
	EGP_LIST(ngp) {
	    egp_sort[i++] = ngp;
	} EGP_LISTEND;
	qsort((caddr_t) egp_sort, egp_neighbors, sizeof(*egp_sort), egp_sort_compare);
    } else {
	egp_sort = (struct egpngh **) 0;
    }
}

#endif				/* defined(AGENT_SNMP) */


/*
 *	Initialize the reachability structure
 */
void
egp_init_reachability()
{
    int reach, mask, bit, n_bits;

    for (reach = 0; reach < 1 << REACH_RATIO; reach++) {
	n_bits = 0;
	for (bit = 0; bit < REACH_RATIO; bit++) {
	    mask = 1 << bit;
	    if (reach & mask) {
		n_bits++;
	    }
	}
	egp_reachability[reach] = n_bits;
    }
}


/*
 *	Terminate signal received for a task
 */
static void
egp_ngp_terminate(tp)
task *tp;
{
    struct egpngh *ngp = (struct egpngh *) tp->task_data;

    ngp->ng_flags |= NGF_DELETE;

    egp_event_stop(ngp, GODOWN);
}


/*
 *	Cleanup before re-parse
 */
static void
egp_ngp_cleanup(tp)
task *tp;
{
    struct egpngh *ngp = (struct egpngh *) tp->task_data;

    ngp->ng_flags |= NGF_DELETE;

    adv_cleanup((int *) 0, (int *) 0, (gw_entry *) 0,
		&ngp->ng_accept, &ngp->ng_propagate,
		(adv_entry ***) 0, (adv_entry ***) 0);
}


/*
 *	Re-init after re-parse
 */
static void
egp_ngp_reinit(tp)
task *tp;
{
    struct egpngh *ngp = (struct egpngh *) tp->task_data;

    if (!doing_egp || (ngp->ng_flags & NGF_DELETE)) {
	ngp->ng_flags |= NGF_DELETE;
	egp_event_stop(ngp, GODOWN);
    } else {
	switch (ngp->ng_state) {
	    case NGS_IDLE:
		egp_event_start(tp);
		break;
	    case NGS_ACQUISITION:
	    case NGS_DOWN:
	    case NGS_UP:
	    case NGS_CEASE:
		break;
	}

	/* Locate our new policy */
	if (ngp->ng_asin) {
	    /* If the AS isn't valid now, the correct policy will be located */
	    /* when the AS becomes valid */
	    ngp->ng_accept = control_exterior_locate(egp_accept_list, ngp->ng_asin);
	    ngp->ng_propagate = control_exterior_locate(egp_propagate_list, ngp->ng_asin);
	}
    }
}


/*
 *	Cleanup main EGP task before reparse
 */
/*ARGSUSED*/
static void
egp_cleanup(tp)
task *tp;
{
    adv_free_list(egp_accept_list);
    egp_accept_list = (adv_entry *) 0;
    adv_free_list(egp_propagate_list);
    egp_propagate_list = (adv_entry *) 0;
}


/*
 *	Initialize EGP socket and task
 */
void
egp_init()
{
    struct egpngh *ngp;
    if_entry *ifp;
    struct sockaddr_in addr;
    static task *egp_task = (task *) 0;

    sockclear_in(&addr);

    if (doing_egp) {
	if (!egp_task) {
	    egp_task = task_alloc("EGP");
	    egp_task->task_cleanup = egp_cleanup;
	    egp_task->task_dump = egp_dump;
	    if (!task_create(egp_task, egp_pktsize)) {
		quit(EINVAL);
	    }
	}
	EGP_LIST(ngp) {
	    if (!ngp->ng_task) {
		/* Check that I have a direct net to neighbor, or if NGO_GATEWAY	*/
		/* is set, that I have a direct net to the specified gateway	*/
		if (!(ngp->ng_options & NGO_INTERFACE)) {
		    if (!(ngp->ng_options & NGO_GATEWAY)) {
			sockcopy(&ngp->ng_addr, &addr);
		    } else {
			addr = ngp->ng_gateway;	/* struct copy */
		    }
		    if (ifp = if_withdst((sockaddr_un *) & addr)) {
			ngp->ng_interface = ifp;
		    } else if (!(ngp->ng_options & NGO_GATEWAY)) {
			/* If we only have one interface, use it */
			ifp = NULL;
			if (n_interfaces == 1) {
			    IF_LIST(ifp) {
				if (!(ifp->int_state & IFS_LOOPBACK)) {
				    ngp->ng_interface = ifp;
				    break;
				}
			    } IF_LISTEND(ifp) ;
			}
			if (!ifp) {
			    trace(TR_INT, LOG_ERR, "egp_init: Can't determine interface for neighbor %s", ngp->ng_name);
			    quit(EDESTADDRREQ);
			}
		    } else {
			trace(TR_INT, LOG_ERR, "egp_init: no direct net to gateway %s", ngp->ng_name);
			quit(EDESTADDRREQ);
		    }
		}
		/* If ASout is not specified, default to global value */
		if (!(ngp->ng_options & NGO_ASOUT)) {
		    ngp->ng_asout = my_system;
		}
		if (ngp->ng_options & NGO_ASIN) {
		    ngp->ng_accept = control_exterior_locate(egp_accept_list, ngp->ng_asin);
		    ngp->ng_propagate = control_exterior_locate(egp_propagate_list, ngp->ng_asin);
		}
		if (!(ngp->ng_options & NGO_SADDR)) {
		    ngp->ng_saddr.sin_addr = gd_inet_makeaddr(gd_inet_wholenetof(ngp->ng_addr.sin_addr), 0, FALSE);
		}
		if (!(ngp->ng_options & NGO_PREFERENCE)) {
		    ngp->ng_preference = egp_preference;
		}
		if (!(ngp->ng_options & NGO_P1)) {
		    ngp->ng_P1 = EGP_P1;
		}
		if (!(ngp->ng_options & NGO_P2)) {
		    ngp->ng_P2 = EGP_P2;
		}
		ngp->ng_state = NGS_IDLE;
		ngp->ng_V = (ngp->ng_options & NGO_VERSION) ? ngp->ng_version : EGPVER;
		ngp->ng_S = 1;
		ngp->ng_T1 = ngp->ng_P1 + HELLOMARGIN;
		ngp->ng_hello_rate.rate_min = ngp->ng_P1;
		ngp->ng_poll_rate.rate_min = ngp->ng_P2;
		ngp->ng_M = ACTIVE;

		ngp->ng_task = task_alloc("EGP");
		ngp->ng_task->task_flags = TASKF_IPHEADER;
		ngp->ng_task->task_proto = IPPROTO_EGP;
		ngp->ng_task->task_rtproto = RTPROTO_EGP;
		sockcopy(&ngp->ng_addr, &ngp->ng_task->task_addr);
		ngp->ng_task->task_recv = egp_recv;
		ngp->ng_task->task_data = (caddr_t) ngp;
		ngp->ng_task->task_terminate = egp_ngp_terminate;
		ngp->ng_task->task_cleanup = egp_ngp_cleanup;
		ngp->ng_task->task_reinit = egp_ngp_reinit;
		/* Allocate a socket for this peer */
		if ((ngp->ng_task->task_socket = task_get_socket(AF_INET, SOCK_RAW, IPPROTO_EGP)) < 0) {
		    quit(errno);
		}
		if (!task_create(ngp->ng_task, egp_pktsize)) {
		    quit(EINVAL);
		}
		/* Set the receive and send buffers so they can hold two max size update packets */
		if (task_set_option(ngp->ng_task, TASKOPTION_RECVBUF, (caddr_t) (egp_pktsize * 2)) < 0) {
		    quit(errno);
		}
		if (task_set_option(ngp->ng_task, TASKOPTION_SENDBUF, (caddr_t) (egp_pktsize * 2)) < 0) {
		    quit(errno);
		}
		if (if_withdst((sockaddr_un *) & ngp->ng_addr) &&
		    task_set_option(ngp->ng_task, TASKOPTION_DONTROUTE, (caddr_t) TRUE) < 0) {
		    quit(errno);
		}
		if (!test_flag) {
		    /* Set the remote address of the socket */
		    if (connect(ngp->ng_task->task_socket, (struct sockaddr *) & ngp->ng_addr, socksize(&ngp->ng_addr)) < 0) {
			trace(TR_ALL, LOG_ERR, "egp_init: connect %A: %m",
			      &addr);
			quit(errno);
		    }
		    /* Set the local address of the socket */
		    if (bind(ngp->ng_task->task_socket,
		       (struct sockaddr *) & ngp->ng_interface->int_addr,
			   socksize(&ngp->ng_interface->int_addr)) < 0) {
			trace(TR_ALL, LOG_ERR, "egp_init: bind %A: %m",
			      &ngp->ng_interface->int_addr);
			quit(errno);
		    }
		}
		(void) timer_create(ngp->ng_task,
				    EGP_TIMER_t1,
				    "t1",
				    0,
				    (time_t) 0,
				    egp_event_t1);

		(void) timer_create(ngp->ng_task,
				    EGP_TIMER_t2,
				    "t2",
				    0,
				    (time_t) 0,
				    egp_event_t2);

		(void) timer_create(ngp->ng_task,
				    EGP_TIMER_t3,
				    "t3",
				    TIMERF_ABSOLUTE,
				    (ngp->ng_flags & NGF_WAIT) ? (time_t) 0 : (time_t) EGP_START_DELAY,
				    egp_event_t3);

	    } EGP_LISTEND;
	}
	egp_init_reachability();
    } else {
	/* Delete any neighbors without tasks */
	EGP_LIST(ngp) {
	    if (!ngp->ng_task) {
		egp_event_delete(ngp);
	    }
	} EGP_LISTEND;

	egp_cleanup((task *) 0);
	if (egp_task) {
	    task_delete(egp_task);
	    egp_task = (task *) 0;
	}
    }
}

#endif				/* PROTO_EGP */
