/* @(#) $Revision: 1.18.83.4 $ */       
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/kernel.h,v $
 * $Revision: 1.18.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:27:49 $
 */
#ifndef _SYS_KERNEL_INCLUDED /* allows multiple inclusion */
#define _SYS_KERNEL_INCLUDED
/*
 * Global variables for the kernel
 */

long	rmalloc();

/* 1.1 */
#ifdef	__hp9000s800

extern	long	hostid;

extern struct utsname utsname;

#endif	/* __hp9000s800 */

/* 1.2 */
#ifdef	__hp9000s800
extern	struct	timeval boottime;
#if defined(VOLATILE_TUNE) && defined(_KERNEL)
extern	volatile struct	timeval time;
#else
extern	struct	timeval time;
#endif /* VOLATILE_TUNE */
extern	struct	timezone tz;		/* XXX */
extern	int	hz;
extern	int	phz;			/* alternate clock's frequency */
extern	int	tick;
extern	int	lbolt;			/* awoken once a second */
#else	/* not __hp9000s800 */
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;			/* XXX */
#ifndef	__hp9000s300	/* hz, phz, & tick are constants in param.h */
int	hz;
int	phz;				/* alternate clock's frequency */
int	tick;
#endif	/* __hp9000s300 */
int	lbolt;				/* awoken once a second */
#endif	/* else not __hp9000s800 */
int	realitexpire();

#ifdef	__hp9000s800
extern	double	avenrun[3];
#else	/* not __hp9000s800 */
double	avenrun[3];
#endif	/* else not __hp9000s800 */

#ifdef	__hp9000s300
extern	int	timeslice;		/* unit: 20ms tick */
#endif	/* __hp9000s300 */

#ifdef GPROF
extern	int profiling;
extern	char *s_lowpc;
extern	u_long s_textsize;
extern	u_short *kcount;
#ifdef	__hp9000s300
extern	u_short	*sbuf;
#endif	/* __hp9000s300 */
#endif
#define DOMAINNAMELENGTH	65
int	sbolt;			/* syncd awaken per SYNC_PARAM sec */
#endif /* _SYS_KERNEL_INCLUDED */
