/* $Header: dux_vnops.c,v 1.13.83.7 94/08/19 10:40:29 craig Exp $ */
/* HPUX_ID: @(#)dux_vnops.c	51.2		88/01/20 */

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
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/kernel.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../ufs/fsdir.h"
#include "../dux/dux_dev.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookupmsgs.h"
#include "../h/mount.h"
#include "../dux/dux_hooks.h"	/* Needed for configurability - daveg */
#include "../dux/duxfs.h"
#include "../cdfs/cdfs_hooks.h"

#include "../h/unistd.h"	/* defines of F_ULOCK,etc. */
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA
#include "../h/kern_sem.h"

extern dux_inval();
#define dux_open	dux_inval
extern dux_close();
#define dux_rdwr	ufs_rdwr
extern dux_rdwr();
#define dux_ioctl	ufs_ioctl
extern dux_ioctl();
#define dux_select	ufs_select
extern dux_select();
extern dux_getattr();
extern dux_setattr();
#define dux_access	ufs_access	/*This won't quite work, but for now..*/
extern dux_access();
#define dux_lookup	dux_inval
#ifdef	LOCAL_DISC
extern dux_create();
#else
#define dux_create	dux_inval
#endif	LOCAL_DISC
#define dux_remove	dux_inval
#define dux_link	dux_inval
extern dux_rename();
#ifdef	LOCAL_DISC
extern dux_mkdir();
#else
#define dux_mkdir	dux_inval
#endif	LOCAL_DISC
#define dux_rmdir	dux_inval
#define dux_readlink	ufs_readlink
extern dux_readlink();
extern dux_readdir();
extern dux_symlink();
extern dux_fsync();
#define dux_inactive	ufs_inactive
extern dux_inactive();
extern dux_bmap();
#define dux_bread	dux_inval
#define dux_brelse	dux_inval
extern dux_pathsend();
#ifdef ACLS
extern dux_setacl();
extern dux_getacl();
#endif
#ifdef	POSIX
#define dux_pathconf	dux_inval
extern dux_fpathconf();
#endif	POSIX
extern int dux_lockctl();
extern int dux_clockf();
extern int dux_fid();
extern int dux_fsctl();
extern int vfs_prefill();
extern int vfs_pagein();
extern int vfs_pageout();

/*
** The dux_strategy() routine is replaced by the dux_nop() routine
** for configurability purposes. If the kernel is configured with
** DUX in, then the dux_vnodeops location corresponding to dux_strategy
** is filled in at boot time. - daveg (See dux_hooks.c)
*/
extern dux_nop();

struct vnodeops dux_vnodeops =
{
	dux_open,
	dux_close,
	dux_rdwr,
	dux_ioctl,
	dux_select,
	dux_getattr,
	dux_setattr,
	dux_access,
	dux_lookup,
	dux_create,
	dux_remove,
	dux_link,
	dux_rename,
	dux_mkdir,
	dux_rmdir,
	dux_readdir,
	dux_symlink,
	dux_readlink,
	dux_fsync,
	dux_inactive,
	dux_bmap,
	dux_nop,
	dux_bread,
	dux_brelse,
	dux_pathsend,
#ifdef ACLS
	dux_setacl,
	dux_getacl,
#endif
#ifdef	POSIX
	dux_pathconf,
	dux_fpathconf,
#endif	POSIX
	dux_lockctl,
	dux_clockf,	/* client lockf */
	dux_fid,	/* VOP_FID() */
	dux_fsctl,
	vfs_prefill,
	vfs_pagein,
	vfs_pageout,
	NULL,
	NULL,
};

dux_fsctl()
{
	return(EINVAL);
}

dux_inval()
{
	panic ("Unimplemented dux vnode operation");
}

/* READ/WRITE CALLS */

/*ARGSUSED*/
int
dux_rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	struct vnode *vp;
	struct buf *bp;
	struct duxfs *dfs;
	daddr_t lbn, bn, rabn;
	register int n, on, type;
	int size, rabsize;
	long bsize;
	long bshift;
	long bmask;
	int total;
	int syncio_flag;

	if (uio->uio_resid == 0)
		return (0);
	type = ip->i_mode&IFMT;
	/* If the serving site has died, return an error */
	if (devsite(ip->i_dev) == 0)
		return (DM_CANNOT_DELIVER);
	/* If this is a fifo, use fifo I/O */
	if (type == IFIFO)
	{
		if (rw == UIO_READ)
			return (dux_fifo_read(ip, uio, ioflag));
		else
			return (dux_fifo_write(ip, uio, ioflag));
	}
	/* If synchronous I/O is necessary, call syncio to perform it. */
	if (ip->i_flag & ISYNC || type == IFDIR || type == IFNWK)
	{
		return (syncio(ip, uio, rw, ioflag));
	}
	vp = ITOV(ip);
	/*make sure incore buffers are valid*/
	if (!(ip->i_flag&IBUFVALID))
	{
		invalip(ip);
		ip->i_flag |= IBUFVALID;
	}

	syncio_flag = ((rw == UIO_WRITE ? 
	    uio->uio_fpflags & FSYNCIO:0) | (ioflag & IO_SYNC));
	if (syncio_flag)
		ip->i_flag |= IFRAGSYNC;
	total = uio->uio_resid;
	dfs = ip->i_dfs;
	bsize = dfs->dfs_bsize;
	bmask = ~dfs->dfs_bmask;
	bshift = dfs->dfs_bshift;
	VASSERT(1<<bshift == bsize);
	VASSERT(bmask + 1 == bsize);
	u.u_error = 0;
	do {
		lbn = uio->uio_offset >> bshift;
		on = uio->uio_offset & bmask;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		if (rw == UIO_READ) {
			int diff = ip->i_size - uio->uio_offset;
			if (diff <= 0) {
				/*
				 * DSDe400736 (POSIX): Need to mark access time
				 * even for sucessful read at EOF.  Special
				 * case this to preserve optimization where
				 * IACC is touched only when B_CACHE information
				 * is accessed; otherwise the server already
				 * has the correct access time.
				 */
				imark(ip, IACC);
				return (0);
			}
			if (diff < n)
				n = diff;
		}
		else if (rw == UIO_WRITE && type == IFREG) {
			if (((uio->uio_offset + n - 1) >> 9) >=
			u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
				n = ((u.u_rlimit[RLIMIT_FSIZE].rlim_cur << 9)
				- uio->uio_offset);
				if (n <= 0)
					if (total == uio->uio_resid)
						return(EFBIG);
					else {
						return(u.u_error);
					}
			}
		}
		dux_bmap(vp, lbn, NULL, &bn);
		if (rw == UIO_WRITE &&
		   (uio->uio_offset + n > ip->i_size))
		{
			u.u_error = dux_grow(ip, uio->uio_offset + n);
			if (u.u_error)
				return(u.u_error);
		}
		size = dux_blksize(dfs, ip, lbn);
		if (rw == UIO_READ) {
			if (ip->i_lastr + 1 == lbn &&
				(lbn+1)<<dfs->dfs_bshift < ip->i_size) {
				
				dux_bmap(vp, lbn+1, NULL, &rabn);
				rabsize = dux_blksize(dfs, ip, lbn+1);
#ifdef	FSD_KI
				bp = breada(vp, bn, size, rabn, rabsize, 
					B_dux_rwip|B_data);
#else	FSD_KI
				bp = breada(vp, bn, size, rabn,
					rabsize);
#endif	FSD_KI
			}
			else
#ifdef	FSD_KI
				bp = bread(vp, bn, size,
					B_dux_rwip|B_data);
#else	FSD_KI
				bp = bread(vp, bn, size);
#endif	FSD_KI
			if (bp->b_flags & B_CACHE)
				imark(ip, IACC);
			ip->i_lastr = lbn;
		} else {
			/* In normal files, this test reads "if (n == bsize)"
			 * For DUX files we use size instead, so as to
			 * match on partial blocks.
			 */
			if (n == size) 
#ifdef	FSD_KI
				bp = getblk(vp, bn, size, 
					B_dux_rwip|B_data);
#else	FSD_KI
				bp = getblk(vp, bn, size);
#endif	FSD_KI
			else
#ifdef	FSD_KI
				bp = bread(vp, bn, size, 
					B_dux_rwip|B_data);
#else	FSD_KI
				bp = bread(vp, bn, size);
#endif	FSD_KI
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
#ifdef QUOTA
			if ((syncio_flag) ||
			    (ip->i_mount != NULL &&
			    (ip->i_mount->m_flag & M_IS_SYNC)) 
                            || (ip->i_dquot && ip->i_dquot->dq_flags & 
                                (DQ_HARD_BLKS | DQ_TIME_BLKS | DQ_SOFT_BLKS)))
			{
#else not QUOTA
			if ((syncio_flag) ||
				(ip->i_mount != NULL &&
				(ip->i_mount->m_flag & M_IS_SYNC)))
			{
#endif QUOTA
				bwrite(bp);
				if (u.u_error == ENOSPC)
					uprintf("\n%s: write failed, file system is full\n",
						ip->i_dfs->dfs_fsmnt);
#ifdef QUOTA
                                else if (ip->i_dquot && 
					(ip->i_dquot->dq_uid != 0))
                                {
                                   if ((u.u_error == EDQUOT)
	                              && (ip->i_vnode.v_fstype == VDUX)
                                      )
   				   {
                                      if ((ip->i_dquot->dq_flags & 
                                           DQ_HARD_BLKS)
                                          && ip->i_uid == u.u_ruid)
				      {
                                         uprintf("\nDISK LIMIT REACHED (%s) - WRITE FAILED\n",
                                         ip->i_dfs->dfs_fsmnt);
				      }
                                      else 
                                      if ((ip->i_dquot->dq_flags &
                                           DQ_TIME_BLKS)
                                          && ip->i_uid == u.u_ruid)
				      {
        	                         uprintf("\nOVER DISK QUOTA: (%s) NO MORE DISK SPACE\n",
		                         ip->i_dfs->dfs_fsmnt);
                                      }
                                   }       
                                   else  /* no error but may need a warning */
                                   if ((ip->i_dquot->dq_flags &
                                           DQ_SOFT_BLKS)
                                          && (time.tv_sec - 
                                              ip->i_dquot->dq_btimelimit > 
                                              DQ_MSG_TIME)
                                          && ip->i_uid == u.u_ruid)
			           {
                                      uprintf("\nWARNING: disk quota (%s) exceeded\n",
                                      ip->i_dfs->dfs_fsmnt);
                                      ip->i_dquot->dq_btimelimit = 
                                                          time.tv_sec;
                                   }
                                   /* msg flags cleared in duxint if under */
                                }    
#endif QUOTA
			}
			else if (n + on == bsize && !(bp->b_flags & B_REWRITE)) {
				bp->b_flags |= B_REWRITE;
				bawrite(bp);
			} else
				bdwrite(bp);

		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
	return (u.u_error);
}


dux_syncip(ip)
register struct inode *ip;
{
	register long lbn, lastlbn;
	int s;
	daddr_t blkno;
	register struct duxfs *dfs;
	register struct vnode *vp;

	dfs = ip->i_dfs;
	if (dfs == NULL)	/*only possible if serving site died*/
		return;
	lastlbn = howmany(ip->i_size, dfs->dfs_bsize);
	vp = ITOV(ip);
	/* If we have only a few blocks, seek them out */
	if (lastlbn < nbuf/2)
	{
		for (lbn = 0; lbn < lastlbn; lbn ++)
		{
			dux_bmap(vp, lbn, NULL, &blkno);
			blkflush(vp, blkno, dfs->dfs_bsize, 0, NULL);
		}
	}
	else
	/* If the file is big, just search the whole cache */
	{
		dux_flush_cache (vp);
	}
}

/*
 * Invalidate all the blocks associated with an inode.  Only do this if
 * the file is remote.  If it is local, return.
 */
invalip(ip)
	register struct inode *ip;
{
	register struct vnode *vp;

	if (remoteip(ip))
	{
		vp = ITOV(ip);
		binval(vp);
	}
}


/*
 * An inode is growing because of a write.  The following steps must be done:
 *
 *	1)  If the last block of the current inode is in core, and is not
 *	complete, extend it to either the end of the block or the new end of
 *	the inode.  This gurarantees that only the last block of the inode
 *	is ever partial.
 *
 *	2)  Change the inode size.
 *
 *	3)  Call iupdat if necessary
 */
dux_grow(ip, newsize)
register struct inode *ip;
int newsize;
{
	daddr_t lastlbn, lastbn;
	register struct vnode *vp;
	register struct duxfs *dfs;
	int bsize;
	int osize;
	register struct buf *bp;
#ifdef notdef
	int error;
#endif /* notdef */

	dfs = ip->i_dfs;
	/*Compute the size of the last block of the inode*/
	osize = duxblkoff(dfs, ip->i_size);
	/* If the last block is a full block, we don't need to extend it */
	if (osize != 0)
	{
		vp = ITOV(ip);
		/*Find the last block of the inode*/
		lastlbn = duxlblkno(dfs, ip->i_size);
		dux_bmap(vp, lastlbn, NULL, &lastbn);
		/*Is the block in core?*/
		if (incore(vp, lastbn))
		{
			int old_size;

			/*compute the new block size which is the minimum of:
			 *	The fs block size
			 *	The remainder of the file
			 */
			bsize = newsize - (lastlbn << dfs->dfs_bshift);
			if (bsize > dfs->dfs_bsize)
				bsize = dfs->dfs_bsize;
			/* reallocate the block.  This code is duplicated
			 * from realloccg.  Note that the bread should
			 * never really bread since we know the buffer
			 * is incore
			 */
			do
			{
#ifdef	FSD_KI
				bp = bread (vp, lastbn, osize, 
					B_dux_grow|B_data);
#else	FSD_KI
				bp = bread (vp, lastbn, osize);
#endif	FSD_KI
				/*
				 * The following does not seem necessary;
				 * since we know this buffer to be
				 * in core, we should never get an error.
				 */
#ifdef notdef
				if (bp->b_flags & B_ERROR)
				{
					error = geterror(bp);
					brelse(bp);
					return (error);
				}
#endif
				old_size = bp->b_bufsize;
			} while (brealloc(bp, bsize) == 0);
			/* maintain accurate DUX quotas */
                        if (  (old_size != bp->b_bufsize)
			    &&(bp->b_flags & B_DUX_REM_REQ)) {
                                dux_buf_size(bp->b_bufsize - old_size);
                        }
			bp->b_flags |= B_DONE;
			bzero(bp->b_un.b_addr+osize, (unsigned)bsize - osize);
			brelse(bp);
		}
	}
	ip->i_size = newsize;
	return (0);
}

/*
 *BMAP
 *
 * The conversion is very simple.  The vnode mapping is a null operation,
 * returning the vnode passed in.  The block # mapping is simply a conversion
 * from the logical block size (dfs_bsize), to DEV_BSIZE
 */
dux_bmap(vp, lbn, vpp, bnp)
struct vnode *vp;	/*file's vnode*/
daddr_t lbn;		/*logical block #*/
struct vnode **vpp;	/*return vp of file*/
daddr_t *bnp;		/*Return block #*/
{
	struct inode *ip = VTOI(vp);

	if (vpp)
		*vpp = vp;

	/* if devsite == 0, server site has died and the inode will
	 * have been cleaned up by ino_cleanup.  Reference to the i_dfs
	 * field below would cause a segmentation violation.  mry 1/6/91
	 */
	if (bnp && (devsite(ip->i_dev) != 0))
		*bnp = (lbn * ip->i_dfs->dfs_bsize) / DEV_BSIZE;
	return (0);
}

/* CLOSE CALL */

/*ARGSUSED*/
int
dux_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
{
	register struct inode *ip = VTOI(vp);

	/* If the serving site has died, references to inode fields
	 * which were cleaned up by ino_cleanup may result in 
	 * segmentation violations, so return an error.   mry 1/6/91
	 */
	if (devsite(ip->i_dev) == 0)
		return (DM_CANNOT_DELIVER);

	/*
	 * Make sure that all I/O has been flushed to server before
	 * closing file.
	 */
	switch(ip->i_mode & IFMT)
	{
	case IFIFO:
		/*
		 * This is not necessary for fifos because closep will
		 * invalidate all file system buffers associated with
		 * the fifo.
		 */
		break;
	case IFREG:
		/*
		 * Tuning for case of frequent close of read-only files
		 * (like /lib/libc.sl & /lib/dld.sl).
		 *
		 * Dont post buffers that this proc could NOT of
		 * written.  Let the proc that wrote them, post them
		 * at their close.
		 *
		 * For regular files that are open read-only, there is
		 * no need to search for buffers that are marked
		 * B_DELWRI.  The only ones that we would ever find
		 * would be associated with some other open() -- when
		 * that one gets closed, we will flush the buffers out.
		 * There is no need to flush the buffers before this
		 * point (in fact it hurts write performance for the
		 * other guy).
		 *
		 * This is a MAJOR performance enhancement that has a
		 * very large impact on read performance of regular
		 * files and especially shared libraries.
		 */
		if (((flag & FWRITE) == 0) && (getsitecount(&ip->i_opensites,
				u.u_site) > 1))
			break;
		/* fall thru */
	default:
	    {
		ilock(ip);
		dux_flush_cache (vp);
		iunlock(ip);
	    }
	}
	dux_closei(ip, flag);
	close_send(ip, flag);
	return (0);
}

/*
 *Send a close message.
 *This is separated out from closing devices, to permit calling of this
 *function if a device open fails.
 */
close_send(ip, flag)
	register struct inode *ip;
	int flag;
{
	dm_message request, reply;
	register struct closereq *rp;
	int error;
	register long rversion;

	ilock(ip);
	/* Keep track of vnodes we have open that are regular files.  This
	 * is so we know to flush on the last close.
	 */
	if (ITOV(ip)->v_type == VREG) {
		updatesitecount (&(ip->i_opensites), u.u_site, -1);
	}
	request = dm_alloc(sizeof (struct closereq), WAIT);
	rp = DM_CONVERT(request, struct closereq);
	rp->dev = ip->i_dev;
	rp->inumber = ip->i_number;
	rp->fstype = MOUNT_UFS;
	rp->flag = flag;
	rp->timeflag = ip->i_flag & (ICHG|IUPD|IACC);
	if (rp->timeflag != 0) {
		rp->atime.tv_sec = ip->i_atime.tv_sec;
		rp->ctime.tv_sec = ip->i_ctime.tv_sec;
		rp->mtime.tv_sec = ip->i_mtime.tv_sec;
		ip->i_flag &= ~(IUPD|IACC|ICHG);
	}
	iunlock(ip);
	reply = dm_send (request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_CLOSE, devsite (ip->i_dev), sizeof (struct closerepl),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	if (!error && !(ip->i_flag & ISYNC))
	{	/*don't update version if we have been doing sync io*/
		rversion = (DM_CONVERT(reply, struct closerepl))->version;
		if (rversion != ip->i_fversion)
		{
			ip->i_fversion = rversion;
			/*If we've changed the file any executable pages
			 *are invalid.  However buffer pages are not invalid
			 *since all the changes were made here.
			 */
			ip->i_flag &= ~IPAGEVALID;
		}
	}
	dm_release(reply,0);
}

/* close an inode on the server when the client's inode table */
/* is full */
close_send_no_ino(dev, inum, flag)
	dev_t dev;
	ino_t inum;
	int flag;
{
	dm_message request, reply;
	register struct closereq *rp;

	request = dm_alloc(sizeof(struct closereq), WAIT);
	rp = DM_CONVERT(request, struct closereq);
	rp->fstype = MOUNT_UFS;
	rp->dev = dev;
	rp->inumber = inum;
	rp->flag = flag;
	rp->timeflag = 0;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_CLOSE, devsite(dev), sizeof(struct closerepl),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	dm_release(reply, 0);
}

/* Close the devices */
dux_closei(ip, flag)
	struct inode *ip;
	int flag;
{

	/*close the device if any*/
	if (setjmp(&u.u_qsave)){
		PSEMA(&filesys_sema);
		return (EINTR);
	}
	switch (ip->i_mode & IFMT)
	{
	case IFBLK:
#ifdef FULLDUX
		closed(ip->i_rdev, IFBLK, flag, my_site);
#else
		closed(ip->i_rdev, IFBLK, flag);
#endif
		break;
	case IFCHR:
#ifdef FULLDUX
		closed(ip->i_rdev, IFCHR, flag, my_site);
#else
		closed(ip->i_rdev, IFCHR, flag);
#endif
		break;
	case IFIFO:
		closep (ip, flag);
		break;
	}
	return (0);
}

close_serve(request)
	dm_message request;
{
	struct closereq *rp = DM_CONVERT(request, struct closereq);
	if (rp->fstype == MOUNT_UFS) {
		closei_serve(rp);
	}
	else if (rp->fstype == MOUNT_CDFS) {
		CDFSCALL(CLOSECD_SERVE)(rp);
	}
	else panic("close_serve: illegal fs type");
}
closei_serve(rp)
struct closereq *rp;
{
	dm_message reply;
	register struct inode *ip;
	int is_fifo;
	int flag = 0;

	ip = ifind (localdev(rp->dev), rp->inumber);
	if (ip == NULL)	/*can't happen*/
	{
		printf("ifind failed:  close_serve\n");
		dm_quick_reply (ENOENT);
		return;
	}
	reply = dm_alloc(sizeof (struct closerepl), WAIT);
	unlock(ip);
	/*
	 * Desynchronize the inode if necessary
	 */
	isynclock(ip);
	is_fifo = (ip->i_mode&IFMT) == IFIFO;

	updatesitecount (&(ip->i_opensites), u.u_site, -1);
	if (rp->flag & FWRITE)
	{
		updatesitecount (&(ip->i_writesites), u.u_site, -1);
		ip->i_fversion++;
		ip->i_flag |= ICHG;
	}
	if (is_fifo && (rp->flag & FREAD))
	{
		updatesitecount (&(ip->i_fifordsites), u.u_site, -1);
	}
	checksync(ip);
	isyncunlock(ip);

	ilock(ip);

	if (rp->timeflag) {
		if (rp->timeflag & IACC) {
			if (rp->atime.tv_sec > ip->i_atime.tv_sec) {
				ip->i_atime.tv_sec = rp->atime.tv_sec;
				flag |= IACC;
			}
		}
		if (rp->timeflag & ICHG) {
			if (rp->ctime.tv_sec > ip->i_ctime.tv_sec) {
				ip->i_ctime.tv_sec = rp->ctime.tv_sec;
				flag |= ICHG;
			}
		}
		if (rp->timeflag & IUPD) {
			if (rp->mtime.tv_sec > ip->i_mtime.tv_sec) {
				ip->i_mtime.tv_sec = rp->mtime.tv_sec;
				flag |= IUPD;
			}
		}
		ip->i_flag |= flag;
	}

	/* If this is a fifo, we must close it. */
	if (is_fifo)
		closep(ip, rp->flag);
	else if ((rp->flag & FWRITE) && ((ip->i_mode&IFMT) == IFREG) &&
		 (ip->i_size) && (ip->i_nlink)) 
		syncip(ip, 0, 1);
	else
		iupdat(ip, 0, 1);

	/* If we are serving a remote site for write to a file,
	 * this site may be eliminated from the bookkeeping for its disc
	 */
	if ((rp->flag & FWRITE) && (ITOV(ip)->v_type == VREG))
		mdev_update(ip, u.u_site, -1);
	iput(ip);
	(DM_CONVERT(reply, struct closerepl)->version = ip->i_fversion);
	dm_reply (reply, 0, 0, NULL, NULL, NULL);
}

/* SET AND GET ATTRIBUTE CALLS */

/* setattr request */
struct setattr_req		/*DUX MESSAGE STRUCTURE*/
{
	struct vattr vattr;
	struct packed_ucred cred;
	dev_t dev;
	ino_t ino;
};

/* settattr reply is just error code */

/* getattr request */
struct getattr_req
{
	struct packed_ucred cred;
	dev_t dev;
	ino_t ino;
	cdno_t cdno;
	int   fstype;
};

/* getattr reply is struct vattr */

int
dux_setattr(vp, vap, cred)
register struct vnode *vp;
register struct vattr *vap;
struct ucred *cred;
{
	register struct inode *ip;
	dm_message request;
	register struct setattr_req *sp;
	dm_message reply;
	int error;
	u_long orig_size;

	ip = VTOI(vp);
	request = dm_alloc(sizeof(struct setattr_req),WAIT);
	sp = DM_CONVERT(request, struct setattr_req);
	pack_ucred(cred, &sp->cred);
	sp->vattr = *vap;
	sp->dev = ip->i_dev;
	sp->ino = ip->i_number;
	orig_size = ip->i_size;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST, DM_SETATTR,
		devsite(ip->i_dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	dm_release(reply,0);

	/*
	 * If request succeeded and it is reducing the size of a file
	 * that is memory mapped, we want to call mtrunc() to make sure
	 * that any pages past the end of the file are invalidated.
	 */
	if (error == 0 && (vp->v_flag & VMMF) &&
	    vap->va_size != -1 && vap->va_size < orig_size) {
		mtrunc(vp, vap->va_size);
	}
	return (error);
}

dux_setattr_serve(request)
dm_message request;
{
	register struct setattr_req *sp =
		DM_CONVERT(request, struct setattr_req);
	register struct inode *ip;
	register struct vnode *vp;
	struct ucred ucred;

	ip = ifind(localdev(sp->dev), sp->ino);
	if (ip == NULL)	/*shouldn't happen*/
	{
		printf("ifind failed:  dux_setattr_serve\n");
		dm_quick_reply (ENOENT);
		return;
	}
	vp = ITOV(ip);
	unpack_ucred(&sp->cred, &ucred);
	/*kludge:
	 *since ufs_setattr calls suser, we also need to put the ucred in
	 *the u area
	 */
	u.u_cred = &ucred;
	dm_quick_reply (VOP_SETATTR(vp,&sp->vattr,&ucred, 0));
	/* this is a kludge !!! */
	restore_ulookup();	/*to restore the ucred*/
}

int
dux_getattr(vp, vap, cred, vsync)
register struct vnode *vp;
register struct vattr *vap;
struct ucred *cred;
enum vsync vsync;
{
	register struct inode *ip;
	dm_message request;
	register struct getattr_req *gp;
	dm_message reply;
	int error;
	register struct cdnode *cdp;
	register int	fstype;
	site_t	site;

	if (vp->v_fstype == VDUX) {
		ip = VTOI(vp);

		/* If the serving site has died, references to inode fields
		 * which were cleaned up by ino_cleanup may result in 
		 * segmentation violations, so return an error.   mry 1/6/91
		 */
		if (devsite(ip->i_dev) == 0)
			return (DM_CANNOT_DELIVER);

		fstype = MOUNT_UFS;
		if (vsync == VASYNC || 
		    (vsync == VIFSYNC && !(ip->i_flag & ISYNC)))
		{	/*can request be executed locally?*/
			return (ufs_getattr(vp, vap, cred, VASYNC));
		}
		site = devsite(ip->i_dev);
	}
	else if (vp->v_fstype == VDUX_CDFS) {
		cdp = VTOCD(vp);

		/* If the serving site has died, references to inode fields
		 * which were cleaned up by ino_cleanup may result in 
		 * segmentation violations, so return an error.   mry 1/6/91
		 */
		if (devsite(cdp->cd_dev) == 0)
			return (DM_CANNOT_DELIVER);

		fstype = MOUNT_CDFS;
		if (vsync == VASYNC || vsync == VIFSYNC)
		{	/*can request be executed locally?*/
			return (CDFSCALL(CDFS_GETATTR)(vp, vap, cred, VASYNC));
		}
		site = devsite(cdp->cd_dev);
	}
	else {
		panic("dux_getattr: illegal fs type");
	}
	request = dm_alloc(sizeof(struct getattr_req),WAIT);
	gp = DM_CONVERT(request, struct getattr_req);
	pack_ucred(cred, &gp->cred);
	if (fstype == MOUNT_UFS) {
		gp->dev = ip->i_dev;
		gp->ino = ip->i_number;
	}
	else {
		gp->dev = cdp->cd_dev;
		gp->cdno = cdp->cd_num;
	}
	gp->fstype = fstype;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST,
			 DM_GETATTR, site,
			 sizeof (struct vattr), NULL, NULL,
			 NULL, NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	if (!error)
	{
		*vap = *(DM_CONVERT(reply, struct vattr));
		if (fstype == MOUNT_UFS) {
			vap->va_fsid = ip->i_dev;
		}
		else {
			vap->va_fsid = cdp->cd_dev;
		}
	}
	dm_release(reply,0);
	return (error);
}

dux_getattr_serve(request)
dm_message request;
{
	dm_message reply;
	register struct getattr_req *gp =
		DM_CONVERT(request, struct getattr_req);
	register struct inode *ip;
	register struct vnode *vp;
	int error;
	struct ucred ucred;
	register struct cdnode *cdp;

	if (gp->fstype == MOUNT_UFS) {
		ip = ifind(localdev(gp->dev), gp->ino);
		if (ip == NULL)	/*shouldn't happen*/
		{
			printf("ifind failed:  dux_getattr_serve\n");
			dm_quick_reply (ENOENT);
			return;
		}
		vp = ITOV(ip);
	}
	else if (gp->fstype == MOUNT_CDFS) {
		cdp = (struct cdnode *)CDFSCALL(CDFIND)(localdev(gp->dev), gp->cdno);
		if (cdp == NULL)	/*shouldn't happen*/
		{
			printf("cdfind failed:  dux_getattr_serve\n");
			dm_quick_reply (ENOENT);
			return;
		}
		vp = CDTOV(cdp);
	}
	else {
		panic("dux_getattr_serve: illegal type");
		dm_quick_reply (ENOENT);
		return;
	}
	unpack_ucred(&gp->cred, &ucred);
	u.u_cred = &ucred;
	reply = dm_alloc(sizeof (struct vattr), WAIT);
	error = VOP_GETATTR(vp,DM_CONVERT(reply, struct vattr),&ucred,VSYNC);
	dm_reply (reply, 0, error, NULL, NULL, NULL);
	restore_ulookup();	/*to restore the ucred*/
}


/*
 *send a request to decrement the reference count on an inode, releasing it
 *if necessary
 */
send_ref_update (ip, subtract)
register struct inode *ip;
int subtract;
{
	dm_message request;
	register struct iref_update *rp;

	request = dm_alloc(sizeof (struct iref_update), WAIT);
	rp = DM_CONVERT(request, struct iref_update);
	rp->fstype = MOUNT_UFS;
	rp->dev = ip->i_dev;
	rp->un.ino = ip->i_number;
	rp->subtract = subtract;
	dm_send(request, DM_RELEASE_REQUEST|DM_RELEASE_REPLY|(subtract<0?DM_SLEEP:0), DM_REF_UPDATE,
		devsite(ip->i_dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
}

/*
 *send a request to decrement the reference count on a server's inode,
 * when was unable to get an inode on the client side.
 */
send_ref_update_no_ino(dev, inum)
dev_t dev;
ino_t inum;
{
	dm_message request;
	register struct iref_update *rp;

	request = dm_alloc(sizeof (struct iref_update), WAIT);
	rp = DM_CONVERT(request, struct iref_update);
	rp->fstype = MOUNT_UFS;
	rp->dev = dev;
	rp->un.ino = inum;
	rp->subtract = 1;
	dm_send(request, DM_RELEASE_REQUEST|DM_RELEASE_REPLY, DM_REF_UPDATE,
		devsite(dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
}

serve_ref_update(request)
dm_message request;
{
	register struct iref_update *rp;
	register struct inode *ip;
	register i;
	register struct cdnode *cdp;

	rp = DM_CONVERT(request, struct iref_update);
	if (rp->fstype == MOUNT_CDFS) {
	   cdp = (struct cdnode *)CDFSCALL(CDFIND)(localdev(rp->dev), rp->un.cdno);
	   if (cdp != NULL)		/*should never be null*/
	   {
		updatesitecount (&(cdp->cd_refsites), u.u_site, -rp->subtract);
		if (rp->subtract >= 0) {
			for (i=rp->subtract; i>0; i--)
				VN_RELE(CDTOV(cdp));
                } else {
			for (i=rp->subtract; i<0; i++)
				VN_HOLD(CDTOV(cdp));
                }
	   } else {
		printf("cdfind failed:  serve_ref_update\n");
	   }
	}
	else {
	   ip = ifind(localdev(rp->dev), rp->un.ino);
	   if (ip != NULL)		/*should never be null*/
	   {
		updatesitecount (&(ip->i_refsites), u.u_site, -rp->subtract);
		if (rp->subtract >= 0) {
			for (i=rp->subtract; i>0; i--)
				VN_RELE(ITOV(ip));
                } else {
			for (i=rp->subtract; i<0; i++)
				VN_HOLD(ITOV(ip));
                }
	   } else {
		printf("ifind failed:  serve_ref_update\n");
	   }
	}
	dm_quick_reply(0);
}

/*
 *If this is a dux vnode, update the real and virtual counts.  If necessary
 *send a message to the server.
 */
update_duxref(vp, delta, serverinitiated)
struct vnode *vp;
int delta;
int serverinitiated;
{
	int subtract;

	if (vp->v_fstype == VDUX)
	{
		subtract = updatedcount(&((VTOI(vp))->i_refcount),delta,
			serverinitiated);
		if (subtract) {
			/* Make sure all outstanding I/O's have completed
			 * before we release the inode on the server.  This
			 * prevents servestratreadi from not being able to
			 * find the inode in the case of read ahead.
			 */
			dux_flush_cache(vp);
			send_ref_update(VTOI(vp), subtract);
		}
	}
	else if (vp->v_fstype == VDUX_CDFS)
	{
		subtract = updatedcount(&((VTOCD(vp))->cd_refcount),delta,
			serverinitiated);
		if (subtract) {
			/* Make sure all outstanding I/O's have completed
			 * before we release the cdnode on the server.  This
			 * prevents servestratreadcd from not being able to
			 * find the cdnode in the case of read ahead.
			 */
			dux_flush_cache(vp);
			send_ref_update_v(vp, subtract);
		}
	}
}

/*fsync request*/
/*
 *NOTE:  To be perfectly correct, we should ship the cred field with the
 *message.  However, nobody uses it anyway, so why bother?
 */

struct fsyncreq			/*DUX MESSAGE STRUCTURE*/
{
	dev_t fs_dev;		/*device*/
	ino_t fs_ino;		/*inode number*/
};

/*ARGSUSED*/
int
dux_fsync(vp, cred, ino_chg)
register struct vnode *vp;
struct ucred *cred;
int ino_chg;
{
	dm_message request, response;
	struct inode *ip = VTOI(vp);
	register struct fsyncreq *fsp;
	int error;

	/* First sync the vnode locally, to force buffers to server */
	ufs_fsync(vp,u.u_cred,0);
	/* Now tell the server to fsync the file */
	request = dm_alloc(sizeof(struct fsyncreq),WAIT);
	fsp = DM_CONVERT(request,struct fsyncreq);
	fsp->fs_dev = ip->i_dev;
	fsp->fs_ino = ip->i_number;
	response = dm_send (request,
		DM_RELEASE_REQUEST|DM_REPEATABLE|DM_SLEEP|DM_INTERRUPTABLE,
		DM_FSYNC, devsite(ip->i_dev), DM_EMPTY, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(response);
	dm_release(response, 0);
	return (error);
}

/*serve the fsync request*/

servefsync(request)
dm_message request;
{
	register struct fsyncreq *fsp;
	register struct inode *ip;
	int error;

	fsp = DM_CONVERT(request, struct fsyncreq);
	ip = ifind (localdev(fsp->fs_dev), fsp->fs_ino);
	if (ip == NULL) {
		error = ENOENT;
		printf("ifind failed:  servefsync\n");
	}
	else
	{
		error = VOP_FSYNC(ITOV(ip),u.u_cred,1);
	}
	dm_quick_reply(error);
}


/*ARGSUSED*/
dux_readdir(vp, uiop, cred)
struct vnode *vp;
register struct uio *uiop;
struct ucred *cred;
{
	register struct iovec *iovp;
	register int count;
        struct inode *ip;

        ip = VTOI(vp);

	/* Make sure the argument is a directory */
	if ((ip->i_mode&IFMT) != IFDIR)
		return(ENOTDIR);

	iovp = uiop->uio_iov;
	count = iovp->iov_len;
	if ((uiop->uio_iovcnt != 1) || (count < DIRBLKSIZ))
		return (EINVAL);
	count &= ~(DIRBLKSIZ - 1);
	uiop->uio_resid -= iovp->iov_len - count;
	iovp->iov_len = count;
	return (dux_rwip(ip, uiop, UIO_READ, 0));
}

struct symlink_req {
	ino_t	sr_ino;
	dev_t	sr_dev;
	struct vattr	sr_vattr;
	struct	lk_usave sr_usave;
	int	sr_lnamlen;
	int	sr_tnamlen;
};

struct symlink_reply {
	struct vattr srp_vattr;
#ifdef QUOTA
        dev_t   rdev;  /* device that link is on */
        site_t  site;  /* site of fs that link is on (ushort) */
        ushort  quota_flags;  /* flags to tell client of quota violations */
#define SYMLINKDQOFF       1
#define SYMLINKDQOVERSOFT  2
#define SYMLINKDQOVERHARD  4
#define SYMLINKDQOVERTIME  8
#endif QUOTA
};

/*ARGSUSED*/
dux_symlink(dvp, lnm, vap, tnm, cred)
struct vnode *dvp;
char *lnm;
struct vattr *vap;
char *tnm;
struct ucred *cred;
{
	dm_message request, reply;
	register struct symlink_req *symrp;
	register struct symlink_reply *symrepp;
	register struct inode *ip = VTOI(dvp);
	register int i;
	register int error;
	struct buf *bp;
	site_t site;
#ifdef QUOTA
        struct mount *mp;
        struct dquot *dquotp;
        extern void unpack_quota_flags();
	extern struct mount *getrmp();
#endif QUOTA

	request = dm_alloc(sizeof(struct symlink_req), WAIT);
	symrp = DM_CONVERT(request, struct symlink_req);
	symrp -> sr_dev = ip -> i_dev;
	site = devsite(ip -> i_dev);
	symrp -> sr_ino = ip -> i_number;
	lookup_upack(&symrp -> sr_usave);
	bp = geteblk(MAXNAMLEN+MAXPATHLEN+2);
	i = symrp -> sr_tnamlen = strcplen(bp -> b_un.b_addr, tnm);
	symrp -> sr_vattr = *vap;
	symrp -> sr_lnamlen = strcplen(bp -> b_un.b_addr + i, lnm);
	i += symrp -> sr_lnamlen;
	
#ifdef QUOTA
	reply = dm_send(request, DM_SLEEP| DM_REQUEST_BUF| DM_INTERRUPTABLE,
			DM_SYMLINK,
			site, 
                        MAX(sizeof(struct symlink_reply),
                            sizeof(struct quota_reply)),
                        NULL, bp, i, 0,
			NULL, NULL, NULL);
#else not QUOTA
	reply = dm_send(request, DM_SLEEP| DM_REQUEST_BUF| DM_INTERRUPTABLE,
			DM_SYMLINK,
			site, sizeof(struct symlink_reply), NULL, bp, i, 0,
			NULL, NULL, NULL);
#endif QUOTA
	error = DM_RETURN_CODE(reply);
	if (!error) {
           symrepp = DM_CONVERT(reply, struct symlink_reply);
	   *vap = symrepp -> srp_vattr;
#ifdef QUOTA                
           if ((symrepp->quota_flags & SYMLINKDQOFF) == 0)  
           /* quotas were on for this filesystem at server */
	   {
              /* test for soft limits here and issue warning if needed */
              mp = getrmp(symrepp->rdev, symrepp->site);

              if ((mp != NULL) && !getdiskquota(u.u_ruid, mp, 0, &dquotp))
	      {
                 if (symrepp->quota_flags & SYMLINKDQOVERSOFT)
                 {
                    if ((time.tv_sec - dquotp->dq_ftimelimit > DQ_MSG_TIME)
                         && (dquotp->dq_uid == u.u_ruid)
			 && (dquotp->dq_uid != 0))
	            {
	                 uprintf("\nWARNING - too many files (%s)\n",
                                 mp->m_dfs->dfs_fsmnt);
                         dquotp->dq_ftimelimit = time.tv_sec;            
                    }
                 }
                 else if (dquotp->dq_uid != 0)
                    /* clear warning issued time since under limits again */
                    dquotp->dq_ftimelimit = 0;

                 dqput(dquotp);           
	      }
           }
#endif QUOTA
	}
#ifdef QUOTA
        else 
        if (error == EDQUOT)  /* determine type and issue message */
	{
           unpack_quota_flags(reply);
	}
#endif QUOTA
	dm_release(request, 1);
	dm_release(reply, 0);
	return(error);
}

dux_symlink_recv(request)
dm_message request;
{
	dm_message reply;
	struct symlink_req *symrp;
	struct symlink_reply *symrepp;
	char *lnm, *tnm;
	struct buf *bp;
	dev_t	dev;
	struct mount *mp;
	struct inode *ip;
	struct vattr	vattr;
	register int error;
	struct vnode *vp;
	struct	ucred ucred;
#ifdef QUOTA
        extern void reply_quota_flags();
        struct dquot *dquotp;
#endif QUOTA
	
	
	symrp = DM_CONVERT(request, struct symlink_req);
	bp = DM_BUF(request);

	lookup_uunpack(&symrp->sr_usave, &ucred);
	dev = localdev(symrp -> sr_dev);
	mp = getmp(dev);
	if (mp == NULL)    /*should not happen, for it is done once*/
	{
		dm_quick_reply(ENOENT);
		goto out;
	}
	ip = iget(dev, mp, symrp -> sr_ino);
	if (ip == NULL) 
	{
		dm_quick_reply(ENOENT);
		goto out;
	}
	iunlock(ip);
	vattr = symrp -> sr_vattr;
	
	vp = ITOV(ip);
	tnm = bp -> b_un.b_addr;
	lnm = bp -> b_un.b_addr + symrp -> sr_tnamlen;
	error = VOP_SYMLINK(vp, lnm,  &vattr, tnm, &ucred);
	VN_RELE(vp);
	if (error) {
#ifdef QUOTA
                if (error == EDQUOT)
                   reply_quota_flags(vp, error);
                else
#endif QUOTA
	 	   dm_quick_reply(error);
		goto out;
	}
	reply = dm_alloc(sizeof(struct symlink_reply), WAIT);
	symrepp = DM_CONVERT(reply, struct symlink_reply);
	symrepp -> srp_vattr = vattr;
#ifdef QUOTA
        if ((mp != NULL) && !getdiskquota(u.u_ruid, mp, 0, &dquotp))
        {
           symrepp->quota_flags = (dquotp->dq_flags & DQ_SOFT_FILES) ?
                            SYMLINKDQOVERSOFT :
                            0;
           symrepp->quota_flags = (dquotp->dq_flags & DQ_HARD_FILES) ?
                            symrepp->quota_flags | SYMLINKDQOVERHARD :
                            symrepp->quota_flags & ~SYMLINKDQOVERHARD;
           symrepp->quota_flags = (dquotp->dq_flags & DQ_TIME_FILES) ?
                            symrepp->quota_flags | SYMLINKDQOVERTIME :
                            symrepp->quota_flags & ~SYMLINKDQOVERTIME;

           symrepp->rdev = mp->m_rdev;
           symrepp->site = mp->m_site;

           dqput(dquotp);           
        }
        else
           symrepp->quota_flags = SYMLINKDQOFF;
#endif QUOTA
	dm_reply(reply, 0, 0, NULL, NULL, NULL);
out:
	restore_ulookup();
}

struct rename_req {
	dev_t	rn_dev;
	ino_t	srn_ino;
	ino_t	trn_ino;
	struct	lk_usave rn_usave;
	int	snamlen;
	int	tnamlen;
};

/*ARGSUSED*/
dux_rename(sdvp, snm, tdvp, tnm, cred)
struct vnode *sdvp;
char *snm;
struct vnode *tdvp;
char *tnm;
struct ucred *cred;
{
	dm_message request, reply;
	register struct rename_req *srnrp;
	register struct inode *sip = VTOI(sdvp);
	register struct inode *tip = VTOI(tdvp);
	register int i;
	register int error;
	struct buf *bp;
	site_t site;

	if ((sip -> i_dev) != (tip -> i_dev)) {
		return(EXDEV); /* should not happen, since vn_rename() check
				  vfs already.*/
	}
	request = dm_alloc(sizeof(struct rename_req), WAIT);
	srnrp = DM_CONVERT(request, struct rename_req);
	site = devsite(sip -> i_dev);
	srnrp -> rn_dev = sip -> i_dev;
	srnrp -> srn_ino = sip -> i_number;
	srnrp -> trn_ino = tip -> i_number;
	lookup_upack(&srnrp -> rn_usave);

	bp = geteblk(MAXNAMLEN+MAXNAMLEN+4);
	i = srnrp -> snamlen = strcplen(bp -> b_un.b_addr, snm);
	srnrp -> tnamlen = strcplen(bp -> b_un.b_addr + i, tnm);
	i += srnrp -> tnamlen;
	
	reply = dm_send(request, DM_SLEEP | DM_REQUEST_BUF | DM_INTERRUPTABLE,
			DM_RENAME, site, 
			DM_EMPTY, NULL, bp, i, 0, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	dm_release(request, 1);
	dm_release(reply, 0);
	return(error);
}

dux_rename_recv(request)
dm_message request;
{
	struct rename_req *synrp;
	char *snm, *tnm;
	dev_t	dev;
	struct mount *mp;
	register struct inode *sip, *tip;
	register int error;
	register struct vnode *sdvp, *tdvp;
	struct	ucred ucred;
	struct buf *bp;
	
	
	synrp = DM_CONVERT(request, struct rename_req);
	bp = DM_BUF(request);
	lookup_uunpack(&synrp -> rn_usave, &ucred);

	dev = localdev(synrp -> rn_dev);
	mp = getmp(dev);
	if (mp == NULL)    /*should not happen, for it is done once*/
	{
		dm_quick_reply(ENOENT);
		goto out;
	}
	sip = iget(dev, mp, synrp -> srn_ino);
	if (sip == NULL) 
	{
		dm_quick_reply(ENOENT);
		goto out;
	}
	iunlock(sip);
	sdvp = ITOV(sip);
	tip = iget(dev, mp, synrp -> trn_ino);
	if (tip == NULL) 
	{
		VN_RELE(sdvp);
		dm_quick_reply(ENOENT);
		goto out;
	}
	iunlock(tip);
	tdvp = ITOV(tip);
	
	snm = bp -> b_un.b_addr;
	tnm = snm + synrp -> snamlen;
	error = VOP_RENAME(sdvp, snm,  tdvp, tnm, &ucred);
	VN_RELE(sdvp);
	VN_RELE(tdvp);
	if (error) {
		dm_quick_reply(error);
		goto out;
	}
	dm_quick_reply(0);
out:
	restore_ulookup();
}

/*copy string and return length of string (length includes the NULL) */
strcplen(to, from)
register char *to, *from;
{
	register int i;

	for (i=1; *to++ = *from++; i++) ;
	return(i);
}

#ifdef	LOCAL_DISC
/*ARGSUSED*/
dux_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	if (dvp->v_type == VDIR)
		return(EISDIR);
	panic("dux_create not valid");
	/*NOTREACHED*/
}

/*ARGSUSED*/
dux_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;
{

	if (dvp->v_type == VDIR)
		return(EEXIST);
	panic("dux_mkdir not valid");
	/*NOTREACHED*/
}
#endif	LOCAL_DISC

#ifdef ACLS
struct getacl_req			/* a getacl request */
{
	struct packed_ucred cred;
	dev_t dev;
	ino_t ino;
	int   ntuples;
};

struct getacl_rep			/* a getacl reply */
{
	struct	dux_acl_tuple	acl[NACLTUPLES];
	int	tuple_count;
};

/* D U X _ G E T A C L
 *
 * This is the vnodeop which is run on a diskless node for a getacl call.
 * It allocates a request, packages up the request, sends it to the 
 * server. The reply contains the acl and count returned and any error.
 * If the number of tuples requested is greater than zero then copy the
 * acl out to user land. Return the tuple count to the user.
 */

dux_getacl(vp, ntuples, tupleset)
register struct vnode * vp;
int ntuples;
register struct acl_tuple_user *tupleset;
{
	register struct inode *ip;
	dm_message request;
	register struct getacl_req *grqp;
	register struct getacl_rep *grpp;
	dm_message reply;
        register int i;
        struct acl_tuple_user tmp_tupleset[NACLTUPLES];

	ip = VTOI(vp);
	request = dm_alloc(sizeof(struct getacl_req),WAIT);
	grqp = DM_CONVERT(request, struct getacl_req);
	pack_ucred(u.u_cred, &grqp->cred);
	grqp->dev = ip->i_dev;
	grqp->ino = ip->i_number;
	grqp->ntuples = ntuples;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST,
			 DM_GETACL, devsite(ip->i_dev),
			 sizeof (struct getacl_rep), NULL, NULL,
			 NULL, NULL, NULL, NULL, NULL);
	u.u_error = DM_RETURN_CODE(reply);
	grpp = (DM_CONVERT(reply, struct getacl_rep));
	if (ntuples && !u.u_error)
	{
                for (i=0; i<ntuples; i++){
                        tmp_tupleset[i].uid=grpp->acl[i].uid;
                        tmp_tupleset[i].gid=grpp->acl[i].gid;
                        tmp_tupleset[i].mode=grpp->acl[i].mode;
                }
		u.u_error = copyout(tmp_tupleset,tupleset,ntuples *
				sizeof (struct acl_tuple_user));
	}
	u.u_r.r_val1 = grpp->tuple_count;
	dm_release(reply,0);
}

/* D U X _ G E T A C L _ S E R V E
 *
 * Service the getacl request. We need to find the inode on the local
 * system first (should always succeed). We then allocate a reply message.
 * The reply message is filled in by calling vno_getacl. Any errors are
 * contained in u.u_error, which is returned as part of the reply.
 */

dux_getacl_serve(request)
dm_message request;
{
	dm_message reply;
	register struct getacl_req *grqp =
		DM_CONVERT(request, struct getacl_req);
	register struct getacl_rep *grpp;
	register struct inode *ip;
	register struct vnode *vp;
	struct ucred ucred;
	struct acl_tuple acl[NACLTUPLES];
   	register int i;

	ip = ifind(localdev(grqp->dev), grqp->ino);
	if (ip == NULL)	/*shouldn't happen*/
	{
		printf("ifind failed:  dux_getacl_serve\n");
		dm_quick_reply (ENOENT);
		return;
	}
	unpack_ucred(&grqp->cred, &ucred);  /* probably unneccessary */
	u.u_cred = &ucred;
	vp = ITOV(ip);
	reply = dm_alloc(sizeof (struct getacl_rep), WAIT);
	grpp = (DM_CONVERT(reply, struct getacl_rep));
	grpp->tuple_count = vno_getacl(vp, grqp->ntuples, acl);
	if (grqp->ntuples && !u.u_error)
	{
		for (i=0; i<grqp->ntuples; i++) {
			grpp->acl[i].uid = acl[i].uid;
			grpp->acl[i].gid = acl[i].gid;
			grpp->acl[i].mode = acl[i].mode;
		}
	}	
	dm_reply (reply, 0, u.u_error, NULL, NULL, NULL);
	restore_ulookup();
}

struct setacl_req				/* a setacl request */
{
	struct	packed_ucred cred;
	dev_t	dev;
	ino_t	ino;
	int	ntuples;
	struct	dux_acl_tuple acl[NACLTUPLES];
};

/* D U X _ S E T A C L
 *
 * This is the vnodeop called to handle a setacl call on a diskless node.
 * We allocate the request, package up the data and ship it over the net.
 * Any error is returned in the reply
 */

dux_setacl(vp,ntuples,tupleset)
register struct vnode * vp;
int ntuples;
register struct acl_tuple *tupleset;
{
	register int i;
	register struct inode *ip;
	dm_message request;
	register struct setacl_req *srqp;
	dm_message reply;

	ip = VTOI(vp);
	request = dm_alloc(sizeof(struct setacl_req),WAIT);
	srqp = DM_CONVERT(request, struct setacl_req);
	pack_ucred(u.u_cred, &srqp->cred);
	srqp->dev = ip->i_dev;
	srqp->ino = ip->i_number;
	srqp->ntuples = ntuples;
	for (i=0; i<NACLTUPLES; i++) {
                srqp->acl[i].uid = tupleset[i].uid;
                srqp->acl[i].gid = tupleset[i].gid;
                srqp->acl[i].mode = tupleset[i].mode;
        }
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST, DM_SETACL,
		devsite(ip->i_dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
	u.u_error = DM_RETURN_CODE(reply);
	dm_release(reply,0);
}

/* D U X _ S E T A C L _ S E R V E
 *
 * Service the setacl request. First find the inode on the local machine.
 * Unpack the ucred since we will be checking credentials. Call the vnodeop
 * to handle the request. Send back the return value and any errno. Restore
 * the credentials.
 */

dux_setacl_serve(request)
dm_message request;
{
	register struct setacl_req *srqp =
		DM_CONVERT(request, struct setacl_req);
	register struct inode *ip;
	register struct vnode *vp;
	register int i;
        struct acl_tuple acl_list[NACLTUPLES];
	struct ucred ucred;

	ip = ifind(localdev(srqp->dev), srqp->ino);
	if (ip == NULL)	/*shouldn't happen*/
	{
		printf("ifind failed:  dux_setacl_serve\n");
		dm_quick_reply (ENOENT);
		return;
	}
	vp = ITOV(ip);
	unpack_ucred(&srqp->cred, &ucred);
	u.u_cred = &ucred;
        for (i=0; i<NACLTUPLES; i++) {
                acl_list[i].uid = srqp->acl[i].uid;
                acl_list[i].gid = srqp->acl[i].gid;
                acl_list[i].mode = srqp->acl[i].mode;
        }
	VOP_SETACL(vp,srqp->ntuples, acl_list);
	dm_quick_reply (u.u_error);
	restore_ulookup();
}
#endif /* ACLS */
#ifdef	POSIX
struct fpathconf_req				/* a fpathconf request */
{
	struct	packed_ucred cred;
	dev_t	dev;
	ino_t	ino;
	int	name;
	cdno_t	cdno;
	int	fstype;
};
struct fpathconf_rep				/* a fpathconf reply */
{
	int	result;
};
int
dux_fpathconf(vp, name, resultp, cred)
register struct vnode *vp;
register int name;
int	*resultp;
struct ucred *cred;
{
	register struct inode *ip;
	dm_message request;
	register struct fpathconf_req *fpp;
	dm_message reply;
	int error;

	site_t	site;
	register struct cdnode *cdp;
	request = dm_alloc(sizeof(struct fpathconf_req),WAIT);
	fpp = DM_CONVERT(request, struct fpathconf_req);
	pack_ucred(cred, &fpp->cred);
	if (vp->v_fstype == VDUX) {
		ip = VTOI(vp);
		fpp->dev = ip->i_dev;
		fpp->ino = ip->i_number;
		fpp->fstype = MOUNT_UFS;
		site = devsite(ip->i_dev);
	}
	else if (vp->v_fstype == VDUX_CDFS) {
		cdp = VTOCD(vp);
		fpp->dev = cdp->cd_dev;
		fpp->cdno = cdp->cd_num;
		fpp->fstype = MOUNT_CDFS;
		site = devsite(cdp->cd_dev);
	}
	else panic("dux_fpathconf: illegal fs type");
	fpp->name = name;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST,
			 DM_FPATHCONF, site,
			 sizeof (struct fpathconf_rep), NULL, NULL,
			 NULL, NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	if (!error)
	{
		*resultp = DM_CONVERT(reply, struct fpathconf_rep) -> result;
	}
	dm_release(reply,0);
	return (error);
}

dux_fpathconf_serve(request)
dm_message request;
{
	dm_message reply;
	register struct fpathconf_req *fpp =
		DM_CONVERT(request, struct fpathconf_req);
	register struct inode *ip;
	register struct vnode *vp;
	int error;
	struct ucred ucred;
	register struct cdnode *cdp;

	if(fpp->fstype == MOUNT_UFS) {
		ip = ifind(localdev(fpp->dev), fpp->ino);
		if (ip == NULL)	/*shouldn't happen*/
		{
			printf("ifind failed:  dux_fpathconf_serve\n");
			dm_quick_reply (ENOENT);
			return;
		}
		vp = ITOV(ip);
	}
	else if(fpp->fstype == MOUNT_CDFS) {
		cdp = (struct cdnode *)CDFSCALL(CDFIND)(localdev(fpp->dev), fpp->cdno);
		if (cdp == NULL)	/*shouldn't happen*/
		{
			printf("cdfind failed:  dux_fpathconf_serve\n");
			dm_quick_reply (ENOENT);
			return;
		}
		vp = CDTOV(cdp);
	}
	else {
		printf("dux_fpathconf_serve: illegal fs type");
		dm_quick_reply (ENOENT);
		return;
	}

	unpack_ucred(&fpp->cred, &ucred);
	reply = dm_alloc(sizeof (struct fpathconf_rep), WAIT);
	error = VOP_FPATHCONF(vp, fpp->name, 
		     &(DM_CONVERT(reply, struct fpathconf_rep)->result),&ucred);
	dm_reply (reply, 0, error, NULL, NULL, NULL);
}
#endif	POSIX

/*
 * NOTE: dux_lockctl() and dux_clockf() are located in the file
 * dux/dux_lockf.c
 */


close_send_no_vno(rvp, filemode)
register struct remvno *rvp;
int	filemode;
{
	register struct mount *mp;
	extern struct mount *getrmount();
	register dev_t dev;
	register cdno_t	cdnum;
	register ino_t	inum;


	switch(rvp->rv_fstype) {
	case MOUNT_CDFS:
		mp = getrmount(rvp->rv_cdno.dev, rvp->rv_cdno.site);
		dev = mp->m_dev;
		cdnum = rvp->rv_cdno.cdno;
		CDFSCALL(CLOSE_SEND_NO_CDNO)(dev, cdnum, filemode);
		break;
	case MOUNT_UFS:
		mp = getrmount(rvp->rv_ino.dev, rvp->rv_ino.site);
		dev = mp->m_dev;
		inum = rvp->rv_ino.ino;
		close_send_no_ino(dev, inum, filemode);
		break;
	default:
		panic("close_send_no_vno: illegal fs type");
	}
}

fsctl_serve(request)
dm_message request;
{
	register struct fsctl_request *fstrqp;

	fstrqp = DM_CONVERT(request, struct fsctl_request);
	if (fstrqp->fstype == MOUNT_CDFS) {
		CDFSCALL(FSCTLCD_SERVE)(fstrqp);
	}
	else {
		panic("fsctl_serve: illegal fs type");
	}
}

/*ARGSUSED*/
int 
dux_fid(vp, fidpp)
  struct vnode *vp;
  caddr_t **fidpp;
/* Operation unsupported on dux clients since they can't be nfs servers */
{ return(EINVAL); }
