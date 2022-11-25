/*
 * @@(#)sema.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:34:18 $
 * $Locker:  $
 */

#ifndef _SEMA_INCLUDED 
#define _SEMA_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/kern_sem.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/kern_sem.h>
#endif /* _KERNEL_BUILD */

#endif /* _SEMA_INCLUDED */
