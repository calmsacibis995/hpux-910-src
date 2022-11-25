/* @(#) $Revision: 66.5 $ */
#ifndef _SETJMP_INCLUDED /* allow multiple inclusions */
#define _SETJMP_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE__STDC__
#  ifdef __hp9000s300
#    define _JBLEN 100
     typedef int jmp_buf[_JBLEN];
#  endif /* __hp9000s300 */
#  ifdef hp9000s500
#    define _JBLEN 7
     typedef int jmp_buf[_JBLEN];
#  endif /* hp9000s500 */
#  ifdef __hp9000s800
#    define _JBLEN 50
     typedef double jmp_buf[_JBLEN/2];
#  endif /* __hp9000s800 */


#  if defined(__STDC__) || defined(__cplusplus)
     extern int setjmp(jmp_buf);
     extern void longjmp(jmp_buf, int);
#  else /* not __STDC__ || __cplusplus */
     extern int setjmp();
     extern void longjmp();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifdef __hp9000s800
     typedef double sigjmp_buf[_JBLEN/2];
#  endif /* __hp9000s800 */
#  ifdef __hp9000s300
     typedef int sigjmp_buf[_JBLEN];
#  endif /* __hp9000s300 */

#    if defined(__STDC__) || defined(__cplusplus)
       extern int sigsetjmp(sigjmp_buf, int);
       extern void siglongjmp(sigjmp_buf, int);
#    else /* not __STDC__ || __cplusplus */
       extern int sigsetjmp();
       extern void siglongjmp();
#    endif /* __STDC__ || __cplusplus*/
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  if defined(__STDC__) || defined(__cplusplus)
     extern int _setjmp(jmp_buf);
     extern void _longjmp(jmp_buf, int);
#  else /* not __STDC__ || __cplusplus */
     extern int _setjmp();
     extern void _longjmp();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /*_SETJMP_INCLUDED */
