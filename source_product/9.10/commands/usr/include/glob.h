/* @(#) $Revision: 70.4 $ */
#ifndef _GLOB_INCLUDED
#define _GLOB_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
   extern "C" {
#endif

#if defined( _INCLUDE_POSIX2_SOURCE ) || defined( _XPG4 )

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif /* _SIZE_T */

   typedef struct {
     size_t   gl_pathc;   /* count of paths matched by pattern */
     char     **gl_pathv; /* pointer to matched pathnames */
     size_t   gl_offs;    /* slots at beginning of gl_argv */
     char     *gl_mem;    /* string storage for glob */
   } glob_t;

#  define GLOB_ERR        0x01    /* return on unreadable directory */
#  define GLOB_MARK       0x02    /* append slashes to dirnames that match */
#  define GLOB_NOSORT     0x04    /* don't sort output */
#  define GLOB_NOCHECK    0x08    /* return 1 match if no special chars */
#  define GLOB_DOOFFS     0x10    /* use gl_offs field */
#  define GLOB_APPEND     0x20    /* append to previous glob_t */
#  define GLOB_NOESCAPE   0x40    /* don't interpret char. after backshash */

#  define GLOB_NOSPACE    1       /* memory allocation failed */
#  define GLOB_ABORTED    2       /* stopped scan */
#  define GLOB_NOMATCH    3       /* no match and GLOB_NOCHECK not set */
#  ifdef _XPG4
#  define GLOB_NOSYS      4	  /* does not support this function */
#  endif /* _XPG4 */

#  ifdef _PROTOTYPES 
     extern int glob(const char *, int, int (*)(const char *, int), glob_t *);
     extern void globfree(glob_t *);
#  else /* _PROTOTYPES */
     extern int glob();
     extern void globfree();
#  endif /* _PROTOTYPES */
#endif  /* defined( _INCLUDE_POSIX2_SOURCE ) || defined( _XPG4 ) */


#ifdef _INCLUDE_HPUX_SOURCE
#  define GLOB_ABEND      GLOB_ABORTED   /* stopped scan */
#endif /* _INCLUDE_HPUX_SOURCE */


#ifdef __cplusplus
   }
#endif

#endif /* _GLOB_INCLUDED */
