/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_mallo.c,v $
 * $Revision: 1.9.83.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/03/03 12:28:30 $
 */

/*
 * BEGIN_DESC 
 *
 *   File:
 *	kern_mallo.c
 *
 *   Purpose:
 *	Implements the kernel memory allocator for getting kernel
 *	memory in chunks less than a page size.  It can also 
 *	allocate whole pages.
 *
 *   Interface:
 *	MALLOC(space, cast, size, type, flags) in malloc.h
 *	FREE(space, type)  in malloc.h
 *	kmalloc(size, type, flags)
 *	kfree(addr, type)
 *	kmalloc_reserve(type, size, count, func)
 *	kfree_unused()
 *
 *   Theory:
 *
 * The Mckusick Memory allocator is a general purpose kernel memory allocator.
 * It can either allocate pieces of a page or it can allocate whole pages.  
 * The caller is required to return the same virtual address given by
 * MALLOC to the FREE macro.  The allocator works by rounding requests into
 * a power of 2, and breaking a page into that many pieces.  The MALLOC
 * macro is expanded within the calling routine to check that an entry of
 * the appropriate power of 2 is available and to give this entry to the
 * requester.  If the size field of the macro is passed an expression that
 * cpp can compute, the MALLOC macro will turn into a rather simple 
 * set of instructions:
 *	spinlock
 *		if (bucket_non_empty) {
 *			bucket = next_entry
 *			spinunlock
 *			return(entry);
 *		}
 *		get_more_entries
 *	spinunlock
 *
 * If on the other hand, the size field is not a constant expression
 * the in-line code expansion can be very large.  In that case, it might
 * be prefferable to use kmalloc and kfree directly.
 *
 * Currently memory that is returned to the allocator is not returned to
 * the primary system free pool.  It is possible to add such a feature
 * in the future; however, we have not seen this as a real problem in
 * a running system.  
 * I have added this feature for IND.  kfree_unused() will free all
 * whole pages it can.  cwb
 *
 *
 * The allocator also supports the creation of small reserved pools of 
 * memory for each type (M_???).  The pool is limited in the total amount
 * of memory that can be reserved, as well as, the amount that any type
 * can reserve.  The limits exists to prevent subsystem from creating
 * large pools of memory that are never used.  The private pools should
 * only be used by code that can not afford to wait and can not afford
 * to get an error.  Typically, this is code that is running on the 
 * interrupt stack.
 *
 * END_DESC
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
 *	@(#)kern_malloc.c	7.10 (Berkeley) 6/29/88
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/sema.h"
#include "../h/malloc.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/mbuf.h"		/* When dux_mbuf_res is moved remove this */
#include "../dux/cct.h"

extern int mainentered;
extern caddr_t kalloc();

/*
 * Reserved pools are bad because they tie up memory for a single
 * subsystem.  We place an upper bound limit on the amount of
 * memory the system will commit to being reserved.  See the
 * PMVM team to change these constants!
 */
#define TOTAL_TYPE	15 /* Each type can have a max of 15 pages. */
#define TOTAL_RESERVED	30 /* The system limit on reserved pages is 30 */

struct kmembuckets bucket[MAXBUCKET+1];
struct kmembuckets reserved_bucket[MAXBUCKET+1];
struct kmemstats kmemstats[M_LAST];

int total_reserved = 0;

vm_lock_t kmemlock;
caddr_t alloc_reserve();

/*
 * BEGIN_DESC 
 *
 * kmeminit()
 *
 * Input Parameters: None.
 *
 * Output Parameters: None.
 *
 * Return Value: None.
 *
 * Globals Referenced:
 *	kmemlock
 *
 * Description:
 *	Initialize the global kernel memory allocator.
 *
 * Algorithm:
 *	First check that certain defines are correct.
 *		If not, fail to compile.
 *	Initialize the memory allocator spinlock.
 *
 * In/Out conditions: None.
 *
 * END_DESC
 */
kmem_init()
{
	kmeminit();
}

kmeminit()
{
#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < NBPG)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif
	vm_initlock(kmemlock, KMEMLOCK_ORDER, "kernel memory lock");
}

/*
 * BEGIN_DESC 
 *
 * kmalloc()
 *
 * Return Value:
 *	Null - no memory was available and no wait was specified.
 *	Non Null - A pointer to memory of at least size bytes.
 *
 * Globals Referenced:
 *	bucket
 *	reserved_bucket
 *	kmemstats
 *	kmem_info
 *
 * Description:
 *
 *	Allocate memory of at least size bytes and return a pointer to
 *	it.
 *
 * Algorithm:
 *	The main free pool is first checked to see if 
 *	free memory exists.  If so, the memory is broken
 *	into chunks and placed on the appropriate bucket
 *	entry.  If no memory exists, the allocator will
 *	check if a reserved pool exists and allocate
 *	memory from the reserved pool.  Each type of
 *	memory can have its own reserved pool.
 *
 * In/Out conditions: 
 *		This routine is called on the ICS.		      
 *
 *
 * END_DESC
 */
/*ARGSUSED*/
caddr_t
kmalloc(size, type, flags)
	u_long size;		/* Size of request in bytes */
	int type; 	   	/* Type of memory (M_???) */
	int flags;		/* Wait or not to wait */
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
        register u_int context;
	struct kmemstats *ksp;
	u_short indx;
	caddr_t va;

#ifdef OSDEBUG
	/*
	 * make sure that if the caller said we can wait, we actually
	 * can.
	 */
	if ((flags & M_NOWAIT) == 0) {
		if (mainentered)
			VASSERT(!ON_ISTACK);
	}
#endif
	indx = BUCKETINDX(size);
	kbp = &bucket[indx];
	ksp = &kmemstats[type];

        SPINLOCK_USAV(kmemlock, context);
	if ((size > MAXALLOCSAVE) || (kbp->kb_next == NULL)) {
		long allocsize;
		u_long npg;
		caddr_t cp, endva;

		/*
		 * Bucket index is empty or request size is 
		 * greater than a whole page.  Allocate the
		 * pages requested and either break up the
		 * page or return the allocated pages.
		 */
		SPINUNLOCK_USAV(kmemlock, context);
		if (size > MAXALLOCSAVE)
			allocsize = ptob(btorp(size));
		else
			allocsize = 1 << indx;
		npg = btorp(allocsize);

		/*
		 * Allocate memory.  If we can't afford to wait then we
		 * fail the allocation if we can't get memory from the
		 * system pool.  Each type of memory (M_???) has the 
		 * ability to reserve a pool of memory pages for that 
		 * type.  If the main memory allocator is out of pages,
		 * we will try the reserved pool.
		 *
		 * If the M_NOWAIT flag is not set, then we call kalloc
		 * without the KALLOC_NOWAIT flag.  If the N_NOWAIT flag
		 * is set then we attempt to use the reserved pool if
		 * it exists
		 *
		 * Note: no spinlock needs to be held while accessing 
		 * ks_flag because the KS_RESERVED field is set once
		 * and never cleared.
		 */
		if ((flags&M_NOWAIT) || (ksp->ks_flags&KS_RESERVED)) {
			if (flags&M_NORESV)
				va = kalloc(npg, KALLOC_NORESV|KALLOC_NOWAIT);
			else
				va = kalloc(npg, KALLOC_NOZERO|KALLOC_NOWAIT);
			if (va == NULL) {
				/*
				 * If kalloc fails and this type has a 
				 * reserved pool, we will attempt to
				 * allocate from the reserved pool.
				 * The pool can only be used if the
				 * ks_check routine returns non null.
				 * The caller is allowed to not specify a
				 * check routine.
				 */
				if ((ksp->ks_flags&KS_RESERVED) == 0) {
					return(NULL);
				} else {
					/*
					 * Reserved pool.
					 */
					if (ksp->ks_check && 
					    ((*(ksp->ks_check))(type, size, 
							      flags) == 0)) {
						if ((flags&M_NOWAIT) == 0) {
							va = kalloc(npg,
							       KALLOC_NOZERO);
						} else {
							return(NULL);
						}
					} else {
						return(alloc_reserve(type,
							      size, flags));
					}
				}
			}
		} else {
			va = kalloc(npg, KALLOC_NOZERO);
		}
		VASSERT(va);
		kup = btokup(va);
		kup->ku_indx = indx;
		kup->ku_pagecnt = 0;
		if (allocsize > MAXALLOCSAVE) {
			/*
			 * Ensure that pagecnt, which is a short
			 * does not get exceeded.
			 */
			if (npg > MAXPGCNT)
				panic("malloc: allocation too large");
			kup->ku_pagecnt = (u_short)npg;

#ifdef OSDEBUG
			SPINLOCK_USAV(kmemlock, context);
			ksp->ks_inuse++;
			ksp->ks_calls++;
			ksp->ks_memuse += allocsize;
			ksp->ks_maxused = MAX(ksp->ks_maxused, 
					      ksp->ks_memuse);
			SPINUNLOCK_USAV(kmemlock, context);
#endif
			goto out;
		}
		endva = va + (npg * NBPG) - allocsize;
		for (cp = va; cp < endva; cp += allocsize) {
			*(caddr_t *)cp = cp + allocsize;
		}
                SPINLOCK_USAV(kmemlock, context);
#ifdef OSDEBUG
		kbp->kb_total += npg;
		kbp->kb_totalfree += (ptob(npg) / allocsize);
#endif
		*(caddr_t *)cp = kbp->kb_next;
		kbp->kb_next = va;
		/*
		 * Spinlock still held, fall through
		 */
	}
	va = kbp->kb_next;
	kbp->kb_next = *(caddr_t *)va;
	VASSERT(kbp->kb_totalfree != 0);
#ifdef OSDEBUG
	kbp->kb_totalfree--;	/* Decrement number free */
	kbp->kb_calls++;
	ksp->ks_inuse++;
	ksp->ks_calls++;
	ksp->ks_memuse += (1 << indx);
	ksp->ks_maxused = MAX(ksp->ks_maxused, ksp->ks_memuse);
#endif
        SPINUNLOCK_USAV(kmemlock, context);
out:
	return ((caddr_t)va);
}

#ifdef OSDEBUG
/*
 * BEGIN_DESC 
 *
 * kalloc_aligned()
 *
 * Return Value:
 *	zero - addr is not aligned
 *	non zero - addr is aligned
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function is used to catch FREE()'s who aren't aligned with the
 *	storage they allocated in the first place.
 *
 * Algorithm:
 *	See if addr is aligned on proper boundry.
 *
 * In/Out conditions:
 *	None.
 *
 * END_DESC
 */
int
kalloc_aligned(addr)
	u_long addr;		/* Addr of request */
{
	struct kmemusage *kup = btokup((caddr_t)addr);
	int indx = (kup->ku_indx &~ RESERVED_POOL);
	int size;
	
	if (indx > MAXBUCKETSAVE) {
		return((addr & (NBPG-1)) == 0);
	}
	size = (1 << indx);
	return((addr & (size-1)) == 0);
}
#endif /* OSDEBUG */

/*
 * BEGIN_DESC 
 *
 * kfree()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	bucket
 *	reserved_bucket
 *	kmemstats
 *	kmem_info
 *
 * Description:
 *	Free the allocate memory.  Make sure to pass the
 *	addressed obtained from MALLOC or kmalloc.
 *
 * Algorithm:
 *	If the memory allocated was a chunk of whole pages
 *	return this to the global free pool (kdfree).  If
 *	the memory was from the reserved pool, return it
 *	to the reserved pool at the appropriate bucket
 *	size.
 *
 * In/Out conditions:
 *	Memory addressed by addr is no longer available.
 *	This routine can run on the ICS.
 *
 * END_DESC
 */
/*ARGSUSED*/
void
kfree(addr, type)
	caddr_t addr;	/* Addr to free */
	int type;	/* type of memory (M_???) */
{
	long size;
        register u_int context;
	register struct kmembuckets *kbp;
	register struct kmemusage *kup = btokup(addr);
	register struct kmemstats *ksp = &kmemstats[type];
	register int reserved = (kup->ku_indx & RESERVED_POOL);
	int indx = kup->ku_indx &~ RESERVED_POOL;

	/*
	 * Greater than a page.
	 */
	if (indx > MAXBUCKETSAVE) {
		int pgcnt = (int)kup->ku_pagecnt;
		VASSERT(pgcnt);
#ifdef OSDEBUG
		SPINLOCK_USAV(kmemlock, context);
		ksp->ks_inuse--;
		ksp->ks_memuse -= ptob(pgcnt);
		SPINUNLOCK_USAV(kmemlock, context);
#endif
		kdfree(addr, (u_long)pgcnt);
		return;
	}

	/*
	 * Either a reserved pool or its less than
	 * a page.
	 */
	if (reserved) {
		VASSERT(ksp->ks_flags&KS_RESERVED);
		VASSERT(kup->ku_pagecnt == 0);
		kbp = &reserved_bucket[indx];
	} else {
		kbp = &bucket[indx];
	}
	size = 1 << indx;
	SPINLOCK_USAV(kmemlock, context);
	/*
	 * Link the element on to its list.
	 */
	*(caddr_t *)addr = kbp->kb_next;
	kbp->kb_next = addr;
	/*
	 * Track the amount of reserved in use.
	 */
	if (reserved) {
		ksp->ks_resinuse -= size;
		VASSERT(ksp->ks_resinuse >= 0);
	}
#ifdef OSDEBUG
	ksp->ks_inuse--;
	ksp->ks_memuse -= size;
	kbp->kb_totalfree++;
#endif
	SPINUNLOCK_USAV(kmemlock, context);
	/*
	 * wake anyone waiting for the
	 * reserved pool entry
	 */
	if (reserved) {
		unsigned int x;
		if (ksp->ks_sleep) {
			x = sleep_lock();
			if (ksp->ks_sleep) {
				ksp->ks_sleep = 0;
				wakeup((caddr_t)ksp);
			}
			sleep_unlock(x);
		}
	}
}


/*
 * BEGIN_DESC 
 *
 * kmalloc_reserve()
 *
 * Return Value:
 *	zero - memory reserved
 *	non zero - failed to reserve memory
 *
 * Globals Referenced:
 *	reserved_bucket
 *	kmemstats
 *	total_reserved
 *	kmem_info
 *
 * Description:
 *	kmalloc_reserve allows a subsystem to reserve
 *	a small portion of memory to be used when the 
 *	global system free pool gets tight.
 *
 * Algorithm:
 *	First check that we have not exceeded the upper
 *	 bound on the amount of memory we will prereserve.
 *	 This limit should NOT be increased without
 *	 careful consideration.
 *	Grab the necessary amount of pages and break 
 *	 them into the desired chunks.
 *	Link the chunks into the reserved_bucket pool.
 *
 * In/Out conditions:
 *	This malloc should be called during startup,
 *	with virtual mode turned on, free pool initialized,
 *	and before other processors start.
 *
 * END_DESC
 */
int
kmalloc_reserve(type, size, count, func)
	int type;	/* Memory type (M_???) */
	int size;	/* Size in bytes */
	int count;	/* Count of size units to reserve */
	int (*func)();	/* Govern function to call on alloc */
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register struct kmemstats *ksp;
        register u_int context;
	int indx;
	u_long npg;
	int allocsize;
	caddr_t va;
	caddr_t endva;
	caddr_t cp;
	int i;
	
	/*
	 * This reservation scheme while more general than the previous
	 * scheme does have some limitations.  These limitations can
	 * be removed in the future.  Namely, that each type only 
	 * allocates from the reserved pool in less than or eqaul to
	 * a page size.
	 */
	if (size > MAXALLOCSAVE)
		panic("kmalloc_reserve: reservation > MAXALLOCSAVE");

	/*
	 * Static indicies and computations do not
	 * need spinlocks.
	 */
	ksp = &kmemstats[type];
	indx = BUCKETINDX(size);
	kbp = &reserved_bucket[indx];
	allocsize = 1 << indx;
	npg = btorp(allocsize * count);

	/*
	 * Because the pages associated with the reserved buckets
	 * can not be used by the system for any other purpose,
	 * we dont want a lot of the system memory being used up
	 * in this pool.  We limit the amount each type can have
	 * to TOTAL_TYPE pages and the system to TOTAL_RESERVED
	 * pages.
	 */
	SPINLOCK_USAV(kmemlock, context);
	if ((total_reserved + npg) > TOTAL_RESERVED)
		return(1);
	if ((btorp(ksp->ks_reslimit + (allocsize * count))) > TOTAL_TYPE)
		return(1);
	SPINUNLOCK_USAV(kmemlock, context);

	/*
	 * Allocate the memory.
	 */
	if ((va = kalloc(npg, KALLOC_NOZERO)) == NULL) {
		printf("malloc_reserve: tries to reserve %d pgs - type %d\n",
		       npg, type);
		panic("malloc_reserve could not get memory");
	}

	/*
	 * Break the page into its linked list of pointers.
	 * Note: cp is used below.
	 */
	endva = va + (npg * NBPG) - allocsize;
	for (cp = va; cp < endva; cp += allocsize) {
		*(caddr_t *)cp = cp + allocsize;
	}

	/*
	 * Get the pageinfo structure fill it in for
	 * each page before putting the pages into the resreved_buckets
	 * linked list. Note: cp is used below.
	 */
	for (cp = va, i = npg; i > 0; cp += NBPG, i--) {
		kup = btokup(cp);
		kup->ku_indx = indx|RESERVED_POOL;
		kup->ku_pagecnt = 0;
	}

	/*
	 * Set up the type entry.  If the entry is already
	 * set and the user specified a check routine, make
	 * sure that the same one is specified.  If not this
	 * is a problem; so return an error condition.
	 */
	SPINLOCK_USAV(kmemlock, context);
	if (ksp->ks_flags&KS_RESERVED) {
		if (func && (ksp->ks_check != func)) {
			SPINUNLOCK_USAV(kmemlock, context);
			kdfree(va, npg);
			return(1);
		}
	}
	ksp->ks_check = func;
	ksp->ks_sleep = 0;

	/*
	 * Link the newly allocated page onto the 
	 * reserved_bucket array.
	 */
	cp = endva;
	*(caddr_t *)cp = kbp->kb_next;
	kbp->kb_next = va;

	/*
	 * Set the maximum resource allocation (bytes)
	 * for this type of memory.  We will track the
	 * actual amount used and keep it from exceeding
	 * this amount.
	 */
	ksp->ks_reslimit += ptob(npg);
	total_reserved += npg;

#ifdef OSDEBUG
	kbp->kb_total += npg;
	kbp->kb_totalfree += (ptob(npg) / allocsize);
#endif
	ksp->ks_flags |= KS_RESERVED;
	SPINUNLOCK_USAV(kmemlock, context);
	return(0);
}


/*
 * BEGIN_DESC 
 *
 * alloc_reserve()
 *
 * Return Value:
 *	A pointer to the reserved memory or NULL
 *
 * Globals Referenced:
 *	reserved_bucket
 *	kmemstats
 *
 * Description:
 *	Allocate memory from the reserved pool.  If
 *	the reserved pool is empty and the system
 *	can wait, sleep until the memory is returned.
 *
 * Algorithm:
 *	The reserved pool is handled in a similar manner
 *	to the regular pool.  Each page is broken into
 *	a particular bucket size.  The bucket pointers
 *	for the reserved pool is called reserved_bucket.
 *	Each type of memory has an upper bound on the 
 *	amount of memory reserved for it.  We track
 *	how much of the memory in the reserved pool is 
 *	being used for this type, and limit the type
 *	to only the amount it reserved.
 *
 * In/Out conditions:
 *	This routine can run on the ICS.
 *
 * END_DESC
 */
caddr_t
alloc_reserve(type, size, flags)
	int type;		/* Type of memory (M_???) */
	u_long size;		/* Size of the request */
	int flags;		/* MALLOC flags */
{
	register struct kmembuckets *kbp;
	register struct kmemstats *ksp;
#ifdef OSDEBUG
	register struct kmemusage *kup;
#endif

	int indx;
	caddr_t va;
        register u_int context;
	int x;
	int allocsize;

	VASSERT(size <= MAXALLOCSAVE);
	indx = BUCKETINDX(size);
	allocsize = (1 << indx);
	kbp = &reserved_bucket[indx];
	ksp = &kmemstats[type]; 

        SPINLOCK_USAV(kmemlock, context);
retry:
	if ((kbp->kb_next == NULL) || 
	    ((ksp->ks_resinuse + allocsize) > ksp->ks_reslimit))  {
		if (flags & M_NOWAIT) {
        		SPINUNLOCK_USAV(kmemlock, context);
#ifdef OSDEBUG
			printf(
#else
			msg_printf(
#endif
			"WARNING: alloc_reserve() out of reserved type: %d of size: %d\n",
			       type, size);
			return(NULL);
		} else {
        		SPINUNLOCK_USAV(kmemlock, context);
#ifdef OSDEBUG
			printf(
#else
			msg_printf(
#endif
			"WARNING: alloc_reserve() out of reserved type: %d of size: %d -\
				SLEEPING\n", type, size);
			x = sleep_lock();
			ksp->ks_sleep = 1;
			sleep_then_unlock(ksp, PMEM, x);
        		SPINLOCK_USAV(kmemlock, context);
			goto retry;
		}
	}
	va = kbp->kb_next;
	kbp->kb_next = *(caddr_t *)va;
	ksp->ks_resinuse += allocsize;
	/*
	 * *** HP REVISIT ***
	 * The following line was taken out of OSDEBUG for a late
	 * change (CRT approved) for which we needed to
	 * verify if the reserve pool was ever being hit.
	 *  -byb 1/21/91
	 * *** HP REVISIT ***
	 */
	kbp->kb_calls++;
#ifdef OSDEBUG
	kup = btokup(va);
	VASSERT(kup->ku_indx == indx|RESERVED_POOL);
	VASSERT(kup->ku_pagecnt == 0);
	kup->ku_indx = indx|RESERVED_POOL;
	kup->ku_pagecnt = 0;
	ksp->ks_calls++;
	kbp->kb_totalfree--;
	ksp->ks_inuse++;
	ksp->ks_memuse += allocsize;
	ksp->ks_maxused = MAX(ksp->ks_maxused, ksp->ks_memuse);
#endif
	SPINUNLOCK_USAV(kmemlock, context);
	return((caddr_t)va);
}


/*
 * Compatibility routines for heap_kmem.c
 */
caddr_t
kmem_alloc(nbytes)
	register u_int nbytes;
{
	register caddr_t va;

	MALLOC(va, caddr_t, nbytes, M_KMEM_ALL, M_WAITOK);
	return(va);
}

/*ARGSUSED*/
kmem_free(ptr, nbytes)
	register caddr_t ptr;
	register u_int nbytes;
{
	if (nbytes == 0) {
	    return;
	}
	FREE(ptr, M_KMEM_ALL);
}

/*
 * MOVE THIS STUFF OVER TO A NETWORKING FILE AT SOME POINT.
 */

/*
 * Check to see if I am a client who is swapping remotely and I am
 * requesting an mbuf so I can send a request to swap someone or
 * some page out.
 */
/*ARGSUSED*/
int
dux_mbuf_special(type, size, flags)
	register int type;
	register u_long size;
	register int flags;
{
	/*
	 * Fix for FSDdt07488:
	 *
	 * All diskless requests that we reply to on the ICS are very
	 * important.  Thus, we always want to allow allocation from
	 * the reserved pool.  This is especially important for replies
	 * to DM_ALIVE messages.  If we do not respond in a timely
	 * fashion, the rootserver might declare us dead.
	 */
	if (ON_ICS)
		return 1;

	/*
	 * If we are swapping remotely, and the swapper or pageout
	 * daemon is asking for memory, give it to him.	 Otherwise,
	 * we could have a deadlock (we need memory to page things
	 * out, but we cannot free any up because we cannot page
	 * things out).
	 */
	if ((my_site_status & CCT_SLWS) &&
	    (u.u_procp->p_pid == PID_SWAPPER ||
	     u.u_procp->p_pid == PID_PAGEOUT))
		return 1;

	/*
	 * Some peon is asking for memory, too bad.
	 */
	return 0;
}

	/* This is actually defined in if_ether.h, but you would
	 * not believe what you have to include to include if_ether.h
	 * so I am not doing it.  We should move this all out of
	 * here for 10.0.  cwb.
	 */
#define ETHERMTU	1500

#define NUM_MSIZE	4
#define NUM_CLUSTERS   10

/*
 * Reserve some memory for mbufs and mclusters to make sure we
 * can swap if we start running out of memory
 */

res_mbuf_init()
{
	int msize = ptob(NUM_MSIZE) / MSIZE;
	/* This convoluted calculation is make sure we round up ETHERMTU to
	 * a power of 2 before figuring out how many clusters we can get
	 * out of NUM_CLUSTERS pages.  kmalloc_reserve will also round up
	 * to a power of 2.
	 */
	int clusters = ptob(NUM_CLUSTERS) / (1<<BUCKETINDX(ETHERMTU));

	if (kmalloc_reserve(M_MBUF, MSIZE, msize, dux_mbuf_special))
		panic("res_mbuf_init: reservation for MSIZE failed");
	if (kmalloc_reserve(M_MBUF, ETHERMTU, clusters, dux_mbuf_special))
		panic("res_mbuf_init: reservation for CLUSTERS failed");
}


#ifdef	__hp9000s300
/* Only on the 300 at present */
#define	STACK_300_OVERFLOW_FIX
#endif
#ifdef	STACK_300_OVERFLOW_FIX

/* structure for management */
struct	vapor_malloc_a
{
	struct	vapor_malloc_a	*v_link;
#ifdef	OSDEBUG
	int			 v_type;
#endif /* OSDEBUG */

};
/*
 * BEGIN_DESC 
 *
 *
 *   Purpose:
 *	Implements the kernel memory allocator for getting temporary kernel
 *	memory using MALLOC(), then having it "automatically" returned at 
 *      process exit or return to userland.
 *
 *   Interface:
 *	VAPOR_MALLOC(space, cast, size, type, flags) in malloc.h
 *	vapor_malloc(size, type, flags)
 *	vapor_malloc_free() 
 *
 * Return Value:
 *	A pointer to the reserved memory or NULL
 *
 * Globals Referenced:
 *   u.u_vapor_mlist
 *   MALLOC()
 *
 * Description:
 * This routine will allocate memory and link it to the current process. 
 * The memory will get returned when the process dies or when returned to USER.
 * The purpose of this routine is for systemcalls that need large amounts of
 * temporary memory that would normally be on the stack.  This routine helps 
 * the kernel stack overflow problem.
 * 
 * Vapor_malloc() should be used instead of the VAPOR_MALLOC(..) macro when
 * performance is not a concern or when the "size" field is not a compile time
 * constant.
 *
 * Algorithm:
 *      Bump size by amount needed for struct vapor_malloc_a. 
 *	Use MALLOC() to get memory from the reserved pool.
 *      Put allocated memory in a linked list (u_vapor_mlist) headed from the 
 *      u_area.
 *
 * In/Out conditions:
 *	This routine MUST be run in the user's context.
 *
 * END_DESC

/* This routine will deallocate ALL memory allocated by VAPOR_MALLOC() */
vapor_malloc_free()
{
	int	type;
	struct	vapor_malloc_a *linkp, *linkp_save;
#ifdef OSDEBUG
	VASSERT(!ON_ISTACK);
#endif /* OSDEBUG */

	linkp_save = (struct vapor_malloc_a *)u.u_vapor_mlist;

	while(linkp = linkp_save)
	{
		/* save next link because FREE destroys it */
		linkp_save = linkp->v_link;
#ifdef OSDEBUG
		type = linkp->v_type;
#endif /* OSDEBUG */
		/* free the memory */
		FREE(linkp, type); 
	}
	u.u_vapor_mlist = NULL;
}

struct	vapor_malloc_a *
vapor_malloc(size, type, flags)
{
	struct	vapor_malloc_a  *linkp;

	/* bump size up by link + type */
#ifdef	OSDEBUG
	VASSERT(!ON_ISTACK);
#endif /* OSDEBUG */
	size += sizeof(struct vapor_malloc_a);

	MALLOC(linkp, struct vapor_malloc_a *, size, type, flags);

	if (linkp == NULL) return(NULL);

	linkp->v_link = (struct	vapor_malloc_a *)u.u_vapor_mlist;
#ifdef	OSDEBUG
	linkp->v_type = type;
#endif /* OSDEBUG */
	u.u_vapor_mlist = (caddr_t)linkp;
	return(linkp+1);
}
#endif	/* STACK_300_OVERFLOW_FIX */

/* 
 * BEGIN_DESC
 *
 * kfree_unused()
 *
 *
 * Input Parameters:    None
 *
 * Output Parameters:   None
 *
 * Return Value:        Number of pages freed
 *
 * Globals Referenced:
 *      None
 *
 * Description:
 *	kfree_unused() will coalesce pages and free any unused pages.
 *
 * Algorithm:
 *	Sort the addresses of each bucket into ascending order, then see
 *	if any full pages can be freed.
 *
 * In/Out conditions: None
 *
 *
 * END_DESC
 */
int kfree_unused_stats[MAXBUCKET+1];
int kfree_unused_calls;
int kfree_unused_successes;

#define NEXT_ELEM(elem)         ((caddr_t *)(*(elem)))

int
kfree_unused()
{
	struct kmembuckets *kbp;
	register int index;
	register caddr_t *ptr;
	register int context;
	register int count;
	int total=0;
#define START_SIZE	NBPG/8  /* No use working on the small ones */

	kfree_unused_calls++;
	for (index = BUCKETINDX(START_SIZE); index <= MAXBUCKET; index++) {
                SPINLOCK_USAV(kmemlock, context);
		kbp = &bucket[index];

		/* Make sure there is always at least one element free */
		if ((kbp->kb_next != NULL) &&
		    (*NEXT_ELEM(&kbp->kb_next) != NULL)) {

			/* Count the number of elements */
			count = 0;
			for (ptr = &(kbp->kb_next); *ptr != NULL;
							ptr = NEXT_ELEM(ptr)) {
				count++;
			}

			/* Don't need to sort page size objects */
			if (index < BUCKETINDX(NBPG)) {
				kfree_sort_list(&(kbp->kb_next), count);
			}

			/* See if we can free any pages */
			total += kfree_free_pages(&(kbp->kb_next), count,index);
			kfree_unused_stats[index] += total;
		}
		SPINUNLOCK_USAV(kmemlock, context);
	}

	if (total) kfree_unused_successes++;
	return(total);
}

kfree_sort_list(list, count)
	register caddr_t *list;
	register int count;
{
	register int i, j;
	register caddr_t *elem;
	register caddr_t *tmp;
	register int swapped;

	/* Simple bubble sort, with extra check to quit early */
	for (i = 0; i < count; i++) {
		swapped=0;
		elem = list;
                for (j = i+1; j < count; j++) {
                        if (*elem > *NEXT_ELEM(elem)) {
                                tmp = NEXT_ELEM(elem);
                                *elem = *NEXT_ELEM(elem);
                                *tmp = *NEXT_ELEM(elem);
                                *NEXT_ELEM(elem) = (caddr_t)tmp;
                                swapped++;
                        }
                        elem = NEXT_ELEM(elem);
		}
		if (!swapped) return;
	}
}

kfree_free_pages(list, count, index)
	register caddr_t *list;
	register int count;
	int index;
{
	register int i, j;
	register caddr_t *elem;
	register caddr_t *tmp;
	register int run;
	register int size;
	int total=0;

	/* Size of elements */
	size = 1<<index;

	/* We need this many elements in a row to free the whole page */
	run = NBPG/size;

	/* Make sure algorithm works if MAXBUCKET is > one page */
	if (run < 1) run = 1;
	i = 0;
	while (i < count) {
		/* Look for starting element on page boundary */
		if ((int)*list % NBPG) {
			i++;
			list = NEXT_ELEM(list);
			continue;
		}
		elem = list;
		/* Make sure there are enough element left to consider */
		if ((i+run) > count) break;

		/* See if we have a full pages worth of free elements */
		for (j = 1; j < run; j++) {
                        if (*NEXT_ELEM(elem) != (*elem + size)) {
                                break;
                        }
                        elem = NEXT_ELEM(elem);
		}
		if (j == run) {
			register caddr_t freep;

			/* Success, go ahead and free the page */
			freep = *list;

			/* Take page out of list */
			*list = *NEXT_ELEM(elem);
#ifdef OSDEBUG
			bucket[index].kb_totalfree -= run;
#endif
                	kdfree(freep, (u_long)1);
			total++;
		}
		else {
			list = elem;
		}

		/* Try the next set */
		i += j;
	}
	return(total);
}

#ifdef FDDI_VM

/* 
 * BEGIN_DESC
 *
 * Malloc_switch()
 *
 *
 * Input Parameters:    
 *	pfn1:	A physical page number to have malloc related data exchanged...
 *	pfn2:	with this one.
 *
 * Output Parameters:   None
 *
 * Return Value:        None
 *
 * Globals Referenced:
 *	kmeminfo
 *	firstfree
 *      maxfree
 *
 * Description:
 *	malloc_switch() is called to handle the exchanging of any of the
 *	malloc related data structures associated with the physical pages
 *	passed in.
 *
 * Algorithm:
 *	Save a copy of the kmemusage element associated with the first pfn.
 *	Assign the kmemusage data associated with the second pfn into
 *	the kmemusage element of the first pfn.  Then assign the saved
 *	kmemusage data into the second pfn's kmemusage element.
 *
 * In/Out conditions: None
 *
 *
 * END_DESC
 */

void
malloc_switch(pfn1, pfn2)
	int pfn1, pfn2;
{
        extern int firstfree, maxfree;
	struct kmemusage save_usage;
	struct kmemusage *ku = (struct kmemusage *)kmeminfo;
	int p1 = pfn1 - firstfree;
	int p2 = pfn2 - firstfree;

	/*
	 * Assert that pfns are within the array bounds of kmeminfo.
	 */
	VASSERT((pfn1 >= firstfree) && (pfn1 <= maxfree));
	VASSERT((pfn2 >= firstfree) && (pfn2 <= maxfree));
        /* 
	 * Save the kmemusage entry for the first pfn. 
	 */
	save_usage = ku[p1];

	/* 
	 * Assign the second entry to the first. 
	 */
        ku[p1] = ku[p2];

	/* 
	 * Assign the saved value to the second pfn's kmemusage entry.
 	 */
        ku[p2] = save_usage;
}
#endif /* FDDI_VM */
