/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_time.c,v $
 * $Revision: 1.23.83.7 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 14:33:00 $
 */

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

#include "../h/debug.h"

#include "../machine/reg.h"

#include "../h/param.h"
#ifdef __hp9000s800
#include "../h/dir.h"		/* XXX */
#endif
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../h/kern_sem.h"

/* 
 * Time of day and interval timer support.
 *
 * These routines provide the kernel entry points to get and set
 * the time-of-day and per-process interval timers.  Subroutines
 * here provide support for adding and subtracting timeval structures
 * and decrementing interval timers, optionally reloading the interval
 * timers when they expire.
 */

gettimeofday()
{
	register struct a {
		struct	timeval *tp;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;
#ifdef __hp9000s800
    	struct timeval ms_gettimeofday();
#endif

#ifdef __hp9000s800
	atv = ms_gettimeofday();	/* in machine/clock.c */
#endif
#ifdef __hp9000s300
	get_precise_time(&atv);
#endif

#ifdef __hp9000s300
	u.u_error = copyout4((caddr_t)&atv, (caddr_t)uap->tp, sizeof (atv)/4);
#else
	u.u_error = copyout((caddr_t)&atv, (caddr_t)uap->tp, sizeof (atv));
#endif
	if (u.u_error)
		return;
	if (uap->tzp == 0)
		return;
	/* SHOULD HAVE PER-PROCESS TIMEZONE */
#ifdef __hp9000s300
	u.u_error = copyout4((caddr_t)&tz, (caddr_t)uap->tzp, sizeof (tz)/4);
#else
	u.u_error = copyout((caddr_t)&tz, (caddr_t)uap->tzp, sizeof (tz));
#endif
}

#ifdef __hp9000s300
/*Dennis Freidel 7/86
* The following 2 functions are added for NFS port of 4.2 from
* the Series 800. they are "ms_gettimeofday and kernel_gettimeofday
*/
struct timeval
ms_gettimeofday()
{
	struct timeval tp;

	get_precise_time ( &tp );
	return (tp);
}

kernel_gettimeofday(tp)
struct timeval *tp;
{
	return (get_precise_time (tp) );
}
#endif /* __hp9000s300 */

#if defined(__hp9000s800) && defined (PESTAT)
extern int skip_KS_log;		/* flag to prevent logging the current	*/
				/* kernel preemption measurement	*/
#endif

settimeofday()
{
	register struct a {
		struct	timeval *tv;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;
	struct timezone atz;
	sv_sema_t SS;

	if (!suser())
		return;
	u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
		sizeof (struct timeval));
	if (u.u_error)
		return;
	if (uap->tzp)
		u.u_error = copyin((caddr_t)uap->tzp, (caddr_t)&atz, 
				sizeof (atz));
	/*  need to flushd out inode times becasue of date compare */
	/*  algorithm for inodes for discless */
	if (my_site_status & CCT_CLUSTERED){
		PXSEMA(&filesys_sema, &SS);
		sync();
		VXSEMA(&filesys_sema, &SS);
	}
	if (uap->tzp && !u.u_error)
		settimeofday1(&atv, &atz);
	else
		settimeofday1(&atv, (struct timezone *) 0);

	if (my_site_status & CCT_CLUSTERED)
		DUXCALL(REQ_SETTIMEOFDAY)();
}

settimeofday1(atv, atz)
	struct timeval *atv;
	struct timezone *atz;
{
	register int s;

	boottime.tv_sec += atv->tv_sec - time.tv_sec;
	s = spl7(); 
	time = *atv; 
	splx(s);
	if (atz)
		tz = *atz;
	resettodr(atv);
#if defined(__hp9000s800) && defined(PESTAT)
	skip_KS_log = 1;	/* prevent logging the current kernel
				   preemption measurement because 
				   resetting the clock makes the
				   measurement meanless */
#endif
}

#if defined(__hp9000s800) && defined(PESTAT)
extern int skip_KS_log;		/* flag to prevent logging the current	*/
				/* kernel preemption measurement	*/
#endif

setthetime(tv)
	struct timeval *tv;
{
	int s;

	if (!suser())
		return;
	boottime.tv_sec += tv->tv_sec - time.tv_sec;
	s = spl7(); time = *tv; splx(s);
	resettodr(tv);
#if defined(__hp9000s800) && defined(PESTAT)
	skip_KS_log = 1;	/* prevent logging the current kernel */
				/* preemption measurement because 
				   resetting the clock makes the
				   measurement meanless */
#endif
}

/*
 * Get value of an interval timer.  The process virtual and
 * profiling virtual time timers are kept in the u. area, since
 * they can be swapped out.  These are kept internally in the
 * way they are specified externally: in time until they expire.
 *
 * The real time interval timer is kept in the process table slot
 * for the process, and its value (it_value) is kept as an
 * absolute time rather than as a delta, so that it is easy to keep
 * periodic real-time signals from drifting.
 *
 * Virtual time timers are processed in the hardclock() routine of
 * kern_clock.c.  The real time timer is processed by a timeout
 * routine, called from the softclock() routine.  Since a callout
 * may be delayed in real time due to interrupt processing in the system,
 * it is possible for the real time timeout routine (realitexpire, given below),
 * to be delayed in real time past when it is supposed to occur.  It
 * does not suffice, therefore, to reload the real timer .it_value from the
 * real time timers .it_interval.  Rather, we compute the next time in
 * absolute time the timer should go off.
 */

/*
 * THERE IS A VERY OBSCURE RACE IN THE ORIGINAL DESIGN OF THE INTERVAL
 * TIMERS.  Getitimer() is often called by user code to save the pending
 * timer prior to changing it.  The saved value, suitably adjusted, will be
 * re-installed at a later time.  Sleep(3C) is one example of user code
 * which does this.
 *
 * The problem is that a timer value of zero (timerisset() returns false)
 * is ambiguous.  One meaning is that no timer is set.  The other is that a
 * timer is set, but has just expired.  For example, imagine the following
 * timeline:
 *
 * 			T0	V	G	T1
 * 	    ... --------+-------+-------+-------+-------- ...
 *
 * Time increases to the right, T0 and T1 are clock ticks.  The user asked
 * for a timer event at time V.  By definition (see setitimer(2)), the
 * kernel will deliver the signal at the next clock tick, T1.  The user
 * calls getitimer() at time G.  Since V is before G, the timer remaining
 * is negative and getitimer is not permitted to return such a value (see
 * getitimer(2)), so it returns zero.  Fine, there is no more time
 * remaining on the timer, however, the signal has not yet been sent.  If
 * the user assumes this zero result means that no timer was pending and
 * that no SIGALRM has been sent or will be sent, he will either get an
 * extra signal or miss one entirely.  Additionally, if the timer had an
 * it_interval component (which realitexpire() would use to reset the timer
 * at T1) the user would not be aware that he needed to reinstall it.  He
 * would therefore miss out on an arbitrary number of signals he expected
 * to get.
 *
 * The solution is to never return a zero it_value as long as there is a
 * realitexpire() event on the calltodo list.  Since we can't indicate a
 * pending timer and return a zero it_value at the same time, we return an
 * it_value that's the closest thing to zero we have, 1 microsecond.
 */

#define	timersettiny(tvp)	((tvp)->tv_sec = 0, (tvp)->tv_usec = 1)

getitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv;
	} *uap = (struct a *)u.u_ap;
	struct itimerval aitv;
	int s;
	register int hzleft;

	switch (uap->which) {
	case ITIMER_REAL:
	case ITIMER_VIRTUAL:
	case ITIMER_PROF:
		break;		/* parameter is legal */
	default:
		u.u_error = EINVAL;
		return;
		break;
	}

	s = spl7();

	if (uap->which == ITIMER_REAL) {

		/*
		 * Call find_timeout() to find remaining time
		 * for real timer, in hz. find_timeout returns
		 * -1 if no timeout is found on the callout list.
		 * This should never happen if timerisset returns
		 * true, since realitexpire() zeros the timer
		 * if it does not schedule another timeout; However,
		 * rather than panic, we just clear the timer.
		 *
		 * We use find_timeout() instead of computing
		 * against the absolute time so that getitimer
		 * will return the right value when the system
		 * time has been changed between the time the
		 * timer was set and the time it goes off.
		 */

		aitv = u.u_procp->p_realtimer;
		if (timerisset(&aitv.it_value)) {
			hzleft = find_timeout(realitexpire,
					      (caddr_t)u.u_procp,
					      0);
			if (hzleft == -1)
			    timerclear(&aitv.it_value); /* shouldn't happen */
			else {
			    if (hzleft > 0) {
				aitv.it_value.tv_sec = hzleft / HZ;
				aitv.it_value.tv_usec =
				   (hzleft % HZ) * (1000000 / HZ);
			    }
			    else
				timersettiny(&aitv.it_value);
			}
		}
	} else
		aitv = u.u_timer[uap->which];

	splx(s);
	u.u_error = copyout((caddr_t)&aitv, (caddr_t)uap->itv,
	    sizeof (struct itimerval));
}

setitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv, *oitv;
	} *uap = (struct a *)u.u_ap;
	struct itimerval aitv;
	int s;
	register struct proc *p = u.u_procp;

	switch (uap->which) {
	case ITIMER_REAL:
	case ITIMER_VIRTUAL:
	case ITIMER_PROF:
		break;		/* parameter is legal */
	default:
		u.u_error = EINVAL;
		return;
		break;
	}

	u.u_error = copyin((caddr_t)uap->itv, (caddr_t)&aitv,
	    sizeof (struct itimerval));
	if (u.u_error)
		return;

	/*
	 * Prevent timer interrupts between getting the old value
	 * and setting the new one.
	 */
	s = spl7();

	if (uap->oitv) {
		uap->itv = uap->oitv;
		getitimer();
	}
	if (itimerfix(&aitv.it_value) || itimerfix(&aitv.it_interval)) {
		u.u_error = EINVAL;
		splx(s);
		return;
	}
	if (uap->which == ITIMER_REAL) {
		untimeout(realitexpire, (caddr_t)p);
		if (aitv.it_value.tv_sec >= MAX_ALARM) {
			aitv.it_value.tv_sec = MAX_ALARM;
			aitv.it_value.tv_usec = 0;
		}
		if (aitv.it_interval.tv_sec >= MAX_ALARM) {
			aitv.it_interval.tv_sec = MAX_ALARM;
			aitv.it_interval.tv_usec = 0;
		}

		/* spl moved to an earlier point to avoid a race */
		if (timerisset(&aitv.it_value)) {
			timevaladd(&aitv.it_value, &time);
			timeout(realitexpire, (caddr_t)p, hzto(&aitv.it_value));
		}
		p->p_realtimer = aitv;
	}
	else if (uap->which == ITIMER_VIRTUAL) {
		if (aitv.it_value.tv_sec >= MAX_VTALARM) {
			aitv.it_value.tv_sec = MAX_VTALARM;
			aitv.it_value.tv_usec = 0;
		}
		if (aitv.it_interval.tv_sec >= MAX_VTALARM) {
			aitv.it_interval.tv_sec = MAX_VTALARM;
			aitv.it_interval.tv_usec = 0;
		}
		u.u_timer[ITIMER_VIRTUAL] = aitv;
	}
	else /* ITIMER_PROF */ {
		if (aitv.it_value.tv_sec >= MAX_PROF) {
			aitv.it_value.tv_sec = MAX_PROF;
			aitv.it_value.tv_usec = 0;
		}
		if (aitv.it_interval.tv_sec >= MAX_PROF) {
			aitv.it_interval.tv_sec = MAX_PROF;
			aitv.it_interval.tv_usec = 0;
		}
		u.u_timer[ITIMER_PROF] = aitv;
	}

	splx(s);
}

/*
 * Real interval timer expired:
 * send process whose timer expired an alarm signal.
 * If time is not set up to reload, then just return.
 * Else compute next time timer should go off which is > current time.
 * This is where delay in processing this timeout causes multiple
 * SIGALRM calls to be compressed into one.
 */
realitexpire(p)
	register struct proc *p;
{
	int s;

	s = spl7();

	psignal(p, SIGALRM);
	if (!timerisset(&p->p_realtimer.it_interval)) {
		timerclear(&p->p_realtimer.it_value);
		splx(s);
		return;
	}

	/*
	 * If we are ahead of system time by more than a second
	 * (before the interval is added), then the system time
	 * must have been changed backwards (we could be behind
	 * by more than a second due to processing overhead,
	 * but we should never be ahead). We allow the 1 second
	 * difference to account for the fact that we are
	 * only comparing the tv_sec component, and to allow
	 * for roundoff in the hzto() computation.
	 *
	 * If the system time has been changed backwards then
	 * we resync with the new system time before scheduling
	 * another timeout.
	 *
	 * Note, if the system time has been moved forward it
	 * will be covered by the for loop, although this
	 * could cause the system to hang if a minimum
	 * interval timeout is in effect and the system
	 * time is moved ahead significantly (days). Since
	 * only super-user can change the system time, this
	 * should not be a real problem.
	 */

	if ((p->p_realtimer.it_value.tv_sec - time.tv_sec) > 1)
	    p->p_realtimer.it_value = time;

	for (;;) {
		timevaladd(&p->p_realtimer.it_value,
		    &p->p_realtimer.it_interval);
		if (timercmp(&p->p_realtimer.it_value, &time, >)) {
			timeout(realitexpire, (caddr_t)p,
			    hzto(&p->p_realtimer.it_value));
			splx(s);
			return;
		}
	}
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
itimerfix(tv)
	struct timeval *tv;
{

	if (tv->tv_usec < 0 || tv->tv_usec >= 1000000)
		return (EINVAL);
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < tick)
		tv->tv_usec = tick;
	return (0);
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */
itimerdecr(itp, usec)
#ifdef	MP
	register struct itimerval ^itp;	/* LONG_PTR */
#else
	register struct itimerval *itp;
#endif
	int usec;
{

	if (itp->it_value.tv_usec < usec) {
		if (itp->it_value.tv_sec == 0) {
			/* expired, and already in next interval */
			usec -= itp->it_value.tv_usec;
			goto expire;
		}
		itp->it_value.tv_usec += 1000000;
		itp->it_value.tv_sec--;
	}
	itp->it_value.tv_usec -= usec;
	usec = 0;
	if (timerisset(&itp->it_value))
		return (1);
	/* expired, exactly at end of interval */
expire:
	if (timerisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_usec -= usec;
		if (itp->it_value.tv_usec < 0) {
			itp->it_value.tv_usec += 1000000;
			itp->it_value.tv_sec--;
		}
	} else
		itp->it_value.tv_usec = 0;		/* sec is already 0 */
	return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
timevaladd(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

#ifdef __hp9000s800
/*
 * Convert a timeval to clock ticks (round to nearsest).  Try not to
 * lose anything to integer overflow.
 */
long
tvtoticks(tv)
	struct timeval *tv;
{
	return (tv->tv_sec*HZ)+((tv->tv_usec+(500000/HZ))*HZ)/1000000;
}

/*
 * Copy a timeval atomically.
 *
 * NOTE:  On an 8Mhz Series 800 with HZ set at 100, we will make only one
 *	  pass through this loop 999997 times per million calls.  More
 *	  passes will be correspondingly more rare.  As the clock speed
 *	  goes up (and HZ remains constant) more passes will be rarer still.
 */
void
tvcopy(dst, src)
	struct timeval *dst, *src;
{
#ifdef OSDEBUG
	int loopcount = 0;
#endif

	do {
		dst->tv_sec = src->tv_sec;
		dst->tv_usec = src->tv_usec;
		VASSERT(++loopcount < 10);	/* an arbitrary small # */
	} while (dst->tv_sec != src->tv_sec || dst->tv_usec != src->tv_usec);
}

#endif   /*  hp9000s800  */



int tickadj = 30000 / (60 * HZ);	/* can adjust 30ms in 60s */
int tickdelta;				/* current clock skew, us. per tick */
long timedelta;				/* unapplied time correction, us. */
long bigadj = 1000000;			/* use 10x skew above bigadj us. */


int adjtime_called = 0;


adjtime()
{
	register struct a {
		struct	timeval *delta;
		struct	timeval *olddelta;
	} *uap = (struct a *)u.u_ap;
	register int s;
	struct timeval atv;
	register long ndelta, ntickdelta, odelta;


	adjtime_called++;

	if (!suser())
	    return;
	if (my_site_status & CCT_CLUSTERED) 
	    if (!(my_site_status & CCT_ROOT)) {
		u.u_error = EPERM;
		return;
	    }
	u.u_error = copyin((caddr_t)uap->delta, (caddr_t)&atv,
		sizeof (struct timeval));
	if (u.u_error)
		return;

	if (atv.tv_sec)
		msg_printf("adjtime: sec u-sec are %d %d\n", atv.tv_sec, atv.tv_usec);
	
	/* 
	 * Compute the total correction and the rate at which to apply it.
	 * Round the adjustment down to a whole multiple of the per-tick
	 * delta, so that after some number of incremental changes in
	 * hardclock(), tickdelta will become zero, lest the correction
	 * overshoot and start taking us away from the desired final time,
	 */
	ndelta = atv.tv_sec * 1000000 + atv.tv_usec;
	if (ndelta > bigadj) 
	    ntickdelta = 10 * tickadj;
	else
	    ntickdelta = tickadj;
	if (ndelta % ntickdelta)
	    ndelta = ndelta / ntickdelta * ntickdelta;
	
	/* 
	 * To make hardclock()'s job easier, make the per-tick delta negative
	 * if we want time to run slower; then hardclock can simply compute
	 * tick + tickdelta, and subtract tickdelta from timedelta.
	 */
	if (ndelta < 0)
	    ntickdelta = -ntickdelta;
	
	s = spl7(); 
	odelta = timedelta;
	timedelta = ndelta;
	tickdelta = ntickdelta;
	splx(s);
	
	if (uap->olddelta) {
	    atv.tv_sec = odelta / 1000000;
	    atv.tv_usec = odelta % 1000000;
#ifdef __hp9000s300
	    u.u_error = copyout4((caddr_t)&atv, (caddr_t)uap->olddelta, sizeof (atv)/4);
#else
	    u.u_error = copyout((caddr_t)&atv, (caddr_t)uap->olddelta, sizeof (atv));
#endif
	    if (u.u_error)
		return;
	}
}
