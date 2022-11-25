/*
 * $Header: trace.h,v 1.1.109.5 92/02/28 16:02:16 ash Exp $
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


/*
 * trace.h
 */

#define	TIME_MARK	60*10		/* Duration between marks in seconds */

 /* tracing levels */
#define	TR_INT		0x01		/* internal errors */
#define TR_EXT		0x02		/* external changes resulting from egp */
#define TR_RT		0x04		/* routing changes */
#define TR_EGP		0x08		/* all egp packets sent and received */
#define TR_UPDATE	0x10		/* trace update info sent */
#define TR_RIP		0x20		/* trace update info sent */
#define TR_HELLO	0x40		/* trace update info sent */
#define	TR_TASK		0x80		/* trace task dispatching */
#define	TR_MARK		0x0100		/* time mark every TIME_MARK seconds */
#define	TR_SNMP		0x0200
#define	TR_ICMP		0x0400		/* ICMP packet */
#define	TR_PROTOCOL	0x0800		/* trace a protocol */
#define	TR_BGP		0x1000		/* BGP protocol */
#define	TR_OSPF		0x2000		/* OSPF protocol */
#define	TR_IGRP		0x4000		/* IGRP protocol */
#define	TR_KRT		0x8000		/* Kernel routes */
#define	TR_LEX		0x010000	/* Trace Lexical analyzer */
#define	TR_PARSE	0x020000	/* Trace Parser */
#define	TR_CONFIG	0x040000	/* Trace config file */
#define	TR_TIMER	0x080000	/* Trace timers */
#define	TR_ALL		~TR_NOSTAMP	/* trace everything but the time stamp */
#define	TR_GEN		TR_INT | TR_EXT | TR_RT	/* General flags */
#define TR_NOSTAMP	0x80000000	/* no timestamp requested */

#define	IF_EGPUPD	if ( (trace_flags & (TR_EGP|TR_UPDATE)) == (TR_EGP|TR_UPDATE) )
#define	IF_EGPPROTO	if ( (trace_flags & (TR_EGP|TR_PROTOCOL)) == (TR_EGP|TR_PROTOCOL) )
#define	IF_RIPUPD	if ( (trace_flags & (TR_RIP|TR_UPDATE)) == (TR_RIP|TR_UPDATE) )
#define	IF_HELUPD	if ( (trace_flags & (TR_HELLO|TR_UPDATE)) == (TR_HELLO|TR_UPDATE) )
#define	IF_SNMPUPD	if ( (trace_flags & (TR_SNMP|TR_UPDATE)) == (TR_SNMP|TR_UPDATE) )
#define	IF_KRTUPD	if ( (trace_flags & (TR_KRT|TR_UPDATE)) == (TR_KRT|TR_UPDATE) )
#define	IF_BGPUPD	if ( (trace_flags & (TR_BGP|TR_UPDATE)) == (TR_BGP|TR_UPDATE) )
#define	IF_BGPPROTO	if ( (trace_flags & (TR_BGP|TR_PROTOCOL)) == (TR_BGP|TR_PROTOCOL) )
#define	IF_OSPFUPD	if ( (trace_flags & (TR_OSPF|TR_UPDATE)) == (TR_OSPF|TR_UPDATE) )
#define	IF_IGRPUPD	if ( (trace_flags & (TR_IGRP|TR_UPDATE)) == (TR_IGRP|TR_UPDATE) )

/*
 *	Trace routines
 */
#ifdef	USE_PROTOTYPES
extern void trace_display(flag_t tr_flags);
extern void trace_off(void);
extern void trace_close(void);
extern void trace_on(char *file, int append);
extern flag_t trace_args(char *flag);
extern char *trace_bits(bits * bp, flag_t mask);
extern void trace_dump(int);
extern void trace_mark(task * tp);

#ifdef	STDARG
extern void trace(flag_t flags, int pri, const char *fmt,...);
extern void tracef(const char *fmt,...);

#else				/* STDARG */
extern void trace();
extern void tracef();

#endif				/* STDARG */
#else				/* USE_PROTOTYPES */
extern void trace_display();
extern void trace_off();
extern void trace_close();
extern void trace_on();
extern flag_t trace_args();
extern char *trace_bits();
extern void trace_dump();
extern void trace_mark();
extern void trace();
extern void tracef();

#endif				/* USE_PROTOTYPES */

#define trace_state(bits, mask) bits[mask].t_name

extern flag_t trace_flags;		/* trace packets and route changes */
extern flag_t trace_flags_save;		/* save tracing flags */
extern bits trace_types[];		/* Names for the tracing flags */
extern char *trace_file;		/* File we are tracing to */
