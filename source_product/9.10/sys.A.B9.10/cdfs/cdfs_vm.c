
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_vm.c,v $
 * $Revision: 1.8.83.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:18 $
 */

/*
 * cdfs_vm.c--routines implementing the vnode layer abstraction for cdfs
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/dbd.h"
#include "../h/vfd.h"
#include "../h/region.h"
#include "../h/pregion.h"
#include "../h/vmmeter.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/pfdat.h"
#include "../h/sysinfo.h"
#include "../h/buf.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"


/*
 * CDFS variant of the DBD format.  Simply the DBD type, and a block number
 *  for reading the data off the devvp for the vnode.
 */
typedef struct cdfsdbd {
	uint dbd_type : 4;
	uint dbd_blkno : 28;
} cdfsdbd_t;


/*
 * Cdfs pagein routine.
 */
int
cdfs_pagein(prp, wrt, space, vaddr)
	preg_t *prp;
	int wrt;
	space_t space;
	caddr_t vaddr;
{
	int errorfl;
	int resid;
	off_t	offset;
	dbd_t *dbd2;
	vfd_t *vfd2;
	register cdfsdbd_t *dbd;
	register vfd_t *vfd;
	int pgindx;
	reg_t *rp = prp->p_reg;
	struct vnode *filevp;
	pfd_t *pfd;
	int count;
	caddr_t nvaddr;
	space_t nspace;
	struct cdnode *cdp;
	struct cdfs *cdfs;
	vm_sema_state;		/* semaphore save state */
	void memunreserve();
	vmemp_lockx();		/* lock down VM empire */
	/*	
	 * Find vfd and dbd as mapped by region
	 */
	pgindx = regindx(prp, vaddr);

	FINDENTRY(rp, pgindx, &vfd2, &dbd2);
	vfd = vfd2;
	dbd = (cdfsdbd_t *)dbd2;

	/*
	 * Which vnode did I fault for?
	 *
	 * Note:  Code in hdl_pfault (S800) depends on the algorithm
	 * used here for hashing pages in the page cache.  This
	 * should probably be a future VOP function so that the
	 * details aren't exported to that routine.
	 */
	if (dbd->dbd_type == DBD_FSTORE)
	    filevp = rp->r_fstore;
	else
	    filevp = rp->r_bstore;

	cdp = VTOCD(filevp);
	cdfs = cdp->cd_fs;

	/*
	 * Make sure that the dbd contains a valid block
	 * number.
	 */
	offset = vnodindx(rp, pgindx);
	if (dbd->dbd_blkno == DBD_DINVAL)
		dbd->dbd_blkno = offset / NBPG;

	/*	
	 * If the page we want is in memory already, take it
	 */
	if (pfd = pageincache(filevp, dbd->dbd_blkno)) {
		VASSERT((pfd->pf_data == dbd->dbd_blkno) &&
			     (pfd->pf_devvp == filevp));
#ifdef PFDAT32
		vfd->pgm.pg_pfn = pfd - pfdat;
#else
		vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
		vfd->pgm.pg_v = 1;
		rp->r_nvalid++;
		if (!hdl_cwfault(wrt, prp, pgindx)) {
			vmemp_returnx(0);     /* free up VM empire and return */
		} else {
			minfo.cache++;
			cnt.v_pgrec++;
                        cnt.v_pgfrec++;
			u.u_ru.ru_minflt++;	/* count reclaims */
                        if (prp->p_type == PT_TEXT)
                        	cnt.v_xifrec++;
			vmemp_returnx(1);     /* free up VM empire and return */
		}
	}
	u.u_ru.ru_majflt++;		/* It's a real fault, not a reclaim */

	/*
	 * How many pages to bring in at once?
	 * Cdfs has a logical block size of either 512, 1K, or 2k. 
	 * Thus the block size is less than the page size. Because of 
	 * this we will read out of the buffer cache instead, so lets
	 * just get one page for now.
	 */
	count = 1;

	/*
	 * First reserve memory for the vfdfill below.  We must do this 
	 * now to prevent problems when we go to insert the page in the 
	 * hash.
	 */
	if (memreserve(rp, (unsigned int)count)) {
		/*
		 * The vfd/dbd pointers could have become stale when
		 * we slept without the region lock in memreserve()
		 */
		FINDENTRY(rp, pgindx, &vfd2, &dbd2);
		vfd = vfd2;
		dbd = (cdfsdbd_t *)dbd2;
		/*	
		 * We went to sleep waiting for memory.
		 * check if the page we're after got loaded in
		 * the mean time.  If so, give back the memory
		 * and return
		 */
		if (vfd->pgm.pg_v) {
			memunreserve((unsigned int)count);
			vmemp_returnx(1);    /* free up VM empire and return */
		}
		if (pfd = pageincache(filevp, dbd->dbd_blkno)) {
			VASSERT((pfd->pf_data == dbd->dbd_blkno) &&
			     (pfd->pf_devvp == filevp));
			memunreserve((unsigned int)count);
#ifdef PFDAT32
			vfd->pgm.pg_pfn = pfd - pfdat;
#else
			vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
			vfd->pgm.pg_v = 1;
			rp->r_nvalid++;
			if (!hdl_cwfault(wrt, prp, pgindx)) {
				vmemp_returnx(0);
			} else {
				minfo.cache++;
				cnt.v_pgrec++;
                                cnt.v_pgfrec++;
                                if (prp->p_type == PT_TEXT)
                                        cnt.v_xifrec++;
				vmemp_returnx(1);
			}
		}
	}

	vfdfill(pgindx, (unsigned) count, prp);

	/*
	 * We now have a vfds with a page frame
	 * assigned, whose rightful contents are not in the
	 * page cache. Insert the page frame, and read from
	 * disk.
	 */
	pfd = addtocache((int)vfd->pgm.pg_pfn, filevp, dbd->dbd_blkno);
#ifdef PFDAT32
	if ((pfd - pfdat) != vfd->pgm.pg_pfn) {
		VASSERT((hdl_getbits((int)(pfd - pfdat)) & VPG_MOD) == 0);
		freepfd(&pfdat[vfd->pgm.pg_pfn]);
		vfd->pgm.pg_pfn = pfd - pfdat;
#else
	if (pfd->pf_pfn != vfd->pgm.pg_pfn) {
		VASSERT((hdl_getbits((int)pfd->pf_pfn) & VPG_MOD) == 0);
		freepfd(&pfdat[vfd->pgm.pg_pfn]);
		vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
	} else {
		VASSERT((pfd->pf_data == dbd->dbd_blkno) &&
		        (pfd->pf_devvp == filevp));
	}

	/* 
	 * Protect the pages from the user during
	 *  the I/O operation.
	 */
	hdl_user_protect(prp, space, vaddr, count, &nspace, &nvaddr, 0);

	/*
	 * Because of cdfs's weird logical block sizes, we cannot easily
	 * read straight from disk. Instead we'll call vn_rdwr() to read
	 * the data from the buffer cache and let it determine how to handle
	 * the strange block sizes.
	 */
	errorfl = vn_rdwr(UIO_READ, filevp, nvaddr, NBPG, offset, UIOSEG_PAGEIN,
			  IO_UNIT, &resid, 0);
	

	if (errorfl || rp->r_zomb)
                goto backout;
	/*
	 * We must detect end-of-file and only read in one page worth.
	 * This requires zeroing out the end of the page for mmf's
	 */
	if ((offset + ptob(count)) > cdp->cd_size) {
		int diff;
		
		diff = (offset + ptob(count)) - cdp->cd_size;
		VASSERT((diff / NBPG) == 0);
		hdl_zero_page(nspace, nvaddr + ptob(count)-diff, diff);
	}

        /*
         * clear the modification bit
         */
	hdl_unsetbits(vfd->pgm.pg_pfn, VPG_MOD);

	/*
	 * Unprotect the pages.
	 */
	hdl_user_unprotect(nspace, nvaddr, count, 0);

	/* 	
	 * Mark the I/O done, and awaken anyone
	 * waiting for pfdats
	 */
	pfd = &pfdat[vfd->pgm.pg_pfn];
	pfdatunlock(pfd);
	cnt.v_exfod += count;	/* number of pages from file */
	vmemp_unlockx();	/* free up VM empire */
	return(count);


backout:
	/*
	 * if error during vn_rdwr, unhash the page and delete 
	 * any translations, set count to 0 and set r_zomb in the
	 * region so the process gets killed.
	 */
	VASSERT(PAGEINHASH(pfd));
	pageremove(pfd);
	hdl_user_unprotect(nspace, nvaddr, count, 1);
	pfdatunlock(pfd);
	rp->r_zomb = 1;
	vmemp_unlockx();	/* free up VM empire */
	return(0);
}
