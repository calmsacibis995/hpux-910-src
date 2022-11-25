/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_vm.c,v $
 * $Revision: 1.11.83.4 $	$Author: dkm $
 * $State: Exp $	$Locker:  $
 * $Date: 94/05/05 15:29:10 $
 */

/*
 * vfs_vm.c--routines implementing the vnode layer abstraction for generic
 * filesystem types.
 *
 * These routines are used by various filesystem *_vm.c routines. If you
 * wish to vary from these routines for one specific file system type
 * and the change will not work for all types, create your own new routine
 * to place in *_vm.c and set the vnodeopes table to point to the new
 * routine.
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/duxfs.h"
#include "../h/dbd.h"
#include "../h/vfd.h"
#include "../h/region.h"
#include "../h/pregion.h"
#include "../h/vmmeter.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/pfdat.h"
#include "../h/sysinfo.h"
#include "../h/tuneable.h"
#include "../h/buf.h"
#include "../h/sar.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"

/*
 * max number of pages that vfs_pagein may bring in at a time
 * max number of pages that vfs_pageout may write out at a time
 */
#define VFS_MAXPGIN	16
#define VFS_MAXPGOUT	16

#define BIG_OK_DBD	0x7fffffff

extern int gpgslim;
extern int freemem, lotsfree;
extern int stealvisited;
extern int maxpendpageouts;
extern int parolemem;

extern struct buf *bswalloc();
extern void pregtrunc();
extern void mtrunc();

/*
 * Gross stuff for NFS...
 *
 * NFS requires memory resources to perform a pageout.  However, if
 * vhand is trying to steal pages, we probably do not have memory
 * available to page out NFS pages.  To avoid a deadlock, we do the
 * following for NFS (but only when called by vhand):
 *
 *   . When paging out an NFS region, we conditionally reserve enough
 *     pages (NFS_PAGEOUT_MEM) to guarantee that the NFS will never
 *     have to sleep waiting for memory.  This is essentially one page
 *     for every distinct size of memory that NFS needs (i.e. 1 page for
 *     128 byte buffers, 1 page for 512 byte buffers, etc).  If we
 *     cannot get the memory, we skip the dirty page and go on.
 *
 *   . In pagein, we do not allow an NFS pagein to proceed until there
 *     is at least minumum number of free pages (2 * NFS_PAGEOUT_MEM).
 *     This guarantees that there will always be enough non-NFS pages
 *     to page out so that we can reserve enough pages to page out
 *     dirty NFS pages.
 */
#define NFS_PAGEOUT_MEM		    7	/* empirically determined */
#define NFS_PAGEIN_MEM		   14
#define PAGEOUT_RESERVED   0x80000000

/*
 * vfs prefill
 */
vfs_prefill(prp)
	register preg_t *prp;
{
#ifdef OSDEBUG
	panic("vfs_prefill called!");
#endif /* OSDEBUG */
}

/*
 * vfs_mapdbd() --
 *    Computes a dbd_data for a page within an MMF.  Write requests to
 *    local files cause space to be allocated (for sparse files).  Space
 *    reservation for remote files is handled by vfs_pagein().
 */
dbd_t
vfs_mapdbd(pgindx, rp, filevp, rwflg)
int pgindx;
reg_t *rp;
struct vnode *filevp;
int rwflg;	/* B_READ or B_WRITE */
{
    daddr_t lbn, bn;
    int on;
    long offset;
    dbd_t ret_dbd;

    /*
     * lbn = <logical_block_number> for this part of
     *	 the file (bytes)
     * on = offset in the block (bytes)
     */
    offset = vnodindx(rp, pgindx);

    switch (filevp->v_fstype) {
    case VUFS:
	{
	    register struct inode *ip = VTOI(filevp);

	    lbn = lblkno(ip->i_fs, offset);
	    on = blkoff(ip->i_fs, offset);
	    VASSERT((on % NBPG) == 0);
	    bn = fsbtodb(ip->i_fs, bmap(ip, lbn, rwflg, NBPG, 0, 0, 0));
	}
	break;

    case VNFS:
	{
	    long bsize = vtoblksz(filevp) & ~(DEV_BSIZE - 1);

	    if (bsize <= 0 )
		panic("vfs_mapdbd: zero size");
	    lbn = offset / bsize;
	    on = offset % bsize;
	    VASSERT((on % NBPG) == 0);
	    VOP_BMAP(filevp, lbn, (struct vnode *)NULL, &bn);
	}
	break;

    case VDUX:
	lbn = duxlblkno(VTOI(filevp)->i_dfs, offset);
	on = duxblkoff(VTOI(filevp)->i_dfs, offset);
	VASSERT((on % NBPG) == 0);
	VOP_BMAP(filevp, lbn, (struct vnode *)NULL, &bn);
	break;

    default:
	panic("vfs_mapdbd: unknown file system type");
    }

    if ((long)bn < 0) {
	ret_dbd.dbd_type = DBD_HOLE;
	ret_dbd.dbd_data = DBD_DINVAL;
    }
    else {
	ret_dbd.dbd_type = DBD_FSTORE;
	ret_dbd.dbd_data = bn + btodb(on);
    }
    return ret_dbd;
}

/* The arguments passed to our sparse walk routine */
typedef struct {
    preg_t *prp;
    int    flags;
    struct vnode *vp;	 /* file vnode pointer */
    u_long isize;	 /* file size */
    u_long bsize;	 /* file system block size */
    u_long ok_dbd_limit; /* one past the last dbd that we can trust */
    long   maxpgs;	 /* maximum length a run can be */
    size_t start;	 /* start of a run */
    dbd_t  dbd_start;	 /* dbd for start */
    size_t end;		 /* end of a run */
    dbd_t  dbd_end;	 /* dbd for end */
    int    run;		 /* a run has been detected */
    int	   remote;	 /* the file is remote (DUX, NFS) */
    int	   remote_down;  /* is the remote system down now? */
} vfsargs_t;

/*
 * lastpageinblock() --
 *    Given a dbd, and a file system block size, this macro determines
 *    if the dbd is the last page of the block.
 */
#define lastpageinblock(dbd, bsize) \
	((dbtob(dbd) + NBPG) % (bsize) == 0)

/*
 * Sparse walk function to free memory in the file's mapping
 *
 * Return code 0 = continue looping in calling routine.
 * Return code 1 = stop looping in calling routine.
 */
static int
vfs_vfdcheck(rindex, vfd, dbd, args)
int rindex;
vfd_t *vfd;
dbd_t *dbd;
vfsargs_t *args;
{
    preg_t *prp = args->prp;
    reg_t *rp = prp->p_reg;
    int pfn = vfd->pgm.pg_pfn;
    pfd_t *pfd = &pfdat[pfn];
    int pgbits;
    dbd_t cur_dbd;
    struct vnode *filevp;
    int vhand;		/* commonly used flag */
    int hard;		/* commonly used flag */
    int steal;		/* commonly used flag */
    int purge;		/* commonly used flag */

 
    /*
     * If we have a run and this page is not contiguous with the
     * end of our run, terminate the run.
     */
    if (args->run && rindex != (args->end + 1))
	return 1;

    /*
     * Initialize commonly used flags
     */
    vhand = (args->flags & PAGEOUT_VHAND);
    hard  = (args->flags & PAGEOUT_HARD);
    steal = (args->flags & PAGEOUT_FREE);
    purge = (args->flags & PAGEOUT_PURGE);

#ifdef OSDEBUG
    /*
     * If PAGEOUT_PURGE was set, PAGEOUT_HARD and PAGEOUT_FREE
     * must also be set.
     */
    if (purge)
	VASSERT(hard && steal);
#endif /* OSDEBUG */

    /*
     * If vhand is trying to steal pages and we have succeeded in
     * freeing enough memory for now, stop scanning for more things
     * to free.
     */
    if (vhand && steal && !hard && freemem >= lotsfree)
	return 1;

    /*
     * Keep "end" up to date so that vhand can tell the last page
     * that we visited.
     */
    if (args->run == 0)
	args->end = rindex;

    /*
     * Skip locked vfds.
     *
     * If we have a run, stop and process it.  If no run has been
     * found yet, keep looking for a run.
     */
    if (vfd->pgm.pg_lock)
	return args->run;

    /*
     * Take the lock on the page.  If it's already locked and we're
     * not doing a hard pageout, just give up on this page.
     */
    if (!cpfdatlock(pfd)) {
	if (hard == 0) {
	    /*
	     * If we have a run, stop and process it.  If no run
	     * has been found yet, keep looking for a run.
	     */
	    return args->run;
	}
	pfdatlock(pfd);
    }

    if (vhand && steal)
	++stealvisited;

    /*
     * We have the page locked.  If we are want to steal this page, we
     * see if it has been referenced.  We do not steal referenced pages
     * unless PAGEOUT_HARD is set.  If we are not going to steal this
     * page, we do not care if it has been referenced, we still want
     * to write it out if it is dirty.
     */
    pgbits = hdl_getbits(pfn);
    if (!hard && steal && (pgbits & VPG_REF)) {
	pfdatunlock(pfd);
	return args->run;
    }

    /*
     * A clean page.  Steal it if PAGEOUT_FREE is set, otherwise
     * leave it alone.
     */
    if (!(pgbits & VPG_MOD)) {
	if (!steal) {
	    pfdatunlock(pfd);
	    return args->run;
	}

    freeit:
	/*
	 * Clear HDL translation for it, return it to the free pool
	 */
	vfd->pgm.pg_v = 0;
	rp->r_nvalid--;

	/*
	 * Tell HDL, as otherwise we've cleared the VFD
	 * but potentially left an HDL translation.
	 */
#ifdef __hp9000s800
	/*
	 * Oh, what a kludge.  On PA-RISC, we really do want to delete
	 * whatever translation that exists for this page.  On 68k,
	 * hdl_deletetrans() does this, but the PA-RISC version only
	 * deletes the translation if the page is translated to the
	 * (space, vaddr) that we give it.  Since we are paging out
	 * through the psuedo-pregion, this is not really the correct
	 * thing.  The easiest way to fix this (for now) is to simply
	 * call hdl_unvirtualize() and delete whatever translation
	 * exists for this page.
	 */
	hdl_unvirtualize(pfn);
#else
	hdl_deletetrans(prp, prp->p_space,
			prp->p_vaddr + ptob(rindex - prp->p_off), pfn);
#endif /* __hp9000s800 */
	if (purge && PAGEINHASH(pfd))
	    pageremove(pfd);
	if (vhand)
	    cnt.v_dfree++;
#ifdef FSD_KI
	memfree(vfd, rp);
#else  /* ! FSD_KI */
	memfree(vfd);
#endif /* ! FSD_KI */

	return args->run;
    }

    /*
     * We have a dirty page (pfd).  If the file we are paging to is on
     * a remote system that is "down", we skip paging dirty pages for
     * the time being.  The next time vhand visits this region, we
     * will re-evaluate the state of the remote system, and decide
     * what to do.
     */
    if (args->remote_down) {
	/*
	 * The remote system is down, unlock the current
	 * (dirty) page and return.
	 */
	pfdatunlock(pfd);
	VASSERT(args->run == 0);
	return 0;
    }

    /*
     * A dirty page.  If we have not yet determined the file size and
     * other attributes that we need to write out pages (the block
     * size and ok_dbd_limit), get that information now.
     */
    if (args->bsize == 0) {
	struct inode *ip;
	u_long isize;
	long bsize;
	int ok_dbd_limit;	/* limit of trustworthy DBDs */
	int file_is_remote;
	struct vattr va;

	/*
	 * If this region has been marked dead, just throw away the
	 * dirty page.
	 */
	if (rp->r_zomb) {
	    hdl_unsetbits(pfn, VPG_MOD);
	    if (PAGEINHASH(pfd))
		 pageremove(pfd);
	    goto freeit;
	}

	/*
	 * Get the various attributes about the file.  Store them
	 * in args for the next time around.
	 */
	filevp = rp->r_bstore;
	switch (filevp->v_fstype) {
	case VUFS:
	    ip = VTOI(filevp);
	    bsize = ip->i_fs->fs_bsize;
	    args->maxpgs = VFS_MAXPGOUT;
	    isize = ip->i_size;

	    /*
	     * Compute the limit of trustworthy dbds.  dbds for pages
	     * starting at ok_dbd_limit are potentially within a
	     * fragment, which is subject to reallocation.  We must
	     * re-compute the dbd for all such pages.
	     *
	     * Files larger than NDADDR * bsize cannot have fragments
	     * so we can cache all dbds for such files.
	     */
	    if (isize >= (NDADDR << ip->i_fs->fs_bshift))
		ok_dbd_limit = BIG_OK_DBD;
	    else
		ok_dbd_limit = btop(isize - isize % bsize) - rp->r_off;
	    file_is_remote = 0;
	    args->remote_down = 0;
	    break;

	case VNFS:
	    bsize = vtoblksz(filevp);
	    args->maxpgs = btop(bsize);

	    if (VOP_GETATTR(filevp, &va, u.u_cred, VIFSYNC) != 0) {
		/*
		 * The VOP_GETATTR() failed.  If the mount information
		 * for this file system says that the server is down,
		 * and we are vhand, and this is a hard mount, we will
		 * skip dirty pages for a while and try again later.
		 */
		if (vhand && vtomi(filevp)->mi_down && vtomi(filevp)->mi_hard) {
		    args->remote_down = 1;
		    pfdatunlock(pfd);
		    VASSERT(args->run == 0);
		    return 0;
		}

		/*
		 * This is a "soft" mount, or some other error was
		 * returned from the NFS server.  Mark this region
		 * as a zombie, and free this dirty page.
		 */
		rp->r_zomb = 1;
		hdl_unsetbits(pfn, VPG_MOD);
		if (PAGEINHASH(pfd))
		     pageremove(pfd);
		goto freeit;
	    }
	    isize = va.va_size;

	    /*
	     * Since remote files handle fragments transparently, we can
	     * always trust cached dbd values.  Set the limit to the size
	     * of the file.  We can ignore rp->r_off, since the smallest
	     * value it can have is zero, and all we want is a value that
	     * is guaranteed to be larger then the last page in the memory
	     * object.
	     */
	    ok_dbd_limit = BIG_OK_DBD;
	    file_is_remote = 1;
	    break;

	case VDUX:
	    bsize = VTOI(filevp)->i_dfs->dfs_bsize;
	    args->maxpgs = btop(bsize);
	    if (VOP_GETATTR(filevp, &va, u.u_cred, VIFSYNC) != 0) {
		rp->r_zomb = 1;
		hdl_unsetbits(pfn, VPG_MOD);
		if (PAGEINHASH(pfd))
		     pageremove(pfd);
		goto freeit;
	    }
	    isize = va.va_size;

	    /*
	     * Since remote files handle fragments transparently, we can
	     * always trust cached dbd values.  Set the limit to the size
	     * of the file.  We can ignore rp->r_off, since the smallest
	     * value it can have is zero, and all we want is a value that
	     * is guaranteed to be larger then the last page in the memory
	     * object.
	     */
	    ok_dbd_limit = BIG_OK_DBD;
	    file_is_remote = 1;
	    args->remote_down = 0;
	    break;
	}

	args->isize = isize;
	args->bsize = bsize;
	args->ok_dbd_limit = ok_dbd_limit;
	args->remote = file_is_remote;

	/*
	 * See if the file has shrunk (this could have happened
	 * asynchronously because of NFS or DUX).  If so, invalidate
	 * all of the pages past the end of the file. This is only
	 * needed for remote files, as local files are truncated
	 * synchronously.
	 */
	if (file_is_remote && vnodindx(rp, rindex) > isize) {
	    int pglen = btorp(isize);

	    /*
	     * This page is past the end of the file.  Unlock this page
	     * (regtrunc will throw it away) and then call regtrunc()
	     * to invalidate all pages past the new end of the file.
	     */
	    pfdatunlock(pfd);
	    regtrunc(rp, pglen, pglen + 1);
	    return args->run;
	}
    }
    else {
	filevp = rp->r_bstore;
    }

    /*
     * If this region has been marked dead, just throw away the
     * dirty page.
     */
    if (rp->r_zomb) {
	hdl_unsetbits(pfn, VPG_MOD);
	if (PAGEINHASH(pfd))
	     pageremove(pfd);
	goto freeit;
    }

    /*
     * Gross stuff for NFS.  See comment at the beginning of this file
     * for details...
     */
    if (vhand && !(args->flags & PAGEOUT_RESERVED) && filevp->v_fstype == VNFS) {
	VASSERT(args->run == 0);
	if (procmemreserve(NFS_PAGEOUT_MEM, 0, (reg_t *)0)) {
	    /*
	     * Got enough memory to pageout.  Mark the fact that we did
	     * a procmemreserve(), so that we can procmemunreserve() it
	     * later (in vfs_pageout()).
	     */
	    args->flags |= PAGEOUT_RESERVED;
	}
	else {
	    /*
	     * We do not have enough memory to do this pageout.  By
	     * definition, we do not yet have a run, so we just unlock
	     * this page and tell foreach_valid() to continue scanning.
	     * If we come across another dirty page, we will try to
	     * reserve memory again.  That is okay, in fact some memory
	     * may have freed up (as earlier pageouts complete under
	     * interrupt).
	     */
	    pfdatunlock(pfd);
	    return 0;
	}
    }

    /*
     * Make sure that we have a valid dbd.  In most cases, this
     * is filled in when the page was faulted in, but we cannot
     * pre-compute the dbds for fragments at the end of the file,
     * since these can be rearranged.
     */
    cur_dbd = *dbd;
    VASSERT(cur_dbd.dbd_type != DBD_HOLE);
    if (cur_dbd.dbd_data == DBD_DINVAL || rindex >= args->ok_dbd_limit) {
	cur_dbd = vfs_mapdbd(rindex, prp->p_reg, args->vp, B_READ);
	VASSERT(cur_dbd.dbd_type != DBD_HOLE);
    }

    /*
     * A new run, fill in the start and end points and set the run flag.
     */
    if (args->run == 0) {
	args->start = args->end = rindex;
	args->dbd_start = args->dbd_end = cur_dbd;
	args->run = 1;

	/*
	 * We will be sending this page out.  If we will also be
	 * stealing the translations for the page, mark the page
	 * invalid since it is on its way out now.
	 */
	if (steal) {
	    vfd->pgm.pg_v = 0;
	    rp->r_nvalid--;
	}
	if (purge && PAGEINHASH(pfd))
	    pageremove(pfd);
	dbd->dbd_type = DBD_FSTORE;

	/*
	 * If the maximum number of contiguous pages that we can page
	 * out at once is only a single page, stop scanning vfds and
	 * return to vfs_pageout() so that it can process this run.
	 *
	 * If this is a remote file, and this page corresponds to the
	 * last part of a file system block, we also return 1.  We do
	 * not want a run to span a file system block boundary.
	 */
	if (args->maxpgs == 1 ||
	    (args->remote && lastpageinblock(cur_dbd.dbd_data, args->bsize)))
	    return 1;
	else
	    return 0;
    }

    /*
     * We already have a run, see if this page is contiguous on disk
     * with the end of our current run.
     */
    if (cur_dbd.dbd_data == (args->dbd_end.dbd_data + btodb(NBPG))) {
	args->end = rindex;
	args->dbd_end = cur_dbd;

	/*
	 * We will be sending this page out.  If we will also be
	 * stealing the translations for the page, mark the page
	 * invalid since it is on its way out now.
	 */
	if (steal) {
	    vfd->pgm.pg_v = 0;
	    rp->r_nvalid--;
	}
	if (purge && PAGEINHASH(pfd))
	    pageremove(pfd);
	dbd->dbd_type = DBD_FSTORE;

	/*
	 * See if we have hit our maximum run length.  If so, stop
	 * scanning vfds and return to vfs_pageout() so that it can
	 * process this run.
	 *
	 * If this is a remote file, and this page corresponds to the
	 * last part of a file system block, we also return 1.  We do
	 * not want a run to span a file system block boundary.
	 */
	VASSERT((args->end + 1 - args->start) <= args->maxpgs);
	if ((args->end + 1 - args->start) == args->maxpgs ||
	    (args->remote && lastpageinblock(cur_dbd.dbd_data, args->bsize)))
	    return 1;
	else
	    return 0;
    }

    /*
     * This page is not contiguous with the current run.  Stop scanning
     * vfds and return to vfs_pageout() so that it can process this run.
     */
    pfdatunlock(pfd);
    return 1;
}

/*
 * vfs_pageout() --
 *   This function performs several functions, based on the
 *   setting of the 'flags' parameter.  The basic operations
 *   are:
 *     . optionally write out dirty pages (optionally free)
 *     . optionally free clean pages
 *     . optionally wait until all schedule I/O is complete
 *
 * flags --
 *   PAGEOUT_FREE  -- free the pages when I/O complete
 *   PAGEOUT_HARD  -- we really want this page to go out. sleep for page
 *		      lock, pageout/free even if referenced.
 *   PAGEOUT_WAIT  -- wait for I/O to complete
 *   PAGEOUT_VHAND -- vhand initiated this VOP_PAGEOUT
 *   PAGEOUT_SWAP  -- swapping rather than paging (statistics)
 */
void
vfs_pageout(prp, start, end, flags)
preg_t *prp;
size_t start, end;
int flags;
{
    vm_sema_state;	/* semaphore save state */
    reg_t *rp = prp->p_reg;
    struct vnode *filevp;
    struct vnode *devvp;
    struct inode *ip;
    int i;
    int inode_changed = 0;
    vfsargs_t args;
    int steal;
    int vhand;
    int hard;
    int *piocnt;	/* wakeup counter used if PAGEOUT_WAIT */
    int file_is_remote;
    struct ucred *old_cred;

    VASSERT(rp->r_fstore == rp->r_bstore);
    steal = (flags & PAGEOUT_FREE);
    vhand = (flags & PAGEOUT_VHAND);
    hard  = (flags & PAGEOUT_HARD);

    vmemp_lockx();

    /*
     * If the region is marked "don't swap", then don't steal any pages
     * from it.  We can, however, write dirty pages out to disk (only if
     * PAGEOUT_FREE is not set).
     */
    if (prp->p_reg->r_mlockcnt && steal) {
	vmemp_unlockx();
	return;
    }

    /*
     * If the caller wants to wait until the I/O is complete, we
     * allocate a counter to count the I/Os that we schedule.
     * biodone() will decrement this counter as the I/Os complete,
     * when the counter gets to 0, biodone() will issue a wakeup
     * on the address of the counter.
     *
     * We pre-initialize the counter to 1 so that it does not hit
     * zero until we are ready for it.
     */
    if (flags & PAGEOUT_WAIT) {
	piocnt = &u.u_procp->p_wakeup_cnt;
	*piocnt = 1;
    }
    else
	piocnt = (int *)0;

    filevp = rp->r_bstore;	/* always page out to back store */

    args.remote_down = 0;	/* assume remote file servers are up */
    if (filevp->v_fstype == VUFS) {
	ip = VTOI(filevp);
	devvp = ip->i_devvp;
	file_is_remote = 0;
    }
    else {
	file_is_remote = 1;
	devvp = filevp;

	/*
	 * If we are vhand(), and this is an NFS file, we need to
	 * see if the NFS server is "down".  If so, we decide
	 * if we will try to talk to it again, or defer pageouts
	 * of dirty NFS pages until a future time.
	 */
	if (vhand && filevp->v_fstype == VNFS &&
		vtomi(filevp)->mi_down && vtomi(filevp)->mi_hard) {
	    extern long vhand_nfs_retry;
	    /*
	     * If there is still time left on our timer, we will
	     * not talk to this server right now.
	     */
	    if (vhand_nfs_retry > 0)
		args.remote_down = 1;
	}
    }

    /*
     * Initialize args.  We set bsize to 0 to tell vfs_vfdcheck() that
     * it must get the file size and other attributes if it comes across
     * a dirty page.
     */
    args.prp = prp;
    args.flags = flags;
    args.bsize = 0;
    args.vp = filevp;
    i = start;

    while (i <= end) {
	extern int pageiodone();
	u_int context;
	struct buf *bp;
	space_t nspace;
	caddr_t nvaddr;
	long start;
	int npages;
	long nbytes;

       /*
	* If we get into vfs_vfdcheck with a good page, end will be
	* reset so we know how far we got, otherwise there are no pages
	* we can page, so we've visited all of the ones we can page,
	* so initiallize end to represent this.
	*/
	args.end = prp->p_off + end;
	args.run = 0;
	foreach_valid(rp, (int)prp->p_off + i, (int)end-i+1,
		      vfs_vfdcheck, (caddr_t)&args);

	if (args.run == 0)
	    break;

	/*
	 * We have a run of dirty pages [args.start...args.end].
	 */
	VASSERT(filevp->v_fstype != VCDFS);
	VASSERT((filevp->v_vfsp->vfs_flag & VFS_RDONLY) == 0);

	context = sleep_lock();
	rp->r_poip++;
	sleep_unlock(context);

	/*
	 * Okay, get set to perform the I/O.
	 */
	inode_changed = 1;
	npages = args.end + 1 - args.start;

	/*
	 * Allocate and initialize an I/O buffer.
	 */
	bp = bswalloc();
	bp->b_rp = rp;
	if (piocnt) {
	    context = spl6();
	    (*piocnt)++;
	    splx(context);
	}
	bp->b_pcnt = piocnt;
#ifdef FSD_KI
	/*
	 * Set the b_apid/b_upid fields to the pid (this process' pid)
	 * that last allocated/used this buffer.
	 */
	bp->b_apid = bp->b_upid = u.u_procp->p_pid;

 	/* Identify this buffer for KI */
	bp->b_bptype = B_vfs_pageout|B_pagebf;

 	/* Save site(cnode) that last used this buffer */
	bp->b_site = u.u_procp->p_faddr;
#endif /* FSD_KI */

	if (steal)
	    bp->b_flags = B_CALL|B_BUSY|B_PAGEOUT;    /* steal pages */
	else
	    bp->b_flags = B_CALL|B_BUSY;	      /* keep pages */

	/*
	 * If we are vhand paging over NFS, we will wait for the I/O
	 * to complete.  nfs_strategy() is synchronous anyway, and we
	 * do not want to mark the region a zombie if we are vhand.
	 */
	if (vhand && filevp->v_fstype == VNFS)
	    bp->b_flags &= ~B_CALL;
	else
	    bp->b_iodone = pageiodone;

	/*
	 * Make sure we do not write past the end of the file.
	 */
	nbytes = ptob(npages);
	start = vnodindx(rp, args.start);
	if (start + nbytes > args.isize) {
#ifdef OSDEBUG
	    /*
	     * The amount we are off better not be bigger than a
	     * filesystem block.
	     */
	    if (start + nbytes - args.isize >= args.bsize) {
		printf("start = %d, nbytes = %d, isize = %d, bsize = %d\n",
		    start, nbytes, args.isize, args.bsize);
		panic("vfs_pageout: remainder too large");
	    }
#endif
	    /*
	     * Reset the size of the I/O as necessary.  For remote
	     * files, we set the size to the exact number of bytes to
	     * the end of the file.  For local files, we round this up
	     * to the nearest DEV_BSIZE chunk since disk I/O must always
	     * be in multiples of DEV_BSIZE.  In this case, we do not
	     * bother to zero out the data past the "real" end of the
	     * file, this is done when the data is read (either through
	     * mmap() or by normal file system access).
	     */
	    if (file_is_remote)
		nbytes = args.isize - start;
	    else
		nbytes = roundup(args.isize - start, DEV_BSIZE);
	}

	/*
	 * If this is an NFS write by vhand(), we will not be calling
	 * pageiodone().  asyncpageio() increments parolemem for us
	 * if bp->b_iodone is pageiodone, so we must do it manually
	 * if pageiodone() will not be called automatically.
	 */
	if (!(bp->b_flags & B_CALL) && steal) {
	    int s = CRIT();
	    parolemem += btorp(nbytes);
	    UNCRIT(s);
	}

	/*
	 * Now get ready to perform the I/O
	 */
	hdl_user_protect(prp, prp->p_space,
			 prp->p_vaddr + ptob(args.start - prp->p_off),
			 npages, &nspace, &nvaddr, steal);
	blkflush(devvp, args.dbd_start.dbd_data, nbytes, 1, rp);

	/*
	 * If vhand is the one paging things out, and this is an NFS
	 * file, we need to temporarily become a different user so
	 * that we are not trying to page over NFS as root.  We use
	 * the user credentials associated with the writable file
	 * pointer that is in the psuedo-vas for this MMF.
	 *
	 * NOTE: we are currently using "va_rss" to store the ucred
	 *       value in the vas (this should be fixed in 10.0).
	 */
	old_cred = u.u_cred;
	if (vhand && filevp->v_fstype == VNFS) {
	    u.u_cred = (struct ucred *)filevp->v_vas->va_rss;

	    /*
	     * If root was the one who opened the mmf for write,
	     * va_rss will be NULL.  So reset u.u_cred to what it
	     * was.  We will page out as root, but that is the
	     * correct thing to do in this case anyway.
	     */
	    if (u.u_cred == (struct ucred *)0)
		u.u_cred = old_cred;
	}

	/*
	 * Really do the I/O.
	 */
	asyncpageio(bp, args.dbd_start.dbd_data, nspace, nvaddr, nbytes,
		    B_WRITE, devvp);

	/*
	 * If we are vhand paging over NFS we want to wait for the
	 * I/O to complete and take the appropriate actions if an
	 * error is encountered.
	 */
	if (vhand && filevp->v_fstype == VNFS) {
	    if (waitforpageio(bp) &&
		    vtomi(filevp)->mi_down && vtomi(filevp)->mi_hard) {
		/*
		 * The server is down, ignore this failure, and
		 * try again later. (rfscall() has set our retry
		 * timer).
		 */
		args.remote_down = 1;
		pageiocleanup(bp, 0);

		/*
		 * vfs_vfdcheck() has cleared the valid bit on the
		 * vfds for these pages.  We must go back and set the
		 * valid bit, as the pages are really not gone.
		 *
		 * NOTE: we can do this because we still hold (and have
		 * not released) the region lock.
		 */
		if (steal) {
		    int pgi;

		    for (pgi = args.start; pgi <= args.end; pgi++) {
			vfd_t *vfdv = FINDVFD(rp, pgi);

			VASSERT(vfdv->pgm.pg_v == 0);
			if (vfdv->pgm.pg_v == 0) {
			    vfdv->pgm.pg_v = 1;
			    rp->r_nvalid++;
			}
		    }
		}
	    }
	    else {
		/*
		 * The I/O succeeded, or we had an error that we do
		 * not want to defer until later.  Call pageidone()
		 * to handle things.
		 */
		pageiodone(bp);
	    }
	}

	/*
	 * And restore our credentials to what they were.
	 */
	u.u_cred = old_cred;

	/*
	 * If we reserved memory in vfs_vfdcheck(), (only for NFS) we
	 * can now unreserve it.
	 */
	if (args.flags & PAGEOUT_RESERVED) {
	    args.flags &= ~PAGEOUT_RESERVED;
	    procmemunreserve();
	}

	/*
	 * Update statistics
	 */
	if (steal) {
	    if (flags & PAGEOUT_SWAP) {
		syswait.swap--;
		cnt.v_pswpout += npages;
		sar_bswapout += ptod(npages);
	    }
	    else if (vhand) {
		cnt.v_pgout++;
		cnt.v_pgpgout += npages;
	    }
	}

	/*
	 * If time and patience have delivered enough
	 * pages, then quit now while we are ahead.
	 */
	if (vhand && !hard && parolemem >= maxpendpageouts)
	    break;

	i = args.end - prp->p_off + 1;
    }

    if (vhand)
	prp->p_stealscan = args.end - prp->p_off + 1;

    vmemp_unlockx();

    /*
     * If we wanted to wait for the I/O to complete, sleep on piocnt.
     * We must decrement it by one first, and then make sure that it
     * is non-zero before going to sleep.
     */
    if (piocnt) {
	regrele(rp);
	i = spl6();
	if (--(*piocnt) > 0)
	    sleep(piocnt, PRIBIO+1);
	splx(i);
	reglock(rp);
    }

    if (inode_changed && !file_is_remote) {
	imark(ip, IUPD|ICHG);
	iupdat(ip, 0, 0);
    }
}

/*
 * vfs_alloc_hole() --
 *    Given a page that we previously faulted in read-only that maps
 *    to a potential hole in a remote file, this function forces the
 *    remote system to allocate file system space by issuing a write
 *    to the corresponding block.
 *
 *    This is a utility function for vfs_pagein() that is called only
 *    for remote (DUX, NFS) files.
 */
int
vfs_alloc_hole(rp, space, vaddr, devvp, curblk)
reg_t *rp;
space_t space;
caddr_t vaddr;
struct vnode *devvp;
u_long curblk;
{
    u_int context;
    int error;
    int saved_skeep_state = 0;

    /*
     * If this page is not currently all zeros, then it cannot be a
     * hole.  Thus, we can skip the write, and just return.
     */
    if (!vfs_all_zero(space, vaddr, 1))
	return 0;

    /*
     * We want to write to a hole in a remote memory mapped file.  We
     * must force disk space to be allocated for the hole by writing at
     * least some data to the file at this point.
     *
     * First, set SKEEP to prevent the process from being swapped out
     * while waiting for I/O.
     */
    SPINLOCK_USAV(sched_lock, context);
    if (u.u_procp->p_flag & SKEEP)
	    saved_skeep_state = 1;
    u.u_procp->p_flag |= SKEEP;
    SPINUNLOCK_USAV(sched_lock, context);

    /*
     * The user wants to write to this page.  Write some minimal amount
     * of data back to the remote file to force allocation of file
     * space.  We only need to write a small amount, since holes are
     * always at least one filesystem block in size.
     */
#ifdef FSD_KI
    {
    struct buf *bp;
    bp = bswalloc();
    bp->b_bptype = B_vfs_alloc_hole|B_pagebf;
    bp->b_rp = rp;
    asyncpageio(bp, (swblk_t)curblk, space, vaddr, DEV_BSIZE, B_WRITE, devvp);
    error = waitforpageio(bp);
    bswfree(bp);
    }
#else  /* ! FSD_KI */
    error = syncpageio((swblk_t)curblk, space, vaddr, DEV_BSIZE,
		       B_WRITE, devvp);
#endif /* ! FSD_KI */

    SPINLOCK_USAV(sched_lock, context);
    if (!saved_skeep_state)
	    u.u_procp->p_flag &= ~SKEEP;
    SPINUNLOCK_USAV(sched_lock, context);

    return error;
}

/*
 * vfs_mark_dbds() --
 *     change the dbd_type field of all of the dbds for the block
 *     containing the page indexed by pgindx to DBD_FSTORE and set
 *     all of the dbd_data fields to the correct block numbers.
 *     This function is used for remote files (NFS, DUX) only.
 */
void
vfs_mark_dbds(rp, pgindx, bsize, blkno)
reg_t *rp;
int pgindx;
int bsize;
u_long blkno;
{
    u_long off = vnodindx(rp, pgindx);
    int count = bsize / NBPG;
    int rem = off % bsize;
    int i;

    VASSERT(bsize % NBPG == 0);
    VASSERT(rem % NBPG == 0);

    pgindx -= btop(rem);
    blkno -= btodb(rem);

    /*
     * This region could start in mid-block.  If so, pgindx could be
     * less than 0, so we adjust pgindx and blkno back up so that
     * pgindx is 0.
     */
    if (pgindx < 0) {
	rem = 0 - pgindx;
	pgindx = 0;
	count -= rem;
	blkno += btodb(ptob(rem));
    }

    /*
     * Mark all of the dbds for this chunk with DBD_FSTORE
     * and block number.
     */
    for (i = 0; i < count && pgindx < rp->r_pgsz; i++) {
	dbd_t *dbd = (dbd_t *)FINDDBD(rp, pgindx);

	if (dbd->dbd_data == DBD_DINVAL || dbd->dbd_data == blkno) {
	    dbd->dbd_type = DBD_FSTORE;
	    dbd->dbd_data = blkno;
	}
	else {
	    /*
	     * The region may have changed while we had it unlocked.
	     * Skip changing the dbds, as the new data is correct.
	     */
	    return;
	}

	pgindx++;
	blkno += btodb(NBPG);
    }
}

#ifdef OSDEBUG
	/* Number of times we had to sleep to get memory in vfs_pagein.
	 * If this number is high, may want to keep memory that we get.
	 */
	int vfs_pagein_memreserve = 0;
#endif /* OSDEBUG */

/*ARGSUSED*/
int
vfs_pagein(prp, wrt, space, vaddr, ret_startindex)
preg_t *prp;
int wrt;
space_t space;
caddr_t vaddr;
int *ret_startindex;
{
    void memunreserve();
    dbd_t *dbd2;
    vfd_t *vfd2;
    register dbd_t *dbd;
    register vfd_t *vfd;
    int startindex;
    int pgindx = *ret_startindex;
    reg_t *rp = prp->p_reg;
    struct vnode *vp;
    struct vnode *devvp;
    struct inode *ip;
    pfd_t *pfd;
    int maxpagein, i, j, count, start_blk;
    caddr_t nvaddr;
    space_t nspace;
    int bsize;
    int error;
    u_long isize;
    vm_sema_state;		/* semaphore save state */
    u_int context;
    int pfn[VFS_MAXPGIN];
    int file_is_remote;
    int mmf;			/* writable memory mapped file */
    int retval = 0;
    u_long ok_dbd_limit; 	/* last dbd that we can trust */
    int change_to_fstore = 0;	/* need to change dbds to DBD_FSTORE */
    int saved_skeep_state = 0;
    int bpages;			/* number of pages per block */
    int flush_start_blk = 0;
    int flush_end_blk = 0;

    vmemp_lockx();		/* lock down VM empire */

    /*
     * Find vfd and dbd as mapped by region
     */
    FINDENTRY(rp, pgindx, &vfd2, &dbd2);
    vfd = vfd2;
    dbd = dbd2;
    VASSERT(dbd->dbd_type != DBD_NONE);

    /*
     * Which vnode did I fault for?
     *
     * Note:  Code in hdl_pfault (S800) depends on the algorithm
     * used here for hashing pages in the page cache.  This
     * should probably be a future VOP function so that the
     * details aren't exported to that routine.
     */
    mmf = (rp->r_fstore == rp->r_bstore);
    if (dbd->dbd_type == DBD_FSTORE || dbd->dbd_type == DBD_HOLE)
	vp = rp->r_fstore;
    else
	vp = rp->r_bstore;

    /*
     * Get the devvp and block size for this vnode type
     */
    switch (vp->v_fstype) {
    case VUFS:
	ip = VTOI(vp);
	devvp = ip->i_devvp;
	bsize = ip->i_fs->fs_bsize;
	file_is_remote = 0;
	break;

    case VNFS:
	devvp = vp;
	bsize = vtoblksz(vp);
	bsize &= ~(DEV_BSIZE - 1);
	if (bsize <= 0 )
	    panic("vfs_pagein: zero size");
	file_is_remote = 1;

	/*
	 * Gross stuff for NFS.  See comments at the beginning of this
	 * file for details.
	 */
	if (freemem < NFS_PAGEIN_MEM) {
	    extern int minsleepmem;
	    extern int memory_sleepers;

	    regrele(rp);
	    do {
		int s;

		s = spl6();
		memory_sleepers = 1;
		if (minsleepmem > NFS_PAGEIN_MEM)
		    minsleepmem = NFS_PAGEIN_MEM;
		sleep(&memory_sleepers, PSWP+2);
		splx(s);
	    } while (freemem < NFS_PAGEIN_MEM);

	    /*
	     * Okay, we have enough free memory to allow this NFS
	     * pagein.  Re-lock the region and then check that:
	     *   1.  The region is not a zombie.
	     *   2.  The page is still not valid.
	     */
	    reglock(rp);
	    if (rp->r_zomb)
		vmemp_returnx(0);
	    FINDENTRY(rp, pgindx, &vfd2, &dbd2);
	    vfd = vfd2;
	    if (vfd->pgm.pg_v)
		vmemp_returnx(0);

	    /*
	     * Even if the page is not valid, it could be that it
	     * has been paged in and then paged back out again.
	     * For pages in modifiable regions, the dbd type may
	     * no longer be front store, in which case we want to
	     * restart the entire fault process.
	     */
	    dbd = dbd2;
	    VASSERT(dbd->dbd_type != DBD_NONE);
	    if (dbd->dbd_type != DBD_FSTORE && dbd->dbd_type != DBD_HOLE)
		vmemp_returnx(0);
	}
	break;

    case VDUX:
	ip = VTOI(vp);
	devvp = vp;
	bsize = ip->i_dfs->dfs_bsize;
	file_is_remote = 1;
	break;

    default:
	printf ("dbd_type = %d, vp = %x, v_fstype = %x\n",
		dbd->dbd_type, vp, vp->v_fstype);
	panic("vfs_pagein: unsupported file system type");
    }

    bpages = btop(bsize);
    VASSERT(bpages > 0);

retry:		/* Come here if we have to release the region lock before
		 * locking pages.  This can happen in memreserve() and
		 * blkflush().
		 */

    /*
     * For remote files, we want to check to see if the file has shrunk.
     * If so, we should invalidate any pages past the end.  In the name
     * of efficiency, we only do this if the page we want to fault is
     * past the end of the file.
     */
    if (file_is_remote) {
	struct vattr va;

	if (VOP_GETATTR(vp, &va, u.u_cred, VIFSYNC) != 0) {
	    rp->r_zomb = 1;
	    vmemp_returnx(0);
	}

	isize = va.va_size;
	if (vnodindx(rp, pgindx) >= isize) {
	    /*
	     * The file has shrunk and someone is trying to access a
	     * page past the end of the object.  Shrink the object back
	     * to its currrent size, send a SIGBUS to the faulting
	     * process and return.
	     *
	     * We must release the region lock before calling mtrunc(),
	     * since mtrunc() locks all the regions that are using this
	     * file.
	     */
	    regrele(rp);
	    mtrunc(vp, isize);
	    reglock(rp);
	    vmemp_returnx(-SIGBUS);
	}

	/*
	 * Since remote files handle fragments transparently, we can
	 * always trust cached dbd values.  Set the limit to the size
	 * of the file.  We can ignore rp->r_off, since the smallest
	 * value it can have is zero, and all we want is a value that
	 * is guaranteed to be larger then the last page in the memory
	 * object.
	 */
	ok_dbd_limit = BIG_OK_DBD;
    }
    else {
	isize = ip->i_size;

	if (vnodindx(rp, pgindx) >= isize) {
	    /*
	     * Someone is trying to reference a page beyond the end of
	     * the file.  Send the process a SIGBUS.
	     */
	    vmemp_returnx(-SIGBUS);
	}

	/*
	 * Compute the limit of trustworthy dbds.  dbds for pages
	 * starting at ok_dbd_limit are potentially within a
	 * fragment, which is subject to reallocation.  We must
	 * re-compute the dbd for all such pages.
	 *
	 * Files larger than (NDADDR * bsize) cannot have fragments
	 * so we can cache all dbds past this point.
	 */
	if (isize >= (NDADDR << ip->i_fs->fs_bshift))
	    ok_dbd_limit = BIG_OK_DBD;
	else
	    ok_dbd_limit = btop(isize - isize % bsize) - rp->r_off;
    }

    /*
     * Make sure that the dbd contains a valid block number.
     * We only remap DBD_HOLE entries for local files.  Remote
     * files that are marked DBD_HOLE are handled below.
     *
     * For remote files that are already marked DBD_HOLE, we can
     * skip this entire step (even if dbd_data is DBD_DINVAL).
     */
    if (!(file_is_remote && dbd->dbd_type == DBD_HOLE) &&
	(dbd->dbd_data == DBD_DINVAL ||
	 dbd->dbd_type == DBD_HOLE || pgindx >= ok_dbd_limit)) {
	int flag;

	flag = (wrt && mmf) ? B_WRITE : B_READ;
	*dbd = vfs_mapdbd(pgindx, rp, vp, flag);
	VASSERT(dbd->dbd_type != DBD_NONE);

	if (dbd->dbd_type == DBD_HOLE) {
	    int restart;

	    VASSERT(vp->v_fstype == VUFS);
	    if (flag == B_WRITE) {
		/*
		 * We wanted to write to this page, but failed to get
		 * space for the hole.  Send a SIGBUS to the process.
		 */
		vmemp_returnx(-SIGBUS);
	    }

	    /*
	     * If this is not an MMF, we want to set the dbd type back
	     * to DBD_FSTORE, so that the user will get write access
	     * to the page if they are supposed to.
	     *
	     * If this is a writable shared memory mapped file (i.e. one
	     * where dirty pages are written back to the file), we leave
	     * the type of this page as DBD_HOLE.  A write fault will
	     * cause virtual_fault() to re-fault the page for write (so
	     * that space is allocated on the disk).
	     */
	    if (!mmf) {
		dbd->dbd_type = DBD_FSTORE;
		dbd->dbd_data = DBD_DINVAL;
	    }

	    /*
	     * A hole in the file.  We must allocate a page
	     * and fill it in with zeros.
	     *
	     * Note:
	     *    The block size of the file MUST be greater
	     *    than or equal to the page size, or this
	     *    will not work, because mutilple blocks
	     *    would need to be checked for each page.
	     */
	    VASSERT(bsize >= NBPG);
	    vfd->pgm.pg_cw = 0;
	    VFDMEM_ONE(prp, rp, space, vaddr, pgindx, vfd, restart);
	    if (restart) {
		vmemp_returnx(0);
	    }
	    hdl_user_protect(prp, space, vaddr, 1, &nspace, &nvaddr, 0);
	    hdl_zero_page(nspace, nvaddr, ptob(1));

	    vfd = FINDVFD(rp, pgindx);
	    hdl_unsetbits(vfd->pgm.pg_pfn, VPG_MOD);
	    hdl_user_unprotect(nspace, nvaddr, 1, 0);
	    pfnunlock(vfd->pgm.pg_pfn);

	    if (!file_is_remote && prp->p_type == PT_MMAP)
		imark(ip, IACC);

	    vmemp_returnx(1);
	}

	/*
	 * The page is not necessarily in a hole, however remote files
	 * might have holes.  For the first fault, we assume that the
	 * page is in a hole.  Later on (after the read), we check to
	 * see if the data is really a hole.  If so, we leave the type
	 * DBD_HOLE, otherwise we set the type back to DBD_FSTORE.  For
	 * now, we mark the page as DBD_HOLE.
	 */
	if (file_is_remote && mmf)
	    dbd->dbd_type = DBD_HOLE;
    }

#ifdef OSDEBUG
    /*
     * We should never have DBD_HOLE pages in a non-MMF region.
     */
    if (!mmf)
	VASSERT(dbd->dbd_type != DBD_HOLE);
#endif
    VASSERT(dbd->dbd_type != DBD_NONE);

    /*
     * If the page we want is in memory already, take it
     */
    if (pfd = pageincache(devvp, dbd->dbd_data)) {
#ifdef PFDAT32
	vfd->pgm.pg_pfn = pfd - pfdat;
#else
	vfd->pgm.pg_pfn = pfd->pf_pfn;
#endif
	vfd->pgm.pg_v = 1;
	rp->r_nvalid++;

	/*
	 * This might be a hole in a remote file.  If they want
	 * write access to the page, force allocation for the
	 * hole on the remote system.
	 */
	if (wrt && dbd->dbd_type == DBD_HOLE) {
	    /*
	     * Protect the page from the user while we (possibly)
	     * perform the I/O.
	     */
	    hdl_user_protect(prp, space, vaddr, 1, &nspace, &nvaddr, 0);

	    /*
	     * Now try to allocate disk space for this page.
	     */
	    error = vfs_alloc_hole(rp, nspace, nvaddr, devvp, dbd->dbd_data);

	    /*
	     * The I/O is finished, unprotect the page now.
	     */
	    hdl_user_unprotect(nspace, nvaddr, 1, 0);

	    /*
	     * If some sort of I/O error occurred we generate a
	     * SIGBUS for the process that caused the write,
	     * undo our page locks, etc and return.
	     */
	    if (error || rp->r_zomb) {
		if (PAGEINHASH(pfd))
		    pageremove(pfd);
		pfdatunlock(pfd);
		vmemp_returnx(error ? -SIGBUS : 0);
	    }

	    vfs_mark_dbds(rp, pgindx, bsize, dbd->dbd_data);
	}

	if (!file_is_remote && prp->p_type == PT_MMAP)
	    imark(ip, IACC);

	if (!hdl_cwfault(wrt, prp, pgindx)) {
	    vmemp_returnx(0);    /* free up VM empire and return */
	}
	else {
	    minfo.cache++;
	    cnt.v_pgrec++;
	    cnt.v_pgfrec++;
	    u.u_ru.ru_minflt++;		/* count reclaims */
	    if (prp->p_type == PT_TEXT)
		cnt.v_xifrec++;
	    vmemp_returnx(1);    /* free up VM empire and return */
	}
    }
    u.u_ru.ru_majflt++;		/* It's a real fault, not a reclaim */

    startindex = *ret_startindex;
    maxpagein = pick_maxpagein(prp);
    if (file_is_remote)
	maxpagein = MIN(maxpagein, bpages);

    /*
     * Scan vfds and dbds looking for a run of contiguous pages to load.
     * We do not go past the end of the current pregion nor past the end
     * of the current file.
     */
    {
	int minpage;	/* first page to bring in */
	int maxpage;	/* one past last page to bring in */
	int soft_stop;
	int dbdtype;

	count = 1;
	if (file_is_remote)
	    maxpage = startindex + (bpages - (startindex+rp->r_off) % bpages);
	else {
	    maxpage = startindex + maxpagein/2 + 1;
	    /* attempt to prevent 1 page fragments from forming: */
	    maxpage += (maxpage + rp->r_off) % 2;
	    /* don't pre-fault past ok_dbd_limit */
	    maxpage = MIN(maxpage, MAX(ok_dbd_limit, startindex+1));
	}
	maxpage = MIN(maxpage, prp->p_off + prp->p_count);
	maxpage = MIN(maxpage, btorp(isize) - rp->r_off);
	maxpage = MIN(maxpage, startindex + maxpagein);
	VASSERT(maxpage > startindex);
	/*
	 * Expanding the fault will create calls to FINDENTRY() for new
	 * pages, which will obsolete "dbd", so copy what it points to
	 * and clear it to prevent using stale data.
	 */
	dbdtype = dbd->dbd_type;
	start_blk = dbd->dbd_data;
	dbd = NULL;
	VASSERT(dbdtype != DBD_NONE);
	count = expand_faultin_up(rp, dbdtype, vp, devvp, bpages,
				  maxpage, count, startindex, start_blk);
	soft_stop = maxpage == startindex + count;
	maxpage = startindex + count;
	VASSERT(maxpage <= startindex + maxpagein);
	if (file_is_remote) {
	    minpage = startindex - (startindex+rp->r_off) % bpages;
	    minpage = MAX(minpage, maxpage - maxpagein);
	} else
	    minpage = maxpage - maxpagein;
	VASSERT(startindex >= (long)prp->p_off);
	minpage = MAX(minpage, (long)prp->p_off);
	VASSERT(minpage <= startindex);
	count = expand_faultin_down(rp, dbdtype, vp, devvp, bpages,
				    minpage, count, &startindex, &start_blk);
	if (!file_is_remote && soft_stop && maxpage < startindex + maxpagein) {
	    maxpage = startindex + maxpagein;
	    maxpage = MIN(maxpage, ok_dbd_limit);
	    maxpage = MIN(maxpage, prp->p_off + prp->p_count);
	    maxpage = MIN(maxpage, btorp(isize) - rp->r_off);
	    count = expand_faultin_up(rp, dbdtype, vp, devvp, bpages,
				      maxpage, count, startindex, start_blk);
	}
    }

    VASSERT(maxpagein >= count);

    /*
     * Flush the buffer cache (not necessary with coherent MMF).  blkflush
     * will release the region lock if it has to sleep.  This prevents a
     * deadlock between
     *		a) swapper wanting to lock region to swap in process.
     *		b) process owns buffer and is swapped out.
     *		c) we have region lock and are waiting on the buffer.
     *	Other ways to fix the deadlock (we chose not to do these in IF3)
     *		a) not swap out processes owning critical resources like buffers
     *		b) implement vas_rwip.
     *		c) don't require swapper to aquire the region lock.
     *
     *	We do not need complete coherency so we do not flush data more than
     *	once.  Don't want to fight other processes which are accessing the
     *	same vnode through the file system.
     */
    {
	int flushit = 0;
	int heldmemory = 0;

    	if (flush_start_blk == 0 || flush_start_blk > start_blk) {
		flush_start_blk = start_blk;
		flushit = 1;
    	}
    	if (flush_end_blk < start_blk + count) {
    		flush_end_blk = start_blk + count;
		flushit = 1;
     	}
	/* Note the expression below.  We don't neccesarily want to reserve
	 * memory if we sleep in blkflush.
	 * Reserve memory for the vfdfill below.  We must do this
	 * now to prevent problems when we go to insert the page in the hash.
	 * If we have to sleep to get memory, we give up the memory and
	 * restart.  This assumes that restarting here is an infrequent
	 * occurance.  An alternative would be to keep the memory, and adjust
	 * maxpagein to not exceed this amount.  However all prior exits from
	 * this function must release the memory before returning.
     	 */
    	if ((flushit && blkflush(devvp, (daddr_t)start_blk, ptob(count), 0, rp))
	    || (heldmemory = memreserve(rp, (unsigned int)count))) {
#ifdef OSDEBUG
		vfs_pagein_memreserve++;
#endif /* OSDEBUG */
	 	if (heldmemory)
		    memunreserve((unsigned int)count);

		/*
	 	 * The vfd/dbd pointers could have become stale when
	 	 * we slept without the region lock in memreserve()
	 	 */
		FINDENTRY(rp, pgindx, &vfd2, &dbd2);
		vfd = vfd2;
		if (vfd->pgm.pg_v)
		    vmemp_returnx(1);

		/*
		 * Even if the page is not valid, it could be that it
		 * has been paged in and then paged back out again.
		 * For pages in modifiable regions, the dbd type may
		 * no longer be front store, in which case we want to
		 * restart the entire fault process.
		 */
		dbd = dbd2;
		VASSERT(dbd->dbd_type != DBD_NONE);
		if (dbd->dbd_type != DBD_FSTORE && dbd->dbd_type != DBD_HOLE)
		    vmemp_returnx(0);

		goto retry;
    	}
    }

    /*
     * fill in page tables with real pages.
     */
    vfdfill(startindex, (unsigned)count, prp);

    /*
     * We now have a series of vfds with page frames assigned, whose
     * rightful contents are not in the page cache.  Insert the page
     * frames, and read from disk.
     */
    for (i = 0, j = startindex; i < count; i++, j++) {
	vfd_t *vfdp;
	dbd_t *dbdp;

	FINDENTRY(rp, j, &vfdp, &dbdp);
	VASSERT(vp->v_fstype != VDUX_CDFS);

	pfd = addtocache((int)vfdp->pgm.pg_pfn, devvp, dbdp->dbd_data);
	if (j >= ok_dbd_limit) {
	    /*
	     * Don't cache addresses for data in fragments, as it they
	     * can be asynchronously reallocated on the disc.  Setting
	     * the dbd_data field of the dbd to DBD_DINVAL will force
	     * re-computing the address at the next pageout or pagein.
	     */
	    dbdp->dbd_data = DBD_DINVAL;
	}
#ifdef PFDAT32
	if ((pfd - pfdat) != vfdp->pgm.pg_pfn) {
#else
	if (pfd->pf_pfn != vfdp->pgm.pg_pfn) {
#endif
	    /*
	     * After all the checking above, how is it that we now
	     * find the page in the page cache?  This can happen
	     * because the page is inserted into another region, not
	     * the one we have locked.  We will not share the same
	     * dev,blk pair with another region unless the page is
	     * copy-on-write.  That is why there is the assertion
	     * that the page is not modified.
	     *
	     * Note: we will free the page allocated in vfdfill()
	     *       and do DMA on top of the COW page.  Doing a DMA
	     *       on top of this page should be OK since we know
	     *       that the COW page is an up-to-date copy of the
	     *       disc image.  If we didn't re-do the I/O we
	     *       would need to un_vfdfill() from here through
	     *       the end of the page-in chunk.
	     */
	    /*
	     * This VASSERT has an MP race in it.  We don't
	     * ship with OSDEBUG turned on, and the VASSERT
	     * is probably useful to catch other kinds of
	     * more serious problems, so it stays in.
	     */
#ifdef PFDAT32
	    VASSERT((hdl_getbits((int)(pfd-pfdat)) & VPG_MOD) == 0);
	    freepfd(&pfdat[vfdp->pgm.pg_pfn]);
	    vfdp->pgm.pg_pfn = pfd - pfdat;
	}

	/*
	 * Save pfn.
	 */
	pfn[i] = pfd - pfdat;
#else	
	    VASSERT((hdl_getbits((int)pfd->pf_pfn) & VPG_MOD) == 0);
	    freepfd(&pfdat[vfdp->pgm.pg_pfn]);
	    vfdp->pgm.pg_pfn = pfd->pf_pfn;
	}

	/*
	 * Save pfn.
	 */
	pfn[i] = pfd->pf_pfn;
#endif
    }

    /*
     * Protect the pages from the user during the I/O operation.
     */
    hdl_user_protect(prp, space, vaddr - (pgindx-startindex) * NBPG, count,
			&nspace, &nvaddr, 0);

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
     * Lock the region when the I/O is done.
     */
    regrele(rp);
#ifdef FSD_KI
    {
    struct buf *bp;
    bp = bswalloc();
    bp->b_bptype = B_vfs_pagein|B_pagebf;
    bp->b_rp = rp;
    asyncpageio(bp, (swblk_t)start_blk, nspace, nvaddr,
		       (int)ptob(count), B_READ, devvp);
    error = waitforpageio(bp);
    bswfree(bp);
    }
#else  /* ! FSD_KI */
    error = syncpageio((swblk_t)start_blk, nspace, nvaddr,
		       (int)ptob(count), B_READ, devvp);
#endif /* ! FSD_KI */

    if (rp->r_zomb)
	goto backout;

    if (error) {
	retval = -SIGBUS;
	rp->r_zomb = 1;
	goto backout;
    }

    if (!file_is_remote && prp->p_type == PT_MMAP)
	imark(ip, IACC);

    /*
     * We must detect end-of-file and only read in one page worth.
     * This requires zeroing out the end of the page for mmf's.
     * Not necessary for DUX or NFS because their drivers already
     * guarentee that the data is zero.
     * HP revisit (MMF).
     */
    if (!file_is_remote) {
	int length;
	int chunk_end = vnodindx(rp, startindex + count);

	if (chunk_end > isize) {
	    /*
	     * NOTE: This is currently only for UFS. NFS and DUX should
	     * never enter this section and CDFS has its own pagein()
	     * routine.
	     */
	    VASSERT(vp->v_fstype == VUFS);

	    length = chunk_end - isize;
	    VASSERT(length < NBPG);
	    hdl_zero_page(nspace, nvaddr + ptob(count) - length, length);
	}
    }

    /*
     * For a writable memory mapped file that is remote we must
     * detect potential holes in the file and force allocation of
     * disk space on the remote system.  Unfortunately, there is
     * no easy way to do this, so this gets a little ugly.
     */
    if (file_is_remote && mmf) {
	if (wrt) {
	    if (vfs_all_zero(nspace, nvaddr, count)) {
		/*
		 * The user wants to write to this page.  Write some
		 * minimal amount of data back to the remote file to
		 * force allocation of file space.  We only need to
		 * write a small amount, since holes are always at
		 * least one filesystem block in size.
		 *
		 * Release the region lock while waiting for I/O.
		 * Lock the region when the I/O is done.
		 */
#ifdef FSD_KI
    		{
    		struct buf *bp;
		bp = bswalloc();
		bp->b_bptype = B_vfs_pagein|B_pagebf;
		bp->b_rp = rp;
		asyncpageio(bp, (swblk_t)start_blk, nspace, nvaddr,
				   DEV_BSIZE, B_WRITE, devvp);
		error = waitforpageio(bp);
		bswfree(bp);
		}
#else  /* ! FSD_KI */
		error = syncpageio((swblk_t)start_blk, nspace, nvaddr,
				   DEV_BSIZE, B_WRITE, devvp);
#endif /* ! FSD_KI */

		if (rp->r_zomb)
		    goto backout;

		/*
		 * If some sort of I/O error occurred we generate a
		 * SIGBUS for the process that caused the write,
		 * undo our page locks, etc and return.
		 */
		if (error) {
		    retval = -SIGBUS;
		    goto backout;
		}
	    }

	    /*
	     * Change these dbds to DBD_FSTORE.  We cannot do it here,
	     * since the region must be locked, and it is not locked
	     * at the moment.  We cannot lock the region yet, as we
	     * first have to release the page locks.
	     */
	    change_to_fstore = 1;
	}
    }

    /*
     * Clear the modification bit for all of the pages that we just
     * brought in.  This must be done before hdl_user_unprotect() for
     * MP.
     */
    for (i = 0; i < count; i++)
	hdl_unsetbits(pfn[i], VPG_MOD);

    /*
     * Unprotect the pages.
     */
    hdl_user_unprotect(nspace, nvaddr, count, 0);

    for (i = 0; i < count; i++) {
	/*
	 * Mark the I/O done, and awaken anyone waiting for pfdats
	 */
	pfnunlock(pfn[i]);
    }

    /*
     * Lock the region.
     */
    reglock(rp);

    if (change_to_fstore) {
	/*
	 * Note:  since this only changes one block, it assumes only
	 * one block was faulted in.  Currently this is always true
	 * for remote files, and we only get here for remote files,
	 * so everything is ok.
	 */
	vfs_mark_dbds(rp, startindex, bsize, start_blk);
    }

    SPINLOCK_USAV(sched_lock, context);
    if (!saved_skeep_state)
	    u.u_procp->p_flag &= ~SKEEP;
    SPINUNLOCK_USAV(sched_lock, context);

    cnt.v_exfod += count; /* number of pages from file */
    vmemp_unlockx();	/* free up VM empire */
    *ret_startindex = startindex;
    return count;

backout:

    for (i = 0; i < count; i++) {
	pfd = &pfdat[pfn[i]];
	/*
	 * If error during syncpageio, unhash the page
	 */
	if (PAGEINHASH(pfd))
	    pageremove(pfd);
    }

    hdl_user_unprotect(nspace, nvaddr, count, 1);

    for (i = 0; i < count; i++) {
	pfnunlock(pfn[i]);
    }

    /*
     * Lock the region.
     */
    reglock(rp);

    SPINLOCK_USAV(sched_lock, context);
    if (!saved_skeep_state)
	    u.u_procp->p_flag &= ~SKEEP;
    SPINUNLOCK_USAV(sched_lock, context);

    vmemp_unlockx();	/* free up VM empire */
    return retval;
}

/*
 * Decide how many pages to plan to bring in for this fault.  This function
 * is set up to do a larger fault-in for patterned (e.g. sequential) access
 * than for random access, but the support of the data structures is not
 * here yet, so some of it is a nop.
 */
pick_maxpagein(prp)
    preg_t *prp;
{
    int maxpagein;

    /*
     * How many pages to bring in at once?
     */
    if (prp->p_trend_strength > 1) {
	/* we're doing patterned I/O */
	maxpagein = VFS_MAXPGIN;
    } else
	/* we're doing somewhat random I/O */
	maxpagein = VFS_MAXPGIN/2;

    if (maxpagein >= freemem/2 + 1)
	maxpagein = freemem/2 + 1;

    if (prp->p_trend_strength > 1) {
	/* The pattern may not need all maxpagein pages brought in,
	 * so clip it to the last page in the pattern which is within
	 * maxpagein's of where we are now.
	 */
	VASSERT (prp->p_trend_diff);
	maxpagein -= (maxpagein-1) % prp->p_trend_diff;
    }

    VASSERT (maxpagein > 0 && maxpagein <= VFS_MAXPGIN);

    return maxpagein;
}

/*
 * Take an existing range for a fault-in and try to extend its upper
 * limit.  This is done one page at a time until some limit is hit,
 * like 'maxpage', a block that can't be extended into, or a page
 * that can't be extended into.
 *
 * Return the new size of the fault-in.
 */
int
expand_faultin_up(rp, type, vp, devvp, bpages, maxpage, count,
		  start_pg, start_blk)
    reg_t *rp;
    int type;
    struct vnode *vp, *devvp;
    int bpages;		/* number of pages per fs-block */
    int maxpage;	/* on past limit on max page in fault-in */
    int count;		/* current number of pages in fault-in */
    int start_pg, start_blk;	/* beginning of fault-in */
{
    int page;	/* next page to consider adding to the fault-in */
    int curblk;	/* disc block associated with 'page' */

    VASSERT(type != DBD_NONE);

    page = start_pg + count;
    curblk = start_blk + count * btodb(NBPG);
    while (page < maxpage) {
	if ((page + rp->r_off) % bpages == 0 /* a new block */ &&
			dont_fault_block(rp, page, curblk, vp))
	    break;
	if (dont_fault_page(rp, page, type, curblk, devvp))
	    break;

	++count;
	++page;
	curblk += btodb(NBPG);
    }

    return count;
}

/*
 * Take an existing range for a fault-in and try to extend its lower
 * limit.  This is done one page at a time until some limit is hit,
 * like 'minpage', a block that can't be extended into, or a page
 * that can't be extended into.
 *
 * Return the new size of the fault-in.
 */
int
expand_faultin_down(rp, type, vp, devvp, bpages, minpage, count,
		    start_pg, start_blk)
    reg_t *rp;
    int type;
    struct vnode *vp, *devvp;
    int bpages;		/* number of pages per fs-block */
    int minpage;	/* on past limit on max page in fault-in */
    int count;		/* current number of pages in fault-in */
    int *start_pg, *start_blk;	/* beginning of fault-in */
{
    int page;	/* next page to consider adding to the fault-in */
    int curblk;	/* disc block associated with 'page' */

    VASSERT(type != DBD_NONE);

    page = *start_pg - 1;
    curblk = *start_blk - btodb(NBPG);
    while (page >= minpage) {
	if ((page + rp->r_off + 1) % bpages == 0 /* a new block */ &&
			dont_fault_block(rp, page, curblk, vp))
	    break;
	if (dont_fault_page(rp, page, type, curblk, devvp))
	    break;

	++count;
	--page;
	--*start_pg;
	curblk -= btodb(NBPG);
    }
    *start_blk = curblk + btodb(NBPG);

    return count;
}

/*
 * Return true if this block should not be faulted in as part of the current
 * fault-in being constructed.  Conditions include the new block being
 * in a hole or not being contiguous on the disc with the current
 * fault.
 */
dont_fault_block(rp, page, blk, vp)
    reg_t *rp;
    int page;	/* page in block under consideration */
    int blk;
    struct vnode *vp;
{
    vfd_t *curvfd;
    dbd_t *curdbd;

    FINDENTRY(rp, page, &curvfd, &curdbd);

    if (curdbd->dbd_type != DBD_FSTORE)
	return 1;
    if (curdbd->dbd_data == DBD_DINVAL)
	*curdbd = vfs_mapdbd(page, rp, vp, B_READ);
    if (blk != curdbd->dbd_data)
	return 1;

    return 0;
}

/*
 * Return true if this page should not be faulted in as part of the current
 * fault-in being constructed.  Conditions include the page being a
 * different dbd-type than the rest of the fault, or the page is already in
 * memory.
 *
 * This function also sets up the pages dbd from information (blk)
 * already found out about the block the page is in.  This allows having
 * to do only one vfs_mapdbd() per block instead of per page.
 */
dont_fault_page(rp, page, type, blk, devvp)
    reg_t *rp;
    int page;
    int type;
    int blk;
    struct vnode *devvp;
{
    vfd_t *vfdp;
    dbd_t *dbdp;
    int mmf;

    FINDENTRY(rp, page, &vfdp, &dbdp);

    if (vfdp->pgm.pg_v)
	return 1;

    mmf = (rp->r_fstore == rp->r_bstore);
    if (mmf || dbdp->dbd_type == type) {
	VASSERT(dbdp->dbd_data == DBD_DINVAL || dbdp->dbd_data == blk);
	dbdp->dbd_type = type;
	dbdp->dbd_data = blk;
    }

    /*
     * For MMF we ignore the dbd_type.  It is always either
     * DBD_FSTORE or DBD_HOLE.
     */
    if (mmf)
	VASSERT(dbdp->dbd_type == DBD_FSTORE || dbdp->dbd_type == DBD_HOLE);

    return (!mmf && dbdp->dbd_type != type) || hash_peek(devvp, (u_int)blk);
}

/*
 * vfs_all_zero() --
 *    Examine a range of pages and determine if they are all zero.
 *    Returns 0 if not, 1 if they are.
 */
vfs_all_zero(space, vaddr, count)
space_t space;
caddr_t vaddr;
int count;
{
    int i;
    int all_zero = 1;

    /*
     * See if this chunk is all zeros.
     */
    for (i = 0; all_zero && i < count; i++, vaddr += NBPG) {
	u_long *kaddr;
	u_long *ptr;
	int pfn;
	int j;

	pfn = hdl_vpfn(space, vaddr);
	kaddr = ptr = (u_long *)hdl_kmap(pfn);
	for (j = 0; j < NBPG/sizeof (u_long); j++) {
	    if (*ptr++ != 0) {
		all_zero = 0;
		break;
	    }
	}
	hdl_remap(space, vaddr, pfn);
	hdl_kunmap(kaddr);
    }

    return all_zero;
}
