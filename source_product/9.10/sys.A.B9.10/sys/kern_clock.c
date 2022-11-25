/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/kern_clock.c,v $
 * $Revision: 1.6.84.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/21 13:33:02 $
 */

/* HPUX_ID: @(#)kern_clock.c	55.1		88/12/23 */

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

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../h/param.h"

#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/callout.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"

#ifdef FSD_KI
#include "../h/ki_calls.h"

/* nested interrupt level counter */
int	interrupt;
#endif /* FSD_KI */

#ifdef GPROF
#include "../h/gprof.h"
int nmi_link;
#endif

extern int ave_num_fault_procs, num_fault_procs;

/*
 * Clock handling routines.
 *
 * This code is written to operate with two timers which run
 * independently of each other. The main clock, running at hz
 * times per second, is used to do scheduling and timeout calculations.
 * The second timer does resource utilization estimation statistically
 * based on the state of the machine phz times a second. Both functions
 * can be performed by a single clock (ie hz == phz), however the
 * statistics will be much more prone to errors. Ideally a machine
 * would have separate clocks measuring time spent in user state, system
 * state, interrupt state, and idle state. These clocks would allow a non-
 * approximate measure of resource utilization.
 */

/*
 * TODO:
 *	time of day, system/user timing, timeouts, profiling on separate timers
 *	allocate more timeout table slots when table overflows.
 */

/*
 * The hz hardware interval timer.
 * We update the events relating to real time.
 * If this timer is also being used to gather statistics,
 * we run through the statistics gathering routine as well.
 */

/* hardclock() and gatherstats() are not merged due to high level of       */
/* differences between the code. It might be possible to merge hardclock() */
/* if and when KI is implemented on Series 800.                            */

hardclock(pc, ps, ticks)
	caddr_t pc;
	int ps;
	register long ticks;
{
	extern int activity_led_enb;
	register struct callout *p1;
#ifdef FSD_KI
	register struct proc *p = u.u_procp;
#else
	register struct proc *p;
#endif
	register int s, cpstate;
	register int elapsed_usec;
	register long residual_ticks;

	/* eliminate multiply overhead for most cases */
	extern int old_tick;

	extern int tickdelta;
	extern long timedelta;


	switch (ticks) {
	case 1:
		if (timedelta == 0)
		    elapsed_usec = old_tick;
		else {
		    elapsed_usec = old_tick + tickdelta;
		    timedelta -= tickdelta;
		}
		break;
	case 2:
		if (timedelta == 0)
		    elapsed_usec = old_tick + old_tick;
		else {
		    elapsed_usec = (old_tick + old_tick) + (tickdelta + tickdelta);
		    timedelta -= (tickdelta + tickdelta);
		}
		break;
	default:
		if (timedelta == 0)
		    elapsed_usec = ticks * old_tick;
		else {
		    elapsed_usec = ticks * (old_tick + tickdelta);
		    timedelta -= ticks * tickdelta;
		}
		break;
	}

	/*
	 * Update real-time timeout queue.
	 * At front of queue are some number of events which are ``due''.
	 * The time to these is <= 0 and if negative represents the
	 * number of ticks which have passed since it was supposed to happen.
	 * The rest of the q elements (times > 0) are events yet to happen,
	 * where the time for each is given as a delta from the previous.
	 * Decrementing just the first of these serves to decrement the time
	 * to all events.
	 */
	p1 = calltodo.c_next;
	residual_ticks = ticks;
	while (residual_ticks && p1) {
		if (residual_ticks <= p1->c_time) {
			p1->c_time -= residual_ticks;	/* the typical case */
			break;
		}
		else {
			s = p1->c_time;
			p1->c_time -= residual_ticks;
			if (s > 0)
				residual_ticks -= s;
			p1 = p1->c_next;
		}
	}

#ifdef FSD_KI
	/* 		Now determine the cpu state
	 *
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	if (interrupt > 1)		/* must be an ISR */
		cpstate = CP_INTR;
	else if (noproc) 		/* must be in IDLE mode */
		cpstate = CP_IDLE;
	else {				/* must be a running process */
		if (USERMODE(ps)) {	/* process in USER mode */
			if (p->p_nice > NZERO)	{cpstate = CP_NICE;}
			else			{cpstate = CP_USER;}
		}
		else { 			/* process in kernel mode */
			if (p->p_flag & SSYS)	{cpstate = CP_SSYS;}
			else 			{cpstate = CP_SYS; }
		}
	}
	switch(cpstate) {
	case	CP_IDLE:	/* IDLE mode */
	case	CP_INTR:	/* INTERRUPT mode */
	default:
		break;
	case	CP_USER:	/* user mode of USER process */
	case	CP_NICE:	/* user mode of USER process at nice priority */
	case	CP_SYS:		/* kernel mode of USER process */
	case	CP_SSYS:	/* kernel mode of KERNEL process */
#endif /* FSD_KI */

	/*
	 * We adjust the priority of the current process.
	 * The priority of a process gets worse as it accumulates
	 * CPU time.  The cpu usage estimator (p_cpu) is increased here
	 * and the formula for computing priorities (in kern_synch.c)
	 * will compute a different value each time the p_cpu increases
	 * by 4.  The cpu usage estimator ramps up quite quickly when
	 * the process is running (linearly), and decays away exponentially,
	 * at a rate which is proportionally slower when the system is
	 * busy.  The basic principal is that the system will 90% forget
	 * that a process used a lot of CPU time in 5*loadav seconds.
	 * This causes the system to favor processes which haven't run
	 * much recently, and to round-robin among other processes.
	 */
#ifndef	FSD_KI
	if (!noproc) {
		p = u.u_procp;
#endif  not FSD_KI
		p->p_cpticks++;
		if (++p->p_cpu == 0)
			p->p_cpu--;
		if ((p->p_cpu&3) == 0) {
			(void) setpri(p);
			if (p->p_pri >= PUSER)
				p->p_pri = p->p_usrpri;
		}
#ifndef	FSD_KI
	}
#endif  /* not FSD_KI */

#ifdef FSD_KI
		/*
	 	 * The cpu is currently scheduled to a process, then
	 	 * charge it with resource utilization for a tick, updating
	 	 * statistics which run in (user+system) virtual time,
	 	 * such as the cpu time limit and profiling timers.
	 	 * This assumes that the current process has been running
	 	 * the entire last tick.
	 	 */
#else  /* not FSD_KI */

	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	if (USERMODE(ps)) {
		/*
		 * CPU was in user state.  Increment user time counter,
		 * and process process-virtual time interval timer.
		 */
		++u.u_procp->p_uticks;
		if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(p, SIGVTALRM);
		if (p->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	}
	else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if (noproc) {
			if (BASEPRI(ps))
				cpstate = CP_IDLE;
		}
		else {
			++u.u_procp->p_sticks;

#ifdef GPROF
			if (!nmi_link)
			{
				s = pc - s_lowpc;
				if (profiling < 2 && s < s_textsize)
					kcount[s / (HISTFRACTION * sizeof(*kcount))]++;
			}
#endif /* GPROF */
		}
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */
	if (noproc == 0 && cpstate != CP_IDLE) {
#endif /* not FSD_KI */
		if (timerisset(&u.u_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_PROF], tick) == 0)
			psignal(p, SIGPROF);
		s = prssize(p);
		u.u_ru.ru_idrss += s; u.u_ru.ru_isrss += 0;	/* XXX */
		if (s > u.u_ru.ru_maxrss)
			u.u_ru.ru_maxrss = s;
#ifdef FSD_KI
		switch(cpstate) {
		case	CP_USER:/* user mode of USER process */
		case	CP_NICE:/* user mode of USER process at nice priority */
			/*
			 * CPU was in user state.  Increment
			 * user time counter, and process process-virtual time
			 * interval timer.
			 */
			++u.u_procp->p_uticks;
			if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
			    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0)
				psignal(p, SIGVTALRM);
			break;
		case	CP_SYS:		/* kernel mode of USER process */
		case	CP_SSYS:	/* kernel mode of KERNEL process */
/* 
 * The  following  code is part of the fix to  charge  this  tick used by the 040
 * floating  pt.  software  emulation  package to USER -- even though the code is
 * actually in the kernel.  If this clock tick  interrupted  the 040 floating pt.
 * software  emulation  package  then charge the tick to USER rather than SYSTEM.
 * The software  emulation  package will check runrun and  and do  a rescheduable 
 * trap -- which will take care of properly charging profiling ticks. (dlb)
 */
			{
			extern soft_emulation_beg;
			extern soft_emulation_end;

			if (pc >= (caddr_t)&soft_emulation_beg && 
				pc <  (caddr_t)&soft_emulation_end) 
			{
				++u.u_procp->p_uticks;
			} else
			{
				++u.u_procp->p_sticks;
			}
			}
#ifdef GPROF
			if (!nmi_link)
			{
				s = pc - s_lowpc;
				if (profiling < 2 && s < s_textsize)
					kcount[s / (HISTFRACTION * sizeof(*kcount))]++;
			}
#endif /* GPROF */
			break;
		}
		break;
#endif /* FSD_KI */
	}

#ifdef FSD_KI

/* begin GATHERSTATS */
	/*
	 * We maintain statistics shown by user-level statistics
	 * programs:  the amount of time in each cpu state, and
	 * the amount of time each of the ``drives'' is busy.
	 */
	cp_time[cpstate]++;
	{
		register long *cnt = dk_time;	/* pointer to first slot */
		for (s=dk_busy; s; s>>=1) {	/* inspect busy bits */
			if (s & 1)		/* if this one is busy */
				++*cnt;		/* bump its count */
			cnt++;			/* go to next counter */
		}
	}
/* end  GATHERSTATS */

#else  /* not FSD_KI */

	/*
	 * If the alternate clock has not made itself known then
	 * we must gather the statistics.
	 */
	if (!nmi_link)
		gatherstats(pc, ps);

#endif /* not FSD_KI */

	/*
	 * Increment the time-of-day.
	 * Unlike the VAX, softclock() is called separately for
	 * processing of the callouts.
	 */
	bumptime(&time, elapsed_usec);
	lbolt += ticks;
	ticks_since_boot += ticks;
	/*
	 * Approximate about a one-second moving average of the number of
	 * faulting processes, updated about six times per second.
	 */
	if ((ticks_since_boot & 0x7) == 0) {
		if (ave_num_fault_procs > FAULTCNTPERPROC * num_fault_procs)
			ave_num_fault_procs = (7 * ave_num_fault_procs +
				       FAULTCNTPERPROC * num_fault_procs) >> 3;
		else
			ave_num_fault_procs = (7 * ave_num_fault_procs + 7 + 
				       FAULTCNTPERPROC * num_fault_procs) >> 3;
	}
#ifdef FSD_KI
	KI_hardclock(pc, cpstate);	/* tell the KI that clock ticked */
#endif FSD_KI

	if (activity_led_enb)
		display_led_activity();
}

#ifdef GPROF
nmi_kcounts(pc, ps)
caddr_t	pc;
int	ps;
{
	register s;

	if (!USERMODE(ps)) {
		s = pc - s_lowpc;
		if (profiling < 2 && s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof(*kcount))]++;
	}
}
#endif /* GPROF */

#ifndef	FSD_KI
/*
 * Gather statistics on resource utilization.
 *
 * We make a gross assumption: that the system has been in the
 * state it is in (user state, kernel state, interrupt state,
 * or idle state) for the entire last time interval, and
 * update statistics accordingly.
 */

/*ARGSUSED*/
gatherstats(pc, ps)
	caddr_t pc;
	int ps;
{
	int cpstate, s;

	/*
	 * Determine what state the cpu is in.
	 */
	if (USERMODE(ps)) {
		/*
		 * CPU was in user state.
		 */
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	}
	else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.
		 */
		if (noproc && BASEPRI(ps))
			cpstate = CP_IDLE;
		else
			cpstate = CP_SYS;
	}

	/*
	 * We maintain statistics shown by user-level statistics
	 * programs:  the amount of time in each cpu state, and
	 * the amount of time each of DK_NDRIVE ``drives'' is busy.
	 */
	cp_time[cpstate]++;
	for (s = 0; s < DK_NDRIVE; s++)
		if (dk_busy&(1<<s))
			dk_time[s]++;
}
#endif not FSD_KI

/*
 * Software priority level clock interrupt.
 * Run periodic events from timeout queue.
 */
/*ARGSUSED*/
softclock(pc, ps, ticks)
	caddr_t pc;
	int ps;
	long ticks;
{
	for (;;) {
		register struct callout *p1;
		register caddr_t arg;
		register int (*func)();
		register int a, s;

		s = spl7();
		if ((p1 = calltodo.c_next) == 0 || p1->c_time > 0) {
			splx(s);
			break;
		}
		arg = p1->c_arg; func = p1->c_func; a = p1->c_time;
		calltodo.c_next = p1->c_next;
		p1->c_next = callfree;
		callfree = p1;
		splx(s);
		(*func)(arg, a);
	}
	/*
	 * If trapped user-mode or soft emulation soft and profiling, 
	 * give it a profiling tick.
	 */
	if (u.u_prof.pr_scale) 
	{
		extern soft_emulation_beg;
		extern soft_emulation_end;

		/* check if profiling ticks need to be charged */
		/* The soft_emulation stuff if for the 040+ processor */
		if (USERMODE(ps) || (pc >= (caddr_t)&soft_emulation_beg && 
			pc <  (caddr_t)&soft_emulation_end)) 
		{
			u.u_procp->p_flag |= SOWEUPC;
			aston();
		}
	}
}

/*
 * Bump a timeval by any number of usec's. while loop used for robustness.
 * extra test will incurred fairly infrequently.
 */
bumptime(tp, usec)
	register struct timeval *tp;
	int usec;
{
	tp->tv_usec += usec;
	while (tp->tv_usec >= 1000000) {
		tp->tv_usec -= 1000000;
		tp->tv_sec++;
	}
}

/*
 * Arrange that (*fun)(arg) is called in t/hz seconds.
 */
timeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	register int t;
{
	register struct callout *p1, *p2, *pnew;
	register int s = spl7();

	if (t == 0)
		t = 1;
	if (t < 0) {
		printf ("Timeout called with negative time.\n");
		printf ("Function == 0x%X, arg == 0x%X, t == 0x%X.\n",
			fun, arg, t);
		t = 1;
	}
	pnew = callfree;
	if (pnew == NULL)
		panic("timeout table overflow");
	callfree = pnew->c_next;
	pnew->c_arg = arg;
	pnew->c_func = fun;
	for (p1 = &calltodo; (p2 = p1->c_next) && p2->c_time < t; p1 = p2)
		if (p2->c_time > 0)
			t -= p2->c_time;
	p1->c_next = pnew;
	pnew->c_next = p2;
	pnew->c_time = t;
	if (p2)
		p2->c_time -= t;
	splx(s);
}

/*
 * untimeout is called to remove a function timeout call
 * from the callout structure.
 */
untimeout(fun, arg)
	int (*fun)();
	caddr_t arg;
{
	return(find_timeout(fun,arg,1));
}

/*
 *  find_timeout searches for a particular timeout on the timeout
 *  queue. It keeps track of the relative time to the timeout and
 *  returns that value if the timeout is located, otherwise it returns
 *  -1. find_timeout will also remove the timeout from the queue if
 *  removeflag is non-zero.
 */

int
find_timeout(fun, arg, removeflag)
	int (*fun)();
	caddr_t arg;
	int removeflag;
{
	register struct callout *p1, *p2;
	register int s;
	register int hzleft;
	register int hznext;

	hzleft = 0;
	s = spl7();
	for (p1 = &calltodo; (p2 = p1->c_next) != 0; p1 = p2) {

		if ((hznext = p2->c_time) > 0)
		    hzleft += hznext;

		if (p2->c_func == fun && p2->c_arg == arg) {
			if (removeflag) {
			    if (p2->c_next && p2->c_time > 0)
				    p2->c_next->c_time += p2->c_time;
			    p1->c_next = p2->c_next;
			    p2->c_next = callfree;
			    callfree = p2;
			}
			break;
		}
	}
	splx(s);

	if (p2 == (struct callout *)0)
	    return(-1);

	return(hzleft);
}

/*
 * Compute number of hz until specified time.
 * Used to compute third argument to timeout() from an
 * absolute time.
 */
hzto(tv)
	struct timeval *tv;
{
	register long ticks;
	register unsigned long sec;
	int s = spl7();

	/*
	 * If number of ticks will fit in 32 bit arithmetic,
	 * then compute it, rounding all partial ticks up to
	 * a full tick.  Round times greater than representible
	 * to maximum value.
	 */
	sec = tv->tv_sec - time.tv_sec;
	if (sec <= 0x7fffffff / HZ - HZ)
		ticks =
		    sec * HZ + (tv->tv_usec - time.tv_usec + tick - 1) / tick;
	else
		ticks = 0x7fffffff;
	splx(s);
	return (ticks);
}

profil()
{
	register struct a {
		short	*bufbase;
		unsigned bufsize;
		unsigned pcoffset;
		unsigned pcscale;
	} *uap = (struct a *)u.u_ap;
	register struct uprof *upp = &u.u_prof;

	upp->pr_base = uap->bufbase;
	upp->pr_size = uap->bufsize;
	upp->pr_off = uap->pcoffset;
	/* a scale of 1 is documented as turning profiling off */
	upp->pr_scale = (uap->pcscale == 1) ? 0 : uap->pcscale;
}

opause()
{
	for (;;)
		sleep((caddr_t)&u, PSLEP);
}

uniqtime(tv)
	register struct timeval *tv;
{
	static struct timeval last;
	static int uniq;

	while (last.tv_usec != time.tv_usec || last.tv_sec != time.tv_sec) {
		last = time;
		uniq = 0;
	}
	*tv = last;
	tv->tv_usec += uniq++;
}

struct callout *smart_poll;	/* head of smart poll list */

/*
** Arrange to have func(arg) called at various places in the kernel.
** Initially written to compensate for brain dead SCSI hardware which
** doesn't generate an interrupt when it should.
** It may be desirable to add a third argument which somehow specifies
** under what conditions func(arg) should be called, but it is currently
** unnecessary.
*/
set_smart_poll(func, arg)
int (*func)();
caddr_t arg;
{
	register struct callout *spn;
	register int x;

	x = CRIT();

	if ((spn = callfree) == NULL)
		panic("timeout table overflow");

	callfree = spn->c_next;
	spn->c_func = func;
	spn->c_arg = arg;
	spn->c_next = smart_poll;
	smart_poll = spn;

	UNCRIT(x);

	return(!spn);
}

do_smart_poll()
{
	register struct callout *spn, *todo;
	register int x;

	if (smart_poll == NULL)
		return;		/* fast exit */

	x = CRIT();
	todo = smart_poll;
	smart_poll = NULL;
	UNCRIT(x);

	while ((spn = todo) != NULL) {
		(*todo->c_func)(todo->c_arg);
		todo = todo->c_next;
		x = CRIT();
		spn->c_next = callfree;
		callfree = spn;
		UNCRIT(x);
	}
}
