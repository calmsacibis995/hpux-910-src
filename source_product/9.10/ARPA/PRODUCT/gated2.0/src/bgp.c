/*
 *  $Header: bgp.c,v 1.1.109.5 92/02/28 14:01:11 ash Exp $
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

void bgp_event_OpenFail();
void bgp_event_Closed();

/*
 *
 *  Routines to log messages about events and state transitions
 *
 */
void
bgp_msg_event(bnp, event)
bgpPeer *bnp;
int event;
{

    trace(TR_BGP, 0, "bgp_msg_event: peer %s state %s event %s",
	  bnp->bgp_name, trace_state(bgpStates, bnp->bgp_state), trace_state(bgpEvents, event));
}


void
bgp_msg_state(bnp, state)
bgpPeer *bnp;
int state;
{

    trace(TR_BGP, 0, "bgp_msg_state: peer %s state %s transition to state %s",
	  bnp->bgp_name, trace_state(bgpStates, bnp->bgp_state), trace_state(bgpStates, state));
}


void
bgp_msg_confused(bnp, event)
bgpPeer *bnp;
int event;
{
    trace(TR_BGP, 0, "bgp_msg_confused: peer %s event %s should not occur in state %s",
	  bnp->bgp_name, trace_state(bgpEvents, event), trace_state(bgpStates, bnp->bgp_state));
}


/*
 *
 *	Routines to transition to new states.  Processing that always
 *	occurs during a state transition is done here.
 *
 */

void
bgp_state_Idle(bnp, interval)
bgpPeer *bnp;
time_t interval;			/* How long before entering Active state */
{
    int changes = 0;

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_IDLE);

    /* Set the Abort timer for automatic Start event */
    if (interval) {
	timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], interval);
    }
    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	    break;
	case BGPSTATE_CONNECT:
	    bgp_connect_finit(bnp);
	    break;
	case BGPSTATE_ESTABLISHED:
	    trace(TR_BGP, LOG_NOTICE, "bgp_state_Idle: lost peer %s AS %d",
		  bnp->bgp_name,
		  bnp->bgp_asin);
	    changes += rt_gwunreach(bnp->bgp_task, &bnp->bgp_gw);
	    if (bnp->bgp_flags & BGPF_GENDEFAULT) {
		changes += rt_default_delete();
		bnp->bgp_flags &= ~BGPF_GENDEFAULT;
	    }
	    if (changes) {
		trace(TR_RT, 0, "bgp_state_Idle: above changes due to loss of peer %s", bnp->bgp_name);
	    }
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	    bgp_session_finit(bnp, FALSE);
	    break;
    }

    if (bnp->bgp_packet) {
	/* Free packet buffer */
	(void) free((caddr_t) bnp->bgp_packet);
    }
    bnp->bgp_state = BGPSTATE_IDLE;

}


void
bgp_state_Active(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_ACTIVE);

    bnp->bgp_state = BGPSTATE_ACTIVE;

    if (bnp->bgp_task->task_socket != -1) {
	bgp_session_finit(bnp, FALSE);
    }
    timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], (time_t) BGP_IDLE_SHORT);

}


void
bgp_state_Connect(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_CONNECT);

    bnp->bgp_state = BGPSTATE_CONNECT;

    timer_reset(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME]);
    bgp_connect_init(bnp);
}


void
bgp_state_OpenSent(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_OPENSENT);

    bnp->bgp_state = BGPSTATE_OPENSENT;

    /* Allocate receive packet */
    bnp->bgp_packet = (bgpPdu *) malloc(BGPMAXPACKETSIZE);
    if (!bnp->bgp_packet) {
	trace(TR_ALL, LOG_ERR, "bgp_state_OpenSent: peer %s malloc receive packet: %m",
	      bnp->bgp_name);
	quit(errno);
    }
    bnp->bgp_length = 0;		/* Setup to receive first packet header */
}


void
bgp_state_OpenConfirm(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_OPENCONFIRM);

    bnp->bgp_state = BGPSTATE_OPENCONFIRM;
}


void
bgp_state_Established(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_state(bnp, BGPSTATE_ESTABLISHED);

    bnp->bgp_state = BGPSTATE_ESTABLISHED;

    /* Reset the abort timer */
    timer_set(bnp->bgp_task->task_timer[BGPTIMER_KEEPALIVE], bnp->bgp_holdtime_out / 3);

    trace(TR_BGP, LOG_WARNING, "bgp_state_Established: established peer %s AS %d",
	  bnp->bgp_name,
	  bnp->bgp_asin);
}


void
bgp_send(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{

    length += sizeof(pduHeader);

    PDU->header.marker = htons(bgpMarker);
    PDU->header.length = length;
    PDU->header.length = htons(PDU->header.length);
    PDU->header.version = bgpVersion;
    PDU->header.holdTime = htons(bnp->bgp_holdtime_out);

    bgp_trace((struct sockaddr_in *) & bnp->bgp_interface->int_addr,
	      (struct sockaddr_in *) & bnp->bgp_addr,
	      "Send",
	      PDU,
	      length);

    if (task_send_packet(bnp->bgp_task, (caddr_t) PDU, length, 0, (sockaddr_un *) 0) < 0) {
	switch (errno) {
	    case ENETDOWN:
	    case ENETUNREACH:
	    case EHOSTDOWN:
	    case EHOSTUNREACH:
		break;

	    default:
		switch (bnp->bgp_state) {
		    case BGPSTATE_IDLE:
		    case BGPSTATE_ACTIVE:
			break;

		    case BGPSTATE_CONNECT:
			bgp_event_OpenFail(bnp);
			break;

		    case BGPSTATE_OPENSENT:
		    case BGPSTATE_OPENCONFIRM:
		    case BGPSTATE_ESTABLISHED:
			bgp_event_Closed(bnp);
			break;
		}
	}
    }
}


void
bgp_send_KeepAlive(bnp)
bgpPeer *bnp;
{
    bgpPdu *PDU = (bgpPdu *) bgp_send_buffer;

    PDU->header.type = bgpPduKeepAlive;

    bgp_send(bnp, PDU, 0);
}


void
bgp_send_OpenConfirm(bnp)
bgpPeer *bnp;
{
    bgpPdu *PDU = (bgpPdu *) bgp_send_buffer;

    PDU->header.type = bgpPduOpenConfirm;

    bgp_send(bnp, PDU, 0);
}


void
bgp_send_Open(bnp)
bgpPeer *bnp;
{
    bgpPdu *PDU = (bgpPdu *) bgp_send_buffer;

    PDU->header.type = bgpPduOpen;

    PDU->pdu.open.openAs = htons(bnp->bgp_asout);
    PDU->pdu.open.openLinkType = bnp->bgp_linktype;
    PDU->pdu.open.openAuthCode = htons(0);

    bgp_send(bnp, PDU, sizeof(openPdu) - 1);
}


void
bgp_send_Notify(bnp, code, data)
bgpPeer *bnp;
int code;				/* Error code */
u_char *data;				/* Data in network byte order */
{
    int length;
    bgpPdu *PDU = (bgpPdu *) bgp_send_buffer;

    length = notifyLengths[code];

    PDU->header.type = bgpPduNotify;

    PDU->pdu.notify.notifyCode = code;
    PDU->pdu.notify.notifyCode = htons(PDU->pdu.notify.notifyCode);
    memcpy((char *) PDU->pdu.notify.notifyData, (caddr_t) data, length);

    bgp_send(bnp, PDU, sizeof(PDU->pdu.notify.notifyCode) + length);

}


void
bgp_send_NotifyUpdate(bnp, code, packet, size)
bgpPeer *bnp;
int code;				/* Error code */
bgpPdu *packet;
int size;
{
    int length;
    u_short short_code;
    bgpPdu *PDU = (bgpPdu *) bgp_send_buffer;

    length = sizeof(notifyPdu) - 1 + size - sizeof(pduHeader);
    if ((length + sizeof(pduHeader)) > BGPMAXPACKETSIZE) {
	length = BGPMAXPACKETSIZE;
    }
    PDU->header.type = bgpPduNotify;

    PDU->pdu.notify.notifyCode = htons(BGPERRCD_UPDATE);
    short_code = code;
    short_code = htons(short_code);
    memcpy((caddr_t) PDU->pdu.notify.notifyData, (caddr_t) & short_code, sizeof(PDU->pdu.notify.notifyData));

    memcpy((char *) PDU->pdu.notify.notifyPacket, (caddr_t) packet + sizeof(pduHeader),
	   length - (sizeof(PDU->pdu.notify) - sizeof(PDU->pdu.notify.notifyPacket)));

    bgp_send(bnp, PDU, length);

}



void
bgp_event_Start(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_START);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	    bgp_state_Connect(bnp);
	    break;
	case BGPSTATE_CONNECT:
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    bgp_msg_confused(bnp, BGPEVENT_START);
#ifdef	notdef
	    /* Allow for operator initiated starts */
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
#endif				/* notdef */
	    break;
    }
}


void
bgp_event_Open(bnp)
bgpPeer *bnp;
{
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_OPEN);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    bgp_msg_confused(bnp, BGPEVENT_OPEN);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	    bgp_send_Open(bnp);
	    timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], (time_t) BGP_ABORT_OPEN);
	    bgp_state_OpenSent(bnp);
    }

}


void
bgp_event_Closed(bnp)
bgpPeer *bnp;
{
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_CLOSED);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	    bgp_msg_confused(bnp, BGPEVENT_CLOSED);
	    /* Fall Through */
	case BGPSTATE_ESTABLISHED:
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_SHORT);
	    break;
	case BGPSTATE_OPENSENT:
	    /* Deviation from RFC1105 - need to do this to make opens succeed */
	    bgp_state_Active(bnp);
	    break;
	case BGPSTATE_OPENCONFIRM:
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_SHORT);
	    break;
    }
}


void
bgp_event_OpenFail(bnp)
bgpPeer *bnp;
{
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_OPENFAIL);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    bgp_msg_confused(bnp, BGPEVENT_OPENFAIL);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_CONNECT:
	    bgp_state_Active(bnp);
	    break;
	case BGPSTATE_OPENSENT:
	    /* Open failed after connect succeded */
	    bgp_state_Active(bnp);
	    break;
    }
}



void
bgp_event_RecvOpen(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{
    int error = 0;
    u_char *errptr = NULL;
    static u_char linkTypes[] =
    {
	openLinkInternal,
	openLinkDown,
	openLinkUp,
	openLinkHorizontal,
    };

#define	ptrTypeInternal	&linkTypes[0]	/* Pointer to correct link type for internal connection */

    if (length < (sizeof(pduHeader) + sizeof(openPdu) - sizeof(PDU->pdu.open.openAuthData))) {
	error = BGPERRCD_MSGLEN;
	errptr = (u_char *) & PDU->header.length;
	goto Error;
    }
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_RECVOPEN);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    bgp_msg_confused(bnp, BGPEVENT_RECVOPEN);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    return;
	case BGPSTATE_OPENSENT:
	    /* If AsIn option specified, verify that his AS matches */
	    if (bnp->bgp_options & BGPO_ASIN) {
		if (bnp->bgp_asin != ntohs(PDU->pdu.open.openAs)) {
		    error = BGPERRCD_OPENAS;
		    goto Error;
		}
	    } else {
		bnp->bgp_asin = ntohs(PDU->pdu.open.openAs);
		bnp->bgp_accept = control_exterior_locate(bgp_accept_list, bnp->bgp_asin);
		bnp->bgp_propagate = control_exterior_locate(bgp_propagate_list, bnp->bgp_asin);
	    }

	    /* If his AS matches ours, link type must be Internal, if it does not, it must not */
	    if (bnp->bgp_asin == bnp->bgp_asout) {
		if (PDU->pdu.open.openLinkType != openLinkInternal) {
		    error = BGPERRCD_LINKTYPE;
		    errptr = ptrTypeInternal;
		    goto Error;
		}
	    } else {
		if (PDU->pdu.open.openLinkType == openLinkInternal) {
		    error = BGPERRCD_LINKTYPE;
		    errptr = ptrTypeInternal;
		    goto Error;
		}
		if (PDU->pdu.open.openLinkType != linkTypes[bnp->bgp_linktype]) {
		    error = BGPERRCD_LINKTYPE;
		    errptr = &linkTypes[bnp->bgp_linktype];
		    goto Error;
		}
	    }

	    /* Authorization code goes here */
	    if (PDU->pdu.open.openAuthCode != 0) {
		error = BGPERRCD_AUTHCODE;
		goto Error;
	    }
	    /* Open packet OK, send confirm, change state and set HoldTimer */
	    bgp_send_OpenConfirm(bnp);
	    bgp_state_OpenConfirm(bnp);
	    timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], bnp->bgp_holdtime_in);
    }

  Error:
    if (error) {
	bgp_send_Notify(bnp, error, errptr);
	bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
    }
}


/*ARGSUSED*/
void
bgp_event_RecvConfirm(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{

    if (length != sizeof(pduHeader)) {
	bgp_send_Notify(bnp, BGPERRCD_MSGLEN, (u_char *) & PDU->header.length);
	bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
    } else {
	IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_RECVCONFIRM);

	switch (bnp->bgp_state) {
	    case BGPSTATE_IDLE:
	    case BGPSTATE_ACTIVE:
	    case BGPSTATE_CONNECT:
	    case BGPSTATE_OPENSENT:
	    case BGPSTATE_ESTABLISHED:
		bgp_msg_confused(bnp, BGPEVENT_RECVCONFIRM);
		bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
		break;
	    case BGPSTATE_OPENCONFIRM:
		bgp_state_Established(bnp);
		bgp_send_update(bnp, FALSE);	/* Send a complete update */
	}
    }

}


void
bgp_event_RecvKeepAlive(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{

    if (length != sizeof(pduHeader)) {
	bgp_send_Notify(bnp, BGPERRCD_MSGLEN, (u_char *) & PDU->header.length);
	bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
    } else {
	IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_RECVKEEPALIVE);

	switch (bnp->bgp_state) {
	    case BGPSTATE_IDLE:
	    case BGPSTATE_ACTIVE:
	    case BGPSTATE_CONNECT:
	    case BGPSTATE_OPENSENT:
	    case BGPSTATE_OPENCONFIRM:
		bgp_msg_confused(bnp, BGPEVENT_RECVKEEPALIVE);
		bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
		break;
	    case BGPSTATE_ESTABLISHED:
		timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], bnp->bgp_holdtime_in);
	}
    }
}


void
bgp_event_RecvUpdate(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_RECVUPDATE);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	    bgp_msg_confused(bnp, BGPEVENT_RECVUPDATE);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_ESTABLISHED:
	    bgp_recv_Update(bnp, PDU, length);
	    timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], bnp->bgp_holdtime_in);
    }
}


/*ARGSUSED*/
void
bgp_event_RecvNotify(bnp, PDU, length)
bgpPeer *bnp;
bgpPdu *PDU;
int length;
{
    u_short error_code;
    const char *err_msg = "Invalid error code";

#ifdef	notdef
    /* XXX - This calculation was not working correctly */
    if (length < (sizeof(pduHeader) + sizeof(notifyPdu) - sizeof(PDU->pdu.notify.notifyPacket))) {
	error = BGPERRCD_MSGLEN;
	errptr = (u_char *) & PDU->header.length;
	goto Error;
    }
#endif				/* notdef */

    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_RECVNOTIFY);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	    bgp_msg_confused(bnp, BGPEVENT_RECVNOTIFY);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    error_code = ntohs(PDU->pdu.notify.notifyCode);
	    if (error_code != BGPERRCD_UPDATE) {
		if (error_code && (error_code <= BGPERRCD_MAX)) {
		    err_msg = trace_state(bgpErrors, error_code);
		}
		trace(TR_EXT, LOG_WARNING, "bgp_event_RecvNotify: peer %s error %s(%d)",
		      bnp->bgp_name,
		      err_msg,
		      error_code);
		if (error_code == BGPERRCD_CEASE) {
		    bgp_state_Idle(bnp, (time_t) BGP_IDLE_SHORT);
		} else {
		    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
		}
	    } else {
		memcpy((caddr_t) & error_code, (caddr_t) PDU->pdu.notify.notifyData, sizeof(error_code));
		error_code = ntohs(error_code);
		if (error_code && (error_code <= BGPUPDERR_MAX)) {
		    err_msg = trace_state(bgpUpdateErrors, error_code);
		}
		trace(TR_EXT, LOG_WARNING, "bgp_event_RecvNotify: peer %s update error %s(%d)",
		      bnp->bgp_name,
		      err_msg,
		      error_code);
		timer_set(bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME], bnp->bgp_holdtime_in);
	    }
    }
}


/*ARGSUSED*/
void
bgp_event_Holdtime(tip, interval)
timer *tip;
time_t interval;
{
    bgpPeer *bnp = (bgpPeer *) tip->timer_task->task_data;

    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_HOLDTIME);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	    /* Automatically issue a start event when HoldTimer fires */
	    /* in Idle and Active states */
	    bgp_event_Start(bnp);
	    break;
	case BGPSTATE_CONNECT:
	    bgp_msg_confused(bnp, BGPEVENT_HOLDTIME);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_SHORT);
    }
}


/*ARGSUSED*/
void
bgp_event_KeepAlive(tip, interval)
timer *tip;
time_t interval;
{
    bgpPeer *bnp = (bgpPeer *) tip->timer_task->task_data;

    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_KEEPALIVE);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	    bgp_msg_confused(bnp, BGPEVENT_KEEPALIVE);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_ESTABLISHED:
	    bgp_send_KeepAlive(bnp);
    }
}


void
bgp_event_Cease(bnp)
bgpPeer *bnp;
{
    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_CEASE);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	    bgp_msg_confused(bnp, BGPEVENT_CEASE);
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
	    break;
	case BGPSTATE_ESTABLISHED:
	    bgp_send_Notify(bnp, BGPERRCD_CEASE, (u_char *) 0);	/* XXX - is the spec right about this? */
	    bgp_state_Idle(bnp, (time_t) BGP_IDLE_SHORT);
    }
}


void
bgp_event_Stop(bnp)
bgpPeer *bnp;
{

    IF_BGPPROTO bgp_msg_event(bnp, BGPEVENT_STOP);

    switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	    break;
	case BGPSTATE_OPENSENT:
	case BGPSTATE_OPENCONFIRM:
	case BGPSTATE_ESTABLISHED:
	    /* If connected, send an error before shutting down */
	    bgp_send_Notify(bnp, BGPERRCD_CEASE, (u_char *) 0);
	    /* Fall Through */
	case BGPSTATE_ACTIVE:
	case BGPSTATE_CONNECT:
	    bgp_state_Idle(bnp, (time_t) 0);
	    break;
    }
}


/*
 *  Process a complete received packet
 */
void
bgp_in(tp)
task *tp;
{
    int length;
    int error = 0;
    u_char *errptr = NULL;
    u_short holdTime;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;
    bgpPdu *PDU;

    PDU = bnp->bgp_packet;
    length = bnp->bgp_length;

    bgp_trace((struct sockaddr_in *) & bnp->bgp_addr,
	      (struct sockaddr_in *) & bnp->bgp_interface->int_addr,
	      "Recv",
	      PDU,
	      length);

    holdTime = ntohs(PDU->header.holdTime);
    /* XXX - Should check validity of Hold Time */
    if (bnp->bgp_holdtime_in != holdTime) {
	bnp->bgp_holdtime_in = holdTime;
	IF_BGPPROTO trace(TR_BGP, 0, "bgp_in: peer %s holdtime set to %#T",
			   bnp->bgp_name,
			   bnp->bgp_holdtime_in);
    }
    switch (PDU->header.type) {
	case bgpPduOpen:
	    bgp_event_RecvOpen(bnp, PDU, length);
	    break;
	case bgpPduUpdate:
	    if (length < (sizeof(pduHeader) + updatePduMinSize)) {
		error = BGPERRCD_MSGLEN;
		errptr = (u_char *) & PDU->header.length;
		goto Error;
	    }
	    bgp_event_RecvUpdate(bnp, PDU, length);
	    break;
	case bgpPduNotify:
	    bgp_event_RecvNotify(bnp, PDU, length);
	    break;
	case bgpPduKeepAlive:
	    bgp_event_RecvKeepAlive(bnp, PDU, length);
	    break;
	case bgpPduOpenConfirm:
	    bgp_event_RecvConfirm(bnp, PDU, length);
	    break;
    }

  Error:
    if (error) {
	bgp_send_Notify(bnp, error, errptr);
	bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
    }
}


/*
 * Process a successful select for read by reading up to one BGP
 * packet or until the read would block.
 */
void
bgp_read(tp)
task *tp;
{
    int error = 0;
    int count;
    u_char *errptr = NULL;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;
    bgpPdu *PDU = bnp->bgp_packet;

    if (!bnp->bgp_length) {
	/* New packet */
	bnp->bgp_length = sizeof(pduHeader);
	bnp->bgp_length_accumulated = 0;
	bnp->bgp_readpointer = (caddr_t) bnp->bgp_packet;
	bnp->bgp_flags |= BGPF_HEADER;	/* Indicate we are trying to read header */
    }
    while ((count = bgp_recv(tp)) > 0) {

	/* Process data if we have read desired length */
	if (bnp->bgp_length_accumulated == bnp->bgp_length) {

	    /* Complete packet or header has been read */
	    if (bnp->bgp_flags & BGPF_HEADER) {

		/* Complete Header has been read */

		bnp->bgp_length = ntohs(PDU->header.length);
		/* Verify Marker */
		if (ntohs(PDU->header.marker) != bgpMarker) {
		    trace(TR_INT, LOG_WARNING, "bgp_read: peer %s missing marker or out of sync",
			  bnp->bgp_name);
		    error = BGPERRCD_SYNC;
		    goto Error;
		};

		/* Verify packet length */
		if ((bnp->bgp_length < sizeof(pduHeader)) || (bnp->bgp_length > BGPMAXPACKETSIZE)) {
		    trace(TR_INT, LOG_WARNING, "bgp_read: peer %s Invalid length %d",
			  bnp->bgp_name,
			  bnp->bgp_length);
		    error = BGPERRCD_MSGLEN;
		    errptr = (u_char *) & PDU->header.length;
		    goto Error;
		}
		/* Verify version */
		if (PDU->header.version != BGP_VERSION) {
		    trace(TR_EXT, 0, "bgp_read: peer %s invalid version: %d",
			  bnp->bgp_name,
			  PDU->header.version);
		    error = BGPERRCD_VERSION;
		    errptr = &PDU->header.version;
		    goto Error;
		}
		/* Verify that packet type is valid */
		if (!PDU->header.type || (PDU->header.type > bgpPduMax)) {
		    trace(TR_EXT, 0, "bgp_read: peer %s invalid packet type: %d",
			  bnp->bgp_name,
			  PDU->header.type);
		    error = BGPERRCD_MSGTYPE;
		    errptr = (u_char *) & PDU->header.type;
		    goto Error;
		}
		if (bnp->bgp_length > sizeof(pduHeader)) {
		    bnp->bgp_length_accumulated = sizeof(pduHeader);
		    bnp->bgp_readpointer = (caddr_t) PDU + sizeof(pduHeader);
		    bnp->bgp_flags &= ~BGPF_HEADER;
		    continue;		/* Retry loop for rest of packet */
		}
	    }
	    bgp_in(tp);

	    /* Setup for header and break from loop */
	    bnp->bgp_length = 0;
	    bnp->bgp_length_accumulated = 0;
	    break;
	}
    }

    if (count < 0) {
	bgp_event_Closed(bnp);
    }
  Error:
    if (error) {
	bgp_send_Notify(bnp, error, errptr);
	bgp_state_Idle(bnp, (time_t) BGP_IDLE_FATAL);
    }
}


int
bgp_peer_changed(old, new)
bgpPeer *old, *new;
{
    int changed = FALSE;
    flag_t changed_options = old->bgp_options ^ new->bgp_options;
    flag_t new_options = new->bgp_options;

    if (changed_options &
	(BGPO_METRICOUT | BGPO_ASIN | BGPO_ASOUT | BGPO_GATEWAY | BGPO_PREFERENCE | BGPO_INTERFACE | BGPO_LINKTYPE)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_METRICOUT) && (old->bgp_metricout != new->bgp_metricout)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_ASIN) && (old->bgp_asin != new->bgp_asin)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_ASOUT) && (old->bgp_asout != new->bgp_asout)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_GATEWAY) && !equal(&old->bgp_gateway, &new->bgp_gateway)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_PREFERENCE) && (old->bgp_preference != new->bgp_preference)) {
	changed = TRUE;
    }
    if ((new_options & BGPO_INTERFACE) && (old->bgp_interface != new->bgp_interface)) {
	changed = TRUE;
    }
    if (((changed_options & BGPO_LINKTYPE) || (new_options & BGPO_LINKTYPE)) && (old->bgp_linktype != new->bgp_linktype)) {
	changed = TRUE;
    }
    if (!changed) {
	/* Nothing has changed that has required a restart.  Let's deal with things */
	/* that can be changed on the fly */

	/* Default propagation and generation options can be changed by just changing the flags */
#define	FLAGS	(BGPO_NOGENDEFAULT|BGPO_LINKTYPE)
	old->bgp_options = (old->bgp_options & ~(FLAGS)) | (new->bgp_options & (FLAGS));
	changed_options &= ~(FLAGS);
#undef	FLAGS
    }
    return (changed);
}


void
bgp_dump(fd)
FILE *fd;
{
    bgpPeer *bnp;

    if (doing_bgp) {
	(void) fprintf(fd, "BGP status:\n");
	(void) fprintf(fd, "\tdefaultbgpmetric: %d\tpreference: %d\n",
		       bgp_default_metric,
		       bgp_preference);
	(void) fprintf(fd, "\tAutonomous System: %u\n\n",
		       my_system);
	if (bgp_accept_list) {
	    control_exterior_dump(fd, 1, control_accept_dump, bgp_accept_list);
	}
	if (bgp_propagate_list) {
	    control_exterior_dump(fd, 1, control_propagate_dump, bgp_propagate_list);
	}
	BGP_LIST(bnp) {
	    (void) fprintf(fd, "\t%-15s\n\t\tinterface: %s\tLinkType: %s\n",
			   bnp->bgp_name,
			   bnp->bgp_interface->int_name,
			   trace_state(bgpOpenType, bnp->bgp_linktype));
	    (void) fprintf(fd, "\t\tAsIn: %5u\t\tAsOut: %5u\t\tPreference: %3u\n",
			   bnp->bgp_asin,
			   bnp->bgp_asout,
			   bnp->bgp_preference);
	    (void) fprintf(fd, "\t\tState: <%s>\tFlags:<%s>\n",
			   trace_state(bgpStates, bnp->bgp_state),
			   trace_bits(bgpFlags, bnp->bgp_flags));
	    (void) fprintf(fd, "\t\tOptions: <%s>\n",
			   trace_bits(bgpOptions, bnp->bgp_options));
	    (void) fprintf(fd, "\t\tHoldTime: In: %#T  Out: %#T\n",
			   bnp->bgp_holdtime_in,
			   bnp->bgp_holdtime_out);
	    (void) fprintf(fd, "\t\tAbort at: %T\n",
			   bnp->bgp_task->task_timer[BGPTIMER_HOLDTIME]->timer_next_time);
	    if (bnp->bgp_options & BGPO_GATEWAY) {
		(void) fprintf(fd, "\t\tGateway: %A\n",
			       &bnp->bgp_gateway);
	    }
	    if (bnp->bgp_options & BGPO_METRICOUT) {
		(void) fprintf(fd, "\t\tMetricOut: %d\n",
			       bnp->bgp_metricout);
	    }
	    (void) fprintf(fd, "\n\n");
	} BGP_LISTEND;
    }
    /* Dump the AS paths */
    bgp_as_dump(fd);
}


void
bgp_trace_update(comment, update, length)
char *comment;
u_char *update;
int length;
{
    u_char *cp, *lp;
    u_char asCount;
    u_char asDirection;
    as_t asNumber;
    struct sockaddr_in gateway;
    u_short netCount;
    u_short netMetric;
    struct sockaddr_in netNetwork;

    sockclear_in(&gateway);
    sockclear_in(&netNetwork);

    cp = update;
    lp = cp + length;

    PickUp(cp, gateway.sin_addr);
    PickUp(cp, asCount);
    trace(TR_BGP, 0, "%s Gateway %A asCount %d",
	  comment,
	  &gateway,
	  asCount);
    if (!asCount || ((cp + asCount * (sizeof(asDirection) + sizeof(asNumber))) >= lp)) {
	trace(TR_BGP, 0, "invalid asCount");
	return;
    }
    for (; asCount; asCount--) {
	PickUp(cp, asDirection);
	PickUp(cp, asNumber);
	asNumber = ntohs(asNumber);
	trace(TR_BGP, 0, "%s\tAS %5d  Direction %s(%d)",
	      comment,
	      asNumber,
	      trace_state(bgpAsDirs, asDirection),
	      asDirection);
    }
    if ((cp + sizeof(gateway.sin_addr)) >= lp) {
	trace(TR_BGP, 0, "premature end of packet");
	return;
    }
    PickUp(cp, netCount);
    netCount = ntohs(netCount);
    trace(TR_BGP, 0, "%s Gateway %A  netCount %d",
	  comment,
	  &gateway,
	  netCount);
    if (!netCount || (cp + netCount * (sizeof(netMetric) + sizeof(netNetwork.sin_addr))) != lp) {
	trace(TR_BGP, 0, "invalid net Count");
	return;
    }
    for (; netCount; netCount--) {
	PickUp(cp, netNetwork.sin_addr);
	tracef("%s\tNetwork %-15A  Metric ",
	       comment,
	       &netNetwork);
	PickUp(cp, netMetric);
	netMetric = ntohs(netMetric);
	if (netMetric == bgpMetricInfinity) {
	    tracef("unreachable");
	} else {
	    tracef("%d",
		   netMetric);
	}
	trace(TR_BGP, 0, NULL);
    }
}


void
bgp_trace(src, dst, direction, PDU, length)
struct sockaddr_in *src, *dst;
const char *direction;
bgpPdu *PDU;
int length;
{
    char comment[BUFSIZ];
    const char *pduType;
    u_short notifyCode;
    u_short twobyte;
    const char *msg;

    (void) sprintf(comment, "BGP %s", direction);

    pduType = trace_state(bgpPduType, PDU->header.type);
    if (!*pduType) {
	pduType = "Invalid";
    }
    trace(TR_BGP, 0, "%s %A -> %A  length %d  version %d  type %s(%d)  HoldTime %d",
	  comment,
	  src, dst,
	  htons(PDU->header.length),
	  PDU->header.version,
	  pduType,
	  PDU->header.type,
	  htons(PDU->header.holdTime));

    switch (PDU->header.type) {
	case bgpPduOpen:
	    trace(TR_BGP, 0, "%s  as %d  linkType %s(%d)  authCode %d",
		  comment,
		  htons(PDU->pdu.open.openAs),
		  trace_state(bgpOpenType, PDU->pdu.open.openLinkType),
		  PDU->pdu.open.openLinkType,
		  PDU->pdu.open.openAuthCode);
	    break;
	case bgpPduUpdate:
	    if (trace_flags & TR_UPDATE) {
		bgp_trace_update(comment, (u_char *) PDU + sizeof(pduHeader), length - sizeof(pduHeader));
	    }
	    break;
	case bgpPduNotify:
	    notifyCode = ntohs(PDU->pdu.notify.notifyCode);
	    msg = trace_state(bgpErrors, notifyCode);
	    if (!msg) {
		msg = "Invalid notification code";
	    }
	    tracef("%s errorCode: %s(%d)",
		   comment,
		   msg,
		   notifyCode);
	    switch (notifyCode) {
		case BGPERRCD_LINKTYPE:
		    trace(TR_BGP, 0, "  correct link type should be %s(%d)",
			  trace_state(bgpOpenType, PDU->pdu.notify.notifyData[0]),
			  PDU->pdu.notify.notifyData[0]);
		    break;
		case BGPERRCD_UPDATE:
		    memcpy((caddr_t) & twobyte, (caddr_t) PDU->pdu.notify.notifyData, sizeof(twobyte));
		    twobyte = ntohs(twobyte);
		    trace(TR_BGP, 0, " updateErrorCode: %s(%d)",
			  trace_state(bgpUpdateErrors, twobyte),
			  twobyte);
		    (void) sprintf(comment, "BGP %s Error", direction);
		    bgp_trace_update(comment, (u_char *) PDU->pdu.notify.notifyPacket,
				     length - (sizeof(pduHeader) + sizeof(notifyPdu) - sizeof(PDU->pdu.notify.notifyPacket)));
		    break;
		case BGPERRCD_MSGLEN:
		    memcpy((caddr_t) & twobyte, (caddr_t) PDU->pdu.notify.notifyData, sizeof(twobyte));
		    twobyte = ntohs(twobyte);
		    trace(TR_BGP, 0, " incorrect length: %d",
			  twobyte);
		    break;
		case BGPERRCD_MSGTYPE:
		    trace(TR_BGP, 0, " incorrect type: %d",
			  PDU->pdu.notify.notifyData[0]);
		    break;
		case BGPERRCD_VERSION:
		    trace(TR_BGP, 0, " incorrect version: %d",
			  PDU->pdu.notify.notifyData[0]);
		    break;
		case BGPERRCD_AUTHCODE:
		case BGPERRCD_AUTHFAIL:
		case BGPERRCD_SYNC:
		case BGPERRCD_OPENAS:
		default:
		    trace(TR_BGP, 0, NULL);
		    break;
	    }
	    break;
	case bgpPduKeepAlive:
	    /* fall through */
	case bgpPduOpenConfirm:
	    break;
    }
    trace(TR_BGP, 0, "");
}

#endif				/* PROTO_BGP */
