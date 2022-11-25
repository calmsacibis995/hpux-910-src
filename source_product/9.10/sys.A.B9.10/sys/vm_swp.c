/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_swp.c,v $
 * $Revision: 1.44.83.4 $       $Author: dkm $
 * $State: Exp $	$Locker:  $
 * $Date: 94/05/05 15:29:42 $
 */
/*	MATCHES with 1.36.3.34			*/
/*	vm_swp.c	6.2	83/09/09	*/

#include "../h/debug.h"
#include "../h/types.h"

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/rtprio.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/vas.h"
#include "../h/swap.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/sar.h"
#include "../h/pfdat.h"
#ifdef MP
#include "../h/kern_sem.h"
#endif MP
#include "../dux/lookupops.h"
#ifdef FSD_KI
#include "../h/ki_calls.h"
#endif /* FSD_KI */
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"

int parolemem;  /* number of pages currently scheduled in the I/O sub-system
		    for page-out, which when complete will create free pages */

void report_pageio_error();

/*
 * Swap IO headers -
 * They contain the necessary information for the swap I/O.
 * At any given time, a swap header can be in two
 * different lists. When free it is in the free list,
 * when allocated and the I/O queued, it is on the swap
 * device list.
 */

/*
 * Allocate a swap IO buffer header
 */
struct buf *
bswalloc()
{
	 struct buf *bp;
	 int s;

	 s = spl6();
	 while (bswlist.av_forw == NULL) {
		 bswlist.b_flags |= B_WANTED;
		 sleep((caddr_t)&bswlist, PSWP+1);
	 }
	 bp = bswlist.av_forw;
	 bswlist.av_forw = bp->av_forw;
	 splx(s);

	 /*
	  * Panic if the buffer is still in use.
	  */
	 VASSERT((bp->b_flags&B_BUSY) == 0);

	 /*
	  * Clear all flags.
	  */
	 bp->b_flags = 0;
	 bp->b_resid = 0;
	 bp->b_error = 0;
	 bp->b_vp = NULL;
	 return(bp);
}

/*
 * Free a swap IO buffer header
 */
bswfree(bp)
	 struct buf *bp;
{
	 int s;

	 /* The following VASSERT was removed because there are
	  * several cases in physio() where bswfree() is called
	  * before IO is attempted, so in these cases B_DONE will
	  * never be set, but the bswfree() is still valid.
	  */
	 /* VASSERT(bp->b_flags&B_DONE); */
	 bp->b_flags &= ~B_BUSY;

	 s = spl6();
	 bp->av_forw = bswlist.av_forw;
	 bswlist.av_forw = bp;
	 if (bswlist.b_flags & B_WANTED) {
		 bswlist.b_flags &= ~B_WANTED;
		 wakeup((caddr_t)&bswlist);
	 }
	 splx(s);
}

extern int pageoutcnt;

/*
 * pageiodone is called after asyncpageio completes a pageout.
 * It is its job to free the memory and the buffer associated with
 * the i/o.
 */
pageiodone(bp)
struct buf *bp;
{
	/*
	 * Check for errors during the I/O.  If an error was found, we
	 * report it to the console and set the r_zomb flag in the
	 * region to prevent processes from using the region further.
	 */
	if ((bp->b_flags & B_ERROR) || bp->b_resid) {
		bp->b_rp->r_zomb = 1;
		report_pageio_error(bp, 1);
	}

	pageiocleanup(bp, 1);
}

/*
 * pageiocleanup is called after asyncpageio completes a pageout.
 * If "clean_pages" is non-zero, we clear the modification bit
 * on the pages and free them if B_PAGEOUT is set.  If "clean_pages"
 * is 0, then the pageout has failed, and we will try to page them
 * out again later.  In this case, we do not clear the modification
 * bit nor release the pages.
 */
pageiocleanup(bp, clean_pages)
struct buf *bp;
int clean_pages;
{
	int size = pagesinrange(bp->b_un.b_addr, bp->b_bcount);
	space_t space;
	caddr_t vaddr;
	int pfn;
	register u_int context;
	struct region *rp;
	int all_done = 0;

#ifdef LATER
	syswait.swap--;
#endif LATER

   	space = bp->b_spaddr;
	vaddr = (caddr_t)((u_int)bp->b_un.b_addr &~ (NBPG-1));
	for (; --size >= 0; vaddr += ptob(1)) {
		/*
		 * Get the page number to free.
		 */
		pfn = hdl_vpfn(space, vaddr);
#ifdef PFDAT32
		VASSERT(pfd_is_locked(&pfdat[pfn]));
#else	
		VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif	
		if (clean_pages)
			hdl_unsetbits(pfn, VPG_MOD);
		hdl_user_unprotect(space, vaddr, 1, 0);
		VASSERT(pfdat[pfn].pf_use > 0);
		if (bp->b_flags & B_PAGEOUT) {
			int s;

			if (clean_pages)
				freepfd(&pfdat[pfn]);
			else
				pfnunlock(pfn);

			s = CRIT();
			--parolemem;
			++pageoutcnt;
			UNCRIT(s);
		}
		else
			pfnunlock(pfn);
	}
	VASSERT(parolemem >= 0);

	rp = bp->b_rp;
	bswfree(bp);		/* free the swap buffer */

	/*
	 * decrement page-out-in-progress count
	 */
	context = sleep_lock();

	rp->r_poip--;
	VASSERT(rp->r_poip >= 0);
	if (rp->r_poip == 0)
		all_done = 1;

	sleep_unlock(context);

	if (all_done)
		wakeup((caddr_t)&rp->r_poip);
}

/*
 * start asynchronous paging I/O -
 *
 * We simply initialize the header and queue the I/O but
 * do not wait for completion.  When the I/O completes,
 * biodone() will announce the fact either by waking
 * up sleepers or calling a specified routine.
 */

asyncpageio(bp, dblkno, sp, addr, nbytes, rdflg, vp)
	struct buf *bp;
	swblk_t dblkno;
	space_t	sp;
	caddr_t addr;
	int nbytes, rdflg;
	struct vnode *vp;
{

	VASSERT(bp->b_resid == 0);

	bp->b_flags |= B_BUSY | B_PHYS | rdflg;
	bp->b_proc = (struct proc *) 0;
	bp->b_un.b_addr = addr;
	bp->b_spaddr = sp;
	bp->b_bcount = nbytes;
	bp->b_blkno = dblkno;
#ifdef _WSIO
	bp->b_offset = dblkno << DEV_BSHIFT;
#endif /* _WSIO */
	bp->b_dev = vp->v_rdev;
	bp->b_vp = vp;
	bsetprio(bp);

#ifdef TRACE
	trace(TR_SWAPIO, vp, bp->b_blkno);
#endif
	/* pageiodone() frees pages, which increments freemem.  So if that's
	 * the path out, we are about to schedule I/O which when done,
	 * will create free memory, so increment parolemem.  pageiodone()
	 * will do the corresponding decrement.
	 */
	if ((bp->b_flags & B_CALL) && (bp->b_flags & B_PAGEOUT) &&
	    bp->b_iodone == pageiodone) {
		int s = CRIT();
		parolemem += pagesinrange(bp->b_un.b_addr, bp->b_bcount);
		UNCRIT(s);
	}
#ifdef FSD_KI
	KI_asyncpageio(bp);
#endif /* FSD_KI */

	(*vp->v_op->vn_strategy)(bp);
}

#ifdef __hp9000s300
/*
 * If rout == 0 then killed on swap error, else
 * rout is the name of the routine where we ran out of
 * swap space.
 */
swkill(p, rout)
	struct proc *p;
	char *rout;
{
	char *mesg;

	printf("pid %d: ", p->p_pid);
	if (rout)
		printf(mesg = rout);
	else
		printf(mesg = "killed on swap error\n");
	uprintf("sorry, pid %d was %s", p->p_pid, mesg);
	psignal(p, SIGKILL);
}
#endif /* __hp9000s300 */

int
waitforpageio(bp)
	 struct buf *bp;
{
	 int s;

	s = spl6();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, PSWP);
	splx(s);

	if ((bp->b_flags & B_ERROR) || bp->b_resid) {
		report_pageio_error(bp, 0);
		return(1);
	}
	return(0);
}

#ifndef FSD_KI
int
syncpageio(dblkno, sp, addr, nbytes, rdflg, vp)
	swblk_t dblkno;
	space_t	sp;
	caddr_t addr;
	int nbytes, rdflg;
	struct vnode *vp;
{
	struct buf *bp;
	int error;

	bp = bswalloc();
	asyncpageio(bp, dblkno, sp, addr, nbytes, rdflg, vp);
	error = waitforpageio(bp);
	bswfree(bp);
	return(error);
}
#endif /* FSD_KI */

/*
 * report_pageio_error() --
 *    Report that an error occurred during a pagein/pageout.  We only
 *    print a message if "is_async" is 1, otherwise, the IO was
 *    synchronous, and the process that started the I/O will be informed
 *    of the error through some other means.
 */
void
report_pageio_error(bp, is_async)
	struct buf *bp;
	int is_async;
{
	char *nfs_host = (char *)0;
	char *nfs_path = (char *)0;
	char *preamble;
	struct vnode *vp = bp->b_vp;

#ifdef OSDEBUG
	if (!is_async) {
		is_async = 1;
		printf("Synchronous ");
	}
#endif

	if (vp && vp->v_fstype == VNFS) {
		/*
		 * We do not report synchronous NFS errors to the
		 * console or message buffer.
		 */
		if (!is_async)
		    return;

		/*
		 * Get information about the NFS mount point for
		 * printing.
		 */
		nfs_host = vtomi(vp)->mi_hostname;
#ifdef GETMOUNT
		nfs_path = vtomi(vp)->mi_fsmnt;
#endif
	}

	preamble = "Page I/O error occurred while paging to/from ";
	if (nfs_host != (char *)0) {
		if (nfs_path != (char *)0) {
			printf("%sNFS server %s\nfile system is %s\n",
			    preamble, nfs_host, nfs_path);
		}
		else {
			printf("%sNFS server %s\n",
			    preamble, nfs_host);
		}
	}
	else {
		printf("%sdisk\ndevice 0x%08x, 1K block #%d, %d bytes of %d bytes not paged\n",
		    preamble, bp->b_dev, bp->b_blkno, 
		    bp->b_resid, bp->b_bcount);
	}
}

/*
 * return swap information on file system specified
 */
swapfs()
{
	register struct a {
		char    *fname;
		struct  swapfs_info *swfs_info;
	} *uap;
	struct vnode *vp;
	struct vfs *looking_for_vfsp;
	struct fswdevt *fswp;
	struct swapfs_info swfs_temp;
	long chunks_to_frags;
	int i;

	uap = (struct a *)u.u_ap;

	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP, (caddr_t)0);
	if (u.u_error){
	    return(0);
	}

	looking_for_vfsp = vp->v_vfsp;
	bzero(&swfs_temp, sizeof(struct swapfs_info));

	fswp = fswdevt;
	for (i = 0; i < nswapfs; i++)
	{
	    if ((*fswp->fsw_mntpoint != '\0' ) &&
		(fswp->fsw_vnode->v_vfsp == looking_for_vfsp))
	    {
		chunks_to_frags = (swchunk * DEV_BSIZE) /
		(VTOI(fswp->fsw_vnode))->i_fs->fs_fsize;

		swfs_temp.sw_binuse = fswp->fsw_allocated * chunks_to_frags;
		swfs_temp.sw_bavail = fswp->fsw_limit * chunks_to_frags;
		swfs_temp.sw_breserve = fswp->fsw_reserve;
		swfs_temp.sw_priority = fswp->fsw_priority;
		(void) strcpy(swfs_temp.sw_mntpoint, fswp->fsw_mntpoint);

		update_duxref(vp, -1, 0);
		VN_RELE(vp);
		return (copyout((caddr_t)&swfs_temp,
				(caddr_t)uap->swfs_info, sizeof(swfs_temp)));
	    }
	    fswp++;
	}
	update_duxref(vp, -1, 0);
	VN_RELE(vp);
	u.u_error = ENOENT;
	return(0);
}

sw1_strategy()
{
	panic("sw1_strategy called");
}
