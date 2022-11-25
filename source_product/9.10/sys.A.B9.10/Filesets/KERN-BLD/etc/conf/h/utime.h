/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/utime.h,v $
 * $Revision: 1.2.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:01:53 $
 */
/* @(#) $Revision: 1.2.84.3 $ */    
#ifndef _UTIME_INCLUDED /* allows multiple inclusion */
#define _UTIME_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
 * Structure used by utime()
 */
struct utimbuf {
	time_t	actime;		/* Access time */
	time_t	modtime;	/* Modification time */
};

#endif /* _UTIME_INCLUDED */
