/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_inode.c,v $
 * $Revision: 1.40.83.6 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/19 13:48:07 $
 */

/* HPUX_ID: @(#)ufs_inode.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988, 1989 Hewlett-Packard Company.
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


#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/mount.h"
#include "../h/buf.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../nfs/nfs.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA
#include "../h/kernel.h"
#include "../h/pfdat.h"
#ifdef SAR
#include "../h/sar.h"
#endif	
#include "../h/kern_sem.h"

#include "../h/conf.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../dux/dmmsgtype.h"

extern nodev();

/* declared in space.h */
extern int INOHSZ;
extern int INOMASK;

extern union ihead {			/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[];

/* INOMASK must always be a power of 2 - 1 */
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))&INOMASK)

struct inode *ifreeh, **ifreet;

/*
 * Convert inode format to vnode types
 */
enum vtype iftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};
int vttoif_tab[] = {
	0, IFREG, IFDIR, IFBLK, IFCHR, IFLNK, IFSOCK, IFMT, IFIFO, IFNWK, IFDIR
};


/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
ihinit()
{
	register int i;
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	ip->i_vnode.v_data = (caddr_t)ip;
	ip->i_vnode.v_op = &ufs_vnodeops;
	ip->i_vnode.v_fstype = VUFS;
	ip->i_flag = IBUFVALID|IPAGEVALID;
	for (i = ninode; --i > 0; ) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
		ip->i_vnode.v_data = (caddr_t)ip;
		ip->i_vnode.v_op = &ufs_vnodeops;
		ip->i_vnode.v_fstype = VUFS;
		ip->i_flag = IBUFVALID|IPAGEVALID;
	}
	ip->i_freef = NULL;
}

/*
 * Find an inode if it is incore.
 * This is the equivalent, for inodes,
 * of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *
ifind(dev, ino)
	dev_t dev;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;

	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino==ip->i_number && dev==ip->i_dev)
			return (ip);
	return ((struct inode *)0);
}

/*
 * Return an empty inode.  Place it on the hash chain indicated by ih.  If
 * ih is NULL, put it on a hash chain by itself.
 * If there are no inodes on the free list we call dnlc_purge1() and
 * bufvpfree() to try to free up an inode.  These routines can, however
 * sleep.  If we were looking for a specific inode in iget, that inode
 * could be looked up by someone else while we are sleeping, and can
 * therefore be introduced into the inode cache when we wake up.  Since we
 * will be introducing ourselves into the inode cache, this will result in
 * two copies.  We therefore return -1 if we had to call these routines to
 * tell iget to research the cache.  Other callers, looking for an empty
 * inode will need to recall this routine upon receiving a return of -1.
 * ( dts: DSDa100660 )
 */
struct inode *
eiget(ih)
	register union  ihead *ih;
{
	register struct inode *ip;
	register struct inode *iq;

	if ((ip = ifreeh) == NULL) {
		while (ifreeh == NULL) {
			if (dnlc_purge1() == 0) {
				break;
			}
		}
		/* See if can free any inodes by releasing */
		/* the vnode associated with buffers on a free list */
		/* This is needed for the discless node because */
		/* a buffer is associated with the file's vnode on */
		/* the discless site */
		while (ifreeh == NULL) {
			if (bufvpfree() == 0) {
				break;
			}
		}
		if ((ip = ifreeh) == NULL) {
			tablefull("inode");
#ifdef SAR
			inodeovf++;
#endif
			u.u_error = ENFILE;
			return(NULL);
		} else
			return((struct inode *)-1);
	}
	if (iq = ip->i_freef)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	ip->i_freef = NULL;
	ip->i_freeb = NULL;
	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 */

	remque(ip);
	if (ih)
		insque(ip, ih);
	else {
		ip->i_back = ip;
		ip->i_forw = ip;
	}

	/* Check some assumptions about the inode.  Diskless assumes that
	 * the reference counts and sitemaps are empty and does not 
	 * initialize them.  It could be that this ionode will not be
	 * used for VDUX or VDUX_CDFS but we check anyway.
	*/

	VASSERT(ip->i_refcount.d_rcount == 0);
	VASSERT(ip->i_refcount.d_vcount == 0);
	VASSERT(ip->i_execdcount.d_rcount == 0);
	VASSERT(ip->i_execdcount.d_vcount == 0);

	VASSERT(ip->i_opensites.s_maptype == S_MAPEMPTY);
	VASSERT(ip->i_writesites.s_maptype == S_MAPEMPTY);
	VASSERT(ip->i_execsites.s_maptype == S_MAPEMPTY);
	VASSERT(ip->i_refsites.s_maptype == S_MAPEMPTY);
 
	/*
	 *	WARNING!!!! The inode cache is now in an
	 *	inconsistent state. This inode is not on the
	 *	free list but the IREF bit is not set. Look at
	 *	ieget() to see why this is a problem. Our 
	 *	CALLER MUST SET THE IREF bit and lock the inode
	 *	before sleeping. Locking should succeed without
	 *	sleeping because the inode was free. DSDe403479.
	 */
	return (ip);
}


/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode *
ieget(dev,mp,ino)
	dev_t dev;
	struct mount *mp;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct inode *iq;
loop:
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev) {
			if ((ip->i_flag&ILOCKED) != 0) {
				ip->i_flag |= IWANT;
				sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			ilock(ip);	/* Shouldn't sleep, because we have
					 * "pre-checked" if it's locked. */
			/*
			 *if inode is on free list, remove it 
			 */
			if ((ip->i_flag & IREF) == 0) {
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
				/* If the inode was taken from the free list,
				 * restore i_rdev to be i_device --
				 * device number could be meaningless
				 */
				ip->i_rdev = ip->i_device;
			}
			/*
			 * mark inode locked and referenced and return it.
			 */
			ip->i_flag |= IREF;
			VN_HOLD(ITOV(ip));
			return(ip);
		}
	/*
	 * Inode was not in cache.  Get fre inode slot for new inode.
	 */
	ip = eiget(ih);
	if (ip == NULL)
		return (NULL);
	else if (ip == (struct inode *)-1)
		goto loop;

	ip->i_flag = IREF;
	ilock(ip);
#ifdef ITRACE
	itrace (ip, caller(), 1);
	timeout (ilpanic, ip, 30*hz);
#endif ITRACE
	ip->i_dev = dev;
	if (ip->i_devvp) {
		VN_RELE(ip->i_devvp);
		ip->i_devvp = NULL;
	}
	ip->i_devvp = devtovp(dev);
	ip->i_diroff = 0;
	if (mp->m_bufp)
		ip->i_fs = mp->m_bufp->b_un.b_fs;
	else
		ip->i_fs = NULL;
	ip->i_dfs = mp->m_dfs;
	ip->i_number = ino;
	ip->i_lastr = -1;	/* encourage read ahead at block zero */
#ifdef	ACLS
	ip->i_contip = 0;
        /*
         * This does not need to be done if the path through iget is followed
         * but since the dux code calls ieget directly, this value needs to
         * be zeroed for empty inodes.  cwb
         */
        ip->i_contin = 0;
#endif	ACLS
#ifdef QUOTA
	dqrele(ip->i_dquot);
	ip->i_dquot = NULL;
#endif QUOTA
	return (ip);
}

#ifdef ACLS
struct inode * in_get();

struct inode *
iget(dev, mp, ino)
{
	register struct inode *ip;
	register struct inode *cip;

	/* For access control lists (which use a continuation inode to
	 * store the list) when we get the primary inode we will also
	 * need to get the continuation inode, unless it has already
	 * been gotten (ip->i_contip is set).
	 */

	ip = in_get(dev,mp,ino);

	if (ip != NULL && ip->i_contin && !ip->i_contip)
	{
		if ((cip = in_get(dev,mp,ip->i_contin)) == NULL)
		{
			/* if we can't get the continuation inode
			 * release the primary inode and return
			 * an error
			 */
			
			iunlock(ip);
			VN_RELE(ITOV(ip));
			return NULL;
		}
		iunlock(cip);
		ip->i_contip = cip;
	}
	if (ip != NULL)  {
		if (ip->i_contin == 0)
			ip->i_contip = 0;
		else
		if (ip->i_contin && ip->i_contip) {
			if ((ip->i_contip->i_mode & IFMT) != IFCONT)
				panic("iget: continuation inode is wrong file type; fsck the disc");
		}
	}
	return ip;
}

struct inode *
in_get(dev, mp, ino)
#else
struct inode *
iget(dev, mp, ino)
#endif
	dev_t dev;
	register struct mount *mp;
	ino_t ino;
{
	register struct inode *ip;
	register struct buf *bp;
	register struct dinode *dp;
	register struct vnode *vp;
	register struct fs *fs;

#ifdef SAR
	sysiget++;
#endif

	ip = ieget(dev,mp,ino);
	if (ip == NULL || ITOV(ip)->v_count > 0)
		return (ip);
	fs = mp->m_bufp->b_un.b_fs;
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, itod(fs, ino)), 
#ifdef	FSD_KI
	    (int)fs->fs_bsize, B_iget|B_inode);
#else	FSD_KI
		(int)fs->fs_bsize);
#endif	FSD_KI
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		/*
		 * the inode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'iput' will take care of putting it back on the free list.
		 */

		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * we also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		/*
		 * this is a bug fix to prevent an ifree: freeing free inode
		 * panic.  This occurs when an I/O error has happened and the
		 * link count on the inode is 0.  The iinactive code will
		 * see the 0 link count and try to return it to the inode
		 * free map when it already is in the inode free map
		 * ghs - 12/3/87
		 */
		iunlock(ip);
		ip->i_flag &= IBUFVALID|IPAGEVALID;
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
		return(NULL);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, ino);
#ifdef ACLS
	ip->i_icun.i_ic = dp->di_ic;
#else
	ip->i_ic = dp->di_ic;
#endif
	ip->i_rdev = dp->di_rdev;
	ip->i_mount = mp;
	vp = ITOV(ip);
	VN_INIT(vp, mp->m_vfsp, IFTOVT(ip->i_mode), ip->i_rdev);
	if (ino == (ino_t)ROOTINO) {
		vp->v_flag |= VROOT;
	}
	vp->v_op = &ufs_vnodeops;  /*dux may have changed it to dux_vnodeops*/
	vp->v_fstype = VUFS;  /*dux may have changed it to VDUX*/
	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode != 0)
		ip->i_dquot = getinoquota(ip);
#endif QUOTA
	return (ip);
}

/*
 * Unlock inode and vn_rele associated vnode
 */
iput(ip)
	register struct inode *ip;
{

	if (!ilocked(ip))
		panic("iput");
	iunlock(ip);
	VN_RELE(ITOV(ip));
}
/*
 * Check that inode is not locked and release associated vnode.
 */
irele(ip)
	register struct inode *ip;
{
	if (ilocked(ip))
		panic("irele");
	VN_RELE(ITOV(ip));
}

/*
 * Drop inode without going through the normal chain of unlocking
 * and releasing.
 */
idrop(ip)
	register struct inode *ip;
{
	register struct vnode *vp = &ip->i_vnode;

	if (!ilocked(ip))
		panic("idrop");
	iunlock(ip);
	/* the following locked segment is a bit long, but we'll
	** worry about that later - dbm
	*/
	SPINLOCK(v_count_lock);
	if (--vp->v_count == 0) {
		ip->i_flag &= IBUFVALID|IPAGEVALID;
		/*
		 * Put the inode back on the end of the free list.
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
	}
	SPINUNLOCK(v_count_lock);
}

/*
 * Vnode is no loger referenced, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
#ifdef ACLS

/* For access control lists (which use a continuation inode to
 * store the list) when we free the primary inode we will also
 * need to free the continuation inode and when we de-allocate
 * the primary inode we must de-allocate the continuation inode
 * by setting its link count to zero. If we had to release the
 * primary inode without getting the continuation inode (in iget)
 * then i_contin would have a value, but i_contip would not;
 * therefore we only check i_contip here. Since update could
 * lock the continuation inode we need to wait for the continuation 
 * inode lock before releasing the continuation inode. i_contip is
 * zeroed when releasing primary inode; i_contin is zeroed when
 * primary inode is de-allocated. We zero the vnode count ourselves
 * since we are going around the vnode layer.
 */
iinactive(ip)
	register struct inode *ip;
{
	register struct inode *cip;

	if (ilocked(ip))
		panic("ufs_inactive");

	if (cip = ip->i_contip)
	{
		if ((cip->i_mode & IFMT) != IFCONT)
		{
			panic("iinactive: continuation inode is wrong file type; fsck the disc");

			/* make it fairly safe to continue from a panic -
			 * just zero continuation inode fields and release
			 * the primary inode.
			 */

			ip->i_contin = 0;
			ip->i_contip = 0;
			in_inactive(ip);
			return;
		}

		ilock(ip);

		/* wait for inode to be available -make sure not doing update */
		ilock(cip);
		iunlock(cip);

		if (ip->i_nlink == 0)
		{	cip->i_nlink = 0;	/* de-allocate cont inode */
			ip->i_contin = 0;
#ifdef QUOTA
                        /* decrement quota count for the continuation inode */
                        /* The "real" inode's fields are passed in here,    */
                        /* since the continuation inode doesn't have all    */
                        /* the necessary fields.                            */
                        (void)chkiq(VFSTOM(ip->i_vnode.v_vfsp), ip, 
                                     ip->i_uid, 0);
#endif QUOTA
		}
		cip->i_vnode.v_count = 0;
		in_inactive(cip);
		ip->i_contip = 0;
		iunlock(ip);
	}
	in_inactive(ip);
}

/*
 * NOTE: any changes made to iinactive code must be duplicated
 * in the post_inactive routine to ensure the same functionality
 * with and without ASYNC_UNLINK defined
 */
in_inactive(ip)
	register struct inode *ip;
{
	int mode;

#else	/* not ACLS */
iinactive(ip)
	register struct inode *ip;
{
	int mode;
#endif	/* not ACLS */

	if (ilocked(ip))
		panic("ufs_inactive");

	/* Trying to trap code doing a VN_RELE when they shouldn't. cwb */
	VASSERT(ITOV(ip)->v_vfsmountedhere == (struct vfs *)0);

	if (ip->i_fs && ip->i_fs->fs_ronly == 0) {
		ilock(ip);
#ifdef ITRACE
		itrace(ip, caller(), 1);
		timeout(ilpanic, ip, 30*hz);
#endif
#ifdef QUOTA
		if (ip->i_nlink <= 0 && remoteip(ip)) 
                {  /* at client and thru with inode */
                   dqrele(ip->i_dquot);
		   ip->i_dquot = NULL;
                }
                else
#endif QUOTA
		if (ip->i_nlink <= 0 && !remoteip(ip)) {
			ip->i_gen++;
#ifdef  ACLS
			if ((ip->i_mode & IFMT) == IFCONT)
				bzero(ip->i_acl,sizeof(struct acl_tuple)*
					NOPTTUPLES);
			else
			itrunc(ip, (u_long)0);
#else
			itrunc(ip, (u_long)0);
#endif
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_device = 0;
			ip->i_rsite = 0;

			/*
			 * For fast symlinks, we must ensure that the
			 * i_flags field is set back to 0.
			 */
			ip->i_flags = 0;

			imark(ip, IUPD|ICHG);
			ifree(ip, ip->i_number, mode);
#ifdef QUOTA
			(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp),
			    ip, ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
#endif

		}
		else	
			if (
			     (ip->i_flag & IFRAG) &&
#ifdef ACLS
			     !( (ip->i_mode & IFMT) == IFCONT ) &&
#endif /* ACLS */
			     !remoteip(ip)
			   )
				frag_fit(ip); /* best fit the fragments */	

		if (ip->i_flag & (IUPD|IACC|ICHG))
			iupdat(ip, 0, 0);
		iunlock(ip);
	}
	ip->i_flag &= IBUFVALID|IPAGEVALID;

	/*
	 * Put the inode on the end of the free list.
	 * Possibly in some cases it would be better to
	 * put the inode at the head of the free list,
	 * (eg: where i_mode == 0 || i_number == 0)
	 * but I will think about that later .. kre
	 * (i_number is rarely 0 - only after an i/o error in iget,
	 * where i_mode == 0, the inode will probably be wanted
	 * again soon for an ialloc, so possibly we should keep it)
	 */
	if (ifreeh) {
		*ifreet = ip;
		ip->i_freeb = ifreet;
	} else {
		ifreeh = ip;
		ip->i_freeb = &ifreeh;
	}
	ip->i_freef = NULL;
	ifreet = &ip->i_freef;

	/* Check some assumptions about the inode.  Diskless assumes that
	 * the reference counts and sitemaps are empty and does not 
	 * initialize them.  It could be that this ionode will not be
	 * used for VDUX or VDUX_CDFS but we check anyway.
	*/

        VASSERT(ip->i_refcount.d_rcount == 0);
        VASSERT(ip->i_refcount.d_vcount == 0);
        VASSERT(ip->i_execdcount.d_rcount == 0);
        VASSERT(ip->i_execdcount.d_vcount == 0);

        VASSERT(ip->i_opensites.s_maptype == S_MAPEMPTY);
        VASSERT(ip->i_writesites.s_maptype == S_MAPEMPTY);
        VASSERT(ip->i_execsites.s_maptype == S_MAPEMPTY);
        VASSERT(ip->i_refsites.s_maptype == S_MAPEMPTY);

}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any is on, update the inode
 * with the current time.
 * If syncio is given, then must insure
 * i/o is done, so wait for write to complete.
 * If ordered, then must insure i/o order.  The method used to insure order
 * depends on the the value of fs_async.
 */
iupdat(ip, syncio, ordered)
	register struct inode *ip;
	int syncio;	/* must do i/o synchronously */
	int ordered;	/* 0: may be reordered at will
	            	   1: should keep i/o in order for recoverability
			   2: must keep i/o's in order for recoverability */
{
	register struct buf *bp;
	struct dinode *dp;
	register struct fs *fp;
	int file_type;
	int pipe_update = 0;
	extern char *panicstr;

	if (panicstr == NULL)
    		KPREEMPTPOINT();
	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG)) != 0) {
		/* Note:  The only way fp can be NULL is if the inode
		 * serving site has failed
		 */
		if (fp == NULL || fp->fs_ronly)
			return;
		if (remoteip(ip))
			return;
		/* don't write pipe inode out to disc */
		file_type = ip->i_mode & IFMT;
		if ((file_type == IFIFO) && (ip->i_nlink == 0))
			pipe_update = 1;
		else {
			bp = bread(ip->i_devvp,
			(daddr_t)fsbtodb(fp, itod(fp, ip->i_number)),
#ifdef	FSD_KI
				(int)fp->fs_bsize, B_iupdat|B_inode);
#else	FSD_KI
			(int)fp->fs_bsize);
#endif	FSD_KI
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return;
			}
		}
		if (panicstr == NULL)
	    		KPREEMPTPOINT();
		ip->i_flag &= ~(IUPD|IACC|ICHG);
		/* don't write pipe inode out to disc */
		if (pipe_update) 
			return;
		dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
#ifdef ACLS
		dp->di_ic = ip->i_icun.i_ic;
#else
		dp->di_ic = ip->i_ic;
#endif
		/* don't write fifo process info to disc. */
		if (file_type == IFIFO) {
		    int i;
			for (i = 0; i < NDADDR; i++)
			        dp->di_db[i] = 0;
			dp->di_ib[0] = 0;
			dp->di_ib[1] = 0;
			dp->di_ib[2] = 0;

		/* since we have don't write the data block info, we 
 	         * don't write the size or count data either */

			dp->di_size = 0;
			dp->di_blocks = 0;
		}
		bp->b_flags2 |= ip->i_ord_flags;
#ifdef	PF_ASYNC
		if (syncio && (dp->di_flags & IC_ASYNC) == 0) {
#else	
		if (syncio) {
#endif	
			bp->b_flags |= B_SYNC;
			bwrite(bp);
#ifdef	PF_ASYNC
		} else if (ordered && (dp->di_flags & IC_ASYNC) == 0) {
			bxwrite (bp, ordered == 1 ? 0 : 1);    /*  XXX   */
#else	
		} else if (ordered) {
			bxwrite (bp, ordered == 1 ? 0 : 1);
#endif	
		}
		else
			bdwrite(bp);
		ip->i_ord_flags = 0;
	}
	if (panicstr == NULL)
    		KPREEMPTPOINT();
}

/*
 * Mark the accessed, updated, or changed times in an inode
 * with the current (unique) time
 */
imark(ip, flag)
	register struct inode *ip;
	register int flag;
{
	struct timeval ut;

	if (ITOV(ip)->v_vfsp->vfs_flag & VFS_RDONLY)
		return;

	uniqtime(&ut);
	ip->i_flag |= flag;
	if (flag & IACC)
		ip->i_atime = ut;
	if (flag & IUPD)
		ip->i_mtime = ut;
	if (flag & ICHG) {
		ip->i_diroff = 0;
		ip->i_ctime = ut;
	}
}
#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */
/*
 * Truncate the inode ip to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
itrunc(oip, length)
	register struct inode *oip;
	u_long length;
{
	register i;
	register daddr_t lastblock;
	daddr_t bn, lastiblock[NIADDR];
	register struct fs *fs;
	register struct inode *ip;
	struct buf *bp;
	struct inode tip;
	long blocksreleased = 0, nblocks;
	long indirtrunc();
	int level, newblock;
	int bsize, size,  cg;
	daddr_t bno;
	extern daddr_t alloccg();

	int type;
        int offset, osize, count, s;
        daddr_t lbn;
        struct vnode *devvp;
	int hash_bno, hash_count, hash_i;

	type = oip->i_mode & IFMT;
	if (type != IFREG && type != IFDIR && type != IFLNK && type != IFNWK
		&& type != IFIFO)
		return(0);

	/* do not truncate file size of fifo if somebody currently using it */
	if (type == IFIFO && ((oip->i_frcnt !=0) || (oip->i_fwcnt != 0)))
		return(0);

	/* do not truncate file size of fifo if it's unreasonable */
	if (type == IFIFO && ((length < 0) || (length > PIPSIZ)))
		return(EINVAL);

	/*
	 * Fast symlinks --
	 *   fast symbolic links do not have any disk storage, the path
	 *   is stored within the inode.  All we need do is zap the
	 *   extra bytes in the path, update the inode and return.
	 *
	 * NOTE: This code must exist here, not within the later test
	 *       since we do not want to compute the offset or lbn of
	 *       length.
	 */
	if (type == IFLNK && (oip->i_flags & IC_FASTLINK) &&
	    oip->i_size > length) {
	    bzero(&(oip->i_symlink[length]), oip->i_size - length);
	    oip->i_size = length;
	    imark(oip, ICHG|IUPD);
	    iupdat(oip, 0, 0);
	    return 0;
	}

	fs = oip->i_fs;
	offset = blkoff(fs, length);
	lbn = lblkno(fs, length-1);
	if (oip->i_size < length) {
		int iupdat_flag = 0;
		/*
		IUPDAT (oip, 1);
		return(0);
		*/
		/*
		 * Trunc up case.  bmap will insure that the right blocks
		 * are allocated.  This includes extending the old frag to a
		 * full block (if needed) in addition to doing any work
		 * needed for allocating the last block.
		 */
		if (offset == 0)
			bn = bmap(oip, lbn, B_WRITE, (int)fs->fs_bsize,
				&iupdat_flag, 0, 0);
		else
			bn = bmap(oip, lbn, B_WRITE, offset, &iupdat_flag, 0, 0);
 
		if (u.u_error == 0 || bn >= (daddr_t)0) {
			oip->i_size = length;
			imark (oip, ICHG|IUPD);
		}
 
		if (iupdat_flag != 0)
			iupdat(oip, 0, 2);
		return (u.u_error);
	} else if (oip->i_size == length) {
		imark (oip, ICHG|IUPD);
		/* no need to post to disk via iupdat if length unchanged */
		iupdat(oip, 0, 0);
		return(0);
	}
	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	newblock = -1;
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update size of file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 * (? fsck doesn't check validity of pointers in indirect blocks)
	 */
	tip = *oip;
	if (type != IFIFO)
		for (level = TRIPLE; level >= SINGLE; level--)
			if (lastiblock[level] < 0) {
				oip->i_ib[level] = 0;
				lastiblock[level] = -1;
			}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;
	/*
	 * Now we have truncated the file to the correct size.
	 * If the file had holes, it is possible that the
	 * new size is in the middle of one of these holes.  If
	 * this is true, then we must add a block so that realloccg will
	 * not panic with a bprev of 0.  To do this we add a zero filled
	 * fragment set.
	 */
	bsize = fs->fs_bsize;
	if ((lastblock >= 0) && 
	    (lastblock < NDADDR) &&
            (length % bsize) && (oip->i_db[lblkno(fs, length)] == 0)) {
		size = fragroundup(fs, length % bsize);
		cg = itog(fs, oip->i_number);
		bno = (daddr_t)hashalloc(oip, cg, 0, size,
			(u_long (*)())alloccg,B_WORSTFIT);
		if (bno <= 0)
			panic("itrunc: no space");

		/* munhash newly allocated block */
		hash_bno = fsbtodb(fs, bno);
		hash_count = howmany(size, DEV_BSIZE);
		for (hash_i = 0; hash_i < hash_count; hash_i++)
			munhash(oip->i_devvp, hash_bno+hash_i);

		/*
		 * Here we add the new space to blocks.
		 * Although i_blocks is much higher then
		 * it should be, it will be correct as long
		 * as we don't panic before we update i_blocks
		 * at the end.  We can't fix it corectly here 
		 * since we don't know what holes exist in the
		 * file and that is a pain to figure out.
		 */

		oip->i_blocks += btodb(size);
#ifdef	FSD_KI
		bp = getblk(oip->i_devvp, fsbtodb(fs, bno), size,
				B_itrunc|B_data);
#else	FSD_KI
		bp = getblk(oip->i_devvp, fsbtodb(fs, bno), size);
#endif	FSD_KI
		clrbuf(bp);
		newblock = lblkno(fs, length);
		oip->i_db[newblock] = bno;
		bxwrite(bp, 0);
                oip->i_size = length;
        } else {
                /* If the file is not being
                 * truncated to a block boundry, the contents of the
                 * partial block following the end of the file must be
                 * zero'ed in case it ever becomes accessable again because
                 * of subsequent file growth.
                 */
                osize = oip->i_size;
                offset = blkoff(fs, length);
                if (offset == 0) {
                        oip->i_size = length;
                } else {
                        lbn = lblkno(fs, length);
                        bn = fsbtodb(fs, bmap(oip, lbn, B_WRITE, offset, 0, 0, 0));
                        if (u.u_error || (long)bn < 0)
                                return(0);
                        oip->i_size = length;
                        size = blksize(fs, oip, lbn);
                        count = howmany(size, DEV_BSIZE);
                        devvp = oip->i_devvp;
                        s = splimp();
			for (i = 0; i < count; i++)
                        	munhash(devvp, bn + i);
                        (void) splx(s);
#ifdef  FSD_KI
			bp = bread(devvp, bn, size, B_itrunc|B_data);
#else   FSD_KI
                        bp = bread(devvp, bn, size);
#endif	FSD_KI
                        if (bp->b_flags & B_ERROR) {
                                u.u_error = EIO;
				oip->i_size = osize;
				brelse(bp);
				return(0);
			}
			bzero(bp->b_un.b_addr + offset, (u_int)(size - offset));
			bdwrite(bp);
		}
	}
                          		
	/*
	 * i_blocks will be 0 since file size is being truncated
	 * to 0, so might as well have i_blocks and i_size
	 * consistent when writing out the inode
	 */
	if (length == 0)
		oip->i_blocks = 0;
	oip->i_ord_flags |= B2_UNLINKDATA;
	imark(oip, IUPD|ICHG);
	syncip(oip, 0, 0);
	ip = &tip;

	/*
	 * Indirect blocks first.
	 */
	/* fifo's indirect blocks have different meanings */
	if (type != IFIFO) {
		for (level = TRIPLE; level >= SINGLE; level--) {
			bn = ip->i_ib[level];
			if (bn != 0) {
				blocksreleased +=
					indirtrunc(ip, bn, lastiblock[level], level);
				if (lastiblock[level] < 0) {
					ip->i_ib[level] = 0;
					bpurge (ip->i_devvp, (daddr_t) fsbtodb(fs, bn));
					free(ip, bn, (off_t)fs->fs_bsize);
					blocksreleased += nblocks;
				}
			}
			if (lastiblock[level] >= 0)
				goto done;
		}
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		register int bksize;

		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		bksize = (off_t)blksize(fs, ip, i);
		bpurge (ip->i_devvp, (daddr_t) fsbtodb(fs, bn));
		free(ip, bn, bksize);
		blocksreleased += btodb(bksize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		int oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		if (newspace == 0)
			panic("itrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(ip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	if (type != IFIFO)
		for (level = SINGLE; level <= TRIPLE; level++)
			if (ip->i_ib[level] != oip->i_ib[level])
				panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if ((ip->i_db[i] != oip->i_db[i]) && (i != newblock))
			panic("itrunc2");
/* END PARANOIA */

        if (length != 0) {
		oip->i_blocks -= blocksreleased;
		if (oip->i_blocks < 0)			/* sanity */
			oip->i_blocks = 0;
	}
	imark(oip, ICHG);
#ifdef QUOTA
        (void) chkdq(oip, -blocksreleased, 0);
#endif QUOTA

	return (u.u_error);
}

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
long
indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;
#ifdef KPREEMPT
	int x;
#endif

    	KPREEMPTPOINT();

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those 
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
    	KPREEMPTPOINT();
#ifdef	FSD_KI
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize,
		B_itrunc|B_indbk);
#else	FSD_KI
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize);
#endif	FSD_KI
    	KPREEMPTPOINT();
	if (bp->b_flags&B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
#ifdef KPREEMPT
	x = splpreemptok();
#endif
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
#ifdef KPREEMPT
	splx(x);
#endif
	bxwrite(bp, 0);
	bp = copy, bap = bp->b_un.b_daddr;

    	KPREEMPTPOINT();
	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
	    	KPREEMPTPOINT();
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		bpurge (ip->i_devvp, (daddr_t) fsbtodb(fs, nb));
		free(ip, nb, (int)fs->fs_bsize);
		blocksreleased += nblocks;
	}
    	KPREEMPTPOINT();

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(ip, nb, last, level - 1);
	}
    	KPREEMPTPOINT();
	brelse(bp);
    	KPREEMPTPOINT();
	return (blocksreleased);
}




/*
 * remove any inodes in the inode cache belonging to dev
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * this is called from sumount()/sys3.c when dev is being unmounted
 */
#ifdef QUOTA
iflush(dev, iq)
	dev_t dev;
	struct inode *iq;
#else
iflush(dev)
	dev_t dev;
#endif
{
	register struct inode *ip;
	register open = 0;

	for (ip = inode; ip < inodeNINODE; ip++) {
#ifdef QUOTA
		if (ip != iq && ip->i_dev == dev)
#else
		if (ip->i_dev == dev)
#endif
		{
			if (ip->i_flag & IREF)
			{
				/* If this is a DUX vnode, the count may
				 * be due to having buffers in core
				 * so invalidate them
				 */
				if ((ITOV(ip))->v_fstype == VDUX)
				{
					binval(ITOV(ip));
				}
				if (ip->i_flag & IREF)
				{
					return(-1);
				}
			}
				remque(ip);
				ip->i_forw = ip;
				ip->i_back = ip;
				/*
				 * as i_count == 0, the inode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
#ifdef QUOTA
				dqrele(ip->i_dquot);
				ip->i_dquot = NULL;
#endif QUOTA
		/*This ifdef may look silly, but the braces end two
		 * different statements--I really don't want to timeshare
		 * them
		 */
		}
		else if ((ip->i_flag & IREF) && (ip->i_mode&IFMT)==IFBLK &&
		    ip->i_rdev == dev)
			open++;
	}
	return (open);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
ilock(ip)
	register struct inode *ip;
{
	ILOCK(ip);
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
iunlock(ip)
	register struct inode *ip;
{
	if (!ilocked(ip))
		panic ("iunlock");
	IUNLOCK(ip);
}

ilocked(ip)
	register struct inode *ip;
{
	return (ip->i_flag & ILOCKED);
}

/*
 * Check mode permission on inode.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * read and write permissions,
 * and is granted exec permissions
 * if any exec bits are set in
 * ip->i_mode.
 */
#define ANY_EXEC	(IEXEC | IEXEC >> 3 | IEXEC >> 6)
iaccess(ip, m)
	register struct inode *ip;
	register int m;
{
#ifndef ACLS
	register gid_t *gp;
#endif /* !ACLS */
#ifdef ACLS
	register int mode;
#endif

	if (m & IWRITE) {
		register struct vnode *vp;

		vp = ITOV(ip);
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK) {
				u.u_error = EROFS;
				return (EROFS);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (vp->v_flag & VTEXT)
			xrele(vp);
		if (vp->v_flag & VTEXT) {
			u.u_error = ETXTBSY;
			return (ETXTBSY);
		}
	}
	/*
	 * If you're the super-user,
	 * you always get read/write access,
	 * and exec access if any exec bit is
	 * set, or if it's a directory.
	 */
	if ((u.u_uid == 0) && 
	    (!(m & IEXEC) || 
	     ((ip->i_mode & IFMT) == IFDIR) || (ip->i_mode & ANY_EXEC)))
		return (0);
#ifndef ACLS
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		m >>= 3;
		if (u.u_gid == ip->i_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if ((ip->i_mode & m) == m)
		return (0);
	u.u_error = EACCES;
	return (EACCES);
#else
	mode = iget_access(ip);
	m >>= 6;
	if ((mode & m) == m)
		return (0);
	u.u_error = EACCES;
	return (EACCES);
#endif
}

#ifdef ITRACE
struct itr {
	int (*it_cfn)();
	struct inode *it_ip;
	int it_lock;
} itracebuf[NITRACE];
int itracex;


itrace(cfn, ip, lock)
	int (*cfn)();
	struct inode *ip;
	int lock;
{
	struct itr *itp;

	itp = &itracebuf[itracex++];
	itp->it_cfn = cfn;
	itp->it_ip = ip;
	itp->it_lock = lock;
	if (itracex >= NITRACE)
		itracex = 0;
}

ilpanic(ip)
	struct inode *ip;
{
	printf("ilpanic: hung locked inode");
	printf("ip = 0x%x, dev = 0x%x, ino = %d, flags = 0x%x\n",
	    ip, ip->i_dev, ip->i_number, ip->i_flag);
}
#endif

#ifdef AUTOCHANGER
/* Clears all the driver mount entries for all ufs mounted file systems */
void
clear_driver_mount_entries()
{
	register struct mount *mp;
	register struct mounthead *mhp;

	int i;

	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
		     mp = mp->m_hforw)
		{
			if ((mp->m_flag & MINUSE) == 0 || mp->m_dev == NODEV ||
			    bdevrmt(mp->m_dev))
				continue;
			/* call the driver mount entry to clear the 
		      	   pmount/punmount for that device */
			if ((bdevsw[major(mp->m_dev)].d_mount)!=nodev) {
                        	(*bdevsw[major(mp->m_dev)].d_mount)(mp->m_dev,
                                	NULL,NULL,NULL,0);
			}
		}
        }
        /* Call each drivers mount entry if it exists with the reboot flag set */
	for (i=0 ; i < nblkdev ; i++) {
	   if ((bdevsw[i].d_mount)!=nodev)
              (*bdevsw[i].d_mount)(NULL,NULL,NULL,NULL,1);
	}
}

#ifdef hp9000s800
/*
 * Closes all the autochanger devices so they have enougth time
 * to flush all the buffers.  100ms is no where near long enough
 * to wait if media motion is required.  Since close is async
 * all the buffers will be flushed when it returns.
 */

close_all_changer_devices()
{
	register struct mount *mp;
	register struct mounthead *mhp;
	int found=0;
	int first=1;

	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
		     mp = mp->m_hforw)
		{
			if ((mp->m_flag & MINUSE) == 0 || mp->m_dev == NODEV ||
			    bdevrmt(mp->m_dev))
				continue;
			/* call the driver close entry to clear the 
		      	    queues for that device */
			if ((bdevsw[major(mp->m_dev)].d_mount)!=nodev) {
				found=1;
				if (first==1) {
					printf("flushing autochanger devices..");
					first=0;
				}
				else
					printf(".");
                        	(*bdevsw[major(mp->m_dev)].d_close)(mp->m_dev,
                                	0);
			}
		}
        }
	if (found) printf("\n");
}
#endif /* 800 */

#endif /* AUTOCHANGER */

/*Clean up the file systems before reboot.  The following steps must be
 *taken:
#ifdef AUTOCHANGER
 *0) Clear all the mount entries for the drivers so the autochanger won't
 *   deadlock.
 *
#endif
 *
 *1) Freeze all file system modification by calling freeze_alldevs().  This
 *is essentially the same routine that is used by mirroring to take one
 *device off line.  It will guarantee that no system call that modifies the
 *file system is partially through.  However it doesn't block all file
 *system access.  For example, file system lookup can still proceed.
 *
 *2) If any pipes are open, close them.
 *
 *3) Any inodes with a link count of zero, should be freed.
 *
 *Note that inodes held in steps 2 and 3 must be locked, since we have not
 *blocked all file system activity.  However, it is not possible to simply
 *step through the inode table and lock all all the inodes, since this can
 *lead to deadlock with other processes that try to lock two inodes (e.g.
 *pathname lookup).  However, it is safe to lock pipes and inodes with a 0
 *link count, since they cannot lead to deadlocks.
 */
boot_clean_fs()
{
	register struct inode *ip;
#ifdef	ACLS
	register struct inode *cip;
#endif	ACLS
	
#ifdef AUTOCHANGER
        /* clear the mount entries */
        clear_driver_mount_entries();
#endif
	/*first freeze the devices*/
	freeze_alldevs();
	/*If any inode is active and has a
	 *reference count of zero, force it
	 *free.
	 *Also, clean up any pipes
	 */
	for (ip = inode; ip < inodeNINODE; ip++) {
		int mode;
		
		mode = (ip->i_mode & IFMT);

		
		if ((ip->i_flag & IREF) == 0 ||
		    (ip->i_nlink > 0 && mode != IFIFO)||
		    remoteip(ip))
			continue;
		VN_HOLD(ITOV(ip));
		ilock(ip);
		if (mode == IFIFO)
		{
			/*Just reset the read and
			 *write counts to zero
			 *and call closep to reset
			 *the rest of the pipe
			 */
			ip->i_fwcnt = ip->i_frcnt = 0;
			closep(ip,0);
		}
		if (ip->i_nlink == 0)
		{
			/*The following code is
			 *extracted from iinactive
			 *to free the inode
			 */
#ifdef	ACLS
			if (cip = ip->i_contip)
			{
				if ((cip->i_mode & IFMT) != IFCONT)
				{
					panic("continuation inode is wrong file type");
					
					/* make it fairly safe to continue from a panic -
					 * just zero continuation inode fields and release
					 * the primary inode.
					 */
					
					ip->i_contin = 0;
					ip->i_contip = 0;
				}
				else
				{
					cip->i_nlink = 0;	/* de-allocate cont inode */
					ip->i_contin = 0;
					cip->i_vnode.v_count = 0;
					cip->i_gen++;
					bzero(cip->i_acl,sizeof(struct acl_tuple)*
					      NOPTTUPLES);
					cip->i_mode = 0;
					cip->i_device = 0;
					cip->i_rsite = 0;

					/*
					 * For fast symlinks, we must
					 * ensure that the i_flags field
					 * is set back to 0.
					 */
					ip->i_flags = 0;

					imark(cip, IUPD|ICHG);
					ifree(cip, cip->i_number, mode);
#ifdef QUOTA
                                        /* decrement quota count for the   */
                                        /* continuation inode, but use the */
                                        /* "real" inode's fields for the   */
                                        /* parameters since the cont.      */
                                        /* inode doesn't have all the      */
                                        /* needed fields.                  */
					(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp),
						    ip, ip->i_uid, 0);
					dqrele(ip->i_dquot);
					ip->i_dquot = NULL;
#endif QUOTA
					iupdat(cip, 0, 0);
					ip->i_contip=0;
				}
			}
#endif	ACLS
			ip->i_gen++;
			itrunc(ip, (u_long)0);
			ip->i_mode = 0;
			ip->i_device = 0;
			ip->i_rsite = 0;

			/*
			 * For fast symlinks, we must ensure that the
			 * i_flags field is set back to 0.
			 */
			ip->i_flags = 0;

			imark(ip, IUPD|ICHG);
			ifree(ip, ip->i_number, mode);
#ifdef QUOTA
			(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp),
				    ip, ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
			
#endif
		}
		iupdat(ip, 0, 0);
		/*NOTE: We leave the inode locked, since we have just
		 *truncated it.  Any process that attempts to access it
		 *will block until the reboot.
		 */
	}
#ifdef	NSYNC
	update(1,1,0);
#else
	update(1,1);
#endif	/* NSYNC */
}

#ifdef ACLS
/* I G E T _ A C C E S S
 *
 * The access check routine. Check access for given inode and
 * current user. Check acl in specificity order ((u,g), (u.*),
 * (*.g) and (*.*)). The user can match more than one (u.g) and 
 * (*.g) tuples. For these it is necessary to check the entire
 * specificity level. For (u.*) return as soon as match is found.
 * Note there is only one (*.*) tuple (the 'other' base mode bits)
 *
 * If this algorithm changes, change frecover(1M) also.  It uses
 * the same method.
 */

iget_access(ip)
register struct inode *ip;
{
	register uid_t uid = u.u_uid;
	register gid_t gid = u.u_gid;
	gid_t *firstgp = u.u_groups, 
	    *maxgp = &u.u_groups[NGROUPS];
	register int tuple_count;
	register int match = FALSE;
	register int mode = 0;
	register struct acl_tuple *iacl;
	register gid_t *gp;

	if (ip->i_contip)
	{
		iacl = ip->i_contip->i_acl;
		tuple_count = 0;
	}
	else
		tuple_count = NOPTTUPLES;

	/* check all the (u,g) tuples first */

	for (;  (tuple_count < NOPTTUPLES) &&
		(iacl->uid < ACL_NSUSER)  &&
		(iacl->gid < ACL_NSGROUP); iacl++, tuple_count++ )
	{
		if (iacl->uid == uid)
		{
			if (iacl->gid == gid)
			{
				match = TRUE;
				mode |= iacl->mode;
				continue;
			}
			for (gp = firstgp; *gp != NOGROUP && gp < maxgp; gp++)
			{
				if (iacl->gid == *gp)
				{
					match = TRUE;
					mode |= iacl->mode;
					break;
				}
			}
		}
	}

	if (match) return mode;

	/* next check all the (u,*) tuples. Start with the base tuple */

	if (ip->i_uid == (ushort) uid) 
		return getbasetuple(ip->i_mode,ACL_USER);

	for (;  (tuple_count < NOPTTUPLES) &&
		(iacl->gid == ACL_NSGROUP); iacl++, tuple_count++ )
	{
		if (iacl->uid == uid)
			return iacl->mode;
	}

	/* and now we check the (*,g) tuples. Start with the base tuple */

	if (ip->i_gid == gid)
	{
		match = TRUE;
		mode = getbasetuple(ip->i_mode,ACL_GROUP);
	}
	else
	{
		for (gp = firstgp; gp < maxgp && *gp != NOGROUP; gp++)
		{
			if (ip->i_gid == *gp)
			{
				match = TRUE;
				mode = getbasetuple(ip->i_mode,ACL_GROUP);
				break;
			}
		}
	}
	for (;  (tuple_count < NOPTTUPLES) &&
		(iacl->uid == ACL_NSUSER); iacl++, tuple_count++ )
	{
		if (iacl->gid == gid)
		{
			match = TRUE;
			mode |= iacl->mode;
			continue;
		}
		for (gp = firstgp; *gp != NOGROUP && gp < maxgp ; gp++)
		{
			if (iacl->gid == *gp)
			{
				match = TRUE;
				mode |= iacl->mode;
				break;
			}
		}
	}

	if (match) return mode;

	/* return the mode of the (*,*) base tuple */

	return getbasetuple(ip->i_mode,ACL_OTHER);
}
#endif
