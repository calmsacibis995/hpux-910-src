/*
 * @(#)sem_sync.h: $Revision: 1.6.83.4 $ $Date: 93/09/17 18:34:07 $
 * $Locker:  $
 */

#ifndef	_SEM_SYNC_INCLUDED
#define	_SEM_SYNC_INCLUDED

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
 *   Sync Semaphore.
 *   NOTE!!! The sync_t structure must agree in field layout with
 *           the sema_t structure.
 */
typedef struct sync 
{
	lock_t	*s_lock;	/* lock for accessing semaphore struct.	*/
	int	count;		/* value of counting semaphore.		*/
	int	sa_fill1;	/* match sema_t				*/
	struct proc *wait_list;	/* head of procs waiting on sema	*/
	int	sa_fill2[3];	/* match sema_t				*/
	int	order;		/* deadlock protocol order for sema.	*/
} sync_t;

#ifdef	MP_runout_notdef
extern sync_t	runin;		/* Swapping synchronization */
extern sync_t	runout;		/* Swapping synchronization */
#endif	/* MP */

#endif	/* ! _SEM_SYNC_INCLUDED */
