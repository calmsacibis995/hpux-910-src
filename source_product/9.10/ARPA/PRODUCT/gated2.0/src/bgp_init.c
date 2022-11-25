/*
 *  $Header: bgp_init.c,v 1.1.109.5 92/02/28 14:01:21 ash Exp $
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
#ifndef vax11c
#include <sys/file.h>
#endif				/* vax11c */
#include "bgp.h"

#ifdef	PROTO_BGP

int doing_bgp = FALSE;			/* Is BGP active? */
pref_t bgp_preference = RTPREF_BGP;	/* Preference for BGP routes */
u_short bgp_port;			/* Well known BGP port */
metric_t bgp_default_metric = bgpMetricInfinity;	/* Default metric if no announce clauses specified */
bgpPeer *bgp_peers;			/* Linked list of BGP peers */
int bgp_n_peers;			/* Number of BGP peers */
bgpPdu *bgp_send_buffer;		/* Send buffer */
adv_entry *bgp_accept_list = NULL;	/* List of BGP advise entries */
adv_entry *bgp_propagate_list = NULL;	/* List of BGP propagate entries */

static timer *bgp_flash_timer = (timer *) 0;
static time_t bgp_next_flash = (time_t) 0;

int notifyLengths[BGPERRCD_MAX + 1] =
{
    0,
    BGPERRLEN_LINKTYPE,
    BGPERRLEN_AUTHCODE,
    BGPERRLEN_AUTHFAIL,
    BGPERRLEN_UPDATE,
    BGPERRLEN_SYNC,
    BGPERRLEN_MSGLEN,
    BGPERRLEN_MSGTYPE,
    BGPERRLEN_VERSION,
    BGPERRLEN_OPENAS
};

bits bgpFlags[] =
{
    {BGPF_HEADER, "Header"},
    {BGPF_DELETE, "Delete"},
    {BGPF_WAIT, "Wait"},
    {BGPF_GENDEFAULT, "GenDefault"},
    {0}
};

bits bgpOptions[] =
{
    {BGPO_METRICOUT, "MetricOut"},
    {BGPO_ASIN, "AsIn"},
    {BGPO_ASOUT, "AsOut"},
    {BGPO_NOGENDEFAULT, "NoGenDefault"},
    {BGPO_GATEWAY, "Gateway"},
    {BGPO_PREFERENCE, "Preference"},
    {BGPO_INTERFACE, "Interface"},
    {BGPO_LINKTYPE, "LinkType"},
    {BGPO_HOLDTIME, "HoldTime"},
    {0}
};

bits bgpStates[] =
{
    {0, "Invalid"},
    {BGPSTATE_IDLE, "Idle"},
    {BGPSTATE_ACTIVE, "Active"},
    {BGPSTATE_CONNECT, "Connect"},
    {BGPSTATE_OPENSENT, "OpenSent"},
    {BGPSTATE_OPENCONFIRM, "Confirm"},
    {BGPSTATE_ESTABLISHED, "Established"},
    {0}
};

bits bgpEvents[] =
{
    {0, "Invalid"},
    {BGPEVENT_START, "Start"},
    {BGPEVENT_OPEN, "Open"},
    {BGPEVENT_CLOSED, "Closed"},
    {BGPEVENT_OPENFAIL, "OpenFail"},
    {BGPEVENT_RECVOPEN, "RecvOpen"},
    {BGPEVENT_RECVCONFIRM, "RecvConfirm"},
    {BGPEVENT_RECVKEEPALIVE, "RecvKeepAlive"},
    {BGPEVENT_RECVUPDATE, "RecvUpdate"},
    {BGPEVENT_RECVNOTIFY, "RecvNotify"},
    {BGPEVENT_HOLDTIME, "HoldTime"},
    {BGPEVENT_KEEPALIVE, "KeepAlive"},
    {BGPEVENT_CEASE, "Cease"},
    {BGPEVENT_STOP, "Stop"},
    {0}
};

bits bgpAsDirs[] =
{
    {0, "Invalid"},
    {asDirUp, "Up"},
    {asDirDown, "Down"},
    {asDirHorizontal, "Horizontal"},
    {asDirEgp, "Egp"},
    {asDirIncomplete, "Incomplete"},
    {0}
};

bits bgpOpenType[] =
{
    {openLinkInternal, "Internal"},
    {openLinkUp, "Up"},
    {openLinkDown, "Down"},
    {openLinkHorizontal, "Horizontal"},
    {0}
};

bits bgpPduType[] =
{
    {0, "Invalid"},
    {bgpPduOpen, "Open"},
    {bgpPduUpdate, "Update"},
    {bgpPduNotify, "Notify"},
    {bgpPduKeepAlive, "KeepAlive"},
    {bgpPduOpenConfirm, "OpenConfirm"},
    {0}
};


bits bgpErrors[] =
{
    {0, NULL},
    {BGPERRCD_LINKTYPE, "link type error in open"},
    {BGPERRCD_AUTHCODE, "unknown authentication code"},
    {BGPERRCD_AUTHFAIL, "authentication failure"},
    {BGPERRCD_UPDATE, "update error"},
    {BGPERRCD_SYNC, "connection out of sync"},
    {BGPERRCD_MSGLEN, "invalid message length"},
    {BGPERRCD_MSGTYPE, "invalid message type"},
    {BGPERRCD_VERSION, "invalid version number"},
    {BGPERRCD_OPENAS, "invalid AS field in Open"},
    {BGPERRCD_CEASE, "BGP neighbor Cease"},
};


bits bgpUpdateErrors[] =
{
    {0, NULL},
    {BGPUPDERR_ASCOUNT, "Invalid AS count"},
    {BGPUPDERR_DIRECTION, "Invalid Direction code"},
    {BGPUPDERR_AS, "Invalid AS"},
    {BGPUPDERR_ORDER, "Terminal Direction in middle of path"},
    {BGPUPDERR_LOOP, "Routing loop detected"},
    {BGPUPDERR_GATEWAY, "Invalid Gateway"},
    {BGPUPDERR_NETCOUNT, "Invalid Network count"},
    {BGPUPDERR_NETWORK, "Invalid Network"}
};


/* We have a socket, create the task */
void
bgp_session_init(bnp, bgp_socket)
bgpPeer *bnp;
int bgp_socket;
{
    struct sockaddr_in addr;
    int addrlen = socksize(&addr);

    if (getpeername(bgp_socket, (struct sockaddr *) & addr, &addrlen) < 0) {
	trace(TR_ALL, LOG_ERR, "bgp_session_init: getpeername(%d): %m",
	      bgp_socket);
    } else {
	IF_BGPPROTO trace(TR_BGP, 0, "bgp_session_init: peer %s socket %d is connected to %A",
			   bnp->bgp_name,
			   bgp_socket,
			  &addr);
    }

    bnp->bgp_task->task_recv = bgp_read;
    task_set_socket(bnp->bgp_task, bgp_socket);

    if (task_set_option(bnp->bgp_task, TASKOPTION_NONBLOCKING, (caddr_t) TRUE) < 0) {
	quit(errno);
    }
    if (task_set_option(bnp->bgp_task, TASKOPTION_RECVBUF, (caddr_t) (32 * 1024)) < 0) {
	quit(errno);
    }
    if (task_set_option(bnp->bgp_task, TASKOPTION_SENDBUF, (caddr_t) (32 * 1024)) < 0) {
	quit(errno);
    }
    if (if_withdst((sockaddr_un *) & bnp->bgp_addr) && task_set_option(bnp->bgp_task, TASKOPTION_DONTROUTE, (caddr_t) TRUE) < 0) {
	quit(errno);
    }
    (void) timer_create(bnp->bgp_task, BGPTIMER_KEEPALIVE, "KeepAlive", 0, (time_t) 0, bgp_event_KeepAlive);
}


/* Close the socket and delete the task */
void
bgp_session_finit(bnp, quick)
bgpPeer *bnp;
int quick;
{

    IF_BGPPROTO trace(TR_BGP, 0, "bgp_session_finit: peer %s",
		       bnp->bgp_name);

    if (!quick) {
	if (task_set_option(bnp->bgp_task, TASKOPTION_LINGER, (caddr_t) BGP_CLOSE_TIMER) < 0) {
	    quit(errno);
	}
    }
    if (close(bnp->bgp_task->task_socket) < 0) {
	trace(TR_ALL, LOG_ERR, "bgp_session_finit: close: %m");
    }
    task_reset_socket(bnp->bgp_task);
    timer_delete(bnp->bgp_task->task_timer[BGPTIMER_KEEPALIVE]);

}


/*
 *	Clean up when connect has completed
 */
void
bgp_connect_finit(bnp)
bgpPeer *bnp;
{
    task_reset_socket(bnp->bgp_task);

    timer_delete(bnp->bgp_task->task_timer[BGPTIMER_CONNECT]);
}


/*
 *	Connect has completed or failed
 */
void
bgp_connect_complete(tp)
task *tp;
{
    int length;
    int bgp_socket = tp->task_socket;
    struct sockaddr_in addr;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    tracef("bgp_connect_complete: peer %s state %s ",
	   bnp->bgp_name,
	   trace_state(bgpStates, bnp->bgp_state));

    length = socksize(&addr);
    if (getpeername(tp->task_socket, (struct sockaddr *) & addr, &length) < 0) {
	trace(TR_EXT, 0, "Connection error: %m");
	if (close(bnp->bgp_task->task_socket) < 0) {
	    trace(TR_ALL, LOG_ERR, "bgp_connect_complete: close(%d): %m",
		  bnp->bgp_task->task_socket);
	    quit(errno);
	}
	bgp_connect_finit(bnp);
	bgp_event_OpenFail(bnp);
    } else {
	trace(TR_BGP, 0, "Connection established with %A",
	      &addr);
	bgp_connect_finit(bnp);
	bgp_session_init(bnp, bgp_socket);
	bgp_event_Open(bnp);
    }

}


void
bgp_connect_start(bnp)
bgpPeer *bnp;
{
    int bgp_socket;

    struct sockaddr_in addr;


    IF_BGPPROTO trace(TR_BGP, 0, "bgp_connect_start: peer %s",
		       bnp->bgp_name);

    if ((bgp_socket = task_get_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	quit(errno);
    }
    bnp->bgp_task->task_flags |= TASKF_CONNECT;
    bnp->bgp_task->task_connect = bgp_connect_complete;
    task_set_socket(bnp->bgp_task, bgp_socket);

    if (task_set_option(bnp->bgp_task, TASKOPTION_NONBLOCKING, (caddr_t) TRUE) < 0) {
	quit(errno);
    }
    if (task_set_option(bnp->bgp_task, TASKOPTION_REUSEADDR, (caddr_t) TRUE) < 0) {
	quit(errno);
    }
    /* There are problems with the linger_close if you bind the local */
    /* address */
    addr = bnp->bgp_interface->int_addr.in;	/* struct copy */
#ifdef	notdef
    addr.sin_port = bgp_port;
#else
    addr.sin_port = 0;		/* leave port wildcarded, should work okay */
#endif				/* notdef */

    if (bind(bgp_socket, &addr, socksize(&addr)) < 0) {
	trace(TR_ALL, LOG_ERR, "bgp_connect_start: bind %A: %m",
	      &addr);
	quit(errno);
    }

    if (connect(bgp_socket, (struct sockaddr *) & bnp->bgp_addr, socksize(&bnp->bgp_addr)) < 0) {
	int log_level = LOG_WARNING;
	int trace_level = TR_BGP | TR_TASK;

	switch (errno) {
	    case EINPROGRESS:
		log_level = 0;
		if (!(trace_flags & TR_PROTOCOL)) {
		    trace_level = 0;
		}
		/* Fall Thru */

	    case ENETDOWN:
	    case ENETUNREACH:
	    case EHOSTDOWN:
	    case EHOSTUNREACH:
	    case EADDRINUSE:
		trace(trace_level, log_level, "bgp_connect_start: connect %A: %m",
		      &bnp->bgp_addr);

		timer_set(bnp->bgp_task->task_timer[BGPTIMER_CONNECT], (time_t) BGP_WAIT_RETRY);
		break;

	    default:
		trace(TR_ALL, LOG_ERR, "bgp_connect_start: connect %A: %m",
		      &bnp->bgp_addr);
		quit(errno);
	}
    } else {
	IF_BGPPROTO trace(TR_BGP, 0, "bgp_connect_start: connect %A succeded",
			  &bnp->bgp_addr);

	bgp_connect_complete(bnp->bgp_task);
    }
}


/*
 *	Close the current socket and start a new connect by calling bgp_connect_start()
 */
/*ARGSUSED*/
void
bgp_connect_job(tip, interval)
timer *tip;
time_t interval;
{
    task *tp = tip->timer_task;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    IF_BGPPROTO trace(TR_BGP, 0, "bgp_connect_job: %s",
		       timer_name(tip));

    if (close(tp->task_socket) < 0) {
	trace(TR_ALL, LOG_ERR, "bgp_connect_job: close(%d): %m",
	      tp->task_socket);
	quit(errno);
    }
    task_reset_socket(tp);

    bgp_connect_start(bnp);
}


/*
 *	Initialize for connecting to peer.
 */
void
bgp_connect_init(bnp)
bgpPeer *bnp;
{

    (void) timer_create(bnp->bgp_task, BGPTIMER_CONNECT, "Connect", 0, (time_t) 0, bgp_connect_job);

    bgp_connect_start(bnp);
}


/*ARGSUSED*/
void
bgp_do_flash(tp, interval)
task *tp;
time_t interval;
{
    bgpPeer *bnp;

    trace(TR_TASK, 0, "bgp_do_flash: Doing flash update for BGP");

    BGP_LIST(bnp) {
	if (bnp->bgp_state == BGPSTATE_ESTABLISHED) {
	    bgp_send_update(bnp, TRUE);
	}
    } BGP_LISTEND;

    bgp_flash_timer = (timer *) 0;
    bgp_next_flash = (time_t) BGP_FLASH_INTERVAL + time_sec;
    trace(TR_TASK, 0, "bgp_do_flash: Flash update done, none before %T", bgp_next_flash);
}


/*
 *	Check to see if a flash update packet is allowed and send or schedule it
 */
void
bgp_flash(tp)
task *tp;
{
    if (time_sec >= bgp_next_flash) {
	/* A flash update can be sent now, do it */
	bgp_do_flash(tp, (time_t) 0);
    } else if (!bgp_flash_timer) {
	/* A flash update can't be sent and one is not yet scheduled */
	bgp_flash_timer = timer_create(tp, 0, "Flash", TIMERF_DELETE | TIMERF_ABSOLUTE, bgp_next_flash - time_sec, bgp_do_flash);
    }
}


/*
 *	Process an incoming connection
 */
void
bgp_listen_accept(tp)
task *tp;
{
    int bgp_socket;
    int addrlen;
    struct sockaddr_in addr;
    bgpPeer *bnp;

    addrlen = socksize(&addr);
    if ((bgp_socket = accept(tp->task_socket, (struct sockaddr *) & addr, &addrlen)) < 0) {
	trace(TR_ALL, LOG_ERR, "bgp_listen_accept: accept(%d): %m", tp->task_socket);
	quit(errno);
    }
    if (addrlen != socksize(&addr)) {
	trace(TR_ALL, LOG_ERR, "bgp_listen_accept: incorrect address length, ignoring connection");
	(void) close(bgp_socket);
	return;
    }
    if (addr.sin_family != AF_INET) {
	trace(TR_ALL, LOG_ERR, "bgp_listen_accept: ignoring non-inet connection request");
	(void) close(bgp_socket);
	return;
    }
    BGP_LIST(bnp) {
 	if (equal_in(bnp->bgp_addr.sin_addr, addr.sin_addr)) {
	    switch (bnp->bgp_state) {
		case BGPSTATE_IDLE:
		case BGPSTATE_OPENSENT:
		case BGPSTATE_OPENCONFIRM:
		case BGPSTATE_ESTABLISHED:
		    trace(TR_EXT, LOG_NOTICE, "bgp_listen_accept: peer %s state %s rejecting connection",
			  bnp->bgp_name,
			  trace_state(bgpStates, bnp->bgp_state));
		    (void) close(bgp_socket);
		    break;
		case BGPSTATE_CONNECT:
		    /* Prevent the active connect from succeding */
		    if (close(bnp->bgp_task->task_socket) < 0) {
			trace(TR_ALL, LOG_ERR, "bgp_connect_complete: close(%d): %m",
			      bnp->bgp_task->task_socket);
			quit(errno);
		    }
		    bgp_connect_finit(bnp);
		case BGPSTATE_ACTIVE:
		    bgp_session_init(bnp, bgp_socket);
		    bgp_event_Open(bnp);
	    }
	    break;
	}
    } BGP_LISTEND;

    if (!bnp) {
	trace(TR_EXT, LOG_NOTICE, "bgp_listen_accept: rejecting connection from unknown peer %A",
	      &addr);
	(void) close(bgp_socket);
    }
}


/*
 *	Cleanup before re-parse
 */
/*ARGSUSED*/
static void
bgp_cleanup(tp)
task *tp;
{
    bgp_default_metric = bgpMetricInfinity;
    bgp_preference = RTPREF_BGP;

    adv_free_list(bgp_accept_list);
    bgp_accept_list = (adv_entry *) 0;
    adv_free_list(bgp_propagate_list);
    bgp_propagate_list = (adv_entry *) 0;
}


/*
 *	Setup to catch incoming connections
 */
void
bgp_listen_init()
{
    int bgp_socket;
    static task *bgp_task = (task *) 0;
    struct sockaddr_in addr;
    int on = 1;

    if (doing_bgp && !bgp_task) {
	sockclear_in(&addr);
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = bgp_port;

	if ((bgp_socket = task_get_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    quit(errno);
	}
	if (setsockopt(bgp_socket, SOL_SOCKET, SO_REUSEADDR,
					(char *)&on, sizeof(int)) < 0) {
	    trace(TR_ALL, LOG_ERR, "bgp_listen_init: setsockopt: %m");
	    quit(errno);
	}
	if (bind(bgp_socket, (struct sockaddr *) & addr, socksize(&addr)) < 0) {
	    trace(TR_ALL, LOG_ERR, "bgp_listen_init: bind %A: %m",
		  &addr);
	    quit(errno);
	}
	if (listen(bgp_socket, 5) < 0) {
	    trace(TR_ALL, LOG_ERR, "bgp_listen_init: listen: %m");
	    quit(errno);
	}
	bgp_task = task_alloc("BGP_listen");
	bgp_task->task_flags = TASKF_ACCEPT;
	sockcopy(&addr, &bgp_task->task_addr);
	bgp_task->task_socket = bgp_socket;
	bgp_task->task_rtproto = RTPROTO_BGP;
	bgp_task->task_accept = bgp_listen_accept;
	bgp_task->task_cleanup = bgp_cleanup;
	bgp_task->task_flash = bgp_flash;	/* Flash updates handled here */
	bgp_task->task_dump = bgp_dump;
	if (!task_create(bgp_task, 0)) {
	    quit(EINVAL);
	}
    } else if (!doing_bgp && bgp_task) {
	task_delete(bgp_task);
	bgp_task = (task *) 0;
    }
}


/*
 *  Receive from socket
 */
int
bgp_recv(tp)
task *tp;
{
    int count, length;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    length = bnp->bgp_length - bnp->bgp_length_accumulated;

    errno = 0;
    count = recv(tp->task_socket, bnp->bgp_readpointer, length, 0);
    if (count > 0) {
	bnp->bgp_length_accumulated += count;
	bnp->bgp_readpointer += count;
	trace(TR_TASK, 0, "bgp_recv: peer %s received %d bytes (%d of %d so far)",
	      bnp->bgp_name,
	      count,
	      bnp->bgp_length_accumulated,
	      bnp->bgp_length);
    } else if (count < 0) {
	int log_level = LOG_WARNING;
	int trace_level = TR_BGP | TR_TASK;

	switch (errno) {
	    case EWOULDBLOCK:
		log_level = 0;
		/* Fall Thru */

	    case ENETDOWN:
	    case ENETUNREACH:
	    case EHOSTDOWN:
	    case EHOSTUNREACH:
		count = 0;
		break;

	    default:
		trace_level = TR_ALL;
		log_level = LOG_ERR;
	}
	trace(trace_level, log_level, "bgp_recv: peer %s recv: %m",
	      bnp->bgp_name);
    } else {
	trace(TR_EXT, LOG_ERR, "bgp_recv: peer %s recv: End of File (Connection Closed)",
	      bnp->bgp_name);
	/* Count is zero - end of file */
	count = -1;
    }
    return (count);
}


/*
 *	Delete a peer
 */
static void
bgp_delete(bnp)
bgpPeer *bnp;
{
    if (bnp == bgp_peers) {
	bgp_peers = bnp->bgp_next;
    } else {
	bgpPeer *bnp2;

	BGP_LIST(bnp2) {
	    if (bnp2->bgp_next == bnp) {
		bnp2->bgp_next = bnp->bgp_next;
		break;
	    }
	} BGP_LISTEND;
    }
    bgp_n_peers--;
    (void) free((caddr_t) bnp);
}


/*
 *	Clean up and shut down
 */
static void
bgp_terminate(tp)
task *tp;
{
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    bgp_event_Stop(bnp);

    timer_delete(tp->task_timer[BGPTIMER_HOLDTIME]);

    task_delete(tp);

    bgp_delete(bnp);
}


/*
 *	Cleanup for a peer
 */
static void
bgp_peer_cleanup(tp)
task *tp;
{
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    bnp->bgp_flags |= BGPF_DELETE;

    adv_cleanup((int *) 0, (int *) 0, (gw_entry *) 0,
		&bnp->bgp_accept, &bnp->bgp_propagate,
		(adv_entry ***) 0, (adv_entry ***) 0);
}


/*
 *	Reinit a peer
 */
static void
bgp_peer_reinit(tp)
task *tp;
{
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    if (!doing_bgp || (bnp->bgp_flags & BGPF_DELETE)) {
	bnp->bgp_flags |= BGPF_DELETE;
	bgp_terminate(tp);
    } else {
	/* Issue a start event for Idle peers */
	switch (bnp->bgp_state) {
	    case BGPSTATE_IDLE:
	    case BGPSTATE_ACTIVE:
		bgp_event_Start(bnp);
		break;
	    case BGPSTATE_CONNECT:
	    case BGPSTATE_OPENSENT:
	    case BGPSTATE_OPENCONFIRM:
	    case BGPSTATE_ESTABLISHED:
		break;
	}

	/* Locate our new policy */
	if (bnp->bgp_asin) {
	    /* If the AS isn't valid now, the correct policy will be located */
	    /* when the AS becomes valid */
	    bnp->bgp_accept = control_exterior_locate(bgp_accept_list, bnp->bgp_asin);
	    bnp->bgp_propagate = control_exterior_locate(bgp_propagate_list, bnp->bgp_asin);
	}
    }
}


void
bgp_init()
{
    bgpPeer *bnp;
    if_entry *ifp;
    struct servent *sp;
    struct sockaddr_in addr;

    if (doing_bgp) {
	if ((sp = getservbyname("bgp", "tcp")) == NULL) {
	    trace(TR_ALL, LOG_ERR, "bgp_init: getservbyname(bgp, tcp): %m - using port %d",
		  IPPROTO_BGP);
	    bgp_port = htons(IPPROTO_BGP);
	} else {
	    bgp_port = sp->s_port;
	}

	sockclear_in(&addr);
	addr.sin_port = bgp_port;

	BGP_LIST(bnp) {
	    if (!bnp->bgp_task) {
		if (!(bnp->bgp_options & BGPO_INTERFACE)) {
		    if (!(bnp->bgp_options & BGPO_GATEWAY)) {
			sockcopy(&bnp->bgp_addr, &addr);
		    } else {
			addr = bnp->bgp_gateway;	/* struct copy */
		    }
		    ifp = if_withdst((sockaddr_un *) & addr);
		    if (ifp) {
			bnp->bgp_interface = ifp;
		    } else if (!(bnp->bgp_options & BGPO_GATEWAY)) {
			/* If we only have one interface, use it */
			ifp = NULL;
			if (n_interfaces == 1) {
			    IF_LIST(ifp) {
				if (!(ifp->int_state & IFS_LOOPBACK)) {
				    bnp->bgp_interface = ifp;
				    break;
				}
			    } IF_LISTEND(ifp) ;
			}
			if (!ifp) {
			    trace(TR_INT, LOG_ERR, "bgp_init: Can't determine interface for peer %s", bnp->bgp_name);
			    quit(EDESTADDRREQ);
			}
		    } else {
			trace(TR_INT, LOG_ERR, "bgp_init: no direct net to gateway %s", bnp->bgp_name);
			quit(EDESTADDRREQ);
		    }
		}
		/* If AsOut was not specified, default to my_system */
		if (!(bnp->bgp_options & BGPO_ASOUT)) {
		    bnp->bgp_asout = my_system;
		}
		if (bnp->bgp_options & BGPO_ASIN) {
		    bnp->bgp_accept = control_exterior_locate(bgp_accept_list, bnp->bgp_asin);
		    bnp->bgp_propagate = control_exterior_locate(bgp_propagate_list, bnp->bgp_asin);
		}
		/* If HoldTime is not specified, assume default */
		if (!(bnp->bgp_options & BGPO_HOLDTIME)) {
		    bnp->bgp_holdtime_out = BGP_KEEP_ALIVE;
		}
		/* If link type is not specifed, Horizontal is assumed if */
		/* the incoming and outgoing AS are different.  If they */
		/* are the same, Internal is the default. */
		if (!(bnp->bgp_options & BGPO_LINKTYPE)) {
		    if ((bnp->bgp_options & BGPO_ASIN) && (bnp->bgp_asin == bnp->bgp_asout)) {
			bnp->bgp_linktype = openLinkInternal;
		    } else {
			bnp->bgp_linktype = openLinkHorizontal;
		    }
		    trace(TR_BGP, LOG_INFO, "bgp_init: peer %s - link type not specified, assuming %s",
			  bnp->bgp_name,
			  trace_state(bgpOpenType, bnp->bgp_linktype));
		}
		if (!(bnp->bgp_options & BGPO_PREFERENCE)) {
		    bnp->bgp_preference = bgp_preference;
		}
		/* If the link type is internal, the protocol is IBGP and */
		/* the default preference is different.  The protocol is */
		/* not set here though, it is set when the task is */
		/* allocated. */
		if (bnp->bgp_linktype == openLinkInternal) {
		    if (!(bnp->bgp_options & BGPO_PREFERENCE)) {
			bnp->bgp_preference = RTPREF_IBGP;
		    }
		}
		/* Initially in the Idle state */
		bnp->bgp_state = BGPSTATE_IDLE;

		/* Create the abort timer task */
		bnp->bgp_task = task_alloc("BGP");
		bnp->bgp_addr.sin_port = bgp_port;
		bnp->bgp_task->task_addr.in = bnp->bgp_addr;	/* struct copy */

		bnp->bgp_task->task_rtproto = RTPROTO_BGP;
		bnp->bgp_task->task_terminate = bgp_terminate;
		bnp->bgp_task->task_cleanup = bgp_peer_cleanup;
		bnp->bgp_task->task_reinit = bgp_peer_reinit;
		bnp->bgp_task->task_data = (caddr_t) bnp;
		if (!task_create(bnp->bgp_task, 0)) {
		    quit(EINVAL);
		}
		/* The hold timer is used to start a connect after a few seconds */
		(void) timer_create(bnp->bgp_task,
				    BGPTIMER_HOLDTIME,
				    "Holdtime",
				    TIMERF_ABSOLUTE,
				    (time_t) BGP_IDLE_INIT,
				    bgp_event_Holdtime);
	    }
	} BGP_LISTEND;

	if (!test_flag) {
	    /* Listen for incoming connections */
	    bgp_listen_init();

	    if ((bgp_send_buffer = (bgpPdu *) malloc(BGPMAXPACKETSIZE)) == NULL) {
		trace(TR_ALL, LOG_ERR, "bgp_init: malloc send buffer: %m");
	    }
	}
    } else {
	/* BGP is not running, but it may have been.  Delete all peers that do not have tasks */
	BGP_LIST(bnp) {
	    if (!bnp->bgp_task) {
		bgp_delete(bnp);
	    }
	} BGP_LISTEND;

	bgp_cleanup((task *) 0);
	bgp_listen_init();
    }
}


#endif				/* PROTO_BGP */
