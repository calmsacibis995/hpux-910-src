/* @(#) $Revision: 70.1 $ */
#ifndef _CTYPE_INCLUDED
#define _CTYPE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE__STDC__
#  if defined(__STDC__) || defined(__cplusplus)
     extern int isalnum(int);
     extern int isalpha(int);
     extern int iscntrl(int);
     extern int isdigit(int);
     extern int isgraph(int);
     extern int islower(int);
     extern int isprint(int);
     extern int ispunct(int);
     extern int isspace(int);
     extern int isupper(int);
     extern int isxdigit(int);
     extern int tolower(int);
     extern int toupper(int);
#  else /* not __STDC__ || __cplusplus */
     extern int isalnum();
     extern int isalpha();
     extern int iscntrl();
     extern int isdigit();
     extern int isgraph();
     extern int islower();
     extern int isprint();
     extern int ispunct();
     extern int isspace();
     extern int isupper();
     extern int isxdigit();
     extern int tolower();
     extern int toupper();
#  endif /* __STDC__ || __cplusplus */

#  define _U	01
#  define _L	02
#  define _N	04
#  define _S	010
#  define _P	020
#  define _C	040
#  define _B	0100
#  define _X	0200

#  ifndef __lint
     extern unsigned char 		*__ctype;

#  ifdef _NAMESPACE_CLEAN
     extern unsigned char		*__upshift;
     extern unsigned char		*__downshift;
#  else /* not _NAMESPACE_CLEAN */
     extern unsigned char		*_upshift;
     extern unsigned char		*_downshift;
#  endif /* else not _NAMESPACE_CLEAN */

#    define isalpha(__c)	(__ctype[__c]&(_U|_L))
#    define isupper(__c)	(__ctype[__c]&_U)
#    define islower(__c)	(__ctype[__c]&_L)
#    define isdigit(__c)	(__ctype[__c]&_N)
#    define isxdigit(__c)	(__ctype[__c]&_X)
#    define isalnum(__c)	(__ctype[__c]&(_U|_L|_N))
#    define isspace(__c)	(__ctype[__c]&_S)
#    define ispunct(__c)	(__ctype[__c]&_P)
#    define isprint(__c)	(__ctype[__c]&(_P|_U|_L|_N|_B))
#    define isgraph(__c)	(__ctype[__c]&(_P|_U|_L|_N))
#    define iscntrl(__c)	(__ctype[__c]&_C)
#  endif /* __lint */

#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifndef __lint
#    define isascii(__c)		((unsigned) (__c)<=0177)
#  ifdef _NAMESPACE_CLEAN
#    define _toupper(__c)		((__upshift)[__c]&0377)
#    define _tolower(__c)		((__downshift)[__c]&0377)
#  else /* not _NAMESPACE_CLEAN */
#    define _toupper(__c)		((_upshift)[__c]&0377)
#    define _tolower(__c)		((_downshift)[__c]&0377)
#  endif /* else not _NAMESPACE_CLEAN */
#    define toascii(__c)		((__c)&0177)
#  endif /* __lint */
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _CTYPE_INCLUDED */
