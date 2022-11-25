/*
 * @(#)spinlock.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:35:10 $
 * $Locker:  $
 */

#ifndef	_SPINLOCK_INCLUDED
#define	_SPINLOCK_INCLUDED


#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../machine/cpu.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <machine/cpu.h>
#endif /* _KERNEL_BUILD */

/*
 *   Lock.  Simple protection mechanism for protecting
 *   critical sections.  Building block for semaphores.
 */

 /* XXXX When this compiles remove "FORMERLY" comments */

typedef struct lock
{
	int	sl_count;
 	int	sl_owner;		/* MP id of owning processor */
#if	defined(MP)
	int	sl_order;	/* FORMERLY "order" */
	char *	sl_name;	/* FORMERLY "name" */
#endif	/* defined(MP) || defined(SPINLOCK_DEBUG) */
#ifdef GPROF
	u_int	sl_spares[3];	/* align on cache boundary */
	u_int	sl_holder
#else	/* ! GPROF */
	u_int	sl_spares[4];	/* align on cache boundary */
#endif /* ! GPROF */
} lock_t;

typedef	lock_t		*vm_lock_t;

#ifdef _KERNEL

typedef	struct {
	lock_t *saved;
} sv_lock_t;

/* XXX PRE_ARBITRATION_MAX needs investigation. 4000 picked out of the air */
#define PRE_ARBITRATION_MAX	4000	

struct spinarb {
	u_int	sa_on;			/* 0=off, 1=on */
	u_int	sa_pre_arbitration_max; /* Debug */
	u_int	sa_spinlocks_attempted;	/* Debug */
	u_int	sa_prearb_attempted; 	/* Debug */
	u_int	sa_arb_attempted;	/* Debug */
	u_int	sa_max_loops;		/* Debug */
	u_int	sa_spares[2];		/* Alignment to 32 bytes */
};

/* Arbitration tickets are 64 bit quantities */
struct arb_ticket{
	u_int at_low;	/* Low order word */
	u_int at_high;  /* High order word */
};

struct percpu_arb {
	lock_t	*a_wanted_lock;		/* Lock I am trying for right now */
	struct arb_ticket a_ticket;	/* When is it my turn? */
	u_int 	a_secs;			/* Debug.  When did I start waiting? */
	u_int	a_spare[4];		/* Alignment to 32 bytes  */
};

extern	lock_t *sched_lock;
extern	lock_t *activeproc_lock;
extern	lock_t	lock_init_lock;
extern	lock_t *spl_lock;
extern	lock_t *semaphore_log_lock;
extern	lock_t *crash_monarch_lock;
extern	lock_t *callout_lock;
extern	lock_t *proc_lock;
extern	lock_t *cred_lock;
extern	lock_t *vmsys_lock;
extern	lock_t *file_table_lock;
extern	lock_t *devvp_lock;
extern	lock_t *biodone_lock;
extern	lock_t *bbusy_lock;
extern	lock_t *ioserv_lock;
extern	lock_t *io_tree_lock;
extern	lock_t *v_count_lock;
extern	lock_t *buf_hash_lock;
extern	lock_t *lpmc_log_lock;
extern	lock_t *itmr_sync_lock;
extern	lock_t *pdce_proc_lock;
extern	lock_t *pfail_cntr_lock;
extern	lock_t *dnlc_lock;

/* networking spinlocks */
extern	lock_t *netisr_lock;
extern	lock_t *ntimo_lock;
extern	lock_t *bsdskts_lock;

extern	lock_t *alloc_spinlock();

extern int spinlock_usav();
/*
 * Locking operations.  
 */
#ifdef	__hp9000s800
#ifdef _KERNEL_BUILD
#include "../machine/inline.h"
#include "../machine/psl.h"
#else /* ! _KERNEL_BUILD */
#include <machine/inline.h>
#include <machine/psl.h>
#endif /* _KERNEL_BUILD */

#define	DISABLE_INT(VAR)	_RSM(PSW_I, VAR)
#define	ENABLE_INT(VAR)		((PSW_I & VAR) ? (_SSM(PSW_I, 0),0) : 0 )
#endif	/* __hp9000s800 */
#ifdef	__hp9000s300
#define	owns_spinlock(lock)	(lock->sl_count == 0)
#define	DISABLE_INT(VAR)	( VAR = CRIT() )
#define	ENABLE_INT(VAR)		( UNCRIT(VAR) )
#endif	/* __hp9000s300 */

#define spinlocks_held()	(getproc_info()->spinlock_depth)
#define processor_locked()	(getproc_info()->lock_depth)

#define lock_processor()		lock_processor_dbg()
#define unlock_processor() 		unlock_processor_dbg() 

#if	defined(MP)


#else	/* ! defined(MP) || defined(SPINLOCK_DEBUG) */

#define	spinlock(LOCK)	{ \
				register bits;				\
				struct	mpinfo	*mpi;			\
									\
				DISABLE_INT(bits);			\
				mpi = getproc_info();			\
				if (((mpi->lock_depth)++) == 0)		\
					mpi->entry_spl_level = bits;	\
				/* mpi->spinlock_depth++; */		\
				(LOCK)->sl_count = 0;			\
}

#define	spinunlock(LOCK) { \
				struct	mpinfo	*mpi;			\
									\
				(LOCK)->sl_count = 1;			\
				mpi = getproc_info();			\
				/* --mpi->spinlock_depth; */		\
				if ((--(mpi->lock_depth)) == 0)		\
					ENABLE_INT(mpi->entry_spl_level); \
}


#endif	/* ! defined(MP) || defined(SPINLOCK_DEBUG) */
/*
      Low overhead spinlocks - not available for public consumption.
      Coming soon to a theater near you.
*/
#if defined(MP)

#define SPINLOCK_USAV(lock, context)	context = spinlock_usav(lock)
#define SPINUNLOCK_USAV(lock, context)	spinunlock_usav(lock, context)

#else /* !(defined(MP)&&!defined(SEMAPHORE_DEBUG)) */

#define SPINLOCK_USAV(lock, context)	spinlock(lock)
#define SPINUNLOCK_USAV(lock, context)	spinunlock(lock)

#endif /* defined(MP) && !defined(SEMAPHORE_DEBUG) */


/* The lock for process local data in the proc structure */
#define PROCESS_LOCK(p)		(proc_lock)

/*
 *  For MP kernels the sleep/wakeup race is protected with spinlocks,
 *  and for non-MP kernels, the sleep/wakeup race is protected with
 *  spl levels.  This means that the non-MP sleep needs to do an
 *  splx() after the sleep.  For MP kernels, the sleep_lock is freed
 *  in the heart of swtch().
 */
#ifdef	MP
#define	sleep_then_unlock(CHAN, PRI, SPLVAL)	sleep(CHAN, PRI)
#define	SLEEP_THEN_UNLOCK(CHAN, PRI, SPLVAL)	sleep(CHAN, PRI)
#else	/* ! MP */
#define	SLEEP_THEN_UNLOCK(CHAN, PRI, SPLVAL)	sleep(CHAN, PRI)
#endif	/* ! MP */

extern unsigned int sleep_lock();
#define SLEEP_LOCK()		(uniprocessor ? 0 : sleep_lock())
#define SLEEP_UNLOCK(SPLVAL)	(uniprocessor ? 0 : sleep_unlock(SPLVAL))

#define MP_SPINLOCK(lock)          if (!uniprocessor) spinlock(lock)
#define MP_SPINUNLOCK(lock)        if (!uniprocessor) spinunlock(lock)

#define SPINLOCK(lock)		MP_SPINLOCK(lock)	/* OBSOLETE */
#define SPINUNLOCK(lock)	MP_SPINUNLOCK(lock)	/* OBSOLETE */

#define	AQUIRESPL_LOCK()	if (!uniprocessor) aquirespl_lock()
#define	RELEASESPL_LOCK()	if (!uniprocessor) releasespl_lock()

#ifdef	MP

extern void spinlock();
extern void spinunlock();

#define	SPINLOCKX(lock,ss)	if (!uniprocessor) spinlockx(lock,ss)
#define	SPINUNLOCKX(lock,ss)	if (!uniprocessor) spinunlockx(lock,ss)

#else	/* ! MP */

/*  this is only so we can compile w/out MP defined */
#define	SPINLOCKX(lock,ss)
#define	SPINUNLOCKX(lock,ss)

#endif	/* ! MP */

#define SPL_REPLACEMENT(lock,func,var)	if (uniprocessor) var=func(); \
					else spinlock(lock)
#define SPLX_REPLACEMENT(lock,var)	if (uniprocessor) splx(var); \
					else spinunlock(lock)
#define	UP_SPL7()		(uniprocessor ? spl7() : 0)
#define	UP_SPL6()		(uniprocessor ? spl6() : 0)
#define	UP_SPL5()		(uniprocessor ? spl5() : 0)
#define	UP_SPL2()		(uniprocessor ? spl2() : 0)
#define	UP_SPLX(var)		(uniprocessor ? splx(var) : 0)

#ifdef  __hp9000s800
#define PFAIL_DISABLE(X)        (DISABLE_INT(X), X)
#define PFAIL_ENABLE(X)         ENABLE_INT(X)
#endif  /* __hp9000s800 */

#define T_SPINLOCK(lock)	MP_ASSERT(owns_spinlock(lock),	\
	"Should own spinlock!")
#define TBAR_SPINLOCK(lock)	MP_ASSERT(!owns_spinlock(lock),	\
	"Can't own spinlock!")
#define SPL_BITCHECK(SPL_LVL)					\
	(!(rsm(0) & PSW_I) /* I bit off always enough */ ||	\
	 (SPL_LVL == SPL7 ? (splcur() == SPL7) 		 	\
			 : (!(~SPL_LVL & splcur()) || (splcur()==SPL7)) ) )
#define	T_SPL(SPL_LVL)						\
	DO_ASSERT(SPL_BITCHECK(SPL_LVL), "Spl not high enough")
#define	T_UPSPL(SPL_LVL)					\
	UP_ASSERT(SPL_BITCHECK(SPL_LVL), "Spl not high enough")

/*
 *  These spinlocks are only meaningful on an MP system
 */
#define vmemp_slock()		SPINLOCK(vmsys_lock)
#define vmemp_sunlock()		SPINUNLOCK(vmsys_lock)
#define vmemp_slockx()		SPINLOCKX(vmsys_lock, &sema_state.lock)
#define vmemp_sunlockx()	SPINUNLOCKX(vmsys_lock, &sema_state.lock)

/*
 *  These are the macro names used in the VM system
 *  Some day we can just rename everything to be clean.  For now...
 */
#define	vm_spinlock(SMP)	spinlock(SMP)
#define	vm_spinunlock(SMP)	spinunlock(SMP)
#define	vm_valulock(LOCK)		( owns_spinlock(LOCK) ? 0 : 1 )
#define vm_initlock(LOCK, ORDER, NAME)	{ LOCK = alloc_spinlock(ORDER, NAME); }

#endif  /* _KERNEL */
#endif	/* ! _SPINLOCK_INCLUDED */
