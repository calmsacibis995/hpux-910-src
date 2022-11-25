/*
 * $Header: parse.h,v 1.1.109.5 92/02/28 15:59:08 ash Exp $
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


#define	PS_DEFINE	0
#define	PS_PROTO	1
#define	PS_ROUTE	2
#define	PS_CONTROL	3
#define	PS_MAX		PS_CONTROL

#define	LIMIT_AS_LOW	1
#define	LIMIT_AS_HIGH	65534

#define	LIMIT_RIP_LOW	0
#define	LIMIT_RIP_HIGH	16

#define	LIMIT_HELLO_LOW	0
#define	LIMIT_HELLO_HIGH	30000

#define	LIMIT_INTERFACE_LOW	0
#define	LIMIT_INTERFACE_HIGH	16

#define	LIMIT_PREFERENCE_LOW	0
#define	LIMIT_PREFERENCE_HIGH	255

#define	LIMIT_EGP_LOW		0
#define	LIMIT_EGP_HIGH		255

#define	LIMIT_BGP_LOW		0
#define	LIMIT_BGP_HIGH		0xffff

#define	LIMIT_OCTET_LOW		0
#define	LIMIT_OCTET_HIGH	255

#define	LIMIT_PORT_LOW		0
#define	LIMIT_PORT_HIGH		65535

typedef struct {
    char *fi_name;			/* File name */
    FILE *fi_FILE;			/* File pointer */
    int fi_lineno;			/* Line number */
} fi_info;

#define	FI_MAX	10			/* Maxiumum %include nesting level */

extern int parse_state;

extern char *parse_where();
extern char *parse_strdup();
extern int parse_include();
extern int parse_open();

extern int yylineno;
extern char parse_error[];
extern char *parse_filename;
extern char *parse_directory;
extern metric_t parse_metric;
extern proto_t protos_seen;
extern flag_t parse_flags;
extern pref_t parse_preference;
extern sockaddr_un parse_addr;

#ifdef	USE_PROTOTYPES
extern int yyparse(void);
extern int parse_keyword(char *text);	/* Lookup a token given a keyword */
extern const char *parse_keyword_lookup(int token);	/* Lookup a keyword given a token */
extern int parse_parse(const char *file);	/* Parse the config file */
char *parse_strdump(char *s);		/* Return a pointer to a duplicate string */
char *parse_where(void);		/* Return pointer to a string  giving current file and line */
int
parse_limit_check(const char *type,
		  int value,
		  int lower,
		  int upper);		/* Limit check an integer */
void
parse_addr_long(sockaddr_un * sockaddr,
		u_long addr);		/* Convert an nunsigned long IP address into a sockaddr_in */
int
parse_addr_hname(sockaddr_un * addr,
		 char *hname,
		 int host_ok,
		 int net_ok);		/* Lookup a string as a network or host name */
adv_entry *
parse_adv_append(adv_entry * old,
		 adv_entry * new,
		 int free);		/* Append one advlist to another */
int
parse_gw_flag(adv_entry * list,
	      proto_t proto,
	      flag_t flag);		/* Set flag in gw_entry for each element in list */
void parse_adv_entry(adv_entry * list);	/* Display an adv_entry */
void
parse_adv_destmask(adv_entry * dest,
		   const char *name1,
		   const char *name2);	/* Display a dest-mask list */
void
parse_adv_list(const char *name1,
	       const char *name2,
	       adv_entry * list,
	       adv_entry * dest);	/* Display a list entry with dest-mask */
void parse_adv_prop_list(adv_entry * list);	/* Display a propagate clause */
void parse_interface(adv_entry * list);	/* Set interface values on specified list or all interfaces */
int parse_new_state(int state);		/* Switch to a new state if it is a logical progression from the current state */
int
parse_metric_check(proto_t proto,
		   int metric);		/* Verify a specified metric */
adv_entry *
parse_adv_propagate(adv_entry * list,
		    proto_t proto,
		    metric_t metric,
		    adv_entry * advlist);	/* Set metric in list for elements without metrics */
void
parse_adv_preference(adv_entry * list,
		     proto_t proto,
		     pref_t preference);/* Set preference in list for elements without preference */
adv_entry *parse_adv_dup(adv_entry * old);	/* Return a pointer to a duplicate of this adv_entry */
adv_entry **parse_adv_interface(adv_entry *** list);	/* Create the specified interface adv_list if necessary */
int
parse_adv_ext(adv_entry ** advlist,
	      adv_entry * adv);		/* Append this list to the list for the specified exterior protocol */

#else				/* USE_PROTOTYPES */
extern int yyparse();
extern int parse_keyword();		/* Lookup a token given a keyword */
extern char *parse_keyword_lookup();	/* Lookup a keyword given a token */
extern int parse_parse();		/* Parse the config file */
char *parse_strdump();			/* Return a pointer to a duplicate string */
char *parse_where();			/* Return pointer to a string  giving current file and line */
int parse_limit_check();		/* Limit check an integer */
void parse_addr_long();			/* Convert an nunsigned long IP address into a sockaddr_in */
int parse_addr_hname();			/* Lookup a string as a network or host name */
adv_entry *parse_adv_append();		/* Append one advlist to another */
int parse_gw_flag();			/* Set flag in gw_entry for each element in list */
void parse_adv_entry();			/* Display an adv_entry */
void parse_adv_destmask();		/* Display a dest-mask list */
void parse_adv_list();			/* Display a list entry with dest-mask */
void parse_adv_prop_list();		/* Display a propagate clause */
void parse_interface();			/* Set interface values on specified list or all interfaces */
int parse_new_state();			/* Switch to a new state if it is a logical progression from the current state */
int parse_metric_check();		/* Verify a specified metric */
adv_entry *parse_adv_propagate();	/* Set metric in list for elements without metrics */
void parse_adv_preference();		/* Set preference in list for elements without preference */
adv_entry *parse_adv_dup();		/* Return a pointer to a duplicate of this adv_entry */
adv_entry **parse_adv_interface();	/* Create the specified interface adv_list if necessary */
int parse_adv_ext();			/* Append this list to the list for the specified exterior protocol */

#endif				/* USE_PROTOTYPES */
