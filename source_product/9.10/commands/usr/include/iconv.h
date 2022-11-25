/* @(#) $Revision: 72.1 $ */

/*
 * Type iconvd also defined in iconvlib.c.
 * Any change to iconvd must be made here and in iconvlib.c.
 */
#ifndef _ICONV_INCLUDED /* allow multiple inclusions */
#define _ICONV_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifdef _XPG4

#    ifndef _SIZE_T
#      define _SIZE_T
       typedef unsigned int size_t;
#    endif /* _SIZE_T */

#    ifndef _SSIZE_T
#      define _SSIZE_T
       typedef int ssize_t;	/* Signed version of size_t */
#    endif /* _SSIZE_T */

     typedef int iconv_t;

#    ifdef _PROTOTYPES
       extern iconv_t iconv_open(const char *, const char *);
       /* iconv() should actually be ssize_t. But X/Open doc. says size_t */
       extern size_t iconv(iconv_t, char **, size_t *, char **, size_t *);
       extern int iconv_close(iconv_t);
#    else /* ! _PROTOTYPES */
       extern iconv_t iconv_open();
       /* iconv() should actually be ssize_t. But X/Open doc. says size_t */
       extern size_t iconv();
       extern int iconv_close();
#    endif /* _PROTOTYPES */

#  endif /* _XPG4 */
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

typedef int iconvd;

extern int (*__iconv[])();
extern int (*__iconv1[])();
extern int (*__iconv2[])();

#if defined(__STDC__) || defined(__cplusplus)
 extern iconvd iconvopen(const char *, const char *, unsigned char *, int, int);
#else /* not __STDC__ || __cplusplus */
 extern iconvd iconvopen();
#endif /* else not __STDC__ || __cplusplus */

#define ICONV( cd, inchar, inbytesleft, outchar, outbytesleft)	\
((*__iconv[(cd)])( (cd), (inchar), (inbytesleft), (outchar), (outbytesleft)))

#define ICONV1( cd, to, from, buflen)				\
	((*__iconv1[(cd)])( (cd), (to), (from), (buflen)))

#define ICONV2( cd, to, from, buflen)				\
	((*__iconv2[(cd)])( (cd), (to), (from), (buflen)))

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _ICONV_INCLUDED */
