/* @(#) $Revision: 1.12.83.4 $ */
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/mman.h,v $
 * $Revision: 1.12.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:29:52 $
 */
#ifndef _SYS_MMAN_INCLUDED /* allows multiple inclusion */
#define _SYS_MMAN_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#  else /* not _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#  endif /* not _KERNEL_BUILD */
#endif /* _SYS_STDSYMS_INCLUDED  */

#ifndef _SYS_TYPES_INCLUDED
#  ifdef _KERNEL_BUILD
#    include "../h/types.h"
#  else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#  endif /* _KERNEL_BUILD */
#endif /* _SYS_TYPES_INCLUDED */

/* protections are chosen from these bits, or-ed together */

#define PROT_NONE       0x0
#define PROT_USER       PROT_NONE
#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define PROT_EXECUTE	0x4
#define PROT_EXEC	PROT_EXECUTE
#define PROT_KERNEL	0x8

#define PROT_URWX	(PROT_USER|PROT_WRITE|PROT_READ|PROT_EXECUTE)
#define PROT_URX	(PROT_USER|PROT_READ|PROT_EXECUTE)
#define PROT_URW	(PROT_USER|PROT_WRITE|PROT_READ)
#define PROT_KRW	(PROT_KERNEL|PROT_READ|PROT_WRITE)

/* sharing types: choose either SHARED or PRIVATE */
/* address placement: choose either VARIABLE or FIXED */
/* mapping type: choose either ANONYMOUS or FILE */
#define	MAP_SHARED	0x01		/* share changes */
#define	MAP_PRIVATE	0x02		/* changes are private */
#define MAP_FIXED	0x04		/* fixed mapping */
#define MAP_VARIABLE	0x08		/* variable mapping (default) */
#define MAP_ANONYMOUS	0x10		/* map from anonymous file -- zero */
#define MAP_FILE	0x20		/* map from file (default) */

/* advice to madvise */
#define	MADV_NORMAL	0		/* no further special treatment */
#define	MADV_RANDOM	1		/* expect random page references */
#define	MADV_SEQUENTIAL	2		/* expect sequential page references */
#define	MADV_WILLNEED	3		/* will need these pages */
#define	MADV_DONTNEED	4		/* dont need these pages */
#define	MADV_SPACEAVAIL	5		/* insure that resources are reserved */

/* flags to msync() */
#define MS_SYNC		0x01		/* wait until I/O is complete */
#define MS_ASYNC	0x02		/* perform I/O asynchronously */
#define MS_INVALIDATE	0x04		/* invalidate pages after I/O done */

/* Constants for msem_init, msem_lock, and msem_unlock */

#define MSEM_UNLOCKED   0x00            /* init semaphore in unlocked state */
#define MSEM_LOCKED     0x01            /* init semaphore in locked state */

#define MSEM_IF_NOWAIT  0x01            /* do not wait for locked semaphore */

#define MSEM_IF_WAITERS 0x01            /* don't unlock if there are waiters */

/* Definition of msemaphore structure */

	/*
	 * Note that on Series 700/800, a ldcws is done on the
	 * mcas_lock field, which means that the field must be
	 * 16 byte aligned.
	 */

typedef struct msemaphore {
#ifdef __hp9000s800
	unsigned long mcas_lock; /* used by _mcas() on Series 700/800 only */
#else
	unsigned long spare;   /* pad so <page size>%sizeof(msemaphore)==0 */
#endif
	unsigned long magic;   /* value to indicate semaphore is valid */
	unsigned long locker;  /* 0 if unlocked, otherwise "id" of locker */
	unsigned long wanted;  /* 0 if not wanted, 1 if wanted */
} msemaphore;

#define __MSEM_MAGIC 0x6d73656d /* msemaphore magic number. Hex for "msem" */

/*
 * return values for _mcas()
 */

#define __MCAS_OK        0 /* Mcas was successful */
#define __MCAS_NOLOCKID  1 /* No msemaphore lock id allocated for process */
#define __MCAS_PFAULT    2 /* page fault */
#define __MCAS_BUSY      3 /* can't get lock */
#define __MCAS_ALIGN     4 /* semaphore address is not 16 byte aligned */

#ifdef _PROTOTYPES
    extern msemaphore *msem_init(msemaphore *, int);
    extern int msem_remove(msemaphore *);
    extern int msem_lock(msemaphore *, int);
    extern int msem_unlock(msemaphore *, int);
    extern int madvise(const caddr_t, size_t, int);
    extern caddr_t mmap(const caddr_t, size_t, int, int, int, off_t);
    extern int mprotect(const caddr_t, size_t, int);
    extern int msync(const caddr_t, size_t, int);
    extern int munmap(const caddr_t, size_t);
#else /* not _PROTOTYPES */
    extern msemaphore *msem_init();
    extern int msem_remove();
    extern int msem_lock();
    extern int msem_unlock();
    extern int madvise();
    extern caddr_t mmap();
    extern int mprotect();
    extern int msync();
    extern int munmap();
#endif /* not _PROTOTYPES */

#ifdef _KERNEL

/*
 * Note that the current msem_hash() algorithm is tailored
 * to the fact that MSEM_HASH_SIZE == 16. If it is changed,
 * msem_hash() should be changed also.
 */

#define MSEM_HASH_SIZE 16


/*
 * There is one msem_procinfo structure for each process that uses
 * the msemaphore routines. It is allocated when the process calls
 * sysconf to allocate its msemaphore lock id.
 */

struct msem_procinfo {
	unsigned long lockid;   /* Msemaphore lock id for this process */
	struct msem_procinfo *msemp_next; /* Next proc using semaphores */
	struct msem_procinfo *msemp_prev; /* Prev proc using semaphores */
	struct msem_info     *msem_waitptr; /* Non-null if waiting */
	struct msem_info     *msem_locklist; /* List of semaphores locked by
						this proc, that have waiters */
};

/*
 * There is one msem_info structure for each semaphore that has waiters.
 */

struct msem_info {
	struct msem_procinfo *msem_locker;    /* ptr to locker (if waiters) */
	struct region   *msem_reg;   /* Region for semaphore */
	unsigned long   msem_offset; /* Region offset for semaphore */
	unsigned long   wait_count;  /* Number of waiters */
	struct msem_info *hash_next; /* Ptr to next struct for same hash */
	struct msem_info *lock_next; /* Ptr to next struct for this proc */
};

#endif /* _KERNEL */

#endif /* _SYS_MMAN_INCLUDED */
