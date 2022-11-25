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

/******************************************************************************
**
**      Filename:       ufs_pmount.c
**      Purpose:        The purpose of this routine is to physically mount
**                      and unmount a surface in such a way that the the
**                      surface is flushed and marked clean and the super-
**                      block is update. This is done so there  will be
**                      no need to fsck all the surfaces that are mounted
**                      in the file system only those that are loaded in
**                      the drives.
**
**
******************************************************************************/

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
#include "../h/acdl.h"
#include "../h/kern_sem.h"

/**---------------------------------------------------------------------------
		Public Function Prototypes
---------------------------------------------------------------------------**/
/*
int ufs_pmount(struct vfs *vfs, dev_t drive_dev,
               dev_t surface_dev, struct buf *bp);
int ufs_punmount(struct vfs *vfs, dev_t drive_dev, dev_t surface_dev,
                 struct buf *bp, struct ac_deadlock_struct *dlp);
*/

/**---------------------------------------------------------------------------
                Private Function Prototypes
---------------------------------------------------------------------------**/
/*
void write_superblock(struct mount *mp, dev_t dev,
                     void (*strat)() strat, struct buf *bp);
int ok_to_mark_clean(dev_t dev);
*/

/*
** write_superblock(mount *mp, dev_t dev, void (*strat)() strat,buf *bp);
** Parameters:
**      *mp     pointer to the mount table that is allocated for a specific
**              surface.
**      dev     the device that the superblock will be written on.
**      strat   the disk strategy that will write the superblock info
**              to the device.
**      bp      pointer to the buffer that keeps the superblock info.
**
** Globals:
** Operation:
**	This routine writes the superblock and cylinder group information
**	to the device (surface) and marks that surface as unmodified.
**	This is done so that if a power failure occurs only this surfaces
**	in the drives will be marked unclean and will need to be fsck'ed.
**	The other surfaces will be marked unmodified and will not need to
**	be fsck'ed.  The strat is a different strategy so it won't block
**	other requests while the disk is frozen.
** Return:
**
*/

extern int numdirty;

void
write_superblock(mp,dev,strat,bp)
	struct mount *mp;
	dev_t dev;
	void (*strat)();
	struct buf *bp;
{
	/* pointer to the filesystem superblocks in the buf structure
           this points to the mounted file system superblock */

	register struct fs *fs = mp->m_bufp->b_un.b_fs;

	int blks;	/* number of DEV_BSIZE blocks of data */
	int chunk;	/* number of DEV_BSIZE blocks per write */
	caddr_t space;	/* used for cylinder group updating */
	int i, size;	/* counter variables */

	sv_sema_t ss;	/* used with mulitple processor machines */


	/* setup buffer to write out the superblock information */
	bp->b_bcount = (int)fs->fs_sbsize; 
	bp->b_dev   = dev;
	bp->b_blkno = SBLOCK;
#ifdef _WSIO
	bp->b_offset = SBLOCK << DEV_BSHIFT;
#endif /* _WSIO */
	bp->b_resid = 0;
	bp->b_error = 0;

	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
	bp->b_flags = B_BUSY | B_FSYSIO;

	p_io_sema(&ss);

	/* use the new disk strategy to write superblock info to the disk */
	(*strat)(bp);

	v_io_sema(&ss);
	biowait(bp);

	/* setup buffer to write out the cylinder block information */
	blks = btodb(fs->fs_cssize);
	space = (caddr_t)fs->fs_csp[0];
	chunk = btodb(bp->b_bufsize);
	for (i = 0; i < blks; i += chunk) {
		size = chunk;
		if (i + chunk > blks)
			size = blks - i;

		bp->b_bcount = size;
		bp->b_dev   = dev;
		bp->b_blkno = (daddr_t)fsbtodb(fs, fs->fs_csaddr) + i;
#ifdef _WSIO
		bp->b_offset = bp->b_blkno << DEV_BSHIFT;
#endif /* _WSIO */
		bp->b_resid = 0;
		bp->b_error = 0;

		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bp->b_flags = B_BUSY | B_FSYSIO;

		p_io_sema(&ss);

		/* use the new disk strategy to write cylinder info */
		(*strat)(bp);

		v_io_sema(&ss);
		biowait(bp);
	}
	/* set modification variable to zero dev not modified */
	fs->fs_fmod = 0;

} /* write_superblock */

/*
** ok_to_mark_clean(dev_t dev)
** Parameters:
**      dev     the surface being checked for any open file or pipes
**
** Globals:
**	inode	pointer to the inode table in the kernel
**	inodeNINODE  pointer to the end of the inode table in the kernel
**
** Operation:
**		loop through the inodes and check if the inode dev is the
**		surface to mark clean.  If it is the right surface check
**		to see if there are any unlinked-open files or pipes for
**		the device.  If so return 0
**		If no unlinked-open files or pipes are found return 1
**
** Return: 
**		zero (0) -> if there are any open files or pipes
**		one (1) -> no open files or pipes
*/

int 
ok_to_mark_clean(dev)
	dev_t dev;
{
	register struct inode *ip;

	for (ip = inode; ip < inodeNINODE; ip++) {
		if ((ip->i_dev == dev) && 
		    ((ip->i_flag & IREF) != 0) &&
		    (ip->i_nlink == 0 || (ip->i_mode & IFMT) == IFIFO)) {
			return(0); 
		}
			/* not ok to mark clean.
			   Found either an unlinked-open file or a pipe */
	}
	return(1); /* is ok to mark clean */

} /* ok_to_mark_clean */

/*
** ufs_pmount(vfs *vfsp, dev_t drive_dev, dev_t surface_dev, buf *bp)
** Parameters:
**	vfsp pointer to the virtual file system for each mounted
**	     file system.
**	drive_dev device for the drive where the surface is located
**		  so the superblock can be updated.
**	surface_dev ??? who knows ???
**	bp	the buffer that will be used to write out the superblock	
**
** Globals:
** Operation:
**	When a mounted cartridge is inserted into a drive it is physically
**	mounted by updating the superblock, cylider groups, and marked
**	clean.
**	
**
** Return: always returns 0.
*/

int
ufs_pmount(vfsp,drive_dev,surface_dev,bp)
	struct vfs *vfsp;
	dev_t drive_dev,surface_dev;
	struct buf *bp;

{
	/* ptr to the mount struct where the dev is 
	   mounted in the file system */
	struct mount *mp = (struct mount *)vfsp->vfs_data; 
	struct fs *fsp = mp->m_bufp->b_un.b_fs; 

#ifdef hp9000s800
	sv_sema_t	ss;
	PXSEMA(&filesys_sema, &ss);
#endif

	if (!(vfsp->vfs_flag & VFS_RDONLY)) {
		if ((fsp->fs_clean == FS_CLEAN) || (fsp->fs_clean == FS_OK)) {
			fsp->fs_clean = FS_OK;
			write_superblock(mp,drive_dev,bdevsw[major(drive_dev)].d_strategy,bp); 
			}
		}

#ifdef hp9000s800
	VXSEMA(&filesys_sema, &ss);
#endif
	return(0);

} /* ufs_pmount */

/*
** ufs_punmount(vfs *vfsp, dev_t drive_dev, dev_t surface_dev, 
**		buf *bp, ac_deadlock_struct *dlp)
** Parameters:
**	vfsp pointer to the virtual file system for each mounted
**	     file system.
**	drive_dev device for the drive where the surface is being 
**		  removed from.
**	surface_dev device for the surface that is being removed from
**		    the drive.
**	bp	the buffer that will be used to write out the superblock	
**	dlp	structure used to check for a deadlock condition within
**		the requests for the autochanger.
**
** Globals:
** Operation:
**	This routine is called by the autochanger driver and 
**	will update the superblock information for the	mounted 
**	surface when it is removed from the drive.  A timeout
**	routine is used to avoid a deadlock while the surface is
**	being unmounted from the drive.  This routine freezes
**	the surface, flushes all the buffers to the surface, makes
**	sure the device is not deadlocked, marks the surface clean,
**	writes out the superblock information and unfreezes the device.
**
** Return: always returns 0.
*/

int
ufs_punmount(vfsp,drive_dev,surface_dev,bp,dlp)
	struct vfs *vfsp;
	dev_t drive_dev,surface_dev;
	struct buf *bp;
	struct ac_deadlock_struct *dlp;

{
	struct mount *mp = (struct mount *)vfsp->vfs_data;
	struct fs *fsp = mp->m_bufp->b_un.b_fs; 
	struct vnode *dev_vp;
        int pri;

#ifdef hp9000s800
	sv_sema_t	ss;
	PXSEMA(&filesys_sema, &ss);
#endif

	/* only do anything if mounted read-write */
	if (!(vfsp->vfs_flag & VFS_RDONLY)) {
		/* Start the deadlock avoidance timer */
		pri=spl6();
		if (dlp->flags & DL_TIMER_ON)
		   panic("UFS_PUNMOUNT:  restarting running timer");
		else {
		   timeout(dlp->recover_func,(caddr_t)dlp,HZ*DEADLOCK_TIMEOUT,NULL);
		}
		dlp->flags |= DL_TIMER_ON;
		splx(pri);

		/* Clear the dnlc to avoid deadlocking on the freezedev call */
		dnlc_purge_dev(surface_dev);

		/* Should also purge the async_unlinks waiting for this device. */

		/* freeze the device */
		/* Note: not all operations are blocked.  Only those
		   that can corrupt the file system. */
		dev_vp = devtovp(surface_dev);
		freeze_dev(dev_vp);

		/* flush the buffer.  Causes all requests to 
		   be scheduled to the autochanger. */
		bflush(dev_vp);

		/* should flush the driver before updating the superblock */

                pri = spl6(); /* critical code */
		if (dlp->flags & DEADLOCKED) { 
			/* the deadlock recovery sequence was started */
			dlp->flags |= RECOVERED;
			sleep(dlp);
			dlp->flags &= ~(DEADLOCKED | RECOVERED); /* reset the flags */
			/* now it is safe to write the superblock */
		} 
		if (dlp->flags & DL_TIMER_ON) {
		   untimeout(dlp->recover_func,(caddr_t)dlp);
		   dlp->flags &= ~DL_TIMER_ON;
		   }
		if (ok_to_mark_clean(surface_dev)) {
			/* mark incore copy clean */
			if (fsp->fs_clean == FS_OK) {
				fsp->fs_clean = FS_CLEAN;
			}
			write_superblock(mp,drive_dev,bdevsw[major(drive_dev)].d_strategy,bp); 
		}
                splx(pri); /* end of critical code */

		/* unfreeze it */
		unfreeze_dev(dev_vp);
		VN_RELE(dev_vp);
	}

#ifdef hp9000s800
	VXSEMA(&filesys_sema, &ss);
#endif
	return(0);

} /* ufs_punmount */

