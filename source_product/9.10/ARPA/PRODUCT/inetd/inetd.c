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
*/

#ifndef lint
static char rcsid[] = "$Header: inetd.c,v 1.16.109.9 94/11/17 15:28:20 mike Exp $";
#endif /* ~lint */

#define _DEFINE
#include "inetd.h"

#define USAGE \
"Usage: inetd [-l]\n       inetd -c\n       inetd -k\n"

/*
** MAIN
**
**     Parse the command line, fork into the background, and accept
**     connections.
*/


main(argc,argv)
int argc;
char **argv;

{
    register struct servtab *sep;
    register struct passwd *pwd;
    char buf[50];
    int pid, i, dofork, secflag, isrunning;
    int err = 0;		
    extern char **environ;
    char **ev, **path;
    time_t clock;
    char *ctime();

#ifdef SETPROCTITLE
    Argv[0] = (char *)malloc(strlen(argv[0])+1);
    strcpy(Argv[0], argv[0]);
#endif				/* SETPROCTITLE */

    /* Cleanup environment inherited by servers */
    cleanenv(&environ,
	     "LANG", "TZ", "LC_CTYPE", "LC_COLLATE",
	     "LC_TIME", "LC_NUMERIC", "LC_MONETARY", 0);

    /* Blow away the PATH, even though cleanenv gives us something safe */
    for (path = NULL, ev = environ; *ev; ev++) 
	if (strncmp(*ev, "PATH=", 5) == 0) {
	    path = ev;
	    (void) memset(*path, 0, strlen(*path));
	    *path = (char *)0;
	    break;
	}
    if (path != NULL)
	do *path++ = *++ev; while (*ev);

    /* Program can only run as root, if not root exit */
    if (geteuid() != 0) {
	fprintf(stderr,
		"inetd: This program requires super user privileges\n");
	exit(1);
    }

    /* initialize global variables */
    initialize();

    /* initialize signal handlers */
    initsignals();

    /* Pare the command line */
    argv++; argc--;
    while (argc > 0) {
	if (argv[0][0] == '-') {
	    switch (argv[0][1]) {
		case 'c':	/* reconfigure the inetd */
		    argv++; argc = 0;
		    sendsig(SIGHUP);
		    break;
		case 'k':	/* kill the inetd */
		    argv++; argc = 0;
		    sendsig(SIGTERM);
		    break;
		case 'l':	/* log usage information */
		case 'L':	/* toggle inetd logging */
		    argv++; argc = 0;
		    logflg = TRUE;
		    /* accept a LOGFILE argument for compatibility sake */
		    if (strncmp(argv[0],"-",1) && (argv[0] != NULL)) {
			fprintf(stderr, 
				"inetd: Warning: The logfile parameter is obsolete.\n");
		    }
		    break;
		default:
		    err++;
		    fprintf(stderr, "inetd: Unknown option %s\n",
			    argv[argc]);
		    argc--; argv++;
	    }
	}
	else {
	    err++;
	    argc--; argv++;
	}
    }

    /* Check the parsing results for errors */
    if (err || (argc > 0)) {
	fprintf(stderr, USAGE);
	exit(1);
    }

    /* Initialize the semaphore */
    isrunning = seminit();

    /* Abnormal failure initializing semaphore */
    if (isrunning < 0) {
	fprintf(stderr, "inetd: Cannot initialize semaphore\n");
	exit(1);
    }

    /* An inetd is already running */
    if (isrunning) {
	/* make -l compatible with -L */
	if (logflg) {
	    sendsig(SIGQUIT);
	    exit(0);
	}
	fprintf(stderr, "An inetd is already running\n");
	fprintf(stderr, USAGE);
	exit(-1);
    }

    /* fork into the background and dissociate from the terminal */
    if (debug == 0) {
	if (fork())
	    exit(0);
	for (i = 0; i < getnumfds(); i++)
	    (void) close(i);
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	setpgrp();
    }

    /* enable syslogging */
    openlog("inetd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);

    /* Log connection logging status */
    if (logflg)
	syslog(LOG_INFO, "Connection logging enabled");


    /* Clear the signal mask */
    sigsetmask(0L);

    /* 
    ** Store the pid of inetd in the semaphore structure.
    */
    semsetpid();


    /* SELECT AND ACCEPT CONNECTIONS */

    /*
    ** Select and accept connections, perform a security
    ** check and execute the corresponding server.
    */
    for (;;) {
	int ctrl, n;
	fd_set readable;
	struct sockaddr_in his_addr;
	int hisaddrlen = sizeof (his_addr);

	while (rereadflg) {
	    long omask;
	    /*
	     ** SIGHUP may already be blocked;
	     ** then again, it may not be.
	     */
	    omask = sigblock(SIGBLOCK);
	    syslog(LOG_INFO, "R%seading configuration",
		   initconf ? "" : "er");
	    initconf = FALSE;
	    rereadflg = FALSE;
	    badfile = TRUE;
	    if (config() < 0) {
		if (badfile)
		    syslog(LOG_ERR,
			   "%s: Unusable configuration file",
			   CONFIG_FILE);
		else
		    syslog(LOG_ERR, "Cannot configure inetd");
	    }
	    else
		syslog(LOG_INFO, "Configuration complete");
	    sigsetmask(omask);
	}

	if (nsock == 0) {
	    sigpause(0);
	    continue;
	}
	readable = allsock;
	if ((n = select(maxsock + 1, &readable, (fd_set *)0,
			(fd_set *)0, (struct timeval *)0)) <= 0) {
	    if (n < 0 && errno != EINTR)
		syslog(LOG_WARNING, "select: %m\n");
	    sleep(1);
	    continue;
	}

	for (sep = servtab; n && sep; sep = sep->se_next)
	if (sep->se_fd != -1 && FD_ISSET(sep->se_fd, &readable)) {
	    n--;
	    if (!sep->se_wait && sep->se_socktype == SOCK_STREAM) {
		ctrl = accept(sep->se_fd, &his_addr, &hisaddrlen);
		if (ctrl < 0) {
		    if (errno == EINTR)
			continue;
		    syslog(LOG_WARNING, "%s: accept: %m",
			   servicename(sep));
		    continue;
		}
	    } else {
		ctrl = sep->se_fd;
		/* see who is sending the datagram */
		recvfrom(ctrl, buf, sizeof(buf), MSG_PEEK, &his_addr,
			 &hisaddrlen);
	    }
	    /*
	    ** If logging or using the security file,
	    ** get the remote IP address and, if logging,
	    ** the remote name
	    */
	    if ( logflg || sep->se_bi != NULL)
		getremotenames(&his_addr, logflg, &remotehp, 
			       &remotehost, &remoteaddr);

	    /* SECURITY CHECK */
	    if ((secflag = 
		inetd_secure(sep->se_isrpc ? sep->se_rpc.name : sep->se_service,
		    &his_addr)) == 0 ) {
		if (logflg) {
			clock = time((time_t *) 0);
			syslog(LOG_NOTICE,
			       "%s: Access denied for %s (%s) at %.24s",
			       servicename(sep), remotehost, remoteaddr,
			       ctime(&clock));
		}
		if (sep->se_socktype != SOCK_STREAM)
		    recv(ctrl, buf, sizeof (buf), 0);
		if (!sep->se_wait && sep->se_socktype == SOCK_STREAM)
		    (void) close(ctrl);
		continue;
	    }

	    if (secflag == BADSECURITY) {
		syslog(LOG_ALERT,
		    "Problem with %s: cannot start any servers",
		    SECURITY_FILE);
		if (sep->se_socktype != SOCK_STREAM)
		    recv(ctrl, buf, sizeof (buf), 0);
		if (!sep->se_wait && sep->se_socktype == SOCK_STREAM)
		    (void) close(ctrl);
		continue;
	    }

	    if (secflag == OUTOFMEMORY) 
		killprocess(0);

	    (void) sigblock(SIGBLOCK);
	    pid = 0;
	    dofork = (sep->se_bi == 0 || sep->se_bi->bi_fork);
	    if (dofork) {
		if (sep->se_socktype != SOCK_STREAM &&
		    !(!strncmp(sep->se_service,"tftp",4)     ||
		      !strncmp(sep->se_service,"bootps",6)   ||
		      !strncmp(sep->se_service,"rpc",3))) {
		    if (sep->se_count++ == 0)
			(void)gettimeofday(&sep->se_time,
					   (struct timezone *)0);
		    else if (sep->se_count >= TOOMANY) {
			struct timeval now;
			
			(void)gettimeofday(&now, (struct timezone *)0);
			if (now.tv_sec - sep->se_time.tv_sec > CNT_INTVL) {
			    sep->se_time = now;
			    sep->se_count = 1;
			} else {
			    syslog(LOG_ERR,
				   "%s: Server failing (looping), service terminated",
				   servicename(sep));
			    if (sep->se_isrpc)
				unregister(sep);
			    delserv(sep);
			    sep->se_count = 0;
			    sigsetmask(0L);
			    if (!timingout) {
				timingout = 1;
				alarm(RETRYTIME);
			    }
			    continue;
			}
		    }
		}
		pid = fork();
	    }
	    if (pid < 0) {
		if (!sep->se_wait && sep->se_socktype == SOCK_STREAM)
		    (void) close(ctrl);
		sigsetmask(0L);
		sleep(1);
		continue;
	    }
	    if (pid && sep->se_wait) {
		sep->se_wait = pid;
		FD_CLR(sep->se_fd, &allsock);
		nsock--;
	    }
	    if (pid == 0) {
		if (dofork) {
			/* make child session leader */
			setsid();
			resetsignals();
			sigsetmask(0L);
		}

		if (logflg) {
		    clock = time((time_t *) 0);
		    syslog(LOG_INFO, 
			   "%s: Connection from %s (%s) at %.24s",
			   servicename(sep), remotehost,
			   remoteaddr, ctime(&clock));
		}
		if (dofork) {
		    closelog();
		    for (i = getnumfds(); --i > 2; )
			if (i != ctrl)
			    (void) close(i);
		}
		if (sep->se_bi) {
		    (*sep->se_bi->bi_fn)(ctrl, sep);
		}
		else {
		    int pgid = -getpid();

		    (void) dup2(ctrl, 0);
		    (void) close(ctrl);
		    (void) dup2(0, 1);
		    (void) dup2(0, 2);

		    /* make child socket process group leader */
		    ioctl(0, SIOCSPGRP, (char *)&pgid);
		    {
			if ((pwd = getpwnam(sep->se_user)) == NULL) {
			    syslog(LOG_ERR,
				   "getpwnam: %s: No such user",
				   sep->se_user);
			    if (sep->se_socktype != SOCK_STREAM)
				recv(0, buf, sizeof (buf), 0);
			    _exit(1);
			}
			if (pwd->pw_uid) {
			    (void) initgroups((uid_t)pwd->pw_name,
					      (gid_t)pwd->pw_gid);
			    (void) setgid((gid_t)pwd->pw_gid);
			    (void) setuid((uid_t)pwd->pw_uid);
			}
		    }
		    if (sep->se_argv[0] != NULL) {
			if (!strcmp(sep->se_argv[0], "%A")) {
			    char addrbuf[32];
			    sprintf(addrbuf, "%s.%d", 
				    inet_ntoa(his_addr.sin_addr.s_addr),
				    ntohs(his_addr.sin_port));
			    execl(sep->se_server,
				  rindex(sep->se_server, '/')+1,
				  sep->se_socktype == SOCK_DGRAM
				  ? (char *)0 : addrbuf, (char *)0);
			} else
			    execv(sep->se_server, sep->se_argv);
		    } else
			execv(sep->se_server, sep->se_argv);
		    if (sep->se_socktype != SOCK_STREAM)
			recv(0, buf, sizeof (buf), 0);
		    syslog(LOG_ERR, "execv %s: %m", sep->se_server);
		    _exit(1);
		}
	    }
	    sigsetmask(0L);
	    if (!sep->se_wait && sep->se_socktype == SOCK_STREAM)
	    {
		int return_code;
		do 
		    return_code = close(ctrl);
                while (return_code < 0 && errno == EINTR);
            }
	}	
    }
	
    /*NOTREACHED*/
    killprocess(0);
}

/*
** SENDSIG
**
**    Send a signal to the running inetd.  This routine assumes
**    that inetd is running.
**
**    This routine is used to implement the -c, -k, and -L options.
*/

sendsig(sig)
    int sig;
{
    int pid, dkill;

    if (semgetid(0) == -1) {
	/* If the semaphore does not exist */
	if (errno == ENOENT) {
	    if (sig != SIGTERM)
		fprintf(stderr,
			"inetd: There is no inetd running\n");
	    exit(1);
	}
	perror("inetd: semget");
	exit(1);
    }
    else {
	pid = semgetpid();
	dkill = kill(pid, sig);
	/*
	 ** If there is no process with the PID 
	 ** used in the kill 
	 */
	if ((dkill == -1) && (errno== ESRCH)) {
	    if (sig != SIGTERM) 
		fprintf(stderr,"inetd: Inetd not found\n");
	    else 
		semclose();
	    exit(1);
	}
    }
    exit(0);
}


/*
** INITIALIZE
**
**    Initialize global variables.
*/

initialize()
{
    initconf = TRUE;
    maxservers = (int)sysconf(_SC_OPEN_MAX) - OTHERDESCRIPTORS;
    remotehost = NULL;
    remoteaddr = NULL;
    nservers = 0;
    /*
    debug = TRUE;
    */
    logflg = FALSE;
    rereadflg = TRUE;
    badfile = TRUE;
    portmap_up = TRUE;
    servtab = NULL;
    FD_ZERO(&allsock);

#ifdef AUDIT
    /*
    **  Turn on HP Auditing
    */
    setaudproc(AUD_PROC);
#endif AUDIT

}
