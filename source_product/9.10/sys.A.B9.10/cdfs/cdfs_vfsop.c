/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_vfsop.c,v $
 * $Revision: 1.9.83.5 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/10/13 16:04:39 $
 */

/* HPUX_ID: @(#)cdfs_vfsop.c	54.6		88/12/12 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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

#include "../h/errno.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/file.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../h/user.h"
#include "../ufs/inode.h"
#include "../h/mount.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../cdfs/cdfs_hooks.h"

#define INV_K_ADR NULL

/*
 * cdfs vfs operations.
 */
extern int cdfs_mount();
extern int cdfs_unmount();
extern int cdfs_root();
extern int cdfs_statfs();
extern int cdfs_sync();
extern int cdfs_vget();
#ifdef  GETMOUNT
int cdfs_getmount();
#endif  GETMOUNT

/*
 * Signatures for ISO-9660 or HSG disks.
 */
char	iso_fs_id[] = "CD001";
char	hsg_fs_id[] = "CDROM";

struct vfsops cdfs_vfsops = {
        cdfs_mount,
        cdfs_unmount,
        cdfs_root,
        cdfs_statfs,
        cdfs_sync,
        cdfs_vget,
#ifdef  GETMOUNT
	cdfs_getmount
#endif  GETMOUNT
};

/******************************************************************************
 * cdfs_root()
 *
 * Given a vfs structure pointer, return the vnode points to the root of this
 * CD-ROM file system (not to be confused with the real root of the whole
 * directory hierarchy)  The vnode pointer is return in location pointed by
 * vpp.
 */
/*
 * find root of cdfs
 */
/*ARGSUSED*/
int
cdfs_root(vfsp, vpp, nm)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *nm;
{
	register struct mount *mp;
	struct cdnode *cdp;
	struct	cdfs	*cdfsp;


        /*
         * Get the mount point from vfs structure.
         */
	mp = (struct mount *)vfsp->vfs_data;
	
        /*
         * This should not happen.  If for some reason this happens, we return
         * error here.  We could have panic here. But since no harm is done,
         * we are more forgiving.
         */
	if (!mp->m_dev) {
		printf("cdfs_root - bad mp: mp and vfsp are %x %x\n", mp, vfsp);
		u.u_error = EACCES;
		return(u.u_error);
	}
	
/* NEED WORK FOR DUX
	if (bdevrmt(mp->m_dev))
		return (dux_pseudo_root(vfsp, vpp));
*/

        /*
         * From mount point structure, we can get the CDFS file system
         * structure.  From it then we can get the cdnode number of the
         * root.
         */
	cdfsp = (struct cdfs *)mp->m_bufp->b_un.b_fs;
	cdp = cdget(mp->m_dev, cdfsp, cdfsp->cdfs_rootcdno, 
		    &(cdfsp->cdfs_rootcdno), 0);

	if (cdp == (struct cdnode *)0) {
		return (u.u_error);
	}

        /*
         * Got it.  Don't forget to unlock it.  cdget() returns a locked
         * cdnode.
         */
	cdunlock(cdp);
	*vpp = CDTOV(cdp);
	return (0);
}


/******************************************************************************
 * cdfs_mount()
 *
 * mount a ISO-9660 or HSG file system.  In HP's diskless implementation,
 * we need both device number and site number.  The caller passes in both
 * of them.  For other implementation, the data could simple be the device.
 */
cdfs_mount(vfsp, path, data) 
struct vfs *vfsp;
char *path;
struct devandsite *data;
{
	int	error;
	dev_t	dev;
	site_t	site;

	dev = data->dev;
	site = data->site;

	if (major(dev) >= nblkdev) {
		return(ENXIO);
	}

	if (site != my_site) {
                /*
                 * For mounting a disc on a remote site.
                 */
		error = send_cdfs_mount(dev, site, vfsp, path);
	} else {
                /*
                 * For mounting a disc on a root site or on a standalone
                 * system.
                 */
		error = mountcdfs(dev, path, vfsp);
	}
	return(error);
}

/*
 * Get file system statistics.
 */
int
cdfs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	register struct cdfs *cdfsp;
	register struct mount *mp;

	mp = (struct mount *) vfsp->vfs_data;
	if (bdevrmt(mp->m_dev))
		return(dux_fstatfs(vfsp, sbp));

#ifdef LOCAL_DISC
	sbp->f_cnode = mp->m_site;
#endif

	cdfsp = (struct cdfs *)mp->m_bufp->b_un.b_fs;

        /*
         * If neither ISO-9660 nor HSG, we are in big trouble.  Panic.
         * It might be cases that this should be more graceful.  Since
         * this disk is mounted, there could be other nasty problem.  We
         * choose to panic here.
         */
	if ((cdfsp->cdfs_magic != CDFS_MAGIC_HSG) && 
	    (cdfsp->cdfs_magic != CDFS_MAGIC_ISO))
		panic("cdfs_statfs");

	sbp->f_type = 0;
	sbp->f_bsize = cdfsp->cdfs_lbsize;
	sbp->f_blocks = cdfsp->cdfs_size;

        /*
         * ROM has nothing to spare.
         */
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_files =  -1;
	sbp->f_ffree = 0;
	bcopy((caddr_t)vfsp->vfs_fsid, (caddr_t)sbp->f_fsid, sizeof (fsid_t));

#if defined(__hp9000s800) && !defined(_WSIO)
	(void)map_mi_to_lu(&sbp->f_fsid[0], IFBLK);
#endif /* hp9000s800 */

	/*
	 * Return length of maximum file name length allowed
	 * on file system.  This is undefined information
	 * in the current statfs spec.
	 * This should be removed when pathconf is available.
	 */
	sbp->f_spare[1] = CDMAXNAMLEN;
	return (0);
}

/*****************************************************************************
 * cdfs_sync()
 *
 * Nothing, only to make system happy.
 */
cdfs_sync() 
{
	return(0);
}

/*****************************************************************************
 * mountcdfs()
 *
 * Mount a device (that contains ISO-9660 or HSG file system format) on a
 * directory (specified by path).
 */
#ifdef	FULLDUX
mountcdfs(rdev, site, path, vfsp)
dev_t	rdev;
site_t	site;
#else	not FULLDUX
mountcdfs(rdev, path, vfsp)
dev_t	rdev;
#endif	FULLDUX
char	*path;
struct vfs	*vfsp;
{
	register struct cdfs	*cdfsp;

	struct buf	*bp = 0;
	struct buf	*tp = 0;
	struct icddfs	*icddfsp;
	struct hcddfs	*hcddfsp;
	struct vnode	*dev_vp;
	struct	mount	*mp = 0;

	long	*lp;
	int	error = 0;
	int	x, y;
	u_int	vol_desc;
	u_char	vol_type;
	dev_t	dev = rdev;


        /*
         * If for some reason, the vfs is not set up as a read-only file
         * system, we return now.  This should not happen.
         */
	if (!(vfsp->vfs_flag & VFS_RDONLY)) {
		return(EROFS);
	}

        /*
         * We need to open the device to read in volume descriptors.
         */
#ifdef FULLDUX
#ifdef HPNSE
	error = opend(&dev, IFBLK, site, FREAD, 0, 0, 0);
#else  /* HPNSE */
	error = opend(&dev, IFBLK, site, FREAD, 0);
#endif /* HPNSE */
#else  /* FULLDUX */
#ifdef HPNSE
	error = opend(&dev, IFBLK, FREAD, 0, 0, 0);
#else  /* HPNSE */
	error = opend(&dev, IFBLK, FREAD, 0);
#endif /* HPNSE */
#endif /* FULLDUX */
	if (error) 
		return(error);
	
	dev_vp = devtovp(dev);
	
        /*
         * Since buffer management and device drivers are expecting blocks of
         * DEV_BSIZE (1k) size, we need to transform the address in sector to
         * DEV_BSIZE block.  The volume descriptors start at 16th sector from
         * the begining of the CD-ROM.
         */
	vol_desc = CDSBLOCK * 2;

        /*
         * We read volume descriptors one by one until either primary
         * volume descriptor or the volume terminator is encountered.
         * All other volume descriptors are ignored.
         */
	while (1) {
#ifdef	FSD_KI
		tp = bread(dev_vp, vol_desc, CDSBSIZE,B_ufs_mountroot|B_sblock);
#else	FSD_KI
		tp = bread(dev_vp, vol_desc, CDSBSIZE);  /* Read a vol. descp */
#endif	FSD_KI
		if (tp->b_flags & B_ERROR) 
			goto out;

                /*
                 * Since this disc could be either ISO-9660 or HSG, we set
                 * pointers for both of them.
                 */
		icddfsp = (struct icddfs *)(tp->b_un.b_addr);
		hcddfsp = (struct hcddfs *)icddfsp;

		/*
                 * Find out which format (ISO-9660 or HSG) by comparing
                 * the signature of the descriptors.
                 */
		if (!bcmp(icddfsp->cdrom_std_id, iso_fs_id, sizeof(iso_fs_id)-1)) {
			hcddfsp = 0;
			vol_type = icddfsp->cdrom_vold_type;
		} else if (!bcmp(hcddfsp->cdrom_std_id, hsg_fs_id, sizeof(hsg_fs_id)-1)) {
			icddfsp = 0;
			vol_type = hcddfsp->cdrom_vold_type;
		} else {
			goto out;
		}

		switch (vol_type) {
		case VOL_TERMINATE:
			goto out;
		case VOL_PRIMARY:
			goto goodvol;
		default:
			break;
		}

                /*
                 * If the descriptor read is neither a primary vol. desc. nor
                 * a vol. terminator, we release the buffer and prepare to
                 * read next one.
                 */
		brelse(tp);
		tp = 0;
		vol_desc += 2;
	}
	

goodvol:
	/*
	 * allocate a mount entry and check to see if device is already mounted
	 */
	mp = (struct mount *)kmem_alloc(sizeof(struct mount));
	if (getmp(dev)) {
		if (mp) {
                        /*
                         * This disc is mounted already, release the memory
                         * and return error.  If, in the future, we decide to
                         * support mount of more than one volume on the
                         * same disk,  this should be allowed.
                         */
			kmem_free((caddr_t)mp, sizeof(struct mount));
			mp = NULL;
		}
		error = EBUSY;
		goto out;
	}

	if (mp == NULL) {               /* Sorry, no memory available         */
		error = ENOMEM;
		goto out;
	}

        /*
         * Initialize the fields in mount point.
         */
	mp->m_flag = MINTER;
	mp->m_dev = dev;
	mp->m_rdev = rdev;
#ifdef	FULLDUX
	mp->m_site = site;
#else	not FULLDUX
	mp->m_site = my_site;
#endif	FULLDUX
	mountinshash(mp);

        /*
         * Attach a cdfs structure to the mount pointe.
         */
	bp = geteblk(sizeof(struct cdfs));
	mp->m_bufp = bp;

        /*
         * m_dfs is invalid for CD-ROM file systems so set it to an invalid
         * kernel address to guarantee a bus error if it is ever dereferenced.
         * For standalone systems, this line should be yanked.
         */
	mp->m_dfs = (struct duxfs *)INV_K_ADR;

        /*
         * Now we are ready to fill in the content of file system structure.
         */
	cdfsp = (struct cdfs *)bp->b_un.b_addr;
	vfsp->vfs_data = (caddr_t) mp;
	mp->m_vfsp = vfsp;
#ifdef CDCASE
	cdfsp->cdfs_flags = 0;
#endif
	
        /*
         * Since the structure defined is different between ISO-9660 and HSG,
         * we need to do it separately.
         */
	if (icddfsp) { /* ISO-9660 file system*/
		cdfsp->cdfs_type	= vol_type;
		cdfsp->cdfs_magic	= CDFS_MAGIC_ISO;
		cdfsp->cdfs_set_size	= icddfsp->cdrom_volset_siz_msb;
		cdfsp->cdfs_seq_num	= icddfsp->cdrom_volset_seq_msb;
		cdfsp->cdfs_pathtbl_siz	= icddfsp->cdrom_pathtbl_siz_msb;
		cdfsp->cdfs_sblkno	= vol_desc; /* in DEV_BSIZE*/
		cdfsp->cdfs_size	= icddfsp->cdrom_vol_size_msb;
		cdfsp->cdfs_sec_size	= SECTOR_SIZ;
		cdfsp->cdfs_lbsize	= icddfsp->cdrom_logblk_siz_msb;
		cdfsp->cdfs_lb	= (SECTOR_SIZ/(icddfsp->cdrom_logblk_siz_msb));
		cdfsp->cdfs_lstodb	= (CDSBSIZE/DEV_BSIZE)-1;
                /*
                 * SECTOR_SIZ and size of logic blocks have to be a power of 2.
                 */
		cdfsp->cdfs_scmask	= SECTOR_SIZ - 1;
		cdfsp->cdfs_lbmask = cdfsp->cdfs_lbsize - 1;

                /*
                 * To improve the performance later on, we compute the shift
                 * value of logic blocks, sectors and conversions of them.
                 * We assume logic block size is at least 512 bytes.
                 */
		for (x=9, y=(cdfsp->cdfs_lbsize>>9); (y & 1) == 0; x++, y>>=1)
				;   /* null loop */
		cdfsp->cdfs_lbshift = x;
	
		for (x=11, y=(cdfsp->cdfs_sec_size>>11); (y & 1) == 0; 
		     x++, y>>=1)
				;   /* null loop */

		cdfsp->cdfs_lsshift = x;
		cdfsp->cdfs_lbtolsc = x - cdfsp->cdfs_lbshift;

		cdfsp->cdfs_pathtblno = icddfsp->cdrom_pathtbl_loc_msb;


                /*
                 * copy the directory entry of "/" directory (root of the
                 * directory tree described by the volume descriptor) and
                 * the cdnode number of "/" to cdfs structure.
                 */
		bcopy(&(icddfsp->cdrom_rootdp), &(cdfsp->cdfs_rootdp), 
			sizeof(struct min_cddir));
		bcopy(cdfsp->cdfs_rootdp.mincdd_loc_msb,
			(caddr_t)&(cdfsp->cdfs_rootcdno), 4);

                /*
                 * Since the actual location of the directory entry for "/"
                 * is not what mincdd_loc says if there is XAR for "/" itself.
                 * In this case we need to skip the XAR to get the real
                 * location.  The cdnode of "/" is the address of "." directory
                 * entry in "/" (first directory entry) in bytes.
                 */
		cdfsp->cdfs_rootcdno += cdfsp->cdfs_rootdp.mincdd_xar_len;
		cdfsp->cdfs_rootcdno <<= cdfsp->cdfs_lbshift;
		bcopy(icddfsp->cdrom_vol_id, cdfsp->cdfs_vol_id, VOL_ID_SIZ);

                /*
                 * NFS requires a generaton number for each mounted file
                 * system, so we make one out of volume ID.
                 */
		lp = (long *) &(icddfsp->cdrom_vol_id)[0];
		x=0;
		for (y = VOL_ID_SIZ/sizeof(long); y>=0; y--, lp++) {
			x ^= *lp;
		}
		cdfsp->cdfs_gen=x;


		bcopy(icddfsp->cdrom_vol_set_id, cdfsp->cdfs_vol_set_id, 
			VOL_SET_ID_SIZ);
		bcopy(icddfsp->cdrom_copyright, cdfsp->cdfs_copyright, 
			CDMAXNAMLEN);
		bcopy(icddfsp->cdrom_abstract, cdfsp->cdfs_abstract, 
			CDMAXNAMLEN);
		bcopy(icddfsp->cdrom_bibliographic, cdfsp->cdfs_bibliographic, 
			CDMAXNAMLEN);

	} else {	/*HSG file system*/
		cdfsp->cdfs_type	= vol_type;
		cdfsp->cdfs_magic	= CDFS_MAGIC_HSG;
		cdfsp->cdfs_set_size	= hcddfsp->cdrom_volset_siz_msb;
		cdfsp->cdfs_seq_num	= hcddfsp->cdrom_volset_seq_msb;
		cdfsp->cdfs_pathtbl_siz	= hcddfsp->cdrom_pathtbl_siz_msb;
		cdfsp->cdfs_sblkno	= vol_desc; /* in DEV_BSIZE*/
		cdfsp->cdfs_size	= hcddfsp->cdrom_vol_size_msb;
		cdfsp->cdfs_sec_size	= SECTOR_SIZ;
		cdfsp->cdfs_lbsize	= hcddfsp->cdrom_logblk_siz_msb;
		cdfsp->cdfs_lb	= SECTOR_SIZ/(hcddfsp->cdrom_logblk_siz_msb);
		cdfsp->cdfs_lstodb	= (CDSBSIZE/DEV_BSIZE)-1;

                /*
                 * SECTOR_SIZ and size of logic blocks have to be a power of 2.
                 */
		cdfsp->cdfs_scmask	= SECTOR_SIZ - 1;
		cdfsp->cdfs_lbmask = cdfsp->cdfs_lbsize - 1;


                /*
                 * To improve the performance later on, we compute the shift
                 * value of logic blocks, sectors and conversions of them.
                 * We assume logic block size is at least 512 bytes.
                 */
		for (x=9, y=(cdfsp->cdfs_lbsize>>9); (y & 1) == 0; x++, y>>=1)
				;   /* null loop */ 
		cdfsp->cdfs_lbshift = x;

		for (x=11, y=(cdfsp->cdfs_sec_size>>11); (y & 1) == 0; 
		     x++, y>>=1)
				;   /* null loop */
		cdfsp->cdfs_lsshift = x;

		cdfsp->cdfs_lbtolsc = x - cdfsp->cdfs_lbshift;

		cdfsp->cdfs_pathtblno = hcddfsp->cdrom_pathtbl_loc_msb;

                /*
                 * copy the directory entry of "/" directory (root of the
                 * directory tree described by the volume descriptor) and
                 * the cdnode number of "/" to cdfs structure.
                 */
		bcopy(&(hcddfsp->cdrom_rootdp), &(cdfsp->cdfs_rootdp), 
			sizeof(struct min_cddir));
		bcopy(cdfsp->cdfs_rootdp.mincdd_loc_msb,
			(caddr_t)&(cdfsp->cdfs_rootcdno), 4);

                /*
                 * Since the actual location of the directory entry for "/"
                 * is not what mincdd_loc says if there is XAR for "/" itself.
                 * In this case we need to skip the XAR to get the real
                 * location.  The cdnode of "/" is the address of "." directory
                 * entry in "/" (first directory entry) in bytes.
                 */
		cdfsp->cdfs_rootcdno += cdfsp->cdfs_rootdp.mincdd_xar_len;
		cdfsp->cdfs_rootcdno <<= cdfsp->cdfs_lbshift;
		bcopy(hcddfsp->cdrom_vol_id, cdfsp->cdfs_vol_id, VOL_ID_SIZ);

                /*
                 * NFS requires a generaton number for each mounted file
                 * system, so we make one out of volume ID.
                 */
		lp = (long *) &(hcddfsp->cdrom_vol_id)[0];
		x=0;
		for (y = VOL_ID_SIZ/sizeof(long); y>=0; y--, lp++) {
			x ^= *lp;
		}
		cdfsp->cdfs_gen=x;

		bcopy(hcddfsp->cdrom_vol_set_id, cdfsp->cdfs_vol_set_id, 
			VOL_SET_ID_SIZ);
		bcopy(hcddfsp->cdrom_copyright, cdfsp->cdfs_copyright, 
			CDMAXNAMLEN);
		bcopy(hcddfsp->cdrom_abstract, cdfsp->cdfs_abstract, 
			CDMAXNAMLEN);

                /*
                 * There is no Bibliographic ID in HSG.
                 */
		bcopy("                             ", 
		      cdfsp->cdfs_bibliographic, CDMAXNAMLEN);
	}

	vfsp->vfs_bsize = CDSBSIZE;
	brelse(tp);
	tp = 0;

        /*
         * Sanity check to make sure there are cdnode allocated before we
         * allow this mount to succeed.
         */
	if (cdnode == cdnodeNCDNODE) {
		printf("mountcdfs: no cdnode space allocated\n");
		error = ENFILE;
		goto out;
	}

        /*
         * We only support single volume volume set.
         */
	if (cdfsp->cdfs_set_size != cdfsp->cdfs_seq_num) 
		goto out;

	mp->m_flag = MINUSE;
#ifdef	GETMOUNT
	(void) strncpy(cdfsp->cdfs_cdfsmnt, path, sizeof(cdfsp->cdfs_cdfsmnt));
#endif	GETMOUNT
	if (error = broadcast_mount(vfsp)) {
		mp->m_flag = MAVAIL;
		goto out;
	}

	vfsp->vfs_fsid[0] = (long)mp->m_dev;
	vfsp->vfs_fsid[1] = MOUNT_CDFS;

	VN_RELE(dev_vp);
	return(0);		/* we did it. */
out:
        /*
         * Something is wrong, we abort the mount.
         */
	closed(dev, IFBLK, ~FREAD);	/*FREAD=VFS_RDONLY=M_RDONLY=1*/
	if (mp) {
		mountremhash(mp);
		kmem_free((caddr_t)mp, sizeof(struct mount));
	}

	if (bp) 
		brelse(bp);

	if (tp) 
		brelse(tp);

	VN_RELE(dev_vp);
	if (!error)	
		error = EBUSY;

	return(error);
}


/******************************************************************************
 * cdfs_unmount()
 *
 * Unmount a CDFS file system.
 */
cdfs_unmount(vfsp)
struct	vfs	*vfsp;
{
	register struct mount *mp;
#ifdef __hp9000s300
	register int	stillopen;
#endif /* hp9000s300 */
#ifndef lint
	register int	i;		/* work around for compiler bug */
#endif /* not lint */

	struct buf	*bp;
	struct vnode	*dev_vp;
	dev_t		dev;

	mp = (struct mount *)vfsp->vfs_data;
	dev = mp->m_dev;

        /*
         * Lock out multiple umount's.
         */
	if (mp->m_flag & MUNMNT) 
		return(EINVAL);
	mp->m_flag |= MUNMNT;	/* Lock out multiple umount's */

        /*
         * Before un-mount the file system, we check to see if there is any
         * reference to it.  If there is any reference, we refuse to do it.
         */
#ifdef __hp9000s300
	if ((stillopen = cdflush(dev)) < 0)
#else /* hp9000s300 */
	if (cdflush(dev) < 0)
#endif /* hp9000s300 */
	{
		mp->m_flag &= ~MUNMNT;
		return (EBUSY);
	}

        /*
         * First, remove from mount table so no one can access this file
         * system.
         */
	bp = mp->m_bufp;
	dev_vp = devtovp(dev);
	mountremhash(mp);

	kmem_free((caddr_t)mp, sizeof(struct mount));

        /*
         * Purge any pages that have reference to this device.
         */
	mpurge(dev_vp);
	brelse(bp);
	dev_vp = devtovp(dev);

#if ! defined(FULLDUX)
	/*
	 * We must send the unmount before closing the device,
	 * so the device index can be computed.
	 */
	global_unmount(dev);
#endif	!FULLDUX
	/*
	 *  Call closed() straight off, since there may be remote sites
	 *  using this device as well, and the device count table will
	 *  take care of everything
	 */
#ifdef FULLDUX
	closed(dev, IFBLK, ~FREAD, my_site);
#else
	closed(dev, IFBLK, ~FREAD);	/*FREAD=VFS_RDONLY=M_RDONLY=1*/
#endif
	VN_RELE(dev_vp);
	return (0);
}


/*****************************************************************************
 * send_cdfs_mount()
 *
 * This is for umount from a diskless node.  This release doesn't support
 * local mount, we print error message to console and return.
 */
/*ARGSUSED*/
send_cdfs_mount(dev, site, vfsp, path)
dev_t	dev;
site_t	site;
struct vfs	*vfsp;
char	*path;
{
	printf("send_cdfs_mount: local mount is not yet supported");
	return(EINVAL);
}


/*
 * Given the file system and a file id (i.e. the cdnode number), generate
 * a vnode.  Primarily used by the NFS 3.2 file handle code to look up
 * the vnode for a file handle.
 */

cdfs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
#ifdef __hp9000s800
        struct cdfid sucdfid;
        register struct cdfid *cdfid = &sucdfid;
#else
	register struct cdfid *cdfid;
#endif
	register struct cdnode *cdp;
	register struct mount  *mp;

	mp = (struct mount *)vfsp->vfs_data;
#ifdef __hp9000s800
        bcopy((caddr_t)fidp, (caddr_t)cdfid, sizeof(struct cdfid));
#else
	cdfid = (struct cdfid *)fidp;
#endif
	cdp = cdget(mp->m_dev, (struct cdfs *)mp->m_bufp->b_un.b_fs,
		    cdfid->cdfid_cdno, 0, 0);

	if (cdp == NULL) {
		*vpp = NULL;
		return (0);
	}

        /*
         * If the disk has been swapped (umount the original and mount another
         * disk), we drop the cdnode and let the caller handle the situation.
         */
	if (cdp->cd_fs->cdfs_gen != cdfid->cdfid_gen) {
		cddrop(cdp);
		*vpp = NULL;
		return (0);
	}

        /*
         * Don't forget to unlock the vnode, since cdget lock it.
         */
	cdunlock(cdp);
	*vpp = CDTOV(cdp);
	return (0);
}


#ifdef GETMOUNT

int
cdfs_getmount(vfsp, fsmntdir, mdp)
struct vfs *vfsp;
caddr_t fsmntdir;
struct mount_data *mdp;
{
	struct mount *mp;
	struct cdfs *cdfsp;
	unsigned int l;

	mp = (struct mount *)vfsp->vfs_data;

	/* 
	 * only modify fields specific to CDFS here 
	 */
	mdp->md_msite = mp->m_site;
	mdp->md_dev = mp->m_dev;
	mdp->md_rdev = mp->m_rdev;

	if (bdevrmt(mp->m_dev))
		cdfsp = (struct cdfs *)mp->m_dfs;
	else
		cdfsp = (struct cdfs *)mp->m_bufp->b_un.b_fs;

	return(copyoutstr(cdfsp->cdfs_cdfsmnt, fsmntdir, MAXPATHLEN, &l));

}
#endif /* GETMOUNT */


/*
 * declarations for the routines to be inserted into the cdfsproc table.
 */

	int cdunlock();
	int pkrmcd();
	int stprmcd();
	int send_ref_update_cd();
	int send_ref_update_no_cdno();
	int cdhinit();
	int duxcd_pseudo_inactive();
	int close_send_no_cdno();
	int closecd_serve();
	int ecdget();
	int cdflush();
	int cdfs_getattr();

	/* added for test - brucet */
	int servestratreadcd();
	int cdno_cleanup();

	int cdfs_strategy();
	int duxcd_strategy();

	int fsctlcd_serve();

/*
 * link routine for cdfs.  Must be called at boot time if cdfs is
 * configured into kernel.  This function should be enough to
 * dereference the rest of CDFS for linking at configuration time.
 * It will be referenced by conf.c, created from /etc/master.
 */

int
#if defined(__hp9000s300) || defined(_WSIO)
cdfs_link()
#else	/* hp9000s800 */
cdfs_init()
#endif	/* hp9000s300 */
{
	extern int cdfs_initialized;
	typedef int (*intfn)();

#ifdef _WSIO
	/*
	 * set up the vfssw so the file system knows about CDFS
	 */
	vfssw[MOUNT_CDFS] = &cdfs_vfsops;
#else
	/* on 800 - done by uxgen in conf.c */
#endif	/* _WSIO */


	/* 
	 * Now add the entries into the cdfsproc table
	 */


	cdfsproc[CDFIND]			= (intfn)cdfind;
	cdfsproc[CDGET]				= (intfn)cdget;
	cdfsproc[CDUNLOCK_PROC]			= cdunlock;
	cdfsproc[PKRMCD]			= pkrmcd;
	cdfsproc[STPRMCD]			= stprmcd;
	cdfsproc[SEND_REF_UPDATE_CD]		= send_ref_update_cd;
	cdfsproc[SEND_REF_UPDATE_NO_CDNO]	= send_ref_update_no_cdno;
	cdfsproc[CDHINIT]			= cdhinit;
	cdfsproc[DUXCD_PSEUDO_INACTIVE]		= duxcd_pseudo_inactive;
	cdfsproc[CLOSE_SEND_NO_CDNO]		= close_send_no_cdno;
	cdfsproc[CLOSECD_SERVE ]		= closecd_serve;
	cdfsproc[ECDGET ]			= ecdget;
	cdfsproc[CDFLUSH]			= cdflush;
	cdfsproc[CDFS_GETATTR]			= cdfs_getattr;

/* Added for a test */
	cdfsproc[SERVESTRATREADCD]		= servestratreadcd;
	cdfsproc[CDNO_CLEANUP]			= cdno_cleanup;
	cdfsproc[CDFS_STRATEGY]			= cdfs_strategy;
	cdfsproc[DUXCD_STRATEGY]		= duxcd_strategy;
	cdfsproc[FSCTLCD_SERVE]			= fsctlcd_serve;

	cdfs_initialized = 1;
}
