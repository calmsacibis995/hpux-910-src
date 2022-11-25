/*
 *  $Header: parse.c,v 1.1.109.5 92/02/28 15:59:03 ash Exp $
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


#include	"include.h"
#ifdef	SYSV
#include	<dirent.h>
#endif	/* SYSV */
#include	"parse.h"
#include	"parser.h"
#include	"rip.h"
#include	"hello.h"
#include	"egp.h"
#include	"bgp.h"
#include	"snmp.h"

static int n_keywords;

/*
 *	Table of keywords recognized.  This table is sorted at runtime
 *	to facilitate binary searching.
 */

static bits keywords[] =
{
    {T_INTERFACE, "interface"},
    {T_INTERFACE, "intf"},
    {T_DIRECT, "direct"},
    {T_PROTO, "protocol"},
    {T_PROTO, "proto"},
    {T_METRIC, "metric"},
    {T_LEX, "lex"},
    {T_PARSE, "yacc"},
    {T_PARSE, "parse"},
    {T_CONFIG, "config"},
    {T_DEFAULT, "default"},
    {T_INCLUDE, "include"},
    {T_DIRECTORY, "directory"},
#if	YYDEBUG != 0
    {T_YYDEBUG, "yydebug"},
    {T_YYSTATE, "yystate"},
    {T_YYQUIT, "yyquit"},
#endif				/* YYDEBUG */
    {T_ON, "on"},
    {T_ON, "yes"},
    {T_OFF, "off"},
    {T_OFF, "no"},
    {T_QUIET, "quiet"},
    {T_POINTOPOINT, "pointopoint"},
    {T_POINTOPOINT, "pointtopoint"},
    {T_POINTOPOINT, "p2p"},
    {T_SUPPLIER, "supplier"},
    {T_GATEWAY, "gateway"},
    {T_GATEWAY, "gw"},
    {T_PREFERENCE, "preference"},
    {T_PREFERENCE, "pref"},
    {T_DEFAULTMETRIC, "defaultmetric"},
#if	defined(PROTO_EGP) || defined(PROTO_BGP)
    {T_NEIGHBOR, "neighbor"},
    {T_NEIGHBOR, "peer"},
    {T_ASIN, "asin"},
    {T_ASOUT, "asout"},
    {T_METRICOUT, "metricout"},
    {T_VERSION, "version"},
    {T_GENDEFAULT, "gendefault"},
    {T_NOGENDEFAULT, "nogendefault"},
    {T_DEFAULTIN, "acceptdefault"},
    {T_DEFAULTOUT, "propagatedefault"},
#endif				/* defined(PROTO_EGP) || defined(PROTO_BGP) */
#ifdef	PROTO_EGP
    {T_EGP, "egp"},
    {T_GROUP, "group"},
    {T_MAXUP, "maxup"},
    {T_SOURCENET, "sourcenet"},
    {T_P1, "p1"},
    {T_P1, "minhello"},
    {T_P2, "p2"},
    {T_P2, "minpoll"},
    {T_PKTSIZE, "packetsize"},
    {T_PKTSIZE, "packet-size"},
#endif				/* PROTO_EGP */
#ifdef	PROTO_BGP
    {T_BGP, "bgp"},
    {T_LINKTYPE, "linktype"},
    {T_INTERNAL, "internal"},
    {T_HORIZONTAL, "horizontal"},
    {T_HOLDTIME, "holdtime"},
#endif				/* PROTO_BGP */
#if	defined(PROTO_RIP) || defined(PROTO_HELLO)
    {T_TRUSTEDGATEWAYS, "trustedgateways"},
    {T_SOURCEGATEWAYS, "sourcegateways"},
#endif				/* defined(PROTO_RIP) || defined(PROTO_HELLO) */
#ifdef	PROTO_RIP
    {T_RIP, "rip"},
    {T_NORIPOUT, "noripout"},
    {T_NORIPIN, "noripin"},
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
    {T_HELLO, "hello"},
    {T_NOHELLOOUT, "nohelloout"},
    {T_NOHELLOIN, "nohelloin"},
#endif				/* PROTO_HELLO */
#ifdef	PROTO_ICMP
    {T_ICMP, "icmp"},
    {T_NOICMPIN, "noicmpin"},
#endif				/* PROTO_ICMP */
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
    {T_REDIRECT, "redirect"},
    {T_REDIRECT, "redirects"},
#endif				/* defined(PROTO_ICMP) || defined(RTM_ADD) */
#ifdef	AGENT_SNMP
    {T_SNMP, "snmp"},
#endif				/* AGENT_SNMP */
    {T_PASSIVE, "passive"},
    {T_STATIC, "static"},
    {T_ANNOUNCE, "announce"},
    {T_NOANNOUNCE, "noannounce"},
    {T_LISTEN, "listen"},
    {T_NOLISTEN, "nolisten"},
    {T_MARTIANS, "martians"},
    {T_AS, "as"},
    {T_AS, "autonomoussystem"},
    {T_AS, "autonomous-system"},
    {T_PROPAGATE, "propagate"},
    {T_ACCEPT, "accept"},
    {T_RESTRICT, "restrict"},
    {T_NORESTRICT, "norestrict"},
    {T_MASK, "mask"},
    {T_OPTIONS, "options"},
    {T_NOINSTALL, "noinstall"},
    {T_TRACEOPTIONS, "traceoptions"},
    {T_TRACEFILE, "tracefile"},
    {T_REPLACE, "replace"},
    {T_ALL, "all"},
    {T_NONE, "none"},
    {T_GENERAL, "general"},
    {T_EXTERNAL, "external"},
    {T_ROUTE, "route"},
    {T_UPDATE, "update"},
    {T_KERNEL, "kernel"},
    {T_TASK, "task"},
    {T_TIMER, "timer"},
    {T_NOSTAMP, "nostamp"},
    {T_MARK, "mark"},
    {0, NULL}
};


/*
 *	A string of alpha characters which is either a keyword or a host/network name.
 *	First do a binary search (Knuth 6.2.1) to lookup up a keyword, if it is not found
 *	assume it is a host/network name.  The proper thing to do is a REJECT, but flex
 *	won't optimize if we use a REJECT.
 */
int
parse_keyword(yytext)
char *yytext;
{
    int c;
    bits *i, *l, *u, *p = (bits *) 0;

    l = keywords;
    u = &keywords[n_keywords - 1];
    do {
	i = (u - l) / 2 + l;
	c = strcasecmp(yytext, i->t_name);
	if (!c) {
	    p = i;
	    break;
	} else if (c < 0) {
	    u = i - 1;
	} else {
	    l = i + 1;
	}
    } while (u >= l);

    return (p ? p->t_bits : UNKNOWN);
}


/*
 *	Look up a token name given it's ID in the keyword table.  There is no way to select
 *	between multiple keywords mapping to the same token once the table is sorted.
 */
const char *
parse_keyword_lookup(token)
int token;
{
    bits *p;
    static char invalid_string[2];
    static bits special_keywords[] =
    {
	{EOS, "end-of-statement"},
	{UNKNOWN, "unknown-keyword"},
	{0, NULL}
    };

    if (token > 0 && token < 257 /* XXX - ASCII dependency here*/ ) {
	*invalid_string = token;
	return (invalid_string);
    }
    for (p = keywords; p->t_name; p++) {
	if (token == p->t_bits) {
	    return (p->t_name);
	}
    }

    for (p = special_keywords; p->t_name; p++) {
	if (token == p->t_bits) {
	    return (p->t_name);
	}
    }

    return ((char *) 0);
}


/*
 *	A comparison routine used by the sorting routine that follows
 */
static int
parse_keyword_sort_compare(p1, p2)
bits *p1, *p2;
{
    return (strcasecmp(p1->t_name, p2->t_name));
}


/*
 *	Quicksort the table of keywords to insure that they are in order, easier
 *	than trying to keep the source sorted and less prone to mistakes.
 *
 *	The sort is only done the first time we are called, i.e. when n_keyword is
 *	zero.
 */
static void
parse_keyword_sort()
{
    bits *p;

    if (!n_keywords) {
	/* Calculate size of table */
	for (p = keywords; p->t_name; p++, n_keywords++) ;

	qsort((caddr_t) keywords, n_keywords, sizeof(bits), parse_keyword_sort_compare);
    }
}


/*
 *	Front end for yyparse().  Exit if any errors
 */
int
parse_parse(file)
const char *file;
{
    int errors;
    extern int yynerrs;
    static int first_parse = TRUE;

    if (first_parse) {
	parse_keyword_sort();		/* Sort the keyword table */
    }

    parse_directory = parse_strdup(task_path_name);
    
    sethostent(1);
    setnetent(1);

    errors = parse_open(parse_strdup(file));
    if (!errors) {
	protos_seen = (proto_t) 0;
	parse_state = PS_DEFINE;
	errors = yyparse();
    }
    errors += yynerrs;

    endhostent();
    endnetent();

    if (errors) {
	trace(TR_ALL, 0, NULL);
	trace(TR_ALL, LOG_ERR, "parse_parse: %d parse error%s", errors, errors > 1 ? "s" : "");
	trace(TR_ALL, 0, NULL);
    }

    if (parse_directory) {
	(void) free(parse_directory);
	parse_directory = (char *) 0;
    }
    first_parse = FALSE;

    return (errors);
}


/*
 *	Duplicate a string and return pointer
 */
char *
parse_strdup(s)
char *s;
{
    char *cp;

    cp = (char *) malloc((u_int) (strlen(s) + 1));
    if (!cp) {
	trace(TR_ALL, LOG_ERR, "parse_strdup: %s malloc: %m",
	      parse_where());
	quit(errno);
    }
    (void) strcpy(cp, s);
    return (cp);
}

/*
 *	Format a message indicating the current line number and return
 *	a pointer to it.
 */
char *
parse_where()
{
    static char where[BUFSIZ];

    if (parse_filename) {
	(void) sprintf(where, "%s:%d",
		       parse_filename,
		       yylineno);
    } else {
	(void) sprintf(where, "%d",
		       yylineno);
    }

    return (where);
}


/*
 *	Limit check a number
 */
int
parse_limit_check(type, value, lower, upper)
const char *type;
int value;
int lower;
int upper;
{
    if ((value < lower) || ((upper != -1) && (value > upper))) {
	(void) sprintf(parse_error, "invalid %s value at '%d'",
		       type,
		       value);
	return (1);
    }
    trace(TR_PARSE, 0, "parse: %s %s: %d",
	  parse_where(),
	  type,
	  value);
    return (0);
}


/*	Translate a u_long IP address into a sockaddr_in	*/
void
parse_addr_long(sockaddr, addr)
sockaddr_un *sockaddr;
u_long addr;
{
    sockclear_in(sockaddr);
    sockaddr->in.sin_addr.s_addr = addr;

    trace(TR_PARSE, 0, "parse: %s IP_ADDR: %A",
	  parse_where(),
	  sockaddr);
}


/*
 *	Look up a string as a host or network name, returning it's IP address
 *	in normalized network byte order.  Also recognizes a network name of
 *	"default", translating to network 0.
 */
int
parse_addr_hname(addr, hname, host_ok, net_ok)
sockaddr_un *addr;
char *hname;
int host_ok, net_ok;
{
    u_long network;
    struct netent *netent;
    struct hostent *hostent;
    int net_unknown = TRUE;
    const char *errmsg = 0;

#ifdef	HOST_NOT_FOUND
    extern int h_errno;
    extern int h_nerr;
    extern char *h_errlist[];

#endif				/* HOST_NOT_FOUND */

    if (net_ok) {
	netent = getnetbyname(hname);
	if (netent) {
	    if (netent->n_addrtype != AF_INET) {
		/* XXX - Should we pretend it does not exist if it is not an INET name? */
		(void) sprintf(parse_error, "network not INET at '%s'",
			       hname);
		return (1);
	    }
	    network = netent->n_net;
	    if (network) {
		while (!(network & 0xff000000)) {
		    network <<= 8;
		}
	    }
	    parse_addr_long(addr, htonl(network));
	    return (0);
#ifdef	HOST_NOT_FOUND
	} else {
	    errmsg = "Unknown network";
	    net_unknown = TRUE;
#endif				/* HOST_NOT_FOUND */
	}
    }
    if (host_ok) {
	hostent = gethostbyname(hname);
	if (hostent) {
	    if (hostent->h_addrtype != AF_INET) {
		/* XXX - Should we pretend it does not exist if it is not an INET name? */
		(void) sprintf(parse_error, "host not INET at '%s'",
			       hname);
		return (1);
	    }
#ifdef	h_addr
	    if (hostent->h_addr_list[1]) {
		/* XXX - For a gateway we could use just the address on our network if there was only one */
		(void) sprintf(parse_error, "host has multiple addresses at '%s'",
			       hname);
		return (1);
	    }
#endif				/* h_addr */
	    memcpy((caddr_t) & network, hostent->h_addr, sizeof(network));
	    parse_addr_long(addr, network);
	    return (0);
	} else {
#ifdef	HOST_NOT_FOUND
	    if ((!h_errno || (h_errno == HOST_NOT_FOUND)) && net_unknown) {
		errmsg = "Unknown host/network";
	    } else if (h_errno && (h_errno < h_nerr)) {
		errmsg = h_errlist[h_errno];
	    } else {
		errmsg = "Unknown host";
	    }
#else				/* HOST_NOT_FOUND */
	    if (net_unknown) {
		errmsg = "Unknown host/network";
	    } else {
		errmsg = "Unknown host";
	    }
#endif				/* HOST_NOT_FOUND */
	}
    }
    (void) sprintf(parse_error, "error resolving '%s': %s",
		   hname,
		   errmsg);
    return (1);
}


/*
 *	Append an advlist to another advlist
 */
adv_entry *
parse_adv_append(old, new, free)
adv_entry *old, *new;
int free;
{
    adv_entry *alo, *aln, *last = NULL;

    /* Add this network to the end of the list */
    if (old) {
	for (alo = old; alo; alo = alo->adv_next) {
	    if (!alo->adv_next) {
		last = alo;
	    }
	    /* Scan list for duplicates */
	    for (aln = new; aln; aln = aln->adv_next) {
		if ((aln->adv_flag & ADVF_TYPE) != (alo->adv_flag & ADVF_TYPE)) {
		    continue;
		}
		switch (aln->adv_flag & ADVF_TYPE) {
		    case ADVF_TANY:
			break;
		    case ADVF_TGW:
			if (aln->adv_gwp == alo->adv_gwp) {
			    (void) sprintf(parse_error, "duplicate gateway in list at '%A'",
					   &aln->adv_gwp->gw_addr);
			    return ((adv_entry *) 0);
			}
			break;
		    case ADVF_TINTF:
			if (aln->adv_ifp == alo->adv_ifp) {
			    (void) sprintf(parse_error, "duplicate interface in list at '%A'",
			    (aln->adv_ifp->int_state & IFS_POINTOPOINT) ?
					   &aln->adv_ifp->int_dstaddr :
					   &aln->adv_ifp->int_addr);
			    return ((adv_entry *) 0);
			}
			break;
		    case ADVF_TAS:
			if (aln->adv_as == alo->adv_as) {
			    (void) sprintf(parse_error, "duplicate autonomous-system in list at '%u'",
					   aln->adv_as);
			    return ((adv_entry *) 0);
			}
			break;
		    case ADVF_TDM:
			/* XXX - make sure same address family */
			if (equal(&aln->adv_dm.dm_dest, &alo->adv_dm.dm_dest) &&
			    equal(&aln->adv_dm.dm_mask, &alo->adv_dm.dm_mask)) {
			    (void) sprintf(parse_error, "duplicate dest and mask in list at '%A mask %A'",
					   &aln->adv_dm.dm_dest,
					   &aln->adv_dm.dm_mask);
			    return ((adv_entry *) 0);
			}
			break;
		}
	    }
	}
	last->adv_next = new;
    } else {
	old = new;
    }
    if (new) {
	/* XXX - bump the refcount, then free it? */
	for (aln = new; aln; aln = aln->adv_next) {
	    aln->adv_refcount++;
	}
	if (free) {
	    adv_free_list(new);
	}
    }
    return (old);
}


/*
 *	Set a flag in the gw structure for each element in a list
 */
int
parse_gw_flag(list, proto, flag)
adv_entry *list;
proto_t proto;
flag_t flag;
{
    int n = 0;
    adv_entry *adv;

    for (adv = list; adv; adv = adv->adv_next) {
	adv->adv_gwp->gw_flags |= flag;
	adv->adv_gwp->gw_proto = proto;
	n++;
    }
    return (n);
}


/*
 *	Display an adv entry
 */
void
parse_adv_entry(list)
adv_entry *list;
{
    int first = TRUE;
    adv_entry *adv;

    if (list) {
	if (list->adv_proto) {
	    tracef("proto %s ",
		   gd_lower(trace_bits(rt_proto_bits, list->adv_proto)));
	}
	adv = list;
	do {
	    switch (adv->adv_flag & ADVF_TYPE) {
		case ADVF_TDM:
		    /* XXX - should not happen */
		case ADVF_TANY:
		    break;
		case ADVF_TGW:
		    tracef("%s%A ",
			   first ? "gateway " : "",
			   &adv->adv_gwp->gw_addr);
		    break;
		case ADVF_TINTF:
		    tracef("%s%A(%s) ",
			   first ? "interface " : "",
			   (adv->adv_ifp->int_state & IFS_POINTOPOINT) ?
			   &adv->adv_ifp->int_dstaddr :
			   &adv->adv_ifp->int_addr,
			   adv->adv_ifp->int_name);
		    break;
		case ADVF_TAS:
		    tracef("%s%d ",
			   first ? "as " : "",
			   adv->adv_as);
		    break;
	    }
	    first = FALSE;
	} while ((adv = adv->adv_next) && !(adv->adv_flag & ADVF_FIRST));
	switch (list->adv_flag & ADVF_OTYPE) {
	    case ADVF_OTNONE:
		break;
	    case ADVF_OTMETRIC:
		tracef("metric %d ", list->adv_metric);
		break;
	    case ADVF_OTPREFERENCE:
		tracef("preference %d ", list->adv_preference);
		break;
	}
    }
}


/*
 *	Display a dest_mask list
 */
void
parse_adv_destmask(dest, name1, name2)
adv_entry *dest;
const char *name1, *name2;
{
    adv_entry *adv;

    if (dest) {
	ADV_LIST(dest, adv) {
	    tracef("parse: %s%s%s%.*s %A mask %A",
		   parse_where(),
		   name1,
		   (adv->adv_flag & ADVF_NO) ? "no" : "",
		   name2 ? strlen(name2) : 0,
		   name2,
		   &adv->adv_dm.dm_dest,
		   &adv->adv_dm.dm_mask);
	    switch (adv->adv_flag & ADVF_OTYPE) {
		case ADVF_OTNONE:
		    break;
		case ADVF_OTMETRIC:
		    tracef(" metric %d", adv->adv_metric);
		    break;
		case ADVF_OTPREFERENCE:
		    tracef(" preference %d", adv->adv_preference);
		    break;
	    }
	    trace(TR_CONFIG, 0, " ;");
	} ADV_LISTEND;
    }
}


/*
 *	Append to an existing list
 */
void
parse_adv_list(name1, name2, list, dest)
const char *name1, *name2;
adv_entry *list, *dest;
{
    if (trace_flags & TR_CONFIG) {
	tracef("parse: %s\t%s ",
	       parse_where(),
	       name1);
	parse_adv_entry(list);
	trace(TR_CONFIG, 0, "{");
	parse_adv_destmask(dest, "\t\t", name2);
	trace(TR_CONFIG, 0, "parse: %s\t} ;",
	      parse_where());
    }
}


/*
 *	Display a propagate clause
 */
void
parse_adv_prop_list(list)
adv_entry *list;
{
    adv_entry *adv;

    if (list) {
	if (trace_flags & TR_CONFIG) {
	    tracef("parse: %s\tpropagate ",
		   parse_where());
	    parse_adv_entry(list);
	    trace(TR_CONFIG, 0, " {");
	    adv = list->adv_list;
	    if (adv) {
		do {
		    tracef("parse: %s\t\t",
			   parse_where());
		    parse_adv_entry(adv);
		    if (adv->adv_list) {
			trace(TR_CONFIG, 0, "{");
			parse_adv_destmask(adv->adv_list, "\t\t\t", "announce");
			tracef("parse: %s\t\t}",
			       parse_where());
		    }
		    trace(TR_CONFIG, 0, " ;");
		    do {
			adv = adv->adv_next;
		    } while (adv && !(adv->adv_flag & ADVF_FIRST));
		} while (adv);
	    }
	    trace(TR_CONFIG, 0, "parse: %s\t} ;",
		  parse_where());
	}
    }
}


/*
 *	Set interface struct with parsed values
 */
static void
parse_interface_set(ifp)
if_entry *ifp;
{
    ifp->int_state |= parse_flags;
    if (parse_metric > 0) {
	ifp->int_metric = parse_metric;
    }
    if (parse_preference) {
	ifp->int_preference = parse_preference;
    }
    if_display("parse", ifp);
}


/*	Set interface flags on specified interface list or all interfaces	*/
void
parse_interface(list)
adv_entry *list;
{
    if_entry *ifp;
    adv_entry *adv;

    if (list) {
	for (adv = list; adv; adv = adv->adv_next) {;
	    parse_interface_set(adv->adv_ifp);
	}
	adv_free_list(list);
    } else {
	IF_LIST(ifp) {
	    parse_interface_set(ifp);
	} IF_LISTEND(ifp) ;
    }
}


/*
 *	Switch to a new state if it is a valid progression from
 *	the current state
 */
int
parse_new_state(state)
int state;
{
    static const char *states[] =
    {
	"define",
	"protocol",
	"route",
	"control"
    };

    if (state < parse_state) {
	(void) sprintf(parse_error, "statement out of order");
	return (1);
    } else if (state > parse_state) {
	parse_state = state;
	trace(TR_PARSE, 0, "parse_new_state: %s %s",
	      parse_where(),
	      states[parse_state]);
    }
    return (0);
}


int
parse_metric_check(proto, metric)
proto_t proto;
int metric;
{
    const char *string;
    int limit_low, limit_high;

    switch (proto) {
#ifdef	PROTO_RIP
	case RTPROTO_RIP:
	    string = "RIP metric";
	    limit_low = LIMIT_RIP_LOW;
	    limit_high = LIMIT_RIP_HIGH;
	    break;
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
	case RTPROTO_HELLO:
	    string = "HELLO metric";
	    limit_low = LIMIT_HELLO_LOW;
	    limit_high = LIMIT_HELLO_HIGH;
	    break;
#endif				/* PROTO_HELLO */
#ifdef	PROTO_EGP
	case RTPROTO_EGP:
	    string = "EGP metric";
	    limit_low = LIMIT_EGP_LOW;
	    limit_high = LIMIT_EGP_HIGH;
	    break;
#endif				/* PROTO_EGP */
#ifdef	PROTO_BGP
	case RTPROTO_BGP:
	    string = "BGP metric";
	    limit_low = LIMIT_BGP_LOW;
	    limit_high = LIMIT_BGP_HIGH;
	    break;
#endif				/* PROTO_BGP */
	case RTPROTO_DIRECT:
	    string = "interface metric";
	    limit_low = LIMIT_INTERFACE_LOW;
	    limit_high = LIMIT_INTERFACE_HIGH;
	    break;
	default:
	    (void) sprintf(parse_error, "parse_metric_check: invalid protocol %x",
			   proto);
	    return (1);
    }
    return (parse_limit_check(string, metric, limit_low, limit_high));
}


/*
 *	Set metric for each element in list that does not have one
 */
adv_entry *
parse_adv_propagate(list, proto, metric, advlist)
adv_entry *list;
proto_t proto;
metric_t metric;
adv_entry *advlist;
{
    adv_entry *adv;

    for (adv = list; adv; adv = adv->adv_next) {
	adv->adv_proto = proto;
	adv->adv_list = advlist;
	if (metric >= 0) {
	    adv->adv_flag |= ADVF_OTMETRIC;
	    adv->adv_metric = metric;
	}
    }
    return (list);
}


/*
 *	Set preference for each elmit in list that does not have one
 */
void
parse_adv_preference(list, proto, preference)
adv_entry *list;
proto_t proto;
pref_t preference;
{
    for (; list; list = list->adv_next) {
	if ((list->adv_flag & ADVF_OTYPE) == ADVF_OTNONE) {
	    list->adv_proto = proto;
	    list->adv_flag |= ADVF_OTPREFERENCE;
	    list->adv_preference = preference;
	}
    }
}


/*
 *	Return a pointer to a duplicate of this adv_entry
 */
adv_entry *
parse_adv_dup(old)
adv_entry *old;
{
    adv_entry *new = NULL, *adv, *root;

    root = NULL;

    for (; old; old = old->adv_next) {
	adv = adv_alloc(old->adv_flag, old->adv_proto);
	memcpy((caddr_t) adv, (caddr_t) old, sizeof(*adv));
	if (!root) {
	    root = adv;
	} else {
	    new->adv_next = adv;
	}
	if (adv->adv_list) {
	    adv->adv_list->adv_refcount++;
	}
	new = adv;
    }
    return (root);
}


/*
 *	If the pointed to table of adv lists for interfaces does not
 *	exist, create it.
 */
adv_entry **
parse_adv_interface(list)
adv_entry ***list;
{
    if (!*list) {
	*list = (adv_entry **) calloc((u_int) (int_index_max + 1), sizeof(adv_entry));
	if (!*list) {
	    trace(TR_ALL, LOG_ERR, "parse_adv_interface: calloc %m");
	    quit(errno);
	}
    }
    return (*list);
}


/*
 *	Lookup the entry in the list for the exterior protocol and append this list to it.
 */
int
parse_adv_ext(advlist, adv)
adv_entry **advlist;
adv_entry *adv;
{
    adv_entry *list;

    ADV_LIST(*advlist, list) {
	if (adv->adv_as == list->adv_as) {
	    break;
	}
    } ADV_LISTEND;

    if (!list) {
	list = adv_alloc(ADVF_TAS, adv->adv_proto);
	list->adv_as = adv->adv_as;
	*advlist = parse_adv_append(*advlist, list, TRUE);
	if (!*advlist) {
	    return (0);
	}
    }
    list->adv_list = parse_adv_append(list->adv_list, adv, TRUE);
    if (!list->adv_list) {
	return (0);
    }
    return (1);
}
