/* $Header: ntpd.c,v 1.2.109.3 94/11/09 10:49:46 mike Exp $
 * ntpd.c - main program for the fixed point NTP daemon
 */
# include <stdio.h>
# include <string.h>
# include <signal.h>
# include <errno.h>
# ifdef        _AUX_SOURCE
# 	include <sys/types.h>
# endif	/* _AUX_SOURCE */
# include <sys/param.h>
# include <sys/signal.h>
# include <sys/ioctl.h>
# include <sys/time.h>
# include <sys/resource.h>
# if defined(HPUX)
# 	include <sys/lock.h>
# 	include <sys/rtprio.h>
# endif	/* defined (HPUX) */
# ifdef LOCK_PROCESS
# 	include <sys/lock.h>
# endif	/* LOCK_PROCESS */
# ifdef SOLARIS
# 	include <sys/termios.h>
# endif	/* SOLARIS */

# include "ntpd.h"

/*
 * Mask for blocking SIGIO and SIGALRM
 */
# define	BLOCKSIGMASK	(sigmask(SIGIO)|sigmask(SIGALRM))

/*
 * Signals we catch for debugging.  If not debugging we ignore them.
 */
# define	MOREDEBUGSIG	SIGUSR1
# define	LESSDEBUGSIG	SIGUSR2

/*
 * Signals which terminate us gracefully.
 */
# define	SIGDIE1		SIGHUP
# define	SIGDIE2		SIGINT
# define	SIGDIE3		SIGQUIT
# define	SIGDIE4		SIGTERM

/*
 * Scheduling priority we run at
 */
# define	NTPD_PRIO	(-12)

/*
 * Debugging flag
 */
int     debug;

/*
 * Initializing flag.  All async routines watch this and only do their
 * thing when it is clear.
 */
int     initializing;

/*
 * Version declaration
 */
extern char    *Version;

/*
 * Alarm flag.  Imported from timer module
 */
extern int  alarm_flag;

# ifdef	SIGDIE1
static RETSIGTYPE   finish P ((int));
# endif				/* SIGDIE1 */

# ifdef	DEBUG
static RETSIGTYPE   moredebug P ((int));
static RETSIGTYPE   lessdebug P ((int));
# endif				/* DEBUG */

/*
 * Main program.  Initialize us, disconnect us from the tty if necessary,
 * and loop waiting for I/O and/or timer expiries.
 */
void
main (argc, argv)
int     argc;
char   *argv[];
{
    char   *cp;
    int     was_alarmed;
    struct recvbuf *rbuflist;
    struct recvbuf *rbuf;

    initializing = 1;		/* mark that we are
				   initializing */
    debug = 0;			/* no debugging by default 
				*/

    getstartup (argc, argv);	/* startup configuration,
				   may set debug */

# ifndef NODETACH
    /* 
     * Detach us from the terminal.  May need an #ifndef GIZMO.
     */
# 	ifdef	DEBUG
    if (!debug)
        {
# 	endif	/* DEBUG */
# 	undef BSD19906
# 	if defined(BSD)&&!defined(sun)
# 		if (BSD >= 199006 && !defined(i386))
# 			define  BSD19906
# 		endif	/* !defined (i386)) */
# 	endif	/* defined (BSD)&&!defined(sun) */
# 	if defined(BSD19906)
	daemon (0, 0);
# 	else	/* ! defined (BSD19906) */
	if (fork ())
	    exit (0);

	    {
	    unsigned long   s;
# 		ifdef SOLARIS
	    struct rlimit   rlp;
	    rlim_t  max_fd;

	    getrlimit (RLIMIT_NOFILE, &rlp);
	    max_fd = rlp.rlim_cur;
# 		else	/* ! SOLARIS */
	    int     max_fd;
	    max_fd = getdtablesize ();
# 		endif	/* SOLARIS */
	    for (s = 0; s < max_fd; s++)
		(void)close (s);
	    (void)open ("/", 0);
	    (void)dup2 (0, 1);
	    (void)dup2 (0, 2);
# 		if defined(HPUX)
	    (void)setsid ();
# 		else	/* ! defined (HPUX) */
# 			if defined(SOLARIS) || defined(sgi)
	    (void)setpgrp ();
# 			else	/* ! defined (SOLARIS) || defined(sgi) */
	    (void)setpgrp (0, getpid ());
# 			endif	/* defined (SOLARIS) || defined(sgi) */
# 		endif	/* defined (HPUX) */
# 		if defined(HPUX)
	    if (fork ())
		exit (0);
# 		else	/* ! defined (HPUX) */
# 			ifdef apollo
/*
 * This breaks... the program fails to listen to any packets coming
 * in on the UDP socket.  So how do you break terminal affiliation?
 */
# 			else	/* ! apollo */
	        {
		int     fid;

		fid = open ("/dev/tty", 2);
		if (fid >= 0)
		    {
		    (void)ioctl (fid, (U_LONG)TIOCNOTTY,
			    (char  *)0);
		    (void)close (fid);
		    }
	        }
# 			endif	/* apollo */
# 		endif	/* defined (HPUX) */
	    }
# 	endif	/* defined (BSD19906) */
# 	ifdef	DEBUG
        }
# 	endif	/* DEBUG */
# endif				/* NODETACH */

    /* 
     * Logging.  This may actually work on the gizmo board.  Find a name
     * to log with by using the basename of argv[0]
     */
    cp = strrchr (argv[0], '/');
    if (cp == 0)
	cp = argv[0];
    else
	cp++;

# ifndef	LOG_DAEMON
    openlog (cp, LOG_PID);
# else	/* ! not LOG_DAEMON */

# 	ifndef	LOG_NTP
# 		define	LOG_NTP	LOG_DAEMON
# 	endif	/* not LOG_NTP */
    openlog (cp, LOG_PID | LOG_NDELAY, LOG_NTP);
# 	ifdef	DEBUG
    if (debug)
	setlogmask (LOG_UPTO (LOG_DEBUG));
    else
# 	endif				/* DEBUG */
	setlogmask (LOG_UPTO (LOG_DEBUG));
				/* @@@ was INFO */
# endif				/* LOG_DAEMON */

    syslog (LOG_INFO, Version);


# if defined(HPUX)
    /* 
     * Lock text into ram, set real time priority
     */
    if (plock (TXTLOCK) < 0)
	syslog (LOG_ERR, "plock() error: %m");
    if (rtprio (0, 120) < 0)
	syslog (LOG_ERR, "rtprio() error: %m");
# else	/* ! defined (HPUX) */
# 	if defined(PROCLOCK) && defined(LOCK_PROCESS)
    /* 
     * lock the process into memory
     */
    if (plock (PROCLOCK) < 0)
	syslog (LOG_ERR, "plock(): %m");
# 	endif	/* defined (PROCLOCK) && defined(LOCK_PROCESS) */
# 	if defined(NTPD_PRIO) && NTPD_PRIO != 0
    /* 
     * Set the priority.
     */
# 		if	defined(_AUX_SOURCE) || defined(SOLARIS)
    nice (NTPD_PRIO);
# 		else	/* ! defined (_AUX_SOURCE) || defined(SOLARIS) */
    (void)setpriority (PRIO_PROCESS, 0, NTPD_PRIO);
# 		endif	/* defined (_AUX_SOURCE) || defined(SOLARIS) */
# 	endif	/* defined (NTPD_PRIO) && NTPD_PRIO != 0 */
# endif	/* defined (HPUX) */

    /* 
     * Set up signals we pay attention to locally.
     */
# ifdef SIGDIE1
    (void)signal (SIGDIE1, finish);
# endif				/* SIGDIE1 */
# ifdef SIGDIE2
    (void)signal (SIGDIE2, finish);
# endif				/* SIGDIE2 */
# ifdef SIGDIE3
    (void)signal (SIGDIE3, finish);
# endif				/* SIGDIE3 */
# ifdef SIGDIE4
    (void)signal (SIGDIE4, finish);
# endif				/* SIGDIE4 */

# ifdef DEBUG
    (void)signal (MOREDEBUGSIG, moredebug);
    (void)signal (LESSDEBUGSIG, lessdebug);
# else	/* ! DEBUG */
    (void)signal (MOREDEBUGSIG, SIG_IGN);
    (void)signal (LESSDEBUGSIG, SIG_IGN);
# endif				/* DEBUG */

    /* 
     * Call the init_ routines to initialize the data structures.
     * Note that init_systime() may run a protocol to get a crude
     * estimate of the time as an NTP client when running on the
     * gizmo board.  It is important that this be run before
     * init_subs() since the latter uses the time of day to seed
     * the random number generator.  That is not the only
     * dependency between these, either, be real careful about
     * reordering.
     */
    init_auth ();
    init_util ();
    init_restrict ();
    init_mon ();
    init_systime ();
    init_timer ();
    init_lib ();
    init_random ();
    init_request ();
    init_control ();
    init_leap ();
    init_peer ();
# ifdef REFCLOCK
    init_refclock ();
# endif	/* REFCLOCK */
    init_proto ();
    init_io ();
    init_loopfilter ();

    /* 
     * Get configuration.  This (including argument list parsing) is
     * done in a separate module since this will definitely be different
     * for the gizmo board.
     */
    getconfig (argc, argv);
    initializing = 0;

    /* 
     * Report that we're up to any trappers
     */
    report_event (EVNT_SYSRESTART, (struct peer    *)0);

    /* 
     * Done all the preparation stuff, now the real thing.  We block
     * SIGIO and SIGALRM and check to see if either has occured.
     * If not, we pause until one or the other does.  We then call
     * the timer processing routine and/or feed the incoming packets
     * to the protocol module.  Then around again.
     */
    was_alarmed = 0;
    rbuflist = (struct recvbuf *)0;
    for (;;)
        {

# ifndef USESELECT
# 	ifdef SOLARIS
	sigset_t    nmask,
	            omask;

	sigemptyset (&nmask);
	sigaddset (&nmask, SIGIO);
	sigaddset (&nmask, SIGALRM);
	sigprocmask (SIG_BLOCK, &nmask, &omask);
# 	else	/* ! SOLARIS */
	int     omask;

	omask = sigblock (BLOCKSIGMASK);
# 	endif	/* SOLARIS */
	if (alarm_flag)
	    {			/* alarmed? */
	    was_alarmed = 1;
	    alarm_flag = 0;
	    }
	rbuflist = getrecvbufs ();
				/* get received buffers */

	if (!was_alarmed && rbuflist == (struct recvbuf    *)0)
	    {
	    /* 
	     * Nothing to do.  Wait for something.
	     */
# 	ifdef SOLARIS
	    sigsuspend (&omask);
# 	else	/* ! SOLARIS */
	    sigpause (omask);
# 	endif	/* SOLARIS */
	    if (alarm_flag)
	        {		/* alarmed? */
		was_alarmed = 1;
		alarm_flag = 0;
	        }
	    rbuflist = getrecvbufs ();
				/* get received buffers */
	    }
# 	ifdef SOLARIS
    sigprocmask (SIG_SETMASK, &omask, NULL);
# 	else	/* ! SOLARIS */
	(void)sigsetmask (omask);
# 	endif	/* SOLARIS */
# else	/* ! not USESELECT */
	extern fd_set   activefds;
	extern int  maxactivefd;

	fd_set  rdfdes;
	int     nfound;

	/* 
	 * If we can't use SIGIO for both socket and TTY I/O, we
	 * cheat and do a select() on all input fd's for unlimited
	 * time.  select() will terminate on SIGALARM or on the
	 * reception of input.  Using select() means we can't do
	 * robust signal handling and we get a potential race
	 * between checking for alarms and doing the select().
	 * Mostly harmless, I think.
	 */

	rbuflist = getrecvbufs ();
				/* get received buffers */
	if (alarm_flag)
	    {			/* alarmed? */
	    was_alarmed = 1;
	    alarm_flag = 0;
	    }

	if (!was_alarmed && rbuflist == (struct recvbuf    *)0)
	    {
	    /* 
	     * Nothing to do.  Wait for something.
	     */
	    rdfdes = activefds;
	    nfound = select (maxactivefd + 1, &rdfdes, (fd_set *)0,
		    (fd_set *)0, (struct timeval   *)0);
	    if (nfound > 0)
		input_handler ();
	    else if (nfound == -1 && errno != EINTR)
	        {
		syslog (LOG_ERR, "select() error: %m");
	        }
	    if (alarm_flag)
	        {		/* alarmed? */
		was_alarmed = 1;
		alarm_flag = 0;
	        }
	    rbuflist = getrecvbufs ();
				/* get received buffers */
	    }
# endif	/* not USESELECT */

    /* 
     * Out here, signals are unblocked.  Call timer routine
     * to process expiry.
     */
	if (was_alarmed)
	    {
	    timer ();
	    was_alarmed = 0;
	    }

	/* 
	 * Call the data procedure to handle each received
	 * packet.
	 */
	while (rbuflist != (struct recvbuf *)0)
	    {
	    rbuf = rbuflist;
	    rbuflist = rbuf -> next;
	    (rbuf -> receiver) (rbuf);
	    freerecvbuf (rbuf);
	    }
	/* 
	 * Go around again
	 */
        }
}


# ifdef SIGDIE1
/*
 * finish - exit gracefully
 */
static RETSIGTYPE
finish (sig)
int     sig;
{
    struct timeval  tv;

    /* 
     * The only thing we really want to do here is make sure
     * any pending time adjustment is terminated, as a bug
     * preventative.  Also log any useful info before exiting.
     */
    tv.tv_sec = tv.tv_usec = 0;
    (void)adjtime (&tv, (struct timeval    *)0);

# 	ifdef notdef
    log_exit_stats ();
# 	endif	/* notdef */
    exit (0);
}
# endif				/* SIGDIE1 */


# ifdef DEBUG
/*
 * moredebug - increase debugging verbosity
 */
static RETSIGTYPE
moredebug (sig)
int     sig;
{
    if (debug < 255)
        {
	debug++;
	syslog (LOG_DEBUG, "debug raised to %d", debug);
        }
# 	if	!defined(HAVE_RESTARTABLE_SYSCALLS)
    (void)signal (MOREDEBUGSIG, moredebug);
# 	endif	/* !defined (HAVE_RESTARTABLE_SYSCALLS) */
}


/*
 * lessdebug - decrease debugging verbosity
 */
static RETSIGTYPE
lessdebug (sig)
int     sig;
{
    if (debug > 0)
        {
	debug--;
	syslog (LOG_DEBUG, "debug lowered to %d", debug);
        }
# 	if	!defined(HAVE_RESTARTABLE_SYSCALLS)
    (void)signal (LESSDEBUGSIG, lessdebug);
# 	endif	/* !defined (HAVE_RESTARTABLE_SYSCALLS) */
}
# endif				/* DEBUG */
