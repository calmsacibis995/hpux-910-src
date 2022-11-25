/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_subr.c,v $
 * $Revision: 1.28.83.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/01/04 15:21:41 $
 */

/* HPUX_ID: @(#)ufs_subr.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988, 1988 Hewlett-Packard Company.
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


#ifdef _KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/conf.h"
#include "../dux/dux_dev.h"
#include "../dux/duxfs.h"

#else not _KERNEL
#include <sys/types.h>
#include <sys/param.h>
#include <sys/fs.h>
#endif
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"


#ifdef __hp9000s800
#define INV_K_ADR NULL
#endif

#ifdef __hp9000s800
/*** temporary - fix this for good if we make ufs configurable ***/
ufs_init() {}  /* for subsystem config */
/*** end temp ***/
#endif

#ifdef _KERNEL
#if (defined(__hp9000s300) || defined(_WSIO)) && defined(OSDEBUG)
int	syncprt = 0;
extern void bufstats();
#endif


/*
 * Processes running update must normally get the update lock,
 * ensuring that executions of update are serialized.
 */
#define	UPDL_BUSY 1
#define UPDL_WANTED 2

lock_update() 
{
	while (updlock & UPDL_BUSY) { 
	        updlock |= UPDL_WANTED;
		sleep ((caddr_t)&updlock, PRIBIO); 
	}
	updlock |= UPDL_BUSY;
}

unlock_update()
{
	updlock &= ~UPDL_BUSY;
	if (updlock & UPDL_WANTED) {
	        updlock &= ~UPDL_WANTED;
		wakeup((caddr_t)&updlock);
	}
}

#define update_locked() (updlock & UPDL_BUSY)

#ifdef	NSYNC
updlock_locked()
{
	return(update_locked());
}

updlock_set()
{
	updlock |= UPDL_BUSY;
}
#endif	/* NSYNC */



/*
 * Update is the internal name of 'sync'.  It goes through the disk
 * queues to initiate sandbagged IO; goes through the inodes to write
 * modified nodes; and it goes through the mount table to initiate
 * the writing of the modified super blocks.
 */
#ifdef	NSYNC
update(shutdown,force,syncer)
#else
update(shutdown,force)
#endif	/* NSYNC */
int shutdown;
int force;	/*see comments below*/
#ifdef	NSYNC
int syncer;
#endif	/* NSYNC */
{
	register struct inode *ip;
	register struct mount *mp;
	register struct mounthead *mhp;
	struct fs *fs;

	extern char *panicstr;	/* if non-NULL don't take KPREEMPTPOINTs
				 * because we don't want to let spl level
				 * go as low as splpreemptok while panicking.
				 */
#if (defined(__hp9000s300) || defined(_WSIO)) && defined(OSDEBUG)
	if (syncprt)
		bufstats();
#endif

	if (panicstr == NULL)
		/*
		 * not panicking -- ok to let preemptions in
		 */
    		KPREEMPTPOINT();
	/* Serialize calls on update. Previous solution
        ** of returning immediately allowed unmount to go
	** ahead and invalidate buffer pointers while the
	** previous call to update was still sleeping.
	**
	** The fix of serializing all calls to update caused multiple
	** concurrent sync's to run very slowly, breaking some benchmarks.
	** To fix this, the parameter force has been added.  If set, we
	** will still serialize.  If not set, we will return if the lock
	** is held.  This routine should always be called with force set
	** except from the sync system call itself.
	*/
	if (update_locked() && !force)
		return;
	if (update_locked() && shutdown == 2)
		return;		/* see machdep.c: boot() */

	lock_update();
	/*
	 * Write back modified superblocks.
	 * Consistency check that the superblock
	 * of each file system is still in the buffer cache.
	 * Note that the following code could have a race condition but
	 * doesn't.  While writing out the superblock, the device could be
	 * unmounted, and the linked list traversal would break.  However
	 * this race condition doesn't actually exist, because the
	 * lock on update prevents an unmount from taking place while the
	 * update is in progress.  (Note that an update can start while
	 * the unmount is already taking place.  However that is safe
	 * too because the unmount will have already written out the
	 * superblock, and we won't need to.  Thus, we won't sleep.)
	 */
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
		     mp = mp->m_hforw)
		{
			if ((mp->m_flag & MINUSE) == 0 || mp->m_dev == NODEV ||
			    (mp->m_vfsp->vfs_mtype != MOUNT_UFS))
				continue;
			if (bdevrmt(mp->m_dev))
				continue;
			fs = mp->m_bufp->b_un.b_fs;
			if (shutdown == 1 && !fs->fs_ronly) {
				if (fs->fs_clean == FS_OK) {
					fs->fs_clean = FS_CLEAN;
					fs->fs_fmod = 1;
				}
#ifdef  QUOTA
				if (FS_QFLAG( fs) == FS_QOK) {
					qsync( mp);
					FS_QSET( fs, FS_QCLEAN);
					fs->fs_fmod = 1;
				}
#endif  /* QUOTA */
			}
			if (fs->fs_fmod == 0)
				continue;
			if (fs->fs_ronly != 0) {		/* XXX */
				printf("fs = %s\n", fs->fs_fsmnt);
				panic("update: rofs mod");
			}
			fs->fs_fmod = 0;
			fs->fs_time = time.tv_sec;
#ifdef  AUTOCHANGER
			sbupdate(mp,shutdown);
#else
			sbupdate(mp);
#endif  /* AUTOCHANGER */
		}
	}
	if (panicstr == NULL)
		/*
		 * not panicking -- ok to let preemptions in
		 */
    		KPREEMPTPOINT();
	/*
	 * Write back each (modified) inode.
	 */
	for (ip = inode; ip < inodeNINODE; ip++) {
		if (ilocked(ip) || (ip->i_flag & IREF) == 0 ||
		    (ip->i_flag & (IACC | IUPD | ICHG)) == 0 ||
		    remoteip(ip))
			continue;
		/* slight MP race here */
		ilock(ip);
#ifdef ITRACE
		itrace (ip,caller(), 1);
		timeout (ilpanic, ip, 10*hz);
#endif ITRACE
		VN_HOLD(ITOV(ip));
		if (panicstr == NULL)
			/*
			 * not panicking -- ok to let preemptions in
			 */
			KPREEMPTPOINT();
		iupdat(ip, 0, 0);
		iput(ip);
	}
	/*
	 * Force stale buffer cache information to be flushed,
	 * for all devices.
	 */
#ifdef	NSYNC
	if (syncer)
		tflush();
	else
		bflush((struct vnode *) 0);
#else
	bflush((struct vnode *) 0);
#endif	/* NSYNC */

	unlock_update();
}




/* write out the inode's indirect blocks synchronously */
/* only do this if we didn't flush the entire cache in syncip */
/* note that this won't get the last triple indirect block, */
/* but we don't need to worry about it yet */
int
sync_indirects(ip)
	struct inode *ip;
{

    int i, j;
    register struct buf *bp, *nbp;
    register struct fs *fs;
    daddr_t *bap;


    if ((ip->i_mode & IFMT) != IFREG)   /*  don't want to do this for  */
        return(0);                      /*  things like FIFOs          */

    fs = ip->i_fs;
    for (i = 0; i < NIADDR; i++) {
        /* this writes out the first indirect blocks for single,
           double and triple indirect */
	if ( ip->i_ib[i] != 0 ) {
	    if (incore(ip->i_devvp, (daddr_t)fsbtodb(fs, ip->i_ib[i]))) {
		bp = getblk(ip->i_devvp, (daddr_t)fsbtodb(fs, ip->i_ib[i]),
#ifdef  FSD_KI
			    (int)fs->fs_bsize, B_indbk);
#else   FSD_KI
			    (int)fs->fs_bsize);
#endif  FSD_KI
		if ((bp->b_flags & B_DONE) && (bp->b_flags & B_DELWRI)) {
			bp->b_flags |= B_SYNC;
			bwrite(bp);
		}
		else {
			brelse(bp);
		}
	    }
	    /* this writes out the second indirect block for double and
	       triple indirects. not writing out the triple indirects for
	       triples because we currently don't use triple indirects.
	       (we get 4 gig files with just double indirects */
	    if (i != 0) {
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, ip->i_ib[i]),
#ifdef  FSD_KI
			    (int)fs->fs_bsize, B_indbk);
#else   FSD_KI
			    (int)fs->fs_bsize);
#endif  FSD_KI
		bap = bp->b_un.b_daddr;
                for (j=0; j < (fs->fs_bsize / sizeof(daddr_t)); j++) {
                    if (bap[j] != 0) {
		        if (incore(ip->i_devvp, (daddr_t)fsbtodb(fs, bap[j]))) {
		            nbp = getblk(ip->i_devvp,
					(daddr_t)fsbtodb(fs, bap[j]),
#ifdef FSD_KI
			                (int)fs->fs_bsize, B_data);
#else  FSD_KI
			                (int)fs->fs_bsize);
#endif FSD_KI
		    	    if ((nbp->b_flags & B_DONE) &&
				(nbp->b_flags & B_DELWRI)) {
					nbp->b_flags |= B_SYNC;
					bwrite(nbp);
		    	    }
			    else {
					brelse(nbp);
			    }
		        }
	            }
	        }
		brelse(bp);
	    }
	}
    }
    return (0);
}



/*
 * Flush all the blocks associated with an inode.
 * there are two strategies based on the size of the file;
 * large files are those with more than (nbuf / 2) blocks
 * i.e., more than half the buffer cache.
 * Large files
 *      Walk through the buffer pool and push any dirty pages
 *      associated with the device on which the file resides.
 * Small files
 *      Look up each block in the file to see if it is in the
 *      buffer pool writing any that are found to disk.
 *     Note that we make a more stringent check of
 *      writing out any block in the buffer pool that may
 *      overlap the inode. This brings the inode up to
 *      date with recent mods to the cooked device.
 */
syncip(ip, ino_chg, syncio)
	register struct inode *ip;
	short  ino_chg;             /* 0 = don't update inode ICHG time
				       1 = update inode ICHG time
				       2 = don't update inode at all
				     */
	int syncio;		/* should update be synchronous? */
{
	register struct fs *fs;
	register long lbn, lastlbn;
	daddr_t blkno;
	dev_t dev;

	if (remoteip(ip)) {
		dux_syncip(ip);
		return;
	}
	if (syncio || fs_async < 2) {
		fs = ip->i_fs;
		if ((ip->i_mode & IFMT) != IFBLK)
			lastlbn = howmany(ip->i_size, fs->fs_bsize);
		else
			lastlbn = nbuf;
		if (lastlbn < (nbuf / 2)) {
			for (lbn = 0; lbn < lastlbn; lbn++) {
				blkno= fsbtodb(fs, bmap(ip,lbn,B_READ,0,0,0,0));
				blkflush(ip->i_devvp, blkno, 
						(long)blksize(fs, ip, lbn),
						0, /* don't purge */
						NULL);
			}
			sync_indirects(ip);
		} else {
			if ((ip->i_mode & IFMT) == IFBLK)
				dev = ip->i_rdev;
			else
				dev = ip->i_dev;
			syncip_flush_cache (dev, syncio);

		}
	}
	if (ino_chg == 1) {
		/* mark the inode as changed if called from fsync */
		imark(ip, ICHG);
	}
	iupdat(ip, syncio, 2);
}

#ifdef NEVER_CALLED
purgeip(ip)
	register struct inode *ip;
{
	register struct fs *fs;
	register long lbn, lastlbn;
	daddr_t blkno;

	fs = ip->i_fs;
	lastlbn = howmany(ip->i_size, fs->fs_bsize);
	for (lbn = 0; lbn < lastlbn; lbn++) {
		blkno = fsbtodb(fs, bmap(ip,lbn,B_READ,0,0,0,0));
		bpurge(ip->i_devvp, blkno);
	}
}
#endif /* NEVER_CALLED */

#endif

extern	int around[9];
extern	int inside[9];
extern	u_char *fragtbl[];

/*
 * Update the frsum fields to reflect addition or deletion 
 * of some frags.
 */
fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

#ifdef _KERNEL
/*
 * Check that a specified block number is in range.
 */
badblock(fs, bn)
	register struct fs *fs;
	daddr_t bn;
{

	if ((unsigned)bn >= fs->fs_size) {
		printf("bad block %d, ", bn);
		fserr(fs, "bad block");
		return (1);
	}
	return (0);
}
#endif

/*
 * block operations
 *
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("isblock");
		return (NULL);
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	u_char *cp;
	daddr_t h;
{

	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("clrblock");
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{

	switch (fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("setblock");
	}
}

struct mount *
getmp(dev)
	dev_t dev;
{
	register struct mount *mp;
	register struct fs *fs;
	register struct mount *mhp;	/*hash pointer*/

	mhp = MOUNTHASH(dev);

	for (mp = mhp->m_hforw; mp != mhp; mp = mp->m_hforw)
	{
		if ((mp->m_bufp == NULL && mp->m_dfs == NULL)
		    || mp->m_dev != dev)
			continue;
		if (!bdevrmt(dev))
		{
		if (mp->m_vfsp->vfs_mtype == MOUNT_UFS) {
			fs = mp->m_bufp->b_un.b_fs;

			if (fs->fs_magic != FS_MAGIC && 
			    fs->fs_magic != FS_MAGIC_LFN && 
			    fs->fs_magic != FD_FSMAGIC) {
				printf("dev = 0x%x, fs = %s\n", dev, fs->fs_fsmnt);
				panic("getmp: bad magic");
			}
			if (FSF_UNKNOWN(fs->fs_featurebits)) {
				printf("dev = 0x%x, fs = %s\n", dev, fs->fs_fsmnt);
				panic("getmp: bad feature bits");
			}
		}
		else if (mp->m_vfsp->vfs_mtype == MOUNT_CDFS) {
		   struct cdfs	*cdfs;
		   cdfs = (struct cdfs *) mp->m_bufp->b_un.b_fs;
		   if (cdfs->cdfs_magic != CDFS_MAGIC_ISO && 
		       cdfs->cdfs_magic != CDFS_MAGIC_HSG) {
			printf("dev = 0x%x, cdfs volume id = %s\n", dev, 
			        cdfs->cdfs_vol_id);
			panic("getmp: bad magic ISO-9660/HSG");
		   }
		}
		}
		return (mp);
	}
	return (NULL);
}

/*
 * getrmp is identical to getmp except that it uses the real device and site.
 * It should only be called for a remote device.
 */
struct mount *
getrmp(dev,site)
	dev_t dev;
	site_t site;
{
	register struct mount *mp;
	register struct mount *mhp;	/*hash pointer*/

	mhp = MOUNTHASH(dev);

	for (mp = mhp->m_rhforw; mp != mhp; mp = mp->m_rhforw)
	{
		if ((mp->m_bufp == NULL && mp->m_dfs == NULL)
		    || mp->m_rdev != dev || mp->m_site != site)
			continue;
		return (mp);
	}
	return (NULL);
}

#ifdef _KERNEL

/*
 * Getmount is a front end to getmp that performs additional error checking.
 * panic: no mount -- the device is not mounted.
 * this "cannot happen"
 */
struct mount *
getmount(dev)
	register dev_t dev;
{
	register struct mount *mp;

	mp = getmp(dev);
	if (mp == NULL || !(mp->m_flag & MINUSE))
	{
		printf("dev = 0x%x\n", dev);
		panic("getmount: no mount");
	}
	return (mp);
}

/*
 * Getrmount is similar to getmount except that it uses a real device number
 * and site.
 */
struct mount *
getrmount(dev, site)
	register dev_t dev;
	register site_t site;
{
	register struct mount *mp;

	mp = getrmp(dev,site);
	if (mp == NULL || !(mp->m_flag & MINUSE))
	{
		printf("rdev = 0x%x, rsite=%d\n", dev, site);
		panic("getrmount: no mount");
	}
	return (mp);
}

#endif _KERNEL

