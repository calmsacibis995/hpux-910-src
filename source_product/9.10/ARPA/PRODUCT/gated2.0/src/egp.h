/*
 * $Header: egp.h,v 1.1.109.5 92/02/28 14:01:47 ash Exp $
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


#ifdef	PROTO_EGP
#include "egp_param.h"

#define EGP_N_POLLAGE	3		/* minimum number of poll intervals before a route is deleted when not updated */

#define	EGP_LIMIT_PKTSIZE	1024, IP_MAXPACKET - sizeof (struct ip)

/*
 *	EGP Polling rate structure.
 */
struct egp_rate {
    time_t rate_min;
    time_t rate_last;
    time_t rate_window[RATE_WINDOW];
};


/*
 *	EGP stats definition
 */
struct egpstats_t {
    u_int inmsgs;
    u_int inerrors;
    u_int outmsgs;
    u_int outerrors;
    u_int inerrmsgs;
    u_int outerrmsgs;
    u_int stateups;
    u_int statedowns;
    int trigger;
};


/*
 * Structure egpngh stores state information for an EGP neighbor. There is
 * one such structure allocated at initialization for each of the trusted EGP
 * neighbors read from the initialization file. The egpngh structures are in a
 * singly linked list pointed to by external variable "egp_neighbor_head".
 * The major states of a neighbor are IDLE, ACQUIRED, DOWN, UP and CEASE.
 */


struct egpngh {
    struct egpngh *ng_next;		/* next state table of linked list */
    char ng_name[16];			/* Printable address of this neighbor */

    /* Status */
    int ng_state;			/* Current state of protocol */
    flag_t ng_flags;
    u_int ng_status;			/* Info saved for cease retransmission */

    /* Variables */
    u_short ng_R;			/* receive sequence number */
    u_short ng_S;			/* send sequence number */
    time_t ng_T1;			/* interval between Hello command retransmissions */
    time_t ng_T2;			/* interval between Poll command retransmissions */
    time_t ng_T3;			/* interval during which neighbor-reachibility indications are counted */
    time_t ng_P1;			/* Minimum interval acceptable between successive Hello commands received */
    time_t ng_P2;			/* Minimum interval acceptable between successive Poll commands received */
    int ng_M;				/* hello polling mode */
    int ng_j;				/* neighbor-up threshold */
    int ng_k;				/* neighbor-down threshold */
    int ng_V;				/* EGP version we speak */

    /* Polling info */
    struct egp_rate ng_poll_rate;	/* Polling rate */
    struct egp_rate ng_hello_rate;	/* Hello rate */
    u_int ng_R_lastpoll;		/* sequence number of last poll */
    u_short ng_S_lasthello;		/* sequence number of last hello sent */
    int ng_noupdate;			/* # successive polls (new id) which did not receive a valid update */

    /* Tasks and timers */
    task *ng_task;			/* Task for this peer */

    /* Addresses */
    struct sockaddr_in ng_gateway;	/* Address of local gateway */
    if_entry *ng_interface;		/* Pointer to Interface for sending packets */
    gw_entry ng_gw;			/* gw_entry for this peer */
#define	ng_addr		ng_gw.gw_addr.in
#define	ng_accept	ng_gw.gw_accept
#define	ng_propagate	ng_gw.gw_propagate
#define	ng_time		ng_gw.gw_time

    struct sockaddr_in ng_paddr;	/* Last address he polled */
    struct sockaddr_in ng_saddr;	/* Address I should poll */

    /* Neighbor Reachability Info */
    int ng_responses;			/* Shift register of responses for determining
				        reachability, each set bit corresponds to a
				        response, each zero to no response */

    /* Configured info */
    time_t ng_rtage;			/* Maximum age of these routes */
    flag_t ng_options;			/* Option flags */
    as_t ng_asin;			/* AS number our neighbor should have */
    as_t ng_asout;			/* AS number we should tell our neighbor */
    metric_t ng_defaultmetric;		/* Metric to use for default */
    u_short ng_version;			/* Configuration specified version */
    metric_t ng_metricout;		/* Metric to use for all outgoing nets */
    pref_t ng_preference;		/* Preference */

    /* Max acquire info */
    struct egpngh *ng_gr_head;		/* Pointer to head of group */
    u_short ng_gr_acquire;		/* Maximum neighbors to acquire in this group */
    u_short ng_gr_number;		/* Number of neighbors in this group */
    u_short ng_gr_index;		/* Group number */

    /* Statistics */
    struct egpstats_t ng_stats;		/* Statistic structre */
};

/* Timers */
#define	EGP_TIMER_t1	0		/* timer t1 (user to control Request, Hello and Cease command retransmissions) */
#define	EGP_TIMER_t2	1		/* timer t2 (used to control Poll command retransmissions) */
#define	EGP_TIMER_t3	2		/* timer 3 (abort timer) */

/* States */
#define NGS_IDLE		0
#define NGS_ACQUISITION		1
#define NGS_DOWN		2
#define NGS_UP			3
#define NGS_CEASE		4

/* flags */
#define	NGF_SENT_UNSOL		0x01	/* An unsolicited update has been sent since we were polled */
#define	NGF_SENT_POLL		0x02	/* A Poll has been sent, an update is outstanding */
#define	NGF_SENT_REPOLL		0x04	/* A rePoll has been sent, an update is outstanding */
#define	NGF_RECV_REPOLL		0x08	/* An rePoll has been received on this id */
#define	NGF_RECV_UNSOL		0x10	/* An unsolicited update has been received since we sent a poll */
#define	NGF_PROC_POLL		0x20	/* A Poll is being processed, don't send an unsolicited if we transition to Up */
#define	NGF_DELETE		0x40	/* Delete after re-init should be cleared by parser */
#define	NGF_WAIT		0x80	/* Waiting for deleted neighbor to start us */
#define	NGF_GENDEFAULT		0x100	/* Default generation has been requested */

/* Options */
#define NGO_METRICOUT		0x01	/* Use and outbound metric */
#define NGO_ASIN		0x02	/* Verify inbound AS number */
#define NGO_ASOUT		0x04	/* Use this outbound AS number */
#define NGO_NOGENDEFAULT	0x08	/* Don't consider this neighbor for default generation */
#define NGO_DEFAULTIN		0x10	/* Allow default in an NG packet */
#define NGO_DEFAULTOUT		0x20	/* Send DEFAULT net to this neighbor */
#define	NGO_INTERFACE		0x40	/* Interface was specified */
#define	NGO_SADDR		0x80	/* IP Source Network was specified */
#define	NGO_GATEWAY		0x0100	/* Address of local gateway to Source Network */
#define	NGO_MAXACQUIRE		0x0200	/* Maxacquire for this group */
#define	NGO_VERSION		0x0400	/* EGP version number to use initially */
#define	NGO_PREFERENCE		0x0800	/* Preference for this AS */
#define	NGO_P1			0x1000	/* P1 was specified */
#define	NGO_P2			0x2000	/* P2 was specified */

/* Basic EGP packet */

struct egppkt {
    u_char egp_ver;			/* Version # */
    u_char egp_type;			/* Opcode */
    u_char egp_code;
    u_char egp_status;
    u_short egp_chksum;
    as_t egp_system;			/* Autonomous system */
    u_short egp_id;
};


/* EGP neighbor acquisition packet */
struct egpacq {
    struct egppkt ea_pkt;
    u_short ea_hint;			/* Hello interval in seconds */
    u_short ea_pint;			/* NR poll interval in seconds */
};

/* EGP NR poll packet */
struct egppoll {
    struct egppkt ep_pkt;
    u_short ep_unused;
    struct in_addr ep_net;		/* Source net */
};

/* EGP NR Message packet */
struct egpnr {
    struct egppkt en_pkt;
    u_char en_igw;			/* No. internal gateways */
    u_char en_egw;			/* No. external gateways */
    struct in_addr en_net;		/* shared net */
};

#define NRMAXNETUNIT 9			/* maximum size per net in octets of net part
																		       of NR message */
/* EGP Error packet */
struct egperr {
    struct egppkt ee_pkt;
    u_short ee_rsn;
    u_char ee_egphd[12];		/* First 12 bytes of bad egp pkt */
};

#define EGPVER	2
#define	EGPVERDEFAULT	2
#define	EGPVMASK	0x03		/* We speak version 2 and sometime version 3 */

/* EGP Types */
#define EGPNR		1
#define	EGPPOLL		2
#define EGPACQ		3
#define EGPHELLO	5
#define	EGPERR		8

/* Neighbor Acquisition Codes */
#define NAREQ	0			/* Neighbor acq. request */
#define NACONF	1			/* Neighbor acq. confirmation */
#define NAREFUS	2			/* Neighbor acq. refuse */
#define NACEASE	3			/* Neighbor cease */
#define NACACK	4			/* Neighbor cease ack */

/* Neighbor Acquisition Message Status Info */
#define UNSPEC		0
#define	ACTIVE		1
#define	PASSIVE		2
#define	NORESOURCE	3
#define	ADMINPROHIB	4
#define	GODOWN		5
#define	PARAMPROB	6
#define	PROTOVIOL	7

/* Neighbor Hello Codes */
#define neighHello	0
#define neighHeardU	1

/* Reachability, poll and update status */
#define INDETERMINATE	0
#define UP		1
#define DOWN		2
#define UNSOLICITED   128

/* Error reason status */
#define	EUNSPEC		0
#define EBADHEAD	1
#define	EBADDATA	2
#define ENOREACH	3
#define	EXSPOLL		4
#define ENORESPONSE	5
#define	EUVERSION	6
#define	EMAXERR		EUVERSION


extern struct egpstats_t egp_stats;

#ifdef	USE_PROTOTYPES
extern void egp_init(void);
extern void egp_reinit(void);
extern void egp_dump(FILE * fd);
extern void egp_recv(task * tp);
extern void egp_event_t1(timer * tip, time_t interval);
extern void egp_event_t2(timer * tip, time_t interval);
extern void egp_event_t3(timer * tip, time_t interval);
extern void egp_event_stop(struct egpngh * ngp, u_int status);
extern void egp_event_start(task * tp);
extern void egp_event_delete(struct egpngh * ngp);

extern int egp_rt_recv(struct egpngh * ngp, struct egppkt * pkt, int egplen);
extern int egp_rt_send(struct egpnr * nrpkt, struct egpngh * ngp);
extern int egp_neighbor_changed(struct egpngh * ngpo, struct egpngh * ngpn);

#else				/* USE_PROTOTYPES */
extern void egp_init();
extern void egp_reinit();
extern void egp_dump();
extern void egp_recv();
extern void egp_event_t1();
extern void egp_event_t2();
extern void egp_event_t3();
extern void egp_event_stop();
extern void egp_event_start();
extern void egp_event_delete();

extern int egp_rt_recv();
extern int egp_rt_send();
extern int egp_neighbor_changed();

#endif				/* USE_PROTOTYPES */

#define	egp_clear_timer(tp) egp_set_timer(tp, (time_t)0)

extern bits egp_states[];
extern bits egp_flags[];
extern bits egp_options[];

extern u_int egp_reachability[];	/* Number of bits in a given state of
				           the reachability register */

extern const char *egp_acq_codes[];	/* Acquisition packet types */
extern const char *egp_reach_codes[];	/* Reachability codes */
extern const char *egp_nr_status[];	/* Network reachability states */
extern const char *egp_acq_status[];	/* Acquisition packet codes */
extern const char *egp_reasons[];	/* Error code reasons */

extern int egp_neighbors;		/* number of egp neighbors */
extern struct egpngh *egp_neighbor_head;/* start of linked list of egp neighbor state tables */

#if	defined(AGENT_SNMP)
extern void egp_sort_neighbors();
extern struct egpngh **egp_sort;	/* Sorted list of pointers to neighbors */

#endif				/* defined(AGENT_SNMP) */

extern u_short egprid_h;		/* sequence number of received egp packet
				           in host byte order - all ids in internal
				           tables are in host byte order */
extern int doing_egp;			/* Are we running EGP protocols? */
extern int egp_pktsize;			/* Maximum packet size */
extern pref_t egp_preference;		/* Preference for EGP routes */
extern metric_t egp_default_metric;	/* default EGP metric */
extern adv_entry *egp_accept_list;	/* List of EGP advise entries */
extern adv_entry *egp_propagate_list;	/* List of EGP propagate entries */

#define	EGP_LIST(ngp)	for (ngp = egp_neighbor_head; ngp; ngp = ngp->ng_next)
#define	EGP_LISTEND

#endif				/* PROTO_EGP */
