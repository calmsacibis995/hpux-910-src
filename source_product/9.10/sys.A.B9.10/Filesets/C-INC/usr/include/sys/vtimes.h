/* @(#) $Revision: 1.8.83.5 $ */       
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vtimes.h,v $
 * $Revision: 1.8.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 12:41:16 $
 */
#ifndef _SYS_VTIMES_INCLUDED
#define _SYS_VTIMES_INCLUDED

/*
 * Structure returned by vtimes() and in vwait().
 * In vtimes() two of these are returned, one for the process itself
 * and one for all its children.  In vwait() these are combined
 * by adding componentwise (except for maxrss, which is max'ed).
 */
struct vtimes {
	int	vm_utime;		/* user time (60'ths) */
	int	vm_stime;		/* system time (60'ths) */
	/* divide next two by utime+stime to get averages */
	unsigned vm_idsrss;		/* integral of d+s rss */
	unsigned vm_ixrss;		/* integral of text rss */
	int	vm_maxrss;		/* maximum rss */
	int	vm_majflt;		/* major page faults */
	int	vm_minflt;		/* minor page faults */
	int	vm_nswap;		/* number of swaps */
	int	vm_inblk;		/* block reads */
	int	vm_oublk;		/* block writes */
};

#endif /* _SYS_VTIMES_INCLUDED */
