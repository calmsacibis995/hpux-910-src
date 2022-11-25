/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sem_alpha.c,v $
 * $Revision: 1.6.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:09:59 $
 */

/*
 *   			KERNEL SEMAPHORES - ALPHA CLASS
 *
 *   File:
 *	sem_alpha.c
 *
 *   Services:
 *	initsema- initialize a mutual exclusion semaphore.
 *	psema	- acquire a semaphore with promotion.
 *	cpsema	- conditionally acquire a semaphore with promotion.
 *	vsema	- release a semaphore with priority restoration.
 *	cvsema	- release a semaphore if higher priority proc blocked on it.
 *	disown_sema - release ownership but leave semaphore locked.
 *	valusema- return the value of semaphore.
 *
 *	init_proc_semas - initializes process so that it owns no semaphores.
 *	proc_owns_semas - tests whether process owns semaphores.
 *	pxsema - acquire a semaphore while releasing a held semaphore.
 *	vxsema - release a semaphore while reacquiring formerly held semaphore.
 *
 *	sema_add	- binds a semaphore to a process and adjusts process's
 *			  priority to reflect possession of that semaphore.
 *	sema_delete	- unbinds a semaphore from a process and adjusts
 *			  priority to reflect remaining held semaphores.
 *
 *	FIELD			GUARDED BY
 *	p_sema			internal logic (right!)
 *	sema->sa_owner		semaphore itself
 *	sema->sa_next		semaphore itself
 *	sema->sa_prev		semaphore itself
 *	p_sleep_sema		sema->sa_lock
 *	p_wait_list		sema->sa_lock
 *	p_rwait_list		sema->sa_lock
 *	sema->sa_wait_list	sema->sa_lock
 *	sema->sa_count		sema->sa_lock
 */	

#ifdef	MP
#define VHACK
#include "../h/assert.h"
#include "../h/types.h"
#include "../h/dir.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/sem_alpha.h"
#include "../h/sem_utl.h"
#include "../h/dbg.h"
#include "../machine/psl.h"

/* #include "testsem.h" */

#ifdef	SEMAPHORE_DEBUG
#define	static
sema_t		*global_sema_list = SEM_NULL;
#define	DO_PREVPC
#endif	/* SEMAPHORE_DEBUG */
#ifdef SEMA_COUNTING
#define DO_PREVPC
#endif /* SEMA_COUNTING */


/*
 *  Semaphore data declarations
 */
#ifdef	SEMAPHORE_DEBUG
#define	INIT_ALPHA_SEMA	{ SEM_ALPHA, SEM_NULL, 1 }
#else	/* ! SEMAPHORE_DEBUG */
#define	INIT_ALPHA_SEMA	{ SEM_NULL, 1 }
#endif	/* ! SEMAPHORE_DEBUG */

#ifdef	DO_PREVPC
#define	PREVPC(var, uwptr)	(do_alpha_prevpc ? var = prevpc(uwptr) : 0 )
void	*prevpc();
int	do_alpha_prevpc = 1;
void	*psema_dbg_unw;
void	*vsema_dbg_unw;
void	*cpsema_dbg_unw;
void	*cvsema_dbg_unw;
void	*pxsema_dbg_unw;
void	*vxsema_dbg_unw;
#else	/* ! DO_PREVPC */
#define	PREVPC(var, uwptr)
#endif	/* ! DO_PREVPC */

sema_t filesys_sema = INIT_ALPHA_SEMA;

sema_t vmsys_sema = INIT_ALPHA_SEMA;

sema_t pm_sema = INIT_ALPHA_SEMA;

sema_t io_sema = INIT_ALPHA_SEMA;

sema_t sysVsem_sema = INIT_ALPHA_SEMA;

sema_t sysVmsg_sema = INIT_ALPHA_SEMA;


/* #ifdef SEMA_COUNTING   XXXX */

#define SMP_SEMA_COUNTING	0x1000

unsigned int sema_sleepened;
unsigned int sema_wakened;
unsigned int sema_runned;
	
/* #endif XXXX */ /* SEMA_COUNTING */

#ifdef SEMAPHORE_DEBUG
#define MP_PSEMA psema_dbg
#define MP_VSEMA vsema_dbg
#define CONVERT_USAV(context)
#else	/* ! SEMAPHORE_DEBUG */
#define MP_PSEMA mp_psema
#define MP_VSEMA mp_vsema
#define CONVERT_USAV(context)	((void)convert_usav(context))
#endif	/* ! SEMAPHORE_DEBUG */
/*
 *   psema_dbg()
 *	
 *	Debug version of psema() -- acquire a mutual exclusion semaphore.
 */
MP_PSEMA(semaphore)
	register sema_t *semaphore;
{
	register u_int context;
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */

	PREVPC(pc, &psema_dbg_unw);
	SD_ASSERT(!getnoproc(), "psema: called during noproc");
	SD_ASSERT(!ON_ICS, "psema: called from ICS");
	SD_ASSERT((spinlocks_held() == 0), "psema: called while lock held");
	SD_ASSERT(!owns_sema(semaphore), "psema: proc already owns sema");

	SPINLOCK_USAV(semaphore->sa_lock, context);
	SEMA_LOG(semaphore, SEMA_LOG_PSEMA, pc);
	VERIFY_DEADLOCK_PROTOCOL(u.u_procp, semaphore);
	if (semaphore->sa_count > 0) {
		semaphore->sa_count--;
		SPINUNLOCK_USAV(semaphore->sa_lock, context);
		sema_add(u.u_procp, semaphore);
#ifdef MP
		OWN_INTERRUPTS;
#endif MP
	} else {
		mp_psema_locked(semaphore, context);
	}
#ifdef	DO_PREVPC
	semaphore->sa_pcaller = pc;
#endif	/* DO_PREVPC */

}
mp_psema_locked (semaphore, context)
	register sema_t *semaphore;
	register u_int   context;
{
	CONVERT_USAV(context);

	if ( !cspinlock(sched_lock) ) {
		spinunlock(semaphore->sa_lock);
		spinlock(sched_lock);
		spinlock(semaphore->sa_lock);
	}
	/*  Now we hold both locks */
	if ( --semaphore->sa_count < 0 ) {
		/*
		 *   Unable to get semaphore, so block until it's 
		 *   available or until a signal is legally received.  
		 */
		SEMA_LOG(semaphore, SEMA_LOG_SLEEPY_PSEMA, 0);
		SEMA_NOSWAP(u.u_procp);
#ifdef SEMA_COUNTING
		sema_sleepened++;
		semaphore->sa_count_enqueue++;
		u.u_procp->p_mpflag |= SMP_SEMA_COUNTING;
#endif /* SEMA_COUNTING */
		raise_kernel_pri(u.u_procp, semaphore->sa_priority);
		sema_sleep(u.u_procp, semaphore, P_DEFER_SIGNALS);
		SEMA_SWAP(u.u_procp);
		SD_ASSERT(u.u_procp->p_mpflag & SMP_SEMA_WAKE,
			"psema: awakened by signal");
		SD_ASSERT(semaphore->sa_owner == u.u_procp,
			"psema: not awakend owning semaphore");
	} else {
		/*
		 *  Although unlikely, during the above unlock/lock, 
		 *  the sema_t became free, so just take it.
		 */
		spinunlock(semaphore->sa_lock);
		spinunlock(sched_lock);
		sema_add(u.u_procp, semaphore);
	}
#ifdef MP
	OWN_INTERRUPTS;
#endif MP
}

/*
 *   vsema_dbg()
 *
 *	Promotional V semaphore operation.  Debug version.
 */
MP_VSEMA(semaphore)
	register sema_t *semaphore;
{
	register u_int context;
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */

	PREVPC(pc, &vsema_dbg_unw);
	SD_ASSERT(semaphore->sa_owner != SEM_NULL ? owns_sema(semaphore) : 1,
		 "vsema: sema not owned");
	SD_ASSERT(semaphore->sa_count <= 0, "vsema: count too big");

	SEMA_LOG(semaphore, SEMA_LOG_VSEMA, pc);
#ifdef	DO_PREVPC
#ifndef SEMA_COUNTING
	if (semaphore->sa_vcaller & 0x1)
		semaphore->sa_vcaller &= ~0x1;	/* from [pv]xsema */
	else
		semaphore->sa_vcaller = pc;
#endif /* SEMA_COUNTING */
#endif	/* DO_PREVPC */
	if (!ON_ICS) {
		sema_delete(semaphore->sa_owner, semaphore);
	}
	SPINLOCK_USAV(semaphore->sa_lock, context);
#ifdef MP
	DISOWN_INTERRUPTS;
#endif MP
	if (semaphore->sa_count >= 0) {
		semaphore->sa_count++;
		/* no processes waiting for semaphore */
		SPINUNLOCK_USAV(semaphore->sa_lock, context);
	} else {
		mp_vsema_wanted(semaphore, context);
	}
}

mp_vsema_wanted(semaphore, context)
	register sema_t *semaphore;
	register u_int   context;
{
 	sv_lock_t nsp;

	CONVERT_USAV(context);
	if ( cspinlock(sched_lock) ) {
		nsp.saved = sched_lock; /* we acquired it here */
	} else {
		spinunlock(semaphore->sa_lock);
		spinlockx(sched_lock, &nsp);
		spinlock(semaphore->sa_lock);
	}
	if (semaphore->sa_count++ < 0) {
		struct proc  *p, *sema_dequeue();

		SEMA_LOG(semaphore, SEMA_LOG_VSEMA_WAKE, 0);
		if ((p = sema_dequeue(semaphore)) == SEM_NULL)
			panic("vsema: count inconsistancy");
		SD_ASSERT(!process_owns_semaphore(p, semaphore),
			"vsema: process owns sema that its blocked on");
		spinunlock(semaphore->sa_lock);
		sema_add(p, semaphore);
		sema_wakeup(p, semaphore);
#ifdef SEMA_COUNTING
		if(p->p_mpflag & SMP_SEMA_COUNTING){
			sema_wakened++;
			semaphore->sa_count_dequeue++;
			p->p_mpflag &= ~SMP_SEMA_COUNTING;
		}
#endif /* SEMA_COUNTING */
		spinunlockx(sched_lock, &nsp);
	} else {
		/*
		 *  Although unlikely, during the above unlock/lock, 
		 *  the waiter was awakened, so no process waiting.
		 */
		spinunlock(semaphore->sa_lock);
		spinunlockx(sched_lock, &nsp);
	}
}

/*
 *   disown_sema()
 *
 *	Release ownership of a mutual exclusion semaphore but keep semaphore
 *	locked.  Needed when the P and V operations are not in the same
 *	process, e.g. [process 1]: psema(A)....disown_sema(A);
 *	              [interrupt routine]: vsema(A);
 */
disown_sema(semaphore)
	sema_t *semaphore;
{
	SD_ASSERT( !ON_ICS, "disown_sema: called from ICS");
	SD_ASSERT(! getnoproc(), "disown_sema: called during noproc");
	SD_ASSERT(semaphore->sa_count < 1, "disown_sema: inconsistant count");

	sema_delete(u.u_procp, semaphore);
	SEMA_SWAP(u.u_procp);
	/*
	 *   Need to check priorities and possibly reschedule here.
	 */
}

/*
 *   cpsema_dbg()
 *	
 *	Conditional promotional form of P semaphore routine.
 */
int
cpsema_dbg(semaphore)
	sema_t *semaphore;
{
	int result;
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */

	PREVPC(pc, &cpsema_dbg_unw);
	spinlock(semaphore->sa_lock);
	SEMA_LOG(semaphore, SEMA_LOG_CPSEMA, pc);
	if (semaphore->sa_count > 0) {
		semaphore->sa_count--;
		spinunlock(semaphore->sa_lock);
		if (!ON_ICS) {
			sema_add(u.u_procp, semaphore);
 			SEMA_NOSWAP(u.u_procp);
		} 
		result = SEMA_SUCCESS;
#ifdef	DO_PREVPC
		semaphore->sa_pcaller = pc;
#endif	/* DO_PREVPC */
	}
	else {
		SEMA_LOG(semaphore, SEMA_LOG_FAILED_CP, 0);
		spinunlock(semaphore->sa_lock);
		result = SEMA_FAILURE;
	}
	return(result);
}

/*
 *   cvsema_dbg()
 *
 *	Conditional mutual exclusion V operation.
 *	Performs a V operation only if a higher priority, ready to run
 *	process is blocked on the semaphore.  Returns SEMA_SUCCESS if the
 *	V operation is performed, otherwise returns SEMA_FAILURE.
 */
int
cvsema_dbg(semaphore)
	sema_t *semaphore;
{
	struct proc  *sleeping_process, *sema_dequeue();
	sv_lock_t	nsp;
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */

	PREVPC(pc, &cvsema_dbg_unw);
	SD_ASSERT((spinlocks_held() == 0), "cvsema: called while lock held");
	SD_ASSERT(owns_sema(semaphore),
		"cvsema: proc does not own sema");
	SD_ASSERT(semaphore->sa_count <= 0, "cvsema: count too big");

	spinlockx(sched_lock, &nsp);
	spinlock(semaphore->sa_lock);
	SEMA_LOG(semaphore, SEMA_LOG_CVSEMA, pc);

	if ((sleeping_process = semaphore->sa_wait_list) == SEM_NULL 
	    || (sleeping_process->p_flag & SRTPROC) == 0
	    || sleeping_process->p_pri >= CURPRI
	    || (sleeping_process->p_flag & SLOAD) == 0        ) 
	{	
		spinunlock(semaphore->sa_lock);
		spinunlockx(sched_lock, &nsp);
		return(SEMA_FAILURE);
	}
#ifdef	DO_PREVPC
#ifndef SEMA_COUNTING
	semaphore->sa_vcaller = pc;
#endif /* SEMA_COUNTING */
#endif	/* DO_PREVPC */
	if (! ON_ICS)
		sema_delete(u.u_procp, semaphore);
	++semaphore->sa_count;
#ifdef MP
	DISOWN_INTERRUPTS;
#endif MP
	if ((sleeping_process = sema_dequeue(semaphore)) == SEM_NULL)
		panic("cvsema: inconsistancy");
	spinunlock(semaphore->sa_lock);
	sema_add(sleeping_process, semaphore);
	sema_wakeup(sleeping_process, semaphore);
	spinunlockx(sched_lock, &nsp);
	if (! ON_ICS)
		SEMA_SWAP(u.u_procp);
	return(SEMA_SUCCESS);

}

/*
 *	pxsema(), vxsema()
 *	
 *	These are used for crossing into and out of another empire;
 *	The callee bears the burden.
 *
 *	e.g.
 *	emp1()
 *		psema(&emp1_sema);
 *		emp2();
 *	}
 *	emp2(){
 *		sv_sema_t saved;  /* local save structure */
/*
 *		pxsema(&emp2_sema,&saved);
 *		work();
 *		vxsema(&emp2_sema,&saved);
 *	}	
 *
 *	if you hold exactly one semaphore, releases it, puts it on
 *	a list, then reacquires it when vxsema is called.
 */

#ifdef PXSEMA_DBG

int pxsema_count;	/* total number of pxsemas */
int pxsema_vsema;	/* number of times we vsema inside pxsema */
int pxsema_psema;	/* number of times we psema inside pxsema */
#define dbg_pxsema_count	(pxsema_count++)
#define dbg_pxsema_vsema	(pxsema_vsema++)
#define dbg_pxsema_psema	(pxsema_psema++)

#else

#define dbg_pxsema_count
#define dbg_pxsema_vsema
#define dbg_pxsema_psema

#endif PXSEMA_DBG

pxsema(semaphore,save)
	sema_t *semaphore;
	sv_sema_t *save;
	
{
	sema_t	*xchng_sema;	/* exchange semaphore */
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */

	PREVPC(pc, &pxsema_dbg_unw);
	SEMA_LOG(semaphore, SEMA_LOG_PXSEMA, pc);
	dbg_pxsema_count;

	SD_ASSERT( ! ON_ICS, "pxsema: called from ICS");
	if(semaphore == u.u_procp->p_sema){
		save->nsaved = 1;
		save->saved[0].sema = semaphore;
		return;
	}

	/*
	 * what semaphore are we going to take off the list ?
	 */
	xchng_sema = u.u_procp->p_sema;

	if(xchng_sema){
		save->nsaved = 1;
		save->saved[0].sema = xchng_sema;
#ifdef	DO_PREVPC
#ifndef SEMA_COUNTING
		xchng_sema->sa_vcaller = pc | 0x1;
#endif /* SEMA_COUNTING */
#endif	/* DO_PREVPC */

		vsema(xchng_sema);
		dbg_pxsema_vsema;
		xchng_sema = u.u_procp->p_sema;
		if(xchng_sema){
			panic("pxsema: Too many semaphores!!");
		}
	} else
		save->nsaved = 0;


	dbg_pxsema_psema;

#ifdef	VHACK
	if(semaphore == &vmsys_sema){
		return;
	}
#endif /* VHACK */
	psema(semaphore);
#ifdef	DO_PREVPC
	semaphore->sa_pcaller = pc;
#endif	/* DO_PREVPC */


	return;
	
}


vxsema(semaphore,save)
	sema_t *semaphore;
	sv_sema_t *save;
	
{
	sema_t	*xchng_sema;	/* exchange semaphore */
#ifdef	DO_PREVPC
	u_int	pc;
#endif	/* DO_PREVPC */
	
	PREVPC(pc, &vxsema_dbg_unw);
	SEMA_LOG(semaphore, SEMA_LOG_VXSEMA, pc);

	if(save->nsaved == 0){
#ifdef	VHACK
		if(semaphore == &vmsys_sema){
			return;
		}
#endif /* VHACK */
#ifdef	DO_PREVPC
#ifndef SEMA_COUNTING
		semaphore->sa_vcaller = pc | 0x1;
#endif /* SEMA_COUNTING */
#endif	/* DO_PREVPC */

		vsema(semaphore);
		return;
	}


	if(save->nsaved != 1){
		panic("vxsema: too many saved semaphores to undo!");
	}

	if(semaphore == save->saved[0].sema)
		return;
	
#ifdef	DO_PREVPC
#ifndef SEMA_COUNTING
	semaphore->sa_vcaller = pc | 0x1;
#endif /* SEMA_COUNTING */
#endif	/* DO_PREVPC */

#ifdef	VHACK
   if(semaphore != &vmsys_sema)
#endif /* VHACK */
	vsema(semaphore);
	xchng_sema = u.u_procp->p_sema;

	if(xchng_sema){
		panic("vxsema: Too many process semaphores!!");
	}
	psema(save->saved[0].sema);
#ifdef	DO_PREVPC
	save->saved[0].sema->sa_pcaller = pc;
#endif	/* DO_PREVPC */

	return;
}

/*
 *   init_proc_semas()
 *
 *	Initializes a process's semaphore ownership so that it owns no
 *	semaphores.
 */
init_proc_semas(process)
	struct proc *process;
{
	process->p_sema = (sema_t *) SEM_NULL;
	process->p_sleep_sema = (sema_t *) SEM_NULL;
#ifdef	SEMAPHORE_DEBUG
	process->p_sema_reaquire = (sema_t **) SEM_NULL;
#endif	SEMAPHORE_DEBUG
}


/*
 *   initsema()
 *
 *	Initializes a mutual exclusion semaphore.  Must be called before a
 *	semaphore is used.  *** Warning *** Must not be called when the
 *	semaphore is actively being used by the kernel.
 */
initsema(semaphore, value, priority, order)
	sema_t *semaphore;
	int	value;		/*!!!!!!!!!! this is going away !!!!!!!*/
	int	priority;
	int	order;
{
	if (value != 1)
		panic("initsema: sorry, semaphores are binary now");
	semaphore->sa_count = 1;
	semaphore->sa_lock = 
		alloc_spinlock(SEMAPHORE_LOCK_ORDER, "sema_t spinlock");
	semaphore->sa_wait_list = SEM_NULL;	/* no processes waiting */
	semaphore->sa_owner = SEM_NULL;
	semaphore->sa_priority = priority;
	semaphore->sa_prev = SEM_NULL;
	semaphore->sa_next = SEM_NULL;

#ifdef	SEMAPHORE_DEBUG
	/*
	 *  Link this semaphore on a global list of semaphores
	 */
	spinlock(&lock_init_lock);
	semaphore->sa_link = global_sema_list;
	global_sema_list = semaphore;
	spinunlock(&lock_init_lock);

	semaphore->sa_order =
	 order & ~(SEMA_CONTENTION | SEMA_DEADLOCK_SAFE);
	semaphore->sa_last_pid = 0;
	semaphore->sa_max_waiters = 0;
	semaphore->sa_p_count = 0;
	semaphore->sa_sleepy_p_count = 0;
	semaphore->sa_type = SEM_ALPHA;
	semaphore->sa_contention = (order&SEMA_CONTENTION) ? SEM_TRUE:SEM_FALSE;
	semaphore->sa_deadlock_safe =
	 (order & SEMA_DEADLOCK_SAFE) ? SEM_TRUE : SEM_FALSE;
#endif
}


/*
 *   owns_sema()
 *
 *	Returns TRUE if the current process owns the specified semaphore,
 *	otherwise returns FALSE.
 */
int
owns_sema(semaphore)
	sema_t *semaphore;
{
	SD_ASSERT( ! ON_ICS, "owns_sema: called from ICS");

	return(semaphore->sa_owner == u.u_procp);
}

int
doesnt_own_sema(semaphore)
	sema_t *semaphore;
{
	SD_ASSERT( ! ON_ICS, "owns_sema: called from ICS");
	return(semaphore->sa_owner == u.u_procp);
}


/*
 *   proc_owns_semas()
 *
 *	Returns TRUE if a process owns one of more semaphores, otherwise
 *	returns FALSE.
 */
int
proc_owns_semas(process)
	struct proc *process;
{
#ifdef SEMAPHORE_DEBUG
	SD_ASSERT(process != SEM_NULL, "proc_owns_semas: null process");
	return((process->p_sema != SEM_NULL) ? SEM_TRUE : SEM_FALSE);
#else
	panic("proc_owns_semas: SEMAPHORE_DEBUG only");
#endif
}

/*
 * process_owns_semaphore()
 */
int
process_owns_semaphore(proc, sema)
	struct proc *proc;
	sema_t *sema;
{
	return (sema->sa_owner == proc);
}

/*
 *   valusema()
 *
 *	Returns the current count for a semaphore.
 */
int 
valusema(semaphore)
	sema_t *semaphore;
{
	return(semaphore->sa_count);
}

/*
 *   sema_add
 *
 *	Add a reference to a newly acquired semaphore into a process's
 *	proc structure for future reference.  Update the process's priority.
 *
 *	spinlockx called because we could be called from cpsema
 */
static
sema_add(process, semaphore)
	struct proc *process;
	sema_t      *semaphore;
{
	sv_lock_t	nsp;

	(semaphore)->sa_owner = (process);
#ifdef SEMA_PREEMPT
/* KPREEMPT:  Before enabling this ifdef please read this comment.
 *
 * If kernel preemption is in effect, we probably want to raise the
 * priority of the process to that of the semaphore.  NOTE: I would
 * prefer not to have to get the sched_lock in order to do it.  I think
 * that this locking is superfulous (i.e., it dosn't affect "correctness");
 * however, this needs more thought.
 *
 * Also, this routine (without this section) is in-line in the assembly
 * routines like psema().  So if this is enabled, they should probably
 * be changed too!
 */
	spinlockx(sched_lock, &nsp);
	if ((semaphore)->sa_priority < (process)->p_pri)
		(process)->p_pri = (semaphore)->sa_priority;
	spinunlockx(sched_lock, &nsp);
#endif	/* SEMA_PREEMPT */
	SD_ASSERT(semaphore->sa_prev == SEM_NULL, 
		"sema_add: semaphore->sa_prev != SEM_NULL");
	SD_ASSERT(semaphore->sa_next == SEM_NULL, 
		"sema_add: semaphore->sa_next != SEM_NULL");
	if ((semaphore->sa_next = process->p_sema) != SEM_NULL) {
		SD_ASSERT(process->p_sema->sa_prev == SEM_NULL,
			"sema_add: process->p_sema->sa_prev != SEM_NULL");
		process->p_sema->sa_prev = semaphore;
	}
	process->p_sema = semaphore;
}


/*
 *   sema_delete
 *
 *	Remove a reference to a semaphore from a process's proc structure
 *	and recompute the process's priority.  The new priority will be
 *	the highest priority in set formed by the process's base priority
 *	(priority before it acquired any semaphores) and the priorities of
 *	the remaining semaphores still possessed by the process.
 */
static
sema_delete(proc, sema)
	struct proc *proc;
	sema_t      *sema;
{

	SD_ASSERT( !ON_ICS, "sema_delete: called from ICS");
	SD_ASSERT( proc == u.u_procp, "sema_delete: we aren't us!");
	SD_ASSERT( process_owns_semaphore(proc, sema),
		   "sema_delete: process does not own sema");
	if (proc->p_sema == sema) {
		SD_ASSERT(sema->sa_prev == SEM_NULL,
			"sema_delete: sema->sa_prev != SEM_NULL");
		if ((proc->p_sema = sema->sa_next) != SEM_NULL)
			proc->p_sema->sa_prev = SEM_NULL;
	} else {
		SD_ASSERT(sema->sa_prev != SEM_NULL,
			"sema_delete: sema->sa_prev == SEM_NULL");
		if ((sema->sa_prev->sa_next = sema->sa_next) != SEM_NULL)
			sema->sa_next->sa_prev = sema->sa_prev;
		sema->sa_prev = SEM_NULL;
	}
	sema->sa_next = SEM_NULL;


#ifdef SEMAPHORE_DEBUG
	sema->sa_last_pid = (int) proc;
#ifdef SEMA_PREEMPT
/* KPREEMPT:  Before enabling this ifdef please read this comment.
 *
 * This routine (without this section) is in-line in the assembly
 * routines like vsema().  So if this is enabled, they should probably
 * be changed too!
 */
	/*
	 *  Recompute process priority
	 */
	if (sema->sa_owner != SEM_NULL) {
		int 		new_priority;
		sv_sema_t	nsp;
		sema_t		*own;

		sema->sa_owner = SEM_NULL;

		spinlockx(sched_lock, &nsp);
		new_priority = (proc->p_flag & SRTPROC) ?
			proc->p_rtpri : proc->p_usrpri;
		for (own = proc->p_sema; own != SEM_NULL; own = own->sa_next) {
			if (own->sa_priority < new_priority)
				new_priority = own->sa_priority;
		}
		proc->p_pri = new_priority;
		spinunlockx(sched_lock, &nsp);
	}
#endif 	/* SEMA_PREEMPT */
#endif /* SEMAPHORE_DEBUG */
	sema->sa_owner = SEM_NULL;
} 
#endif	/* MP */
