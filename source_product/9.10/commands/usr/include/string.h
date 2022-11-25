/* @(#) $Revision: 70.6 $ */
#ifndef _STRING_INCLUDED
#define _STRING_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE__STDC__
#  ifndef NULL
#    define NULL 0
#  endif

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif

#  if defined(__STDC__) || defined(__cplusplus)
     extern int memcmp(const void *, const void *, size_t);
     extern char *strncat(char *, const char *, size_t);
     extern int strncmp(const char *, const char *, size_t);
     extern void *memmove(void *, const void *, size_t);
     extern char *strcpy(char *, const char *);
     extern char *strncpy(char *, const char *, size_t);
     extern char *strcat(char *, const char *);
     extern int strcmp(const char *, const char *);
     extern int strcoll(const char *, const char *);
     extern size_t strxfrm(char *, const char *, size_t);
     extern char *strchr(const char *, int);
     extern char *strpbrk(const char *, const char *);
     extern char *strrchr(const char *, int);
     extern char *strstr(const char *, const char *);
     extern char *strtok(char *, const char *);
     extern char *strerror(int);
#  else /* __STDC__ || __cplusplus */
     extern int memcmp();
     extern void *memmove();
     extern char *strcpy();
     extern char *strncpy();
     extern char *strcat();
     extern char *strncat();
     extern int strcmp();
     extern int strncmp();
     extern int strcoll();
     extern size_t strxfrm();
     extern char *strchr();
     extern char *strpbrk();
     extern char *strrchr();
     extern char *strstr();
     extern char *strtok();
     extern char *strerror();
#  endif /* __STDC__ || __cplusplus */

#  ifdef _CLASSIC_ANSI_TYPES
     extern char *memcpy();
     extern char *memchr();
     extern char *memset();
     extern int strcspn();
     extern int strspn();
     extern int strlen();
#  else
#  if defined(__STDC__) || defined(__cplusplus)
       extern void *memcpy(void *, const void *, size_t);
       extern void *memchr(const void *, int, size_t);
       extern void *memset(void *, int, size_t);
       extern size_t strcspn(const char *, const char *);
       extern size_t strspn(const char *, const char *);
#      if defined(_INCLUDE_AES_SOURCE) && !defined(_XPG4)
          extern size_t strlen(char *);
#      else /* not _INCLUDE_AES_SOURCE || _XPG4 */
          extern size_t strlen(const char *);
#      endif /* not _INCLUDE_AES_SOURCE || _XPG4 */
#  else /* not __STDC__ || __cplusplus */
       extern void *memcpy();
       extern void *memchr();
       extern void *memset();
       extern size_t strcspn();
       extern size_t strspn();
       extern size_t strlen();
#    endif /* else not __STDC__ || __cplusplus */
#  endif
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifdef _CLASSIC_XOPEN_TYPES
     extern char *memccpy();
#  else
#    if defined(__STDC__) || defined(__cplusplus)
       extern void *memccpy(void *, const void *, int, size_t);
#    else /* __STDC__ || __cplusplus */
       extern void *memccpy();
#    endif /* __STDC__ || __cplusplus */
#  endif
#endif /* _INCLUDE_XOPEN_SOURCE */


#if defined(_INCLUDE_AES_SOURCE) && !defined(_INCLUDE_HPUX_SOURCE)
   /* swab() is defined here for AES and in <sys/unistd.h> otherwise */
#  ifdef _PROTOTYPES
     extern void swab(const char *, char *, int);
#  else	/* no _PROTOTYPES */
     extern void swab();
#  endif /* _PROTOTYPES */
#endif /* _INCLUDE_AES_SOURCE && not _INCLUDE_HPUX_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE
#  if defined(__STDC__) || defined(__cplusplus)
     extern char *strrstr(const char *, const char *);
     extern int strcasecmp(const char *, const char *);
     extern int strncasecmp(const char *, const char *, size_t);
     extern char *strdup(const char *);
     extern int nl_strcmp(const char *, const char *);
     extern int nl_strncmp(const char *, const char *, size_t);
#  else /* __STDC__ || __cplusplus */
     extern char *strrstr();
     extern int strcasecmp();
     extern int strncasecmp();
     extern char *strdup();
     extern int nl_strcmp();
     extern int nl_strncmp();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _STRING_INCLUDED */
