/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/clocksync.c,v $
 * $Revision: 1.6.83.4 $	$Author: marshall $
 * $State: Exp $      $Locker:  $
 * $Date: 93/12/15 12:01:43 $
 */

/* HPUX_ID: @(#)clocksync.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include	"../h/param.h"
#include	"../dux/dmmsgtype.h"
#include	"../dux/dm.h"
#include	"../h/time.h"
#include	"../dux/cct.h"
#include	"../dux/duxparam.h"

#ifdef	__hp9000s300
#include	"../wsio/timeout.h"

#include "../s200io/lnatypes.h"
#endif	/* hp9000s300 */

#undef timeout			/* get the correct timeout routine. */

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

/*
 * Clock sync algorithm:
 *
 * Every CLOCKSYNCPERIOD seconds, the root server broadcasts the current
 * (precise) time to the cluster.  Precise time is obtained from the
 * system snooze clock, which has a resolution of about 4 micro seconds.
 *
 * Upon receiving the current time, each workstation computes the amount
 * of error between its clock and the root server's.
 *
 * There is a new concept, the uS/tick increment value.  This is the
 * "old_tick" variable.  In normal UNIX(tm) systems, the increment value
 * is a constant, usually 1,000,000/HZ.  In this system, "old_tick"
 * is added to the system time at each clock tick.  By adjusting
 * this value downward, time moves more slowly, allowing the rootserver to
 * catch up.  Conversely, to make the workstation catch up, adjust this
 * value upward.
 *
 * Since retries would invalidate the time message, they aren't done !
 * The workstation algorithm allows for missed sync pulses without
 * drifting too far off.  See (4) below.
 *
 * Some minor adjustments to this basic algorithm:
 * 1) Compute and add LAN software/hardware delay
 * 2) When the error is less than CLOCKSYNCPERIOD microseconds, the
 *    adjustment is less than 1.  This is difficult to do with integers. :-)
 *    When this happens, the old_tick (uS/tick increment) value is adjusted
 *    so that sometime between now and the next CLOCKSYNC, the error is zero.
 * 3) To tolerate missed CLOCKSYNC's (we don't retry), a timeout is set by
 *    the workstation to adjust old_tick to make another zero-error crossing.
 *    This timeout method continues ad nauseum, so it takes quite a few
 *    consecutive lost sync's to cause a >20mSec error.
 *
 * Some numbers:
 * 
 * The clock accuracy in the s300 is +/- 5 seconds/day.  This works out to
 * one tick per 17280.  The file system stores time in seconds.  The only
 * time a clock error will show up is if the root server is on one side
 * of one second, while the workstation is on the other.
 *
 * To compute the probability of this happening, take the average error,
 * and divide by one second.  Thus, if the error is 20000 uS (one tick),
 * the probability is .020000/1.000000 = 2%.
 */

/*
** Global variables
*/
#define CLOCKSYNCPERIOD 2*60		/* seconds */
extern volatile int old_tick;		/* at clock ISR, time += old_tick; */
extern volatile struct timeval time;
extern struct timezone tz;

struct dux_clkmsg {
	struct timeval time;
	struct timezone tz;
};

#ifdef	__hp9000s300
struct sw_intloc clocksync_intloc;
#endif

long LanDelaySum = 0;			/* LAN message delay stats */
long LanDelayN   = 0;
volatile long LanDelayAvg = 0;
long syncN   = 0;			/* clock error stats */
long syncMax = 0x80000000;		/* -2**31 */
long syncAvg = 0;
long syncMin = 0x7FFFFFFF;		/* 2**31 - 1 */
long syncSum = 0;
/*
 * If we miss a clocksync from the root server, this routine "bends"
 * the clock the other way to ensure another zero-error crossing.
 * The timeout() continues this process for multiple clocksync misses.
 */
volatile short old_tick_adjustment;	/* either +1 or -1 */

adjust_old_tick()
{
	old_tick_adjustment = -old_tick_adjustment;
	old_tick += old_tick_adjustment;
	timeout(adjust_old_tick, 0, CLOCKSYNCPERIOD*HZ);
}

/*
 * this routine is called by settimeofday() to change the time in the cluster
 */
req_settimeofday()
{
	register dm_message reqp;
	register struct dux_clkmsg *reqtp;

	reqp = dm_alloc(sizeof(struct dux_clkmsg), WAIT);
	reqtp = DM_CONVERT(reqp, struct dux_clkmsg);
	dux_gettimeofday(&reqtp->time);
	reqtp->tz = tz;

	/*
	 * broadcast a message to all cluster members to set 
	 * their clocks with reference to this site.
	 * This calls serve_settimeofday() on the other sites.
	 */
	dm_send(reqp, DM_RELEASE_REQUEST | DM_RELEASE_REPLY | DM_SLEEP,
		DM_SERSETTIME, DM_CLUSTERCAST,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL);
}
/*
 * This function is called remotely via a CLUSTERCAST message to
 * synchronize the clock on this site with the settimeofday() requestor.
 */
serve_settimeofday(reqp)
dm_message reqp;
{
	register struct dux_clkmsg *reqtp;

	reqtp = DM_CONVERT(reqp, struct dux_clkmsg);
	reqtp->time.tv_usec += LanDelayAvg;
	settimeofday1(&reqtp->time,&reqtp->tz);
	dm_quick_reply(0);	/* let the root server know we got it */
}

clocksync()
#ifdef	__hp9000s300
{	 /* sw_trigger to prevent collision in hw_send */
	int root_clocksync();

	sw_trigger(&clocksync_intloc, root_clocksync, 0,
			LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL+1);
}
/* statistics */
root_clocksync()
#endif	/* hp9000s300 */
{
	register struct dux_clkmsg *reqtp;
	struct mbuf clocksync_mbuf;		/* mbuf on the stack (!) */

	if ((my_site_status & CCT_ROOT) == 0)
		panic("clocksync");		/* programming error */
	init_static_mbuf(sizeof(struct dux_clkmsg), &clocksync_mbuf);
	reqtp = DM_CONVERT(&clocksync_mbuf, struct dux_clkmsg);
	dux_gettimeofday(&reqtp->time);
	dm_send(&clocksync_mbuf, DM_DATAGRAM, DM_SERSYNCREQ, DM_CLUSTERCAST,
		NULL, NULL, NULL, NULL,		/* calls serve_sync_req() */
		NULL, NULL, NULL, NULL);
	timeout(clocksync,0,CLOCKSYNCPERIOD*HZ);/* again, later */
}

#define DELAY_FLAG 0xBADBAD00		/* I hope this is unique enough */

serve_sync_req(reqp)
dm_message reqp;
{
	register struct dux_clkmsg *reqtp;
	register error;
	struct timeval diskless_time;

	dux_gettimeofday(&diskless_time); /* get this right away */
	reqtp = DM_CONVERT(reqp, struct dux_clkmsg);

/*
 * The LAN delay computation algorithm is piggybacked on the clocksync dm
 * interface.  Until a sufficient LAN delay sample is taken, the workstation
 * pings the root server.  The rootserver simply echoes the ping back.
 */
	if (my_site_status & CCT_ROOT) {	/* root server */
	    if (reqtp->tz.tz_minuteswest != DELAY_FLAG)
		panic("clocksync");		/* programming error */
	    dm_send(reqp, DM_DATAGRAM,		/* fire it right back */
		DM_SERSYNCREQ, DM_SOURCE(reqp),
		NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL);
	    goto done;				/* done with packet */
	} else {				/* workstation */
	    if (reqtp->tz.tz_minuteswest != DELAY_FLAG) {
						/* normal packet, ping root */
		if ((LanDelayN < 20) && (syncN > 0)) {/* arbitrary numbers */
		    struct mbuf buf;
		    register struct dux_clkmsg *bufp;
		    init_static_mbuf(sizeof(struct dux_clkmsg),&buf);
		    bufp = DM_CONVERT(&buf, struct dux_clkmsg);
		    bufp->time = diskless_time;
		    bufp->tz.tz_minuteswest = DELAY_FLAG;
		    dm_send(&buf, DM_DATAGRAM,	/* send to root server */
			DM_SERSYNCREQ, DM_SOURCE(reqp),
			NULL,NULL,NULL,NULL,
			NULL,NULL,NULL,NULL);
						/* continue processing packet */
		}
	    } else {				/* returned ping from root */

		error  = diskless_time.tv_sec;  error -= reqtp->time.tv_sec;
		error *= 1000000;		/* convert to micro-seconds */
		error += diskless_time.tv_usec; error -= reqtp->time.tv_usec;
		LanDelaySum += error;
		LanDelayN++;
		LanDelayAvg = LanDelaySum / LanDelayN;
		goto done;			/* done with packet */
	    }
	}
	reqtp->time.tv_usec += LanDelayAvg;

/* 
 * Note on use of constants:  we are assuming that manufacturing is
 * doing a good job, and not shipping the wrong clock crystals.  If
 * the actual clock speed is radically different from HZ interrupts
 * per second, this algorithm falls apart.  Since we are not worried
 * about this in non-DUX systems, let's not worry about it now.   ;-)
 */

/* make sure error (in micro seconds) doesn't overflow */
	error = reqtp->time.tv_sec; error -= diskless_time.tv_sec;
	if ((-2146 > error) || (error > 2146)) { /* 2146 = (2^31-1)/1000000 */
		time = reqtp->time;/* drastic action for a drastic problem */
		old_tick = 1000000/HZ;
		goto done;
	}
	error *= 1000000;	/* convert to micro-seconds */
	error += reqtp->time.tv_usec; error -= diskless_time.tv_usec;
	old_tick_adjustment = -1;
    	if (error > 0) {	/* error>0  => rootserver is ahead */
/* if rootserver is ahead, round old_tick up so we cross zero error within the
   next CLOCKSYNCPERIOD.  This rounds "error" up in old tick computation */
		error += (CLOCKSYNCPERIOD*HZ - 1);
		old_tick_adjustment = 1;
	}
	old_tick = (1000000*CLOCKSYNCPERIOD + error) / (CLOCKSYNCPERIOD*HZ);
	if (old_tick <= 0)
		old_tick = 1;		/* time cannot run backwards */
/*
 * Set up timeout to bend old_tick the other way if next clock sync is missed.
 */
	untimeout(adjust_old_tick,0);
	timeout(adjust_old_tick, 0, CLOCKSYNCPERIOD*HZ+1);

/* the QA plan consists of periodic examination of these statistics */
	if (error < 0)
		error = -error;		/* absolute value */
	if (syncMax < error)
		syncMax = error;
	if (syncMin > error)
		syncMin = error;
	syncSum += error;
	syncN++;
	syncAvg = syncSum; syncAvg /= syncN;
done:
	dm_release(reqp,0);
}
