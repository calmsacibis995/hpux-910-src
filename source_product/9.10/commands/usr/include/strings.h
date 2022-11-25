/* @(#) $Revision: 66.5 $ */
#ifndef _STRINGS_INCLUDED
#define _STRINGS_INCLUDED

/*
 * This header file is for BSD applications importability which expects
 * a header file with this name to include all the string functions and
 * types.
 */

#include <string.h>
#include <sys/stdsyms.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_HPUX_SOURCE
#  if defined(__STDC__) || defined(__cplusplus)
     extern char *index(const char *, char);
     extern char *rindex(const char *, char);
     extern void bcopy(const char *, char *, int);
     extern int bcmp(const char *, const char *, int);
     extern void bzero(char *, int);
     extern int ffs(int);
#  else /* __STDC__ || __cplusplus */
     extern char *index();
     extern char *rindex();
     extern void bcopy();
     extern int bcmp();
     extern void bzero();
     extern int ffs();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _STRINGS_INCLUDED */
