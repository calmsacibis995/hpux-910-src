/*
 * $Header: rip.h,v 1.1.109.5 92/02/28 16:01:21 ash Exp $
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


#ifdef	PROTO_RIP
#define RIP_INTERVAL	30

#define RIP_PORT 520
#define RIPHOPCNT_INFINITY	16
#define	RIP_HOP			1	/* Minimum hop count when passing through */
#define RIPPACKETSIZE 512

#define	min(a,b)	((a)>(b)?(b):(a))

extern int rip_pointopoint;		/* Are we ONLY doing pointopoint RIP? */
extern int rip_supplier;		/* Are we broadcasting RIP protocols? */
extern metric_t rip_default_metric;	/* Default metric to use when propogating */
extern int doing_rip;			/* Are we running RIP protocols? */
extern pref_t rip_preference;		/* Preference for RIP routes */
extern int rip_n_trusted;		/* Number of Trusted RIP gateways */
extern int rip_n_source;		/* Number of gateways to receive explicate RIP info */
extern adv_entry *rip_accept_list;	/* List of nets to accept and not accept */
extern adv_entry *rip_propagate_list;	/* List of nets to propagate */
extern gw_entry *rip_gw_list;		/* List of RIP gateways */
extern adv_entry **rip_int_accept;	/* List of accept lists per interface */
extern adv_entry **rip_int_propagate;	/* List of propagate lists per interface */

extern void rip_init();

#endif				/* PROTO_RIP */
