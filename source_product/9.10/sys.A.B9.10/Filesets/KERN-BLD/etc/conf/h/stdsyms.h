/* $Header: stdsyms.h,v 1.7.83.4 93/09/17 18:35:22 kcs Exp $ */

#ifndef _SYS_STDSYMS_INCLUDED /* allows multiple inclusion */
#define _SYS_STDSYMS_INCLUDED

#ifdef _HPUX_SOURCE
#  ifndef _INCLUDE__STDC__
#    define _INCLUDE__STDC__
#  endif /* _INCLUDE__STDC__ */
#  define _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_POSIX2_SOURCE
#  define _INCLUDE_XOPEN_SOURCE
#  define _INCLUDE_AES_SOURCE
#  define _INCLUDE_HPUX_SOURCE
#else
#  ifdef _AES_SOURCE
#    ifndef _INCLUDE__STDC__
#      define _INCLUDE__STDC__
#    endif /* _INCLUDE__STDC__ */
#    define _INCLUDE_POSIX_SOURCE
#    define _INCLUDE_XOPEN_SOURCE
#    define _INCLUDE_AES_SOURCE
#    define _UNDEF_DIRTY
#  else
#    ifdef _XOPEN_SOURCE
#      ifndef _INCLUDE__STDC__
#        define _INCLUDE__STDC__
#      endif /* _INCLUDE__STDC__ */
#      define _INCLUDE_POSIX_SOURCE
#      define _INCLUDE_XOPEN_SOURCE
#      define _UNDEF_DIRTY
#    else
#      ifdef _POSIX2_SOURCE
#        ifndef _POSIX_C_SOURCE
#          define _POSIX_C_SOURCE 2
#        endif /* not _POSIX_C_SOURCE */
#      endif /* _POSIX2_SOURCE */
#      if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
#        ifndef _INCLUDE__STDC__
#          define _INCLUDE__STDC__
#        endif /* _INCLUDE__STDC__ */
#        define _INCLUDE_POSIX_SOURCE
#        define _UNDEF_DIRTY
#      else
#        ifndef __STDC__
#          ifndef _INCLUDE__STDC__
#            define _HPUX_SOURCE
#            define _INCLUDE__STDC__
#            define _INCLUDE_POSIX_SOURCE
#            define _INCLUDE_POSIX2_SOURCE
#            define _INCLUDE_XOPEN_SOURCE
#            define _INCLUDE_AES_SOURCE
#            define _INCLUDE_HPUX_SOURCE
#          endif /* not _INCLUDE__STDC__ */
#        else /* __STDC__ */
#          ifndef _INCLUDE__STDC__
#            define _INCLUDE__STDC__
#          endif /* _INCLUDE__STDC__ */
#          define _UNDEF_DIRTY
#        endif /* else __STDC__ */
#      endif /* _POSIX_SOURCE || _POSIX_C_SOURCE */
#    endif /* _XOPEN_SOURCE */
#  endif /* _AES_SOURCE */
#endif /* _HPUX_SOURCE */

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE > 1)
#  if !defined(_INCLUDE_POSIX2_SOURCE)
#    define _INCLUDE_POSIX2_SOURCE
#  endif /* _INCLUDE_POSIX2_SOURCE */
#endif /* _POSIX_C_SOURCE > 1 */

#if defined(_CLASSIC_TYPES) || defined(_XPG2) || defined(_SVID2)
#  ifndef __STDC__
#    ifdef _HPUX_SOURCE
#      define _CLASSIC_ANSI_TYPES
#      define _CLASSIC_POSIX_TYPES
#      define _CLASSIC_XOPEN_TYPES
#      define _CLASSIC_ID_TYPES
#      if defined(_SVID2) && !defined(_XPG2)
#         define _XPG2
#      endif /* _SVID2 && not _XPG2 */
#    else /* not _HPUX_SOURCE */
#      if defined(_POSIX_SOURCE) && ! defined(_XOPEN_SOURCE)
#        define _CLASSIC_ANSI_TYPES
#      endif /* _POSIX_SOURCE && not _XOPEN_SOURCE */
#    endif /* else not _HPUX_SOURCE */
#  endif /* not __STDC__ */
#endif /* _CLASSIC_TYPES */

#ifdef _XOPEN_SOURCE
#  if !defined(_XPG2) && !defined(_XPG3) && !defined(_XPG4)
#    define _XPG3
#  endif
#endif /* _XOPEN_SOURCE && not _XPG2 && not _XPG4 */

#if defined(_AES_SOURCE) && !defined(_XPG2) && !defined(_XPG4)
#  ifndef _XPG3
#    define _XPG3
#  endif /* _XPG3 */
#endif /* _AES_SOURCE && not _XPG2 && not _XPG4 */

#if defined(_INCLUDE_HPUX_SOURCE) && !defined(_XPG2) && !defined(_XPG3)
#  ifndef _XPG4
#    define _XPG4
#  endif /* _XPG4 */
#endif

#if defined(_XPG3) && !defined(_XPG4) && !defined(_POSIX_C_SOURCE)
#  ifndef _POSIX1_1988
#    define _POSIX1_1988
#  endif /* _POSIX1_1988 */
#endif /* _XPG3 && not _XPG4 && not _POSIX_C_SOURCE */

#ifdef _XPG4
#  ifndef _INCLUDE_POSIX2_SOURCE
#    define _INCLUDE_POSIX2_SOURCE
#  endif /* not _INCLUDE_POSIX2_SOURCE */
#endif /* _XPG4 */

#if defined(__STDC__) || defined(__cplusplus)
#  ifndef _PROTOTYPES
#     define _PROTOTYPES
#  endif /* _PROTOTYPES */
#endif /* __STDC__ || __cplusplus */

#ifdef _UNDEF_DIRTY
/* Undefine predefined symbols from cpp which begin with a letter
   (i.e. - Clean up the namespace)
 */
#  undef unix
#  undef hp9000s800
#  undef hp9000s300
#  undef hp9000s200
#  undef hp9000s500
#  undef hp9000ipc
#  undef PWB
#  undef hpux
#  undef hppa
#endif /* _UNDEF_DIRTY */

#if !defined(_WSIO) && !defined(_SIO)
#if defined(__hp9000s700) || defined(__hp9000s300)
#  define _WSIO
#else /* not (__hp9000s700 || __hp9000s300) */
#  define _SIO
#endif /* not (__hp9000s700 || __hp9000s300) */
#endif /* not _WSIO && not _SIO) */

#endif /* _SYS_STDSYMS_INCLUDED */
