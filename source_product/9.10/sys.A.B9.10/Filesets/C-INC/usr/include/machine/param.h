/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/param.h,v $
 * $Revision: 1.6.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 17:45:12 $
 */
/* @(#) $Revision: 1.6.84.4 $ */    
#ifndef _MACHINE_PARAM_INCLUDED /* allow multiple inclusions */
#define _MACHINE_PARAM_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/sysmacros.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sysmacros.h>
#endif /* _KERNEL_BUILD */

/*
 * Machine dependent constants
 */

#define NBPG       4096       /* bytes/page */
#define PGOFSET    (NBPG-1)   /* byte offset into page */
#define PGSHIFT    12         /* LOG2(NBPG) */
#define BYTESHIFT   2         /* LOG2(bytes/word) */

#define MAXPHYS (64 * 1024)    /* Maximum size of physical I/O transfer */

#define	CLSIZE		1
#define	CLSIZELOG2	0
#define CLBYTES		(CLSIZE * NBPG)

/*
 * Some macros for units conversion
 */
/* Core clicks (??? bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks to disk blocks */
#define	ctod(x)	(x<<(PGSHIFT- DEV_BSHIFT))
#define	dtoc(x)	(x>>(PGSHIFT- DEV_BSHIFT))         /* truncated conversion only */
#define	dtob(x)	((x)<<DEV_BSHIFT)       

/* page count to physical page frame number and back */
#ifdef _KERNEL
extern int physmembase;
#endif /* _KERNEL */
#define	pctopfn(x)	((x) + physmembase)
#define	pfntopc(x)	((x) - physmembase)
#ifdef _KERNEL
extern unsigned int hil_poffset;
#endif /* _KERNEL */
#define hiltopfn(x)	((x) + hil_poffset)
#define pfntohil(x)	((x) - hil_poffset)

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & PS_S) == 0)  /*check for user mode */
#define	BASEPRI(ps)	(((ps) & PS_IPL) == 0)   /*check for int level 0 */

#define	DELAY(n)	{ register int N = (n); while (--N > 0); }

#ifdef	_KERNEL
/*
 * Constants for what are configurable variables (declared in kernel.h)
 * on some machines
 */
#define	hz	HZ
#define	tick	(1000000/HZ)
#define	phz	0		/* no alternate clock on s200 */

#ifndef INTRLVE
/* macros replacing interleaving functions */
#define dkblock(bp)     ((bp)->b_blkno)
#define dkunit(bp)      (minor((bp)->b_dev) >> 3)
#endif /* not INTRLVE */

#endif	/* _KERNEL */


#endif /* _MACHINE_PARAM_INCLUDED */
