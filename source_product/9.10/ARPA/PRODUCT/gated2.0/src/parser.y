%{
/*
 *  $Header: parser.y,v 1.1.109.5 92/02/28 15:59:32 ash Exp $
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
#include	<sys/stat.h>
#include	"parse.h"
#include	"rip.h"
#include	"hello.h"
#include	"egp.h"
#include	"bgp.h"
#include	"snmp.h"

char parse_error[BUFSIZ];
char *parse_filename;

static	int	parse_group_index;
static	proto_t	parse_proto;
static	proto_t	parse_prop_proto;
static	gw_entry	**parse_gwlist;
static	gw_entry	*parse_gwp;
static  char *parse_serv_proto;
#ifdef	PROTO_EGP
static	struct egpngh *ngp, egp_group, *gr_ngp;
#endif	/* PROTO_EGP */
#ifdef	PROTO_BGP
static	bgpPeer *bnp;
#endif	/* PROTO_BGP */

char *parse_directory;
int parse_state;
proto_t protos_seen;
metric_t parse_metric;
flag_t	parse_flags;
pref_t	parse_preference;
sockaddr_un parse_addr;

static void yyerror();			/* Log a parsing error */

#define	PARSE_ERROR	yyerror(parse_error);	YYERROR;
#define	PROTO_SEEN	if (protos_seen & parse_proto) {\
    sprintf(parse_error, "parse_proto_seen: duplicate %s clause",\
		gd_lower(trace_bits(rt_proto_bits, parse_proto)));\
	PARSE_ERROR; } else protos_seen |= parse_proto

%}
/* Global */
%union {
	int	num;
	char	*ptr;
	flag_t	flag;
	time_t	time;
	as_t	as;
	proto_t	proto;
	metric_t metric;
	pref_t	pref;
	if_entry	*ifp;
	adv_entry	*adv;
	gw_entry	*gwp;
	sockaddr_un	sockaddr;
	dest_mask	dm;
}

%token		EOS
%token		UNKNOWN
%token	<num>	NUMBER
%token	<ptr>	STRING HNAME
%token	<num>	T_DIRECT T_INTERFACE T_PROTO T_METRIC T_LEX T_PARSE T_CONFIG T_DEFAULT
%token	<num>	T_YYDEBUG T_YYSTATE T_YYQUIT
%token	<num>	T_INCLUDE T_DIRECTORY
%token	<num>	T_ON T_OFF T_QUIET T_POINTOPOINT T_SUPPLIER T_GATEWAY T_PREFERENCE T_DEFAULTMETRIC
/* BGP and EGP */
%token	<num>	T_ASIN T_ASOUT T_NEIGHBOR
%token	<metric>	T_METRICOUT
%token	<num>	T_GENDEFAULT T_NOGENDEFAULT
%token	<num>	T_DEFAULTIN T_DEFAULTOUT
/* EGP */
%token	<num>	T_EGP T_GROUP
%token	<num>	T_VERSION T_MAXUP T_SOURCENET T_P1 T_P2 T_PKTSIZE
/* BGP */
%token	<num>	T_BGP T_HOLDTIME
%token	<num>	T_LINKTYPE T_INTERNAL T_HORIZONTAL
/* RIP and HELLO */
%token	<num>	T_TRUSTEDGATEWAYS T_SOURCEGATEWAYS
/* RIP */
%token	<num>	T_RIP T_NORIPOUT T_NORIPIN
/* HELLO */
%token	<num>	T_HELLO T_NOHELLOOUT T_NOHELLOIN
/* Other protocols */
%token	<num>	T_REDIRECT T_NOICMPIN T_ICMP
%token	<num>	T_SNMP
%token	<port>	T_PORT
%token	<ptr>	T_PASSWD
/* Interface */
%token	<num>	T_PASSIVE
/* Control */
%token	<num>	T_STATIC T_ANNOUNCE T_NOANNOUNCE T_LISTEN T_NOLISTEN T_MARTIANS
%token	<num>	T_PROPAGATE T_ACCEPT
%token	<num>	T_RESTRICT T_NORESTRICT
%token	<num>	T_MASK
/* AS control */
%token	<num>	T_AS
/* Tracing */
%token		T_OPTIONS T_NOINSTALL
%token	<num>	T_TRACEOPTIONS T_TRACEFILE T_REPLACE
%token	<num>	T_ALL T_NONE T_GENERAL T_EXTERNAL T_ROUTE T_UPDATE T_KERNEL
%token	<num>	T_TASK T_TIMER T_NOSTAMP T_MARK

%type	<num>	octet
%type	<metric>	metric metric_option
%type	<time>	time
%type	<num>	bgp_linktype
%type	<num>	onoff_option interior_option
%type	<flag>	trace_option trace_options trace_options_none
%type	<num>	trace_replace
%type	<proto>	proto_interior proto_exterior
%type	<proto>	accept_interior
%type	<proto> prop_interior
%type	<as>	autonomous_system
%type	<pref>	preference preference_option
%type	<num>	pref_option
%type	<ifp>	interface
%type	<sockaddr>	ip_addr dest host network interface_addr
%type	<gwp>	gateway
%type	<dm>	dest_mask
%type	<adv>	interface_list interface_list_all gateway_list
%type	<adv>	dest_mask_list
%type	<adv>	accept_list accept_listen
%type	<adv>	prop_source prop_source_list
%type	<adv>	prop_restrict prop_restrict_list prop_restrict_option
%type	<ptr>	string host_name
%type	<num>	port

%%

config		: /* Empty */
		| statements
		;

statements	: statement
		| statements statement
		;

statement	: parse_statement
		| trace_statement
		| define_order define_statement
		| proto_order proto_statement
		| route_order route_statement
		| control_order control_statement
		| error EOS
			{
				yyerrok;
			}
		| EOS
		;

/*  */

parse_statement	: T_YYDEBUG onoff_option EOS
			{
#if	YYDEBUG != 0
				if ($2 == T_OFF) {
					yydebug = 0;
				} else {
					yydebug = 1;
				}
				trace(TR_CONFIG, 0, "parse: %s yydebug %s ;",
					parse_where(),
					yydebug ? "on" : "off");
#endif	/* YYDEBUG */
			}
		| T_YYSTATE NUMBER EOS
			{
#if	YYDEBUG != 0
				if ($2 < 0 || $2 > PS_MAX) {
					(void) sprintf(parse_error, "invalid yystate value: %d",
						$2);
					PARSE_ERROR;
				}
				parse_state = $2;
				trace(TR_CONFIG, 0, "parse: %s yystate %d ;",
					parse_where(),
					parse_state);
#endif	/* YYDEBUG */
			}
		| T_YYQUIT EOS
			{
#if	YYDEBUG != 0
				trace(TR_CONFIG, 0, "parse: %s yyquit ;",
					parse_where());
				quit(0);
#endif	/* YYDEBUG */
			}
		| '%' T_INCLUDE string EOS
			{
			    char *name = $3;

			    switch (*name) {
			    case '/':
				break;

			    default:
				name = (char *) malloc(strlen(name) + strlen(parse_directory) + 2);

				strcpy(name, parse_directory);
				strcat(name, "/");
				strcat(name, $3);
			    }
				
			    if (parse_include(name)) {
				PARSE_ERROR;
			    }
			}
		| '%' T_DIRECTORY string EOS
			{
#ifndef	vax11c
			    struct stat stbuf;

			    if (stat($3, &stbuf) < 0) {
				sprintf(parse_error, "stat(%s): %m",
					$3);
				PARSE_ERROR;
			    }

			    switch (stbuf.st_mode & S_IFMT) {
			    case S_IFDIR:
				break;

			    default:
				sprintf(parse_error, "%s is not a directory",
					$3);
				PARSE_ERROR;
			    }
#endif	/* vax11c */
			    if (parse_directory) {
				free(parse_directory);
			    }
			    if ($3[strlen($3)-1] == '/') {
				$3[strlen($3)-1] = (char) 0;
			    }
			    parse_directory = $3;
			    trace(TR_PARSE, 0, "parse: %s included file prefeix now %s",
				  parse_where(),
				  parse_directory);
			}
		;

/*  */

trace_statement	: T_TRACEOPTIONS trace_options_none EOS
			{
			    if ($2) {
				if (trace_flags) {
				    trace_flags = trace_flags_save = $2;
				    trace_display(trace_flags);
				} else {
				    trace_flags_save = $2;
				    if (trace_file) {
					trace_on(trace_file, TRUE);
				    }
				}
			    } else {
				if (trace_flags) {
				    trace_display(0);
				    trace_off();
				}
				trace_flags = trace_flags_save = $2;
			    }
			}
		| T_TRACEFILE string trace_replace EOS
			{
			    if (trace_flags) {
				trace_flags_save = trace_flags;
				trace_off();
			    }
			    trace_on(trace_file = $2, $3);
			}
		;

trace_replace	: /* Empty */	{ $$ = TRUE; }
		| T_REPLACE	{ $$ = FALSE; }
		;

trace_options_none : trace_options		{ $$ = $1; }
		| T_NONE			{ $$ = (flag_t) 0; }
                ;

trace_options	: trace_option			{ $$ = $1; }
		| trace_options trace_option	{ $$ = $1 | $2; }
		;
		
trace_option	: T_ALL		{ $$ = TR_ALL; }
		| T_GENERAL	{ $$ = TR_INT|TR_EXT|TR_RT; }
		| T_INTERNAL	{ $$ = TR_INT; }
		| T_EXTERNAL	{ $$ = TR_EXT; }
		| T_ROUTE	{ $$ = TR_RT; }
		| T_EGP		{ $$ = TR_EGP; }
		| T_UPDATE	{ $$ = TR_UPDATE; }
		| T_RIP		{ $$ = TR_RIP; }
		| T_HELLO	{ $$ = TR_HELLO; }
		| T_ICMP	{ $$ = TR_ICMP; }
		| T_TASK	{ $$ = TR_TASK; }
		| T_TIMER	{ $$ = TR_TIMER; }
		| T_NOSTAMP	{ $$ = TR_NOSTAMP; }
		| T_MARK	{ $$ = TR_MARK; }
		| T_PROTO	{ $$ = TR_PROTOCOL; }
		| T_KERNEL	{ $$ = TR_KRT; }
		| T_BGP		{ $$ = TR_BGP; }
		| T_SNMP	{ $$ = TR_SNMP; }
		| T_LEX		{ $$ = TR_LEX; }
		| T_PARSE	{ $$ = TR_PARSE; }
		| T_CONFIG	{ $$ = TR_CONFIG; }
		;

/*  */

define_order	: /*Empty */
			{
				if (parse_new_state(PS_DEFINE)) {
					PARSE_ERROR;
				}
			}
		;

define_statement
		: interface_statement
		| as_statement
		| T_OPTIONS option_list EOS
		| T_MARTIANS '{' dest_mask_list '}' EOS
			{
				martian_list = parse_adv_append(martian_list, $3, TRUE);
				if (!martian_list) {
					PARSE_ERROR;
				}
				parse_adv_list("martians", (char *)0, (adv_entry *)0, $3);
			}
		;


option_list	: option
		| option_list option
		;


option		: T_NOINSTALL
			{
				install = FALSE;
			}
		| T_GENDEFAULT
			{
			    rt_default_needed = TRUE;
			}
		;


/*  */

interface_statement
		: T_INTERFACE interface_init interface_list_all interface_option_list EOS
			{
				parse_interface($3);
			}
		;

interface_init	: /* Initialize variables used */
			{
				parse_metric = -1;
				parse_preference = 0;
				parse_flags = 0;
			}
		;

interface_option_list
		: interface_option
		| interface_option_list interface_option
		;

interface_option
		: T_METRIC metric
			{
				if (parse_metric_check(RTPROTO_DIRECT, $2)) {
					PARSE_ERROR;
				}
				parse_metric = $2;
				parse_flags |= IFS_METRICSET;
			}
		| T_PREFERENCE preference
			{
				parse_preference = $2;
			}
		| T_PASSIVE
			{
				parse_flags |= IFS_NOAGE;
			}
		;

interface_list_all
		: T_ALL
			{
				/* Return a null pointer to indicate all interfaces */
				$$ = (adv_entry *) 0;
			}
		| interface_list
			{
				$$ = $1;
			}
		;

interface_list
		: interface
			{
				$$ = adv_alloc(ADVF_TINTF | ADVF_FIRST, (proto_t) 0);
				$$->adv_ifp = $1;
			}
		| interface_list interface
			{
				$$ = adv_alloc(ADVF_TINTF, (proto_t) 0);
				$$->adv_ifp = $2;
				$$ = parse_adv_append($1, $$, TRUE);
				if (!$$) {
					PARSE_ERROR;
				}
			}
		;

interface	: interface_addr
			{
				$$ = if_withaddr(&$1);
				if (!$$) {
					(void) sprintf(parse_error, "Invalid interface at '%A'",
						&$1);
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s INTERFACE: %A (%s)",
					parse_where(),
					&$$->int_addr,
					$$->int_name);
			}
		;

interface_addr	: ip_addr
			{
				$$ = $1;
			}
		| host_name
			{ 
				if(parse_addr_hname(&$$, $1, TRUE, FALSE)) {
					if_entry *ifp;

					ifp = if_withname($1);
					if (!ifp) {
						(void) sprintf(parse_error, "unknown interface name at %s",
							$1);
						PARSE_ERROR;
					}
					if (ifp->int_state & IFS_POINTOPOINT) {
					    $$ = ifp->int_dstaddr;
					} else {
					    $$ = ifp->int_addr;
					}
				}
			}
		;


/*  */
as_statement	: T_AS autonomous_system EOS
			{
				trace(TR_CONFIG, 0, "parse: %s autonomoussystem %d ;",
					parse_where(),
					$2);
				my_system = $2;
			}
		;

autonomous_system
		: NUMBER
			{
				if (parse_limit_check("autonomous system", $1, LIMIT_AS_LOW, LIMIT_AS_HIGH)) {
					PARSE_ERROR;
				}
				$$ = $1;
			}
		;

/*  */

proto_order	: /* Empty */
			{
				if (parse_new_state(PS_PROTO)) {
					PARSE_ERROR;
				}
			}
		;

proto_statement	: rip_statement
		| hello_statement
		| egp_statement
		| bgp_statement
		| redirect_statement
		| snmp_statement
		;

/*  */

rip_statement	: T_RIP rip_init interior_option rip_group EOS
			{
#ifdef	PROTO_RIP
 			        PROTO_SEEN;

				doing_rip = TRUE;
				rip_pointopoint = FALSE;
				rip_supplier = -1;
				switch ($3) {
					case T_OFF:
						doing_rip = FALSE;
						rip_supplier = FALSE;
						break;
					case T_ON:
						break;
					case T_QUIET:
						rip_supplier = FALSE;
						break;
					case T_SUPPLIER:
						rip_supplier = TRUE;
						break;
					case T_POINTOPOINT:
						rip_pointopoint = TRUE;
						rip_supplier = TRUE;
						break;
				}
#endif	/* PROTO_RIP */
			}
		;

rip_init	:
			{
#ifdef	PROTO_RIP
				parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;

				rip_default_metric = RIPHOPCNT_INFINITY;
				rip_preference = RTPREF_RIP;

#endif	/* PROTO_RIP */
			}
		;

rip_group	: /* Empty */
		| '{' rip_group_stmts '}'
		;

rip_group_stmts	: /* Empty */
		| rip_group_stmts rip_group_stmt EOS
		| rip_group_stmts error EOS
			{
				yyerrok;
			}
		;

rip_group_stmt	: T_PREFERENCE preference
			{
#ifdef	PROTO_RIP
				rip_preference = $2;
#endif	/* PROTO_RIP */
			}
		| T_DEFAULTMETRIC metric
			{
#ifdef	PROTO_RIP
				if (parse_metric_check(RTPROTO_RIP, $2)) {
					PARSE_ERROR;
				}
				rip_default_metric = $2;
#endif	/* PROTO_RIP */
			}
		| T_INTERFACE interface_init interface_list_all rip_interface_options
			{
#ifdef	PROTO_RIP
				parse_interface($3);
#endif	/* PROTO_RIP */
			}
		| T_TRUSTEDGATEWAYS gateway_list
			{
#ifdef	PROTO_RIP
				rip_n_trusted += parse_gw_flag($2, RTPROTO_RIP, GWF_TRUSTED);
				if (!rip_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* PROTO_RIP */
			}
		| T_SOURCEGATEWAYS gateway_list
			{
#ifdef	PROTO_RIP
				rip_n_source += parse_gw_flag($2, RTPROTO_RIP, GWF_SOURCE);
				if (!rip_n_source) {
					PARSE_ERROR;
				}
#endif	/* PROTO_RIP */
			}
		;

rip_interface_options
		: rip_interface_option
		| rip_interface_options rip_interface_option
		;

rip_interface_option
		: T_NORIPIN
			{
#ifdef	PROTO_RIP
				parse_flags |= IFS_NORIPIN;
#endif	/* PROTO_RIP */
			}
		| T_NORIPOUT
			{
#ifdef	PROTO_RIP
				parse_flags |= IFS_NORIPOUT;
#endif	/* PROTO_RIP */
			}
		;

/*  */

hello_statement	: T_HELLO hello_init interior_option hello_group EOS
			{
#ifdef	PROTO_HELLO
 			        PROTO_SEEN;

				doing_hello = TRUE;
				hello_pointopoint = FALSE;
				hello_supplier = -1;
				switch ($3) {
					case T_OFF:
						doing_hello = FALSE;
						hello_supplier = FALSE;
						break;
					case T_ON:
						break;
					case T_QUIET:
						hello_supplier = FALSE;
						break;
					case T_SUPPLIER:
						hello_supplier = TRUE;
						break;
					case T_POINTOPOINT:
						hello_pointopoint = TRUE;
						hello_supplier = TRUE;
						break;
				}
#endif	/* PROTO_HELLO */
			}
		;

hello_init	:
			{
#ifdef	PROTO_HELLO
				parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				
				hello_default_metric = DELAY_INFINITY;
				hello_preference = RTPREF_HELLO;
#endif	/* PROTO_HELLO */
			}
		;

hello_group	: /* Empty */
		| '{' hello_group_stmts '}'
		;

hello_group_stmts
		: /* Empty */
		| hello_group_stmts hello_group_stmt EOS
		| hello_group_stmts error EOS
			{
				yyerrok;
			}
		;

hello_group_stmt
		: T_PREFERENCE preference
			{
#ifdef	PROTO_HELLO
				hello_preference = $2;
#endif	/* PROTO_HELLO */
			}
		| T_DEFAULTMETRIC metric
			{
#ifdef	PROTO_HELLO
				if (parse_metric_check(RTPROTO_HELLO, $2)) {
					PARSE_ERROR;
				}
				hello_default_metric = $2;
#endif	/* PROTO_HELLO */
			}
		| T_INTERFACE interface_init interface_list_all hello_interface_options
			{
#ifdef	PROTO_HELLO
				parse_interface($3);
#endif	/* PROTO_HELLO */
			}
		| T_TRUSTEDGATEWAYS gateway_list
			{
#ifdef	PROTO_HELLO
				hello_n_trusted += parse_gw_flag($2, RTPROTO_HELLO, GWF_TRUSTED);
				if (!hello_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* PROTO_HELLO */
			}
		| T_SOURCEGATEWAYS gateway_list
			{
#ifdef	PROTO_HELLO
				hello_n_source += parse_gw_flag($2, RTPROTO_HELLO, GWF_SOURCE);
				if (!hello_n_source) {
					PARSE_ERROR;
				}
#endif	/* PROTO_HELLO */
			}
		;

hello_interface_options
		: hello_interface_option
		| hello_interface_options hello_interface_option
		;

hello_interface_option
		: T_NOHELLOIN
			{
#ifdef	PROTO_HELLO
				parse_flags |= IFS_NOHELLOIN;
#endif	/* PROTO_HELLO */
			}
		| T_NOHELLOOUT
			{
#ifdef	PROTO_HELLO
				parse_flags |= IFS_NOHELLOOUT;
#endif	/* PROTO_HELLO */
			}
		;

/*  */

egp_statement	: T_EGP egp_init onoff_option egp_group EOS
			{
#ifdef	PROTO_EGP
 			        PROTO_SEEN;

				if ($3 == T_OFF) {
					doing_egp = FALSE;
					trace(TR_CONFIG, 0, "parse: %s egp off ;",
					      parse_where());
				} else {
					doing_egp = TRUE;
					if (!my_system) {
						(void) sprintf(parse_error, "parse: %s autonomous-system not specified",
							parse_where());
						PARSE_ERROR;
					}
					if (!egp_neighbors) {
						(void) sprintf(parse_error, "parse: %s no EGP neighbors specified",
							parse_where());
						PARSE_ERROR;
					}

#if	defined(AGENT_SNMP)
					egp_sort_neighbors();
#endif	/* defined */(AGENT_SNMP)

					if (trace_flags & TR_CONFIG) {
					    trace(TR_CONFIG, 0, "parse: %s egp on {",
						  parse_where());
					    trace(TR_CONFIG, 0, "parse: %s   preference %d ;",
						  parse_where(),
						  egp_preference);
					    trace(TR_CONFIG, 0, "parse: %s   defaultmetric %d ;",
						  parse_where(),
						    egp_default_metric);

					    gr_ngp = (struct egpngh *) 0;
					    EGP_LIST(ngp) {
						if (gr_ngp != ngp->ng_gr_head) {
						    if (gr_ngp) {
							trace(TR_CONFIG, 0, "parse: %s   } ;",
							      parse_where());
						    }
						    gr_ngp = ngp->ng_gr_head;
						    tracef("parse: %s   group",
							   parse_where());
						    if (ngp->ng_options & NGO_VERSION) {
							tracef(" version %d", ngp->ng_version);
						    }
						    if (ngp->ng_options & NGO_MAXACQUIRE) {
							tracef(" maxup %d", ngp->ng_gr_acquire);
						    }
						    if (ngp->ng_options & NGO_ASIN) {
							tracef(" asin %d", ngp->ng_asin);
						    }
						    if (ngp->ng_options & NGO_ASOUT) {
							tracef(" asout %d", ngp->ng_asout);
						    }
						    if (ngp->ng_options & NGO_PREFERENCE) {
							tracef(" preference %d", ngp->ng_preference);
						    }
						    trace(TR_CONFIG, 0, " {");
						}
						tracef("parse: %s     neighbor %s",
						       parse_where(),
						       ngp->ng_name);
						if (ngp->ng_options & NGO_INTERFACE) {
						    tracef(" intf %A",
							   &ngp->ng_interface->int_addr);
						}
						if (ngp->ng_options & NGO_SADDR) {
						    tracef(" sourcenet %A",
							   &ngp->ng_saddr);
						}
						if (ngp->ng_options & NGO_GATEWAY) {
						    tracef(" gateway %A",
							   &ngp->ng_gateway);
						}
						if (ngp->ng_options & NGO_METRICOUT) {
						    tracef(" egpmetricout %d", ngp->ng_metricout);
						}
						if (ngp->ng_options & NGO_NOGENDEFAULT) {
						    tracef(" nogendefault");
						}
						if (ngp->ng_options & NGO_DEFAULTIN) {
						    tracef(" acceptdefault");
						}
						if (ngp->ng_options & NGO_DEFAULTOUT) {
						    tracef(" propagatedefault");
						}
						if (ngp->ng_options & NGO_P1) {
						    tracef(" p1 %#T",
							   ngp->ng_P1);
						}
						if (ngp->ng_options & NGO_P2) {
						    tracef(" p2 %#T",
							   ngp->ng_P2);
						}
						trace(TR_CONFIG, 0, " ;");
					    } EGP_LISTEND ;
 					    trace(TR_CONFIG, 0, "parse: %s   } ;",
						  parse_where());
 					    trace(TR_CONFIG, 0, "parse: %s } ;",
						  parse_where());
					}
				}
#endif	/* PROTO_EGP */
			}
		;

egp_init	:
			{
			    parse_proto = RTPROTO_EGP;

			    egp_default_metric = EGP_INFINITY;
			    egp_preference = RTPREF_EGP;
			    egp_pktsize = EGPMAXPACKETSIZE;
			}
		;

egp_group	: /* Empty */
		| '{' egp_group_stmts '}'
		;

egp_group_stmts	: /* Empty */
		| egp_group_stmts egp_group_stmt EOS
		| egp_group_stmts error EOS
			{
				yyerrok;
			}
		;

egp_group_stmt	: T_PREFERENCE preference
			{
#ifdef	PROTO_EGP
				egp_preference = $2;
#endif	/* PROTO_EGP */
			}
		| T_DEFAULTMETRIC metric
			{
#ifdef	PROTO_EGP
				if (parse_metric_check(RTPROTO_EGP, $2)) {
					PARSE_ERROR;
				}
				egp_default_metric = $2;
#endif	/* PROTO_EGP */
			}
		| T_PKTSIZE NUMBER
			{
#ifdef	PROTO_EGP
			    if (parse_limit_check("packetsize", $2, EGP_LIMIT_PKTSIZE)) {
				PARSE_ERROR;
			    }
			    egp_pktsize = $2;
#endif	/* PROTO_EGP */
			}
		| T_GROUP egp_group_init egp_group_options '{' egp_peer_stmts '}'
			{
#ifdef	PROTO_EGP
				if (gr_ngp->ng_gr_acquire > gr_ngp->ng_gr_number) {
					(void) sprintf(parse_error,
						       "maxacquire %u is greater than number of neighbors %u in group %d",
						       gr_ngp->ng_gr_acquire,
						       gr_ngp->ng_gr_number,
						       parse_group_index);
				} else if (!gr_ngp->ng_gr_acquire) {
					gr_ngp->ng_gr_acquire = gr_ngp->ng_gr_number;
				}

#endif	/* PROTO_EGP */
			}
		;

egp_group_init	: /* Initialize at start of group */
			{
#ifdef	PROTO_EGP
				/* Clear group structure and set fill pointer */
				memset((caddr_t) &egp_group, (char) 0, sizeof(egp_group));
				ngp = &egp_group;
				sockclear_in(&ngp->ng_addr);
				sockclear_in(&ngp->ng_gateway);
				sockclear_in(&ngp->ng_paddr);
				sockclear_in(&ngp->ng_saddr);
				/* First neighbor in group is head of group */
				gr_ngp = (struct egpngh *) 0;
				parse_group_index++;
#endif	/* PROTO_EGP */
			}
		;

egp_peer_stmts	: /* Empty */
		| egp_peer_stmts egp_peer_stmt EOS
		| egp_peer_stmts error EOS
			{
				yyerrok;
			}
		;

egp_peer_stmt	: T_NEIGHBOR egp_peer_init host egp_peer_options
                        {
#ifdef	PROTO_EGP
			    struct egpngh *ngp2, *last = (struct egpngh *) 0;

			    /* Set neighbor's address */
			    ngp->ng_addr = $3.in;	/* struct copy */
			    ngp->ng_gw.gw_proto = RTPROTO_EGP;
			    strcpy(ngp->ng_name, inet_ntoa(ngp->ng_addr.sin_addr));

			    /* Set group pointer and count this neighbor */
			    ngp->ng_gr_head = gr_ngp;
			    ngp->ng_gr_index = parse_group_index;
			    gr_ngp->ng_gr_number++;

			    if (!egp_neighbor_head) {
				egp_neighbor_head = ngp;	/* first neighbor */
				egp_neighbors++;
			    } else {
				EGP_LIST(ngp2) {
				    if (equal(&ngp->ng_addr, &ngp2->ng_addr)) {
					if (ngp2->ng_flags & NGF_DELETE) {
					    if (!egp_neighbor_changed(ngp2, ngp)) {
						ngp2->ng_flags &= ~NGF_DELETE;
						(void) free((caddr_t) ngp);
						break;
					    } else {
						ngp->ng_flags = NGF_WAIT;
					    }
					} else {
					    (void) sprintf(parse_error, "duplicate EGP neighbor at %A",
						    &ngp->ng_addr);
					    PARSE_ERROR;
					}
				    }
				    if (!ngp2->ng_next) {
					last = ngp2;
				    }
				} EGP_LISTEND ;
			    }
			    
			    /* Add this neighbor to end of the list */
			    if (last) {
				last->ng_next = ngp;
				egp_neighbors++;
			    }
#endif	/* PROTO_EGP */
			}
		;

egp_peer_init	: /* Allocate an EGP neighbor structure */
			{
#ifdef	PROTO_EGP
				ngp = (struct egpngh *) calloc(1, sizeof(struct egpngh));
				if (!ngp) {
					trace(TR_ALL, LOG_ERR, "parse: %s calloc: %m",
						parse_where()),
					quit(errno);
				}
				/* Initialize neighbor structure with group structure */
				memcpy((caddr_t) ngp, (caddr_t) &egp_group, sizeof(*ngp));
				/* This neighbor is head of the group */
				if (!gr_ngp) {
					gr_ngp = ngp;
				}
				parse_gwlist = &parse_gwp;
#endif	/* PROTO_EGP */
			}
		;

egp_group_options
		: /* Empty */
		| egp_group_options egp_group_option
		| egp_group_options egp_peer_option
		;

egp_group_option
		: T_ASIN autonomous_system
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_ASIN;
				ngp->ng_asin = $2;
#endif	/* PROTO_EGP */
			}
		| T_ASOUT autonomous_system
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_ASOUT;
				ngp->ng_asout = $2;
#endif	/* PROTO_EGP */
			}
		| T_MAXUP NUMBER
			{
#ifdef	PROTO_EGP
				/* XXX - Limit check maxup value */
				ngp->ng_options |= NGO_MAXACQUIRE;
				ngp->ng_gr_acquire = $2;
#endif	/* PROTO_EGP */
			}
		| T_VERSION NUMBER
			{
#ifdef	PROTO_EGP
				if ( !(EGPVMASK & (1 << ($2 - 2))) ) {
					(void) sprintf(parse_error, "unsupported EGP version: %d",
						$2);
					PARSE_ERROR;
				}
				ngp->ng_options |= NGO_VERSION;
				ngp->ng_version = $2;
#endif	/* PROTO_EGP */
			}
		| T_PREFERENCE preference
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_PREFERENCE;
				ngp->ng_preference = $2;
#endif	/* PROTO_EGP */
			}
		;

egp_peer_options
		: /* Empty */
		| egp_peer_options egp_peer_option
		;

egp_peer_option	: T_METRICOUT metric
			{
#ifdef	PROTO_EGP
				if (parse_metric_check(RTPROTO_EGP, $2)) {
					PARSE_ERROR;
				}
				ngp->ng_options |= NGO_METRICOUT;
				ngp->ng_metricout = $2;
#endif	/* PROTO_EGP */
			}
		| T_NOGENDEFAULT
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_NOGENDEFAULT;
#endif	/* PROTO_EGP */
			}
		| T_DEFAULTIN
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_DEFAULTIN;
#endif	/* PROTO_EGP */
			}
		| T_DEFAULTOUT
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_DEFAULTOUT;
#endif	/* PROTO_EGP */
			}
		| T_GATEWAY gateway
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_GATEWAY;
				sockcopy(&$2->gw_addr, &ngp->ng_gateway);
				(void) free((caddr_t)$2);
				parse_gwlist = (gw_entry **) 0;
				parse_gwp = (gw_entry *) 0;
#endif	/* PROTO_EGP */
			}
		| T_INTERFACE interface
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_INTERFACE;
				ngp->ng_interface = $2;
#endif	/* PROTO_EGP */
			}
		| T_SOURCENET network
			{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_SADDR;
				sockcopy(&$2, &ngp->ng_saddr);
#endif	/* PROTO_EGP */
			}
		| T_P1 time
			{
#ifdef	PROTO_EGP
			    if (parse_limit_check("P1", $2, EGP_P1, MAXHELLOINT)) {
				PARSE_ERROR;
			    }
			    ngp->ng_options |= NGO_P1;
			    ngp->ng_P1 = $2;
#endif	/* PROTO_EGP */
			}
		| T_P2 time
			{
#ifdef	PROTO_EGP
			    if (parse_limit_check("P2", $2, EGP_P2, MAXPOLLINT)) {
				PARSE_ERROR;
			    }
			    ngp->ng_options |= NGO_P2;
			    ngp->ng_P2 = $2;
#endif	/* PROTO_EGP */
			}
		;

/*  */

bgp_statement	: T_BGP bgp_init onoff_option bgp_group EOS
			{
#ifdef	PROTO_BGP
 			        PROTO_SEEN;

				if ($3 == T_OFF) {
					doing_bgp = FALSE;
					trace(TR_CONFIG, 0, "parse: %s bgp off ;",
					      parse_where());
				} else {
					doing_bgp = TRUE;
					if (!my_system) {
						(void) sprintf(parse_error, "parse: %s autonomous-system not specified",
							parse_where());
						PARSE_ERROR;
					}
					if (!bgp_n_peers) {
						(void) sprintf(parse_error, "parse: %s no BGP peers specified",
							parse_where());
						PARSE_ERROR;
					}
					if (trace_flags & TR_CONFIG) {
					    trace(TR_CONFIG, 0, "parse: %s bgp on {",
						  parse_where());
					    trace(TR_CONFIG, 0, "parse: %s   preference %d ;",
						  parse_where(),
						  bgp_preference);
					    trace(TR_CONFIG, 0, "parse: %s   defaultmetric %d ;",
						  parse_where(),
						    bgp_default_metric);
					    BGP_LIST(bnp) {
					        tracef("parse: %s     neighbor %s",
						       parse_where(),
						       bnp->bgp_name);
						if (bnp->bgp_options & BGPO_HOLDTIME) {
						    tracef(" holdtime %#T",
							   bnp->bgp_holdtime_out);
						}
						if (bnp->bgp_options & BGPO_LINKTYPE) {
						    tracef(" linktype %s",
							   trace_state(bgpOpenType, bnp->bgp_linktype));
						}
						if (bnp->bgp_options & BGPO_GATEWAY) {
						    tracef(" gateway %A",
							   &bnp->bgp_gateway);
						}
						if (bnp->bgp_options & BGPO_INTERFACE) {
						    tracef(" intf %A",
							   &bnp->bgp_interface->int_addr);
						}
						if (bnp->bgp_options & BGPO_METRICOUT) {
						    tracef(" metricout %d",
							   bnp->bgp_metricout);
						}
						if (bnp->bgp_options & BGPO_ASIN) {
						    tracef(" asin %d",
							   bnp->bgp_asin);
						}
						if (bnp->bgp_options & BGPO_ASOUT) {
						    tracef(" asout %d",
							   bnp->bgp_asout);
						}
						if (bnp->bgp_options & BGPO_NOGENDEFAULT) {
					            tracef(" nogendefault");
						}
						if (bnp->bgp_options & BGPO_PREFERENCE) {
						    tracef(" preference %d",
							   bnp->bgp_preference);
						}
						trace(TR_CONFIG, 0, " ;");
					    } BGP_LISTEND ;
					    trace(TR_CONFIG, 0, "parse: %s } ;",
						  parse_where());
					}
				    }
#endif	/* PROTO_BGP */
			}
		;

bgp_init	:	{ parse_proto = RTPROTO_BGP; }
		;

bgp_group	: /* Empty */
		| '{' bgp_group_stmts '}'
		;

bgp_group_stmts	: /* Empty */
		| bgp_group_stmts bgp_group_stmt EOS
		| bgp_group_stmts error EOS
			{
				yyerrok;
			}
		;

bgp_group_stmt	: T_PREFERENCE preference
			{
#ifdef	PROTO_BGP
				bgp_preference = $2;
#endif	/* PROTO_BGP */
			}
		| T_DEFAULTMETRIC metric
			{
#ifdef	PROTO_BGP
				if (parse_metric_check(RTPROTO_BGP, $2)) {
					PARSE_ERROR;
				}
				bgp_default_metric = $2;
#endif	/* PROTO_BGP */
			}
		| T_NEIGHBOR bgp_peer_init host bgp_peer_options
			{
#ifdef	PROTO_BGP
			    bgpPeer *bnp2, *last = (bgpPeer *) 0;

			    /* Set peer address */
			    bnp->bgp_addr = $3.in;	/* struct copy */
			    bnp->bgp_gw.gw_proto = RTPROTO_BGP;
			    strcpy(bnp->bgp_name, inet_ntoa(bnp->bgp_addr.sin_addr));

			    /* Add to end of peer list */
			    if (bgp_peers == NULL) {
				bgp_peers = bnp;	/* first peer */
				bgp_n_peers++;
			    } else {
				BGP_LIST(bnp2) {
				    if (equal_in(bnp->bgp_addr.sin_addr, bnp2->bgp_addr.sin_addr)) {
					if (bnp2->bgp_flags & BGPF_DELETE) {
					    if (!bgp_peer_changed(bnp2, bnp)) {
						bnp2->bgp_flags &= ~BGPF_DELETE;
						(void) free((caddr_t) bnp);
						break;
					    } else {
						/* XXX - BGP doesn't have to wait, does it? */
						bnp->bgp_flags = BGPF_WAIT;
					    }
					} else {
					    (void) sprintf(parse_error, "duplicate BGP peer at %A",
						    &bnp->bgp_addr);
					    PARSE_ERROR;
					}
				    }
				    if (!bnp2->bgp_next) {
					last = bnp2;
				    }
				} BGP_LISTEND ;
			    }
			    if (last) {
				last->bgp_next = bnp;
				bgp_n_peers++;
			    }

#endif	/* PROTO_BGP */
			}
		;

bgp_peer_init	: /* Empty */
			{
#ifdef	PROTO_BGP
				bnp = (bgpPeer *) calloc(1, sizeof(bgpPeer));
				if (!bnp) {
					trace(TR_ALL, LOG_ERR, "parse: %s calloc: %m",
						parse_where());
					quit(errno);
				}
				sockclear_in(&bnp->bgp_addr);
				sockclear_in(&bnp->bgp_gateway);
				parse_gwlist = &parse_gwp;
#endif	/* PROTO_BGP */
			}
		;

bgp_peer_options
		: /* Empty */
		| bgp_peer_options bgp_peer_option
		;

bgp_peer_option	: T_METRICOUT metric
			{
#ifdef	PROTO_BGP
				if (parse_metric_check(RTPROTO_BGP, $2)) {
					PARSE_ERROR;
				}
				bnp->bgp_options |= BGPO_METRICOUT;
				bnp->bgp_metricout = $2;
#endif	/* PROTO_BGP */
			}
		| T_ASIN autonomous_system
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_ASIN;
				bnp->bgp_asin = $2;
#endif	/* PROTO_BGP */
			}
		| T_ASOUT autonomous_system
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_ASOUT;
				bnp->bgp_asout = $2;
#endif	/* PROTO_BGP */
			}
		| T_NOGENDEFAULT
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_NOGENDEFAULT;
#endif	/* PROTO_BGP */
			}
		| T_GATEWAY gateway
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_GATEWAY;
				sockcopy(&$2->gw_addr, &bnp->bgp_gateway);
				(void) free((caddr_t)$2);
				parse_gwlist = (gw_entry **) 0;
				parse_gwp = (gw_entry *) 0;
#endif	/* PROTO_BGP */
			}
		| T_PREFERENCE preference
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_PREFERENCE;
				bnp->bgp_preference = $2;
#endif	/* PROTO_BGP */
			}
		| T_LINKTYPE bgp_linktype
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_LINKTYPE;
				bnp->bgp_linktype = $2;
#endif	/* PROTO_BGP */
			}
		| T_INTERFACE interface
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_INTERFACE;
				bnp->bgp_interface = $2;
#endif	/* PROTO_BGP */
			}
		| T_HOLDTIME time
			{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_HOLDTIME;
				bnp->bgp_holdtime_out = $2;
#endif	/* PROTO_BGP */
			}
		;

bgp_linktype	: T_INTERNAL
			{
#ifdef	PROTO_BGP
				$$ = openLinkInternal;
#endif	/* PROTO_BGP */
			}
		| T_HORIZONTAL
			{
#ifdef	PROTO_BGP
				$$ = openLinkHorizontal;
#endif	/* PROTO_BGP */
			}
		;

/*  */

redirect_statement	: T_REDIRECT redirect_init onoff_option redirect_group EOS
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
 			        PROTO_SEEN;

				ignore_redirects = ($3 == T_OFF) ? TRUE : FALSE;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		;

redirect_init	:
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
			    redirect_preference = RTPREF_REDIRECT;
			    parse_proto = RTPROTO_REDIRECT;
			    parse_gwlist = &redirect_gw_list;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		;

redirect_group	: /* Empty */
		| '{' redirect_group_stmts '}'
		;

redirect_group_stmts
		: /* Empty */
		| redirect_group_stmts redirect_group_stmt EOS
		| redirect_group_stmts error EOS
			{
				yyerrok;
			}
		;

redirect_group_stmt	: T_PREFERENCE preference
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				redirect_preference = $2;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		| T_INTERFACE interface_init interface_list_all redirect_interface_options
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				parse_interface($3);
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		| T_TRUSTEDGATEWAYS gateway_list
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				redirect_n_trusted += parse_gw_flag($2, RTPROTO_REDIRECT, GWF_TRUSTED);
				if (!redirect_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}

		;

redirect_interface_options
		: redirect_interface_option
		| redirect_interface_options redirect_interface_option
		;

redirect_interface_option
		: T_NOICMPIN
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				parse_flags |= IFS_NOICMPIN;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		;
/*  */

snmp_statement	: T_SNMP onoff_option EOS
			{
#ifdef	AGENT_SNMP
			        parse_proto = RTPROTO_SNMP;
 			        PROTO_SEEN;

				doing_snmp = ($2 == T_OFF) ? FALSE : TRUE;
				trace(TR_CONFIG, 0, "parse: %s snmp %s ;",
					parse_where(),
					doing_snmp ? "on" : "off");
#endif	/* AGENT_SNMP */
			}
		;
		
/*  */

route_order	: /* Empty */
			{
				if (parse_new_state(PS_ROUTE)) {
					PARSE_ERROR;
				}
			}
		;

route_statement : static_init '{' route_stmts '}' static_finit
		;

route_stmts	: route_stmt EOS
		| route_stmts route_stmt EOS
		| route_stmts error EOS
			{
				yyerrok;
			}
		;

static_init	: T_STATIC
			{
				/* Need to set this for static routes, not used for interface routes */
				parse_proto = RTPROTO_STATIC;
				parse_gwlist = &rt_gw_list;
				rt_open(rt_task);
				trace(TR_CONFIG, 0, "parse: %s static {",
				      parse_where());
			}
		;

static_finit	: /* Empty */
			{
				(void) rt_close(rt_task, (gw_entry *) 0, 0);
				trace(TR_CONFIG, 0, "parse: %s } ;",
				      parse_where());
			}
		;

route_stmt	: dest T_GATEWAY gateway preference_option
			{
				flag_t table;
				rt_entry *rt;
				sockaddr_un gateway;

				gateway = $3->gw_addr;	/* struct copy */

				table = gd_inet_ishost(&$1) ? RTS_HOSTROUTE : RTS_INTERIOR;

				if (rt = rt_locate(table, &$1, RTPROTO_STATIC)) {
				    if (rt->rt_state & RTS_NOAGE) {
					(void) sprintf(parse_error, "duplicate static route to %A",
						&$1);
					PARSE_ERROR;
				    }
				    rt->rt_state |= RTS_NOAGE;
				    if (!rt_change(rt,
						   &gateway,
						   0,
						   (time_t) 0,
						   $4 ? $4 : RTPREF_STATIC)) {
					rt = (rt_entry *) 0;
				    }
				} else {
				    rt = rt_add(&$1,
						(sockaddr_un *) 0,
						&gateway,
						(gw_entry *) 0,
						0,
						table | RTS_NOAGE,
						RTPROTO_STATIC,
						my_system,
						(time_t) 0,
						$4 ? $4 : RTPREF_STATIC);
				}
				if (!rt) {
					(void) sprintf(parse_error, "error adding static route to %A",
						&$1);
					PARSE_ERROR;
				}
				trace(TR_CONFIG, 0, "parse: %s   %A gateway %A preference %d ;",
					parse_where(),
					&$1,
					&gateway,
					$4 ? $4 : RTPREF_STATIC);
			}
		| dest T_INTERFACE interface preference_option
			{
				flag_t table;
				rt_entry *rt;

				table = gd_inet_ishost(&$1) ? RTS_HOSTROUTE : RTS_INTERIOR;

				if (rt = rt_locate(table, &$1, RTPROTO_DIRECT)) {
				    if (rt->rt_state & RTS_NOAGE) {
					(void) sprintf(parse_error, "duplicate interface route to %A",
						&$1);
					PARSE_ERROR;
				    }
				    rt->rt_state |= RTS_NOAGE;
				    if (!rt_change(rt,
						   &$3->int_addr,
						   0,
						   (time_t) 0,
						   $4 ? $4 : RTPREF_STATIC)) {
					rt = (rt_entry *) 0;
				    }
				} else {
				    rt = rt_add(&$1,
						(sockaddr_un *) 0,
						&$3->int_addr,
						(gw_entry *) 0,
						$3->int_metric,
						table | RTS_NOAGE,
						RTPROTO_DIRECT,
						my_system,
						(time_t) 0,
						$4 ? $4 : RTPREF_STATIC);
				}
				if (!rt) {
				    (void) sprintf(parse_error, "error adding interface route to %A",
					    &$1);
				    PARSE_ERROR;
				}
				trace(TR_CONFIG, 0, "parse: %s   %A interface %A preference %d",
					parse_where(),
					&$1,
					&$3->int_addr,
					$4 ? $4 : RTPREF_STATIC);
			}
		;
	
/*  */

control_order	: /* Empty */
			{
				if (parse_new_state(PS_CONTROL)) {
					PARSE_ERROR;
				}
			}
		;
		
control_statement
		: T_ACCEPT T_PROTO proto_exterior T_AS autonomous_system preference_option '{' accept_list '}' EOS
			{
				adv_entry *adv;

				/*
				 *	Tack the list of destinations onto the end of the list
				 *	for neighbors with the specified AS.
				 */
				adv = adv_alloc(ADVF_TAS, $3);
				adv->adv_as = $5;
				adv->adv_list = $8;
				if ($6 > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = $6;
				}

				switch ($3) {
#ifdef	PROTO_BGP
					case RTPROTO_BGP:
						if (!parse_adv_ext(&bgp_accept_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_BGP */
#ifdef	PROTO_EGP
					case RTPROTO_EGP:
						if (!parse_adv_ext(&egp_accept_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_EGP */
				}

				parse_adv_list("accept", "listen", adv, adv->adv_list);
			}
		| T_ACCEPT T_PROTO accept_interior preference_option '{' accept_list '}' EOS
			{
				adv_entry *adv;

				/*
				 *	Append the dest_mask list to the end of the accept list
				 *	for the specified protocol.
				 */
				adv = adv_alloc((flag_t) 0, $3);
				adv->adv_list = $6;
				if ($4 > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = $4;
				}

				switch ($3) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						hello_accept_list = parse_adv_append(hello_accept_list, adv, TRUE);
						if (!hello_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_HELLO */
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
					case RTPROTO_REDIRECT:
						redirect_accept_list = parse_adv_append(redirect_accept_list, adv, TRUE);
						if (!redirect_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						rip_accept_list = parse_adv_append(rip_accept_list, adv, TRUE);
						if (!rip_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_RIP */
				}
				parse_adv_list("accept", "listen", adv, adv->adv_list);
			}
		| T_ACCEPT T_PROTO accept_interior T_INTERFACE interface_list preference_option '{' accept_list '}' EOS
			{
				adv_entry *adv, *advn, **int_adv = (adv_entry **) 0;

				switch ($3) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						int_adv = parse_adv_interface(&hello_int_accept);
						break;
#endif	/* PROTO_HELLO */
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
					case RTPROTO_REDIRECT:
						int_adv = parse_adv_interface(&redirect_int_accept);
						break;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						int_adv = parse_adv_interface(&rip_int_accept);
						break;
#endif	/* PROTO_RIP */
				}

				for (adv = $5; adv; adv = adv->adv_next) {
				    adv->adv_proto = $3;
				    if ($6 > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = $6;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, $8, FALSE);
				}
				adv = $5;
				adv_free_list($8);
				parse_adv_list("accept", "listen", adv, adv->adv_list);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					if (!(int_adv[adv->adv_ifp->int_index - 1] =
					    parse_adv_append(INT_CONTROL(int_adv, adv->adv_ifp), adv, TRUE))) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			}
		| T_ACCEPT T_PROTO accept_interior T_GATEWAY gateway_list preference_option '{' accept_list '}' EOS
			{
				/*
				 * A side effect is that accept_interior sets parse_gwlist for gateway_list
				 */
				adv_entry *adv, *advn;

				for (adv = $5; adv; adv = adv->adv_next) {
				    adv->adv_proto = $3;
				    if ($6 > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = $6;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, $8, FALSE);
				}
				adv = $5;
				adv_free_list($8);
				parse_adv_list("accept", "listen", adv, adv->adv_list);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					adv->adv_gwp->gw_accept = parse_adv_append(adv->adv_gwp->gw_accept, adv, TRUE);
					if (!adv->adv_gwp->gw_accept) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			}
		| T_PROPAGATE T_PROTO proto_exterior T_AS autonomous_system metric_option '{' prop_source_list '}' EOS
			{
				adv_entry *adv;

				/*
				 *	Tack the list of destinations onto the end of the list
				 *	for neighbors with the specified AS.
				 */
				adv = adv_alloc(ADVF_TAS, $3);
				adv->adv_as = $5;
				adv->adv_list = $8;
				if ($6 > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = $6;
				}

				switch ($3) {
#ifdef	PROTO_BGP
					case RTPROTO_BGP:
						if (!parse_adv_ext(&bgp_propagate_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_BGP */
#ifdef	PROTO_EGP
					case RTPROTO_EGP:
						if (!parse_adv_ext(&egp_propagate_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_EGP */
				}

				parse_adv_prop_list(adv);
			}
		| T_PROPAGATE T_PROTO proto_interior metric_option '{' prop_source_list '}' EOS
			{
				adv_entry *adv;

				/*
				 *	Append the dest_mask list to the end of the propagate list
				 *	for the specified protocol.
				 */
				adv = adv_alloc((flag_t) 0, $3);
				adv->adv_list = $6;
				if ($4 > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = $4;
				}

				switch ($3) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						hello_propagate_list = parse_adv_append(hello_propagate_list, adv, TRUE);
						if (!hello_propagate_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_HELLO */
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						rip_propagate_list = parse_adv_append(rip_propagate_list, adv, TRUE);
						if (!rip_propagate_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_RIP */
				}
				parse_adv_prop_list(adv);
			}
		| T_PROPAGATE T_PROTO proto_interior T_INTERFACE interface_list metric_option '{' prop_source_list '}' EOS
			{
				adv_entry *adv, *advn, **int_adv = (adv_entry **) 0;

				switch ($3) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						int_adv = parse_adv_interface(&hello_int_propagate);
						break;
#endif	/* PROTO_HELLO */
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						int_adv = parse_adv_interface(&rip_int_propagate);
						break;
#endif	/* PROTO_RIP */
				}

				for (adv = $5; adv; adv = adv->adv_next) {
				    adv->adv_proto = $3;
				    if ($6 > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = $6;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, $8, FALSE);
				}
				adv = $5;
				adv_free_list($8);
				parse_adv_prop_list(adv);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					if (!(int_adv[adv->adv_ifp->int_index - 1] =
					    parse_adv_append(INT_CONTROL(int_adv, adv->adv_ifp), adv, TRUE))) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			}
		| T_PROPAGATE T_PROTO proto_interior T_GATEWAY gateway_list metric_option '{' prop_source_list '}' EOS
			{
			    /*
			     * A side effect is that prop_interior sets parse_gwlist for gateway_list
			     */
			    adv_entry *adv, *advn;

			    for (adv = $5; adv; adv = adv->adv_next) {
				adv->adv_proto = $3;
				if ($6 > 0) {
				    adv->adv_flag |= ADVF_OTMETRIC;
				    adv->adv_metric = $6;
				}
				adv->adv_list = parse_adv_append(adv->adv_list, $8, FALSE);
			    }
			    adv = $5;
			    adv_free_list($8);
			    parse_adv_prop_list(adv);
			    do {
				advn = adv->adv_next;
				adv->adv_next = NULL;
				adv->adv_gwp->gw_propagate = parse_adv_append(adv->adv_gwp->gw_propagate, adv, TRUE);
				if (!adv->adv_gwp->gw_propagate) {
				    PARSE_ERROR;
				}
			    } while (adv = advn);
			}
		;	

/*  */

/* Support for accept clauses */

accept_interior	: T_RIP
			{
#ifdef	PROTO_RIP
				$$ = parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_RIP */
			}
		| T_HELLO
			{
#ifdef	PROTO_HELLO
				$$ = parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_HELLO */
			}
		| T_REDIRECT
			{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				$$ = parse_proto = RTPROTO_REDIRECT;
				parse_gwlist = &redirect_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			}
		;

accept_list	: /* Empty */
			{
				$$ = (adv_entry *) 0;
			}
		| accept_list accept_listen EOS
			{
			    if ($1) {
				$$ = parse_adv_append($1, $2, TRUE);
				if (!$$) {
					PARSE_ERROR;
				}
			    } else {
				$$ = $2;
			    }
			}
		| accept_list error EOS
			{
				yyerrok;
			}
		;

accept_listen	: T_LISTEN dest_mask preference_option
			{
				$$ = adv_alloc(ADVF_TDM, parse_proto);
				$$->adv_dm = $2;
				if ($3 > 0) {
					$$->adv_preference = $3;
					$$->adv_flag |= ADVF_OTPREFERENCE;
				}
			}
		| T_NOLISTEN dest_mask
			{
				$$ = adv_alloc(ADVF_TDM | ADVF_NO, parse_proto);
				$$->adv_dm = $2;
			}
		;

/*  */

/* Support for Propagate clauses */

prop_source_list
		: prop_source EOS
			{
				$$ = $1;
			}
		| prop_source_list prop_source EOS
			{
				$$ = parse_adv_append($1, $2, TRUE);
				if (!$$) {
					PARSE_ERROR;
				}
			}
		| prop_source_list error EOS
			{
				yyerrok;
			}
		;

prop_source	: T_PROTO proto_exterior T_AS autonomous_system metric_option prop_restrict_option
			{
				$$ = adv_alloc(ADVF_TAS | ADVF_FIRST, (proto_t) 0);
				$$->adv_as = $4;
				$$ = parse_adv_propagate($$, $2, $5, $6);
			}
		| T_PROTO prop_interior metric_option prop_restrict_option
			{
				$$ = adv_alloc(ADVF_TANY | ADVF_FIRST, (proto_t) 0);
				$$ = parse_adv_propagate($$, $2, $3, $4);
			}
		| T_PROTO prop_interior T_INTERFACE interface_list metric_option prop_restrict_option
			{
				$$ = parse_adv_propagate($4, $2, $5, $6);
			}
		| T_PROTO prop_interior T_GATEWAY gateway_list metric_option prop_restrict_option
			{
				$$ = parse_adv_propagate($4, $2, $5, $6);
			}
		;

prop_interior	: T_RIP
			{
#ifdef	PROTO_RIP
				$$ = parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_RIP */
			}
		| T_HELLO
			{
#ifdef	PROTO_HELLO
				$$ = parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_HELLO */
			}
		| T_DIRECT
			{
				$$ = parse_proto = RTPROTO_DIRECT;
				parse_gwlist = (gw_entry **)0;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
			}
		| T_STATIC
			{
				$$ = parse_proto = RTPROTO_STATIC;
				parse_gwlist = &rt_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
			}
		| T_DEFAULT
			{
				$$ = parse_proto = RTPROTO_DEFAULT;
				parse_gwlist = (gw_entry **)0;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
			}
		;


prop_restrict_option
		: /* Empty */
			{
				$$ = (adv_entry *) 0;
			}
		| '{' prop_restrict_list '}'
			{
				$$ = $2;
			}
		;


prop_restrict_list
		: /* Empty */
			{
			    $$ = (adv_entry *) 0;
			}
		| prop_restrict_list prop_restrict EOS
			{
			    if ($1) {
				$$ = parse_adv_append($1, $2, TRUE);
				if (!$$) {
				    PARSE_ERROR;
				}
			    } else {
				$$ = $2;
			    }
			}
		| prop_restrict_list error EOS
			{
			    yyerrok;
			}
		;

prop_restrict	: T_ANNOUNCE dest_mask metric_option
			{
			    $$ = adv_alloc(ADVF_TDM, (proto_t) 0);
			    $$->adv_dm = $2;
			    if ($3 >= 0) {
				$$->adv_metric = $3;
				$$->adv_flag |= ADVF_OTMETRIC;
			    }
			}
		| T_NOANNOUNCE dest_mask
			{
			    $$ = adv_alloc(ADVF_TDM | ADVF_NO, (proto_t) 0);
			    $$->adv_dm = $2;
			}
		;

/*  */

/* Addresses */
dest_mask_list	: dest_mask EOS
			{
				$$ = adv_alloc(ADVF_TDM, (proto_t) 0);
				$$->adv_dm = $1;
			}
		| dest_mask_list dest_mask EOS
			{
				$$ = adv_alloc(ADVF_TDM, (proto_t) 0);
				$$->adv_dm = $2;
				$$ = parse_adv_append($1, $$, TRUE);
				if (!$$) {
					PARSE_ERROR;
				}
			}
		| dest_mask_list error EOS
			{
				yyerrok;
			}
		;
		
	
/* Destination and mask pair used for control lists */
dest_mask	: T_ALL
			{
			    /* XXX - need to specify a generic protocol */
			    sockclear_in(&$$.dm_dest.in);
			    sockclear_in(&$$.dm_mask.in);
			    $$.dm_dest.in.sin_addr.s_addr = INADDR_ANY;
			    $$.dm_mask.in.sin_addr.s_addr = INADDR_ANY;
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &$$.dm_dest,
				  &$$.dm_mask);
			}
		| dest
			{
			    /* XXX - need to match protocols */
			    $$.dm_dest = $1;		/* struct copy */
			    sockclear_in(&$$.dm_mask.in);
			    $$.dm_mask.in.sin_addr.s_addr = INADDR_BROADCAST;
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &$$.dm_dest,
				  &$$.dm_mask);
			}
		| dest T_MASK dest
			{
			    $$.dm_dest = $1;		/* struct copy */
			    $$.dm_mask = $3;		/* struct copy */
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &$$.dm_dest,
				  &$$.dm_mask);
			}
		;

/* A destination is any host or network */
dest		: ip_addr
			{
				$$ = $1;
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&$$);
			}
		| T_DEFAULT
			{
			    sockclear_in(&$$);
			    socktype_in(&$$)->sin_addr.s_addr = INADDR_ANY;
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&$$);
			}
		| host_name
			{ 
				if(parse_addr_hname(&$$, $1, TRUE, TRUE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&$$);
			}
		;

/* Gateway list */
gateway_list : gateway
			{
				$$ = adv_alloc(ADVF_TGW | ADVF_FIRST, (proto_t) 0);
				$$->adv_gwp = $1;
			}
		| gateway_list gateway
			{
				$$ = adv_alloc(ADVF_TGW, (proto_t) 0);
				$$->adv_gwp = $2;
				$$ = parse_adv_append($1, $$, TRUE);
				if (!$$) {
					PARSE_ERROR;
				}
			}
		;

/* A gateway is a host on an attached network */
gateway		: host
			{
				/*
				 *	Make sure host is on a locally attached network then
				 *	find or create a gw structure for it.  Requires that
				 *	parse_proto and parse_gwlist are previously set
				 */
				if (!if_withdst(&$1)) {
					(void) sprintf(parse_error, "gateway not a host address on an attached network: '%A'",
						&$1);
					PARSE_ERROR;
				}
				if (!parse_gwlist) {
					(void) sprintf(parse_error, "gateway specification not valid for %s",
						trace_bits(rt_proto_bits, parse_proto));
					PARSE_ERROR;
				}
				$$ = gw_locate(parse_gwlist, parse_proto, &$1);
				trace(TR_PARSE, 0, "parse: %s GATEWAY: %A  PROTO: %s",
					parse_where(),
					&$$->gw_addr,
					gd_lower(trace_bits(rt_proto_bits, $$->gw_proto)));
			}
		;

/* A host is a host */
host		: ip_addr
			{
				if (!gd_inet_ishost(&$1)) {
					(void) sprintf(parse_error, "not a host address: '%A'",
						&$1);
					PARSE_ERROR;
				}
				$$ = $1;
				trace(TR_PARSE, 0, "parse: %s HOST: %A",
					parse_where(),
					&$$);
			}
		| host_name
			{ 
				if(parse_addr_hname(&$$, $1, TRUE, FALSE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s HOST: %A",
					parse_where(),
					&$$);
			}
		;

/* A network is a network */
network		: ip_addr
			{
				if (gd_inet_ishost(&$1)) {
					(void) sprintf(parse_error, "not a network address: '%A'",
						&$1);
					PARSE_ERROR;
				}
				$$ = $1;
				trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
					parse_where(),
					&$$);
			}
		| T_DEFAULT
			{
			    sockclear_in(&$$);
			    socktype_in(&$$)->sin_addr.s_addr = INADDR_ANY;
			    trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
				  parse_where(),
				  &$$);
			}
		| host_name
			{ 
				if(parse_addr_hname(&$$, $1, FALSE, TRUE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
					parse_where(),
					&$$);
			}
		;

/* IP address */
ip_addr		: octet
			{
				u_long addr;

				addr = $1 << 24;
				parse_addr_long(&$$, htonl(addr));
			}
		| octet '.' octet
			{
				u_long addr;

				addr = 	($1 << 24) + ($3 << 16);
				parse_addr_long(&$$, htonl(addr));
			}
		| octet '.' octet '.' octet
			{
				u_long addr;

				addr = ($1 << 24) + ($3 << 16) + ($5 << 8);
				parse_addr_long(&$$, htonl(addr));
			}
		| octet '.' octet '.' octet '.' octet
			{
				u_long addr;

				addr = ($1 << 24) + ($3 << 16) + ($5 << 8) + $7;
				parse_addr_long(&$$, htonl(addr));
			}
		;

/* Host name */
host_name	: HNAME
			{
			    $$ = $1;
			}
		| string
			{
			    $$ = $1;
			}
		;

/* Protocols */
proto_interior	: T_RIP
			{
#ifdef	PROTO_RIP
				$$ = parse_prop_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_RIP */
			}
		| T_HELLO
			{
#ifdef	PROTO_HELLO
				$$ = parse_prop_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_HELLO */
			}
		;

proto_exterior	: T_EGP
			{
#ifdef	PROTO_EGP
				$$ = parse_prop_proto = RTPROTO_EGP;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_EGP */
			}
		| T_BGP
			{
#ifdef	PROTO_BGP
				$$ = parse_prop_proto = RTPROTO_BGP;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, $$)));
#endif	/* PROTO_BGP */
			}
		;

onoff_option	: T_ON		{ $$ = T_ON; }
		| T_OFF		{ $$ = T_OFF; }
		;

interior_option	: onoff_option	{ $$ = $1; }
		| T_QUIET	{ $$ = T_QUIET; }
		| T_SUPPLIER	{ $$ = T_SUPPLIER; }
		| T_POINTOPOINT	{ $$ = T_POINTOPOINT; }
		;

/* Metric */

metric		: NUMBER
			{
				$$ = $1;
				trace(TR_PARSE, 0, "parse: %s METRIC: %d",
					parse_where(),
					$$);
			}
		;

metric_option	: /* Empty */
			{
				$$ = -1;
			}
		| T_METRIC metric
			{
				$$ = $2;
				if (parse_metric_check(parse_prop_proto, $2)) {
				    PARSE_ERROR;
				}
			}
		;
		
/* Preference */
preference_option
		: /* Empty */
			{
				/* Take advantage of the fact that only interfaces can have a preference of zero */
				$$ = 0;
			}
		| T_PREFERENCE preference
			{
				$$ = $2;
			}
		;

preference	: NUMBER
			{
				if (parse_limit_check("preference", $1, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
				$$ = $1;
			}
		| pref_option
			{
				if (parse_limit_check("preference", $1, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
				$$ = $1;
			}
		| pref_option '+' NUMBER
			{
				$$ = $1 + $3;
				if (parse_limit_check("preference", $$, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
			}
		| pref_option '-' NUMBER
			{
				$$ = $1 - $3;
				if (parse_limit_check("preference", $$, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
			}
		;


pref_option	: T_DIRECT	{ $$ = RTPREF_DIRECT; }
		| T_DEFAULT	{ $$ = RTPREF_DEFAULT; }
		| T_REDIRECT	{ $$ = RTPREF_REDIRECT; }
		| T_STATIC	{ $$ = RTPREF_STATIC; }
/*		| T_OSPF	{ $$ = RTPREF_OSPF; } */
/*		| T_IGRP	{ $$ = RTPREF_IGRP; } */
		| T_HELLO	{ $$ = RTPREF_HELLO; }
		| T_RIP		{ $$ = RTPREF_RIP; }
		| T_BGP		{ $$ = RTPREF_BGP; }
		| T_EGP		{ $$ = RTPREF_EGP; }
		| T_KERNEL	{ $$ = RTPREF_KERNEL; }
		;

/* Numbers and such */
string		: STRING
			{
				/* Remove quotes from the string if present */
				char *cp;

				cp = $1;
				if (*cp == '"' || *cp == '<') {
					cp++;
				}
				$$ = parse_strdup(cp);
				cp = $$;
				cp += strlen(cp) - 1;
				if (*cp == '"' || *cp == '>') {
					*cp = (char) 0;
				}
				trace(TR_PARSE, 0, "parse: %s STRING: \"%s\"",
					parse_where(),
					$$);
			}
		;

octet		: NUMBER
			{
				if (parse_limit_check("octet", $1, LIMIT_OCTET_LOW, LIMIT_OCTET_HIGH)) {
					PARSE_ERROR;
				}
				$$ = $1;
			}
		;


time		: NUMBER
			{
			  	if (parse_limit_check("seconds", $1, 0, -1)) {
				  	PARSE_ERROR;
				};
				$$ = $1;
			}
		| NUMBER ':' NUMBER
			{
			  	if (parse_limit_check("minutes", $1, 0, -1)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("seconds", $3, 0, 59)) {
				  	PARSE_ERROR;
				}
				$$ = ($1 * 60) + $3;
			}
		| NUMBER ':' NUMBER ':' NUMBER
			{
			  	if (parse_limit_check("hours", $1, 0, -1)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("minutes", $3, 0, 59)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("seconds", $5, 0, 59)) {
				  	PARSE_ERROR;
				}
				$$ = (($1 * 60) + $3) * 60 + $5;
			}
		;

/* Internet ports */
port		: NUMBER
			{
			    if (parse_limit_check("port", $1, LIMIT_PORT_LOW, LIMIT_PORT_HIGH)) {
				PARSE_ERROR;
			    }
			    $$ = $1;
			}
		| HNAME
			{
			    struct servent *sp;

			    if (!(sp = getservbyname($1, parse_serv_proto))) {
				(void) sprintf(parse_error, "unknown protocol %s/%s",
					$1, parse_serv_proto);
				PARSE_ERROR;
			    }

			    $$ = sp->s_port;

			    trace(TR_PARSE, 0, "parse: %s PORT %s (%d)",
				  parse_where(),
				  $1,
				  ntohs($$));
			}
		;
%%

/*
 *	Log any parsing errors
 */
static void
yyerror(s)
char *s;
{
	const char *cp;

	trace(TR_ALL, 0, NULL);
	tracef("parse: %s %s ",
		parse_where(),
		s);

	switch (yychar) {
	case STRING:
	case UNKNOWN:
	case HNAME:
	    tracef("at '%s'",
		   yylval.ptr);
	    break;
	case NUMBER:
	    tracef("at '%d'",
		   yylval.num);
	    break;
	default:
	    cp = parse_keyword_lookup(yychar);
	    if (cp) {
		tracef("at '%s'",
		   cp);
	    }
	}
	trace(TR_ALL, LOG_ERR, NULL);
	trace(TR_ALL, 0, NULL);
}
