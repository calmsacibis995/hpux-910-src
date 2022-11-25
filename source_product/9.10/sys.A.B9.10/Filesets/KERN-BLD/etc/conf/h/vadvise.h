/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/vadvise.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:01:59 $
 */
/* @(#) $Revision: 1.3.84.3 $ */      
#ifndef _SYS_VADVISE_INCLUDED /* allows multiple inclusion */
#define _SYS_VADVISE_INCLUDED
/*
 * Parameters to vadvise() to tell system of particular paging
 * behaviour:
 *	VA_NORM		Normal strategy
 *	VA_ANOM		Sampling page behaviour is not a win, don't bother
 *			Suitable during GCs in LISP, or sequential or random
 *			page referencing.
 *	VA_SEQL		Sequential behaviour expected.
 *	VA_FLUSH	Invalidate all page table entries.
 */
#define	VA_NORM	0
#define	VA_ANOM	1
#define	VA_SEQL	2
#define	VA_FLUSH 3
#endif /* _SYS_VADVISE_INCLUDED */
