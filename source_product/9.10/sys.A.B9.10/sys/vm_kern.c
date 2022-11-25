/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_kern.c,v $
 * $Revision: 1.6.83.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:25 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/malloc.h"
#include "../machine/vmparam.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/sema.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../h/var.h"
#include "../h/mnttab.h"
#include "../h/mount.h"
#include "../h/vas.h"
#include "../h/buf.h"
#include "../h/map.h"
#include "../h/pfdat.h"
#include "../h/proc.h"
#include "../h/swap.h"
#include "../h/debug.h"
#include "../h/vm.h"
#include "../h/tuneable.h"
#include "../machine/param.h"

extern int mem_free;
extern int swapmem_cnt;

/*
 * Kernel Memory allocation file.  This file contains all of the HARDWARE 
 * INDEPENDENT routines used to allocate kernel memory.
 */

/*
 * A routine for allocating one page of memory for the dynamic buffer cache.
 *      If kdget or kalloc is modified then changes should be reflected
 *      here as well. 
 *	RETURN - pointer to the pfd 
 */
pfd_t *
dbc_kdget()
{
	void memunreserve();
        register pfd_t *pfd;
	vm_sema_state;		/* semaphore save state */

	/*
 	* Steal memory from swapper,
 	* if reservation required.
 	* Fail out if we can't get it
 	*/
	if (steal_swap((reg_t *)NULL, 1, 1) == 0) {
		return(NULL);
	}
	if (cmemreserve(1) == 0) {
		return_swap(1);
		return(NULL);
	}

       	/* 
 	* Now everything has been reserved, so we can get the
 	* actual pages from the free pool.
 	*
 	* Allocate the physical pages into the allocated vaddrs.
 	* In-line kdget, keep them consistent.
 	*/
        if ((pfd = allocpfd()) == (pfd_t *)NULL)
        	goto bailout;
	pfdatdisown(pfd);
        pfd->pf_flags |= P_SYS;

        return(pfd);

bailout:
#ifdef OSDEBUG
	printf ("dbc_kdget: allocpfd returned NULL");
#endif
        /*
         * memunreserve the pages that were not allocated.
         */
        memunreserve(1);
	
        /*
         * Return the swap memory reservation.
         */
        return_swap(1);
        return(NULL);
}
/*
 * A flag based routine for allocating memory.
 * convert all of the callers over to it and then remove
 * kdalloc and kdalloc_nowait.
 *
 * Allocate system virtual address space and
 * real allocate pages.
 *	size - size of virtual segment in pages
 *	flags - zero, sleep, swap reserve
 *	RETURN - virtual address of the virtual segment
 */
caddr_t
kalloc(size, flag)
        register unsigned long size;
        int flag;
{
	void memunreserve();
        register unsigned int sva, count, i;
        register pfd_t *pfd;
        unsigned alloc_addr;
	extern long rmalloc();
        int nowait = (flag&KALLOC_NOWAIT);
        int nozero = (flag&KALLOC_NOZERO);
        int noreserve = (flag&KALLOC_NORESV);
	vm_sema_state;		/* semaphore save state */

        /* 
	 * Allocate space in the resource map. The map cant give
	 * out 0, so there is an "off by one" to account for.
	 */
        if ((sva = (unsigned int)ptob(rmalloc(sysmap, size))) == 0) {
                if (nowait)
                        return(NULL);
                else
                        panic("kalloc: out of kernel virtual space");
        }
        sva -= ptob(1);		
        alloc_addr = sva;

	if (!noreserve)
		if (nowait) {
			/*
			 * Steal memory from swapper,
			 * if reservation required.
			 * Fail out if we can't get it
			 */
			if (steal_swap((reg_t *)NULL, size, 1) == 0) {
				rmfree(sysmap, size,(unsigned long)btop(sva)+1);
				return(NULL);
			}
			if (cmemreserve(size) == 0) {
				return_swap(size);
				rmfree(sysmap, size,(unsigned long)btop(sva)+1);
				return(NULL);
			}
		} else {
			/*
			 * Block waiting for memory to arrive.
			 */
			(void)steal_swap((reg_t *)NULL, size, 0);

			/*
			 * get mp vm empire sema to guard the
			 * the crit region in memreserve where
			 * freemem is manipulated and slept on
			 * This should be fixed for MP as its bad
			 * for MP performance, very bad.
			 */
			vmemp_lockx();          /* lock down VM empire */
			(void)memreserve((reg_t *)NULL, size);
			vmemp_unlockx();        /* free up VM empire */
		}

	/* 
	 * since the memory we are stealing is locked down, we must 
	 * use lockable memory to avoid deadlock possiblites
	 */

	steal_lockmem(size);


        /* 
	 * Now everything has been reserved, so we can get the
	 * actual pages from the free pool.
	 *
	 * Allocate the physical pages into the allocated vaddrs.
	 * In-line kdget, keep them consistent.
	 */
        for (count = 0; count < size; ++count, sva += ptob(1)) {
                if ((pfd = allocpfd()) == (pfd_t *)NULL)
                        goto bailout;
		pfdatdisown(pfd);
                pfd->pf_flags |= P_SYS;
#ifdef PFDAT32
                if (hdl_addtrans(KERNPREG, KERNELSPACE, sva, 0, pfd - pfdat)) {
#else
                if (hdl_addtrans(KERNPREG, KERNELSPACE, sva, 0, pfd->pf_pfn)) {
#endif
			pfd->pf_flags &= ~P_SYS;
			freepfd(pfd);
			goto bailout;
		}
        }

        if (!(nozero))
                hdl_zero_page(KERNELSPACE, (caddr_t)alloc_addr,(int)ptob(size));
        return((caddr_t) alloc_addr);

bailout:
        VASSERT(nowait);

        /*
         * back out of the pages allocated and return the space.
         */
        for (i = 0, sva = alloc_addr; i < count; i++, sva += ptob(1)) {
                int pfn;
                pfn = hdl_vpfn(KERNELSPACE, sva);
		hdl_deletetrans(KERNPREG, KERNELSPACE, sva, pfn);
                pfdat[pfn].pf_flags &= ~P_SYS;
                freepfd(&pfdat[pfn]);
        }

	if (!noreserve)
	{
		/*
		 * memunreserve the pages that were not allocated.
		 */
		memunreserve(size - count);

		/*
		 * Return the swap memory reservation.
		 */
		return_swap(size);
	}

        /*
         * Return the virtual address range.
         */

	lockmemunreserve(size);
        rmfree(sysmap, size, btop(alloc_addr)+1);
        return(NULL);
}

/*
 * Allocate a page frame from the free list.
 * PDIR entry of returned frame is locked.
 *      Code in dbc_freepage is modeled on this routine.  Any changes here 
 *      should also be reflected there.
 */

kdget(sva, size)
	register caddr_t 	sva;
	register unsigned	size;
{
	register pfd_t *pfd;
	register int count;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	/*
	 * since the swapper shares systems memory, let the
	 * swapper know that some memory is being wired down
	 */

	(void) steal_swap((reg_t *) 0, (int)size, 0);

	memreserve((reg_t *) 0, size);
		
	steal_lockmem(size);

	for(count = (int) size; --count >= 0; sva += ptob(1)) {
		pfd = allocpfd();
		pfdatdisown(pfd);
		pfd->pf_flags |= P_SYS;
#ifdef PFDAT32
		hdl_addtrans(KERNPREG, KERNELSPACE, sva, 0, pfd - pfdat);
#else
		hdl_addtrans(KERNPREG, KERNELSPACE, sva, 0, (int) pfd->pf_pfn);
#endif
	}
	vmemp_unlockx();	/* free up VM empire */
}


/*
 * Free up a system virtual segment.
 *	sva - system virtual start of virtual segment
 *	size - number of pages in the v.s. to free
 *	RETURN - UNDEFINED
 *      Code in bc_freepage is modeled on this routine.  Any changes here 
 *      should also be reflected there.
 */

kdfree(sva, size)
   register caddr_t	sva;
   register unsigned long size;
{
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	kdrele(sva, (unsigned int)size);
	
	/*
	 * Free up the space in the systems map of free virtual space.
	 */
	rmfree(sysmap, size, (unsigned long) btorp((unsigned)sva + ptob(1)));
	vmemp_unlockx();	/* free up VM empire */
}


/* Free up a page frame.  PDIR for page frame must be locked. 
 *      Code in bc_freepage is modeled on this routine.  Any changes here 
 *      should also be reflected there.
*/

kdrele(sva, size)
   register caddr_t	sva;
   register unsigned	size;
{
   register pfd_t *pfd;
   register int count;

	/*
	 * since the swapper shares system memory, update swap counts
	 */
	return_swap(size);
	lockmemunreserve(size);

	for (count = (int) size; --count >= 0; sva += ptob(1)) {
		pfd = pfdat + hdl_vpfn(KERNELSPACE, sva);

		/* unvirtualize the page & unlock the PDIR */
#ifdef PFDAT32
		hdl_deletetrans(KERNPREG, KERNELSPACE, sva, pfd - pfdat);
#else
		hdl_deletetrans(KERNPREG, KERNELSPACE, sva, (int) pfd->pf_pfn);
#endif

		/*	Free pages that aren't being used by anyone else
		 */
		pfd->pf_flags &= ~P_SYS;
		VASSERT(pfd->pf_use == 1);
		freepfd(pfd);
	}
}

#ifdef NEVER_CALLED
/*
 * Allocate virtual space, don't allocate any real memory
 * to go with it.
 */
caddr_t
kdvmap(size)
   register unsigned size;
{
   register unsigned int tmp;
   extern long rmalloc();

	if ((tmp = (unsigned int)rmalloc(sysmap, (unsigned long)size)) == 0)
		panic("kdvmap: out of virtual space");
	return((caddr_t) ptob(tmp - 1));
}

/*
 * Free up virtual space, don't free any real memory.
 */
kdunmap(va, size)
   register caddr_t va;
   register unsigned long size;
{

	rmfree(sysmap, size, (unsigned long) btorp((unsigned) va) + 1);
}
#endif /* NEVER_CALLED */


/*
 *	OUTMODED: Replace with direct calls to kalloc in your code
 * Allocate system virtual address space and
 * real allocate pages.
 *	size - size of virtual segment in pages
 *	RETURN - virtual address of the virtual segment
 */
caddr_t
kdalloc(size)
    unsigned long size;
{
    return(kalloc(size, 0));	/* zero memory, sleep if necessary */
}


/*
 *	OUTMODED: Replace with direct calls to kalloc in your code
 * Allocate system virtual address space and real allocate pages.
 *	size - size of virtual segment in pages
 *	RETURN - virtual address of the virtual segment, or NULL
 */
caddr_t
kdalloc_nowait(size)
    unsigned long size;
{
	return(kalloc(size, KALLOC_NOWAIT));	/* zero memory, no sleep */
}


/*
 * wmemall - allocate kernel system memory.
 */
/*ARGSUSED*/
caddr_t
wmemall(nproc, n)
   int (*nproc)();
   register int n;
{
	return(kalloc( (unsigned long) btorp(n), KALLOC_NOZERO));
}

/*
 * zmemall - allocate and zero kernel system memory.
 */
/*ARGSUSED*/
caddr_t
zmemall(nproc, n)
   int (*nproc)();
   register int n;
{
	return(kalloc( (unsigned long) btorp(n), 0));
}

/*
 * wmemfree - free previously allocated kernel memory.
 */
wmemfree(va, n)
   register caddr_t va;
   register int n;
{
	kdfree(va, (unsigned long) btorp(n));
}



/*
 * vmemall - Dummy routine to do the compile
 */
/*ARGSUSED*/
vmemall(vfd,size,p,type)
	struct vfd *vfd;
	int	size;
	struct proc *p;
	int	type;
{
	printf("vmemall: Should never be called.\n");
	Qstop(1);
}

/*
 * memall - allocate memory
 */
/*ARGSUSED*/
memall(vfd,size,p,type,preempt)
	struct vfd *vfd;
	int	size;
	struct proc *p;
	int	preempt, type;
{
	printf("memall: Should never be called\n");
	Qstop(1);
}

Qstop(stop)
register int	stop;
{
	while (stop)
		;
}




