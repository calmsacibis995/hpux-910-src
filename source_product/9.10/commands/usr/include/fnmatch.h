/* @(#) $Revision: 70.5 $ */

#ifndef _FNMATCH_INCLUDED
#  define _FNMATCH_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#  ifdef __cplusplus
     extern "C" {
#  endif


#  if defined(_XPG4) || defined(_INCLUDE_POSIX2_SOURCE)
#    define FNM_PATHNAME 0x01   /* Slash in string only matches slash in 
                                   pattern. */
#    define FNM_PERIOD   0x02   /* Leading period in string must be exactly
                                   matched by period in pattern. */
#    define _FNM_UAE     0x04   /* RESERVED */
#    define FNM_NOESCAPE 0x08   /* Disable backslash escaping. */

#    define FNM_NOMATCH  1      /* Return value when no match found */
#    define FNM_NOSYS    2      /* Return value when function not supported */
 
#    ifdef _PROTOTYPES
       extern int fnmatch(const char *, const char *, int);
#    else   /* _PROTOTYPES */
       extern int fnmatch();
#    endif  /* _PROTOTYPES */
#  endif  /* defined (_XPG4) || defined (_INCLUDE_POSIX2_SOURCE) */


#  ifdef __cplusplus
     }
#  endif    /* __cplusplus */


#endif  /* _FNMATCH_INCLUDED */ 
