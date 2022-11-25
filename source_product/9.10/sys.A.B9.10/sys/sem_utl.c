/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sem_utl.c,v $
 * $Revision: 1.9.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:10:23 $
 */

/*
 *   			KERNEL SEMAPHORE UTILITY ROUTINES
 *
 *   File:
 *	sem_utl.c
 *
 *   Services:
 *	wsignal	- awaken a semaphore-blocked process with a signal.
 *
 *	sema_sleep		must own sa_lock & sched_lock 
 *				  both freed by time of exit
 *	sema_wakeup		must own sched_lock 
 *				  held on exit
 *	sema_enqueue		must own semaphore->sa_lock
 *	sema_dequeue		must own semaphore->sa_lock
 *	sema_unlink		must own semaphore->sa_lock
 *
 */	

#ifdef	MP
#include "../h/types.h"
#include "../h/dir.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/sem_utl.h"
#include "../h/systm.h"
#include "../h/dbg.h"

#include "../machine/psl.h"

/* #include "testsem.h" */


#ifdef	SEMAPHORE_DEBUG

int	sema_log_enable = 1;	/* 1 => log operations into log table.	*/

/*
 *   Statistics: counts of each semaphore operation.
 */
int psema_count = 0;
int cpsema_count = 0;
int psync_count = 0;
int pchansync_count = 0;
int vsema_count = 0;
int cvsema_count = 0;
int vsync_count = 0;
int cvsync_count = 0;
int wsync_count = 0;
int vchansync_count = 0;
int wsignal_count = 0;

int stolen_p_count = 0;
int failed_cp_count = 0;
int sleepy_psema_count = 0;
int sleepy_psync_count = 0;

#endif	SEMAPHORE_DEBUG

#ifdef	DO_PREVPC
int	do_prevpc = 1;
void	*psema_dbg_unw = 0;
void	*vsema_dbg_unw = 0;
void	*cpsema_dbg_unw = 0;
void	*cvsema_dbg_unw = 0;
void	*pxsema_dbg_unw = 0;
void	*vxsema_dbg_unw = 0;
#endif	/* ! DO_PREVPC */

/*
 *   wsignal()
 *
 *	Wakeup a process sleeping on a semaphore with a signal.
 *	This will fail (and return FALSE) if the specified process
 *	isn't sleeping on that semaphore, or if that process has
 *	not allowed itself to be disturbed by signals.  Note that
 *	the awakened process does NOT acquire the semaphore.
 *
 *	NOTE: sched_lock and process_lock must be held prior to entry.
 *		wsignal is not permitted to release them.
 */
int
wsignal(process)
	struct proc *process;
{
	sync_t *semaphore;
	sv_lock_t	nsp;

	if ( ((semaphore = (sync_t *)process->p_sleep_sema) == SEM_NULL) ||
	     !(process->p_flag & SSIGABL) ||
	     (process->p_stat != SSLEEP && process->p_stat != SSTOP) ){
		return(SEMA_FAILURE);
	}

	SPINLOCKX(sched_lock, &nsp);
	spinlock(semaphore->s_lock);
	if (process->p_sleep_sema == SEM_NULL  ||
	     !(process->p_flag & SSIGABL) ||
	     (process->p_stat != SSLEEP && process->p_stat != SSTOP) ){
		spinunlock(semaphore->s_lock);
		SPINUNLOCKX(sched_lock, &nsp);
		return(SEMA_FAILURE);
	}

	SEMA_LOG(semaphore, SEMA_LOG_WSIGNAL, 0);
	if (semaphore->count++ < 0) {
		sema_unlink(semaphore, process);
		spinunlock(semaphore->s_lock);
		process->p_mpflag &= ~SMP_SEMA_WAKE;

		sema_wakeup(process, semaphore);
		SPINUNLOCKX(sched_lock, &nsp);
		return(SEMA_SUCCESS);
	} else {
		/* no processes waiting for semaphore */
		spinunlock(semaphore->s_lock);
		SPINUNLOCK(sched_lock);
		panic("wsignal: semaphore count inconsistancy");
	}
	return(SEMA_FAILURE);
}

/*
 *  release_semas()
 *
 *  Called from sleep() etc.
 *  Frees all alpha class semaphores, and saves them in the save structure
 *  that is passed as a parameter.
 *
 */
release_semas(ss)
	sv_sema_t *ss;
{
#ifdef	SEMAPHORE_DEBUG
	int order;
#endif
	int	count;
	struct	proc	*p;
	sema_t  *semap;
	struct	sv_sema_saved *savep;

	/*
	 * Release semaphores
	 */
	p = u.u_procp;
	savep = ss->saved;
#ifdef	SEMAPHORE_DEBUG
	p->p_sema_reaquire = ss;
#endif
	for (count = 0; (semap = p->p_sema) != SEM_NULL; count++, savep++) {
		SD_ASSERT(count < MAX_SEMAS,
			"release_semas: process owns too many semaphores");
#ifdef	SEMAPHORE_DEBUG
		if (count > 0) {
			SD_ASSERT(semap->sa_order < order,
				"release_semas: semaphore order violation");
		}
		order = semap->sa_order;
#endif
		savep->sema = semap;
		vsema(semap);
	}
	ss->nsaved = count;
}

/*
 *  reaquire_semas()
 *
 *  Called from sleep() etc.
 *  Reaquires all the alpha class semaphores that were saved in the sv_sema_t
 *  structure when release_semas() was called.
 *
 */
reaquire_semas(ss)
	sv_sema_t *ss; 
{ 
	sema_t	*semap;
	int	count;

	/*
	 * Re-aquire semaphores
	 */
	count = ss->nsaved;
	SD_ASSERT(count >= 0 && count <= MAX_SEMAS,
		"reaquire_semas: number of semaphores saved is bogus");
	while (count-- > 0) {
		semap = ss->saved[count].sema;
		psema(semap);
	}
}

init_semaphores()
{
	initsema(&filesys_sema, 1, FILESYS_SEMA_PRI, FILESYS_SEMA_ORDER);
	initsema(&pm_sema, 1, PROC_PRI, PROC_ORDER);
	initsema(&io_sema, 1, IO_SEMA_PRI, IO_SEMA_ORDER);
#ifdef	MP
	initsync(&runin);
	initsync(&runout);
#endif	/* MP */
	sleep_semas_init();
}

#ifdef DO_SEMA_NOSWAP

void
sema_noswap(p)
	struct proc *p;
{
 	sv_lock_t nsp;
	
	SD_ASSERT( p == u.u_procp, "sema_noswap: Hey! we aren't us!");
	if (! (p->p_mpflag & SMP_SEMA_NOSWAP) ) {
		spinlockx(sched_lock, &nsp);
		p->p_mpflag |= SMP_SEMA_NOSWAP;
		spinunlockx(sched_lock, &nsp);
	}
}

void
sema_swap(p)
	struct proc *p;
{
 	sv_lock_t nsp;
	
	SD_ASSERT( p == u.u_procp, "sema_swap: Hey! we aren't us!");
	if (p->p_sema == SEM_NULL) {
		spinlockx(sched_lock, &nsp);
		p->p_mpflag &= ~SMP_SEMA_NOSWAP;
		spinunlockx(sched_lock, &nsp);
	}
}
#endif	/* DO_SEMA_NOSWAP */

/*
 *			UTILITY SEMAPHORE FUNCTIONS
 *
 *	
 *   The following functions are local to this module:
 *
 *	sema_enqueue	- enqueue a process on a semaphore's waiting list in
 *			  priority order.
 *	sema_unlink	- unlink a specific process from a semaphore's waiting
 *			  list.
 *	sema_sleep	- puts a process to sleep on a semaphore.
 *
 *	sema_signal_pending - returns TRUE if process has a signal pending.
 *
 *	sema_wakeup	- wakes up a process that was sleeping on a semaphore.
 *	                  schedules a ready-to-run process (newly awakened
 *			  from a semaphore).
 *	sema_log	- logs a semaphore operation.
 *
 *
 *   Algorithm:
 *	
 *	A process blocked on a semaphore has its proc structure stored
 *	on the semaphore's doubly linked waiting list.  Processes are
 *	inserted into the list in priority order, with the highest priority
 *	process at the list head.  The "wait_list" field in the semaphore
 *	data structure points to the head of the list. The processes are
 *	linked using the "p_wait_list" and "p_rwait_list" fields of the 
 *	proc structure
 *	(normally used for linking a process into a run queue). "p_wait_list"
 *	points to the next lowest priority process in the chain, while
 *	"p_rwait_list" points to the next highest priority process.  The
 *	"p_wait_list" field of the tail process and the "p_rwait_list" 
 *	field of the head process are both set to NULL.
 *
 *	When a process acquires a semaphore, the address of that semaphore
 *	is stored in the proc structure and the priority of the process is
 *	recomputed to be the smallest value (highest precedence) of the
 *	set formed by the base priority of the process and the priorities
 * 	of the possessed.  When a process releases a semaphore, its entry
 *	in the proc structure is deleted and the priority is recomputed to
 *	reflect the remaining held semaphores.
 *
 *	WARNING:  These routines have one thing in common -- they presume
 *	that the proc structure and semaphore structures are already locked
 *	(proc.p_lock and semaphore.sa_lock respectively) before the routine
 *	is called !!!!!
 */


/*
 *   sema_enqueue()
 *
 *	Enqueue a process onto a semaphore's waiting list in priority
 *	order.
 */
sema_enqueue(semaphore, process)
	sema_t *semaphore;
	struct proc *process;
{
	struct proc *marker, *last_marker;


	T_SPINLOCK(semaphore->sa_lock);
	if ((marker = semaphore->sa_wait_list) == SEM_NULL) {
		/* first entry */
		semaphore->sa_wait_list = process;
		process->p_wait_list = SEM_NULL;
		process->p_rwait_list = SEM_NULL;
	}
	/*
	 *  The priority of the running process may change, but
	 *  it will not affect correctness, just where the process
	 *  gets inserted on the wait list.
	 */
	else if (process->p_pri < marker->p_pri || 
		 process->p_pri == marker->p_pri && 
		 process->p_usrpri < marker->p_usrpri) { 
		/* insert at head */
		marker->p_rwait_list = process;
		process->p_wait_list = marker;
		process->p_rwait_list = SEM_NULL;
		semaphore->sa_wait_list = process;
	}
	else for (;;) {
		last_marker = marker;
		if ((marker = marker->p_wait_list) == SEM_NULL) {
			/*append to tail*/
			last_marker->p_wait_list = process;
			process->p_rwait_list = last_marker;
			process->p_wait_list = SEM_NULL;
			break;
		}
		else if (process->p_pri < marker->p_pri || 
			 process->p_pri == marker->p_pri && 
			 process->p_usrpri < marker->p_usrpri) {
			/* insert in middle */
			last_marker->p_wait_list = process;	
			process->p_rwait_list = last_marker;
			marker->p_rwait_list = process;
			process->p_wait_list = marker;
			break;
		}
	}
	process->p_sleep_sema = semaphore;
}
	
/*
 *   sema_dequeue()
 *
 *	Dequeue the highest priority process from a semaphore's waiting list.
 */
struct proc *
sema_dequeue(semaphore)
	sema_t *semaphore;
{
	struct proc *process;

	T_SPINLOCK(semaphore->sa_lock);

	if ((process = semaphore->sa_wait_list) != SEM_NULL) {
		if ((semaphore->sa_wait_list =process->p_wait_list) != SEM_NULL)
			semaphore->sa_wait_list->p_rwait_list = SEM_NULL;
		process->p_wait_list = SEM_NULL;
	}
	process->p_sleep_sema = (sema_t *)SEM_NULL;
	return(process);
}


/*
 *   sema_unlink()
 *
 *	Unlink a process from a semaphore's waiting list.
 *
 *	Assumes semaphore lock already held. Does not release it.
 */
sema_unlink(semaphore, process)
	sema_t *semaphore;
	struct proc *process;
{
 	/* Ensure that we have these locks */
	T_SPINLOCK(semaphore->sa_lock);
 
	if (semaphore->sa_wait_list == process) {
		if ((semaphore->sa_wait_list =process->p_wait_list) != SEM_NULL)
			semaphore->sa_wait_list->p_rwait_list = SEM_NULL;
	}
	else
		process->p_rwait_list->p_wait_list = process->p_wait_list;
	if (process->p_wait_list != SEM_NULL)
		process->p_wait_list->p_rwait_list = process->p_rwait_list;
	process->p_wait_list = SEM_NULL;
	process->p_rwait_list = SEM_NULL;

	process->p_sleep_sema = (sema_t *)SEM_NULL;
}


/*
 *   sema_sleep()
 *
 *	Puts a process to sleep on a semaphore.  Returns TRUE if the
 *	process was awakened by a V operation, FALSE if awakened by a signal.
 *	This routine is local to the semaphore module -- NOT for global
 *	consumption.
 *
 *      NOTE: semaphore->sa_lock must be held on entry, and it is freed
 *	by the time sema_sleep returns
 */


int
#ifdef FSD_KI
sema_sleep(p, semaphore, signal_option, caller)
	struct proc *p;
	sema_t  *semaphore;
	sigoption_t signal_option;
        caddr_t caller;
#else
sema_sleep(p, semaphore, signal_option)
	struct proc *p;
	sema_t  *semaphore;
	sigoption_t signal_option;
#endif /* FSD_KI */
{
	u_int ret;

	/* This is where we really go to sleep */


	T_SPINLOCK(sched_lock);
	T_SPINLOCK(semaphore->sa_lock);

	sema_enqueue(semaphore, p);
	spinunlock(semaphore->sa_lock);

#ifdef SYNC_SEMA_RECOVERY
	if (p->p_recover_sema) {
		/*
		 * Note that we must recover this semaphore upon return
		 */
		vsema(p->p_recover_sema);
	} 
#endif /* SYNC_SEMA_RECOVERY */

	if (signal_option == P_DEFER_SIGNALS)
		p->p_flag &= ~SSIGABL;  /* do not wake for signals */
	else
		p->p_flag |= SSIGABL;  /* wake for signals */
	p->p_mpflag |= SMP_SEMA_WAKE | SMP_SEMA_BLOCK;
	if (p->p_stat != SSTOP)
		p->p_stat = SSLEEP;
	p->p_slptime = 0;
#ifdef DRV1_DBG
	if (p->p_pid == 1) {
		printf(">>>>>> Init Blocks on sema=0x%X <<<<<<< \n", semaphore);
	}
#endif
	u.u_ru.ru_nvcsw++;
#ifdef MP
	if (int_owner==getiva()) {
		int_owner = 0;
	}
#endif MP
#ifdef FSD_KI
        _swtch(caller);	/* releases sched_lock when held */
#else
        swtch();	/* releases sched_lock when held */
#endif /* FSD_KI */
#ifdef MP
	OWN_INTERRUPTS
#endif MP
	if (signal_option == P_DEFER_SIGNALS) {
		spinlock(sched_lock);
		p->p_flag |= SSIGABL;   /* let process be signalable again */
		spinunlock(sched_lock);
	}

	/*
	 *   Check to see if awakened by signal or V operation.
	 */
	ret = p->p_mpflag & SMP_SEMA_WAKE;
	return(ret);
}


int sema_signal_own = 0;
/*
 *   sema_signal_pending()
 *
 *	Returns TRUE if the current process has a pending signal, otherwise
 *	FALSE.
 *
 *   It should be ok to LOOK at the sig values for our processor
 *   if all we want is a hint.
 */
sema_signal_pending()
{
		
	return(ISSIG(u.u_procp));
}


/*
 *   sema_wakeup()
 *
 *	Wakes up a process that has been sleeping on a semaphore.  Preempts
 *	current process if awakened process is of higher priority.
 *
 *	Sched_lock must be held on entry and must
 *	still be held upon return.
 */
sema_wakeup(p, semaphore)
	struct proc *p;
	sema_t *semaphore;
{
	/* Make sure we own these locks */
	T_SPINLOCK(sched_lock);

	p->p_mpflag &= ~SMP_SEMA_BLOCK;
#ifdef DRV1_DBG
	if (p->p_pid == 1) {
		printf(" >>>>>> Init awakens from sema=0x%X <<<<<< \n", p->p_sleep_sema);
	}
#endif
	p->p_wchan = SEM_NULL;
        /* do not set the process to run if it's stopped. */
        if (p->p_stat == SSTOP)
                return;

	p->p_stat = SRUN;
	p->p_mpflag &= ~SMP_STOP; /* clear if on, we are nolonger stopped */
	if (p->p_flag & SLOAD){
#ifdef SYNC_SEMA_RECOVERY

		if ( (p->p_recover_sema == 0) ||
		     (((sema_t *)(p->p_recover_sema))->sa_count > 0))
			setrq(p);
		else {
			sema_t	*recov = p->p_recover_sema;

			/*
			 * Sort of an in-line psema() on behalf of another
			 * process when the sema is already held; else the
			 *  psema() occurs when that process starts running.
			 */
			spinlock(recov->sa_lock);
			/* recheck that sema was not freed */
			if(recov->sa_count > 0 ){
				/* Sema is not held, so run q  (grab later) */
				spinunlock(recov->sa_lock);
				setrq(p);
			}else{
				p->p_recover_sema = 0;
				recov->sa_count--;
				/* Sema is held, so get in line */
				sema_enqueue(recov,p);
				spinunlock(recov->sa_lock);
				p->p_mpflag |= SMP_SEMA_WAKE | SMP_SEMA_BLOCK;
			}
		}
#else	/* SYNC_SEMA_RECOVERY */
		setrq(p);
#endif /* SYNC_SEMA_RECOVERY */
	}
 	/*
  	 * If "process" is of higher priority than the current process,
 	 * then arranges for "process" to run as soon as
 	 * possible: very soon if "process" is realtime, no worse than end of
 	 * system call if "process" is timeshare (assuming no other higher
 	 * priority processes ready-to-run).  If "process" is swapped out,
 	 * arrangements are made to swap it in.   Note that preemption of
 	 * timeshare processes takes place only
	 * if this is a realtime process with stronger 
	 * priority than the current process then 
	 * preempt the current process even if it's 
	 * executing in the kernel.
	 */
	if ((p->p_flag&SLOAD) == 0) {
		/* 
		 * Raise priority of swapper to 
		 * priority of rt process
		 * we have to swap in.
		 */
		if((p->p_flag & SRTPROC) && 
			(proc[S_SWAPPER].p_pri > p->p_rtpri)){
			(void)changepri(&proc[S_SWAPPER], p->p_rtpri);
		}
#ifdef	MP
		cvsync(&runout);
#else	/* ! MP */
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;

		/* Make sure we still own these locks */
		T_SPINLOCK(sched_lock);

		return;
	}
#ifndef notdef
	/* Much simpler than alternate below .... */
	else {
		if (p->p_pri < CURPRI) {
			runrun++;
			aston();
		}
	}

	/* Make sure we still own these locks */
	T_SPINLOCK(sched_lock);

	return;

}

#else	/* notdef */

	/* XXX This needs to be changed to take sched_lock into account - jkr */
	else if ( spinlocks_held() ) {
		/* 
		 * spinlocks are held here... no preemption 
		 */
		runrun++;
		aston();
	} else {
		/*
		 *  only allow preemption if there are no spinlocks held 
		 */
		if ((p->p_flag&SRTPROC) && (!getnoproc())
			&& (p->p_pri < u.u_procp->p_pri))
			preemptkernel();
		else if ((p->p_pri < getcurpri())&&(p->p_pri < CURPRI))

#ifdef	SEMAPHORE_DEBUG
		{
			if (timeshare_preempt)
				preemptkernel();
			else {
				runrun++;
				aston();
			}
		}
#else	/* ! SEMAPHORE_DEBUG */
		{
			runrun++;
			aston();
		}
#endif	/* ! SEMAPHORE_DEBUG */
	}
}
#endif	/* notdef */

#ifdef	SEMAPHORE_DEBUG
/*
 *   sema_log
 *
 *	log a semaphore operation for debugging.
 */
int nsemalogs = LOG_ENTRIES;	/* total number of semaphore log entries*/
int semalog_entry_size = sizeof(struct mp_sem_log);

#ifdef	DVL_PROCTRACE
extern	int	proctrace_sequence;
#endif	/* DVL_PROCTRACE */

sema_log(sema, function, pc)
	sema_t *sema;
	int	function;
	u_int	pc;
{
	struct mp_sem_log *entry;
	extern lock_t *semaphore_log_lock; /* temp. - DBM */
	int	index;

	switch (function) {
	    case SEMA_LOG_PSEMA:
		psema_count++; sema->sa_p_count++; break;
	    case SEMA_LOG_CPSEMA:
		cpsema_count++; sema->sa_p_count++; break;
	    case SEMA_LOG_PSYNC:
		psync_count++; sema->sa_p_count++; break;
	    case SEMA_LOG_PCHANSYNC:
		pchansync_count++;sema->sa_p_count++;break;
	    case SEMA_LOG_VSEMA:	vsema_count++; break;
	    case SEMA_LOG_CVSEMA:	cvsema_count++; break;
	    case SEMA_LOG_VSYNC:	vsync_count++; break;
	    case SEMA_LOG_CVSYNC:	cvsync_count++; break;
	    case SEMA_LOG_WSYNC:	wsync_count++; break;
	    case SEMA_LOG_VCHANSYNC:	vchansync_count++; break;
	    case SEMA_LOG_WSIGNAL:	wsignal_count++; break;
	    case SEMA_LOG_STOLEN_P:	stolen_p_count++; break;
	    case SEMA_LOG_FAILED_CP:	failed_cp_count++; break;
	    case SEMA_LOG_SLEEPY_PSEMA:
		sleepy_psema_count++;
		sema->sa_sleepy_p_count++;
		if(sema->sa_max_waiters < -sema->sa_count)
		   sema->sa_max_waiters = -sema->sa_count;
		break;
	    case SEMA_LOG_SLEEPY_PSYNC:
		sleepy_psync_count++;
		sema->sa_sleepy_p_count++;
		if(sema->sa_max_waiters < -sema->sa_count)
		   sema->sa_max_waiters = -sema->sa_count;
		break;
	}
	if (!sema_log_enable) 
		return;
	switch (function) {	/* log mutual exclusion operations */
	    case SEMA_LOG_PXSEMA:
	    case SEMA_LOG_VXSEMA:
	    case SEMA_LOG_PSEMA:
	    case SEMA_LOG_CPSEMA:
	    case SEMA_LOG_VSEMA:
	    case SEMA_LOG_CVSEMA:
	    case SEMA_LOG_SLEEPY_PSEMA:
	    case SEMA_LOG_VSEMA_WAKE:
		log_total++;
		log_index = log_total % LOG_ENTRIES;
		entry = &semaphore_log [log_index];
		last_log_entry = entry;
		entry->last_pid		= sema->sa_last_pid;
		entry->sema_value	= sema->sa_count;
		entry->semaphore	= sema;
		entry->max_waiters	= sema->sa_max_waiters;
		entry->timestamp	= lbolt;
		entry->caller_address	= pc;
		entry->caller_pid	= (int) u.u_procp;
		entry->sema_function	= function;
		index 			= getprocindex();
		entry->procid		= index;
#ifdef	DVL_PROCTRACE
		entry->proctrace_index	= proctrace_sequence;
#endif	/* DVL_PROCTRACE */
		break;
	    default:
		break;
	}
}



/*
 *   verify_deadlock_protocol
 *	
 *	Verifies that the deadlock protocols are being observed.  Causes
 *	a panic if acquiring "semaphore" could potentially cause a deadlock.
 */
verify_deadlock_protocol(process, semaphore)
	struct proc *process;
	sema_t *semaphore;
{
	int contention_warning;
	sema_t *sema, *contention_breaker;

	if (semaphore->sa_type == SEM_SYNC)
		deadlock_error(process, semaphore, "can't own sync sema");

	contention_breaker = SEM_NULL;
	contention_warning = SEM_FALSE;

	for (sema = process->p_sema; sema != SEM_NULL; sema = sema->sa_next) {
		if (sema->sa_contention) {
			if (contention_breaker != SEM_NULL)
				deadlock_error(process, semaphore,
					"two contention breakers owned");
			contention_breaker = sema;
		}
		if (semaphore->sa_order < sema->sa_order)
			deadlock_error(process, semaphore,
				"deadlock order violation");
		if (semaphore->sa_order == sema->sa_order)
			if (!semaphore->sa_deadlock_safe || !sema->sa_deadlock_safe)
				contention_warning = SEM_TRUE;
	}
	if (!contention_warning)
		return;
	if (contention_breaker == SEM_NULL)
		deadlock_error(process, semaphore, "deadlock order violation");
	if (contention_breaker->sa_order != semaphore->sa_order)
		deadlock_error(process, semaphore, "deadlock order violation");
}

/*
 *   deadlock_error
 *
 *	Prints out useful diagnostic information before panicing when a
 *	potential deadlock condition exists.
 */
deadlock_error(process, semaphore, message)
	struct proc *process;
	sema_t *semaphore;
	char *message;
{
	sema_t *sema;

	printf("Semas owned(sema,order,contention breaker,deadlock safe):\n");
	for (sema = process->p_sema; sema != SEM_NULL; sema = sema->sa_next)
		printf("   %x  %d  %d  %d\n", 
			sema,
			sema->sa_order,
			sema->sa_contention,
			sema->sa_deadlock_safe);
	printf("(sema,order,contention breaker,deadlock safe)to be added:\n");
	printf("   %x  %d  %d  %d\n",
		semaphore,
		semaphore->sa_order, 
		semaphore->sa_contention,
		semaphore->sa_deadlock_safe);
	panic(message);
}
#endif	SEMAPHORE_DEBUG

#endif	/* MP */
