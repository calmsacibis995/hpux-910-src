/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_page.c,v $
 * $Revision: 1.85.83.6 $       $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/05/05 15:28:52 $
 */
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../h/vmmac.h"
#include "../h/systm.h"
#include "../h/pfdat.h"
#include "../h/debug.h"
#include "../h/pregion.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/vmparam.h"
#include "../h/sema.h"
#include "../h/tuneable.h"
#include "../h/buf.h"
#include "../dux/cct.h"
#ifdef FSD_KI
#include "../h/ki_calls.h"
#endif /* FSD_KI */

int	firstfree, maxfree, freemem;
caddr_t	kmembase;
caddr_t *kmeminfo;
int gpgslim;
extern int freemem_cnt;
extern parolemem;
extern hitzerofreemem;

vm_lock_t pfdat_hash;
vm_lock_t pfdat_lock;

int maxpageout = MAXPG;
int prepage = MAXPREP;

extern vm_sema_t vhandsema;

#ifdef PFDAT32

/*
 *  In order to reduce kernel RAM usage, I have trimmed the pfdat structure
 *  from 56 bytes/page to 32 bytes/page.  This involved touching lots of
 *  files, and making several key changes:
 *	- switching from an 8-byte vm_sema_t to a lock bit (see code below)
 *	- combining HI/HD bit/flag fields
 *	- HD: combining ATL and IPTE fields, since we use one or the other
 *      - HD: adding the concept of "direct PTEs" for non-ATL, non-IPTE stuff
 *
 *						-- DKM 5-5-94
 */

pfd_lock(pfd)
struct pfdat *pfd;
{
	int x;
	
	
	x = CRIT();
	while (pfd->pf_locked) {
		pfd->pf_wanted = 1;
		sleep(pfd, PMEM);
	}

	pfd->pf_locked = 1;
	UNCRIT(x);
}


pfd_test_lock(pfd)
struct pfdat *pfd;
{
	int x;
	
	
	x = CRIT();
	if (pfd->pf_locked) {
		UNCRIT(x);
		return(0);
	} else
		pfd->pf_locked = 1;

	UNCRIT(x);
	return(1);
}



pfd_unlock(pfd)
struct pfdat *pfd;
{
	int x;
	

	x = CRIT();
	pfd->pf_locked = 0;
	if (pfd->pf_wanted) {
		pfd->pf_wanted = 0;
		wakeup(pfd);
	}
	UNCRIT(x);
}

#endif


/*
 * pfdatnumentries	the total number of pfdat's in the system
 *			(does not vary once initialized)
 *
 * pfdatnumfree		the number of pfdat's that are on the free list
 *			(must be >= 0 and <= pfdatnumentries)
 *
 * pfdatnumreserved	the number of pfdat's that are *both* on the free
 *			list and reserved (must be >= 0 and <= pfdatnumfree)
 *
 * These variables are only used for debugging.  They appear in VASSERT's
 * and are available post mortem.
 *
 * The expectation is that before taking any page from the free list, it
 * must first have been reserved (see memreserve and cmemreserve).  Once
 * reserved, the page is either unreserved (see memunreserve), allocated
 * (see allocpfd), or just removed from the free list (see removepfd).
 * Returning a page to the free list (see freepfd) completes the cycle.
 */
int pfdatnumentries;
int pfdatnumfree;
int pfdatnumreserved;

/*
 * Count indicating that we are waiting for a page to be unlocked.
 */
int phead_cnt = 0;

/*
 * This function is used by the drivers to do physical I/O, not by the VM
 * system to do paging.  It needs a better home.  The file that has
 * physio() in it looks best, but is diverged, so I'll leave it here
 * for now.
 */
unsigned
minphys(bp)
	 struct buf *bp;
{
	if ((my_site_status & CCT_SLWS) && (bp->b_vp == swapdev_vp)) {
		if (bp->b_bcount > NETMAXPHYS)
			bp->b_bcount = NETMAXPHYS;
	}
	else {
		if (bp->b_bcount > MAXPHYS)
			bp->b_bcount = MAXPHYS;
	}
}

#ifdef OSDEBUG
page_info(x)
	caddr_t x;
{
        int pfn;

        pfn = hdl_vpfn(KERNELSPACE,(x));
        VASSERT((pfn >= firstfree) || pfn < maxfree);
        pfn -= firstfree;
        VASSERT(pfn >= 0);
        return((int)&kmeminfo[pfn]);
}
#endif
/*
 * Initialize memory map
 *	first	-> first free page #
 *	last	-> last free page #
 *	vpage	-> first virtual page #
 * returns:
 *	none
 */
meminit(first, last, vpage)
	int first, last, vpage;
{
	register pfd_t *pfd;
	register int i;

	firstfree = first;		/* first available page # */
	maxfree = last;			/* last  available page # */
	freemem = last - first + 1;	/* # of available pages */
	kmembase =			/* first virtual page addr */
	    (caddr_t)ptob(vpage);

	pfdatnumfree = freemem;		/* useful for debugging */
	pfdatnumreserved = 0;		/* ditto */
	pfdatnumentries = freemem;	/* max pages ever (never changed) */

	/*
	 * Setup queue of pfdat structures.
	 * One for each page of available memory.
	 */
	phead.pf_next = &phead;
	phead.pf_prev = &phead;

	/*
	 * Add pages to queue, high memory at end of queue
	 * Pages added to queue FIFO
	 */
	pfd = &pfdat[first];
	for (i = first; i <= last; pfd++,i++) {
		pfd->pf_next = &phead;
		pfd->pf_prev = phead.pf_prev;
		phead.pf_prev->pf_next = pfd;
		phead.pf_prev = pfd;
		pfd->pf_flags = P_QUEUE;
		pfd->pf_use = 0;
#ifdef PFDAT32
		pfd->pf_locked = 0;
#else
		pfd->pf_pfn = i;
		/* PDIR entry is unlocked */
		vm_initsema(&pfd->pf_lock, 0, PFD_LOCK_ORDER, "pfdat sema");	
		hdl_unvirtualize((int)pfd->pf_pfn);
		pfdatunlock(pfd);
#endif
	}

	/* vhand should not run */
	vm_initsema(&vhandsema, 0, VHAND_SEMA_ORDER, "vhand sema"); 
}

int allpageshashed = 0;
int allpageslocked = 0;




/* 
 * Get a page frame from the free list 
 *
 * NOTE: memreserve() must be called before calling
 *       allocpfd() to let the memory reservation
 *       scheme know we are taking a page.
 */
pfd_t *
allocpfd()
{
	register pfd_t *pfd;
        register u_int context;
        extern int unhash;


retry:
        SPINLOCK_USAV(pfdat_lock, context);

	if (phead.pf_next == &phead)
		panic("allocpfd: reservation exceeded free list");

	/*
	 * Look for a single page to allocate.
	 */
	for (pfd = phead.pf_next; pfd != &phead; pfd = pfd->pf_next) {
		if (cpfdatlock(pfd)) {
			/*
			 * If DMEM (currently 800 only) marked this as bad,
			 * we don't want it.  Maybe it will be unmarked later.
			 */
			if (pfd->pf_flags & P_BAD) {
				pfdatunlock(pfd);
				continue;
			}
			/*
			 * If we are on the interrupt stack and this page
			 * is hashed, we can not take it because we
			 * must perform a VN_RELE to unhash the page
			 * and this could cause to sleep.  We give up
			 * here because the free list is sorted with the
			 * unhashed pages at the beginning.
			 */
			if ((ON_ISTACK) && PAGEINHASH(pfd)) {
				allpageshashed++;
				pfdatunlock(pfd);
				goto punt_ics;
			}
			break;
		}
	}
	/*
	 * were we able to lock one?
	 */
	if (pfd == &phead) {
		/*
		 * allocpfd: all pages on free list locked
		 */
		if (ON_ISTACK) {
			/*
			 * cannot sleep, so backout.
			 */
			allpageslocked++;
			goto punt_ics;
		} else {
                        SPINUNLOCK_USAV(pfdat_lock, context);
			phead_cnt++;
			if (freemem + parolemem < gpgslim)
			    wakeup(vhandsema);
			sleep(&phead, PMEM);
			phead_cnt--;
			goto retry;
		}
	}

	/*
	 * Remove from free list
	 */
	pfd->pf_prev->pf_next = pfd->pf_next;
	pfd->pf_next->pf_prev = pfd->pf_prev;
#ifdef PFDAT32
	VASSERT(pfd->pf_use == 0);
#endif

	/*
	 * Initialize fields.
	 *	This is kind of kludgey at the moment because we are 
	 *	using the same memory for attach lists, indirect PTEs,
	 *	and "kernel" PTEs.
	 */
#ifdef PFDAT32
        pfd->pf_hdl.pf_bits &= PFHDL_ZERO_BITS;
       	pfd->pf_hdl.pf_rw_pte.pg_pfnum = hiltopfn((pfd - pfdat));

	if (indirect_ptes) {
		pfd->pf_hdl.pf_ro_pte = pfd->pf_hdl.pf_rw_pte;
		pfd->pf_hdl.pf_ro_pte.pg_prot = 1;
		pfd->pf_hdl.pf_ro_pte.pg_ropte = 1;
	} else {
		pfd->pf_hdl.pf_atl.max_index = 0;
		pfd->pf_hdl.pf_atl.cur_index = 0;		
		pfd->pf_hdl.pf_atl.atl_data = (struct pfd_atl_entry *) 0;
	}

#else	
	VASSERT(pfd->pf_use == 0);
        VASSERT((hdl_getbits((int)pfd->pf_pfn) & (VPG_MOD|VPG_REF))==0);
#endif	
	pfd->pf_use = 1;
	pfd->pf_flags &= ~P_QUEUE;
	pfd->pf_next = NULL;
	pfd->pf_prev = NULL;

	pfdatnumfree--;
	pfdatnumreserved--;
	VASSERT(pfdatnumreserved >= 0);
	VASSERT(pfdatnumfree >= 0);
	VASSERT(pfdatnumfree < pfdatnumentries);
	VASSERT(pfdatnumreserved <= pfdatnumfree);
	VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

        SPINUNLOCK_USAV(pfdat_lock, context);

#if	defined(__hp9000s300) && defined(SEMAPHORE_DEBUG)
	pfdatdisown(pfd);
#endif	/* defined(__hp9000s300) && defined(SEMAPHORE_DEBUG) */

	/*
	 * If page is in page cache remove it.
	 */

	if (PAGEINHASH(pfd))
		pageremove(pfd);

	return(pfd);

punt_ics:
	/*
	 * If we "found" a lockable hashed page, and are on the interrupt
	 * stack, the first page we could grab was hashed.  We must
	 * give up at this point because all of the rest of the
	 * pages on the free list are hashed and we can not use 
	 * any of them.
	 *
	 * also, if pfd == &phead, all the pages were locked.  Also bad.
	 */
        SPINUNLOCK_USAV(pfdat_lock, context);
	VASSERT(ON_ISTACK);

	if (freemem + parolemem < gpgslim) {
	    /*
	     * Wakeup vhand.  It may be able to add new, unhashed,
	     * pages to the free list.
	     * Should we make it real-time?????
	     */
	    wakeup(vhandsema);
	}

        /*
         *  Wakeup unhash daemon.  It runs unhashpages() which
         *  should free maxunhash pages to front of freelist.
         */
        wakeup((caddr_t)&unhash);

	return((pfd_t *)NULL);
}

int minsleepmem; 	/* minimum value of freemem a process is currently
				sleeping on */
int memory_sleepers=0;	/* number of processes sleeping on non-zero values
				of freemem */

/*
 * Called anytime memory is freed so that processes sleeping on non-zero
 * values of freemem can be woken up.  This function does nothing if
 * no processes are sleeping on non-zero values of freemem or all such
 * processes are sleeping on higher values of freemem than is currently
 * available.
 */
void wakeup_memsleepers()
{
    int s;

    if (memory_sleepers && freemem >= minsleepmem) {
	s = spl6();
	minsleepmem = desfree;
	memory_sleepers = 0;
	splx(s);
	wakeup(&memory_sleepers);
    }
}

/*
 * NOTE: even though memreserve() must be called before a call
 *       to allocpfd(), memunreserve() should NOT be called
 *	 before freepfd() since freepfd() increments the free
 *       memory count for us.  
 */
freepfd(pfd)
	register pfd_t *pfd;
{
	unsigned lock_state;
        register u_int  context;

#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
	VASSERT(pfd->pf_use > 0);

	if (--pfd->pf_use > 0) {
		pfdatunlock(pfd);
		return;
	}
	VASSERT(pfd->pf_use == 0);

	/*
	 * Since the page is no longer used remove all references to it.
	 */
#ifdef PFDAT32
	hdl_unvirtualize((int)(pfd - pfdat));
#else
	hdl_unvirtualize((int)pfd->pf_pfn);
        VASSERT((hdl_getbits((int)pfd->pf_pfn) & (VPG_MOD|VPG_REF))==0);
#endif
        SPINLOCK_USAV(pfdat_lock, context);

	/*
	 * Link into free list.  If page is hashed put at the 
	 * end of the freelist.
	 */
	if (PAGEINHASH(pfd)) {
		pfd->pf_prev = phead.pf_prev;
		pfd->pf_next = &phead;
		phead.pf_prev->pf_next = pfd;
		phead.pf_prev = pfd;
	} else {
		pfd->pf_next = phead.pf_next;
		pfd->pf_prev = &phead;
		phead.pf_next->pf_prev = pfd;
		phead.pf_next = pfd;
	}
	pfd->pf_flags |= P_QUEUE;
	freemem++;

	pfdatnumfree++;
	VASSERT(pfdatnumreserved >= 0);
	VASSERT(pfdatnumfree > 0);
	VASSERT(pfdatnumfree <= pfdatnumentries);
	VASSERT(pfdatnumreserved <= pfdatnumfree);
	VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

        SPINUNLOCK_USAV(pfdat_lock, context);
	pfdatunlock(pfd);

	if (freemem_cnt) { /* low overhead check of freemem_cnt */
		lock_state = sleep_lock();
		/* if anyone is waiting on memory then wake him up */
		if (freemem_cnt) {
			freemem_cnt = 0;
			wakeup((caddr_t)&freemem);
		}
		sleep_unlock(lock_state);
	}

	wakeup_memsleepers();
}

#ifdef OSDEBUG
/* Now a macro in pfdat.h */
pageonfree(pfd)
	register pfd_t *pfd;
{
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
	
	return(pfd->pf_flags&P_QUEUE);
}
#endif



removepfn(pfd)
	register pfd_t *pfd;
{
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
	VASSERT(pfd->pf_flags&P_QUEUE);
	VASSERT(pfd->pf_use == 0);

	if (!cmemreserve(1)) {
		/*
		 * Go ahead and remove it from
		 * the page cache; so a new 
		 * one can be created later.
		 */
		pageremove(pfd);
		pfdatunlock(pfd);
		return(0);
	}

	pfdatlstlock();
	VASSERT(pfd->pf_use == 0);
	pfd->pf_use = 1;
	pfd->pf_flags &= ~P_QUEUE;
	pfd->pf_prev->pf_next = pfd->pf_next;
	pfd->pf_next->pf_prev = pfd->pf_prev;
	pfd->pf_next = NULL;
	pfd->pf_prev = NULL;
	cnt.v_pgfrec++;

	pfdatnumfree--;
	pfdatnumreserved--;
	VASSERT(pfdatnumreserved >= 0);
	VASSERT(pfdatnumfree >= 0);
	VASSERT(pfdatnumfree < pfdatnumentries);
	VASSERT(pfdatnumreserved <= pfdatnumfree);
	VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

	pfdatlstunlock();
	return(1);
}


/*
 * Preallocate memory to a process.  Will sleep if "waitok" is true.
 * Returns:
 *	0 - failed to allocate
 *	1 - success
 */
procmemreserve(size, waitok, rp)
	unsigned size;
	int waitok;
	reg_t *rp;
{
	void memunreserve();
	VASSERT(u.u_procp->p_memresv == -1);
	VASSERT(u.u_procp->p_swpresv == -1);

	if (cmemreserve(size) == 0) {
		if (!waitok)
			return(0);

		memreserve(rp, size);
	}
	if (steal_swap(rp, (int)size, 1) == 0) {
                if (!waitok) {
                        memunreserve((unsigned int)rp);
                        return(0);
                }
                (void) steal_swap(rp, (int)size, 0);
        }
	steal_lockmem(size);	
	/*
	 * since we have reserved the total amount of memory 
	 * needed up front, it needs to be recored in the
	 * the proc structure so that we do not allocate more later.
	 *
	 * p_memresv = amount of memory reserved.
	 * p_swpresv = amount of memory the swapper cannot use.
	 */
	u.u_procp->p_memresv = size;
	u.u_procp->p_swpresv = size;
	return(1);
}

/*
 * Release any unused memory resources reserved to this process
 */
void
procmemunreserve()
{
	register int size;
	register struct proc *p = u.u_procp;
	void memunreserve();

	size = p->p_memresv;
	p->p_memresv = -1;		/* reset to initial value */
	memunreserve((unsigned int)size);
	size = p->p_swpresv;
        p->p_swpresv = -1;              /* reset to initial value */
        return_swap(size);
	lockmemunreserve(size);
}

/*
 * Returns 1 if a process has procmemreserved and 0 otherwise
 */
has_procmemreserved()
{
        return u.u_procp->p_memresv != -1;
}

#ifdef NEVER_CALLED
/*
 * Conditionally reserve size memory pages for I/O.
 * Does not block waiting for memory to become available.
 * Return values:
 *	0 - Insufficient memory available; freemem unaltered.
 *	1 - Memory available immediately;  freemem decremented by size.
 */
int
ciomemreserve(size)
	register unsigned size;
{
        register u_int context;

	VASSERT(freemem >= 0);
	VASSERT(size != (unsigned int) 0);

        SPINLOCK_USAV(pfdat_lock, context);
	if (freemem >= size) {
		freemem -= size;
		pfdatnumreserved += size;
                SPINUNLOCK_USAV(pfdat_lock, context);
		return(1);
	}
        SPINUNLOCK_USAV(pfdat_lock, context);
	return(0);
}
#endif /* NEVER_CALLED */

/*
 * Conditionally reserve size memory pages.
 * Does not block waiting for memory to become available.
 * Return values:
 *	0 - Insufficient memory available; freemem unaltered.
 *	1 - Memory available immediately;  freemem decremented by size.
 */
int
cmemreserve(size)
	register unsigned size;
{
        register u_int context;

	VASSERT(freemem >= 0);
	VASSERT(size != (unsigned int) 0);
	VASSERT(size <= pfdatnumentries);

	/*
	 * If we have a process context, see if we have pre-reserved memory
	 * to allocate from.
	 */
	if (!ON_ISTACK) {
		register struct proc *p = u.u_procp;

		if (p->p_memresv != -1) {
			if (size > p->p_memresv)
				panic("cmemreserve: reservation overrun");
			p->p_memresv -= size;
			return(1);
		}
	}

        SPINLOCK_USAV(pfdat_lock, context);
	if (freemem >= size) {
		freemem -= size;

		pfdatnumreserved += size;
		VASSERT(pfdatnumreserved >= size);
		VASSERT(pfdatnumfree >= 0);
		VASSERT(pfdatnumfree <= pfdatnumentries);
		VASSERT(pfdatnumreserved <= pfdatnumfree);
		VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

                SPINUNLOCK_USAV(pfdat_lock, context);
		return(1);
	}
        SPINUNLOCK_USAV(pfdat_lock, context);
	return(0);
}

/*
 * Reserve size memory pages.  Returns with freemem decremented by size.
 * Blocks waiting for memory to become available, if necessary.
 * Return values:
 *	0 - Memory available immediately
 *	1 - Had to sleep to get memory
 */
int
memreserve(rp, size)
	register reg_t *rp;
	register unsigned size;
{
	extern int mainentered;
	register proc_t *p;
	unsigned lock_state;

	VASSERT(!rp || vm_valusema(&rp->r_lock) <= 0);
	VASSERT(size != (unsigned int) 0);
	VASSERT(size <= pfdatnumentries);

	if (cmemreserve(size))
		return(0);

	if (!mainentered)
		panic("memreserve: initialization could not get memory");

	p = u.u_procp;

	pfdatlstlock();

	while (freemem < size) {
		int boost_prio = 0, new_prio, old_prio;

		/*
		 * We're going to boost vhand's priority to help out
		 * when it's a realtime process that needs memory.  We
		 * boost vhand to a priority 1 weaker than the process
		 * that wants the memory.  This keeps vhand from
		 * competing with other processes of the same strength.
		 * Note:  If making it 1 priority weaker would make it
		 * a timeshare process, we forget about being friendly
		 * and just let it compete with others of the same prio.
		 */
		if (p->p_flag & SRTPROC) {
			VASSERT(p->p_pri<PTIMESHARE);
			if (p->p_pri < PTIMESHARE-1)
				new_prio = p->p_pri+1;	/* not *too* good */
			else
				new_prio = p->p_pri;	/* must not go TS */
			if (new_prio < proc[S_PAGEOUT].p_pri)
				boost_prio = 1;	/* it will help, so do it */
		}

		/* take what we can get so this swtch was not wasted */

		pfdatnumreserved += freemem;
		VASSERT(pfdatnumreserved >= freemem);
		VASSERT(pfdatnumfree >= 0);
		VASSERT(pfdatnumfree <= pfdatnumentries);
		VASSERT(pfdatnumreserved <= pfdatnumfree);

		VASSERT(freemem >= 0);
		size -= freemem;
		freemem = 0;
		VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

		/* State that someone intends to sleep */
		pfdatlstunlock();
		if (rp != NULL)
			regrele(rp);

		if (freemem + parolemem < gpgslim) {
		    /* Give vhand the opportunity to promote its priority */
		    if (boost_prio)
			    old_prio = changepri(&proc[S_PAGEOUT], new_prio);
		    wakeup(vhandsema);
		} else boost_prio = 0;

		lock_state = sleep_lock();
		freemem_cnt++;

		/* Wait for memory-type event */
		++hitzerofreemem; /* feedback for vhand */
		sleep_then_unlock((caddr_t)&freemem, PSWP+2, lock_state);

		/* Move vhand back if boosted and would be too strong */
		if (boost_prio && proc[S_PAGEOUT].p_pri < old_prio)
			(void)changepri(&proc[S_PAGEOUT], old_prio);

		if (rp != NULL)
			reglock(rp);
		pfdatlstlock();
	}
	freemem -= size;
	VASSERT(freemem >= 0);

	pfdatnumreserved += size;
	VASSERT(pfdatnumreserved >= size);
	VASSERT(pfdatnumfree >= 0);
	VASSERT(pfdatnumfree <= pfdatnumentries);
	VASSERT(pfdatnumreserved <= pfdatnumfree);
	VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

	pfdatlstunlock();
	return(1);
}

/*
 * Unreserve size memory pages.
 * Increments freemem by size and wakes up any processes waiting for memory.
 */
void
memunreserve(size)
	unsigned size;
{
	register struct proc *p = u.u_procp;
	unsigned lock_state;

	VASSERT(size <= pfdatnumreserved);
	VASSERT(freemem < physmem);

	if (p->p_memresv != -1) {
		p->p_memresv += size;
		return;
	}

	pfdatlstlock();
	freemem += size;

	pfdatnumreserved -= size;
	VASSERT(pfdatnumreserved >= 0);
	VASSERT(pfdatnumfree >= 0);
	VASSERT(pfdatnumfree <= pfdatnumentries);
	VASSERT(pfdatnumreserved <= pfdatnumfree-size);
	VASSERT(pfdatnumfree-pfdatnumreserved == freemem);

	pfdatlstunlock();
	lock_state = sleep_lock();
	/* if anyone is waiting on memory then wake him up */
	if (freemem_cnt) {
		freemem_cnt = 0;
		wakeup((caddr_t)&freemem);
	}
	sleep_unlock(lock_state);

	wakeup_memsleepers();
}


/*
 * Fill in vfds
 *
 * NOTE:  we do not need to call memreserve() before the allocpfd()
 *        in vfdfill() since we already called memreserve() before
 *	  calling vfdfill().	
 */
vfdfill(start, size, prp)
	int start;
	unsigned size;
	register preg_t	*prp;
{
	pfd_t *pfd;
	register vfd_t *vfd;
	register int count;
	register reg_t *rp = prp->p_reg;

	VASSERT(rp && start >= 0);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	for (count = (int) size; --count >= 0; start++) {
		vfd = FINDVFD(rp, start);
		VASSERT((vfd->pgm.pg_v) == 0);
		pfd = allocpfd();

		/*
		 * Insert in vfd
		 */
#ifdef PFDAT32
		vfd->pgm.pg_pfn = pfd - pfdat;
#else
		vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
		vfd->pgm.pg_v = 1;
		rp->r_nvalid++;
		/*  Removed hdl_addtrans() -jkr */
	}
	VASSERT(rp->r_nvalid <= rp->r_pgsz);
}

/*
 * Allocate pages and fill in page table
 *	rp		-> region pages are being added to.
 *	start		-> index of beginning page
 *	size		-> # of pages needed
 * returns:
 *	0	Memory allocated as requested.
 *	1	Had to unlock region and go to sleep before
 *		memory was obtained.  After awakening, the
 *		first page was valid so no page was allocated.
 */

vfdmemall(prp, start, size)
	register preg_t *prp;
	int start;
	unsigned size;
{
	void memunreserve();
	register vfd_t *vfd;
	reg_t *rp = prp->p_reg;

	VASSERT(rp && start >= 0);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	memreserve(rp, size);
	vfd = FINDVFD(rp, start);
	if (vfd->pgm.pg_v) {
		memunreserve(size);
		return(1);
	}
	vfdfill(start, size, prp);
	return(0);
}


/*
 * do_free()--local function to free up the resources on an active
 * entry in the region.
 */
int
do_free(idx, vfd, dbd, rp)
	int idx;
	vfd_t *vfd;
	dbd_t *dbd;
	reg_t *rp;
{
	pfd_t *pfd;
	extern dbd_t *default_dbd();

	if (vfd->pgm.pg_v) {
		rp->r_nvalid--;

		/* 
		 * Free page--freepfd() will unvirtualize the page
		 * on last reference
		 */
		pfd = &pfdat[vfd->pgm.pg_pfn];
		pfdatlock(pfd);
		freepfd(pfd);
	}
	switch (dbd->dbd_type) {
		case DBD_FSTORE:
			VOP_DBDDEALLOC(rp->r_fstore, dbd);
			break;
		case DBD_BSTORE:
			VOP_DBDDEALLOC(rp->r_bstore, dbd);
			break;
	}
	/*
	 * Clear out dbd and vfd.  Note that the vfd is just cleared,
	 * whereas the dbd must be initialized back to its initial
	 * value (in order for the prototype DBD concept to work).
	 */
	vfd->pgi.pg_vfd = 0;
	*dbd = *default_dbd(rp, idx);
	return(0);
}

/*
 * do_freec()--local function to free up the resources on all active
 * entries in the region.
 */
/*ARGSUSED*/
int
do_freec(rp, idx, vd, count, args)
	reg_t *rp;
	int idx;
	struct vfddbd *vd;
	int count;
	caddr_t args;
{
	int dealloc = 1;

	/*
	 * The following performance hack avoid the dereference
	 * of the vnode if DBDDEALLOC is NEVER going to do anything.
	 */
	if ((rp->r_fstore == rp->r_bstore) && 
	    (rp->r_fstore->v_op->vn_dbddealloc == NULL)) {
		dealloc = 0;
	}

	for (; count--; vd++) {
		vfd_t *vfd = &(vd->c_vfd);
		dbd_t *dbd = &(vd->c_dbd);

		if (vfd->pgm.pg_v) {
			pfd_t *pfd;
			rp->r_nvalid--;

			/* 
			 * Free page--freepfd() will unvirtualize the page
			 * on last reference
			 */
			pfd = &pfdat[vfd->pgm.pg_pfn];
			pfdatlock(pfd);
			freepfd(pfd);
		}
		if (dealloc) {
			switch (dbd->dbd_type) {
			case DBD_FSTORE:
				VOP_DBDDEALLOC(rp->r_fstore, dbd);
				break;
			case DBD_BSTORE:
				VOP_DBDDEALLOC(rp->r_bstore, dbd);
				break;
			}
		}
		/*
		 * Clear out dbd and vfd.  Note that the vfd is just cleared,
		 * whereas the dbd must be initialized back to its initial
		 * value (in order for the prototype DBD concept to work).
		 */
		vfd->pgi.pg_vfd = 0;
		*dbd = *default_dbd(rp, idx);
	}
	return(0);
}

/*
 * Shred page table and update accounting for swapped
 * and resident pages
 *	rp	-> ptr to the region structure.
 *	start	-> index of beginning page
 *	size	-> nbr of pages to free.
 */
pgfree(rp, start, size)
	register reg_t	*rp;
	register int		start;
	register unsigned	size;
{
	register u_int          context;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(start >= 0);

	while (rp->r_poip) {
		context = sleep_lock();
		if (rp->r_poip)
			sleep_then_unlock(&rp->r_poip, PMEM, context);
		else
			sleep_unlock(context);
	}
	if ((start == 0) && (size == rp->r_pgsz))
		foreach_chunk(rp, start, (int) size, do_freec, (caddr_t)NULL);
	else
		foreach_entry(rp, start, (int) size, do_free, (caddr_t)rp);
}

/* 
 * Free memory pages.  
 * This routine is called by a vnode pageout routine to free pages assocaited
 * with a region.  The valid count should be decremented by the pageout 
 * routine prior to calling memfree.
 */
#ifdef FSD_KI
memfree(vfd, rp)
	vfd_t *vfd;
	reg_t *rp;
#else /* ! FSD_KI */
memfree(vfd)
	vfd_t *vfd;
#endif /* ! FSD_KI */
{
	register pfd_t *pfd;

	VASSERT(vfd->pgm.pg_v == 0);
	pfd = &pfdat[vfd->pgm.pg_pfn];
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	

#ifdef FSD_KI
	KI_memfree(rp, vfd->pgm.pg_pfn);
#endif /* FSD_KI */
	/*	
	 * Free unused page.
	 */
	VASSERT(pfd->pf_use > 0);
	freepfd(pfd);
}
