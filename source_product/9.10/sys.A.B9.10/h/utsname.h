/* $Header: utsname.h,v 1.11.83.4 93/09/17 18:38:07 kcs Exp $ */

#ifndef _SYS_UTSNAME_INCLUDED
#define _SYS_UTSNAME_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE
#  define _SYS_NMLN	9	/* length of strings returned by uname(OS) */
#  define _SNLEN	15

   struct utsname {
	char	sysname[_SYS_NMLN];
	char	nodename[_SYS_NMLN];
	char	release[_SYS_NMLN];
	char	version[_SYS_NMLN];
	char	machine[_SYS_NMLN];
	char	__idnumber[_SNLEN];
   };

#ifndef _KERNEL
#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern int uname(struct utsname *);
#  else /* not _PROTOTYPES */
     extern int uname();
#  endif /* _not _PROTOTYPES */

#ifdef __cplusplus
  }
#endif /* __cplusplus */
#endif /* not _KERNEL */

#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  define SYS_NMLN	_SYS_NMLN
#  define UTSLEN	_SYS_NMLN
#  define SNLEN		_SNLEN

   extern struct utsname utsname;
#  define idnumber	__idnumber
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_UTSNAME_INCLUDED */
