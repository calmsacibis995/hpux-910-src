/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_fault.c,v $
 * $Revision: 1.16.83.5 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/07/05 10:00:43 $
 */
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vm.h"
#include "../machine/vmparam.h"
#include "../machine/reg.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/var.h"
#include "../h/sema.h"
#include "../h/vfd.h"
#include "../h/vas.h"
#include "../h/buf.h"
#include "../h/utsname.h"
#include "../h/pfdat.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/swap.h"
#include "../h/tuneable.h"
#include "../h/debug.h"
#include "../h/sysinfo.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/uio.h"
#include "../h/pregion.h"
#include "../h/ki_calls.h"

/*
 * Protection fault handler (this is the external interface for the rest of
 * the system).
 *	wrt   -> type of access: 0=READ, 1=WRITE
 *	space	-> virtual space of fault
 *	vaddr	-> virtual address of fault
 */
pfault(vas, wrt, space, vaddr)
	vas_t *vas;
	register int wrt;
	register unsigned int space;
	register caddr_t vaddr;
{
	int error;
	preg_t *prp;
	vm_sema_state;		/* semaphore save state */

	vaddr = (caddr_t) ((unsigned) vaddr & ~(NBPG-1));

	/*
	 * Find the region containing the page that faulted.
	 */
	vmemp_lockx();
	prp = (preg_t *)findprp(vas, space, vaddr);

	/*
	 * If prp is NULL, the address is not in our address space.
	 */
	if (prp == (preg_t *)NULL) {
		vmemp_unlockx();
		return(SIGSEGV);
	}

	/*
	 * If this pregion has been mprotected, see what the
	 * protections for the page really are.
	 */
	if (IS_MPROTECTED(prp)) {
		u_long mprot = hdl_page_mprot(prp, vaddr);

		switch (mprot) {
		case MPROT_UNMAPPED:
			regrele(prp->p_reg);
			vmemp_unlockx();
			return(SIGSEGV);

		case MPROT_NONE:
			regrele(prp->p_reg);
			vmemp_unlockx();
			return(SIGBUS);
		
		case MPROT_RO:
			if (wrt) {
				regrele(prp->p_reg);
				vmemp_unlockx();
				return(SIGBUS);
			}
			break;
		}
	}
	else if (wrt && (prp->p_prot & PROT_WRITE) == 0) {
		regrele(prp->p_reg);
		vmemp_unlockx();
		return(SIGBUS);
	}

	if ((error = hdl_pfault(prp, wrt, space, vaddr)) > 0)
		if (prot_fault(prp, wrt, space, vaddr) == 0)
			error = 0;
	if (prp)
	    regrele(prp->p_reg);
	if (!error)
	{
		KI_pfault(wrt, space, vaddr, prp);
	}
	vmemp_unlockx();
	if (error < 0)
		error = -error;
	return(error);
}

/*
 * Translation fault handler (this is the external interface for the rest of
 * the system).
 *	vas   -> pointer to virtual address space
 *	wrt   -> type of access: 0=READ, 1=WRITE
 *	space -> virtual space of fault
 *	vaddr -> virtual address of fault
 */
vfault(vas, wrt, space, vaddr)
	register vas_t *vas;
	register int wrt;
	register space_t space;
	register caddr_t vaddr;
{
	int error, ret;
	preg_t *prp;
	vm_sema_state;		/* semaphore save state */

	cnt.v_faults++;

	vaddr = (caddr_t) ((unsigned) vaddr & ~(NBPG-1));

	/*
	 * Find the region containing the page that faulted.
	 */
	vmemp_lockx();
	prp = (preg_t *)findprp(vas, space, vaddr);

	/*
	 * prp could be NULL if we would like to grow our stack.
	 */
	if (prp != (preg_t *)NULL) {
		if (IS_MPROTECTED(prp)) {
			u_long mprot = hdl_page_mprot(prp, vaddr);

			switch (mprot) {
			case MPROT_UNMAPPED:
				regrele(prp->p_reg);
				vmemp_unlockx();
				return(SIGSEGV);

			case MPROT_NONE:
				regrele(prp->p_reg);
				vmemp_unlockx();
				return(SIGBUS);
			
			case MPROT_RO:
				if (wrt) {
					regrele(prp->p_reg);
					vmemp_unlockx();
					return(SIGBUS);
				}
				break;
			}
		}
		else if (wrt && (prp->p_prot & PROT_WRITE) == 0) {
			regrele(prp->p_reg);
			vmemp_unlockx();
			return(SIGBUS);
		}
	}

	if ((error = hdl_vfault(prp, wrt, space, vaddr)) > 0)
		if ((ret = virtual_fault(prp, wrt, space, vaddr)) <= 0)
			error = ret;
	if (prp)
	    regrele(prp->p_reg);
	vmemp_unlockx();
	if (error < 0)
		error = -error;
	return(error);
}

/*
 * If appropriate, try to allocate more swap space for this region
 */
int
sw_lazy(rp, dbd)
	reg_t *rp;
	dbd_t *dbd;
{
	int memreserved = 0;

	VASSERT(rp->r_flags & RF_SWLAZY);
	VASSERT(rp->r_bstore == swapdev_vp);
	VASSERT(rp->r_type == RT_PRIVATE);

	/* If it's coming back from backstore, it's already allocated */
	if (dbd->dbd_type == DBD_BSTORE)
		return(0);

	/* If the region is memory locked, call lockmemreserve
	 * to reserve lockable memory for the page.  However,
	 * if the region is currently in the process of being
	 * locked in memory, don't call lockmemreserve.
	 * Mlockpreg would have reserved the lockable memory.
	 */
	if (rp->r_mlockcnt != 0 && !(rp->r_flags & RF_MLOCKING)) {
		if (lockmemreserve(1) < 0)
			return(1);
		else
			memreserved = 1;
	}

	/* Otherwise, try to get another page of swap */
	if (!reserve_swap(rp, 1, SWAP_NOWAIT)) {
        	if (memreserved) {
			lockmemunreserve(1);
		}
		return(1);
	}
	rp->r_swalloc += 1;
	return(0);
}

#define INC_FAULTING				\
    {						\
	SPINLOCK(sched_lock);			\
	u.u_procp->p_flag |= SFAULTING;		\
	++num_fault_procs;			\
	SPINUNLOCK(sched_lock);			\
    }

#define DEC_FAULTING				\
    {						\
	SPINLOCK(sched_lock);			\
	u.u_procp->p_flag &= ~SFAULTING;	\
	--num_fault_procs;			\
	SPINUNLOCK(sched_lock);			\
    }

/*
 * Main virtual fault handler.
 *	vas	-> pointer to vritual address space
 *	wrt	-> type of access: 0=READ, 1=WRITE
 *	space	-> virtual space
 *	vaddr	-> virtual address
 *
 * returns either the signal to send (failure) or 0 (success).
 */
int
virtual_fault(prp, wrt, space, vaddr)
	preg_t *prp;
	register int wrt;
	unsigned int space;
	caddr_t vaddr;
{
	reg_t *rp;
	int count, i, j;
	int pgindx;	/* index of page faulted on */
	int startindex;	/* index of page where the fault really started */
	vfd_t *vfd;
	dbd_t *dbd;
	int restart;
	int sleepmem;
	extern int minsleepmem, memory_sleepers;
	extern int num_fault_procs;
	extern int xkillcnt;

	/*
	 * note that returning SIGSEGV here is no different from
	 * returning any positive value; hdl_vfault is the
	 * one who actually has generated the default SIGSEGV
	 * error
	 */
	if (prp == NULL)
		return(SIGSEGV);
	rp = prp->p_reg;

	/* 
	 * If swapper told us to quit faulting, deactivate ourselves
	 * by going to sleep.
	 */

	SPINLOCK(sched_lock);
	while (u.u_procp->p_flag & SSTOPFAULTING) {
		if (u.u_procp->p_flag & (SKEEP|SWEXIT|SPHYSIO|SRTPROC))
			u.u_procp->p_flag &= ~SSTOPFAULTING;
		else {
			regrele(rp);
			deactivate_proc(u.u_procp);
			SPINUNLOCK(sched_lock);
			sleep(&u.u_procp->p_upreg->p_lastfault, PSWP+3);
			reglock(rp);
			SPINLOCK(sched_lock);
		}
	}
	SPINUNLOCK(sched_lock);

	/*
	 * Find vfd and dbd as mapped by region
	 */
	pgindx = regindx(prp, vaddr);
	startindex = pgindx;

	FINDENTRY(rp, pgindx, &vfd, &dbd);

	/* 
	 * If we deactivated ourselves, the page might have become valid
	 * while we slept.  If so, there is nothing to do, as the page
	 * we were faulting on is now there for us.
	 */
	if (vfd->pgm.pg_v)
		return(0);

	/* If lazy swap allocation applies, handle it up front */
	if (rp->r_flags & RF_SWLAZY) {
		if (sw_lazy(rp, dbd))
			return(1);
	}

	/*	See what state the page is in.
	 */

	INC_FAULTING;

	switch (dbd->dbd_type) {
        case DBD_FSTORE:
	case DBD_HOLE:
		/*
		 * We only check r_zomb for FSTORE because
		 * the purpose of this check for now is to avoid
		 * entering the VOP_PAGEIN routines in case
		 * it is invalid to do so (e.g. cleaned up
		 * text vnode as a result of an LMFS crash).
		 * For other types of pages (e.g. BSTORE)
		 * we may still legitimately bring in the page.
		 * Doing this minimizes the number of cases for
		 * which we can encounter an r_zomb failure --
		 * in particular we cannot currently handle
		 * a kernel induced fault on such a region
		 * very gracefully.  See DTS #DSDe404153.
		 *
		 * This assumption will have to change if the
		 * BSTORE pagein routines (for now just devswap_pagein)
		 * decide to use the r_zomb mechanism, and for
		 * coherent MMF's because then we may wish to extend
		 * r_zomb to affect decisions on BSTORE faults/actions.
		 *
		 * XXX MMF_REVISIT XXX
		 * XXX HP_REVISIT XXX
		 */
		if (rp->r_zomb) {
			uprintf("Pid %d killed due to text modification or page I/O error\n",
				u.u_procp->p_pid);
			xkillcnt++;
			/*
			 * -SIGKILL overrides any error previously
			 * returned by hdl_vfault().
			 */
			DEC_FAULTING;
			return(-SIGKILL);
		}

		/*
		 * If the file isn't page-aligned in its view, read
		 * in through the file system and do an unaligned copy
		 * into the user's space.
		 */
		if (rp->r_flags&RF_UNALIGNED) {
			count = unaligned_fault(prp, space, vaddr, vfd, dbd);
		} else {
			u.u_procp->p_flag2 |= SANYPAGE;
			count = VOP_PAGEIN(rp->r_fstore, prp, wrt,
						space, vaddr, &startindex);
			u.u_procp->p_flag2 &= ~SANYPAGE;
                }
		if (count < 0) {
			DEC_FAULTING;
			return count;
		}
		cnt.v_pgin++;		/* number of pageins */
		cnt.v_pgpgin += count;	/* number of pages paged in */
		break;

	case DBD_BSTORE:
                u.u_procp->p_flag2 |= SANYPAGE;
		count = VOP_PAGEIN(rp->r_bstore, prp, wrt, space, vaddr, 
					&startindex);
                u.u_procp->p_flag2 &= ~SANYPAGE;
		if (count < 0) {
			DEC_FAULTING;
			return count;
		}
		cnt.v_pgin++;		/* number of pageins */
		cnt.v_pgpgin += count;	/* number of pages paged in */
		break;

	case DBD_NONE:
		/*
		 * An unassigned page, this is a real error.
		 */
		panic("virtual_fault: on DBD_NONE page");
		break;
	default:
		panic("virtual_fault: unknown dbd type");
		break;

	case DBD_DFILL:
	case DBD_DZERO:
		/*
		 * Demand zero or demand fill page.
		 */

		vfd->pgm.pg_cw = 0;

		if (freemem < desfree && !(u.u_procp->p_flag & SSYS) && 
			u.u_procp->p_usrpri > PUSER) {
		    /*
		     * If this is not a system process or a high priority
		     * process, and memory is tight, wait for more memory to
		     * become available, instead of just grabbing whatever is
		     * left right now.  The level we may drive free memory
		     * down to is related to our priority.  A process running
		     * at PUSER (default nice, recently idle) may drive
		     * freemem to zero, but lower priority processes may not.
		     * Worst case for a process means waiting until memory is
		     * above desfree.
		     */
		    sleepmem = (u.u_procp->p_usrpri - PUSER) * desfree /
					(PMAX_TIMESHARE - PUSER);
		    if (freemem < sleepmem) {
			regrele(rp);
			do {
			    int s;

			    s = spl6();
			    memory_sleepers = 1;
			    if (minsleepmem > sleepmem)
				minsleepmem = sleepmem;
			    sleep(&memory_sleepers, PSWP+2);
			    splx(s);
			} while (freemem < sleepmem);
			reglock(rp);
			vfd = FINDVFD(rp, pgindx);
			if (vfd->pgm.pg_v) {
			    DEC_FAULTING;
			    return 0;
			}
		    }
		}

		VFDMEM_ONE(prp, rp, space, vaddr, pgindx, vfd, restart);
		if (restart) {
			DEC_FAULTING;
			return(0);
		}

		/*
		 * The vfd/dbd pointers could have become stale when
		 * we slept without the region lock in VFDMEM_ONE()
		 */
		FINDENTRY(rp, pgindx, &vfd, &dbd);
#ifdef PFDAT32
		if (dbd->dbd_type == DBD_DZERO) {
			bzero(ptob(hiltopfn(vfd->pgm.pg_pfn)), NBPG);
			cnt.v_zfod++;
		}
		hdl_addtrans(prp, space, vaddr, 0, (int) vfd->pgm.pg_pfn);
		purge_dcache();
#else		
		if (dbd->dbd_type == DBD_DZERO) {
			space_t	nspace;
			caddr_t	nvaddr;

			hdl_user_protect(prp, space, vaddr, 1, 
				&nspace, &nvaddr, 0 /* steal */);
			hdl_zero_page(nspace, nvaddr, ptob(1));
			hdl_user_unprotect(nspace, nvaddr, 1, 0);
			cnt.v_zfod++;
		} else {
			hdl_addtrans(prp, space, vaddr, 
				0 /* cw */, (int) vfd->pgm.pg_pfn);
		}
#endif		
		dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff0c;
		pfnunlock(vfd->pgm.pg_pfn);
		count = 1;
		break;

	}
	DEC_FAULTING;
	KI_vfault(wrt, space, vaddr, prp);

	/*
	 * If writing to a c.w. page, then save a fault by breaking
	 * the disk association now.
	 */
	if (wrt) {
		for (i = 0, j = startindex; i < count; i++, j++) {
			vfd_t *vfdp;
			vfdp = FINDVFD(rp, j);
			/*
			 * if count > 1, we may have slept in a
			 * previous hdl_cwfault (memreserve())
			 * and lost some or all of our newly
			 * pre-paged pages.  No big deal, skip that
			 * page and let the user fault on it again.
			 */
			if ((vfdp->pgm.pg_v) && (vfdp->pgm.pg_cw)) {
				pfnlock(vfdp->pgm.pg_pfn);
				if (!hdl_cwfault(wrt, prp, j))
					return(0);
			}
		}
	}
	return(0);
}

/*
 * Page in a page by reading it throught the vn_rdwr
 * interface and then copying onto the region pages.
 *
 * XXX - In the future, we want to prepage in several pages
 * just like for the aligned case.
 */
unaligned_fault(prp, space, vaddr, vfd, dbd)
	preg_t *prp;
	space_t space;
	caddr_t vaddr;
	vfd_t *vfd;
	dbd_t *dbd;
{
	register int off, len;	/* offset/length in text file */
	int resid;		/* resid count from vn_rdwr */
	int unread;		/* # bytes not filled by vn_rdwr in page */
	register int pgindx;
	space_t nspace;
	caddr_t nvaddr;
	void memunreserve();
	register reg_t *rp = prp->p_reg;
	int ret = 1;
	int rval;

	pgindx = regindx(prp, vaddr);

	/* Reserve the page used by vfdfill() */
	if (memreserve(rp, (unsigned)1)) {
		/*
		 *  The vfd/dbd pointers could have become stale when
		 *  we slept without the region lock in memreserve()
                 *  There is also the possibility that some other process
                 *  paged this in for us, then the pageout daemon moved 
                 *  it back out.
		 */
	        FINDENTRY(rp, pgindx, &vfd, &dbd);
		if (vfd->pgm.pg_v || (dbd->dbd_type != DBD_FSTORE)) {
			VASSERT(dbd->dbd_type != DBD_HOLE);
			memunreserve(1);
			return(1);
		}
	}

	/*
	 * Calculate offset & length of chunk to read
	 */
	off = ptob(pgindx);
	len = NBPG;
	if ((off+len) > rp->r_bytelen) {
	    len = rp->r_bytelen - off;
	    VASSERT(len > 0);
	}

	/*
	 * Give the region a page at the appropriate point.
	 */
	vfdfill(pgindx, 1, prp);

	/* Read in the data */
	hdl_user_protect(prp, space, vaddr, 1, &nspace, &nvaddr, 0);
	if (rval = vn_rdwr(UIO_READ, rp->r_fstore, nvaddr, len, 
			 (int)rp->r_byte + off, UIOSEG_PAGEIN, IO_UNIT, 
			 &resid, 0) != 0)
		ret = -rval;


	/* Clear off any unfilled bytes at end of read */
	unread = NBPG - (len-resid);
	if (unread) {
		hdl_zero_page(nspace, nvaddr + (NBPG - unread), unread);
	}

        /*
         * Clear the modification bit.  This must be done before
         * hdl_user_unprotect for MP.
         */
	hdl_unsetbits((int)vfd->pgm.pg_pfn, VPG_MOD);

	hdl_user_unprotect(nspace, nvaddr, 1, 0);

	pfnunlock(vfd->pgm.pg_pfn);
	dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff0d;
	return(ret);
}

/*
 * Main protection fault handler.
 *	vas	-> pointer to vritual address space
 *	wrt	-> type of access: 0=READ, 1=WRITE
 *	space	-> virtual space
 *	vaddr	-> virtual address
 *
 * returns either the signal to send (failure) or 0 (success).
 */
/*ARGSUSED*/
prot_fault(prp, wrt, space, vaddr)
	preg_t *prp;
	int wrt;
   	space_t space;
   	caddr_t vaddr;
{
	reg_t	*rp;
	register vfd_t	*vfd;
	int	pgindx;

	if (prp == NULL)
		return(SIGBUS);

	/*
	 * Find vfd as mapped by region
	 */
	rp = prp->p_reg;
	pgindx = regindx(prp, vaddr);
	vfd = FINDVFD(rp, pgindx);

	if (!vfd->pgm.pg_v) {
		/* page became invalid due to vhand or sched
		 * while we acquired region lock.  Backout and
		 * retry instruction to cause entry to vfault.
		 */
		return(0);
	}

	/*
	 * Check that the fault is due to a copy on write page.
	 * This might also be a write fault to a DBD_HOLE in a
	 * memory mapped file.
	 */
	if (!vfd->pgm.pg_cw) {
		int writable;

		/*
		 * Check to see if this page has been protected
		 * or unmapped with mprotect().  If the page is
		 * really unmapped, we deliver a SIGSEGV to the
		 * process.  If they do not have the requested
		 * access, we deliver a SIGBUS to the process.
		 */
		if (IS_MPROTECTED(prp)) {
			u_long mprot = hdl_page_mprot(prp, vaddr);

			switch (mprot) {
			case MPROT_UNMAPPED:
				return(SIGSEGV);

			case MPROT_NONE:
				return(SIGBUS);

			case MPROT_RO:
				writable = 0;
				break;

			case MPROT_RW:
				writable = 1;
				break;
			}
		}
		else {
			writable = (prp->p_prot & PROT_WRITE);
		}

		if (wrt && writable) {
			/*
			 * This may be an attempt to write to a hole
			 * in a writable memory mapped file.  If so,
			 * free the page (it will still be in the page
			 * cache).  Then backout, the retry of the
			 * instruction will cause an entry to vfault().
			 *
			 * NOTE:
			 *   we could have used FINDENTRY() earlier
			 *   to get the dbd at the same time we were
			 *   getting the vfd, but that would have
			 *   slowed down the "typical" case.  This
			 *   code should rarely get executed, so we
			 *   get the dbd here, optimizing for the
			 *   more common path.
			 *
			 * We check for both DBD_HOLE and DBD_FSTORE
			 * since vfs_mark_dbds() can change the state
			 * of a dbd from HOLE to FSTORE while leaving
			 * the translation read-only.  The reason for
			 * checking the DBD type at all is so that we
			 * do not try to pagein things that we cannot
			 * possibly pagein (like DBD_NONE).
			 */
			dbd_t *dbd = FINDDBD(rp, pgindx);
			int dbd_type = dbd->dbd_type;

			if (dbd_type == DBD_HOLE || dbd_type == DBD_FSTORE) {
				int pfn = vfd->pgm.pg_pfn;

				pfnlock(pfn);
				vfd->pgm.pg_v = 0;
				rp->r_nvalid--;
				hdl_deletetrans(prp, space, vaddr, pfn);
#ifdef FSD_KI
				memfree(vfd, rp);
#else  /* ! FSD_KI */
				memfree(vfd);
#endif /* ! FSD_KI */
				return(0);
			}
		}
		return(SIGBUS);
	}

	/*	Copy on write
	 */
	pfnlock(vfd->pgm.pg_pfn);
	if (hdl_cwfault(wrt, prp, pgindx))
		cnt.v_cwfault++;
	return(0);
}
