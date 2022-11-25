/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/spinlock.c,v $
 * $Revision: 1.10.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 11:53:46 $
 */


#ifdef	SEMAPHORE_DEBUG
#define	SPINLOCK_DEADLOCK_CHECKING
#define	SPIN_TIMEOUT
#define	DO_PREVPC
#endif	/* SEMAPHORE_DEBUG */

#ifdef	RDB
#define	SPIN_RDB_WAKE
#endif	/* RDB */

#include "../h/assert.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/proc.h"
#include "../h/mp.h"
#include "../h/semglobal.h"
#include "../h/spinlock.h"
#include "../h/debug.h"

#ifdef	__hp9000s800
#include "../machine/rdb.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/cpu.h"
#include "../machine/inline.h"

#include "../machine/spl.h"
#define	SPINLOCK_EIEM	(REMOTE_PFAIL_INT_ENABLE | RDB_INT_ENABLE)
#endif	/* __hp9000s800 */

#ifdef	SPINLOCK_DEADLOCK_CHECKING

#define	DEADLOCK_CHECK(LOCK)	deadlock_check(LOCK)
#define	SPINLOCK_ADD(LOCK)	spinlock_add(LOCK)
#define	SPINLOCK_DELETE(LOCK)	spinlock_delete(LOCK)

#else	/* ! SPINLOCK_DEADLOCK_CHECKING */

#define	DEADLOCK_CHECK(LOCK)
#define	SPINLOCK_ADD(LOCK)
#define	SPINLOCK_DELETE(LOCK)

#endif	/* ! SPINLOCK_DEADLOCK_CHECKING */

#ifdef	DO_PREVPC
#define	PREVPC(var, uwptr)	(do_spin_prevpc ? var = prevpc(uwptr) : 0 )
void	*prevpc();
int 	do_spin_prevpc = 1;
void	*spinlock_dbg_unw = 0;
void	*cspinlock_dbg_unw = 0;
void	*spinunlock_dbg_unw = 0;
void	*spinlockx_unw = 0;
void	*spinunlockx_unw = 0;
#else	/* ! DO_PREVPC */
#define	PREVPC(var, uwptr)
#endif	/* ! DO_PREVPC */

/* Spinlocks... */

#if	defined(MP) || defined(SEMAPHORE_DEBUG)
#define	LOCK_INIT(ORDER, NAME)	{ 1, 0, ORDER, NAME }
#else	/* ! defined(MP) || defined(SEMAPHORE_DEBUG) */
#define	LOCK_INIT(ORDER, NAME)	{ 1, 0 }
#endif	/* ! defined(MP) || defined(SEMAPHORE_DEBUG) */

#ifdef	__hp9000s800
#pragma	align 16
#endif	/* __hp9000s800 */

lock_t	lock_init_lock = LOCK_INIT(LOCK_INIT_LOCK_ORDER, "initialization lock");
/*
 * Process table locks
 */
lock_t *sched_lock = NULL;
lock_t *activeproc_lock = NULL;

#ifdef	__hp9000s800
#pragma	align 16
#endif	/* __hp9000s800 */

lock_t	_spl_lock = LOCK_INIT(SPL_LOCK_ORDER, "SPL spinlock");
lock_t *spl_lock = &_spl_lock;
/*		lock_t *crash_monarch_lock = NULL;	OLD_CRASH_MONARCH */
lock_t *io_tree_lock = NULL;
lock_t *proc_lock = NULL;
lock_t *cred_lock = NULL;
lock_t *file_table_lock = NULL;
lock_t *devvp_lock = NULL;
lock_t *dnlc_lock = NULL;

#ifdef	MP
lock_t *ioserv_lock = NULL;
lock_t *callout_lock = NULL;
lock_t *semaphore_log_lock = NULL;
lock_t *biodone_lock = NULL;
lock_t *bbusy_lock = NULL;
lock_t *vmsys_lock = NULL;		/* generic spinlock for VM system */
lock_t *lpmc_log_lock = NULL;		/* lock for LPMC logging */
lock_t *itmr_sync_lock = NULL;		/* lock for itmr synchronization */
lock_t *pdce_proc_lock = NULL;		/* lock for calling pdce_proc */
lock_t *pfail_cntr_lock = NULL;		/* lock for power-down counters */

/* Networking spinlocks */
lock_t *netisr_lock = NULL;
lock_t *bsdskts_lock = NULL;
#endif /* MP */

lock_t *v_count_lock = NULL;
lock_t *buf_hash_lock = NULL;
lock_t *clist_lock = NULL;		/* lock for clists in getc/putc */

/* Networking spinlocks */
lock_t *ntimo_lock = NULL;

#ifdef	MP

#define	WATCHDOG_CHASSIS
#ifdef	WATCHDOG_CHASSIS
#pragma	align	16
lock_t	live_proc_lock = { 1 };	/* lock for updating which procs are alive */
unsigned char	live_proc_count;
unsigned char	live_proc_map;
unsigned char	live_proc_cycle;
#endif	WATCHDOG_CHASSIS

#endif	MP

#ifdef	__hp9000s300
struct mpinfo mpproc_info[MAX_PROCS] = {0};
#endif	/* __hp9000s300 */

/*
 *   Debugging version of spinlocks.  State is held in a per-processor lock
 *      save area.
 *
 *   Algorithm:
 *	A lock is initialized to 1 by initlock().  Spinlock() sets it 0.
 *	Spinunlock() restores it to 0.  Acquiring a spinlock blocks
 *	interrupts.  Nested acquisition keeps interrupts disabled, with
 *	the initial spl level restored only when the last spinlock is
 *	released.  Note that since a context switch of any kind cannot
 *	occur while any spinlock is held, the spinlock_depth and
 *	spinlock_spl_level are sufficient for holding state.
 */
#define MICROSECOND		(ITICK_PER_SEC / 1000000)


#ifdef __hp9000s800
/*------------------------------------------------------------------------
 *
 *		P R O C E S S O R    L O C K S
 *
 * lock_processor_dbg()    -- acquire processor by disabling interrupts.
 * unlock_processor_dbg()  -- release processor by restoring interrupts.
 *
 *------------------------------------------------------------------------*/

/*
 *  Should use something like [un]lock_processor_dbg from spl routines, and
 *  then these routines should be the ones setting and clearing the I bit.
 */
lock_processor_dbg()
{
	register u_int	ibit;
	struct mpinfo	*mpi;

	mpi = getproc_info();
	DISABLE_INT(ibit);
	if (mpi->lock_depth++ == 0) {
		mpi->entry_psw_mask = ibit;
	}
}

unlock_processor_dbg()
{
	struct mpinfo	*mpi;
	register u_int	ibit;

	mpi = getproc_info();
	SD_ASSERT((mpi->lock_depth > 0),
		"unlock_processor_dbg: not locked");
#ifdef	__hp9000s800
	SD_ASSERT( ! (_SSM(0, ibit) , (ibit & PSW_I)),
		"unlock_processor_dbg entered w/ PSW_I bit on.");
#endif	/* __hp9000s800 */
	if (--mpi->lock_depth == 0) {
		ibit = mpi->entry_psw_mask;
		ENABLE_INT(ibit);
	}
}
#endif /* __hp9000s800 */

#ifdef NEVER_CALLED
processor_locked_dbg()
{
	return(getproc_info()->lock_depth != 0);
}
#endif /* NEVER_CALLED */



/*------------------------------------------------------------------------
 *
 *			S P I N L O C K S
 *
 *------------------------------------------------------------------------*/

/*
 *  Here are some macros to make the code more readable with debug
 *  stuff in place
 */
/*
 *  Catch spinlocks which wait too long
 */
#ifdef	SPIN_TIMEOUT
int 		spinlock_timeout = 1;
unsigned int	spin_loop_cnt;
#define	SPIN_LOOP_CNT	1000000	/* this many times through the acquire loop */

#define TOO_MANY_SPINS(loop_cnt) \
	if (spinlock_timeout && (++(loop_cnt) > SPIN_LOOP_CNT)) {\
		printf("Spinlock '%s' took too long. Iterations = %d\n", \
						lock->sl_name, loop_cnt);\
					panic("Spinlock timeout.");\
	}
#else	/* ! SPIN_TIMEOUT */
#define	TOO_MANY_SPINS(loop_cnt)
#endif	/* ! SPIN_TIMEOUT */


/*
 *  Allow RDB interrupts while waiting for spinlocks
 */
#ifdef	SPIN_RDB_WAKE
int
break_rdb_in_spin(){}

extern int comm_and_sync_mask;
#define BREAK_RDB_IN_SPIN()\
		{	register int	eirr;\
			_MFCTL(CR_EIRR, eirr);\
			if ( eirr & comm_and_sync_mask )\
				break_rdb_in_spin();\
		}
#else	/* ! SPIN_RDB_WAKE */
#define BREAK_RDB_IN_SPIN()
#endif	/* ! SPIN_RDB_WAKE */

/*
 *  Code for checking spinlock ordering, and shutting off the checks
 *  when the system is going down (try to avoid multiple panics)
 */
#ifdef	SPINLOCK_DEADLOCK_CHECKING
int bypass_spinlocking=0;

#define	DEADLOCK_BYPASS()\
		if ( bypass_spinlocking ) {\
			return;\
		}
#else	/* ! SPINLOCK_DEADLOCK_CHECKING */
#define	DEADLOCK_BYPASS()
#endif	/* ! SPINLOCK_DEADLOCK_CHECKING */

/*
 *  Arbitration debug macros
 */
#ifdef SEMAPHORE_DEBUG

/* These all ignore wrap-around.  We don't run systems that long */
#define SPINLOCKS_ATTEMPTED	(spinarb.sa_spinlocks_attempted++)
#define PREARB_ATTEMPTED	(spinarb.sa_prearb_attempted++)
#define ARB_ATTEMPTED		(spinarb.sa_arb_attempted++)
#define MAX_LOOPS(d)		{if(spinarb.sa_max_loops<d) \
					spinarb.sa_max_loops=d; }

#else

#define SPINLOCKS_ATTEMPTED		
#define PREARB_ATTEMPTED
#define ARB_ATTEMPTED
#define MAX_LOOPS(d)

#endif /* SEMAPHORE_DEBUG */

#ifdef	__hp9000s800
/*
 * Spinarb indicates whether arbitration is in effect or not.
 *
 * Notice that it is a global resource.  It could easily be made
 * a per-spinlock resource if the performance of this is not
 * high enough (this could happen if arbitration were common).  In
 * that case it would only require 1 word per spinlock, as most of it
 * is debug information.
 */

#pragma	align	32	/* Performance reasons only, on cache lines */
struct spinarb spinarb = {0, PRE_ARBITRATION_MAX, 0, 0, 0, 0, 0, 0};

u_int	spinlock_arbitration();

#ifdef SEMAPHORE_DEBUG
int spinlock_type = 0;
spinlock(lock)
	lock_t *lock;
{

	SD_ASSERT( (lock > (lock_t *) 0x100), "spinlock: bad lock pointer");
	SD_ASSERT( (lock == ((unsigned int)lock & -16)), 
		   "spinlock: bad spinlock address. Unaligned");
	SD_ASSERT( ! owns_spinlock(lock), "Spinlock already acquired");
	SD_ASSERT( pf_spinlock_ok(lock), "spinlock: failed pfail checks");


	DEADLOCK_CHECK(lock);

	SPINLOCKS_ATTEMPTED;		/* Count spinlock attempts */

#ifdef MPW
#define old_spinlock_owner sl_spares[0]
#define new_spinlock_owner sl_spares[1]

	/* Note: this is not exact, but it is close enough. */
	if (getiva() == lock->sl_count)
		lock->old_spinlock_owner++;
	else
		lock->new_spinlock_owner++;
#endif  /* MPW */


	if((spinlock_type & 1) == 0)
		asm_spinlock(lock);
	else
		cc_spinlock(lock);

	SPINLOCK_ADD(lock);
}

cc_spinlock(lock)
	lock_t *lock;
{
	register int	locked;
	register int	*count;
	register u_int	ibit;
	register u_int	eiem;
	register u_int	iva;
	struct mpinfo	*mpi;
	register u_int	pre_arb_cnt;

	count = &lock->sl_count;

	DISABLE_INT(ibit);
	_LDCWS(0, 0, count, locked);
	if ( ! locked ) {
		ENABLE_INT(ibit);
		DEADLOCK_BYPASS();
		pre_arb_cnt = 0;

		while(1){
			while(! *count){
				if((pre_arb_cnt++ > PRE_ARBITRATION_MAX)
				   || spinarb.sa_on){
 				        ibit= spinlock_arbitration(lock, count);
					goto got_spinlock;
				}
			}
			DISABLE_INT(ibit);
			_LDCWS(0, 0, count, locked);
			if(locked)
				break;
			ENABLE_INT(ibit);
		}
	}

got_spinlock:
	mpi = getproc_info();
	if (mpi->spinlock_depth++ == 0) {
		_MFCTL(CR_EIEM, eiem);		/* save current spl/eiem */
		mpi->entry_spl_level = eiem;
		eiem = SPINLOCK_EIEM;
		_MTCTL(eiem, CR_EIEM);	
	}
	_MFCTL(CR_IVA, iva);
	lock->sl_owner = iva;
	ENABLE_INT(ibit);
}
#endif /* SEMAPHORE_DEBUG */

/* 
 * The global spinlock arbitration ticket.  A two word quantity
 * so that wrap-around should be put off until after the system 
 * has crashed.
 */
#pragma	align	32
struct arb_ticket arb_ticket;

/*
 * The array of per-cpu arbitration structures.  In it we store
 * our ticket and which lock we are waiting for
 */
#pragma	align	32	/* Performance reasons only, on cache lines */
struct percpu_arb arbs[MAX_PROCS];	/* one for each CPU, 32 bytes long */

/*
 * spinlock_arbitration() provides a mechanism to decide which
 * processor will get a particular spinlock next when more than
 * one is waiting for it.
 *
 * The general algorithm is:
 *	Processor gets a ticket.
 *      loop:
 *		Processor waits for a lower ticket holder to obtain lock.
 *		Processor trys for lock.  If successful, break.
 *		Processor waits for lock to become available.
 *		Processor trys for lock.  If successful, break.
 *	return
 *		PSW_I is off
 *		return the PSW mask as it was when we disabled the PSW_I bit
 *
 * General notes;
 *	- External interrupts are blocked, but polled for.
 *	- Arbitration is a big pain.
 */

u_int
spinlock_arbitration(lock, count)
	lock_t *lock;
	register int	*count;
{
	/*
	 * Arbitration info
	 */
	register int	locked;		/* Value of spinlock checked via LDCWS*/
	struct arb_ticket my_ticket;	/* The value of my ticket */
	struct percpu_arb *my_arb;	/* Pointer to my arbitration struct */
#ifdef SEMAPHORE_DEBUG
	u_int my_time;
#endif /* SEMAPHORE_DEBUG */

	/* arb struct that we will wait for */
	struct percpu_arb *waitfor_arb;

	/*
	 * save state in case of interrupts.
	 */
	struct percpu_arb saved_arb;	
	register u_int saved_eiem;
	register u_int ibit;

	register u_int eirr;	/* Just used to test for external interrupts*/

	/*
	 * Other
	 */
	int cpu, my_cpu;	/* Processor number loop variable, my proc num*/
	u_int t_lo, t_hi;	/* Temporary ticket holders */

#ifdef SEMAPHORE_DEBUG
	int loop_cnt = 0;	/* Panic if we loop too much */
	ARB_ATTEMPTED;		/* Count arbitration attempts */
#endif /* SEMAPHORE_DEBUG */


	/*
	 * Get my ticket in the queue.
	 * Tickets have the following property;
	 *	Each ticket is larger than any issued a while ago.
	 *	No two currently existing tickets are the same.
	 *	Eventually, a ticket is guaranteed to be the lowest
	 *		held by any processor trying to get this
	 *		spinlock, at which point the holder will 
	 *		eventually become the only processor spinning 
	 *		waiting for it. 
	 * We assume that our two word tickets never wrap around.
	 * Even if they do wrap around, the queue just has to empty
	 * once to get us back to being OK.
	 */
	my_cpu = getprocindex();
	my_arb = &arbs[my_cpu];

	/* 
	 * Notice state of Interrupt bit.
	 * Save and clear the eiem.
	 */
	_RSM(0,ibit);
	_MFCTL(CR_EIEM, saved_eiem);
	_MTCTL( 0, CR_EIEM);	/* turn off external interrupts */

#ifdef SEMAPHORE_DEBUG
	my_time = time.tv_sec;
	my_arb->a_secs   = my_time;
#endif /* SEMAPHORE_DEBUG */

new_ticket:
	/*
	 * The sequence here is important to guarantee starvation
	 * avoidance.
	 * The key point is that you have to retrieve things 
	 * from the arb_ticket in the order (high,low) and set
	 * them in the order (low,high);  then `my_ticket` will
	 * always have the smaller high order word.
	 * STRONGLY_ORDERED
	 */
	t_hi = arb_ticket.at_high ;
	t_lo = arb_ticket.at_low ;

	my_ticket.at_low  = t_lo + my_cpu;
	my_ticket.at_high = t_hi;
	
	t_lo += MAX_PROCS;
	arb_ticket.at_low = t_lo;
	if(t_lo < MAX_PROCS){
		t_hi += 1;
		arb_ticket.at_high = t_hi;	/* STRONGLY_ORDERED */
	}

	/*
	 * Set my per-processor status saying I am waiting for
	 * this spinlock with my ticket. 
	 * STRONGLY_ORDERED
	 */
	my_arb->a_ticket = my_ticket;
	my_arb->a_wanted_lock = lock;	/* STRONGLY_ORDERED */


	while (1) {
		/*
		 * Look through the processors for anyone with our lock
		 * and a lower ticket number.
		 */
		waitfor_arb = 0;
		for(cpu = 0; cpu < MAX_PROCS; cpu++){
			struct percpu_arb *a;
			a= &arbs[cpu];

			if((a->a_wanted_lock == lock) &&
			   ((a->a_ticket.at_high < my_ticket.at_high) ||
			    ((a->a_ticket.at_high == my_ticket.at_high) &&
			     (a->a_ticket.at_low < my_ticket.at_low)))){
				waitfor_arb = a;
				break;
			}
		}

	
		/*
		 * Wait for that lower ticket holder to vacate 
		 * his ticket.  Once he does so, we fall through
		 * and try to grab the lock ( perhaps along with
		 * some others also waiting for the lower ticket 
		 * holder to finish)
		 */
		if(waitfor_arb){

			while (( waitfor_arb->a_wanted_lock == lock) &&
			((waitfor_arb->a_ticket.at_high < my_ticket.at_high) ||
			((waitfor_arb->a_ticket.at_high == my_ticket.at_high) &&
			(waitfor_arb->a_ticket.at_low < my_ticket.at_low)))){

				TOO_MANY_SPINS(loop_cnt);/* Debug */

				/* Check for external interrupts */
				_MFCTL(CR_EIRR,eirr);
				if( (ibit & PSW_I) &&( eirr & saved_eiem)){
					saved_arb = *my_arb;
					my_arb->a_wanted_lock = 0;
					_MTCTL(saved_eiem, CR_EIEM );
					_MTCTL(0, CR_EIEM);
					my_cpu = getprocindex();
					my_arb = &arbs[my_cpu];
					*my_arb = saved_arb;
				}
			}
		}

		/*
		 * Our turn to try for the processor
		 */
		DISABLE_INT(ibit);
		_LDCWS(0, 0, count, locked);
		if ( locked )
			break;
		ENABLE_INT(ibit);

		/*
		 * Now spin waiting for the spinlock to become free
		 */
		while (! *count) {
			/* 
			 * Tell everyone we are arbitrating right now!
			 * (Test first to avoid cache thrash.)
			 */
			if(spinarb.sa_on == 0)		/* Arbitration on? */
				spinarb.sa_on = 1;	/* Arbitration ON! */

			/*
			 * Check for external interrupts
			 */
			_MFCTL(CR_EIRR,eirr);
			if((ibit & PSW_I) && ( eirr & saved_eiem )){
				saved_arb = *my_arb;
				my_arb->a_wanted_lock = 0;
				_MTCTL(saved_eiem, CR_EIEM );
				_MTCTL(0, CR_EIEM );
				my_cpu = getprocindex();
				my_arb = &arbs[my_cpu];
				*my_arb = saved_arb;
			}

			/* Debug code */
			BREAK_RDB_IN_SPIN();		/* Debug only */
			TOO_MANY_SPINS(loop_cnt);	/* Debug only */
			MAX_LOOPS(loop_cnt);		/* Debug only */
		}

		/*
		 * Our turn to try for the processor, again!
		 */
		DISABLE_INT(ibit);
		_LDCWS(0, 0, count, locked);
		if ( locked )
			break;
		ENABLE_INT(ibit);
	}

	my_arb->a_wanted_lock = 0;	/* I'm not arbitrating ... */
	spinarb.sa_on = 0;		/*  ... so nobody should arbitrate! */
	_MTCTL(saved_eiem, CR_EIEM);

	return ibit;
}


cspinlock(lock)
	lock_t *lock;
{
	register int	locked;
	register int	*count;
	register u_int	ibit;
	register u_int	eiem;
	register u_int	iva;

	VASSERT((int)lock > 0x100);
#ifndef	SEMAPHORE_DEBUG
	if(uniprocessor){

		if (lock->sl_count == 0) {
#if	defined(OSDEBUG) && defined(__hp9000s800)
			_MFCTL(CR_EIEM, eiem);
			VASSERT(eiem == SPINLOCK_EIEM);
#endif	/* defined(OSDEBUG) && defined(__hp9000s800) */
			return(0);
		}

		DISABLE_INT(ibit);

		/* Now do the spinlock for processor 0 */
		lock->sl_count = 0;
		if (mpproc_info[MP_MONARCH_INDEX].spinlock_depth++ == 0) {
			_MFCTL(CR_EIEM, eiem);	/* save current spl/eiem */
			mpproc_info[MP_MONARCH_INDEX].entry_spl_level = eiem;
			eiem = SPINLOCK_EIEM;	
			_MTCTL(eiem, CR_EIEM);	
		}

		lock->sl_owner = (int)mpproc_info[MP_MONARCH_INDEX].iva;
		ENABLE_INT(ibit);
		return(1);
	}
#endif	/* ! SEMAPHORE_DEBUG */

	count = &lock->sl_count;
	DISABLE_INT(ibit);
	_LDCWS(0, 0, count, locked);
	if (locked) {
		struct	mpinfo	*mpi;

		mpi = getproc_info();
		if (mpi->spinlock_depth++ == 0) {
			_MFCTL(CR_EIEM, eiem);	/* save current spl/eiem */
			mpi->entry_spl_level = eiem;
			eiem = SPINLOCK_EIEM;	
			_MTCTL(eiem, CR_EIEM);	
		}
		_MFCTL(CR_IVA, iva);
		lock->sl_owner = iva;
		PREVPC(lock->sl_lock_caller, &cspinlock_dbg_unw);
		SPINLOCK_ADD(lock);
	}
	ENABLE_INT(ibit);
	return(locked);
}

#ifdef SEMAPHORE_DEBUG
spinunlock(lock)		
	lock_t *lock;
{

	VASSERT((int)lock > 0x100);

	SD_ASSERT( (lock > (lock_t *) 0x100), "spinunlock: bad lock pointer");
	SD_ASSERT( (lock == ((unsigned int)lock & -16)), 
		   "spinunlock: bad spinlock address. Unaligned");

#ifdef	SPINLOCK_DEADLOCK_CHECKING
	if(!bypass_spinlocking){
		SD_ASSERT( owns_spinlock(lock), 
			"Spinunlock of a lock we do not own");
	}else if(!owns_spinlock(lock))
		return;
#else	/* ! SPINLOCK_DEADLOCK_CHECKING */
	SD_ASSERT( owns_spinlock(lock), 
			"Spinunlock of a lock we do not own");

#endif	/* ! SPINLOCK_DEADLOCK_CHECKING */

#ifdef	DO_PREVPC
	if (lock->sl_unlock_caller & 0x1)
		lock->sl_unlock_caller &= ~0x1;	/* spinunlockx caller */
	else{
		PREVPC(lock->sl_unlock_caller, &spinunlock_dbg_unw);
	}
#endif	/* DO_PREVPC */


	SPINLOCK_DELETE(lock);


	if((spinlock_type & 2) == 0)
		asm_spinunlock(lock);
	else
		cc_spinunlock(lock);
}

cc_spinunlock(lock)
	lock_t *lock;
{
	register u_int	iva;
	register u_int	ibit;
	register u_int	eiem;
	struct mpinfo	*mpi;

	DISABLE_INT(ibit);

	lock->sl_owner = 0;

	mpi = getproc_info();
	if (--mpi->spinlock_depth == 0) {
		eiem = mpi->entry_spl_level;
		_MTCTL(eiem, CR_EIEM);
	}

	/* useful non-zero value for debugging */
	_MFCTL(CR_IVA, iva);
	lock->sl_count = iva;

	ENABLE_INT(ibit);
}

#endif /* SEMAPHORE_DEBUG */

#ifdef	SEMAPHORE_DEBUG
/*
 *  Acquire spinlock if we don't already own it
 */
spinlockx(lock, state)
	lock_t	*lock;
	sv_lock_t *state;
{

	if ( owns_spinlock(lock) )
		asm_spinlockx(lock,state);
	else {
		asm_spinlockx(lock,state);
		SPINLOCK_ADD(lock);
		PREVPC(lock->sl_lock_caller, &spinlockx_unw);
	}

}

/*
 *  Release spinlock if we are the ones that acquired it
 */
spinunlockx(lock, state)
	lock_t	*lock;
	sv_lock_t *state;
{
	if (! state->saved)
		asm_spinunlockx(lock,state);
	else {
#ifdef	DO_PREVPC
		PREVPC(lock->sl_unlock_caller, &spinunlockx_unw);
		lock->sl_unlock_caller |= 0x1;
#endif	/* DO_PREVPC */
		SPINLOCK_DELETE(lock);
		asm_spinunlockx(lock,state);
	}

}
#endif	/* SEMAPHORE_DEBUG */
#endif	/* __hp9000s800 */

#ifdef SPINLOCK_DEBUG
#ifdef	__hp9000s300
spinlock_dbg(lock)
	lock_t *lock;
{
	VASSERT((int)lock > 0x100);
	SD_ASSERT( ! owns_spinlock(lock), "Spinlock already acquired");
	lock_processor();
	DEADLOCK_CHECK(lock);
	lock->sl_count = 0;
	lock->sl_owner = GETMP_ID();
	getproc_info()->spinlock_depth++;
	PREVPC(lock->sl_lock_caller, &spinlock_dbg_unw);
	SPINLOCK_ADD(lock);
}
#endif	/* SPINLOCK_DEBUG */

#ifdef MP
cspinlock(lock)
	lock_t *lock;
{

	VASSERT((int)lock > 0x100);
	if (owns_spinlock(lock))
		return 0;
	lock_processor();
	/* Now do the spinlock for processor 0 */
	lock->sl_count = 0;
	lock->sl_owner = GETMP_ID();
	getproc_info()->spinlock_depth++;
	PREVPC(lock->sl_lock_caller, &cspinlock_dbg_unw);
	SPINLOCK_ADD(lock);
	return 1;
}
#endif /* MP*/

#ifdef SPINLOCK_DEBUG
spinunlock_dbg(lock)
	lock_t *lock;
{

	VASSERT((int)lock > 0x100);

#ifdef	SPINLOCK_DEADLOCK_CHECKING
	if(!bypass_spinlocking){
		SD_ASSERT( owns_spinlock(lock), 
			"Spinunlock of a lock we do not own");
	}else if(!owns_spinlock(lock))
		return;
#else	/* ! SPINLOCK_DEADLOCK_CHECKING */
	SD_ASSERT( owns_spinlock(lock), 
			"Spinunlock of a lock we do not own");

#endif	/* ! SPINLOCK_DEADLOCK_CHECKING */

#ifdef	DO_PREVPC
	if (lock->sl_unlock_caller & 0x1)
		lock->sl_unlock_caller &= ~0x1;	/* spinunlockx caller */
	else
		PREVPC(lock->sl_unlock_caller, &spinunlock_dbg_unw);
#endif	/* DO_PREVPC */

	SPINLOCK_DELETE(lock);
	getproc_info()->spinlock_depth--;
	lock->sl_owner = 0;
	lock->sl_count = GETMP_ID();	/* useful non-zero value for debug */
	unlock_processor(); 
}
#endif /* SPINLOCK_DEBUG */

#ifdef	MP
/*
 *  Acquire spinlock if we don't already own it
 */
spinlockx(lock, state)
	lock_t	*lock;
	sv_lock_t *state;
{
	if ( owns_spinlock(lock) ) {
		state->saved = (lock_t *)NULL;
	} else {
		state->saved = lock;
		spinlock(lock);
		PREVPC(lock->sl_lock_caller, &spinlockx_unw);
	}
}

/*
 *  Release spinlock if we are the ones that acquired it
 */
spinunlockx(lock, state)
	lock_t	*lock;
	sv_lock_t *state;
{
	if (state->saved) {
		state->saved = (lock_t *)NULL;
#ifdef	DO_PREVPC
		PREVPC(lock->sl_unlock_caller, &spinunlockx_unw);
		lock->sl_unlock_caller |= 0x1;
#endif	/* DO_PREVPC */
		spinunlock(lock);
	}
}
#endif	/* MP */
#endif	/* __hp9000s300 */

#ifdef NEVER_CALLED
spinlocked_dbg(lock)
	lock_t *lock;
{
	return (lock->sl_count == 0);
}

spinlocks_held_dbg()
{
	return (getproc_info()->spinlock_depth);
}
#endif /* NEVER_CALLED */

extern	lock_t	spin_alloc_base[];
extern	lock_t	*spin_alloc_end;
lock_t	*spin_alloc_next = spin_alloc_base;
#ifdef	SEMAPHORE_DEBUG
lock_t	*global_lock_list = NULL;
#endif

initlock(lock, order, name)
	lock_t *lock;
	char *name;
{
	lock->sl_count = 1;
	lock->sl_owner = 0;
#if	defined(MP) || defined(SEMAPHORE_DEBUG)
	lock->sl_order = order;
	lock->sl_name  = name;
#endif	/* defined(MP) || defined(SEMAPHORE_DEBUG) */

#ifdef	SEMAPHORE_DEBUG
	spinlock(&lock_init_lock);
	lock->sl_g_link = global_lock_list;
	global_lock_list = lock;
	spinunlock(&lock_init_lock);
#endif	/* SEMAPHORE_DEBUG */
}

lock_t *
alloc_spinlock(order, name)
	char *name;
{
	lock_t	*alloc; 
	register lock_t;

	/* one allocator at a time */
	spinlock(&lock_init_lock);
	alloc = spin_alloc_next;
	if (alloc == spin_alloc_end) {
		spinunlock(&lock_init_lock);
		panic("alloc_spinlock: not enough spinlocks pre allocated");
	}
	spin_alloc_next++;
	spinunlock(&lock_init_lock);

	/* initialize this lock */
	initlock(alloc, order, name);
	return(alloc);
}

int
init_spinlocks()
{

	/* Process management spinlocks */
	sched_lock = 
		alloc_spinlock(SCHED_LOCK_ORDER, "sched_lock");
	activeproc_lock = 
		alloc_spinlock(ACTIVEPROC_LOCK_ORDER, "activeproc_lock");
#ifdef	MP
	proc_lock = 
		alloc_spinlock(PROC_LOCK_ORDER, "proc_lock");
	callout_lock = 
		alloc_spinlock(CALLOUT_LOCK_ORDER, "callout_lock");
	cred_lock = 
		alloc_spinlock(CRED_LOCK_ORDER, "cred_lock");

	/* File system locks */
	file_table_lock = 
		alloc_spinlock(FILE_TABLE_LOCK_ORDER, "file_table_lock");
	devvp_lock = 
		alloc_spinlock(DEVVP_LOCK_ORDER, "devvp_lock");
	dnlc_lock = 
		alloc_spinlock(DEVVP_LOCK_ORDER, "dnlc_lock");
	biodone_lock = 
		alloc_spinlock(BIODONE_LOCK_ORDER, "biodone_lock");
	bbusy_lock = 
		alloc_spinlock(BBUSY_LOCK_ORDER, "bbusy_lock");
	v_count_lock = 
		alloc_spinlock(V_COUNT_LOCK_ORDER, "v_count_lock");
	buf_hash_lock = 
		alloc_spinlock(BUF_HASH_LOCK_ORDER, "buf_hash_lock");

	/* Networking spinlocks */
	netisr_lock = 
		alloc_spinlock(NETISR_LOCK_ORDER, "netisr_lock");
	ntimo_lock = 
		alloc_spinlock(NTIMO_LOCK_ORDER, "ntimo_lock");
	bsdskts_lock = 
		alloc_spinlock(BSDSKTS_LOCK_ORDER, "bsdskts_lock");

	semaphore_log_lock = 
		alloc_spinlock(SEMAPHORE_LOG_LOCK_ORDER, "semaphore_log_lock");
#ifdef OLD_CRASH_MONARCH
	crash_monarch_lock = 
		alloc_spinlock(CRASH_MONARCH_LOCK_ORDER, "crash_monarch_lock");
#endif OLD_CRASH_MONARCH
	ioserv_lock = 
		alloc_spinlock(IOSERV_LOCK_ORDER, "ioserv_lock");
	vmsys_lock = 
		alloc_spinlock(VMSYS_LOCK_ORDER, "Generic VM system spinlock");
	lpmc_log_lock =
		alloc_spinlock(LPMC_LOG_LOCK_ORDER, "lpmc_log_lock");
	itmr_sync_lock =
		alloc_spinlock(ITMR_SYNC_LOCK_ORDER, "itmr_sync_lock");
	pdce_proc_lock =
		alloc_spinlock(PDCE_PROC_LOCK_ORDER, "pdce_proc_lock");
	pfail_cntr_lock =
		alloc_spinlock(PFAIL_CNTR_LOCK_ORDER, "pfail_cntr_lock");
#endif	/* MP */
	/* Er, io_tree spinlock */
	io_tree_lock = 
		alloc_spinlock(IO_TREE_LOCK_ORDER, "io_tree_lock");
	clist_lock =
 		alloc_spinlock(CLIST_LOCK_ORDER, "clist_lock");

	/* 
	 *  Hash table of spinlocks for beta-class semaphores
	 */
	b_sema_htbl_init();		
}

#ifdef	SPINLOCK_DEADLOCK_CHECKING

int do_spin_deadlock = 1;
int found_spin_deadlock = 0;

#define offsetof(FIELD, STRUCT) ((caddr_t)(&(((STRUCT *)0)->FIELD)))
#define	SPIN_LINK_ANCHOR(SPIN_PTR)	\
	((lock_t *) ((caddr_t)&(SPIN_PTR) - \
	 offsetof(sl_link, lock_t)))


spinlock_add(lock)
	lock_t	*lock;
{
	struct	mpinfo	*mpi;
	lock_t	*held;
	lock_t	*l_held;	/* for list insertion */
	
	if ( ! do_spin_deadlock ) return;
	if ( found_spin_deadlock ) return;
	mpi = getproc_info();
	held = SPIN_LINK_ANCHOR(mpi->held_spinlock);
	do {
		l_held = held;
		held = l_held->sl_link;
	} while ( held && (held->sl_order > lock->sl_order));

	lock->sl_link = held;	/* point forward to lower order lock */
	l_held->sl_link = lock;	/* point at lock from anchor/higher order */
}

spinlock_delete(lock)
	lock_t	*lock;
{
	struct	mpinfo	*mpi;
	lock_t	*held;
	lock_t	*l_held;	/* for list deletion */
	
	if ( ! do_spin_deadlock ) return;
	if ( found_spin_deadlock ) return;
	mpi = getproc_info();
	held = SPIN_LINK_ANCHOR(mpi->held_spinlock);
	do {
		l_held = held;
		held = l_held->sl_link;
		if (held == lock) {
			l_held->sl_link = lock->sl_link;
			return;
		}
	} while ( held );
	panic("spinlock_delete: lock not on linked list");
}

deadlock_check(lock)
	lock_t	*lock;
{
	struct	mpinfo	*mpi;
	lock_t	*held;

	if ( ! do_spin_deadlock ) return;
	if ( found_spin_deadlock ) return;
	mpi = getproc_info();
	held = mpi->held_spinlock;
	if ( held && (held->sl_order >= lock->sl_order) ) {
		found_spin_deadlock = 1;
		bypass_spinlocking = 1;
		spin_deadlock_failure(lock, held);
	}
}

spin_deadlock_failure(lock, held)
	lock_t	*lock;
	lock_t	*held;
{
	printf("Trying to get spinlock %s when spinlock %s is held.\n",
		lock->sl_name, held->sl_name);
	panic("spin_deadlock_failure");
}

#endif	/* SPINLOCK_DEADLOCK_CHECKING */
