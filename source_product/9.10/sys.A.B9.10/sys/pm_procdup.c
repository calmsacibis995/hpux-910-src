/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/pm_procdup.c,v $
 * $Revision: 1.8.84.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:21:04 $
 */

#include "../h/debug.h"
#include "../h/param.h"
#include "../h/types.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/swap.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/pfdat.h"
#include "../machine/pte.h"
#ifdef __hp9000s300
#include "../machine/trap.h"
/* XXX add the following to pcb.h someday.... */
#define pcb_locregs pcb_float[2]
#endif /* __hp9000s300 */

extern int hil_pbase;

/*
 *	build a u area and address space for the child
 */
/*ARGSUSED*/
struct user *
createU(cp, vforkexec)
	register struct proc *cp;
	int vforkexec;
{
	register int i;
	register preg_t *prp;
	register reg_t *rp;
	struct user *useg;
	vfd_t *vfd;
	int cupfn, cspfn;
	int nstackpages;
	extern int processor;

	/*
	 * The stuff below assumes this & I don't see any easy way to
	 * make them more "portable" (whatever that means, considering
	 * what we're doing here)
	 */
	VASSERT(btorp(sizeof(struct user)) == 1);

	if (!vforkexec) {
	    /*
	     * Snapshot the current U area register and float context;
	     * so the copy is correct.
	     */
	    setjmp(&u.u_rsave);     /* this keeps longjmp happy */
	    save_floating_point(u.u_pcb.pcb_float);
	}

	/*
	 * Create a new region for the child's UAREA
	 */
	if ((rp = allocreg(0, swapdev_vp, RT_PRIVATE)) == NULL)
		panic("createU: can not allocate a region");

	if (growreg(rp, KSTACK_PAGES + UPAGES, DBD_DFILL) < 0) {
		freereg(rp);
		return(NULL);
	}

	/*
	 * PF_NOPAGE will keep vhand from paging the uarea.
	 */
	if ((prp = attachreg(cp->p_vas, rp, PROT_KRW, PT_UAREA,
			     PF_EXACT | PF_NOPAGE,
			     UAREA - (KSTACK_PAGES * NBPG),
			     0, KSTACK_PAGES + UPAGES)) == NULL)
		panic("createU: can not attach region");


	/* 
	 * Fill in the uarea page
	 * and get its page number.
	 */
	fillpreg(prp, KSTACK_PAGES);
	vfd = FINDVFD(rp, KSTACK_PAGES);
	cupfn = vfd->pgm.pg_pfn;

	/* 
	 * Copy the U area through the ttlb
	 * window.  Remember to flush the cache.
	 */
	useg = (struct user *)ttlbva(cupfn);
	purge_dcache();
	pg_copy(&u, useg);
	if (processor == M68040)
	    purge_dcache_physical();

	/* get the number of stack pages being used currently */

	nstackpages = get_nstackpages();

	/* 
	 * Fill in the stack pages and copy them
	 */

	for (i = 1; i <= nstackpages; i++) {

	    fillpreg(prp, KSTACK_PAGES - i);
	    vfd = FINDVFD(rp, KSTACK_PAGES - i);
	    cspfn = vfd->pgm.pg_pfn;

	    /*
	     * Now copy the stack page.
	     */

	    pg_copy(((caddr_t)(&u))-ptob(i), ttlbva(cspfn));
	}
	
	/* Set # of kernel stack pages */

	cp->p_stackpages = nstackpages;

	/* Set this here for hdl_procattach */
	useg->u_procp = cp;

	/* Attach the pregion to its process */
	cp->p_upreg = prp;
	hdl_procattach(useg, cp->p_vas, prp);
	regrele(rp);

	/* Save pointer in proc[] to PTEs for U area--locore uses this */
	cp->p_addr = vtoipte(prp, ptob(KSTACK_PAGES));

	if (!vforkexec) {
	    /*
	     * Arrange for a non-local goto when the new process
	     * is started, to resume here, returning nonzero from setjmp.
	     */
	    useg->u_pcb.pcb_sswap = (int *)&u.u_ssave;

#define	STACK_300_OVERFLOW_FIX

#ifdef	STACK_300_OVERFLOW_FIX
	    useg->u_vapor_mlist = NULL;
#endif	/* STACK_300_OVERFLOW_FIX */

	    /* Interval timers are not inherited across forks.  They   */
	    /* must be cleared before the new process gets to run,     */
	    /* to assure that it does not receive a signal from        */
	    /* expiration of a timer.                                  */
	    /* Note that u_timer[ITIMER_REAL] not used; p_realtimer is */
	    /* cleared  in creation of the new proc structure (and its */
	    /* it_value field is redundantly cleared in newproc().     */

	    timerclear(&useg->u_timer[ITIMER_VIRTUAL].it_value);
	    timerclear(&useg->u_timer[ITIMER_VIRTUAL].it_interval);
	    timerclear(&useg->u_timer[ITIMER_PROF].it_value);
	    timerclear(&useg->u_timer[ITIMER_PROF].it_interval);
	}
#ifdef __hp9000s300
	/*
	 * For the 68040 allocate memory for bus error handling.
	 */
	if (processor == M68040)
	    useg->u_pcb.pcb_locregs =
		(u_int) kmem_alloc(sizeof(struct mc68040_exception_stack));
#endif /* __hp9000s300 */

	return(useg);
}


/*
 * create a duplicate copy of a process
 */
procdup(forktype, pp, cp)
	int forktype;
	proc_t *pp, *cp;
{
	register preg_t	*prp;
	register user_t *useg;
	register vas_t *cvas;
	struct vforkinfo *vi;
	int i;
	extern vas_t *dupvas();
	extern void (*graphics_forkptr)();

	/*
	 * Allocate a u area for the child, and set up memory management
	 */
	VASSERT(u.u_procp == pp);

	/*
	 * Duplicate the parent's virtual address space.
	 */
	if ((cvas = dupvas(forktype, pp->p_vas)) == (vas_t *)NULL)
		goto bad1;
	cp->p_vas = cvas;
	cp->p_vas->va_proc = cp;

	/*
	 * Copy the page tables that are not yet filled in for
	 * the child.  If the child is going to be just a
	 * daemon process, we don't want the parents page tables.
	 * If we are vforking then we are sharing the vas, so
	 * it is not necessary to copy the page tables.
	 */
	if (forktype == FORK_PROCESS) {
		vm_dup_tables(pp->p_vas, cp->p_vas);
		VASSERT(cp->p_vas->va_hdl.va_seg);
	}
	cp->p_segptr = cp->p_vas->va_hdl.va_seg;

	if (forktype == FORK_VFORK) {

	    useg = &u;
	    cp->p_upreg = pp->p_upreg;
	}
	else {
	    if ((useg = createU(cp,0)) == NULL)
		    goto bad;


	    /*
	     * Loop through attaching the pregions to the process context.
	     */
	    vaslock(cvas);
	    for (prp = cvas->va_next; prp != (preg_t *)cvas; prp = prp->p_next) {
		    if (prp->p_type != PT_UAREA) {
			    reglock(prp->p_reg);
			    hdl_procattach(useg, cvas, prp);
			    regrele(prp->p_reg);
		    }
	    }
	    vasunlock(cvas);
	}

	/*
	 *  We do not have to waste time checking this function pointer
	 *  against NULL.  There is no way for the SGRAPHICS flag to 
	 *  be set if this function pointer is not valid.
	 */
	if (pp->p_flag2 & SGRAPHICS) {
		(*graphics_forkptr)(pp, cp);
		if (u.u_error)
			goto bad;
	}

	/*
	 * u_pcb.pcb_sswap assigned below to point at this 
	 * save area.
	 */
	if (setjmp(&(useg->u_ssave))) {
		return(FORKRTN_CHILD);
	}


	if (forktype != FORK_VFORK && useg->u_pcb.pcb_dragon_bank != -1) {
		useg->u_pcb.pcb_dragon_bank = -1;
		dragon_detach(cp);
	}

	/* Allocate space for the child's p_ofilep. */

	cp->p_ofilep = (struct ofile_t **)
		kmem_alloc((sizeof(struct ofile_t *)) * NFDCHUNKS(u.u_maxof));

	/* Set all the elements of the u_ofilep to NULL */
	bzero(cp->p_ofilep,
		(sizeof(struct ofile_t *) * NFDCHUNKS(u.u_maxof)));
	/*
	 * allocate memory and copy the parent's for all non-null ofileps.
	 */

	for (i = 0; i<NFDCHUNKS(u.u_maxof) && pp->p_ofilep[i] != NULL; i++) {
		cp->p_ofilep[i] =
			(struct ofile_t *)kmem_alloc(sizeof(struct ofile_t));
		bcopy ((caddr_t)pp->p_ofilep[i], cp->p_ofilep[i],
			sizeof(struct ofile_t));
	}

	if (forktype != FORK_VFORK) {

	    bzero((caddr_t) &(useg->u_ru), sizeof(struct rusage));
	    bzero((caddr_t) &(useg->u_cru), sizeof(struct rusage));
	    useg->u_outime = 0;

	    /*
	     * Assign our sswap to be our save state address for resume.
	     * Also put the savestate back to where it belongs.
	     */
	    useg->u_pcb.pcb_sswap = (int *)&u.u_ssave;
	}

	/*
	 * Parent process running.
	 */

#ifdef __hp9000s800
	if (ms_turned_on(ms_fork_id))
		ms_post_fork(pp,cp);
	if (ms_turned_on(ms_fork_extra_id))
		ms_post_forkextra(pp);   /* all fields from parent */
#endif

	VASSERT(pp->p_reglocks == 0);

	/*
	 * Make child runnable and add to run queue.
	 */
	i = UP_SPL6();
	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(cp));

	cp->p_stat = SRUN;
	setrq(cp);

	/*
	 * Make sure child runs first, thus breaking any copy-on-write
	 * association so the parent can steal his pages back easily.
	 */
	runrun++;

	if (forktype != FORK_VFORK) {

	    /*
	     * Now can be swapped.
	     */

	    pp->p_flag &= ~SKEEP;
	}

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(cp));
	UP_SPLX(i);

	if (forktype == FORK_VFORK) {

		/*
		 * Set vforkinfo who_fork field to VFORK_PARENT
		 * so that next context switch we will save parents
		 * stack.
		 */

		vi = pp->p_vforkbuf;

		vi->vfork_state = VFORK_PARENT;
		vi->pprocp = pp;
		vi->cprocp = cp;
		cp->p_vforkbuf = pp->p_vforkbuf;

		/*
		 * The parent sleeps here, and must not be
		 * woken up until the child exits or execs
		 * The resume code saves the parents stack
		 * and U-area so that the child can trash
		 * it. If the parent wakes up before the
		 * child restores the u-area and stack, there
		 * is no telling what will happen (most likely
		 * a double bus fault)
		 */

		sleep((caddr_t)&(cp->p_vforkbuf), PZERO - 1);

		/* OK, everything is back to normal! */
	}

	return(FORKRTN_PARENT);
bad:
	if (pp->p_vas != cp->p_vas) {
		if (useg == NULL) {
		    /* dispose of pregions, but since hdl_procattach 
		     * wasn't called don't call hdl_procdetach       
		     */
		    dispreg(cp, (struct vnode *) NULL, NOPROCATTACH);

		    /* get rid of any translation tables allocated by
		     * vm_dup_tables().
		     */
		    vm_dealloc_tables(cp->p_vas);
		}
		else {
		    /* dispose of pregions, including call to 
		     * hdl_procattach. Then remove Uarea.
		     */
		    (void) dispreg(cp, (struct vnode *) NULL, 
				   PROCATTACHED);
		    kissofdeath(cp);
		}
        }
bad1:
	u.u_error = ENOMEM;
	return(FORKRTN_ERROR);
}


vfork_createU()
{
    register struct proc *p = u.u_procp;
    vas_t *ovas;
    vas_t *nvas;
    struct user *useg;
    struct vforkinfo *vi;
    register int x;

    if ((nvas = allocvas()) == (vas_t *)0)
	return(-1);

    vasunlock(nvas);

    /* Release old vas */

    ovas = p->p_vas;
    VASSERT(ovas);
    vaslock(ovas);
    ovas->va_refcnt--;
    freevas(ovas);
    p->p_vas = nvas;

    /* Create a new U area and stack and copy old one */

    if ((useg = createU(p,1)) == (struct user *)0)
	return(-1); /* new vas should be freed by exit() */

    /* Change vforkbuf state so switchU will restore parent */

    vi = p->p_vforkbuf;
    vi->vfork_state = VFORK_CHILDEXIT;

    /* Switch onto new stack and use new U-area */

    x = CRIT();
    vfork_switchU(p->p_addr,p->p_stackpages);
    UNCRIT(x);

    return(0);
}
