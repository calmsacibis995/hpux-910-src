/* $Header: timeb.h,v 1.10.83.5 93/12/09 12:40:33 marshall Exp $ */    

#ifndef _SYS_TIMEB_INCLUDED
#define _SYS_TIMEB_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#    ifndef _TIME_T
#      define _TIME_T
       typedef long time_t;
#    endif /* _TIME_T */

      /*
       * Structure returned by ftime system call
       */
      struct timeb
      {
	   time_t	 time;
	   unsigned short millitm;
	   short	timezone;
	   short	dstflag;
      };

#    ifndef _KERNEL
#    ifdef __cplusplus
       extern "C" {
#    endif /* __cplusplus */

#    ifdef _PROTOTYPES
       extern int ftime(struct timeb *);
#    else /* not _PROTOTYPES */
       extern int ftime();
#    endif /* not _PROTOTYPES */

#    ifdef __cplusplus
       }
#    endif /* __cplusplus */
#    endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_TIMEB_INCLUDED */
