/* $Header: resource.h,v 1.13.83.6 93/10/21 13:11:30 dkm Exp $ */

#ifndef _SYS_RESOURCE_INCLUDED
#define _SYS_RESOURCE_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/time.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/time.h>
#endif /* _KERNEL_BUILD */

/*
 * Resource limits
 */

struct rlimit {
	int	rlim_cur;		/* current (soft) limit */
	int	rlim_max;		/* maximum value for rlim_cur */
};


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
   extern int getrlimit(int, struct rlimit *);
   extern int setrlimit(int, const struct rlimit *);
#else /* not _PROTOTYPES */
   extern int getrlimit();
   extern int setrlimit();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */



/*
 * Process priority specifications to get/setpriority.
 */

#define	PRIO_MIN	-20
#define	PRIO_MAX	20

#define	PRIO_PROCESS	0
#define	PRIO_PGRP	1
#define	PRIO_USER	2

/*
 * Resource utilization information.
 */

#define	RUSAGE_SELF	0
#define	RUSAGE_CHILDREN	-1

struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
					/* actual values kept in proc struct */
	long	ru_maxrss;
#define	ru_first	ru_ixrss
	long	ru_ixrss;		/* integral shared memory size */
	long	ru_idrss;		/* integral unshared data " */
	long	ru_isrss;		/* integral unshared stack " */
	long	ru_minflt;		/* page reclaims */
	long	ru_majflt;		/* page faults */
	long	ru_nswap;		/* swaps */
	long	ru_inblock;		/* block input operations */
	long	ru_oublock;		/* block output operations */
	long	ru_ioch;		/* # of characters read/written */
	long	ru_msgsnd;		/* messages sent */
	long	ru_msgrcv;		/* messages received */
	long	ru_nsignals;		/* signals received */
	long	ru_nvcsw;		/* voluntary context switches */
	long	ru_nivcsw;		/* involuntary " */
#define	ru_last		ru_nivcsw
};

/*
 * Within the kernel, time information is stored as ticks rather than as
 * seconds plus microseconds.  This makes normal updates more efficient
 * and eliminates race conditions while updating.  The seconds field is
 * used to store ticks and the microseconds field is unused.
 */

/*
 * The information is converted to seconds plus microseconds when given
 * to the user via the getrusage and wait3 system calls.
 */

#ifdef __hp9000s300
#define ru_sticks	ru_stime.tv_sec
#define ru_uticks	ru_utime.tv_sec

#ifdef _KERNEL
#define	ru_ticks_to_timeval(tv)	(tv).tv_usec = (tv).tv_sec % hz * tick, \
				(tv).tv_sec /= hz
#endif /* _KERNEL */
#endif /* __hp9000s300 */

/*
 * Resource limits
 */
#ifdef _KERNEL	/* unsupported BSD stuff */
#define	RLIMIT_CPU	0		/* cpu time in milliseconds */
#define	RLIMIT_FSIZE	1		/* maximum file size */
#endif	

#define	RLIMIT_DATA	2		/* data size */
#define	RLIMIT_STACK	3		/* stack size */
#define	RLIMIT_CORE	4		/* core file size */
#define	RLIMIT_RSS	5		/* resident set size */

#define RLIMIT_NOFILE   6               /* maximum number of open files */
#define RLIMIT_OPEN_MAX RLIMIT_NOFILE   /* maximum number of open files */
#define	RLIM_NLIMITS	7		/* number of resource limits */

#define	RLIM_INFINITY	0x7fffffff

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_RESOURCE_INCLUDED */
