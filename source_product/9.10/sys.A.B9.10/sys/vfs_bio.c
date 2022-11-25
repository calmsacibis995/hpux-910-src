/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_bio.c,v $
 * $Revision: 1.20.83.16 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/01/04 16:59:30 $
 */

/* HPUX_ID: @(#)vfs_bio.c	55.1		88/12/23 */

/*	@(#)vfs_bio.c 1.1 86/02/03 SMI	*/
/*      NFSSRC @(#)vfs_bio.c	2.1 86/04/15 */

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

#include "../h/types.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#if (defined(_WSIO) || defined(__hp9000s300))
#include "../h/dir.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#endif	/* defined (_WSIO) || defined(__hp9000s300) */
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/bcache.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../h/vnode.h"
#include "../h/rtprio.h"
#ifdef SAR
#include "../h/sar.h"
#endif 
#include "../dux/selftest.h"
#include "../dux/dux_hooks.h"

#include "../h/kern_sem.h"
#include "../ufs/fs.h"
#include "../h/pfdat.h"

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI

#ifdef	NSYNC
#include "../h/kernel.h"
#endif	/* NSYNC */

#include "../h/malloc.h"

#ifdef OSDEBUG
	int getfreebuf = 0;
#endif /* OSDEBUG */	 

/* Insert a buffer, "newbp" to the left of "bp" in the free list */
#define inserttail(bp, newbp){		\
	register struct buf *tmpbp;	\
	tmpbp = bp->av_back;		\
	newbp->av_forw = bp;		\
	tmpbp->av_forw = newbp;		\
	newbp->av_back = tmpbp;		\
	bp->av_back = newbp;		\
}

/* Insert a buffer, "newbp" to the right of "bp" in the free list */
#define inserthead(bp, newbp){		\
	register struct buf *tmpbp;	\
	tmpbp = bp->av_forw;		\
	newbp->av_forw = tmpbp;		\
	tmpbp->av_back = newbp;		\
	newbp->av_back = bp;		\
	bp->av_forw = newbp;		\
}

/* XXX make_wanted should migrate to buf.h, plus there are bsetprio()
 * sharing possibilities to work out.  It could also be called in more places.
 * Not having it called everywhere is not a defect, the more places
 * it is used the better off we are.  The uses in vfs_bio.c happen
 * to be most of the important ones.
 *
 * This is also not "MP safe", but the only danger is not resetting the
 * b_prio field, which is no worse than trying, so it doesn't really matter,
 * especially since an MP collision here will be quite rare.
 */
#ifdef _WSIO
#define make_wanted(bp) { \
	int p = (u.u_procp->p_flag & SRTPROC) ? 0 : u.u_procp->p_nice; \
	(bp)->b_wanted = 1; /* MP safe */ \
	if (p < (bp)->b_prio) \
	    (bp)->b_prio = p; \
}
#else /* _WSIO */
#define make_wanted(bp) { \
	(bp)->b_wanted = 1; /* MP safe */ \
}
#endif /* _WSIO */

static void bump_priority();
static void release_inval();	/* put B_INVAL buffer on the free list */
void bremfree();		/* remove buffer from the free list */
static void release_buffer();	/* put buffer on the free list */ 
static void release_empty();	/* add buffer to the empty list */

struct buf *incore();

int toppriority = 0;		/* buffers put at the head of the list	*/
				/* assume this priority.		*/
int non_empty;			/* number of non-empty buffers. */
int binvalcount = 0;
unsigned int getblk_real_hit, getblk_miss;

extern char *panicstr;

/*
 * 8804 - dah
 * Keep 3 lists: preferred (use old LRU), unpreferred (use old AGE), and
 * virt (new).  A buffer becomes preferred when it has been hit several
 * times since it came into the cache.  The virt list keeps track of headers
 * that were recently kicked out of the cache, but we still want to keep
 * counts on in case they are on their way to being preferred (e.g.
 * repeatedly accessing a 600K file when pref is 400K and unpref is 400K).
 *
 * Remember that the most available (oldest) buffer is kept at the front.
 */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
#ifdef	FSD_KI
bread(vp, blkno, size, bptype)
#else	FSD_KI
bread(vp, blkno, size)
#endif	FSD_KI
	struct vnode *vp;
	daddr_t blkno;
	int size;
#ifdef	FSD_KI
	int bptype;
#endif	FSD_KI
{
	register struct buf *bp;

	if (size == 0)
		panic("bread: size 0");
	cnt.f_bread++;                 /* Increment global reads for monitor */
	cnt.f_breadsize += size;                    /* Accumulate sizes also */
#ifdef	FSD_KI
	bp = getblk(vp, blkno, size, bptype);
#else	FSD_KI
	bp = getblk(vp, blkno, size);
#endif	FSD_KI
	if (bp->b_flags&B_DONE) {
		cnt.f_breadcache++;  /* Count cache hits for monitor */
		trace(TR_BREADHIT, vp, blkno);
		return(bp);
	}
	bp->b_flags |= B_READ | B_FSYSIO;
	if (bp->b_bcount > bp->b_bufsize)
		panic("bread");
	bsetprio(bp);
	if (panicstr == NULL)
		KPREEMPTPOINT();
	VOP_STRATEGY(bp);
	if (panicstr == NULL)
		KPREEMPTPOINT();
	trace(TR_BREADMISS, vp, blkno);
	u.u_ru.ru_inblock++;		/* pay for read */
#ifdef SAR
	readdisk++;
#endif 
	biowait(bp);
	return (bp);
}

/*
 * Read in the block, like bread, but also start I/O on n
 * read-ahead blocks (which are not allocated to the caller)
 */
/* 
 * rablks is an array of physical blocks to read asynchronously (read-ahead).
 * rabss is an array of three entries organized as:
 * 
 *    rabss[0] -> number of read aheads to do
 *    rabss[1] -> block size of all but last read ahead
 *    rabss[2] -> block size of the last read ahead 
 *			(or the only one if only one)
 */
struct buf *
#ifdef	FSD_KI
breadan(vp, blkno, size, rablks, rabss, bptype)
#else	FSD_KI
breadan(vp, blkno, size, rablks, rabss)
#endif	FSD_KI
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t *rablks; int *rabss;
#ifdef	FSD_KI
	int bptype;
#endif	FSD_KI
{
	register struct buf *bp, *rabp;
	int ra;

	bp = NULL;
	/*
	 * If the block isn't in core, then allocate
	 * a buffer and initiate i/o (getblk checks
	 * for a cache hit).
	 */
	if (!incore(vp, blkno)) {
		cnt.f_bread++;         /* Increment global reads for monitor */
		cnt.f_breadsize += size;           /* Accumulate sizes also */
#ifdef	FSD_KI
		bp = getblk(vp, blkno, size, bptype);
#else	FSD_KI
		bp = getblk(vp, blkno, size);
#endif	FSD_KI
		if ((bp->b_flags&B_DONE) == 0) {
		KPREEMPTPOINT();
			bp->b_flags |= B_READ | B_FSYSIO;
			if (bp->b_bcount > bp->b_bufsize)
				panic("breada");
			bsetprio(bp);
			KPREEMPTPOINT();

			VOP_STRATEGY(bp);
			KPREEMPTPOINT();
			trace(TR_BREADMISS, vp, blkno);
			u.u_ru.ru_inblock++;		/* pay for read */
#ifdef SAR
			readdisk++;
#endif 
		} else {
			cnt.f_breadcache++;  /* Count cache hits for monitor */
			trace(TR_BREADHIT, vp, blkno);
		}				
	}
	KPREEMPTPOINT();
	/*
	 * Find out which read ahead blocks to actually do I/O on.  This
	 * is a performance trick that assumes that normally all but the 
	 * last few read-ahead are already incore from previous calls to 
	 * breadan().
	 */
	for (ra = rabss[0]-1; ra >= 0; --ra)
		if (incore(vp, rablks[ra]))
			break;
	++ra;
	cnt.f_breada += ra;	/* bump global read ahead count for monitor */
	cnt.f_breadacache += ra;

	/*
	 * Start I/O for those read ahead blocks not incore.
	 */
	for (; ra < rabss[0]; ++ra) {
		int ra_size = ra < rabss[0]-1 ? rabss[1] : rabss[2];

		VASSERT(ra_size > 0);
		cnt.f_breada++;	/* Increment global read aheads for monitor */
		cnt.f_breadasize += ra_size;	/* Accumulate sizes also */
#ifdef	FSD_KI
		rabp = getblk(vp, rablks[ra], ra_size, bptype);
#else	FSD_KI
		rabp = getblk(vp, rablks[ra], ra_size);
#endif	FSD_KI
		KPREEMPTPOINT();
		if (rabp->b_flags & B_DONE) {
			brelse(rabp);
			cnt.f_breadacache++;  /* Count cache hits */
			trace(TR_BREADHITRA, vp, blkno);
		} else {
#ifdef _WSIO
			rabp->b_proc = 0;	/* disown this buffer. */
#endif  /* _WSIO */
			rabp->b_flags |= B_READ|B_ASYNC|B_FSYSIO;
			if (rabp->b_bcount > rabp->b_bufsize)
				panic("breadrabp");
			bsetprio(rabp);
			KPREEMPTPOINT();
			VOP_STRATEGY(rabp);
			KPREEMPTPOINT();
			trace(TR_BREADMISSRA, vp, rablock);
			u.u_ru.ru_inblock++;	/* pay in advance */
#ifdef SAR
			readdisk++;
#endif 
		}
	}

	/*
	 * If block was in core, let bread get it.
	 * If block wasn't in core, then the read was started
	 * above, and just wait for it.
	 */
    	/*
    	 * Kernel premption is allowed in this routine.  It will work
	 * because the in-core buffer is obtained via bread.  If this
	 * is changed to just get the buffer from memory, then the 
	 * premption points will not work.
	 */
	if (bp == NULL)
#ifdef	FSD_KI
		return (bread(vp, blkno, size, bptype));
#else	FSD_KI
		return (bread(vp, blkno, size));
#endif	FSD_KI
	biowait(bp);
	return (bp);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *
#ifdef	FSD_KI
breada(vp, blkno, size, rablkno, rabsize, bptype)
#else	FSD_KI
breada(vp, blkno, size, rablkno, rabsize)
#endif	FSD_KI
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablkno; int rabsize;
#ifdef	FSD_KI
	int bptype;
#endif	FSD_KI
{
	daddr_t rablks[1]; 
	int rabss[3];

	if (rablkno) {
		rablks[0] = rablkno;
		rabss[0] = 1;
		rabss[2] = rabsize;
	} else
		rabss[0] = 0;

#ifdef	FSD_KI
	return breadan(vp, blkno, size, rablks, rabss, bptype);
#else	FSD_KI
	return breadan(vp, blkno, size, rablks, rabss);
#endif	FSD_KI
}

int numdirty = 0;
int bwrite_count = 0;
int bwritesync_count = 0;
int bxwrite_count = 0;
int biodone_write_count = 0;

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
bwrite(bp)
	register struct buf *bp;
{
	register flag;

	++bwrite_count;
	cnt.f_bwrite++;               /* Increment global writes for monitor */
	cnt.f_bwritesize += bp->b_bcount;           /* Accumulate sizes also */
#ifdef SAR
	lwrite++;
#endif 
	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	bp->b_flags |= B_FSYSIO;


	if (flag & B_DELWRI) {
		--numdirty;
		reassignbuf(bp, bp->b_vp);
	} else
		u.u_ru.ru_oublock++;		/* no one paid yet */
	bsetprio(bp);

	if (!(flag & (B_DELWRI|B_ASYNC))) {	/* If a direct write request */
		bp->b_flags |= B_SYNC;		/* Indicate so */
		++bwritesync_count;
	}

	trace(TR_BWRITE, bp->b_vp, bp->b_blkno);
	if (bp->b_bcount > bp->b_bufsize)
		panic("bwrite");
	if (panicstr == NULL)
		KPREEMPTPOINT();
	
#ifdef _WSIO
	bp->b_proc = 0;		/* disown this buffer. really only needed for */
				/* B_ASYNC buffers.			*/
#endif  /* _WSIO */

	if (!order_write(bp))
		VOP_STRATEGY(bp);
	/* else bp was ordered after some other write(s), which now
	 * point to it, and it will be scheduled at biodone time for
	 * the last write pointing to it
	 */

	if (panicstr == NULL)
		KPREEMPTPOINT();
#ifdef SAR
	writedisk++;
#endif 
	/*
	 * If the write was synchronous, then await i/o completion.
	 * If the write was "delayed", then we put the buffer on
	 * the q of blocks awaiting i/o completion status.
	 */
	
	if ((flag&B_ASYNC) == 0) {
		biowait(bp);
		brelse(bp);
#ifdef	FSD_KI
	} else if (flag & B_DELWRI) {
		cnt.f_delwrite++;	/* delayed write buffer written */
#endif	FSD_KI
	}
}

#ifdef AUTOCHANGER
/* These are copied from autoch.h, which has 500 lines of stuff I
 * don't want to import into this file, especially at this point in
 * the release cycle.  They are to special case the autochanger for
 * ordered writes, because the autochanger has the defect of sleeping
 * in strategy when called from the ics.  Hopefully, in the future,
 * the autochanger strategy will be fixed, and then this won't have to
 * be here at all.  Also, we want to do fs_async per file-system,
 * and then these checks would be file-system attribute checks instead
 * of device type checks.
 */
#ifdef _WSIO
#define AC_MAJ_BLK 10   /* autochanger driver major block number */
#else /* s800 */
#define AC_MAJ_BLK 12   /* autochanger driver major block number */
#endif  /* _WSIO */
#endif /* AUTOCHANGER */

/*
 * If this write should follow other already scheduled writes, then
 * link it up on the ordered write list.  There are two directions
 * the ordered write list can grow: within a transaction (b_ord_intra)
 * and between transactions (b_ord_inter).
 */
order_write(bp)
    struct buf *bp;
{
    int s;
    int ordered;
#ifdef AUTOCHANGER
    /*
     * Don't do ordered writes for the autochanger because it can sleep
     * in strategy, which must be called from the ics in order to do 
     * ordered writes.
     */
    int async = major(bp->b_dev) == AC_MAJ_BLK ? -1 : fs_async;
#else
    int async = fs_async;
#endif /* AUTOCHANGER */

    bp->b_ord_infra = NULL;
    bp->b_ord_refcnt = 0;
    if (async != 0)
	bp->b_flags2 &= ~B2_ORDMASK;

    if ((bp->b_flags & B_ORDWRI) == 0 && (bp->b_flags2 & B2_ORDMASK) == 0)
	return 0;

    /* 
     * See if this write is a continuation of a transaction.  A transaction
     * is the list of writes done through bxwrite() between calls to 
     * bordend().  The uarea tracks the last block, and buffers track the
     * process that used them.  So we can do a cache look-up of the previous
     * block and see if it has yet to have biodone() called for it.
     */
    ordered = 0;
    s = CRIT();
    if (bp->b_flags & B_ORDWRI) {
	bp->b_ord_proc = u.u_procp;
	if (u.u_ord_blk != -1) {
	    struct buf *prevbp;

	    prevbp = incore(bp->b_vp, u.u_ord_blk);
	    if (prevbp && prevbp != bp && (prevbp->b_flags & B_ORDWRI) 
		       && (prevbp->b_ord_proc == u.u_procp)) {
		if (prevbp->b_ord_infra == NULL) {
			prevbp->b_ord_infra = bp;
			++bp->b_ord_refcnt;
			ordered = 1;
		}
#ifdef OSDEBUG	
		  /* u.u_ord_blk should be associated with a vnode.  We
		  ** assume it belongs to the same vnode that we are writing
		  ** at this time.  If it does not, then it is possible to
		  ** clobber an ordered write chain on another vnode.  If
		  ** b_ord_infa is not null, then it is a sign that we need 
		  ** to clean up the higher level code to do bordend() in 
		  ** all the correct places. 
		  */
		  else {
			printf ("Ordered write infra collision vp=%x\n",
				prevbp->b_ord_infra);
				
		}
#endif /* OSDEBUG*/	
	    }
	} 
	u.u_ord_blk = bp->b_blkno;
    }
    /*
     * See if this write must be ordered after other transactions.   There
     * are two streams of transactions per file system, one for links to 
     * meta-data (e.g. inodes) and one for links to normal data (file contents).
     */
    if (bp->b_flags2 & B2_ORDMASK) {
	bp->b_flags |= B_ORDWRI;
	if (bp->b_flags2 & (B2_UNLINKMETA | B2_LINKMETA)) {
	    if (bp->b_vp->v_ord_lastmetalink) {
		bp->b_vp->v_ord_lastmetalink->b_ord_inter = bp;
		++bp->b_ord_refcnt;
		ordered = 1;
	    }
	    bp->b_vp->v_ord_lastmetalink = bp;
	    bp->b_ord_inter = NULL;
	} else {
	    if (bp->b_vp->v_ord_lastdatalink) {
		bp->b_vp->v_ord_lastdatalink->b_ord_inter = bp;
		++bp->b_ord_refcnt;
		ordered = 1;
	    }
	    bp->b_vp->v_ord_lastdatalink = bp;
	    bp->b_ord_inter = NULL;
	}
    }
    UNCRIT(s);

    return ordered;
}

bordend()
{
    u.u_ord_blk = -1;
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
bdwrite(bp)
	register struct buf *bp;
{
	register long flags;

	flags =  bp->b_flags;
	bp->b_flags = flags | B_DELWRI | B_DONE | B_FSYSIO;

	if ((flags & B_DELWRI) == 0) {
		u.u_ru.ru_oublock++;		/* no one paid yet */
		++numdirty; 
		reassignbuf(bp, bp->b_vp);
	}

	cnt.f_bdwrite++;    /* Inc global delayed writes for monitor */
	cnt.f_bdwritesize += bp->b_bcount;  /* Accumulate sizes also */
#ifdef SAR
	lwrite++;
#endif 

	brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
bawrite(bp)
	register struct buf *bp;
{
	bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * Select type of write based on the reliability vs. performance level
 * of fs_async:
 *
 *	-1:	standard 8.0 file system
 *	 0:	faster file system, but without losing disk recoverability
 *	 1:	faster file system, sacrificing disk recoverability, but
 *			not too badly
 *	 2:	fastest file system, also most dangerous
 */
bxwrite(bp, nondirty)
	register struct buf *bp;
	int nondirty;		/* true => dirty write would be dangerous */
{
#ifdef AUTOCHANGER
    /*
     * Don't do ordered writes for the autochanger because it can sleep
     * in strategy, which must be called from the ics in order to do 
     * ordered writes.
     */
    int async = major(bp->b_dev) == AC_MAJ_BLK ? -1 : fs_async;
#else
    int async = fs_async;
#endif /* AUTOCHANGER */

    ++bxwrite_count;
    if (async == 0 || async == 1 && nondirty) {
	bp->b_flags |= (B_ASYNC | B_ORDWRI);
	bwrite(bp);
    } else if (async < 0) {
	bp->b_flags |= B_SYNC;
	bwrite(bp);
    } else
	bdwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 */
brelse(bp)
	register struct buf *bp;
{
	register s;
	int flags;

	/*
	 * If the buffer is from the network pool, return it 
	 */
	flags = bp->b_flags;
	if (flags & B_NETBUF)
	{
		if (DUXCALL(BRELSE_TO_NET)(bp))
			return;
	}

	if (flags & (B_NOCACHE | B_ERROR))
		bp->b_flags |= B_INVAL;

	/* HP REVISIT.  Should an error result in the buffer being 
	   invalidated.  Should numdirty be changed under spl6?
	*/
	if (bp->b_flags & B_INVAL) { 
	    if (flags & B_DELWRI) {
	    	bp->b_flags &= ~B_DELWRI;
		--numdirty;
	    }
	    bp->b_flags &= ~B_ORDWRI;
	    bp->b_flags2 &= ~B2_ORDMASK;
	    if(bp->b_vp)
		brelvp(bp);
	}
	/*
	 * Clear the resid
	 */
	bp->b_resid = 0;

        /* Buffer header stays on hash chains, but has been invalidated.  
         * Remove from the hash chain at next convenience.  bufpages has
         * not yet been decremented by the number of pages in this buffer.
	 * HP REVISIT.  If the policy is to release the pages for all 
         * buffers with B_INVAL set, then you might do things a little 
         * differently.  Could use the B_NOCACHE bit when stealing dirty
         * buffers and writing to the disk.  Need to be careful when 
         * bufpages is decremented.  May hurt performance to be release
         * pages and virtual addresses all the time, however this is tied
         * to the algorithm (getnewbuf) which selects "unused" buffers.
	 *
	 * NOTE b_flags will change in dbc_fremem()
        */
	if (flags & B_PAGEOUT) {
          dbc_freemem(bp);	
        } 

	/*
	 * Stick the buffer back on the free list.  
	 */

#ifdef _WSIO
	bp->b_proc = 0;		/* disown this buffer */
#endif  /* _WSIO */


	s = CRIT();
	if (bp->b_bufsize <= 0) {
		/* block has no buffer...  Could be put onto either end of the
		 * empty list.  
		 * put at front (head) of unused buffer list 
		*/
		release_empty(bp);
	} else {
		/* May want to optimize when reference counts are updated.  
                 * need to do it for buffers without pages.
                */
        	dbc_reference (bp);
		if (bp->b_flags & B_INVAL) {
			/* block has no info...  put at front of most 
			 * free list so that it will be re-used first.
			*/
			release_inval(bp);
			binvalcount++;
	        } else {
			/* If somebody wants this buffer then promote its
			 * priority so it won't dissapear on us.
			*/
			if (bp->b_wanted){
				int pri;
				pri =  (bp->access_cnt + toppriority)/2;
				if (bp->access_cnt < pri)
					bp->access_cnt = pri;
			}
			release_buffer(bp);
		}
	}


	bp->b_flags &= ~(B_BUSY|B_SYNC|B_ASYNC|B_NOCACHE|B_FSYSIO);
	bclearprio(bp);
	/*
	 * If someone's waiting for the buffer, or
	 * is waiting for a buffer wake 'em up.
	 */
	if (bp->b_flags&B_WANTED) {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
	if (bp->b_wanted){
		bp->b_wanted = 0;
		wakeup((caddr_t)bp);
	}
	if (bfreelist[0].b_wanted){
		bfreelist[0].b_wanted = 0;
		wakeup((caddr_t)bfreelist);
	}
#ifdef	FSD_KI
	KI_brelse(bp);	/* tell KI buffer is being put on LRU/AGE */
#endif	/* FSD_KI */

	UNCRIT(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
struct buf *
incore(vp, blkno)
	struct vnode *vp;
	daddr_t blkno;
{
	register struct buf *bp;
	register struct buf *dp;

	dp = BUFHASH(vp, blkno);
	SPINLOCK(buf_hash_lock);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	    if (bp->b_blkno == blkno && bp->b_vp == vp &&
		(bp->b_flags & B_INVAL) == 0) {
		    SPINUNLOCK(buf_hash_lock);
	            return bp;
	    }
	SPINUNLOCK(buf_hash_lock);
	return NULL;
}


/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 *
 * We use splx here because this routine may be called
 * on the interrupt stack during a dump, and we don't
 * want to lower the ipl back to 0.
 */
struct buf *
#ifdef	FSD_KI
getblk(vp, blkno, size, bptype)
#else	FSD_KI
getblk(vp, blkno, size)
#endif	FSD_KI
	struct vnode *vp;
	register daddr_t blkno;
	int size;
#ifdef	FSD_KI
	int bptype;
#endif	FSD_KI
{
	register struct buf *bp, *dp, *bp2;
	register s;
#if defined(__hp9000s800) && defined(VOLATILE_TUNE)
	extern volatile struct timeval time;
#else
	extern struct timeval time;
#endif /* __hp9000s800 && VOLATILE_TUNE */
	int dux_rem_req = 0;
	int checkdup;

#ifdef hp9000s800
	VASSERT(!(ON_ICS));
#endif /* hp9000s800 */
	if (size > MAXBSIZE)
                panic ("getblk: size > MAXBSIZE");

	/*
	 * Search the cache for the block.  If we hit, but
	 * the buffer is in use for i/o, then we wait until
	 * the i/o has completed.
	 */
	dp = BUFHASH(vp, blkno);
loop:
	SPINLOCK(buf_hash_lock);
        checkdup = ((struct bufhd *)dp)->b_checkdup;
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_vp != vp ||
		    bp->b_flags&B_INVAL) {
			continue;
		}

	        SPINUNLOCK(buf_hash_lock);
		s = CRIT();

		/* We let go of the locks above so we need to double 
		 * check that we still have the correct buffer.
		*/ 
		if (bp->b_blkno != blkno || bp->b_vp != vp ||
		    bp->b_flags&B_INVAL) {
			UNCRIT(s);
			goto loop;
		}

		if (bp->b_flags&B_BUSY) {
			make_wanted(bp);
#ifdef	FSD_KI
			cnt.f_bufbusy++; /* count number times buf busy */
#endif	FSD_KI

		/*  This is a flag used by SAR to keep track of
		    The system is waiting for i/o               */

#ifdef SAR
			syswait.iowait ++;
#endif /* hp9000s800 */
			sleep((caddr_t)bp, PRIBIO+1);
#ifdef SAR
			syswait.iowait --;
#endif /* hp9000s800 */
			UNCRIT(s);
			goto loop;
		}
		bremfree(bp);
		bp->b_flags |= B_BUSY;
		UNCRIT(s);
		/* phys->phys transition */
#ifdef	NSYNC
		bp->io_tv = time.tv_sec;
		bp->sync_time = 0;
#endif	/* NSYNC */
		if (brealloc(bp, size) == 0) {
			goto loop;
		}
		bp->b_flags |= B_CACHE;
		bump_priority(bp);
		getblk_real_hit++;
#ifdef _WSIO
		bp->b_proc = u.u_procp;	
#endif  /* _WSIO */
		return (bp);
	}
/*
	if (major(dev) >= nblkdev)
		panic("blkdev");
*/
	SPINUNLOCK(buf_hash_lock);

	/*
	 * dux_buf_check(vp) is used to check for remote DUX request
	 * deadlocks.  There are three return values from dux_buf_check():
	 *   0: If the vp is not VDUX.
	 *  -1: If the vp is VDUX and we have exceeded some limit as to
	 *	the number of buffers that we can use for remote requests,
	 *	we sleep, waiting for one to be freed and then return -1.
	 *   1: If the vp is VDUX and we haven't exceeded any limit.
	 * In the 0 case, we just continue on.	In the -1 case we loop.
	 * In the 1 case we must mark the bp returned by getnewbuf with two
	 * flags bits, B_NETBUF and B_DUX_REM_REQ.
	 */
	if ((dux_rem_req = dux_buf_check(vp)) == -1)
		goto loop;

        /*
         * getnewbuf may need to sleep.  Some other process may bring
         * our buffer in for us.  We check the timestamp on the hash
         * chain to see if it has been modified.   If so, we need to
         * take a second look at the hash chain.
         */
#ifdef	FSD_KI
	bp = getnewbuf(bptype, BC_DEFAULT);
#else	FSD_KI
	bp = getnewbuf(BC_DEFAULT);
#endif	FSD_KI
	bgetvp(vp, bp);		/* associate this buffer with a vnode */
        bp->b_blkno = blkno;

        SPINLOCK(buf_hash_lock);
        if (checkdup != ((struct bufhd *)dp)->b_checkdup) {
	    for (bp2 = dp->b_forw; bp2 != dp; bp2 = bp2->b_forw) {
		if (   bp2->b_blkno == blkno 
		    || bp2->b_vp == vp 
		    || bp2->b_flags & B_INVAL) {

			SPINUNLOCK(buf_hash_lock);
			if (dux_rem_req)
                        	dux_buf_fail();
			bp->b_flags |= B_INVAL;
			brelse(bp);
			goto loop;
		}
	    }
        }
        bremhash(bp);
        ((struct bufhd *)dp)->b_checkdup++;
        binshash(bp, dp);
        SPINUNLOCK(buf_hash_lock);

	bp->access_cnt =  toppriority - non_empty/2;
	VASSERT(bp->access_cnt >= 0);

	if (dux_rem_req)
		bp->b_flags |= B_NETBUF|B_DUX_REM_REQ;

	getblk_miss++;
#ifdef	NSYNC
	bp->io_tv = time.tv_sec;
	bp->sync_time = 0;
#endif	/* NSYNC */
	bp->b_bcount = 0;
	bp->b_dev = vp->v_rdev;
#ifdef _WSIO
	bp->b_offset = blkno << DEV_BSHIFT;
#endif /* _WSIO */
	bp->b_error = 0;
	bp->b_resid = 0;
	if (brealloc(bp, size) == 0) {
		if (dux_rem_req)
			dux_buf_fail();
		goto loop;
	}
	if (dux_rem_req)
		dux_buf_size(bp->b_bufsize);
	return (bp);
}


/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf *
geteblk(size)
	int size;
{
	register struct buf *bp;
#ifdef hp9000s800
	if (ON_ICS)
		panic ("geteblk: called from ICS"); 
#endif /* hp9000s800 */
	if (size > MAXBSIZE)
		panic ("geteblk: size > MAXBSIZE");
loop:
#ifdef	FSD_KI
	bp = getnewbuf(B_geteblk|B_unknown, BC_DEFAULT);
#else	FSD_KI
	bp = getnewbuf(BC_DEFAULT);
#endif	FSD_KI
	bp->b_flags |= B_INVAL;
	bp->b_bcount = 0;
	SPINLOCK(buf_hash_lock);
	bremhash(bp);
    {
	register struct bufqhead *flist;
	flist = &bfreelist[BQ_AGE];
	binshash(bp, flist);
    }
	SPINUNLOCK(buf_hash_lock);
	bp->b_error = 0;
	bp->b_resid = 0;
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

int brealloc_collisions = 0;

/*
 * Allocate space associated with a buffer.
 * If can't get space, buffer is released
 */
brealloc(bp, size)
	register struct buf *bp;
	int size;
{
	daddr_t start, last;
	register struct buf *ep;
	struct buf *dp;
	int s;


	/*
	 * First need to make sure that all overlaping previous I/O
	 * is dispatched with.
	 */
#ifdef	FSD_KI
	if (size == bp->b_bcount) {
		/* save pid that last used this buffer */
		bp->b_upid = u.u_procp->p_pid;

		/* site(cnode) that last used this bp */
		bp->b_site = u.u_procp->p_faddr; /* process cnode */

		/* stamp with last useage time */
		KI_getprectime(&bp->b_timeval_qs);

		return (1);
	}
#else	FSD_KI
	if (size == bp->b_bcount)
		return (1);
#endif	FSD_KI
	/*
	 * THE FOLLOWING COMMENT DESCRIBES A BUG WHICH IS IN THE CODE
	 * BELOW.  THIS BUG IS NEVER HIT BY THE READS AND WRITES GENERATED
	 * BY ANY CURRENT FILE SYSTEM. THIS COMMENT DESCRIBES A FIX TO
	 * THIS BUG, TO BE USED FOR FUTURE REFERENCE IF FILE SYSTEM
	 * ALGORITHMS EVER CHANGE SO THIS IS HIT.
	 *
	 * If size > bp->b_count and we are reading this block in
	 * and B_DELWRI is set, the b_count data which has never 
	 * been written to disk may be lost if we do not write
	 * out this buffer now.  No current file systems generate
	 * such reads -- they always read full blocks unless reading the
	 * end of a file where the last part of the file is a fragment.
	 * It is therefore not appropriate to fix this problem now.  If
	 * this bug becomes a problem in the future, the correct fix is:  
	 * modify bread(), getblk(), and brealloc() so that bread() can 
	 * send a flag down to getblk() indicating * it is a read request, 
	 * getblk() can send this flag down * to brealloc(), and brealloc() 
	 * can do something like:
	 *   if ((size > bp->b_count) && (bp->b_flags & DELWRI) && (reading)) {
	 *	bwrite(bp);
	 *	return(0);
	 *   }
	 *
	 * NOTE:  The fix is NOT to just bwrite() the buffer if 
	 * size <> bp->b_count -- this leads to enormous performance
	 * degradation for user programs sequentially writing files
	 * using write sizes of less than a full block, as every time
         * the file grows, the block is brealloced and this would lead
         * to a physical write for each fragment of the file.
	 */
	if (size < bp->b_bcount) { 
		if (bp->b_flags & B_DELWRI) {
			bwrite(bp);
			return (0);
		}
		return (allocbuf(bp, size));
	}
	bp->b_flags &= ~B_DONE;
	if (bp->b_vp == (struct vnode *) 0)
		return (allocbuf(bp, size));

	/*
	 * Search cache for any buffers that overlap the one that we
	 * are trying to allocate. Overlapping buffers must be marked
	 * invalid, after being written out if they are dirty. (indicated
	 * by B_DELWRI) A disk block must be mapped by at most one buffer
	 * at any point in time. Care must be taken to avoid deadlocking
	 * when two buffer are trying to get the same set of disk blocks.
	 */
	start = bp->b_blkno;
	last = start + btodbup(size) - 1;
	dp = BUFHASH(bp->b_vp, bp->b_blkno);
loop:
	SPINLOCK(buf_hash_lock);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep == bp || ep->b_vp != bp->b_vp || (ep->b_flags&B_INVAL))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btodbup(ep->b_bcount) <= start)
			continue;
		SPINUNLOCK(buf_hash_lock);
		s = CRIT();
		if (ep->b_flags&B_BUSY) {
			make_wanted(ep);
			sleep((caddr_t)ep, PRIBIO+1);
			UNCRIT(s);
			goto loop;
		}
		bremfree(ep);
		ep->b_flags |= B_BUSY;
		UNCRIT(s);
		brealloc_collisions++;
		if (ep->b_flags & B_DELWRI) {
			bwrite(ep);
			goto loop;
		}
		ep->b_flags |= B_INVAL;
		brelse(ep);
		SPINLOCK(buf_hash_lock);
	}
	SPINUNLOCK(buf_hash_lock);
	return (allocbuf(bp, size));
}

/* We got here because bp was a dirty block at head of AGE/LRU list */
/* Search a max of (16) blocks for a max of (8) dirty sequential blocks */
/* Note: this code ignores the virtual list because it does not work */

/* Turns sequential enqueueing on/off */
int	 enqueue_seq_flag = 1;

/* Maximum number of blocks to enqueue */
int 	ENQUEUE_MAX = 8;

/* Search for blocks that are multiple of primary file system blocks */
int 	ENQUEUE_BLOCK_SIZE = (MAXFRAG);

/* Multiplier of maximum number of ENQUEUE_MAX blocks to look at */
int 	ENQUEUE_MAX_INTERLEAVE = 2;

enqueue_seq_blks(bp)
register struct buf *bp;
{
	register struct buf *dp;
	struct 	vnode *vp;
	daddr_t wr_blkno;	/* current block numb to try */
	int 	blks_enqueued;	/* current count of enqueue blocks */
	int 	max_blkno;	/* highest+1 block to enqueue */
	int	s;

	/* Get starting block and disk -- hash chain parameters */
	vp = bp->b_vp;
	wr_blkno = bp->b_blkno;

	/* Put 1st block on disk queue, but dont wait for it */
	bp->b_flags |= B_ASYNC;
	bwrite(bp);

	/* forget about unaligned blocks */
	if (wr_blkno % (ENQUEUE_BLOCK_SIZE))
	{
		return;
	}
	/* calculate max last+1 block to enqueue */
	max_blkno = wr_blkno + 
		(ENQUEUE_MAX * ENQUEUE_MAX_INTERLEAVE * ENQUEUE_BLOCK_SIZE);

	/* bump to next block for try */
	wr_blkno += ENQUEUE_BLOCK_SIZE;

	/* Try to find next trial block by searching hash chain */
	/* Note: all exits from this loop must restore SPL */
	for (blks_enqueued = 0; 
	     wr_blkno < max_blkno && blks_enqueued < (ENQUEUE_MAX-1);
             wr_blkno += ENQUEUE_BLOCK_SIZE)
	{
		s = CRIT();
		/* check if next block is in core and dirty 
		** we have no clue as to what the real increment is XXX 
		*/
		dp = BUFHASH(vp, wr_blkno);

		/* search hash chain for required block */
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	    		if ((bp->b_blkno == wr_blkno) && (bp->b_vp == vp))
				break;
		
		if (bp != dp &&	       /* check to see if we found the block */
		   ((bp->b_flags & B_INVAL) == 0) &&   /* and if it is valid */
		   (bp->b_flags & B_DELWRI)) 	  	  /* and it is dirty */
		{
			/* if on queue, give up on this section of disk */
			if (bp->b_flags&B_BUSY) 
			{
				UNCRIT(s);
				return;
			}
			/* stick it on disk queue */
			bremfree(bp);
			bp->b_flags |= (B_BUSY|B_ASYNC);
			UNCRIT(s);
			bwrite(bp);

			/* count dirty blocks put on queue */
			blks_enqueued++; 
		} else
		{
			/* did not find block, get some air */
			UNCRIT(s);
		}
	}
}
/*
 * BEGIN_DESC
 *
 * alloc_more_headers()
 *
 * Return Value:
 *      If sucessful, a TRUE value (1) is returned indicating that there is
 *      now an empty buffer on the BQ_EMPTY list. 
 *
 * Globals Referenced:
 *
 * Description:
 *      alloc_more_headres() allocates more buffer headers and puts them on
 *      the BQ_EMPTY list.  It can fail in its task if it can't get any 
 *      memory (it does not wait for memory).
 *
 * Algorithm:
 *
 * In/Out conditions: Buffer cache lock (SPL level) assumed on entry.  Lock
 *                    is released but re-aquired before exit. 
 *
 * END_DESC
 */

int
alloc_more_headers(s)
int *s;				/* SPL level we are under. */
{

	extern struct buf * dbc_alloc();
	extern int buffer_high;
	struct buf *bp, *dp;
	int i;

        /* Create some more buffer headers.  HP REVISIT.  NOT MP SAFE.
         * Other routines traverse the list of buffer headers.
        */
#ifdef OSDEBUG
        struct bufqhead *bqp;
        bqp = &bfreelist[BQ_EMPTY];
        VASSERT(bqp->av_forw == bqp);
#endif /* OSDEBUG */

	UNCRIT(*s);
        bp = dbc_alloc (&i);
        *s = CRIT();

        if (bp == NULL) return 0;

        /* move buffers to the EMPTY list */
        nbuf += i;
	non_empty +=i;
	toppriority += i;
	/* HP REVISIT   MP note.  toppriority should always be larger than
			non_empty/2 to ensure that buffer priorities are
			allways positive values.  Until the add_empty()
			has been called for all buffers that could be
			violated. 	
	*/
#ifdef hp9000s800
        if ((nbuf * (MAXBSIZE/NBPG)) > btop(SIZEV_THIRDQUAD))
                buffer_high = btop(SIZEV_THIRDQUAD);  /* 1 gigabyte. */
#endif /* hp9000s800 */

        while (bp != NULL) {
            binshash(bp, &bfreelist[BQ_AGE]); /* put on any hash chain */
            bp->b_flags = B_BUSY|B_INVAL;
	    bclearprio(bp);
	    release_empty(bp);
            
	    /* for now we assign one page of virtual address space to each
	       buffer header.  This will be adjusted as needed.
	    */
            bp->b_virtsize = 1;
            bp->b_un.b_addr = bfget(1);
            VASSERT(bp->b_un.b_addr != NULL);
#ifdef hp9000s800
            bp->b_spaddr = ldsid(bp->b_un.b_addr);
#else
            bp->b_spaddr = KERNELSPACE;
#endif /* hp9000s800 */

            dp=bp;                  /* Put buffer on global list */
            bp=bp->b_nexthdr;
            dp->b_nexthdr = dbc_hdr;
            dbc_hdr = dp;
        }

	/*
	 * PHKL_2762
	 *
	 * Be sure to do a wakeup just in case there are others
	 * waiting for buffers in the BQ_EMPTY chain.  That could
	 * happen if they went to sleep due to memory pressure.
	 */
	if (bfreelist[0].b_wanted) {
	   bfreelist[0].b_wanted = 0;
	   wakeup((caddr_t)bfreelist);
	}

	return 1;
}
/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.
 * Callers beware.  If getnewbuf has to sleep waiting for a buffer,
 * there is a race condition where another process may bring in the
 * specific block you are looking for. If no free buffers
 * were available and another process allocates the same buffer while
 * it was sleeping, two copies of the same block will be in memory
 */
struct buf *
#ifdef	FSD_KI
getnewbuf(bptype, flags)
int bptype;
#else	FSD_KI
getnewbuf(flags)
#endif	FSD_KI
u_int flags;			/* Try to allocate a "new" buffer as opposed
				 * to reusing an existing (non empty) buffer
				*/
{
    register struct buf *bp;
    register struct bufqhead *dp;
    int s;

    /*  We have the opportunity here to add some memory to the buffer 
     *  cache, but that can only happen if we return an EMPTY buffer header
     *  and force the caller to find memory for it.  Don't worry about 
     *  memory pressure being too high, because we don't WAIT in our request
     *  to malloc more buffer headers.   It is allocbuf()'s responsibility 
     *  to reuse pages if memory pressure is too high.  We don't worry about
     *  creating too many buffer headers, because once the supply of pages
     *  stops growing, EMPTY buffer headers will occur naturally.
     *  HP REVISIT.  Even more optimal if under memory pressure we return
     *               a non-empty buffer.
     *
     *	Note that alloc_more_headers releases the critical region, so
     *  any pointers into the free list may be stale.
    */

loop:

	bp = NULL;
	s = CRIT();
	dp = &bfreelist[BQ_AGE];
	bp = dp->av_forw; 

  /* First try to reuse a B_INVAL buffer at the LRU end of the free list. 
   * If that fails and the caller can tolerate a buffer without any memory
   * (BC_DEFAULT), then return an empty buffer header.
   * And if we can't get an empty buffer header, then reuse a good buffer 
   * on the LRU end of the free list.
  */
	if (   (bp != (struct buf *)dp) 
	    && (bp->b_flags & B_INVAL)){	/* Tune.. turn on B_INVAL on */
						/* freelist header.	     */
	    binvalcount--;
	    bp = dp->av_forw; 
	} else {

	    dp = &bfreelist[BQ_EMPTY];
	    if (  (flags & BC_DEFAULT) && 
	          (! MEMORY_PRESSURE)  &&
	          (    (dp->av_forw != (struct buf *)dp) 
			|| alloc_more_headers(&s))) { 
#ifdef OSDEBUG
		getfreebuf++;
#endif /* OSDEBUG */	 
		non_empty++;
		bp = dp->av_forw; 
		VASSERT(bp->which_list == NULL);
    	    } else {
		dp = &bfreelist[BQ_AGE];
		bp = dp->av_forw; 
		if (bp == (struct buf *)dp) {	/* no free blocks */
		    VASSERT(dp == bfreelist);
#ifdef  FSD_KI
                    cnt.f_flsempty++;	/* free list empty */
                    cnt.f_clnbkfl--;	/* clean block not found */
#endif  FSD_KI
		    make_wanted(dp);
		    sleep((caddr_t)dp, PRIBIO+1);
		    UNCRIT(s);
		    goto loop;
        	} else {

  		    /* If we are forced into re-using a buffer with valid data, 
		     * then we should credit ourselves as having released some 
		     * pages voluntarily.  Note that the request could come 
		     * from allocbuf, which may only use some of the pages. The 
		     * whole buffer gets trashed however, so we will give 
		     * ourselves credit for the whole thing.
		     */ 
		    if (! (bp->b_flags & B_DELWRI)) {
			dbc_vhandcredit += btop(bp->b_bufsize);
			toppriority++;
		    }
		}
    	    }
	}


	bremfree(bp);
	bp->b_flags |= B_BUSY;
	UNCRIT(s);


	/* HP REVISIT
	 * Would like to prevent the bursting of IO that could happen if 
	 * numerous dirty buffers are at the end of the AGE list.  Someone
	 * should be looking ahead to write these to the disk.
	 * HP REVISIT  These buffers should not have their reference bits set.
	*/
	if (bp->b_flags & B_DELWRI) {
		if (enqueue_seq_flag) {
			enqueue_seq_blks(bp);
		} else {
			bp->b_flags |= B_ASYNC;
			bwrite(bp);
		}
		goto loop;
	}

	if (bp->b_vp)
		brelvp(bp);
#ifdef	FSD_KI
	/* clean block found immediately */
	cnt.f_clnbkfl++;

	/* save pid that allocated this buffer */
	bp->b_apid = u.u_procp->p_pid;

	bp->b_bptype = bptype;

	/* site(cnode) that last used this bp */
	bp->b_site = u.u_procp->p_faddr; /* process cnode */

	/* stamp with allocation time */
	KI_getprectime(&bp->b_timeval_eq);

	KI_getnewbuf(bp);		/* tell KI buffer is being allocated */
#endif	FSD_KI
	trace(TR_BRELSE, bp->b_vp, bp->b_blkno);
	bp->access_cnt =  toppriority - non_empty/2;
	bp->b_flags = B_BUSY;
	if (bp->b_resid)
		panic("getnewbuf: resid set");
	/*record that we have allocated a buffer for selftest purposes*/
	TEST_PASSED(ST_BUFFER);
#ifdef _WSIO
	bp->b_proc = u.u_procp;	
#endif  /* _WSIO */
	return (bp);
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
biowait(bp)
	register struct buf *bp;
{
	register s;

	s = CRIT();
	while ((bp->b_flags&B_DONE)==0) {
#ifdef SAR
		syswait.iowait ++;
#endif /* hp9000s800 */
		sleep((caddr_t)bp, PRIBIO);
#ifdef SAR
		syswait.iowait --;
#endif /* hp9000s800 */
	}
	UNCRIT(s);
	if (u.u_error == 0) 			/* XXX */
		u.u_error = geterror(bp);
}

static struct buf *pending_done_tl = NULL;

int bio_errs = 0;



/*
 * Mark I/O complete on a buffer.
 * If someone should be called, e.g. the pageout
 * daemon, do so.  Otherwise, wake up anyone
 * waiting for it.
 */
biodone(bp)
	register struct buf *bp;
{
	int s;

	if (bp->b_flags & B_DONE)
		panic("B_DONE set in biodone");
	bp->b_flags |= B_DONE;
#if defined (hp9000s800) && !defined(_WSIO)
/* Modifying b_bcount breaks the s300 drivers */
/* Snakes drivers behave like s300 drivers, so don't modify for snakes either */
	if (bp->b_resid)
		bp->b_bcount -= bp->b_resid;
#endif /* hp9000s800 && !_WSIO */

	/*
	 * If b_pcnt is non-null, then this request was part of a group
	 * of I/Os.  Decrement the request counter, and if it is now
	 * zero, issue a wakeup on the counter.
	 */
	if (bp->b_pcnt != (int *)0) {
		s = CRIT();

		if (--(*bp->b_pcnt) <= 0)
			wakeup(bp->b_pcnt);
		bp->b_pcnt = (int *)0;
		UNCRIT(s);
	}

        /* Although it looks like you should call drivers with
         * their own iodone routine before you do the above 
         * request counter work, it turns out that pageout 
         * depends on the above wakeup before it gets called
         * here -- so leave this here!  -- pam 01/04/94
        */ 
	if (bp->b_flags & B_CALL) {
		bp->b_flags &= ~B_CALL;
		(*bp->b_iodone)(bp);
		return;
	}

#ifdef	NSYNC
	if (bp->b_flags & B_PHYS) {
		bp->sync_time = 0;
		bp->io_tv = time.tv_sec;
	}
#endif	/* NSYNC */

	if (bp->b_flags & B_ERROR) {
		bio_errs++;
#ifdef OSDEBUG
		msg_printf("block I/O error: device %x, block %d, [pid %d]\n",
			   bp->b_dev, bp->b_blkno, bp->b_upid);
#endif	
	}

	if (bp->b_flags & B_ORDWRI)
		ordered_biodone(bp);
	else
		finish_biodone(bp);
}

finish_biodone(bp)
    struct buf *bp;
{
    int s;
    int pages;
    extern int dbc_parolemem;
    extern int parolemem, pageoutcnt;

    if (bp->b_flags & B_ASYNC) {
	if (bp->b_flags & B_PAGEOUT) {
	    pages = btop(bp->b_bufsize);
	    s = CRIT();
	    dbc_parolemem -= pages;
	    parolemem -= pages;
	    pageoutcnt += pages;
	    UNCRIT(s);
	}
	brelse(bp);
    } else {
	bp->b_flags &= ~B_WANTED;
	bp->b_wanted = 0;
	wakeup((caddr_t)bp);
    }
}

/* 
 * Schedule writes pointed to by bp.  These writes have been waiting for
 * status on bp because they must get to non-volatile media after bp.
 * It is the driver's job to make sure biodone() is not called for bp
 * until it is guaranteed that subsequently scheduled writes will reach
 * non-volatile media after bp.
 */
/* If strategy() calls biodone(bp) before returning, this function
 * will recurse, a chain of such writes could recurse to an 
 * arbitrary depth.  Since it would be better to iterate for such
 * circumstances, we detect the recursion, and iterate instead.  
 * Also, such recursions could nest (an ongoing recursion in a 
 * process context could be interrupted by an ics biodone
 * which will also recurse).
 *
 * Thus, a specific recursion must be detected and the mechanism
 * cannot interfere with other possible recursions.
 */
ordered_biodone(bp)
    struct buf *bp;
{
    int s;
    struct buf *donebp;
    static int processing_writes = 0;

    /*
     * Since this write is done, make it no longer a candidate to be
     * ordered after.
     */
    if ((bp->b_flags2 & B2_ORDMASK) && !bp->b_ord_inter) {
	VASSERT(bp == bp->b_vp->v_ord_lastdatalink ||
	        bp == bp->b_vp->v_ord_lastmetalink);
	if (bp == bp->b_vp->v_ord_lastdatalink)
	    bp->b_vp->v_ord_lastdatalink = NULL;
	else
	    bp->b_vp->v_ord_lastmetalink = NULL;
    }
    bp->b_flags2 &= ~B2_ORDMASK;
    bp->b_flags &= ~B_ORDWRI;

    /*
     * See if there is already a call to ordered_biodone() processing
     * buffers.  If so put this buffer on the list of pending
     * buffers, so that it will be processed by the original call,
     * not this call.  This is to prevent indefinite amounts of recursion.
     */
    s = CRIT();
    bp->av_forw = NULL;
    if (processing_writes) {
	VASSERT(pending_done_tl);
	pending_done_tl->av_forw = bp;
	pending_done_tl = bp;
	UNCRIT(s);
	return;
    } else {
	processing_writes = 1;
	pending_done_tl = bp;
	UNCRIT(s);
    }

    /*
     * Process this write, plus all others pending.
     */
    do {
	ordered_strategy(bp);
	s = CRIT();
	donebp = bp;
	bp = bp->av_forw;
	if (bp == NULL) {
	    pending_done_tl = NULL;
	    processing_writes = 0;
	}
	UNCRIT(s);
	finish_biodone(donebp);
    } while (bp);
}

ordered_strategy(bp)
    struct buf *bp;
{
    struct buf *nextbp;	/* pointer to future write, impacts refcnt */
    struct buf *schedbp;
    int s;
    int schedtype;
    struct buf *endbp;

    nextbp = bp->b_ord_inter;
    bp->b_ord_inter = NULL;
    /* nextbp links bp->b_ord_inter, bp unlinks it, implying
			    bp_ord_refcnt stays the same*/

    s = CRIT();
    if (nextbp && nextbp->b_ord_inter) {
	schedtype = nextbp->b_flags2 & B2_ORDMASK;
	for (endbp = nextbp->b_ord_inter; endbp->b_ord_inter;
						endbp = endbp->b_ord_inter) {
	    if ((schedtype & B2_UNLINKDATA) && (endbp->b_flags2 & B2_LINKDATA)||
	        (schedtype & B2_LINKMETA) && (endbp->b_flags2 & B2_UNLINKMETA))
		break;
	    schedtype |= endbp->b_flags2 & B2_ORDMASK;
	}
    } else
	endbp = NULL;
    UNCRIT(s);

    do {
	s = CRIT();
	if (bp->b_ord_infra) {
	    schedbp = bp->b_ord_infra;
	    bp->b_ord_infra = NULL;
	    --schedbp->b_ord_refcnt;    /* bp unlink */
	} else if (nextbp && nextbp != endbp) {
	    schedbp = nextbp;
	    nextbp = schedbp->b_ord_inter;
	    --schedbp->b_ord_refcnt;    /* nextbp unlink */
	    if (nextbp)
		++nextbp->b_ord_refcnt; /* nextbp link */
	    if (endbp && schedbp->b_ord_inter != endbp) {
		schedbp->b_ord_inter = endbp;
		--nextbp->b_ord_refcnt; /* schedbp unlink */
		++endbp->b_ord_refcnt;  /* schedbp link */
	    }
	} else {
	    schedbp = NULL;
	    if (nextbp) {
		--nextbp->b_ord_refcnt; /* nextbp unlink ('cause we're done) */
		VASSERT(nextbp->b_ord_refcnt > 0);
	    }
	}
	if (schedbp && schedbp->b_ord_refcnt == 0) {
	    UNCRIT(s);
	    VOP_STRATEGY(schedbp);
	    ++biodone_write_count;
	} else
	    UNCRIT(s);
    } while (schedbp);
}

#ifdef hp9000s700
/*
 * The following globals are for dealing with different weights and units
 * on the factors effecting bp_priority(): 
 * 	time:		how long the buffer is in the queue, linear at
 *				first, then logarithmic
 *	latency: 	how long it will take to seek 
 *	priority:	nice value of process scheduling the I/O
 *	async:		file system asynchronous request
 * 	offset:		amount to move everthing to get a positive signed int
 */
int bpsort_latency_shift = 0;	/* exponent of multiplication for latency */
int bpsort_priority_shift = 11;	/* exponent of multiplication for priority */
int bpsort_time_shift = 9;	/* exponent of multiplication for time */
int bpsort_async_shift = 2;	/* exponent of division for time for 
					asynchronous file system writes */
int bpsort_offset = 0x40000000;	/* keep result positive */

int bpsort_time_break = HZ;	/* border between linear and logarithmic
					time behavior */

/*
 * Return the priority of bp w.r.t. disk queue sorting.
 * The function returns a signed integer, but the value is always non-negative.
 * Smaller numbers indicate better priorities.
 * The input parameter, latency, is the disk driver's best guess at the number
 * of micro-seconds latency which will be incurred by this I/O given the
 * current state of the disk.
 *
 * This function assumes it will only be executed on one
 * member of a single sequential steam.  That is, it selects which sequential
 * stream to work on next.  If asked to differentiate between two members
 * of the same sequential stream, it may select that they be done backwards
 * if the second one is closer to the disk head's current position.
 *
 * Note: drivers that call this function should set b_enq_ticks at enqueue
 * time.
 */
bp_priority(bp, latency)
struct buf *bp;
{
    int time;
    
    time = (ticks_since_boot - bp->b_enq_ticks) * (100 / HZ);

    if (time > bpsort_time_break) {
	int extratime;

	extratime = time - bpsort_time_break;
	time = bpsort_time_break;
	while (extratime >>= 1)
	    ++time;
    }

    if (bp->b_wanted == 0 && !(bp->b_flags & B_WANTED) && 
			     !(bp->b_flags & B_READ) &&
			      (bp->b_flags & B_FSYSIO) && 
			      (bp->b_flags & B_ASYNC))
	return bpsort_offset +
	       (latency    << bpsort_latency_shift) +
	       (bp->b_prio << bpsort_priority_shift) -
	       (time       << bpsort_time_shift - bpsort_async_shift);
    else
	return bpsort_offset +
	       (latency    << bpsort_latency_shift) +
	       (bp->b_prio << bpsort_priority_shift) -
	       (time       << bpsort_time_shift);
}
#endif

/*
 * Insure that no part of the specified blocks are in an incore buffer.  The 
 * minimum requirement is that the disk contains an up to date copy of the
 * data.  The data will be purged from the cache if neccessary.  
 * Return value:
	1 	blkflush had to sleep waiting on a busy buffer
	0	blkflush did not have to sleep.

 * There is a test of buffer ownership that is used to avoid deadlock when
 * blkflush is called from vfs_pagein.  We want to avoid the situation where
 * a file system read is issued with a pointer to a memory mapped file. 
 * rwip() could try to aquire the buffer then fault on a page during 
 * uiomove().  We don't want the fault to wait on the buffer again.
 */
blkflush(vp, blkno, size, purge, rp)
	struct vnode *vp;
	daddr_t blkno;
	long size;
	int purge;	/* Non-zero means purge the buffer from the cache.  */
	reg_t *rp;	/* If non-null, unlock region before sleeping.	*/
{
	register struct buf *ep;
	struct buf *dp;
	daddr_t last;	/* one past the last block to flush */
	int s;
	int flags;
	int slept = 0;

	last = blkno + btodbup(size);

	for (; blkno < last; blkno = (blkno + RND) & ~(RND-1)) {
		dp = BUFHASH(vp, blkno);
loop:
		SPINLOCK(buf_hash_lock);
		for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
			/* look for overlap */
			if ((ep->b_vp != vp)
			|| (ep->b_flags&B_INVAL)
			|| (ep->b_bcount == 0)
			|| (ep->b_blkno >= last)
			|| (ep->b_blkno + btodbup(ep->b_bcount) <= blkno)){
				continue;
			}

			SPINUNLOCK(buf_hash_lock);	/* NOT MP SAFE */
			s = CRIT();
			flags = ep->b_flags;
			if (flags&B_BUSY) {
#ifdef _WSIO
				/* Don't neccessarily need the test for rp.
				 * But this isolates this functionality to
				 * vfs_pagein() and vfs_pageout()
				 */
		        	if (u.u_procp == ep->b_proc && rp != NULL) {
					UNCRIT(s);
					SPINLOCK(buf_hash_lock);
					continue;/* HP REVISIT see note above */
				}
#endif /* _WSIO */
				make_wanted(ep);
				if (rp != NULL) {
				    if (! slept) 
					regrele(rp);
				}
				slept = 1;
				sleep((caddr_t)ep, PRIBIO+1);
				UNCRIT(s);
				goto loop;
			}
			if (flags & B_DELWRI) {
				/*
				 * A dirty buffer, write it out and purge it
				 * from the buffer cache.
				 */
				bremfree(ep);
				if (purge)
					ep->b_flags |= B_BUSY|B_NOCACHE|B_SYNC;
				else    ep->b_flags |= B_BUSY|B_SYNC;
				UNCRIT(s);
				bwrite(ep);
				goto loop;
			}
			/*
			 * A clean buffer, purge it from the buffer cache if 
			 * neccessary.
			 */
			if (purge) {
				bremfree(ep);
				ep->b_flags |= B_BUSY|B_NOCACHE;
				UNCRIT(s);
				brelse(ep);
				SPINLOCK(buf_hash_lock);
			} else {
				UNCRIT(s);
				/* NOT MP SAFE. The new9 branch does it right */
				SPINLOCK(buf_hash_lock);
			}
		}
		SPINUNLOCK(buf_hash_lock);
	}
	if ((rp != NULL) && slept) {
		reglock(rp);
	}
	return slept;
}

/*
 * Make sure all write-behind blocks
 * associated with vp
 * are flushed out.
 * (from fsync)
 *
 * DEPENDENCIES
 *	Possible interactions with bremfree() and binvalfree()
 */
bflush(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct buf *blist;
	register struct buf *nbp;
	register struct bufqhead *flist;
	int s;

	/* HP REVISIT  Should we drop from SPL6 from time to time? */
	blist = NULL;
	s = spl6();

	if (vp == (struct vnode *) 0) {

	    /* Anything dirty on the free list gets written. */
	    for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
		for (bp = flist->av_forw; 
		     bp != (struct buf *)flist; 
		     bp = nbp) {

		    nbp = bp->av_forw;		/* Possible dependency */
		    if (bp->b_flags & B_DELWRI) {
			notavail(bp);
			bp->av_forw = blist;
			blist = bp;
		    }
		}
	} else {

		/* Find all dirty buffers. */
		bp = vp->v_dirtyblkhd;
		while (bp != NULL) {
		    if ((bp->b_flags&B_BUSY) == 0) {
			notavail(bp);
			bp->av_forw = blist;
			blist = bp;
		    }
		    bp = bp->b_blockf;		/* Possible dependency */
		}
	}

	(void) splx(s);

	bp = blist;
	while (bp != NULL) {
		bp->b_flags |= B_ASYNC;
		blist = bp->av_forw;
		bwrite(bp);
		bp = blist;
	}
}

#ifdef	NSYNC

#ifdef hp9000s800
#define SYNC_PARAM	2
#else
#define SYNC_PARAM	5
#endif /* hp9000s800 */

#define	AGE_TO_SYNC	30		/* default age to sync */

static int sync_period = AGE_TO_SYNC;
static long sync_last  = 0;		/* last time normal syncer is up */

static int syncd_run  = 0;		/* flag to activate syncd */
static int syncd_cnt  = 0;		/* count syncd since last syncer on */
static int syncd_next = 2*AGE_TO_SYNC;	/* interval to wake up syncd */

sendsync()
{
	wakeup((caddr_t) &sbolt);
        timeout(sendsync, (caddr_t) 0, syncd_next*hz);
}

/* Syncer daemon */

syncd_fork()
{
	char *process_name = "syncdaemon";
	register struct buf *bp;
	register struct bufqhead *flist;
	register int t;
	int i = 0;
	int s;
	sv_sema_t sdSS;
	pid_t newpid;
	struct proc *pp, *allocproc();


	newpid = getnewpid();
	pp = allocproc(S_DONTCARE);
	proc_hash(pp, 0, newpid, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_DAEMON, pp)) {
		/*
		 * Give syncd its name.
		 */
		pstat_cmd(u.u_procp, process_name, 1, process_name);
		u.u_syscall = KI_SYNCDAEMON;

		do {
			u.u_comm[i] = process_name[i];
		} while (process_name[i++]);

		PXSEMA(&filesys_sema, &sdSS);
		while (1) {

		/*
		 * updlock is used by update() in ufs_subr.c to serialize
		 * disk flushing activities, see the comment in update().
		 */

			if (!updlock_locked() && syncd_run) {
				updlock_set();
				t = 0;

				if (syncd_cnt++ > (sync_period + AGE_TO_SYNC)) {
					syncd_next  = 2*AGE_TO_SYNC;
					sync_period = AGE_TO_SYNC;
					syncd_run   = 0;
					sync_last   = 0;
				}


				s = spl6();
				for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
				for (bp=flist->av_forw; 
				     bp != (struct buf *)flist; 
				     bp=bp->av_forw) {
					if ((time.tv_sec - bp->io_tv) < sync_period) 
						break;

					if (t++ < AGE_TO_SYNC) {
						if ((bp->b_flags & B_INVAL) || (!(bp->b_flags & B_DELWRI)))
							continue;

						bp->b_flags |= B_ASYNC;
						notavail(bp);
						bwrite(bp);
					}
					break;
				}
				(void) splx(s);
				unlock_update();
			}
			sleep((caddr_t) &sbolt, PZERO-1);
		}
		/*NOTREACHED*/
	}
}

tflush()
{
	register struct buf *bp;
	register struct bufqhead *flist;
	int s;

	if (!syncd_run) {
		syncd_run++;
		syncd_next = SYNC_PARAM;
	}

	if (!sync_last ||
	    ((sync_period = time.tv_sec - sync_last) < AGE_TO_SYNC))
		sync_period = AGE_TO_SYNC;

	sync_last = time.tv_sec;
	syncd_cnt = 0;

loop:
	s = spl6();
	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
		for (bp = flist->av_forw; 
		     bp != (struct buf *)flist; 
		     bp = bp->av_forw) {
			if ((bp->b_flags & B_INVAL) || !(bp->b_flags & B_DELWRI))
				if (IFKPREEMPTPOINT()){
					(void) splx(s);
					goto loop;
				}
				else
					continue;

			if (!bp->sync_time)
				bp->sync_time = sync_last;

			if (((time.tv_sec - bp->io_tv) >= sync_period) ||
			    (bp->sync_time != sync_last)) {
				bp->b_flags |= B_ASYNC;
				notavail(bp);
				(void) splx(s);
				bwrite(bp);
				KPREEMPTPOINT();
				goto loop;
			}
		}
	(void) splx(s);
}
#endif	/* NSYNC */

/* 
 * bpurge() after free()'ing, lest we kick out
 * garbage delwrite blocks.
 *  HP REVISIT.  Can you quit after finding the block no you are
 *		interested in?  On MP systems can the block no change
 *		between the time you release the buf_hash_lock and 
 *		aquire the buffer?
 */
bpurge (vp, blkno)
	register struct vnode *vp;
	daddr_t blkno;
{

    register struct buf *dp, *bp;
    int s;

    dp = BUFHASH(vp, blkno);
    SPINLOCK(buf_hash_lock);
    for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	if (bp->b_blkno == blkno && bp->b_vp == vp) {
	    if (bp->b_flags& B_BUSY)
		break;

	    SPINUNLOCK(buf_hash_lock);
	    s = spl6();
	    notavail(bp);
	    (void) splx(s);

	    bp->b_flags |=  B_INVAL;
	    brelse(bp);
	    SPINLOCK(buf_hash_lock);
	}
    SPINUNLOCK(buf_hash_lock);
}

/*
 * Invalidate blocks associated with vp which are on the freelist.  Could
 * Make sure all write-behind blocks associated with vp are flushed out.
 */
binvalfree(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register s;

	VASSERT(vp != NULL);
loop:
	s = CRIT();

	/* HP REVISIT  Note that we do not neccessarily wait for dirty
	 * buffers to be written.  Is this okay?  On 8.07 we did not
	 * even mark the dirty buffers with B_NOCACHE so they could 
	 * have been returned to the free list.  Do we really need to
	 * branch back to loop for each clean buffer that is invalidated?
	 * Same question for each dirty buffer that is written.
	*/

	/* Write and Clobber dirty buffers */
	for (bp = vp->v_dirtyblkhd; bp; bp=bp->b_blockf) {
		VASSERT(bp->b_flags & B_DELWRI);
		if ((bp->b_flags&B_BUSY) == 0) {
			notavail(bp);
			bp->b_flags |= (B_ASYNC|B_NOCACHE);
			UNCRIT(s);
			bwrite(bp);
			goto loop;	
		}
	}

	/* Clobber clean buffers */
	for (bp = vp->v_cleanblkhd; bp; bp=bp->b_blockf) {
		if ((bp->b_flags&B_BUSY) == 0) {
			notavail(bp);
			bp->b_flags |= B_INVAL;
			UNCRIT(s);
			brelse(bp);
			goto loop;
		}
	}
	UNCRIT(s);
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if (bp->b_flags&B_ERROR)
		if ((error = bp->b_error)==0)
			return (EIO);
	return (error);
}

/*
 * Invalidate in core blocks belonging to closed or umounted filesystem
 *
 * This is not nicely done at all - the buffer ought to be removed from the
 * hash chains & have its dev/blkno fields clobbered, but unfortunately we
 * can't do that here, as it is quite possible that the block is still
 * being used for i/o. Eventually, all disc drivers should be forced to
 * have a close routine, which ought ensure that the queue is empty, then
 * properly flush the queues. Until that happy day, this suffices for
 * correctness.						... kre
 * This routine assumes that all the buffers have been written.
 */
binval(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register s;

loop:
	s = spl6();

	/* Clobber dirty buffers */
	while ((bp = vp->v_dirtyblkhd) != NULL) {
		if (bp->b_flags&B_BUSY) {
			make_wanted(bp);
			sleep((caddr_t)bp, PRIBIO+1);
			splx(s);
			goto loop;
		}
		notavail(bp);
		bp->b_dev = NODEV;	/* clobber dev?  ala invalip_cache() */
		bp->b_flags |= B_INVAL;
		brelse(bp);
		(void) splx(s);		/* These two statements needed? */
		goto loop;		/* avoid long spl6 intervals? */
	}

	/* Clobber clean buffers */
	while ((bp = vp->v_cleanblkhd) != NULL) {
		if (bp->b_flags&B_BUSY) {
			make_wanted(bp);
			sleep((caddr_t)bp, PRIBIO+1);
			splx(s);
			goto loop;
		}
		notavail(bp);
		bp->b_dev = NODEV;	/* clobber dev?  ala invalip_cache() */
		bp->b_flags |= B_INVAL;
		brelse(bp);
		(void) splx(s);		/* These two statements needed? */
		goto loop;		/* avoid long spl6 intervals? */
					/* HP REVISIT invalip_cache() holds */
					/* spl6() for entire time. */
	}
	(void) splx(s);
}

#ifdef NEVER_CALLED
set_delwri (bp) 
struct buf *bp;
{
	++numdirty;
	reassignbuf (bp, bp->b_vp);	
}
#endif /* NEVER_CALLED */

clear_delwri (bp)
struct buf *bp;
{
	--numdirty;
	reassignbuf (bp, bp->b_vp);	
}

/*
 * Reassign a buffer from one vnode to another, or move buffer from clean
 * list to dirty list or vice versa.  Right now it only supports moving 
 * between lists.  newvp must be the same as bp->b_bp.
 *
 * Could be used in the future to assign file specific control information
 * (indirect blocks) to the vnode to which they belong.
 */
reassignbuf(bp, newvp)
        register struct buf *bp;
        register struct vnode *newvp;
{
        register struct buf     *bq, **listheadp;
        register struct vnode   *vp;
        register                 s;

	VASSERT(newvp == bp->b_vp);	/* DELETE THIS WHEN NOT DESIRED */
        VASSERT(newvp != NULL);
        vp = bp->b_vp;
        /*
         * Delete from old vnode list, if on one.
         */
        if (vp != (struct vnode *) 0) {
                VASSERT(bp->b_blockb != 0);
		s = CRIT();
                if (bq = bp->b_blockf)
                        bq->b_blockb = bp->b_blockb;
                *bp->b_blockb = bq;
                UNCRIT(s);
        }
        /*
         * If dirty, put on list of dirty buffers;
         * otherwise insert onto list of clean buffers.
         */
        if (bp->b_flags & B_DELWRI)
                listheadp = &newvp->v_dirtyblkhd;
        else
                listheadp = &newvp->v_cleanblkhd;
	s = CRIT();
        if (*listheadp) {
                bp->b_blockf = *listheadp;
                bp->b_blockb = listheadp;
                bp->b_blockf->b_blockb = &bp->b_blockf;
                *listheadp = bp;
        } else {
                *listheadp = bp;
                bp->b_blockb = listheadp;
                bp->b_blockf = NULL;
        }
        UNCRIT(s);
        bp->b_vp = newvp;
}


/*
 * Disassociate a buffer from a vnode.
 */
brelvp(bp)
	struct buf *bp;
{
	struct vnode *vp;
	struct buf   *bq;
	int s;

	if (bp->b_vp == (struct vnode *) 0) {
		return;
	}

	/* Delete buffer from old vnode list. */
	s = CRIT();
	/* Grab lock to change vnode list? */
        if (bp->b_blockb) {
                if (bq = bp->b_blockf)
                        bq->b_blockb = bp->b_blockb;
                *bp->b_blockb = bq;
                bp->b_blockf = NULL;
                bp->b_blockb = NULL;
        }
	/* Release lock to change vnode list? */
	UNCRIT(s);

	vp = bp->b_vp;         /* bug fix: save vp because VN_RELE may sleep */
	bp->b_vp = (struct vnode *) 0;
	VN_RELE(vp);
}

/*
 * Associate a buffer with a vnode.
 */
bgetvp(vp, bp)
        register struct vnode *vp;
        register struct buf *bp;
{
        int s;

        VASSERT(vp != NULL);
        VASSERT(bp->b_vp == NULL);
        VN_HOLD(vp);
        bp->b_vp = vp;

        /*
         * Insert onto list for new vnode.
         */
        s = CRIT();
        if (vp->v_cleanblkhd) {
                bp->b_blockf = vp->v_cleanblkhd;
                bp->b_blockb = &vp->v_cleanblkhd;
                vp->v_cleanblkhd->b_blockb = &bp->b_blockf;
                vp->v_cleanblkhd = bp;
        } else {
                vp->v_cleanblkhd = bp;
                bp->b_blockb = &vp->v_cleanblkhd;
                bp->b_blockf = NULL;
        }
	UNCRIT(s);
}




/* The purpose of this routine is to release the vnode associated */
/* with a buffer.  This is needed when the inode table on a discless */
/* node gets full and it can't get a new inode just because it is */
/* being held high by the buffer cache. */

bufvpfree()
{
	register struct buf *bp;
	register struct bufqhead *flist;
	register struct vnode *vp;
	int s;

loop:
	s = CRIT();

	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
	for (bp = flist->av_forw; bp != (struct buf *)flist; bp = bp->av_forw) {
		vp = bp->b_vp;
		if ((vp) && ((vp->v_fstype==VDUX)||(vp->v_fstype==VDUX_CDFS))){
			if (bp->b_flags & B_DELWRI) {
				bp->b_flags |= B_ASYNC;
				bremfree(bp);
				bp->b_flags |= B_BUSY;
				UNCRIT(s);
				bwrite(bp);
				goto loop;
			} else {
				notavail(bp);
				UNCRIT(s);
				bp->b_flags |= B_INVAL;
				brelse(bp);
				return(1);
			}
		}
	}
	UNCRIT(s);
	return(0);
}

#if (defined(_WSIO) || defined(__hp9000s300))

/*
 * block_to_raw() maps a block device dev_t to a raw device dev_t by
 * searching the device table for the correct matching d_open()
 * routine.
 */
dev_t
block_to_raw(dev)
	dev_t dev;
{
	dev_t maj = major(dev);
	int i;

	for (i = 0; i < nchrdev; i++)
		if (cdevsw[i].d_open == bdevsw[maj].d_open)
			return makedev(i, minor(dev));
	return NODEV;
}

/* pre_open on the 300 just checks for the root pseudo-device.	On the
 * 800, it does lu->mi mapping.
 *
 * In either case, it uses pointers to a dev_t and a minor int to return
 * the device and minor numbers, and a pointer to the mode to determine
 * whether a character or block device is needed.
 *
 * It returns 0 on success, ENXIO on failure.
 */

/* ARGSUSED */
pre_open(devp, modep, minnump)
	dev_t *devp;
	u_int *modep;
	int   *minnump;
{
	if (*devp == NODEV) {
		if (*modep == IFCHR)
			*devp = block_to_raw(rootdev);
		else
			*devp = rootdev;
		if (minnump != NULL)
			*minnump = minor(*devp);
	}

	if (major(*devp) >= ((*modep == IFCHR) ? nchrdev : nblkdev))
		return ENXIO;
	return 0;
}
#endif	/* _WSIO || hp9000s300 */

#ifdef NEVER_CALLED
/*
 *      dirtybufs():  Called  from  boot() to  determine  the  number of
 *      buffers  that  need to be  written  to disk,  but  have not been
 *      scheduled for such.
 */
dirtybufs()
{
        register struct buf *bp;        /* used to count dirty buffers */
        register int ndirty;            /* the count of dirty buffers */

        for ( ndirty = 0, bp = dbc_hdr; bp != NULL; bp = bp->b_nexthdr)
                if ((bp->b_flags & (B_DELWRI|B_INVAL)) == B_DELWRI)
                        ndirty++;

        return ndirty;
}
#endif /* NEVER_CALLED */

/*
 *      busybufs():  Called  from  boot() to  determine  the  number  of
 *      buffers  still waiting to be written out as a result of the call
 *      to update().
 */
busybufs()
{
        register struct buf *bp;        /* used to count busy buffers */
        register int nbusy;             /* the count of busy buffers */

        for ( nbusy = 0, bp = dbc_hdr; bp != NULL; bp = bp->b_nexthdr)
                if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
                        nbusy++;

        return nbusy;
}

/*
 * Initialize hash links for buffers.
 */
bhinit()
{
	extern int bufhash_chain_length;
	register int target_headers;
	register int i;
	register struct bufhd *bp;

        /*
         * Allocate buffer hash queue space.  The desired hash chain length 
	 * is bufhash_chain_length.  The size MUST be a power of 2, and the 
	 * mask BUFMASK is a lower-bits-on mask for the BUFHASH function.
         * It is difficult (not impossible) to change the size of the hash
         * chain headers once the system is running.  We choose not to do that
	 * even with a buffer cache that can change in size.   We target the
	 * desired length by assuming half of memory is in the buffer cache 
	 * and that the average file system block size is 8K (2 pages).  If
	 * bufhash_chain_length is 4, then the hash queue uses 4K of memory
	 * on a 16MB system.
         */

	/* Note empty loop body */
        target_headers = (freemem / 4) / bufhash_chain_length;
        for (BUFHSZ=1; BUFHSZ<=target_headers; BUFHSZ<<=1) ;

	/*
	 *	Changed MALLOC call to kmalloc to save space. When
	 *	MALLOC is called with a variable size, the text is
	 *	large. When size is a constant, text is smaller due to
	 *	optimization by the compiler. (RPC, 11/11/93)
	 */
	bufhash = (struct bufhd *) kmalloc((sizeof (struct bufhd)) * BUFHSZ,
					    M_DBC, M_NOWAIT);
	BUFMASK = BUFHSZ - 1;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}


/* Signal whether we have have buffer pages that can be re-used.  There is
 * possibly some ecomomizing here.  We could steal pages from B_INVAL buffers
 * and place them on the free list.  But that means breaking translations
 * that might be re-established a short time later.
*/
int
dbc_invalcount()
{
  struct buf *bp;
  struct bufqhead *dp;

  dp = &bfreelist[BQ_AGE];
  bp = dp->av_forw; 
  /* TUNE.. turn on B_INVAL in free list header.  Critical section here? */
  if ((bp != (struct buf *)dp) && (bp->b_flags & B_INVAL)) 
	return (1);
  else return (0);
}

/* Synchronously flush all dirty buffers for this vnode.  Wait until B_BUSY
 * are released.
*/
dux_flush_cache(vp)
register struct vnode *vp;
{
        register struct buf *bp;
        int s;

loop:
	s = spl6();

	/* Write dirty buffers */
	for (bp = vp->v_dirtyblkhd; bp; bp = bp->b_blockf) {
		if (bp->b_flags&B_BUSY) {
			make_wanted(bp);
			sleep((caddr_t)bp, PRIBIO+1);
			splx(s);
			goto loop;
		}
		VASSERT(bp->b_flags & B_DELWRI);
		notavail(bp);
		bp->b_flags |= B_SYNC;
		bwrite(bp);
		splx(s);
		goto loop;
	}

	/* Now wait for B_BUSY buffers to come free */
	for (bp = vp->v_cleanblkhd; bp; bp = bp->b_blockf) {
		if (bp->b_flags&B_BUSY) {
			make_wanted(bp);
			sleep((caddr_t)bp, PRIBIO+1);
			splx(s);
			goto loop;
		}
	}

	/* We made it through both lists without doing any work. */
	splx(s);
}



syncip_flush_cache (dev, syncio)
dev_t dev;
int syncio;
{

	register struct buf *bp;
	int s;

	/* HP REVISIT.   This is not neccessarily MP safe.  
	 * Need to insure that the list of buffer headers is
	 * not being changed.  Buffer headers are now a linked
	 * list to implement a dynamic buffer cache.
	*/
        for (bp = dbc_hdr; bp != NULL; bp=bp->b_nexthdr) {
		if (bp->b_dev != dev || (bp->b_flags & B_DELWRI) == 0)
			continue;
		s = spl6();
		while (bp->b_flags & B_BUSY) {
			make_wanted(bp);
			sleep((caddr_t)bp, PRIBIO+1);
		}
		/* Re-check if we slept on a busy buffer */
		if (bp->b_dev != dev || (bp->b_flags & B_DELWRI) == 0){
			splx(s);
			continue;
		}
		notavail(bp);
		splx(s);
		if (syncio) {
			/*
			 * Let driver know it's synchronous
			 * needed by snakes scsi
			 */
			bp->b_flags |= B_SYNC;
			bwrite(bp);
		} else
			bxwrite (bp, 1);
	}
}



#define MAX_SEGMENT_SIZE (non_empty/8)
#define MAX_PARTITIONS 12
#define MAXPRIORITY 0x0fffffff

struct bf bfheader;
int total_partitions = 0;	/* number of partitions.		*/
struct bf *bfh;			/* pointer to topmost partition.	*/
int buffer_count = 0;		/* approximate size of the topmost partition.*/	

/* Any buffer greater than this priority is probably in the topmost segment. 
 * Not 100% accurate but doesn't have to be.  Used to adjust buffer_count 
 * when buffers are removed from the topmost partition.
*/  
int topsegmentpriority = MAXPRIORITY;	

#ifdef OSDEBUG
int remove_cnt = 0;
int remove_searches = 0;
int release_cnt = 0;	/* number of times that release_buffer() is called */
int release_searches = 0;
int partitions_destroyed = 0;
int partitions_created = 0;
#endif


bf_initialize ()
{
	bfh = &bfheader;
	bfh->bf_bp = (struct buf *)&bfreelist[BQ_AGE]; 
	bfh->bf_bp->access_cnt = MAXPRIORITY+1; 
	bfh->bf_bp->which_list = bfh; 
	bfh->bf_next = bfh;
	toppriority = non_empty;

}

/*
 * The least recently used end of the free list can be found either by 
 *
 *	(bfh->bf_bp)->av_forw;
 *		 OR
 *	&bfreelist[BQ_AGE]->av_forw; 
 *
 * B_INVAL buffers should get set to the base priority before being released.
 * Due to B_INVAL, the base priority will tend to be stale.  
 *
 * Priority headers have strictly increasing priorities.   All buffers with
 * equal or lesser priorities can be found by moving forward (av_forw) through
 * the free list.
 *
 * The toppriority is incremented whenever an existing buffer is canablized
 * (see getnewbuf()).  This ensures that buffers will eventually displace 
 * stale buffers at the top, since the priorities given new buffers are 
 * some percentage of the toppriority.  HP REVISIT.  This percentage can
 * be changed to tune the performance of the LRU.
 *
 * Whenever a buffer is inserted into the topmost partition, it is given a
 * priority larger than what is already there.  This helps maintain a more
 * uniform distribution of priority partitions (near the top).  
 *
 * One situation where this implementation breaks down is when a "small"
 * number of buffers are being referenced heavily.   This creates a big
 * bubble of low prioity buffers, and one small bubble of high priority
 * buffers.  The only way to avoid this is to maintain an accurate count
 * of the number of buffers in the topmost priority and only create new
 * partitions when absolutely neccessary.  HP REVISIT.
 *
 * If B_WANTED is set in brelse(), we adjust the priority upward so that 
 * the buffer is still around when the process needing the buffer wakes up.
 * You generally don't need to worry about starvation.
 *
 * This mechanism allows dirty buffers that are removed from the list solely
 * for the purpose of writing to disk, to be placed back into the list at
 * the same relative location.  Buffers flushed by sync() or dirty buffers
 * being canablized by getnewbuf() get put back on the LRU end of the list.
 *
 * DEPENDENCIES
 *	Possible interactions with bflush() and binvalfree()
 *
 */
void
bremfree (bp)
struct buf *bp;
{

	register struct bf *bf;
	register struct bf *prev_bf;
	register struct buf *tbp;

#ifdef OSDEBUG
	remove_cnt++;
#endif
	/* If the header points to the buffer being removed then 
	 * move it one position left.   If we were to try to move
	 * it right (to a higher priority) it would take work to pull
	 * in all the buffers of that higher priority.  We might
	 * even find that all buffers of the next higher priority 
	 * are in a segment all by themselves, and we would have
	 * to remove a segment anyway.
	 * 
	 * This action can lower the priority of this segment.
	 *
	 */
	if (bp->which_list != NULL) {

		prev_bf = bfh;
		bf = prev_bf->bf_next;
		while (bf->bf_bp != bp) {
#ifdef OSDEBUG
			remove_searches++;
#endif
			prev_bf = bf;
			bf = bf->bf_next;
		}
		tbp = bp->av_back;
		if (tbp->which_list == NULL) {
			tbp->which_list = bf;
			bf->bf_bp = tbp;
		} else {
			prev_bf->bf_next =  bf->bf_next;
			total_partitions--;
			FREE(bf, M_DYNAMIC);
		}
		bp->which_list = NULL;
	}

	if (bp->access_cnt > topsegmentpriority)
		buffer_count--;

        bp->av_back->av_forw = bp->av_forw; 
        bp->av_forw->av_back = bp->av_back; 
        bp->av_forw = bp->av_back = NULL; 

}

/* This routine is called when the toppriority is about to overflow the size
 * of an int. It should rarely happen, but will cause a burp in the freelist
 * when it does.  We can't adjust the priority of B_BUSY buffers, and since
 * we are adjusting priorities downward, most buffers not on the free list 
 * will be inserted in the topmost partition when they return.  We let
 * the dynamics of the system clean up at that point.
 */
static void
readjust_buffer_priorities()
{
	struct buf *p;
	struct buf *tbp;

	tbp=bfh->bf_bp;
	p = tbp->av_forw;
	toppriority = nbuf;	/* some convenient starting point.  Want to
				 * be larger than non_empty when done and 
				 * there could be many busy buffers.
				 */
	while (p != tbp) {
		toppriority++;
		p->access_cnt = toppriority;
		p = p->av_forw;
	}	
	VASSERT (toppriority >= non_empty);
}

/* Assumes that the priority has already been adjusted in the buffer header.
 *
 * The scenerio that this code does not handle very well is the one where
 * a majority of buffers are removed from the free list, then reinserted
 * at their old priorities.  This is a possibility for operations like
 * sync and fsync.
 */
static void
release_buffer(bp)
struct buf *bp;
{
	int pri;
	register struct bf *prev_bf;
	register struct bf *bf;
	register struct buf *tbp;


	pri = bp->access_cnt;
	VASSERT(pri < MAXPRIORITY && (pri >= 0));

	prev_bf = bfh;
	/* First check to see if we have too many partitions.  Too many
 	 * partitions degrades performance.  Consolodate the two bottom
 	 * partitions into one.
 	 */
	if (total_partitions >= MAX_PARTITIONS) {
		bf = prev_bf->bf_next;
		bf->bf_bp->which_list = NULL;			
		prev_bf->bf_next = bf->bf_next;
		total_partitions--;
		FREE(bf, M_DYNAMIC);
#ifdef OSDEBUG
		partitions_destroyed++;
#endif
	} 


	/* Find the priority segment which contains buffers of the 
	 * appropriate  priority 
	 */
	bf = prev_bf->bf_next;
	tbp = bf->bf_bp;
#ifdef OSDEBUG
	release_cnt++;
#endif
	while (pri > tbp->access_cnt) {
#ifdef OSDEBUG
		release_searches++;
#endif
		prev_bf = bf;
		bf = bf->bf_next;
		tbp = bf->bf_bp;
	}
	
	if (bf == bfh) {	/* insert into the topmost segment */

		/* If the topmost segment is fullcreate a new segment.  
		 * Need to adjust MAX_SEGMENT_SIZE so that there
		 * is a good average partition size.
	   	 */
		if (++buffer_count > MAX_SEGMENT_SIZE) {
			register struct bf *newbf;
			register struct buf *p;

			/* make sure there is at least one buffer to place
			 * in the new partition.  If buffer_count is accurate
			 * (and it should be) we don't need to check, but its
			 * cheap insurance for future code changes.
			 * We need to increment the top priority whenever we 
		 	 * create a new topmost partition.  This is neccessary 
		 	 * in order to ensure that each partition is unique.
			 */
			p = tbp->av_back;
			if (p->which_list == NULL) {
				MALLOC (newbf, struct bf *,
					(sizeof(struct bf)),
					M_DYNAMIC,
					M_NOWAIT);
				if (newbf != NULL) {
#ifdef OSDEBUG
					partitions_created++;
#endif
					total_partitions++;
					prev_bf->bf_next = newbf;
					newbf->bf_next = bf;
					newbf->bf_bp = p;
					topsegmentpriority = p->access_cnt;
					p->which_list = newbf;
					if (++toppriority > MAXPRIORITY) 
						readjust_buffer_priorities();
				}
			}
			buffer_count = 0;
		}
		if ((toppriority - topsegmentpriority) <=  buffer_count) 
			if (++toppriority > MAXPRIORITY) 
				readjust_buffer_priorities();
		bp->access_cnt = toppriority;
		inserttail(tbp, bp);

	} else {
		/* Insert at the head of this priority segment.  This may 
	 	 * promote the priority of the buffer.  HP TUNE?  Should we 
		 * insert at the tail if the priority is less than that of 
		 * the lowest priority buffer in the interval?
		 * The top segment may be empty but thats okay.  We can get 
		 * into that situation by invalidating all the buffers in the
		 * top segment.  It should be repopulated on its own.
		*/

		inserthead(tbp, bp);
		if (pri < tbp->access_cnt)
			bp->access_cnt = tbp->access_cnt;
		tbp->which_list = NULL;
		bp->which_list = bf;
		bf->bf_bp = bp;
	}

}

/* Add a buffer to the least recently used end of the free list.  The buffer 
 * will be the first to be chosen when a new buffer is needed.  This routine 
 * should be fast since we know where to put the buffer.
 * If speed were not an issue, then release_buffer() could be used to 
 * put the buffer onto the free list (at the lowest priority).
 * 
 * The priority "access_cnt" does not have to be set, but it helps with
 * debugging.
 */
static void
release_inval(bp)
struct buf *bp;
{
	register struct buf *oldbp;

	VASSERT(bp->b_flags & B_INVAL);
	bp->access_cnt = 0; 		 /* Lowest possible priority */
	bp->which_list=NULL;
	oldbp = bfh->bf_bp;
	inserthead(oldbp, bp);
}	


/* Place a buffer on the EMPTY list.  The buffer header shouldn't have any 
 * pages assigned to it.  The number of non-empty buffers is adjusted so
 * we can keep track of the buffers in use.
 */
static void
release_empty(bp)
struct buf *bp;
{
	register struct buf *oldbp;

	VASSERT(bp->b_bufsize == 0);
	oldbp = (struct buf *)(&bfreelist[BQ_EMPTY]);
	inserthead(oldbp, bp);
	non_empty--;
	VASSERT(non_empty >= 0);
}

/* Adjust the priority of a buffer.  This can be used when a buffer is "HIT".
 * HIT means that the buffer is already in the cache and it is referenced
 * again.  A couple of (unproven) formulas are used.   Can change the buffer
 * to a priority that is midway between its existing priority and the topmost
 * priority.  Or... Change the priority to a minimum of 75% of the 
 * topmost priority.  
 * 
*/
static void
bump_priority(bp)
struct buf *bp;
{
   int pri1, pri2;

   pri1 =  (toppriority + bp->access_cnt)/2;
   pri2 =  toppriority - non_empty/4;

#ifdef OSDEBUG
if ((bp->access_cnt > pri1) && (bp->access_cnt > pri2))
printf ("ogetblk %d %d %d top=%d non_empty=%d\n",
	bp->access_cnt,  pri1, pri2, toppriority, non_empty);
#endif

   if (pri1 > pri2)
	bp->access_cnt =  pri1;
   else	bp->access_cnt =  pri2;
   VASSERT(bp->access_cnt >= 0);
}


#if (defined(__hp9000s300) || defined(_WSIO)) && defined(OSDEBUG)
/*
 * Print out statistics on the current allocation of the buffer pool.
 * Can be enabled to print out on every ``sync'' by setting "syncprt"
 * above.
 */
bufstats()
{
	int s, i, j, count;
	register struct bufqhead *bp;
	register struct buf *dp;
	int counts[MAXBSIZE/NBPG+1];
	static char *bname[BQUEUES] = 
		{ "AGE", "EMPTY" };

	for (bp = bfreelist, i = 0; bp < &bfreelist[BQUEUES]; bp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE/NBPG; j++)
			counts[j] = 0;
		s = spl6();
		for (dp = bp->av_forw; 
		     dp != (struct buf *)bp; 
		     dp = dp->av_forw) {
			counts[dp->b_bufsize/NBPG]++;
			count++;
		}
		(void) splx(s);
		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE/NBPG; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * NBPG, counts[j]);
		printf("\n");
	}

	{
	register struct bf *bf;
	int invalid;
	int badlist;
		bf = bfh->bf_next;
		dp = bfh->bf_bp->av_forw;
		count = 0;
		invalid = 0;
		badlist = 0;
		while (dp != bfh->bf_bp) {
			count++;
			if (dp->access_cnt < dp->av_back->access_cnt)
				badlist = 1;
			if (dp->b_flags & B_INVAL) invalid++;
			if (dp->which_list != NULL) {
			    if (bf->bf_bp != dp)
				printf("list error\n");
			    printf("priority=%d invalid=%d bad=%d count=%d\n", 
				dp->access_cnt, invalid, badlist, count);
			    count = 0;
			    invalid = 0;
			    badlist = 0;
			    bf = bf->bf_next;
			}
			dp = dp->av_forw;
		}		
		printf("priority=%d invalid=%d bad=%d count=%d\n", 
			dp->access_cnt, invalid, badlist, count);
	}

}
#endif /* hp9000s300 */


