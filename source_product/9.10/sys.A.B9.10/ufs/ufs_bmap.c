/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_bmap.c,v $
 * $Revision: 1.22.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:21:16 $
 */

/* HPUX_ID: @(#)ufs_bmap.c	55.1		88/12/23 */

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


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/kernel.h"
#include "../h/debug.h"

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 *
 *      90.6.29 dbm - to make ufs reentrant the globals rablock
 *      and rasize are not used. If requested (rabp & rasp non-zero)
 *      the values are returned via these two new pointer arguments.
 *
 * The semantics that are implemented by rwip and bmap for synchronous i/o
 * are that when the call to rwip completes the data is on the disk.  
 * See the comments before rwip for more details - ghs 12/21/87.
 * 
 */
daddr_t
bmap(ip, bn, rwflg, size, syncio, rabp, rasp)
	register struct inode *ip;
	daddr_t bn;
	int rwflg;
	int size;	/* supplied only when rwflg == B_WRITE */
	int *syncio; 	/* true if synchronous i/o specified   */
	daddr_t *rabp;  /* non-zero if caller wants RA block */
	int *rasp;      /*      and size */

{
	register int i;
	int osize, nsize;
	struct buf *bp, *nbp;
	struct fs *fs;
	int j, sh, sh_shift;
	long nindir, indirshift, indirmask;
	daddr_t nb, lbn, *bap, pref, blkpref();
	register int count;
	register daddr_t tb;


    	KPREEMPTPOINT();	/* allow KERNEL PREEMPTION right here */
	if (bn < 0) {
		u.u_error = EFBIG;
    	    	goto bad0;
	}
	fs = ip->i_fs;
	VASSERT(!(fs->fs_ronly && rwflg == B_WRITE));

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	nb = lblkno(fs, ip->i_size);
	if (rwflg == B_WRITE && nb < NDADDR && nb < bn) {
		osize = blksize(fs, ip, nb);
		if (osize < fs->fs_bsize && osize > 0) {
			bp = realloccg(ip, ip->i_db[nb],
				blkpref(ip, nb, (int)nb, &ip->i_db[0]),
				osize, (int)fs->fs_bsize);
			if (bp == NULL)
    	    	    	    	goto bad1;
			ip->i_size = (nb + 1) * fs->fs_bsize;
			ip->i_db[nb] = dbtofsb(fs, bp->b_blkno);
			/* bug fix */
			/* If we are going to move blocks around, then we
			 * must be sure to check the TEXT hash chains, and 
			 * unmap the newly allocated blocks if necessary.
		   	 */
			tb = bp->b_blkno;
			count = howmany(fs->fs_bsize,DEV_BSIZE);
			for (i = 0; i < count; i++)
				munhash(ip->i_devvp, tb + i);
			ip->i_ord_flags |= B2_LINKDATA;
			imark(ip, IUPD|ICHG);
			/* 
			 * if synchronous operation is specified, then
			 * write out the new block synchronously.
			 */
			if (syncio) {
				bp->b_flags |= B_SYNC;
			    	bwrite(bp);
				*syncio = 1;
			}
			else
				bdwrite(bp);
		}
	}
	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (bn < NDADDR) {
		nb = ip->i_db[bn];
		if (rwflg == B_READ) {
			if (nb == 0)
    	    	    	    	goto bad1;
			goto gotit;
		}
		if (nb == 0 || ip->i_size < (bn + 1) * fs->fs_bsize) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;
				bp = realloccg(ip, nb,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					osize, nsize);
			} else {
				if (ip->i_size < (bn + 1) * fs->fs_bsize)
					nsize = fragroundup(fs, size);
				else
					nsize = fs->fs_bsize;
/*
 *  
 * About frag copying costs: if cost function is approximated 
 * by #frags copied, cost for pure bestfit, pure worstfit, and
 * mixed are as follows:
 *
 *  n -#frags at file close
 *  B -bestfit
 *  W -worstfit
 *  m2 -use bestfit for first frag, worstfit for rest.
 *  m3 -use bestfit for first two frags
 *  
 *  n	B	W	m2	m3
 *  
 *  1	0	1	0	0
 *  2	1	2	1	1
 *  3	3	3	6	3
 *  4	6	4	7	10
 *
 * Worstfit loses wrt. bestfit for n < 3.  m2 seems to be a reasonable
 * compromise.
 */


				bp = alloc(ip,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
#ifdef	FSD_KI
					nsize, B_bmap|B_data,B_WORSTFIT);
#else	FSD_KI
					nsize,B_WORSTFIT);
#endif	FSD_KI
			}
			if (bp == NULL)
				goto bad1;

			/*
			 * Since we have just changed the data in this
			 * block, remove the pages for this block from
			 * the pagecache.
			 */
			tb = bp->b_blkno;
			count = howmany(fs->fs_bsize, DEV_BSIZE);
			for (i = 0; i < count; i++)
				munhash(ip->i_devvp, tb + i);

			nb = dbtofsb(fs, bp->b_blkno);
			if ((ip->i_mode&IFMT) == IFDIR) {
				/*
				 * Write directory blocks synchronously so
				 * they never appear with garbage in them
				 * on the disk.  It is *not* necessary to write
				 * synchronously if syncio is specified since
				 * the block will be written synchronously
				 * by rwip().
				 */
				bxwrite(bp, 0);
				if (u.u_error) {
					ip->i_blocks -= btodb(nsize);
					free(ip, nb, nsize);
					goto bad1;
				}
			}
			else
				bdwrite(bp);
			ip->i_db[bn] = nb;
			ip->i_ord_flags |= B2_LINKDATA;
			imark(ip, IUPD|ICHG);
			if (syncio)
				*syncio = 1;
		}
gotit:
		if (rabp && rasp) {
			int ra;

			for (ra = 0; bn+ra+1 < NDADDR && ra < rasp[0]; ++ra) {
				rabp[ra] = fsbtodb(fs, ip->i_db[bn + ra + 1]);
				if (rabp[ra] == 0)
					break;
			}
			rasp[0] = ra;
			rasp[1] = fs->fs_bsize;
			rasp[2] = blksize(fs, ip, bn + ra);
		}
 	    	KPREEMPTPOINT();
		return (nb);
	}
    	KPREEMPTPOINT();
	/*
	 * Determine how many levels of indirection.
	 * Note the following code used to include multiplies and divides
	 * by NINDIR(fs).  It is much more efficient to do shifts and masks.
	 * Unfortunately, the information is not in the superblock.
	 * Fortunately, it is easy to calculate.
	 */
	nindir = NINDIR(fs);
	indirshift = fs->fs_bshift - 2;
	VASSERT((1<<indirshift) == nindir);
	indirmask = nindir - 1;
	pref = 0;
	sh = 1;
	sh_shift = 0;
	lbn = bn;
	bn -= NDADDR;
	for (j = NIADDR; j>0; j--) {
		sh <<= indirshift;
		sh_shift += indirshift;
		VASSERT (1<<sh_shift == sh);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		u.u_error = EFBIG;
    	    	goto bad0;
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rwflg == B_READ)
    	    	    	goto bad1;
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
#ifdef	FSD_KI
	        bp = alloc(ip, pref, (int)fs->fs_bsize, B_bmap|B_indbk, B_WORSTFIT);
#else	FSD_KI
	        bp = alloc(ip, pref, (int)fs->fs_bsize, B_WORSTFIT);
#endif	FSD_KI
		if (bp == NULL)
    	    	    	goto bad1;
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bxwrite(bp, 0);
		if (u.u_error) {
			ip->i_blocks -= btodb(fs->fs_bsize);
			free(ip, nb, fs->fs_bsize);
			goto bad1;
		}
		ip->i_ib[NIADDR - j] = nb;
		ip->i_ord_flags |= B2_LINKDATA;
		imark(ip, IUPD|ICHG);
		if (syncio)
			*syncio = 1;
	}
    	KPREEMPTPOINT();
	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, nb), 
#ifdef	FSD_KI
		    (int)fs->fs_bsize, B_bmap|B_indbk);
#else	FSD_KI
		    (int)fs->fs_bsize);
#endif	FSD_KI
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
    	    	    	goto bad0;
		}
		bap = bp->b_un.b_daddr;
		sh >>= indirshift;
		sh_shift -= indirshift;
		VASSERT (1<<sh_shift == sh);
		i = (bn >> sh_shift) & indirmask;
		nb = bap[i];
		if (nb == 0) {
			if (rwflg==B_READ) {
				brelse(bp);
    	    	    	    	goto bad1;
			}
			if (pref == 0)
				if (j < NIADDR)
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				else
					pref = blkpref(ip, lbn, i, &bap[0]);
#ifdef	FSD_KI
		        nbp = alloc(ip, pref, (int)fs->fs_bsize, B_bmap|B_data,B_WORSTFIT);
#else	FSD_KI
		        nbp = alloc(ip, pref, (int)fs->fs_bsize,B_WORSTFIT);
#endif	FSD_KI
			if (nbp == NULL) {
				brelse(bp);
    	    	    	    	goto bad1;
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if ((j < NIADDR) || ((ip->i_mode&IFMT) == IFDIR)) {
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 * It is *not* necessary to write the data
				 * block out synchronously if syncio because
				 * it will be written out synchronously
				 * by rwip().
				 */
				nbp->b_flags |= B_SYNC;
				bxwrite(nbp, 1);
				if (u.u_error) {
					ip->i_blocks -= btodb(fs->fs_bsize);
					free(ip, nb, fs->fs_bsize);
					goto bad1;
				}
			}
			else
				bdwrite(nbp);
			bap[i] = nb;
			/*
			 * If syncio is specified, write the indirect
			 * block out synchronously
			 */
			if (syncio){
				bp->b_flags |= B_SYNC;
				bwrite(bp);
				if (u.u_error) {
					bap[i] = 0;
					ip->i_blocks -= btodb(fs->fs_bsize);
					free(ip, nb, fs->fs_bsize);
					goto bad1;
				}
			}
			else
				bdwrite(bp);
		} else
			brelse(bp);
	}

	/*
	 * calculate read-ahead
	 */
	if (rabp && rasp) {
		int ra;

		for (ra = 0; i+ra+1 < nindir && ra < rasp[0]; ++ra) {
			rabp[ra] = fsbtodb(fs, bap[i + ra + 1]);
			if (rabp[ra] == 0)
				break;
		}
		rasp[0] = ra;
		rasp[1] = fs->fs_bsize;
		rasp[2] = fs->fs_bsize;
	}
   	KPREEMPTPOINT();
	return (nb);
	
bad0:
    	KPREEMPTPOINT();
	return((daddr_t)0);
bad1:
    	KPREEMPTPOINT();
	return((daddr_t)-1);
}
