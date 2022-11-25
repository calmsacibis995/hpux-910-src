/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_exit.c,v $
 * $Revision: 1.72.83.7 $       $Author: rpc $
 * $State: Exp $        $Locker:  $
 * $Date: 94/10/11 08:13:31 $
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

#include "../h/types.h"
#include "../h/debug.h"
#include "../machine/psl.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/tty.h"
#ifdef  FSD_KI
#include "../h/ki_calls.h"
#endif  /* FSD_KI */


#ifdef __hp9000s300

#include "../machine/reg.h"
#include "../machine/trap.h"
#define	STACK_300_OVERFLOW_FIX

/* XXX add the following to pcb.h someday.... */
#define pcb_locregs pcb_float[2]

#endif

#ifdef __hp9000s800
#include "../machine/vmparam.h"
#include "../h/pfdat.h"
#include "../h/uio.h"
#endif

#include "../dux/dm.h"		/* To get NFSCALL macro.  Should be in nfs.h */

#ifdef AUDIT
#include "../h/audit.h"		/* auditing support  */
#endif

#ifdef UMEM_DRIVER
#include "../machine/umem.h"
int (*umem_exit_hook)() = NULL;
#endif


extern struct dux_context dux_context;

#ifdef _WSIO
exit_nop()
{
}

int (*do_dil_exit)() = exit_nop;

dil_exit(fp)
register struct file *fp;
{
	register struct buf *bp = (struct buf *)fp->f_buf;

	if (bp != NULL && bp->b_flags & B_DIL)
		(*do_dil_exit)(bp);
}
#endif /* _WSIO */

/*
 * Exit system call: pass back caller's arg
 */
rexit()
{
	register struct a {
		int rval;
	} *uap;

	uap = (struct a *)u.u_ap;
	exit((uap->rval & 0377) << 8);
}

#ifdef __hp9000s800
#ifndef MP
extern struct fast_protid_list fast_protid_list;
#endif /* not MP */
#endif /* hp9000s800 */
#ifdef MP
extern int iosys_valid;
#endif /* MP */

int (*iomap_exitptr)();		/* pointer to iomap exit routine */

/*
 * Release resources.
 * Save u-area for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes and dispose of children.
 */
exit(rv)
	int rv;
{
	register struct proc *p, *q, *startq, *endq;
	register struct proc *sigtarget;
	register int i, szomb = 0;
	register short nextactive, prevactive;
#ifndef MP
	register int noproc;
#endif MP
	int realitexpire();
	extern void (*graphics_exitptr)();
	extern void msem_release();
	int s;
#ifdef XTERMINAL
	extern char sysname[] ;
#endif /* XTERMINAL */
#ifdef MP
	int pm_locked;
#endif /* MP */
	struct vforkinfo *vi;
#ifdef __hp9000s300
	extern int unhash;
#endif /* __hp9000s300 */

	p = u.u_procp;

	MP_ASSERT(! proc_owns_semas(p), "exit: process owns semaphore");

#ifdef MP
	if (iosys_valid) {
		PSEMA(&pm_sema);
		pm_locked = 1;
	}else{
		pm_locked = 0;
	}
#endif /* MP */
	s=spl2();
	SPINLOCK(sched_lock);

	p->p_flag |= SWEXIT;

#ifdef PGINPROF
	vmsizmon();
#endif
	p->p_flag &= ~STRC;
	p->p_cpticks = 0;
	p->p_pctcpu = 0;
	p->p_sigignore = ~0;
	for (i = 0; i < SIGARRAYSIZE; i++)
		u.u_signal[i] = SIG_IGN;
	(void) untimeout(realitexpire, (caddr_t)p);

#ifdef __hp9000s300
	/* clear DIL interrupt info on exit, no longer a DIL process */
	p->p_dil_signal = 0;
	p->p_dil_event_f = 0;
	p->p_dil_event_l = 0;
	p->p_flag2 &= ~S2SENDDILSIG;
#endif /* __hp9000s300 */
	SPINUNLOCK(sched_lock);
	splx(s);

#ifdef UMEM_DRIVER
	if (umem_exit_hook && p->p_cttyfp) {
		struct umem_data *up;
		up = (struct umem_data *) p->p_cttyfp;
		if (up->mpid == p->p_pid)	
			(*umem_exit_hook)(up);
	}
	p->p_cttyfp = 0;
#endif
		
	{
		struct tty *tp = u.u_procp->p_ttyp;

		if (tp != NULL) {  /* exiting process has a controlling tty */
			if ((p->p_pid == p->p_sid)	/* session leader */
			    && (tp->t_cproc != NULL)
			    && (tp->t_cproc->p_pid == p->p_pid)) {
				/*
				 * controlling process exiting,
				 * the tty belongs to this session,
				 * send SIGHUP to fg pgrp
				 */
				gsignal((pid_t)tp->t_pgrp, SIGHUP);
				/*
				 * free tty for another session
				 */
				clear_ctty_from_session(p);
				tp->t_pgrp = 0;
				tp->t_cproc = NULL;

#ifdef __hp9000s300
				new_cons_close(tp);
#endif
#ifdef __hp9000s800
				new_cn_close(tp);
#endif
			}
#ifdef XXX_CTTY_NOT_SET
			else if (p->p_pgrp == tp->t_pgrp) {
				/*
				 * exiting process is in the fg pgrp of ctty
				 */
				for (q = &proc[pgrphash[PGRPHASH(p->p_pgrp)]];
				     q != &proc[0]; q = &proc[q->p_pgrphx])
					if ((q->p_pgrp == p->p_pgrp)
					    && (q->p_pid != p->p_pid))
						break;
				if (q == &proc[0]) {
					/*
					 * p is the last process in the ctty's
					 * fg pgrp.
					 * Set ctty to have no fg pgrp.
					 */
					tp->t_pgrp = PGID_NOT_SET;
				}
			}
#endif /* XXX_CTTY_NOT_SET */
		}
	}

#ifdef __hp9000s800
/* For MP, the fast_protid_list owner is cleared in resume(). */
#ifndef MP
	/*
	 * If the fast_protid_list is currently valid for this process,
	 * invalidate it.
	 */
	if (fast_protid_list.sid == u.u_procp->p_upreg->p_space)
		fast_protid_list.sid = 0;
#endif /* not MP */
#endif /* hp9000s800 */

	/*
	 * Release msemaphores, if used.
	 */

	if (p->p_msem_info != (struct msem_procinfo *)0)
	    msem_release(p);

	/*
	 * Do exit processing for graphics, if present.
         */
	if (p->p_flag2 & SGRAPHICS) {
	    VASSERT(graphics_exitptr);
	    (*graphics_exitptr)();
	}

	/*
	 * Do exit processing for the iomap driver, if needed.
	 */
	if (p->p_vas->va_flags & VA_IOMAP) {
	    VASSERT(iomap_exitptr!=NULL);
	    (*iomap_exitptr)();
	}

	if ((p->p_flag & SVFORK) == 0) {

	    /*
	     * Release virtual memory.
	     */

	    dispreg(u.u_procp, (struct vnode *)0, PROCATTACHED);

#ifdef __hp9000s300
	    /* release memory for 68040 bus error handling */
	    if (processor == M68040) {
		VASSERT(u.u_pcb.pcb_locregs);
		kmem_free(u.u_pcb.pcb_locregs, 
			  sizeof(struct mc68040_exception_stack));
	    }
#endif /* hp9000s300 */
	}

#ifdef __hp9000s300
	if (u.u_pcb.pcb_dragon_bank != -1)
		dragon_bank_free();
#endif /* hp9000s300 */

#ifdef __hp9000s800
	KPREEMPTPOINT();
#endif

	for (i = 0; i <= u.u_highestfd; i++) {
		struct file *f;
		extern struct file *getf_no_ue();

		f = getf_no_ue(i);
		if (f != NULL) {
#ifdef __hp9000s300
			dil_exit(f);
#endif
			/*
			 * Guard againt u.u_error being set by psig()
			 * u.u_error should be 0 if exit is called from
			 * syscall().  In the case of exit() being called
			 * by psig - u.u_error could be set to EINTR.  We
			 * need to clear this so that routines called by
			 * exit() would't return on non-zero u.u_error.
			 * Example: vno_lockrelease()
			 */
			u.u_error = 0;
			closef(f);
			/*
			 * Save flags until after closef() because flags
			 * are used in vno_lockrelease, called from closef().
			 */
			uffree(i);
			KPREEMPTPOINT();
		}
	}
	/* Free up the space taken by the table of ofile_t pointers. */
	kmem_free ((caddr_t)u.u_ofilep,(sizeof(struct ofile_t *))*NFDCHUNKS(u.u_maxof));
	u.u_ofilep = NULL;
	u.u_highestfd = -1;
	release_cdir();
	if (u.u_rdir) {
		update_duxref(u.u_rdir, -1, 0);
		VN_RELE(u.u_rdir);
	}

	/*
	 * If setcontext has been used and a separated context buffer
	 * exists, release it.
	 */
	if (u.u_cntxp != (char **)&dux_context) {
		kmem_free(u.u_cntxp, sizeof(struct dux_context));
		u.u_cntxp = (char **)&dux_context;
	}

#if defined(hp9000s800) && defined(AUDIT)
	if (AUDITEVERON())
		save_aud_data();
#endif /* hp9000s800 && AUDIT */

	/*
	 * Free NFS lock manager resources.
	 */
	if (p->p_flag2 & SISNFSLM) {
		NFSCALL(NFS_LMEXIT)();
		p->p_flag2 &= ~SISNFSLM;
	}

	if ((p->p_flag & SSYS) == 0 || u.u_nsp == 0) {
		/*
		 * Don't write acct record for system processes,
		 * i.e. netisr and csp's.
		 */

		acct(rv);
#ifdef AUDIT
		if (AUDITEVERON())
			save_aud_data();
#endif

	}


	crfree(u.u_cred);

#if defined(SEMA) || defined(__hp9000s300) || defined(hp9000s700)
	/*
	 * Do exit processing for semaphore stuff.
	 */
	semexit();
#endif /* SEMA || __hp9000s300 || hp9000s700 */
	/*
	 * Destroy any adopted processes.
	 */
	if (p->p_flag2 & SADOPTIVE) {
		/*
		 * We need to search all processes here, not just active
		 * ones, because there could be zombies that the debugger
		 * has not waited for that will never be cleaned up by
		 * its parent if the p_dptr filed is not reset.
		 * Also, if the process is a zombie, we send SIGCLD to
		 * the parent and wake it up.
		 *
		 * This is not a significant performance hit since this
		 * code is only executed by exiting processes (most likely
		 * a debugger) that have adopted another process.
		 */
		for (q = proc; q < procNPROC; q++) {
			if (q->p_stat != NULL && q->p_dptr == p) {
				q->p_dptr = (struct proc *)0;
				q->p_flag &= ~STRC;
				if (q->p_stat == SZOMB && p != q->p_pptr) {
					psignal(q->p_pptr, SIGCLD);
					wakeup((caddr_t)q->p_pptr);
				}
				else
					psignal(q, SIGKILL);
			}
		}
	}

	/*
	 * We need to signal our pgrp if our exit()ing is causing it
	 * to become an orphan -- and it has stopped processes in it.
	 */
	if (orphaned(p->p_pgrp,p->p_sid,p) && 	 /* now an orphan and*/
	   !orphaned(p->p_pgrp,p->p_sid,(struct proc *)0) && /*wasn't before */
	    any_stopped(p->p_pgrp)) {		 /* and has stopped procs */
		/*
		 * p is skipped in the orphaned() check just in case
		 * its parent is the only one that can restart this pgrp.
		 * If that's the case, once p exits, no process will
		 * restart this pgrp.
		 */
		gsignal(p->p_pgrp, SIGHUP);
		gsignal(p->p_pgrp, SIGCONT);
	}

	if (p->p_cptr != NULL) {
		startq = p->p_cptr;
		for (q = startq; q != NULL; q = q->p_osptr) {
			endq = q;
			KPREEMPTPOINT();
			if (q->p_pptr != p)
				panic("exit: bad child");
			q->p_pptr = &proc[S_INIT];
			q->p_ppid = PID_INIT;
			if (q->p_stat == SZOMB) {
				szomb++;
				if (   q->p_vas != (vas_t *)0
				    && (q->p_flag & SVFORK) == 0 )
					kissofdeath(q);
			}
			/*
			 * Traced processes are killed since their
			 * existence means someone is screwing up.
			 *
			 * Check for orphaned pgrp.
			 * Stopped processes in an orphaned pgrp are sent
			 * a HANGUP and a CONTINUE.
			 *
			 * This is designed to be ``safe'' for setuid
			 * processes since they must be willing to tolerate
			 * HANGUPs anyways.
			 */
			if (q->p_flag&STRC) {
				q->p_flag &= ~STRC;
				psignal(q, SIGKILL);
			}
			/*
			 * here, only zap those for which the parent is also
			 * in another pgrp - as we have gotten the only other
			 * case where it is our exit()ing that is doing the
			 * orphaning, above.
			 */
			else if ((q->p_sid == p->p_sid)	/* same session */
				   && (q->p_pgrp != p->p_pgrp)
				   && orphaned(q->p_pgrp,q->p_sid,
							(struct proc *)0)
				   && any_stopped(q->p_pgrp)) {
				gsignal(q->p_pgrp, SIGHUP);
				gsignal(q->p_pgrp, SIGCONT);
			}

			/*
			 * Protect this process from future tty signal,
			 * clear TSTP/TTIN/TTOU if pending.
			 */
			(void)spgrp2(q);
		}

		/* link this chain to proc[S_INIT]'s children chain */
		if (proc[S_INIT].p_cptr && startq) {
			proc[S_INIT].p_cptr->p_ysptr = endq;
			endq->p_osptr = proc[S_INIT].p_cptr;
		}
		if (startq)
			proc[S_INIT].p_cptr = startq;
		/* tell init she has zombies waiting */
		if (szomb) {
			psignal(&proc[S_INIT], SIGCLD);
			wakeup((caddr_t)&proc[S_INIT]);
		}
	}
	KPREEMPTPOINT();

	/*
	 * spl5() is used to keep schedcpu from being scheduled, and
	 * atempting to alter the priority of a process not on the run queue
	 * after we have set noproc to one.
	 */
	(void)spl5();		/* hack for mem alloc race XXX */

#ifdef MP
	/* DB_SENDRECV */
	/* Aquires and releases schedlock */
	dbexit();
#endif

	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);
	multprog--;

#if 	defined(hp9000s800) && defined(PESTAT)
	/*
	 * Setting noproc to 2 rather than 1 here allows valid preemption
	 * measurements to still be taken during subsequent processing in
	 * exit() (see measurement code).
	 */
#ifdef MP
	setnoproc(2);
#else
	noproc = 2;
#endif

#else 	/* ! defined(hp9000s800) && defined(PESTAT) */

#ifdef MP
	setnoproc(1);
#else
	noproc = 1;
#endif
#endif 	/* ! defined(hp9000s800) && defined(PESTAT) */

	if (p->p_pid == PID_INIT) {
#ifdef XTERMINAL
		for (i = 0; i < 10; i++)
			sysname[i] = '\0' ;
		boot(0,0);
#else
		printf("init died with return value %d.\n", rv);
		panic("init died");
#endif /* XTERMINAL */
	}

	/* unlink from active list of procs */
	nextactive = p->p_fandx;
	prevactive = p->p_pandx;
	(proc + nextactive)->p_pandx = prevactive;
	(proc + prevactive)->p_fandx = nextactive;
	SPINUNLOCK(activeproc_lock);

	p->p_stat = SZOMB;
	p->p_xstat = rv;
	ruadd(&u.u_ru, &u.u_cru);

	/*
	 *  Choose which process to send SIGCLD to
	 */
	SPINLOCK(PROCESS_LOCK(sigtarget));
	if (p->p_flag&SSYS) {
		/*
		 * If we are a daemon process and we are exiting
		 * we will make ourselves the child of init
		 * so that init will clean us up.
		 */
		abandonchild(p);
		makechild(&proc[S_INIT], p);

		/*
		 * Now wakeup init to clean us up.
		 */
		sigtarget = &proc[S_INIT];
	}
	else {
		if (p->p_dptr && (p->p_dptr != p->p_pptr))
			sigtarget = p->p_dptr;
		else
			sigtarget = p->p_pptr;
	}
	pm_psignal(sigtarget, SIGCLD);
	SPINUNLOCK(PROCESS_LOCK(sigtarget));
	wakeup((caddr_t)sigtarget);

#ifndef MP
	/* DB_SENDRECV */
	dbexit();
#endif

	VASSERT(p->p_reglocks == 0);

#ifdef MP
	if(pm_locked){
		VSEMA(&pm_sema);
	}
#endif /* MP */
#ifdef  FSD_KI
	/* The following kludge is because an exit() systemcall does 
	 * not finish -- IE does not get to a pop_TOS -- fake it here
	 */
	if (ki_kernelenable[KI_GETPRECTIME])
	{
		register int my_index = getprocindex();

		if (ki_kernelenable[KI_SYSCALLS])
		{
			ki_kernelcounts(my_index)[KI_SYSCALLS]++;
		}
		ki_syscallcounts(my_index)[SYS_exit]++;
		ki_clockcounts(my_index, KT_SYS_CLOCK)++;
	}
#endif  /* FSD_KI */

	MP_ASSERT(! proc_owns_semas(p), "exit: process owns semaphore");
#ifdef	STACK_300_OVERFLOW_FIX
	/* This will probably added to the 800 soon */
	vapor_malloc_free();
#endif	/* STACK_300_OVERFLOW_FIX */

	if (p->p_flag & SVFORK) {

	    /* Change vforkbuf state so resume will restore parent */

	    vi = p->p_vforkbuf;
	    vi->vfork_state = VFORK_CHILDEXIT;
#ifdef __hp9000s300
	    if (rq_empty()) 
		wakeup((caddr_t)&unhash);
#endif /* __hp9000s300 */

#ifdef __hp9000s800

	    /* Wake parent up */

	    wakeup((caddr_t)&(p->p_vforkbuf));
#endif
	}

	swtch();	/* releases sched_lock */
}


int
any_stopped(pgrp)	/* returns 1 if any proc in a pgrp is stopped, else 0 */
	pid_t pgrp;
{
	register struct proc *p;
	register int s;

	s = spl6();	/* mux driver can try to change pgrp's on the ICS ... */

	for (p = &proc[pgrphash[PGRPHASH(pgrp)]]; p != &proc[0]; p = &proc[p->p_pgrphx]) {
		if ((p->p_pgrp == pgrp) && (p->p_stat == SSTOP) &&
				!(p->p_flag & STRC)) {
			splx(s);
			return 1;
		}
	}

	splx(s);
	return 0;
}


orphaned(pgrp,sid,q)		/* returns 1 if pgrp is orphaned, else 0 */
	pid_t pgrp;
	sid_t sid;
	struct proc *q;		/* skip proc q in the pgrp */
{
	register struct proc *p;
	register int s;
	sv_lock_t	nsp;	/* spinlock save state */
	int nonempty = 0;

	SPINLOCKX(sched_lock, &nsp);
	/* mux driver can try to change pgrp's on the ICS ... */
	s = UP_SPL7();

	for (p = &proc[pgrphash[PGRPHASH(pgrp)]];
			p != &proc[0]; p = &proc[p->p_pgrphx]) {
		if ((p->p_pgrp == pgrp) && (p != q)) {
			if ((p->p_stat != SIDL) &&
			    (p->p_pptr->p_pgrp != pgrp) &&
			    (p->p_pptr->p_sid == sid))
			    {
				UP_SPLX(s);
				SPINUNLOCKX(sched_lock, &nsp);
				return(0);	/* not orphaned */
			    }
			nonempty = 1;
		}
	}

	UP_SPLX(s);
	SPINUNLOCKX(sched_lock, &nsp);
	return(nonempty);
}

#ifdef __hp9000s300
/*
 * modified for 68K:
 *	(1) options parameter passed in R1
 *	(2) rusage parameter passed in AR0
 *      (3) status value returned through pointer as well as in R1
 * (1) and (2) because R0 is unavailable, (3) for object compatibility
 */
pid_t
wait()
{
	register struct a {
		unsigned *stat_loc;
	} *uap;

	struct rusage ru, *rup;

	uap = (struct a *)u.u_ap;

	if ((((struct exception_stack *)u.u_ar0)->e_PS & PSL_ALLCC)
	    != PSL_ALLCC)
		u.u_error = wait1(-1, 0, (struct rusage *)0);
	else {
		rup = (struct rusage *)u.u_ar0[AR0];
		u.u_error = wait1(-1, u.u_ar0[R1], &ru);
		if (u.u_error)
			return;
		ru_ticks_to_timeval(ru.ru_stime);
		ru_ticks_to_timeval(ru.ru_utime);
		u.u_error =
		  copyout4((caddr_t)&ru, (caddr_t)rup, sizeof(struct rusage)/4);
	}
	if (u.u_error)
		return;

	u.u_r.r_val2 &= 0xffff;	/* 16 bits significant -- clear sign ext */
	if (uap->stat_loc != NULL &&
	    suword((caddr_t)uap->stat_loc, u.u_r.r_val2))
		u.u_error = EFAULT;
}
#endif /* __hp9000s300 */

#ifdef __hp9000s800

pid_t
wait()
{
	register struct a {
		union wait *status;
	} *uap = (struct a *)u.u_ap;

	u.u_error = wait1(-1, 0, (struct rusage *)0);
	if (u.u_error)
		return;
	if (uap->status != NULL && suword((caddr_t)uap->status, u.u_r.r_val2))
		u.u_error = EFAULT;
}
#endif /* hp9000s800 */

wait3()
{
	register struct a {
#ifdef __hp9000s300
		unsigned *stat_loc;
#endif
#ifdef __hp9000s800
		union wait *status;
#endif
		int options;
		struct rusage *rusage;
	} *uap = (struct a *)u.u_ap;

	if (uap->rusage != 0) {
		u.u_error = EINVAL;
		return;
	}

	u.u_error = wait1(-1, uap->options, (struct rusage *)0);
	if (u.u_error)
		return;
#ifdef __hp9000s300
	if (uap->stat_loc != NULL &&
	    suword((caddr_t)uap->stat_loc, u.u_r.r_val2))
#endif
#ifdef __hp9000s800
	if (uap->status != NULL &&
	    suword((caddr_t)uap->status, u.u_r.r_val2))
#endif
		u.u_error = EFAULT;
}

/*
 * POSIX version of wait() system call.  The pid argument tells
 * which processes are of interest:
 *	-1	all processes
 *	>0	only process with process ID == pid
 *	0	only processes in same process group as caller
 *	<-1	only processes with process group ID == -pid
 */
pid_t
waitpid()
{
	register struct a {
		int pid;
		unsigned *stat_loc;
		int options;
	} *uap = (struct a *)u.u_ap;

	if (uap->pid == 0)
		uap->pid = -u.u_procp->p_pgrp;	/* translate for wait1() */
	if ((uap->options) & ~_WAITMASK) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = wait1((pid_t)uap->pid, uap->options, (struct rusage *)0);

	if (u.u_error)
		return;
	if (uap->stat_loc != NULL &&
	    suword((caddr_t)uap->stat_loc, u.u_r.r_val2))
		u.u_error = EFAULT;
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 * The pid argument tells which processes are of interest;
 * it is the same as waitpid() except that the case of 0
 * has been translated to a specific negated prgp ID.
 */
wait1(pid, options, ru)
	register pid_t pid;
	register int options;
	struct rusage *ru;
{
	register f;
	register struct proc *p;

	sv_sema_t	wait1_ss;
	int		retval;

#ifdef MP
	int		pm_locked;

	if(iosys_valid) {
		PXSEMA(&pm_sema,&wait1_ss);
		pm_locked = 1;
	}else{
		pm_locked = 0;
	}
#endif /* MP */

loop:
	f = 0;
	/*
	 * Find and count our step-children who are not our children.
	 * If zombies, undebug them and signal their real parents.
	 * If stopped, signal the step-parent.
	 */
	if (u.u_procp->p_flag2 & SADOPTIVE)
                /*
                 * We need to search all processes here.  Searching
                 * only the active process list is not good enough
                 * because zombie processes are no longer on the
                 * active process list.
                 */
                for (p = proc; p < procNPROC; p++) {
			if (p->p_dptr == u.u_procp && p->p_pptr != u.u_procp &&
			   (pid == -1 ||
				(pid > 0 && pid == p->p_pid) ||
				(-pid == p->p_pgrp))) {
				f++;
				if (p->p_stat == SZOMB) {
					u.u_r.r_val1 = p->p_pid;
					u.u_r.r_val2 = p->p_xstat;
					p->p_dptr = 0;
					psignal(p->p_pptr, SIGCLD);
					wakeup((caddr_t)p->p_pptr);
					retval = 0;
					goto leave_wait1;
				}
				if (p->p_stat == SSTOP &&
				    !(p->p_flag & SWTED) &&
				    (p->p_flag & STRC || options & WUNTRACED)) {
					p->p_flag |= SWTED;
					u.u_r.r_val1 = p->p_pid;
					u.u_r.r_val2 =
						(p->p_cursig << 8) | WSTOPPED;
					retval = 0;
					goto leave_wait1;
				}
			}
		}
	for (p = (u.u_procp)->p_cptr; p != NULL; p = p->p_osptr) {
		if (!(pid==-1 || (pid>0 && pid==p->p_pid) || (-pid==p->p_pgrp)))
			continue;	/* not a process we're interested in */
		f++;
		if (p->p_dptr && (p->p_dptr != p->p_pptr))
			continue;	/* give the debugger the first shot */
		if (p->p_stat == SZOMB) {
			/*
			 * This check is not necessary in System III/V
			 * because issig() is always called between
			 * executing user code on two processes, which
			 * guarantees that no ignored zombies exist at
			 * this point.  This is not true on our system,
			 * at least in the case of page faults.
			 */
			p->p_dptr = 0;

			if (u.u_signal[SIGCLD-1] == SIG_IGN) {
				proc_unhash(p);
				freeproc(p);
				f--;	      /* pretend we never saw it */
			}
			else {
				u.u_r.r_val1 = p->p_pid;
				u.u_r.r_val2 = p->p_xstat;

				/* not being implemented in regions */
				if (ru) {
					VASSERT(ru == 0);
					/* Code to copy child's u_ru to ru */
				}
				proc_unhash(p);
				freeproc(p);
				retval = 0;
				goto leave_wait1;
			}
		}
		if (p->p_stat == SSTOP && (p->p_flag&SWTED)==0 &&
		    (p->p_flag&STRC || options&WUNTRACED)) {
			p->p_flag |= SWTED;
			u.u_r.r_val1 = p->p_pid;
			u.u_r.r_val2 = (p->p_cursig<<8) | WSTOPPED;
			retval =  0;
			goto leave_wait1;
		}
	}

	if (f == 0) {
		retval = ECHILD;
		goto leave_wait1;
	}
	if (options&WNOHANG) {
		u.u_r.r_val1 = 0;
		retval = 0;
		goto leave_wait1;
	}
	if ((u.u_procp->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
		u.u_eosys = RESTARTSYS;
		/*
		 *  If we actually restart the system call, u.u_error
		 *  will be cleared.
		 *
		 *  /bin/sh would hang if SIGCLD was set to SIG_DFL
		 *  because without u.u_error set to EINTR, it looked
		 *  like a pid of 0 was returned, and the shell looped
		 *  infinitely.
		 */
		retval = EINTR;	/* fix /bin/sh & /bin/ksh hangs */
		/*
		 * When we come back here from sleep (via longjmp() ),
		 * the pm_sema is actually already unlocked, so we
		 * don't have to go down to leave_wait1 to do that.
		 * Just return.
		 */
		return retval;
	}
	sleep((caddr_t)u.u_procp, PWAIT);
	goto loop;

leave_wait1:
#ifdef MP
	if(pm_locked ) {
		VXSEMA(&pm_sema,&wait1_ss);
	}
#endif /* MP */
	return retval;
}

#ifdef __hp9000s800
procexit(str)
	char *str;
{
	uprintf("Sorry pid %d killed due to: %s\n",
		u.u_procp->p_pid, str);
	printf("Sorry pid %d killed due to: %s\n",
		u.u_procp->p_pid, str);
	exit(SIGKILL);
}
#endif /* hp9000s800 */
/*
 * clear_ctty_from_session() clears the controlling tty from all the processes
 * in the session to which p (the session leader) belongs.  This is to prevent 
 * any future open() of /dev/t * controlling process is exiting, the ctty is no
 * longer associated with this session.
 *
 */

clear_ctty_from_session(p)
     struct proc *p;
     
{
  
  struct proc *q;
  

  for (q = &proc[sidhash[SIDHASH(p->p_sid)]]; q != &proc[0];
       q = &proc[q->p_sidhx]) 
    {
      
      if (q->p_sid == p->p_sid){
	q->p_ttyp = NULL;
	q->p_ttyd = NODEV;
      }
      
    }
  
}
