/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_vfsops.c,v $
 * $Revision: 1.14.83.7 $       $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 13:33:14 $
 */

/* HPUX_ID: @(#)ufs_vfsops.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
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


/*	@(#)ufs_vfsops.c 1.1 86/02/03 SMI; from UCB 4.1 83/05/27	*/
/*	@(#)ufs_vfsops.c	2.2 86/05/14 NFSSRC */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/pathname.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/signal.h"
#include "../ufs/fs.h"
#include "../h/mount.h"
#include "../ufs/inode.h"
#ifdef __hp9000s800
#include "../h/vmmac.h"
#endif
#include "../ufs/fsdir.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../dux/duxfs.h"
#if defined(GETMOUNT) && defined(QUOTA)
#include "../ufs/quota.h"
#endif
#include "../h/kern_sem.h"

/*
 * ufs vfs operations.
 */
extern int ufs_mount();
extern int ufs_unmount();
extern int ufs_root();
extern int ufs_statfs();
extern int ufs_sync();
extern int nodev();
extern int ufs_vget();
#ifdef GETMOUNT
int ufs_getmount();
#endif GETMOUNT

struct vfsops ufs_vfsops = {
	ufs_mount,
	ufs_unmount,
	ufs_root,
	ufs_statfs,
	ufs_sync,
	ufs_vget,
#ifdef GETMOUNT
	ufs_getmount
#endif GETMOUNT
};

extern int syncdisccheck();

/*
 * this is the default filesystem type.
 * this should be setup by the configurator
 */
extern int ufs_mountroot();
int (*rootfsmount)() = ufs_mountroot;

/*
 * Default device to mount on.
 */
extern dev_t rootdev;

int local_mounts = 0;	/* number of locally mounted filesystems */

#ifdef __hp9000s300
/* This is in globals.h on S800,  The 300 doesn't use its globals.h,
 * therefore its here.
 */
/*
 * Mount table.
 */
struct mounthead mounthash[MNTHASHSZ];
#endif

/*Initialize all the mount hash chains*/

mounthashinit()
{
	register int i;
	register struct mounthead *mp;

	for (mp = mounthash, i = 0; i < MNTHASHSZ; i++, mp++)
	{
		mp->m_rhforw = mp->m_rhback =
		mp->m_hforw = mp->m_hback = (struct mount *)mp;
	}
}

/*
 *Insert a mount entry onto the appropriate mount hash chain.
 *We add the entry to the end of the hash chain, because that way earlier
 *higher mounts such as the root will be found before later mounts.  It
 *is assumed, possibly incorrectly, that the root will be accessed more
 *frequently than lower level paths.  (In any event we need to insert it
 *somewhere, and adding it to the end seems to be as good as adding it
 *anywhere else.
 */
 
mountinshash(mp)
register struct mount *mp;
{
	register struct mount *mhp;
	register int s;

	s = spl6();
	mhp = MOUNTHASH(mp->m_dev);
	mp->m_hback = mhp->m_hback;
	mp->m_hforw = mhp;
	mhp->m_hback->m_hforw = mp;
	mhp->m_hback = mp;

	/*If this is a DUX device, we also need to insert it on the real
	 *device hash chain*/
	if (bdevrmt(mp->m_dev))
	{
		mhp = MOUNTHASH(mp->m_rdev);
		mp->m_rhback = mhp->m_rhback;
		mp->m_rhforw = mhp;
		mhp->m_rhback->m_rhforw = mp;
		mhp->m_rhback = mp;
	}
	else
		mp->m_rhforw = mp->m_rhback = NULL;
	splx(s);
}

/*
 *Remove a mount table entry from the hash table
 */
mountremhash(mp)
register struct mount *mp;
{
	register int s;

	s = spl6();
	mp->m_hback->m_hforw = mp->m_hforw;
	mp->m_hforw->m_hback = mp->m_hback;
	/*also remove it from the real device hash chain*/
	if (mp->m_rhforw)
	{
		mp->m_rhback->m_rhforw = mp->m_rhforw;
		mp->m_rhforw->m_rhback = mp->m_rhback;
	}
	splx(s);
}

#ifdef __hp9000s800
extern int tape0_open();
#endif /* __hp9000s800 */
/*
 * ufs_mount system call
 */
ufs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	struct devandsite *data;
{
	int error;
	site_t site;
	dev_t dev;

	/*
	 * Get the device to be mounted
	 */
	dev = data->dev;
	site = data->site;
	if (major(dev) >= nblkdev) {
		return (ENXIO);
	}
#ifdef __hp9000s800
	/*
	 * We don't support RW mag tape file systems
	 */
	if ((bdevsw[major(dev)].d_open == tape0_open) &&
		!(vfsp->vfs_flag & VFS_RDONLY)) {
		return(EIO);
	}
#endif /* __hp9000s800 */
	/*
	 * Mount the filesystem.
	 */
	if (site != my_site)
		error = send_ufs_mount (dev, site, vfsp, path);
	else
#ifdef FULLDUX
		error = mountfs(dev, site, path, vfsp);
#else  FULLDUX
		error = mountfs(dev, path, vfsp);
#endif  FULLDUX
	return (error);
}

#ifdef __hp9000s300
extern int install;
extern int noswap;
#endif

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root
 */
ufs_mountroot(flag)
	int flag;
{
	struct vfs *vfsp;
	register struct fs *fsp;
	register int error;
#ifdef __hp9000s300 /* install kernel */
 	struct vnode *dev_vp; 	/* for install kernel */
 	struct buf *bp; 	/* for install kernel */
#endif
#ifdef GETMOUNT
#if defined(__hp9000s800) && defined(VOLATILE_TUNE)
	extern volatile struct timeval time;
#else
	extern struct timeval time;
#endif /* __hp9000s800 && VOLATILE_TUNE */
#endif

	vfsp = (struct vfs *)kmem_alloc(sizeof (struct vfs));
	if (vfsp == NULL) {
		return(ENOMEM);
	}
	VFS_INIT(vfsp, &ufs_vfsops, (caddr_t)0, MOUNT_UFS);
 
#ifdef __hp9000s300 /* install kernel */
 	if (install) /* if install kernel (fs_flags msb set), mount read-only */
 	{
#ifdef FULLDUX
#ifdef HPNSE
 		error = opend(&rootdev, IFBLK, my_site, FREAD, 0, 0, 0);
#else  /* HPNSE */
 		error = opend(&rootdev, IFBLK, my_site, FREAD, 0);
#endif /* HPNSE */
#else  /* FULLDUX */
#ifdef HPNSE
 		error = opend(&rootdev, IFBLK, FREAD, 0, 0, 0);
#else  /* HPNSE */
 		error = opend(&rootdev, IFBLK, FREAD, 0);
#endif /* HPNSE */
#endif /* FULLDUX */
		if (error)
			return(error);
 		dev_vp = devtovp(rootdev);
#ifdef	FSD_KI
 		bp = bread(dev_vp, SBLOCK, SBSIZE,
			B_ufs_mountroot|B_sblock);
#else	FSD_KI
 		bp = bread(dev_vp, SBLOCK, SBSIZE); 
#endif	FSD_KI
		if (bp->b_flags & B_ERROR)
			return(EBUSY);
 		fsp = bp->b_un.b_fs;

 		if (fsp->fs_flags & FS_INSTALL)
 		{
 		/* if this bit is set, this is an install media,
 		   set vfs_flag read-only so root fs is mounted RO.
 		   Also set noswap to 1, possibly overriding swapconf() code 
 		*/
 			printf("mounting root fs RD_ONLY\n");
 			vfsp->vfs_flag |= VFS_RDONLY;
 			noswap = 1;
			dbc_set_ceiling();
 		}
 		else {
 		/* if bit not set, clear noswap (go ahead and swap) and
 	   	clear install (don't do locked execs)	
 		*/
 			install = 0;
 		}
 		brelse(bp);
#ifdef FULLDUX
 		closed(rootdev, IFBLK, FREAD, my_site);
#else /* FULLDUX */
 		closed(rootdev, IFBLK, FREAD);
#endif /* FULLDUX */
 	} /* end if install */
#endif /* hp9000s300 - install kernel */
 
	vfsp->vfs_flag |= VFS_MI_DEV;
	/* prepare to mount read-only if asked */
	vfsp->vfs_flag |= flag;
#ifdef	FULLDUX
	error = mountfs(rootdev, my_site, "/", vfsp);
#else	FULLDUX
	error = mountfs(rootdev, "/", vfsp);
#endif	FULLDUX
	if (error) {
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		return (error);
	}
	error = vfs_add((struct vnode *)0, vfsp, 
		(flag & VFS_RDONLY)? M_RDONLY: 0);
	if (error) {
		unmount1(vfsp);
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		return (error);
	}
	vfs_unlock(vfsp);
	fsp = ((struct mount *)(vfsp->vfs_data))->m_bufp->b_un.b_fs;
	inittodr(fsp->fs_time);
#ifdef	GETMOUNT
	/* hard code pseudo root device name for root file system */
	strcpy(vfsp->vfs_name, "/dev/root");
	vfsp->vfs_mnttime = time.tv_sec;
#endif	GETMOUNT
	return (0);
}

int
#ifdef	FULLDUX
mountfs(rdev, site, path, vfsp)
	dev_t rdev;
	site_t site;
#else	FULLDUX
mountfs(rdev, path, vfsp)
	dev_t rdev;
#endif	FULLDUX
	char *path;
	struct vfs *vfsp;
{
	register struct fs *fsp;
	register struct mount *mp = 0;
	register struct buf *bp = 0;
	struct buf *tp = 0;
	struct vnode *dev_vp;
	int error;
	int blks;
	caddr_t space;
	register int i;
	int size;
	extern char *strncpy();
#ifdef  AUTOCHANGER
	extern int ufs_pmount(), ufs_punmount();
#endif  /*AUTOCHANGER */
#ifdef __hp9000s300
 	extern caddr_t sys_memall();
#endif
	dev_t dev = rdev;

	/*
	 * Open block device mounted on.
	 * When bio is fixed for vnodes this can all be vnode operations
	 */
#ifdef FULLDUX
#ifdef HPNSE
	error = opend (&dev,
		(vfsp->vfs_flag & VFS_MI_DEV) ? IFBLK|IF_MI_DEV : IFBLK,
		site,
		(vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		0, 0, 0);
#else  /* HPNSE */
	error = opend (&dev,
		(vfsp->vfs_flag & VFS_MI_DEV) ? IFBLK|IF_MI_DEV : IFBLK,
		site,
		(vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		0);
#endif /* HPNSE */
#else  /* FULLDUX */
#ifdef HPNSE
	error = opend (&dev,
		(vfsp->vfs_flag & VFS_MI_DEV) ? IFBLK|IF_MI_DEV : IFBLK,
		(vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		0, 0, 0);
#else  /* HPNSE */
	error = opend (&dev,
		(vfsp->vfs_flag & VFS_MI_DEV) ? IFBLK|IF_MI_DEV : IFBLK,
		(vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		0);
#endif /* HPNSE */
#endif /* FULLDUX */
	if (error) {
		return (error);
	}

	bp = geteblk(SBSIZE);
	/*
	 * read in superblock
	 */
	dev_vp = devtovp(dev);
#ifdef	FSD_KI
	tp = bread(dev_vp, SBLOCK, SBSIZE,
		B_mountfs|B_sblock);
#else	FSD_KI
	tp = bread(dev_vp, SBLOCK, SBSIZE);
#endif	FSD_KI
	if (tp->b_flags & B_ERROR) {
		error = EIO;
		goto out;
	}
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
	   (u_int)tp->b_bcount);

	/* BUG fix: mount r/w of write-prot. media gave err,
	 * yet left an entry in the kernel mount table.  This
	 * bwrite(tp) catches the fault earlier and exits.
	 */
	tp->b_flags |= B_SYNC; /* let driver know write is synchronous */
	if ((vfsp->vfs_flag & VFS_RDONLY) == 0) {
		bwrite(tp);
		if (tp->b_flags & B_ERROR) {
			error = EIO;
			tp = 0;
			goto out;
		}
	}
	else {
		if (tp->b_flags & B_DELWRI)
			bwrite(tp);
		else {
			tp->b_flags |= B_INVAL;
			brelse(tp);
		}
	}
	tp = 0;

	/*allocate a mount entry.  We do this prior searching the mount
	 *table, because it is capablbe of sleeping.  During this sleep
	 *there is a small possibility that the same device will
	 *be mounted a second time.  If it turns out that the device
	 *is already mounted, we will release the memory.
	 */
	mp = (struct mount *)kmem_alloc(sizeof (struct mount));
	if (getmp(dev))
	{
		if (mp)
		{
			kmem_free(mp, sizeof(struct mount));
			mp = NULL;
		}
		error = EBUSY;
		goto out;
	}
	if (mp == NULL) {
		error = ENOMEM;
		goto out;
	}

	bzero(mp,sizeof(struct mount));
	mp->m_flag = MINTER;	/* just to reserve this slot */
	mp->m_dev = dev;
	mp->m_rdev = rdev;
#ifdef	FULLDUX
	mp->m_site = site;
#else	FULLDUX
	mp->m_site = my_site;
#endif	FULLDUX
	mountinshash(mp);
	mp->m_bufp = bp;
	mp->m_dfs = NULL;
	fsp = bp->b_un.b_fs;
	vfsp->vfs_data = (caddr_t)mp;
	mp->m_vfsp = vfsp;
/* BUG fix: if fs->fs_sbsize = 0, brealloc will assign a 0 length buffer
 *          to bp.  An attempt to access any field in the buffer results
 *          in a panic (bus address).
 */
	if ((fsp->fs_magic != FS_MAGIC &&
	     fsp->fs_magic != FS_MAGIC_LFN &&
	     fsp->fs_magic != FD_FSMAGIC) ||
	     FSF_UNKNOWN(fsp->fs_featurebits) ||
	    ((fsp->fs_bsize < MINBSIZE) || (fsp->fs_bsize > MAXBSIZE)) ||
	    ((fsp->fs_sbsize <= 0) || (fsp->fs_sbsize > MAXBSIZE))) 
		goto out;

/* Got an SBSIZE size buffer; can it be cut back based on fs_sbsize?   We
   want to allow brealloc the luxury of reassigning the buffer pointer, so
   we reassign the pointer upon return from brealloc.
*/
	brealloc(bp, (int)fsp->fs_sbsize);
        fsp = bp->b_un.b_fs;
	if (vfsp->vfs_flag & VFS_RDONLY) {
		fsp->fs_ronly = 1;
		fsp->fs_fmod = 0;
	} else {
		if (bdevrmt(dev))
			fsp->fs_fmod = 0;
		else
			fsp->fs_fmod = 1;
		fsp->fs_ronly = 0;
	}
	vfsp->vfs_bsize = fsp->fs_bsize;
	/*
	 * Read in cyl group info
	 */
	blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
#ifdef __hp9000s800
	space = wmemall(vmemall, (int)fsp->fs_cssize);
#else /* hp9000s300 */
 	space = sys_memall((int)fsp->fs_cssize);
#endif
	if (space == 0)
		goto out;
	for (i = 0; i < blks; i += fsp->fs_frag) {
		size = fsp->fs_bsize;
		if (i + fsp->fs_frag > blks)
			size = (blks - i) * fsp->fs_fsize;
		tp = bread(dev_vp, (daddr_t)fsbtodb(fsp, fsp->fs_csaddr + i),
#ifdef	FSD_KI
		    size, B_mountfs|B_cylgrp);
#else	FSD_KI
		    size);
#endif	FSD_KI
		if (tp->b_flags&B_ERROR) {
			wmemfree(space, (int)fsp->fs_cssize);
			goto out;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		if (tp->b_flags&B_DELWRI) {
			tp->b_flags |= B_SYNC;
			bwrite(tp);
		} else {
			tp->b_flags |= B_INVAL;
			brelse(tp);
		}
		tp = 0;
		fsp->fs_csp[i / fsp->fs_frag] = (struct csum *)space;
		space += size;
	}
	mp->m_flag = MINUSE;	
	(void) strncpy(fsp->fs_fsmnt, path, sizeof(fsp->fs_fsmnt));
	/* need code for rm -rf bug fix ???? */
	/* broadcast_mount returns immediately if not clustered */
	error = broadcast_mount(vfsp);
	if (error) goto out;

	if (!fsp->fs_ronly && !(bdevrmt(dev))) {
		if (fsp->fs_clean == FS_CLEAN)
			fsp->fs_clean = FS_OK;
		else
			fsp->fs_clean = FS_NOTOK;
#ifdef	QUOTA
		if ((u.u_syscall != SYS_VFSMOUNT) || ! (vfsp->vfs_flag & VFS_QUOTA)) {
			FS_QSET( fsp, FS_QNOTOK);
		}
		else if (!(FS_QFLAG( fsp)) || (FS_QFLAG( fsp) == FS_QCLEAN)) {
			FS_QSET( fsp, FS_QOK);
		}
		else {
			FS_QSET( fsp, FS_QNOTOK);
		}
#endif	/* QUOTA */
#ifdef  AUTOCHANGER
		if (bdevsw[major(dev)].d_mount != nodev)
			(*bdevsw[major(dev)].d_mount)(dev,
				vfsp,ufs_pmount,ufs_punmount,0);
#endif  /* AUTOCHANGER */
		fsp->fs_fmod = 0;
#ifdef	FSD_KI
		tp = getblk(dev_vp, SBLOCK, (int)fsp->fs_sbsize,
		    B_mountfs|B_sblock);
#else	FSD_KI
		tp = getblk(dev_vp, SBLOCK, (int)fsp->fs_sbsize);
#endif	FSD_KI
		bcopy((caddr_t)fsp, tp->b_un.b_addr, (u_int)fsp->fs_sbsize);
		tp->b_flags |= B_SYNC;
		bwrite(tp);
		fsp->fs_fmod = 1;
	}
	vfsp->vfs_fsid[0] = (long)mp->m_dev;
	vfsp->vfs_fsid[1] = MOUNT_UFS;

	VN_RELE(dev_vp);
	(void) ufs_root(vfsp, &(mp->m_rootvp), 0);
	local_mounts++;
	return (0);
out:
	if (mp) {
		mountremhash(mp);
		kmem_free((caddr_t)mp, sizeof(struct mount));
	}

	if (bp)
		brelse(bp);
	if (tp)
		brelse(tp);
	closed(dev, IFBLK, (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE);
	VN_RELE(dev_vp);
	if (!error)
		error = EBUSY;
	return (error);
}

/*
 * vfs operations
 */

ufs_unmount(vfsp)
	struct vfs *vfsp;
{

	return (unmount1(vfsp));
}

unmount1(vfsp)
	register struct vfs *vfsp;
{
	dev_t dev;
	register struct mount *mp;
	register struct fs *fs;
	int flag;
	/*register int i;*/	/* work around for compiler bug from Ca. */
	struct buf *bp;
	struct buf *tp;
	struct vnode *dev_vp;
	struct vnode *rootvp;

	mp = (struct mount *)vfsp->vfs_data;
	dev = mp->m_dev;
	dev_vp = devtovp(dev);
	rootvp = mp->m_rootvp;
	mp->m_rootvp = 0;
	if (rootvp)
		VN_RELE(rootvp);
	if (mp->m_flag & MUNMNT) return(EINVAL);
	mp->m_flag |= MUNMNT;	/* Lock out multiple umount's */
	/* bug fix for rm -rf ??? */

#ifdef QUOTA
	if (iflush(dev, mp->m_qinod)< 0)
#else
	if (iflush(dev) < 0)
#endif
	{
		mp->m_flag &= ~MUNMNT;
		return (EBUSY);
	}

#ifdef QUOTA
	fs = mp->m_bufp->b_un.b_fs;
	if ((!fs->fs_ronly) && (FS_QFLAG( fs) == FS_QOK)) {
		FS_QSET( fs, FS_QCLEAN);
	}
        (void)closedq(mp);
        /*
         * Here we have to iflush again to get rid of the quota inode..
         * A drag, but it would be ugly to cheat, & this doesn't happen often.
         */
        (void)iflush(dev, (struct inode *)NULL);
#endif

#ifdef	NSYNC
	update(0,1,0);
#else
	update(0,1);
#endif	/* NSYNC */
	untimeout(syncdisccheck, mp);

	fs = mp->m_bufp->b_un.b_fs;

#ifdef  AUTOCHANGER
		if (bdevsw[major(dev)].d_mount)
			(*bdevsw[major(dev)].d_mount)(dev,
				NULL,NULL,NULL,0);
#endif  /* AUTOCHANGER */

	flag = !fs->fs_ronly;
	/* remove from mount table so no one can access this file system */
	bp = mp->m_bufp;
	mountremhash(mp);
	kmem_free((caddr_t)mp, sizeof(struct mount));
	mpurge(dev_vp);
	if (!fs->fs_ronly) {
		if (fs->fs_clean == FS_OK) {
			fs->fs_clean = FS_CLEAN;
#ifdef	FSD_KI
			tp = getblk(dev_vp, SBLOCK, (int)fs->fs_sbsize,
		    		B_unmount|B_sblock);
#else	FSD_KI
			tp = getblk(dev_vp, SBLOCK, (int)fs->fs_sbsize);
#endif	FSD_KI
			bcopy((caddr_t)fs, tp->b_un.b_addr, (u_int)fs->fs_sbsize);
			tp->b_flags |= B_SYNC;
			bwrite(tp);
		}
	}
	wmemfree((caddr_t)fs->fs_csp[0], (int)fs->fs_cssize);
	brelse(bp);

#ifdef	NSYNC
	update(0,1,0);
#else
	update(0,1);
#endif	/* NSYNC */

#if	! defined(FULLDUX)
		/* We must send the unmount before closing the device,
		 * so the device index can be computed.
		 */
		global_unmount(dev);
#endif	DUX
	/*  Call closed() straight off, since there may be remote sites
	 *  using this device as well, and the device count table will
	 *  take care of everything
	 */
#ifdef FULLDUX
	closed(dev, IFBLK, flag, my_site);
#else
	closed(dev, IFBLK, flag);
#endif
	VN_RELE(dev_vp);
	local_mounts--;
	return (0);
}

/*
 * find root of ufs
 */
int
ufs_root(vfsp, vpp, nm)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *nm;
{
	register struct mount *mp;
	struct inode *ip;
	register struct inode **ipp;
	register struct inode *oldip;
	char *cntxp;
	register char **cntxpp;
	register int error;
	register int possible_sdo;
	sv_sema_t ufs_rootSS;

	if (nm) {		    /*if request is from alien machine, no sdo
				      is possible*/
		cntxp = nm;         /*use cntxp for temporary pointer*/
		while ( *cntxp++ );
		--cntxp;
		if ( *--cntxp != SDOCHAR ) {
/*			possible_sdo = 1;  until we allow mount suid disk*/
			possible_sdo = 0;
			cntxp = nm;
		}
		else {
			possible_sdo = 0;
		}
	}
	else {
		possible_sdo = 0; 	/*alien request*/
	}

	PXSEMA(&filesys_sema, &ufs_rootSS);
	mp = (struct mount *)vfsp->vfs_data;
	if (bdevrmt(mp->m_dev)){
		int ret = dux_pseudo_root(vfsp, vpp);
		VXSEMA(&filesys_sema, &ufs_rootSS);
		return ret;
	}
	ip = iget(mp->m_dev, mp, (ino_t)ROOTINO);
	if (ip == (struct inode *)0) {
		VXSEMA(&filesys_sema, &ufs_rootSS);
		return (u.u_error);
	}
	if (possible_sdo) {
		error = 0;
		ipp = &ip;
		while (!error && ISSDO(*ipp)) {
			oldip = *ipp;
			iunlock(oldip);
			for (error = ENOENT, cntxpp = u.u_cntxp;
			     ((error == ENOENT) && (**cntxpp != '\0'));
			     cntxpp++ ) {
				error = dirlook(oldip, *cntxpp, ipp, ITOV(oldip));
			}
			VN_RELE(ITOV(oldip));
		}
		if (error) {
			VXSEMA(&filesys_sema, &ufs_rootSS);
			return(error);
		}
	}
	iunlock(ip);
	VXSEMA(&filesys_sema, &ufs_rootSS);
	*vpp = ITOV(ip);
	return (0);
}

/*
 * Get file system statistics.
 */
int
ufs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	register struct fs *fsp;
	register struct mount *mp;

	mp = (struct mount *) vfsp->vfs_data;
	if (bdevrmt(mp->m_dev))
		return(dux_fstatfs(vfsp, sbp));
#ifdef LOCAL_DISC
	sbp->f_cnode= mp->m_site; /* my_site */
#endif LOCAL_DISC
	fsp = mp->m_bufp->b_un.b_fs;

	if ((fsp->fs_magic != FS_MAGIC &&
	     fsp->fs_magic != FS_MAGIC_LFN &&
	     fsp->fs_magic != FD_FSMAGIC) ||
	     FSF_UNKNOWN(fsp->fs_featurebits))
		panic("ufs_statfs");
	sbp->f_type = 0;
	sbp->f_bsize = fsp->fs_fsize;
	sbp->f_blocks = fsp->fs_dsize;
	sbp->f_bfree =
	    fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
		fsp->fs_cstotal.cs_nffree;
	/*
	 * avail = MAX(max_avail - used, 0)
	 */
	sbp->f_bavail =
	    (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
		 (fsp->fs_dsize - sbp->f_bfree);
	/*
	 * inodes
	 */
	sbp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	sbp->f_ffree = fsp->fs_cstotal.cs_nifree;
	bcopy((caddr_t)vfsp->vfs_fsid, (caddr_t)sbp->f_fsid, sizeof (fsid_t));
#if defined(__hp9000s800) && !defined(_WSIO)
	(void)map_mi_to_lu(&sbp->f_fsid[0], IFBLK);
#endif /* hp9000s800 && !_WSIO*/
	sbp->f_magic = fsp->fs_magic;
	sbp->f_featurebits = fsp->fs_featurebits;
	return (0);
}

/*
 * Flush any pending I/O.
 */
int
ufs_sync()
{
	/* update(0, 1); see comment in sync() */
	return (0);
}

#ifdef  AUTOCHANGER
sbupdate(mp,shutdown)
	struct mount *mp;
	int shutdown;
#else
sbupdate(mp)
	struct mount *mp;
#endif  /* AUTOCHANGER */
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	register struct buf *bp;
	int blks;
	caddr_t space;
	int i, size;
	register struct vnode *dev_vp;

	dev_vp = devtovp(mp->m_dev);
#ifdef	FSD_KI
	bp = getblk(dev_vp, SBLOCK, (int)fs->fs_sbsize,
		B_sbupdate|B_sblock);
#else	FSD_KI
	bp = getblk(dev_vp, SBLOCK, (int)fs->fs_sbsize);
#endif	FSD_KI
#ifdef  AUTOCHANGER
	/* Race condition with physically mounting with the autochanger. */
	if (shutdown == 0)
		if ((!fs->fs_ronly) && (fs->fs_clean == FS_CLEAN))
			fs->fs_clean = FS_OK;
#endif  /* AUTOCHANGER */
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
	bp->b_flags |= B_SYNC;
	bwrite(bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(dev_vp, (daddr_t)fsbtodb(fs, fs->fs_csaddr + i),
#ifdef	FSD_KI
		    size, B_sbupdate|B_cylgrp);
#else	FSD_KI
		    size);
#endif	FSD_KI
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bp->b_flags |= B_SYNC;
		bwrite(bp);
	}
	VN_RELE(dev_vp);
}

/*
 * This routine is used for auto-configuring the swap area and for swapon 
 * A return value of -1 means that no file system exists.   A return value
 * of 0 means an error occured when attempting to get the file system size.
 * Otherwise, the (positive) value returned is the file system size.
 */
fs_size(dev)
dev_t dev;
{
	register struct buf *bp;
	register struct fs *fs;
	int ok, fs_bytes;
	struct vnode *dev_vp;

#ifdef FULLDUX
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0, 0, 0))
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0))
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0, 0, 0))
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0))
#endif /* HPNSE */
#endif /* FULLDUX */
	        /* return 0 since we can't even open the device */
		return (0);
	dev_vp = devtovp(dev);
#ifdef	FSD_KI
	bp = bread(dev_vp, SBLOCK, SBSIZE,
		B_fs_size|B_sblock);
#else	FSD_KI
	bp = bread(dev_vp, SBLOCK, SBSIZE);
#endif	FSD_KI
	/*
	 * If an error occured, flag it by returning 0 to indicate that
	 * it cannot be determined if a fs exists or what its size is.
	 */
	if ((bp->b_flags & B_ERROR) != 0) 
	      return (0);
	fs = bp->b_un.b_fs;
	ok = (fs->fs_magic == FS_MAGIC ||
	      fs->fs_magic == FS_MAGIC_LFN ||
	      fs->fs_magic == FD_FSMAGIC) &&
	     !(FSF_UNKNOWN(fs->fs_featurebits)) &&
	     (fs->fs_bsize <= MAXBSIZE);

	fs_bytes = ok ?
	    ((u_long)fs->fs_size * (u_long)fs->fs_fsize) >> DEV_BSHIFT : -1;
	brelse(bp);
#ifdef FULLDUX
#ifdef  HPNSE
	(void) closed(dev, IFBLK, FREAD, my_site, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
	(void) closed(dev, IFBLK, FREAD, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD);
#endif /* HPNSE */
#endif /* FULLDUX */
	return (fs_bytes);
}

#ifdef __hp9000s300
/*
 * This routine is used to destroy the filesystem on a device that is being
 * used exclusivly for swap.  This routine is called by check_swap_conditions()
 * after it has warned the user that there is a filesystem on the primary
 * swap device and then waited 15 seconds for them to pull the plug if they
 * don't want the filesystem creamed.  If we get here the vm system is going to
 * swap over the filesystem so we destroy the header so the user won't get the
 * warning the next time.
 */
fs_clobber(dev)
dev_t dev;
{
	register struct buf *bp;
	register struct fs *fs;
	struct vnode *dev_vp;

	dev_vp = devtovp(dev);
	bp = bread(dev_vp, SBLOCK, SBSIZE, B_fs_size|B_sblock);

	/* If an error occured just return */
	if ((bp->b_flags & B_ERROR) != 0) 
	      return;

	fs = bp->b_un.b_fs;

	/* clobber the magic number */
	fs->fs_magic = 0;
	bwrite(bp);
}
#endif /* hp9000s300 */

/*
 * Given the file system and a file id (i.e. the inode number), generate
 * a vnode.  Primarily used by the NFS 3.2 file handle code to look up
 * the vnode for a file handle.
 */

ufs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct inode *ip;
	register struct mount *mp;
	register struct fs *fs;
#ifdef __hp9000s800
	struct ufid sufid;
	register struct ufid *ufid = &sufid;
#else
	register struct ufid *ufid;
#endif

	mp = (struct mount *)vfsp->vfs_data;
#ifdef __hp9000s800
	bcopy((caddr_t)fidp, (caddr_t)ufid, sizeof(struct ufid));
#else
	ufid = (struct ufid *)fidp;
#endif
	/*
	 * Validity check:  iget doesn't check the range of the inode
	 * number, assuming that all internal users will only ask for valid
	 * inode numbers.  Rather than make everybody pay for the overhead
	 * of the check, do it here where NFS is primary user of the routine.
	 */
	fs = mp->m_bufp->b_un.b_fs;
	if ((unsigned)ufid->ufid_ino >= fs->fs_ipg*fs->fs_ncg) {
		*vpp = NULL;
		return(0);
	}

	ip = iget(mp->m_dev, mp, ufid->ufid_ino);
	if (ip == NULL) {
		*vpp = NULL;
		return (0);
	}
	if (ip->i_gen != ufid->ufid_gen) {
		idrop(ip);
		*vpp = NULL;
		return (0);
	}
#ifdef hpux
	/*
	 * Bug fix... It is possible that the fhandle points to a bogus
	 * inode and that the gen numbers just happen to match.  So add
	 * an extra check on the link count.  If link count is zero, there
 	 * is no way this can be a valid file pointer for the purposes
	 * that VOP_VGET is used for, namely to translate an NFS file handle.
	 */
	if ( ip->i_nlink == 0 ) {
		idrop(ip);
		*vpp = NULL;
		return (0);
	}
#endif hpux
	iunlock(ip);
	*vpp = ITOV(ip);
	return (0);
}

#ifdef GETMOUNT

/*
 * get mount table information
 */

int
ufs_getmount(vfsp, fsmntdir, mdp)
struct vfs *vfsp;
caddr_t fsmntdir;
struct mount_data *mdp;
{
	struct mount *mp;
        char *fsmnt;
	int l;

	mp = (struct mount *)vfsp->vfs_data;

	/* only modify fields specific to UFS here */
	mdp->md_msite = mp->m_site;
	mdp->md_dev = mp->m_dev;
	mdp->md_rdev = mp->m_rdev;

#ifdef QUOTA
	mdp->md_ufsopts = (mp->m_qflags & MQ_ENABLED);
#endif
	if (bdevrmt(mp->m_dev))
		fsmnt = mp->m_dfs->dfs_fsmnt;
	else
		fsmnt = mp->m_bufp->b_un.b_fs->fs_fsmnt;
	return(copyoutstr(fsmnt, fsmntdir, MAXPATHLEN, &l));

}
#endif /* GETMOUNT */
