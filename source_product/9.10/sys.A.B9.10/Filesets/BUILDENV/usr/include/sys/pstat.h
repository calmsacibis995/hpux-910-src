/* $Header: pstat.h,v 1.8.83.4 93/09/17 18:32:40 kcs Exp $ */

#ifndef _SYS_PSTAT_INCLUDED
#define _SYS_PSTAT_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#include "../h/param.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#include <sys/param.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifndef _SIZE_T
#  define _SIZE_T
   typedef unsigned int size_t;
#endif /* _SIZE_T */

/*
 * Header organization (ESCXXX delete this eventually)
 *
 * ANSI function prototypes for the new interface
 * Structures for old interface
 *	old sub structures
 *	old structures
 *	old related constants
 * Structures for new interface
 *	new sub structures,
 *	new structures
 *	new related constants
 * PSTAT_XXX sub-function constants
 * ANSI function prototypes for the old interface
 *	pstun union
 */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
    int pstat_getproc(struct pst_status *, size_t, size_t, int);
    int pstat_getstatic(struct pst_static *, size_t, size_t, int);
    int pstat_getdynamic(struct pst_dynamic *, size_t, size_t, int);
    int pstat_getvminfo(struct pst_vminfo *, size_t, size_t, int);
    int pstat_getdisk(struct pst_diskinfo *, size_t, size_t, int);
    int pstat_getprocessor(struct pst_processor *, size_t, size_t, int);
    int pstat_getlv(struct pst_lv *, size_t, size_t, int);
    int	pstat_getswap(struct pst_swapinfo *, size_t, size_t, int);
#else /* not _PROTOTYPES */
    int pstat_getproc();
    int pstat_getstatic();	/* should these fwd decs be under !KERNEL ?*/
    int pstat_getdynamic();	/* ESCXXX */
    int pstat_getvminfo();
    int pstat_getdisk();
    int pstat_getprocessor();
    int pstat_getlv();
    int pstat_getswap();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/*
 * function codes to the pstat() system call
 *
 * These defines, part of the existing pstat(2) interface will remain.
 * New subcodes will be added after these.
 */
#define PSTAT_PROC	1
#define	PSTAT_STATIC	2
#define	PSTAT_DYNAMIC	3
#define PSTAT_SETCMD	4
#define PSTAT_VMINFO	5
#define PSTAT_DISKINFO	6
#define PSTAT_RESERVED1	7	/*
#define PSTAT_RESERVED2	8	 * These 3 subcodes reserved for expansion
#define PSTAT_RESERVED3	9	 */
#define PSTAT_PROCESSOR	10
#define PSTAT_LVINFO	11
#define PSTAT_SWAPINFO	12

#define PST_CLEN 64	/* PS command line length cached */
#define PST_UCOMMLEN (MAXCOMLEN + 1)	/* u_comm field cached */

#define PST_MAX_CPUSTATES 15   /* Max number of cpu states
                                * that we can return info about.
                                */

#define PST_MAX_PROCS 32	/* Max number of processors in
				 * system.  Used to define sizes
				 * of per-processor structures.
				 */

/* An abstraction of major/minor numbers of a dev */
struct psdev {
    long psd_major;
    long psd_minor;
};



/*
 * This structure contains per-process information
 */
struct pst_status {
	long pst_idx;	/* Index for further pstat() requests */
	long pst_uid;	/* UID */
	long pst_pid;	/* Process ID */
	long pst_ppid;	/* Parent process ID */
	long pst_dsize;	/* # real pages used for data */
	long pst_tsize;	/* # real pages used for text */
	long pst_ssize;	/* # real pages used for stack */
	long pst_nice;	/* Nice value */
	struct psdev	/* TTY of this process; -1/-1 if there isn't one */
	    pst_term;
	long pst_pgrp;	/* Process group of this process */
	long pst_pri;	/* priority of process */
	long pst_addr;	/* address of process (in memory) */
	long pst_cpu;	/* processor utilization for scheduling */
	long pst_utime;	/* user time spent executing (in seconds) */
	long pst_stime;	/* system time spent executing (in seconds) */
	long pst_start;	/* time process started (seconds since epoch) */
	long pst_flag;	/* flags associated with process */
	long pst_stat;	/* Current status */
	long pst_wchan;	/* If state PS_SLEEP, value sleeping on */
	long pst_procnum;	/* processor this proc last run on */
	char		/* Command line for the process, if available */
	    pst_cmd[PST_CLEN];
	long pst_time;	/* resident time for scheduling */
	long pst_cpticks;	/* ticks of cpu time */
	long pst_cptickstotal; /* total ticks for life of process */
	long pst_fss;	/* fair share scheduler group id */
	float pst_pctcpu;	/* %cpu for this process during p_time */
	long pst_rssize;    /* resident set size for process (private pages) */
	long pst_suid;	/* saved UID */
	char		/* executable basename the process is running */
	    pst_ucomm[PST_UCOMMLEN];
	long pst_shmsize;	/* # real pages used for shared memory */
	long pst_mmsize;	/* # real pages used for memory mapped files */
	long pst_usize;		/* # real pages used for U-Area & K-Stack */
	long pst_iosize;	/* # real pages used for I/O device mapping */
	long pst_vtsize;	/* # virtual pages used for text */
	long pst_vdsize;	/* # virtual pages used for data */
	long pst_vssize;	/* # virtual pages used for stack */
	long pst_vshmsize;	/* # virtual pages used for shared memory */
	long pst_vmmsize;	/* # virtual pages used for mem-mapped files */
	long pst_vusize;	/* # virtual pages used for U-Area & K-Stack */
	long pst_viosize;	/* # virtual pages used for I/O dev mapping */
	unsigned long pst_minorfaults;	/* # page reclaims for the process */
	unsigned long pst_majorfaults;	/* # page faults needing disk access */
	unsigned long pst_nswap;	/* # of swaps for the process */
	unsigned long pst_nsignals;	/* # signals received by the process */
	unsigned long pst_msgrcv;	/* # socket msgs received by the proc*/
	unsigned long pst_msgsnd;	/* # of socket msgs sent by the proc */
	long pst_maxrss;	/* highwater mark for proc resident set size */
};

#define	pst_major	pst_term.psd_major
#define	pst_minor	pst_term.psd_minor

/* Process states for pst_stat */
#define PS_SLEEP 1	/* Sleeping on pst_wchan value */
#define PS_RUN 2	/* Running/waiting for CPU */
#define PS_STOP 3	/* Stopped for ptrace() */
#define PS_ZOMBIE 4	/* Dead, waiting for parent to wait() */
#define PS_IDLE 6       /* Being created */
#define PS_OTHER 5	/* Misc. state (forking, exiting, etc.) */

/* Process flag bits for pst_flag */
#define PS_INCORE 0x1	/* this process is in memory */
#define PS_SYS    0x2	/* this process is a system process */
#define PS_LOCKED 0x4	/* this process is locked in memory */
#define PS_TRACE  0x8	/* this process is being traced */
#define PS_TRACE2 0x10	/* this traced process has been waited for */ 

/*
 * This structure contains static system information -- that will
 * remain the same (at least) until reboot
 */
struct pst_static {
	long	       max_proc;
	struct psdev   console_device;
	long           boot_time;
	long           physical_memory;
	long           page_size;
	long           cpu_states;
	long           pst_status_size;
	long           pst_static_size;
	long           pst_dynamic_size;
	long	       pst_vminfo_size;
	long           command_length;
	long           pst_processor_size;
	long           pst_diskinfo_size;
	long           pst_lvinfo_size;
	long           pst_swapinfo_size;
};

/*
 * This structure contains dynamic system variables, ones which may
 * change frequently during normal operation of the kernel.
 */
struct pst_dynamic {
	long           psd_proc_cnt;	/* MP: number of active processors */
	long           psd_max_proc_cnt; /* MP: max active processors */
	long	       psd_last_pid;	/* last run process ID */
	long           psd_rq;		/* run queue length */	
	long           psd_dw;		/* jobs in disk wait */
	long           psd_pw;		/* jobs in page wait */
	long           psd_sl;		/* jobs sleeping in core */
	long           psd_sw;		/* swapped out runnable jobs */
	long           psd_vm;		/* total virtual memory */
	long           psd_avm;		/* active virtual memory */
	long           psd_rm;		/* total real memory */
	long           psd_arm;		/* active real memory */
	long           psd_vmtxt;	/* virt mem text */
	long           psd_avmtxt;	/* active virt mem text */
	long           psd_rmtxt;	/* real mem text */
	long           psd_armtxt;	/* active real mem text */
	long           psd_free;	/* free memory pages */
	double         psd_avg_1_min;	/* global run queue lengths */
	double         psd_avg_5_min;
	double         psd_avg_15_min;
					/* global cpu time/state */
	long           psd_cpu_time[PST_MAX_CPUSTATES];
					/* per-processor run queue lengths */
	double         psd_mp_avg_1_min[PST_MAX_PROCS];
	double         psd_mp_avg_5_min[PST_MAX_PROCS];
	double         psd_mp_avg_15_min[PST_MAX_PROCS];
					/* per-processor cpu time/state */
	long           psd_mp_cpu_time[PST_MAX_PROCS][PST_MAX_CPUSTATES];
	long           psd_openlv;	/* # of open Logical Volumes */
	long           psd_openvg;	/* # of open LV Volume groups */
	long           psd_allocpbuf;	/* # of allocated LV pvol buffers */
	long           psd_usedpbuf;	/* # of LV pvol buffers in used */
	long           psd_maxpbuf;	/* max # of LV pvol buffers avail. */
	/*
	 * Note that psd_activeprocs, psd_activeinodes, and psd_activefiles
	 * were added for sar(1), etc. support.
	 */
	long           psd_activeprocs; /* # of active proc  table entries */
	long           psd_activeinodes;/* # of active inode table entries */
	long           psd_activefiles; /* # of active file  table entries */
};

/*
 * Macro to perform bounds check on # of active table entries; 
 * sar(1), etc. support. 
 */
#define PSTAT_CHECKBOUNDS(numentrys, maxentries) { \
	if (numentrys < 0L) \
		numentrys = 0L; \
	else if (numentrys > ((long)maxentries)) \
		numentrys = (long)maxentries; \
}

/*
 * This structure contains VM-related system variables
 */
struct pst_vminfo {
	long	psv_rdfree;	/* rate:	 pages freed by daemon */
	long	psv_rintr;	/* device interrupts */
	long	psv_rpgpgin;	/* pages paged in */
	long	psv_rpgpgout;	/* pages paged out */
	long	psv_rpgrec;	/* total page reclaims */
	long	psv_rpgtlb;	/* tlb flushes - 800 only  */
	long	psv_rscan;	/* scans in pageout daemon */
	long	psv_rswtch;	/* context switches */
	long	psv_rsyscall;	/* calls to syscall() */
	long	psv_rxifrec;	/* found in freelist rather than in filesys */
	long	psv_rxsfrec;	/* found in freelist rather than on swapdev */
	long	psv_cfree;	/* cnt:		free memory pages */
	long	psv_sswpin;	/* sum:		swapins */
	long	psv_sswpout;	/* swapouts */
	long	psv_sdfree;	/* pages freed by daemon */
	long	psv_sexfod;	/* pages filled on demand from executables */
	long	psv_sfaults;	/* total faults taken */
	long	psv_sintr;	/* device interrupts */
	long	psv_sintrans;	/* intransit blocking page faults */
	long	psv_snexfod;	/* number of exfod's created */
	long	psv_snzfod;	/* number of zero filled on demand */
	long	psv_spgfrec;	/* page reclaims from free list */
	long	psv_spgin;	/* pageins */
	long	psv_spgout;	/* pageouts */
	long	psv_spgpgin;	/* pages paged in */
	long	psv_spgpgout;	/* pages paged out */
	long	psv_spswpin;	/* pages swapped in */
	long	psv_spswpout;	/* pages swapped out */
	long	psv_srev;	/* revolutions of the hand */
	long	psv_sseqfree;	/* pages taken from sequential programs */
	long	psv_sswtch;	/* context switches */
	long	psv_ssyscall;	/* calls to syscall() */
	long	psv_strap;	/* calls to trap */
	long	psv_sxifrec;	/* found in free list rather than in filesys */
	long	psv_sxsfrec;	/* found on free list rather than on swapdev*/
	long	psv_szfod;	/* pages zero filled on demand */
	long	psv_sscan;	/* scans in pageout daemon */
	long	psv_spgrec;	/* total page reclaims */
	long	psv_deficit;	/* estimate of needs of new swapped-in procs */
	long	psv_tknin;	/* number of characters read from ttys */
	long	psv_tknout;	/* number of characters written to ttys */
	long    psv_cntfork;	/* number of forks */
	long    psv_sizfork;	/* number of pages forked */
	unsigned long    psv_lreads;	/* number of disk blk reads issued */
	unsigned long    psv_lwrites;	/* number of disk blk writes issued */
	unsigned long	psv_swpocc;	/* # of times swrq occ'd since boot */
	unsigned long	psv_swpque;	/* cumulative len of swrq since boot */
};

/*
 * This structure contains per-disk information.
 *  Each structure returned describes one disk
 *  see also <sys/dk.h>
 */

struct pst_diskinfo {
	long	psd_idx;	/* Index for further pstat() requests */
	struct psdev psd_dev;	/* device specification for the disk */
				/* for 800, minor field is zero -- describes
				 * what type of disk only.
				 */
	long	psd_dktime;	/* */
	long	psd_dkseek;	/* */
	long	psd_dkxfer;	/* */
	long	psd_dkwds;	/* */
	float	psd_dkmspw;	/* */
};

/*
 * This structure contains per-logical volume information.
 * Each structure returned describes one logical volume.
 */

struct pst_lvinfo {
	unsigned long	psl_idx;	/* Index for further pstat() requests*/
	struct psdev psl_dev;		/* device specification for the vol */
	unsigned long	psl_rxfer;	/* # of reads */
	unsigned long	psl_rcount;	/* # of bytes read */
	unsigned long	psl_wxfer;	/* # of writes */
	unsigned long	psl_wcount;	/* # of bytes written */
	unsigned long	psl_openlv;	/* Number of opened LV's in this LV's LVG */
	unsigned long	psl_mwcwaitq;	/* Length of LV's LVG
				 * mirror write consistency cache (MWC)
				 */
	unsigned long	psl_mwcsize;	/* Size of LV's LVG's MWC */
	unsigned long	psl_mwchits;	/* # of hits to the LV's LVG's MWC */
	unsigned long	psl_mwcmisses;	/* # of misses to the LV's LVG's MWC */
};

/*
 * This structure describes per-processor information.
 * Each structure returned describes one processor on a multi-processor
 * system.  (A total of one structure for uni-processor machines)
 */

struct pst_processor {
	unsigned long	 psp_idx;	/* Index of the current spu in the
					 * array of processor statistic entries
					 */
	unsigned long	psp_fsreads;	/* # of reads from filesys blocks. */
	unsigned long	psp_fswrites;	/* # of writes to filesys blocks. */
	unsigned long	psp_nfsreads;	/* # of nfs disk blk reads issued. */
	unsigned long	psp_nfswrites;	/* # of nfs disk blk writes issued. */
	unsigned long	psp_bnfsread;	/* # of bytes read from NFS. */
	unsigned long	psp_bnfswrite;	/* # of bytes written to NFS. */
	unsigned long	psp_phread;	/* # of physical reads to raw devs. */
	unsigned long	psp_phwrite;	/* # of physical writes to raw devs. */
	unsigned long	psp_runocc;	/* # of times the processor had
					 * processes waiting to run.  This
					 * running total is updated once
					 *a second.
					 */
	unsigned long	psp_runque;	/* # of processes the processor had
					 * waiting to run.  This running total
					 * is updated once a second.
					 */
	unsigned long	psp_sysexec;	/* # of exec system calls. */
	unsigned long	psp_sysread;	/* # of read system calls. */
	unsigned long	psp_syswrite;	/* # of write system calls. */
	unsigned long	psp_sysnami;	/* # of calls to sysnami(). */
	unsigned long	psp_sysiget;	/* # of calls to sysiget(). */
	unsigned long	psp_dirblk;	/* # of filesystem blocks read doing
					 * directory lookup.
					 */
	unsigned long	psp_semacnt;	/* # of System V semaphore ops. */
	unsigned long	psp_msgcnt;	/* # of System V message ops. */
	unsigned long	psp_muxincnt;	/* # of MUX interrupts received. */
	unsigned long	psp_muxoutcnt;	/* # of MUX interrupts sent. */
	unsigned long	psp_ttyrawcnt;	/* # of raw characters read. */
	unsigned long	psp_ttycanoncnt; /* # of canonical chars processed. */
	unsigned long	psp_ttyoutcnt;	/* # of characters output. */
};

struct pss_blk {
	struct psdev	Pss_dev;	/* Device specification */
	unsigned long	Pss_start;	/* For 300,700: starting blk */
	unsigned long	Pss_nblks;	/* total # of blocks */
};

struct pss_fs {
	unsigned long	Pss_allocated;	/* # of blocks curr. avail. */
	unsigned long	Pss_min;	/* min # of blocks to alloc. */
	unsigned long	Pss_limit;	/* max # of blocks to alloc. */
	unsigned long	Pss_reserve;	/* # of blocks to reserve */
	char		Pss_mntpt[256];	/* FS mount point path */
};
	
/*
 * This structure describes per-swap-area information.
 * Each structure returned describes one "pool" of swap space on the system,
 * either a block device or a portion of a filesystem.
 */
struct pst_swapinfo {
	unsigned long	pss_idx;	/* Idx for further pstat() requests */
	unsigned long	pss_flags;	/* flags associated with swap pool */
	unsigned long	pss_priority;	/* priority of the swap pool */
	unsigned long	pss_nfpgs;	/* # of free pages of space in pool */
	union {				/* block and fs swap differ */
		struct pss_blk Pss_blk;	/* Block device Fields */
		struct pss_fs  Pss_fs;	/* File System Fields */
	} pss_un;
};

#define pss_dev		pss_un.Pss_blk.Pss_dev
#define pss_start	pss_un.Pss_blk.Pss_start
#define pss_nblks	pss_un.Pss_blk.Pss_nblks

#define pss_allocated	pss_un.Pss_fs.Pss_allocated
#define pss_min		pss_un.Pss_fs.Pss_min
#define pss_limit	pss_un.Pss_fs.Pss_limit
#define pss_reserve	pss_un.Pss_fs.Pss_reserve
#define pss_mntpt	pss_un.Pss_fs.Pss_mntpt

#define pss_major	pss_dev.psd_major
#define pss_minor	pss_dev.psd_minor

/*
 * Swap info for pss_flags
 */
#define SW_ENABLED	0x1
#define SW_BLOCK	0x2
#define SW_FS		0x4

/*
 * ============ Beginning of old pstat interface spec ===========
 */

/* old union for buf argument */
union pstun {
	struct pst_static	*pst_static;
	struct pst_dynamic	*pst_dynamic;
	struct pst_status	*pst_status;
	char			*pst_command;
};

/* old function prototype */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
    int pstat(int, union pstun, size_t, size_t, int);
#else /* not _PROTOTYPES */
    int pstat();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */

/*
 * ============ End of old pstat interface spec ================
 */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_PSTAT_INCLUDED */

