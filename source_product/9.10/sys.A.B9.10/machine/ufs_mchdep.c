/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/machine/RCS/ufs_mchdep.c,v $
 * $Revision: 1.13.83.8 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:32 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../machine/reg.h" /* For CR_IT */

#include "../h/debug.h"
#include "../h/pfdat.h"
#include "../h/bcache.h"
#ifdef FSD_KI
#include "../h/ki_calls.h"
#endif /* FSD_KI */

/* Series 700 and 800 allocate their virtual addresses from a separate 
 * address map than the system map.  The addresses belong to quadrant
 * three.
*/
#ifdef hp9000s800
#define BUFMAP bufmap
#else
#define BUFMAP sysmap
#endif /* hp9000s800 */


int bvalloc = 0;   /* Total virtual space used for buffer cache. */
int bc_shrink = 0; /* bc_shrink is set when the virtual address space 
		    * allocation exceeds the high water mark "buffer_high".
		    * bc_shrink==1 iplies that b_virtsize = btop(b_bufsize)
		    * for each buffer.
		    */

/*
 * Machine dependent handling of the buffer cache.
 */

#ifdef OSDEBUG
u_int total_allocbufs	= 0;
u_int shrink		= 0;
u_int shrink_over	= 0;
u_int noshrink		= 0;
u_int nochange		= 0;
u_int grow		= 0;
u_int grow_over		= 0;
u_int grow_steal	= 0;
u_int bufsteal		= 0;
u_int pagesteal		= 0;
u_int frenzy		= 0;
u_int frenzy_free	= 0;
u_int bcalloc_malloc	= 0;
u_int bcalloc_inval	= 0;
u_int bcalloc_unused	= 0;
u_int bcfree_unused	= 0;
u_int bcalloc_number_free	= 0;


/* Tracks the size of all buffer headers in the system.(virtual not physical) */
int bcheadersize[(MAXBSIZE/NBPG)+1] = 0;
#endif OSDEBUG

pfd_t bhead; 	       /* bhead links together unused physical pages in the 
			* buffer cache. 
			*/
vm_lock_t bcvirt_lock; /* bcvirt_lock controls access to the total virtual 
			* space count for the buffer cache.
			*/
vm_lock_t bcphys_lock; /* bcphys_lock controls access to the buffer cache 
			* physical page pool.
 			*/

/* The following control when the buffer cache starts and stops stealing
 * virtual memory.  They are variables to allow changes in the field if the
 * values appear to be either to high or low.
 */
int buffer_high = BUFFER_VIRTUAL_HIGH_MARK;
int buffer_low = BUFFER_VIRTUAL_LOW_MARK;

/*
 * BEGIN_DESC 
 *
 * bfget()
 *
 * Return Value:
 *	If sucessful, a virtual address pointing to "size" contiguous pages.
 *	If "size" pages cannot be allocated then a NULL is returned.
 *
 * Globals Referenced:
 *	bc_shrink
 *	bvalloc
 *
 * Description:
 *	bfget() allocates "size" pages worth of virtual memory.  It calls
 *	rmalloc() with "size" to allocate the space.  If rmalloc() fails then
 *	return a NULL.  If successful we must check to see if the buffer cache
 *	has exceeded its max virtual consumption; if it is then set bc_shrink.
 *
 *	We don't hold the SPINLOCK around the check to set bc_shrink
 *	since it doesnt matter if we have a race condition.  bc_shrink is an
 *	indicator not a value.   If set upon the next call thats ok.  It's ok
 *	if the buffer cache goes a little above the high water mark before 
 *	setting bc_shrink.
 *	
 * Algorithm:
 *	if (rmalloc(size)) fails then return NULL
 *
 *	SPINLOCK(bcvirt_lock)
 *	Increment the total buffer cache virtual pages.
 *	SPINUNLOCK(bcvirt_lock)
 *	if (more pages allocated than high water mark)
 *		set bc_shrink to 1
 *	
 * In/Out conditions: Locks held on entry...
 *
 * END_DESC
 */
caddr_t
bfget(size)
   unsigned size;  /* Amount of virtual memory needed in pages. */
{
   register unsigned int tmp;
   register u_int context;
   extern long rmalloc();

	if (size == 0) return((caddr_t)NULL);

	/* Allocate an address of "size" pages. */
	if ((tmp = (unsigned int)rmalloc(BUFMAP, (unsigned long)size)) == 0) {
		return((caddr_t)NULL);
	}

	/* If in adpative mode check to see if we have gone beyone the high
	 * water mark. 
	 */
	SPINLOCK_USAV(bcvirt_lock, context);
	bvalloc += size;
	SPINUNLOCK_USAV(bcvirt_lock, context);
	if (bvalloc > buffer_high)
		bc_shrink = 1;

#ifdef hp9000s800
	return((caddr_t) ptob(tmp));
#else
	tmp =  ptob(tmp - 1); 
    { 
	/* Allocate new page table if necessary */
	static unsigned int s300_high_water_pte = 0;
	unsigned int addr;

        addr = tmp+ptob(size);	/* could ideally subtract one from this */
	if (addr > s300_high_water_pte) {
		s300_high_water_pte = addr;
        	if (vm_alloc_tables(KERNVAS, addr) == NULL)
                	panic ("Could not allocate pte in bfget\n");

	}
    }

	return((caddr_t) tmp); 
#endif /* hp9000s800 */
}

/*
 * BEGIN_DESC 
 *
 * bffree()
 *
 * Return Value:
 *	The size of the chunk created by freeing the space.  rmfree() returns 
 *	the size of the map entry created by releasing the space.  This value
 *	is used by feeding frenzy to know when to stop freeing virtual space.
 *
 * Globals Referenced:
 *	bvalloc
 *
 * Description:
 *	bffree() releases the virtual space starting with "va" for "size"
 *	pages.  If the total virtual consumption goes below the low water 
 *	mark then unset bc_shrink.
 *
 *	We don't hold the SPINLOCK around the check to unset bc_shrink
 *	since it doesnt matter if we have a race condition.  bc_shrink is an
 *	indicator not a value.   If set upon the next call thats ok.  It's ok
 *	if the buffer cache goes a little below the low water mark before 
 *	unsetting bc_shrink.
 *
 * Algorithm:
 *	Call rmfree() to release space; hold onto return value.
 *
 *	SPINLOCK()
 *	decrement buffer cache virtual consumption
 *	SPINUNLOCK()
 *	if buffer cache virtual under low water mark then unset
 *		bc_shrink
 *
 * In/Out conditions: Locks held on entry...
 *
 * END_DESC
 */
u_int
bffree(va, size)
   register caddr_t va;		/* Starting virtual address where to free. */
   register unsigned long size; /* Amount to free in pages. */
{
   register u_int context;
   register u_int hole;
   u_int rmfree();

	VASSERT (size != 0);

#ifndef hp9000s800
	va += ptob(1);
#endif /* hp9000s800 */
	hole = rmfree(BUFMAP, size, (unsigned long) btorp((unsigned) va));

	SPINLOCK_USAV(bcvirt_lock, context);
	bvalloc -= size;
	SPINUNLOCK_USAV(bcvirt_lock, context);
	if ((bc_shrink) && (bvalloc < buffer_low))
		bc_shrink = 0;
	return(hole);
}


/*
 * BEGIN_DESC 
 *
 * bc_freepage()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None
 *
 * Description:
 *	bc_freepage() removes the physical pages starting from "vaddr" for
 *	"size/NBPG" pages.  The translations are removed from the virtual 
 *	address and the physical pages are placed in the buffer cache free 
 *	physical page list.
 *
 * Algorithm:
 *	for size pages remove the virtual to physical translation and place the
 *	free physical page in the buffer cache free list.
 *
 * In/Out conditions: 
 *	It is assumed the buffer header is locked during the time the pages
 *	are being freed.
 *
 * END_DESC
 */
void
bc_freepage(vaddr,size)
register caddr_t vaddr;
register int size;
{
	register int i;
	register int pfn;

	/* Make sure size is multiple of NBPG. */
	VASSERT((size % NBPG == 0));

	/* Delete translation for each page.  Place the free pages into the
	 * physical page pool.
	 */
	for (i=0; i<size; i+=NBPG) {
		pfn = hdl_vpfn(KERNELSPACE, vaddr);
		VASSERT(pfn != 0);
		hdl_deletetrans(KERNPREG, KERNELSPACE, vaddr, pfn);
		bcfree(&pfdat[pfn]);
		vaddr+= NBPG;	/* Next page. */
	}
}


#ifdef DYNAMIC_BC
/*
 * BEGIN_DESC 
 *
 * bc_freepage_hard()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None
 *
 * Description:
 *	bc_freepage() removes the physical pages starting from "vaddr" for
 *	"size/NBPG" pages.  The translations are removed from the virtual 
 *	address and the physical pages are returned to the system.  The
 *      See kdfree and  kdrele for example code.
 *
 * Algorithm:
 *	for size pages remove the virtual to physical translation and  
 *	free the physical page 
 *
 * In/Out conditions: 
 *	It is assumed the buffer header is locked during the time the pages
 *	are being freed.
 *
 * END_DESC
 */
void
bc_freepage_hard(vaddr,size)
register caddr_t vaddr;
register int size;
{
        register u_int context;
	register int i;
	register int pfn;
	register pfd_t *pfd;
        vm_sema_state;          /* semaphore save state */

	/* Make sure size is multiple of NBPG. */
	VASSERT((size % NBPG == 0));

	vmemp_lockx();          /* lock down VM empire */
        return_swap(btop(size));

	/* Delete translation for each page.  Place the free pages into the
	 * physical page pool.
	 */
	for (i=0; i<size; i+=NBPG) {
		pfn = hdl_vpfn(KERNELSPACE, vaddr);
		VASSERT(pfn != 0);
		hdl_deletetrans(KERNPREG, KERNELSPACE, vaddr, pfn);
		pfd = pfdat + pfn;
                pfd->pf_flags &= ~P_SYS; 
                VASSERT(pfd->pf_use == 1);
                freepfd(pfd);            
        SPINLOCK_USAV(bcphys_lock, context);
                bufpages--;             
        SPINUNLOCK_USAV(bcphys_lock, context);
		vaddr+= NBPG;	/* Next page. */
	}

	vmemp_unlockx();        /* free up VM empire */
}

/*
 * BEGIN_DESC 
 *
 * dbc_steal_free_pages()
 *
 * Return Value:
 *	Number of pages given back to VM systesm.
 *
 * Globals Referenced:
 *	None
 *
 * Description:
 *	dbc_steal_free_pages() takes pages off free list and gives them 
 *      back to the VM system.  It returns up to "size" number of pages
 *      but no more.  
 *      See kdfree and  kdrele for example code.
 *
 * Algorithm:
 *	for size pages free the physical page 
 *
 * In/Out conditions: 
 *
 * END_DESC
 */
int 
dbc_steal_free_pages (target_size)
int target_size;
{
	register int size;
	register pfd_t *pfd, *first_pfd;
        register u_int context;
        vm_sema_state;          /* semaphore save state */

                                             
        SPINLOCK_USAV(bcphys_lock, context);
        pfd = &bhead;     
	first_pfd = pfd->pf_next;
	size = 0;
        while ((pfd->pf_next != &bhead) && (size < target_size)){     
                pfd = pfd->pf_next;          
		size++;
        }                                     
        bhead.pf_next = (pfd)->pf_next;
        SPINUNLOCK_USAV(bcphys_lock, context);

	if (size <= 0) return (0);

	vmemp_lockx();          /* lock down VM empire */
        return_swap(size);

	/* Delete translation for each page.  Place the free pages into the
	 * physical page pool.
	 */
	target_size = size;
	while (target_size > 0) {
		pfd = first_pfd;
		first_pfd = pfd->pf_next;
                pfd->pf_flags &= ~P_SYS; 
                VASSERT(pfd->pf_use == 1);
                freepfd(pfd);            
        SPINLOCK_USAV(bcphys_lock, context);
                bufpages--;             
        SPINUNLOCK_USAV(bcphys_lock, context);
		target_size--;				
	}

	vmemp_unlockx();        /* free up VM empire */
	return (size);
}
#endif /* DYNAMIC_BC */


/*
 * BEGIN_DESC 
 *
 * bcfeeding_frenzy()
 *
 * Return Value:
 *	A pointer to a virtual address for "size" pages. 
 *
 * Globals Referenced:
 *	List globals referenced (eg. time, proc)
 *
 * Description:
 *	List what this function does.
 *
 * Algorithm:
 *	List high level steps in algorithm (should be a short list)
 *
 * In/Out conditions: Locks held on entry...
 *
 * END_DESC
 */
caddr_t 
bcfeeding_frenzy(size)
register int size;	/* Number of contiguous pages desired. */
{
	register struct buf *bp;
	register int finished = 0;
	register u_int size_created;
	register u_int bhl_context;
	register caddr_t addr = NULL;

#ifdef OSDEBUG
	frenzy++;
#endif OSDEBUG
	
#ifdef notdef
	/* 
	 * Originally I thought removing buffers from the free list instead of
	 * the empty list was a good idea, but some experimental data I gathered
	 * may show otherwise.  If the empty list was to consist of primarily 
	 * buffers with virtual space then scanning the list would be worth
	 * while.  If the buffers on the empty list are empty then scanning 
	 * the list is useless.  It was extremely difficult to hit this corner
	 * case, after all frenzies have to be occuring.  Once I was able to
	 * get the frenzies to occur I still noticed I was rarely freeing any
	 * virtual space from the empty headers.  One obvious problem is a 
	 * series of frenzies would cause several stripped buffers to be on 
	 * the empty list.  These buffers are useless to us, but take up time 
	 * to scan.  I also think the system tends to balance the free list 
	 * out, a grow matches a shrink and vice versa.  Because of what I saw 
	 * I think its best to feed on aged or lru buffers since we are 
	 * guaranteed virtual space there.  Perhaps a future enhancement would
	 * be keeping track of how many headers on the free list have space and
	 * base our decision to walk the list as a function of that number 
	 * versus the length of the list.
	 *
	 * XXX Revisit XXX  KP 1/22/92
	 */
xxxx -> SPINLOCK for S300
	BC_LOCK;
	for (bp = (&bfreelist[BQ_EMPTY])->av_forw; 
	     bp != (struct buf *)&bfreelist[BQ_EMPTY];
		  bp = bp->av_forw) {

		/* Buffer should not have any physical space. */
		VASSERT(bp->b_bufsize == 0);

		if (bp->b_virtsize) {
#ifdef OSDEBUG
			frenzy_free++;
#endif 
			size_created = bffree(bp->b_un.b_addr, bp->b_virtsize);
#ifdef OSDEBUG
			bcheadersize[bp->b_virtsize]--;
			bcheadersize[0]++;
#endif 
			bp->b_virtsize = 0;
			if (size_created >= size) {
				/* If this fails a race condition caused us to
				 * miss.  If so then keep going.
			 	 */
				if ((addr = bfget(size)) != NULL) {
xxxx -> SPINUNLOCK for S300
					BC_UNLOCK;
					return(addr);
				}
			}
		}
	}
	/* Didn't coalesce a large enough hole.  Unlock the buffer cache 
	 * and start on aged buffers.
	 */
xxxx -> SPINUNLOCK for S300
	BC_UNLOCK;
#endif notdef

	while (!finished) {
		/* Get a buffer who's virtual space we are stealing. */
		bp = getnewbuf((B_allocbuf|B_unknown), BC_PHYSICAL);

		/* First free the physical pages. */
		if (bp->b_bufsize)
			bc_freepage(bp->b_un.b_addr, bp->b_bufsize);
	
		/* Now free the virtual pages and see if we created a large 
		 * enough hole.
	 	 */
		size_created = bffree(bp->b_un.b_addr, bp->b_virtsize);
		if (size_created >= size) {
			/* If this fails a race condition caused us to miss.
			 * If so then keep going.
			 */
			if ((addr = bfget(size)) != NULL) {
				finished = 1;
			}
		}
		
#ifdef OSDEBUG
		bcheadersize[bp->b_virtsize]--;
		bcheadersize[0]++;
#endif OSDEBUG
		/* Zero out the counts on the buffer having all of its virtual
		 * and physical space released.
		 */
		bp->b_bufsize = 0;
		bp->b_bcount = 0;
		bp->b_virtsize = 0;
		bp->b_un.b_addr = (caddr_t)NULL;

#ifdef hp9000s800
		BC_LOCK;
#else 
		SPINLOCK(buf_hash_lock);
#endif /* hp9000s800 */
		bremhash(bp);
		binshash(bp, &bfreelist[BQ_EMPTY]);
#ifdef hp9000s800
		BC_UNLOCK;
#else 
		SPINUNLOCK(buf_hash_lock);
#endif /* hp9000s800 */
		bp->b_dev = (dev_t)NODEV;
		bp->b_error = 0;
		/*
	 	* this is a bug fix.
	 	* Since getnewbuf() sets the buffer header flag word to
	 	* B_BUSY (clearing out all other flags), the flag
	 	* word must have B_INVAL set before the brelse().  If not
	 	* the buffer will have a flag word of 0.  This is an
	 	* illegal buffer state.  
	 	* 
	 	* Perhaps the buffer should be unlinked and have its
	 	* dev clobbered, but this suffices.
	 	*/
		bp->b_flags |= B_INVAL;
		brelse(bp);
	}
	return(addr);
}


/*
 * BEGIN_DESC 
 *
 * allocbuf()
 *
 * Return Value:
 *	Returns the buffer header passed in "tp" with at least "size" physical
 *	and virtual space.
 *
 * Globals Referenced:
 *	List globals referenced (eg. time, proc)
 *
 * Description:
 *	allocbuf() is responsible for taking the user buffer "tp" and making
 *	sure it has at least "size" pages worth of both virtual and physical 
 *	data.
 *
 *	If the buffers physical size is equal to the size requested,
 *	simply return.  Only growing/shrinking require extra work.
 *
 *	Growing/Shrinking can do different operations depending on whether it
 *      the buffer cache is in shrink mode.   Originally we allocated all the
 *      virtual space for buffer headers if the total consumption was
 *      guaranteed to be less than 1 gigabyte.  However, there were pitfalls to
 *      allowing "virtual holes" in the system, such as direct caches favoring
 *      lines, favoring certain buckets in pdir 1.2 implementation(PCX-T).
 *      These problems changed the buffer cache to always initialized headers
 *      with the minimum amount of virtual space, allowing the buffer cache to
 *      grow as the 9.A investigation report describes.
 *
 *      Since each header is initialized with the minimum, the buffer cache is
 *      always in an adaptive mode.  In this mode buffer headers can
 *	have less than MAXBSIZE worth of virtual space.  Before mapping
 *	physical pages to these buffers we must make sure there is enough 
 *	virtual space to back it up.  The algorithm allows up to a high water 
 *	mark worth of virtual space before it begins enforcing buffers to 
 *	have the same amount of physical as virtual.  When it gets to the high
 *	water mark the flag, bc_shrink, is set.  In this mode buffers are not
 *	allowed to have more virtual than physical.  The 9.A Design document 
 *	explains this in more detail
 *
 *	Other than the special processing above, allocbuf() makes sure "tp" has
 *	at least "size" physical pages backing it up.
 *
 * Algorithm:
 *
 *	If buffers physical space equals size requested then return.
 *
 *	if (buffer is shrinking)
 *		if (in shrink mode) {
 *			allocate new smaller virtual range.
 *
 *			If unable to get space go into feeding frenzy
 *			
 *			remap "size" pages to new address.
 *
 *			Free the remaining pages to the BC physical page pool
 *
 *			Free the original virtual space.
 *
 *			Assign b_addr with newly mapped space.
 * 		else
 *			Free unneeded pages to BC physical page pool
 *
 *	}
 *
 *	We are growing!
 *
 *      if (in shrink mode or virtual space is not large enough) {
 *
 *		Allocate new virtual space large enough to hold "size"
 *
 *		If unable to get space go into feeding frenzy
 *
 *		Remap the original pages.
 *
 *		release old virtual space.
 *
 *		update b_addr to point to new space.
 *
 *	}
 *
 *	while (buffer does not have "size" physical pages) {
 *
 *		Check physical page pool.  If it has one then remove it, map it
 *		and continue with the while loop.
 *
 *		No free pages so we remove pages from existing buffers
 *
 *		Grab an existing buffer
 *
 *		Remap its pages into our buffer.
 *
 *		If in shrink mode {
 *
 *			release the rest of the buffers pages, if any
 *
 *			free the buffers virtual space
 *
 *			Update buffers physical size and virtual address(NULL)
 *
 *		} else {
 *
 *			Update buffers physical size and virtual address(NULL)
 *			
 *		}
 *
 *		Put the stripped buffer back into the appropriate BC list.  If
 *		all the physical memory was taken, then place it in the empty
 *		queue, otherwise, place it back in the AGE list.
 *
 *		}
 *
 *
 *	return the buffer.
 * }
 *
 * In/Out conditions: Locks held on entry...
 *	
 *
 * END_DESC
 */

int
allocbuf(tp, size)
register struct buf *tp; /* Pointer to buffer header to possibly grow/shrink. */
register int size;	 /* Size the buffer should be on exit. */
{
	register struct buf *bp;
	register pfd_t *pfd;
	register int sizealloc, take;
	u_int bhl_context;
	register caddr_t tp_addr;
	caddr_t bcfeeding_frenzy(), bfget();
	void bc_freepage();

#ifdef OSDEBUG
	total_allocbufs++;
#endif
	VASSERT(!ON_ICS);

	VASSERT(size > 0);

	sizealloc = roundup(size, (int)ptob(1));

	/*
	 * Buffer size does not change
	 */
	if (sizealloc == tp->b_bufsize) {
#ifdef OSDEBUG
		nochange++;
#endif 
		goto out;
	}

	if (sizealloc < tp->b_bufsize) {
		/*
	 	 * Buffer size is shrinking.
	 	 *
		 * We no longer remap the extra pages into an empty
		 * buffer.  Now with buffer cache pages under the pfdat[], we 
		 * have a physical page resource manager to hold the pages.
		 * Not remapping the pages to empty buffers forces a change in
		 * getnewbuf().  getnewbuf() seeing free pages in the physical
		 * pool and having an empty header, returns the empty header.
		 * In the past getnewbuf() returned only headers with physical
		 * space.
                 *
		 * First check to see if we must shrink the buffers virtual
		 * address space.  We don't if the current virtual total
		 * obtained is within the maximum allowed. 
		 *
		 * If the buffer cache is not above the high water mark, then 
		 * bc_shrink is always 0.
		 */

#ifdef OSDEBUG
		shrink++;
#endif

		if ((bc_shrink) && ((tp->b_virtsize*NBPG) > sizealloc)) {
#ifdef OSDEBUG
			shrink_over++;
#endif
			/* 
			 * Just free the virtual space and physical pages
			 * not needed.
			 */

			/* Free the excess pages to the physical
			 * page pool.
			 */
			bc_freepage(tp->b_un.b_addr + sizealloc,
				      (int)tp->b_bufsize - sizealloc);

			/* Now release the unused virtual address space.
			 */
			bffree(tp->b_un.b_addr + sizealloc, 
				(tp->b_virtsize - (sizealloc/NBPG)));

#ifdef OSDEBUG
			bcheadersize[tp->b_virtsize]--;
			bcheadersize[(sizealloc)/NBPG]++;
#endif OSDEBUG
			/* Now set the new virtual address size. */
			tp->b_virtsize = (sizealloc)/NBPG;

		} else {

			VASSERT(sizealloc >= NBPG);

			bc_freepage((tp->b_un.b_addr + sizealloc),
				((int)tp->b_bufsize - sizealloc));

		}
		/* Adjust the physical size of the buffer. */
		tp->b_bufsize = sizealloc;

		goto out;
	}

	/**************/
	/* Growing!!! */
	/**************/

#ifdef OSDEBUG
	grow++;
#endif 
	/*
	 * Does the buffer have enough virtual space.
	 */
	if (((bc_shrink) || ((tp->b_virtsize*NBPG) < sizealloc))) {

#ifdef OSDEBUG
		grow_over++;
#endif
		/* Grow/shrink address for buffer. */
		tp_addr = bfget((sizealloc - 1 + NBPG)/NBPG);

		/* If the address returned is null the buffer cache
		 * virtual map is heavily fragmented.  We now go into
		 * feeding frenzy where buffer headers have both their
		 * virtual and physical space removed.  This occurs 
		 * until we can allocate "sizealloc" pages from the 
		 * map.
		 */
		if (tp_addr == NULL) {
			tp_addr = bcfeeding_frenzy((sizealloc - 1 + NBPG)/NBPG);

			VASSERT(tp_addr != NULL);
		}

                if(tp->b_virtsize) {

		   /* Move ALL the existing pages. */
                   if(tp->b_bufsize)
			pagemove(tp->b_un.b_addr, tp_addr, tp->b_bufsize);

		   /* Remove the old virtual address space. */
		   bffree(tp->b_un.b_addr, tp->b_virtsize);

#ifdef OSDEBUG
		   bcheadersize[tp->b_virtsize]--;
#endif OSDEBUG
		}
#ifdef OSDEBUG
		bcheadersize[(sizealloc)/NBPG]++;
#endif OSDEBUG
		/* Reset the virtual address and size. */
		tp->b_un.b_addr = tp_addr;
		tp->b_virtsize = btop(sizealloc);

	} 

	/*
	 * More buffer space is needed. Get it out of buffers on
	 * the "most free" list, placing the empty headers on the
	 * BQ_EMPTY buffer header list.
	 */
	while (tp->b_bufsize < sizealloc) {

		/* Before we start removing pages from buffers check the page 
		 * pool.  If memory pressure is tight, it may decide not to
                 * allocate another page even if there is one available.
		 */
		bcalloc(pfd);
		if (pfd != (pfd_t *)NULL) {
#ifdef OSDEBUG
			pagesteal++;
#endif
			/* Map in the page and continue. */
#ifdef PFDAT32
			VASSERT((pfd - pfdat) != 0);
			BC_MAPPAGE(&tp->b_un.b_addr[tp->b_bufsize],(pfd-pfdat));
#else
			VASSERT(pfd->pf_pfn != 0);
			BC_MAPPAGE(&tp->b_un.b_addr[tp->b_bufsize],pfd->pf_pfn);
#endif
			tp->b_bufsize += NBPG;
			continue;
		} else {
			/* We only come here when pages are being removed from 
			 * existing buffers.
		 	 * 
		 	 * Only allocate a destination buffer if we are not
			 * taking all the pages from the buffer and we are
			 * greater than our maximum virtual space requirements.
		 	 */
#ifdef OSDEBUG
			bufsteal++;
#endif
			take = sizealloc - tp->b_bufsize;
			bp = getnewbuf((B_allocbuf|B_unknown), BC_PHYSICAL);
			
			if (take >= bp->b_bufsize)
				take = bp->b_bufsize;

			/* First remap the pages.  Any excess pages
		 	 * are freed into the physical page pool.
			 */
			pagemove(&bp->b_un.b_addr[bp->b_bufsize - take],
		    		&tp->b_un.b_addr[tp->b_bufsize], take);

			/* There are advantages to keeping the remaining pages
			 * with the buffer.  You don't break translations that
			 * we will need later when the buffer is re-used.
			*/
			if (bc_shrink) {
#ifdef OSDEBUG
				grow_steal++;
#endif
				if (take != bp->b_bufsize) {
					/* Free the excess pages. */
					bc_freepage(bp->b_un.b_addr, 
						(bp->b_bufsize - take));
				}
				/* Now unmap the virtual address space 
				 * associated with the buffer from whom we 
				 * just stole the pages.
		 	 	 */
				bffree(bp->b_un.b_addr, bp->b_virtsize);

#ifdef OSDEBUG
				bcheadersize[bp->b_virtsize]--;
				bcheadersize[0]++;
#endif OSDEBUG
				/* Reset the virtual address and size of 
				 * virtual and physical memory.
			 	 */
				bp->b_un.b_addr = NULL;
				bp->b_virtsize = 0;
				bp->b_bufsize = 0;
	
				tp->b_bufsize += take;
			} else {
				tp->b_bufsize += take;
				bp->b_bufsize = bp->b_bufsize - take;
			}

			if (bp->b_bcount > bp->b_bufsize)
				bp->b_bcount = bp->b_bufsize;
			if (bp->b_bufsize <= 0) {
#ifdef hp9000s800
		BC_LOCK;
#else 
		SPINLOCK(buf_hash_lock);
#endif /* hp9000s800 */
				bremhash(bp);
				binshash(bp, &bfreelist[BQ_EMPTY]);
#ifdef hp9000s800
		BC_UNLOCK;
#else 
		SPINUNLOCK(buf_hash_lock);
#endif /* hp9000s800 */
				bp->b_dev = (dev_t)NODEV;
				bp->b_error = 0;
			}
			/*
			 * this is a bug fix.
			 * Since getnewbuf() sets the buffer header flag word to
			 * B_BUSY (clearing out all other flags), the flag
			 * word must have B_INVAL set before the brelse().  If not
			 * the buffer will have a flag word of 0.  This is an
			 * illegal buffer state.  
			 * 
			 * Perhaps the buffer should be unlinked and have its
			 * dev clobbered, but this suffices.
			 */
			bp->b_flags |= B_INVAL;
			brelse(bp);
		}
	}
out:
	VASSERT(tp->b_virtsize >= (tp->b_bufsize/NBPG));

#ifdef FSD_KI
	/* Save pid that last used this buffer */
	tp->b_upid = u.u_procp->p_pid;

	/* Save site(cnode) that last used this buffer */
	tp->b_site = u.u_procp->p_faddr; 

	/* Stamp with last usage time */
	KI_getprectime(&tp->b_timeval_qs);
#endif /* FSD_KI */

	tp->b_bcount = size;
	return (1);
}

#ifdef DYNAMIC_BC
/* 
 *  dbc_freemem gives the pages belonging to the buffer back to the VM system.
    Only release the virtual addresses if we are in bc_adaptive mode, and 
 *  bc_shrink is set. 
*/
dbc_freemem (bp)
struct buf *bp;
{
  if (bp->b_bufsize > 0) {
    bc_freepage_hard(bp->b_un.b_addr, (int)bp->b_bufsize);
    bp->b_bufsize = 0;

    /* Now release the unused virtual address space.  We must have some 
     * because we had some pages mapped in.
    */
    if (bc_shrink) {
        bffree(bp->b_un.b_addr, bp->b_virtsize);
        bp->b_virtsize = 0;
    }
    bp->b_dev = (dev_t)NODEV;
    brelvp(bp);                                 /* check this. */
    bp->b_error = 0;
    bp->b_bcount = 0;
    bp->b_flags |= B_INVAL;
  }
}
#endif /* DYNAMIC_BC*/


/*
 * Initialize the buffer I/O system by creating some number of buffers and
 * placing them on the free list.
 */
binit()
{
	register struct buf *bp, *dp;
	int i;
	register pfd_t *pfd;
	extern struct buf * dbc_alloc();
	int base, residual, bsize;
	int amount;
	extern pfd_t bhead;	
	extern int non_empty, toppriority;
	struct bufqhead *bqp;

	bhead.pf_next = &bhead;		/* empty list of free pages */

	/* malloc up the minimum number of buffers required to get the system
	 * started.  If the customer has pre-configured the size of the buffer
	 * cache, then we get all the buffer headers now.
	 * HP REVISIT need to decide if nbuf is set or not.
	 */
	nbuf = 0;
	while (nbuf < dbc_nbuf) {
            if ((bp = dbc_alloc (&i)) == NULL)
	      panic ("Failed to allocate memory for buffer cache");
	    dp = bp;
	    while (dp->b_nexthdr != NULL) dp = dp->b_nexthdr; 
	    dp->b_nexthdr = dbc_hdr;
	    dbc_hdr = bp;
	    nbuf += i;
	}			
	non_empty = toppriority = nbuf;

#ifdef hp9000s800
	if ((nbuf * (MAXBSIZE/NBPG)) > btop(SIZEV_THIRDQUAD)) 
		buffer_high = btop(SIZEV_THIRDQUAD);  /* 1 gigabyte. */
#endif /* hp9000s800 */


	for (bqp = bfreelist; bqp < &bfreelist[BQUEUES]; bqp++) {
		bqp->b_forw = bqp->b_back = bqp->av_forw = bqp->av_back = bqp;
	}
	bf_initialize();

	/*
	Don't need to call clockmemreserve (dbc_bufpages) because lockable_mem
	has not been initialized yet.  lockable_mem is based in part on freemem
	which will be decreased by the minimum buffer cache size.
	*/
	bufpages = 0;
	base = dbc_bufpages / nbuf;
	residual = dbc_bufpages % nbuf;
	for (bp = dbc_hdr, i=0; bp != NULL; bp=bp->b_nexthdr, i++) {

		if (i < residual) {
			amount = base + 1;
			bsize = ptob(base + 1);
		} else {
			amount = base;
			bsize = ptob(base);
		}

		if (amount == 0) amount = 1;
                bp->b_virtsize = amount;
                /* If amount is zero bfget should return a NULL which
                 * is what we want.
                */
                bp->b_un.b_addr = bfget(amount);
                VASSERT(((bp->b_un.b_addr == NULL) && (amount == 0)) ||
                         (bp->b_un.b_addr != NULL));

		bp->b_bufsize = 0;
                if (bp->b_virtsize > 0) {
#ifdef hp9000s800
			bp->b_spaddr = ldsid(bp->b_un.b_addr);
#else 
			bp->b_spaddr = KERNELSPACE;
#endif /* hp9000s800 */

        	    while (bp->b_bufsize < bsize) {
                    	bcalloc_init(pfd);
                    	if (pfd == (pfd_t *)NULL) 
				panic ("Insufficient buffer cache memory\n");

                    	/* Map in the page and continue. */
#ifdef PFDAT32
                    	VASSERT((pfd - pfdat) != 0);
                    	BC_MAPPAGE(&bp->b_un.b_addr[bp->b_bufsize], (pfd-pfdat));
#else
                    	VASSERT(pfd->pf_pfn != 0);
                    	BC_MAPPAGE( &bp->b_un.b_addr[bp->b_bufsize], 
				    pfd->pf_pfn);
#endif
                    	bp->b_bufsize += NBPG;
                    }
		}

		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
}

