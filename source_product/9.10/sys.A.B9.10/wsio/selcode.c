/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/selcode.c,v $
 * $Revision: 1.3.83.11 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/20 15:18:35 $
 */

/* HPUX_ID: @(#)selcode.c	55.1		88/12/23 */

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
** driver -- select code queue
** See also HPIB_TI9914.c and HPIB_SIMON.c and scsi.c
*/
#include "../h/debug.h"
#include "../h/param.h"
#ifdef	REQ_MERGING
#include "../h/vmmac.h"
#endif	
#include "../h/buf.h"
#ifdef	REQ_MERGING
#include "../h/proc.h"
#include "../h/user.h"
#endif	
#include "../wsio/timeout.h"
#include "../wsio/iobuf.h"
#include "../wsio/hpibio.h"
#include "../wsio/tryrec.h"

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI

#ifdef __hp9000s700   /* BUGBUG */
dma_dequeue()
{
	return(0);
}
#endif /* hp9000s700 */

#ifdef __hp9000s700

/**************************diag_get_selcode******************************
**
**	This routine is used by diagnostic support to get exclusive access
**      to an interface card through the isc table entry for the card.
**      If the card is currently owned, then it jams the request at the
**      beginning of the select code queue.  Otherwise if the card is not
**	owned or active it makes the request owner.
**
**	Called by:   hshpib_diag
**
**	Parameters:	bp  - pointer to the buffer of the request.
**
**	Return value:   1   - If the card is owned by the buffer bp.
**			0   - If the buffer was queued up.
************************************************************************/
diag_get_selcode(bp, proc)
register struct buf *bp;
int (*proc)();
{
	int x;
	struct isc_table_type *sc;

	bp->b_action = proc;
	sc = bp->b_sc;
	x = spl6();
	if( ( sc->active ) || ( sc->owner != NULL ) )
	{
	     bp->av_forw = sc->b_actf;
	     sc->b_actf = bp;
	     if( sc->b_actl == NULL )
	          sc->b_actl = bp;
	     splx(x);
	     return( 0 );
	}
	else
	{
	     sc->active++;
	     sc->owner = bp;
	     bp->av_forw = NULL;
	     splx(x);
	     return( 1 );

	}

}   /* diag_get_selcode */
#endif /* __hp9000s700 */

/************************************************************************
** allocate a select code (through its isc_table_type)
************************************************************************/
get_selcode(bp, proc)
register struct buf *bp;
int (*proc)();
{
	/* assume that by now it is known that the card is plugged in
	   and thus the iobuf is valid */

	register int s;
	register struct isc_table_type *sc;

	sc = bp->b_sc;
	bp->b_action = proc;
		
	bp->av_forw = NULL;

	if (bp == sc->owner)
		panic("already owned");

	s = CRIT();
	if (sc->b_actf == NULL) 
		sc->b_actf = bp;
	else
		sc->b_actl->av_forw = bp;
	sc->b_actl = bp;
	UNCRIT(s);
}

/************************************************************************
** when one activity on a selcode is done, this is called.  If
** there is nothing going on, the next activity is started 
************************************************************************/
selcode_dequeue(sc)
register struct isc_table_type *sc;
{
	register struct buf *bp;
	register int s;

	s = CRIT();
	if ((bp = sc->b_actf) == NULL || sc->active != 0) {
		UNCRIT(s);
		return 0;
	}

	sc->b_actf = bp->av_forw;
	sc->active++;
	sc->owner = bp;
	UNCRIT(s);
	(*bp->b_action)(bp);
	return 1;
}

/************************************************************************
** this indicates to selcode_dequeue that activity for a given select code
** is complete 
************************************************************************/
drop_selcode(bp)
register struct buf *bp;
{
	register int x;
	register struct buf *current;
	register struct buf *previous;

	/* free the select code */
	if (bp->b_sc->owner == bp) {
		x = CRIT();
		bp->b_sc->active = 0;
		bp->b_sc->owner = NULL;
		UNCRIT(x);
	}
	else {	/* take it off the list if present, should not happen often */
		previous = NULL;
		x = CRIT();	/* can't have list change now */
		current = bp->b_sc->b_actf;
		while (current != NULL) {	/* see if on list */
			if (current == bp)
				break;	/* found it */
			previous = current;
			current = current->av_forw;
		}
		if (current != NULL) {	/* it is on list */
			if (previous == NULL) {	/* first on list */
				bp->b_sc->b_actf = current->av_forw;
			}
			else {
				previous->av_forw = current->av_forw;
				/* end of list check */
				if (bp->b_sc->b_actl == current)
					bp->b_sc->b_actl = previous;
			}
		}
		UNCRIT(x);
	}
}

/************************************************************************
** unprotected (lean) version of drop_selcode
**	assumes: the caller has already locked out interrupts
**		 the select code is definitely owned
************************************************************************/
unprotected_drop_selcode(bp)
register struct buf *bp;
{
	if (bp->b_sc->owner != bp)
		panic("unprotected_drop_selcode");
	bp->b_sc->active = 0;
	bp->b_sc->owner = NULL;
}

#ifdef	REQ_MERGING
/* Real time I/O queue in iob */
#define r_actf b_forw
#define r_actl b_back

/* Merged I/O buf headers original count */
#define orig_bcount b_s9
#define orig_b_addr tv.tv_sec	/*  XXX  */


/* I/O merging metrics XXX ??? */
int mio_counts[17];


int merged_io_max_size = 65536;
#define OLD_BUF_SIZE (tbp->b_bcount - bp->b_bcount)


merge_io(queue, bp, tbp)
register struct iobuf *queue;
register struct buf *bp;
register struct buf *tbp;
{
    unsigned int addr;

    /* This request abuts on the right */
    VASSERT(bp->b_offset == tbp->b_offset + tbp->b_bcount);

    /* If it hasn't been done then save this request's
     * original count and add the merged requests count.
     */
    if (!tbp->orig_bcount) {
        tbp->orig_bcount = tbp->b_bcount;
    }
    tbp->b_bcount += bp->b_bcount;

    /* Make the kernel virtual address space 
     * for merged requests contiguous.
     */
    addr = rmalloc(sysmap, btorp(tbp->b_bcount));
    if (addr == 0) {
        if (tbp->b_bcount == bp->b_bcount + tbp->orig_bcount)
            tbp->orig_bcount = 0;
        tbp->b_bcount -= bp->b_bcount;
        return FALSE;
    }
    addr = ptob(addr - 1);
    pagecopy(tbp->b_un.b_addr, addr, OLD_BUF_SIZE);
    pagecopy(bp->b_un.b_addr, addr + OLD_BUF_SIZE, bp->b_bcount);

    if (!tbp->orig_b_addr)
	tbp->orig_b_addr = (int)tbp->b_un.b_addr;
    else {
        pagerm(tbp->b_un.b_addr, OLD_BUF_SIZE);
        rmfree(sysmap, btorp(OLD_BUF_SIZE),btorp(tbp->b_un.b_addr+ptob(1)));
    }

    tbp->b_un.b_addr = (caddr_t)addr;

    /* Put this request on the queue of merged requests */
    bp->av_forw = NULL;
    bp->mio_next = NULL;
    if (tbp->mio_next == NULL)
            tbp->mio_next = bp;
    else
            tbp->mio_last->mio_next = bp;
    tbp->mio_last = bp;

    /* setup resid for both requests */
    tbp->b_resid = tbp->b_bcount;
    bp->b_resid = 0;
    mio_counts[1]++;
    return TRUE;
}

#endif	


int merge_level = 1;


/******************************routine name******************************
** this utility enqueues any requested activity on an iobuf, which
**   is the per-device queue header.  If that iobuf can be immeiately
**   serviced, the action for the queued item is started.   It is expected
**   that all subsequent queueing is on the select code or DMA level,
**   and thus selcode_dequeue and dma_dequeue are then called 
************************************************************************/
enqueue(queue, bp)
register struct buf *bp;
register struct iobuf *queue;
{
#ifdef	REQ_MERGING
    register int i, x;
    register struct buf *tbp;
    int request_merged = FALSE;
#else	
    register int i, x;
#endif	

#ifdef	REQ_MERGING
    bp->av_forw = NULL;
    x = spl6();

    /* put real time processes on a separate queue */
    if (u.u_procp->p_flag & SRTPROC) {
        if (queue->r_actf == NULL) 
	    queue->r_actf = bp;
	else
	    queue->r_actl->av_forw = bp;
	queue->r_actl = bp;
    } else {
	bp->mio_next = NULL;
	bp->mio_last = NULL;
	bp->orig_bcount = 0;
	bp->orig_b_addr = 0;

	if (queue->b_actf == NULL) {
	    queue->b_actf = bp;
	    queue->b_actl = bp;
	} else if (merge_level == 0 || (bp->b_flags & B_MERGE_IO) == 0) {
	    queue->b_actl->av_forw = bp;
	    queue->b_actl = bp;
	} else {
	    /*
	     * If merging is allowed for this request (B_MERGE_IO is set)
             * and both buffers counts are a multiple of the page size and 
	     * merging these won't put us over the max size for merged
             * requests (merged_io_max_size) and the buffer abuts an
             * existing request and the abuting requests are either
             * both reads or both writes then merge them into one request.
	     */
	    mio_counts[0]++;
	    tbp = queue->b_actl;
	    if ((bp->b_offset == tbp->b_offset + tbp->b_bcount) &&
	        (bp->b_un.b_addr < (caddr_t)tt_region_addr) &&
	        (bp->b_bcount & 0xfff) == 0 && 
	        (tbp->b_bcount & 0xfff) == 0 &&
	        ((bp->b_bcount + tbp->b_bcount) <= merged_io_max_size) &&
                (bp->b_flags & B_READ) == (tbp->b_flags & B_READ)) {
		    VASSERT(((long)bp->b_un.b_addr & 0xfff) == 0);
		    VASSERT(((long)tbp->b_un.b_addr & 0xfff) == 0);
		    request_merged = merge_io(queue, bp, tbp);
	    } else for (tbp = queue->b_actf; tbp; tbp = tbp->b_actf) {
	        if ((bp->b_un.b_addr < (caddr_t)tt_region_addr) &&
	            (bp->b_bcount & 0xfff) == 0 && 
	            (tbp->b_bcount & 0xfff) == 0 &&
		    ((bp->b_bcount + tbp->b_bcount) <= merged_io_max_size) &&
                    (bp->b_offset == tbp->b_offset + tbp->b_bcount) &&
		    (bp->b_flags & B_READ) == (tbp->b_flags & B_READ)) {
		    	VASSERT(((long)bp->b_un.b_addr & 0xfff) == 0);
		    	VASSERT(((long)tbp->b_un.b_addr & 0xfff) == 0);
		    	request_merged = merge_io(queue, bp, tbp);
			break;
		} 
	    }
            /* If we didn't merge requests then just link onto the queue */
	    if (!request_merged) {
	        queue->b_actl->av_forw = bp;
	        queue->b_actl = bp;
	    }
	}   
    }
    bp->b_queue = queue;
#else	
	bp->av_forw = NULL;
        x = spl6();
        if (queue->b_actf == NULL) 
                queue->b_actf = bp;
        else
                queue->b_actl->av_forw = bp;
        queue->b_actl = bp;
	bp->b_queue = queue;
#endif	
#ifdef	FSD_KI
#ifdef	REQ_MERGING
    /* increment and save queue length in the bp */
    bp->b_queuelen	= ++queue->b_queuelen;
    /* stamp bp with enqueue time */
    KI_getprectime(&bp->b_timeval_eq);
    KI_enqueue(bp);
#else	
	/* increment and save queue length in the bp */
	bp->b_queuelen	= ++queue->b_queuelen;
	/* stamp bp with enqueue time */
	KI_getprectime(&bp->b_timeval_eq);
	KI_enqueue(bp);
#endif	
#endif	FSD_KI
#ifdef	REQ_MERGING
    splx(x);
    do {
        i = queuestart(queue);
        i += selcode_dequeue(bp->b_sc);
        i += dma_dequeue();
    } while (i);
#else	
        splx(x);
	do {
		i = queuestart(queue);
		i += selcode_dequeue(bp->b_sc);
		i += dma_dequeue();
	} while (i);
#endif	
}




queuestart(queue)
register struct iobuf *queue;
{
        register struct buf *bp;
	int x;
        
	x = spl6();
#ifdef	REQ_MERGING
        if (queue->b_active != 0 || 
            ((bp = queue->r_actf) == NULL && (bp = queue->b_actf) == NULL)) {
#else	
        if (queue->b_active != 0 || (bp = queue->b_actf) == NULL) {
#endif	
		splx(x);
                return 0;
	}
        queue->b_active++;

	/* its necessary to go down the link one step here because
	   av_* fields will be used later on */
#ifdef	REQ_MERGING
	if (queue->r_actf)
		queue->r_actf = bp->av_forw;
	else
		queue->b_actf = bp->av_forw;

#else	
	queue->b_actf = bp->av_forw;
#endif	
	queue->b_state = 0;
#ifdef	FSD_KI
	/* stamp bp with queuestart time */
	KI_getprectime(&bp->b_timeval_qs);

	KI_queuestart(bp);
#endif	FSD_KI
	splx(x);
        (*bp->b_action)(bp);
	/* to prevent runaway recursion, the actions must always
	   return immediately (usually by doing a get_selcode) */
	return 1;
}

/************************************************************************
** indicates that the current owner is done with the queue 
************************************************************************/
queuedone(bp)
register struct buf *bp;
{
#ifdef	REQ_MERGING
    register struct iobuf *queue;
    register int x;
    register struct buf *next;
    register int error;
#else	
	register struct iobuf *queue;
	register int x;
#endif	

#ifdef	REQ_MERGING
    x = CRIT();
    if (bp->b_error != 0) bp->b_flags |= B_ERROR;
    queue = bp->b_queue;

    /* Handle merged I/O chains */
    if (bp->orig_bcount) {
	/* This was a merged I/O. */
	register struct buf *tbp, *lbp;
        register long orig_addr = bp->orig_b_addr;
	register int qcount = 1;

	/* Free up resources */
	pagerm(bp->b_un.b_addr, bp->b_bcount);
	rmfree(sysmap, btorp(bp->b_bcount), btorp(bp->b_un.b_addr + ptob(1)));

	/* Restore the head buffer and call iodone() on it. */
	bp->b_bcount = bp->orig_bcount;
	bp->b_un.b_addr = (caddr_t)bp->orig_b_addr;
	next = bp->mio_next; 
	error = bp->b_error;
	bp->mio_next = NULL;
	bp->orig_bcount = 0;
	bp->orig_b_addr = 0;
        iodone(bp);

	/* call iodone() on everyone else in the chain. */
	for (tbp = next; tbp; ) {
	    qcount++;
	    if (error != 0) 
	    	tbp->b_flags |= B_ERROR;
	    iodone(tbp);
	    lbp = tbp;
	    tbp = tbp->mio_next;
	    lbp->mio_next = NULL;
	}
	mio_counts[qcount]++;
    } else 		/* This was a normal (not merged) I/O. */
        iodone(bp);

    queue->b_active = 0;
#else	
	x = CRIT();
	if (bp->b_error != 0) bp->b_flags |= B_ERROR;
	queue = bp->b_queue;
	iodone(bp);
	queue->b_active = 0;
#endif	
#ifdef	FSD_KI
#ifdef	REQ_MERGING
    /* decrement queue length */
    queue->b_queuelen--;
    KI_queuedone(bp);
#else	
	/* decrement queue length */
	queue->b_queuelen--;
	KI_queuedone(bp);
#endif	
#endif	FSD_KI
#ifdef	REQ_MERGING
    queuestart(queue);
    UNCRIT(x);
#else	
	queuestart(queue);
	UNCRIT(x);
#endif	
}

/* 
	Timeout mechanisms.  There is a moderately tricky problem to solve
	with timeouts on HPIB devices (etc.).  The problem lies with passing
	on the timeout information to the timed out routine without introducing
	a great deal of overhead.  There are several distinct cases:

	1) While the processor is busy doing the work for the activity,
	   such as while it is handshaking bytes of a control message out.
	2) While the processor is on the way to condition 1 above.
	3) While the processor is returning from condition 1 above.
	4) While the processor is released and doing something irrelevent to
	   this task (waiting for an interrupt).
	5) A special case of 4, when the activity could time out once the
	   interrupt occurred.  (interrupt initiated burst mode transfers)
	
	To add to the complexity, there may be other activities on the
	interrupt stack between the timeout interrupt and the timed out
	activity.  (Such as pci interrupt service.)

	To address the handshake operations, the following procedure is used:
	1) A timeout is set up to call a routine which looks at a location
	   in the iob.  It is set up to contain a mark stack (pointer into
	   the stack) whenever a routine that is looping on a hardware
	   condition is active.  (This means any routine polling for a
	   hardware condition which is not 100% (modulo processor failure)
	   guaranteed to occur.)
	   When not in such a routine, the flag is zero.
	   The value of the pointer is to be the location just following
	   the stack frame for the routine where an interrupt will store
	   the return address.
	2) When (if) the timeout routine is called, it unconditionally sets
	   a flag indicating that a timeout has occurred.  In addition,
	   if the pointer is non-zero, it is used to change the return
	   address from the isr which (must have) interrupted the poll
	   routine to point to flag_timeout(), which is really escape()
	   (after setting up a parameter).
	3) The entry to each polling routine 
	   a) sets up the mark stack.
	   b) checks the flag.  If it is set, it clears the mark stack
	      and escapes immediately.
	   (order is important).
	4) Just before exiting, the mark stack is cleared.

	Assume that a timeout occurs at each time noted below:
	a) before entry (or before the store operation into the iob).
	   The mark stack will be filled in, and then the flag is discovered
	   to be set, and an exit occurs.
	b) after the store.  The flag is set, and a forced escape occurs.
	c) after the clear.  Either the timeout occurred just before the
	   operation completed normally, in which case it will complete
	   normally without interference from the timout, or it is an
	   intermediate step, in which case the next occurrence of step a
	   will cause an exit.
	
	This mechanism covers conditions 1,2, and 3 above.

	Case 4 needs to be addressed in the more classical fashion of
	a timeout advancing the state of the machine, or otherwise 
	changing the state of the machine.  This is easy, because there
	is no chance of a machine state being recorded on the stack.

	Case 5 is either case 4 (before the interrupt, which can be
	prevented or sidetracked), or cases 1,2 or 3 (after the interrpt).

	The markstack operation must be done in line so that once the  
	store occurs, if the timeout occurs on the next cycle, the stack
	will be correct.  This is best done using a macro.

	The macros START_POLL and END_POLL do this activity.

*/

/*
**  FURTHER TIMEOUT ANALYSIS/DESCRIPTION  (jpc)
**
**    Where to find things:
**
**	iobuf.h:
**	  - START_FSM, END_FSM
**	  - START_TIME, END_TIME
**	  - TIMEOUT_BODY
**
**	hpibio.h:
**	  - START_POLL/END_POLL
**
**    FSM model for driver xxx:
**	(a simple real example is HPIB_utility found in hpib.c)
**	
**	void xxx_timeout(bp)
**	register struct buf *bp;
**	{
**		TIMEOUT_BODY(iob->intloc,xxx_transfer,bp->b_sc->int_lvl,0,
**								timedout);
**	}
**
**
**	xxx_transfer(bp)
**	register struct buf *bp;
**	{
**		register struct iobuf *iob = bp->b_queue;
**	
**		try
**			START_FSM;
**			switch((enum states)iob->b_state) {
**			
**			case y:
**				...
**				START_TIME(xxx_timeout, n * HZ);
**				<low-level driver op w/ START_POLL/END_POLL>
**				END_TIME;
**				...
**			case x:
**				...
**				START_TIME(xxx_timeout, n * HZ);
**				iob->b_state = (int)z;
**				<overlapped low-level driver op>
**				break;
**			case z:
**				END_TIME;
**				...
**			case timedout:
**				escape(TIMED_OUT);
**			default:
**				panic("Unrecognized xxx_transfer state");
**			}
**			END_FSM;
**		recover {
**			if (escapecode == TIMED_OUT) {
**				...
**			}
**			...
**		}
**	}
**
**  Regions where timeouts can occur and actions taken:
**
**	1) processor not between START_FSM/END_FSM;  typically outside the
**	   fsm, but may be just entering the fsm or just leaving the fsm
**	2) between START_FSM/END_FSM, but not between START_POLL/END_POLL;
**	   on the way to a polling routine, or on the way out of the fsm,
**	   but never in a loop that could "hang"
**	3) between START_FSM/END_FSM, and between START_POLL/END_POLL;
**	   in loop waiting for a hardware condition; can potentially "hang"
**
**		region (1)	    region (2)		region (3)
**
**  state 	!iob->in_fsm	    iob->in_fsm &&	iob->in_fsm &&	
**  definition			    !iob->markstack	iob->markstack
**
**  timeout	set iob->state	    set iob->timeflag	set ISR return
**  routine	sw trigger fsm				addr to function
**  action						which escapes
**
**  Notes:
**	- the START_FSM/END_FSM & START_POLL/END_POLL constructs define
**	  the regions, and must handle the transitions between them
**	- "handling the transition" involves:
**	  1) setting the state variables correctly without introducing
**	     a critical section
**	  2) possibly detecting & handling a timeout that occurred in the
**	     previous region
**	  3) guaranteeing that exactly one timeout is sensed & handled for
**	     each actual timeout occurring
**	- in KLEENIX, the markstack & timeflag variables were initialized
**	  somewhat haphazardly in the fsm's
**	- in region 2 on KLEENIX, it was possible for the timeout to be
**	  sensed by the fsm AFTER the fsm thought it had completed an
**	  operation successfully and had performed a queuedone; in
**	  this case, the fsm set an error flag and performed ANOTHER
**	  queuedone, leading to unknown consequences
**	- in region 2, it is possible, though unlikely, for a timeout to
**	  occur after the END_POLL.  If nothing "significant" happens before
**	  reaching either a START_POLL or an END_FSM, the timeout will
**	  be detected and reported normally; however, if a clear_timeout()
**	  and then a new timeout() is performed, the old timeout will be
**	  reported at the next START_POLL or END_FSM, but the new timeout
**	  will still be in effect!!!  This was a bug in KLEENIX, whose fix
**	  is to check the timeflag every time after doing a clear_timeout()
**	- to fix the above problems and to gain greater uniformity and
**	  control, two new macro's, START_TIME and END_TIME, have been 
**	  defined in MANX.
**	
**  Individual Macro Analysis:
**
**    START_FSM:	(region 1 -> 2)
**	- initialize markstack to NULL (undefined in region 1)
**	- initialize timeflag to FALSE (undefined in region 1)
**	- set iob->in_fsm; (enter region 2)
**	- timeouts occurring while in region 1 result in the state
**	  variable being set to timedout and the fsm being software triggered;
**	- KLEENIX has a bug with this if it so happens that the fsm is being
**	  entered as a result of a hardware interrupt when the timeout occurs;
**	  the state variable is already set to timedout, so timeout processing
**	  occurs; however, when the hardware interrupt exits, the fsm is
**	  AGAIN entered as a result of the software trigger, this time
**	  either entering the next state prematurely (for instance, thinking
**	  it has the select code when it really doesn't), or entering the
**	  default state (resulting in a panic).
**	- in MANX we handle this situation by testing to see if the fsm
**	  has been triggered, but not called as a result of the trigger;
**	  if so, we exit immediately, knowing that the fsm will be re-entered
**	- timeouts occurring while in region 2 result in timeflag being set,
**	  which will be detected elsewhere.
**    END_FSM:		(region 2 -> 1)
**	- clear iob->in_fsm; (enter region 1)
**	- timeouts occurring while in region 2 result in timeflag being
**	  set, so it must be tested for, and handled by escaping, so the
**	  timeout condition won't be forgotten
**	- timeouts occurring while in region 1 result in  the state
**	  variable being set to timedout, and the fsm being software
**	  triggered, so the fsm will be re-entered to handle the timeout
**
**    START_POLL:	(region 2 -> 3)
**	- set markstack; (enter region 3)
**	- timeouts occurring while in region 2 result in timeflag being
**	  set, so it must be tested for, and handled by escaping, since
**	  the timeout has already been triggered
**	- timeouts occurring while in region 3 result in an escape
**    END_POLL:		(region 3 -> 2)
**	- clear markstack; (enter region 2)
**	- timeouts occurring while in region 3 will have already escaped
**	- timeouts occurring while in region 2 result in timeflag being set,
**	  which will be detected elsewhere.
**
**    START_TIME:
**	- call timeout() with appropriate parameters
**    END_TIME:
**	- call clear_timeout with appropriate parameter
**	- if timeflag set, escape
*/

/************************************************************************
** this is called once the pc is changed 
************************************************************************/
flag_timeout()
{
	escape(TIMED_OUT);
}

