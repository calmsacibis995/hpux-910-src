/*
 *  $Header: snmp.c,v 1.1.109.5 92/02/28 16:01:51 ash Exp $
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


#if	defined(AGENT_SNMP)
#include "include.h"
#include "snmp.h"
#include <snmp.h>
#include "egp.h"

static int snmp_request_next;		/* Look at lexically next object */
static int snmp_request_object;		/* Return object identifier also */
static int snmp_request_set;		/* Request is to set variable */

bits snmp_types[] =
{
    {AGENT_REG, "Register"},
    {AGENT_REQ, "Request"},
    {AGENT_ERR, "Error"},
    {AGENT_RSP, "Response"},
    {AGENT_REQN, "RequestNextObject"},
    {AGENT_REQO, "RequestObject"},
    {AGENT_RSPO, "ResponseObject"},
    {AGENT_TRAP, "TrapRequest"},
    {AGENT_SET, "SetRequest"},
    {AGENT_QUERY, "Query"},
    {AGENT_REGSET, "RegisterSet"},
    {0, 0}
};

bits snmp_errors[] =
{
    {NOERR, "noError"},
    {TOOBIG, "tooBig"},
    {NOSUCH, "noSuchName"},
    {BADVAL, "badValue"},
    {RDONLY, "readOnly",},
    {GENERRS, "genErr",},
    {0, 0}
};


int doing_snmp = TRUE;
static task *snmp_task = (task *) 0;

static int ipRoute();

#define	ipRoutePrefix	1, 3, 6, 1, 2, 1, 4, 21, 1
#define	ipRouteDest	1
#define	ipRouteIfIndex	2
#define	ipRouteMetric1	3
#define	ipRouteMetric2	4
#define	ipRouteMetric3	5
#define	ipRouteMetric4	6
#define	ipRouteNextHop	7
#define	ipRouteType	8
#define	ipRouteProto	9
#define	ipRouteAge	10
#define	ipRouteMask	11
#define	ipRouteMetric5	12
#define	ipRouteInfo	13

#ifdef	PROTO_EGP
static int egp();

#define	egpPrefix	1, 3, 6, 1, 2, 1, 8
#define	egpInMsgs	1
#define	egpInErrors	2
#define	egpOutMsgs	3
#define	egpOutErrors	4
#define	egpAs		6

static int egpNeigh();

#define	egpNeighPrefix	5, 1
#define	egpNeighState		1
#define egpNeighAddr		2
#define egpNeighAs		3
#define egpNeighInMsgs		4
#define egpNeighInErrs		5
#define egpNeighOutMsgs		6
#define egpNeighOutErrs		7
#define egpNeighInErrMsgs	8
#define egpNeighOutErrMsgs	9
#define egpNeighStateUps	10
#define egpNeighStateDowns	11
#define egpNeighIntervalHello	12
#define egpNeighIntervalPoll	13
#define egpNeighMode		14
#define egpNeighEventTrigger	15
#endif				/* PROTO_EGP */

static struct mibtbl snmptbl[] =
{
    0,
    {ipRoutePrefix, ipRouteDest}, 0, ipRoute, "ipRouteDest",
    0,
    {ipRoutePrefix, ipRouteIfIndex}, 0, ipRoute, "ipRouteIfIndex",
    0,
    {ipRoutePrefix, ipRouteMetric1}, 0, ipRoute, "ipRouteMetric1",
    0,
    {ipRoutePrefix, ipRouteMetric2}, 0, ipRoute, "ipRouteMetric2",
    0,
    {ipRoutePrefix, ipRouteMetric3}, 0, ipRoute, "ipRouteMetric3",
    0,
    {ipRoutePrefix, ipRouteMetric4}, 0, ipRoute, "ipRouteMetric4",
    0,
    {ipRoutePrefix, ipRouteNextHop}, 0, ipRoute, "ipRouteNextHop",
    0,
    {ipRoutePrefix, ipRouteType}, 0, ipRoute, "ipRouteType",
    0,
    {ipRoutePrefix, ipRouteProto}, 0, ipRoute, "ipRouteProto",
    0,
    {ipRoutePrefix, ipRouteAge}, 0, ipRoute, "ipRouteAge",
    0,
    {ipRoutePrefix, ipRouteMask}, 0, ipRoute, "ipRouteMask",
    0,
    {ipRoutePrefix, ipRouteMetric5}, 0, ipRoute, "ipRouteMetric5",
#ifdef	PROTO_EGP
    0,
    {egpPrefix, egpInMsgs}, MIBF_ONLY, egp, "egpInMsgs",
    0,
    {egpPrefix, egpInErrors}, MIBF_ONLY, egp, "egpInErrors",
    0,
    {egpPrefix, egpOutMsgs}, MIBF_ONLY, egp, "egpOutMsgs",
    0,
    {egpPrefix, egpOutErrors}, MIBF_ONLY, egp, "egpOutErrors",
    0,
{egpPrefix, egpNeighPrefix, egpNeighState}, 0, egpNeigh, "egpNeighState",
    0,
  {egpPrefix, egpNeighPrefix, egpNeighAddr}, 0, egpNeigh, "egpNeighAddr",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighAs}, 0, egpNeigh, "egpNeighAs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighInMsgs}, 0, egpNeigh, "egpNeighInMsgs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighInErrs}, 0, egpNeigh, "egpNeighInErrs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighOutMsgs}, 0, egpNeigh, "egpNeighOutMsgs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighOutErrs}, 0, egpNeigh, "egpNeighOutErrs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighInErrMsgs}, 0, egpNeigh, "egpNeighInErrMsgs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighOutErrMsgs}, 0, egpNeigh, "egpNeighOutErrMsgs",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighStateUps}, 0, egpNeigh, "egpNeighStateUps",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighStateDowns}, 0, egpNeigh, "egpNeighStateDowns",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighIntervalHello}, 0, egpNeigh, "egpNeighIntervalHello",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighIntervalPoll}, 0, egpNeigh, "egpNeighIntervalPoll",
    0,
  {egpPrefix, egpNeighPrefix, egpNeighMode}, 0, egpNeigh, "egpNeighMode",
    0,
    {egpPrefix, egpNeighPrefix, egpNeighEventTrigger}, 0, egpNeigh, "egpNeighEventTrigger",
    0,
    {egpPrefix, egpAs}, MIBF_ONLY, egp, "egpAs",
#endif				/* PROTO_EGP */
    0,
    {0}, 0, 0, 0,
};

/*
 *	Trace an SNMP or SGMP packet.
 */
static void
snmp_trace(tp, direction, packet, length)
task *tp;
char *direction;
char *packet;
int length;
{
    int size, type, integer, i;
    const char *cp;
    char *pkt = packet;
    char comment[MAXHOSTNAMELENGTH];
    struct sockaddr_in addr;
    struct mibtbl *mib_ptr;

    (void) sprintf(comment, "%s %s", tp->task_name, direction);
    cp = trace_state(snmp_types, (*pkt) - 1);
    if (!cp) {
	cp = "Unknown";
    }
    tracef("%s %#A socket %d type %s(%d) length %d",
	   comment,
	   &tp->task_addr,
	   tp->task_socket,
	   cp, *pkt++, length);

    if (trace_flags & TR_UPDATE) {
	if ((pkt - packet) <= length) {
	    switch (*packet) {
		case AGENT_TRAP:
		    trace(TR_SNMP, 0, NULL);
		    type = *pkt++;
		    size = *pkt++;
		    trace(TR_SNMP, 0, "%s\ttrap type %1d",
			  comment,
			  type);
		    tracef("%s\tlength %2d variable: ",
			   comment,
			   size);
		    for (i = 0; i < size; i++) {
			tracef("%s%d", i ? "." : "", *pkt++ & 0xff);
		    }
		    trace(TR_SNMP, 0, NULL);
		    /* Fall through to display variables */
		case AGENT_RSPO:
		case AGENT_RSP:
		    if (*packet != AGENT_TRAP) {
			trace(TR_SNMP, 0, NULL);
		    }
		    while ((pkt - packet) <= length - 1) {
			switch (*pkt++) {
			    case INT:
				size = *pkt++;
				memcpy((char *) &integer, pkt, size);
				pkt += size;
				trace(TR_SNMP, 0, "%s\tinteger length %d value: %d %#x", comment, size, integer, integer);
				break;
			    case STR:
				size = *pkt++;
				tracef("%s\tstring length %d value: ", comment, size);
				for (; size; size--) {
				    tracef("%c", *pkt++);
				}
				trace(TR_SNMP, 0, NULL);
				break;
			    case IPADD:
				size = *pkt++;
				sockclear_in(&addr);
				memcpy((char *) &addr.sin_addr, pkt, size);
				pkt += size;
				trace(TR_SNMP, 0, "%s\tIP address length %d value: %A %#x",
				      comment,
				      size,
				      &addr,
				      addr.sin_addr.s_addr);
				break;
			    case CNTR:
				size = *pkt++;
				memcpy((char *) &integer, pkt, size);
				pkt += size;
				trace(TR_SNMP, 0, "%s\tcounter length %d value: %u %#x", comment, size, integer, integer);
				break;
			    case GAUGE:
				size = *pkt++;
				memcpy((char *) &integer, pkt, size);
				pkt += size;
				trace(TR_SNMP, 0, "%s\tgauge length %d value: %u %#x", comment, size, integer, integer);
				break;
			    case TIME:
				size = *pkt++;
				memcpy((char *) &integer, pkt, size);
				pkt += size;
				trace(TR_SNMP, 0, "%s\ttimer length %d value: %u %#x", comment, size, integer, integer);
				break;
			}
		    }
		    break;
		case AGENT_ERR:
		    if ((pkt - packet) < length) {
			cp = trace_state(snmp_errors, *pkt);
			if (!cp) {
			    cp = "???";
			}
			trace(TR_SNMP, 0, " error %s(%d)",
			      cp, *pkt);
		    }
		    break;
		case AGENT_REQ:
		case AGENT_REG:
		case AGENT_REQO:
		case AGENT_REQN:
		    trace(TR_SNMP, 0, NULL);
		    while ((pkt - packet) <= length - 1) {
			size = *pkt++;
			for (mib_ptr = (struct mibtbl *) tp->task_data; mib_ptr->function; mib_ptr++) {
			    if (size < mib_ptr->length) {
				continue;
			    }
			    if (!memcmp(pkt, mib_ptr->object, mib_ptr->length)) {
				tracef("%s\t%s",
				       comment,
				       mib_ptr->name);
				pkt += mib_ptr->length;
				for (i = mib_ptr->length; i < size; i++) {
				    tracef(".%d", *pkt++ & 0xff);
				}
				trace(TR_SNMP, 0, NULL);
				break;
			    }
			}
			if (!mib_ptr->function) {
			    tracef("%s\tlength %2d variable: ",
				   comment,
				   size);
			    for (i = 0; i < size; i++) {
				tracef("%s%d", i ? "." : "", *pkt++ & 0xff);
			    }
			    trace(TR_SNMP, 0, NULL);
			}
		    }
		    break;
	    }
	}
    } else {
	trace(TR_SNMP, 0, NULL);
    }
    trace(TR_SNMP, 0, NULL);
}


/*
 *  Send and trace an SNMP packet
 */
static void
snmp_send(tp, packet, size)
task *tp;
char *packet;
int size;
{
    TRACE_SNMPPKT(tp, "SEND", packet, size);

    (void) task_send_packet(tp,
			    packet,
			    size,
			    0,
			    (sockaddr_un *) 0);

    return;
}


/*
 *  Register all of our supported variables with SNMPD.
 */
/*ARGSUSED*/
static int
snmp_register(tp, p, write_only)
task *tp;
char *p;
flag_t write_only;
{
    int asize;
    struct mibtbl *mib_ptr;

    *p++ = write_only ? AGENT_REGSET : AGENT_REG;
    asize = 1;

    for (mib_ptr = (struct mibtbl *) tp->task_data; mib_ptr->function; mib_ptr++) {
	if (!write_only || (mib_ptr->flags & MIBF_WRITE)) {
	    *p++ = mib_ptr->length;
	    memcpy(p, mib_ptr->object, mib_ptr->length);
#ifdef	notdef
	    if (mib_ptr->flags & MIBF_WRITE) {
		*(p - 1)++;
		p[mib_ptr->length] = 0;
		p++;
		asize++;
	    }
#endif				/* notdef */
	    p += mib_ptr->length;
	    asize += mib_ptr->length + 1;
	}
    }

    return (asize);
}


/*
 *  Process an incoming request from SNMPD.  Speed is of the essence here.
 *  Not elegance.
 */
static void
snmp_recv(tp)
task *tp;
{
    int size;
    char agntreqpkt[SNMPMAXPKT];
    char *ptr = recv_iovec[RECV_IOVEC_DATA].iov_base;
    char *ptr1 = agntreqpkt;
    int error = 0;
    int retsize;
    int rspsize = 0, reqsize;
    struct mibtbl *mib_ptr;

    if (task_receive_packet(tp, &size)) {
	return;
    }
    TRACE_SNMPPKT(tp, "RECV", ptr, size);
    snmp_request_next = FALSE;
    snmp_request_object = FALSE;
    snmp_request_set = FALSE;
    switch (*ptr) {
	case AGENT_REG:
	case AGENT_RSP:
	case AGENT_ERR:
	case AGENT_RSPO:
	case AGENT_TRAP:
	    trace(TR_ALL, LOG_ERR, "snmp_recv: unexpected AGENT packet type");
	    return;
	case AGENT_REQN:
	    snmp_request_next = TRUE;
	case AGENT_REQO:
	    snmp_request_object = TRUE;
	case AGENT_SET:
	    if (*ptr == AGENT_SET) {
		snmp_request_set = TRUE;
	    }
	case AGENT_REQ:
	    if (snmp_request_object) {
		*ptr1++ = AGENT_RSPO;
		/* Setup space for returned object identifier */
		ASN_SET_TYPE(ptr1, IPADD);
		ASN_SET_LENGTH(ptr1, sizeof(struct in_addr));
		rspsize = ASN_LENGTH(ptr1) + 1;
		ASN_INCR(ptr1);
	    } else {
		*ptr1++ = AGENT_RSP;
		rspsize = 1;
	    }
	    ptr++;
	    reqsize = *ptr++;
	    for (mib_ptr = (struct mibtbl *) tp->task_data; mib_ptr->function; mib_ptr++) {
		if (mib_ptr->length > reqsize) {
		    continue;
		}
		if (memcmp(ptr, mib_ptr->object, mib_ptr->length) == 0) {
		    break;
		}
	    }
	    if (mib_ptr->function) {
		ptr += mib_ptr->length;
		if (!mib_ptr->function) {
		    /* No function for this variable */
		    error++;
		    retsize = NOSUCH;
		} else if (snmp_request_set && !(mib_ptr->flags & MIBF_WRITE)) {
		    /* Variable is not writable */
		    error++;
		    retsize = RDONLY;
		}
		if ((retsize = (mib_ptr->function) (mib_ptr->object[mib_ptr->length - 1],
						    ptr,
						    ptr1,
				      reqsize - mib_ptr->length)) <= 0) {
		    error++;
		    if (retsize < 0) {
			/* Error code is returned negative */
			retsize = -retsize;
		    } else {
			/* No error - assume generic */
			retsize = -GENERRS;
		    }
		} else {
		    rspsize += retsize;
		}
	    }
	    if (error) {
		agntreqpkt[0] = AGENT_ERR;
		agntreqpkt[1] = retsize;
		rspsize = 2;
	    }
	    break;
	case AGENT_QUERY:
	    rspsize = snmp_register(tp, agntreqpkt, TRUE);
	    break;
	default:
	    trace(TR_SNMP, LOG_ERR, "snmp_recv: invalid AGENT packet type");
	    agntreqpkt[0] = AGENT_ERR;
	    rspsize = 1;
	    break;
    }					/* switch */

    snmp_send(tp, agntreqpkt, rspsize);
    return;
}


/*
 *  Register all of our supported variables with SNMPD.
 */
/*ARGSUSED*/
static void
snmp_job(tip, interval)
timer *tip;
time_t interval;
{
    int size;
    char agntpkt[SNMPMAXPKT];

    size = snmp_register(tip->timer_task, agntpkt, FALSE);

    snmp_send(tip->timer_task, agntpkt, size);
}


static void
snmp_terminate(tp)
task *tp;
{
    snmp_task = (task *) 0;
    task_delete(tp);
}


static int
ipRoute(object, src, dst, size)
int object;
char *src, *dst;
int size;
{
    int integer;
    rt_entry *rt = 0;

    if (!size && snmp_request_object) {
	rt = rt_next((sockaddr_un *) 0);
    } else if (size == sizeof(struct in_addr)) {
	struct sockaddr_in dest;

	sockclear_in(&dest);
	memcpy((char *) &dest.sin_addr, src, size);

	if (snmp_request_next) {
	    rt = rt_next((sockaddr_un *) & dest);
	} else {
	    rt = rt_lookup(RTS_NETROUTE | RTS_HOSTROUTE, (sockaddr_un *) & dest);
	    if (!rt && (dest.sin_addr.s_addr == (u_long) DEFAULTNET)) {
		rt = rt_next((sockaddr_un *) & dest);
	    }
	}
    }
    if (!rt) {
	return (-NOSUCH);
    }
    if (snmp_request_object) {
	ASN_SET_VALUE((dst - sizeof(long) - 2), &rt->rt_dest.in.sin_addr);
    }
    switch (object) {
	case ipRouteDest:
	    ASN_SET_TYPE(dst, IPADD);
	    ASN_SET_LENGTH(dst, sizeof(rt->rt_dest.in.sin_addr));
	    ASN_SET_VALUE(dst, &rt->rt_dest.in.sin_addr);
	    break;

	case ipRouteIfIndex:
	    integer = rt->rt_ifp->int_index;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteMetric1:
	    integer = rt->rt_metric;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteMetric2:
	case ipRouteMetric3:
	case ipRouteMetric4:
	case ipRouteMetric5:
	    integer = -1;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteNextHop:
	    ASN_SET_TYPE(dst, IPADD);
	    ASN_SET_LENGTH(dst, sizeof(rt->rt_router.in.sin_addr));
	    ASN_SET_VALUE(dst, &rt->rt_router.in.sin_addr);
	    break;

	case ipRouteType:
	    if (rt->rt_state & RTS_HOLDDOWN) {
		integer = 2;
	    } else {
		switch (rt->rt_proto) {
		    case RTPROTO_DIRECT:
			integer = 3;
			break;
		    default:
			integer = 4;
		}
	    }
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteProto:
	    switch (rt->rt_proto) {
		case RTPROTO_RIP:
		    integer = 8;
		    break;
		case RTPROTO_HELLO:
		    integer = 7;
		    break;
		case RTPROTO_EGP:
		    integer = 5;
		    break;
		case RTPROTO_DIRECT:
		    integer = 1;
		    break;
		case RTPROTO_REDIRECT:
		    integer = 4;
		    break;
		case RTPROTO_DEFAULT:
		    integer = 2;
		    break;
		case RTPROTO_IGP:
		    integer = 9;
		    break;
		case RTPROTO_IGRP:
		    integer = 11;
		    break;
		case RTPROTO_OSPF:
		    integer = 13;
		    break;
		case RTPROTO_BGP:
		    integer = 14;
		    break;
		case RTPROTO_STATIC:
		    integer = 2;
		    break;
		default:
		    integer = 1;
	    }
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteAge:
	    integer = rt->rt_timer;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case ipRouteMask:
	    ASN_SET_TYPE(dst, IPADD);
	    ASN_SET_LENGTH(dst, sizeof(rt->rt_dest_mask.in.sin_addr));
	    ASN_SET_VALUE(dst, &rt->rt_dest_mask.in.sin_addr);
	    break;

	default:
	    return (-NOSUCH);
    }

    return (ASN_LENGTH(dst));
}



/*ARGSUSED*/
static int
egp(object, src, dst, size)
int object;
char *src, *dst;
u_int size;
{
    u_int counter;
    int integer;

    if (size) {
	return (-NOSUCH);
    }
    if (!(doing_egp)) {
	return (-NOSUCH);
    }
    switch (object) {
	case egpInMsgs:
	    counter = egp_stats.inmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpInErrors:
	    counter = egp_stats.inerrors;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpOutMsgs:
	    counter = egp_stats.outmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpOutErrors:
	    counter = egp_stats.outerrors;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpAs:
	    integer = my_system;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	default:
	    return (-NOSUCH);
    }

    return (ASN_LENGTH(dst));
}



static int
egpNeigh(object, src, dst, size)
int object;
char *src, *dst;
int size;
{
    int neighbor = egp_neighbors;
    int integer;
    u_int counter;
    struct egpngh *ngp;

    if (!(doing_egp)) {
	return (-NOSUCH);
    }
    if (!size && snmp_request_next) {
	neighbor = 0;
    } else if (size == sizeof(struct in_addr)) {
	struct sockaddr_in dest;

	sockclear_in(&dest);
	memcpy((char *) &dest.sin_addr, src, size);

	if (dest.sin_addr.s_addr != (u_long) DEFAULTNET) {
	    for (neighbor = 0; neighbor < egp_neighbors; neighbor++) {
		if (equal(&egp_sort[neighbor]->ng_addr, &dest.sin_addr)) {
		    break;
		}
	    }
	    if (snmp_request_next) {
		neighbor++;
	    }
	} else {
	    neighbor = snmp_request_next ? 0 : egp_neighbors;
	}
    }
    if (neighbor >= egp_neighbors) {
	return (-NOSUCH);
    }
    ngp = egp_sort[neighbor];

    if (snmp_request_object) {
	ASN_SET_VALUE((dst - sizeof(ngp->ng_addr.sin_addr) - 2), &ngp->ng_addr.sin_addr);
    }
    switch (object) {
	case egpNeighState:
	    integer = ngp->ng_state + 1;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case egpNeighAddr:
	    ASN_SET_TYPE(dst, IPADD);
	    ASN_SET_LENGTH(dst, sizeof(ngp->ng_addr.sin_addr));
	    ASN_SET_VALUE(dst, &ngp->ng_addr.sin_addr);
	    break;

	case egpNeighAs:
	    integer = ngp->ng_asin;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case egpNeighInMsgs:
	    counter = ngp->ng_stats.inmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighInErrs:
	    counter = ngp->ng_stats.inerrors;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighOutMsgs:
	    counter = ngp->ng_stats.outmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighOutErrs:
	    counter = ngp->ng_stats.outerrors;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighInErrMsgs:
	    counter = ngp->ng_stats.inerrmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighOutErrMsgs:
	    counter = ngp->ng_stats.outerrmsgs;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighStateUps:
	    counter = ngp->ng_stats.stateups;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighStateDowns:
	    counter = ngp->ng_stats.statedowns;
	    ASN_SET_TYPE(dst, CNTR);
	    ASN_SET_LENGTH(dst, sizeof(counter));
	    ASN_SET_VALUE(dst, &counter);
	    break;

	case egpNeighIntervalHello:
	    integer = ngp->ng_T1;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case egpNeighIntervalPoll:
	    integer = ngp->ng_T2;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

	case egpNeighMode:
	    integer = ngp->ng_M;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

#define	egpNeighEventTriggerStart	1
#define	egpNeighEventTriggerStop	2
	case egpNeighEventTrigger:
	    if (!ngp->ng_stats.trigger) {
		ngp->ng_stats.trigger = egpNeighEventTriggerStop;
	    }
	    integer = ngp->ng_stats.trigger;
	    ASN_SET_TYPE(dst, INT);
	    ASN_SET_LENGTH(dst, sizeof(integer));
	    ASN_SET_VALUE(dst, &integer);
	    break;

    }

    return (ASN_LENGTH(dst));
}



static void
snmp_trap(trap, ptr)
int trap;
char *ptr;
{
    int i;
    struct egpngh *ngp = (struct egpngh *) ptr;
    if_entry *ifp = (if_entry *) ptr;
    static objident ifIndex =
    {10,
     {1, 3, 6, 1, 2, 1, 2, 2, 1, 1}};
    static objident egpNeighAddrObj =
    {10,
     {1, 3, 6, 1, 2, 1, 8, 5, 1, 2}};
    objident *obj_ptr;
    char *val_ptr;
    int val_size;
    int val_type;
    char trap_pkt[SNMPMAXPKT];
    int trap_size = 0;
    char *trap_ptr = trap_pkt;

    if (!snmp_task) {
	/* Return if SNMP not active */
	return;
    }
    switch (trap) {
	case LINKDOWN:
	case LINKUP:
	    obj_ptr = &ifIndex;
	    val_ptr = (char *) &ifp->int_index;
	    val_size = 1;
	    val_type = INT;
	    break;
	case EGPNHBRLOST:
	    obj_ptr = &egpNeighAddrObj;
	    val_ptr = (char *) &ngp->ng_addr;
	    val_size = sizeof(ngp->ng_addr);
	    val_type = IPADD;
	    break;
	default:
	    trace(TR_SNMP, LOG_ERR, "snmp_trap: Unknown SNMP trap type: %d", trap);
	    return;
    }

    *trap_ptr++ = AGENT_TRAP;
    ASN_SET_TYPE(trap_ptr, trap);
    ASN_SET_LENGTH(trap_ptr, obj_ptr->ncmp + val_size);
    for (i = 0; i < obj_ptr->ncmp; i++) {
	trap_ptr[i + 2] = obj_ptr->cmp[i] & 0xff;
    }
    memcpy(&trap_ptr[obj_ptr->ncmp + 2], (char *) val_ptr, val_size);
    ASN_INCR(trap_ptr);
    ASN_SET_TYPE(trap_ptr, val_type);
    ASN_SET_LENGTH(trap_ptr, val_size);
    ASN_SET_VALUE(trap_ptr, val_ptr);
    ASN_INCR(trap_ptr);
    trap_size = trap_ptr - trap_pkt;

    snmp_send(snmp_task, trap_pkt, trap_size);
    return;
}


void
snmp_trap_egpNeighborLoss(ngp)
struct egpngh *ngp;
{
    snmp_trap(EGPNHBRLOST, (char *) ngp);
}


/*ARGSUSED*/
static void
snmp_trap_ifchange(tp, ifp)
task *tp;
if_entry *ifp;
{
    snmp_trap((ifp->int_state & IFS_UP) ? LINKUP : LINKDOWN, (char *) ifp);
}


void
snmp_init()
{
    struct mibtbl *mib_ptr = snmptbl;

    if (!doing_snmp) {
	if (snmp_task) {
	    snmp_terminate(snmp_task);
	}
	return;
    } else if (snmp_task) {
	return;
    }
    
    snmp_task = task_alloc("SNMP");
    sockclear_in(&snmp_task->task_addr.in);
    snmp_task->task_addr.in.sin_port = htons(AGENT_SNMP_PORT);
    snmp_task->task_addr.in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    snmp_task->task_recv = snmp_recv;
    snmp_task->task_terminate = snmp_terminate;
    snmp_task->task_ifchange = snmp_trap_ifchange;
    snmp_task->task_data = (caddr_t) mib_ptr;

    if ((snmp_task->task_socket = task_get_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	quit(errno);
    }
    if (!task_create(snmp_task, SNMPMAXPKT)) {
	quit(EINVAL);
    }
    if (task_set_option(snmp_task, TASKOPTION_RECVBUF, (caddr_t) (32 * 1024)) < 0) {
	quit(errno);
    }
    if (task_set_option(snmp_task, TASKOPTION_DONTROUTE, (caddr_t) TRUE) < 0) {
	quit(errno);
    }
    if (!test_flag) {
	struct sockaddr_in addr;

	sockclear_in(&addr);
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(snmp_task->task_socket, (struct sockaddr *) & addr, socksize(&addr)) < 0) {
	    trace(TR_ALL, LOG_ERR, "snmp_init: bind: %m");
	    quit(errno);
	}
	if (connect(snmp_task->task_socket, &snmp_task->task_addr.a, socksize(&snmp_task->task_addr.a)) < 0) {
	    trace(TR_ALL, LOG_ERR, "snmp_init: connect: %m");
	    quit(errno);
	}
    }
    (void) timer_create(snmp_task, 0, "Register", 0, (time_t) SNMP_REGISTER_INTERVAL, snmp_job);

    for (; mib_ptr->function; mib_ptr++) {
	if (!mib_ptr->length) {
	    mib_ptr->length = strlen(mib_ptr->object);
	}
    }

}


#endif				/* AGENT_SNMP */
