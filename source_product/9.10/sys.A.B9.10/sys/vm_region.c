/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_region.c,v $
 * $Revision: 1.16.83.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/03/03 12:28:36 $
 */


#include "../h/types.h"
#include "../h/param.h"
#include "../h/vm.h"
#include "../h/vfd.h"
#include "../h/uio.h"
#include "../h/var.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/sema.h"
#include "../h/proc.h"
#include "../h/errno.h"
#include "../h/debug.h"
#include "../h/vas.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pfdat.h"
#include "../h/swap.h"
#include "../h/malloc.h"
#ifdef FSD_KI
#include "../h/ki_calls.h"
#include "../h/ki.h"
#endif	/* FSD_KI */

vm_lock_t rlistlock;		/* region list lock */
int activeregions;		/* Count of active regions */
reg_t regactive;		/* list of active regions */

void
reginit()
{
	activeregions = 0;

	regactive.r_forw = &regactive;
	regactive.r_back = &regactive;

	vm_initlock(rlistlock, RLISTLOCK_ORDER, "rlist lock");
}


/*
 * Allocate a new region.
 * Returns a locked region pointer or NULL on failure
 * The region is linked into the active list. (if it isn't private)
 * The region is given a virtual address, but no pages are assigned
 * to the region.  The virtual address is rounded down to the nearest
 * page boundry.
 */

reg_t *
allocreg(fstore, bstore, regtype)
   register struct vnode *fstore;
   register struct vnode *bstore;
   int regtype;
{
	register reg_t *rp;
	vm_sema_state;		/* semaphore save state */
	register u_int context;
	extern void hdl_allocreg();

	/*
	 * Allocate a region for this frontstore.
	 */
	MALLOC(rp, reg_t *, sizeof(reg_t), M_REG, M_WAITOK);
	bzero(rp, sizeof(reg_t));

	vmemp_lockx();		/* lock down VM empire */
	/*	Initialize region fields and bump inode reference
	 *	count.
	 */
	rp->r_type = regtype;
	rp->r_flags = RF_ALLOC;
	rp->r_fstore = fstore;
	rp->r_bstore = bstore;
	rp->r_key = UNUSED_IDX;
	rp->r_next = rp->r_prev = rp;

	vm_initsema(&rp->r_lock, 1, REG_R_LOCK_ORDER, "reg sema");
	reglock(rp);			/* also increments p_reglocks */

	vm_initsema(&rp->r_mlock, 1, REG_R_MLOCK_ORDER, "reg mlock sema");

	/* Let HDL fill in its portions */
	hdl_allocreg(rp);

	if (fstore) {
		/*
		 * Bump the reference count on the vnode.
		 */
		VN_HOLD(fstore);
	}
	if (bstore) {
		/*
		 * Bump the reference count on the vnode.
		 */
		VN_HOLD(bstore);
	}

	/*
	 * Add to list of active regions
	 *
	 * WARNING: once the region is added to the active list,
	 * r_fstore CANNOT BE MODIFIED because xinval() relies
	 * on r_fstore not changing.
	 */
	rlstlock(context);
	activeregions++;
	rp->r_forw = &regactive;
	rp->r_back = regactive.r_back;
	regactive.r_back->r_forw = rp;
	regactive.r_back = rp;
	rlstunlock(context);

#ifdef	FSD_KI
	KI_region(rp, KI_ALLOCREG);
#endif	/* FSD_KI */
	vmemp_unlockx();	/* free up VM empire */
	return(rp);
}

/*
 * Free an unused region table entry.
 */
void
freereg(rp)
	register reg_t *rp;
{
	register u_int context;
	extern void shrinkvfd();
	extern void hdl_freereg();

	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(rp->r_flags & RF_ALLOC);
	VASSERT(rp->r_mlockcnt <= rp->r_refcnt);
	VASSERT(vm_valusema(&rp->r_mlock) == 1);

	/*
	 * If the region is still in use, then don't free it.
	 */
	if (rp->r_refcnt != 0) {
		regrele(rp);		/* also decrements p_reglocks */
		return;
	}

	/*
	 * If the region is hashed, unhash it.
	 */
	if (rp->r_flags&RF_HASHED)
		text_unhash(rp);

	if (rp->r_mlockcnt != 0)
		panic("freereg: r_mlockcnt not zero");

	/*
	 * Free up any VFD/DBD pages allocated previously.
	 */
	if (rp->r_pgsz) {
		pgfree(rp, 0, rp->r_pgsz);
		VASSERT(rp->r_nvalid == 0);

		if (rp->r_bstore == swapdev_vp)
			release_swap((int)rp->r_swalloc);
		shrinkvfd(rp, (int)rp->r_pgsz);
		VASSERT(rp->r_pgsz == 0);
	}
	VASSERT(rp->r_nvalid == 0);

	/*
	 * If r_chunk has not been freed, free it.
	 */
	if (rp->r_chunk)
		FREE(rp->r_chunk, M_VFD);

	/*
	 * Decrement use count on associated vnode.
	 */
	if (rp->r_fstore) {
		VN_RELE(rp->r_fstore);
	}

	/*
	 * Decrement use count on associated vnode.
	 */
	if (rp->r_bstore) {
		VN_RELE(rp->r_bstore);
	}

	/*
	 * Free up the region, removing it from the active list
	 * if necessary.
	 *
	 * Remove this region from the list of related regions (this is
	 * a circular list).
	 */
	rlstlock(context);
	activeregions--;

	rp->r_back->r_forw = rp->r_forw;
	rp->r_forw->r_back = rp->r_back;

	rp->r_prev->r_next = rp->r_next;
	rp->r_next->r_prev = rp->r_prev;
	rlstunlock(context);

	/*
	 * Let HDL have a crack at it
	 */
	hdl_freereg(rp);

#ifdef	FSD_KI
	KI_region(rp, KI_FREEREG);
#endif	/* FSD_KI */

	/*
	 * Clean up the region entry.
	 */
	regrele(rp);		/* also decrements p_reglocks */
	vm_termsema(&rp->r_lock);

	/*
	 * Return to system free pool
	 */
	FREE(rp, M_REG);
}

/* Arguments for the sparse walk doing a dupreg() */
struct dupargs {
	int off;
	int avoidcw;
	reg_t *newrp, *oldrp;
	int dbdcnt;
};

/*
 * do_dupc()--local function to duplicate entries under a region
 *
 * Setting copy-on-write (cow) on pages in a region requires some careful
 * consideration.  Besides the need to ensure that the page is shared between
 * two processes without being writable, the system must ensure that the disk
 * blocks on which the page might be written are also shared.  Because certain
 * VOP_PAGEOUT routines reassign the backing for a particular page, the system
 * must ensure that both the child and the parent will always obtain the
 * correct version of the page.  To accomplish this task, all modified pages
 * are unhashed and the child's and parent's dbds are set to DBD_NONE.  This
 * causes the page to be rewritten and hashed by either the child or parent
 * process (which ever of the regions VOP_PAGEOUT gets to first) and to have
 * the other process find the page in the page cache.
 */
int
do_dupc(rp, idx, vd, count, dupargs)
	reg_t *rp;
	int idx;
	struct vfddbd *vd;
	int count;
	struct dupargs *dupargs;
{
	reg_t *rp2 = dupargs->newrp;
	int skipdup = 0;
	int mmf;	/* is source region a shared MMF? */

	/*
	 * The following performance hack avoid the dereference
	 * of the vnode if DBDDUP is NEVER going to do anything.
	 */
	mmf = (rp->r_fstore == rp->r_bstore);
	if (mmf && rp->r_fstore->v_op->vn_dbddup == NULL) {
		VASSERT(rp->r_fstore->v_op->vn_dbddealloc == NULL);
		skipdup = 1;
	}

	for (; count--; idx++, vd++) {
		vfd_t *pvfd = &vd->c_vfd;
		dbd_t *pdbd = &vd->c_dbd;
		vfd_t *cvfd;
		dbd_t *cdbd, map;
		int setcw = 1;

		/*
		 * Stop at end of either region.
		 */
		if ((idx >= rp->r_pgsz) ||
		    ((idx - dupargs->off) >= rp2->r_pgsz)) {
			return(1);
		}

		/*
		 * Look up both the vfd and dbd for the child
		 * right up front
		 */
		FINDENTRY(rp2, idx - dupargs->off, &cvfd, &cdbd);

		/*
		 * If the page frame is valid, and it has already
		 * been copied (or has never needed copying) set
		 * the permission to read only to get the copy.
		 *
		 * If the source region is a memory mapped file, we
		 * leave it alone.  We do not mark the page to be
		 * copy-on-write, nor do we remove it from the page
		 * cache, as even modified pages in an MMF reflect
		 * an accurate copy of what is (or eventually will
		 * be) on disk.
		 */
		if (pvfd->pgm.pg_v) {
			struct pfdat *pfd;
			int	pn;

			rp2->r_nvalid++;
			pn = pvfd->pgm.pg_pfn;
			pfd = &pfdat[pn];
			pfdatlock(pfd);
			VASSERT(pfd->pf_use > 0);
			pfd->pf_use++;
			if (!mmf && !(pvfd->pgm.pg_cw)) {
				setcw = hdl_cw((u_int)pn);
			}
			if (!mmf && HDL_GETMODBIT(pn)) {
				/*
				 * A modified page has been found.
				 * Unhash the page and set the
				 * dbds to DBD_NONE (see procedure
				 * comment block).
				 */
				if (PAGEINHASH(pfd)) {
					pageremove(pfd);
				}
				if (!skipdup) {
					switch (pdbd->dbd_type) {
					case DBD_FSTORE:
						VOP_DBDDEALLOC(rp->r_fstore,
							pdbd);
						break;
					case DBD_BSTORE:
						VOP_DBDDEALLOC(rp->r_bstore,
							pdbd);
						break;
					}
				}
				pdbd->dbd_type = DBD_NONE; pdbd->dbd_data = 0xfffff10;
			}
			pfdatunlock(pfd);
		}
		if (setcw && !mmf) {
			pvfd->pgm.pg_cw = 1;
			*cvfd = *pvfd;
		} else {
			*cvfd = *pvfd;
			cvfd->pgm.pg_cw = 1;
		}

		/*
		 * If no one cares that we duplicate
		 * this information just skip the calls.
		 */
		if (skipdup) {
			*cdbd = *pdbd;
			/*
			 * Since by definition the new region is not a
			 * shared memory mapped file, change any
			 * DBD_HOLE dbds back to DBD_FSTORE.
			 */
			if (cdbd->dbd_type == DBD_HOLE)
				cdbd->dbd_type = DBD_FSTORE;
			continue;
		}

		/*
		 * Inform the vnode layer that the dbd is being
		 * duplicated.
		 */
		map = *pdbd;
		switch (map.dbd_type) {
		case DBD_FSTORE:
			if (rp->r_fstore->v_op->vn_dbddup) {
				union idbd xdbd;
				xdbd.idbd_int = VOP_DBDDUP(rp->r_fstore, map);
				*cdbd = xdbd.idbd;
			} else {
				*cdbd = map;
			}
			break;
		case DBD_BSTORE:
			if (rp->r_bstore->v_op->vn_dbddup) {
				union idbd xdbd;
				xdbd.idbd_int = VOP_DBDDUP(rp->r_bstore, map);
				*cdbd = xdbd.idbd;
				dupargs->dbdcnt++;
			} else {
				*cdbd = map;
			}
			break;
		default:
			*cdbd = map;
			/*
			 * Since by definition the new region is not a
			 * shared memory mapped file, change any
			 * DBD_HOLE dbds back to DBD_FSTORE.
			 */
			if (cdbd->dbd_type == DBD_HOLE)
				cdbd->dbd_type = DBD_FSTORE;
			break;
		}
	}
	return(0);
}

/*
 * Duplicate a region.  We always use swapdev_vp as the back store
 * for the new region.
 */
reg_t *
dupreg(rp, regtype, off, count)
	register reg_t *rp;
	int regtype;
	size_t off, count;
{
	reg_t *rp2;
	struct dupargs dupargs;
	extern void foreach_chunk();
	int lazy = (rp->r_flags & RF_SWLAZY);
	size_t swap_to_reserve;
	register u_int context;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

#ifdef OSDEBUG
	/*
	 * Because of the way we compute the dbd_cnt in do_dup, lazy can
	 * only be used with back store equal to swap.
	 */
	if (lazy)
		VASSERT(rp->r_bstore == swapdev_vp);
#endif

	/*
	 * If region is shared, there is no work to do.
	 * Just return the passed region.  The region reference
	 * counts are incremented by attachreg.
	 */
	if ((rp->r_type == RT_SHARED) && (regtype == RT_SHARED))
		return(rp);

	/*
	 * If the count is greater than the shared region
	 * we return 0.  We have seen this when a NFS vnode is overwritten
	 * while it is running.  The execve() will get information out of
	 * the (new) a.out header which is incompatible with the mapped
	 * vnode.  Note that the r_zomb bit may be set in the region.
	 */
	if ((off + count) > rp->r_pgsz) {
		u.u_error = EFAULT;
		return(NULL);
	}

	/*
	 * We always reserve the maximum possibly needed, unless
	 * doing lazy swap. In that case, we peek inside the
	 * source region and see how many swap pages it has allocated.
	 */
	swap_to_reserve = lazy ? rp->r_swalloc : count;
	if (!reserve_swap(rp, swap_to_reserve, SWAP_NOWAIT)) {
		u.u_error = ENOMEM;
		return(NULL);
	}

	/*
	 * Need to copy the region.
	 * Allocate a region descriptor and grow its VFD
	 * list large enough.
	 */
	if ((rp2 = allocreg(rp->r_fstore, swapdev_vp, regtype)) == NULL) {
		release_swap((int)swap_to_reserve);
		return(NULL);
	}

	if (rp->r_zomb)
		rp2->r_zomb = 1;

	/*
	 * Duplicate the vfd's.  The dbd prototype concept requires that
	 * the method of duplication be hidden.
	 */
	if (dupvfds(rp, rp2, off, count)) {
		release_swap((int)swap_to_reserve);
		freereg(rp2);
		return(NULL);
	}

	/*
	 * Set r_swalloc to the amount of swap space we have reserved.
	 * r_swalloc always indicates the amount of swap that we have
	 * reserved for a region.
	 */
	rp2->r_swalloc = swap_to_reserve;

	/*
	 * Compute this regions view of the vnode.
	 */
	rp2->r_off = rp->r_off + off;

	/*
	 * If the original region is an unaligned view of the vnode
	 * make the new region one also (support for old a.out).
	 */
	if (rp->r_flags & RF_UNALIGNED) {
		rp2->r_flags |= RF_UNALIGNED;
		rp2->r_byte = rp->r_byte;
		rp2->r_bytelen = rp->r_bytelen;
	}

	/*
	 * Set copy-on-write for the ranges we have privately
	 * duplicated.  We do not set any of the parent region
	 * pages to be copy-on-write if it is a shared memory
	 * mapped file.
	 */
	if (rp->r_fstore != rp->r_bstore)
		vfd_setcw(rp, (int)off, (int)count);
	vfd_setcw(rp2, 0, (int)count);

	/* Scan the parents page table list and fix up each page table.
	 * Allocate a page table and map table for the child and
	 * copy it from the parent.
	 */
	dupargs.off = off;
	dupargs.oldrp = rp;
	dupargs.newrp = rp2;
	dupargs.dbdcnt = 0;
	foreach_chunk(rp, (int) off, (int) count, do_dupc, (caddr_t) &dupargs);

	/*
	 * If lazy swapping, mark the region as such.
	 */
	if (lazy)
		rp2->r_flags |= RF_SWLAZY;

	/*
	 * Add this new region to the same list of regions that the
	 * source region is in (a circular list).
	 */
	rlstlock(context);
	rp2->r_prev = rp;
	rp2->r_next = rp->r_next;
	rp->r_next->r_prev = rp2;
	rp->r_next = rp2;
	rlstunlock(context);

	VASSERT(rp2->r_nvalid <= rp2->r_pgsz);
#ifdef	FSD_KI
	KI_region(rp2, KI_DUPREG);
#endif	/* FSD_KI */
	return(rp2);
}


/*
 * Change the size of a region
 *  change == 0  -> no-op
 *  change  < 0  -> shrink
 *  change  > 0  -> expand
 *
 * Returns 0 on no-op, -1 on failure, and 1 on success.
 */
int
growreg(rp, change, type)
	reg_t *rp;
	int change;
	int type;
{
	size_t oldpgindx;
	extern void shrinkvfd();
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	VASSERT(rp != NULL);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(change >= 0 || (-change <= rp->r_pgsz));

	if (change == 0)
		vmemp_returnx(0);

	oldpgindx = rp->r_pgsz;
	if (change < 0) {
		/* Make the change a positive number */
		change = -change;

		/*	The region is being shrunk.  Compute the new
		 *	size and free up the unneeded space.
		 *	If region is memory locked, release lockable memory.
		 */
		pgfree(rp, (int)oldpgindx - change, (unsigned int)change);

		if (rp->r_bstore == swapdev_vp) {
			VASSERT((rp->r_flags & RF_SWLAZY) == 0);
			release_swap(change);
			rp->r_swalloc -= change;
		}
		shrinkvfd(rp, change);

		if (rp->r_mlockcnt != 0) {	/* region is memory locked */
			lockmemunreserve((unsigned int)change);
		}
	} else {
		int alloc_swap = !(rp->r_flags & RF_SWLAZY) &&
				 rp->r_bstore == swapdev_vp;

		/*	Initialize the new page tables and allocate
		 *	pages if required.
		 */
		if (alloc_swap) {
			if (!reserve_swap(rp, change, SWAP_NOWAIT)) {
				u.u_error = ENOMEM;
				vmemp_returnx(-1);
			}
		}

		if (growvfd(rp, change, type)){
			if (alloc_swap)
				release_swap(change);

			u.u_error = ENOMEM;
			vmemp_returnx(-1);
		}

		if (rp->r_mlockcnt != 0) {	/* region is memory locked */
			if (lockmemreserve((unsigned int)change) < 0) {
				if (alloc_swap)
					release_swap(change);
				shrinkvfd(rp, change);
				u.u_error = ENOMEM;
				vmemp_returnx(-1);
			}
		}

		/*
		 * If we reserved swap, update r_swalloc.  We do this
		 * here to minimize the backout code that we would need
		 * above if we were to have done it earlier.
		 */
		if (alloc_swap)
			rp->r_swalloc += change;

		/*
		 * Count the number of xfod pages allocated.
		 */
		if (type == DBD_DZERO)
			cnt.v_nzfod += change;
	}
#ifdef	FSD_KI
	KI_region(rp, KI_GROWREG);
#endif	/* FSD_KI */
	vmemp_unlockx();	/* free up VM empire */
	return(1);
}
