/*
 * @(#)kern_sem.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:27:44 $
 * $Locker:  $
 */

#ifndef	_KERN_SEM_INCLUDED
#define	_KERN_SEM_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/sem_alpha.h"
#include "../h/sem_beta.h"
#include "../h/sem_sync.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sem_alpha.h>
#include <sys/sem_beta.h>
#include <sys/sem_sync.h>
#endif /* _KERNEL_BUILD */

#endif	/* ! _KERN_SEM_INCLUDED */
