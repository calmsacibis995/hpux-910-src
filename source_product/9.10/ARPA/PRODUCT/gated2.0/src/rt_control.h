/*
 * $Header: rt_control.h,v 1.1.109.5 92/02/28 16:01:35 ash Exp $
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


typedef struct _dest_mask {
    sockaddr_un dm_dest;
    sockaddr_un dm_mask;
} dest_mask;


/*
 *	Structure describing a gateway
 */
typedef struct _gw_entry {
    struct _gw_entry *gw_next;
    proto_t gw_proto;			/* Protocol of this gateway */
    sockaddr_un gw_addr;		/* Address of this gateway */
    flag_t gw_flags;			/* Flags for this gateway */
    time_t gw_time;			/* Time this gateway was last heard from */
    struct _adv_entry *gw_accept;	/* What to accept from this gateway */
    struct _adv_entry *gw_propagate;	/* What to propagate to this gateway */
} gw_entry;

#define	GWF_SOURCE	0x01		/* This is a source gateway */
#define	GWF_TRUSTED	0x02		/* This is a trusted gateway */
#define	GWF_ACCEPT	0x04		/* We accepted a packet from this gateway */
#define	GWF_REJECT	0x08		/* We rejected a packet from this gateway */
#define	GWF_QUERY	0x10		/* RIP query packet received */


/*
 *	Structure used for all control lists.  Nested unions are used
 *	to minimize unused space.
 */
typedef struct _adv_entry {
    struct _adv_entry *adv_next;	/* Pointer to next entry in list */
    flag_t adv_flag;			/* Flags */
    proto_t adv_proto;			/* Protocol for this match */
    union {
	dest_mask advu_dm;
	struct {
	    union {
		gw_entry *advu_gwp;	/* Match a gateway address */
		struct _if_entry *advu_ifp;	/* Match an interface */
		as_t advu_as;		/* Match an AS */
#ifdef	notdef
		pathmatch_t advu_path;	/* Match on AS path */
#endif	/* notdef */
	    } adv_s;
	    struct _adv_entry *advu_list;	/* List of allowed sources */
	} adv_us;
    } adv_u;
#define	adv_dm	adv_u.advu_dm
#define	adv_gwp	adv_u.adv_us.adv_s.advu_gwp
#define	adv_ifp	adv_u.adv_us.adv_s.advu_ifp
#define	adv_as	adv_u.adv_us.adv_s.advu_as
#define	adv_path	adv_u.adv_us.adv_s.advu_path
#define	adv_list	adv_u.adv_us.advu_list
    union {
	metric_t advu_metric;		/* Use this metric */
	pref_t advu_preference;		/* Use this preference */
    } adv_v;
#define	adv_metric	adv_v.advu_metric
#define	adv_preference	adv_v.advu_preference
    u_char adv_refcount;		/* Number of references */
} adv_entry;

#define	ADVF_TYPE		0x0f	/* Type to match */
#define	ADVF_TANY		0x00	/* No type specified */
#define	ADVF_TGW		0x01	/* Match gateway address */
#define	ADVF_TINTF		0x02	/* Match interface */
#define	ADVF_TAS		0x03	/* Match on AS */
#define	ADVF_TPATH		0x04	/* Match on AS path */
#define	ADVF_TDM		0x05	/* Match on dest/mask pair */

#define	ADVF_OTYPE		0xf0	/* Option type */
#define	ADVF_OTNONE		0x00	/* No option specified */
#define	ADVF_OTMETRIC		0x10	/* Metric option */
#define	ADVF_OTPREFERENCE	0x20	/* Preference option */

#define	ADVF_NO			0x1000	/* Negative (i.e. noannounce, nolisten, nopropogate) */
#define	ADVF_FIRST		0x2000	/* First entry in a sequence (of gateways or interfaces) */


#define	GW_LIST(list, gwp)	for (gwp = list; gwp; gwp = gwp->gw_next)
#define	GW_LISTEND

#define	ADV_LIST(list, adv)	for (adv = list; adv; adv = adv->adv_next)
#define	ADV_LISTEND


#define	INT_CONTROL(list, ifp)	list ? list[ifp->int_index - 1] : NULL

extern adv_entry *martian_list;
extern unsigned int adv_n_allocated;

#ifdef	USE_PROTOTYPES
extern void control_dump(FILE * fd);
extern void control_init(void);
extern void
control_accept_dump(FILE * fd,
		    int level,
		    adv_entry * proto_list,
		    adv_entry ** int_list,
		    gw_entry * gw_list);
extern void
control_propagate_dump(FILE * fd,
		       int level,
		       adv_entry * proto_list,
		       adv_entry ** int_list,
		       gw_entry * gw_list);
extern void
control_exterior_dump(FILE * fd,
		      int level,
		      void (*func) (),
		      adv_entry * list);
extern adv_entry *
control_exterior_locate(adv_entry * list,
			as_t as);

extern int
is_valid_in(sockaddr_un * dst,
	    adv_entry * proto_list,
	    adv_entry * int_list,
	    adv_entry * gw_list,
	    int *preference);
extern int
propagate(struct _rt_entry * rt,
	  proto_t proto,
	  adv_entry * proto_list,
	  adv_entry * int_list,
	  adv_entry * gw_list,
	  metric_t * metric);
extern int is_martian(sockaddr_un * dst);

extern adv_entry *adv_alloc(flag_t flags, proto_t proto);
extern void adv_free_list(adv_entry * adv);
extern void
adv_cleanup(int *n_trusted,
	    int *n_source,
	    gw_entry * gw_list,
	    adv_entry ** accept_list,
	    adv_entry ** propagate_list,
	    adv_entry *** int_accept,
	    adv_entry *** int_propagate);

extern gw_entry *
gw_lookup(gw_entry ** list,
	  proto_t proto,
	  sockaddr_un * addr);
extern gw_entry *
gw_add(gw_entry ** list,
       proto_t proto,
       sockaddr_un * addr);
extern gw_entry *
gw_locate(gw_entry ** list,
	  proto_t proto,
	  sockaddr_un * addr);
extern gw_entry *
gw_timestamp(gw_entry ** list,
	     proto_t proto,
	     sockaddr_un * addr);
extern void
gw_dump(FILE * fd,
	const char *name,
	gw_entry * list);

#else				/* USE_PROTOTYPES */
extern void control_dump();
extern void control_init();
extern void control_accept_dump();
extern void control_propagate_dump();
extern void control_exterior_dump();
extern adv_entry *control_exterior_locate();

extern int is_valid_in();
extern int propagate();
extern int propagate_as();
extern int is_martian();

extern adv_entry *adv_alloc();		/* Allocate an adv_entry */
extern void adv_free_list();		/* Free an adv_entry list */
extern void adv_cleanup();		/* Free all adv_entries for a protocol */

extern gw_entry *gw_lookup();
extern gw_entry *gw_add();
extern gw_entry *gw_locate();
extern gw_entry *gw_timestamp();
extern void gw_dump();

#endif				/* USE_PROTOTYPES */
