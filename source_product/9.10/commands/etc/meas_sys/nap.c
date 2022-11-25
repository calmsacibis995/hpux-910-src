/*LINTLIBRARY*/

/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/nap.c,v $
 * $Revision: 4.7 $		$Author: dah $
 * $State: Exp $		$Locker:  $
 * $Date: 85/12/11 16:54:31 $
 *
 * $Log:	nap.c,v $
 * Revision 4.7  85/12/11  16:54:31  16:54:31  dah
 * 
 * clean up more for lint
 * 
 * Revision 4.6  85/12/11  14:59:51  dah (Dave Holt)
 * make lint happy.
 * As a result, handle blocked signals more correctly.  Leave them as
 * I found them when I'm done, and don't catch (+ ignore) any that
 * were supposed to be blocked.
 * 
 * Revision 4.5  85/12/11  11:23:03  dah (Dave Holt)
 * some progress in linting
 * 
 * Revision 4.4  85/12/11  10:19:18  dah (Dave Holt)
 * make lint happy by doing something sensible with error status
 * from sigpause.
 * 
 * Revision 4.3  85/12/03  16:47:29  dah (Dave Holt)
 * added standard firstci header
 * 
 * $Endlog$
 */

/*
Dave Holt

This version is designed to use reliable signals.
Do not mix with code that uses Bell signals.
 */

#include <sys/time.h>
#include <signal.h>
#include <sys/errno.h>

#ifdef DEBUG
#include <stdio.h>
#endif DEBUG

#define MILLION	1000000

extern int errno;

static char rcsid[] = "$Header: nap.c,v 4.7 85/12/11 16:54:31 dah Exp $";

static int alarm_rang = 0;

/* nop signal handler used in nap */
int handle_alarm()
{
    alarm_rang = 1;
    return(0);
}

/* sleep for secs seconds of real time */
nap(secs)
     float secs;
{
#ifdef DEBUG
    struct timezone tz;
    struct timeval tv1, tv2;
#endif DEBUG

    static int first_time = 1;

    struct sigvec vec;

    long err, oldmask;
    struct itimerval itv, oitv;

    int isecs, usecs;

    (void) rcsid;		/* make lint happy */

    if (first_time) {
	vec.sv_handler = handle_alarm;
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	if (sigvector(SIGALRM, &vec, (struct sigvec *) 0) != 0) {
	    perror("sigvector");
	    exit(1);
	}
	first_time = 0;
    }

    timerclear(&itv.it_interval);
    isecs = (int) secs;
    usecs = MILLION * (secs - isecs) + 0.5;
    itv.it_value.tv_sec = isecs;
    itv.it_value.tv_usec = usecs;

    if (isecs == 0 && usecs == 0) {
#ifdef DEBUG
	printf("zero time\n");
#endif DEBUG
	return;
    }

    oldmask = sigblock(1L << (SIGALRM -1));

    if (setitimer(ITIMER_REAL, &itv, &oitv) != 0) {
	perror("setitimer");
	exit(1);
    }

#ifdef DEBUG
    gettimeofday(&tv1, &tz);
#endif DEBUG

    /* wait til we get a SIGALRM */
    /* other signals can process themselves, but they are not for us */
    while (!alarm_rang) {
	err = sigpause(oldmask);
	if ((err != -1) || errno != EINTR) {
	    warn("sigpause: err = %d, errno = %d", err, errno);
	}
    }
    (void) sigsetmask(oldmask);
    alarm_rang = 0;

#ifdef DEBUG
    gettimeofday(&tv2, &tz);
    printf("nap: requested time %f .\nActual interval: %u.%06d to %u.%06d\n",
	   secs, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);
#endif DEBUG
}


