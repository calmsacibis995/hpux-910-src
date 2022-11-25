/* @(#) $Revision: 70.1 $ */     

/*
 * Header file to support devnm(3).
 */

#ifndef _DEVNM_INCLUDED	 /* allow multiple inclusions */
#define _DEVNM_INCLUDED

#include <sys/types.h>		/* for various types */
#include <sys/stat.h>		/* for stat() values */

#ifndef _SYS_STDSYMS_INCLUDED
#include <sys/stdsyms.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_HPUX_SOURCE
#if defined(__STDC__) || defined(__cplusplus)
extern int devnm (mode_t, dev_t, char *, size_t, int);
#else
extern int devnm();
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DEVNM_INCLUDED */
