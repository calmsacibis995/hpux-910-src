static char *HPUX_ID = "@(#) $Revision: 70.4 $";
/*
 * Update the file system every timer seconds.
 * For Diskless/HP-UX, the -l flag means sync only the local system.
 * For backward compat, -s does NOT update /etc/mnttab
 * For cache benefit, open directories specified with the -d option.
 *
 *	syncer [ seconds ] [ -l ] [ -d directories ... ]
 */

#ifdef GETMOUNT
#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#endif /* GETMOUNT */
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>

#if defined(__hp9000s800)
#include <unistd.h>		/* for sysconf() */
#endif

#define DEFAULT_TIME 30

/** amount of time in seconds between syncs ***/

int timer = DEFAULT_TIME;
int dflag = 0;
#if defined(DUX) || defined(DISKLESS)
int lflag  = 0;
int __s800 = 0;
#endif /* DUX || DISKLESS */
#ifdef GETMOUNT
int save_mnttab = 0;
extern int update_mnttab();

/*
 * For checking time stamp on /etc/mnttab, allow a four second lee way
 * for kernels within a cluster in which the kernel mount table time
 * stamps may be slightly out of sync.  Kernels should not be more than
 * one or two seconds out of sync.
 */
#define TIME_PAD 2
#endif /* GETMOUNT */

main(argc,argv)
int argc;
char **argv;
{
    int i;
    char *options;

    /*
     * get the amount of time between syncs and any flags
     * that are set
     */
    for (; argc > 1 && !dflag; argv++, argc--)
    {
	if (argv[1][0] == '-')
	{
	    options = &argv[1][1];
	    while (*options)
		switch (*options++)
		{
		case 'd':
		    ++dflag;
		    break;
#if defined(DUX) || defined(DISKLESS)
		case 'l':
		    ++lflag;
		    break;
#endif
#ifdef GETMOUNT
		case 's':
		    ++save_mnttab;
		    break;
#endif /* GETMOUNT */
		}
	}
	else
	{
	    timer = atoi(argv[1]);
	    if (timer < 1)
		timer = DEFAULT_TIME;
	}
    }

    /* Check the system type */
#if defined(__hp9000s800)
    __s800 = (sysconf(_SC_IO_TYPE) != IO_TYPE_WSIO); /* true on s800 */
						     /* false on s700 */
#endif /* __hp9000s800 */

    /** ignore hangups and interrupts **/
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    /*
     * The S800 does not support multiple syncer processes.
     * If there is an outstanding call to tsync(2), another call
     * will return -1 and EBUSY.  This could cause multiple instances
     * of the syncer program to eat up a lot of CPU cycles.
     *
     * We make an honest attempt to prevent this problem by running
     * the first instance of dosync() as the parent.  If the call
     * fails, we display an error message and exit.
     *
     */
#if defined(DUX) || defined(DISKLESS)
    if (__s800 && dosync()) {
       fprintf(stderr, "syncer: a syncer process already exists.\n");
       exit(1);
    }
#endif

#ifndef DEBUG
    if (fork())
	exit(0);
#endif

    /*
     * change group so that this process is in its own group
     */
    setpgrp();

    /*
     * start the syncing
     */
    close(0);
    close(1);
    close(2);
    if (dflag)
    {
	for (i = 1; i < argc; i++)
	    open(argv[i], O_RDONLY);
    }
#if defined(DUX) || defined(DISKLESS)
    if (__s800) {
	for(;;)
	   if (dosync()) {
	      syslog(LOG_WARNING|LOG_DAEMON, "syncer: a syncer process already exists.\n");
	      exit(2);
	   }
    } else {
	dosync();
	for(;;) pause();
    }
#else
    dosync();
    for (;;) pause();
#endif
}

dosync()
{
    int retval;

#ifdef GETMOUNT
    struct stat statbuf;	/* for stat of mnttab */
    time_t mnttab_time = 0;	/* for kernel mnttab time stamp */

    if (!save_mnttab)
    {
	/* get kernel last mount change time */
	(void)getmount_cnt(&mnttab_time);
	/* 
         * Update if mnttab file is more than 2*TIME_PAD 
	 * seconds out of date, or if stat fails 
	 */
	if ((stat(MNT_MNTTAB, &statbuf) != 0) 			||
	     ((mnttab_time < statbuf.st_mtime - TIME_PAD)	||
	       (mnttab_time > statbuf.st_mtime + TIME_PAD)))
	{
	    /* update mnttab & reset time -- ignore errors */
	    (void)update_mnttab();
	}
    }
#endif /* GETMOUNT */

#if defined(DUX) || defined(DISKLESS)
    /* The following code takes advantage of new functionality provided by 
     * tsync on the S800. The flag value of 2 and 3 corresponds to the
     * old value of 0 and 1 but specifies that a second option (the timer
     * value) is being passed. In this case, the kernel will take care
     * of "sleeping" for that amount of time.
     */
    if (__s800)
    {
	if (lflag)
	    retval = tsync(2, timer);
	else
	    retval = tsync(3, timer);
	if ((retval < 0) && (errno == EBUSY)) return(1);
	else return(0);
    } else {
	if (lflag)
	    tsync(0);
	else
	    tsync(1);
    	signal(SIGALRM, dosync);
    	alarm(timer);
	return(0);
    }

#else
    sync();
    signal(SIGALRM, dosync);
    alarm(timer);
    return(0);
#endif
}
