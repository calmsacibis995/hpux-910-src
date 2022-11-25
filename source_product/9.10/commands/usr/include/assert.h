/* @(#) $Revision: 70.1 $ */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

#ifdef _INCLUDE__STDC__

#  undef assert

#  ifdef NDEBUG
#    define assert(_EX)  ((void)0)
#  else /* not NDEBUG */

#  ifdef _NAMESPACE_CLEAN
#    ifdef _PROTOTYPES
       extern void __assert(char *, char *, int);

#      if defined(__cplusplus) && !defined(__STDCPP__)
#        define assert(_EX) \
  	    ((_EX) ? (void)0 : __assert("_EX", __FILE__, __LINE__))
#      else
#        define assert(_EX) \
  	    ((_EX) ? (void)0 : __assert(#_EX, __FILE__, __LINE__))
#      endif

#    else /* ! _PROTOTYPES */
       extern void __assert();

#      define assert(_EX) \
  	  ((_EX) ? (void)0 : __assert("_EX", __FILE__, __LINE__))
#    endif /* else ! _PROTOTYPES */
#  else /* not _NAMESPACE_CLEAN */
#    ifdef _PROTOTYPES
       extern void _assert(char *, char *, int);

#      if defined(__cplusplus) && !defined(__STDCPP__)
#        define assert(_EX) \
  	    ((_EX) ? (void)0 : _assert("_EX", __FILE__, __LINE__))
#      else
#        define assert(_EX) \
  	    ((_EX) ? (void)0 : _assert(#_EX, __FILE__, __LINE__))
#      endif

#    else /* ! _PROTOTYPES */
       extern void _assert();

#      define assert(_EX) \
  	  ((_EX) ? (void)0 : _assert("_EX", __FILE__, __LINE__))
#    endif /* else ! _PROTOTYPES */
#  endif /* else not _NAMESPACE_CLEAN */
#  endif /* not NDEBUG */

#endif /* _INCLUDE__STDC__ */

#ifdef __cplusplus
 }
#endif /* __cplusplus */
