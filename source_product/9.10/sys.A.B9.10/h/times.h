/* $Header: times.h,v 1.11.83.4 93/09/17 18:36:33 kcs Exp $ */

#ifndef _SYS_TIMES_INCLUDED
#define _SYS_TIMES_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE

#  ifndef _CLOCK_T
#    define _CLOCK_T
     typedef unsigned long clock_t;
#  endif /* _CLOCK_T */

   /*
    * Structure returned by times()
    */
      struct tms {
	   clock_t tms_utime;		/* user time */
	   clock_t tms_stime;		/* system time */
	   clock_t tms_cutime;		/* user time, children */
	   clock_t tms_cstime;		/* system time, children */
      };

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif

#  ifdef _PROTOTYPES
         extern clock_t times(struct tms *);
#  else /* not _PROTOTYPES */
#    ifdef _CLASSIC_POSIX_TYPES
         extern long times();
#    else /* not _CLASSIC_POSIX_TYPES */
         extern clock_t times();
#    endif /* not _CLASSIC_POSIX_TYPES */
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif
#  endif /* _KERNEL */

#  endif /* _INCLUDE_POSIX_SOURCE */

#endif /* _SYS_TIMES_INCLUDED */
