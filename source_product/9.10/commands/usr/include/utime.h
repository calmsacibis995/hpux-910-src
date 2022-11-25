/* @(#) $Revision: 70.1 $ */
#ifndef _UTIME_INCLUDED
#define _UTIME_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef KERNEL
#    include "../h/types.h"
#  else
#    include <sys/types.h>
#  endif
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_POSIX_SOURCE

#  ifndef _TIME_T
#    define _TIME_T
     typedef long time_t;
#  endif /* _TIME_T */

   struct utimbuf {
	time_t actime;
	time_t modtime;
   };

#   ifndef _INCLUDE_AES_SOURCE
#       if defined(__STDC__) || defined(__cplusplus)
           extern int utime(const char *, const struct utimbuf *);
#       else /* __STDC__ || __cplusplus */
           extern int utime();
#       endif /* __STDC__ || __cplusplus */
#   endif /* _INCLUDE_AES_SOURCE */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_AES_SOURCE
#       if defined(__STDC__) || defined(__cplusplus)
           extern int utime(const char *, struct utimbuf *);
#       else /* __STDC__ || __cplusplus */
           extern int utime();
#       endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _UTIME_INCLUDED */
