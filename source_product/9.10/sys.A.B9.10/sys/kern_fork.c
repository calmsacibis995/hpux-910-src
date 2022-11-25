/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_fork.c,v $
 * $Revision: 1.66.83.6 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/07/20 10:57:29 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/kernel.h"
#include "../h/vmsystm.h"
#include "../h/vmmac.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/acct.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/sema.h"
#include "../h/kern_sem.h"
#include "../h/malloc.h"
#include "../dux/duxparam.h"

#ifdef __hp9000s800
#include "../graf.800/venom.h"
#endif

#ifdef __hp9000s800
extern struct venom *venom;                             /* from vsc_config.c  */
extern u_int rsm(), ssm();
#endif

extern	int maxuprc;
extern struct file *getf_no_ue();

extern struct dux_context dux_context;

/*
 * fork system call.
 */
fork()
{
	fork1(FORK_PROCESS);
}

vfork()
{
	fork1(FORK_VFORK);
}

fork1(forktype)
	int forktype;
{
	pid_t cpid, ppid;
	proc_t *cp;
	proc_t *p;
	short nextactive, prevactive;
	int phx, proc_count;
	char **child_cntxp;
	struct vforkinfo *freep;

	VASSERT(forktype != FORK_DAEMON);

	/*
	 * If we are going to vfork, allocate the vfork info
	 * buffer. If it fails, we bail out with ENOMEM.
	 */

	p = u.u_procp;
	if (forktype == FORK_VFORK && vfork_buffer_init(p) != 0) {
		u.u_error = ENOMEM;
		goto out;
	}

	/*
	 * For setcontext(2).
	 *
	 * Get a separate copy of context if current one is not default.
	 * We must do this here, so that we have less to back out of
	 * if we can't get any memory.
	 */
	if (u.u_cntxp != (char **)&dux_context) {
	    child_cntxp = (char **)kmem_alloc(sizeof (struct dux_context));

	    if (child_cntxp == (char **)0) {
		u.u_error = ENOMEM;
		goto out;
	    }
	    bcopy((char *)u.u_cntxp, (char *)child_cntxp,
		  sizeof (struct dux_context));
	}
	else
		child_cntxp = (char **)0;

	/*
         * Since getnewpid() can sleep, we must call it before
         * we search the proc table.  Note that any errors
         * after this will cause us to waste the new pid.
         */
	if ((cpid = getnewpid()) < 0) {
		u.u_error = EAGAIN;
		goto out;
	}

	SPINLOCK(sched_lock);

	/* 
	 * Only allow maxuprc+1 processes if the user is not root.
	 * Resuser() sets the accounting ASU flag (meaning superuser
	 * powers have been invoked) if the user is root, u.u_error
	 * otherwise.
	 */
	proc_count = maxuprc;
	phx = uidhash[UIDHASH(u.u_ruid)];
	for (; phx != 0; phx = proc[phx].p_uidhx) {
		VASSERT(phx > 0);
		VASSERT(phx < nproc);
		if (proc[phx].p_uid == u.u_ruid)
			if (--proc_count < 0)
				break;
	}
	if (proc_count < 0 && !resuser()) {
		SPINUNLOCK(sched_lock);
		u.u_error = EAGAIN;
		goto out;
	}

	SPINUNLOCK(sched_lock);

	/*
	 * Allocate ourselves a proc slot.
	 */
	if ((cp = allocproc(S_DONTCARE)) == (proc_t *)NULL) {
		u.u_error = EAGAIN;
		goto out;
	}		

	/*
	 * Hash this proc structure for our uid and
	 * pid.
	 */
	proc_hash(cp, u.u_ruid, cpid, p->p_pgrp, p->p_sid);


	/*
	 * Copy the pstat information into the new
	 * process.
	 *
	 * This just fills in the command line info associated with
	 * this proc slot.  If the fork() fails, it doesn't need to do
	 * any cleanup.
	 */
	pstat_fork(p, cp);

	/*
	 * Actually create the appropriate child.
	 */
	ppid = p->p_pid;
	switch (newproc(forktype, cp)) {
	case FORKRTN_CHILD: {
		int a;

		/*
		 * Fill in childs Uarea now that we have it.
		 * Don't use variable "p" since it points to the parents 
		 * proc table entry. 
		 */
		u.u_r.r_val1 = ppid;
		u.u_r.r_val2 = 1;  /* child */
		u.u_procp->p_start = time.tv_sec;
		u.u_ticks = ticks_since_boot;
		u.u_acflag = AFORK;

		/*
		 * For setcontext(2).  If the parent had a non-standard
		 * context, we duplicated it above.  Now set u.u_cntxp
		 * to the copy of the non-standard context.
		 */
		if (child_cntxp != (char **)0)
			u.u_cntxp = child_cntxp;

                /*
                 * FROM SUN NFS 3.2. -- turn off flags that a lockf()
                 * has been done, set in fcntl(), lockf()
                 */

                /*
                 * Child must not inherit file-descriptor-oriented locks
                 * (i.e., System-V style record-locking)
                 */
		for (a = 0; a <= u.u_highestfd; a++) {
		    char *pp;
		    extern char *getpp();

		    pp = getpp(a);
		    *pp &= ~UF_FDLOCK;
		}
		return;
	}
	case FORKRTN_PARENT:
		u.u_r.r_val1 = cpid;
		child_cntxp = (char **)0; /* so we do not free it */
		break;
	case FORKRTN_ERROR:
		/* 
		 * remove off of active chain.  We really should
		 * break freeproc() up into smaller pieces of 
		 * functionality
		 */
		SPINLOCK(activeproc_lock);
		nextactive = cp->p_fandx;
                prevactive = cp->p_pandx;
                (proc + nextactive)->p_pandx = prevactive;
                (proc + prevactive)->p_fandx = nextactive;
		SPINUNLOCK(activeproc_lock);
		/*
		 *  Assert that in the failure case, we have already
		 *  freed up our uarea.  freeproc() will try to
		 *  access the uarea if it can find it, and this
		 *  is incorrect for the fork failure case.
		 */
		VASSERT(!(cp->p_vas) || !(findpregtype(cp->p_vas, PT_UAREA)));
		proc_unhash(cp);
		freeproc(cp);
		u.u_error = ENOMEM;
		goto out;
		break;
	default:
		panic("fork1: Unknown return value of newproc");
	}

out:
	/* child should never execute this code */

	if (child_cntxp != (char **)0)
		kmem_free(child_cntxp, sizeof (struct dux_context));

	freep = p->p_vforkbuf;
	if (forktype == FORK_VFORK && freep) {


#ifdef __hp9000s800
	    /* HP REVISIT.  Change libc so this is not needed. */
	{
	    preg_t *prp;
	    reg_t *rp;

	    prp = findpregtype(p->p_vas, PT_STACK);
	    rp = prp->p_reg;
	    reglock(rp);
	    copytopreg(prp, (caddr_t)freep->saved_rp_ptr - prp->p_vaddr,
		       (caddr_t)&freep->saved_rp, sizeof(long));
	    regrele(rp);
	}
#endif /* __hp9000s800 */
	    p->p_vforkbuf = freep->prev;

	    kdfree((caddr_t)freep,freep->buffer_pages);
	}

	u.u_r.r_val2 = 0;
}

/*
 * Create a new process
 * It returns 1 in the new process, 0 in the old.
 */
newproc(forktype, cp)
	int forktype;
	proc_t *cp;
{
	proc_t *pp;
	int x;

#ifdef __hp9000s800
        struct venom_ss *venom_ssp;
        struct venom_ss *venom_pssp;
        register struct venom *venomp;
        register int sm;         /* Must be "register" for in-line assembly */
#endif

	VASSERT(cp);
	VASSERT(cp >= &proc[0]);
	VASSERT(cp < procNPROC);

	if (cp->p_stat != SIDL)
		panic("newproc: cp not SIDL");

	/*
	 * Get a pointer to the parent process.
	 */
	pp = u.u_procp;

	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(cp));

	/*
	 * Perform all of the direct assignments
	 * from the parent proc structure to the
	 * child's.
	 */
	
	fork_inherit(pp, cp);

	/*
	 * DB_SENDRECV
	 *
	 * This routine does not require any cleanup if the fork() fails
	 */
	dbfork(cp);

	/*
	 * Set up the childs flags in one place (Please adhere to this!).
	 */
#ifdef STRACE	 
 	cp->p_flag = SLOAD|SPRIV|(pp->p_flag & (SSCT|SSIGABL|SRTPROC|SOUSIG));
#else
 	cp->p_flag = SLOAD|SPRIV|(pp->p_flag & (SSIGABL|SRTPROC|SOUSIG));
#endif
#ifdef	BSDJOBCTLCONT
	cp->p_flag |= SWTED;	/* disallow wait until status changes */
#endif
	if (forktype == FORK_VFORK)
	    cp->p_flag |= SVFORK;

	cp->p_flag2 = pp->p_flag2 &~ PROCFLAGS2;

	switch (forktype) {
	case FORK_PROCESS:
		/* Fall through */
	case FORK_VFORK:
		if (pp->p_flag&SSYS)
			cp->p_flag &= ~SSYS;
		break;
	case FORK_DAEMON:
		cp->p_flag |= SLOAD|SSYS;
		break;
	}


	timerclear(&cp->p_realtimer.it_value);
	timerclear(&cp->p_realtimer.it_interval);
 	if (pp->p_flag & SRTPROC)
 		cp->p_pri = pp->p_rtpri;
 	else
 		cp->p_pri = PZERO - 1;
	cp->p_memresv = -1;
	cp->p_swpresv = -1;
	cp->p_start = time.tv_sec;
	cp->p_msem_info = (struct msem_procinfo *)0;
#ifdef UMEM_DRIVER
	cp->p_cttyfp = 0;
#endif	
	/*
	 * Make the new child proc, the real child
	 * of:
	 *	FORK_PROCESS: the current process
	 *	FORK_VFORK:   the current process
	 *	FORK_DAEMON:   proc[0].
	 */
	switch (forktype) {
	case FORK_PROCESS:
		/* Fall through */
	case FORK_VFORK:
		makechild(pp, cp);
		break;
	case FORK_DAEMON:
		makechild(&proc[0], cp);
		break;
	}

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(cp));

	/*
	 * Keep some statistics on fork.
	 */
	forkstat.cntfork++;
	forkstat.sizfork += DATASIZE(pp) + USTACKSIZE(pp);
	/*
	 * Increase references on shared objects.
	 */
	bump_shared_objects();

#ifdef __hp9000s800
	/*
	 * Save venom registers if parent has some to save. In order to
         * save time, the operation is performed here with inline code,
         * rather than via a call to graph3_coproc_save(). Values are
         * copied from the venom register values set by the parent directly
         * into the save state of the child.
	 */
	if (pp->graf_ss) {
		MALLOC(cp->graf_ss, caddr_t, sizeof(struct venom_ss), M_DYNAMIC,
			 M_WAITOK);
               /*
                * First we create a local copy of the "venom" global variable
                * in order to speed up accesses to the Venom page.
                * Since "venom" is a global, the compiler treats all references
                * to it as "volatile" (i.e., it will not cache it in a register,
                * but will perform a "ldw" to get its value on every reference).
                * Creating a local copy of "venom" will allow the compiler
                * to cache the pointer dereferences to the Venom page.
                * Note however that the Venom registers themselves ARE volatile,
                * so that all accesses to them are not cached.
                */
                venomp = venom;
                venom_ssp  = (struct venom_ss *)(cp->graf_ss);
                venom_pssp = (struct venom_ss *)(pp->graf_ss);
                _SYNC();
                venom_ssp->sup_control      = venomp->sup_control;
                _RSM(PSW_P, sm);
                venom_ssp->user_control     = venomp->user_control;
                venom_ssp->span_count       = venomp->span_count;
                venom_ssp->graphics_address = venomp->graphics_address;
                venom_ssp->zslope_integer   = venomp->zslope_integer;
                venom_ssp->zslope_fraction  = venomp->zslope_fraction;
                venom_ssp->z_fraction       = venomp->z_fraction;
                venom_ssp->z_integer        = venomp->z_integer;
                venom_ssp->blue_slope       = venomp->blue_slope;
                venom_ssp->blue_color       = venomp->blue_color;
                venom_ssp->red_slope        = venomp->red_slope;
                venom_ssp->red_color        = venomp->red_color;
                venom_ssp->green_slope      = venomp->green_slope;
                venom_ssp->green_color      = venomp->green_color;
                if (sm & PSW_P)
                   _SSM(PSW_P, 0);
            
             /* Now, copy info that is not venom register contents */

            venom_ssp->venomgrp             = venom_pssp->venomgrp;   
            venom_ssp->set_context_implicit = venom_pssp->set_context_implicit;
	} 
	else {
		cp->graf_ss = NULL;
	}
#endif

	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(cp));
	multprog++;

	switch (forktype) {
	case FORK_VFORK:
		break;
	case FORK_DAEMON:
		cp->p_flag |= SLOAD|SSYS;
		break;
	}

	/*
	 * Partially simulate the environment
	 * of the new process so that when it is actually
	 * created (by copying) it will look right.
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	pp->p_flag |= SKEEP;

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(cp));

	/*
	 * When the resume is executed for the new process,
	 * here's where it will resume.
	 */
	switch (procdup(forktype, pp, cp)) {
	case FORKRTN_ERROR:
		/*
		 * Error occurred creating process
		 * still in parent context.
		 */
		VASSERT(pp->p_reglocks == 0);
		unbump_shared_objects();

#ifdef __hp9000s800
		/*
		 * cleanup malloc'ed venom save state if error
		 */
		if (cp->graf_ss) {
			FREE(cp->graf_ss, M_DYNAMIC);
			cp->graf_ss = NULL;
		}
#endif

		return(FORKRTN_ERROR);
	case FORKRTN_CHILD: {
		/*
		 * Child process running.
		 */
		int i;

		SPINLOCK(PROCESS_LOCK(cp));

		VASSERT(cp->p_reglocks == 0);

		switch (forktype) {
		case FORK_PROCESS:
			/* Fall through */
		case FORK_VFORK:
			if (pp->p_flag&SSYS) {
				/*
				 * Parent was SSYS; so we need to reset to 
				 * default actions.
				 */
				u.u_procp->p_sigignore = 0;
				u.u_procp->p_sigmask = 0;
				u.u_procp->p_sigcatch = 0;
				for (i = 0; i < NSIG; i++) {
					u.u_signal[i] = SIG_DFL;
				}
			}
			break;
		case FORK_DAEMON:
			/*
			 * SSYS process ignore all signals.
			 */
			u.u_procp->p_sigignore = ~0;
			u.u_procp->p_sigmask = 0;
			u.u_procp->p_sigcatch = 0;
			for (i = 0; i < NSIG; i++) {
 				u.u_signal[i] = SIG_IGN;
			}

			/* 
			 * since this is a kernel process, it is not
			 * swap'able, and therefore we must use
			 * lockable memory for its resources
			 * to prevent deadlocks
			 */
			{
				int lock_count=1; /* one page for misc tables */
				struct proc *p = u.u_procp;

				lock_count += UPAGES;
				lock_count += hdl_swapicnt(p, p->p_vas);
				steal_lockmem(lock_count);
			}
			break;
		}

		/* 
		 * Set any fields which should not be 
		 * inherited in the U structure 
		 */
		u.u_flag &= ~UF_MEMSIGL;
		/* 
		 * The ITIMER_REAL in the u-area is ignored in favor
		 * of the p_realtimer in the proc structure.
		 */
		timerclear(&u.u_timer[ITIMER_VIRTUAL].it_value);
		timerclear(&u.u_timer[ITIMER_VIRTUAL].it_interval);
		timerclear(&u.u_timer[ITIMER_PROF].it_value);
		timerclear(&u.u_timer[ITIMER_PROF].it_interval);

#ifdef PESTAT
		/* start a new preemption measurement interval for the child */
		intr_start();	
#endif PESTAT
		SPINUNLOCK(PROCESS_LOCK(cp));
#ifdef	MP_LIFTTW
		/* Need to acquire kernel sema for child */
		PSEMA(&kernel_sema);
#endif	/* MP_LIFTTW */
		return(FORKRTN_CHILD); /* return 1 means child */
		break;
	}
	case FORKRTN_PARENT: 
		return (FORKRTN_PARENT);
		break;
	default:
		panic("newproc: unknown return value");
	}
	panic("newproc: statement never reached");
	return(FORKRTN_ERROR);
}

/*
 * This routine does all of the simple assignments.
 * Only put direct assignments here!
 */
fork_inherit(pp, cp)
	proc_t *pp, *cp;
{
	/* Must own parent process lock */
	T_SPINLOCK(PROCESS_LOCK(pp));

	cp->p_suid = pp->p_suid;
	cp->p_nice = pp->p_nice;
	cp->p_fss = pp->p_fss;
 	cp->p_rtpri = pp->p_rtpri;
 	cp->p_usrpri = pp->p_usrpri;
	cp->p_sigmask = pp->p_sigmask;
	cp->p_sigcatch = pp->p_sigcatch;
	cp->p_sigignore = pp->p_sigignore;
	cp->p_maxrss = pp->p_maxrss;
	cp->p_ttyp = pp->p_ttyp;
	cp->p_ttyd = pp->p_ttyd;
	cp->p_coreflags = pp->p_coreflags;
	cp->p_maxof = pp->p_maxof;
        cp->p_cdir = pp->p_cdir;
        cp->p_rdir = pp->p_rdir;
#ifdef __hp9000s300
	cp->p_dil_signal = pp->p_dil_signal;
#endif
#ifdef MP
	/*      cp->p_desproc = pp->p_desproc; */
	cp->p_desproc = getprocindex();
	cp->p_descnt = pp->p_descnt;
#endif
#ifdef PESTAT
	/* kernel nonpreemptable timing statistics */
	cp->intr_callid = pp->intr_callid;
	cp->intr_trace  = pp->intr_trace;
#endif

}


/*
 * Bump shared references that current process is
 * using.
 */
bump_shared_objects()
{
	struct file *fp;
	int i;

	MP_ASSERT(!spinlocks_held(),"bump_shared_objects(): spinlocks held");

	/*
	 * Increase reference counts on shared objects.
	 */
	for (i = 0; i <= u.u_highestfd; i++){
		if ((fp = getf_no_ue(i)) != NULL) {
			SPINLOCK(file_table_lock);
			fp->f_count++;
			SPINUNLOCK(file_table_lock);
		}
	}

	VN_HOLD(u.u_cdir);
	update_duxref(u.u_cdir,1,0);
	if (u.u_rdir) {
		VN_HOLD(u.u_rdir);
		update_duxref(u.u_rdir,1,0);
	}
	crhold(u.u_cred);
}

/*
 * decrement shared references that current process is
 * using.
 */
unbump_shared_objects()
{
	struct file *fp;
	int i;

	MP_ASSERT(!spinlocks_held(),"unbump_shared_objects(): spinlocks held");

	/*
	 * Decrease reference counts on shared objects.
	 */
	for (i = 0; i <= u.u_highestfd; i++) {
		if ((fp = getf_no_ue(i)) != NULL) {
			SPINLOCK(file_table_lock);
			fp->f_count--;
			SPINUNLOCK(file_table_lock);
		}
	}
	update_duxref(u.u_cdir,-1,0);
	VN_RELE(u.u_cdir);
	if (u.u_rdir) {
		update_duxref(u.u_rdir,-1,0);
		VN_RELE(u.u_rdir);
	}
	crfree(u.u_cred);
}

proc_unhash(pp)
	proc_t *pp;
{
#ifdef MP
        sv_sema_t       sema_save;
#endif MP
	PXSEMA(&pm_sema, &sema_save);
        SPINLOCK(sched_lock);
        SPINLOCK(PROCESS_LOCK(pp));

	if (!(uremove(pp)))
        	panic("proc_unhash: lost uidhx");
        if (!(premove(pp)))
                panic("proc_unhash: lost idhash");
        if (!(gremove(pp)))
                panic("proc_unhash: lost pgrphx");
        if (!(sremove(pp)))
                panic("proc_unhash: lost sidhx");

	SPINUNLOCK(sched_lock);
        SPINUNLOCK(PROCESS_LOCK(pp));
	VXSEMA(&pm_sema, &sema_save);
}
proc_hash(pp, uid, pid, pgrp, sid)
	proc_t *pp;
	uid_t uid;
	pid_t pid;
	pid_t pgrp;
	sid_t sid;
{
	register int s;
#ifdef	MP
	sv_lock_t	slt;
#endif	/* MP */

	s = UP_SPL6();
	SPINLOCKX(sched_lock, &slt);
	SPINLOCK(PROCESS_LOCK(pp));

	/* 
	 * hash process into the uid hash chain
	 * (ulink is not spl protected)
	 */
	ulink(pp, uid);
	pp->p_uid = uid;

	/* 
	 * hash process into the pid hash chain
	 * (plink is not spl protected)
	 */
	plink(pp, pid);
	pp->p_pid = pid;

	/* 
	 * hash process into the process group hash chain
	 */
	glink(pp, pgrp);
	pp->p_pgrp = pgrp;

	/* 
	 * hash process into the session hash chain
	 */
	slink(pp, sid);
	pp->p_sid = sid;

	SPINUNLOCK(PROCESS_LOCK(pp));
	SPINUNLOCKX(sched_lock, &slt);
	UP_SPLX(s);
}

#define VFORK_STACK_OVHD 0x500 /* Room enough to call newproc & sleep! */

int
vfork_buffer_init(p)
    register struct proc *p;
{
    register struct vforkinfo *vi;
    register unsigned char *s;
    unsigned long alloc_size;
    unsigned long frame_size;

#ifdef __hp9000s300

    /* Compute stack + u-area size -- Series 300 specific */

    frame_size = KSTACKADDR - (unsigned long)&frame_size;
    frame_size += sizeof(struct user);
#endif

#ifdef __hp9000s800

    /* Compute stack + u-area size -- Series 700/800 specific */

    frame_size = (unsigned long) &frame_size - UAREA;
#endif

    frame_size += VFORK_STACK_OVHD;

    alloc_size = sizeof(struct vforkinfo) + frame_size;


    /* Convert to pages */

    alloc_size = btorp(alloc_size);

    /* Allocate memory */

    s = (unsigned char *)kalloc(alloc_size, KALLOC_NOZERO);
    if (s == (unsigned char *)0)
	return(-1);

    /* Fill in information */

    vi = (struct vforkinfo *)s;
    s += sizeof(struct vforkinfo);
    vi->u_and_stack_buf = s;

    vi->vfork_state = VFORK_INIT;
    vi->buffer_pages = alloc_size;

    /* save frame_size so we can check when we do actual save */

    vi->u_and_stack_len = frame_size;

    vi->prev = p->p_vforkbuf;

#ifdef __hp9000s800
	/* HP REVISIT.  This hack is to workaround a vfork problem in libc */

    vi->saved_rp_ptr = u.u_sstatep->ss_sp - 0x18;
    copyin((caddr_t)vi->saved_rp_ptr, &(vi->saved_rp),sizeof(long));

#endif /* __hp9000s800 */

    p->p_vforkbuf = vi;

    return(0);
}

void
#ifdef __hp9000s300
vfork_transfer(stackp)
    unsigned long stackp;
#else
vfork_transfer()
#endif
{
    register struct proc *p;
    register struct vforkinfo *vi;
    register int frame_size;
#ifdef __hp9000s800
    unsigned long stackp;
    register preg_t *prp;
#endif

    p = u.u_procp;
    vi = p->p_vforkbuf;

    switch (vi->vfork_state) {

    case VFORK_INIT:
    case VFORK_CHILDRUN:
	break;

    case VFORK_PARENT:

	if (p != vi->pprocp)
	    panic("Bad parent entry into vfork_transfer.");

	/* Save stack and u_area in buffer. */

#ifdef __hp9000s300

	/* Compute stack + u_area size -- Series 300 specific */

	frame_size = KSTACKADDR - stackp;
	frame_size += sizeof(struct user);

	if (frame_size > vi->u_and_stack_len)
	    panic("VFORK_STACK_OVHD too small.");

	bcopy((caddr_t)stackp, (caddr_t)vi->u_and_stack_buf, frame_size);
	vi->u_and_stack_addr = (unsigned char *)stackp;
#endif
#ifdef __hp9000s800

	/* Compute stack + u_area size -- Series 700/800 specific */

	frame_size = (int) &stackp - UAREA;

	if (frame_size > vi->u_and_stack_len)
	    panic("VFORK_STACK_OVHD too small.");

	bcopy((caddr_t)UAREA, (caddr_t)vi->u_and_stack_buf, frame_size);
#endif

	vi->u_and_stack_len = frame_size;

	/*
	 * Prepare U area for child, also do proc structure
	 * assignments for child that would have been done in createU
	 * and procdup for a normal fork.
	 */

#ifdef __hp9000s300
	vi->cprocp->p_stackpages = p->p_stackpages;
	vi->cprocp->p_addr = p->p_addr;

	u.u_vapor_mlist = NULL;
#endif

	bzero((caddr_t) &u.u_ru, sizeof(struct rusage));
	bzero((caddr_t) &u.u_cru, sizeof(struct rusage));
	u.u_outime = 0;

	u.u_pcb.pcb_sswap = (int *)&u.u_ssave;

#ifdef __hp9000s300
	/* Interval timers are not inherited across forks.  They   */
	/* must be cleared before the new process gets to run,     */
	/* to assure that it does not receive a signal from        */
	/* expiration of a timer.                                  */
	/* Note that u_timer[ITIMER_REAL] not used; p_realtimer is */
	/* cleared  in creation of the new proc structure (and its */
	/* it_value field is redundantly cleared in newproc().     */

	/* NOTE: This appears to be redundant, needs to be investigated */

	timerclear(&u.u_timer[ITIMER_VIRTUAL].it_value);
	timerclear(&u.u_timer[ITIMER_VIRTUAL].it_interval);
	timerclear(&u.u_timer[ITIMER_PROF].it_value);
	timerclear(&u.u_timer[ITIMER_PROF].it_interval);
#endif

	u.u_procp = vi->cprocp;

	/* make sure we don't reenter until child restores us */

	vi->vfork_state = VFORK_CHILDRUN;
	break;

    case VFORK_CHILDEXIT:

	/*
	 * Note: For Series 700/800, if state is VFORK_CHILDEXIT
	 * due to the child exiting (instead of the child exec'ing),
	 * then we are currently not in the parents or child's
	 * context. Therefore, don't reference "u" in any way.
	 */

	if (p != vi->cprocp)
	    panic("Bad child entry into vfork_transfer.");

	/* Restore stack and U-area of parent */

#ifdef __hp9000s300
	bcopy((caddr_t)vi->u_and_stack_buf,
	      (caddr_t)vi->u_and_stack_addr,
	      vi->u_and_stack_len);

	/*
	 * The child may have been swapped. If this happened, all
	 * new page tables would have been constructed, which would
	 * mean that the parents p_segptr and p_addr fields are
	 * stale. We reset them here, just in case. Note that we
	 * cannot use the child's vas, since in the case of exec,
	 * the new one has already been allocated.
	 */

	vi->pprocp->p_segptr = vi->pprocp->p_vas->va_hdl.va_seg;
	vi->pprocp->p_addr = vastoipte(vi->pprocp->p_vas, UAREA);
#endif
#ifdef __hp9000s800

	/*
	 * Since we should not reference U and we are not on the
	 * interrupt stack, we have to expand the reglock and regrele
	 * macros, since they reference U.
	 */

	prp = findpregtype(vi->pprocp->p_vas, PT_UAREA);
	vi->pprocp->p_reglocks++;
	vm_psema(&prp->p_reg->r_lock, PZERO);
	copytopreg(prp, 0, (caddr_t)vi->u_and_stack_buf, vi->u_and_stack_len);
	vm_vsema(&prp->p_reg->r_lock, 0);
	vi->pprocp->p_reglocks--;

#endif

        /*
         * While the child borrowed our vas, it changed vas->va_proc to
         * point to itself.  Since we now have the vas back, we reset
         * vas->va_proc to point to our proc entry.
         */
        vi->pprocp->p_vas->va_proc = vi->pprocp;

	vi->vfork_state = VFORK_BAD;
	p->p_vforkbuf = 0;

#ifdef __hp9000s300

	/* Wake parent up */

	wakeup((caddr_t)&(p->p_vforkbuf));
#endif
	break;

    case VFORK_BAD:
	panic("vfork_transfer entered in bad state.");

#ifdef OSDEBUG
    default:
	panic("Bad vfork type in vfork_transfer.");
#endif
    }

    return;
}
