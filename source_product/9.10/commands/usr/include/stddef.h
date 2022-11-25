/* @(#) $Revision: 66.2 $ */
#ifndef _STDDEF_INCLUDED
#define _STDDEF_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _INCLUDE__STDC__
#  ifndef NULL
#    define	NULL	0
#  endif

#  define offsetof(__s_name, __m_name)  ((size_t)&(((__s_name*)0)->__m_name))

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif

#  ifndef _WCHAR_T
#    define _WCHAR_T
     typedef unsigned int wchar_t;
#  endif

   typedef int ptrdiff_t;

#endif /* _INCLUDE__STDC__ */
#endif /* _STDDEF_INCLUDED */
