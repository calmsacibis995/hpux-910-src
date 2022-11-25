/* $Header: ustat.h,v 1.9.83.4 93/09/17 18:38:02 kcs Exp $ */

#ifndef _SYS_USTAT_INCLUDED
#define _SYS_USTAT_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#if defined(_INCLUDE_HPUX_SOURCE) || defined(_XPG2)

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */

   struct  ustat {
	   daddr_t	f_tfree;	/* total free */
	   ino_t	f_tinode;	/* total inodes free */
	   char		f_fname[6];	/* filsys name */
	   char		f_fpack[6];	/* filsys pack name */
	   int		f_blksize;	/* filsys block size */
   };


#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern int ustat(dev_t, struct ustat *);
#  else /* not _PROTOTYPES */
     extern int ustat();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE || _XPG2 */

#endif /* _SYS_USTAT_INCLUDED */
