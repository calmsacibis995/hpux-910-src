/*
 * @(#)strings.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:30 $
 * $Locker:  $
 */

/* strings.h,v 3.1 1993/07/06 01:07:05 jbj Exp
 * strings.h - string compatibility for HPUX.
 * XXX may not needed anymore.
 */
#if defined(HPUX) && (HPUX < 8)
#include <string.h>

extern char *index(), *rindex();
#endif
#if defined(SOLARIS)
#include <string.h>

#define index(s,c)     strchr(s,c)
#define rindex(s,c)    strrchr(s,c)
#define bcopy(s1,s2,n) memcpy(s2, s1, n)
#define bzero(s,n)     memset(s, 0, n)
#define bcmp(s1,s2,n)  memcmp(s1, s2, n)
#endif
