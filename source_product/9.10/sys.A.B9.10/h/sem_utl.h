/*
 * @(#)sem_utl.h: $Revision: 1.5.83.4 $ $Date: 93/09/17 18:34:13 $
 * $Locker:  $
 */

#ifndef	_SEM_UTL_INCLUDED
#define	_SEM_UTL_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/semglobal.h"
#include "../h/mp.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/semglobal.h>
#include <sys/mp.h>
#endif /* _KERNEL_BUILD */

#ifdef _KERNEL

#define	DO_SEMA_NOSWAP

/*
 * Semaphore types - Debug only
 */
#define	SEM_ALPHA	0x1
#define	SEM_BETA	0x2
#define	SEM_SYNC	0x3

#define	SEM_TRUE	1
#define SEM_FALSE	0
#define SEM_NULL	0

/*
 *   Function return values for semaphore routines.
 */
#define	SEMA_SUCCESS	1	/* semaphore operation succeeded.	*/
#define SEMA_FAILURE	0	/* semaphore operation failed.		*/
#define SEMA_SIGNAL_CAUGHT -1	/* P operation aborted by signal.	*/
/*
 *   Signal options:
 */
typedef int sigoption_t;

#define P_ALLOW_SIGNALS		0
#define P_DEFER_SIGNALS		1
#define P_CATCH_SIGNALS		2

/*
 * Contention breaker bit is the sign bit of an int. Assumes a byte is 8 bits.
 */
#define SEMA_CONTENTION	(1 << ((sizeof(int) * 8) -1))

/*
 * Deadlock safe bit is next least significant bit after the contention
 * breaker bit.
 */
#define SEMA_DEADLOCK_SAFE (1 << ((sizeof(int) * 8) -2))


#ifdef MP
#define CURPRI (((!getnoproc()) && (u.u_procp->p_flag & SRTPROC)) ? u.u_procp->p_pri : getcurpri())
#else /* !MP */
#define CURPRI (((!noproc) && (u.u_procp->p_flag & SRTPROC)) ? u.u_procp->p_pri : curpri)
#endif /* !MP */


#ifdef MP
struct mp_iva *int_owner;

#define	OWN_INTERRUPTS							\
{									\
int	old_spl;							\
	if (!ON_ICS && owns_sema(io_sema) && int_owner!=getiva()) {	\
			old_spl=spl7();					\
			int_owner = getiva();				\
			splx(old_spl);					\
	}								\
}				

#define	DISOWN_INTERRUPTS						\
	if (!ON_ICS && !owns_sema(io_sema) && int_owner == getiva()) {	\
		int_owner = 0;						\
	}
#endif /* MP */

#ifdef DO_SEMA_NOSWAP
/*
 * Semaphores and Swapping
 * -----------------------
 *  A process that a) owns an alpha-class semaphore, b) is waiting for an
 *  alpha-class semaphore is considered non-swappable.
 */

#define	SEMA_NOSWAP(p)	sema_noswap(p)
#define	SEMA_SWAP(p)	sema_swap(p)
void sema_noswap();
void sema_swap();

#else	/* ! DO_SEMA_NOSWAP */

#define	SEMA_NOSWAP(p)
#define	SEMA_SWAP(p)

#endif	/* ! DO_SEMA_NOSWAP */

#ifdef SEMAPHORE_DEBUG
/* 
 *	D E B U G    L O G S
 */

/*
 *   Semaphore operations, as recorded in the semaphore log.
 */
#define	SEMA_LOG_PSEMA		0x20
#define SEMA_LOG_CPSEMA		0x21
#define SEMA_LOG_VSEMA		0x28
#define SEMA_LOG_CVSEMA		0x29
#define SEMA_LOG_PSYNC		0x30
#define SEMA_LOG_VSYNC		0x38
#define SEMA_LOG_CVSYNC		0x39
#define SEMA_LOG_WSYNC		0x3a
#define SEMA_LOG_PCHANSYNC	0x40
#define SEMA_LOG_VCHANSYNC	0x48
#define SEMA_LOG_WSIGNAL	0x58
#define SEMA_LOG_PXSEMA		0x60
#define SEMA_LOG_VXSEMA		0x68
#define SEMA_LOG_FAILED_CP	1
#define SEMA_LOG_SLEEPY_PSEMA	2
#define SEMA_LOG_SLEEPY_PSYNC	3
#define SEMA_LOG_STOLEN_P	4
#define SEMA_LOG_VSEMA_WAKE	5

/*
 *   Semaphore log, used to record semaphore operations.
 *   Here for analyze.
 */
#define LOG_ENTRIES 1024
struct mp_sem_log {
	int	*caller_address;/* address of the caller.		*/
	int	caller_pid;	/* process ID of caller.		*/
	int	last_pid;	/* pid of last process to get semaphore.*/
	int	sema_function;	/* semaphore operation (see above).	*/
	int	sema_value;	/* value of semaphore on entry.		*/
	struct	sema	*semaphore;	/* address of semaphore.	*/
	int	max_waiters;	/* max processes waited on semaphore.	*/
	int	timestamp;	/* time of operation.			*/
	int	proctrace_index;
	int	procid;		/* for now this is the IVA of the proc	*/
} semaphore_log[LOG_ENTRIES];

struct mp_sem_log   *last_log_entry;	/* pointer to last entry in log.	*/
unsigned int log_index;		/* indexes the last entry made in log.	*/
unsigned int log_total;		/* total number of semaphore log calls	*/

#define SEMA_LOG(SEMA, FUNCTION, PREVPC)   sema_log(SEMA, FUNCTION, PREVPC)
#define	VERIFY_DEADLOCK_PROTOCOL(PROC,SEM) verify_deadlock_protocol(PROC,SEM)

#else	/* ! SEMAPHORE_DEBUG */

#define SEMA_LOG(SEMA, FUNCTION, PREVPC)
#define VERIFY_DEADLOCK_PROTOCOL(PROC,SEM)

#endif	/* ! SEMAPHORE_DEBUG */

#endif 	/* _KERNEL */

#endif	/* ! _SEM_UTL_INCLUDED */
