/* $Header: monetary.h,v 72.2 92/09/25 09:41:49 ssa Exp $ */

#ifndef _MONETARY_INCLUDED /* allow multiple inclusions */
#define _MONETARY_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#if defined(_INCLUDE_XOPEN_SOURCE) && !defined(_XPG3) && !defined(_XPG2)

#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;	/* Type returned by sizeof() */
#  endif /* _SIZE_T */

#  ifndef _SSIZE_T
#     define _SSIZE_T
      typedef int ssize_t;		/* Signed version of size_t */
#  endif /* _SSIZE_T */

#  ifdef _PROTOTYPES
     extern ssize_t strfmon(char *, size_t, const char *, ...);
#  else /* not _PROTOTYPES */
     extern ssize_t strfmon();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */

#endif /* _INCLUDE_XOPEN_SOURCE && !_XPG3 && !_XPG2 */
#endif /* not _MONETARY_INCLUDED */
