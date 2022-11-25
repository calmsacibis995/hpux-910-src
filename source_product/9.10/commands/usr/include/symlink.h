/* @(#) $Revision: 70.1 $ */
#ifndef _SYMLINK_INCLUDED
#define _SYMLINK_INCLUDED

/*
 * symlink.h
 *
 * Declarations for symbolic link functions.
 */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */


#ifdef _INCLUDE_AES_SOURCE

/* Types needed for this file */
#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;	/* Type returned by sizeof() */
#  endif /* _SIZE_T */

/* Function prototypes */

#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  if defined(__STDC__) || defined(__cplusplus)
#    ifdef _INCLUDE_HPUX_SOURCE
       extern int readlink(const char *, char *, size_t);
#    else /* not _INCLUDE_HPUX_SOURCE */
       extern int readlink(const char *, char *, int);
#    endif /* not _INCLUDE_HPUX_SOURCE */
       extern int symlink(const char *, const char *);
#  else /* not (__STDC__ || __cplusplus) */
       extern int readlink();
       extern int symlink();
#  endif /* not (__STDC__ || __cplusplus) */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */

#endif /* _INCLUDE_AES_SOURCE */

#endif /* _SYMLINK_INCLUDED */
