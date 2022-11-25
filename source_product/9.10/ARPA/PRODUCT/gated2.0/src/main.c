/*
 *  $Header: main.c,v 1.1.109.7 92/03/09 12:51:39 ash Exp $
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


/* main.c
 *
 * Main function of the EGP user process. Includes packet reception,
 * ICMP handler and timer interrupt handler.
 *
 * Functions: main, receive_packet, icmpin, timeout, getod, quit
 */
/*
 * Software Overview:
 *
 * At start up, the controlling function, main(), first sets tracing options
 * based on starting command arguments. It then calls the initialization
 * functions described in the next section, sets up signal handlers for
 * termination (SIGTERM) and timer interrupt (SIGALRM) signals, calls
 * task_dispatch() to start periodic interrupt processing, and finally waits in a
 * loop to receive incoming EGP or ICMP redirect packets
 *
 * When an EGP packet is received, egpin() is called to handle it. It in turn
 * calls a separate function for each EGP command type, which sends the
 * appropriate response. When an ICMP packet is received icmpin() is called.
 *
 * The timer interrupt routine, task_dispatch(), calls egpjob() to perform periodic
 * EGP processing such as reachability determination, command sending and
 * retransmission. It in turn calls separate functions to format each command
 * type. task_dispatch() also periodically calls rt_time() to increment route ages
 * and delete routes when too old.
 */

#include "include.h"
#include "bgp.h"
#include "egp.h"
#include "hello.h"
#include "icmp.h"
#include "rip.h"
#include "parse.h"
#if	defined(_IBMR2)
#include <time.h>
#endif				/* defined(_IBMR2) */
#include <signal.h>
#include <sys/time.h>
#ifdef	SYSV
#include <sys/fcntl.h>
#endif	/* SYSV */
#include <sys/file.h>
#ifdef	SYSV
#include <sys/sioctl.h>
#include <sys/stropts.h>
#else	/* SYSV */
#include <sys/ioctl.h>
#endif	/* SYSV */

static int conf_file_specified;		/* A config file was specified */
static int no_config_file;		/* No config file present */

extern void trace_mark();

#ifdef	AGENT_SNMP
extern void snmp_init();

#endif				/* AGENT_SNMP */

#ifndef	vax11c
int
parse_args(argc, argv)
int argc;
char **argv;
{
    int arg_n, err_flag = 0;
    char *arg, *cp, *ap;
    char seen[MAXHOSTNAMELENGTH];

    trace_file = (char *) 0;
    memset(seen, (char) 0, MAXHOSTNAMELENGTH);

    for (arg_n = 1; arg_n < argc; arg_n++) {
	arg = argv[arg_n];
	if (*arg == '-') {
	    cp = arg + 1;
	    if (index(seen, *cp)) {
		(void) fprintf(stderr, "%s: duplicate switch: %s\n", my_name, arg);
		err_flag++;
		continue;
	    }
	    seen[strlen(seen)] = *cp;
	    switch (*cp++) {
		case 'c':
		    /* Test configuration */
		    test_flag++;
		    trace_flags_save = trace_flags = TR_GEN | TR_KRT | TR_CONFIG | TR_NOSTAMP;
		    break;
		case 'n':
		    /* Don't install in kernel */
		    install = FALSE;
		    break;
		case 't':
		    /* Set trace flags */
		    if (!(trace_flags_save = trace_args(cp))) {
			err_flag++;
		    }
		    break;
		case 'f':
		    /* Specify config file */
		    ap = arg + 2;
		    if (*ap == (char) 0) {
			if (((arg_n + 1) < argc) && (*argv[arg_n + 1] != '-')) {
			    ap = argv[++arg_n];
			}
		    }
		    if (*ap == (char) 0) {
			(void) fprintf(stderr, "%s: missing argument for switch: %s\n", my_name, arg);
			err_flag++;
			break;
		    }
		    EGPINITFILE = ap;
		    conf_file_specified = TRUE;
		    break;
		default:
		    (void) fprintf(stderr, "%s: invalid switch: %s\n", my_name, arg);
		    err_flag++;
	    }
	} else if (!trace_file) {
	    trace_file = arg;
	} else {
	    (void) fprintf(stderr, "%s: extraneous information on command line: %s\n", my_name, arg);
	    err_flag++;
	}
    }
    if (err_flag) {
	(void) fprintf(stderr, "Usage: %s [-c] [-n] [-t[flags]] [-f config-file] [trace-file]\n", my_name);
    }
    return (err_flag);
}

#else				/* vax11c */
int
 parse_args(argc, argv);
int argc;
char **argv;

{
    int i;

    for (i = 1; i < argc; i++) {
	if (strcasecmp(argv[i], "bootfile") == 0) {
	    if (i >= argc) {
		(void) printf("ERROR: No GATED boot file specified!\n");
		return (1);
	    }
	    i++;
	    EGPINITFILE = argv[i];
	    continue;
	}
	if (strcasecmp(argv[i], "trace") == 0) {
	    trace_flags = TR_INT | TR_EXT | TR_RT | TR_EGP;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-all") == 0) {
	    trace_flags = TR_INT | TR_EXT | TR_RT | TR_EGP |
		TR_UPDATE | TR_RIP | TR_HELLO;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-internal-errors") == 0) {
	    trace_flags |= TR_INT;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-external-changes") == 0) {
	    trace_flags |= TR_EXT;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-routing-changes") == 0) {
	    trace_flags |= TR_RT;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-packets") == 0) {
	    trace_flags |= TR_EGP;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-egp-updates") == 0) {
	    trace_flags |= TR_UPDATE;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-rip-updates") == 0) {
	    trace_flags |= TR_RIP;
	    continue;
	}
	if (strcasecmp(argv[i], "trace-hello-updates") == 0) {
	    trace_flags |= TR_HELLO;
	    continue;
	}
#if	defined(AGENT_SNMP)
	if (strcasecmp(argv[i], "trace-snmp") == 0) {
	    trace_flags |= TR_SNMP;
	    continue;
	}
#endif				/* defined(AGENT_SNMP) */
	(void) printf("GATED bad arg \"%s\"\n", argv[i]);
	return (1);
    }
    return (0);
}

#endif				/* vax11c */

#ifndef vax11c
int
main(argc, argv)
#else				/* vax11c */
gw_main(argc, argv)
#endif				/* vax11c */
int argc;
char *argv[];
{
#ifdef vax11c
    int i;

#endif				/* vax11c */
    char *cp;
    FILE *fp;

    getod();				/* start time */

    if (!(my_hostname = (char *) calloc(MAXHOSTNAMELENGTH + 1, sizeof(char)))) {
	trace(TR_ALL, LOG_ERR, "main: calloc: %m");
	quit(errno);
    }
    if (gethostname(my_hostname, MAXHOSTNAMELENGTH + 1)) {
	trace(TR_ALL, LOG_ERR, "main: gethostname: %m");
	quit(errno);
    }
    /* check arguments for turning on tracing and a trace file */

    my_name = argv[0];
    if (cp = (char *) rindex(my_name, '/')) {
	my_name = cp + 1;
    }
    trace_flags = trace_flags_save = 0;
    if (parse_args(argc, argv)) {
	quit(0);
    }

    task_getpaths();

#ifndef vax11c
    if (!test_flag) {
	if (getuid()) {
	    fprintf(stderr, "%s: must be root\n",
		    my_name);
	    exit(2);
	}
	
	if (!trace_flags_save || trace_file) {
	    /* This code rearranged after reading "Unix Network Programming" */
	    /* by W. Richard Stevens */
	    int t;

	    /*
	     * On System V if we were started by init from /etc/inittab there
	     * is no need to detach.
	     * There is some ambiguity here if we were orphaned by another
	     * process.
	     */
	    if (getppid() != 1) {
		/* Ignore terminal signals */
#ifdef	SIGTTOU
		signal(SIGTTOU, SIG_IGN);
#endif	/* SIGTTOU */
#ifdef	SIGTTIN
		signal(SIGTTIN, SIG_IGN);
#endif	/* SIGTTIN */
#ifdef	SIGTSTP
		signal(SIGTSTP, SIG_IGN);
#endif	/* SIGTSTP */

		switch (fork()) {
		case 0:
		    /* Child */
		    break;

		case -1:
		    /* Error */
		    perror("main: fork");
		    exit(1);

		default:
		    /* Parent */
		    exit(0);
		}

		/* Remove our association with a controling tty */
#ifdef	USE_SETPGRP
		t = setpgrp();
		if (t < 0) {
		    perror("main: setpgrp");
		    exit(1);
		}

		signal(SIGHUP, SIG_IGN);

		/* Fork again so we are not a process group leader */
		switch (fork()) {
		case 0:
		    /* Child */
		    break;

		case -1:
		    /* Error */
		    perror("main: fork");
		    exit(1);

		default:
		    /* Parent */
		    exit(0);
		}
#else	/* USE_SETPGRP */
		t = setpgrp(0, getpid());
		if (t < 0) {
		    perror("main: setpgrp");
		    exit(1);
		}


		t = open("/dev/tty", O_RDWR, 0);
		if (t >= 0) {
		    if (ioctl(t, TIOCNOTTY, (char *) 0) < 0) {
			perror("main: ioctl(TIOCNOTTY)");
			exit(1);
		    }
		    (void) close(t);
		}
#endif	/* USE_SETPGRP */
	    }

	    /* Close all open files */
	    t = getdtablesize();
	    do {
		(void) close(--t);
	    } while (t);

	    /* Reset umask */
	    umask(0);

	}
    }
    my_pid = my_mpid = getpid();

#if	defined(LOG_DAEMON)
    openlog(my_name, LOG_PID | LOG_CONS | LOG_NDELAY, LOG_FACILITY);
    (void) setlogmask(LOG_UPTO(LOG_NOTICE));
#else				/* defined(LOG_DAEMON) */
    openlog(my_name, LOG_PID);
#endif				/* defined(LOG_DAEMON) */

    if (trace_flags_save) {
	(void) trace_on(trace_file, TRUE);
    }
#endif				/* vax11c */

    trace(TR_ALL, 0, NULL);
    trace(TR_ALL, LOG_NOTICE, "Start %s[%d] version %s built %s", my_name, my_pid, version, build_date);
    trace(TR_ALL, 0, NULL);

/* open initialization file */
    no_config_file = FALSE;

    rt_init();				/* initialize route hash tables */

    control_init();			/* initialize control info tables */

    if_init();				/* initialize interface tables */

    krt_init();				/* Read kernel routing table */

    /*
     * initialize the hello_default net.
     */
    sockclear_in(&default_net);
    default_net.sin_addr.s_addr = htonl((u_long) DEFAULTNET);

    trace(TR_TASK, 0, NULL);
    trace(TR_TASK, 0, "main: Initializing protocols and tasks:");

    if (parse_parse(EGPINITFILE)) {	/* Read the config file */
	quit(0);
    }
#if	defined(PROTO_ICMP) && !defined(RTM_ADD)
    icmp_init();			/* Initialize to catch ICMP redirects */
#endif				/* defined(PROTO_ICMP) && !defined(RTM_ADD) */
#ifdef	PROTO_EGP
    egp_init();
#endif				/* PROTO_EGP */
#ifdef	PROTO_BGP
    bgp_init();
#endif				/* PROTO_BGP */
#ifdef	PROTO_RIP
    rip_init();
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
    hello_init();
#endif				/* PROTO_HELLO */
#ifdef	AGENT_SNMP
    snmp_init();
#endif				/* AGENT_SNMP */

    (void) timer_create((task *) 0,
			0,
			"Time.Mark",
			0,
			(time_t) TIME_MARK,
			trace_mark);

    trace(TR_TASK, 0, NULL);

    if_rtinit();			/* initialize interior routes for direct nets */

    if (no_config_file
#ifdef	PROTO_RIP
	&& !rip_supplier
#endif				/* PROTO_RIP */
	&& rt_locate(RTS_NETROUTE, (sockaddr_un *) & default_net, RTPROTO_KERNEL)) {
	trace(TR_ALL, 0, NULL);
	trace(TR_ALL, LOG_NOTICE, "No config file, one interface and a default route, gated exiting");
	trace(TR_ALL, 0, NULL);
	quit(0);
    }
    trace(TR_ALL, 0, NULL);
    trace(TR_ALL, 0, "***Routes are %sbeing installed in kernel", install ? "" : "not ");
    trace(TR_ALL, 0, NULL);

    if (test_flag) {
	/* Just testing configuration */
	trace_dump(TRUE);
	quit(0);
    }
#ifndef vax11c
    fp = fopen(PIDFILE, "w");
    if (fp != NULL) {
	(void) fprintf(fp, "%d\n", my_pid);
	(void) fclose(fp);
    }
    fp = fopen(VERSIONFILE, "w");
    if (fp != NULL) {
	(void) fprintf(fp, "%s version %s built %s\n\tpid %d, started %s",
		       my_name, version, build_date, my_pid, time_full);
	(void) fclose(fp);
    }
    task_init();
#endif				/* vax11c */

    getod();
    srandom((unsigned) time_sec);

    trace(TR_INT, 0, NULL);
    trace(TR_INT, LOG_NOTICE, "Commence routing updates");
    trace(TR_INT, 0, NULL);

    task_main();

    return (0);				/* To keep the complier happy */
}


/*
 * get time of day in seconds and as an ASCII string.
 * Called at each interrupt and saved in external variables.
 */

void
getod()
{
    struct timeval tp;
    struct timezone tzp;
    struct tm *tm;

    if (gettimeofday(&tp, &tzp)) {
	trace(TR_ALL, LOG_ERR, "getod: gettimeofday: %m");
    }
    time_sec = tp.tv_sec;
    tm = localtime(&tp.tv_sec);
    (void) sprintf(time_string, "%d/%d %02d:%02d:%02d\n",
		   tm->tm_mon + 1, tm->tm_mday,
		   tm->tm_hour, tm->tm_min, tm->tm_sec);
    (void) strcpy(time_full, (char *) ctime(&tp.tv_sec));
}


/* exit gated */

void
quit(code)
int code;
{
    rt_default_reset();
    getod();
    trace(TR_ALL, 0, NULL);
    switch (code) {
	case 0:
	case EDESTADDRREQ:
	    tracef("Exit %s[%d] version %s", my_name, my_pid, version);
	    if (code) {
		errno = code;
		tracef(": %m");
	    }
	    trace(TR_ALL, LOG_NOTICE, NULL);
	    trace(TR_ALL, 0, NULL);
	    trace_close();
	    break;
	default:
	    errno = code;
	    trace(TR_ALL, LOG_NOTICE, "Abort %s[%d] version %s: %m", my_name, my_pid, version);
	    trace(TR_ALL, 0, NULL);
	    trace_close();
	    chdir(DUMPDIR);
	    abort();
	    break;
    };
    exit(code);
}
