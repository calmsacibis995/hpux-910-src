/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** Portions are:
**
** Copyright (c) 1983 Regents of the University of California.
** All rights reserved.  The Berkeley software License Agreement
** specifies the terms and conditions for redistribution.
**
** from 1.38 88/02/07 SMI; from UCB 5.14 (Berkeley) 1/23/89
**
**
** SIGNAL.C
**
** This file contains the routines which establish inetd's signal handlers,
** reset signal handlers to their defaults, and handle signals.
** detect the presence of an inetd that is already running and to find
** the pid of a running inetd so that it can be sent signals.
*/

#ifndef lint
static char rcsid[] = "$Header: signal.c,v 1.5.109.3 94/11/17 15:39:55 mike Exp $";
#endif /* ~lint */

#include "inetd.h"

/*
**
** INITSIGNALS
**
**    Set up initial signal disposition for ALL signals. We don't let
**    any old signal kill us by accident -- we make sure to clean up
**    if possible
*/

initsignals()
{ 
    struct sigvec vec, ovec;
    int sign;
    void reapchild(), togglelog(), reread(), dumpservers(), dumpvars();

    /*
    ** Setup the common part of all the vectors.
    */
    vec.sv_mask = 0;
    vec.sv_onstack = 0;

    for (sign = 1; sign < NSIG; sign++) {
	switch (sign) {
	    /*
	    **	SIGHUP is sent by "inetd -c"
	    */
	    case SIGHUP:
	        vec.sv_mask = SIGBLOCK;
	        vec.sv_handler = reread;
		break;
	    /*
	    ** SIGQUIT is sent by "inetd -L"
	    */
	    case SIGQUIT:
	        vec.sv_mask = SIGBLOCK;
		vec.sv_handler = togglelog;
		break;
	    case SIGCLD:
	        vec.sv_mask = SIGBLOCK;
		vec.sv_handler = reapchild;
		break;
	    case SIGALRM:
	        vec.sv_mask = SIGBLOCK;
		vec.sv_handler = retry;
		break;
		/*
		** leave these guys alone ...
	        */
	    case SIGPWR:
	    case SIGIO:
	    case SIGWINDOW:
	    case SIGTSTP:
	    case SIGCONT:
	    case SIGTTIN:
	    case SIGTTOU:
	    case SIGURG:
	        vec.sv_mask = 0;
		vec.sv_handler = SIG_DFL;
		break;
#ifdef DEBUG
		/*
		** Setup some useful signals for debugging.
		*/
	    case SIGUSR1:
		vec.sv_mask = SIGBLOCK | sigmask(SIGUSR1) | sigmask(SIGUSR2);
		vec.sv_handler = dumpservers;
		break;
	    case SIGUSR2:
		vec.sv_mask = SIGBLOCK | sigmask(SIGUSR1) | sigmask(SIGUSR2);
		vec.sv_handler = dumpvars;
		break;
#endif /* DEBUG */
		/*
		**	all other signals: die gracefully
		*/
	    default:
	        vec.sv_mask = 0;
		vec.sv_handler = killprocess;
	}
	sigvector(sign, &vec, &ovec);
    }
}

/*
** RESETSIGNALS
**
**    Setup the default signal handler for all signals.
*/
resetsignals()
{ 
    struct sigvec vec, ovec;
    int sign;
    void reapchild(), togglelog(), reread();


    /*
    ** Setup the common part of all the vectors.
    */
    vec.sv_mask = 0;
    vec.sv_onstack = 0;
    vec.sv_handler = SIG_DFL;
    for (sign = 1; sign < NSIG; sign++)
	sigvector(sign, &vec, &ovec);
}



/*
** REAPCHILD
**
**    Signal handler for SIGCLD.  In case of the death of a child
**    process, that is, completion of remote service, we need to
**    decrement count of the number of remote services running.
*/

void
reapchild()
{
    int status, pid;
    register struct servtab *sep;
    struct sigvec vec;

    for (;;) {
	pid = wait3(&status, WNOHANG, (struct rusage *)0);
	if (pid <= 0)
	    break;
	for (sep = servtab; sep; sep = sep->se_next)
	    if (sep->se_wait == pid) {
		if (WIFEXITED(status) && WEXITSTATUS(status)) {
		    syslog(LOG_INFO, 
			   "%s: Exit status %d",
			   servicename(sep),
			   WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
		    syslog(LOG_INFO, 
			   "%s: Died on signal %d",
			   servicename(sep),
			   WTERMSIG(status));
		}

		if (!sep->se_isrpc || (sep->se_checked < 3))
		/* 
		   Resume monitoring the sep->se_fd socket,
		   unless this is an RPC server that has been
		   reconfigured (in which case monitoring is
		   already in place).
		*/
		{
			FD_SET(sep->se_fd, &allsock);
			nsock++;
		}

		if (sep->se_endwait) {
		    /*
		    ** last reconfigure changed wait
		    ** to nowait
		    */
		    sep->se_wait = sep->se_endwait = 0;
		}
		else {
		    sep->se_wait = 1;
		}
	    }
    }
    /*
    ** Reinstating the SIGCLD handler so that if there are
    ** multiple zombies present it will check for them.
    */
    vec.sv_mask = SIGBLOCK;
    vec.sv_handler = reapchild;
    vec.sv_onstack = 0;
    sigvector(SIGCLD, &vec, NULL);
}


/*
** RETRY
**
**    Signal handler for SIGALRM.
**
**    Try again to establish sockets on which to listen for requests
**    for non-RPC-based services (if the attempt failed before, it was
**    either because a socket could not be created, or more likely
**    because the socket could not be bound to the service's address -
**    probably because there was already a daemon out there with a
**    socket bound to that address).
*/

void
retry()
{
    register struct servtab *sep;

    timingout = 0;
    for (sep = servtab; sep; sep = sep->se_next) {
	if (sep->se_fd == -1 && !sep->se_isrpc) {
	    if (nservers < maxservers)
		if (setup(sep) >= 0)
		    syslog(LOG_INFO, "%s: Service enabled", servicename(sep));
	}
    }
}

/*
** KILLPROCESS
**
**    Signal handler for SIGTERM, SIGBUS and SIGSEGV, and other
**    terminating signals.
*/

void
killprocess(sig)
{
    struct servtab *sep;
    int mypid = getpid(), dpid;

    /*
    ** Log our death.
    */
    if (sig != 0) {
	syslog(LOG_INFO, "Going down on signal %d", sig);
    }
 
    dpid = semgetpid();

    /*
    ** If the real inetd is going down, perform some cleanup.
    ** We don't want to do this if a signal is received after
    ** inetd has forked.
    */
    if (dpid == mypid) {
	/*
	** Cleanup our semaphore.
	*/
	semclose();


	/*
	** Gracefully terminate each service.
	*/
	for (sep = servtab; sep; sep = sep->se_next)
		{
			if (sep->se_isrpc)
				unregister(sep);
			delserv(sep);
		}

    }

    /*
    ** Reset the signal handlers
    */
    resetsignals();

    /*
    ** send ourselves the signal we received to get here ...
    */
    sigsetmask(sigsetmask(~0L) & ~sigmask(sig));
    kill(mypid, sig);

    /*
     ** if we didn't die there, then we simply _exit() ...
     */
    _exit(1);

}


/*
** REREAD
**
**	Signal handler for SIGHUP.  Causes inetd to reread it's configuration.
*/

void
reread()
{
    rereadflg = TRUE;
}

/*
** TOGGLELOG
**
**    Signal handler for SIGQUIT.  Will turn on and off logging of
**    connections.
*/

void
togglelog()
{
    logflg = (logflg == FALSE) ? TRUE : FALSE;
    syslog(LOG_INFO, "Connection logging %s", 
	   logflg ? "enabled" : "disabled");
}


#ifdef DEBUG

/*
** DUMPSERVERS
**
**     Handler for SIGUSR1.  Logs the contents of the server table to
**     the system log.
*/

char *socktypes[] = { "*undefined*", "stream", "dgram", "raw", "rdm",
		      "seqpacket", "ns_request", "ns_reply", "ns_dest" };

#define socktype(s) ((s < 0 || s > 8) ? socktypes[0] : socktypes[s])

void
dumpservers()
{
    register struct servtab *sep;

    for (sep = servtab;  sep != NULL;  sep = sep->se_next) {
	syslog(LOG_DEBUG, "-----------------------");
	syslog(LOG_DEBUG, "%s", servicename(sep));
	syslog(LOG_DEBUG, "         type=%s", socktype(sep->se_socktype));
	syslog(LOG_DEBUG, "         wait=%d, endwait=%d", sep->se_wait,
	       sep->se_endwait);
	syslog(LOG_DEBUG, "         checked=%d", sep->se_checked);
	if (sep->se_isrpc) {
	    syslog(LOG_DEBUG, "         prog=%d", sep->se_rpc.prog);
	    syslog(LOG_DEBUG, "         lowvers=%d", sep->se_rpc.lowvers);
	    syslog(LOG_DEBUG, "         highvers=%d", sep->se_rpc.highvers);
	    syslog(LOG_DEBUG, "         name=%s", sep->se_rpc.name);
	} else {
	    syslog(LOG_DEBUG, "         port=%d", sep->se_ctrladdr.sin_port);
	}
	syslog(LOG_DEBUG, "         fd=%d", sep->se_fd);
	if (sep->se_bi != NULL)
	    syslog(LOG_DEBUG, "         builtin=%s", sep->se_bi->bi_service);
	syslog(LOG_DEBUG, "         server=%s", sep->se_server);
	syslog(LOG_DEBUG, "         count=%d", sep->se_count);
    }
    syslog(LOG_DEBUG, "-----------------------");
    
}

/*
** DUMPVARS
**
**    Handler for SIGUSR2.  Log some global variables to the system log.
*/
void
dumpvars()
{
    syslog(LOG_DEBUG, "-----------------------");
    syslog(LOG_DEBUG, "nservers=%d", nservers);
    syslog(LOG_DEBUG, "maxservers=%d", maxservers);
    syslog(LOG_DEBUG, "badfile=%d", badfile);
    syslog(LOG_DEBUG, "initconf=%d", initconf);
    syslog(LOG_DEBUG, "logflg=%d", logflg);
    syslog(LOG_DEBUG, "rereadflg=%d", rereadflg);
    syslog(LOG_DEBUG, "-----------------------");
}

#endif /* DEBUG */


