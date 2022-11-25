/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_dsort.c,v $
 * $Revision: 1.15.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 11:43:13 $
 */
/*
 * CURRENTLY THIS FILE CONTAINS ONLY SERIES 700 and 800 CODE
 * S300 does not use this disksort.  Series 300 now has one,
 * but it works in a different way.
 */
#ifdef __hp9000s800

/*	ufs_dsort.c	6.1	83/07/29	*/

#ifndef _KERNEL
#include "../h/types.h"
#endif _KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"

#ifdef _WSIO
#include "../h/scsi.h"
#include "../wsio/xpt.h"
#include "../wsio/s2disk.h"
#else
#include "../h/rtprio.h"
#include "../h/mirror.h"
#include "../sio/mirror2.h"
#include "../h/errno.h"
#endif

int rtdisksort = 1;
#ifdef DBG
int rtdsort_dbg = 0;
#endif DBG

/* True if two writes must maintain their current order.  There is no
 * interprocess ordering, and an ordered write must maintain its order
 * wrt other ordered writes in the same process.  It must also stay in
 * front of all writes that come after it from the same process, even if 
 * they aren't ordered. 
#define ORDERED(oldb,newb) (((oldb)->b_flags & B_ORDWRI) && \
			   !((newb)->b_flags & B_READ) && \
			   (oldb)->b_pid == (newb)->b_pid)
 */

#ifdef _WSIO

int maxtravcnt = 100;	/* maximum number of elements to traverse in queue */
int maxstarve = 16;	/* maximum number of times a hanger can pushed back */

/* Fields borrowed from s2d_queue_t structure: */
/*     Current position of disk head, set by dequeue from b_cylin. */
#define diskpos b_last_front
/*     Number of times a starved buffer has been pushed back. */
#define starvecnt b_s6

/*
 * HANGER should tell whether or not the buffer is currently hanging or
 * likely to be hanging a process.  This includes all paging I/O and
 * file system level synchronous writes (!B_ASYNC), all reads (B_READ),
 * and all buffers wanted by another process (B_WANTED or b_wanted).
 */
#define HANGER(bp) (((bp)->b_flags & (B_READ|B_WANTED)) || \
		   !((bp)->b_flags & B_ASYNC) || ((bp)->b_wanted))

/*
 * Insert a new element (bp) into an existing queue (dp).  This is done
 * in a way that tries to simultaneously satisfy the following constraints:
 *
 *	- it's nice to do things FIFO so that processes requests are 
 *		satisfied roughly in the order requested
 *		(i.e. be fair)
 *	- buffers that are causing a process to hang until their I/O is
 *		done should not stay on the queue very long
 *		(i.e. avoid starvation)
 *	- buffers that are paging related also should not stay on the 
 *		queue very long
 *	- buffers should go out in an order that minimizes disc seeks
 *		(i.e. improve throughput)
 *	- must be very time efficient: it's bad if the disk has to wait
 *		for the cpu cause its inserting another element in the
 *		queue; normal insertion sorts visit half the elements 
 *		in their list per insertion.  This takes too long if 
 *		hundereds of writes are queued all at once by syncer 
 *		or a process writing a file
 *	- don't reorder buffers that must maintain their relative order
 *		for disc recoverability reasons
 *	- don't pile up the whole buffer cache on the queue:  major
 *		problem with fast CPU with or without a disc sort.
 *		There is nothing special in this sort to avoid this.
 *
 * This function also takes into account the current disc head position.
 * This is very important for doing good sorting when only a few elements
 * are on the queue (very common case).  This is something not done
 * by the s800 version of this function.  Nor does the s800 version give
 * priority to buffers that are hanging processes.
 *
 * The concept of a "hanger" is used to separate requests into two 
 * categories.  Hangers are buffers suspending a process until the I/O
 * is done, or paging related buffers.  Non-hangers are what's left,
 * typically asynchronous writes (most writes are asynchronous).
 *
 * Seek minimization is done by keeping the queue as a set of concatenated
 * ascending sequences.  New elements are normally inserted somewhere in 
 * order in one of the ascending sequences, or at the end of the queue 
 * forming a new ascending sequence.
 *
 * Fairness, efficiency, and ordering constraints are done by searching 
 * from the tail, and, for non-hanging I/O, limiting the search by 
 * maxtravcnt.  Thus it is O(n) for iteratively inserting n asynchronous 
 * writes for (typical case for a large queue), as opposed to the s800 
 * version of this which is O(n^2).
 *
 * Starvation is prevented in two ways.  One by keeping the queue as
 * ascending sequences, making the disc head make forward sweeps across 
 * the disc.  The other method is by putting a cap on the number of times 
 * a new element may be inserted in front of a hanger.
 *
 * Priority is given to buffer that are hanging processes in the following ways:
 *    - non-hanging buffers are not allowed to be inserted in front of 
 *		hanging ones
 *    - hanging buffers may go past the first places found to insert them
 *		(searching backwards remember) if they are not yet among
 *		other hanging buffers
 *    - hanging buffers may violate the ascending order rule to get earlier
 *		in the list, as long as they do not pass another hanging
 *		buffer
 */
void disksort(dp, bp)
    register s2d_queue_t *dp;	/* queue */
    register struct buf *bp;	/* new buffer */
{
    struct buf *pred;	/* buffer to insert bp after, set every time a
				good place is found to insert */
    int cnt;		/* count-down: limit number of elements to look at 
				for inserting a non-hanger */
    int bphangs;	/* boolean: true if bp is holding up a process */
    struct buf *seenhanger = NULL;	/* first hanger seen, NULL until then */
    struct buf *qb;	/* for traversing the current queue */
    int x;		/* for m_crit() */
    int pushed_hanger = 0;	/* true once pred has been set in front of
					a hanger */
    void insert_bp();

    m_crit(x);

    bphangs = HANGER(bp);

    pred = dp->tail;	/* nominal insertion point:  FIFO style */

    for (cnt = (bphangs ? -1 : maxtravcnt), qb = dp->tail; cnt && qb; 
						--cnt, qb = qb->av_back) {
	/* does it fit between two ascending elements? */ 
	if (qb->b_cylin <= bp->b_cylin && (!qb->av_forw ||
			   bp->b_cylin < qb->av_forw->b_cylin) ||
	/* or, is this a break between two ascending lists, and bp
					    can extend one of the lists? */
	    qb->av_forw && qb->b_cylin > qb->av_forw->b_cylin &&
	    		   (bp->b_cylin < qb->av_forw->b_cylin || 
			    bp->b_cylin >= qb->b_cylin)) {
	    pred = qb;
	    if (seenhanger)
		pushed_hanger = 1;
	    if (seenhanger || !bphangs)
		goto done;
	}

	/*  Changing over to use file system to maintain order.  Once done
	    this code can be completely deleted.

	if (ORDERED(qb, bp)) {
	    if (bphangs && !seenhanger)
		pred = qb;
	    goto done;
	}
	*/

	if (bphangs) {
	    if (!seenhanger && HANGER(qb)) {
		pred = qb;
		seenhanger = qb;
	    }
	    if (HANGER(qb) && qb->starvecnt >= maxstarve)
		goto done;
	}
	else if (HANGER(qb))
	    goto done;
    }

    /* include current disk position if we got that far */
    if (qb == NULL && 
	   (dp->diskpos <= bp->b_cylin && (dp->head == NULL || 
					   bp->b_cylin <= dp->head->b_cylin) ||
	    bphangs && !seenhanger)) {
	pred = NULL;
	if (seenhanger)
	    pushed_hanger = 1;
    }

done:
    if (pushed_hanger) {	/* increment starve counts of all hangers
				   being pushed back in the queue */
	for (qb = seenhanger; qb != pred; qb = qb->av_back)
	    if (HANGER (qb))
		++qb->starvecnt;
    }
    bp->starvecnt = 0;

    insert_bp(dp, pred, bp);
    m_uncrit(x);
}

/*
 * Do a normal double list insert and update head and tail when necessary.
 * bp will be inserted after pred.  If pred is NULL, then bp becomes the
 * head of the list.
 */
void insert_bp(dp, pred, bp)
    register s2d_queue_t *dp;	/* queue to insert into */
    register struct buf *pred;	/* element to insert after (NULL => new head) */
    register struct buf *bp;	/* element to insert */
{
    if (pred == NULL) {
	bp->av_forw = dp->head;
	if (dp->head)
	    dp->head->av_back = bp;
	dp->head = bp;
    }
    else {
	bp->av_forw = pred->av_forw;
	if (pred->av_forw)
	    pred->av_forw->av_back = bp;
	pred->av_forw = bp;
    }
    bp->av_back = pred;
    if (bp->av_forw == NULL)
	dp->tail = bp;
}
#else /* not WSIO */

#define	b_cylin	b_resid

/*
 * Seek sort for disks.  We depend on the driver
 * which calls us using b_resid as the current cylinder number.
 *
 * The argument dp structure holds a b_actf activity chain pointer
 * on which we keep two queues, sorted in ascending cylinder order.
 * The first queue holds those requests which are positioned after
 * the current cylinder (in the first request); the second holds
 * requests which came in after their cylinder number was passed.
 * Thus we implement a one way scan, retracting after reaching the
 * end of the drive to the first request on the second queue,
 * at which time it becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead
 * blocks are allocated.
 */

disksort(dp, bp, mirroring)
	register struct buf *dp, *bp;
{
	register struct buf *ap;

	/*
	 * If nothing on the activity queue, then
	 * we become the only thing.
	 */
	ap = dp->b_actf;

	if(ap == NULL) {
		dp->b_actf = bp;
		dp->b_actl = bp;
		bp->av_forw = NULL;
                bp->av_back = NULL;
	        m_uncrit(x); 
		return;
	}

	if (rtdisksort == 0)
	    /*
	     * Disable privilages for realtime processes.
	     */
	    bp->b_prio = RTPRIO_MAX+1;


	/*
 	 * If mirroring, we avoid race conditions between reimaging and
	 * normal writes by ensuring that reimages are queued first.
	 * There are other parts to the strategy; see disc2.c for the details.
	 */
	if (mirroring) {
	    if (bp->b_dev & DEV_REIMAGE_MASK) {
		bp->av_forw = ap;
		dp->b_actf = bp;
		return;
	    }
     
	    /*                                                       */
	    /* Skip the REIMAGE WRITE for queue head pointer.        */
	    /*                                                       */
	    while (ap->av_forw &&
		  (ap->b_dev & DEV_REIMAGE_MASK)) ap = ap->av_forw;
	    /*                                                       */
	    /* Queue and return if no request except REIMAGE WRITE.  */
	    /*                                                       */
	    if(ap->av_forw == NULL) {
		ap->av_forw = bp;
		dp->b_actl = bp;
		bp->av_forw = NULL;
		return;
	    }

	}

	/*
	 * Let's make the code a little easier and
	 * change the realtime priority of the front
	 * of the current request so it looks as if
	 * it's the end of a previous priority queue.
	 */
	ap->b_prio = MIN(ap->b_prio, bp->b_prio-1);

	/*
	 * Position ap to the entry immediately preceeding
	 * the priority queue we want to insert into (if
	 * it exists).
	 */
	while (   ap->av_forw
	       && ap->av_forw->b_prio < bp->b_prio)
	    ap = ap->av_forw;

	/* special case multiuser to avoid starvation */
	if (bp->b_prio == RTPRIO_MAX +1) {
	    timeshare_insert(dp, ap, bp);
	    return;
	}

	/*
	 * Move ap up to the position within this priority
	 * queue where we want to insert bp.
	 *
	 * If we lie before the last request on the immediately
	 * preceeding queue (or the current active request if
	 * there is no preceeding queue) place ourselves in the
	 * second half of this priority queue.
	 */
	if (   ap->av_forw
	    && bp->b_prio == ap->av_forw->b_prio
	    && bp->b_cylin < ap->b_cylin)
	{
	    /*
	     * We belong in the second half of this queue
	     *
	     * Check for rtprio <= as when we start ap is end of previous
	     * priority queue.
	     */
	    while (   ap->av_forw
		   && ap->b_prio <= ap->av_forw->b_prio)
	    {
		/*
		 * Check for an ``inversion'' in the
		 * normally ascending cylinder numbers,
		 * indicating the start of the second request list.
		 */
		if (ap->av_forw->b_cylin < ap->b_cylin) {
		    /*
		     * Found the second half of this priority queue.
		     * Search the second half of the queue
		     * for the first request at a larger
		     * cylinder number.  We go before that;
		     * if there is no such request, we go at end.
		     */
		    do {
			if (bp->b_cylin < ap->av_forw->b_cylin)
			    goto insert;
			ap = ap->av_forw;
		    } while (   ap->av_forw
			     && ap->av_forw->b_prio == ap->b_prio);
		    goto insert;		/* after last */
		}
		ap = ap->av_forw;
	    }
	    /*
	     * No inversions... we will go after the last, and
	     * be the first request in the second request list.
	     */
	    goto insert;
	}
	/*
	 *
	 * We are >=  ap->b_cylin
	 * Ap is at the last entry in the previous priority queue
	 * (or it's the active request).
	 * Place bp into the first half of it's priority queue.
	 */
	while (   ap->av_forw
	       && ap->av_forw->b_prio == bp->b_prio)
	{
	    /*
	     * Find the location spot in this priority queue
	     * >= ap->b_cylin && <= bp->b_cylin
	     */
	    if (ap->av_forw->b_cylin < ap->b_cylin ||
		bp->b_cylin < ap->av_forw->b_cylin)
		    goto insert;
	    ap = ap->av_forw;
	}

	/*
	 * Place bp immediately following ap.
	 */
insert:
	bp->av_forw = ap->av_forw;
	ap = (ap->av_forw = bp);
	
	/*
	 * If ap is the last entry in this priority queue then
	 * reorder every queue following (ugh!)
	 */
	while (   ap->av_forw
	       && ap->av_forw->b_prio != ap->b_prio)
	{
	    register struct buf *qhead = ap->av_forw;
	    register struct buf *qtail = qhead;

	    /*
	     * If the first entry on this queue lies before
	     * ap's cylinder, move the stuff to the end of
	     * the queue.
	     */
	    if (qhead->b_cylin < ap->b_cylin) {
		register struct buf *mid_point = 0;
		/*
		 * Gather all transactions in this queue
		 *
		 * Remember where the transition point from the
		 * first half of the queue to the second half is.
		 * If every entry in this queue is < the last
		 * entry in the previous queue we need to append
		 * the first half to the end of the second half.
		 */
		while (   qtail->av_forw
		       && qtail->av_forw->b_prio == qhead->b_prio
		       && qtail->av_forw->b_cylin < ap->b_cylin)
		{
		    if (qtail->av_forw->b_cylin < qtail->b_cylin)
			mid_point = qtail;
		    qtail = qtail->av_forw;
		}
		
		/*
		 * If we ran off the end of the queue, qtail
		 * points to the mid point.
		 */
		if (   !qtail->av_forw
		    || qtail->av_forw->b_prio > qtail->b_prio)
		{
		    if (mid_point)
			qtail = mid_point;
		    else
			goto done;	/* Must be nothing to do */
		}

		/*
		 * Move this queue segment to the end of the
		 * queue for this priority.
		 */
		ap = (ap->av_forw = qtail->av_forw);
		while (   ap->av_forw
		       && ap->av_forw->b_prio == qhead->b_prio)
		    ap = ap->av_forw;

		/*
		 * Only re-link if it wasn't already at the end of
		 * the queue.
		 */
		if (ap != qtail) {
		    qtail->av_forw = ap->av_forw;
		    ap->av_forw = qhead;
		    ap = qtail;
		}
	    } else {
		/*
		 * See if there's some transactions at the
		 * back of the queue that can be moved forward.
		 */
		while (   qtail->av_forw
		       && qtail->av_forw->b_prio == qtail->b_prio
		       && qtail->av_forw->b_cylin > ap->b_cylin)
		    qtail = qtail->av_forw;

		/*
		 * If we have found something at this priority
		 * level move it up.
		 */
		if (   qtail->av_forw
		    && qtail->av_forw->b_prio == qtail->b_prio)
		{
		    ap = (ap->av_forw = qtail->av_forw);
		    while (   ap->av_forw
			   && ap->av_forw->b_prio == ap->b_prio)
			ap = ap->av_forw;
		    qtail->av_forw = ap->av_forw;
		    ap->av_forw = qhead;
		    ap = qtail;
		} else {
		    /*
		     * At least bump up ap
		     */
		    ap = qhead;
		}
	    }
	}

	/*
	 * If we're at the end of the while shebang
	 * reset dp->b_actl
	 */
	if (!ap->av_forw)
		dp->b_actl = ap;

    done: ;
#ifdef DBG
	if (rtdsort_dbg) {
		rtdsort_dbg = 0;
		rtdsort_pq(dp);
	}
#endif DBG
}

/*
 * Find and return the earliest place at or after ap that bp may be 
 * inserted without violating any ordering constraints.  This is done 
 * by starting at the end of the list and walking it backwards until a 
 * buffer is found that cannot be re-ordered wrt to bp.
 *
 *	dp:	queue data structure
 *	ap:	earliest buffer can be inserted
 *	bp:	buffer to insert
 */
struct buf *find_earliestinsert (dp, ap, bp)
     register struct buf *dp, *ap, *bp;
{
    struct buf *op;

    /* Make this check so that just in case this function is broken, or
     * the data structures it works on are broken, or causes the user
     * some kind of problem, they can turn it off by going back to normal
     * synchronous behaviour through setting fs_async = -1.
     */
    if (fs_async < 0)
	return ap;

    for (op = dp->b_actl; op != ap; op = op->av_back) {
	if (op == NULL) {
	    /* If we get to here, the av_forw, av_back, b_actf, and b_actl
	     * are not forming a consistent data structure.
	     */
	    panic ("Inconsistent queue in find_earliestinsert().\n");
	}
	if (ORDERED (op, bp))
	    break;
    }

    return op;
}

#if defined(DBG)
void rtdsort_pq(bp)
	register struct buf *bp;
{
	printf("disk_queue:");
	for (bp = bp->av_forw; bp; bp = bp->av_forw) {
		if (bp->b_prio <= RTPRIO_MAX)
			printf(" %d.%d", bp->b_prio, bp->b_cylin);
		else
			printf(" %d", bp->b_cylin);
	}
	printf("\n");
}
#endif RTPRIO && DBG

/*
 * Changed to merge static and dynamic limits - dah 9011
 * xxx - needs updated comments
 *
 * 870724
 * Several bugs involving starvation or near starvation have been
 * traced to the way we order disk requests.  The following code
 * attempts to circumvent at least the common cases seen.  It is
 * used only for timeshare processes, as realtime processes have
 * the right to choke the system.
 *
 * Two limiting techniques are used.  The basic idea is to come to
 * a compromise between maximum disk throughput (as obtained by the
 * old unmodified scan algorithm) and reasonable response time.
 * The new algorithm will never do more than disk_limit
 * accesses to the same group of cylinders before moving on to the next.
 * This prevents one process from filling the buffer cache with
 * delayed writes, then freezing the system for seconds to minutes
 * while the sequential writes are posted to disk.  Many writes
 * which are scattered over the disk are less of a problem, since then
 * the normal cylinder ordering gives other processes an occasional
 * access.  
 *
 * The other problem addressed is steady-state access to the same
 * cylinder.  In this case there may not be many requests queued
 * up at one time, but they come in as fast as they are finished.
 * The dynamic count (disk_dynamic_limit) forces cylinder progress
 * in this case.
 *
 * If the above requirements are met, the queue is normal.  Otherwise,
 * extra requests are tacked on after the normal queue, again sorted
 * in cylinder order.  If that queue has too many requests at a given
 * cylinder, the new request is pushed further down the list until it
 * finally finds a home.
 *
 * Pictorially: (queue position is x axis, increasing cylinder is y)
 *
 *       .      .      .      .
 *      .      .      .      .
 *     .      .      .      .
 *    .      .      .      .
 *   .      .      .      .
 *         .      .      .
 *        .      .      .
 * where each dot can represent between zero and disk_limit
 * requests at the same group of cylinders.
 *
 * The list might look like
 * 3, 3, 9, 9, 9, 9, 9, 15, 22, 9, 9
 *                             ^
 * (The '^' shows the cycle break)
 * New requests to cylinder 9 would go in the second cycle (at the end
 * in this case) to  avoid delaying the cylinder 15 request too long.
 *
 * If new requests for cylinder 3 kept arriving (and being inserted
 * just before the first 9) as fast as they were removed from the 
 * front of the list, the rest of the requests would be starved.
 * To prevent this, eventually the dynamic count mechanism will force
 * new 3's to be inserted later, such as
 * 3, 3, 9, 9, 9, 9, 9, 15, 22, 3, 9, 9
 *                             ^   
 *      
 * If enough new requests for 3 occurred before the 3's were removed 
 * the front of the list, the new ones could begin a third cycle like
 * 3, 3, 9, 9, 9, 9, 9, 15, 22, 3, 3, 3, 3, 3, 9, 9, 3
 *                             ^                    ^
 * 
 * Some time later we might find degenerate cycles with only one cylinder
 * in them such as
 * 22, 3, 3, 3, 3, 3, 9, 9, 9, 9, 9, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
 *    ^                             ^              ^              ^
 * In this case we actually have five cycles, though the later ones
 * are important only if (for example) we needed to later add another 9 as in
 * 22, 3, 3, 3, 3, 3, 9, 9, 9, 9, 9, 3, 3, 3, 3, 3, 9, 3, 3, 3, 3, 3, 3
 *    ^                             ^                 ^              ^
 *
 * Testing showed that with disk_limit==5, impolite programs were
 * still slowing down the system too much by queueing 5 accesses on each
 * of a number of cylinders.  Decreasing disk_limit enough to
 * avoid this behavior would have unreasonably penalized normal programs
 * that had, say, 3 access on one cylinder.  Therefore cyl_mask
 * was added, allowing 5 accesses to each group of 4 consecutive cylinders,
 * so that the impolite processes are effectively restrained
 * while well-behaved ones are able to proceed normally.
 * dynamic_cyl_mask is analogous to cyl_mask.  [Actually, it's not
 * clear that the dynamic mask really helps anything, it's there mostly 
 * for symmetry].
 *
 * The parameters disk_{static,dynamic}_limit and {static,dynamic}_cyl_mask
 * can be adjusted to deliver a variety of algorithms, including:
 *
 * 1) currently they are set to reach a good compromise between throughput
 * and response-time.
 * 
 * 2) If the masks are set to 1 and the limits to large numbers, s800
 * release 1.0 behavior (aka 4.3BSD behavior) is obtained (with it's 
 * dangers of system lock-up and starvation).
 *
 * 3) If the masks are set to 0xffffffff and the limits to 1, you get
 * pure FIFO (along with its poor total throughput)
 *
 * Latest tests show dynamic and static limits of 3-4 perform better than 
 * 5, hence the limits have been changed appropriately.
 */

/* disk_limit, and cyl_mask are globals so
 * that adb can be used for tuning experiments.
 */

/* How many 'nearby' cylinder accesses allowed before forcing progress */
int disk_limit = 4;

/* mask to determine if cylinders are 'nearby' for static count */
int cyl_mask = ~(0x3);
#define masked_equal(a, b) \
  (((a) & cyl_mask) == ((b) & cyl_mask))
#define masked_gt(a, b) \
  (((a) & cyl_mask) > ((b) & cyl_mask))

/*
 * These are just counters to see how much the static advancing
 * is being used.
 */
int advance_cnt = 0;

#include "../h/uio.h"
#include "../h/time.h"
#include "../h/ms_macros.h"

/* 9011
 * bp is the new block.  Its cyl is new_cyl
 * ap is the "previous" block.  Its cyl is last_cyl
 *                       next_cyl is the cyl of the block after ap
 * db is the head-of-list block (no real data).
 */
timeshare_insert(dp, ap, bp)
     register struct buf *dp, *ap, *bp;
{
    int new_cyl;		/* cylinder of block we're inserting */
    /* 'pointer' is defined to point between blocks in the list */
    int last_cyl;		/* cylinder of block before pointer */
    int next_cyl;		/* cylinder of block after pointer */
    int cycle_break;		/* boolean.  We are between cycles */
    int *last_front_p;		/* For dynamic count.  List head last time */
    int *front_cnt_p;		/* For dynamic count.  Count of head hits */
    int here_cnt;		/* Static count of 'nearby' cyl accesses */
    int first_run;
    struct buf *earliest;	/* first place bp may be inserted */

    check_queue(dp, bp);

    /*
     * We need to get the state (last_front + front_cnt) on this queue.
     * These values are now maintained in the header for each physical
     * device list. (i.e. dp->b_last_front)
     *
     * $Header: ufs_dsort.c,v 1.15.83.4 93/12/08 11:43:13 marshall Exp $
     * 9011
     * What we are now saving are:
     *   b_last_front: the cyl of the block at the head of the
     *     list the last time we were here (note that the list also
     *     changes by removal elsewhere).
     *   b_front_cnt: the number of blocks (masked) equal to that
     *     cyl at the front of the list.
     * We need to remember this info so that if we start with
     * 5, 5, 5, 5, 5, 9, 9, 5, 5
     * and then finish a couple of 5's off the front, (like this)
     *       5, 5, 5, 9, 9, 5, 5
     * we don't try to add a newly arriving 5 at the beginning!
     * 
     */
    last_front_p = (int *) &(dp->b_last_front);
    front_cnt_p = &(dp->b_front_cnt);

    /* set up */
    new_cyl = bp->b_cylin;
    last_cyl = ap->b_cylin;

    /* if current front of list has changed from last value seen */
    if (!masked_equal(last_cyl, *last_front_p)) {
	*last_front_p = last_cyl;
	*front_cnt_p = 1;
    }

    here_cnt = 1;
    first_run = 1;
    earliest = find_earliestinsert (dp, ap, bp);
    if (earliest != ap) {
	first_run = 0;
	ap = earliest;
	last_cyl = ap->b_cylin;
    }
    while (1) {
	if (ap->av_forw == NULL) {
	    insert(first_run, here_cnt, front_cnt_p, new_cyl, last_cyl, dp, ap, bp);
	    return;
	}
	
	/* if new block matches prev (current) block) */
	if (masked_equal(new_cyl, last_cyl)) {
	    /* and if current run is too long (static or dynamic) */
	    if (here_cnt +1 > disk_limit ||
		(first_run && (*front_cnt_p +1 > disk_limit))) {
		/*
		 * If a count is exceeded, step beyond the offending
		 * group of cylinders.  Last_cyl will NOT
		 * be in the offending group.  [We're after it].
		 */
		advance_cnt++;
		while ((ap->av_forw != NULL) &&
		       masked_equal(new_cyl, ap->b_cylin)) {
		    ap = ap->av_forw;
		    here_cnt++;
		}
		last_cyl = ap->b_cylin;
		if (first_run) {
		    /* fix front_cnt if last_front has changed */
		    if (here_cnt > *front_cnt_p) {
			*front_cnt_p = here_cnt;
		    }
		    first_run = 0;
		}
		if (ap->av_forw == NULL) {
		    insert(first_run, here_cnt, front_cnt_p, new_cyl, last_cyl, dp, ap, bp);
		    return;
		}
		here_cnt = 1;
	    }
	}

	next_cyl = (ap->av_forw)->b_cylin;
	cycle_break = masked_gt(last_cyl, next_cyl);

	if (here_cnt > disk_limit ||
	    ((here_cnt == disk_limit) && masked_equal(next_cyl,last_cyl))) {
	    cycle_break = 1;
	}

	if (cycle_break) {
/* 	    if ((new_cyl >= last_cyl) || (new_cyl < next_cyl)) */
 	    if (!masked_gt(last_cyl, new_cyl) ||
		masked_gt(next_cyl, new_cyl)) {
		/* e.g. 1000 goes between 20,10 */
		/*         5 goes between 20,10 */
		/*        15 doesn't go between 20,10 */
		insert(first_run, here_cnt, front_cnt_p, new_cyl, last_cyl, dp, ap, bp);
		return;
	    }
	} else {		/* not cycle_break */
/*	    if ((new_cyl < next_cyl) && (new_cyl >= last_cyl)) */
	    if (masked_gt(next_cyl, new_cyl) &&
		!masked_gt(last_cyl, new_cyl)) {
		/* e.g. 10 goes between 10,20 */
		/*      19 goes between 10,20 */
		/*       5 doesn't go between 10,20 */
		/*    1000 doesn't go between 10,20 */
		insert(first_run, here_cnt, front_cnt_p, new_cyl, last_cyl, dp, ap, bp);
		return;
	    }
	}
	if (masked_equal(next_cyl, last_cyl)) {
	    here_cnt++;
	} else {
	    if (first_run) {
		/* fix front_cnt if last_front has changed */
		if (here_cnt > *front_cnt_p) {
		    *front_cnt_p = here_cnt;
		}
		first_run = 0;
	    }
	    here_cnt = 1;
	}

	ap = ap->av_forw;
	last_cyl = ap->b_cylin;
    }
}

check_queue(dp, bp)
     struct buf *dp, *bp;
{
    /* variables for measurement system instrumentation */
    struct buf *xp;
    static int q_dah[200];	/* safe since we spl7 the whole time */
    static int ms_timeshare_id = -2;
    int q_cnt;
    static int ms_timeshare_id2 = -2;
    static int q_dah2[200];
    int save;

    if (ms_timeshare_id == -2) {
	ms_timeshare_id =
	  ms_grab_id("timeshare_insert : time, new_cyl, cur_len, cur_queue");
    }

    if (ms_timeshare_id2 == -2) {
	ms_timeshare_id2 =
	  ms_grab_id("timeshare_insert2: time, new_blk, cur_len, cur_blk_queue");
    }

    /*
     * Can't afford to have the list changing while we're printing
     * it, so remember it, then print
     */
    if (ms_turned_on(ms_timeshare_id)) {
	q_cnt = 0;
	save = spl7();
	for (xp = dp->b_actf; xp != NULL; xp = xp->av_forw) {
	    if (q_cnt < 200) {
		q_dah2[q_cnt] = xp->b_blkno;

		if (xp->b_prio == RTPRIO_MAX +1) {
		    q_dah[q_cnt] = xp->b_cylin;
		} else {
		    q_dah[q_cnt] = 0 - xp->b_cylin; /* record RT as neg */
		}
	    }

	    q_cnt++;
	}

	ms_t2a(ms_timeshare_id, bp->b_cylin, q_cnt, q_dah,
	       MIN(q_cnt, 200) * sizeof(int));

	ms_t2a(ms_timeshare_id2, bp->b_blkno, q_cnt, q_dah2,
	       MIN(q_cnt, 200) * sizeof(int));
	splx(save);
    }
}

/* try making MS instrumentation less intrusive (shorter) */
#define set_iov(i, b, l) (i.iov_base = (caddr_t) b, i.iov_len = l)
/*
 * This routine outputs a record containing:
 *   a timeval
 *   the two specified longs
 *   an array (or structure) whose length is passed in
 */
int ms_t2a(id, i1, i2, a, len)
     long int id, i1, i2, len;
     caddr_t a;
{
    struct timeval tv, ms_gettimeofday();
    struct iovec iov[4];

    tv = ms_gettimeofday();
    set_iov(iov[0], &tv, sizeof(tv));
    set_iov(iov[1], &i1, sizeof(long));
    set_iov(iov[2], &i2, sizeof(long));
    set_iov(iov[3], a, len);
    ms_data_v(id, iov, 4);
}

static int insert(first_run, here_cnt, front_cnt_p, new_cyl, last_cyl, dp, ap, bp)
     int first_run, here_cnt, new_cyl, last_cyl;
     int *front_cnt_p;
     struct buf *dp, *ap, *bp;
{
    if (first_run) { 
	/* fix front_cnt if last_front has changed */
	if (here_cnt > *front_cnt_p) {
	    *front_cnt_p = here_cnt;
	}
	/* if the first run is growing */
	if (masked_equal(new_cyl, last_cyl)) {
	    (*front_cnt_p) += 1;
	}
    }
    bp->av_forw = ap->av_forw;
    ap->av_forw = bp;

    if (bp->av_forw == NULL) {
        bp->av_back = dp->b_actl;
        dp->b_actl = bp;
    } else {
        bp->av_back = bp->av_forw->av_back;
        bp->av_forw->av_back = bp;
    }

    check_queue(dp, bp);
}

nblock_queue_check(dp)
	struct buf *dp;
{
	int nblocked = 0;

	while (dp->b_actf != NULL && (dp->b_actf->b_flags & B_NDELAY)) {
		struct buf *bp = dp->b_actf;
		dp->b_actf = bp->av_forw;
		bp->b_error = EAGAIN;
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		nblocked++;
	}
	if (dp->b_actf == NULL)
		return nblocked;
	dp = dp->b_actf;
	while (dp->av_forw != NULL) {
		if (dp->av_forw->b_flags & B_NDELAY) {
			struct buf *bp = dp->av_forw;
			dp->av_forw = bp->av_forw;
			bp->b_error = EAGAIN;
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			nblocked++;
		} else
			dp = dp->av_forw;
	}
	return nblocked;
}
#endif /* WSIO */
#endif /* hp9000s800 */
