/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_devswap.c,v $
 * $Revision: 1.14.83.4 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/05/05 15:29:04 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/time.h"
#include "../h/swap.h"
#include "../h/pregion.h"
#include "../h/region.h"
#include "../h/pfdat.h"
#include "../h/vnode.h"
#include "../h/signal.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/debug.h"
#include "../h/tuneable.h"

#include "../h/sar.h"
#include "../h/vmmeter.h"
#include "../h/sysinfo.h"
#include "../h/reboot.h"
#include "../dux/cct.h"
/*
 * max number of pages that devswap_pagein may bring in at a time
 */
#define DEVSWAP_MAXPGIN (MAX(MAXPREP, NETMAXPG))

extern int pageiodone();
extern int freemem, parolemem, lotsfree;
extern int maxpageout, maxpendpageouts, prepage;
extern int stealvisited;

extern int devtovp_open();
extern int devtovp_close();
extern int devtovp_inactive();
extern int devtovp_strategy();
extern int devtovp_badop();
extern int devswap_strategy();
extern int devswap_prefill();
extern int devswap_pagein();
extern int devswap_pageout();
extern int devswap_dbddup();
extern int devswap_dbddealloc();

struct vnodeops devswap_vnode_ops = {
	devtovp_open,
	devtovp_close,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_badop,
	devtovp_inactive,
	devtovp_badop,
	devswap_strategy,
	devtovp_badop,
	devtovp_badop,
        devtovp_badop,
#ifdef ACLS
        devtovp_badop,
        devtovp_badop,
#endif /* ACLS */
#ifdef POSIX
        devtovp_badop,
        devtovp_badop,
#endif /* POSIX */
        devtovp_badop,
        devtovp_badop,
        devtovp_badop,
        devtovp_badop,
	devswap_prefill,
	devswap_pagein,
	devswap_pageout,
	devswap_dbddup,
	devswap_dbddealloc
};
typedef struct swargs {
	preg_t *prp;
	int flags;
	int migrate;
	size_t start;
	size_t end;
	short run;
} swargs_t;

/*
 * Create the swap device vp and initialize it
 * to point at the devswap_vnode_ops array.
 */
struct vnode *swapdev_vp;
extern int swapdev;

devswap_init()
{
	swapdev_vp = devtovp((dev_t)swapdev);
	swapdev_vp->v_op = &devswap_vnode_ops;

}

/*ARGSUSED*/
devswap_prefill(prp)
	preg_t *prp;
{
	panic("devswap_prefill: called");
}
	
int
devswap_pagein(prp, wrt, space, vaddr)
	preg_t *prp;
	int wrt;
	space_t space;
	caddr_t vaddr;
{
	dbd_t *dbd2;
	vfd_t *vfd2;
	int pgindx;
	pfd_t *pfd;
	int maxprepage, i, j, count;
	int curblk, type, swpi;
	swpdbd_t *dbd, *dbdp;
	vfd_t *vfd, *vfdp;
	int swapdel;
	int error;
	space_t nspace;
	caddr_t nvaddr;
	reg_t *rp = prp->p_reg;
	u_int context;
        int pfn[DEVSWAP_MAXPGIN];
	extern int reboot_option;
	int saved_skeep_state = 0;
	void memunreserve();

	
	/*	
	 * Find vfd and dbd as mapped by region
	 */
	pgindx = (btop((unsigned)vaddr - (int)prp->p_vaddr) + prp->p_off);

	FINDENTRY(rp, pgindx, &vfd2, &dbd2);
	vfd = vfd2;
	dbd = (swpdbd_t *)dbd2;
	
	/*	
	 * If the page we want is in memory already, take it
	 */
	if (pfd = pageincache(swapdev_vp, ((dbd_t *)dbd)->dbd_data)) {
#ifdef PFDAT32
		vfd->pgm.pg_pfn = pfd - pfdat;	
#else
		vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
		vfd->pgm.pg_v = 1;
		rp->r_nvalid++;
		if (!hdl_cwfault(wrt, prp, pgindx)) {
			return(0);
		} else {
			minfo.cache++;
			cnt.v_pgrec++;
                        cnt.v_pgfrec++;
			u.u_ru.ru_minflt++;	/* count reclaims */
                        if (prp->p_type == PT_TEXT)
                        	cnt.v_xsfrec++;
			return(1);
		}
	}
	u.u_ru.ru_majflt++;		/* It's a real fault, not a reclaim */

	/*
	 * How many pages to bring in at once?
	 */
	maxprepage = MIN(prepage, DEVSWAP_MAXPGIN);
	if (maxprepage >= freemem)
	    maxprepage = freemem/2 + 1;
	VASSERT(maxprepage > 0);

	/*
	 * Otherwise, get page(s) from disk. First reserve memory
	 * for the vfdfill below.  We must do this now to prevent
	 * problems when we go to insert the page in the hash.
	 */
	if (memreserve(rp, (unsigned int)maxprepage)) {
		/*
		 * The vfd/dbd pointers could have become stale when
		 * we slept without the region lock in memreserve()
		 */
		FINDENTRY(rp, pgindx, &vfd2, &dbd2);
		vfd = vfd2;
		dbd = (swpdbd_t *)dbd2;
		/*	
		 * We went to sleep waiting for memory.
		 * check if the page we're after got loaded in
		 * the mean time.  If so, give back the memory
		 * and return
		 */
		if (vfd->pgm.pg_v) {
			memunreserve((unsigned int)maxprepage);
			return(1);
		}
		if (pfd = pageincache(swapdev_vp, ((dbd_t *)dbd)->dbd_data)) {
			memunreserve((unsigned int)maxprepage);
#ifdef PFDAT32
			vfd->pgm.pg_pfn = pfd - pfdat;
#else
			vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
			vfd->pgm.pg_v = 1;
			rp->r_nvalid++;
			if (!hdl_cwfault(wrt, prp, pgindx)) {
				return(0);
			} else {
				minfo.cache++;
				cnt.v_pgrec++;
				cnt.v_pgfrec++;
				if (prp->p_type == PT_TEXT)
					cnt.v_xsfrec++;
				return(1);
			}
		}
	}

	/*	Scan vfds and dbds looking for a run of
	 *	contiguous pages to load from disk
	 */

	i = pgindx + 1;
	count = 1;
	curblk = dbd->dbd_swpmp + 1;
	type = dbd->dbd_type;
	swpi = ((swpdbd_t *)dbd)->dbd_swptb;
	while ((count < maxprepage) && 
	       (i < (prp->p_off + prp->p_count))) {
		FINDENTRY(rp, i, &vfdp, (dbd_t **)&dbdp);

		if (vfdp->pgm.pg_v ||
		    dbdp->dbd_type != type ||
		    dbdp->dbd_swptb != swpi ||
		    dbdp->dbd_swpmp != curblk ||
		    hash_peek(swapdev_vp, ((dbd_t *)dbdp)->dbd_data))
			break;
		count++;
		i++;
		curblk++;
	}


	/*	
	 * Give back excess memory we're holding and fill
	 * in page tables with real pages.
	 */
	VASSERT(maxprepage >= count);
	memunreserve((unsigned int)maxprepage - count);
	vfdfill(pgindx, (unsigned) count, prp);

	/*
	 * We now have a series of vfds with page frames
	 * assigned, whose rightful contents are not in the
	 * page cache. Insert the page frames, and read from
	 * disk.
	 */
	swapdel = swaptab[swpi].st_flags & ST_INDEL;
	if (swapdel == 0) {
		for (i = 0, j = pgindx; i < count; i++, j++) {
			FINDENTRY(rp, j, &vfdp, (dbd_t **)&dbdp);
			pfd = addtocache((int)vfdp->pgm.pg_pfn, swapdev_vp, 
					 ((dbd_t *)dbdp)->dbd_data);
#ifdef PFDAT32
			if ((pfd - pfdat) != vfdp->pgm.pg_pfn) {
				VASSERT(!HDL_GETMODBIT((pfd - pfdat)));
				freepfd(&pfdat[vfdp->pgm.pg_pfn]);
				vfdp->pgm.pg_pfn = pfd - pfdat;
			}

			/*
			 * Save pfn.
			 */
			pfn[i] = pfd - pfdat;
#else
			if (pfd->pf_pfn != vfdp->pgm.pg_pfn) {
				VASSERT(!HDL_GETMODBIT(pfd->pf_pfn));
				freepfd(&pfdat[vfdp->pgm.pg_pfn]);
				vfdp->pgm.pg_pfn = pfd->pf_pfn;
			}

			/*
			 * Save pfn.
			 */
			pfn[i] = pfd->pf_pfn;
#endif
		}
	}
	else {
		for (i = 0, j = pgindx; i < count; i++, j++) {
			vfdp = FINDVFD(rp, j);
			/*
			 * Save pfn.
			 */
			pfn[i] = vfdp->pgm.pg_pfn;
		}
	}

	/* 
	 * Protect the pages from the user during
	 * the I/O operation.
	 */
	hdl_user_protect(prp, space, vaddr, count, &nspace, &nvaddr, 0);

	dbd = (swpdbd_t *)FINDDBD(rp, pgindx);

        /*
         * Set SKEEP to prevent the process from being swapped out
         * while waiting for I/O.
         */
        SPINLOCK_USAV(sched_lock, context);
	if (u.u_procp->p_flag & SKEEP) 
		saved_skeep_state = 1;
        u.u_procp->p_flag |= SKEEP;
        SPINUNLOCK_USAV(sched_lock, context);

        /*
         * Release the region lock while waiting for I/O.
         */
	curblk = ((dbd_t *)dbd)->dbd_data;
        regrele(rp);

#ifdef FSD_KI
	{
	struct buf *bp;
	bp = bswalloc();
	bp->b_bptype = B_devswap_pagein|B_pagebf;
	bp->b_rp = rp;
	asyncpageio(bp, (swblk_t)curblk, nspace, nvaddr, 
				(int)ptob(count), B_READ, swapdev_vp);
	error = waitforpageio(bp);
	bswfree(bp);
	}
#else  /* ! FSD_KI */
	error = syncpageio((swblk_t)curblk, nspace, nvaddr, 
				(int)ptob(count), B_READ, swapdev_vp);
#endif /* ! FSD_KI */

	/*
         * If syncpageio returns an error we cannot recover.
	 * This panic has been moved up out of waitpageio().
	 */
	if (error)
	    panic("devswap_pagein: syncpageio detected an error");

        /*
         * Clear the modification bit.  This must be done before
         * hdl_user_unprotect for MP.
         */
        for (i = 0; i < count; i++) {
                hdl_unsetbits(pfn[i], VPG_MOD);
        }

	/*
	 * Unprotect the pages.
	 */
	hdl_user_unprotect(nspace, nvaddr, count, 0);

	/*
	 * Release pfdat lock first before doing a region lock.
	 */
	for (i = 0; i < count; i++) {
		pfnunlock(pfn[i]);
	}

	reglock(rp);

	/*
	 * do not reset the SKEEP state if somebody above us has set it.
	 * eg: add_bss or fault_each().
	 */

	SPINLOCK_USAV(sched_lock, context);
	if (!saved_skeep_state)
        	u.u_procp->p_flag &= ~SKEEP;
	SPINUNLOCK_USAV(sched_lock, context);

        if (swapdel) {
                for (i = 0, j = pgindx; i < count; i++, j++) {
                        dbdp = (swpdbd_t *)FINDDBD(rp, j);
                        (void)swfree1(dbdp);
                        dbdp->dbd_type = DBD_NONE; ((dbd_t *)dbdp)->dbd_data = 0xfffff08;
                }
        }

	return(count);
}

/*ARGSUSED*/
devswap_dbddup(vp, dbd)
	struct vnode *vp;
	union idbd dbd;
{
	VASSERT(swpuse((swpdbd_t *)&dbd) != 0);
	swaplock();
	if (!swpinc((swpdbd_t *)&dbd, "dbddup")){
		/* 
		 * swap use count overflowed.
		 */
                panic("devswap_dbddup: swap use count overflowed");
	}
	swapunlock();
	return(dbd.idbd_int);
}

/*ARGSUSED*/
devswap_dbddealloc(vp, dbd) 
	struct vnode *vp;
	swpdbd_t *dbd;
{
/* 
 *	If decrementing the use count on the swap map entry for the dbd
 * 	leads to a zero count, this function would ideally atomically free 
 *	the swap map entry and remove the page from the page cache.
 *	This is necessary because code in devswap_vfdcheck() assumes that 
 *	if a dbd type is DBD_NONE while the page is in the page cache, then
 *	some other process sharing this physical page has already allocated 
 *	swap for it, which the process then shares by calling swpinc().
 *
 *	This function was originally written as
 *
 *	    if (swfree1(dbd) == 0)
 *		    removefromcache(swapdev_vp, ((dbd_t *)dbd)->dbd_data);
 *
 *	This is bad, because removefromcache() can sleep before actually
 *	removing the page.  If during this sleep another process sharing
 *	the page sees a page type of DBD_NONE with the page is still in the 
 *	page cache, that process will assume a shared swap block for the
 *	page.  In this case the other process will do a swpinc(), which
 *	will increment the use count on the dbd that was just swfree1()'d.
 *	This gives a postive use-count on a free swap map entry, which
 *	can then later be freed.  Freeing a free swap map entry leads to
 *	a circular free list, which eventually leads the kernel to 
 *	an infinite loop while allocated swap map entries (if debug is
 *	on, the kernel would panic before getting to the infinite loop).
 *
 *	The following method is still not guaranteed atomic, as there is a 
 *	VN_RELE in removefromcache() after the page is removed.  However, we
 *	are trying to free the page if we called removefromcache(), so it
 *	does not matter that this will cause another process to allocate
 *	a separate dbd to swap to for this same page.  Once we call swfree1()
 *	it will be just as if we'd finished before the other process looked.
 *
 *					jk
 */
	if (swfree_willfree(dbd))
		removefromcache(swapdev_vp, ((dbd_t *)dbd)->dbd_data);
	swfree1(dbd);
}

devswap_vfdcheck(rindex, vfd, dbd, args)
int rindex;
vfd_t *vfd;
swpdbd_t *dbd;
swargs_t *args;
{
    int migrate = args->migrate;
    int hard = (args->flags & PAGEOUT_HARD);
    int vhand = (args->flags & PAGEOUT_VHAND);
    preg_t *prp = args->prp;
    reg_t *rp = prp->p_reg;
    caddr_t vaddr = prp->p_vaddr + ptob(rindex - prp->p_off);
    pfd_t *pfd;
    u_int pn;
    swpdbd_t swdbd;

    /*
     * Don't examine this page if already in a run
     * and it is not part of the current run.
     */
    if (args->run && (args->end + 1) != rindex)
	return 1;

    /*
     * Don't exceed maximum amount that can be contiguously swapped.
     */
    if (args->run && args->end - args->start + 1 >= maxpageout)
	return 1;

    /*
     * Stop for vhand if enough memory is free, and we're not yet
     * in a run.  parolemem based stopping criteria is done outside
     * this function, because it is not worth interrupting a "run" for.
     */
    if (vhand && !args->run && freemem >= lotsfree)
	return 1;

    if (!args->run)
	args->end = rindex;

    /*
     * If page is locked, then if we're in a run terminate it,
     * otherwise just skip the page
     */
    if (vfd->pgm.pg_lock)
	return args->run;

    /*
     * Try and lock the page.  If the page is currently locked
     * and this is not a hard push, skip it and catch the
     * next time around.
     */
    pn = vfd->pgm.pg_pfn;
    if (!cpfnlock(pn)) {
	if (!hard)
	    return args->run;
	pfnlock(pn);
    }
#ifdef PFDAT32
    VASSERT(pfd_is_locked(&pfdat[pn]));
#else	
    VASSERT(vm_valusema(&pfdat[pn].pf_lock) <= 0);
#endif	

    if (vhand)
	++stealvisited;

    /* We have a page, see if we want to steal it.
     * We don't steal it if this isn't a force out
     * and it has been touched since the last aging.
     */
    if (!hard && (HDL_GETREFBIT(pn))) {
	pfnunlock(pn);
	return args->run;
    }

    if (args->flags & PAGEOUT_FREE) {
	/*
	 * Mark the page invalid, since it's on its way out now!
	 */
	vfd->pgm.pg_v = 0;
	rp->r_nvalid--;
    }

    /* We cannot handle a dbd type of DFILL or DZERO. */
    VASSERT((dbd->dbd_type != DBD_DFILL) && (dbd->dbd_type != DBD_DZERO));

    pfd = &pfdat[pn];

    /* See if this page must be written to swap. */
    switch (dbd->dbd_type) {
    case DBD_NONE:
	/*
	 * This may be a copy-on-write page which has been written
	 * to swap by another process which shares the page.  If
	 * so, just use the same swap block.  In the rare case
	 * that the swap use count overflows, we allocate
	 * another swap page.
	 */
	swaplock();
	if (PAGEINHASH(pfd)) {
	    *(int *)(&swdbd) = pfd->pf_data;
	    dbd->dbd_swptb = swdbd.dbd_swptb;
	    dbd->dbd_swpmp = swdbd.dbd_swpmp;
	    dbd->dbd_type = DBD_BSTORE;
	    if (!swpinc(dbd, "getpages")) {
		dbd->dbd_type = DBD_NONE; ((dbd_t *)dbd)->dbd_data = 0xfffff09;
		swapunlock();
		if (!args->run) {
		    args->run = 1;
		    args->start = rindex;
		}
		args->end = rindex;
		return 0;
	    }
	    swapunlock();
	    if (args->flags & PAGEOUT_FREE) {
		hdl_deletetrans(prp, prp->p_space, vaddr, (int)pn);
#ifdef FSD_KI
        	memfree(vfd, rp);
#else  /* ! FSD_KI */
		memfree(vfd);
#endif /* ! FSD_KI */
		if (vhand)
		    cnt.v_dfree++;
	    }
	    else
		pfnunlock(pn);
	    return args->run;
	}
	swapunlock();
	if (!args->run) {
	    args->run = 1;
	    args->start = rindex;
	}
	args->end = rindex;
	return 0;

    case DBD_BSTORE:
	/*
	 * See if this page has been modified since it was read
	 * in from swap.  If not, then just use the copy already on
	 * the swap file unless we are trying to delete the swap file.
	 * If we are, then release the current swap copy and write
	 * the page out to another swap file.
	 */
	if ((!HDL_GETMODBIT(pn)) &&
	    (swaptab[dbd->dbd_swptb].st_flags & ST_INDEL) == 0) {
	    if (!(args->flags & PAGEOUT_FREE)) {
		pfnunlock(pn);
		return args->run;
	    }

	    hdl_deletetrans(prp, prp->p_space, vaddr, (int) pn);
	    /*
	     * For MP we must check mod again after removing
	     * the translation
	     */
	    if (!HDL_GETMODBIT(pn)) {
#ifdef FSD_KI
        	memfree(vfd, rp);
#else  /* ! FSD_KI */
		memfree(vfd);
#endif /* ! FSD_KI */
		minfo.unmodsw++;
		if (vhand)
			cnt.v_dfree++;
		return args->run;
	    }
	}

	/*
	 * The page has been modified. Release the current swap
	 * block and add it to the list of pages to be swapped out
	 * later.
	 */
	if (PAGEINHASH(pfd))
	    pageremove(pfd);
	(void)swfree1(dbd);

	dbd->dbd_type = DBD_NONE; ((dbd_t *)dbd)->dbd_data = 0xfffff0a;
	if (!args->run) {
	    args->run = 1;
	    args->start = rindex;
	}
	args->end = rindex;
	return 0;

    case DBD_HOLE:
	/*
	 * We should never have a DBD_HOLE in a non-MMF region.
	 * When we duplicated the region, we changed all DBD_HOLEs
	 * back to DBD_FSTORE.
	 */
	panic("DBD_HOLE in non-MMF region");
	break;
	dbd->dbd_type = DBD_FSTORE;
	/* FALLS THROUGH */

    case DBD_FSTORE:
	/*
	 * See if the page has been modified.  If it has, then
	 * we cannot use the copy on the file and must assign
	 * swap for it.
	 *
	 * The code for handling a modified page was once
	 * inappropriately removed and replaced with a panic because
	 * it did not seem like a FSTORE page should ever be modified.
	 * However, there is no fundamental reason for this to be
	 * true.  For example, if exec'ing writes zereos at address 0
	 * to satisfy null dereference semantics, a front store page
	 * could might have its MOD bit set.  In the future,
	 * writable memory mapped files may well create lots of
	 * FSTORE pages with MOD bits set.
	 *
	 * 8.07 got panics here for the write zereos at address
	 * zero reason.  This could be avoided by at that time also
	 * changing dbd_type to DBD_NONE.  However, panics are meant
	 * for fundamentally inconsistent things, which modified
	 * FSTORE pages are not.
	 *
	 * 				jk
	 */
	if (!migrate && !HDL_GETMODBIT(pn)) {
	    if (!(args->flags & PAGEOUT_FREE)) {
		pfnunlock(pn);
		return args->run;
	    }
	    hdl_deletetrans(prp, prp->p_space, vaddr, pn);
	    /*
	     * For MP we must check mod again after removing
	     * the translation
	     */
	    if (!HDL_GETMODBIT(pn)) {
		/*
		 * Page has not been modified.  Just
		 * point to the copy on the file.
		 */
#ifdef FSD_KI
        	memfree(vfd, rp);
#else  /* ! FSD_KI */
		memfree(vfd);
#endif /* ! FSD_KI */
		minfo.unmodfl++;
		if (vhand)
		    cnt.v_dfree++;
		return args->run;
	    }
	}

	/*
	 * We will be writing this page out
	 */
	if (PAGEINHASH(pfd))
	    pageremove(pfd);
	dbd->dbd_type = DBD_NONE; ((dbd_t *)dbd)->dbd_data = 0xfffff0b;
	if (!args->run) {
	    args->run = 1;
	    args->start = rindex;
	}
	args->end = rindex;
	return 0;
    }
    return args->run;
}

/*
 * Swap out pages from region rp which is locked by
 * our caller.  If hard is set, take all valid pages,
 * othersize take only unreferenced pages
 */
devswap_pageout(prp, start, end, flags)
	preg_t *prp;
	size_t start;
	size_t end;
	int flags;
{
	int migrate;
	extern void foreach_valid();
	reg_t *rp = prp->p_reg;
	int steal = (flags & PAGEOUT_FREE);
	int swap  = (flags & PAGEOUT_SWAP);
	int npgs;
#ifdef FSD_KI
	/* Identify the reason (caller) for this pageout operation */
	int bptype = (flags & PAGEOUT_VHAND) ? (B_vhand|B_pagebf) :
		((flags & PAGEOUT_SWAP) ? (B_swapout|B_swpbf) :
		(B_devswap_pageout|B_pagebf));
#endif /* FSD_KI */
	
	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	if (start > end)
	    return;
	
	if (rp->r_flags & RF_NOFREE)
		migrate = 1;
	else
		migrate = 0;

	/*	If the region is marked "don't swap", then don't
	 *	steal any pages from it.
	 */

	if (rp->r_mlockcnt != 0)
		return;

	/*	
	 * Look through the pages of the region that this pregion examines.
	 */
	while (start <= end) {
		swargs_t callargs;
		callargs.prp = prp;
		callargs.flags = flags;
		callargs.migrate = migrate;

	       /* If we get into devswap_vfdcheck with a good page, end will be 
  		* reset so we know how far we got, otherwise there are no pages 
		* we can page, so we've visited all of the ones we can page, 
		* so initiallize end to represent this.
		*/
		callargs.end = prp->p_off + end;
		callargs.run = 0;

		foreach_valid(prp->p_reg, (int)prp->p_off + start, 
			      (int)end - start + 1, devswap_vfdcheck, 
							(caddr_t)&callargs);

		start = callargs.end - prp->p_off + 1;
		if (callargs.run == 0)
			break;

		if (swap)
			syswait.swap++;

#ifdef FSD_KI
		swapchunk(prp, callargs.start, callargs.end + 1, steal, bptype);
#else  /* ! FSD_KI */
		swapchunk(prp, callargs.start, callargs.end + 1, steal);
#endif /* ! FSD_KI */

		npgs = callargs.end - callargs.start + 1;
		if (steal) {
			if (swap) {
				syswait.swap--;
				cnt.v_pswpout += (npgs);
				sar_bswapout += ptod(npgs);
			}
			else if (flags & PAGEOUT_VHAND) {
				cnt.v_pgout++;
				cnt.v_pgpgout += (npgs);
			}
		}
		if ((flags & PAGEOUT_VHAND) && parolemem >= maxpendpageouts)
			break;
	}
	prp->p_stealscan = start;
}

/*	Swap out a contiguous chunk of user pages.
 *
 *		Swap out from page low to high-1.
 */
#ifdef FSD_KI
swapchunk(prp, low, high, steal, bptype)
#else  /* ! FSD_KI */
swapchunk(prp, low, high, steal)
#endif /* ! FSD_KI */
	preg_t	*prp;	/* Ptr to region being swapped. */
	int	low;
	int	high;
#ifdef FSD_KI
	int 		bptype;
#endif /* FSD_KI */
{
	register u_int context;
	int tpages;		/* total pages to swap */
	int npages;		/* pages to swap this iteration */
	vfd_t *vfd;
	dbd_t *dbd;
	reg_t *rp = prp->p_reg;
	struct buf *bp;
	int indx;
	pfd_t *pfd;
	pfd_t *newpfd;
	space_t nspace;
	caddr_t nvaddr;
	extern unsigned int sleep_lock();

	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	if (low < 0)
		return;

	tpages = high - low;
	while (tpages > 0) {

		/* Allocate swap space */

		if (swalloc(rp, low, tpages)) {
			/* got all the space in one contiguous chunk */
			npages = tpages;
		} else if (swalloc(rp, low, 1)) {
			npages = 1;	/* at least got one swap page */
		} else
			panic("swapchunk: swapspace exhausted");

		for (indx = low; indx < low + npages; indx++) {
			FINDENTRY(rp, indx, &vfd, &dbd);
			pfd = &pfdat[vfd->pgm.pg_pfn];
			if (!PAGEINHASH(pfd)) {
				newpfd = addtocache((int)vfd->pgm.pg_pfn,
					swapdev_vp, dbd->dbd_data);

				VASSERT(newpfd == pfd);
			}
			VASSERT(pfd->pf_devvp == swapdev_vp &&
			        pfd->pf_data == dbd->dbd_data);

			/*
			 * Disown the page so if we come to swap it
			 * out again later, we won't still own the
			 * lock.
			 */
#ifdef PFDAT32
			pfdatdisown(pfd);
#else	
			vm_disownsema(&pfd->pf_lock);
#endif	
		}
		/*
		 * record how many disc I/Os are pending for this region.
       		 */

       	 	context = sleep_lock();
       	 	rp->r_poip++;
       		sleep_unlock(context);

		bp = bswalloc();
#ifdef FSD_KI
		/*
	 	 * Set the b_apid/b_upid fields to the pid (this process' pid)
	 	 * that last allocated/used this buffer.
	 	 */
		bp->b_apid = bp->b_upid = u.u_procp->p_pid;	

 		/* Identify this buffer for KI */
		bp->b_bptype = bptype;

 		/* Save site(cnode) that last used this buffer */
		bp->b_site = u.u_procp->p_faddr; 
#endif /* FSD_KI */

		bp->b_iodone = pageiodone;
		if (steal)
			bp->b_flags = B_CALL|B_BUSY|B_PAGEOUT;
		else
			bp->b_flags = B_CALL|B_BUSY;
		bp->b_rp = rp;
#ifdef LATER
		syswait.swap++;
#endif LATER
		dbd = FINDDBD(rp, low);
		hdl_user_protect(prp, prp->p_space,
				 prp->p_vaddr + ptob(low - prp->p_off),
				 npages, &nspace, &nvaddr, steal);
		asyncpageio(bp, dbd->dbd_data, nspace, nvaddr,
			   (int)ptob(npages), B_WRITE, swapdev_vp);

		low    += npages;	/* advance page number */
		tpages -= npages;	/* decrement unswapped page count */
	}
}
