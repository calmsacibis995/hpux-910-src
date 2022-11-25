/* $Header: rtprio.h,v 1.11.83.5 93/12/09 12:40:08 marshall Exp $ */       

#ifndef _SYS_RTPRIO_INCLUDED
#define _SYS_RTPRIO_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

   /* Types */

#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;		/* same as in types.h */
#  endif /* _PID_T */


   /* Function prototype */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern int rtprio(pid_t, int);
#  else /* not _PROTOTYPES */
     extern int rtprio();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */


   /*
    * Real time priority 
    *
    */   

#    if defined(__hp9000s800) || defined(_KERNEL)
#      define rtconvert(pri) pri+PRTBASE
#      define rtunconvert(pri) pri-PRTBASE
#    endif	/* defined(__hp9000s800) || defined(_KERNEL) */

#    ifdef __hp9000s300
#      ifdef DISPATCHLCK
#        define DISPATCHLOCK   1002
#        define DISPATCHUNLOCK 1003
#      endif /* DISPATCHLCK */
#    endif /* __hp9000s300 */

#    define RTPRIO_NOCHG   1000
#    define RTPRIO_RTOFF   1001
#    define RTPRIO_MIN        0
#    define RTPRIO_MAX      127

#    ifdef __hp9000s800
#      define NOTREALTIME	-1
#    endif /* __hp9000s800 */

#    if defined(__hp9000s800) || defined(_KERNEL)
#      define ALL_BITS_SET 0xFFFFFFFF
#      define higherqs(pri) (~(ALL_BITS_SET<<(pri>>2))&whichqs)
#    endif /* defined(__hp9000s800) || defined(_KERNEL) */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_RTPRIO_INCLUDED */
