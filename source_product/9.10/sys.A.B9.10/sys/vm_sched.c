/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_sched.c,v $
 * $Revision: 1.53.83.6 $       $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/12/15 17:41:14 $
 */
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/sema.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/var.h"
#include "../h/kernel.h"
#include "../h/vfd.h"
#include "../h/vas.h"
#include "../h/sar.h"
#include "../h/pfdat.h"
#include "../h/debug.h"
#include "../h/vnode.h"
#ifdef	MP_SDTA
#include "../machine/sdta.h"
#else	/* ! MP_SDTA */
#define	SDTA_SUSPEND()
#define	SDTA_RESUME()
#endif	/* ! MP_SDTA */
#include "../h/dk.h"

/*
 * The following parameters control operation of the page replacement
 * algorithm.  They are initialized to 0, and then computed at boot time
 * based on the size of the system.  If they are patched non-zero in
 * a loaded vmunix they are left alone and may thus be changed per system
 * using adb on the loaded system.
 */
int	minfree = 0;
int	desfree = 0;
int	lotsfree = 0;
int	multprog = -1;		/* so we don't count process 2 */
				/* ?? multprog is not used anywhere */
/*
 * Setup the paging constants for the clock algorithm.
 * Called after the system is initialized and the amount of memory
 * and number of paging devices is known.
 *
 * Threshold constants are defined in ../machine/vmparam.h.
 */
setmemthresholds()
{
	/*
	 * Setup memory thresholds:
	 *	lotsfree	is the threshold where paging daemon turns on
         *       		and whose default value will not exceed 2Mb.
	 *	desfree		is amount of memory desired free.  if less
	 *			than this for extended period, do swapping
	 *	minfree		is minimal amount of free memory which is
	 *			tolerable.
	 */
        if (lotsfree <= 0) {
                lotsfree = freemem / LOTSFREEFRACT;
                if (lotsfree > (LOTSFREEMAX/NBPG))
                        lotsfree = (LOTSFREEMAX/NBPG);
        }
	if (desfree <= 0) {
           	desfree = DESFREE / NBPG;
           	if (desfree > freemem / DESFREEFRACT)
               		desfree = freemem / DESFREEFRACT;
	}
	if (minfree <= 0) {
		minfree = MINFREE / NBPG;
		if (minfree > desfree / MINFREEFRACT)
			minfree = desfree / MINFREEFRACT;
	}
}

extern int gpgslim;
extern int parolemem;
extern int pageoutrate, vhandrunrate, prepage;

/*
 * These are not modified by the system, they are "input", and may be patched.
 */
#ifdef hp9000s800
int swrt = 1;			/* non-zero keeps swapper running real time */
#endif
int maxslp = MAXSLP;		/* see comment for MAXSLP */
int deact_num_faults = FAULTCNTPERPROC * 5/4;
				/* fault rate trigger for low throughput */
int deact_idle_pcnt = 75;	/* idle percent trigger for low throughput */
int nice_adjust = 8;		/* p_nice coefficient for proc. selectors */
int mem_adjust = 4;		/* memory size coefficient for proc. selectors*/
#ifdef	MP
int loop_cnt_lim = 20;		/* number of loops before giving up locks */
#endif	/* MP */

/*
 * These are modified by the system to track it and change it.
 */
int deficit = 0;	/* expected memory changes caused by swapper */
int fault_deficit = 0;	/* expected faulting changes caused by swapper */
int avefree;		/* moving average of freemem */
int paging = 0;		/* non-zero if we are paging */
double avenrun[3];	/* load average: number of "running" processes */
int num_fault_procs = 0;/* number of processes faulting at a specific time */
int ave_num_fault_procs = 0;	/* smoothed average of num_fault_procs */
int ave_idle_pcnt = 0;		/* average percent of time spent idle */

/* time interval for smoothing freemem */
#define AVEFREEINT 5

/* insure non-zero */
#define	nz(x)	(x != 0 ? x : 1)

#define DEACMASK (SSYS|SLOCK|SWEXIT|SRTPROC|SLOAD|SSTOPFAULTING)
/*  Under what conditions can we deactivate? */
#ifdef MP
#define deactivatable(p) ((((p)->p_flag&(DEACMASK))==SLOAD) \
			    && (((p)->p_mpflag & (SMP_SEMA_NOSWAP)) == 0) \
			    && ((p)->p_stat!=SIDL) \
			    && !mlockedpreg(p))
#else	/* ! MP*/
#define deactivatable(p) ((((p)->p_flag&(DEACMASK))==SLOAD) \
			    && ((p)->p_stat!=SIDL) \
			    && !mlockedpreg(p))
#endif	/* ! MP */

#define swappable(p) (!((p)->p_flag & SLOAD) && \
		      !((p)->p_flag & SSWAPPED) && (p)->p_slptime > maxslp/4)

#define light_sleeper(p) ((p)->p_stat == SSTOP || \
			  (p)->p_stat == SSLEEP && ((p)->p_flag & SSIGABL))

/*
 * The primary purpose of process swapper (sched) is to limit
 * competition for memory by controlling the number of running processes
 * (deactivation).  Its secondary purpose is to maximize the memory
 * available by sending to disc memory in deactivated processes that
 * process vhand will not page out.
 *
 * Deactivation is done either by removing the process from the run
 * queue, or by asking the process to go to sleep at its next fault.
 * Thus, deactivation does not involve I/O.  Deactivated processes will
 * be paged out by vhand(), not sched().  The only I/O sched does is to
 * get rid of memory vhand() does not yet know how to get rid of (mainly
 * the u-area).
 *
 * sched() is the main loop.  Usually, the basic idea is to:
 *	deactivate dead wood;
 *	see a process wants to be activated;
 *	deactivate processes until there is room;
 *	activate the process;
 *	repeat.
 * but if we're desperate for memory or throughput, then don't consider 
 * activating anyone, just look for someone to swap out.  
 *
 * Deficits are kept as decisions are made, so that the next decision is 
 * based on both the current state of the system and on the direction the 
 * system will be moving due to recent decisions.
 *
 * The runout flag is set whenever someone is swapped out.  Sched sleeps
 * on it awaiting work.
 *
 * Sched sleeps on runin whenever it cannot find enough core (by
 * swapping out or otherwise) to fit the selected swapped process.  It
 * is awakened when the core situation changes and in any case once per
 * second.
 */
sched()
{
	int deserve_in;
	int desperate, plan_desperate;

	vmemp_lock();
loop:

#ifdef hp9000s800
	SPINLOCK(sched_lock);
	if (swrt && (proc[S_SWAPPER].p_flag&SRTPROC) == 0)
		(void)changepri(&proc[S_SWAPPER], PTIMESHARE-1);
	SPINUNLOCK(sched_lock);
#endif hp9000s800

	wantin = 0;
	deserve_in = 0;

	/*
	 * Set desperate if we're sure we need to deactivate a process.
	 * Conditions for this are either average memory is too low,
	 * or we are thrashing (page competition so high that CPU is 
	 * under utilized).
	 *
	 * Note that no matter how bad the system looks, we don't
	 * want to deactivate the only process that's really running.
	 */
	desperate =      (avefree < minfree ||
		          paging && ave_idle_pcnt > deact_idle_pcnt) &&
		         ave_num_fault_procs > deact_num_faults;

	plan_desperate = (avefree - deficit < minfree ||
		          paging && ave_idle_pcnt > deact_idle_pcnt) &&
		         ave_num_fault_procs + fault_deficit > deact_num_faults;

	deactivate_sleepers();

	if (!desperate && !plan_desperate) {
		int actret = activate(&deserve_in);

		if (actret == 0) 
			sleep_activate();
		if (actret >= 0)
			goto loop;
	}

	/* We're either desperate, or activate() failed, so deactivate.  */
	if (deactivate(deserve_in, desperate && plan_desperate) == 0)
		goto loop;

	/* Give the system a chance to change before checking again.  */
	sleep_deactivate();
	goto loop;
}

/*
 * Every 1/4 of maxslp, deactivate (and probably swap) any dead wood.
 * That is, process that have basically deactivated themselves by sleeping
 * for a while (maxslp).
 */
deactivate_sleepers()
{
#ifdef	MP
	int loop_cnt;
#endif	/* MP */
	int endchain;
	struct proc *rp;
	static int last_boot_ticks = 0;

	if ((ticks_since_boot - last_boot_ticks) / HZ < maxslp/4) {
		last_boot_ticks = ticks_since_boot;
		return;
	}
	last_boot_ticks = ticks_since_boot;

	if (avefree > lotsfree - AVEFREEINT)
		return;

restart:
#ifdef	MP
	loop_cnt = loop_cnt_lim;
#endif	/* MP */

	endchain = 0;

	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	for (rp = proc; endchain == 0; rp = &proc[rp->p_fandx]) {
#ifdef	MP
		/*
		 *  Give up lock every `N' times through the loop.
		 *  We only need to restart if the process we have
		 *  chosen is either not in use or a zombie;
		 *  otherwise we are on the active chain and OK.
		 *  If we restarted every time, it could easily
		 *  happen that we would never make any forward
		 *  progress.  
		 *
		 *  This code is appears in two other places, maybe we
		 *  need a foreach_proc(action()) function.
		 */
		if (--loop_cnt == 0) {
			SPINUNLOCK(sched_lock);
			SPINLOCK(sched_lock);
			if ((rp->p_stat == NULL) || (rp->p_stat == SZOMB)) {
				SPINUNLOCK(sched_lock);
				SPINUNLOCK(activeproc_lock);
				goto restart;
			}
			loop_cnt = loop_cnt_lim;
		}
#endif	/* ! MP */

		if (rp->p_fandx == 0)
			endchain = 1;

		if ((rp->p_stat == SSLEEP || rp->p_stat == SSTOP) &&
		    deactivatable(rp) && rp->p_slptime > maxslp) {
			/* 
			 * This is dead wood.  Deactivate it, so we can
			 * we can swap-out non-pageable stuff.
			 */
			try_deactivate_proc(rp);
		}
		if (swappable(rp)) {
			SPINUNLOCK(sched_lock);
			SPINUNLOCK(activeproc_lock);
			swapout(rp);
			goto restart;
		}
	}
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);
}

#define MINSWAPPRI -200000000

/*
 * Look for processes to activate.  They must be deactivated but runnable.
 * The most deserving one is found, but only brought in if conditions
 * are favorable.  If it was deserving enough, but we did not bring it in,
 * set deserve_in to give deactivate() extra incentive.
 *
 * Returns
 *    -1 a process wants to be activated, but we didn't activate it
 *     0 no-one wants to be activated so we did nothing
 *     1 activated a process
 */
activate(deserve_in)
	int *deserve_in;
{
#ifdef	MP
	int loop_cnt;
#endif	/* MP */
	int endchain;	/* Used for kpreemption control */
	struct proc *p; /* The best process to swap in we have found so far */
	int outpri;	/* The "inswap" priority of *p */
	struct proc *rp; /* The current process we are looking at
			  * and considering to swap in */
	int rppri;	/* The "inswap" priority of *rp */
	int needs;	/* How much space *p needs in memory */
	int divisor;

restart:
#ifdef	MP
	loop_cnt = loop_cnt_lim;
#endif	/* MP */

	outpri = MINSWAPPRI;
	endchain = 0;

	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	for (rp = proc; endchain == 0 ; rp = &proc[rp->p_fandx]) {
#ifdef	MP
		/*  See comment in deactivate_sleepers().  */
		if (--loop_cnt == 0) {
			SPINUNLOCK(sched_lock);
			SPINLOCK(sched_lock);
			if ((rp->p_stat == NULL) || (rp->p_stat == SZOMB)) {
				SPINUNLOCK(sched_lock);
				SPINUNLOCK(activeproc_lock);
				goto restart;
			}
			loop_cnt = loop_cnt_lim;
		}
#endif	/* ! MP */

		if (rp->p_fandx == 0)
			endchain = 1;

		if (rp->p_stat == SRUN && !(rp->p_flag & SLOAD) ||
					(rp->p_flag & SSTOPFAULTING)) {
		    /* This is a candidate to activate.  */
		    rppri = rp->p_time + rp->p_slptime - 
			    mem_adjust * deact_prssize(rp) / nz(pageoutrate) -
			    nice_adjust * (rp->p_nice - NZERO);
		    if (rppri > outpri) {
			    p = rp;
			    outpri = rppri;
		    }
		}
	}
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);

	if (outpri == MINSWAPPRI)
		/* No one needs to be activated. */
		return 0;

	SPINLOCK(sched_lock);
	if ((p->p_flag & SSTOPFAULTING) && (p->p_flag & SLOAD)) {
		/* Then we told this process to deactivate, but it hasn't
		 * done so yet, so nothing to activate, just undo our telling.
		 */
		p->p_flag &= ~SSTOPFAULTING;
	}
	if (p->p_flag & SLOAD) {
		/* This process is active with no plans to deactivate.  
		 * Just take credit for an activation.
		 */
		SPINUNLOCK(sched_lock);
		return 1;
	}
	/* Process is deactivated, so we no longer need sched_lock. */
	SPINUNLOCK(sched_lock);

	/*
	 * Decide how deserving this guy is.  If he is deserving
	 * we will be willing to work harder to bring him in.
	 * Needs is an estimate of how much core he will need.
	 * If he has been out for a while, then we will
	 * bring him in with 1/2 the core he will need, otherwise
	 * we are conservative.
	 */
	if (outpri <= maxslp/2 && p->p_nice <= PZERO && p->p_time == 127) {
		/* According to the rppri formula, this guy does not deserve 
		 * in.  This if-test therefore implies he never will.  Fake it.
		 *
		 * NOTE: we need a bigger p_time field to avoid this kludge.
		 */
		 outpri = maxslp/2 + 1;
	}
	*deserve_in = 0;
	divisor = 1;
	/* for Real time processes, outpri is always > maxslp/2 */
	if (outpri > maxslp/2) {
		*deserve_in = 1;
		divisor = 2;
	} else if (paging && 
		 ave_num_fault_procs > deact_num_faults - FAULTCNTPERPROC) {
		/* 
		 * This process does not yet deserve in, and activating it
		 * is likely to re-start thrashing.
		 */
		return -1;
	}

	needs = imin(deact_prssize(p) - prssize(p), gpgslim);
	if (avefree - deficit > needs / divisor) {
		deficit += needs;
		fault_deficit += FAULTCNTPERPROC;
		if (activate_proc(p))
			return 1;
		deficit -= MIN(needs, deficit);
		fault_deficit -= MIN(FAULTCNTPERPROC, fault_deficit);
	}

	return -1;
}

sleep_activate()
{
	int x;

	/* Setting and sleeping on runout must be atomic for powerfail.  */
	x = UP_SPL7();
	if (wantin) {
		wantin = 0;
		vmemp_unlock();
		sleep((caddr_t)&lbolt, PSWP);
		vmemp_lock();
	} else {
		/* 
		 * The swapper may have been set to be a RT process by setrun 
		 * or wakeup.  If so, turn it off.  Notice that we  do notturn 
		 * it off if someone wants in.
		 */
		(void)changepri(&proc[S_SWAPPER], PTIMESHARE);
#ifndef	MP
		runout++;
#endif	/* MP */
		vmemp_unlock();		/* free up VM empire */
#ifdef	MP
		psync(&runout, P_DEFER_SIGNALS);
#else	/* ! MP */
		sleep((caddr_t)&runout, PSWP);
#endif	/* ! MP */
		vmemp_lock();
	}
	UP_SPLX(x);
}

sleep_deactivate()
{
	int x = spl6();

#ifndef	MP
	runin++;
#endif	/* ! MP */
	vmemp_unlock();		/* free up VM empire */
#ifdef	MP
	psync(&runin, P_DEFER_SIGNALS);
#else	/* ! MP */
	sleep((caddr_t)&runin, PSWP);
#endif	/* ! MP */
	vmemp_lock();		/* lock down VM empire */
	splx(x);
}

#define	MAXNBIG	10
int	nbig = 4;

struct bigp {
	struct	proc *bp_proc;
	int	bp_pri;
	struct	bigp *bp_link;
} bigp[MAXNBIG], bplist;

deactivate(deserve_in, hard)
	int deserve_in;
	int hard;
{
	struct proc *rp,*p;
	int sleeper, tsleeper;
	int nextbig;
	int endchain;
	int inpri, rppri;
#ifdef	MP
	int loop_cnt = loop_cnt_lim;
#endif	/* MP */

	register struct bigp *bp, *nbp;

	/*
	 * Need resources (kernel map or memory), swap someone out.
	 * Select the nbig largest jobs, then the oldest of these
	 * is ``most likely to get booted.''
	 */
	if (nbig > MAXNBIG)
		nbig = MAXNBIG;
	if (nbig < 1)
		nbig = 1;
	nextbig = 0;
	bplist.bp_link = 0;

	sleeper = 0;
	endchain = 0;
	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	for (rp = proc; endchain == 0; rp = &proc[rp->p_fandx]) {
		if (rp->p_fandx == 0)
			endchain = 1;
#ifdef	MP
		/*  See comment in deactivate_sleepers().  */
		if (--loop_cnt == 0) {
			SPINUNLOCK(sched_lock);
			SPINLOCK(sched_lock);
			if ((rp->p_stat == NULL) || (rp->p_stat == SZOMB)) {
				SPINUNLOCK(sched_lock);
				SPINUNLOCK(activeproc_lock);
				return 0;
			}
			loop_cnt = loop_cnt_lim;
		}
#endif	/* ! MP */

		MP_ASSERT( (rp->p_stat != SZOMB),
			"deactivate: unexpected SZOMB state");

		if (!deactivatable(rp))
			continue;
		if (rp->p_stat==SZOMB)
			continue;

		if (rp->p_slptime > maxslp && light_sleeper(rp)) {
			tsleeper = rp->p_slptime + nice_adjust * rp->p_nice;
			if (sleeper < tsleeper) {
				p = rp;
				sleeper = tsleeper;
			}
		} else if (!sleeper && (rp->p_stat==SRUN||rp->p_stat==SSLEEP)) {
			rppri = prssize(rp);
			if (nextbig < nbig)
				nbp = &bigp[nextbig++];
			else {
				nbp = bplist.bp_link;
				if (nbp->bp_pri > rppri)
					continue;
				bplist.bp_link = nbp->bp_link;
			}
			for (bp = &bplist; bp->bp_link; bp = bp->bp_link)
				if (rppri < bp->bp_link->bp_pri)
					break;
			nbp->bp_link = bp->bp_link;
			bp->bp_link = nbp;
			nbp->bp_pri = rppri;
			nbp->bp_proc = rp;
		}
	}
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);
	if (!sleeper) {
		p = NULL;
		inpri = MINSWAPPRI;
		for (bp = bplist.bp_link; bp; bp = bp->bp_link) {
			rp = bp->bp_proc;
			rppri = rp->p_time + (rp->p_nice - NZERO);
			if (rppri >= inpri) {
				p = rp;
				inpri = rppri;
			}
		}
	}
	/*
	 * If we found a long-time sleeper, or we plan to deactivate and
	 * found someone to deactivate, or if someone deserves to come
	 * in and we didn't find a sleeper, but found someone who
	 * has been in core for a reasonable length of time, then
	 * deactivate them.
	 */
	if (sleeper || (hard && p) || (deserve_in && inpri > maxslp)) {
		SPINLOCK(sched_lock);
#ifdef	MP
		if (p->p_stat == NULL || p->p_stat == SZOMB ||
		    !deactivatable(p)) {
			SPINUNLOCK(sched_lock);
			return 0;
		}
#endif	/* MP */
		try_deactivate_proc(p);
		SPINUNLOCK(sched_lock);
		deficit -= imin(prssize(p), gpgslim);
		fault_deficit -= FAULTCNTPERPROC;
	}

	return 1;
}

/* POWER FAIL REVISIT:  before comments claimed "SLOAD, p_stat, and run queue 
 * must be consistent for powerfail.", with appropriate code under
 * UP_SPL7().  This is no longer clear with new meaning of SLOAD.
 */

/* 
 * try_deactivate_proc() and deactivate_proc() need to be called with 
 * sched_lock still held from figuring out that a deactivation makes sense.
 */
try_deactivate_proc(p)
	struct proc *p;
{
	if ((p->p_flag & (SKEEP|SPHYSIO)) == 0 && p->p_reglocks == 0 &&
	    light_sleeper(p)) {
		/* SLOAD, p_stat and run-queue must be consistent for 
		 * powerfail. */
		if (p->p_stat == SRUN)
			remrq(p);
		if (p->p_flag & SFAULTING)
			--num_fault_procs;
		deactivate_proc(p);
	} else {
		p->p_flag |= SSTOPFAULTING;
		/* The process will call deactivate_proc() itself when it
		 * stops faulting.  This should occur on its next fault.
		 */
	}
}

deactivate_proc(p)
	struct proc *p;
{
	register preg_t *prp;
	vas_t *vas = p->p_vas;

	p->p_flag &= ~SLOAD;
	p->p_time = 0;
	bump_nswap(p->p_upreg); /* increment nswap count */

	/* Walk through process regions to get r_swnvalid.  We just 
	 * need an approximate number, so don't bother with region locks.
	 * Then force deact_prssize() to compute a sum and cache it.
	 */
	if (!cvaslock(vas))
		return;
	for (prp = vas->va_next; prp != (preg_t *)(vas); prp = prp->p_next)
		prp->p_reg->r_swnvalid = prp->p_reg->r_nvalid;
	p->p_vas->va_swprss = 0;
	vasunlock(p->p_vas);
	(void)deact_prssize(p);
	cnt.v_swpout++;
	sar_swapout++;
}

activate_proc(p)
	struct proc *p;
{
	if (! swapin(p))
		return 0;

	SPINLOCK(sched_lock);
	p->p_flag &= ~SSTOPFAULTING;
	p->p_flag |= SLOAD;
	p->p_time = 0;
	if (p->p_flag & SFAULTING)
		++num_fault_procs;
	if (p->p_stat == SRUN)
		setrq(p);
	else
		wakeup((caddr_t)&p->p_upreg->p_lastfault);
	sar_swapin++;
	cnt.v_swpin++;
	SPINUNLOCK(sched_lock);

	return 1;
}

/*
 * This global indicates when vhand is deferring pageouts to NFS
 * file systems that are "down".  When it goes to 0, vhand will
 * retry NFS pageouts, even to a "down" mount.
 */
long vhand_nfs_retry;

/*
 * Called once per second.
 */
vmmeter()
{
	register unsigned *cp, *rp, *sp;

	/*
	 * If vhand is postponing pageouts to NFS mounts,
	 * update its retry counter once per second.
	 */
	if (vhand_nfs_retry > 0)
		--vhand_nfs_retry;

	/* Forget the memory deficit at vhand's paging rate */
	if (deficit > pageoutrate)
	    	deficit -= pageoutrate;
	else if (deficit < -pageoutrate)
	    	deficit += pageoutrate;
	else
	    	deficit = 0;

	/* Forget 3/4 of the fault deficit.  This matches the rate 
	 * hard_clock()'s smoothing of ave_num_fault_procs forgets one 
	 * faulting process.
	 */
	fault_deficit = (fault_deficit + 2) >> 2;
	ave(avefree, freemem, AVEFREEINT);
	idle_ave();

	/* v_pgin is maintained by clock.c */
	cp = &cnt.v_first; rp = &rate.v_first; sp = &sum.v_first;
	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
	cnt.v_free = freemem;
	if (time.tv_sec % 5 == 0) {
		/* vmtotal is done directly from the schedcpu process */
		/* in the PERFORM_MEM kernel. This removes searching  */
		/* the proc table on the interrupt stack.	      */
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
	}
	SDTA_SUSPEND();
#ifdef	MP
	if (avefree < minfree && (valusync(&runout) <= 0) || proc[S_SWAPPER].p_slptime > maxslp/2) {
		SDTA_RESUME();
		cvsync(&runin);
		cvsync(&runout);
	}
#else	/* ! MP */
	if (avefree < minfree && runout || proc[S_SWAPPER].p_slptime > maxslp/2) {
		SDTA_RESUME();
		runout = 0;
		runin = 0;
		wakeup((caddr_t)&runin);
		wakeup((caddr_t)&runout);
	}
#endif	/* ! MP */
	SDTA_RESUME();
}

/*
 * Schedule rate for paging.
 */
schedpaging()
{
	extern vm_sema_t vhandsema;
	/*
	 * Keep track of whether we are paging.  Currently set up as a
	 * one-and-a-half-second memory of freemem being too low.
	 */
	if (freemem < gpgslim)
		paging = vhandrunrate * 3/2;
	else {
		if (paging > 0)
			--paging;
	}

	/* Wake up page aging/stealing process vhandrunrate times per second
	 * unless we have lots of free memory, or are about to.
	 */
	if (freemem + parolemem < lotsfree)
		wakeup((caddr_t)&vhandsema);

	timeout(schedpaging, (caddr_t)0, hz / vhandrunrate);
}


#ifdef hp9000s800
/*
 * shproc take a proc entry which other non-converged routines are expected
 *  to return.  This was all done for kernel preemption, which is only
 *  available on the 800.  So I've ifdef'ed this function out.
 *			... Andy
 */
extern struct proc *shproc;
#endif hp9000s800

vmtotal()
{
	register struct proc *p;
	register int endchain;
#ifdef hp9000s800
	register struct proc *sproc;
	register short phx1;
	int kcount;
#endif hp9000s800
	int nrun = 0;

	total.t_vmtxt = 0;
	total.t_avmtxt = 0;
	total.t_rmtxt = 0;
	total.t_armtxt = 0;
	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_pw = 0;
	total.t_sl = 0;
	total.t_sw = 0;

#ifdef hp9000s800
	kcount = 0;
	sproc = shproc;
	phx1 = pindx(sproc);
#endif hp9000s800

	endchain = 0;
	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	for (p = proc; endchain == 0 ; p = &proc[p->p_fandx]) {
		if (p->p_fandx == 0)
			endchain = 1;
		/* A preemption point will cause a glitch in the total
		 * data. And of course the rate structure (which will
		 * have this frozen total averaged in while we are
		 * preempted here.
		 */
		if (p->p_flag & SSYS)
			continue;
		if (p->p_stat) {
			int dsize, tsize, ssize;

			(void)pregusage(p->p_vas, &dsize, &tsize, &ssize);
			total.t_vm += dsize + ssize;
			total.t_rm += prssize(p);
			switch (p->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (!(p->p_flag&SSIGABL)&&(p->p_pri<=PZERO))
					nrun++;
				else if (p->p_flag & SLOAD) {
				        if (!(p->p_flag&SSIGABL) &&
				            (p->p_pri<=PZERO))
						total.t_dw++;
					else if (p->p_slptime < maxslp)
						total.t_sl++;
				} else if (p->p_slptime < maxslp)
					total.t_sw++;
				if (p->p_slptime < maxslp)
					goto active;
				break;

			case SRUN:
			case SIDL:
				nrun++;
				if (p->p_flag & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
active:
				{
				    int tdsize, ttsize, tssize;

				    (void)pregusage(p->p_vas, &tdsize,
					    &ttsize, &tssize);
				    total.t_avm += tdsize + tssize;
				    total.t_arm += prssize(p);
				}
				break;
			}
		}
#ifdef hp9000s800
		if (kcount++ >= 3){
			kcount = 0;
			if (endchain == 1)
				break;
			/* link in dummy entry in case we are preempted 
			 *  (we will only do this every 4 times around)
			 *  (restarting was bad for throughput) 
			 */
			sproc->p_fandx = p->p_fandx;
			sproc->p_pandx = proc[p->p_fandx].p_pandx;
			proc[sproc->p_fandx].p_pandx = phx1;
			p->p_fandx = phx1;
			p = sproc;
			SPINUNLOCK(sched_lock);
			SPINUNLOCK(activeproc_lock);
			KPREEMPTPOINT();
			SPINLOCK(activeproc_lock);
			SPINLOCK(sched_lock);
			if (sproc->p_fandx == 0){
				endchain = 1;
				proc[sproc->p_pandx].p_fandx = 0;
			} else {
				proc[sproc->p_pandx].p_fandx = sproc->p_fandx;
				proc[sproc->p_fandx].p_pandx = sproc->p_pandx;
			}
		}
#endif hp9000s800
	}
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);
	total.t_vm += total.t_vmtxt;
	total.t_avm += total.t_avmtxt;
	total.t_rm += total.t_rmtxt;
	total.t_arm += total.t_armtxt;
	total.t_free = freemem;
	KPREEMPTPOINT();

	/* All the load averages in the world */
	loadav(avenrun, nrun);
#ifndef _WSIO
	display_activity(nrun);	/* display run queue length on chassis led's */
#endif /* _WSIO */
}

#ifdef PRINTPROC
/*
 * Prints some interesting info about the proc structure
 * if you are interested.
 */
printproc(p)
	register struct proc *p;
{
	printf("pid = %d procnum = %x flags = %x stat = %x pri = %x rtpri = %x"
	,p->p_pid,p-proc,p->p_flag, p->p_stat,p->p_pri,p->p_rtpri);
	printf("\n   flags: ");
	if( p->p_flag & SRTPROC) printf("SRTPROC ");
	if( p->p_flag & SLOAD) printf("SLOAD ");
	if( p->p_flag & SSIGABL) printf("SSIGABL ");
	if( p->p_flag & SSYS) printf("SSYS ");
	printf("\n   stat: ");
	if( p->p_stat == SSLEEP) printf("SSLEEP ");
	if( p->p_stat == SWAIT) printf("SWAIT ");
	if( p->p_stat == SRUN) printf("SRUN ");
	if( p->p_stat == SIDL) printf("SIDL ");
	if( p->p_stat == SZOMB) printf("SZOMB ");
	printf("\n");
	printf(" slptime  %x",p->p_slptime);
	printf(" wait-channel = %x ", p->p_wchan);
	printf(" cursig %x sigmask %x \n",p->p_cursig,p->p_sig);
}
#endif PRINTPROC
/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 */
double	cexp[3] = {
	0.9200444146293232,	/* exp(-1/12) */
	0.9834714538216174,	/* exp(-1/60) */
	0.9944598480048967,	/* exp(-1/180) */
};

/*
 * Compute a tenex style load average of a quantity on
 * 1, 5 and 15 minute intervals.
 */
loadav(avg, n)
	register double *avg;
	int n;
{
	register int i;

	for (i = 0; i < 3; i++)
		avg[i] = cexp[i] * avg[i] + n * (1.0 - cexp[i]);

}

/* MP REVISIT: how to measure idle time on MP ?? */
idle_ave()
{
	static int lastbootticks = 0;
	static int lastidleticks = 0;
	int idleticks, ticks;
	
	ticks = ticks_since_boot - lastbootticks;
	lastbootticks = ticks_since_boot;

	idleticks =   cp_time[CP_IDLE]
#ifdef CP_WAIT
		    + cp_time[CP_WAIT]
#endif
		    ;
	ave(ave_idle_pcnt, 100 * (idleticks - lastidleticks) / nz(ticks), 5);
	lastidleticks = idleticks;
}

/* 
 * Return the private resident set size of p, while computing and caching
 * both the private and shared resident set sizes.  If we cannot get a lock, 
 * just return the previously cached value.
 */
int
prssize(p)
struct proc *p;
{
	register vas_t *vas = p->p_vas;
	register preg_t *prp;
	register reg_t *rp;
	register ushort tmpincore;
	register int private = 0;
	register int shared = 0;

	/*
         * Because the system activates (allows sched) to look at
	 * a proc structure before a vas structure is assigned to
	 * the proc structure, the resident set size calculation
	 * must protect itself against dereferencing a NULL vas
	 * pointer.
	 */
	if (vas == (vas_t *)NULL)
		return 0;
	if (!cvaslock(vas))
		return vas->va_prss;
	for (prp = vas->va_next; prp != (preg_t *)(vas); prp = prp->p_next) {
		if (prp->p_type == PT_IO)
			continue;
		rp = prp->p_reg;
		VASSERT(rp);
		/*
		 * Since we do not lock the region, we can not
		 * expect its data structures to be perfect.
		 * If a shared incore is 0, just skip it.
		 */
		tmpincore = rp->r_incore;
		if (rp->r_type == RT_SHARED && tmpincore)
			shared += rp->r_nvalid / tmpincore;
		else
			private += rp->r_nvalid;
	}
	/*
	 * Cache the value for PRSSIZE_APPROX() and friends (vmmac.h)
	 */
	vas->va_prss = private;
	vas->va_rss = private+shared;
	vasunlock(vas);

	return private;
}

/* 
 * Return the private resident set size of the process when it was
 * deactivated.  The result is cached, and since by definition it does
 * not change until the next deactivation (at which time it is cleared),
 * if the cached value is non-zero it is returned.
 */
int
deact_prssize(p)
struct proc *p;
{
	vas_t *vas = p->p_vas;
	preg_t *prp;
	int retv = 0;

	if (!vas)
		return 0;

	if (vas->va_swprss > 0)
		return vas->va_swprss;

	if (!cvaslock(vas))
		return 0;

	/* Add up sizes per region that were saved at deactivation time */
	for (prp = vas->va_next; prp != (preg_t *)(vas); prp = prp->p_next) {
		VASSERT(prp && prp->p_reg);
		if (prp->p_type != PT_IO && prp->p_reg->r_type != RT_SHARED)
			retv += prp->p_reg->r_swnvalid;
	}

	/* Cache the answer */
	vas->va_swprss = retv;
	vasunlock(vas);

	return(retv);
}

/*	
 * Swap out p.  Don't write pages vhand will find, as vhand can do a
 * better job of writing them (knowing how much to write at a time and
 * knowing when to quit writing).
 */
swapout(p)
	register struct proc *p;
{
	register preg_t *prp;
	register reg_t *rp;
	extern void hdl_swapo();
	extern void hdl_procswapo();

	VASSERT(p->p_rlink == 0);	     /* and removed from run queue */
	VASSERT(p->p_reglocks == 0);	     /* caller must guarantee this */

	p->p_flag |= SSWAPPED;

	/*
	 * Walk through process regions Private regions or shared regions
	 * that are being used exclusively by this process get paged out en
	 * masse.  Other shared regions are just pruned.
	 */
	vaslock(p->p_vas);
	for (prp = p->p_vas->va_next; prp != (preg_t *)(p->p_vas);
						prp = prp->p_next) {
		/*
		 * I/O pregions don't correspond to real memory
		 */
		if (prp->p_type == PT_IO)
			continue;

		rp = prp->p_reg;
		VASSERT(rp);
		reglock(rp);
		rp->r_incore--;
		VASSERT(rp->r_incore == 0 || rp->r_type != RT_PRIVATE);

		/* 
		 * if we ever start swapping out processes with memory
		 * locked pregions then sched() should be enhanced
		 * to give them swapin preference.
		 */
		if (PREGMLOCKED(prp))
			panic("swapout: process has memory locked pregion");

		if (prp->p_type == PT_UAREA) {
			VASSERT(rp->r_incore == 0);
			VOP_PAGEOUT(prp->p_reg->r_bstore, prp, 0, 
				    prp->p_count - 1, 
				    PAGEOUT_SWAP|PAGEOUT_FREE|PAGEOUT_HARD);
#ifdef __hp9000s300
			VASSERT(prp->p_reg->r_type != RT_PRIVATE || 
				prp->p_hdl.p_ntran == 0);
#endif
			VASSERT(rp->r_nvalid == 0);
			vfdswapo(rp);
		}
#ifdef __hp9000s300
		/* Destroy the translations.   On 8.0 we called VOP_PAGEOUT
		** for all pregions.  This also destroyed the translations.
		** At first glance it would not seem obvious that this is
		** neccessary, since hdl_swapo destroys the segment/page
		** tables and the attach lists.  Don't know why this is
		** neccessary.  cem  HP Revisit.
		*/
		else hdl_alt_swapout(prp);
#endif
		/* else vhand will page it out */
		regrele(rp);
	}

	/* Tell the HDL */
	hdl_swapo(p->p_vas);
	hdl_procswapo(p, p->p_vas);

	vasunlock(p->p_vas);
}

bump_nswap(prp)
preg_t *prp;
{
	register reg_t *rp;
	struct rusage copyru;

	rp = prp->p_reg;
	VASSERT(rp);
	reglock(rp);
#ifdef __hp9000s300
	hdl_copy_page((vas_t *)prp->p_space, prp->p_space,
	    &(((struct user *)(prp->p_vaddr+KSTACK_PAGES*NBPG))->u_ru),
		      KERNVAS, KERNELSPACE, &copyru,
		      sizeof(struct rusage));
#else

	copyfrompreg(prp, (int)&((struct user *)0)->u_ru,
	       (caddr_t)&copyru, sizeof(struct rusage));
#endif
	copyru.ru_nswap++;
#ifdef __hp9000s300
	hdl_copy_page(KERNVAS, KERNELSPACE, &copyru,
		(vas_t *)prp->p_space, prp->p_space,
		&(((struct user *)(prp->p_vaddr+KSTACK_PAGES*NBPG))->u_ru),
		sizeof(struct rusage));
#else

	copytopreg(prp, (int)&((struct user *)0)->u_ru,
	       (caddr_t)&copyru, sizeof(struct rusage));
#endif

	regrele(rp);
}

/*
 * Swap in process p.  Only read from disc what demand faulting won't.
 */
int
swapin(p)
struct proc *p;
{
	register preg_t *prp;
	register reg_t *rp;
	register int nsegs;	/* # of pages needed to swapin vfds */
	extern void vfdswapi();
	extern void hdl_swapi();
	extern void hdl_procswapi();
	extern void procmemunreserve();
	register int i;
	vas_t *vas = p->p_vas;
#ifdef hp9000s800
	vfd_t *vfd;
#endif

	if(!(p->p_flag & SSWAPPED))
		return 1;

	/* Calculate upper bound on number of pages needed to swapin
	 * vfds for all the regions.  This is an upper bound because,
	 * even though we may sleep here, only sched() will (indirectly)
	 * call vfdswapo(), hence no more vfds will get swapped out.
	 * Note that some of vfds may get swapped in while we sleep,
	 * but we tolerate this.
	 */
	nsegs = countsegs(p);	/* # of pages needed to swapin vfds */

	/* Add on size of U area and prepaging amount */
	nsegs += p->p_upreg->p_count;
	nsegs += prepage;
	nsegs += hdl_swapicnt(p, vas);

	/* Hide enough pages so that the scheduler can't end up in a
	 * position where it is waiting on itself for pages.
	 */
	if (procmemreserve((unsigned int)nsegs, 0, (reg_t *)0) == 0)
		return 0;

	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *)(vas); prp = prp->p_next) {
		if (prp->p_type == PT_IO)
			continue; /* I/O pregions don't have "real" memory */

		rp = prp->p_reg;
		reglock(rp);
		rp->r_incore++;
		if(rp->r_dbd) {
			syswait.swap++;
			vfdswapi(rp);
			syswait.swap--;
			nsegs -= countvfd(rp);
		}
		regrele(rp);
	}

	hdl_swapi(vas);
	vasunlock(vas);

#ifdef hp9000s800
	prp = p->p_upreg;
	rp = prp->p_reg;
	for (i = prp->p_off; i < (prp->p_off + prp->p_count); i++) {
		if ((vfd = FINDVFD(rp, i)) == (vfd_t *) NULL)
			panic("swapin: vfd not found");

		if (!(vfd->pgm.pg_v)) {
			/*
			 * Page is invalid, call vfault
			 * to make it valid.
			 */
			syswait.swap++;
			if (vfault(vas, 0, prp->p_space,
				   prp->p_vaddr + ptob(i - prp->p_off))) {
				panic("swapin: cannot bring in U area");
			}
			syswait.swap--;
			cnt.v_pswpin++;
       	        	cnt.v_pgpgin--;
	               	sar_bswapin += ptod(1);
		}
	}
#endif
#ifdef __hp9000s300
	for (i = 0; i <= p->p_stackpages; i++) {
#ifdef SAR		
	    syswait.swap++;
#endif	    
	    if (vfault(vas, 1, vas, p->p_upreg->p_vaddr + ptob(KSTACK_PAGES-i)))
		    panic("swapin: cannot bring in U-area/stack page");
#ifdef SAR
	    syswait.swap--;
#endif	    
	    cnt.v_pswpin++;
	    cnt.v_pgpgin--;
	    sar_bswapin += ptod(1);
	}
#endif

	/* Fill in rest of HDL now that U area is present */
	vaslock(vas);
	hdl_procswapi(p, vas);
	vasunlock(vas);
	p->p_flag &= ~SSWAPPED;

	/* Release whatever memory is no longer needed */
	procmemunreserve();
	
	return(1);	/* successfully swapped process in */
}

countsegs(p)
struct proc *p;
{
	register preg_t *prp;
	register reg_t *rp;
	register retv = 0;

	/*	Walk through process regions counting segments for
	 *	swapped out regions.
	 */
	vaslock(p->p_vas);
	for (prp = p->p_vas->va_next; prp != (preg_t *)(p->p_vas);
						prp = prp->p_next) {
		rp = prp->p_reg;
		if (rp->r_dbd != 0)
			retv += countvfd(rp);
	}
	vasunlock(p->p_vas);
	return(retv);
}
