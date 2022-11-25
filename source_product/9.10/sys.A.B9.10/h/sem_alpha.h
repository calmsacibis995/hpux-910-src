/*
 * @(#)sem_alpha.h: $Revision: 1.7.83.5 $ $Date: 94/04/21 14:09:42 $
 * $Locker:  $
 */

#ifndef	_SEM_ALPHA_INCLUDED
#define	_SEM_ALPHA_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/semglobal.h"
#include "../h/sem_utl.h"
#include "../h/spinlock.h"
#include "../h/mp.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/semglobal.h>
#include <sys/sem_utl.h>
#include <sys/spinlock.h>
#include <sys/mp.h>
#endif /* _KERNEL_BUILD */

/*
 *   Semaphore structure.
 *   NOTE!!! The sync_t structure must agree in field layout with
 *           the sema_t structure.
 *   Be sure to keep the 'hot' members at the front of the structure
 *   packed into one cacheline. Always align sema_t's on cachelines.
 */
typedef struct sema
{
	lock_t	*sa_lock;	/* lock for accessing semaphore struct.	*/
	int	sa_count;	/* value of counting semaphore.		*/
	struct proc *sa_owner;	/* process which owns this semaphore.	*/
	struct proc *sa_wait_list; /* head of procs waiting on sema     */
	struct sema *sa_prev;	/* prev semaphore in per-process list	*/
	struct sema *sa_next;	/* next semaphore in per-process list	*/
	int	sa_priority;	/* priority of a mutual exclusion sema.	*/
#ifdef	SEMAPHORE_DEBUG
	unsigned	char	sa_type;
#endif	/* SEMAPHORE_DEBUG */
	int	sa_order;	/* deadlock protocol order for sema.	*/
#ifdef	SEMAPHORE_DEBUG
	int	sa_last_pid;	/* I.D. of last proc to get semaphore.	*/
	int	sa_max_waiters;	/* maximum length of the wait list.	*/
	int	sa_p_count;	/* total number of P operations.	*/
	int	sa_sleepy_p_count;/* number of P's requiring sleep.	*/
	char	sa_contention;	/* TRUE if sema is contention breaker.	*/
	char	sa_deadlock_safe; /*TRUE if sema class safe from deadlock*/
	struct sema *sa_link;	/* link of all semaphores in system	*/
	u_int	sa_pcaller;	/* rp of last thread to do p operation	*/
	u_int	sa_vcaller;	/* rp of last thread to do v operation	*/
#endif	/* SEMAPHORE_DEBUG */
#ifdef SEMA_COUNTING
	u_int	sa_count_enqueue;	/* Number of times enqueued */
	u_int	sa_count_dequeue;	/* Number of times dequeued */
	u_int	sa_count_ticks;		/* (enqueue-dequeue) * (gprof_ticks) */
	u_int	sa_pcaller;		/* spare */
#endif /* SEMA_COUNTING */
} sema_t;

#ifdef _KERNEL
/*
 * Generic semaphore save structure
 * Used for stuff like pxsema,vxsema and in sleep()
 */

#define	MAX_SEMAS	10     /* maximum number of mutual exclusion
				* semaphores that can be simultaneously
				* owned by a process.  
				*/
typedef	struct {
	int nsaved;
	struct sv_sema_saved {
		sema_t	*sema;
		int	s_ks_reacquire;
	}	saved[MAX_SEMAS];
	int info;
} sv_sema_t;

/*
 *   Semaphores: extern declarations.
 */
extern sema_t	vmsys_sema;	/* VM empire semaphore */
extern sema_t	pm_sema;	/* Process management empire semaphore */
extern sema_t	io_sema;  	/* IO empire semaphore.	*/
extern sema_t	filesys_sema;	/* File system empire semaphore */
extern sema_t	sysVsem_sema;	/* System V (user-level) semaphore protection*/
extern sema_t	sysVmsg_sema;	/* System V (user-level) messages protection*/


extern int	do_semaphoring;	/* Don't do semaphoring during boot-up	*/

/*
 *  Macro definitions
 */
#ifdef	MP

#define	PSEMA(sema)	(!uniprocessor ? psema(sema):0)
#define	VSEMA(sema)	(!uniprocessor ? vsema(sema):0)
#define	PXSEMA(sema,ss)	(!uniprocessor ? pxsema(sema,ss):0)
#define	VXSEMA(sema,ss)	(!uniprocessor ? vxsema(sema,ss):0)
#define OWN_NET_SEMA		(owns_sema(&io_sema))

#else	/* ! MP */
/* only so we can compile kernels w/out MP defined */

#define	PSEMA(sema)
#define	VSEMA(sema)
#define	PXSEMA(sema,ss)
#define	VXSEMA(sema,ss)
#define OWN_NET_SEMA		(1)

#endif	/* ! MP */

/* Macros for Networking */
#define NET_PXSEMA(ss)		PXSEMA(&io_sema, ss)
#define NET_VXSEMA(ss)		VXSEMA(&io_sema, ss)
#define NET_PSEMA()		PSEMA(&io_sema)
#define NET_VSEMA()		VSEMA(&io_sema)

/* Macros for VM system */
#ifdef	MP_VM

#define vmemp_lock()	PSEMA(&vmsys_sema)
#define vmemp_unlock()	VSEMA(&vmsys_sema)
#define vmemp_lockx()	PXSEMA(&vmsys_sema, &sema_state.sema)
#define vmemp_unlockx()	VXSEMA(&vmsys_sema, &sema_state.sema)
#define vmemp_returnx(X)	{ \
					vmemp_unlockx(); \
					return X ; \
				}
#define vmemp_return(X)		{ \
					vmemp_unlock(); \
					return X ; \
				}

#define vm_sema_state	union { \
				sv_sema_t sema;	\
				sv_lock_t lock; \
			} sema_state

#else	/* ! MP_VM */

#define vmemp_lock()
#define vmemp_unlock()
#define vmemp_lockx()
#define vmemp_unlockx()
#define vmemp_returnx(X)	return X
#define vmemp_return()		return

#define vm_sema_state	extern void vm_sema_state_rtn() /* dummy */

#endif	/* ! MP_VM */

/*  Macros for I/O system */
#ifdef MP
#define	p_io_sema(save)							\
	{SD_ASSERT(! ON_ICS, "IO psema on ICS");			\
	 PXSEMA(&io_sema, save);}

#define	v_io_sema(save)							\
	{SD_ASSERT(! ON_ICS, "IO vsema on ICS");			\
	 VXSEMA(&io_sema, save);}
#else	/* not MP */
#define	p_io_sema(save)	
#define	v_io_sema(save)
#endif  /* MP */


#define cvsema(semaphore)	cvsema_dbg(semaphore)

#if	defined(SEMAPHORE_DEBUG) || defined(__hp9000s300)
#define psema(semaphore)	psema_dbg(semaphore)
#define vsema(semaphore)	vsema_dbg(semaphore)
#define cpsema(semaphore)	cpsema_dbg(semaphore)
#endif	/* SEMAPHORE_DEBUG || __hp9000s300 */

#ifndef	SEMAPHORE_DEBUG
#ifdef	OPTIMIZED_PV
#define psema(sema) 				\
	{					\
		/* spinlock((sema)->lock); */	\
		lock_sema();			\
		if (--(sema)->count < 0) {	\
			sema_locked();		\
			psema_fail(sema);	\
		} else {				\
			(sema)->owner = udot.u_procp;	\
			/* spinunlock((sema)->lock); */\
			unlock_sema();		\
		}				\
	}

#define cpsema(sema) \
	(spinlock((sema)->lock), --(sema)->count < 0  ? \
		(++(sema)->count, spinunlock((sema)->lock), SEMA_FAILURE) : \
		(spinunlock((sema)->lock), SEMA_SUCCESS)            )

#define vsema(sema)				\
	{					\
		/* spinlock((sema)->lock); */	\
		lock_sema();			\
		(sema)->owner = 0;		\
		if ((sema)->count++ < 0) {	\
			sema_locked();		\
			vsema_wake(sema);	\
		} else {			\
			/* spinunlock((sema)->lock); */  \
			unlock_sema();		\
		}				\
	}


#define cvsema(semaphore)	(SEMA_FAILURE)

#endif	/* OPTIMIZED_PV */

#endif	/* ! SEMAPHORE_DEBUG */

#endif /* _KERNEL */

#endif	/* ! _SEM_ALPHA_INCLUDED */
