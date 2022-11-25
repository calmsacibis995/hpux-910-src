/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_alloc.c,v $
 * $Revision: 1.32.83.4 $       $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/10/12 12:51:33 $
 */

/* HPUX_ID: @(#)ufs_alloc.c	55.2		89/01/12 */

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


#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../h/kernel.h"
#include "../h/mount.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../ufs/quota.h"
#include "../dux/cct.h"
#include "../h/kern_sem.h"

extern u_long		hashalloc();
extern ino_t		ialloccg();
extern daddr_t		alloccg();
extern daddr_t		alloccgblk();
extern daddr_t		fragextend();
extern daddr_t		blkpref();
extern daddr_t		mapsearch();
extern int		inside[], around[];
extern unsigned char	*fragtbl[];

/*
 * Allocate a block in the file system.
 * 
 * The size of the requested block is given, which must be some
 * multiple of fs_fsize and <= fs_bsize.
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 * If no block preference is given the following heirarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *      inode for the file.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 */
struct buf *
#ifdef	FSD_KI
alloc(ip, bpref, size, bptype, policy)
#else	FSD_KI
alloc(ip, bpref, size, policy)
#endif	FSD_KI
	register struct inode *ip;
	daddr_t bpref;
	int size;
#ifdef	FSD_KI
	int bptype;
#endif	FSD_KI
	int policy;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp;
	int cg;
#ifdef KPREEMPT
	int x;
#endif
	int infree;

    	KPREEMPTPOINT();
	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("alloc: bad size");
	}
	infree = freespace(fs, fs->fs_minfree) <= 0;
	if ((my_site_status & CCT_CLUSTERED) &&
		(infree || fs->fs_cstotal.cs_nbfree < ip->i_mount->m_maxbufs) &&
		(!(ip->i_mount->m_flag & M_IS_SYNC)))
		syncdisc(ip->i_mount);
	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0)
		goto nospace;

	if (u.u_uid != 0 && infree)
		goto nospace;
#ifdef QUOTA
	   if (policy & B_DQ_FORCE)
	      /* Don't need to set u_error here because it will always
	       * return 0.  This was causing u_error to be overwritten
	       * when VN_RELE() was called from vfsmount().  cwb
	       */
              (void) chkdq(ip, (long)btodb(size), 1);
           else {
              u.u_error = chkdq(ip, (long)btodb(size), 0);
	      if (u.u_error)
		  return (NULL);
	   }
#endif QUOTA
	if ((bpref < 0) || (bpref >= fs->fs_size))
		bpref = 0;
	if (bpref == 0)
		cg = itog(fs, ip->i_number);
	else
		cg = dtog(fs, bpref);
	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, size,
		(u_long (*)())alloccg, policy);
	if (bno <= 0)
		goto nospace;
	ip->i_blocks += btodb(size);
	ip->i_ord_flags |= B2_LINKDATA;
	imark(ip, ICHG);
#ifdef	FSD_KI
	bp = getblk(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), size, bptype);
#else	FSD_KI
	bp = getblk(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), size);
#endif	FSD_KI
#ifdef KPREEMPT
    	x = splpreemptok();
#endif
	clrbuf(bp);
#ifdef KPREEMPT
    	splx(x);
#endif
	return (bp);
nospace:
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
    	KPREEMPTPOINT();
	return (NULL);
}

/*
 * Reallocate a fragment to a bigger size
 *
 * The number and size of the old block is given, and a preference
 * and new size is also specified. The allocator attempts to extend
 * the original block. Failing that, the regular block allocator is
 * invoked to get an appropriate block.
 */
struct buf *
realloccg(ip, bprev, bpref, osize, nsize)
	register struct inode *ip;
	daddr_t bprev, bpref;
	int osize, nsize;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp, *obp;
	int cg;
#ifdef KPREEMPT
    	int x;
#endif
	int infree;

	KPREEMPTPOINT();
	
	fs = ip->i_fs;
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		printf("dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		panic("realloccg: bad size");
	}
	infree = freespace(fs, fs->fs_minfree) <= 0;
	if ((my_site_status & CCT_CLUSTERED) &&
		(infree || fs->fs_cstotal.cs_nbfree < ip->i_mount->m_maxbufs) &&
		(!(ip->i_mount->m_flag & M_IS_SYNC)))
		syncdisc(ip->i_mount);
	if (u.u_uid != 0 && infree)
		goto nospace;
	if (bprev == 0) {
		printf("dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		panic("realloccg: bad bprev");
	}
#ifdef QUOTA
	u.u_error = chkdq(ip, (long)btodb(nsize - osize), 0);
	if (u.u_error)
		return (NULL);
#endif QUOTA
	cg = dtog(fs, bprev);
	bno = fragextend(ip, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		do {
#ifdef	FSD_KI
			bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), osize, 
				B_realloccg|B_data);
#else	FSD_KI
			bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), 
				osize);
#endif	FSD_KI
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return (NULL);
			}
		} while (brealloc(bp, nsize) == 0);
		bp->b_flags |= B_DONE;
#ifdef KPREEMPT
    	    	x = splpreemptok();
#endif
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
#ifdef KPREEMPT
    	    	splx(x);
#endif
		ip->i_blocks += btodb(nsize - osize);
		ip->i_ord_flags |= B2_LINKDATA;
		imark(ip, ICHG);
		return (bp);
	}
	if ((bpref < 0) || (bpref >= fs->fs_size))
		bpref = 0;
	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, nsize,
		(u_long (*)())alloccg,B_WORSTFIT);
	if (bno > 0) {
#ifdef	FSD_KI
		obp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bprev), osize, 
				B_realloccg|B_data);
#else	FSD_KI
		obp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bprev), osize);
#endif	FSD_KI
		if (obp->b_flags & B_ERROR) {
			brelse(obp);
			return (NULL);
		}
#ifdef	FSD_KI
		bp = getblk(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), nsize,
				B_realloccg|B_data);
#else	FSD_KI
		bp = getblk(ip->i_devvp, (daddr_t)fsbtodb(fs, bno), nsize);
#endif	FSD_KI
#ifdef KPREEMPT
		x = splpreemptok();
#endif
		bcopy(obp->b_un.b_addr, bp->b_un.b_addr, (u_int)osize);
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
#ifdef KPREEMPT
    	    	splx(x);
#endif
		brelse(obp);
		bpurge(ip->i_devvp, (daddr_t)fsbtodb(fs, bprev));
		free(ip, bprev, (off_t)osize);
		ip->i_blocks += btodb(nsize - osize);
		ip->i_ord_flags |= B2_LINKDATA;
		imark(ip, ICHG);
		return (bp);
	}
nospace:
	/*
	 * no space available
	 */
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

extern char panic_msg[]; /*  from subr_prf.c; holds text of panic messages  */
extern int panic_msg_len;

/*
 * Allocate an inode in the file system.
 * 
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate an inode:
 *   1) allocate the requested inode.
 *   2) allocate an inode in the same cylinder group.
 *   3) quadradically rehash into other cylinder groups, until an
 *      available inode is located.
 * If no inode preference is given the following heirarchy is used
 * to allocate an inode:
 *   1) allocate an inode in cylinder group 0.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available inode is located.
 */
struct inode *
ialloc(pip, ipref, mode)
	register struct inode *pip;
	ino_t ipref;
	int mode;
{
	ino_t ino;
	register struct fs *fs;
	register struct inode *ip;
	int cg;
	
	fs = pip->i_fs;
	if (fs->fs_cstotal.cs_nifree == 0)
		goto noinodes;
#ifdef QUOTA
	u.u_error = chkiq(VFSTOM(pip->i_vnode.v_vfsp),
		 	(struct inode *)NULL, u.u_uid, 0);
	if (u.u_error)
		return (NULL);
#endif QUOTA
	if (ipref >= fs->fs_ncg * fs->fs_ipg)
		ipref = 0;
	cg = itog(fs, ipref);
	ino = (ino_t)hashalloc(pip, cg, (long)ipref, mode, ialloccg ,B_BESTFIT);
	if (ino == 0)
		goto noinodes;
    	KPREEMPTPOINT();
	ip = iget(pip->i_dev, pip->i_mount, ino);
	if (ip == NULL) {
		ifree(pip, ino, 0);
		return (NULL);
	}
	if (ip->i_mode) {
		sprintf(panic_msg, panic_msg_len, 
		        "filesystem %s is corrupt; fsck it\n  (device 0x%x, mode 0%o, inode %d; dup alloc)\n",
		        fs->fs_fsmnt, ip->i_dev, ip->i_mode, ip->i_number);
		panic(panic_msg);
	}
	if (ip->i_blocks) {				/* XXX */
		printf("free inode %s/%d had %d blocks\n",
		    fs->fs_fsmnt, ino, ip->i_blocks);
		ip->i_blocks = 0;
	}

	/* the following fields are cleared because of their use */
	/* with fifos.  If the system would crash then their old */
	/* values would still be there */

	ip->i_ib[0] = 0;
	ip->i_ib[1] = 0;
	ip->i_ib[2] = 0;

	/*
	 * For fast symlinks, we must ensure that the i_flags
	 * field is initialized.
	 */
	ip->i_flags = 0;

#ifdef	ILOGGING
	ilog(ip, 1);
#endif
	ip->i_fversion++;	/*indicate inode has changed*/
	return (ip);
noinodes:
	fserr(fs, "out of inodes");
	uprintf("\n%s: create/symlink failed, no inodes free\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free inodes, the one with the smallest number of directories.
 */
ino_t
dirpref(fs)
	register struct fs *fs;
{
	int cg, minndir, mincg, avgifree;

	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs(fs, cg).cs_ndir < minndir &&
		    fs->fs_cs(fs, cg).cs_nifree >= avgifree) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((ino_t)(fs->fs_ipg * mincg));
}

/*
 * Select the desired position for the next block in a file.  The file is
 * logically divided into sections. The first section is composed of the
 * direct blocks. Each additional section contains fs_maxbpg blocks.
 * 
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the inode that describes
 * the file. If no blocks have been allocated in any other section, the
 * policy is to place the section in a cylinder group with a greater than
 * average number of free blocks.  An appropriate cylinder group is found
 * by using a rotor that sweeps the cylinder groups. When a new group of
 * blocks is needed, the sweep begins in the cylinder group following the
 * cylinder group from which the previous allocation was made. The sweep
 * continues until a cylinder group with greater than the average number
 * of free blocks is found. If the allocation is for the first block in an
 * indirect block, the information on the previous allocation is unavailable;
 * here a best guess is made based upon the logical block number being
 * allocated.
 * 
 * If a section is already partially allocated, the policy is to
 * contiguously allocate fs_maxcontig blocks.  The end of one of these
 * contiguous blocks and the beginning of the next is physically separated
 * so that the disk head will be in transit between them for at least
 * fs_rotdelay milliseconds.  This is to allow time for the processor to
 * schedule another I/O transfer.
 */
daddr_t
blkpref(ip, lbn, indx, bap)
	struct inode *ip;
	daddr_t lbn;
	int indx;
	daddr_t *bap;
{
	register struct fs *fs;
	register int cg;
	int avgbfree, startcg;
	daddr_t nextblk;
	
    	KPREEMPTPOINT();
	fs = ip->i_fs;
	if (indx % fs->fs_maxbpg == 0 || bap[indx - 1] == 0) {
		if (lbn < NDADDR) {
			cg = itog(fs, ip->i_number);
			nextblk = fs->fs_fpg * cg + fs->fs_frag;
			goto done;
		}
		/*
		 * Find a cylinder with greater than average number of
		 * unused data blocks.
		 */
		if (indx == 0 || bap[indx - 1] == 0)
			startcg = itog(fs, ip->i_number) + lbn / fs->fs_maxbpg;
		else
			startcg = dtog(fs, bap[indx - 1]) + 1;
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				nextblk = fs->fs_fpg * cg + fs->fs_frag;
				goto done;
			}
		for (cg = 0; cg < startcg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				nextblk = fs->fs_fpg * cg + fs->fs_frag;
				goto done;
			}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 */
	nextblk = bap[indx - 1] + fs->fs_frag;
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk)
		goto done;
	if (fs->fs_rotdelay != 0) {
		long nspf1000;
		/*
		 * Here we convert ms of delay to frags as:
		 * (frags) = (ms) * (rev/sec) * (sect/rev) /
		 *	((sect/frag) * (ms/sec))
		 * then round up to the next block.
		 */
                /* The old code had a bug here.  The division truncated
                 * The fraction to the next lowest integer before rounding up.
                 * This meant that the value could be rounded down instead of
                 * rounding up.  This happened in an 8K/8K file system.
                 * By precomuting nspf1000 and adding one less than it before
                 * the division, we are forcing a round up.
                 */
                 nspf1000 = NSPF(fs) * 1000;
                 nextblk += roundup((fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect
                     +nspf1000-1) / nspf1000, fs->fs_frag);
        }
done:
	if ((nextblk < 0) || (nextblk >= fs->fs_size))
		return(NULL);
	else
		return (nextblk);
}

/*
 * Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadradically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
/*VARARGS5*/
u_long
hashalloc(ip, cg, pref, size, allocator, policy)
	struct inode *ip;
	int cg;
	long pref;
	int size;	/* size for data blocks, mode for inodes */
	u_long (*allocator)();
	int policy;
{
	register struct fs *fs;
	long result;
	int i, icg = cg;

	fs = ip->i_fs;
    	KPREEMPTPOINT();
	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(ip, cg, pref, size, policy);
	if (result)
		return (result);
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		result = (*allocator)(ip, cg, 0, size, policy);
		if (result)
			return (result);
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		result = (*allocator)(ip, cg, 0, size,policy);
		if (result)
			return (result);
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
	return (NULL);
}

/*
 * Determine whether a fragment can be extended.
 *
 * Check to see if the necessary fragments are available, and 
 * if they are, allocate them.
 */
daddr_t
fragextend(ip, cg, bprev, osize, nsize)
	struct inode *ip;
	int cg;
	long bprev;
	int osize, nsize;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	long bno;
	int frags, bbase;
	int i;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize))
		return (NULL);
	frags = numfrags(fs, nsize);
	bbase = bprev % fs->fs_frag;
	if (bbase > (bprev + frags - 1) % fs->fs_frag) {
		/* cannot extend across a block boundry */
		return (NULL);
	}
#ifdef	FSD_KI
   	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
			(int)fs->fs_cgsize, B_fragextend|B_cylgrp);
#else	FSD_KI
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
			(int)fs->fs_cgsize);
#endif	FSD_KI
    	KPREEMPTPOINT();
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	bno = dtogd(fs, bprev);
	for (i = numfrags(fs, osize); i < frags; i++)
		if (isclr(cgp->cg_free, bno + i)) {
			brelse(bp);
			return (NULL);
		}
	/*
	 * the current fragment can be extended
	 * deduct the count on fragment being extended into
	 * increase the count on the remaining fragment (if any)
	 * allocate the extended piece
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++)
		if (isclr(cgp->cg_free, bno + i))
			break;
	cgp->cg_frsum[i - numfrags(fs, osize)]--;
	if (i != frags)
		cgp->cg_frsum[i - frags]++;
	for (i = numfrags(fs, osize); i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
		cgp->cg_cs.cs_nffree--;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_nffree--;
		fs->fs_cs(fs, cg).cs_nffree--;
		SPINUNLOCK(v_count_lock);
	}
	fs->fs_fmod = 1;
	bdwrite(bp);
	return (bprev);
}

/*
 * Determine whether a block can be allocated.
 *
 * Check to see if a block of the apprpriate size is available,
 * and if it is, allocate it.
 */
daddr_t
alloccg(ip, cg, bpref, size, policy)
	struct inode *ip;
	int cg;
	daddr_t bpref;
	int size;
	int policy;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	int bno, frags;
	int allocsiz;
	register int i;

	KPREEMPTPOINT();

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize)
		return (NULL);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
#ifdef	FSD_KI
		(int)fs->fs_cgsize, B_alloccg|B_cylgrp);
#else	FSD_KI
		(int)fs->fs_cgsize);
#endif	FSD_KI
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (NULL);
	}
	/* this is a bug fix */
	if (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	if (size == fs->fs_bsize) {
		bno = alloccgblk(fs, cgp, bpref);
		bdwrite(bp);
    	    	KPREEMPTPOINT();
		return (bno);
	}

	frags = numfrags(fs, size);

	/* Try to get a full block to satisfy this request unless the disk
	   is getting full.  When there are no blocks left we do not use the
	   worst fit policy.
	*/

	if ((policy == B_WORSTFIT) && 
	    (fs->fs_cstotal.cs_nbfree))
	{
		allocsiz = fs->fs_frag;      /* get a block for growing files */
		ip->i_flag |= IFRAG;
	}
	else
	{

		/*
		 * check to see if any fragments are already available
		 * allocsiz is the size which will be allocated, hacking
		 * it down to a smaller size if necessary
		 */
		for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++)
			if (cgp->cg_frsum[allocsiz] != 0)
				break;
	}
    	KPREEMPTPOINT();
	if (allocsiz == fs->fs_frag) {
		/*
		 * no fragments were available, so a block will be 
		 * allocated, and hacked up
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
    	    	    	KPREEMPTPOINT();
			return (NULL);
		}
		bno = alloccgblk(fs, cgp, bpref);
		bpref = dtogd(fs, bno);
		for (i = frags; i < fs->fs_frag; i++)
			setbit(cgp->cg_free, bpref + i);
		i = fs->fs_frag - frags;
		cgp->cg_cs.cs_nffree += i;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		SPINUNLOCK(v_count_lock);
		fs->fs_fmod = 1;
		cgp->cg_frsum[i]++;
		bdwrite(bp);
    	    	KPREEMPTPOINT();
		return (bno);
	}
    	KPREEMPTPOINT();
	bno = mapsearch(fs, cgp, bpref, allocsiz);
	/* this is a bug fix */
	if (bno < 0) {
		brelse(bp);
		return (NULL);
	}
	for (i = 0; i < frags; i++)
		clrbit(cgp->cg_free, bno + i);
	cgp->cg_cs.cs_nffree -= frags;
	SPINLOCK(v_count_lock);
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	SPINUNLOCK(v_count_lock);
	fs->fs_fmod = 1;
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	bdwrite(bp);
    	KPREEMPTPOINT();
	return (cg * fs->fs_fpg + bno);
}

/*
 * Allocate a block in a cylinder group.
 *
 * This algorithm implements the following policy:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate the next available block on the block rotor for the
 *      specified cylinder group.
 * Note that this routine only allocates fs_bsize blocks; these
 * blocks may be fragmented by the routine that allocates them.
 */
daddr_t
alloccgblk(fs, cgp, bpref)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
{
	daddr_t bno;
	int cylno, pos, delta;
	short *cylbp;
	register int i;

	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref &= ~(fs->fs_frag - 1);
	bpref = dtogd(fs, bpref);
	/*
	 * if the requested block is available, use it
	 */
	if (isblock(fs, cgp->cg_free, fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * check for a block available on the same cylinder
	 */
	cylno = cbtocylno(fs, bpref);
	if (cgp->cg_btot[cylno] == 0)
		goto norot;
	if (fs->fs_cpc == 0) {
		/*
		 * block layout info is not available, so just have
		 * to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cgp->cg_b[cylno];
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < NRPOS; i++)
		if (cylbp[i] > 0)
			break;
	if (i == NRPOS)
		for (i = 0; i < pos; i++)
			if (cylbp[i] > 0)
				break;
	if (cylbp[i] > 0) {
		/*
		 * found a rotational position, now find the actual
		 * block. A panic if none is actually there.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs->fs_postbl[pos][i] == -1) {
			printf("pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			panic("alloccgblk: cyl groups corrupted");
		}
		for (i = fs->fs_postbl[pos][i];; ) {
			if (isblock(fs, cgp->cg_free, bno + i)) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs->fs_rotbl[i];
			if (delta <= 0 || delta > MAXBPC - i)
				break;
			i += delta;
		}
		printf("pos = %d, i = %d, fs = %s\n", pos, i, fs->fs_fsmnt);
		panic("alloccgblk: can't find blk in cyl");
	}
norot:
	/*
	 * no blocks in the requested cylinder, so take next
	 * available one in this cylinder group.
	 */
	bno = mapsearch(fs, cgp, bpref, (int)fs->fs_frag);
	if (bno < 0)
		return (NULL);
	cgp->cg_rotor = bno;
gotit:
	clrblock(fs, cgp->cg_free, (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	SPINLOCK(v_count_lock);
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	SPINUNLOCK(v_count_lock);
	cylno = cbtocylno(fs, bno);
	cgp->cg_b[cylno][cbtorpos(fs, bno)]--;
	cgp->cg_btot[cylno]--;

	fs->fs_fmod = 1;
	return (cgp->cg_cgx * fs->fs_fpg + bno);
}
	
/*
 * Determine whether an inode can be allocated.
 *
 * Check to see if an inode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested inode.
 *   2) allocate the next available inode after the requested
 *      inode in the specified cylinder group.
 */
/*ARGSUSED*/
ino_t
ialloccg(ip, cg, ipref, mode, policy)
	struct inode *ip;
	int cg;
	daddr_t ipref;
	int mode;
	int policy;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	int i;

  	KPREEMPTPOINT();

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nifree == 0)
		return (NULL);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
#ifdef	FSD_KI
		(int)fs->fs_cgsize, B_alloccg|B_cylgrp);
#else	FSD_KI
		(int)fs->fs_cgsize);
#endif	FSD_KI
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
    	    	KPREEMPTPOINT();
		return (NULL);
	}
	/* this is a bug fix */
	if (cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
    	    	KPREEMPTPOINT();
		return (NULL);
	}

    	KPREEMPTPOINT();
	cgp->cg_time = time.tv_sec;
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, ipref))
			goto gotit;
	} else
		ipref = cgp->cg_irotor;
	for (i = 0; i < fs->fs_ipg; i++) {
    	    	KPREEMPTPOINT();
		ipref++;
		if (ipref >= fs->fs_ipg)
			ipref = 0;
		if (isclr(cgp->cg_iused, ipref)) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
	}
	brelse(bp);
    	KPREEMPTPOINT();
	return (NULL);
gotit:
	setbit(cgp->cg_iused, ipref);
	cgp->cg_cs.cs_nifree--;
	SPINLOCK(v_count_lock);
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	SPINUNLOCK(v_count_lock);
	fs->fs_fmod = 1;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir++;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
		SPINUNLOCK(v_count_lock);
	}
	bdwrite(bp);
    	KPREEMPTPOINT();
	return (cg * fs->fs_ipg + ipref);
}


/*
 * Free a block or fragment.
 *
 * The specified block or fragment is placed back in the
 * free map. If a fragment is deallocated, a possible 
 * block reassembly is checked.
 */
free(ip, bno, size)
	register struct inode *ip;
	daddr_t bno;
	off_t size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg, blk, frags, bbase;
	register int i;
	daddr_t savebno;

	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("free: bad size");
	}
	cg = dtog(fs, bno);
	if (badblock(fs, bno)) {
		printf("bad block %d, ino %d\n", bno, ip->i_number);
		return;
	}
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
#ifdef	FSD_KI
		(int)fs->fs_cgsize, B_free|B_cylgrp);
#else	FSD_KI
		(int)fs->fs_cgsize);
#endif	FSD_KI
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
        savebno = bno;
	bno = dtogd(fs, bno);
	if (size == fs->fs_bsize) {
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bno))) {
			sprintf(panic_msg, panic_msg_len, 
			        "filesystem %s is corrupt; fsck it\n  (device 0x%x, block %d; freeing free block)\n",
			        fs->fs_fsmnt, ip->i_dev, savebno);
			/*
			 * free bp in case is needed while syncing disks for
			 * panic.  Otherwise, panic may hang.
			 */
			brelse(bp);
			panic(panic_msg);
		}
		setblock(fs, cgp->cg_free, fragstoblks(fs, bno));
		cgp->cg_cs.cs_nbfree++;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		SPINUNLOCK(v_count_lock);
		i = cbtocylno(fs, bno);
		cgp->cg_b[i][cbtorpos(fs, bno)]++;
		cgp->cg_btot[i]++;
	} else {
		bbase = bno - (bno % fs->fs_frag);
		/*
		 * decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, bno + i)) {
				sprintf(panic_msg, panic_msg_len, 
				        "filesystem %s is corrupt; fsck it\n  (device 0x%x, block %d; freeing free frag)\n",
				        fs->fs_fsmnt, ip->i_dev, bno + i);
				/*
				 * free bp in case is needed while syncing
				 * disks for panic.  Otherwise, panic may hang.
				 */
				brelse(bp);
				panic(panic_msg);
			}
			setbit(cgp->cg_free, bno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		SPINUNLOCK(v_count_lock);
		/*
		 * add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * if a complete block has been reassembled, account for it
		 */
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			SPINLOCK(v_count_lock);
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			SPINUNLOCK(v_count_lock);
			i = cbtocylno(fs, bbase);
			cgp->cg_b[i][cbtorpos(fs, bbase)]++;
			cgp->cg_btot[i]++;
		}
	}
	fs->fs_fmod = 1;
	bdwrite(bp);
}

/*
 * Free an inode.
 *
 * The specified inode is placed back in the free map.
 */
ifree(ip, ino, mode)
	struct inode *ip;
	ino_t ino;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg;
	ino_t saveino;

	fs = ip->i_fs;
	if ((unsigned)ino >= fs->fs_ipg*fs->fs_ncg) {
		printf("dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, ino, fs->fs_fsmnt);
		panic("ifree: range");
	}
	cg = itog(fs, ino);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, cgtod(fs, cg)), 
#ifdef	FSD_KI
		(int)fs->fs_cgsize, B_ifree|B_cylgrp);
#else	FSD_KI
		(int)fs->fs_cgsize);
#endif	FSD_KI
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
        saveino = ino;
	ino %= fs->fs_ipg;
	if (isclr(cgp->cg_iused, ino)) {
		sprintf(panic_msg, panic_msg_len, 
		        "filesystem %s is corrupt; fsck it\n  (device 0x%x, block %d; freeing free inode)\n",
		        fs->fs_fsmnt, ip->i_dev, saveino);
		/*
		 * free bp in case is needed while syncing disks for
		 * panic.  Otherwise, panic may hang.
		 */
		brelse(bp);
		panic(panic_msg);
	}
	clrbit(cgp->cg_iused, ino);
 	if (ino < cgp->cg_irotor)
 		cgp->cg_irotor = ino;
	cgp->cg_cs.cs_nifree++;
	SPINLOCK(v_count_lock);
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	SPINUNLOCK(v_count_lock);
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir--;
		SPINLOCK(v_count_lock);
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
		SPINUNLOCK(v_count_lock);
	}
	fs->fs_fmod = 1;
	bdwrite(bp);
}

/*
 * Find a block of the specified size in the specified cylinder group.
 *
 * It is a panic if a request is made to find a block if none are
 * available.
 */
daddr_t
mapsearch(fs, cgp, bpref, allocsiz)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
	int allocsiz;
{
	daddr_t bno;
	int start, len, loc, i;
	int blk, field, subfield, pos;

	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref > 0)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[start],
		(caddr_t)fragtbl[fs->fs_frag],
		(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[start],
			(caddr_t)fragtbl[fs->fs_frag],
			(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
		if (loc == 0)
			return (-1);
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * found the byte in the map
	 * sift through the bits to find the selected frag
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cgp->cg_free, bno);
		blk <<= 1;
		field = around[allocsiz];
		subfield = inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield)
				return (bno + pos);
			field <<= 1;
			subfield <<= 1;
		}
	}
	printf("bno = %d, fs = %s\n", bno, fs->fs_fsmnt);
	panic("alloccg: block not in map");
	return (-1);
}

/*
 * Fserr prints the name of a file system with an error diagnostic.
 * 
 * The form of the error message is:
 *	fs: error message
 */
fserr(fs, cp)
	struct fs *fs;
	char *cp;
{

	printf("%s: %s\n", fs->fs_fsmnt, cp);
}



extern int numdirty;

int
frag_fit(ip)
struct inode *ip;
{
	/* best fit the last fragment if necessary */

	register struct fs *fs = ip->i_fs; /* the file system */
	int nb = lblkno(fs, ip->i_size); /* the number of blocks */
	int datasize = blkoff(fs,ip->i_size);  /* the number of used frags */
	int fragsize = fragroundup(fs, datasize);  /* the requied number of frags */
	int freed = 0; /* was a block freed ? */
	struct buf *bp, *obp;
	long bprev;


	if (
	    (ip->i_flag & IFRAG) && 
#ifdef ACLS
	    !((ip->i_mode & IFMT) == IFCONT) &&  /* not a continuation inode */
#endif
	    (ip->i_vnode.v_fstype == VUFS) && /* only for ufs file systems */
	    ip->i_nlink && /* file still exists in the namespace */
	    datasize) /* the file is not empty */
	{
		ip->i_flag &= ~IFRAG;
		
		if (nb<NDADDR) /* only necessary for direct blocks */
			bprev = ip->i_db[nb];
		else
			return 0; /* we did not move any frags */

		if ((fragsize < fs->fs_bsize) && /* doesnt need the whole last block */
		    (fragextend(ip, dtog(fs, bprev), (long)bprev, fragsize, 
				fs->fs_bsize))) /* could this frag be a block? */
		{
#ifdef  FSD_KI
			obp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bprev), fragsize, 
				B_ufs_close|B_data);
#else  	 /* FSD_KI */
			obp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bprev), fragsize);
#endif	/* FSD_KI */
#ifdef FSD_KI
			bp = alloc(ip,
				blkpref(ip, nb, (int)nb, &ip->i_db[0]),
				fragsize, B_ufs_close|B_data,B_BESTFIT
#ifdef QUOTA
                                | B_DQ_FORCE
#endif QUOTA
                                );
#else  /*  FSD_KI */
			bp = alloc(ip,
				blkpref(ip, nb, (int)nb, &ip->i_db[0]),
				fragsize,B_BESTFIT
#ifdef QUOTA
                                | B_DQ_FORCE
#endif QUOTA
                                );
#endif  /* FSD_KI */
#ifdef QUOTA
                        if (bp != NULL)  
                           /* return "borrowed" blocks to quota account    *
                            * force field is 1 so that user message status *
                            * remains unchanged.                           */
                           (void) chkdq(ip, (long)(-btodb(fragsize)), 1);
#endif QUOTA
			if ((obp->b_flags & B_ERROR) || /* error gettting the old block */
			    (bp == NULL))		/* or the new block */
			{
				brelse(obp);
				free(ip,bprev+numfrags(fs, fragsize),
				     (off_t)(fs->fs_bsize-fragsize)); /* free the extended block */
				if (bp != NULL)
					brelse(bp);
				u.u_error = 0;	/* if alloc had failed */
			}
			else /* no error - move the block */	
			{
				bcopy(obp->b_un.b_addr, bp->b_un.b_addr, (u_int) datasize);
				bzero(bp->b_un.b_addr + datasize, (unsigned)(fragsize-datasize));
				/*
				 * If we are going to move blocks around, then we
				 * must be sure to check the TEXT hash chains, and
				 * unmap the newly allocated block if necessary.
				 */
				{
					int i = 0;
					int tb = bp->b_blkno;
					int count = howmany(fragsize,DEV_BSIZE);
					for (; i < count; i++)
						munhash(ip->i_devvp, tb + i);
				}
				ip->i_db[nb] = dbtofsb(fs, bp->b_blkno);
				ip->i_blocks -= btodb(fragsize); /* undo side effect of alloc */
				ip->i_ord_flags |= B2_LINKDATA;
				imark(ip, ICHG);
				if (ip->i_flag & (ISYNC|IFRAGSYNC)) {
					iupdat(ip, 1, 0);
					bwrite(bp);
				}
				else {
					iupdat(ip, 0, 1);
					bdwrite(bp); 
				}
				ip->i_flag &= ~IFRAGSYNC;

				if(obp->b_flags & B_DELWRI) {
				    u.u_ru.ru_oublock--; /* cancel io */
				    obp->b_flags |=  B_NOCACHE;
				    obp->b_flags &= ~B_DELWRI;
				    clear_delwri(obp);
				}
				brelse(obp); /* we are done with obp */
				free(ip, bprev, (off_t)(fs->fs_bsize)); /* free the block */
				freed = 1;
			}	

		}
	}
	return freed; /* empty, not linked or not ufs */
}

#ifdef	notdef
/* The following code is never called, so I have ifdef'ed it out */

struct inode *fit_inode_place=NULL;

fit_all_frags(fs)
register struct fs *fs;
{

	/* walk through all inodes of a file system attempting to 
	   best fit the last fragment.  This is called when a file 
	   system is out of space to see if some blocks can be freed */

	register struct inode *ip;
	register int i;
	int x,nfreed = 0; /* the number of blocks freed */
	u_short uid_save = u.u_uid;

	u.u_uid = 0; /* since we are actually freeing space, let us dip into minfree */

	/* since fit_inode_place is global and it is possible for more than
	   one person to be in the code at once, processes may "chase" 
	   each other through the inodes.  Oh well. */

	ip = fit_inode_place;

	for(i=0;i<ninode;i++,ip++)
	{
		if ((ip == inodeNINODE) || (ip == NULL))
			ip = inode;
		if (!(ilocked(ip))  && /* noone has it locked */
		   (ip->i_flag & IREF)       && /* it is in use */
		   (ip->i_fs == fs))            /* it is on this file system */
		{
			ilock(ip);
			nfreed += frag_fit(ip);
			iunlock(ip);
			if (nfreed)
				break;
		}
	}
	fit_inode_place = ip;

	u.u_uid = uid_save;

	return nfreed;
}
#endif	/*notdef*/
