/*
 * $Header: hello.h,v 1.1.109.5 92/02/28 15:52:57 ash Exp $
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


#ifdef	PROTO_HELLO

#ifndef IPPROTO_HELLO
#define	IPPROTO_HELLO	63
#endif				/* IPPROTO_HELLO */
#define	HELLOMAXPACKETSIZE 1440
#define	DELAY_INFINITY	30000		/* in ms */
#define	HELLO_DELAY	100		/* minimum delay */
#define	HELLO_TIMERRATE	15		/* in seconds */
#define	HELLO_HYST(s)	(int)(s*.25)	/* 25% of old route, in ms */

#define	HELLO_DEFAULT	0		/* net 0 as default */

#define	METRIC_DIFF(x,y)	(x > y ? x - y : y - x)

/*	Define the DCN HELLO protocol packet			*/

struct hellohdr {
    u_short h_cksum;			/* Ip checksum of this header and data ares */
    u_short h_date;			/* Julian days since 1 January 1972 */
    u_long h_time;			/* Local time (milliseconds since midnight UT) */
    u_short h_tstp;			/* (used to calculate delay/offset) */
};

#define	Size_hellohdr	10
#define	PickUp_hellohdr(s, hellohdr) \
  PickUp(s, hellohdr.h_cksum); \
  PickUp(s, hellohdr.h_date); hellohdr.h_date = ntohs(hellohdr.h_date); \
  PickUp(s, hellohdr.h_time); hellohdr.h_time = ntohl(hellohdr.h_time); \
  PickUp(s, hellohdr.h_tstp); hellohdr.h_tstp = ntohs(hellohdr.h_tstp);
#define	PutDown_hellohdr(s, hellohdr) \
  PutDown(s, hellohdr.h_cksum); \
  hellohdr.h_date = htons(hellohdr.h_date); PutDown(s, hellohdr.h_date); \
  hellohdr.h_time = htonl(hellohdr.h_time); PutDown(s, hellohdr.h_time); \
  hellohdr.h_tstp = htons(hellohdr.h_tstp); PutDown(s, hellohdr.h_tstp);

#define	H_DATE_BITS	0xC000		/* Flag bits */
#define	H_DATE_LEAPADD	0x4000		/* Insert leap second at end of current day */
#define	H_DATE_LEAPDEL	0x8000		/* Delete leap second at end of current day */
#define	H_DATE_UNSYNC	0xC000		/* Clock is unsynchronized */

#define	H_DATE_MON_SHIFT	10
#define	H_DATE_MON_MASK		0x0f
#define	H_DATE_DAY_SHIFT	5
#define	H_DATE_DAY_MASK		0x1f
#define	H_DATE_YEAR_SHIFT	0
#define	H_DATE_YEAR_MASK	0x1f
#define	H_DATE_YEAR_BASE	72


struct hm_hdr {
    u_char hm_count;			/* Number of elements that follow */
    u_char hm_type;			/* Type of elements */
};

#define	Size_hm_hdr	2
#define	PickUp_hm_hdr(s, hm_hdr) \
  PickUp(s, hm_hdr.hm_count);\
  PickUp(s, hm_hdr.hm_type);
#define	PutDown_hm_hdr(s, hm_hdr) \
  PutDown(s, hm_hdr.hm_count);\
  PutDown(s, hm_hdr.hm_type);


struct type0pair {
    u_short d0_delay;			/* Delay to peer (milliseconds) */
    u_short d0_offset;			/* Clock offset of peer (milliseconds) */
};

#define	Size_type0pair	4
#define	PickUp_type0pair(s, type0pair) \
  PickUp(s, type0pair.d0_delay);  type0pair.d0_delay = ntohs(type0pair.d0_delay);\
  PickUp(s, type0pair.d0_offset); type0pair.d0_offset = ntohs(type0pair.d0_offset);
#define	PutDown_type0pair(s, type0pair) \
  type0pair.d0_delay = htons(type0pair.d0_delay); PutDown(s, type0pair.d0_delay);\
  type0pair.d0_offset = htons(type0pair.d0_offset); PutDown(s, type0pair.d0_offset);


struct type1pair {
    struct in_addr d1_dst;		/* IP host/network address */
    u_short d1_delay;			/* Delay to peer (milliseconds) */
    short d1_offset;			/* CLock offset of peer (milliseconds) */
};

#define	Size_type1pair	8
#define	PickUp_type1pair(s, type1pair) \
  PickUp(s, type1pair.d1_dst);\
  PickUp(s, type1pair.d1_delay); type1pair.d1_delay = ntohs(type1pair.d1_delay);\
  PickUp(s, type1pair.d1_offset); type1pair.d1_offset = ntohs(type1pair.d1_offset);
#define	PutDown_type1pair(s, type1pair) \
  PutDown(s, type1pair.d1_dst);\
  type1pair.d1_delay = htons(type1pair.d1_delay); PutDown(s, type1pair.d1_delay);\
  type1pair.d1_offset = htons(type1pair.d1_offset); PutDown(s, type1pair.d1_offset);


#define	WINDOW_INTERVAL		6	/* in minutes */
#define HELLO_INTERVAL		15	/* HELLO rate coming in, in secs */
#define HWINSIZE		(WINDOW_INTERVAL * (60 / HELLO_INTERVAL))
#define HELLO_REPORT	8		/* how far back we will report */

struct hello_win {
    rt_data h_head;
    int h_win[HWINSIZE];
    int h_index;
    int h_min;
    int h_min_ttl;
};

#define TRACE_HELLOPKT(comment, src, dst, hello, length, nets) { \
	    if (trace_flags & TR_HELLO) \
		hello_trace(comment, src, dst, hello, length, nets); \
	}

extern int hello_pointopoint;		/* ONLY doing pointopoint HELLO? */
extern int hello_supplier;		/* Are we broadcasting HELLO info? */
extern metric_t hello_default_metric;	/* Default metric to use when propogating */
extern int doing_hello;			/* Are we running HELLO protocols? */
extern pref_t hello_preference;		/* Preference for HELLO routes */
extern int hello_n_trusted;		/* Number of trusted gateways */
extern int hello_n_source;		/* Number of gateways to receive explicit HELLO packets */
extern adv_entry *hello_accept_list;
extern adv_entry *hello_propagate_list;
extern gw_entry *hello_gw_list;		/* List of defined and learned HELLO gateways */
extern adv_entry **hello_int_accept;	/* List of accept lists per interface */
extern adv_entry **hello_int_propagate;	/* List of propagate lists per interface */

extern void hello_init();

#endif				/* PROTO_HELLO */
