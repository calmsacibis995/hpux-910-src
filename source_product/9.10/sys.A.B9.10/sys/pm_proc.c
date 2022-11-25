/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/pm_proc.c,v $
 * $Revision: 1.8.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:09:24 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/sar.h"
#include "../h/vm.h"

/*
 * Initialize all of the proc table entries.
 */
procinit()
{
	int i;
	for (i = 0; i < nproc; i++) {
		/*
		 * activeproc_lock not required at boot
		 */
		proc[i].p_fandx = -1;
#ifdef __hp9000s800
		proc[i].p_pindx = i;
#endif
	}
}

/*
 * Link all free proc structures onto the freeproc_list.
 * (except for proc[0])
 */
init_freeproc_list()
{
	register struct proc *p1;
	
	freeproc_list = 0;
	for ( p1 = (procNPROC - 1); p1 > proc; p1--){
		/*
		 * activeproc_lock not required at boot
		 */
		if (p1 <= &proc[S_STAT]) {
			p1->p_fandx = -1;
		} else {
			p1->p_fandx = freeproc_list;
			freeproc_list = pindx(p1);
		}
 	}

}

/*
 * Allocate a free proc structure entry and clear it out.  Place all 0
 * assignments here!  If the slotnum is S_DONTCARE, allocate any proc
 * structure; otherwise try and allocate the desired one.  Only the startup
 * code currently uses the slotnum.  It is a panic condition if the desired
 * slot is not on the free list.
 */
proc_t *
allocproc(slotnum)
	int slotnum;
{
	proc_t *pp = NULL;
	int phx;
	int phx2;

	VASSERT(slotnum != 0);


#define ACTIVEPROC_HACK	/* TEMPORARY HACK -  Remove at merge to branch */
#ifdef ACTIVEPROC_HACK
	if(!uniprocessor && owns_spinlock(sched_lock))
		panic("allocproc: Already own sched_lock!");
#endif /* ACTIVEPROC_HACK */

	TBAR_SPINLOCK(sched_lock);

	/*
	 * No free proc slots so; return NULL.
	 */
	SPINLOCK(activeproc_lock);
	if (slotnum == S_DONTCARE) {
		if (freeproc_list == 0) {
			tablefull("proc");
			procovf++;
			SPINUNLOCK(activeproc_lock);
			return((proc_t *)NULL);
		}
		pp = &proc[freeproc_list];
		phx = freeproc_list;
		freeproc_list = pp->p_fandx;
	} else {
		phx = slotnum;
		pp = &proc[phx];
		VASSERT(pp->p_fandx == -1);
	}

	TBAR_SPINLOCK(PROCESS_LOCK(pp));

	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(pp));

	/* 
	 * insert proc slot onto the active list 
	 * after proc[0].  The following code does a doubly
	 * linked list insert using indexes instead of pointers.
	 */
	phx2 = proc[0].p_fandx;
	pp->p_fandx = phx2;
	pp->p_pandx = 0;	
	proc[0].p_fandx = phx;
        proc[phx2].p_pandx = phx;

	/* mark proc slot as taken */
	pp->p_stat = SIDL; 

	/*
	 * We can't bzero the proc structure because certain fields 
	 * must remain (p_ndx for example).  For now, we just 0 out
	 * the appropriate fields each time.
	 */
	pp->p_ysptr = NULL;
	pp->p_pptr = NULL;
	pp->p_time = 0;
	pp->p_utime.tv_sec = 0;
	pp->p_utime.tv_usec = 0;
	pp->p_stime.tv_sec = 0;
	pp->p_stime.tv_usec = 0;
	pp->p_wchan = 0;
	pp->p_slptime = 0;
	pp->p_cpu = 0;
	pp->p_pctcpu = 0;
	pp->p_cpticks = 0;
	pp->p_cptickstotal = 0;
	pp->p_start = 0;

#ifdef AUDIT
	pp->p_idwrite = 0;		/* clear flag */
#endif AUDIT

	pp->p_ndx = pindx(pp);

	VASSERT((slotnum == S_DONTCARE) || (pp->p_ndx == slotnum));

	SPINUNLOCK(PROCESS_LOCK(pp));
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);
	return(pp);
}

freeproc (pp)
	proc_t *pp;
{
	reg_t *rp;
	preg_t *prp;
	struct rusage copyru;
#ifdef MP
	sv_sema_t	sema_save;
#endif MP


	PXSEMA(&pm_sema, &sema_save);
	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(pp));

#ifdef MPDEBUG
	if ((pp->p_mpflag & SRUNPROC)) 
		panic("freeproc mp_flag non zero");
#endif

#ifdef MP
	pp->p_mpflag = 0;
#endif

	pp->p_stat = 0;		/* 
				 * do this before freeing up stuff as
				 * it is an indicator of the proc's liveness
				 */
	pp->p_xstat = 0;

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(pp));

	VASSERT(pp->p_reglocks == 0);

	if ((pp->p_flag & SVFORK) == 0 && (pp->p_vas)
	    && (prp = findpregtype(pp->p_vas, PT_UAREA))) {

		/*
		 * Add in resources from child
		 */
		reglock((rp = prp->p_reg));
#ifdef __hp9000s300
		hdl_copy_page((vas_t *)prp->p_space, prp->p_space, 
		    &(((struct user *)(prp->p_vaddr+KSTACK_PAGES*NBPG))->u_ru),
			      KERNVAS, KERNELSPACE, &copyru, 
			      sizeof(struct rusage));
#else
		
		copyfrompreg(prp, (int)&((struct user *)0)->u_ru,
                       (caddr_t)&copyru, sizeof(struct rusage));
#endif
		regrele(rp);
		timevaladd(&copyru.ru_utime, &pp->p_utime);
		timevaladd(&copyru.ru_stime, &pp->p_stime);
		ruadd(&u.u_cru, &copyru);

#ifdef __hp9000s300
		/*
		 * Init dies during the reboot operation.  Hide this fact here
		 */
		if (pp->p_pid != PID_INIT)
#endif
			kissofdeath(pp);
	}

	/* Release vas */
	if (pp->p_vas) {
		vas_t *vas = pp->p_vas;
		pp->p_vas = (vas_t *)NULL;
		vaslock(vas);
		vas->va_refcnt--;
		freevas(vas);
	}

	/*
	 * break child parent relationship.
	 */

	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(pp));

	abandonchild(pp);
	pp->p_pid = 0;
	pp->p_sig = 0;
	pp->p_sigcatch = 0;
	pp->p_sigignore = 0;
	pp->p_sigmask = 0;
	pp->p_pgrp = 0;	     /* not always done in exit() */
	pp->p_sid = 0;
	pp->p_sidhx = 0;
	pp->p_flag = 0;
	pp->p_flag2 = 0;
	pp->p_wchan = 0;
	pp->p_cursig = 0;
	pp->p_reglocks = 0;

	/* return proc entry to free list */
	pp->p_fandx = freeproc_list;
	freeproc_list = pp - proc;

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(pp));

	VXSEMA(&pm_sema, &sema_save);
}

makechild(pp, cp)
	proc_t *pp, *cp;
{
	T_SPINLOCK(sched_lock);
	T_SPINLOCK(PROCESS_LOCK(pp));
	cp->p_pptr = pp;
	cp->p_ppid = pp->p_pid;
	cp->p_osptr = pp->p_cptr;
	if (pp->p_cptr)
		pp->p_cptr->p_ysptr = cp;
	pp->p_cptr = cp;
}

abandonchild(p)
	proc_t *p;
{
	proc_t *q;

	T_SPINLOCK(sched_lock);
	T_SPINLOCK(PROCESS_LOCK(q));
	if (q = p->p_ysptr)
		q->p_osptr = p->p_osptr;
	if (q = p->p_osptr)
		q->p_ysptr = p->p_ysptr;
	if ((q = p->p_pptr)->p_cptr == p)
		q->p_cptr = p->p_osptr;
	p->p_pptr = 0;
	p->p_ysptr = 0;
	p->p_osptr = 0;
	p->p_cptr = 0;
	p->p_ppid = 0;
}
