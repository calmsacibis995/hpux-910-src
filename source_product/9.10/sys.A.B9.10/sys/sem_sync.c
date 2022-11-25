/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sem_sync.c,v $
 * $Revision: 1.5.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:10:16 $
 */

/*
 *   			KERNEL SYNCHRONIZATION SEMAPHORES
 *
 *   File:
 *	kern_sem.c
 *
 *   Services:
 *	initsync- initialize a synchronization semaphore.
 *	psync	- acquire a semaphore.
 *	vsync	- release a semaphore.
 *	cvsync	- conditionally release a semaphore.
 *	wsync	- awaken a particular process sleeping on a semaphore.
 *	pchansync- acquire a semaphore with a key.
 *	vchansync- release a semaphore with a key.
 *	valusync- return the value of semaphore.
 */	
#ifdef	MP

#include "../h/assert.h"
#include "../h/types.h"
#include "../h/dir.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/sem_sync.h"
#include "../h/sem_utl.h"
#include "../h/dbg.h"
#include "../machine/psl.h"

/* #include "testsem.h" */


/*
 *   psync()
 *	
 *	Synchronization P semaphore routine.
 *
 *	Although sched_lock can be held upon entry to psync(), it
 *	is always released by psync()
 */
int
#ifdef FSD_KI
_psync(semaphore, signal_option, caller)
	sync_t *semaphore;
	sigoption_t signal_option;
        caddr_t caller;
#else
psync(semaphore, signal_option)
	sync_t *semaphore;
	sigoption_t signal_option;
#endif /* FSD_KI */
{
	int result;

	SD_ASSERT(! getnoproc(), "psync: called during noproc");
	SD_ASSERT(! ON_ICS, "psync: called from ICS");
	SD_ASSERT((spinlocks_held() == 0) ||
		((spinlocks_held()== 1) && (owns_spinlock(sched_lock))), 
			"psync: called while lock held");


/*
 * SYNC_SEMA_RECOVERY means that psync will recover up to one alpha
 * class semaphore.  When you hold a semaphore coming in, you will
 * hold it coming out
 */
#ifdef SYNC_SEMA_RECOVERY
	SD_ASSERT ((signal_option  == P_DEFER_SIGNALS)  ||
		   ( ! proc_owns_semas(u.u_procp)),
		"psync: process owns semas");
#else
	SD_ASSERT (! proc_owns_semas(u.u_procp),
		"psync: process owns semas");
#endif /* not SYNC_SEMA_RECOVERY */

	/*
	 *  It should be ok to LOOK at the sig values for our processor
         *  if all we want is a hint.  Sema_signal_pending should call
         *  issig so that job control signals (e.g., SIGTSTP, SIGCONT)
         *  will not cause sema_signal_pending to return true.
	 */
	if (signal_option != P_DEFER_SIGNALS  &&  sema_signal_pending()){
		if(owns_spinlock(sched_lock))
			spinunlock(sched_lock);
		if (signal_option == P_ALLOW_SIGNALS) {
			longjmp(&u.u_qsave);
		} else /* P_CATCH_SIGNALS */ {
			return(SEMA_SIGNAL_CAUGHT);
		}
	}
	spinlock(semaphore->s_lock);
	SEMA_LOG(semaphore, SEMA_LOG_PSYNC, 0);
	if (semaphore->count > 0) {
		semaphore->count--;
		spinunlock(semaphore->s_lock);
		if(owns_spinlock(sched_lock))
			spinunlock(sched_lock);
		return(SEMA_SUCCESS);
	} else if ( cspinlock(sched_lock) || owns_spinlock(sched_lock)) {
		semaphore->count--;
	} else {
		/* avoid deadlock */
		spinunlock(semaphore->s_lock);
		spinlock(sched_lock);
		spinlock(semaphore->s_lock);
		if (--semaphore->count >= 0) {
			/*
			 *  Although unlikely, during the above unlock/lock, 
			 *  the sync_t became free, so just take it.
			 */
			spinunlock(semaphore->s_lock);
			spinunlock(sched_lock);
			return(SEMA_SUCCESS);
		}
	}
	/*
	 *   Unable to get semaphore, so block until it's available or
	 *   until a signal is legally received.
	 */
	SEMA_LOG(semaphore, SEMA_LOG_SLEEPY_PSYNC, 0);

	/* 
	 *  Sema_sleep unlocks semaphore->s_lock and the sched_lock
	 */
#ifdef SYNC_SEMA_RECOVERY
	u.u_procp->p_recover_sema = u.u_procp->p_sema;
#ifdef FSD_KI
        result = sema_sleep(u.u_procp, semaphore, signal_option, caller);
#else
	result = sema_sleep(u.u_procp, semaphore, signal_option);
#endif /* FSD_KI */
	if (u.u_procp->p_recover_sema) {
		sema_t *recov = u.u_procp->p_recover_sema;
		u.u_procp->p_recover_sema = 0;
		psema(recov);
	} else {
		OWN_INTERRUPTS;
	}
#else 
#ifdef FSD_KI
        result = sema_sleep(u.u_procp, semaphore, signal_option, caller);
#else
	result = sema_sleep(u.u_procp, semaphore, signal_option);
#endif /* FSD_KI */
#endif /* SYNC_SEMA_RECOVERY */

        /*
	 *  If signal_option is not P_DEFER_SIGNALS, sema_signal_pending 
	 *  should always be called after returning from sema_sleep.
	 *  In the case where SIGTSTP and SIGCONT occurred while the
	 *  process was sleeping, p->p_sig would have the SIGCONT bit
	 *  turned on, p->p_cursig would be SIGTSTP and the result returned 
	 *  by sema_sleep would be SMP_SEMA_WAKE.  If sema_signal_pending is
	 *  not called in this case, p->p_cursig would remain as SIGTSTP 
	 *  when the process returns to syscall.  Syscall calls psig if
         *  p->p_cursig is not 0.  Psig would then cause the process to
         *  exit as a result of SIGTSTP, which is wrong.
         *
         */
        if (signal_option != P_DEFER_SIGNALS  &&  sema_signal_pending()){
		if (signal_option == P_ALLOW_SIGNALS) {
			longjmp(&u.u_qsave);
		} else /* P_CATCH_SIGNALS */ {
			return(SEMA_SIGNAL_CAUGHT);
		}
	}

	if (result)
		return(SEMA_SUCCESS);
	else
		return(SEMA_FAILURE);
}

/*
 *   vsync()
 *
 *	Synchronization V semaphore operation.  
 */
vsync(semaphore)
	sync_t *semaphore;
{
	struct proc  *sleeping_process, *sema_dequeue();
	sv_lock_t	nsp;

	spinlock(semaphore->s_lock);
	SEMA_LOG(semaphore, SEMA_LOG_VSYNC, 0);
	if (semaphore->count >= 0) {
		semaphore->count++;
		/* no processes waiting for semaphore */
		spinunlock(semaphore->s_lock);
	} else {
		if ( uniprocessor || cspinlock(sched_lock) ) {
			nsp.saved = sched_lock; /* we acquired it here */
		} else {
			spinunlock(semaphore->s_lock);
			spinlockx(sched_lock, &nsp);
			spinlock(semaphore->s_lock);
		}
		if (semaphore->count++ < 0) {
			if ((sleeping_process = sema_dequeue(semaphore)) == SEM_NULL)
				panic("vsync: count inconsistancy");
			spinunlock(semaphore->s_lock);

			sema_wakeup(sleeping_process, semaphore);
			SPINUNLOCKX(sched_lock, &nsp);
		} else {
			/*
			 *  Although unlikely, during the above unlock/lock, 
			 *  the waiter was awakened, so no process waiting.
			 */
			spinunlock(semaphore->s_lock);
			SPINUNLOCKX(sched_lock, &nsp);
		}
	}
}

#ifdef SEMA_COUNTING
#define SMP_SEMA_COUNTING_IO	0x2000

unsigned int sema_io_sleepened;	
unsigned int sema_io_wakened;	
#endif /* SEMA_COUNTING */
/*
 *   pchansync()
 *	
 *	Synchronization P semaphore routine.
 */
int
#ifdef FSD_KI
pchansync(semaphore, signal_option, channel, caller)
	sync_t *semaphore;
	sigoption_t signal_option;
	caddr_t channel;
        caddr_t caller;
#else
pchansync(semaphore, signal_option, channel)
	sync_t *semaphore;
	sigoption_t signal_option;
	caddr_t channel;
#endif /* FSD_KI */
{
	sv_sema_t sv_sema;
	int result;

	SD_ASSERT(! getnoproc(), "pchansync: called during noproc");
	SD_ASSERT(! ON_ICS, "pchansync: called from ICS");
	SD_ASSERT((spinlocks_held() == 0) ||
		((spinlocks_held()== 1) && (owns_spinlock(sched_lock))), 
			"pchansync: called while lock held");

	/*
	 *  It should be ok to LOOK at the sig values for our processor
         *  if all we want is a hint.  Sema_signal_pending should call
         *  issig so that job control signals (e.g., SIGTSTP, SIGCONT)
         *  will not cause sema_signal_pending to return true.
	 */
	if (signal_option != P_DEFER_SIGNALS  &&  sema_signal_pending()){
		if(owns_spinlock(sched_lock))
			spinunlock(sched_lock);
		if (signal_option == P_ALLOW_SIGNALS) {
#ifdef SYNC_SEMA_RECOVERY
			release_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
			longjmp(&u.u_qsave);
		} else /* P_CATCH_SIGNALS */
			return(SEMA_SIGNAL_CAUGHT);
	}
	spinlock(semaphore->s_lock);
	SEMA_LOG(semaphore, SEMA_LOG_PCHANSYNC, 0);
	if (semaphore->count > 0) {
		semaphore->count--;
		spinunlock(semaphore->s_lock);
		if(owns_spinlock(sched_lock))
			spinunlock(sched_lock);
		return(SEMA_SUCCESS);
	} else if ( cspinlock(sched_lock) || owns_spinlock(sched_lock) ) {
		semaphore->count--;
	} else {
		/* avoid deadlock */
		spinunlock(semaphore->s_lock);
		spinlock(sched_lock);
		spinlock(semaphore->s_lock);
		if (--semaphore->count >= 0) {
			/*
			 *  Although unlikely, during the above unlock/lock, 
			 *  the sync_t became free, so just take it.
			 */
			spinunlock(semaphore->s_lock);
			spinunlock(sched_lock);
			return(SEMA_SUCCESS);
		}
	}
	/*
	 *   Unable to get semaphore, so block until it's available or
	 *   until a signal is legally received.
	 */
	SEMA_LOG(semaphore, SEMA_LOG_SLEEPY_PSYNC, 0);
	u.u_procp->p_wchan = channel;

	/* 
	 *  Sema_sleep unlocks semaphore->s_lock and the sched_lock
	 */
#ifdef SEMA_COUNTING
	if (u.u_procp->p_pri == PRIBIO || u.u_procp->p_pri == (PRIBIO+1)) {

		u.u_procp->p_mpflag |= SMP_SEMA_COUNTING_IO;
		sema_io_sleepened++;
	}
#endif /* SEMA_COUNTING */

#ifdef SYNC_SEMA_RECOVERY
	u.u_procp->p_recover_sema = u.u_procp->p_sema;
#ifdef FSD_KI
        result = sema_sleep(u.u_procp, semaphore, signal_option, caller);
#else
        result = sema_sleep(u.u_procp, semaphore, signal_option);
#endif
	if (u.u_procp->p_recover_sema) {
		sema_t *recov = u.u_procp->p_recover_sema;
		u.u_procp->p_recover_sema = 0;
		psema(recov);
	} else {
		OWN_INTERRUPTS;
	}
#else
#ifdef FSD_KI
        result = sema_sleep(u.u_procp, semaphore, signal_option, caller);
#else
        result = sema_sleep(u.u_procp, semaphore, signal_option);
#endif /* FSD_KI */
#endif /* SYNC_SEMA_RECOVERY */

        /*
	 *  If signal_option is not P_DEFER_SIGNALS, sema_signal_pending 
	 *  should always be called after returning from sema_sleep.
	 *  In the case where SIGTSTP and SIGCONT occurred while the
	 *  process was sleeping, p->p_sig would have the SIGCONT bit
	 *  turned on, p->p_cursig would be SIGTSTP and the result returned 
	 *  by sema_sleep would be SMP_SEMA_WAKE.  If sema_signal_pending is
	 *  not called in this case, p->p_cursig would remain as SIGTSTP 
	 *  when the process returns to syscall.  Syscall calls psig if
         *  p->p_cursig is not 0.  Psig would then cause the process to
         *  exit as a result of SIGTSTP, which is wrong.
         *
         */
	if (signal_option != P_DEFER_SIGNALS  &&  sema_signal_pending()) {
		if (signal_option == P_ALLOW_SIGNALS) {
#ifdef SYNC_SEMA_RECOVERY
			release_semas(&sv_sema);
#endif /* SYNC_SEMA_RECOVERY */
			longjmp(&u.u_qsave);
		} else /* P_CATCH_SIGNALS */
			return(SEMA_SIGNAL_CAUGHT);
	}

	if (result)
		return(SEMA_SUCCESS);
	else
		return(SEMA_FAILURE);
}

/*
 *   vchansync()
 *
 *	Synchronization V semaphore operation.  
 *	Wakes up a process that blocked on a semaphore with a specific
 *	channel.
 *	Returns TRUE if a process was awakened by the V operation,
 *	otherwise FALSE.
 *
 *	Must hold sched_lock upon entrance.  Not permitted to release it.
 */
int
vchansync(semaphore, channel)
	sync_t *semaphore;
	caddr_t channel;
{
	struct proc *marker;
	sv_lock_t	nsp;

	SPINLOCKX(sched_lock, &nsp);
	spinlock(semaphore->s_lock);
	SEMA_LOG(semaphore, SEMA_LOG_VCHANSYNC, 0);
	for (marker=semaphore->wait_list; marker!=SEM_NULL; 
		marker=marker->p_wait_list) {
		if (marker->p_wchan == channel) {
			sema_unlink(semaphore, marker);
			semaphore->count++;
 			spinunlock(semaphore->s_lock);

			sema_wakeup(marker, semaphore);
#ifdef SEMA_COUNTING
			if(marker->p_mpflag & SMP_SEMA_COUNTING_IO){
				sema_io_wakened++;
				marker->p_mpflag &= ~SMP_SEMA_COUNTING_IO;
			}
#endif /* SEMA_COUNTING */

			SPINUNLOCKX(sched_lock, &nsp);
			return(SEMA_SUCCESS);
		}
	}
	spinunlock(semaphore->s_lock);
	SPINUNLOCKX(sched_lock, &nsp);
	return(SEMA_FAILURE); /* no process sleeping on the specified channel */
}

/*
 *   cvsync()
 *
 *	Conditional synchronization V semaphore operation.  
 *	Does a V operation if and only if there is at least one
 *	process sleeping on the semaphore wait list.
 *	Returns SEMA_SUCCESS if a process was awakened by the V operation,
 *	SEMA_FAILURE if the V operation failed.
 */
int
cvsync(semaphore)
	sync_t *semaphore;
{
	struct proc	*sleeping_process, *sema_dequeue();
	sv_sema_t	nsp;

	spinlock(semaphore->s_lock);
	SEMA_LOG(semaphore, SEMA_LOG_CVSYNC, 0);
	if (semaphore->count < 0) {
		semaphore->count++;
		if ((sleeping_process = sema_dequeue(semaphore)) == SEM_NULL)
			panic("cvsync: count inconsistancy");
		spinunlock(semaphore->s_lock);
		/* 
		 *  No race here with psignal, since wsignal will
		 *  return failure if called since p_sleep_sema == 0 
		 */
		if (!uniprocessor)
			spinlockx(sched_lock, &nsp);
		sema_wakeup(sleeping_process, semaphore);
		if (!uniprocessor)
			spinunlockx(sched_lock, &nsp);
		return(SEMA_SUCCESS);
	}
	else { /* no processes waiting for semaphore, do NOT V */
		spinunlock(semaphore->s_lock);
		return(SEMA_FAILURE);
	}
}

/*
 *   wsync()
 *
 *	Synchronization V semaphore operation.  
 *	Wakes up a specific process sleeping on a synchronization
 *	semaphore.
 *	Returns TRUE if the process was awakened by the V operation,
 *	otherwise FALSE.
 */
int
wsync(semaphore, process)
	sync_t *semaphore;
	struct proc *process;
{
 	sv_lock_t	nsp;
 
	/*
	 *  The sched_lock may only have to be around the sema_wakeup XXX -jkr
	 */
 	SPINLOCKX(sched_lock, &nsp);
	spinlock(semaphore->s_lock);  
	if (process->p_sleep_sema != (struct sema *)semaphore  ||
	     !(process->p_mpflag & SMP_SEMA_BLOCK) ){
		spinunlock(semaphore->s_lock);
		SPINUNLOCKX(sched_lock, &nsp);
		return(SEMA_FAILURE);
	}
	SEMA_LOG(semaphore, SEMA_LOG_WSYNC, 0);
	if (semaphore->count++ < 0) {
		sema_unlink(semaphore, process);
		spinunlock(semaphore->s_lock);

		sema_wakeup(process, semaphore);

 		SPINUNLOCKX(sched_lock,&nsp);
		return(SEMA_SUCCESS);
	} else { 
		/* no processes waiting for semaphore */
		spinunlock(semaphore->s_lock);
 		SPINUNLOCK(sched_lock);
		panic("wsync: count inconsistancy");
	}

	return(SEMA_FAILURE);
}

/*
 *   initsync()
 *
 *	Initializes a synchronization semaphore.  Must be called before a
 *	semaphore is used.  *** Warning *** Must not be called when the
 *	semaphore is actively being used by the kernel.
 */
initsync(semaphore)
	sync_t *semaphore;
{
	semaphore->count = 0;
	semaphore->wait_list = SEM_NULL;	/* no processes waiting */
#ifdef	SEMAPHORE_DEBUG
	semaphore->last_pid = 0;
	semaphore->max_waiters = 0;
	semaphore->p_count = 0;
	semaphore->failed_cp_count = 0;
	semaphore->sleepy_p_count = 0;
	semaphore->s_type = SEM_SYNC;
#endif
	semaphore->s_lock = alloc_spinlock(SEMAPHORE_LOCK_ORDER, "sync_t spinlock");
}

/*
 *   valusync()
 *
 *	Returns the current count for a semaphore.
 */
int 
valusync(semaphore)
	sync_t *semaphore;
{
	return(semaphore->count);
}
#endif	/* MP */
