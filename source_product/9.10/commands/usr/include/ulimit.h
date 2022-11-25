/* @(#) $Revision: 66.4 $ */
#ifndef  _ULIMIT_INCLUDED
#define  _ULIMIT_INCLUDED	/* Allows ulimit.h to be multiply included */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE
/* Just provide a unique ID for each symbolic name follows */
#  define  UL_GETFSIZE    1  
#  define  UL_SETFSIZE    2

#  if defined(__STDC__) || defined(__cplusplus)
     extern long ulimit(int, ...);
#  else /* not __STDC__ || __cplusplus */
     extern long ulimit();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  define  UL_GETMAXBRK   3 
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif  /* _ULIMIT_INCLUDED */
