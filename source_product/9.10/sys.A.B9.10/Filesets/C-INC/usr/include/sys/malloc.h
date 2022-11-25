/*
 * @(#)malloc.h: $Revision: 1.11.83.9 $ $Date: 94/09/14 19:34:48 $
 * $Locker:  $
 */

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)malloc.h	7.7 (Berkeley) 6/27/88
 */

#ifndef _SYS_MALLOC_INCLUDED /* allows multiple inclusion */
#define	_SYS_MALLOC_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#include "../h/debug.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/vfd.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#include <sys/debug.h>
#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/vfd.h>
#endif /* _KERNEL_BUILD */

#define MAXALLOCSAVE (NBPG)	/* Stop using powers-of-two at a page */
#define MAXBUCKETSAVE (PGSHIFT)	/* Stop using powers-of-two at a page */
#ifdef __hp9000s300
#define MINBUCKET  (4)		/* Minimum allocation size--16 bytes */
#else
#define MINBUCKET  (5)		/* Minimum allocation size--32 bytes */
#endif
#define MAXBUCKET (MINBUCKET+15)/* Max allocation size */

/*
 * flags to malloc
 */
#define M_WAITOK	0x0000
#define M_NOWAIT	0x0001
#define M_ALIGN		0x0002
#define M_EXECUTE	0x0004
#define M_NORESV	0x0008

/*
 * Types of memory to be allocated
 */
#define	M_FREE		0	/* should be on free list */
#define	M_MBUF		1	/* network memory buffer */
#define	M_DYNAMIC	2	/* dynamic (data) allocation */
#define	M_HEADER	3	/* packet header */
#define	M_SOCKET	4	/* socket structure */
#define	M_PCB		5	/* protocol control block */
#define	M_RTABLE	6	/* routing tables */
#define	M_HTABLE	7	/* IMP host tables */
#define	M_ATABLE	8	/* address resolution tables */
#define	M_SONAME	9	/* socket name */
#define	M_SOOPTS	10	/* socket options */
#define	M_FTABLE	11	/* fragment reassembly header */
#define	M_RIGHTS	12	/* access rights */
#define	M_IFADDR	13	/* interface address */
#define	M_NTIMO		14	/* network timeouts */
#define M_DEVBUF	15	/* device driver memory */
#define	M_ZOMBIE	16	/* zombie proc status */
#define M_NAMEI		17	/* namei path name buffer */
#define M_GPROF		18	/* kernel profiling buffer */
#define M_IOCTLOPS	19	/* ioctl data buffer */
#define M_SUPERBLK	20	/* super block data */
#define M_CRED		21	/* credentials */
#define M_TEMP		22	/* misc temporary data buffers */
#define M_VAS		23	/* VAS structure */
#define M_PREG		24	/* Pregion structure */
#define M_REG		25	/* Region structure */
#define M_IOSYS		26	/* I/O system structure */
#define	M_NIPCCB	27	/* Netipc control block */
#define	M_NIPCSR	28	/* Netipc socket registry name record */
#define	M_NIPCPR	29	/* Netipc path report */
#define	M_DQUOTA 	30	/* Disk Quotas Cache entries */
#define M_DMA		31	/* Snakes I/O dma services */
#define M_GRAF		32	/* Non-ITE graphics */
#define M_ITE		33	/* ITE (Internal Terminal Emulator) */
#define M_ATL		34	/* S300 attach list entries */
#define M_LOCKLIST	35	/* Locklist entry */
#define M_DBC		36	/* Related to dynamic buffer cache */
#define M_LOSTPAGE      37      /* VM copy avoidance lost page list entries */
#define M_COWARGS       38      /* VM copy-on-write reference descriptions */
#define M_SWAPMAP       39      /* VM system swap maps */
#define M_TRACE		40	/* system call tracing buffers */
#define M_VFD		41	/* VFDs/DBDs */
#define M_KMEM_ALL	42	/* callers of kmem_alloc() */
#define M_UMEMD         43	/* umem driver */
#define M_CDNODE	44	/* cdnode table for CD-ROM fs */
#define M_VFD2		45	/* kmalloced chunks of VFD/DBDs */
#define M_LAST          46


	/*
	 * If adding new types and changing M_LAST, remember to modify
	 * sys/vm_misc.c and analyze/standard/analyze.c, as well.
	 */


#define btokup(va) ((struct kmemusage *)PAGE_INFO((caddr_t)va))

struct kmemstats {
	int	ks_flags;	/* Flags */
	int	(*ks_check)();
	int	ks_sleep;	/* Some one is waiting for this bucket */
	long	ks_reslimit;	/* Total reserved bytes allowed */
	long	ks_resinuse;	/* Total reserved bytes in use */
#ifdef OSDEBUG
	long	ks_inuse;	/* # of bucket entries (of all sizes) of */
                                /* this type currently in use */
	long	ks_calls;	/* total # of bucket entries (of all sizes) */
                                /* of this type ever allocated */
	long 	ks_memuse;	/* total memory held in bytes */
	long	ks_maxused;	/* maximum number ever used */
#ifdef LATER
	u_short	ks_limblocks;	/* number of times block for hitting limit */
	u_short	ks_mapblocks;	/* number of times blocked for kernel map */
	long	ks_limit;	/* most that are allowed to exist */
#endif /* LATER */
#endif /* OSDEBUG */
};

#define KS_RESERVED 0x01

/*
 * Array of descriptors that describe the contents of each page
 */
struct kmemusage {
	u_short	ku_indx;		/* bucket index */
	u_short	ku_pagecnt;	/* for large allocations, */
				/* pages allocated, else 0 */
};

/*
 * Set high order bit in a short.
 */
#define RESERVED_POOL (1 << 15)
#define MAXPGCNT (~RESERVED_POOL)

/*
 * Set of buckets for each size of memory block that is retained
 */
struct kmembuckets {
	caddr_t kb_next;	/* list of free blocks */
	long	kb_calls;	/* total calls to allocate this size */
	long	kb_total;	/* total number of pages allocated to a */
	                        /* bucket (even if some elements are free) */
	long	kb_totalfree;	/* # of free elements in this bucket */
#ifdef LATER
	long	kb_elmpercl;	/* # of elements in this sized allocation */
	long	kb_highwat;	/* high water mark */
	long	kb_couldfree;	/* over high water mark and could free */
#endif /* LATER */
};

#ifdef _KERNEL

caddr_t	kmalloc();
caddr_t	kmem_alloc();

#define MINALLOCSIZE	(1 << MINBUCKET)
#define BUCKETINDX(size) \
	((size) <= (MINALLOCSIZE * 128) \
		? (size) <= (MINALLOCSIZE * 8) \
			? (size) <= (MINALLOCSIZE * 2) \
				? (size) <= (MINALLOCSIZE * 1) \
					? (MINBUCKET + 0) \
					: (MINBUCKET + 1) \
				: (size) <= (MINALLOCSIZE * 4) \
					? (MINBUCKET + 2) \
					: (MINBUCKET + 3) \
			: (size) <= (MINALLOCSIZE* 32) \
				? (size) <= (MINALLOCSIZE * 16) \
					? (MINBUCKET + 4) \
					: (MINBUCKET + 5) \
				: (size) <= (MINALLOCSIZE * 64) \
					? (MINBUCKET + 6) \
					: (MINBUCKET + 7) \
		: (size) <= (MINALLOCSIZE * 2048) \
			? (size) <= (MINALLOCSIZE * 512) \
				? (size) <= (MINALLOCSIZE * 256) \
					? (MINBUCKET + 8) \
					: (MINBUCKET + 9) \
				: (size) <= (MINALLOCSIZE * 1024) \
					? (MINBUCKET + 10) \
					: (MINBUCKET + 11) \
			: (size) <= (MINALLOCSIZE * 8192) \
				? (size) <= (MINALLOCSIZE * 4096) \
					? (MINBUCKET + 12) \
					: (MINBUCKET + 13) \
				: (size) <= (MINALLOCSIZE * 16384) \
					? (MINBUCKET + 14) \
					: (MINBUCKET + 15))

/*
 * Macro versions for the usual cases of kmalloc/kfree
 * Be sure that modifications are made to both the
 * OSDEBUG and non-OSDEBUG versions.
 */
#ifdef OSDEBUG
#define MALLOC(space, cast, size, type, flags) { \
	struct kmembuckets *kbp = &bucket[BUCKETINDX(size)]; \
	register int context; \
	VASSERT(type < M_LAST); \
	SPINLOCK_USAV(kmemlock, context); \
	if ((size > MAXALLOCSAVE) || (kbp->kb_next == NULL)) { \
		SPINUNLOCK_USAV(kmemlock, context); \
		(space) = (cast)kmalloc((u_long)(size), type, flags); \
	} else { \
		struct kmemstats *ksp = &kmemstats[type]; \
		VASSERT(BUCKETINDX(size) <= MAXBUCKET); \
		(space) = (cast)kbp->kb_next; \
		kbp->kb_next = *(caddr_t *)(space); \
		kbp->kb_calls++; \
		kbp->kb_totalfree--; \
		ksp->ks_inuse++; \
		ksp->ks_calls++; \
		ksp->ks_memuse += 1 << BUCKETINDX(size); \
		ksp->ks_maxused = MAX(ksp->ks_maxused, ksp->ks_memuse); \
		SPINUNLOCK_USAV(kmemlock, context); \
	} \
}
#else /* not OSDEBUG */
#define MALLOC(space, cast, size, type, flags) { \
	struct kmembuckets *kbp = &bucket[BUCKETINDX(size)]; \
	register int context; \
	SPINLOCK_USAV(kmemlock, context); \
	if ((size > MAXALLOCSAVE) || (kbp->kb_next == NULL)) { \
		SPINUNLOCK_USAV(kmemlock, context); \
		(space) = (cast)kmalloc((u_long)(size), type, flags); \
	} else { \
		(space) = (cast)kbp->kb_next; \
		kbp->kb_next = *(caddr_t *)(space); \
		SPINUNLOCK_USAV(kmemlock, context); \
	} \
}
#endif /* OSDEBUG */

/* Use this macro when "size" is compile time known else use vapor_malloc() */
#ifdef	OSDEBUG
#define VAPOR_MALLOC(space, cast, size, type, flags) \
{ \
	struct	Vm { \
		struct	Vm	*v_link; \
		int		 v_type; }; \
	MALLOC((space), cast, ((size)+sizeof(struct Vm)), (type), (flags)); \
	if (space) { \
		((struct Vm *)(space))->v_link = (struct Vm *)u.u_vapor_mlist; \
		((struct Vm *)(space))->v_type = type; \
		u.u_vapor_mlist = (caddr_t)(space); \
		(struct Vm *)(space) += 1; \
	} \
}
#else /* not OSDEBUG */
#define VAPOR_MALLOC(space, cast, size, type, flags) \
{ \
	struct	Vm { \
		struct	Vm	*v_link; }; \
	MALLOC((space), cast, ((size)+sizeof(struct Vm)), (type), (flags)); \
	if (space) { \
		((struct Vm *)(space))->v_link = (struct Vm *)u.u_vapor_mlist; \
		u.u_vapor_mlist = (caddr_t)(space); \
		(struct Vm *)(space) += 1; \
	} \
}
#endif /* OSDEBUG */

extern void kfree();

#ifdef OSDEBUG
#define FREE(x, type) { \
	struct kmemusage *kup = btokup((caddr_t)x); \
	VASSERT(type < M_LAST); \
	VASSERT(kalloc_aligned((unsigned long)x)); \
	if (kup->ku_indx > MAXBUCKETSAVE) { \
		kfree((caddr_t)(x), type); \
	} else { \
		register int context;		\
		struct kmembuckets *kbp = &bucket[kup->ku_indx]; \
		struct kmemstats *ksp = &kmemstats[type]; \
		VASSERT(kup->ku_pagecnt == 0); \
		SPINLOCK_USAV(kmemlock, context); \
		*(caddr_t *)(x) = kbp->kb_next; \
		kbp->kb_next = (caddr_t)(x); \
		ksp->ks_inuse--; \
		ksp->ks_memuse -= 1 << kup->ku_indx; \
		kbp->kb_totalfree++; \
		SPINUNLOCK_USAV(kmemlock, context); \
	} \
}
#else /* not OSDEBUG */
#define FREE(x, type) { \
	struct kmemusage *kup = btokup((caddr_t)x); \
	if (kup->ku_indx > MAXBUCKETSAVE) { \
		kfree((caddr_t)(x), type); \
	} else { \
		register int context;		\
		struct kmembuckets *kbp = &bucket[kup->ku_indx]; \
		SPINLOCK_USAV(kmemlock, context); \
		*(caddr_t *)(x) = kbp->kb_next; \
		kbp->kb_next = (caddr_t)(x); \
		SPINUNLOCK_USAV(kmemlock, context); \
	} \
}
#endif /* OSDEBUG */

extern struct kmembuckets bucket[];
extern struct kmembuckets reserved_bucket[];
extern struct kmemstats kmemstats[];

extern vm_lock_t kmemlock;

/*
 * Defines for calling kalloc (low level memory allocator).
 */
#define KALLOC_NOWAIT	0x01
#define KALLOC_NOZERO	0x02
#define KALLOC_NORESV	0x04

extern caddr_t kalloc();

#endif /* _KERNEL */
#endif /* _SYS_MALLOC_INCLUDED */
