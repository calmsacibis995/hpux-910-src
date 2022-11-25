/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_synch.c,v $
 * $Revision: 1.54.83.6 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 10:45:26 $
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

#ifdef MP
#define	WATCHDOG_CHASSIS
#endif /* MP */

#ifdef hp9000s800
#include "../machine/psl.h"
#include "../machine/spl.h"
#endif /* hp9000s800 */
#include "../h/debug.h"
#include "../h/assert.h"
#include "../h/param.h"
#ifdef MP
#include "../machine/reg.h"
#include "../machine/inline.h"
#include "../machine/mp.h"
#include "../machine/cpu.h"
#include "../h/kern_sem.h"
#endif /* MP */
#include "../h/sleep.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/pfdat.h"
#ifdef hp9000s800
#include "../h/vnode.h"
#endif /* hp9000s800 */
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/dbg.h"
#ifdef SAR
#include "../h/sar.h"
#endif 

#ifdef FSD_KI
#include "../h/ki_calls.h"
#endif /* FSD_KI */

#ifdef KPREEMPTDEBUG
extern int nonrtkpreempt;	/* (defined in sys/kpreempt.c) */
#endif /* KPREEMPTDEBUG */

#define mask(sig)	(1 << (sig-1))


#ifdef MP
extern struct mpinfo mpproc_info[];
extern int mp_bias;

#ifdef WATCHDOG_CHASSIS
extern lock_t		live_proc_lock;
extern unsigned char	live_proc_count;
extern unsigned char	live_proc_map;
extern unsigned char	live_proc_cycle;
#define	LIVE_PROC_CYCLE	10
#endif /* WATCHDOG_CHASSIS */
#endif /* MP */

#if defined(__hp9000s300) && defined(RTPRIO_DEBUG)
#include "../h/rtprio.h"
#endif /* defined(__hp9000s300) && defined(RTPRIO_DEBUG) */

#ifdef FDDI_VM
extern int lcowfree_pending;
extern lost_page_t losthead;
extern void lcow_freepages();
#endif /* FDDI_VM */

/*
 * Force switch among equal priority processes every timeslice.
 */
extern timeslice;
roundrobin()
{
	register int t;
#ifdef MP
	register int i;
#endif /* MP */

	/* This means no timeslicing */
	if (timeslice == -1)
		t = hz;		/* check again in a while */
	else {
		runrun++;
		aston();
		t = timeslice;
#ifdef MP
#ifdef WATCHDOG_CHASSIS
		if (live_proc_cycle-- == 0) {
			live_proc_cycle = LIVE_PROC_CYCLE;

			spinlock(&live_proc_lock);
			live_proc_count = 1;
			live_proc_map = 1 << getprocindex();
			spinunlock(&live_proc_lock);
		}
#endif /* WATCHDOG_CHASSIS */

		/* signal the other processors to time slice as well */
		for (i = 1; i < MAX_PROCS; i++) {
			if (mpproc_info[i].prochpa)
				mpsched_set(mpproc_info[i].prochpa);
		}
#endif /* MP */
	}
	timeout(roundrobin, (caddr_t)0, t);
}

/*
 * sendlbolt sends wakes up all processes waiting on lbolt, and
 * a few other processes sleeping on some event.
 */
sendlbolt()
{
	wakeup((caddr_t)&lbolt);
	/*
	 *  There is a race between someone sleeping on phead and the 
	 *  wakeup done on the ICS when a page is freed.  In order
	 *  to keep us from having to lock down each of these wakeups
	 *  the race will go on, and every lbolt we will try to
	 *  pick up the strays.
	 */ 
	if (phead_cnt && FREEPAGES()) {
		wakeup((caddr_t)&phead);
	}
	timeout(sendlbolt, (caddr_t)0, hz);
}

/*
 * Table lookup is faster than floating point operations, so we keep
 * araound a list of the most common values.  We'll do the full
 * computation only for cases where the table doesn't go high enough.
 *
 * NOTE:  90% of the usage should decay away after 5*loadav_time.
 */
#undef ave
#define	NRSCALE	2.0
#define	CCPU	0.95122942450071400909		/* exp(-1/20) */
double	ccpu = CCPU;
double	one_minus_ccpu_over_hz = (1.0 - CCPU) / HZ;
#define	SIZE_OF_TABLE	15
#define	ONE_MINUS_CCPU_OVER_HZ	((1.0 - CCPU) / HZ)
static	double lookup[SIZE_OF_TABLE] = {
	0.0,
	ONE_MINUS_CCPU_OVER_HZ,
	ONE_MINUS_CCPU_OVER_HZ*2.0,
	ONE_MINUS_CCPU_OVER_HZ*3.0,
	ONE_MINUS_CCPU_OVER_HZ*4.0,
	ONE_MINUS_CCPU_OVER_HZ*5.0,
	ONE_MINUS_CCPU_OVER_HZ*6.0,
	ONE_MINUS_CCPU_OVER_HZ*7.0,
	ONE_MINUS_CCPU_OVER_HZ*8.0,
	ONE_MINUS_CCPU_OVER_HZ*9.0,
	ONE_MINUS_CCPU_OVER_HZ*11.0,
	ONE_MINUS_CCPU_OVER_HZ*12.0,
	ONE_MINUS_CCPU_OVER_HZ*13.0,
	ONE_MINUS_CCPU_OVER_HZ*14.0,
};

extern struct proc *shproc;

/*
 * Recompute process priorities, once per second.
 */
schedcpu()
{
	register struct proc *p;
	register int s, a;
	register int endchain;
	double ave0;
	u_int  ave0_uint;
#ifdef SAR
	static rqlen, sqlen;
#endif
#ifdef hp9000s800
	register short phx1;
	register struct proc *sproc;
	int kcount;
#ifdef MP
	int sx;
#endif
#endif /* hpd9000s800 */
#ifdef FSS
	struct fss *f;
	extern struct fss fsstab[];
	extern int active_shares, fssgrp;
#endif /* FSS */

#ifdef hp9000s800
	/*
	 * Get a dummy proc table entry to hold our place in case we
	 * are preempted.
	 */
	 SPINLOCK(activeproc_lock);
	 shproc = sproc = &proc[freeproc_list];
	 phx1 = freeproc_list;
	 freeproc_list = sproc->p_fandx;
	 SPINUNLOCK(activeproc_lock);
#endif /* hp9000s800 */

	/*
	 * We are a kernel daemon.  Loop forever.
	 */
loop:

#ifdef hp9000s800
	kcount = 0;
#endif /* hp9000s800 */
#ifdef SAR
	rqlen = 0;
	sqlen = 0;
#endif

#ifdef _WSIO
	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** This is the "kernel is alive" led.  It should be toggeled
	** (not blinked) once a second.  
	*/

	toggle_led(KERN_OK_LED);

#endif /* _WSIO */

	ave0 = avenrun[0]*NRSCALE;
	VASSERT(ave0 >= 0);
	ave0 = ave0/(ave0 + 1.0);
	ave0_uint = (u_int)(ave0*16777216.0);	/* times 2^24 */

	endchain = 0;

#ifdef MP
	sx = spl2();	/* XXX Only to avoid deadlock? */
#endif

	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(proc));

	for (p = proc; endchain == 0; p = &proc[p->p_fandx]) {
		if (p->p_fandx == 0)
			endchain = 1;
		if (p->p_time != 127)
			p->p_time++;
		if (p->p_stat==SSLEEP || p->p_stat==SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		if (p->p_flag&SLOAD) {
			p->p_pctcpu *= ccpu;
			if (p->p_cpticks < SIZE_OF_TABLE)
				p->p_pctcpu += (lookup[p->p_cpticks]);
			else
				p->p_pctcpu +=
					(one_minus_ccpu_over_hz * p->p_cpticks);
#if defined(hp9000s800) && !defined(TOPGUN_FPU_PROBLEM)
#define	THRESHOLD	0.000001
			/*
			 * The TOPGUN fpu generates assist exceptions for
			 * double-to-float conversions that underflow.
			 * The kernel currently handles this by panic'ing.
			 * The above computation is the only known cause
			 * of this in the kernel, so until the fix is put
			 * in, we'll just kludge around the problem.
			 */
			if (p->p_pctcpu < THRESHOLD)
				p->p_pctcpu = THRESHOLD;
#endif /* hp9000s800 && !TOPGUN_FPU_PROBLEM */
		}
		p->p_cptickstotal += p->p_cpticks;
		p->p_cpticks = 0;
		a = (int)(((u_int)((p->p_cpu & 0377)*ave0_uint)) >> 24) +
							p->p_nice - NZERO;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		(void) setpri(p);

#ifdef SAR
		if (p->p_stat == SRUN && ((p->p_flag & SSYS) == 0))
			if (p->p_flag & SLOAD)
				rqlen++;
			else
				sqlen++;
#endif 

		/*
		 * Prevent state changes.  Run queue and p_pri
		 * must be consistent for powerfail.
		 */
		s = UP_SPL7();
		if (p->p_pri >= PUSER) {
#ifdef MP
			/* Removed noproc, because it has to be 0 here.
			 * We are a process. */
			if ((!(p->p_mpflag & SRUNPROC)) &&
#else /* !MP */
			if ((p != u.u_procp || noproc) &&
#endif /* !MP */
			    p->p_stat == SRUN &&
			    (p->p_flag & SLOAD) &&
			    p->p_pri != p->p_usrpri) {
				remrq(p);
				p->p_pri = p->p_usrpri;
				setrq(p);
			} else
				p->p_pri = p->p_usrpri;
		}
		UP_SPLX(s);
#ifdef hp9000s800
		if (kcount++ >= 2) {
			kcount = 0;
			if (endchain == 1)
				break;
			/*
			 * link in dummy entry in case we are preempted
			 * (we will only do this every 3 times around)
			 * (restarting was bad for throughput)
			 */
			sproc->p_fandx = p->p_fandx;
			sproc->p_pandx = proc[p->p_fandx].p_pandx;
			proc[sproc->p_fandx].p_pandx = phx1;
			p->p_fandx = phx1;
			p = sproc;
			SPINUNLOCK(activeproc_lock);
			SPINUNLOCK(sched_lock);
			SPINUNLOCK(PROCESS_LOCK(p));
			KPREEMPTPOINT();
			SPINLOCK(activeproc_lock);
			SPINLOCK(sched_lock);
			SPINLOCK(PROCESS_LOCK(p));
			if (sproc->p_fandx == 0) {
				endchain = 1;
				proc[sproc->p_pandx].p_fandx = 0;
			}
			else {
				proc[sproc->p_pandx].p_fandx = sproc->p_fandx;
				proc[sproc->p_fandx].p_pandx = sproc->p_pandx;
			}
		}
#endif /* hp9000s800 */
	}

	SPINUNLOCK(activeproc_lock);
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(p));

#ifdef MP
	splx(sx);
#endif

#ifdef SAR
	if (rqlen) {
		runque += rqlen;
		runocc++;
	}
	if (sqlen) {
		swpque += sqlen;
		swpocc++;
	}

	KPREEMPTPOINT();
#endif 
#ifdef FSS
	/*
	 * Fair Share Scheduler counters are reset every second.
	 */
	for (f = &fsstab[0]; f < &fsstab[fssgrp]; f++)
		if (f->fs_flag&FS_INUSE) {
			f->fs_prev = (f->fs_ticks*active_shares)/hz;
			f->fs_usage += f->fs_prev;
			f->fs_pticks = f->fs_ticks;
			f->fs_ticks = 0;
			f->fs_active = 0;
		}
#endif /* FSS */

	vmmeter();
	{
		/*
		 * VM stats are computed every 5 seconds.
		 */
		static int times = 0;
		if (++times == 5) {
			vmtotal();
			times = 0;

#ifdef FDDI_VM
			/* 
			 * Call to FDDI VM code to clean up lost page list if 
			 * necessary.  Only make the call if there are entries
			 * on the lost page list and the number of completed 
			 * calls to lcowfree matches the number of calls to 
			 * lcow.  This indicates that pages are lost and
			 * the callers to lcow/lcowfree have no outstanding
			 * requests.  This should be a rare corner case.  
			 * lcow_freepages() will only get called after
			 * an equal number of lcow/lcowfree calls and when
			 * after the last lcow_freepages() call, one or more
			 * pages were still on the lost page list.
			 */
			if ((lcowfree_pending == 0) && 
			    (losthead.next != &losthead))
				lcow_freepages();
	
#endif /* FDDI_VM */

		}
	}

#ifdef	MP
	cvsync(&runin);
#else	/* ! MP */
	if (runin!=0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
#endif	/* ! MP */
	dbg(DBG_CHKRQS, chkrqs());

	/*
	 * Go to sleep for a second, then start over ...
	 */
	sleep((caddr_t)&lbolt, PSWP);
	goto loop;
}

#ifdef MP

sleep_semas_init()
{
	register int i;

	for (i=0; i<SQSIZE; i++)    /* For each sleep bucket */
	  initsync((sync_t *)&slpsem[ i ]);
}

#endif /* MP */

/*
 * Give up the processor till a wakeup occurs on chan,
 * at which time the process enters the scheduling queue at priority pri.
 *
 * Note that a signal will only be able to be disturbed if SSIGABL
 * will be set, and the setting of SSIGABL is dependent on the value of pri.
 *
 * The most important effect of pri is that when pri<=PZERO a signal
 * cannot disturb the sleep; if pri>PZERO signals will be processed.
 *
 * Note that for RTPRIO, a signal will only be able to be disturbed
 * if SSIGABL will be set, and the setting of SSIGABL is dependent
 * on the value of pri. Callers of this routine must be prepared for
 * premature return, and check that the reason for sleeping has gone away.
 */

#ifdef hp9000s800
#ifndef	MP
extern int *istackptr;		/* zero if running on interrupt stack */
#endif /* !MP */
#endif /* hp9000s800 */


#ifdef FSD_KI
/*ARGSUSED*/
_sleep(chan, disp, caller)
	caddr_t caller;
#else /* !FSD_KI */
sleep(chan, disp)
#endif /* !FSD_KI */
	caddr_t chan;
	int disp;
{
	register struct proc *rp, **hp;
#ifdef hp9000s800

	register struct proc **tp;
	int x;
#endif /* hp9000s800 */
	register s;
	register pri;

#ifdef MP
	sv_sema_t sv_sema;
	sv_lock_t nsp;
#endif /* MP */

	if (ON_ICS)
#ifdef FSD_KI
		panic("sleep called under interrupt, caller = 0x%x\n", caller);
#else /* !FSD_KI */
		panic("sleep called under interrupt");
#endif /* !FSD_KI */

	pri = disp & PMASK;
	rp = u.u_procp;

	SPINLOCKX(sched_lock, &nsp);
	s = UP_SPL7();

#ifdef	MP

#ifdef SYNC_SEMA_RECOVERY
	SD_ASSERT((rp->p_sema == 0) || (rp->p_sema->sa_next == 0),
			"sleep: Held more than one semaphore upon entry");
#else /* not  SYNC_SEMA_RECOVERY */
	release_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
#endif	/* MP */
	VASSERT(chan != 0);
	VASSERT(rp->p_stat == SRUN);
	VASSERT(rp->p_rlink == 0);
#ifndef MP
	rp->p_wchan = chan;
#endif /* !MP */

	rp->p_slptime = 0;
	if (rp->p_flag & SRTPROC)
		rp->p_pri = (pri < rp->p_rtpri) ? pri : rp->p_rtpri;
	else
		rp->p_pri = pri;
#ifndef	MP
	/* Queue on the sleep chain, FIFO fashion */
#ifdef __hp9000s300
	hp = &slpque[HASH(chan)];
	rp->p_link = *hp;
	*hp = rp;
#endif /* __hp9000s300 */
#ifdef hp9000s800
	hp = &slpque[x = HASH(chan)];
	tp = &slptl[x];
	if (*hp == NULL)
		*hp = *tp = rp;
	else {
		(*tp)->p_link = rp;
		(*tp) = rp;
	}
	rp->p_link = NULL;
#endif /* hp9000s800 */

	if (pri > PZERO) {
		rp->p_flag |= SSIGABL;
		if (ISSIG(rp)) {
			if (rp->p_wchan)
				unsleep(rp);
			rp->p_stat = SRUN;
#ifdef __hp9000s300
			(void) spl0();
#endif /* __hp9000s300 */
#ifdef hp9000s800
			(void) splnopreempt();	/* allow device interrupts */
#endif /* hp9000s800 */
			goto psig;
		}
		if (rp->p_wchan == 0)
			goto out;
		rp->p_stat = SSLEEP;

		/* do NOT allow device interrupts here (pdg 891010) */
		u.u_ru.ru_nvcsw++;
#ifdef FSD_KI
		_swtch(caller);
#else /* !FSD_KI */
		swtch();
#endif /* !FSD_KI */
		if (ISSIG(rp))
			goto psig;
	}
	else {
		rp->p_flag &= ~SSIGABL;
		rp->p_stat = SSLEEP;

		/* do NOT allow device interrupts here (pdg 890406) */
		u.u_ru.ru_nvcsw++;
#ifdef FSD_KI
		_swtch(caller);
#else /* !FSD_KI */
		swtch();
#endif /* !FSD_KI */
	}
out:
	splx(s);
	return(0);

	/*
	 * If priority was low (>PZERO) and there has been a signal,
	 * for HP-UX if PCATCH is set, return 1, else execute a
	 * non-local goto through u.u_qsave, aborting the system call
	 * in progress (see trap.c) (or finishing a tsleep, see below)
	 */
psig:
	splx(s);
	if (disp&PCATCH) {
		return(1);
	}
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
#else /* MP */

        if (pri > PZERO) {
#ifdef FSD_KI
            if (pchansync((sync_t *)&slpsem[HASH(chan)],
		P_CATCH_SIGNALS, chan, caller) == SEMA_SIGNAL_CAUGHT)
#else
            if (pchansync((sync_t *)&slpsem[HASH(chan)],
		P_CATCH_SIGNALS, chan) == SEMA_SIGNAL_CAUGHT)
#endif
                        goto psig;

	} else {
#ifdef FSD_KI
	    pchansync((sync_t *)&slpsem[HASH(chan)],
		      P_DEFER_SIGNALS, chan, caller);
#else
	    pchansync((sync_t *)&slpsem[HASH(chan)], P_DEFER_SIGNALS, chan);
#endif
	}

	UP_SPLX(s);
#ifndef SYNC_SEMA_RECOVERY
	reaquire_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
	return(0);

	/*
	 * If priority was low (>PZERO) and there has been a signal,
	 * for HP-UX if PCATCH is set, return 1, else execute a
	 * non-local goto through u.u_qsave, aborting the system call
	 * in progress (see trap.c) (or finishing a tsleep, see below)
	 */
psig:
	UP_SPLX(s);
	if (disp&PCATCH) {
#ifndef SYNC_SEMA_RECOVERY
		reaquire_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
		return(1);
	}
#ifdef SYNC_SEMA_RECOVERY
	release_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
#endif /* MP */
}

#ifdef MP
/*
 * Remove a process from its wait queue
 */
/*ARGSUSED*/
unsleep(p)
	register struct proc *p;
{
/*
 * This stub really does nothing. It is put here since
 * the there is a call to setrun (which calls unsleep) in
 * the file ns_socket.c. The addition of a no-op unsleep
 * allows that file to link in without unresolved references.
 */
 printf("Unsleep called\n");
}
#else /* ! MP */
/*
 * Remove a process from its wait queue
 */
unsleep(p)
	register struct proc *p;
{
	register struct proc **hp, *prev = NULL;
	register s, x;

	s = spl7();	/* Avoid powerfail hazards: slpque & p_wchan */
	/* DB_SENDRECV */
	dbunsleep(p);

	if (p->p_wchan) {
		hp = &slpque[x = HASH(p->p_wchan)];
		while (*hp != p)
			hp = &(prev = *hp)->p_link;
		*hp = p->p_link;
		p->p_wchan = 0;
#ifdef hp9000s800
		VASSERT(p->p_rlink == 0);
#ifdef FSS
		if (p->p_fss->fs_crate != 0)		/* if not idle-time */
			p->p_flag2 |= S2TRANSIENT;
#endif /* FSS */
		/* If removed tail, update tail pointer */
		if (p == slptl[x])
			slptl[x] = prev;
#endif /* hp9000s800 */
	}
	splx(s);
}
#endif /* ! MP */

#if	defined(SEMAPHORE_DEBUG)
#define	DO_PREVPC
#endif	/* defined(SEMAPHORE_DEBUG) */

#ifdef	DO_PREVPC
#define	PREVPC(var, uwptr)	var = prevpc(uwptr)
void	*prevpc();
void	*sleep_lock_unw = 0;
#else	/* ! DO_PREVPC */
#define	PREVPC(var, uwptr)
#endif	/* ! DO_PREVPC */

unsigned int
sleep_lock()
{
#ifdef	MP
	spinlock(sched_lock);
	PREVPC(sched_lock->sl_lock_caller, &sleep_lock_unw);
	return 0;
#else	/* ! MP */
	return spl7();
#endif	/* ! MP */
}

#ifndef MP
/* this is the non-MP version, the MP version is in spinlock.h */
int
sleep_then_unlock(chan, disp, spl_level)
	caddr_t chan;
	int disp;
	unsigned int spl_level;
{
	register int retval;

	retval = sleep(chan, disp);
	splx(spl_level);
	return retval;
}
#endif

/*ARGSUSED*/
sleep_unlock(spl_level)
	unsigned int	spl_level;
{
#ifdef	MP
	spinunlock(sched_lock);
#else	/* ! MP */
	splx(spl_level);
#endif	/* ! MP */
}

#ifdef MP

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	caddr_t chan;
{
	unsigned int	s;
	sema_t *sleep_sema = &slpsem[HASH(chan)];

	s = UP_SPL7();
	while (vchansync((sync_t *)sleep_sema, chan) == SEMA_SUCCESS)
		continue;
	UP_SPLX(s);
}

#else /* ! MP */
wakeup(chan)
	caddr_t chan;
{
	register struct proc *p, **q, **h, *prev = NULL;
	register struct proc *awaken = NULL;
	int s, x;

	s = spl7();
	h = &slpque[x = HASH(chan)];

restart:
	/* find processes to awaken */
	for (q = h; p = *q; /* nothing */) {
		VASSERT(p->p_rlink == 0);
		VASSERT(p->p_stat == SSLEEP || p->p_stat == SSTOP);
		if (p->p_wchan == (caddr_t)chan) {
			p->p_wchan = 0;
			*q = p->p_link;
#ifdef hp9000s800
			if (p == slptl[x])
				slptl[x] = prev;
#endif /* hp9000s800 */
			p->p_slptime = 0;
			if (p->p_stat == SSLEEP) {
				/* Reverse LIFO list to give FIFO ordering of
				 * setrq() calls.  This fixes semaphore bug
				 * with more than two processes.  It's also
				 * more fair than the old method.
				 */
				p->p_link = awaken;
				awaken = p;
				goto restart;
			}
		}
		else {
			q = &p->p_link;
			prev = p;
		}
	}

	/* awaken listed processes */
	while (p = awaken) {
		awaken = p->p_link;	/* must do this before setrq */

		/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
		p->p_stat = SRUN;
		if (p->p_flag & SLOAD) {
			VASSERT(p->p_rlink == 0);
#ifdef FSS
			/* if not idle-time, make transient */
			if (p->p_fss->fs_crate != 0)
				p->p_flag2 |= S2TRANSIENT;
#endif /* FSS */
			setrq(p);
		}
		/*
		 * If two priorities are equal in the timeshare range,
		 * the current process's priority will drop when it
		 * leaves the kernel.  Various code (eg. semaphores)
		 * implicitly relies on this for fairness.  The same isn't
		 * true with realtime priorities, so we force the same
		 * behavior here.
		 */
		if ((p->p_pri < curpri) && (p->p_pri < CURPRI)) {
			runrun++;
			aston();
#if defined(KPREEMPTDEBUG) && defined(hp9000s800)
			if (nonrtkpreempt)
				preemptkernel();
				/* request kernel preemption */
#endif /* defined(KPREEMPTDEBUG) && defined(hp9000s800) */
		}

#ifdef hp9000s800
		/* If this is a realtime process with stronger
		 * priority than the current process then
		 * preempt the current process even if it's
		 * executing in the kernel.
		 */
		if ((p->p_flag & SRTPROC) &&
		    (!noproc) && (p->p_pri < u.u_procp->p_pri))
			preemptkernel();
#endif /* hp9000s800 */

		if ((p->p_flag&SLOAD) == 0) {
#ifdef	MP
			cvsync(&runout);
#else	/* ! MP */
			if (runout != 0) {
				runout = 0;
				wakeup((caddr_t)&runout);
			}
#endif	/* ! MP */
			wantin++;

			/*
			 * Raise priority of swapper to
			 * priority of rt process
			 * we have to swap in.
			 */
			if ((p->p_flag & SRTPROC) &&
					(proc[S_SWAPPER].p_pri > p->p_rtpri)) {
				(void)changepri(&proc[S_SWAPPER], p->p_rtpri);
			}
		}
		/* END INLINE EXPANSION */
	}
	splx(s);
}
#endif /* ! MP */

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
rqinit()
{
	register int i;

	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
setrun(p)
	register struct proc *p;
{
	register int s;
        register u_int reg_temp;
#ifdef MP
	int noproc;
	u_char curpri;

	noproc = GETNOPROC(reg_temp);
        curpri= GETCURPRI(reg_temp);
#endif /* MP */

	/*
	 * Avoid powerfail hazards.  Run queue must be consistent with
	 * p_stat for powerfail.
	 */
	s = spl7();

	switch (p->p_stat) {
	case SRUN:
#ifdef hp9000s800
		/* Check for case where we got a powerfail between
		   decision to call setrun and above spl7.  If so,
		   return; else, panic.
		 */
#ifdef MP
		/* If its runnable, then we don't have to do anything */
		splx(s);
		return;
#else /* !MP */
		if (p->p_sig & mask(SIGPWR)) {
			splx(s);
			return;
		}
#endif /* !MP */
		/* else fall through */
#endif /* hp9000s800 */
	case 0:
	case SWAIT:
	case SZOMB:
	default:
		panic("setrun");
		break;
	case SSTOP:
	case SSLEEP:
#ifdef MP
		if (p->p_wchan) {
			wakeup(p->p_wchan);
			splx(s);
			return;
		}
#else /* !MP */
		unsleep(p);		/* e.g. when sending signals */
#endif /* !MP */
		break;
	case SIDL:
		break;
	}
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD) {
		setrq(p);
	}
	splx(s);
#ifdef MP
	if ((p->p_pri < (curpri + mp_bias)) && (p->p_pri < CURPRI)) {
#else /* !MP */
	if ((p->p_pri < curpri) && (p->p_pri < CURPRI)) {
#endif /* !MP */
		runrun++;
		aston();
#if defined(KPREEMPTDEBUG) && defined(hp9000s800)
		if (nonrtkpreempt)
			preemptkernel();  /* request kernel preemption */
#endif /* defined(KPREEMPTDEBUG) && defined(hp9000s800) */
	}

#ifdef hp9000s800
	/* If this is a realtime process with stronger priority than the
	 * current process then preempt the current process even if it's
	 * executing in the kernel.
	 */
	if ((p->p_flag & SRTPROC) && (!noproc) && (p->p_pri < u.u_procp->p_pri))
		preemptkernel();
#endif /* hp9000s800 */

	if ((p->p_flag&SLOAD) == 0) {
#ifdef	MP
		cvsync(&runout);
#else	/* ! MP */
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;

		/*
		 * Raise priority of swapper to priority of rt process
		 * we have to swap in.
		 */
		if ((p->p_flag & SRTPROC) &&
					(proc[S_SWAPPER].p_pri > p->p_rtpri))
			(void)changepri(&proc[S_SWAPPER], p->p_rtpri);
	}
}

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 */

setpri(pp)
	register struct proc *pp;
{
	register int p;
        register u_int reg_temp;
#ifdef MP
	int noproc;
	u_char curpri;

	noproc = GETNOPROC(reg_temp);
        curpri = GETCURPRI(reg_temp);
#endif /* MP */

	if (pp->p_flag & SRTPROC)		/* RT processes */
		p = pp->p_rtpri;
	else {					/* TS processes */
		p = ((pp->p_cpu & 0377) >> 2);
		p += PUSER + 2*(pp->p_nice - NZERO);
		if (PRSSIZE_APPROX(pp) > pp->p_maxrss && freemem < desfree)
			p += 8;		/* effectively, nice(4) */
		if (p > PMAX_TIMESHARE)
			p = PMAX_TIMESHARE;
	}
#ifdef MP
	if ((p < (curpri + mp_bias)) && (p < CURPRI)) {
#else /* !MP */
	if ((p < curpri) && (p < CURPRI)) {
#endif /* !MP */
		runrun++;
		aston();
#if defined(KPREEMPTDEBUG) && defined(hp9000s800)
		if (nonrtkpreempt)
			preemptkernel();  /* request kernel preemption */
#endif /* defined(KPREEMPTDEBUG && defined(hp9000s800 */
	}

#ifdef hp9000s800
	/* If this is a realtime process with stronger priority than the
	 * current process then preempt the current process even if it's
	 * executing in the kernel.
	 */
	if ((pp->p_flag & SRTPROC) && (!noproc) && (p < u.u_procp->p_pri))
		preemptkernel();
#endif /* hp9000s800 */

	pp->p_usrpri = p;
	return p;
}

#if defined(__hp9000s300) && defined(RTPRIO_DEBUG)
/*
 * test the (doubly-linked) run queues
 * to be empty down to the priority passed
 * in.  Normally called from trap or syscall
 */
rqtest(pri, n)
	int pri, n;
{
	register int i;

	if (higherqs(pri))
		printf("higherqs mismatch %2d !! \n", 0);
	for (i = 0; (i+1) < pri/4; i++)
		if (qs[i].ph_link != qs[i].ph_rlink) {
			printf("rqtest: curpri %3d procpri %3d occupri %3d n %1d \n ",
				curpri,pri,i*4,n);
			printf("curproc = %x queueproc = %x \n",
				u.u_procp,qs[i].ph_link);
			if (higherqs(pri))
				printf("higherqs mismatch %2d !! \n", i);
		}
}
#endif /* __hp9000s300 && RTPRIO_DEBUG */

/*
 * Change the priority of a process (often the scheduler or swapper).
 *
 * The previous priority is returned in case the caller wants to restore
 * it later.
 */

int
changepri(p, pri)
	struct proc *p;		/* process to adjust */
	register int pri;	/* new priority */
{
	register int x;
	int oldpri;		/* the priority before we changed it */

#ifdef MP
	sv_lock_t nsp_sched, nsp_proc;
#endif /* MP */

	VASSERT((pri == PNOT_REALTIME) || (pri >= 0));

	/*
	 * Since proc[S_SWAPPER] can be made runnable by a powerfail,
	 * we must insure that p_pri and run queues are consistent.
	 * (Also in powerfail recovery path.)
	 */
	x = UP_SPL7();

	SPINLOCKX(sched_lock,&nsp_sched);
	SPINLOCKX(PROCESS_LOCK(p),&nsp_proc);

	oldpri = p->p_pri;
	if (p->p_stat==SRUN && (p->p_flag & SLOAD) &&
#ifdef MP
	   (!(p->p_mpflag & SRUNPROC))) {
#else
	   ((noproc || p != u.u_procp))) {
#endif
		remrq(p);
		fiddle_pri(p, pri);
		setrq(p);
	}
	else
		fiddle_pri(p, pri);

	SPINUNLOCKX(sched_lock, &nsp_sched);
	SPINUNLOCKX(PROCESS_LOCK(p), &nsp_proc);
	UP_SPLX(x);

	return oldpri;
}

/*
 * Code factored out of changepri().
 * You may take the static declaration as a hint that this routine is not
 * for general use.
 */

static
fiddle_pri(p, pri)
struct proc *p;
int pri;
{
	VASSERT(p->p_rlink == 0);	/* better not be on a run queue */
	T_SPINLOCK(sched_lock);
	T_SPINLOCK(PROCESS_LOCK(p));

	if (pri == PNOT_REALTIME) {
		p->p_flag &= ~SRTPROC;
		setpri(p);
	}
	else {
		if (pri < PTIMESHARE) {
			p->p_flag |= SRTPROC;
			p->p_rtpri = pri;
		}
		else
			p->p_flag &= ~SRTPROC;

		p->p_usrpri = pri;
		p->p_pri = pri;
	}
}

/*
 * Raise the priority of a process while it is in the kernel.
 *
 * We don't want to deal with complicated things we know aren't an issue
 * here.  We know we will not make anyone realtime by this call.  We
 * also know that we don't want to change the user priority.  For these
 * reasons we are not calling changepri().
 */

void
raise_kernel_pri(p, pri)
	struct proc	*p;
	register int	pri;
{
#ifdef	OSDEBUG
	register u_int	reg_temp;
#endif	/* OSDEBUG */

	VASSERT((pri >= PTIMESHARE) && (pri <= PMAX_TIMESHARE));
	VASSERT((!ON_ICS) && (!GETNOPROC(reg_temp)) && (p == u.u_procp));
	VASSERT((p >= proc) && (p < procNPROC));
	T_SPINLOCK(sched_lock);
	/*
	 * If we would be making this process' kernel priority stronger,
	 * change it.
	 */
	if (pri < p->p_pri) {
		p->p_pri = pri;
	}
}
