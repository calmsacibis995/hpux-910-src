/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sem_beta.c,v $
 * $Revision: 1.8.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/15 10:41:53 $
 */

/*
 *   			KERNEL SEMAPHORES - BETA CLASS
 *
 *   File:
 *	sem_beta.c
 *
 *   FIELD			GUARDED BY
 *	p_bsema			internal logic (right!)
 *	b_sema->owner		semaphore itself
 *	b_sema->b_lock		beta_semaphore_lock
 *	bh_sema->fp		beta_semaphore_lock
 *	bh_sema->bp		beta_semaphore_lock
 *	p_sleep_sema		beta_semaphore_lock
 *	p_wait_list		beta_semaphore_lock
 *	p_rwait_list		beta_semaphore_lock
 *
 */	

#include "../h/assert.h"
#include "../h/types.h"
#include "../h/dir.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/sem_beta.h"
#include "../h/dbg.h"
#include "../h/debug.h"
#include "../machine/psl.h"
#ifdef __hp9000s800
#include "../machine/spl.h"
#include "../machine/reg.h"
#include "../machine/inline.h"
#endif /* __hp9000s800 */

/* #include "testsem.h" */

lock_t		*beta_semaphore_lock = (lock_t *)0;
bh_sema_t	b_sema_htbl[B_SEMA_HTBL_SIZE] = { 0 };



#define	B_SEMA_PRI(b_sema)		PRIBETA

#define	B_SEMA_HASH(X)	((((unsigned long)(X))>>6) & (B_SEMA_HTBL_SIZE-1))



#define	HASH_TO_BH_SEMAP(X)	(&b_sema_htbl[B_SEMA_HASH(X)])
#define	SEMA_WAIT_HEAD(H_SEMA_ADDR)	\
	((struct proc *) ((size_t)&((H_SEMA_ADDR)->fp) - \
	offsetof(p_wait_list, struct proc)))
#define offsetof(FIELD, STRUCT) ((size_t)(&(((STRUCT *)0)->FIELD)))

#define	WAKEUP(CHAN)	wakeup(CHAN)
#define	THISPROC(reg_temp) ((ON_ICS || GETNOPROC(reg_temp)) ? \
				 (struct proc *)-1 : u.u_procp )

#ifdef	SEMAPHORE_DEBUG
#define	DO_PREVPC
#define	static	/* Note:  dont try to declare static data in this file! */
b_sema_t	*global_bsema_list = SEM_NULL;
bh_sema_t	*global_bhsema_list = SEM_NULL;
int		do_deadlock_protocol = 1;

#define	B_VERIFY_DEADLOCK_PROTOCOL(PROC, B_SEMA, CALLER) 	\
			if (do_deadlock_protocol) 		\
				b_verify_deadlock_protocol(PROC, B_SEMA, CALLER)
#define B_SEMA_ADD(PROC, B_SEMA)	b_sema_add(PROC, B_SEMA)
#define	B_SEMA_DELETE(B_SEMA)		b_sema_delete(B_SEMA)
static	void	b_sema_add();
static	void	b_sema_delete();
#define B_CONVERT_USAV(context)
#else	/* ! SEMAPHORE_DEBUG */
#define	B_VERIFY_DEADLOCK_PROTOCOL(PROC, B_SEMA, CALLER)
#define B_SEMA_ADD(PROC, B_SEMA) (B_SEMA)->owner = PROC
#define	B_SEMA_DELETE(B_SEMA)	(B_SEMA)->owner = (struct proc *)SEM_NULL
#define B_CONVERT_USAV(context)	((void)convert_usav(context))
#endif	/* ! SEMAPHORE_DEBUG */

#ifdef	DO_PREVPC
#define	PREVPC(var, uwptr)	(do_beta_prevpc ? var = prevpc(uwptr) : 0 )
void	*prevpc();
int 	do_beta_prevpc = 1;
void	*b_psema_unw = 0;
void	*mp_b_psema_unw = 0;
void	*b_cpsema_unw = 0;
void	*mp_b_cpsema_unw = 0;
void	*b_vsema_unw = 0;
void	*mp_b_vsema_unw = 0;
void	*b_initsema_unw = 0;
#else	/* ! DO_PREVPC */
#define	PREVPC(var, uwptr)
#endif	/* ! DO_PREVPC */

void b_sema_sleep();
static void b_sema_wakeup();
#ifdef	MP
static void mp_b_sema_sleep();
static void mp_b_sema_wakeup();
#endif	/* MP */
static struct proc *b_sema_dequeue();


#ifdef	SEMAPHORE_DEBUG
/*
 *  b_psema	beta class p routine acquire with wait
 */
b_psema(b_sema)
	register b_sema_t *b_sema;
{
	extern int mainentered;	/* zero if main() not yet reached */
	register unsigned int	psw_state;
	register u_int	reg_temp;
	void		*caller;

	/* 
	 * cannot be on interrupt stack 
	 */
	SD_ASSERT(!ON_ICS || !mainentered, "b_psema called from ICS");  

	PREVPC(caller, &b_psema_unw);
	B_VERIFY_DEADLOCK_PROTOCOL(THISPROC(reg_temp), b_sema, caller);

	DISABLE_INT(psw_state);
	if (! (b_sema->b_lock & SEM_LOCKED)) 
	{
		b_sema->b_lock |= SEM_LOCKED;
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);		/* DEBUG */
	} else 
	{
		b_sema_sleep(b_sema);
	}
	ENABLE_INT(psw_state);

#ifdef	DO_PREVPC
	b_sema->b_pcaller = (u_int)caller;
#endif	/* DO_PREVPC */
}
/*
 *  b_psema	beta class p routine release
 */
b_vsema(b_sema)
	register b_sema_t *b_sema;
{
	struct proc	*p;
	sv_lock_t	nsp;
	register u_int	psw_state;
	register u_int  reg_temp;

	SD_ASSERT( b_sema->b_lock & SEM_LOCKED, "b_vsema: semaphore not locked");
	/*
	 *  If there is no owner, we can free it.
	 *  If we are are a process and we own this semaphore, then we
	 *  can free it.  
	 *  Otherwise we cant free it.
	 */
	SD_ASSERT( ((b_sema->owner == (struct proc *)-1) ||
		    ((!(GETNOPROC(reg_temp) || ON_ICS)) &&
				 (b_sema->owner == u.u_procp))), 
			   "b_vsema: we do not own semaphore");

	PREVPC(b_sema->b_vcaller, &b_vsema_unw);
	B_SEMA_DELETE(b_sema);				/* DEBUG */

	DISABLE_INT(psw_state);
	/* clear lock bit and test for want bit XXX */

	b_sema->b_lock &= ~SEM_LOCKED;
	if (b_sema->b_lock & SEM_WANT) 
	{
		ENABLE_INT(psw_state); /* One could argue this is not true beta */
		b_sema_wanted(b_sema);
	}
     /* ENABLE_INT(psw_state);  One could argue this is truer beta */
}

/*
 *  b_psema	beta class p routine acquire without wait
 */
b_cpsema(b_sema)
	register b_sema_t *b_sema;
{
	int		ret;
	register unsigned int	psw_state;
	register u_int	reg_temp;

	DISABLE_INT(psw_state);
	if (!(b_sema->b_lock & SEM_LOCKED)) {
		b_sema->b_lock |= SEM_LOCKED;
		ENABLE_INT(psw_state);
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);		/* DEBUG */
		PREVPC(b_sema->b_pcaller, &b_cpsema_unw);
		ret = 1;
	} else {
		ENABLE_INT(psw_state);
		ret = 0;
	}
	return(ret);
}
#endif /* SEMAPHORE_DEBUG */

#ifdef	MP
mp_b_psema(b_sema)
	register b_sema_t *b_sema;
{
	register u_int reg_temp;
	extern int mainentered;	/* zero if main() not yet reached */
	void		*caller;		/* XXX prevpc not finished MP */

	/* 
	 * cannot be on interrupt stack 
	 */
	SD_ASSERT(!ON_ICS || !mainentered, "b_psema called from ICS");  

	PREVPC(caller, &mp_b_psema_unw);
	B_VERIFY_DEADLOCK_PROTOCOL(THISPROC(reg_temp), b_sema, caller);

	SPINLOCK_USAV(beta_semaphore_lock, reg_temp);
	if (! (b_sema->b_lock & SEM_LOCKED)) {
		b_sema->b_lock |= SEM_LOCKED;
		SPINUNLOCK_USAV(beta_semaphore_lock, reg_temp);
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);		/* DEBUG */
	} else {
		mp_b_sema_locked(b_sema, reg_temp);
	}
#ifdef	DO_PREVPC
	b_sema->b_pcaller = (u_int)caller;
#endif	/* DO_PREVPC */
}

mp_b_sema_locked(b_sema, context)
	register b_sema_t *b_sema;
	register u_int     context;
{
	B_CONVERT_USAV(context);
	if ( !cspinlock(sched_lock) ) {
		spinunlock(beta_semaphore_lock);
		spinlock(sched_lock);
		spinlock(beta_semaphore_lock);
	}
	if ( b_sema->b_lock & SEM_LOCKED ) {
		/*
		 * mp_b_sema_sleep releases semaphore lock & sched_lock
		 */
		mp_b_sema_sleep(b_sema);
	} else {
		register u_int	reg_temp;
		/*
		 *  Although unlikely, during the above unlock/lock, 
		 *  the b_sema_t became free, so just take it.
		 */
		b_sema->b_lock |= SEM_LOCKED;
		spinunlock(beta_semaphore_lock);
		spinunlock(sched_lock);
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);	/* DEBUG */
	}
}
#endif	/* MP */

#ifdef	MP
mp_b_cpsema(b_sema)
	register b_sema_t *b_sema;
{
	int		ret;
	register u_int	reg_temp;

	SPINLOCK_USAV(beta_semaphore_lock, reg_temp);
	if (!(b_sema->b_lock & SEM_LOCKED)) {
		b_sema->b_lock |= SEM_LOCKED;
		SPINUNLOCK_USAV(beta_semaphore_lock, reg_temp);
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);		/* DEBUG */
		PREVPC(b_sema->b_pcaller, &mp_b_cpsema_unw);
		ret = 1;
	} else {
		SPINUNLOCK_USAV(beta_semaphore_lock, reg_temp);
		ret = 0;
	}
	return(ret);
}
#endif	/* MP */


b_sema_wanted(b_sema)
      register b_sema_t *b_sema;
{
	struct proc     *p;
	register unsigned int	psw_state;

	/* Check "again" if semiphore is wanted */
	DISABLE_INT(psw_state);
	if (b_sema->b_lock & SEM_WANT) 
	{
		b_sema->b_lock |= SEM_LOCKED;
		p = b_sema_dequeue(b_sema);
		if (p) {
			B_SEMA_ADD(p, b_sema);          /* DEBUG */
			b_sema_wakeup(p);
		} else {
			b_sema->b_lock &= ~SEM_LOCKED;
		}
	}
	ENABLE_INT(psw_state);
}


#ifdef	MP
mp_b_vsema(b_sema)
	register b_sema_t *b_sema;
{
	register u_int	reg_temp;

	SD_ASSERT( b_sema->b_lock & SEM_LOCKED, "b_vsema: semaphore not locked");
	/*
	 *  If there is no owner, we can free it.
	 *  If we are are a process and we own this semaphore, then we
	 *  can free it.  
	 *  Otherwise we cant free it.
	 */
	SD_ASSERT( ((b_sema->owner == (struct proc *)-1) ||
		    ((!(GETNOPROC(reg_temp) || ON_ICS)) &&
				 (b_sema->owner == u.u_procp))), 
			   "b_vsema: we do not own semaphore");

	PREVPC(b_sema->b_vcaller, &mp_b_vsema_unw);

	B_SEMA_DELETE(b_sema);				/* DEBUG */
	SPINLOCK_USAV(beta_semaphore_lock, reg_temp);
	if (!(b_sema->b_lock & SEM_WANT)) {
		b_sema->b_lock &= ~ SEM_LOCKED;
		SPINUNLOCK_USAV(beta_semaphore_lock, reg_temp);
	} else {
		mp_b_sema_wanted(b_sema, reg_temp);
	}
}

mp_b_sema_wanted(b_sema, reg_temp)
	register b_sema_t *b_sema;
	register u_int     reg_temp;
{
	struct proc	*p;
	sv_lock_t	nsp;

	B_CONVERT_USAV(reg_temp);
	/* XXX knows how SPINLOCKX works */
	if ( cspinlock(sched_lock) ) {		
		nsp.saved = sched_lock;
	} else {
		spinunlock(beta_semaphore_lock);
		spinlockx(sched_lock, &nsp);
		spinlock(beta_semaphore_lock);
	}
	SD_ASSERT( b_sema->b_lock & SEM_WANT, 
		   "b_vsema: someone other than owner freed the lock");
	p = b_sema_dequeue(b_sema);
	if (p) {
		spinunlock(beta_semaphore_lock);
		B_SEMA_ADD(p, b_sema);		/* DEBUG */
		mp_b_sema_wakeup(p);
		spinunlockx(sched_lock, &nsp);
	} else {
		b_sema->b_lock &= ~SEM_LOCKED;
		spinunlock(beta_semaphore_lock);
		spinunlockx(sched_lock, &nsp);
	}
}

#endif	/* MP */

b_disownsema(b_sema)
	register b_sema_t *b_sema;
{
	SD_ASSERT(b_valusema(b_sema) <= 0, 
		  "b_disownsema: semaphore not held");
	SD_ASSERT(b_sema->owner == (struct proc *)-1 || b_sema->owner == u.u_procp,
		  "b_disownsema: semaphore not owned by us");
	B_SEMA_DELETE(b_sema);				/* DEBUG */
	b_sema->owner = (struct proc *)-1;
}


#if defined(SEMAPHORE_DEBUG) || defined(OSDEBUG)
b_valusema(b_sema)
	register b_sema_t *b_sema;
{
	if (b_sema->b_lock & SEM_LOCKED)
		return(0);
	else
		return(1);
}
#endif /*  SEMAPHORE_DEBUG || OSDEBUG */

#ifdef NEVER_CALLED
/* 
 * BEGIN_DESC
 *
 * b_sema_waiting()
 *
 *
 * Input Parameters:    
 *	b_sema:	       beta class semaphore to determine if others are waiting 
 *		       for.
 *
 * Output Parameters:  None
 *
 * Return Value:       Zero if no one else is waiting. 
 *                     One if some other process is.
 *
 * Globals Referenced:
 *	beta_semaphore_lock
 *
 * Description:
 *      This routine determines if any other processes are waiting for the
 *	beta class semaphore, b_sema.  It does not check to see if any process
 *	has locked the semaphore and does not wish to do so.
 *
 *	Warning:  This routine only guarantees that no one is waiting at the
 *	time this routine is called.  Right after returning there is no reason
 *	a process may acquire the beta class semaphore another processor.
 *      It's up to the user to guarantee this.
 *
 * Algorithm:
 *      Initialize flag to zero.
 * 	Check to see if the wanted bit is cleared, if so then return 0, no one
 *		is waiting.
 *	Find where b_sema hashes to.
 *	Verify that we own the beta_semaphore_lock.
 *	For each process on this hash list
 *	      If the semaphore it is waiting on is b_sema, set flag and break
 *      Return value of flag.
 *	
 * In/Out conditions: None
 *	
 * END_DESC
 */

b_sema_waiting(b_sema)
	b_sema_t *b_sema;
{
	struct proc *p;
	int process_waiting = 0;
	bh_sema_t *h_sema;

	spinlock(beta_semaphore_lock);	
			
	/* If the wanted bit is not set then there is no need to search the 
	 * the semaphore hash list. 
	 */
	if (!(b_sema->b_lock & SEM_WANT)) {
		spinunlock(beta_semaphore_lock);
		return(0);
	}

	h_sema = HASH_TO_BH_SEMAP(b_sema);
	for (p = h_sema->fp; 
	     p != SEMA_WAIT_HEAD(h_sema); 
	     p = p->p_wait_list) {
		if ((b_sema_t *)(p->p_sleep_sema) == b_sema) {
			process_waiting = 1;
			break;
		}
	}
	spinunlock(beta_semaphore_lock);
	return(process_waiting);
}
#endif /* NEVER_CALLED */

#ifdef	SEMAPHORE_DEBUG
int	bh_semas_initialized = 0;
#endif	/* SEMAPHORE_DEBUG */

b_initsema(b_sema, val, order, name)
	register b_sema_t *b_sema;
	register int val;
	register int order;
	char	*name;
{
	register u_int reg_temp;
	SD_ASSERT(bh_semas_initialized, 
		  "b_initsema: called before b_sema_htbl_init()");
#ifdef	SEMAPHORE_DEBUG
	bzero(b_sema, sizeof(b_sema_t));
	b_sema->b_type = SEM_BETA;
	b_sema->name = name;
	b_sema->next = SEM_NULL;	/* next semaphore in per-process list */
	b_sema->prev = SEM_NULL;	/* prev semaphore in per-process list */
	b_sema->b_pcaller = b_sema->b_vcaller = SEM_NULL;
#endif	/* SEMAPHORE_DEBUG */
	b_sema->order = order;

	if (val <= 0) {
		b_sema->b_lock = SEM_LOCKED;
		B_SEMA_ADD(THISPROC(reg_temp), b_sema);	/* DEBUG */
		PREVPC(b_sema->b_pcaller, &b_initsema_unw);
	} else {
		b_sema->b_lock = 0;
		b_sema->owner = (struct proc *)SEM_NULL;
	}
#ifdef	SEMAPHORE_DEBUG

#ifdef	notdef			/* beta class semaphores are dynamic */
	/*
	 *  Link this semaphore on a global list of semaphores
	 */
	spinlock(&lock_init_lock);
	b_sema->link = global_bsema_list;
	global_bsema_list = b_sema;
	spinunlock(&lock_init_lock);
#endif	/* notdef */		/* beta class semaphores are dynamic */

#endif	/* SEMAPHORE_DEBUG */
}

#ifdef SEMAPHORE_DEBUG
/*
 *  This routine is here to balance b_initsema.  Because beta semaphores
 *  sometimes live in dynamically allocated memory, we need a hook to
 *  unlink them from the debug list of all the beta semaphores in the
 *  system.  Currently the code to link them up is ifdefd out in
 *  b_initsema, so we dont need to do anything here.
 */
b_termsema(b_sema)
	b_sema_t *b_sema;
{
	SD_ASSERT( (b_valusema(b_sema) > 0), "b_termsema: sema still locked");
}
#endif	/* SEMAPHORE_DEBUG */

void
b_sema_htbl_init()
{
	int		i;
	bh_sema_t	*h_sema;

#ifdef	SEMAPHORE_DEBUG
	bh_semas_initialized = 1;
#endif	/* SEMAPHORE_DEBUG */
	beta_semaphore_lock = alloc_spinlock(SEMAPHORE_LOCK_ORDER,
					     "beta semaphore spinlock");
	for (i = 0; i < B_SEMA_HTBL_SIZE; i++) {
		h_sema = b_sema_htbl + i;

		h_sema->fp = SEMA_WAIT_HEAD(h_sema);
		h_sema->bp = SEMA_WAIT_HEAD(h_sema);

#ifdef	SEMAPHORE_DEBUG
		/*
		 *  Link this semaphore on a global list of semaphores
		 */
		spinlock(&lock_init_lock);
		h_sema->link = global_bhsema_list;
		global_bhsema_list = h_sema;
		spinunlock(&lock_init_lock);
#endif	/* SEMAPHORE_DEBUG */

	}
}

#ifdef SEMA_COUNTING
int b_sema_sleepened;
int b_sema_wakened;
int b_sema_runned;
#endif /* SEMA_COUNTING */

/*
 *   b_sema_sleep()
 *
 */
void
b_sema_sleep(b_sema)
	b_sema_t		*b_sema;
{
	struct proc	*p;
	register unsigned int	psw_state;
	register unsigned int	trash;

	/* Check "again" if semiphore is available */
	DISABLE_INT(psw_state);
	if (! (b_sema->b_lock & SEM_LOCKED)) 
	{
		b_sema->b_lock |= SEM_LOCKED;
	} else 
	{
		/* Goto sleep waiting for it to unlock */
		/* SD_ASSERT(PROCESSOR_LOCKED(), "b_sema_sleep: processor !locked"); */
		p = u.u_procp;

		SD_ASSERT( b_sema->owner != p, 
		   	"b_sema_sleep: blocking on a semaphore we already own");
		raise_kernel_pri(p, B_SEMA_PRI(b_sema));
		b_sema_enqueue(b_sema, p);

		p->p_flag &= ~SSIGABL;		/* dont wake for signals */
		SD_ASSERT( p->p_stat != SSTOP, 
		   	"b_sema_sleep: SSTOP is an invalid state");
		p->p_stat = SSLEEP;		/* say were asleep */
		u.u_ru.ru_nvcsw++;
		swtch();

		/* This is in case next line is not atomic with interrupts */
		DISABLE_INT(trash);
		/* This must be an atomic operation -- to interrupted */
		p->p_flag |= SSIGABL;
	}
	ENABLE_INT(psw_state);
}

#ifdef	MP
/*
 *   mp_b_sema_sleep()
 *
 */
static void
mp_b_sema_sleep(b_sema)
	b_sema_t	*b_sema;
{
	struct proc	*p;
	sv_sema_t ss;

	T_SPINLOCK(sched_lock);
	T_SPINLOCK(beta_semaphore_lock);

	p = u.u_procp;

	SD_ASSERT( b_sema->owner != p, 
		   "b_sema_sleep: blocking on a semaphore we already own");
	raise_kernel_pri(p, B_SEMA_PRI(b_sema));
	b_sema_enqueue(b_sema, p);
	spinunlock(beta_semaphore_lock);

	release_semas(&ss);		/* all alpha class semas released */


	p->p_flag &= ~SSIGABL;		/* dont wake for signals */
	SD_ASSERT( p->p_stat != SSTOP, 
		   "b_sema_sleep: SSTOP is an invalid state");
	p->p_stat = SSLEEP;		/* say were asleep */
	u.u_ru.ru_nvcsw++;
	swtch();	/* releases sched_lock for MP case */
	spinlock(sched_lock);
	p->p_flag |= SSIGABL;
	spinunlock(sched_lock);

	reaquire_semas(&ss);		/* get back our alphas */
}
#endif	/* MP */

/*
 *   b_sema_wakeup()
 *
 */
static void
b_sema_wakeup(p)
	struct proc	*p;
{
	register unsigned int	eiem;

	SD_ASSERT( p->p_stat != SRUN, "b_sema_wakeup: invalid process state");
#ifdef	__hp9000s800
	/*
	 *  We dont want even the up_spl7 overhead in the normal b_vsema
	 *  case, so we just flip the PSW_I bit by saying DISABLE_INT...
	 *  But now we are going to call routines which will use spls,
	 *  so we need to set up our EIEM so that the splx(s) will work
	 *
	 *  For the s300 machines DISABLE_INT works with the spl routines.
	 */
	eiem = up_splx(SPL7);
#endif	/* __hp9000s800 */
	p->p_stat = SRUN;			/* say were RUNable */
	if ( p->p_flag & SLOAD ) {
		setrq(p);
		runrun++;
		aston();
	} else {
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
		if ( runout != 0 ) {
			runout = 0;
			WAKEUP((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;
	}
#ifdef	__hp9000s800
	/*
	 *  We now need to put our EIEM back the way it was before we
	 *  were called.
	 */
#ifdef	SEMAPHORE_DEBUG
	{
		register unsigned int	psw_state;

		_RSM(0, psw_state);
		SD_ASSERT( (psw_state & PSW_I) == 0,
				"b_sema_wakeup: interrupts enabled on return");
	}
#endif	/* SEMAPHORE_DEBUG */
	_MTCTL(eiem, EIEM);
#endif	/* __hp9000s800 */
}

#ifdef	MP
/*
 *   mp_b_sema_wakeup()
 *
 */
static void
mp_b_sema_wakeup(p)
	struct proc	*p;
{
	T_SPINLOCK(sched_lock);

	SD_ASSERT( p->p_stat != SRUN, "b_sema_wakeup: invalid process state");
	p->p_stat = SRUN;			/* say were RUNable */
	if ( p->p_flag & SLOAD ) {
		setrq(p);
		runrun++;
		aston();
	} else {
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
		if ( runout != 0 ) {
			runout = 0;
			WAKEUP((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;
	}

	T_SPINLOCK(sched_lock);
}
#endif	/* MP */

/*
 *   b_sema_enqueue()
 *
 *	Enqueue a process at the end of a semaphores waiting list.
 */
static
b_sema_enqueue(b_sema, p)
	b_sema_t *b_sema;
	struct proc *p;
{
	struct proc *last;
	bh_sema_t *h_sema = HASH_TO_BH_SEMAP(b_sema);

	/* UP_SD_ASSERT(PROCESSOR_LOCKED(), "processor !locked"); */
	T_SPINLOCK(beta_semaphore_lock);
#ifdef SEMA_COUNTING
	b_sema_sleepened++;
#endif /* SEMA_COUNTING */
	/*
	 *  insert at tail, after last
	 */
	last = h_sema->bp;
	p->p_wait_list = last->p_wait_list;
	p->p_rwait_list = last;
	last->p_wait_list->p_rwait_list = p;
	last->p_wait_list = p;

	b_sema->b_lock |= SEM_WANT;
	p->p_sleep_sema = (sema_t *)b_sema;
}
	
/*
 *   b_sema_dequeue()
 *
 *	Dequeue the first process from a semaphores waiting list.
 *	
 *	Returns a pointer to the dequeued process, or NULL if there were none.
 */
static struct proc *
b_sema_dequeue(b_sema)
	b_sema_t *b_sema;	/* actual semaphore */
{
	struct proc *p, *marker;
	bh_sema_t *h_sema = HASH_TO_BH_SEMAP(b_sema);

	/* UP_SD_ASSERT(PROCESSOR_LOCKED(), "processor !locked"); */
	T_SPINLOCK(beta_semaphore_lock);

	for (p = h_sema->fp; p != SEMA_WAIT_HEAD(h_sema); p = p->p_wait_list) {
		if ((b_sema_t *)(p->p_sleep_sema) == b_sema) {
#ifdef SEMA_COUNTING
			b_sema_wakened++;
#endif /* SEMA_COUNTING */
			p->p_sleep_sema = SEM_NULL;
			/* 
			 *  remove from wait list
			 */
			marker = p->p_rwait_list;
			marker->p_wait_list = p->p_wait_list;
			marker->p_wait_list->p_rwait_list = marker;
			p->p_wait_list = SEM_NULL;
			p->p_rwait_list = SEM_NULL;
			return(p);
		}
	}
	b_sema->b_lock &= ~SEM_WANT;
	return(SEM_NULL);
}

#ifdef	SEMAPHORE_DEBUG
/*
 *   b_sema_add
 *
 *	Add a reference to a newly acquired semaphore into a processs
 *	proc structure for future reference.
 *
 */
static void
b_sema_add(proc, sema)
	struct proc	*proc;
	b_sema_t	*sema;
{
	sema->owner = proc;
	SD_ASSERT(sema->next == SEM_NULL, "b_sema_add: sema->next != SEM_NULL");
	SD_ASSERT(sema->prev == SEM_NULL, "b_sema_add: sema->prev != SEM_NULL");
	if (proc == (struct proc *)-1)
		return;
	if ((sema->next = proc->p_bsema) != SEM_NULL) {
		SD_ASSERT(proc->p_bsema->prev == SEM_NULL,
			"b_sema_add: proc->p_bsema->prev != SEM_NULL");
		proc->p_bsema->prev = sema;
	}
	proc->p_bsema = sema;
}


/*
 *   b_sema_delete
 *
 *	Remove a reference to a semaphore from a processs proc structure.
 */
static void
b_sema_delete(sema)
	b_sema_t	*sema;
{
	struct proc	*proc;
	extern int	mainentered;

	proc = sema->owner;
	sema->owner = SEM_NULL;

	SD_ASSERT( (!ON_ICS || proc == (struct proc *)-1),
		   "b_sema_delete: ON_ICS, and sema->owner exists!");
	if (proc == (struct proc *)-1)
		return;

	SD_ASSERT( proc == u.u_procp, 
		   "b_sema_delete: process does not own sema");

	if (proc->p_bsema == sema) {
		SD_ASSERT(sema->prev == SEM_NULL,
			"sema_delete: sema->prev != SEM_NULL");
		if ((proc->p_bsema = sema->next) != SEM_NULL)
			proc->p_bsema->prev = SEM_NULL;
	} else {
		SD_ASSERT(sema->prev != SEM_NULL,
			"sema_delete: sema->prev == SEM_NULL");
		if ((sema->prev->next = sema->next) != SEM_NULL)
			sema->next->prev = sema->prev;
		sema->prev = SEM_NULL;
	}
	sema->next = SEM_NULL;

	sema->last_proc = proc;
}

/*
 *   b_verify_deadlock_protocol
 *	
 *	Verifies that the deadlock protocols are being observed.  Causes
 *	a panic if acquiring "semaphore" could potentially cause a deadlock.
 */
static
b_verify_deadlock_protocol(process, semaphore, caller)
	struct proc 	*process;
	b_sema_t 	*semaphore;
	void		*caller;
{
	b_sema_t *sema;

	if (process == (struct proc *)-1)
		return;

	for (sema = process->p_bsema; sema != SEM_NULL; sema = sema->next) {
		if (semaphore->order < sema->order)
			b_deadlock_error(process, semaphore, sema,
				"b_deadlock order violation", caller);
	}
}

int 	panic_when_beta_deadlock = 0;

/*
 *   b_deadlock_error
 *
 *	Prints out useful diagnostic information before panicing when a
 *	potential deadlock condition exists.
 */
static
b_deadlock_error(process, semaphore, held_semaphore, message, caller)
	struct proc 	*process;
	b_sema_t 	*semaphore;
	b_sema_t 	*held_semaphore;
	char 		*message;
	void		*caller;
{
	b_sema_t *sema;

	if (! panic_when_beta_deadlock) {
		beta_deadlock_trace(process, semaphore, held_semaphore, caller);
		return;
	}
	printf("Semas owned(sema,order):\n");
	for (sema = process->p_bsema; sema != SEM_NULL; sema = sema->next)
		printf("   %x  %d\n", 
			sema,
			sema->order);
	printf("(sema,order)to be added:\n");
	printf("   %x  %d\n",
		semaphore,
		semaphore->order);
	panic(message);
}

struct beta_deadlock_elem {
	short	held_order, new_order;
	void	*held_caller, *new_caller;
	int	pad;
};

#define	BETA_DEADLOCK_TRACE_NUM	512	/* power of 2 */
struct beta_deadlock_elem beta_deadlock_buf[BETA_DEADLOCK_TRACE_NUM] = 0;

struct	debug_buf_info {
	unsigned	index;		/* index, non-modulo */
	void		*array;		/* pointer to array */
	unsigned	num;		/* number of elements */
	unsigned	size;		/* size of each element */
};

struct	debug_buf_info beta_deadlock_trace_info = {
	-1,
	beta_deadlock_buf,
	BETA_DEADLOCK_TRACE_NUM,
	sizeof(struct beta_deadlock_elem)
};

int	beta_trace_lookback = 2;

/*
 *  XXX this has races in the log for MP
 */
int
beta_deadlock_trace(process, new_semaphore, held_semaphore, caller)
	struct proc	*process;
	b_sema_t	*new_semaphore;
	b_sema_t	*held_semaphore;
	void		*caller;
{
	unsigned			index, cnt;
	struct beta_deadlock_elem	*ptr, *pptr;
	struct debug_buf_info		*di;

	di = &beta_deadlock_trace_info;

	index = ((di->index+1) & (di->num - 1));
	ptr = ((struct beta_deadlock_elem *)di->array) + index;
	ptr->held_order = held_semaphore->order;
	ptr->new_order = new_semaphore->order;
	ptr->held_caller = (void *)held_semaphore->b_pcaller;
	ptr->new_caller = caller;

	cnt = beta_trace_lookback;
	if (cnt > index)
		cnt = index;	/* dont worry about wrap, just skip it */
	for (pptr = ptr - 1; cnt > 0; pptr--, cnt--) {
		if (bcmp(ptr, pptr, sizeof(struct beta_deadlock_elem)) == 0)
			return;	/* dont add to list */
	}
	di->index++;
}
#endif	/* SEMAPHORE_DEBUG */

#ifdef FDDI_VM
/* 
 * BEGIN_DESC
 *
 * pf_lock_switch()
 *		  
 *		  
 *
 * Input Parameters:    
 *	pfd1:	One of the two pfds to update semaphore information from.
 *	pfd2:	The other one.
 *
 * Output Parameters:  None
 *
 * Return Value:       None
 *
 * Globals Referenced:
 *
 * Description:
 *	This routine is part of the FDDI vm code for copy avoidance.
 *	One of the methods used is switching two physical pages instead
 *	copying one to the other.  This requires switching some of the
 *	data structures associated with the two pages.  This routine will
 *	switch the pf_lock structures from the two pfdat entries.
 *
 * Algorithm:
 *	Save the pf_lock structure from the first pfdat entry.
 *	Copy the second structure into the first's.
 *	Copy the saved structure into the second's.
 *
 * In/Out conditions:
 *	It is assumed that the non-SEMAPHORE_DEBUG fields in both of the 
 *	lock's structure are the same.  Therefore, if SEMAPHORE_DEBUG
 *      is not defined, no data switch is necessary.  If SEMAPHORE_DEBUG is
 *      on, we copy the complete structure even though we are only concerned
 *      with the debug fields.
 *	If a non-SEMAPHORE_DEBUG field can differ, then a field by field
 *	switch will need to be done.
 *
 * END_DESC
 */

void
pf_lock_switch(pfdlock1, pfdlock2)
	vm_sema_t *pfdlock1;
	vm_sema_t *pfdlock2;
{
	/*
	 * Assert that neither of the pfdat semaphores have an owner.
	 */
	VASSERT(pfdlock1->owner == (struct proc *)-1);
	VASSERT(pfdlock2->owner == (struct proc *)-1);

	/*
	 * Assert that both are locked.
	 */
	VASSERT(vm_valusema(pfdlock1) == 0);
	VASSERT(vm_valusema(pfdlock2) == 0);
	
	/*
	 * Assert that the order fields are the same.
	 */
	VASSERT(pfdlock1->order == pfdlock2->order);
	
	/*
	 * If additional non-SEMAPHORE_DEBUG fields are ever added, and
	 * they need to be switched, then code must be added here to do
	 * the switch.
	 */

#ifdef SEMAPHORE_DEBUG
	{
	       vm_sema_t	save_lock;
	       /*
		* Exchange the data from the two structures.
		*/
	       save_lock = *pfdlock1;
	       *pfdlock1 = *pfdlock2;
	       *pfdlock2 = save_lock;
	       
	       /*
		* If non-SEMAPHORE_DEBUG fields can ever differ for the two
		* pfdat entries, and if they should *not* be switched, then 
		* all but those fields will need to be switched individually
		* here instead of the copy done above.
		*/
       }
#endif /* SEMAPHORE_DEBUG */

}

/* 
 * BEGIN_DESC
 *
 * processes_waiting_for_sema()
 *
 *
 * Input Parameters:    
 *	b_sema:	       beta class semaphore to determine if others are waiting for.
 *
 * Output Parameters:  None
 *
 * Return Value:       Zero if no one else is waiting. 
 *                     One if some other process is.
 *
 * Globals Referenced:
 *	beta_semaphore_lock
 *
 * Description:
 *      This routine determines if any other processes are waiting for the
 *	beta class semaphore, b_sema.
 *
 * Algorithm:
 *      Initialize flag to zero.
 *	Find where b_sema hashes to.
 *	Verify that we own the beta_semaphore_lock.
 *	For each process on this hash list
 *	      If the semaphore it is waiting on is b_sema, set flag and break
 *      Return value of flag.
 *	
 * In/Out conditions: None
 *	
 * END_DESC
 */

#ifdef OSDEBUG

processes_waiting_for_sema(b_sema)
	b_sema_t *b_sema;
{
	struct proc *p;
	int process_waiting = 0;
	bh_sema_t *h_sema = HASH_TO_BH_SEMAP(b_sema);

	/* UP_SD_ASSERT(PROCESSOR_LOCKED(), "processor !locked"); */
	spinlock(beta_semaphore_lock);	
			
	for (p = h_sema->fp; 
	     p != SEMA_WAIT_HEAD(h_sema); 
	     p = p->p_wait_list) {
		if ((b_sema_t *)(p->p_sleep_sema) == b_sema) {
			process_waiting = 1;
			break;
		}
	}
	spinunlock(beta_semaphore_lock);
	return(process_waiting);
}

#endif /* OSDEBUG */
#endif /* FDDI_VM */
