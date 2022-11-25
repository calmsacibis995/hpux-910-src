/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/vmparam.h,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:09:02 $
 */
/* @(#) $Revision: 1.5.84.3 $ */
#ifndef _MACHINE_VMPARAM_INCLUDED /* allow multiple inclusions */
#define _MACHINE_VMPARAM_INCLUDED

/* get defines for various machines */
#ifdef _KERNEL_BUILD
#include "../machine/cpu.h"
#else  /* ! _KERNEL_BUILD */
#include <machine/cpu.h>
#endif /* _KERNEL_BUILD */

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 */
#define	SAFERSS		2		/* nominal ``small'' resident set size
					   protected against replacement */
/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.
 * Note:  KLMAX*CLSIZE must be <= dmmin
 */

/********** THESE CONSTANTS SHOULD BE DEPENDENT ON THE             *****/
/********** CONSTANTS OF THE IMPLEMENTATION (NBPG, DEV_BSIZE, etc) *****/
#define	KLMAX	(8/CLSIZE)
#define	KLSEQL	(4/CLSIZE)		/* in klust if vadvise(VA_SEQL) */
#define	KLIN	(4/CLSIZE)		/* default data/stack in klust */
#define	KLTXT	(4/CLSIZE)		/* default text in klust */
#define	KLOUT	(4/CLSIZE)

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */
#define	KLSDIST	3		/* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 */
#define LOTSFREEMAX	(512 * 1024)
#define	LOTSFREEFRACT	8
#define	DESFREE		(200 * 1024)
#define	DESFREEFRACT	16
#define	MINFREE		(64 * 1024)
#define	MINFREEFRACT	2

/*
 * BUFFERZONE is no. of pages between tune.gpgslo and tune.gpgshi. If this
 * number is zero, vhand may oscillate, getting freemem up to lotsfree and
 * quitting, then being activated again as soon as freemem dips below lotsfree
 * This is a software "schmidt trigger" it may buy some X response time.
 */
#define BUFFERZONE      ((100 * 1024)/NBPG)

/*
 * Believed threshold (in megabytes) for which interleaved
 * swapping area is desirable.
 */
#define	LOTSOFMEM	2

#ifdef _KERNEL
/* This "space number" indicates that you're referring to the kernel space */
    /*
     * XXX should be vas_t, but then need to include lots of stuff to
     *  make everybody understand it.
     */
extern struct vas *kernelspace, kernvas;
extern struct pregion *kernpreg;
#define KERNELSPACE ((space_t)kernelspace)
#define KERNPREG (kernpreg)
#define KERNVAS (&kernvas)

/*
 * Tell whether we're on the interrupt stack.  This test isn't of a VM
 *  data structure, but the VM system seems to be the only one who cares,
 *  so I'm putting it here anyway... Andy
 */
extern int interrupt;
extern int mainentered;

#define ON_ISTACK (!mainentered || (interrupt != 0))
#endif /* _KERNEL */

#endif /* _MACHINE_VMPARAM_INCLUDED */
