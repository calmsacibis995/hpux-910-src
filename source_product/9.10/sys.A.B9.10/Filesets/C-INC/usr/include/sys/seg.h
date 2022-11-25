/* @(#) $Revision: 1.9.83.5 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/seg.h,v $
 * $Revision: 1.9.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 12:40:15 $
 */
#ifndef _SYS_SEG_INCLUDED /* allows multiple inclusion */
#define _SYS_SEG_INCLUDED
/*
 * Memory management addresses and bits
 */
#ifdef __hp9000s300
#define	RO	PG_RO		/* access abilities */
#define	RW	PG_RW
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define	RO	PG_URKR		/* access abilities */
#define	RW	PG_UW
#endif /* __hp9000s800 */

#endif /* _SYS_SEG_INCLUDED */
