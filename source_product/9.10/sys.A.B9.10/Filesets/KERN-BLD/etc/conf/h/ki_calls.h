/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/ki_calls.h,v $
 * $Revision: 1.10.83.6 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/11 16:48:23 $
 */
/* HPUX_ID: @(#)ki_calls.h	54.1		88/11/13 */
#ifndef _SYS_KI_CALLS_INCLUDED /* allows multiple inclusion */
#define _SYS_KI_CALLS_INCLUDED

#if defined(__hp9000s300) || defined (__hp9000s700)
#define	KI_MAX_PROCS	1	/* XXX */
#else
#define	KI_MAX_PROCS	MAX_PROCS	/* XXX */
#endif	/* Multiuser */

/************************************************************************/
/*			KI function call values				*/
/************************************************************************/

/* leave room for old Kernel GPROF parameters */
#define	KI_ALLOC_TRACEMEM	10	/* Allocate sys memory for KI */
#define	KI_CONFIG_READ	 	11	/* Read config table of KI    */
#define	KI_TRACE_GET	 	12	/* Get a KI trace buffer      */
#define	KI_USER_TRACE	 	13	/* Put User record in trace   */
#define	KI_ALLOCATE_CT		14	/* Get CT memory from kernel  */
#define	KI_READ_CT	 	15	/* Read some CT memory        */
#define	KI_WRITE_CT	 	16	/* Write some CT memory       */
#define	KI_TIMEOUT_SET	 	17	/* Set max wait time for trace*/
#define	KI_PROC_TRACE	 	18	/* Put any record in trace buf*/
#define	KI_KMEM_GET	 	19	/* Read kernel memory         */

#define	KI_SET_SYSCALLTRACE	20	/* ON  one System call trace  */
#define	KI_CLR_SYSCALLTRACE	21	/* OFF one System call trace  */
#define	KI_SET_ALL_SYSCALTRACES 22	/* ON  all System call traces */
#define	KI_CLR_ALL_SYSCALTRACES	23	/* OFF all System call traces */

#define	KI_SET_KERNELTRACE	24	/* ON  one Kernel stub trace  */
#define	KI_CLR_KERNELTRACE	25	/* OFF one Kernel stub trace  */
#define	KI_SET_ALL_KERNELTRACES	26	/* ON  all Kernel stub traces */
#define	KI_CLR_ALL_KERNELTRACES	27	/* OFF all Kernel stub traces */

#define	KI_FREE_TRACEMEM	28	/* De-alloc sys memory for KI */
#define	KI_CONFIG_CLEAR		29	/* Clear KI counters          */
#define	KI_TRACE_SWITCH	 	30	/* Switch to a new trace buf  */

/* enable flags in the 	kc_kernelenable[KI_MAXKERNCALLS]; buffer      */
#define	KI_GETPRECTIME		0	/* enable on KI precise timer */
#define	KI_SYSCALLS		1	/* enable on system call trace*/
#define	KI_SERVESTRAT		2	/* enable verious kernel stubs*/
#define	KI_SERVSYNCIO		3
#define	KI_ENQUEUE		4
#define	KI_QUEUESTART		5
#define	KI_QUEUEDONE		6
#define	KI_HARDCLOCK		7
#define	KI_CLOSEF		8
#define	KI_BRELSE		9
#define	KI_GETNEWBUF		10
#undef	KI_SWAP		     /* 11 obsolete */
#define	KI_SWTCH		12
#define	KI_RESUME_NCS	     /* 13 obsolete */
#define	KI_RESUME_CSW		14	/* in locore.s */
#define	KI_HARDCLOCK_IDLE	15	/* primarily for debug */
#define	KI_SUIDPROC		16	/* from measurement process */
#define	KI_DM1_SEND		17	/* Discless send requests */
#define	KI_DM1_RECV		18	/* Discless receive requests */
#define	KI_RFSCALL		19	/* NFS server requeste */
#define	KI_RFS_DISPATCH		20	/* NFS client requests */
#define	KI_SETRQ		21	/* in locore.s */
#define	KI_DO_BIO		22	/* NFS driver to server */
#define	KI_USERPROC		23	/* from user process */
#define KI_MEMFREE		24	/* free page from region */
#define KI_ASYNCPAGEIO		25	/* pageing and swapping activity */
#define KI_LOCALLOOKUPPN	26	/* pathname locallookuppn */
#define	KI_VFAULT		27	/* Virtual page faults */
#define KI_MISS_ALPHA		28	/* Missed an alpha semaphore */
#define	KI_PREGION		29	/* Process pregion structures */
#define	KI_REGION		30	/* Process region structures */

/* Size of the KI kernel call table, only the above 26 are used */
#define KI_MAXKERNCALLS	50

/*
 * size of the KI system call table; this constant has to be reviewed
 * on every release to validate the total system call numbers
 */
#define	KI_MAXSYSCALLS	500
/*
 * The following are the fake system call numbers assigned to the 
 * kernel daemon processes that will never do a system call
*/
#define KI_SWAPPER	(KI_MAXSYSCALLS - 2)
#define KI_VHAND	(KI_MAXSYSCALLS - 3)
#define KI_STATDAEMON	(KI_MAXSYSCALLS - 4)
#define KI_SOCKREGD	(KI_MAXSYSCALLS - 5)
#define KI_NETISR	(KI_MAXSYSCALLS - 6)
#define KI_SYNCDAEMON	(KI_MAXSYSCALLS - 7)
#define KI_GCSP		(KI_MAXSYSCALLS - 8)
#define KI_LCSP		(KI_MAXSYSCALLS - 9)
#define KI_SPINUPD	(KI_MAXSYSCALLS -10)
#define KI_XPORTD	(KI_MAXSYSCALLS -11)
#define KI_UNHASHDAEMON	(KI_MAXSYSCALLS -12)
#define KI_VDMAD	(KI_MAXSYSCALLS -13)
#define KI_LVMKD	(KI_MAXSYSCALLS -14)

/* number of parts to the trace buffer - must be power of 2 in size   */
#define	NUM_TBUFS	4	/* number of trace buffers */

/* A convient way to access the ki_cf structure */

/* KI configuration specific information */
#define	ki_version		(ki_cf.kc_version)
#define ki_runningprocs         (ki_cf.kc_runningprocs)
#define	ki_trace_sz		(ki_cf.kc_trace_sz)	
#define ki_cbuf                 (ki_cf.kc_cbuf)
#define	ki_count_sz		(ki_cf.kc_count_sz)
#define ki_timeout		(ki_cf.kc_timeout)
#define ki_timeoutct		(ki_cf.kc_timeoutct)
#define	ki_nunit_per_sec 	(ki_cf.kc_nunit_per_sec)
#define	ki_tracing_info		(ki_cf.kc_tracing_info)
#define	ki_syscallenable	(ki_cf.kc_syscallenable)
#define	ki_kernelenable		(ki_cf.kc_kernelenable)

/* Processor specific information */
#define	ki_proc(I)		(ki_cf.ki_proc_info[I])

#define ki_tbuf(I)  		(ki_proc(I).kt_tbuf)
#define ki_next(I)	   	(ki_proc(I).kt_next)
#define ki_curbuf(I)        	(ki_proc(I).kt_curbuf)
#define ki_sequence_count(I) 	(ki_proc(I).kt_sequence_count)
#define ki_time(I)          	(ki_proc(I).kt_time)
#define ki_inckclk(I)       	(ki_proc(I).kt_inckclk)
#define ki_incmclk(I)       	(ki_proc(I).kt_incmclk)
#define	ki_syscallcounts(I)	(ki_proc(I).kt_syscallcounts)
#define	ki_kernelcounts(I)	(ki_proc(I).kt_kernelcounts)
#define	ki_syscall_times(I)	(ki_proc(I).kt_syscall_times)
#define	ki_offset_correction(I) (ki_proc(I).kt_offset_correction)
#define	ki_freq_ratio(I)	(ki_proc(I).kt_freq_ratio)

/* KTC specific information */
#define ki_timesstruct(I)   	(ki_proc(I).kt_timestruct)
#define ki_clockcounts(I,c)   	(ki_proc(I).kt_timestruct[c].kp_clockcnt)
#define ki_accumtm(I,c)   	(ki_proc(I).kt_timestruct[c].kp_accumtm)

#define	KI_CF	sizeof(struct ki_config)

/* Align on cacheline boundary				*/
#ifdef _KERNEL
#ifndef __hp9000s300
#pragma align 32
#endif	/* __hp9000s300 */
#endif /* _KERNEL */

struct	ki_config 
{
	char	kc_version[32];		/*32 version string			*/
	int	kc_runningprocs;	/* 4 number of active processors	*/
	u_int	kc_trace_sz;		/* 4 trace buffer size         		*/
	caddr_t kc_cbuf;                /* 4 allocated counter buffer size	*/
	u_int	kc_count_sz;		/* 4 counter (CT) buffer size       	*/
	u_int	kc_timeout;		/* 4 timeout in 1/HZ seconds   		*/
	u_int	kc_timeoutct; 		/* 4 timeout clock KI_TRACE_GET 	*/
	u_int	kc_nunit_per_sec;	/* 4 native units per second		*/
	int	kc_pad_[1];		/* 4 to make the below sum == 2**n	*/
/* 					  __					*/
/*  This sum should be a power of 2       64   or moved down to end of struct	*/

/*	It is preferable to have this structure start on a cache line		*/
/*	 Define a structure for general use per processor            		*/
	struct ki_proc_info 
	{	/* Relative rate of the CR_IT's from the monarch processor 	*/
		double	kt_freq_ratio;		/* 8 CR_IT ratio from monarch	*/
		struct	ki_timeval kt_time;	/* 8 This cpu's view of time	*/
		caddr_t	kt_tbuf[NUM_TBUFS];	/*16 allocated trace buffers ^	*/
		int	kt_next[NUM_TBUFS];	/*16 next avail byte of buffr N	*/
		u_int	kt_incmclk;		/* 4 CR_IT for last KTC clock 	*/
		u_int	kt_inckrc;		/* 4 CR_RC for last KTC clock	*/
		u_int	kt_offset_correction;	/* 4 add my_CR_IT=monarch_CR_IT	*/
		u_int	kt_inckclk;		/* 4 CR_IT for ki_getprectime()	*/
/* 					          __				*/
/*  This sum should be a power of 2               64   or moved  end of struct	*/


/* accumulated KERNEL (systemcall) time in behalf of process 	*/
#define	KT_SYS_CLOCK		0
/* accumulated USER time in behalf of process			*/
#define	KT_USR_CLOCK		1
/* accumulated CSW time (context switch) 		      	*/
#define	KT_CSW_CLOCK		2
/* accumulated IDLE time 				      	*/
#define	KT_IDLE_CLOCK		3
/* accumulated INT/USR interrupt time robbed from process     	*/
#define	KT_INTUSR_CLOCK		4
/* accumulated INT/IDLE interrupt time robbed from IDLE       	*/
#define	KT_INTIDLE_CLOCK	5
/* accumulated INT/SYS interrupt time robbed from process     	*/
#define	KT_INTSYS_CLOCK		6
/* accumulated virtual page fault time from process	      	*/
#define	KT_VFAULT_CLOCK		7
/* accumulated trap time from process			      	*/
#define	KT_TRAP_CLOCK		8
/* accumulated spare 0 time from process		      	*/
#define	KT_SPARE_CLOCK		9
/* accumulated spare 1 time from process		      	*/
#define	KT_SPARE1_CLOCK		10
/* accumulated spare 2 time from process		      	*/
#define	KT_SPARE2_CLOCK		11

#define	KT_NUMB_CLOCKS		12

/*	 Define a structure for KTC clock use per processor            		*/
		struct	ki_runtimes  
		{
			struct	ki_timeval	kp_accumtm;
			struct	ki_timeval	kp_accumin; /* as big counter   */
			u_int			kp_clockcnt;
		} kt_timestruct[KT_NUMB_CLOCKS];

		u_int	kt_curbuf;		/*  4 current buffer number	*/
		u_int	kt_sequence_count;	/*  4 tracing sequence count	*/

		/* counts of kernel routine call when enabled (above)         	*/
		u_int	kt_kernelcounts[KI_MAXKERNCALLS];

		/* counts of system calls when enabled (above) (see signal.h) */
		u_int	kt_syscallcounts[KI_MAXSYSCALLS];

		/* array to store accumulated system call times */
		struct	ki_timeval	kt_syscall_times[KI_MAXSYSCALLS];	

	} ki_proc_info [KI_MAX_PROCS];

	/* pointers to routines to handle system call traces         		*/
	/* NULL if this system call in not being traced              		*/
	caddr_t	kc_syscallenable[KI_MAXSYSCALLS];

	/* kernel routine enable flags - not == 0 when enabled       		*/
	u_char 	kc_kernelenable[KI_MAXKERNCALLS];
};

#ifdef _KERNEL
extern struct	ki_config	ki_cf;

/* 			Kernel Call trace stubs  	              		*/
#define	k_E   ki_kernelenable
#define	KI_getprectime(A)	if (k_E[KI_GETPRECTIME]) ki_getprectime(A)
#define KI_syscalltrace()       if (k_E[KI_SYSCALLS])	ki_syscalltrace()
#define	KI_servestrat(A,B,C)	if (k_E[KI_SERVESTRAT]) ki_servestrat(A,B,C)
#define	KI_servsyncio(A,B,C,D)	if (k_E[KI_SERVSYNCIO]) ki_servsyncio(A,B,C,D)
#define	KI_enqueue(A)		if (k_E[KI_ENQUEUE]) 	ki_enqueue(A)
#define	KI_queuestart(A)	if (k_E[KI_QUEUESTART]) ki_queuestart(A)
#define	KI_queuedone(A)		if (k_E[KI_QUEUEDONE]) 	ki_queuedone(A)

#ifdef  __hp9000s300
#define	KI_hardclock(A,B)				ki_hardclock(A,B)
#else /* not __hp9000s300 (i.e. 700 or 800) */
#define	KI_hardclock(A)					ki_hardclock(A)
#endif  /* __hp9000s300 */

#define	KI_brelse(A)		if (k_E[KI_BRELSE]) 	ki_brelse(A)
#define	KI_getnewbuf(A)		if (k_E[KI_GETNEWBUF]) 	ki_getnewbuf(A)
#define	KI_swtch(A)		if (k_E[KI_SWTCH]) 	ki_swtch(A)
#define	KI_resume_csw() 	if (k_E[KI_RESUME_CSW]) ki_resume_csw()
#define	KI_closef(A)		if (k_E[KI_CLOSEF]) 	ki_closef(A)
#define	KI_dm1_send(A,B)	if (k_E[KI_DM1_SEND]) 	ki_dm1_send(A,B)
#define	KI_dm1_recv(A,B)	if (k_E[KI_DM1_RECV]) 	ki_dm1_recv(A,B)
#define	KI_rfscall(A,B,C,D,E)	if (k_E[KI_RFSCALL]) 	ki_rfscall(A,B,C,D,E)
#define	KI_rfs_dispatch(A,B,C,D) if (k_E[KI_RFS_DISPATCH]) ki_rfs_dispatch(A,B,C,D)
#define	KI_setrq(A,B)		if (k_E[KI_SETRQ])	ki_setrq(A,B)
#define	KI_do_bio(A)		if (k_E[KI_DO_BIO])	ki_do_bio(A)
#define	KI_memfree(A,B)		if (k_E[KI_MEMFREE]) 	ki_memfree(A,B)
#define	KI_asyncpageio(A)	if (k_E[KI_ASYNCPAGEIO]) ki_asyncpageio(A)
#define	KI_locallookuppn(A,B,C,D) if (k_E[KI_LOCALLOOKUPPN]) ki_locallookuppn(A,B,C,D)
#endif /* _KERNEL */

#define	KI_PFAULTN	1
#define	KI_VFAULTN	2
#ifdef _KERNEL
#define	KI_vfault(A,B,C,D)	if (k_E[KI_VFAULT]) 	ki_vfault(A,B,C,D)
#define	KI_pfault(A,B,C,D)	if (k_E[KI_VFAULT]) 	ki_pfault(A,B,C,D)
#endif /* _KERNEL */

#define	KI_ALLOCREG	1
#define	KI_FREEREG	2
#define	KI_DUPREG	3
#define	KI_GROWREG	4
#ifdef _KERNEL
#define	KI_region(A,B)		if (k_E[KI_REGION]) 	ki_region(A,B)
#endif /* _KERNEL */

#define	KI_FREEPREG	1
#define	KI_DUPPREGION	2
#define	KI_ATTACHREG	3
#define	KI_GROWPREG	4
#ifdef _KERNEL
#define	KI_pregion(A,B)		if (k_E[KI_PREGION]) 	ki_pregion(A,B)

/* KI clock Related Conversion Macros */

#define KI_CLK_TOS_STACK_PTR		(u.ki_clk_tos_ptr)
#define	KI_CLK_BEGINNING_STACK_ADDRS	(&(u.ki_clk_stack[KI_CLK_STACK_SIZE-1]))
#define	KI_CLK_END_STACK_ADDRS		(&(u.ki_clk_stack[0]))

#endif /* _KERNEL */

/* KI Time Related Conversion Macros */

/* ki_timeval conversions */
#define	double_ki_timeval_to_sec(X) \
	(((double) (X).tv_sec) + double_nunit_to_sec((unsigned int)((X).tv_nunit)))
#define	double_ki_timeval_to_msec(X) \
	((((double) (X).tv_sec)*1000.0) + double_nunit_to_msec((unsigned int)((X).tv_nunit)))
#define	double_ki_timeval_to_usec(X) \
	(((double) (X).tv_sec)*1000000.0 + double_nunit_to_usec((unsigned int)((X).tv_nunit)))

/* timeval conversions */
#define double_timeval_to_sec(X) \
        (((double) (X).tv_sec) + ((double) (X).tv_usec)/1000000.0)
#define	double_timeval_to_msec(X) \
	((((double) (X).tv_sec)*1000.0) + (((double)(X).tv_usec)/1000.0))
#define	double_timeval_to_usec(X) \
	(((double) (X).tv_sec)*1000000.0 + (X).tv_usec)

/* nunit conversions */
#define	double_nunit_to_sec(A) \
	(((double) (A))/((double) ki_nunit_per_sec))

#ifdef	FAST_NUNIT_CONVERSION
#ifndef	_KERNEL
double ki_nunit_per_msec;
double ki_nunit_per_usec;
#endif	/* _KERNEL */

/* Must first call fast_nunit_conversion_init() at appropriate place */
/* (e.g. shortly after a call to ki_call(KI_CONFIG_READ))            */
#define	fast_nunit_conversion_init() \
	ki_nunit_per_msec = ((double) ki_nunit_per_sec)/1000.0; \
	ki_nunit_per_usec = ((double) ki_nunit_per_sec)/1000000.0
#define	double_nunit_to_msec(A) \
	(((double) (A))/ki_nunit_per_msec)
#define	double_nunit_to_usec(A) \
	(((double) (A))/ki_nunit_per_usec)
#else	/* FAST_NUNIT_CONVERSION */
#define	double_nunit_to_msec(A) \
	(((double) (A))/((double) ki_nunit_per_sec)*1000.0)
#define	double_nunit_to_usec(A) \
	(((double) (A))/((double) ki_nunit_per_sec)*1000000.0)
#endif	/* FAST_NUNIT_CONVERSION */

#endif /* _SYS_KI_CALLS_INCLUDED */
