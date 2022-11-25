/*
 *  $Header: rt_control.c,v 1.1.109.5 92/02/28 16:01:31 ash Exp $
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

adv_entry *martian_list = NULL;		/* List of Martian nets */
unsigned int adv_n_allocated = 0;	/* Number of adv's allocated */

bits gw_bits[] =
{
    {GWF_SOURCE, "Source"},
    {GWF_TRUSTED, "Trusted"},
    {GWF_ACCEPT, "Accept"},
    {GWF_REJECT, "Reject"},
    {GWF_QUERY, "Query"},
    {0, NULL}
};


/*
 *	Allocate an adv_entry.
 */
adv_entry *
#ifdef	USE_PROTOTYPES
adv_alloc(flag_t flags, proto_t proto)
#else				/* USE_PROTOTYPES */
adv_alloc(flags, proto)
flag_t flags;
proto_t proto;

#endif				/* USE_PROTOTYPES */
{
    adv_entry *ale;

    /* Allocate an adv_list entry and put address into it */
    ale = (adv_entry *) calloc(1, sizeof(adv_entry));
    if (!ale) {
	trace(TR_ALL, LOG_ERR, "adv_alloc: calloc: %m");
	quit(errno);
    }
    ale->adv_refcount = 1;
    ale->adv_flag = flags;
    ale->adv_proto = proto;
    trace(TR_PARSE, 0, "adv_alloc: node %X proto %s flags %X refcount %d",
	  ale,
	  trace_bits(rt_proto_bits, ale->adv_proto),
	  ale->adv_flag,
	  ale->adv_refcount);
    adv_n_allocated++;
    return (ale);
}


/*	Free an adv_entry list	*/
void
adv_free_list(adv)
adv_entry *adv;
{
    static int level = 0;
    const char *tabs = "\t\t\t\t\t\t";
    int allocated = adv_n_allocated;
    register adv_entry *advn;

    level++;

    while (adv) {
	advn = adv;
	adv = adv->adv_next;
	trace(TR_PARSE, 0, "adv_free_list:%.*snode %X proto %s flags %X refcount %d",
	      level,
	      tabs,
	      advn,
	      trace_bits(rt_proto_bits, advn->adv_proto),
	      advn->adv_flag,
	      advn->adv_refcount);
	if (!--advn->adv_refcount) {
	    switch (advn->adv_flag & ADVF_TYPE) {
		case ADVF_TANY:
		case ADVF_TGW:
		case ADVF_TINTF:
		case ADVF_TAS:
		    adv_free_list(advn->adv_list);
		    break;
		case ADVF_TDM:
		    break;
		default:
		    trace(TR_ALL, LOG_ERR, "adv_free_list: Unknown type %x in adv_entry",
			  advn->adv_flag & ADVF_TYPE);
		    quit(EBADF);
	    }
	    (void) free((caddr_t) advn);
	    adv_n_allocated--;
	}
    }
    if (allocated != adv_n_allocated) {
	trace(TR_PARSE, 0, "adv_free_list:%.*s%d of %d freed",
	      level,
	      tabs,
	      allocated - adv_n_allocated,
	      allocated);
    }
    level--;
}


/*
 *	Cleanup for a protocol
 */
void
adv_cleanup(n_trusted, n_source, gw_list, accept_list, propagate_list, int_accept, int_propagate)
int *n_trusted;
int *n_source;
gw_entry *gw_list;
adv_entry **accept_list;
adv_entry **propagate_list;
adv_entry ***int_accept;
adv_entry ***int_propagate;
{
    gw_entry *gwp;

    /* Reset gateway list */
    if (n_trusted) {
	*n_trusted = 0;
    }
    if (n_source) {
	*n_source = 0;
    }

    GW_LIST(gw_list, gwp) {
	gwp->gw_flags &= ~(GWF_TRUSTED | GWF_SOURCE);
	adv_free_list(gwp->gw_accept);
	gwp->gw_accept = (adv_entry *) 0;
	adv_free_list(gwp->gw_propagate);
	gwp->gw_propagate = (adv_entry *) 0;
    } GW_LISTEND;

    /* Free accept an propagate lists */
    if (accept_list && *accept_list) {
	adv_free_list(*accept_list);
	*accept_list = (adv_entry *) 0;
    }

    if (propagate_list && *propagate_list) {
	adv_free_list(*propagate_list);
	*propagate_list = (adv_entry *) 0;
    }

    /* Free the interface accept list */
    if (int_accept && *int_accept) {
	int i = int_index_max;

	do {
	    adv_free_list((*int_accept)[i]);
	} while (i--);
	(void) free((caddr_t) * int_accept);
	*int_accept = (adv_entry **) 0;
    }

    if (int_propagate && *int_propagate) {
	int i = int_index_max;

	do {
	    adv_free_list((*int_propagate)[i]);
	} while (i--);
	(void) free((caddr_t) * int_propagate);
	*int_propagate = (adv_entry **) 0;
    }
}


/*	Look for the specified address in the specified list	*/
static adv_entry *
dmlist_match(list, addr)
adv_entry *list;
sockaddr_un *addr;
{
    ADV_LIST(list, list) {
	if (addr->a.sa_family != list->adv_dm.dm_dest.a.sa_family) {
	    continue;
	}
	if ((socktype_in(addr)->sin_addr.s_addr & list->adv_dm.dm_mask.in.sin_addr.s_addr) ==
	    list->adv_dm.dm_dest.in.sin_addr.s_addr) {
	    break;
	}
    } ADV_LISTEND;
    return (list);
}


/*
 *	Determine if a route is valid to an interior protocol
 */
int
propagate(rt, proto, proto_list, int_list, gw_list, metric)
rt_entry *rt;
proto_t proto;
adv_entry *proto_list;
adv_entry *int_list;
adv_entry *gw_list;
metric_t *metric;
{
    int i, success, set_met, match = FALSE;
    adv_entry *adv = NULL;
    adv_entry *list = NULL;
    adv_entry *sublist = NULL;
    adv_entry *lists[3];

    /* Build an array of lists to ease processing */
    lists[0] = proto_list;
    lists[1] = int_list;
    lists[2] = gw_list;

    if (proto) {
	/* Default is to propagate this protocol and direct routes */
	success = (rt->rt_proto & (RTPROTO_DIRECT | proto)) ? TRUE : FALSE;

	/* If propagating the same protocol, use incoming metric */
	if (rt->rt_proto == proto) {
	    *metric = rt->rt_metric;
	    set_met = FALSE;
	} else {
	    set_met = TRUE;
	}
    } else {
	/* Default is to propagate only directly attached networks */
	success = (rt->rt_proto & RTPROTO_DIRECT) ? TRUE : FALSE;
	set_met = TRUE;
    }

    /* Repeat for each list, gw, int and proto */
    i = 2;
    do {
	if (lists[i]) {
	    ADV_LIST(lists[i], list) {
		ADV_LIST(list->adv_list, sublist) {
		    if (rt->rt_proto == sublist->adv_proto) {
			switch (sublist->adv_flag & ADVF_TYPE) {
			    case ADVF_TANY:
				match = TRUE;
				break;
			    case ADVF_TGW:
				if (rt->rt_sourcegw == sublist->adv_gwp) {
				    match = TRUE;
				}
				break;
			    case ADVF_TINTF:
				if (rt->rt_ifp == sublist->adv_ifp) {
				    match = TRUE;
				}
				break;
			    case ADVF_TAS:
				if (rt->rt_as == sublist->adv_as) {
				    match = TRUE;
				}
				break;
			    case ADVF_TDM:
				/* XXX - should not happen */
				break;
			}
			if (match) {
			    if (sublist->adv_list) {
				if (adv = dmlist_match(sublist->adv_list, &rt->rt_dest)) {
				    goto Match;
				}
				match = FALSE;
			    } else {
				success = TRUE;
				goto Match;
			    }
			}
		    }
		} ADV_LISTEND ;
	    } ADV_LISTEND ;
	    success = FALSE;
	}
    } while (i--);

 Match:
    if (match) {
	if (adv && (adv->adv_flag & ADVF_NO)) {
	    success = FALSE;
	} else {
	    success = TRUE;
	    if (set_met) {
		if (adv && (adv->adv_flag & ADVF_OTMETRIC)) {
		    *metric = adv->adv_metric;
		} else if (sublist->adv_flag & ADVF_OTMETRIC) {
		    *metric = sublist->adv_metric;
		} else if (list->adv_flag & ADVF_OTMETRIC) {
		    *metric = list->adv_metric;
		}
	    }
	}
    }
    return (success);
}


/*
 *  Determine if a route is valid from an interior protocol.  The default
 *  action and preference should be preset into the preference and success
 *  arguments.
 */
int
is_valid_in(dst, proto_list, int_list, gw_list, preference)
sockaddr_un *dst;
adv_entry *proto_list;
adv_entry *int_list;
adv_entry *gw_list;
pref_t *preference;
{
    int i, success = TRUE;
    adv_entry *adv = NULL;
    adv_entry *list = NULL;
    adv_entry *lists[3];

    /* Build an array of lists to ease processing */
    lists[0] = proto_list;
    lists[1] = int_list;
    lists[2] = gw_list;

    /* Repeat for each list, gw, int and proto */
    i = 2;
    do {
	if (lists[i]) {
	    ADV_LIST(lists[i], list) {
		if (adv = dmlist_match(list->adv_list, dst)) {
		    break;
		}
	    } ADV_LISTEND;
	    success = FALSE;
	}
    } while (i-- && !adv);

    if (adv) {
	if (adv->adv_flag & ADVF_NO) {
	    success = FALSE;
	} else {
	    success = TRUE;
	    if (adv->adv_flag & ADVF_OTPREFERENCE) {
		*preference = adv->adv_preference;
	    } else if (list->adv_flag & ADVF_OTPREFERENCE) {
		*preference = list->adv_preference;
	    }
	}
    }
    return (success);
}


static void
martian_init()
{
    const char **ptr;
    struct sockaddr_in net;
    adv_entry *adv;
    static const char *martian_nets[] =
    {
	"127.0.0.0", "255.0.0.0",
	"128.0.0.0", "255.255.0.0",
	"191.255.0.0", "255.255.255.0",
	"192.0.0.0", "255.255.255.0",
	"223.255.255.0", "255.255.255.0",
	"224.0.0.0", "255.0.0.0",
	NULL, NULL,
    };

    sockclear_in(&net);

    for (ptr = martian_nets; *ptr; ptr++) {
	adv = (adv_entry *) calloc(1, sizeof(*adv));
	if (!adv) {
	    trace(TR_ALL, LOG_ERR, "martian_init: calloc %m");
	}
	/* XXX - Should be protocol independent */
	sockclear_in(&adv->adv_dm.dm_dest);
	sockclear_in(&adv->adv_dm.dm_mask);
	adv->adv_dm.dm_dest.in.sin_addr.s_addr = inet_addr(*ptr);
	adv->adv_dm.dm_mask.in.sin_addr.s_addr = inet_addr(*(++ptr));;
	adv->adv_next = martian_list;
	martian_list = adv;
    }
}


/*  Return true if said network is a martian */
int
is_martian(dst)
sockaddr_un *dst;
{
    return (dmlist_match(martian_list, dst) ? 1 : 0);
}


void
control_init()
{

    martian_init();
}


/*
 *	Lookup a gateway entry
 */
gw_entry *
gw_lookup(list, proto, addr)
gw_entry **list;
proto_t proto;
sockaddr_un *addr;
{
    gw_entry *gwp;

    GW_LIST(*list, gwp) {
	if ((gwp->gw_proto == proto) && equal(&gwp->gw_addr, addr)) {
	    break;
	}
    } GW_LISTEND;

    return (gwp);
}


/*
 *	Add a gateway entry to end of the list
 */
gw_entry *
gw_add(list, proto, addr)
gw_entry **list;
proto_t proto;
sockaddr_un *addr;
{
    gw_entry *gwp, *gwp_new;

    gwp_new = (gw_entry *) calloc(1, sizeof(gw_entry));
    if (!gwp_new) {
	trace(TR_ALL, LOG_ERR, "gw_add: calloc: %m");
	quit(errno);
    }
    if (*list) {
	GW_LIST(*list, gwp) {
	    if (!gwp->gw_next) {
		gwp->gw_next = gwp_new;
		break;
	    }
	} GW_LISTEND;
    } else {
	*list = gwp_new;
    }
    gwp_new->gw_addr = *addr;		/* struct copy */
    gwp_new->gw_proto = proto;
    return (gwp_new);
}


/*
 *	Find an existing gw_entry or create a new one
 */
gw_entry *
gw_locate(list, proto, addr)
gw_entry **list;
proto_t proto;
sockaddr_un *addr;
{
    gw_entry *gwp;

    gwp = gw_lookup(list, proto, addr);
    if (!gwp) {
	gwp = gw_add(list, proto, addr);
    }
    return (gwp);
}


/*
 *	Update last heard from timer for a gateway
 */
gw_entry *
gw_timestamp(list, proto, addr)
gw_entry **list;
proto_t proto;
sockaddr_un *addr;
{
    gw_entry *gwp;

    gwp = gw_locate(list, proto, addr);
    gwp->gw_time = time_sec;
    return (gwp);
}


/*
 *	Dump gateway information
 */
void
gw_dump(fd, name, list)
FILE *fd;
const char *name;
gw_entry *list;
{
    gw_entry *gwp;

    GW_LIST(list, gwp) {
	(void) fprintf(fd, "%s %-20A",
		       name,
		       &gwp->gw_addr);
	if (gwp->gw_time) {
	    (void) fprintf(fd, " last update: %T",
			   gwp->gw_time);
	}
	if (gwp->gw_flags) {
	    (void) fprintf(fd, " flags: <%s>", trace_bits(gw_bits, gwp->gw_flags));
	}
	(void) fprintf(fd, "\n");
    } GW_LISTEND;
}


/*
 *	Dump a dest/mask list displaying metric and preference if present
 */
void
control_dmlist_dump(fd, level, name, list)
FILE *fd;
int level;
char *name;
adv_entry *list;
{
    adv_entry *adv;
    const char *tabs = "\t\t\t\t\t\t\t";

    ADV_LIST(list, adv) {
	(void) fprintf(fd, "%.*s%s%.*s%s%-15A  mask %-15A",
		       level,
		       tabs,
		       (adv->adv_flag & ADVF_NO) ? "no" : "",
		       name ? strlen(name) : 0,
		       name,
		       name ? "\t" : "",
		       &adv->adv_dm.dm_dest,
		       &adv->adv_dm.dm_mask);
	switch (adv->adv_flag & ADVF_OTYPE) {
	    case ADVF_OTNONE:
		(void) fprintf(fd, "\n");
		break;
	    case ADVF_OTMETRIC:
		(void) fprintf(fd, "  metric %d\n", adv->adv_metric);
		break;
	    case ADVF_OTPREFERENCE:
		(void) fprintf(fd, "  preference %d\n", adv->adv_preference);
		break;
	}
    } ADV_LISTEND
}


/*
 *	Dump the policy database
 */
void
control_dump(fd)
FILE *fd;
{
    /* Martian networks */
    (void) fprintf(fd, "Martians:\n");
    control_dmlist_dump(fd, 2, (char *) 0, martian_list);
}


void
control_accept_dump(fd, level, proto_list, int_list, gw_list)
FILE *fd;
int level;
adv_entry *proto_list;
adv_entry **int_list;
gw_entry *gw_list;
{
    int interface;
    int lower;
    adv_entry *adv;
    gw_entry *gwp;
    const char *tabs = "\t\t\t\t\t\t\t";

    if (proto_list || int_list || gw_list) {
	(void) fprintf(fd, "%.*sAccept controls:\n",
		       level++, tabs);
    }
    if (proto_list) {
	ADV_LIST(proto_list, adv) {
	    lower = level;
	    if (adv->adv_flag & ADVF_OTPREFERENCE) {
		(void) fprintf(fd, "%.*sPreference %d:\n",
			       level, tabs,
			       adv->adv_preference);
		lower++;
	    }
	    control_dmlist_dump(fd, lower, "listen", adv->adv_list);
	} ADV_LISTEND
    }
    if (int_list) {
	for (interface = 0; interface <= int_index_max; interface++) {
	    adv = int_list[interface];
	    if (!adv) {
		continue;
	    }
	    lower = level + 1;
	    (void) fprintf(fd, "%.*sInterface %s  Address %A:\n",
			   level, tabs,
			   adv->adv_ifp->int_name,
			   &adv->adv_ifp->int_addr);
	    if (adv->adv_flag & ADVF_OTPREFERENCE) {
		(void) fprintf(fd, "%.*sPreference %d:\n",
			       level + 1, tabs,
			       adv->adv_preference);
		lower++;
	    }
	    control_dmlist_dump(fd, lower, "listen", adv->adv_list);
	}
    }
    if (gw_list) {
	GW_LIST(gw_list, gwp) {
	    adv = gwp->gw_accept;
	    if (!adv) {
		continue;
	    }
	    lower = level + 1;
	    (void) fprintf(fd, "%.*sGateway %A:\n",
			   level, tabs,
			   &gwp->gw_addr);
	    if (adv->adv_flag & ADVF_OTPREFERENCE) {
		(void) fprintf(fd, "%.*sPreference %d:\n",
			       level + 1, tabs,
			       adv->adv_preference);
		lower++;
	    }
	    control_dmlist_dump(fd, lower, "listen", adv->adv_list);
	} GW_LISTEND;
    }
}


static void
control_entry_dump(fd, level, list)
FILE *fd;
int level;
adv_entry *list;
{
    int first = TRUE;
    const char *tabs = "\t\t\t\t\t\t\t";
    adv_entry *adv;

    if (list) {
	(void) fprintf(fd, "%.*s", level, tabs);
	if (list->adv_proto) {
	    (void) fprintf(fd, "Protocol %s ",
			   trace_bits(rt_proto_bits, list->adv_proto));
	}
	adv = list;
	if (adv) {
	    do {
		switch (adv->adv_flag & ADVF_TYPE) {
		    case ADVF_TDM:
			/* XXX - should not happen */
		    case ADVF_TANY:
			break;
		    case ADVF_TGW:
			(void) fprintf(fd, " %s%A",
				       first ? "gateway " : "",
				       &adv->adv_gwp->gw_addr);
			break;
		    case ADVF_TINTF:
			(void) fprintf(fd, " %s%A(%s)",
				       first ? "interface " : "",
				       &adv->adv_ifp->int_addr,
				       adv->adv_ifp->int_name);
			break;
		    case ADVF_TAS:
			(void) fprintf(fd, " %s%u",
				       first ? "as " : "",
				       adv->adv_as);
			break;
		}
		first = FALSE;
	    } while ((adv = adv->adv_next) && !(adv->adv_flag & ADVF_FIRST));
	}
	switch (list->adv_flag & ADVF_OTYPE) {
	    case ADVF_OTNONE:
		break;
	    case ADVF_OTMETRIC:
		(void) fprintf(fd, " metric %d", list->adv_metric);
		break;
	    case ADVF_OTPREFERENCE:
		(void) fprintf(fd, " preference %d", list->adv_preference);
		break;
	}
    }
    (void) fprintf(fd, "\n");
}


void
control_propagate_list_dump(fd, level, list)
FILE *fd;
int level;
adv_entry *list;
{
    int lower;
    adv_entry *adv;
    const char *tabs = "\t\t\t\t\t\t\t";

    if (list) {
	ADV_LIST(list, list) {
	    lower = level;
	    if (list->adv_flag & ADVF_OTMETRIC) {
		(void) fprintf(fd, "%.*sMetric %d:\n",
			       level, tabs,
			       list->adv_metric);
		lower++;
	    }
	    adv = list->adv_list;
	    if (adv) {
		do {
		    control_entry_dump(fd, lower, adv);
		    if (adv->adv_list) {
			control_dmlist_dump(fd, lower + 1, "announce", adv->adv_list);
		    }
		    do {
			adv = adv->adv_next;
		    } while (adv && !(adv->adv_flag & ADVF_FIRST));
		} while (adv);
	    }
	} ADV_LISTEND
    }
}


void
control_propagate_dump(fd, level, proto_list, int_list, gw_list)
FILE *fd;
int level;
adv_entry *proto_list;
adv_entry **int_list;
gw_entry *gw_list;
{
    int interface;
    adv_entry *adv;
    gw_entry *gwp;
    const char *tabs = "\t\t\t\t\t\t\t";

    if (proto_list || int_list || gw_list) {
	(void) fprintf(fd, "%.*sPropagate controls:\n",
		       level++, tabs);
    }
    control_propagate_list_dump(fd, level, proto_list);

    if (int_list) {
	for (interface = 0; interface <= int_index_max; interface++) {
	    adv = int_list[interface];
	    if (adv) {
		(void) fprintf(fd, "%.*sInterface %s  Address %A:\n",
			       level, tabs,
			       adv->adv_ifp->int_name,
			       &adv->adv_ifp->int_addr);
		control_propagate_list_dump(fd, level + 1, adv);
	    }
	}
    }
    if (gw_list) {
	GW_LIST(gw_list, gwp) {
	    adv = gwp->gw_propagate;
	    if (adv) {
		(void) fprintf(fd, "%.*sGateway %A:\n",
			       level, tabs,
			       &gwp->gw_addr);
		control_propagate_list_dump(fd, level + 1, adv);
	    }
	} GW_LISTEND;
    }
}


void
control_exterior_dump(fd, level, func, list)
FILE *fd;
int level;
void (*func) ();
adv_entry *list;
{
    adv_entry *adv;
    const char *tabs = "\t\t\t\t\t\t\t";

    ADV_LIST(list, adv) {
	(void) fprintf(fd, "%.*sAS %u:\n",
		       level, tabs,
		       adv->adv_as);
	func(fd, level + 1, adv->adv_list, (adv_entry **) 0, (gw_entry *) 0);
	(void) fprintf(fd, "\n");
    } ADV_LISTEND;
}


adv_entry *
#ifdef	USE_PROTOTYPES
control_exterior_locate(adv_entry * list, as_t as)
#else				/* USE_PROTOTYPES */
control_exterior_locate(list, as)
adv_entry *list;
as_t as;

#endif				/* USE_PROTOTYPES */
{
    if (list) {
	ADV_LIST(list, list) {
	    if (list->adv_as == as) {
		break;
	    }
	} ADV_LISTEND;
    }
    return ((list && list->adv_list) ? list->adv_list : list);
}
