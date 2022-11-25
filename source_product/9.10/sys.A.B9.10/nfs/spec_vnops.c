/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/spec_vnops.c,v $
 * $Revision: 1.8.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:02:47 $
 */

/*
 * Copyright (c) 1988 by Hewlett Packard
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../nfs/snode.h"
#include "../h/pfdat.h"

#include "../ufs/inode.h" 	/* Used to get IFBLK, IFCHR defines */

#ifndef hpux
#include "../krpc/lockmgr.h"
#endif hpux
#include "../h/kern_sem.h"

int spec_open();
int spec_close();
int spec_rdwr();
int spec_ioctl();
int spec_select();
int spec_getattr();
int spec_setattr();
int spec_access();
int spec_link();
int spec_inactive();
#ifdef notdef
int spec_badop();
#endif notdef
int spec_noop();
int spec_notdir();	/* Returns ENOTDIR instead of EINVAL like spec_noop */
int spec_fsync();
int spec_lockctl();
int spec_lockf();
int spec_fid();
#ifdef ACLS
int spec_setacl();
int spec_getacl();
#endif ACLS
#ifdef POSIX
int spec_pathconf();
#endif POSIX

struct vnodeops spec_vnodeops = {
	spec_open,
	spec_close,
	spec_rdwr,
	spec_ioctl,
	spec_select,
	spec_getattr,
	spec_setattr,
	spec_access,
	spec_notdir,	/* lookup */
	spec_notdir,	/* create */
	spec_notdir,	/* remove */
	spec_link,
	spec_notdir,	/* rename */
	spec_notdir,	/* mkdir */
	spec_notdir,	/* rmdir */
	spec_notdir,	/* readdir */
	spec_notdir,	/* symlink */
	spec_noop,	/* readlink */
	spec_fsync,
	spec_inactive,
	spec_noop,	/* bmap */
	spec_noop,	/* strategy */
	spec_noop,	/* bread */
	spec_noop,	/* brelse */
	spec_noop,	/* pathsend */
#ifdef ACLS
	spec_setacl,
	spec_getacl,
#endif ACLS
#ifdef POSIX
	spec_pathconf,
	spec_pathconf,
#endif POSIX
	spec_lockctl,
	spec_lockf,
	spec_fid,
	spec_noop,	/* fscntl() */
	spec_noop,	/* prefill() */
	spec_noop,	/* pagein() */
	spec_noop,	/* pageout() */
	NULL,		/* dbddealloc() */
	NULL,		/* dbddup() */
};

/*
 * open a special file (device)
 * some weird stuff here having to do with clone and indirect devices:
 * When a file open operation happens (e.g. ufs_open) and the vnode has
 * type VDEV the open routine makes a spec vnode and calls us. When we
 * do the device open routine there are two possible strange results:
 * 1) an indirect device will return an error on open and return a new
 *    dev number. we have to make that into a spec vnode and call open
 *    on it again.
 * 2) a clone device will return a new dev number on open but no error.
 *    in this case we just make a new spec vnode out of the new dev number
 *    and return that.
 *
 * HPNFS:  Modified to all DUX routines to open device.  This will do two
 * things.  First, help insure that we behave the same as the local code,
 * and second, check for the final close before closing a block device, which
 * is something Sun missed in 3.2 code.  They handle it somewhat in the 4.0
 * code by adding a counter to the snode.  Since the dux routines handle this,
 * just use them.
 */
/*ARGSUSED*/
int
spec_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;
	dev_t dev;
	dev_t newdev;
	dev_t olddev;
	int error;

	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */
	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		error = EINTR;
		(void) spec_close(*vpp, flag & FMASK, cred);
		return (error);
	}
	sp = VTOS(*vpp);

	/*
	 * Do open protocol for special type.
	 */
	olddev = dev = sp->s_dev;

	switch ((*vpp)->v_type) {

	case VCHR:
		/*
		 * HPNFS: Sun code assumed that cloning drivers have to
		 * be called again to open the cloned device.  HP cloning
		 * drivers open their new device before returning the
		 * new number, so the logic is simpler.
		*/
		newdev = dev;
#ifdef FULLDUX
		error = opend(&dev, IFCHR, my_site, flag, &newdev);
#else  /* FULLDUX */
		error = opend(&dev, IFCHR, flag, &newdev);
#endif /* FULLDUX */

		if (olddev != newdev && error == 0) {
			register struct vnode *nvp;

			/*
			 * Allocate new snode with new minor device. Release
			 * old snode. Set vpp to point to new one.  This snode
			 * will go away when the last reference to it goes away.
			 * Warning: if you stat this, and try to match it with
			 * a name in the filesystem you will fail, unless you
			 * had previously put names in that match.
			 */
			nvp = specvp(*vpp, newdev);
			VN_RELE(*vpp);
			*vpp = nvp;
		}
		break;

	case VBLK:
#ifdef FULLDUX
		error = opend(&dev, IFBLK, my_site, flag, 0);
#else  /* FULLDUX */
		error = opend(&dev, IFBLK, flag, 0);
#endif /* FULLDUX */
		break;

#ifndef hpux
	/*
	 * Sun does a printf in case that can never happen.  What a
	 * useful piece of code!
	 */
	case VFIFO:
		printf("spec_open: got a VFIFO???\n");
#endif hpux

	case VSOCK:
		error = EOPNOTSUPP;
		break;
	
	default:
		error = 0;
		break;
	}
	return (error);
}

/*
 * HPNFS:  Changed the close code to look more like the local code and to
 * use the DUX close routines.  The DUX close routines handle the proper
 * pairing of blocked device files and make sure the device is only closed
 * on the last close.  This works because these particular DUX routines
 * don't rely on having a vnode!
 */

/*ARGSUSED*/
int
spec_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;
	dev_t dev;

	/*
	 * setjmp in case close is interrupted
	 */
	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		return (EINTR);
	}
	sp = VTOS(vp);
	dev = sp->s_dev;
	switch(vp->v_type) {

	case VCHR:
#ifdef FULLDUX
		closed (dev, IFCHR, flag, u.u_site);
#else
		closed (dev, IFCHR, flag);
#endif
		break;

	case VBLK:
#ifdef FULLDUX
		closed (dev, IFBLK, flag, u.u_site);
#else
		closed (dev, IFBLK, flag);
#endif
		break;

#ifndef hpux
	/*
	 * This case can never happen except with an internal error.
	 */
	case VFIFO:
		printf("spec_close: got a VFIFO???\n");
#endif hpux

	default:
		return (0);
	}
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
spec_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct snode *sp;
	struct vnode *blkvp;
	dev_t dev;
	struct buf *bp;
	daddr_t lbn, bn;
	register int n, on;
	int size;
	long bsize;
#ifdef __hp9000s300
	extern int ethernet_no;
	extern int ieee802_no;
#endif
	int error = 0;
	int synch_write;

	sp = VTOS(vp);
	dev = (dev_t)sp->s_dev;
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwsp");
	if (rw == UIO_READ && uiop->uio_resid == 0)
		return (0);
        /* uio_offset can go negative for software drivers that open, read,
           or write continuously, and never close */
	if ((uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0)) {
#ifdef __hp9000s300
	    if (major(dev) == ethernet_no || major(dev) == ieee802_no)
		    /* kludge!  how do we set f_offset to 0 ??? */
		    uiop->uio_offset = 0;
	    else
#endif /* hp9000s300 */
	    if (vp->v_type != VCHR)
		return (EINVAL);
	}
	if (rw == UIO_READ) {
		smark(VTOS(vp), SACC);
	}
	if (vp->v_type == VCHR) {
		if (rw == UIO_READ) {
			error = (*cdevsw[major(dev)].d_read)(dev, uiop);
		} else {
			smark(VTOS(vp), SUPD|SCHG);
			error = (*cdevsw[major(dev)].d_write)(dev, uiop);
		}
		return (error);
	} else if (vp->v_type != VBLK) {
		return (EOPNOTSUPP);
	}
	if (uiop->uio_resid == 0)
		return (0);
        synch_write = ((ioflag & IO_SYNC) |
                (uiop->uio_fpflags & FSYNCIO));
	bsize = BLKDEV_IOSIZE;
	u.u_error = 0;
	blkvp = VTOS(vp)->s_bdevvp;
	do {
		lbn = uiop->uio_offset / bsize;
		on = uiop->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uiop->uio_resid);
		bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
		rablock = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
		rasize = size = bsize;
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (sp->s_lastr + 1 == lbn)
				bp = breada(blkvp, bn, size, rablock,
#ifdef	FSD_KI
					rasize, B_spec_rdwr|B_data);
#else	FSD_KI
					rasize);
#endif	FSD_KI
			else
#ifdef	FSD_KI
				bp = bread(blkvp, bn, size, B_spec_rdwr|B_data);
#else	FSD_KI
				bp = bread(blkvp, bn, size);
#endif	FSD_KI
			sp->s_lastr = lbn;
		} else {
			int i, count;

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += NBPG/DEV_BSIZE)
				munhash(blkvp, (daddr_t)(bn + i));
			if (n == bsize) 
#ifdef	FSD_KI
				bp = getblk(blkvp, bn, size, B_spec_rdwr|B_data);
#else	FSD_KI
				bp = getblk(blkvp, bn, size);
#endif	FSD_KI
			else
#ifdef	FSD_KI
				bp = bread(blkvp, bn, size, B_spec_rdwr|B_data);
#else	FSD_KI
				bp = bread(blkvp, bn, size);
#endif	FSD_KI
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uiop);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
			if (synch_write)
				bwrite(bp);
			else if (n + on == bsize && !(bp->b_flags & B_REWRITE)) {
				bp->b_flags |= B_REWRITE;
				bawrite(bp);
			} else
				bdwrite(bp);
			smark(VTOS(vp), SUPD|SCHG);
		}
	} while (u.u_error == 0 && uiop->uio_resid > 0 && n != 0);
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

/*ARGSUSED*/
int
spec_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_ioctl");
	return ((*cdevsw[major(sp->s_dev)].d_ioctl)
			(sp->s_dev, com, data, flag));
}

/*ARGSUSED*/
int
spec_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_select");
	return ((*cdevsw[major(sp->s_dev)].d_select)(sp->s_dev, which));
}

/*ARGSUSED*/
int
spec_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

	/* !!! Problem - spec_fsync calls VOP_GETATTR and can goes to
	   sleep.  If another process does VN_HOLD/VN_RELE the v_count
	   will equals to 0 and spec_inactive will be called to free
	   up the snode.  When VOP_GETATTR wakes up and call kmem_free
	   - panic - the snode has already been freed.  The fix is:
	   v_count++ before spec_fsync and v_count-- when return from
	   spec_fsync so that no other process will call spec_inactive
	   while it is sleeping.  Additional check is also added to make 
	   sure v_count is still 0 when we return so that no other
	   process does VN_RELE and calls spec_inactive again.  
	 */
	SPINLOCK(v_count_lock);
	vp->v_count++;
	SPINUNLOCK(v_count_lock);
	(void) spec_fsync(vp, cred, 0);
	/* XXX
	 * spec_fsync does a xxx_setattr which may set u.u_error. Blech.
	 */
	/*
	 * We shouldn't just zero the errno value here, since if there was
	 * an error already (before we entered this function), the errno
	 * would be lost.  Note that Sun removed this in 4.0.
	 * old code: u.u_error = 0;
	 */
	SPINLOCK(v_count_lock);
	vp->v_count--;
	SPINUNLOCK(v_count_lock);
	if (vp->v_count > 0) return(0);
	sunsave(VTOS(vp));
	kmem_free((caddr_t)VTOS(vp), (u_int)sizeof (struct snode));
	return (0);
}

int
spec_getattr(vp, vap, cred,sync)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error;
	register struct snode *sp;

	sp = VTOS(vp);
	error = VOP_GETATTR(sp->s_realvp, vap, cred, sync);
	if (!error) {
		/* set current times from snode, even if older than vnode */
		vap->va_atime = sp->s_atime;
		vap->va_mtime = sp->s_mtime;
		vap->va_ctime = sp->s_ctime;

		/* set device-dependent blocksizes */
		switch (vap->va_type) {
		case VBLK:
			vap->va_blocksize = BLKDEV_IOSIZE;
			break;

		case VCHR:
			vap->va_blocksize = MAXBSIZE;
			break;
		}
	}
	return (error);
}

int
#ifdef hpux
spec_setattr(vp, vap, cred, nulltime)
#else hpux
spec_setattr(vp, vap, cred)
#endif hpux
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	struct snode *sp;
	int error;
	register int chtime = 0;

	sp = VTOS(vp);
#ifdef hpux
	error = VOP_SETATTR(sp->s_realvp, vap, cred, nulltime);
#else hpux
	error = VOP_SETATTR(sp->s_realvp, vap, cred);
#endif hpux
	if (!error) {
		/* if times were changed, update snode */
		if (vap->va_atime.tv_sec != -1) {
			sp->s_atime = vap->va_atime;
			chtime++;
		}
		if (vap->va_mtime.tv_sec != -1) {
			sp->s_mtime = vap->va_mtime;
			chtime++;
		}
		if (chtime)
			sp->s_ctime = time;
	}
	return (error);
}

int
spec_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{

	return (VOP_ACCESS(VTOS(vp)->s_realvp, mode, cred));
}

spec_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{

	return (VOP_LINK(VTOS(vp)->s_realvp, tdvp, tnm, cred));
}

/*
 * In order to sync out the snode times without multi-client problems,
 * make sure the times written out are never earlier than the times
 * already set in the vnode.
 */
int
#ifdef hpux
spec_fsync(vp, cred, update)
#else hpux
spec_fsync(vp, cred)
#endif hpux
	struct vnode *vp;
	struct ucred *cred;
{
	register int error;
	register struct snode *sp;
	struct vattr *vap;
	struct vattr *vatmp;

	sp = VTOS(vp);
	/* if times didn't change, don't flush anything */
	if ((sp->s_flag & (SACC|SUPD|SCHG)) == 0)
		return (0);

	vatmp = (struct vattr *)kmem_alloc((u_int)sizeof (*vatmp));
	error = VOP_GETATTR(sp->s_realvp, vatmp, cred,VSYNC);
	if (!error) {
		vap = (struct vattr *)kmem_alloc((u_int)sizeof (*vap));
		vattr_null(vap);
		vap->va_atime = timercmp(&vatmp->va_atime, &sp->s_atime, >) ?
		    vatmp->va_atime : sp->s_atime;
		vap->va_mtime = timercmp(&vatmp->va_mtime, &sp->s_mtime, >) ?
		    vatmp->va_mtime : sp->s_mtime;
#ifdef hpux
		VOP_SETATTR(sp->s_realvp, vap, cred, 0);
#else hpux
		VOP_SETATTR(sp->s_realvp, vap, cred);
#endif hpux
		kmem_free((caddr_t)vap, (u_int)sizeof (*vap));
	}
	kmem_free((caddr_t)vatmp, (u_int)sizeof (*vatmp));
#ifdef hpux
	/*
	 * For block devices we need to flush the block associated with the
	 * block device.  The problem is that we really can't know the size
	 * of the device to flush all blocks.  The best we can do is start
	 * delayed writes on all of them.
	 */
	if ( vp->v_type == VBLK )
		bflush(sp->s_bdevvp);
	(void) VOP_FSYNC(sp->s_realvp, cred, update);
#else hpux
	(void) VOP_FSYNC(sp->s_realvp, cred);
#endif hpux
	return (0);
}

#ifdef notdef
int
spec_badop()
{
	panic("spec_badop");
}
#endif notdef

int
spec_noop()
{
	u.u_error = EINVAL;
	return (EINVAL);
}

/*
 * spec_notdir() -- return ENOTDIR for operations that work on directories
 *	instead of the more general EINVAL.
 */
int
spec_notdir()
{
	u.u_error = ENOTDIR;
	return (ENOTDIR);
}

/*
 * Record-locking requests are passed back to the real vnode handler.
 */
int
spec_lockctl(vp, ld, cmd, cred, fp, LB, UB)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	struct file *fp;
	off_t LB, UB;
{
	return (VOP_LOCKCTL(VTOS(vp)->s_realvp, ld, cmd, cred, fp, LB, UB));
}

int
spec_lockf(vp, cmd, len, cred, fp, LB, UB)
	struct vnode *vp;
	int cmd;
	off_t len;
	struct ucred *cred;
	struct file *fp;
	off_t LB, UB;
{
	return (VOP_LOCKF(VTOS(vp)->s_realvp, cmd, len, cred, fp, LB, UB));
}


spec_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	return (VOP_FID(VTOS(vp)->s_realvp, fidpp));
}


#ifdef ACLS
/*
 * Call the file system specific code to find out about ACLS.
 */
int
spec_setacl( vp, ntuples, tupleset )
register struct vnode *vp;
int ntuples;
struct acl_tuple *tupleset;
{
	return ( VOP_SETACL(VTOS(vp)->s_realvp, ntuples, tupleset) );
}

int
spec_getacl( vp, ntuples, tupleset )
register struct vnode *vp;
int ntuples;
struct acl_tuple *tupleset;
{
	return ( VOP_GETACL(VTOS(vp)->s_realvp, ntuples, tupleset) );
}
#endif ACLS

#ifdef POSIX
/*
 * what pathconf() and fpathconf() depends on the file system being called.
 */

int
spec_pathconf( vp, name, resultp, cred )
register struct vnode *vp;
register int name;
int	*resultp;
struct ucred *cred;
{
	return ( VOP_PATHCONF(VTOS(vp)->s_realvp, name, resultp, cred) );
}
#endif POSIX
