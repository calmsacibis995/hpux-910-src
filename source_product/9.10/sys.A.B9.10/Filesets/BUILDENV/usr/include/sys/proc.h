/*
 * @(#)proc.h: $Revision: 1.68.83.6 $ $Date: 94/07/20 10:57:36 $
 * $Locker:  $
 *
 */

#ifndef _SYS_PROC_INCLUDED /* allows multiple inclusion */
#define _SYS_PROC_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#  else /* not _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#  endif /* not _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef __hp9000s300
#ifdef _KERNEL_BUILD
#include "../machine/pte.h"
#else /* ! _KERNEL_BUILD */
#include <machine/pte.h>
#endif /* _KERNEL_BUILD */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#ifdef _KERNEL_BUILD
#include "../h/fss.h"
#else /* ! _KERNEL_BUILD */
#include <sys/fss.h>
#endif /* _KERNEL_BUILD */
#endif /* __hp9000s800 */

#ifdef _KERNEL_BUILD
#include "../h/vas.h"
#include "../h/pregion.h"
#include "../h/time.h"
#include "../h/mman.h"
#else /* ! _KERNEL_BUILD */
#include <sys/vas.h>
#include <sys/pregion.h>
#include <sys/time.h>
#include <sys/mman.h>
#endif /* _KERNEL_BUILD */

/* Values for vfork_state field in struct vforkinfo */

#define VFORK_INIT      0
#define VFORK_PARENT    1
#define VFORK_CHILDRUN  2
#define VFORK_CHILDEXIT 3
#define VFORK_BAD       4

/*
 * The following structure is used by vfork to hold state while a
 * vfork is in progress.
 */

struct vforkinfo {
    int vfork_state;
    struct proc *pprocp;
    struct proc *cprocp;
    unsigned long buffer_pages;
    unsigned long u_and_stack_len;
#ifdef __hp9000s300
    unsigned char *u_and_stack_addr;
#endif
#ifdef __hp9000s800
    unsigned long saved_rp_ptr;
    unsigned long saved_rp;
#endif
    unsigned char *u_and_stack_buf;
    struct vforkinfo *prev;
};

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
typedef struct	proc {
	struct	proc *p_link;	/* linked list of running processes */
	struct	proc *p_rlink;
	u_char	p_usrpri;	/* user-priority based on p_cpu and p_nice */
	u_char	p_pri;		/* priority, lower numbers are higher pri */
	u_char  p_rtpri;        /* real time priority */
	char	p_cpu;		/* cpu usage for scheduling */
	char	p_stat;
	char	p_nice;		/* nice for cpu usage */
	char	p_cursig;
	int	p_sig;		/* signals pending to this process */
	int	p_sigmask;	/* current signal mask */
	int	p_sigignore;	/* signals being ignored */
	int	p_sigcatch;	/* signals being caught by user */
	int	p_flag;         /* see flag defines below */
	int	p_flag2;	/* more flags; see below */
	int     p_coreflags;    /* core file options; see core.h */
#ifdef _CLASSIC_ID_TYPES
	u_short p_filler_uid;
	u_short	p_uid;		/* user id, used to direct tty signals */
#else
	uid_t	p_uid;		/* user id, used to direct tty signals */
#endif
#ifdef _CLASSIC_ID_TYPES
	u_short p_filler_suid;
	u_short	p_suid;		/* set (effective) uid */
#else
	uid_t	p_suid;		/* set (effective) uid */
#endif
#ifdef _CLASSIC_ID_TYPES
	u_short p_filler_pgrp;
	short	p_pgrp;		/* name of process group leader */
#else
	gid_t	p_pgrp;		/* name of process group leader */
#endif
#ifdef _CLASSIC_ID_TYPES
	u_short p_filler_pid;
	short	p_pid;		/* unique process id */
#else
	pid_t	p_pid;		/* unique process id */
#endif
#ifdef _CLASSIC_ID_TYPES
	u_short p_filler_ppid;
	short	p_ppid;		/* process id of parent */
#else
	pid_t	p_ppid;		/* process id of parent */
#endif
	caddr_t p_wchan;	/* event process is awaiting */
	size_t	p_maxrss;	/* copy of u.u_limit[MAXRSS] */
	u_short	p_cpticks;	/* ticks of cpu time */
	long    p_cptickstotal; /* total for life of process */
	float	p_pctcpu;	/* %cpu for this process during p_time */
	short	p_idhash;	/* hashed based on p_pid for kill+exit+... */
	short	p_pgrphx;	/* pgrp hash index */
	short   p_uidhx;	/* uid hash index */
	short	p_fandx;	/* free/active proc structure index */
	short	p_pandx;	/* previous active proc structure index */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	struct	proc *p_cptr;	/* pointer to youngest living child */
	struct	proc *p_osptr;	/* pointer to older sibling processes */
	struct	proc *p_ysptr;	/* pointer to younger siblings */
	struct	proc *p_dptr;	/* pointer to debugger, if not parent */
	vas_t	*p_vas;		/* Virtual address space for process */
	preg_t  *p_upreg;	/* Pointer to pregion containing U area */
        ushort  p_mpgneed;      /* number of memory pages needed */
	struct  proc *p_mlink;		/* link list of processes    */
					/* sleeping on memwant or    */
					/* swapwant.	  	     */
	short	p_memresv;	/* # pages reserved by this proc */
	short	p_swpresv;	/* # pages reserved by swapper this proc */
	u_short p_xstat;	/* exit stauts */
	char	p_time;		/* resident time for scheduling */
	char	p_slptime;	/* time since last block */
	short	p_ndx;
	struct	itimerval p_realtimer;
	sid_t	p_sid;		/* session ID */
	short	p_sidhx;	/* session ID hash index */
	short	p_idwrite;	/* process ident write flag for auditing */
	struct fss *p_fss;      /* fair share group pointer */
	struct dbipc *p_dbipcp; /* dbipc pointer */
	u_char	p_wakeup_pri;	/* priority when proc awakens on semaphore */
	u_char	p_reglocks;	/* num reglock()'s held (see vm_sched.c) */
				/* VASSERTS in region.h know this is 1 byte */
	caddr_t p_filelock;	/* address of file lock region process is
			           either blocked on or about to block on.*/
	/* Doubly linked list of processes sharing the same controlling tty.
	 * Head of list is u.u_procp->p_ttyp->t_cttyhp.
	 */
        struct proc *p_cttyfp;  /* forward ptr */
	struct proc *p_cttybp;  /* backward ptr */
	caddr_t p_dlchan;	/* Process deadlock channel		*/
	site_t  p_faddr;	/* Process forwarding address		*/
	/* Fields used by the pstat system call. */
	struct timeval
		p_utime,
		p_stime;
	dev_t          p_ttyd;
	time_t         p_start;

	struct tty *p_ttyp;	/* controlling tty pointer */
	int	p_wakeup_cnt;	/* generic counter, wakeup when goes to 0 */
#ifdef MP
#ifdef SYNC_SEMA_RECOVERY
	sema_t	*p_recover_sema; /* Semaphore to recover on exit from sleep */
#endif
	int	p_descnt;	/* proc desire age */
	int	p_desproc;	/* processor desired  */
	int	p_mpflag;	/* mp flag */
	int	p_procnum;	/* Processor it ran on, just for user info */
#endif	/* MP */
	struct proc *p_wait_list;	/* Forward link for wait list */
	struct proc *p_rwait_list;	/* Backward link for wait list */

	struct sema   *p_sleep_sema;	/* semaphore process is blocked on */
	struct sema   *p_sema;	/* alpha: head of per-process semaphore list */

	/* These fields have been moved from user.h because you can no
         * longer retreive this information from a uarea which has been
         * swapped out.
         */
        int     p_maxof;                /* max number of open files allowed */
        struct  vnode *p_cdir;          /* current directory */
        struct  vnode *p_rdir;          /* root directory of current process */
        struct  ofile_t **p_ofilep;     /* pointers to file descriptor chunks
                                           to be allocated as needed. */


	struct vforkinfo *p_vforkbuf;   /* Vfork state information pointer */
	struct msem_procinfo *p_msem_info; /* Pointer to msemaphore info struct */

/* All workstation specific fields */
#ifdef _WSIO
	/* support for dil interrupts */
	struct buf *p_dil_event_f; /* head of list of pending dil interrupts */
	struct buf *p_dil_event_l; /* tail of list of pending dil interrupts */
	struct pte *p_addr;     /* u-area kernel map address */
	struct ste *p_segptr;   /* physical segment table pointer */
	int     p_stackpages;   /* Number of private kernel stack pages */
	u_char  p_dil_signal;   /* which signal to use for DIL interrupts */
#endif /* _WSIO */


#ifdef __hp9000s300
/* Only the 300 uses these time fields in this manner */
#define p_uticks        p_utime.tv_sec
#define p_sticks        p_stime.tv_sec

#endif /* __hp9000s300 */

/* All 800 specific fields */
#ifdef __hp9000s800
	u_short p_pindx;        /* index of this proc table entry */
#	ifdef _WSIO
    	    caddr_t graf_ss;    /* graphics per-process (mostly coproc) data */
#	endif
#ifdef PESTAT

/* the following added to measure the time the kernel executes before
 * allowing preemption by a higher priority process.  (Dave Lennert 5/9/84)
 */
	struct	timeval intr_icstime; /* accumulated ics time of interval */
	struct	timeval intr_time; /* start time of interval */
	int	intr_trace;	/* unique id of kernel stack trace at start */
	int	intr_callid;	/* no. of syscall/trap being executed */
				/* (a trap no. if INTR_TRAP added in) */
#define INTR_TRAP 1000000
#define INTR_ICS  2000000
#endif /* PESTAT */
#endif /* __hp9000s800 */
} proc_t;

#ifdef _KERNEL

extern int PIDHMASK;
extern int PGRPHMASK;
extern int UIDHMASK;
extern int SIDHMASK;

/* The following hashes rely on the hash table size being a power of two. */
#define PIDHASH(pid)    ((pid) & PIDHMASK)
#define PGRPHASH(pgrp)  ((pgrp) & PGRPHMASK)
#define UIDHASH(uid)    ((uid) & UIDHMASK)
#define SIDHASH(sid)	((sid) & SIDHMASK)

/* Some basic link macros for the different hash chains */
#define ulink(p,ruid)	{\
			register int hash = UIDHASH( ruid );\
			(p)->p_uidhx = uidhash[hash]; \
			uidhash[hash] = (pindx(p));\
			}
#define plink(p,pid)    {\
			register int hash = PIDHASH( pid );\
			(p)->p_idhash = pidhash[hash]; \
			pidhash[hash] = (pindx(p));\
			}
#define glink(p,pgrp)	{\
			register int s = UP_SPL6();\
			register int hash = PGRPHASH( pgrp );\
			MP_ASSERT(owns_spinlock(sched_lock), "glink failure");\
			(p)->p_pgrphx = pgrphash[hash]; \
			pgrphash[hash] = (pindx(p));\
			UP_SPLX(s);\
			}
#define slink(p,sid)	{\
			register int s = UP_SPL6();\
			register int hash = SIDHASH( sid );\
			MP_ASSERT(owns_spinlock(sched_lock), "slink failure");\
			(p)->p_sidhx = sidhash[hash]; \
			sidhash[hash] = (pindx(p));\
			UP_SPLX(s);\
			}

extern	short	pidhash[];
extern	short	pgrphash[];	 /* Process group headr hash array */
extern	short	sidhash[];	 /* Session id headr hash array */
extern	short	uidhash[];	 /* User id headr hash array */
extern	short	freeproc_list; 		 /* headr of the free proc table slot*/
					 /* chain */
extern	struct	proc *pfind();
extern  struct  proc *proc, *procNPROC;  /* the proc table itself */
extern	int	nproc;

#ifdef __hp9000s800
#define	NQS	160		/* 160 run queues = 128 RT + 32 TS */
#define NQSPEL   16             /* Number of run queues per whichqs element*/
#define NQSPELLG 4		/* log2(NQSPEL)*/
#define NQELS    (NQS/NQSPEL)     /*  10 elements to hold bitmask(whichqs) */
#define TSQ     128		/* First time-sharing queue */
#define TSPRI_TO_RUNQ(pri)      (TSQ + (((pri)-PTIMESHARE) >> 2))
#else  /* not __hp9000s800 */
#define	NQS	256		/* 256 run queues 128 RT + 128 TS */
#define NQSPEL   32             /* Number of run queues per whichqs element */
#define NQELS   (NQS/NQSPEL)	/* 8 32-bit elements to hold bitmask(whichqs) */
#define TSPRI_TO_RUNQ(pri)      (pri)   /* Don't use anywhere but schedcpu! */
#endif /* not __hp9000s800 */

struct	prochd {
	struct	proc *ph_link;	/* linked list of running processes */
	struct	proc *ph_rlink;
};

extern	struct	prochd qs[NQS];
extern	int	whichqs[NQELS];	/* bit mask summarizing non-empty qs's */
#endif	/* _KERNEL */

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes (p_flag) */
#define	SLOAD	0x00000001	/* in core */
#define	SSYS	0x00000002	/* swapper or pager process */
#define	SLOCK	0x00000004	/* process being swapped out */
#define	STRC	0x00000008	/* process is being traced */
#define	SWTED	0x00000010	/* another tracing flag */
#define	SSCT	0x00000020	/* system call tracing */
#define	SKEEP	0x00000040	/* another flag to prevent swap out */
#define	SOMASK	0x00000080	/* restore old mask after taking signal */
#define	SWEXIT	0x00000100	/* working on exiting */
#define	SPHYSIO	0x00000200	/* doing physical i/o (bio.c) */
#define SVFORK	0x00000400	/* Vfork in process */
#define	SSEQL	0x00000800	/* user warned of sequential vm behavior */
#define	SUANOM	0x00001000	/* user warned of random vm behavior */
#define	SOUSIG	0x00002000	/* using old signal mechanism */
#define	SOWEUPC	0x00004000	/* owe process an addupc() call at next ast */
#define	SSEL	0x00008000	/* selecting; wakeup/waiting danger */
#define SRTPROC 0x00010000	/* real time processes */
#define SSIGABL 0x00020000	/* signalable process */
#define SPRIV	0x00040000	/* compute privilege mask */
#define	SPREEMPT 0x00080000	/* Preemption flag */
#ifdef HPNSE
#define SPOLL   0x00100000      /* process is polling */
#endif

#ifdef _WSIO
/* more p_flag bits, used for process deactivation */
#define SSTOPFAULTING 0x00200000
#define SSWAPPED 0x00400000
#define SFAULTING 0x00800000

/* used to track number of faulting processes (not a p_flag bit) */
#define FAULTCNTPERPROC 8
#endif /* _WSIO */

#define SUMEM   0x01000000

/* flags for p_flag2 */
#define S2CLDSTOP	0x00000001	/* send SIGCLD for stopped processes */
#define S2EXEC		0x00000002	/* if bit set, process has completed 
					   an exec(OS) call */
#define SGRAPHICS       0x00000004      /* The process is a graphics process */
#define SADOPTIVE	0x00000008	/* process adopted using ptrace */
#ifdef __hp9000s800
#define SSAVED		0x00000010	/* registers saved for ptrace */
#define SCHANGED	0x00000020	/* registers changed by ptrace */
#define SPURGE_SIDS	0x00000100	/* purge cr12 and cr13 in resume() */
#endif /* __hp9000s800 */
#ifdef __hp9000s300
#define S2DATA_WT       0x00000010      /* Process's data segment is write through */
#define S2STACK_WT      0x00000020      /* Process's stack segment is write through */
#endif /* __hp9000s300 */
#define SANYPAGE	0x00000040	/* Doing any kind of pageing */
#define SPA_ON		0x00000080	/* Under consideration for 
					   activation control */
#define S2POSIX_NO_TRUNC  0x00001000  /* no truncate flag for pathname lookup*/
#define POSIX_NO_TRUNC  S2POSIX_NO_TRUNC /* until dux_sdo.c is fixed */

#ifdef _WSIO
#define S2SENDDILSIG	0x00000200 /* whether to send DIL interrupt (cleared on fork) */
#endif /* _WSIO */
#define SLKDONE  	0x00000400    /* Process has done lockf() or fcntl()  */
#define SISNFSLM  	0x00000800    /* Process is NFS lock manager. */
				      /* See nfs_fcntl() in nfs_server.c */

#define S2TRANSIENT 0x00002000  /* transient flag (fair share scheduler) */

#ifdef MP
/* These are p_mpflag values */
#define SLPT		0x00000001 	/* a Lower Priv Transfer trap brought us in */
#define	SRUNPROC	0x00000002	/* Running on a processor */
#define	SMPLOCK		0x00000004	/* Locked  */
#define SMP_SEMA_WAKE	0x00000008	/* proc awakened by V operation, 
					   not signal */
#define	SMP_STOP	0x00000010	/* Process entering stopped state. */
#define SMP_SEMA_BLOCK	0x00000020	/* Process blocked on semaphore */
#define SMP_SEMA_NOSWAP 0x00000040	/* Do not swap this process */
#endif /* MP */

#ifdef __hp9000s300
#define PROCFLAGS2 (SADOPTIVE|S2EXEC|S2SENDDILSIG)
#endif
#ifdef __hp9000s800
#define PROCFLAGS2 (SADOPTIVE|S2EXEC|SCHANGED|SSAVED|S2TRANSIENT)
#endif

/* Constants which are used to call newproc */
#define FORK_PROCESS	1
#define FORK_VFORK	2
#define FORK_DAEMON	3

/* Return values for newproc/procdup */
#define FORKRTN_PARENT	0
#define FORKRTN_CHILD	1
#define FORKRTN_ERROR	-1

/* Constants which can be used to index proc table for kernel daemons*/
#define	S_SWAPPER	0 
#define S_INIT		1
#define S_PAGEOUT	2
#define S_STAT		3
#define S_DONTCARE	-1


/* Constants which can be used for pid argument to newproc() */
/* Note: proc table slot and pid may be different for some processes   */

#define PID_SWAPPER     0
#define PID_INIT        1
#define PID_PAGEOUT     2
#define PID_STAT	3
#define PID_LCSP        4
#define PID_NETISR      5
#define	PID_SOCKREGD	6
#define PID_VDMAD       7
#define PID_MAXSYS      7  /* Used in dux/getpid.c */

struct pidprc {
		short pp_pid;
		u_int pp_prc;
};

#define get_pidprc(pp) \
	(pp)->pp_pid = u.u_procp->p_pid; 

#define get_pidprc2(arg1, arg2) \
	(arg1)->pp_pid = (arg2)->p_pid;

#define eq_pidprc(p1, p2) ((p1)->pp_pid == (p2)->pp_pid)

extern proc_t *allocproc();

#if defined(__hp9000s800) && defined(_KERNEL)
/* The hppa compiler must emit calls to division millicode to compute
 * ((p) - proc), since the size of the proc structure is
 * no longer an exact power of two.  Since this type of operation
 * occurs on sensitive paths (context switch and page fault), and
 * since the cost of doing the millicode is relatively large,
 * (~100 instruction cycles), we will do the inelegant thing and
 * precompute indices.
 */
#define pindx(p) ((p)->p_pindx)
#else /* not __hp9000s800 && _KERNEL */
#define pindx(p) ((p) - proc)
#endif /* not __hp9000s800 && _KERNEL */

#define make_unswappable() { \
	u_int context;  \
	SPINLOCK_USAV(sched_lock, context); \
	u.u_procp->p_flag |= SKEEP; \
	SPINUNLOCK_USAV(sched_lock, context); \
	}

#define make_swappable() { \
	u_int context;  \
	SPINLOCK_USAV(sched_lock, context); \
	u.u_procp->p_flag &= ~SKEEP; \
	SPINUNLOCK_USAV(sched_lock, context); \
	}

#endif /* _SYS_PROC_INCLUDED */
