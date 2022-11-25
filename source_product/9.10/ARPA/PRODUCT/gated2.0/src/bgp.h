/*
 * $Header: bgp.h,v 1.1.109.5 92/02/28 14:01:16 ash Exp $
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


#ifdef	PROTO_BGP

#define	IPPROTO_BGP		179
#define	BGPMAXPACKETSIZE	1024
#define	BGPUPDATEMAXNETS	167	/* Maximum number of networks in an Update */

#define	BGP_VERSION	1		/* BGP version number */

#define	BGP_WAIT_RETRY	60		/* How often to retry a connect (must be greater than kernel timeout) */
#define	BGP_ABORT_OPEN	60		/* Abort timer on open */
#define	BGP_CLOSE_TIMER	45		/* Value to linger on close - */
 /* must be smaller than */
 /* BGP_IDLE_SHORT */
#define	BGP_KEEP_ALIVE	180		/* How often we send keepalives */
#define	BGP_FLASH_INTERVAL	15	/* Minimum number of seconds between flash updates */

/* Various values of times to wait in Idle state before going back to Active */
#define	BGP_IDLE_INIT	15		/* Give gated 15 seconds to stabilize before starting */
#define	BGP_IDLE_SHORT	60		/* Wait time for minor errors */
#define	BGP_IDLE_MED	600		/* 10 minutes for some errors */
#define	BGP_IDLE_FATAL	15*60		/* 15 minute wait for fatal errors */

typedef struct {
    u_char asDirection;
    u_char asNumber[2];
} asPair;

typedef struct {
    u_char asCount;
    asPair asPair[1];
} asPath;

#define	asDirUp		1		/* Route has gone up in graph */
#define	asDirDown	2		/* Route has gone down in graph */
#define	asDirHorizontal	3		/* Route has gone horizontally in graph */
#define	asDirEgp	4		/* Route was learned via EGP */
#define	asDirIncomplete	5		/* ASpath is incomplete */

#define	asDirMax	asDirIncomplete	/* Used to check for valid bits */

typedef struct {
    u_char netNetwork[4];
    u_char netMetric[2];
} netPair;

#define	bgpMetricInfinity	0xffff	/* Metric of infinity */

typedef struct {
    u_char netCount[2];
    netPair netPair[1];
} netEntry;

typedef struct {
    /* Update header */
    u_char gateway[4];
    asPath asPath;
    netEntry netEntry;
} updatePdu;

#define	updatePduMinSize	16

typedef struct {
    /* Open Header */
    as_t openAs;
    u_char openLinkType;
    u_char openAuthCode;
    u_char openAuthData[2];
} openPdu;

#define	openLinkInternal	0	/* Peer is in same AS */
#define	openLinkUp		1	/* Peer is higher in AS hierachy */
#define	openLinkDown		2	/* Peer is lower in AS hierachy */
#define	openLinkHorizontal	3	/* Peer is as same level */

#define	openLinkMax		3	/* Maximum value */

typedef struct {
    u_short notifyCode;
    u_char notifyData[2];
    u_char notifyPacket[2];		/* Returned packet */
} notifyPdu;

#define	BGPERRCD_LINKTYPE	1	/* Link type error */
#define	BGPERRCD_AUTHCODE	2	/* Unknown Authentication code */
#define	BGPERRCD_AUTHFAIL	3	/* Authentication failure */
#define	BGPERRCD_UPDATE		4	/* Update error */
#define	BGPERRCD_SYNC		5	/* Connection out of sync */
#define	BGPERRCD_MSGLEN		6	/* Invalid message length */
#define	BGPERRCD_MSGTYPE	7	/* Invalid message type */
#define	BGPERRCD_VERSION	8	/* Invalid version number */
#define	BGPERRCD_OPENAS		9	/* Invalid AS field in Open */
#define	BGPERRCD_CEASE		10	/* Connection closing gracefully */
#define	BGPERRCD_MAX		10	/* Maximum error code */

#define	BGPERRLEN_LINKTYPE	1	/* Open byte of proper link type */
#define	BGPERRLEN_AUTHCODE	0	/* No data */
#define	BGPERRLEN_AUTHFAIL	0	/* No data */
#define	BGPERRLEN_UPDATE	2	/* Two bytes of subcode followed by Update PDU */
#define	BGPERRLEN_SYNC		0	/* No data */
#define	BGPERRLEN_MSGLEN	2	/* Two bytes of message length */
#define	BGPERRLEN_MSGTYPE	1	/* One byte of message type */
#define	BGPERRLEN_VERSION	1	/* One byte of version */
#define	BGPERRLEN_OPENAS	0	/* No data */

#define	BGPUPDERR_ASCOUNT	1	/* Invalid AS count */
#define	BGPUPDERR_DIRECTION	2	/* Invalid Direction field */
#define	BGPUPDERR_AS		3	/* Invalid AS field */
#define	BGPUPDERR_ORDER		4	/* EGP_LINK or INCOMPLETE_LINK type at other than end of AS path */
#define	BGPUPDERR_LOOP		5	/* Routing loop detected */
#define	BGPUPDERR_GATEWAY	6	/* Invalid Gateway field */
#define	BGPUPDERR_NETCOUNT	7	/* Invalid Net Count field */
#define	BGPUPDERR_NETWORK	8	/* Invalid Network field */
#define BGPUPDERR_MAX		8

typedef struct {
    /* Common BGP header */
    u_short marker;
    u_short length;
    u_char version;
    u_char type;
    u_short holdTime;
} pduHeader;

#define	bgpMarker		0xffff	/* Marker field */
#define	bgpVersion		1	/* Version we support */

#define	bgpPduOpen		1	/* Open packet */
#define	bgpPduUpdate		2	/* Update packet */
#define	bgpPduNotify		3	/* Notification packet */
#define	bgpPduKeepAlive		4	/* KeepAlive packet */
#define	bgpPduOpenConfirm	5	/* Open Confirmation */

#define	bgpPduMax		5	/* Maximum valid type */

typedef struct {
    /* Complete BGP packet (minus data fields) */
    pduHeader header;
    union {
	updatePdu update;
	openPdu open;
	notifyPdu notify;
    } pdu;
} bgpPdu;


typedef struct _as_path {
    short as_count;			/* Length of AS path */
    short as_origin;			/* Where did we learn this from */
    as_t as_number[1];			/* AS path elements */
} as_path;

#define	ASO_IGP		1		/* Originated with an IGP */
#define	ASO_EGP		2		/* Originated with EGP */
#define	ASO_INCOMPLETE	3		/* Origin is unknown */


/*
 *  BGP peer structure
 */
typedef struct _bgpPeer {
    struct _bgpPeer *bgp_next;		/* Pointer to next bgpPeer table */

    u_int bgp_state;			/* Protocol State */
    u_int bgp_flags;			/* Protocol Flags */
    u_int bgp_options;			/* User specified options */

    task *bgp_task;			/* Pointer to task */

    char bgp_name[16];			/* Name of this peer */

    struct sockaddr_in bgp_gateway;	/* Gateway to substitute for all of these routes */
    if_entry *bgp_interface;		/* Pointer to interface for this peer */
    gw_entry bgp_gw;			/* GW block for this peer */
#define	bgp_addr	bgp_gw.gw_addr.in
#define	bgp_accept	bgp_gw.gw_accept
#define	bgp_propagate	bgp_gw.gw_propagate
#define	bgp_time	bgp_gw.gw_time

    /* Received packet and counters */
    bgpPdu *bgp_packet;			/* Receive buffer for incomming packets */
    caddr_t bgp_readpointer;		/* Pointer for next recv */
    int bgp_length;			/* Target length for read or Length from packet read */
    int bgp_length_accumulated;		/* Length read so far */

    time_t bgp_holdtime_in;		/* peer specified HoldTime */
    time_t bgp_holdtime_out;		/* hold time we specify */

    metric_t bgp_metricout;		/* Global outbound metric for these routes */
    as_t bgp_asin;			/* AS this peer must specify */
    as_t bgp_asout;			/* AS to send to this peer */
    u_int bgp_linktype;			/* Type of link to this peer */

    pref_t bgp_preference;		/* Preference for this peer */
} bgpPeer;

/* Timers */
#define	BGPTIMER_KEEPALIVE	0	/* Fire when we should send a keepalive */
#define	BGPTIMER_CONNECT	1	/* Fire when time to try a connect again */
#define	BGPTIMER_HOLDTIME	2	/* Fire when hold time expires */

/* Protocol States */
#define	BGPSTATE_IDLE		1	/* Idle State - ignore connection attempts */
#define BGPSTATE_ACTIVE		2	/* Active State - waiting for a connection */
#define	BGPSTATE_CONNECT	3	/* Connect State - trying to connect */
#define	BGPSTATE_OPENSENT	4	/* Open packet has been sent - connected */
#define	BGPSTATE_OPENCONFIRM	5	/* Confirm packet has been received */
#define	BGPSTATE_ESTABLISHED	6	/* Connection has been established */

/* Events */
#define	BGPEVENT_START		1	/* Start */
#define	BGPEVENT_OPEN		2	/* Transport connection open */
#define	BGPEVENT_CLOSED		3	/* Transport connection closed */
#define	BGPEVENT_OPENFAIL	4	/* Transport connection open failed */
#define	BGPEVENT_RECVOPEN	5	/* Receive Open message */
#define	BGPEVENT_RECVCONFIRM	6	/* Receive OpenConfirm message */
#define	BGPEVENT_RECVKEEPALIVE	7	/* Receive KeepAlive message */
#define	BGPEVENT_RECVUPDATE	8	/* Receive Update message */
#define	BGPEVENT_RECVNOTIFY	9	/* Receive Notification message */
#define	BGPEVENT_HOLDTIME	10	/* Holdtime expired */
#define	BGPEVENT_KEEPALIVE	11	/* KeepAlive timer */
#define	BGPEVENT_CEASE		12	/* Cease */
#define	BGPEVENT_STOP		13	/* Stop */

/* Flags */
#define	BGPF_HEADER		0x01	/* Currently reading header */
#define	BGPF_DELETE		0x02	/* Delete this peer */
#define	BGPF_WAIT		0x04	/* This peer is waiting for another peer to finish */
#define	BGPF_GENDEFAULT		0x08	/* Default generation requested */

/* Options */
#define BGPO_METRICOUT		0x01	/* Use and outbound metric */
#define BGPO_ASIN		0x02	/* Verify inbound AS number */
#define BGPO_ASOUT		0x04	/* Use this outbound AS number */
#define BGPO_NOGENDEFAULT	0x08	/* Don't consider this peer for default generation */
#define	BGPO_GATEWAY		0x10	/* Address of local gateway to Source Network */
#define	BGPO_PREFERENCE		0x20	/* Preference for this AS */
#define	BGPO_INTERFACE		0x40	/* Our interface was specified */
#define	BGPO_LINKTYPE		0x80	/* Link type was specified */
#define	BGPO_HOLDTIME		0x0100	/* Holdtime was specified */


extern int doing_bgp;			/* Are we running BGP protocols? */
extern pref_t bgp_preference;		/* Preference for BGP routes */
extern u_short bgp_port;		/* BGP well known port */
extern bgpPeer *bgp_peers;		/* Linked list of BGP peers */
extern int bgp_n_peers;			/* Number of BGP peers */
extern bgpPdu *bgp_send_buffer;		/* Output buffer */
extern int notifyLengths[];		/* Lengths of notifyData */
extern metric_t bgp_default_metric;	/* Default BGP metric to use */
extern adv_entry *bgp_accept_list;	/* List of BGP advise entries */
extern adv_entry *bgp_propagate_list;	/* List of BGP propagate entries */

extern bits bgpFlags[];			/* Values of the flags */
extern bits bgpStates[];		/* Values of the states */
extern bits bgpEvents[];		/* Event values */
extern bits bgpOptions[];
extern bits bgpAsDirs[];		/* Values of the directions */
extern bits bgpOpenType[];		/* Values of the open types */
extern bits bgpPduType[];		/* Values of the packet types */
extern bits bgpErrors[];		/* Values of error codes */
extern bits bgpUpdateErrors[];		/* Values of update error subcodes */

#define	BGP_LIST(bnp)	for (bnp = bgp_peers; bnp; bnp = bnp->bgp_next)
#define	BGP_LISTEND


#ifdef	USE_PROTOTYPES
extern void bgp_init(void);		/* Protocol Initialization */
extern void bgp_reinit(void);		/* Protocol re-initialization */
extern void bgp_dump(FILE * fd);	/* Dump BGP state */
extern int bgp_peer_changed(bgpPeer * old, bgpPeer * new);
extern void
bgp_trace(struct sockaddr_in * src,
	  struct sockaddr_in * dst,
	  const char *direction,
	  bgpPdu * PDU,
	  int length);			/* Trace a BGP packet */
extern void
bgp_recv_Update(bgpPeer * bnp,
		bgpPdu * PDU,
		int length);		/* Process an incoming BGP packet */
extern void
bgp_send_update(bgpPeer * bnp,
		int flash_flag);	/* Send an update to this peer */
extern void
bgp_send_NotifyUpdate(bgpPeer * bnp,
		      int code,
		      bgpPdu * packet,
		      int size);	/* Send an error packet about an Update Error */
extern void
bgp_session_finit(bgpPeer * bnp,
		  int bgp_socket);	/* Delete main task */
extern void bgp_connect_init(bgpPeer * bnp);	/* Create a connect task */
extern void bgp_connect_finit(bgpPeer * bnp);
extern void
bgp_send(bgpPeer * bnp,
	 bgpPdu * PDU,
	 int length);			/* Send a packet */
extern void bgp_read(task * tp);	/* Data is ready to be read */
extern int bgp_recv(task * tp);		/* Read data from socket */

extern void bgp_event_Start(bgpPeer * bnp);
extern void bgp_event_Closed(bgpPeer * bnp);
extern void bgp_event_Open(bgpPeer * bnp);	/* Open succeded  */
extern void bgp_event_OpenFail(bgpPeer * bnp);	/* Open failure */
extern void
bgp_event_Holdtime(timer * tip,
		   time_t interval);	/* Abort timer fired */
extern void
bgp_event_KeepAlive(timer * tip,
		    time_t interval);	/* KeepAlive Timer */
extern void bgp_event_Stop(bgpPeer * bnp);	/* Gated termination */

extern void bgp_as_dump(FILE * fd);	/* Dump the AS paths */

#else				/* USE_PROTOTYPES */
extern void bgp_init();			/* Protocol Initialization */
extern void bgp_reinit();		/* Protocol re-initialization */
extern void bgp_dump();			/* Dump BGP state */
extern int bgp_peer_changed();
extern void bgp_trace();		/* Trace a BGP packet */
extern void bgp_recv_Update();		/* Process an incoming BGP packet */
extern void bgp_send_update();		/* Send an update to this peer */
extern void bgp_send_NotifyUpdate();	/* Send an error packet about an Update Error */
extern void bgp_session_finit();	/* Delete main task */
extern void bgp_connect_init();		/* Create a connect task */
extern void bgp_connect_finit();
extern void bgp_send();			/* Send a packet */
extern void bgp_read();			/* Data is ready to be read */
extern int bgp_recv();			/* Read data from socket */

extern void bgp_event_Start();
extern void bgp_event_Closed();
extern void bgp_event_Open();		/* Open succeded  */
extern void bgp_event_OpenFail();	/* Open failure */
extern void bgp_event_Holdtime();	/* Abort timer fired */
extern void bgp_event_KeepAlive();	/* KeepAlive Timer */
extern void bgp_event_Stop();		/* Gated termination */

extern void bgp_as_dump();		/* Dump the AS paths */

#endif				/* USE_PROTOTYPES */

#endif				/* PROTO_BGP */
