 /* dux/RCS/netbuf.c,v $
 * $Revision: 1.7.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/04/21 14:06:24 $
 */

/* HPUX_ID: @(#)netbuf.c	55.1		88/12/23 */

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

/*
 * This file contains code dealing with buffer allocation under interrupt.
 * It operates by maintaining a separate reserve of buffers which can be
 * allocated.  Can't neccessarily mix these buffer headers with those in 
 * the buffer cache.  The virtual address space's used for the buffers are
 * the same, but care must be taken when exchanging memory with buffer
 * cache buffers.  bufpages does not reflect the memory consumed by these
 * buffers.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../dux/duxparam.h"
#include "../dux/selftest.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/nsp.h"
#include "../h/dux_mbuf.h"
#include "../h/vnode.h"
#include "../h/bcache.h"

extern int local_mounts;		/* Number of local mounts */

extern struct dux_mbstat dux_mbstat; 	/* Statistics */

extern int dskless_fsbufs;		/* Maximum number of fs_bufs we will */
					/* allocate */

#define MAX_DUX_REQ_PERCENT	70	/*Percentage of buffer cache that can
					 *be used to make remote requests */

int max_dux_req_percent = MAX_DUX_REQ_PERCENT;

int want_dux_req_buf = 0;	/* Variable we sleep on when there aren't
				 * any buffers to do a remote request.  */

int cur_dux_req_bufs,		/* number of bufs allocated to dux requests */
    cur_dux_req_bufmem;		/* amount of memory allocated to dux requests */

struct bufqhead net_bchain;	/*head of list of free buffers*/

#define DUX_MAX_ALLOCATING_BUFS	2 /* Max # of gcsp's trying to alloc fs_bufs */

struct mbuf dux_get_fsbuf_mbuf[DUX_MAX_ALLOCATING_BUFS]; /* static mbufs used
							  * for allocating
							  * fs_bufs not under
							  * interrupt */

int dux_num_on_queue = 0; 	/* Number of ongoing requests to allocate
				 *  fs_bufs by gcsp's */


				/* Macro to round up to a page size, 
				 * quicker that roundup */
#define round_up_to_pagesize(size)	ptob(btop((size) + NBPG-1))

#if defined(__hp9000s700) || defined(__hp9000s800)
#define CRIT()                  spl6()
#define UNCRIT(x)               splx(x)
#endif /* s700 || s800 */


/*
 * brelse_to_net()
 *
 * return a network buffer to the network free list.
 * assumptions:
 *	No one is waiting on the buffer.
 *	The buffer does not contain disc data.
 *	The hash information, device # etc remain empty.
 * This function is called by brelse whenever a buffer with the B_NETBUF flag
 * is set.
 * return 1 if brelse should return immediately, 0 otherwise
 */

brelse_to_net(bp)
register struct buf *bp;
{
    int s;
    struct buf *dp;
    s=splimp();

    if (bp->b_flags & B_DUX_REM_REQ) {
	cur_dux_req_bufs--;
        cur_dux_req_bufmem -= bp->b_bufsize;
	VASSERT(cur_dux_req_bufmem >= 0);
	bp->b_flags &= ~(B_NETBUF|B_DUX_REM_REQ);
	if (want_dux_req_buf) {
		want_dux_req_buf = 0;
		wakeup((caddr_t)&want_dux_req_buf);
	}
	splx(s);
	return(0);
    }
    else {
		/* We should never get empty buffers */
		VASSERT(bp->b_bufsize > 0); 

		/* Keep statistics */
		dux_mbstat.m_nbfree++;
		dux_mbstat.m_freepages += btop(bp->b_bufsize);

		/* Put the buffer on the front of the chain.   Place it on
		 * the end of the chain if it is empty.
		*/
		dp = (struct buf *)&net_bchain;
		if (bp->b_bufsize <= 0) dp = dp->av_back;
		bp->av_back = dp;
		bp->av_forw = dp->av_forw;
		(dp->av_forw)->av_back = bp;
		dp->av_forw = bp;
	}
    splx(s);
    return(1);
}

/* Return 0 on success, 1 on failure.
*/
static 
dux_adjust_virtsize (bp, size)
struct buf * bp;
int size;					/* in units of bytes */
{
    size = btop(size);

    /* only make it bigger */
    if (bp->b_virtsize < size) {
        caddr_t buf_mem;
        caddr_t bufget();

	buf_mem = (caddr_t)bfget(size);
	if (buf_mem == NULL) return 1 ;
    	pagemove(bp->b_un.b_addr, buf_mem, bp->b_bufsize);

        /* Remove the old virtual address space. */
        bffree(bp->b_un.b_addr, bp->b_virtsize);
        bp->b_un.b_addr = buf_mem;
	bp->b_virtsize = size;
    }
    return 0;
}

/*
 * Get a buffer under interrupt of the appropriate size.  If we can't get
 * one, return NULL.
 */
struct buf *
buf_syncgeteblk(size)
int size;		/* already rounded up to an integral number of pages */
{
	register struct buf *bp, *dp;
	register struct bufqhead *bqp;
	int total_size;
	int s, take;

	s=splimp();

	/* Is the list empty.  Rest of routine is guaranteed non-empty list. */
	bqp = &net_bchain;
	dp = bqp->av_forw;
	if (dp == (struct buf *)bqp) {
		splx(s);
		return((struct buf *) NULL);
	}

	/* Find the first one that will work */
	total_size = 0;
	for (bp = dp; bp != (struct buf *)bqp; bp = bp->av_forw) {
	    total_size += bp->b_bufsize;
	    if (bp->b_bufsize >= size) {
		break;
	    }
	}

	/* Make sure we found one */
	if (bp == (struct buf *)bqp) {
		if (total_size < size) {
			splx(s);
			return(NULL);
		}
		bp = dp;
	} 

	/* make sure we have enough virtual address space */
        dux_adjust_virtsize (bp, size);
	if (bp->b_virtsize < btop(size)) {
		splx(s);
		return((struct buf *) NULL);
	}

	/* unlink the buffer */
	(bp->av_back)->av_forw = bp->av_forw;
	(bp->av_forw)->av_back = bp->av_back;

	/* we know we have enough free pages */
	dp=bqp->av_back;
	while (bp->b_bufsize < size) {
	  take = size - bp->b_bufsize; 	
          if (take >= dp->b_bufsize)
	          take = dp->b_bufsize;
          pagemove(&dp->b_un.b_addr[dp->b_bufsize - take],
                   &bp->b_un.b_addr[dp->b_bufsize], take);
          bp->b_bufsize += take;
          dp->b_bufsize -= take;
	  dp=dp->av_back;
	}

	splx(s);

	/*At this point we have a buffer that is acceptable.  Set the
	 *flags, and return the buffer.
	 */
	bp->b_flags = B_BUSY|B_INVAL|B_NETBUF;
	return (bp);
}

/*
 * Allocate a buf not under interrupt so we can afford to sleep, use a gcsp.
 */
static
int wait_for_dux_buf(reqp)
    dm_message reqp;
{
    struct buf *bp;
    struct buf *getnewduxbuf();
    int s;

    bp = getnewduxbuf(reqp->m_quad[0]);
    if (bp != NULL) {

        /* Just release it so someone else can use it */
    	s = splimp();
    	dux_mbstat.m_nbfree--;
    	dux_mbstat.m_freepages -= btop(bp->b_bufsize);
    	splx(s);
        brelse(bp);
    }
    s = CRIT();
    dux_num_on_queue--;
    /* Zero out m_quad[0] field so we know we can use this mbuf again */
    reqp->m_quad[0] = 0;
    UNCRIT(s);
}

void
dux_ics_geteblk (size)
{
	int s;
   
        /* If we can't find a buffer and, we haven't exceeded dskless_fsbufs
	 * get a gcsp to allocate us another dux fsbuf since it can sleep
         */
        s = splimp();
        if ((dux_num_on_queue < DUX_MAX_ALLOCATING_BUFS) &&
            (dux_mbstat.m_netbufs < dskless_fsbufs)) {
	    int i;

	    for (i = 0; i < DUX_MAX_ALLOCATING_BUFS; i++) {
		if (dux_get_fsbuf_mbuf[i].m_quad[0] == 0) {
		    break;
		}
	    }

	    /* always be careful */
	    VASSERT(i < DUX_MAX_ALLOCATING_BUFS);

            dux_get_fsbuf_mbuf[i].m_quad[0] = size;
            if (invoke_nsp(&dux_get_fsbuf_mbuf[i], wait_for_dux_buf,
                           LIMITED_NOT_OK) == 0) {
                dux_num_on_queue++;
            }
	    else {
		/* Make sure mbuf can be reused */
            	dux_get_fsbuf_mbuf[i].m_quad[0] = 0;
	    }
        }
        splx(s);
}




struct buf *
syncgeteblk(size)
int size;
{
    struct buf *bp;
    struct buf *buf_syncgeteblk();
    struct buf *getnewduxbuf();
    register int sizealloc;
    int s;

    /* round up the requested size to a multiple of the page size */
    sizealloc = round_up_to_pagesize(size);

    /* find existing buffer header first (first fit) */
    bp = buf_syncgeteblk (sizealloc);
    if (bp == NULL) {
	/* None exist, try and get a new one */
    	bp = getnewduxbuf(sizealloc);
        if (bp == NULL) {
	    dux_ics_geteblk (sizealloc);
	    return (bp);
	}
    }
    
    bp->b_bcount = size;

    /* Keep statistics */
    s=splimp();
    dux_mbstat.m_nbfree--;
    dux_mbstat.m_nbtotal++;
    dux_mbstat.m_freepages  -= btop(bp->b_bufsize);
    dux_mbstat.m_totalpages += btop(bp->b_bufsize);

    /*record that we have allocated a network buffer for selftest purposes*/
    TEST_PASSED(ST_NETBUF);
    splx(s);

    return bp;
}




/* Swap the pointers for two buffer headers.  Will return 1 on success,
 * zero on failure.
*/
dux_swap_buf (duxbp, bp)
struct buf *duxbp;		/* network buffer */
struct buf *bp;			/* file system buffer */

{
	register caddr_t addr_tmp;
	long size_tmp;

	VASSERT (!(bp->b_flags & B_NETBUF));
	VASSERT (duxbp->b_flags & B_NETBUF);
	VASSERT (duxbp->b_bufsize > 0);

	/* Maintain the relative sizes of the two buffers.  That way memory
	 * won't leak between the two separate pools of buffers.  We don't
	 * have to swap the b_bufsize fields.
	*/
	size_tmp = duxbp->b_bufsize - bp->b_bufsize;

	/* Make sure you have enough virtual address space to cover the data */
	if (size_tmp < 0) { /* dux buffer is smaller */
		if (dux_adjust_virtsize (duxbp, bp->b_bufsize))
			return 0;
                pagemove(&bp->b_un.b_addr[duxbp->b_bufsize], 
                         &duxbp->b_un.b_addr[duxbp->b_bufsize], 
			 -size_tmp);

	} else {
		if (size_tmp > 0) { /* bp is smaller */
			if (dux_adjust_virtsize (bp, duxbp->b_bufsize))
				return 0;
                	pagemove(&duxbp->b_un.b_addr[bp->b_bufsize], 
                         	&bp->b_un.b_addr[bp->b_bufsize], 
			 	size_tmp);
		}
	}

	/*switch the b_addr field*/
	addr_tmp = bp->b_un.b_addr;
	bp->b_un.b_addr = duxbp->b_un.b_addr;
	duxbp->b_un.b_addr = addr_tmp;

	size_tmp = bp->b_virtsize;
	bp->b_virtsize = duxbp->b_virtsize;
	duxbp->b_virtsize = size_tmp;

	return 1;
}

/*
 * MALLOC a new buf.
 */
static
struct buf *getnewduxbuf(size)
    int size;
{
    struct buf *bp;
    int s;

    /* Only allocate dskless_fsbufs buffers */
    if (dux_mbstat.m_netbufs >= dskless_fsbufs) {
        return(NULL);
    }

    if (ON_ISTACK) return (NULL);

    bp = geteblk (size);
    if (bp == NULL) {
        return NULL;
    }
    bp->b_flags = B_BUSY|B_INVAL|B_NETBUF;
    bp->b_bcount = size;

    dux_adjust_virtsize (bp, MAXBSIZE);

    dux_adjust_bufpages(-btop(bp->b_bufsize));

    /* Keep statistics */
    s = splimp();
    dux_mbstat.m_netbufs++;
    dux_mbstat.m_netpages += btop(bp->b_bufsize);
    dux_mbstat.m_nbfree++;
    dux_mbstat.m_freepages += btop(bp->b_bufsize);
    splx(s);

    return bp;
}

initialize_netbufs()
{
	static int didit=0;
	struct buf *bp;
	
	if (!didit) {
		int i;

		
		/* initialize list of free buffers */
                bp = (struct buf *)&net_bchain;
                net_bchain.av_forw = bp;
                net_bchain.av_back = bp;

		/* Zero remote dux buffer stats */
		cur_dux_req_bufs = cur_dux_req_bufmem = 0;

		/* Initialize static mbuf's used to get more dskless fsbufs */
		for (i = 0; i < DUX_MAX_ALLOCATING_BUFS	; i++) {
		    if (init_static_mbuf(DM_EMPTY,
		    			&dux_get_fsbuf_mbuf[i]) == -1) {
			panic("Could not initialize dux_get_fsbuf_mbuf");
		    }
		    DM_HEADER(&dux_get_fsbuf_mbuf[i])->dm_ph.p_flags =
		    						P_REQUEST;
		    /* Initialize so we know which ones are free */
            	    dux_get_fsbuf_mbuf[i].m_quad[0] = 0;
		}

		/* now get some minimum number of buffers to get things 
		 * started.
		*/
		i = dskless_fsbufs;
		if (i > 4) i = 4;
		while (i-- > 0) {
	          if ((bp = getnewduxbuf(NBPG)) != NULL){
			int s;
    			s = splimp();
    			dux_mbstat.m_nbfree--;
    			dux_mbstat.m_freepages -= btop(bp->b_bufsize);
    			splx(s);
			brelse (bp);
		  }
		}
		didit=1;
	}
}


/*
 * dux_buf_check(vp) is used to check for remote DUX request
 * deadlocks.  There are three return values from dux_buf_check():
 *   0: If the vp is not VDUX.
 *  -1: If the vp is VDUX and we have exceeded some limit as to
 *      the number of buffers that we can use for remote requests,
 *      we sleep, waiting for one to be freed and then return -1.
 *   1: If the vp is VDUX and we haven't exceeded any limit.
 * In the 0 case, we just continue on.  In the -1 case we loop.
 * In the 1 case we must mark the bp returned by getnewbuf with two
 * flags bits, B_NETBUF and B_DUX_REM_REQ.
 */
int
dux_buf_check(vp)
struct vnode *vp;
{
    int s;

    if (vp->v_fstype != VDUX)
	return(0);

    s = splimp();

        /* Calculate the maximum number of buffers and buffer pages
         * we can use fo DUX requests.
         */
    if ((local_mounts) &&
        dbc_dux_limits_exceeded (cur_dux_req_bufs,
                                 cur_dux_req_bufmem,
                                 max_dux_req_percent))
    {
        want_dux_req_buf++;
        sleep((caddr_t)&want_dux_req_buf, PRIBIO+1);
        splx(s);
        return(-1);
    }

    cur_dux_req_bufs++;

    splx(s);

    return(1);
}


dux_buf_fail()
{
    int s = splimp();

    cur_dux_req_bufs--;

    splx(s);
}


dux_buf_size(size)
long size;
{
    int s = splimp();

    cur_dux_req_bufmem += size;

    splx(s);
}
