/* @(#) $Revision: 1.18.83.4 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vmsystm.h,v $
 * $Revision: 1.18.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:39:15 $
 */
#ifndef _SYS_VMSYSTM_INCLUDED /* allows multiple inclusion */
#define _SYS_VMSYSTM_INCLUDED

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */

#ifdef _KERNEL
extern	int	freemem;	/* remaining blocks of free memory */
extern	int	freemem_cnt;	/* number of processes waiting on freemem */
extern	int	avefree;	/* moving average of remaining free blocks */
extern	int	avefree30;	/* 30 sec (avefree is 5 sec) moving average */
extern	int	deficit;	/* estimate of needs of new swapped in procs */
extern	int	nscan;		/* number of scans in last second */
extern	int	multprog;	/* current multiprogramming degree */
extern	int	desscan;	/* desired pages scanned per second */

/* writable copies of tunables */
extern	int	maxslp;		/* max sleep time before very swappable */
extern	int	lotsfree;	/* max free before clock freezes */
extern	int	minfree;	/* minimum free pages before swapping begins */
extern	int	desfree;	/* no of pages to try to keep free via daemon */
extern	int	saferss;	/* no pages not to steal; decays with slptime */

/* AGEFRACTION of n means we want to age 1/n of a region before going on */
/* AGEFRACTION of 16 is the smallest possible since p_ageremain is a short */
#define LOGAGEFRACTION 4
#define AGEFRACTION (1 << LOGAGEFRACTION)
#define AGEFRACTIONMASK (AGEFRACTION - 1)
#endif

/*
 * Fork/vfork accounting.
 */
struct	forkstat
{
	int	cntfork;
	int	cntvfork;
#ifdef __hp9000s800
	int	cntsfork;
#endif /* __hp9000s800 */
	int	sizfork;
	int	sizvfork;
#ifdef __hp9000s800
	int	sizsfork;
#endif /* __hp9000s800 */
};
#ifdef	_KERNEL
#ifdef	__hp9000s800
extern	struct	forkstat forkstat;
#else	/* not __hp9000s800 */
struct	forkstat forkstat;
#endif	/* else not __hp9000s800 */
#endif

#endif /* _SYS_VMSYSTM_INCLUDED */
