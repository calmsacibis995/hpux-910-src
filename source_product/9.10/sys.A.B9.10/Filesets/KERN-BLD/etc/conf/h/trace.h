/* @(#) $Revision: 1.14.83.5 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/trace.h,v $
 * $Revision: 1.14.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 12:40:40 $
 */
#ifndef _SYS_TRACE_INCLUDED /* allows multiple inclusion */
#define _SYS_TRACE_INCLUDED

/*
 * File system buffer tracing points; all trace <dev, bn>
 */
#define	TR_BREADHIT	0	/* buffer read found in cache */
#define	TR_BREADMISS	1	/* buffer read not in cache */
#define	TR_BWRITE	2	/* buffer written */
#define	TR_BREADHITRA	3	/* buffer read-ahead found in cache */
#define	TR_BREADMISSRA	4	/* buffer read-ahead not in cache */
#define	TR_XFODMISS	5	/* exe fod read */
#define	TR_XFODHIT	6	/* exe fod read */
#define	TR_BRELSE	7	/* brelse */

/*
 * Memory allocator trace points; all trace the amount of memory involved
 */
#define	TR_MALL		10	/* memory allocated */
#define TR_MFREE	11	/* memory deallocated */
#ifdef __hp9000s800
#define TR_VALIDATE	12	/* validate() called */
#define TR_VALINVAL	13	/* validate() does an invalidate */
#define TR_INVALIDATE	14	/* invalidate() called */
#endif /* __hp9000s800 */

/*
 * Paging trace points: all are <vaddr, pid>
 */
#define	TR_INTRANS	20	/* page intransit block */
#define	TR_EINTRANS	21	/* page intransit wait done */
#define	TR_FRECLAIM	22	/* reclaim from free list */
#define	TR_RECLAIM	23	/* reclaim from loop */
#define	TR_XSFREC	24	/* reclaim from free list instead of drum */
#define	TR_XIFREC	25	/* reclaim from free list instead of fsys */
#define	TR_WAITMEM	26	/* wait for memory in pagein */
#define	TR_EWAITMEM	27	/* end memory wait in pagein */
#define	TR_ZFOD		28	/* zfod page fault */
#define	TR_EXFOD	29	/* exec fod page fault */
#define	TR_VRFOD	30	/* vread fod page fault */
#define	TR_CACHEFOD	31	/* fod in file system cache */
#define	TR_SWAPIN	32	/* drum page fault */
#define	TR_PGINDONE	33	/* page in done */
#define	TR_SWAPIO	34	/* swap i/o request arrives */
#define TR_PSWAPIN	35	/* swap in of process */
#define TR_PSWAPOUT	36	/* swap out of process */

/*
 * System call trace points.
 */
#define	TR_VADVISE	40	/* vadvise occurred with <arg, pid> */

/*
 * Miscellaneous
 */
#define	TR_STAMP	50	/* user said vtrace(VTR_STAMP, value); */

/*
 * This defines the size of the trace flags array.
 */
#define	TR_NFLAGS	100	/* generous */
#define TR_ENTSIZE	  8	/* number of (int) entries per trace */
#define	TRCSIZ		4096	/* size of trace entry array */

/* 
 * Define what the trace flags contain and what trace1() should do
 * on each call.
 */
#define TR_OFF		0	/* turn it off (do not change this!) */
#define TR_LOG		1	/* just log trace points */
#define TR_PRINT	2	/* log trace points and printf to console */

/*
 * Specifications of the vtrace() system call, which takes one argument.
 */
#define	VTRACE		64+51

#define	VTR_DISABLE	0		/* set a trace flag to 0 */
#define	VTR_ENABLE	1		/* set a trace flag to 1 */
#define	VTR_VALUE	2		/* return value of a trace flag */
#define	VTR_UALARM	3		/* set alarm to go off (sig 16) */
					/* in specified number of hz */
#define	VTR_STAMP	4		/* user specified stamp */
#ifdef _KERNEL
#ifdef TRACE
#ifdef	__hp9000s800
extern	char	traceflags[TR_NFLAGS];
extern	struct	proc *traceproc;
extern	int	tracebuf[TRCSIZ];
extern	unsigned tracex;
extern	int	tracewhich;
#else	/* not __hp9000s800 */
char	traceflags[TR_NFLAGS];
struct	proc *traceproc;
int	tracebuf[TRCSIZ];
unsigned tracex;
int	tracewhich;
#endif	/* else not __hp9000s800 */
#define	trace(a,b,c)	if (traceflags[a]) trace1(a,b,c)
#define tracel(a,b,c,d,e,f,g) if (traceflags[a]) trace1(a,b,c,d,e,f,g)
#else /* not TRACE */
#define	trace(a,b,b)	;
#define tracel(a,b,c,d,e,f,g)	;
#endif /* else not TRACE */
#endif /* _KERNEL */
#endif /* _SYS_TRACE_INCLUDED */
