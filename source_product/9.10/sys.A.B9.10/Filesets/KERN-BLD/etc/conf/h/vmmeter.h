/* @(#) $Revision: 1.19.83.5 $ */      
#ifndef _SYS_VMMETER_INCLUDED /* allows multiple inclusion */
#define _SYS_VMMETER_INCLUDED

/*
 * Virtual memory related instrumentation
 */
struct vmmeter
{
#define	v_first	v_swtch
	unsigned v_swtch;	/* context switches */
	unsigned v_trap;	/* calls to trap */
	unsigned v_syscall;	/* calls to syscall() */
	unsigned v_intr;	/* device interrupts */
	unsigned v_pdma;	/* pseudo-dma interrupts */
	unsigned v_pswpin;	/* pages swapped in */
	unsigned v_pswpout;	/* pages swapped out */
	unsigned v_pgin;	/* pageins */
	unsigned v_pgout;	/* pageouts */
	unsigned v_pgpgin;	/* pages paged in */
	unsigned v_pgpgout;	/* pages paged out */
	unsigned v_intrans;	/* intransit blocking page faults */
	unsigned v_pgrec;	/* total page reclaims */
	unsigned v_xsfrec;	/* found in free list rather than on swapdev */
	unsigned v_xifrec;	/* found in free list rather than in filsys */
	unsigned v_exfod;	/* pages filled on demand from executables */
	unsigned v_zfod;	/* pages zero filled on demand */
	unsigned v_vrfod;	/* fills of pages mapped by vread() */
	unsigned v_nexfod;	/* number of exfod's created */
	unsigned v_nzfod;	/* number of zfod's created */
	unsigned v_nvrfod;	/* number of vrfod's created */
	unsigned v_pgfrec;	/* page reclaims from free list */
	unsigned v_faults;	/* total faults taken */
	unsigned v_scan;	/* scans in page out daemon */
	unsigned v_rev;		/* revolutions of the hand */
	unsigned v_seqfree;	/* pages taken from sequential programs */
	unsigned v_dfree;	/* pages freed by daemon */
	unsigned v_cwfault;	/* Copy on write faults */
	unsigned f_bread;	/* total bread requests */
	unsigned f_breadcache;	/* total bread cache hits */
	unsigned f_breadsize;	/* total bread bytes */
	unsigned f_breada;	/* total read aheads */
	unsigned f_breadacache;	/* total read ahead cache hits */
	unsigned f_breadasize;	/* total read ahead bytes */
	unsigned f_bwrite;	/* total bwrite requests */
	unsigned f_bwritesize;	/* total bwrite bytes */
	unsigned f_bdwrite;	/* total bdwrite requests */
	unsigned f_bdwritesize;	/* total bdwrite bytes */
#ifdef __hp9000s800
	unsigned v_pgtlb;	/* tlb flushes */
	unsigned v_swpwrt;	/* swap writes */
#endif /* __hp9000s800 */
	unsigned v_fastpgrec;	/* fast reclaims in locore */
	unsigned f_clnbkfl;	/* clean block found immediatly on free list */
	unsigned f_flsempty;	/* free list empty */
	unsigned f_bufbusy;	/* buffer busy */
	unsigned f_delwrite;	/* delayed write buffer written */
#define v_last   f_delwrite
	unsigned v_free;	/* free memory pages */
	unsigned v_swpin;	/* swapins */
	unsigned v_swpout;	/* swapouts */
	unsigned v_runq;	/* current length of run queue */
};


#ifdef	_KERNEL
#ifdef	__hp9000s800
extern	struct	vmmeter cnt, rate, sum;
#else	/* not __hp9000s800 */
/* ESC XXX fix this if not 800 then == 300 mentality.
 * this is wrong anyway w.r.t the extern's, n'est-ce pas ?? */
struct	vmmeter cnt, rate, sum;
#endif	/* else not __hp9000s800 */
#endif

/* systemwide totals computed every five seconds */
struct vmtotal
{
	unsigned int t_rq;	/* length of the run queue */
	unsigned int t_dw;	/* jobs in ``disk wait'' (neg priority) */
	unsigned int t_pw;	/* jobs in page wait */
	unsigned int t_sl;	/* jobs sleeping in core */
	unsigned int t_sw;	/* swapped out runnable/short block jobs */
	int	t_vm;		/* total virtual memory */
	int	t_avm;		/* active virtual memory */
	unsigned int t_rm;	/* total real memory in use */
	unsigned int t_arm;	/* active real memory */
	int	t_vmtxt;	/* virtual memory used by text */
	int	t_avmtxt;	/* active virtual memory used by text */
	unsigned int t_rmtxt;	/* real memory used by text */
	unsigned int t_armtxt;	/* active real memory used by text */
	unsigned int t_free;	/* free memory pages */
};
#ifdef	_KERNEL
#ifdef	__hp9000s800
extern	struct	vmtotal total;
#else	/* not __hp9000s800 */
struct	vmtotal total;
#endif	/* else not __hp9000s800 */
#endif

/*
 * Optional instrumentation.
 */
#ifdef PGINPROF

#define	NDMON	128
#define	NSMON	128

#ifdef __hp9000s300

#define	DRES	5
#define	SRES	2

#define	PMONMIN	10
#define	PRES	10
#define	NPMON	64

#define	RMONMIN	2
#define	RRES	1
#define	NRMON	64

#else  /* not __hp9000s300 */

#define	DRES	20
#define	SRES	5

#define	PMONMIN	20
#define	PRES	50
#define	NPMON	64

#define	RMONMIN	130
#define	RRES	5
#define	NRMON	64

#endif /* __hp9000s300 */

/* data and stack size distribution counters */
#ifdef	__hp9000s800
extern	unsigned int	dmon[NDMON+1];
extern	unsigned int	smon[NSMON+1];
#else	/* not __hp9000s800 */
unsigned int	dmon[NDMON+1];
unsigned int	smon[NSMON+1];
#endif	/* else not __hp9000s800 */

/* page in time distribution counters */
#ifdef	__hp9000s800
extern	unsigned int	pmon[NPMON+2];
#else
unsigned int	pmon[NPMON+2];
#endif

/* reclaim time distribution counters */
#ifdef	__hp9000s800
extern	unsigned int	rmon[NRMON+2];
#else	/* not __hp9000s800 */
unsigned int	rmon[NRMON+2];
#endif	/* else not __hp9000s800 */

#ifdef	__hp9000s800
extern	int	pmonmin;
extern	int	pres;
extern	int	rmonmin;
extern	int	rres;

extern	unsigned rectime;		/* accumulator for reclaim times */
extern	unsigned pgintime;		/* accumulator for page in times */
#else	/* not __hp9000s800 */
int	pmonmin;
int	pres;
int	rmonmin;
int	rres;

unsigned rectime;		/* accumulator for reclaim times */
unsigned pgintime;		/* accumulator for page in times */
#endif	/* else not __hp9000s800 */
#endif

#endif /* _SYS_VMMETER_INCLUDED */
