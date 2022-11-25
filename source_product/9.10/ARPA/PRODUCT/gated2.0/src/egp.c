/*
 *  $Header: egp.c,v 1.1.109.5 92/02/28 14:01:40 ash Exp $
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
#include "snmp.h"

#ifdef	PROTO_EGP

static void egp_event_up();
static void egp_event_down();

/*
 *	Format an EGP network update packet
 */
static void
egp_trace_NR(nr, nr_length)
struct egpnr *nr;
int nr_length;
{
    int gateways, distances, networks;
    int distance, t_gateways;
    int shared_net_class, net_class;
    char class;
    u_char *nr_ptr, *nr_end;
    struct sockaddr_in gateway, network;

    sockclear_in(&gateway);

    shared_net_class = gd_inet_class((u_char *) & nr->en_net);
    memcpy((char *) &gateway.sin_addr, (char *) &nr->en_net, shared_net_class);
    nr_ptr = (u_char *) nr + sizeof(struct egpnr);
    nr_end = (u_char *) nr + nr_length;
    gateway.sin_addr = nr->en_net;	/* struct copy */
    trace(TR_EGP | TR_NOSTAMP, 0, "\tnet %A (%c) - %d interior gateways, %d exterior gateways",
	  &gateway,
	  'A' - 1 + shared_net_class,
	  nr->en_igw,
	  nr->en_egw);

    t_gateways = nr->en_igw + nr->en_egw;
    for (gateways = 0; gateways < t_gateways; gateways++) {
	memcpy((char *) &gateway.sin_addr + shared_net_class, (char *) nr_ptr, 4 - shared_net_class);
	nr_ptr += 4 - shared_net_class;
	distances = (u_char) * nr_ptr;
	nr_ptr++;
	trace(TR_EGP | TR_NOSTAMP, 0, "\t\t%s gateway %A, %d distances",
	      gateways < nr->en_igw ? "interior" : "exterior",
	      &gateway,
	      distances);
	for (; distances; distances--) {
	    distance = (u_char) * nr_ptr;
	    nr_ptr++;
	    networks = (u_char) * nr_ptr;
	    nr_ptr++;
	    trace(TR_EGP | TR_NOSTAMP, 0, "\t\t\tdistance %d, %d networks",
		  distance, networks);
	    for (; networks; networks--) {
		sockclear_in(&network);
		if ((net_class = gd_inet_class(nr_ptr)) == 0) {
		    net_class = CLAC;
		    class = '?';
		} else {
		    class = 'A' - 1 + net_class;
		}
		memcpy((char *) &network.sin_addr, (char *) nr_ptr, net_class);
		nr_ptr += net_class;
		trace(TR_EGP | TR_NOSTAMP, 0, "\t\t\t\t(%c) %A",
		      class,
		      &network);
		if (nr_ptr > nr_end) {
		    trace(TR_EGP | TR_NOSTAMP, 0, "\tpremature end of packet\n");
		    return;
		}
	    }
	}
    }
    trace(TR_EGP, 0, "end of packet");
    return;
}


/*
 * Trace EGP packet
 */
static void
egp_trace(ngp, comment, send_flag, egp, length)
struct egpngh *ngp;
char *comment;
int send_flag;
struct egppkt *egp;
int length;
{
    struct egppkt *ep;
    int reason;
    char packet_status;
    const char *type = (char *) 0;
    const char *status = (char *) 0;
    const char *code = (char *) 0;
    static const char *no_codes[1] =
    {"0"};
    static struct {
	const char *et_type;
	int et_ncodes;
	const char **et_codes;
	int et_nstatus;
	const char **et_status;
    } egp_types[9] = {
	"Invalid", -1, (const char **) 0, -1, (const char **) 0,	/* 0 - Error */
	"Update", 0, no_codes, 3, egp_nr_status,	/* 1 - Nets Reachable */
	"Poll", 0, no_codes, 3, egp_nr_status,	/* 2 - Poll */
	"Acquire", 5, egp_acq_codes, 7, egp_acq_status,	/* 3 - Neighbor Aquisition */
	"Invalid", -1, (const char **) 0, -1, (const char **) 0,	/* 4 - Error */
	"Neighbor", 2, egp_reach_codes, 3, egp_nr_status,	/* 5 - Neighbor Reachability */
	"Invalid", -1, (const char **) 0, -1, (const char **) 0,	/* 6 - Error */
	"Invalid", -1, (const char **) 0, -1, (const char **) 0,	/* 7 - Error */
	"ERROR", -1, (const char **) 0, 3, egp_nr_status	/* 8 - Error packet */
    };

    trace(TR_EGP, 0, "%s %A -> %A length %d",
	  comment,
	  send_flag ? &ngp->ng_interface->int_addr.in : &ngp->ng_addr,
	  send_flag ? &ngp->ng_addr : &ngp->ng_interface->int_addr.in,
	  length);

    if (egp->egp_type <= EGPERR) {
	type = egp_types[egp->egp_type].et_type;
	if ((short) egp->egp_code <= egp_types[egp->egp_type].et_ncodes) {
	    code = egp_types[egp->egp_type].et_codes[egp->egp_code];
	} else {
	    if (egp->egp_code == 0) {
		code = "";
	    } else {
		code = "Invalid";
	    }
	}
	packet_status = egp->egp_status % UNSOLICITED;
	if (packet_status <= egp_types[egp->egp_type].et_nstatus) {
	    status = egp_types[egp->egp_type].et_status[packet_status];
	} else {
	    status = "Invalid";
	}
    } else {
	type = "Invalid";
    }
    tracef("%s vers %d, type %s(%d), code %s(%d), status %s(%d)%s, AS %d, id %d",
	   comment,
	   egp->egp_ver,
	   type,
	   egp->egp_type,
	   code,
	   egp->egp_code,
	   status,
	   egp->egp_status,
	   egp->egp_status & UNSOLICITED ? " Unsolicited" : "",
	   ntohs(egp->egp_system),
	   ntohs(egp->egp_id));
    if (length >= sizeof(struct egppkt)) {
	switch (egp->egp_type) {
	    case EGPACQ:
		if (length == sizeof(struct egpacq)) {
		    trace(TR_EGP, 0, ", hello %d, poll %d",
			  ntohs(((struct egpacq *) egp)->ea_hint),
			  ntohs(((struct egpacq *) egp)->ea_pint));
		}
		break;
	    case EGPPOLL:
		if (length >= sizeof(struct egppoll)) {
		    struct sockaddr_in addr;

		    sockclear_in(&addr);
		    addr.sin_addr = ((struct egppoll *) egp)->ep_net;

		    trace(TR_EGP, 0, ", src net %A",
			  &addr);
		}
		break;
	    case EGPNR:
		if (length >= sizeof(struct egpnr)) {
		    struct sockaddr_in addr;

		    sockclear_in(&addr);
		    addr.sin_addr = ((struct egpnr *) egp)->en_net;
		    trace(TR_EGP, 0, ", #int %d, #ext %d, src net %A",
			  ((struct egpnr *) egp)->en_igw,
			  ((struct egpnr *) egp)->en_egw,
			  &addr);
		} else if (length >= (sizeof(struct egpnr) - sizeof(struct in_addr))) {
		    trace(TR_EGP, 0, ", #int %d, #ext %d",
			  ((struct egpnr *) egp)->en_igw,
			  ((struct egpnr *) egp)->en_egw);
		}
		if (length > sizeof(struct egpnr) && (trace_flags & TR_UPDATE)) {
		    egp_trace_NR((struct egpnr *) egp, length);
		}
		break;
	    case EGPERR:
		reason = ntohs(((struct egperr *) egp)->ee_rsn);
		if (reason > EMAXERR) {
		    trace(TR_EGP, 0, ", error %d (invalid)", reason);
		} else {
		    trace(TR_EGP, 0, ", error: %s(%d)", egp_reasons[reason], reason);
		}
		ep = (struct egppkt *) ((struct egperr *) egp)->ee_egphd;
		if (length >= sizeof(struct egperr)) {
		    char e_comment[MAXHOSTNAMELENGTH];

		    (void) strcpy(e_comment, comment);
		    (void) strcat(e_comment, " ERROR");
		    egp_trace(ngp, e_comment, send_flag, ep, sizeof(((struct egperr *) egp)->ee_egphd));
		}
		break;
	    case EGPHELLO:
	    default:
		trace(TR_EGP, 0, NULL);
	}
    }
    trace(TR_EGP, 0, NULL);
    return;
}


static void
egp_msg_event(ngp, string)
struct egpngh *ngp;
char *string;
{

    trace(TR_EGP, 0, "egp_msg_event: neighbor %s version %d state %s event %s",
	  ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state), string);
}


static void
egp_msg_state(ngp, state)
struct egpngh *ngp;
int state;
{

    trace(TR_EGP, 0, "egp_msg_state: neighbor %s version %d state %s transition to %s",
	  ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state), trace_state(egp_states, state));
}


static void
egp_msg_confused(ngp, event)
struct egpngh *ngp;
char *event;
{
    trace(TR_EGP, 0, "egp_msg_confused: neighbor %s version %d event %s should not occur in state %s",
	  ngp->ng_name, ngp->ng_V, event, trace_state(egp_states, ngp->ng_state));
}


static void
egp_msg_timer(tip)
timer *tip;
{
    struct egpngh *ngp;

    ngp = (struct egpngh *) tip->timer_task->task_data;

    tracef("egp_msg_timer: neighbor %s version %d state %s timer %s ",
	   ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state), tip->timer_name);
    if (tip->timer_interval) {
	trace(TR_EGP, 0, "reset to %#T at %T ", tip->timer_interval, tip->timer_next_time);
    } else {
	trace(TR_EGP, 0, "stopped");
    }
}


/*
 *	Set new version and print a message
 */
static void
egp_set_version(ngp, egp_version)
struct egpngh *ngp;
u_char egp_version;
{
    trace(TR_EGP, LOG_NOTICE, "egp_set_version: neighbor %s version %d state %s set version %d",
	  ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state), egp_version);
    ngp->ng_V = egp_version;
}


/*
 *	Routines to deal with maxacquire limits
 */
static int
egp_group_acquired(ngp)
struct egpngh *ngp;
{
    int acquired = 0;
    struct egpngh *tngp;

    for (tngp = ngp->ng_gr_head; tngp; tngp = tngp->ng_next) {
	if (tngp->ng_gr_index != ngp->ng_gr_index) {
	    break;
	}
	if (tngp->ng_state == NGS_UP) {
	    acquired++;
	}
    }

    return (acquired);
}


static void
egp_group_checkmax(ngp)
struct egpngh *ngp;
{
    u_short acquired = 0;
    u_short acquire;
    struct egpngh *tngp;

    acquire = ngp->ng_gr_head->ng_gr_acquire;

    for (tngp = ngp->ng_gr_head; tngp; tngp = tngp->ng_next) {
	if (tngp->ng_gr_index != ngp->ng_gr_index) {
	    break;
	}
#ifdef	DEBUG
	trace(TR_INT, LOG_WARNING, "egp_group_checkmax: neighbor %s state %s acquired %d acquire %d",
	      tngp->ng_name, trace_state(egp_states, tngp->ng_state), acquired, acquire);
#endif				/* DEBUG */
	switch (tngp->ng_state) {
	    case NGS_IDLE:
	    case NGS_CEASE:
		break;
	    case NGS_UP:
	    case NGS_ACQUISITION:
	    case DOWN:
		if (acquired >= acquire) {
		    egp_event_stop(tngp, GODOWN);
		}
		if (tngp->ng_state == NGS_UP) {
		    acquired++;
		}
		break;
	}
    }
}


/*
 *  egp_check_as() is called when we loose reachability to
 *  a neighbor.  It scans the neighbor list to determine if there
 *  are any other active neighbors to this AS.  It should probably
 *  delete any EGP learned routes from the AS of the neighbor.
 */
static int
egp_check_as(down_ngp)
struct egpngh *down_ngp;
{
    struct egpngh *ngp;

    EGP_LIST(ngp) {
	if ((ngp != down_ngp) &&
	    (ngp->ng_asin == down_ngp->ng_asin) &&
	    ((ngp->ng_state == NGS_UP) || (ngp->ng_state == NGS_DOWN))) {
	    return 0;
	}
    } EGP_LISTEND;
    trace(TR_EXT, LOG_WARNING, "egp_check_as: lost all neighbors to AS %d",
	  down_ngp->ng_asin);
    return 1;
}


/*
 * egp_check_neighborLoss() handles the loss of a neighbor.  It deletes any
 * routes in the routing table pointing at this gateway, calls egp_check_as()
 * to determine if we lost all neighbors for this AS.
 */
static void
egp_check_neighborLoss(ngp)
struct egpngh *ngp;
{
    int changes = 0;

    if ((ngp->ng_state != NGS_UP) && (ngp->ng_state != NGS_DOWN)) {
	return;
    }
#ifdef	AGENT_SNMP
    snmp_trap_egpNeighborLoss(ngp);
#endif				/* AGENT_SNMP */

    changes += egp_check_as(ngp);	/* check for other direct neighbors in this AS */
    changes += rt_gwunreach(ngp->ng_task, &ngp->ng_gw);	/* delete routes for down gateway */
    if (ngp->ng_flags & NGF_GENDEFAULT) {
	changes += rt_default_delete();
	ngp->ng_flags &= ~NGF_GENDEFAULT;
    }
    if (changes) {
	trace(TR_RT, 0, "egp_check_neighborLoss: above changes due to loss of neighbor %s", ngp->ng_name);
    }
}


/*
 *	Routines to change state
 */
static void
egp_state_idle(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_state(ngp, NGS_IDLE);

    egp_check_neighborLoss(ngp);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_IDLE;

    /* If this task is being deleted, issue a deletion event */
    if (ngp->ng_flags & NGF_DELETE) {
	egp_event_delete(ngp);
    }
}


static void
egp_state_acquisition(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_state(ngp, NGS_ACQUISITION);

    egp_check_neighborLoss(ngp);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_ACQUISITION;
    ngp->ng_status = 0;
}


/*
 * egp_state_down() Set down state.
 */
static void
egp_state_down(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_state(ngp, NGS_DOWN);

    if (ngp->ng_state != NGS_UP) {
	trace(TR_EXT, LOG_WARNING, "egp_state_down: acquired neighbor %s AS %d in %s",
	      ngp->ng_name,
	      ngp->ng_asin,
	      egp_acq_status[ngp->ng_M]);
    }
    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_DOWN;

}


static void
egp_state_up(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_state(ngp, NGS_UP);

    ngp->ng_state = NGS_UP;

    egp_group_checkmax(ngp);

    ngp->ng_stats.stateups++;
}


/*
 * egp_state_cease() initiates the sending of an egp neighbor cease.
 */
static void
egp_state_cease(ngp)
struct egpngh *ngp;
{

    egp_check_neighborLoss(ngp);

    IF_EGPPROTO egp_msg_state(ngp, NGS_CEASE);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_CEASE;

    return;
}


/*
 *	Routines to send packets
 */

/*
 * egp_send() sends an egp packet.
 */
static void
egp_send(ngp, egp, length)
struct egpngh *ngp;
struct egppkt *egp;			/* pointer to start of egp packet */
int length;				/* length in octets of egp packet */
{
    int error = FALSE;
    struct iovec iovec;

    /* Set up iovec */
    iovec.iov_base = (caddr_t) egp;
    iovec.iov_len = length;

    /* Set AS number in outgoing packet */
    egp->egp_system = htons((unsigned short) (ngp ? ngp->ng_asout : my_system));

    /* Set version in outgoing packet */
    egp->egp_ver = ngp ? ngp->ng_V : EGPVER;

    /* Calculate packet checksum */
    egp->egp_chksum = 0;
    egp->egp_chksum = gd_inet_cksum(&iovec, 1, length);

    if (trace_flags & TR_EGP) {
	egp_trace(ngp, "EGP SENT", TRUE, egp, length);
    }
    if (task_send_packet(ngp->ng_task, (caddr_t) egp, length, 0, (sockaddr_un *) 0) < 0) {
	error = TRUE;
    }
    egp_stats.outmsgs++;
    ngp->ng_stats.outmsgs++;
    if (error) {
	egp_stats.outerrors++;
	ngp->ng_stats.outerrors++;
    }
}


/*
 * egp_send_acquire() sends an acquisition or cease packet.
 */
static void
egp_send_acquire(ngp, code, status, id)
struct egpngh *ngp;
u_int code, status;
u_int id;
{
    struct egpacq acqpkt;
    int length;

    acqpkt.ea_pkt.egp_type = EGPACQ;
    acqpkt.ea_pkt.egp_code = code;
    acqpkt.ea_pkt.egp_status = status;
    acqpkt.ea_pkt.egp_id = htons(id);
    acqpkt.ea_hint = htons(ngp->ng_P1);
    acqpkt.ea_pint = htons(ngp->ng_P2);

    if (code == NAREQ || code == NACONF) {
	length = sizeof(acqpkt);
    } else {
	/* omit hello & poll int */
	length = sizeof(acqpkt.ea_pkt);
    }

    egp_send(ngp, (struct egppkt *) & acqpkt, length);
}


/*
 * egp_send_hello() sends a hello or I-H-U packet.
 */
static void
egp_send_hello(ngp, code, id)
struct egpngh *ngp;
u_char code;
u_int id;
{
    struct egppkt hellopkt;

    hellopkt.egp_type = EGPHELLO;
    hellopkt.egp_code = code;
    hellopkt.egp_status = (ngp->ng_state == NGS_UP) ? UP : DOWN;
    hellopkt.egp_id = htons(id);

    if (code == neighHello) {
	/* Remember the ID of this Hello */
	ngp->ng_S_lasthello = id;
    }
    
    egp_send(ngp, (struct egppkt *) & hellopkt,
	     sizeof(hellopkt));
}


/*
 * egp_send_poll() sends an NR poll packet.
 */
static void
egp_send_poll(ngp)
struct egpngh *ngp;
{
    struct egppoll pollpkt;

    pollpkt.ep_pkt.egp_type = EGPPOLL;
    pollpkt.ep_pkt.egp_code = 0;
    pollpkt.ep_pkt.egp_status = (ngp->ng_state == NGS_UP) ? UP : DOWN;
    pollpkt.ep_pkt.egp_id = htons(ngp->ng_S);
    pollpkt.ep_unused = 0;
    pollpkt.ep_net = ngp->ng_saddr.sin_addr;	/* struct copy */

    egp_send(ngp, (struct egppkt *) & pollpkt, sizeof(pollpkt));
}


/*
 * egp_send_error() sends an error packet.
 */
static void
egp_send_error(ngp, egp, length, error, msg)
struct egpngh *ngp;			/* ponter to legit. neighbor table, else zero */
struct egppkt *egp;			/* erroneous egp packet */
int length;				/* length erroneous packet */
int error;
char *msg;
{
    struct egperr errpkt;

    errpkt.ee_pkt.egp_type = EGPERR;
    errpkt.ee_pkt.egp_code = (error == EUVERSION) ? EGPVMASK : 0;
    if (ngp && ((ngp->ng_state == NGS_UP) || (ngp->ng_state == NGS_DOWN))) {
	errpkt.ee_pkt.egp_status = (ngp->ng_state == NGS_UP) ? UP : DOWN;
    } else {
	errpkt.ee_pkt.egp_status = 0;
    }
    errpkt.ee_pkt.egp_id = htons(egprid_h);	/* recvd seq.# */
    errpkt.ee_rsn = htons(error);
    /*
     * copy header of erroneous egp packet
     */
    memset((char *) errpkt.ee_egphd, (char) 0, sizeof(errpkt.ee_egphd));
    if (length > sizeof(errpkt.ee_egphd)) {
	length = sizeof(errpkt.ee_egphd);
    }
    if (length) {
	memcpy((char *) errpkt.ee_egphd, (char *) egp, length);
    } else {
	errpkt.ee_pkt.egp_status |= UNSOLICITED;
    }

    trace(TR_EXT, LOG_WARNING, "egp_send_error: error packet to neighbor %s: %s",
	  ngp->ng_name,
	  msg);

    if ((trace_flags & TR_EXT) && !(trace_flags & TR_EGP)) {
	egp_trace(ngp, "egp_send_error: send error pkt ", TRUE, (struct egppkt *) & errpkt, sizeof(errpkt));
    }
    ngp->ng_stats.outerrmsgs++;
    egp_send(ngp, (struct egppkt *) & errpkt, sizeof(errpkt));
}


/*
 * egp_send_update() sends an NR message packet.
 *
 * It fills in the header information, calls if_rtcheck() to update the
 * interface status information and egp_rt_send() to fill in the reachable
 * networks.
 */

static void
egp_send_update(ngp, unsol)
struct egpngh *ngp;
int unsol;				/* TRUE => set unsolicited bit */
{
    int maxsize, length;
    struct egpnr *nrp;
    struct in_addr egpsnraddr;

    /*
     * allocate message buffer
     */
    maxsize = sizeof(struct egpnr) + NRMAXNETUNIT * (n_interfaces + rt_net_routes);
    nrp = (struct egpnr *) malloc((unsigned) maxsize);
    if (nrp == NULL) {
	trace(TR_ALL, LOG_ERR, "egp_send_update: malloc: %m");
	return;
    }
    /* prepare static part of NR message header */
    nrp->en_pkt.egp_type = EGPNR;
    nrp->en_pkt.egp_code = 0;
    nrp->en_pkt.egp_status = (ngp->ng_state == NGS_UP) ? UP : DOWN;
    if (unsol) {
	nrp->en_pkt.egp_status |= UNSOLICITED;
    }
    nrp->en_pkt.egp_id = htons(ngp->ng_R);
#ifdef	notdef
    nrp->en_egw = 0;			/* no exterior gateways */
#endif				/* notdef */
    /*
     * copy shared net address
     */
    egpsnraddr = gd_inet_makeaddr(gd_inet_wholenetof(ngp->ng_paddr.sin_addr), 0, FALSE);
    nrp->en_net = egpsnraddr;
    length = egp_rt_send(nrp, ngp);

    if (length != ERROR) {
	if (length > egp_pktsize) {
	    trace(TR_ALL, LOG_WARNING, "egp_send_update: neighbor %s AS %d NR message size (%d) larger than EGPMAXPACKETSIZE (%d)",
		  ngp->ng_name,
		  ngp->ng_asin,
		  length,
		  egp_pktsize);
	}
	egp_send(ngp, (struct egppkt *) nrp, length);
    } else {
	trace(TR_ALL, LOG_WARNING, "egp_send_update: NR message not sent");
    }
    free((char *) nrp);
    return;
}


/*
 *	Front end for task timer routines
 */
static void
egp_set_timer(tip, value)
timer *tip;
time_t value;
{

    timer_interval(tip, value);
    IF_EGPPROTO egp_msg_timer(tip);
}


static void
egp_reset_timer(tip, value)
timer *tip;
time_t value;
{

    timer_set(tip, value);
    IF_EGPPROTO egp_msg_timer(tip);
}


/*
 *	Routines to process reachability
 */

/*
 *  egp_check_reachability() checks the reachability status of our neighbors
 */
static void
egp_check_reachability(ngp)
struct egpngh *ngp;
{
    int change = 0;

    IF_EGPPROTO trace(TR_EGP, 0, "egp_check_reachability: neighbor %s version %d state %s [%04B] %d / %d / %d",
	 ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state),
		       ngp->ng_responses, ngp->ng_j, egp_reachability[ngp->ng_responses], ngp->ng_k);

    switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_CEASE:
	    egp_msg_confused(ngp, "ReachabilityCheck");
	    break;
	case NGS_DOWN:
	    if (egp_reachability[ngp->ng_responses] >= ngp->ng_j) {
		egp_event_up(ngp);
		change++;
	    }
	    break;
	case NGS_UP:
	    if (egp_reachability[ngp->ng_responses] <= ngp->ng_k) {
		egp_event_down(ngp);
		change++;
	    }
	    break;
    }
    if (change) {
	trace(TR_EXT, LOG_WARNING, "egp_check_reachability: neighbor %s AS %d state %s received %d of %d %s",
	      ngp->ng_name,
	      ngp->ng_asin,
	      trace_state(egp_states, ngp->ng_state),
	      egp_reachability[ngp->ng_responses],
	      REACH_RATIO,
	      (ngp->ng_M == ACTIVE) ? "responses" : "requests");
	IF_EGPPROTO trace(TR_EGP, 0, "egp_check_reachability: neighbor %s version %d state %s [%04B] %d / %d / %d",
	 ngp->ng_name, ngp->ng_V, trace_state(egp_states, ngp->ng_state),
			   ngp->ng_responses, ngp->ng_j, egp_reachability[ngp->ng_responses], ngp->ng_k);
    }
}


static void
egp_event_reachability(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_event(ngp, "reachability");

    ngp->ng_responses |= 1;
    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P4);
    egp_check_reachability(ngp);
}


static void
egp_shift_reachability(ngp)
struct egpngh *ngp;
{
    ngp->ng_responses = (ngp->ng_responses << 1) & ((1 << REACH_RATIO) - 1);
    egp_check_reachability(ngp);
}


/*
 *	Check for a status change and issue a reachability event
 */
static int
egp_check_status(ngp, egp_status, status)
struct egpngh *ngp;
u_char egp_status;
u_char status;
{

    switch (egp_status) {
	case UP:
	case DOWN:
	    if (ngp->ng_M == status) {
		/* M == ACTIVE && status == ACTIVE */
		/* M == PASSIVE && status == PASSIVE */
		egp_event_reachability(ngp);
	    }
	    break;
	default:
	    return (EBADHEAD);
    }
    return (NOERROR);
}


/*
 *	Routines for checking polling rate
 */
/*ARGSUSED*/
static void
egp_rate_init(ngp, rp, last)
struct egpngh *ngp;
struct egp_rate *rp;
time_t last;
{
    int i;

    for (i = 0; i < RATE_WINDOW; i++) {
	rp->rate_window[i] = rp->rate_min;
    }
    rp->rate_last = last;
}


static int
egp_rate_check(ngp, rp)
struct egpngh *ngp;
struct egp_rate *rp;
{
    int i;
    int excessive;
    time_t interval;

    interval = rp->rate_last ? time_sec - rp->rate_last : rp->rate_min;

    excessive = interval < rp->rate_min ? 1 : 0;

    for (i = RATE_WINDOW - 1; i; i--) {
	if ((rp->rate_window[i] = rp->rate_window[i - 1]) < rp->rate_min) {
	    excessive++;
	}
    }

    IF_EGPPROTO trace(TR_EGP, 0, "egp_rate_check: neighbor %s min %#T last %T excessive %d",
		       ngp->ng_name,
		       rp->rate_min,
		       rp->rate_last,
		       excessive);
    IF_EGPPROTO tracef("egp_rate_check: neighbor %s window ", ngp->ng_name);

    rp->rate_window[0] = interval;

    rp->rate_last = time_sec;

    for (i = 0; i < RATE_WINDOW; i++) {
	IF_EGPPROTO tracef("%#T ", rp->rate_window[i]);
    }

    IF_EGPPROTO trace(TR_EGP, 0, NULL);

    if (excessive < RATE_MAX) {
	return (0);
    } else {
	egp_rate_init(ngp, rp, rp->rate_last);
	return (1);
    }
}


/*
 * egp_set_intervals() sets EGP hello and poll intervals and times.
 * Returns 1 if either poll or hello intervals too big, 0 otherwise.
 */
static int
egp_set_intervals(ngp, egppkt)
struct egpngh *ngp;
struct egppkt *egppkt;
{
    struct egpacq *egpa = (struct egpacq *) egppkt;
    u_short helloint, pollint, ratio;

    /*
     * check parameters within bounds
     */
    helloint = ntohs(egpa->ea_hint);
    pollint = ntohs(egpa->ea_pint);
    if (helloint > MAXHELLOINT || pollint > MAXPOLLINT) {
	trace(TR_EXT, LOG_WARNING, "egp_set_intervals: Hello interval = %d or poll interval = %d too big from %s, code %d",
	      helloint,
	      pollint,
	      ngp->ng_name,
	      egpa->ea_pkt.egp_code);
	return (1);
    }
    if ((helloint != ngp->ng_P1) || (pollint != ngp->ng_P2)) {
	trace(TR_EXT, 0, "egp_set_intervals: neighbor %s specified hello/poll intervals %d/%d, we specified %d/%d",
	      ngp->ng_name,
	      helloint,
	      pollint,
	      ngp->ng_P1,
	      ngp->ng_P2);
    }
    if (helloint < ngp->ng_P1) {
	helloint = ngp->ng_P1;
    }
    if (pollint < ngp->ng_P2) {
	pollint = ngp->ng_P2;
    }
    ratio = (pollint - 1) / helloint + 1;	/* keep ratio pollint:helloint */
    helloint += HELLOMARGIN;
    pollint = ratio * helloint;
    trace(TR_EXT, 0, "egp_set_intervals: neighbor %s version %d state %s using intervals %d/%d",
	  ngp->ng_name,
	  ngp->ng_V,
	  trace_state(egp_states, ngp->ng_state),
	  helloint,
	  pollint);
    ngp->ng_T1 = helloint;
    ngp->ng_T3 = ngp->ng_T1 * REACH_RATIO;
    ngp->ng_T2 = helloint * ratio;
    return (0);
}


/*
 * egp_init_variables() go into neighbor state, initialize most variables.
 */

/* ARGSUSED */
static int
egp_init_variables(ngp, egp)
struct egpngh *ngp;
struct egppkt *egp;
{
    if (egp_set_intervals(ngp, egp)) {
	return (1);
    }
    ngp->ng_responses = 0;
    ngp->ng_status = 0;
    ngp->ng_noupdate = 0;
    ngp->ng_R_lastpoll = -1;	/* Invalid ID for last poll */
    ngp->ng_flags &= ~(NGF_SENT_POLL | NGF_SENT_REPOLL | NGF_SENT_UNSOL | NGF_RECV_UNSOL | NGF_RECV_REPOLL | NGF_PROC_POLL);

    egp_rate_init(ngp, &ngp->ng_hello_rate, (time_t) 0);
    egp_rate_init(ngp, &ngp->ng_poll_rate, (time_t) 0);

    if (!(ngp->ng_options & NGO_ASIN)) {
	ngp->ng_asin = htons(egp->egp_system);
	ngp->ng_accept = control_exterior_locate(egp_accept_list, ngp->ng_asin);
	ngp->ng_propagate = control_exterior_locate(egp_propagate_list, ngp->ng_asin);
    }
    if (egp->egp_status) {
	ngp->ng_M = egp->egp_status == ACTIVE ? PASSIVE : ACTIVE;
    } else {
	ngp->ng_M = ngp->ng_asin < ngp->ng_asout ? PASSIVE : ACTIVE;
    }

    if (ngp->ng_M == ACTIVE) {
	ngp->ng_j = 3;
	ngp->ng_k = 1;
    } else {
	ngp->ng_j = 1;
	ngp->ng_k = 0;
    }

    ngp->ng_rtage = (ngp->ng_T2 * EGP_N_POLLAGE) > RT_T_EXPIRE ? ngp->ng_T2 * EGP_N_POLLAGE : RT_T_EXPIRE;

    return (0);
}


/* Time to send a poll */
static void
egp_do_poll(ngp, sync_t1)
struct egpngh *ngp;
int sync_t1;
{
    timer *tip = ngp->ng_task->task_timer[EGP_TIMER_t2];
    
    if (ngp->ng_flags & NGF_SENT_REPOLL) {
	ngp->ng_flags &= ~(NGF_SENT_POLL | NGF_SENT_REPOLL);
    }
    if (ngp->ng_flags & NGF_SENT_POLL) {
	ngp->ng_flags |= NGF_SENT_REPOLL;
    } else {
	ngp->ng_S++;
	ngp->ng_flags |= NGF_SENT_POLL;
	ngp->ng_flags &= ~(NGF_RECV_UNSOL);
	if (++ngp->ng_noupdate > MAXNOUPDATE) {
	    char buf[BUFSIZ];

	    sprintf(buf, "no Update received for %d successive new poll id's",
		    ngp->ng_noupdate);
	    egp_send_error(ngp, (struct egppkt *) 0, 0, ENORESPONSE, buf);
	    ngp->ng_noupdate = 0;
	    return;
	}
    }
    egp_send_poll(ngp);

    if (sync_t1) {
	/* Sync t1 to t2 */
	egp_reset_timer(tip,
			ngp->ng_T2 - (time_sec - ngp->ng_task->task_timer[EGP_TIMER_t1]->timer_last_time));
    } else if (tip->timer_interval != ngp->ng_T2) {
	/* Set the correct interval */
	egp_set_timer(tip, ngp->ng_T2);
    }
}


/*
 *	Routines to process events
 */
static void
egp_event_up(ngp)
struct egpngh *ngp;
{
    struct sockaddr_in source_net;

    IF_EGPPROTO egp_msg_event(ngp, "Up");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_CEASE:
	case NGS_UP:
	    egp_msg_confused(ngp, "Up");
	    break;
	case NGS_DOWN:
	    egp_state_up(ngp);

	    /* Send a POLL */
	    egp_do_poll(ngp, TRUE);

	    /* Just for good luck, send an unsolicitied update if we can make */
	    /* an educated guess about which net he is interested in */
	    if (!(ngp->ng_flags & (NGF_SENT_UNSOL | NGF_PROC_POLL))) {
		ngp->ng_paddr = ngp->ng_saddr;	/* struct copy */
		sockclear_in(&source_net);
		if (ngp->ng_paddr.sin_addr.s_addr) {
		    /* Polled net is set, use that net address */
		    source_net = ngp->ng_paddr;	/* struct copy */
		} else {
		    /* If polled net not set, use shared net if we are both on it */
		    if (gd_inet_wholenetof(ngp->ng_addr.sin_addr) == gd_inet_wholenetof(ngp->ng_interface->int_addr.in.sin_addr)) {
			source_net.sin_addr.s_addr = gd_inet_wholenetof(ngp->ng_addr.sin_addr);
		    }
		}
		/* If we figured out a net, and have a route to it, send an unsolicited update */
		if (source_net.sin_addr.s_addr) {
		    if (rt_locate(RTS_INTERIOR, (sockaddr_un *) & source_net, RTPROTO_DIRECT)) {
			egp_send_update(ngp, 1);
			ngp->ng_flags |= NGF_SENT_UNSOL;
		    }
		}
	    }
	    break;
    }
}


static void
egp_event_down(ngp)
struct egpngh *ngp;
{

    IF_EGPPROTO egp_msg_event(ngp, "Down");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_CEASE:
	case NGS_DOWN:
	    egp_msg_confused(ngp, "Down");
	    break;
	case NGS_UP:
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    egp_state_down(ngp);
	    break;
    }
}


/*ARGSUSED*/
static void
egp_event_request(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    u_short helloint;

    IF_EGPPROTO egp_msg_event(ngp, "Request");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_UP:
	    if ((ngp->ng_options & NGO_ASIN) && (ngp->ng_asin != htons(egp->egp_system))) {
		trace(TR_EXT, LOG_ERR, "egp_event_request: neighbor %s version %d state %s specified AS %d, we expected %d",
		      ngp->ng_name,
		      ngp->ng_V,
		      trace_state(egp_states, ngp->ng_state),
		      htons(egp->egp_system),
		      ngp->ng_asin);
		egp_send_acquire(ngp, NAREFUS, ADMINPROHIB, egprid_h);
		egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
		egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
		egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_START_LONG);
		egp_state_idle(ngp);
		break;
	    }
	    if (egp_init_variables(ngp, egp)) {
		/* XXX - May want to declare a stop? */
		egp_send_acquire(ngp, NAREFUS, PARAMPROB, egprid_h);
		break;
	    }
	    ngp->ng_R = egprid_h;
	    egp_send_acquire(ngp, NACONF, (u_int) ngp->ng_M, egprid_h);
	    if (ngp->ng_M == ACTIVE) {
		egp_send_hello(ngp, neighHello, ngp->ng_S);
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], ngp->ng_T1);
#ifdef	notdef
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P5);
#else				/* notdef */
	    helloint = ntohs(((struct egpacq *) egp)->ea_hint);
	    if (helloint < ngp->ng_P1) {
		helloint = ngp->ng_P1;
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) (2 * REACH_RATIO * helloint));
#endif				/* notdef */
	    egp_state_down(ngp);
	    break;
	case NGS_CEASE:
	    ngp->ng_status = GODOWN;
	    egp_send_acquire(ngp, NACEASE, ngp->ng_status, ngp->ng_S);
	    egp_state_cease(ngp);
    }
}


static void
egp_event_confirm(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    u_short helloint;

    IF_EGPPROTO egp_msg_event(ngp, "Confirm");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	    if ((ngp->ng_options & NGO_ASIN) && (ngp->ng_asin != htons(egp->egp_system))) {
		trace(TR_EXT, LOG_ERR, "egp_event_confirm: neighbor %s version %d state %s specified AS %d, we expected %d",
		      ngp->ng_name,
		      ngp->ng_V,
		      trace_state(egp_states, ngp->ng_state),
		      htons(egp->egp_system),
		      ngp->ng_asin);
		egp_send_acquire(ngp, NAREFUS, ADMINPROHIB, egprid_h);
		egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
		egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
		egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_START_LONG);
		egp_state_idle(ngp);
		break;
	    }
	    if (egp_init_variables(ngp, egp)) {
		/* XXX - May need more thought */
		egp_event_stop(ngp, PARAMPROB);
		break;
	    }
	    if (egp_check_status(ngp, UP, ACTIVE) != NOERROR) {
		egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Status field in Confirm");
		egp_stats.inerrors++;
		egp_stats.inmsgs--;
		ngp->ng_stats.inerrors++;
		ngp->ng_stats.inmsgs--;
		break;
	    }
	    ngp->ng_R = egprid_h;
	    if (ngp->ng_M == ACTIVE) {
		egp_send_hello(ngp, neighHello, ngp->ng_S);
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], ngp->ng_T1);
#ifdef	notdef
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P5);
#else				/* notdef */
	    helloint = ntohs(((struct egpacq *) egp)->ea_hint);
	    if (helloint < ngp->ng_P1) {
		helloint = ngp->ng_P1;
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) (2 * REACH_RATIO * helloint));
#endif				/* notdef */
	    egp_state_down(ngp);
	    break;
	case NGS_DOWN:
	case NGS_UP:
	case NGS_CEASE:
	    egp_msg_confused(ngp, "Confirm");
	    break;
    }
}


static void
egp_event_refuse(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    time_t restart_delay = 0;

    IF_EGPPROTO egp_msg_event(ngp, "Refuse");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	    trace(TR_EGP, LOG_WARNING, "egp_event_refuse: neighbor %s AS %d state %s Cease Refuse %s",
		  ngp->ng_name,
		  ngp->ng_asin,
		  trace_state(egp_states, ngp->ng_state),
		  egp_acq_status[egp->egp_status]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    switch (egp->egp_status) {
		case UNSPEC:
		case ACTIVE:
		case PASSIVE:
		case NORESOURCE:
		case GODOWN:
		    restart_delay = EGP_START_SHORT;
		    break;
		case ADMINPROHIB:
		case PARAMPROB:
		case PROTOVIOL:
		    restart_delay = EGP_START_LONG;
		    break;
		default:
		    egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Status field in Refuse");
		    egp_stats.inerrors++;
		    egp_stats.inmsgs--;
		    ngp->ng_stats.inerrors++;
		    ngp->ng_stats.inmsgs--;
		    break;
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], restart_delay);
	    egp_state_idle(ngp);
	    break;
	case NGS_DOWN:
	case NGS_UP:
	case NGS_CEASE:
	    egp_msg_confused(ngp, "Refuse");
	    break;
    }
}


static void
egp_event_cease(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    time_t restart_delay = 0;

    IF_EGPPROTO egp_msg_event(ngp, "Cease");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACACK, egp->egp_status, egprid_h);
	    break;
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_UP:
	    trace(TR_EGP, LOG_WARNING, "egp_event_cease: neighbor %s AS %d state %s Cease reason %s",
		  ngp->ng_name,
		  ngp->ng_asin,
		  trace_state(egp_states, ngp->ng_state),
		  egp_acq_status[egp->egp_status]);
	case NGS_CEASE:
	    egp_send_acquire(ngp, NACACK, egp->egp_status, egprid_h);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    switch (egp->egp_status) {
		case UNSPEC:
		case ACTIVE:
		case PASSIVE:
		case NORESOURCE:
		case GODOWN:
		    restart_delay = EGP_START_SHORT;
		    break;
		case ADMINPROHIB:
		case PARAMPROB:
		case PROTOVIOL:
		    restart_delay = EGP_START_LONG;
		    break;
		default:
		    egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Status field in Cease");
		    egp_stats.inerrors++;
		    egp_stats.inmsgs--;
		    ngp->ng_stats.inerrors++;
		    ngp->ng_stats.inmsgs--;
		    break;
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], restart_delay);
	    egp_state_idle(ngp);
	    break;
    }
}


static void
egp_event_ceaseack(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    time_t restart_delay = 0;

    IF_EGPPROTO egp_msg_event(ngp, "Cease-ack");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    break;
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_UP:
	    break;
	case NGS_CEASE:
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    switch (ngp->ng_status) {
		case UNSPEC:
		case ACTIVE:
		case PASSIVE:
		case NORESOURCE:
		case GODOWN:
		    restart_delay = EGP_START_SHORT;
		    break;
		case ADMINPROHIB:
		case PARAMPROB:
		case PROTOVIOL:
		    restart_delay = EGP_START_LONG;
		    break;
		default:
		    egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Status field in Cease-ack");
		    egp_stats.inerrors++;
		    egp_stats.inmsgs--;
		    ngp->ng_stats.inerrors++;
		    ngp->ng_stats.inmsgs--;
		    break;
	    }
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], restart_delay);
	    egp_state_idle(ngp);
	    break;
    }
}


static void
egp_event_hello(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    int error = NOERROR;
    const char *msg = NULL;

    IF_EGPPROTO egp_msg_event(ngp, "Hello");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	case NGS_CEASE:
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    if ((error = egp_check_status(ngp, egp->egp_status, PASSIVE)) != NOERROR) {
		msg = "invalid Status field in Hello";
		egp_stats.inerrors++;
		egp_stats.inmsgs--;
		ngp->ng_stats.inerrors++;
		ngp->ng_stats.inmsgs--;
		break;
	    } else {
		if (egp_rate_check(ngp, &ngp->ng_hello_rate)) {
		    error = EXSPOLL;
		    msg = "excessive HELLO rate";
		    break;
		}
		egp_send_hello(ngp, neighHeardU, egprid_h);
	    }
	    break;
    }
    if (error != NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
    }
}


static void
egp_event_heardu(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{

    IF_EGPPROTO egp_msg_event(ngp, "I-H-U");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	case NGS_CEASE:
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    if (egp_check_status(ngp, egp->egp_status, ACTIVE) != NOERROR) {
		egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Status field in I-H-U");
		egp_stats.inerrors++;
		egp_stats.inmsgs--;
		ngp->ng_stats.inerrors++;
		ngp->ng_stats.inmsgs--;
	    }
	    break;
    }
}


static void
egp_event_poll(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    int error = NOERROR;
    const char *msg = NULL;
    struct sockaddr_in source_net;

    IF_EGPPROTO egp_msg_event(ngp, "Poll");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	case NGS_CEASE:
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    ngp->ng_flags |= NGF_PROC_POLL;
	    if ((error = egp_check_status(ngp, egp->egp_status, PASSIVE)) != NOERROR) {
		ngp->ng_flags &= ~NGF_PROC_POLL;
		msg = "invalid Status field in Poll";
		break;
	    }
	    ngp->ng_flags &= ~NGF_PROC_POLL;
	    if (egp->egp_code != 0) {
		error = EBADHEAD;
		msg = "invalid Code field in Poll";
		break;
	    }
	    ngp->ng_R = egprid_h;
	    if (egprid_h == ngp->ng_R_lastpoll) {
		if (ngp->ng_flags & (NGF_RECV_REPOLL | NGF_SENT_UNSOL)) {
		    error = EXSPOLL;
		    msg = "too many Polls received";
		    break;
		}
		ngp->ng_flags |= NGF_RECV_REPOLL | NGF_SENT_UNSOL;
	    } else {
		ngp->ng_R_lastpoll = egprid_h;
		ngp->ng_flags &= ~(NGF_RECV_REPOLL | NGF_SENT_UNSOL);
	    }
	    if (egp_rate_check(ngp, &ngp->ng_poll_rate)) {
		error = EXSPOLL;
		msg = "excessive Polling rate";
		break;
	    }
	    if (ngp->ng_state == NGS_DOWN) {
		/* Ignore Polls in Down state */
		break;
	    }
	    ngp->ng_paddr.sin_addr = ((struct egppoll *) egp)->ep_net;
	    source_net = ngp->ng_paddr;	/* struct copy */
	    if (!rt_locate(RTS_INTERIOR, (sockaddr_un *) & source_net, RTPROTO_DIRECT)) {
		error = ENOREACH;
		msg = "no interface on net of Poll";
		break;
	    }
	    egp_send_update(ngp, 0);
	    break;
    }
    if (error != NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
}


static void
egp_event_update(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    int error = NOERROR;
    const char *msg = NULL;


    IF_EGPPROTO egp_msg_event(ngp, "Update");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_send_acquire(ngp, NACEASE, PROTOVIOL, ngp->ng_S);
	    break;
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_CEASE:
	    break;
	case NGS_UP:
	    if (egp->egp_code != 0) {
		error = EBADHEAD;
		msg = "invalid Code field in Update";
		break;
	    }
	    if ((error = egp_check_status(ngp, egp->egp_status & ~UNSOLICITED, ACTIVE)) != NOERROR) {
		msg = "invalid Status field in Update";
		break;
	    }
	    if (gd_inet_wholenetof(((struct egpnr *) egp)->en_net) != gd_inet_wholenetof(ngp->ng_saddr.sin_addr)) {
		error = EBADHEAD;
		msg = "Update Response/Indication IP Net Address field does not match command";
		break;
	    }
	    if (egprid_h != ngp->ng_S) {/* wrong seq. # */
		/* Ignore packets with bad sequence number */
		break;
	    }
	    if (egp->egp_status & UNSOLICITED) {
		if (ngp->ng_flags & NGF_RECV_UNSOL) {
		    error = EUNSPEC;
		    msg = "too many unsolicited Update Indications";
		    break;
		}
		ngp->ng_flags |= NGF_RECV_UNSOL;
	    } else {
		if (!(ngp->ng_flags & NGF_SENT_POLL)) {
		    error = EUNSPEC;
		    msg = "too many Update Indications";
		    break;
		}
	    }
	    ngp->ng_flags &= ~(NGF_SENT_POLL | NGF_SENT_REPOLL);
	    if ((error = egp_rt_recv(ngp, egp, egplen)) != NOERROR) {
		switch (error) {
		    case EBADDATA:
			msg = "invalid Update message format";
			break;
		    case EUNSPEC:
			msg = "unable to find interface for this neighbor";
			break;
		    default:
			msg = "internal error parsing Update ";
		}
		break;
	    } else {
		ngp->ng_noupdate = 0;
	    }
	    if (egp->egp_status & UNSOLICITED) {
		/* XXX - What should we do here? */
	    }
#ifdef	notdef
	    egp_set_timer(ngp->ng_task->task_timer[EGP_TIMER_t2], ngp->ng_T2);	/* t2 is reset relative to last start */
#endif	/* notdef */
	    break;
    }
    if (error != NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
}


void
egp_event_start(tp)
task *tp;
{
    struct egpngh *ngp;

    ngp = (struct egpngh *) tp->task_data;

    IF_EGPPROTO egp_msg_event(ngp, "Start");

    switch (ngp->ng_state) {
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_UP:
	    trace(TR_EGP, LOG_WARNING, "egp_event_start: neighbor %s AS %d state %s Start",
		  ngp->ng_name,
		  ngp->ng_asin,
		  trace_state(egp_states, ngp->ng_state));
	case NGS_IDLE:
	    egp_send_acquire(ngp, NAREQ, UNSPEC, ngp->ng_S);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], (time_t) EGP_P3);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P5);
	    egp_state_acquisition(ngp);
	    break;
	case NGS_CEASE:
	    break;
    }
}


/*
 * egp_event_delete() delete the current task
 */
void
egp_event_delete(ngp)
struct egpngh *ngp;
{
    struct egpngh *ngp2;

    IF_EGPPROTO egp_msg_event(ngp, "Delete");

    if (ngp->ng_task) {
	task_delete(ngp->ng_task);
    }
    /* Delete this neighbor from the list of neighbors */
    egp_neighbors--;
    if (egp_neighbor_head == ngp) {
	egp_neighbor_head = ngp->ng_next;
    } else {
	EGP_LIST(ngp2) {
	    if (ngp2->ng_next == ngp) {
		ngp2->ng_next = ngp->ng_next;
		break;
	    }
	} EGP_LISTEND;
    }
#if	defined(AGENT_SNMP)
    egp_sort_neighbors();
#endif				/* defined(AGENT_SNMP) */

    /* Now see if we were superceeded and cause him to wake up */
    EGP_LIST(ngp2) {
	if ((ngp2->ng_flags & NGF_WAIT) && equal(&ngp->ng_addr, &ngp2->ng_addr)) {
	    ngp2->ng_flags &= ~NGF_WAIT;
	    egp_event_t3(ngp2->ng_task->task_timer[EGP_TIMER_t3], (time_t) 0);
	    break;
	}
    } EGP_LISTEND;

    /* And finally free the control block */
    (void) free((caddr_t) ngp);
}


/*
 * egp_event_stop() sends Ceases to all neighbors when going down (when SIGTERM
 * received).
 *
 */

void
egp_event_stop(ngp, status)
struct egpngh *ngp;
u_int status;
{

    IF_EGPPROTO egp_msg_event(ngp, "Stop");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    if (ngp->ng_flags & NGF_DELETE) {
		egp_event_delete(ngp);
	    }
	    break;
	case NGS_ACQUISITION:
	case NGS_CEASE:
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_START_SHORT);
	    egp_state_idle(ngp);
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    trace(TR_EGP, LOG_WARNING, "egp_event_stop: neighbor %s AS %d state %s Stop reason %s",
		  ngp->ng_name,
		  ngp->ng_asin,
		  trace_state(egp_states, ngp->ng_state),
		  egp_acq_status[GODOWN]);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], (time_t) EGP_P3);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P5);
	    ngp->ng_status = status;
	    egp_send_acquire(ngp, NACEASE, ngp->ng_status, ngp->ng_S);
	    egp_state_cease(ngp);
	    break;
    }
}


/*ARGSUSED*/
void
 egp_event_t3
 (tip, interval)
timer *tip;
time_t interval;
{
    struct egpngh *ngp;

    ngp = (struct egpngh *) tip->timer_task->task_data;

    IF_EGPPROTO egp_msg_event(ngp, "t3");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    if (egp_group_acquired(ngp) < ngp->ng_gr_head->ng_gr_acquire) {
		egp_event_start(tip->timer_task);
	    } else {
		egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_START_RETRY);
	    }
	    break;
	case NGS_ACQUISITION:
#ifdef	EGPVERDEFAULT
	    if (ngp->ng_V != EGPVERDEFAULT) {
		egp_set_version(ngp, EGPVERDEFAULT);
		egp_event_start(tip->timer_task);
		break;
	    }
#endif				/* EGPVERDEFAULT */
	case NGS_CEASE:
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t1]);
	    egp_clear_timer(ngp->ng_task->task_timer[EGP_TIMER_t2]);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_START_SHORT);
	    egp_state_idle(ngp);
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    trace(TR_EGP, LOG_WARNING, "egp_event_t3: neighbor %s AS %d state %s Abort",
		  ngp->ng_name,
		  ngp->ng_asin,
		  trace_state(egp_states, ngp->ng_state));
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], (time_t) EGP_P3);
	    egp_reset_timer(ngp->ng_task->task_timer[EGP_TIMER_t3], (time_t) EGP_P5);
	    ngp->ng_status = GODOWN;
	    egp_send_acquire(ngp, NACEASE, ngp->ng_status, ngp->ng_S);
	    egp_state_cease(ngp);
	    break;
    }
}


/*ARGSUSED*/
void
egp_event_t1(tip, interval)
timer *tip;
time_t interval;
{
    struct egpngh *ngp;

    ngp = (struct egpngh *) tip->timer_task->task_data;

    IF_EGPPROTO egp_msg_event(ngp, "t1");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	    egp_msg_confused(ngp, "t1");
	    break;
	case NGS_ACQUISITION:
	    egp_send_acquire(ngp, NAREQ, UNSPEC, ngp->ng_S);
	    egp_set_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], (time_t) EGP_P3);
	    break;
	case NGS_DOWN:
	case NGS_UP:
	    egp_shift_reachability(ngp);
	    if (ngp->ng_M == ACTIVE) {
		egp_send_hello(ngp, neighHello, ngp->ng_S);
	    }

	    /* Check to see if a repoll is due */
	    if ((ngp->ng_flags & NGF_SENT_POLL) && !(ngp->ng_flags & NGF_SENT_REPOLL) &&
		tip->timer_next_time - ngp->ng_task->task_timer[EGP_TIMER_t2]->timer_last_time >= ngp->ng_T1) {
		/* Time for a repoll */
		egp_do_poll(ngp, FALSE);
	    }

	    /* Make sure our interval is correct (should not be necessary) */
	    egp_set_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], ngp->ng_T1);
	    break;
	case NGS_CEASE:
	    egp_send_acquire(ngp, NACEASE, ngp->ng_status, ngp->ng_S);
	    egp_set_timer(ngp->ng_task->task_timer[EGP_TIMER_t1], (time_t) EGP_P3);
	    break;
    }
}


/*ARGSUSED*/
void
egp_event_t2(tip, interval)
timer *tip;
time_t interval;
{
    struct egpngh *ngp = (struct egpngh *) tip->timer_task->task_data;

    IF_EGPPROTO egp_msg_event(ngp, "t2");

    switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_CEASE:
	    egp_msg_confused(ngp, "t2");
	    break;
	case NGS_UP:
	    egp_do_poll(ngp, FALSE);
	    break;
    }
}


/*
 * egp_recv_acquire() handles received Neighbor Acquisition messages: Request, Confirm,
 * Refuse, Cease and Cease-ack.
 *
 */
static void
egp_recv_acquire(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    int error = NOERROR;
    const char *msg = NULL;


    switch (egp->egp_code) {
	case NAREQ:			/* Neighbor acquisition request */
	    if (egplen != sizeof(struct egpacq)) {
		error = EBADHEAD;
		msg = "bad message length";
		break;
	    }
	    egp_event_request(ngp, egp, egplen);
	    break;
	case NACONF:			/* Neighbor acq. confirm */
	    if (egplen != sizeof(struct egpacq)) {
		error = EBADHEAD;
		msg = "bad message length";
		break;
	    }
	    if (egprid_h != ngp->ng_S) {
		/* Ignore packets with invalid sequence number */
		break;
	    }
	    egp_event_confirm(ngp, egp, egplen);
	    break;
	case NAREFUS:			/* Neighbor acq. refuse */
	    if (egplen != sizeof(struct egppkt) && egplen != sizeof(struct egpacq)) {
		error = EBADHEAD;
		msg = "bad message length";
		break;
	    }
	    if (egprid_h != ngp->ng_S) {
		/* Ignore packets with invalid sequence number */
		break;
	    }
	    egp_event_refuse(ngp, egp, egplen);
	    break;
	case NACEASE:			/* Neighbor acq. cease */
	    if (egplen != sizeof(struct egppkt) && egplen != sizeof(struct egpacq)) {
		error = EBADHEAD;
		msg = "bad message length";
		break;
	    }
	    egp_event_cease(ngp, egp, egplen);
	    break;
	case NACACK:			/* Neighbor acq. cease ack */
	    if (egplen != sizeof(struct egppkt) && egplen != sizeof(struct egpacq)) {
		error = EBADHEAD;
		msg = "bad message length";
		break;
	    }
	    if (egprid_h != ngp->ng_S) {
		/* Ignore packets with invalid sequence number */
		break;
	    }
	    egp_event_ceaseack(ngp, egp, egplen);
	    break;
	default:
	    error = EBADHEAD;
	    msg = "invalid Code field";
	    break;
    }
    if (error != NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
    return;
}


/*
 * egp_recv_neighbor() processes received hello packet
 */
static void
egp_recv_neighbor(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{

    switch (egp->egp_code) {
	case neighHello:
	    ngp->ng_R = egprid_h;
	    egp_event_hello(ngp, egp, egplen);
	    break;
	case neighHeardU:
	    if (egprid_h != ngp->ng_S_lasthello) {
		/* Ignore packets with bad sequence numbers */
		break;
	    }
	    egp_event_heardu(ngp, egp, egplen);
	    break;
	default:
	    egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Code field");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    break;
    }
    return;
}


/*ARGSUSED*/
static void
egp_recv_error(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    const char *err_msg;
    int reason;
    struct egperr *ee = (struct egperr *) egp;

    reason = htons(ee->ee_rsn);

    if (reason > EUVERSION) {
	err_msg = "(invalid reason)";
    } else {
	err_msg = egp_reasons[reason];
    }

    ngp->ng_stats.inerrmsgs++;

    trace(TR_EXT, LOG_WARNING, "egp_recv_error: neighbor %s AS %d state %s error %s",
	  ngp->ng_name,
	  ngp->ng_asin,
	  trace_state(egp_states, ngp->ng_state),
	  err_msg);

    switch (reason) {
	case EUNSPEC:
	case EBADHEAD:
	case EBADDATA:
	case EXSPOLL:
	case ENORESPONSE:
	    break;
	case EUVERSION:
	    switch (ngp->ng_state) {
		case NGS_IDLE:
		case NGS_DOWN:
		case NGS_UP:
		case NGS_CEASE:
		    break;
		case NGS_ACQUISITION:
		    if (egp->egp_ver != ngp->ng_V) {
			egp_set_version(ngp, egp->egp_ver);
		    }
		    break;
	    }
	    break;
	case ENOREACH:
	    switch (ngp->ng_state) {
		case NGS_IDLE:
		case NGS_ACQUISITION:
		case NGS_CEASE:
		case NGS_DOWN:
		    break;
		case NGS_UP:
		    egp_event_stop(ngp, GODOWN);
		    break;
	    }
	    break;
    }
}


static int
egp_check_packet(ngp)
struct egpngh *ngp;
{
    struct egppkt *egp = (struct egppkt *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    int egplen;
    struct sockaddr_in addr;
    if_entry *ifp;

    sockclear_in(&addr);
    addr.sin_addr = recv_ip.ip_src;	/* struct copy */

    if (recv_ip.ip_off & ~IP_DF) {
	trace(TR_EXT, LOG_ERR, "egp_check_packet: recv fragmanted pkt from %A",
	      &addr);
	return (0);
    }
    egplen = recv_ip.ip_len;
    if (trace_flags & TR_EGP) {
	egp_trace(ngp, "EGP RECV", FALSE, egp, egplen);
    }
    /* Locate interface used to receive this packet.  If the interface	*/
    /* matches the one we think we should be using, then update the	*/
    /* interface timer.  Otherwise, don't sweat it, this peer might not	*/
    /* share a network with us.						*/

    ifp = if_withdst((sockaddr_un *) & recv_addr);
    if (ifp == ngp->ng_interface) {
	if_rtupdate(ifp);
    }
    if (gd_inet_cksum(&recv_iovec[RECV_IOVEC_DATA], 1, egplen) != (u_short) 0) {
	trace(TR_EXT, LOG_WARNING, "egp_check_packet: bad EGP checksum from %A",
	      &addr);
	return (0);
    }
    if (egplen < sizeof(struct egppkt)) {
	trace(TR_EXT, LOG_WARNING, "egp_check_packet: bad pkt length %d from %A",
	      egplen,
	      &addr);
	return (0);
    }
    return (egplen);
}


static int
egp_check_version(ngp, egp, egplen)
struct egpngh *ngp;
struct egppkt *egp;
int egplen;
{
    int error = NOERROR;
    const char *msg = NULL;

    if (!(EGPVMASK & (1 << (egp->egp_ver - 2)))) {
	if (egp->egp_type != EGPERR) {
	    error = EUVERSION;
	    msg = "unsupported version";
	}
    } else if (ngp && (egp->egp_ver != ngp->ng_V)) {
	switch (egp->egp_type) {
	    case EGPACQ:
		egp_set_version(ngp, egp->egp_ver);
		break;
	    case EGPHELLO:
	    case EGPNR:
	    case EGPPOLL:
		error = EBADHEAD;
		msg = "invalid Version field";
		break;
	    case EGPERR:
		/* Fall through and let egp_recv's switch switch versions */
		break;
	}
    }
    if (error != NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	return (1);
    } else {
	return (0);
    }
}


/*
 *	Process and incoming EGP packet from a known neighbor
 */
/*ARGSUSED*/
void
egp_recv(tp)
task *tp;
{
    int count;
    struct egpngh *ngp;
    struct egppkt *egp = (struct egppkt *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    int egplen;

    if (task_receive_packet(tp, &count)) {
	return;
    }
    ngp = (struct egpngh *) tp->task_data;

    egplen = egp_check_packet(ngp);
    if (!egplen) {
	egp_stats.inerrors++;
	ngp->ng_stats.inerrors++;
	return;
    }
    egprid_h = ntohs(egp->egp_id);	/* save sequence number in host byte order */

    if (egp_check_version(ngp, egp, egplen)) {
	egp_stats.inerrors++;
	ngp->ng_stats.inerrors++;
	return;
    }
    egp_stats.inmsgs++;
    ngp->ng_stats.inmsgs++;

    switch (egp->egp_type) {
	case EGPACQ:
	    egp_recv_acquire(ngp, egp, egplen);
	    break;
	case EGPHELLO:
	    egp_recv_neighbor(ngp, egp, egplen);
	    break;
	case EGPNR:
	    egp_event_update(ngp, egp, egplen);
	    break;
	case EGPPOLL:
	    egp_event_poll(ngp, egp, egplen);
	    break;
	case EGPERR:
	    egp_recv_error(ngp, egp, egplen);
	    break;
	default:
	    egp_send_error(ngp, egp, egplen, EBADHEAD, "invalid Type field");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    return;
    }
}


/*
 *	Compare the old neighbor's configuration with the new configuration.
 *	Some options may be changed on the fly, some require the neighbor to be restarted.
 */
int
egp_neighbor_changed(ngpo, ngpn)
struct egpngh *ngpo, *ngpn;
{
    int changed = FALSE;
    flag_t changed_options = ngpo->ng_options ^ ngpn->ng_options;
    flag_t new_options = ngpn->ng_options;

    /* XXX - What about maxacquire? */

    if (changed_options & (NGO_ASIN | NGO_ASOUT | NGO_INTERFACE | NGO_GATEWAY | NGO_PREFERENCE | NGO_P1 | NGO_P2)) {
	changed = TRUE;
    }
    if ((new_options & NGO_ASIN) && (ngpn->ng_asin != ngpo->ng_asin)) {
	changed = TRUE;
    }
    if ((new_options & NGO_ASOUT) && (ngpn->ng_asout != ngpo->ng_asout)) {
	changed = TRUE;
    }
    if ((new_options & NGO_INTERFACE) && (ngpn->ng_interface != ngpo->ng_interface)) {
	changed = TRUE;
    }
    if ((new_options & NGO_GATEWAY) && !equal(&ngpn->ng_gateway, &ngpo->ng_gateway)) {
	changed = TRUE;
    }
    if ((new_options & NGO_PREFERENCE) && (ngpn->ng_preference != ngpo->ng_preference)) {
	changed = TRUE;
    }
    if ((new_options & NGO_P1) && (ngpn->ng_P1 != ngpo->ng_P1)) {
	changed = TRUE;
    }
    if ((new_options & NGO_P2) && (ngpn->ng_P2 != ngpo->ng_P2)) {
	changed = TRUE;
    }
    if (!changed) {
	/* Nothing has changed that has required a restart.  Let's deal with things */
	/* that can be changed on the fly */

	/* Default propagation and generation options can be changed by just changing the flags */
#define	FLAGS	(NGO_NOGENDEFAULT|NGO_DEFAULTIN|NGO_DEFAULTOUT|NGO_METRICOUT|NGO_SADDR|NGO_VERSION)
	ngpo->ng_options = (ngpo->ng_options & ~(FLAGS)) | (ngpn->ng_options & (FLAGS));
	changed_options &= ~(FLAGS);
#undef	FLAGS

	ngpo->ng_metricout = ngpn->ng_metricout;

	if (ngpn->ng_options & NGO_SADDR) {
	    ngpo->ng_saddr = ngpn->ng_saddr;	/* struct copy */
	}
	if (ngpn->ng_options & NGO_VERSION) {
	    ngpo->ng_version = ngpn->ng_version;
	}
    }
    return (changed);
}


static void
egp_dump_rate(fd, rp, name)
FILE *fd;
struct egp_rate *rp;
char *name;
{
    int i;

    (void) fprintf(fd, "\t\t%s: rate_min: %#T rate_last: %T\n",
		   name,
		   rp->rate_min,
		   rp->rate_last);
    (void) fprintf(fd, "\t\t\twindow:");

    for (i = 0; i < RATE_WINDOW; i++) {
	(void) fprintf(fd, " %#T", rp->rate_window[i]);
    }
    (void) fprintf(fd, "\n");
}


/*
 *	Dump EGP status to dump file
 */
void
egp_dump(fd)
FILE *fd;
{
    struct egpngh *ngp;
    struct egpngh *gr_ngp;

    /*
     *	EGP neighbor status
     */
    if (doing_egp) {
	(void) fprintf(fd, "EGP status:\n");
	(void) fprintf(fd, "\tdefaultegpmetric: %d\tpreference: %d\n",
		       egp_default_metric,
		       egp_preference);
	(void) fprintf(fd, "\tPackets In: %u\t\t\tErrors In: %u\n",
		       egp_stats.inmsgs,
		       egp_stats.inerrors);
	(void) fprintf(fd, "\tPackets Out: %u\t\t\tErrors Out: %u\n",
		       egp_stats.outmsgs,
		       egp_stats.outerrors);
	(void) fprintf(fd, "\t\t\t\t\tTotal Errors: %u\n\n",
		       egp_stats.outerrors + egp_stats.inerrors);
	if (egp_accept_list) {
	    control_exterior_dump(fd, 1, control_accept_dump, egp_accept_list);
	}
	if (egp_propagate_list) {
	    control_exterior_dump(fd, 1, control_propagate_dump, egp_propagate_list);
	}
	gr_ngp = (struct egpngh *) 0;
	EGP_LIST(ngp) {
	    if (gr_ngp != ngp->ng_gr_head) {
		gr_ngp = ngp->ng_gr_head;
		(void) fprintf(fd, "\n\tGroup: %d\tMembers: %d\tAcquire: %d\tAcquired: %d",
			       gr_ngp->ng_gr_index,
			       gr_ngp->ng_gr_number,
			       gr_ngp->ng_gr_acquire,
			       egp_group_acquired(ngp));
		if ((gr_ngp->ng_options & NGO_PREFERENCE)) {
		    (void) fprintf(fd, "\n\t\tPreference: %u",
				   gr_ngp->ng_preference);
		}
		(void) fprintf(fd, "\n\t\t\tASout: ");
		if ((gr_ngp->ng_options & NGO_ASOUT)) {
		    (void) fprintf(fd, "%u",
				   gr_ngp->ng_asout);
		} else {
		    (void) fprintf(fd, "N/A");
		}
		(void) fprintf(fd, "\tASin: ");
		if ((gr_ngp->ng_options & NGO_ASIN) || gr_ngp->ng_asin) {
		    (void) fprintf(fd, "%u",
				   gr_ngp->ng_asin);
		} else {
		    (void) fprintf(fd, "N/A");
		}
		(void) fprintf(fd, "\n");
	    }
	    (void) fprintf(fd, "\n\t%s\tV: %d", ngp->ng_name, ngp->ng_V);
	    (void) fprintf(fd, "\tInterface: %A\n",
			   &ngp->ng_interface->int_addr);
	    (void) fprintf(fd, "\t\tReachability: [%04B] %d\tj: %d\tk: %d\n",
			   ngp->ng_responses,
			   egp_reachability[ngp->ng_responses],
			   ngp->ng_j,
			   ngp->ng_k);
	    (void) fprintf(fd, "\t\tT1: %T\tT2: %T\n", ngp->ng_T1, ngp->ng_T2);
	    (void) fprintf(fd, "\t\tt1: %T\tt2: %T\tt3: %T\n",
		 ngp->ng_task->task_timer[EGP_TIMER_t1]->timer_next_time,
		 ngp->ng_task->task_timer[EGP_TIMER_t2]->timer_next_time,
		ngp->ng_task->task_timer[EGP_TIMER_t3]->timer_next_time);
	    (void) fprintf(fd, "\t\tP1: %#T\tP2: %#T\tP3: %#T\tP4: %#T\tP5: %#T\n",
			   ngp->ng_P1,
			   ngp->ng_P2,
			   EGP_P3,
			   EGP_P4,
			   EGP_P5);
	    (void) fprintf(fd, "\t\tState: <%s>\tMode: %s\n", trace_state(egp_states, ngp->ng_state), egp_acq_status[ngp->ng_M]);
	    (void) fprintf(fd, "\t\tFlags: %#x <%s>\n", ngp->ng_flags, trace_bits(egp_flags, ngp->ng_flags));
	    (void) fprintf(fd, "\t\tOptions: %#x <%s>\n", ngp->ng_options, trace_bits(egp_options, ngp->ng_options));
	    (void) fprintf(fd, "\t\tLast poll received: %A",
			   &ngp->ng_paddr);
	    (void) fprintf(fd, "\tNet to poll: %A\n",
			   &ngp->ng_saddr);
	    (void) fprintf(fd, "\t\tMaximum Route Age: %#T\n", ngp->ng_rtage);
	    (void) fprintf(fd, "\t\tMetricOut: ");
	    if ((ngp->ng_options & NGO_METRICOUT)) {
		(void) fprintf(fd, "%d", ngp->ng_metricout);
	    } else {
		(void) fprintf(fd, "N/A");
	    }
	    (void) fprintf(fd, "\n\t\tDefaultMetric: ");
	    if ((ngp->ng_options & NGO_DEFAULTOUT)) {
		(void) fprintf(fd, "%d", ngp->ng_defaultmetric);
	    } else {
		(void) fprintf(fd, "N/A");
	    }
	    (void) fprintf(fd, "\tGateway: ");
	    if ((ngp->ng_options & NGO_GATEWAY)) {
		(void) fprintf(fd, "%A",
			       &ngp->ng_gateway);
	    } else {
		(void) fprintf(fd, "N/A");
	    }
	    (void) fprintf(fd, "\n");
	    egp_dump_rate(fd, &ngp->ng_poll_rate, "Poll");
	    egp_dump_rate(fd, &ngp->ng_hello_rate, "Hello");
	    (void) fprintf(fd, "\t\tPackets In: %u\t\t\tErrors In: %u\n",
			   ngp->ng_stats.inmsgs,
			   ngp->ng_stats.inerrors);
	    (void) fprintf(fd, "\t\tPackets Out: %d\t\t\tErrors Out: %d\n",
			   ngp->ng_stats.outmsgs,
			   ngp->ng_stats.outerrors);
	} EGP_LISTEND;
	(void) fprintf(fd, "\n");
    }
}


#endif				/* PROTO_EGP */
