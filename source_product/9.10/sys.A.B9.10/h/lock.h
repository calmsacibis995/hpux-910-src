/* $Header: lock.h,v 1.16.83.4 93/09/17 18:28:43 kcs Exp $ */     

#ifndef _SYS_LOCK_INCLUDED
#define _SYS_LOCK_INCLUDED
/*
 * Definitions for locking processes (and segments thereof) into real memory
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;	/* Type returned by sizeof() */
#  endif /* _SIZE_T */

   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
      extern int plock(int);
      extern int datalock(size_t, size_t);
#  else /* not _PROTOTYPES */
      extern int plock();
      extern int datalock();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */


   /* Values for the op argument to plock() */

#  define UNLOCK	0x00000000	/* Remove locks */
#  define PROCLOCK	0x00000001	/* Lock process */
#  define TXTLOCK	0x00000002	/* Lock text */
#  define DATLOCK	0x00000004	/* Lock data */

#  ifdef __hp9000s800
#    define PLOCKSIGNAL	(0x10000000)	/* Send signal */
#  endif /* __hp9000s800 */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_LOCK_INCLUDED */
