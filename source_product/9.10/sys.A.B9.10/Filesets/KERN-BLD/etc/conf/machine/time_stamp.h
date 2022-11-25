/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/time_stamp.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:19:25 $
 */
/* $IRS$ time_stamp */

#ifndef _MACHINE_TIME_STAMP_INCLUDED
#define _MACHINE_TIME_STAMP_INCLUDED

#ifdef CONFIG_VAX
/* VAX timer services require quad word types which have the lsw in the      */
/* upper unsdword.                                                           */
typedef struct {    
	unsdword        time;
	unsdword        cycles;
} time_stamp;
#else
typedef struct {
	unsdword        cycles;
	unsdword        time;
} time_stamp;
#endif
/* $IRS$ */

#endif /* not _MACHINE_TIME_STAMP_INCLUDED */
