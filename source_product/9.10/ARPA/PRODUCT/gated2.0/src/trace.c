/*
 *  $Header: trace.c,v 1.1.109.5 92/02/28 16:02:12 ash Exp $
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
#if	defined(_IBMR2)
#include <time.h>
#endif				/* defined(_IBMR2) */
#include <sys/time.h>
#ifndef vax11c
#include <sys/file.h>
#include <sys/stat.h>
#endif				/* vax11c */

flag_t trace_flags;			/* log errors, route changes &/or packets */
flag_t trace_flags_save;		/* save trace flags */
char *trace_file = NULL;		/* File to trace to */
static FILE *trace_FILE = NULL;
static char trace_buffer[BUFSIZ];

static const char *month_names[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bits trace_types[] =
{
    {TR_INT | TR_EXT | TR_RT, "general"},
    {TR_ALL, "all"},
    {TR_INT, "internal"},
    {TR_EXT, "external"},
    {TR_RT, "route"},
    {TR_PARSE, "parse"},
    {TR_CONFIG, "config"},
    {TR_LEX, "lex"},
#ifdef	PROTO_EGP
    {TR_EGP, "egp"},
#endif				/* PROTO_EGP */
    {TR_UPDATE, "update"},
#ifdef	PROTO_RIP
    {TR_RIP, "rip"},
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
    {TR_HELLO, "hello"},
#endif				/* PROTO_HELLO */
#ifdef	PROTO_ICMP
    {TR_ICMP, "icmp"},
#endif				/* PROTO_ICMP */
    {TR_TASK, "task"},
    {TR_TIMER, "timer"},
    {TR_NOSTAMP, "nostamp"},
    {TR_MARK, "mark"},
    {TR_PROTOCOL, "protocol"},
    {TR_KRT, "kernel"},
#ifdef	PROTO_OSPF
    {TR_OSPF, "ospf"},
#endif				/* PROTO_OSPF */
#ifdef	PROTO_IGRP
    {TR_IGRP, "igrp"},
#endif				/* PROTO_IGRP */
#ifdef	PROTO_BGP
    {TR_BGP, "bgp"},
#endif				/* PROTO_BGP */
#if	defined(AGENT_SNMP)
    {TR_SNMP, "snmp"},
#endif				/* defined(AGENT_SNMP) */
    {0, 0}
};


/*
 *	Display trace options enabled
 */
void
trace_display(tr_flags)
flag_t tr_flags;
{

    trace(TR_ALL, 0, NULL);
    trace(TR_ALL, 0, "Tracing flags enabled: %s", tr_flags ? trace_bits(trace_types, tr_flags) : "none");
    trace(TR_ALL, 0, NULL);
}


/*
 * Turn off tracing.
 */
void
trace_off()
{
    if (!trace_flags) {
	return;
    }
    if (trace_FILE != NULL) {
	trace(TR_ALL, 0, NULL);
	trace(TR_ALL, LOG_NOTICE, "Tracing to \"%s\" suspended", trace_file ? trace_file : "(stdout)");
	trace(TR_ALL, 0, NULL);
	trace_flags = 0;
	(void) fclose(trace_FILE);
	trace_FILE = NULL;
    }
    return;
}


/*
 * Close trace file
 */
void
trace_close()
{
    if (trace_FILE != NULL) {
	trace_flags = 0;
	(void) fclose(trace_FILE);
	trace_FILE = NULL;
    }
    return;
}


/*
 * Turn on tracing.
 */
void
trace_on(file, append)
char *file;
int append;
{
#ifndef vax11c
    struct stat stbuf;

#endif				/* vax11c */

    if (file == NULL) {
	trace_FILE = stdout;
    } else {
	if (trace_FILE == NULL) {
#ifndef vax11c
	    if (stat(file, &stbuf)) {
		if (errno != ENOENT) {
		    trace(TR_ALL, LOG_ERR, "trace_on: stat(%s): %m", file);
		    return;
		}
	    } else if ((stbuf.st_mode & S_IFMT) != S_IFREG) {
		trace(TR_ALL, LOG_ERR, "trace_on: \"%s\" is not a regular file", file);
		return;
	    }
#endif				/* vax11c */
	    if ((trace_FILE = fopen(file, append ? "a" : "w")) == NULL) {
		trace(TR_ALL, LOG_ERR, "trace_on: open(%s): %m", file);
		return;
	    }
	}
#ifndef vax11c
	setlinebuf(trace_FILE);
#endif				/* vax11c */
	trace(TR_ALL, LOG_ERR, "tracing to \"%s\" started", file ? file : "(stdout)");
    }

    trace_flags = trace_flags_save;
    trace_display(trace_flags);
}


/*
 *  Parse trace flags specified on the command line
 */
flag_t
trace_args(flag)
char *flag;
{
    int tr_flags = 0, new_trace;
    struct chars {
	u_int c_bits;
	char c_char;
    } *p;
    static struct chars trace_chars[] =
    {
	{TR_INT, 'i'},
	{TR_EXT, 'e'},
	{TR_RT, 'r'},
#ifdef	PROTO_EGP
	{TR_EGP, 'p'},
#endif				/* PROTO_EGP */
	{TR_UPDATE, 'u'},
	{TR_PROTOCOL, 'P'},
	{TR_ALL, 'A'},
	{TR_TASK | TR_TIMER, 'T'},
#ifdef	PROTO_ICMP
	{TR_ICMP, 'C'},
#endif				/* PROTO_ICMP */
	{TR_MARK, 'm'},
	{TR_NOSTAMP, 't'},
#ifdef	PROTO_RIP
	{TR_RIP, 'R'},
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
	{TR_HELLO, 'H'},
#endif				/* PROTO_HELLO */
#ifdef	PROTO_OSPF
	{TR_OSPF, 'O'},
#endif				/* PROTO_OSPF */
#ifdef	PROTO_IGRP
	{TR_IGRP, 'G'},
#endif				/* PROTO_IGRP */
	{TR_KRT, 'k'},
#ifdef	PROTO_BGP
	{TR_BGP, 'B'},
#endif				/* PROTO_BGP */
#if	defined(AGENT_SNMP)
	{TR_SNMP, 'M'},
#endif				/* defined(AGENT_SNMP) */
	{0, (char) 0}};


    if (*flag == (char) 0) {
	return (TR_GEN);
    }
    for (; *flag; flag++) {
	new_trace = 0;
	for (p = trace_chars; p->c_bits; p++) {
	    if (*flag == p->c_char) {
		new_trace = p->c_bits;
		break;
	    }
	}
	if (!(new_trace)) {
	    (void) fprintf(stderr, "%s: unknown trace flag: %c\n", my_name, *flag);
	} else {
	    tr_flags |= new_trace;
	}
    }
    return (tr_flags);
}


char *
trace_bits(bp, mask)
bits *bp;
flag_t mask;
{
    static char string[BUFSIZ];
    int first = TRUE;
    bits *p;

    *string = (char) 0;

    for (p = bp; p->t_bits; p++) {
	if ((mask & p->t_bits) == p->t_bits) {
	    if (first) {
		first = FALSE;
	    } else {
		(void) strcat(string, " ");
	    }
	    (void) strcat(string, p->t_name);
	}
    }

    return (string);
}


#ifndef	trace_state
char *
trace_state(bp, mask)
bits *bp;
flag_t mask;
{

    return (bp[mask].t_name);
}

#endif				/* trace_state */

/*
 *  Dump everything
 */
static void
trace_do_dump()
{
    FILE *fd;

    if ((fd = fopen(DUMPFILE, "a")) <= (FILE *) 0) {
	trace(TR_ALL, LOG_ERR, "trace_dump: %m");
	return;
    }
#ifndef vax11c
    setlinebuf(fd);
#endif				/* vax11c */
    trace(TR_ALL, LOG_NOTICE, "trace_dump: processing dump to %s", DUMPFILE);
    (void) fprintf(fd, "\f\n\t%s[%d] version %s memory dump on %s at %s\n", my_name, my_mpid, version, my_hostname, time_full);
    if (version_kernel) {
	(void) fprintf(fd, "\t\t%s\n\n", version_kernel);
    }
    /* Task_dump dumps all protocols */
    task_dump(fd);

    (void) fflush(fd);
    (void) fclose(fd);
    trace(TR_ALL, LOG_NOTICE, "trace_dump: dump completed to %s", DUMPFILE);
}


#ifndef	NO_FORK
static task *trace_dump_task;		/* Pointer to the dump task */

static void
trace_dump_done(tp)
task *tp;
{
    trace_dump_task = (task *) 0;
    task_delete(tp);
}

#endif				/* NO_FORK */


void
trace_dump(now)
int now;
{

#ifndef	NO_FORK
    if (trace_dump_task) {
	trace(TR_ALL, LOG_ERR, "trace_dump: %s already active",
	      task_name(trace_dump_task));
    } else if (now) {
	trace_do_dump();
    } else {
	trace_dump_task = task_alloc("TraceDump");
	trace_dump_task->task_child = trace_dump_done;
	trace_dump_task->task_process = trace_do_dump;
	if (!task_fork(trace_dump_task)) {
	    quit(EINVAL);
	}
    }
#else				/* NO_FORK */
    trace_do_dump();
#endif				/* NO_FORK */
}


/*ARGSUSED*/
void
trace_mark(tp)
task *tp;
{
    struct tm *tm;

    tm = localtime(&time_sec);
    trace(TR_MARK | TR_NOSTAMP, 0, "%s %2d %02d:%02d:%02d MARK",
	  month_names[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    return;
}


/*
 *	Trace to the log and syslog
 */
#ifdef	STDARG
/*VARARGS2*/
void
trace(flag_t flags, int pri, const char *fmt,...)
#else				/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
void
trace(va_alist)
va_dcl

#endif				/* STDARG */
{
    struct tm *tm;
    char time_buffer[BUFSIZ];
    va_list args;

#ifdef	STDARG

    va_start(args, fmt);
#else				/* STDARG */
    int flags, pri;
    u_char *fmt;

    va_start(args);

    flags = va_arg(args, int);
    pri = va_arg(args, int);
    fmt = va_arg(args, u_char *);
#endif				/* STDARG */

    if (fmt && *fmt) {
	(void) vsprintf(&trace_buffer[strlen((char *) trace_buffer)], fmt, &args);
    }
    va_end(args);

#ifdef	vax11c
    if (trace_FILE && (trace_FILE != sstdout)) {
#endif	/* vax11c */
	if ((trace_flags & flags) & ~TR_NOSTAMP) {
	    *time_buffer = 0;
	    if (!((trace_flags | flags) & TR_NOSTAMP)) {
		tm = localtime(&time_sec);
		(void) sprintf(time_buffer, "%s %2d %02d:%02d:%02d ",
			       month_names[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	    }
	    if (my_mpid != my_pid) {
		(void) sprintf(&time_buffer[strlen(time_buffer)], "[%d] ",
			       my_pid);
	    }
	    if (*time_buffer) {
		(void) fputs(time_buffer, trace_FILE);
	    }
	    (void) fputs((char *) trace_buffer, trace_FILE);
	    (void) fputc('\n', trace_FILE);
	}
	if (pri && !test_flag) {
	    syslog(pri, (char *) trace_buffer);
	}
#ifdef	vax11c
    } else {
	if (((trace_flags & flags) & ~TR_NOSTAMP) || (pri && !test_flag))
	    syslog(pri, (char *) trace_buffer);
    }
#endif	/* vax11c */
    trace_buffer[0] = (char) 0;
}


/*
 *	Prefill the trace buffer
 */
#ifdef	STDARG
/*VARARGS2*/
void
tracef(const char *fmt,...)
#else				/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
void
tracef(va_alist)
va_dcl

#endif				/* STDARG */
{
    va_list args;

#ifdef	STDARG

    va_start(args, fmt);
#else				/* STDARG */
    u_char *fmt;

    va_start(args);

    fmt = va_arg(args, u_char *);
#endif				/* STDARG */

    if (fmt && *fmt) {
	(void) vsprintf(&trace_buffer[strlen((char *) trace_buffer)], fmt, &args);
    }
    va_end(args);
}
