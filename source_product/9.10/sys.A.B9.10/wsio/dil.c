/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/dil.c,v $
 * $Revision: 1.3.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 14:33:32 $
 */

/* HPUX_ID: @(#)dil.c	55.1		88/12/23 */

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
 *	general purpose routines for power up and buffer handling
 */
#include "../h/debug.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../h/user.h"
#include "../wsio/intrpt.h"
#include "../wsio/iobuf.h"
#include "../wsio/tryrec.h"
#include "../h/systm.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#include "../wsio/dil.h"
#include "../wsio/hpib.h"
#include "../wsio/dilio.h"

#define DILBUFSIZE	sizeof(struct dilbuf)
int	dil_getsc();
int	wait_timeout();

extern int ndilbuffers;
extern struct buf dil_bufs[];	/* buffers for dil io */
extern struct iobuf dil_iobufs[];	/* iobuffers buffers for dil io */

/* info packets for dil io */
extern struct dil_info dil_info[]; 


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_get_selcode(bp, proc)	/* this routine must be proctected by spl6() */
	struct buf *bp;
	int (*proc)();
{
	register struct dil_info *info = (struct dil_info *)bp->dil_packet;
	struct isc_table_type *sc;
	register int x;

	sc = bp->b_sc;
	x = spl6();
	if ((sc->state & LOCKED) &&
            sc->locking_pid == info->last_pid) {
		sc->owner = bp;
		splx(x);
		return 1;	/* already got the channel */
	}
	if (sc->active) {	/* must wait for the bus */
		/* get on selcode bus */
		bp->b_action2 = proc;
		get_selcode(bp, dil_getsc);
		splx(x);
		return 0;	/* have to wait */
	}
	else { /* get the channel */
		sc->active++;
		sc->owner = bp;
		splx(x);
		return 1;	/* got it, keep doing work */
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_getsc(bp)
struct buf *bp;
{
	(*bp->b_action2)(bp);
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_drop_selcode(bp)
struct buf *bp;
{
	struct isc_table_type *sc;
	register struct dil_info *info = (struct dil_info *)bp->dil_packet;
	int x;

	sc = bp->b_sc;
	x = spl6();
	if ((sc->state & LOCKED) && 
            (sc->locking_pid == info->last_pid)) {
			splx(x);
			return;
	}
	drop_selcode(bp);
	splx(x);
}

/******************************routine name******************************
** [short description of routine]
** allocate a dil buffer 
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
struct buf * get_dilbuf()
{
	register struct buf *bp;
	register struct iobuf *iob;
	int x;

	bp = dil_bufs;	/* set to top of list */
	iob = dil_iobufs;	/* set to top of list */
	x = spl6();
	while (iob->b_flags & B_BUSY)
		if ((int)iob >= ((int)dil_iobufs+(sizeof(struct iobuf)*
						 (ndilbuffers - 1)))) {
			splx(x);
			return NULL;	/* no more buffers */
		} else {
			bp++;	/* find one that is not empty */
			iob++;	/* find one that is not empty */
		}

	iob->b_flags = B_BUSY;	/* take this one */
	splx(x);
	
	/* init the iobuffer to the defaults */
	return bp;
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
release_dilbuf(bp)
register struct buf *bp;
{
	int x;

	x = spl6();

	VASSERT(bp == (struct buf *) u.u_fp->f_buf);
	VASSERT(u.u_fp->f_count == 1);

	/* make it available */
	bp->b_queue->b_flags &= ~B_BUSY;

	splx(x);
}


enum dil_ioctl_state     {dil_start = 0, dil_end, dil_timedout, dil_defaul};

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_dequeue(bp)
struct buf *bp;
{
	register int i;
	register struct isc_table_type *sc = bp->b_sc;

	do {
		i = selcode_dequeue(sc);
#ifdef __hp9000s300
		i += dma_dequeue();
#endif /* __hp9000s300 */
	} while (i);
}

int dil_io_lock();

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_lock_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, dil_io_lock, bp->b_sc->int_lvl, 0, dil_timedout);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_io_lock(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register struct isc_table_type *sc = bp->b_sc;
	int x;

	try
		START_FSM;
		switch ((enum dil_ioctl_state) iob->b_state) {
		case dil_start:
			iob->b_state = (int) dil_end;
			x = spl6();
			if (sc->owner && (info->open_flags & FNDELAY)) {
				iob->b_state = (int) dil_defaul;
				splx(x);
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(dil_lock_timeout);
			sc->locks_pending++;
			splx(x);
			if (!dil_get_selcode(bp, dil_io_lock))
				break;
		case dil_end:
			END_TIME;
			iob->b_state = (int) dil_defaul;
			sc->locks_pending--;
			sc->lock_count = 1;
			info->locking_pid = info->last_pid;
			sc->locking_pid = info->locking_pid;
			bp->return_data = 1;
			iob->dil_state |= D_CHAN_LOCKED;
			sc->state |= LOCKED;
			queuedone(bp);
			break;
		case dil_timedout:
			escape(TIMED_OUT);
		default:
			panic("unrecognized dil_io_lock state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) dil_defaul;
		sc->locks_pending--;
		bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		drop_selcode(bp);
		wakeup((caddr_t) &bp->b_queue);
		queuedone(bp);
		dil_dequeue(bp);
	}
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
shared_buf_timeout(bp)
register struct buf *bp;
{
	bp->b_error = EIO;
	wakeup((caddr_t) &bp->b_queue);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
shared_buf_check(bp)
register struct buf *bp;
{
	int x;
	register struct isc_table_type *sc = (struct isc_table_type *) bp->b_sc;
	register struct iobuf *iob = (struct iobuf *) bp->b_queue;
	struct dil_info *info = (struct dil_info *)bp->dil_packet;
	struct timeout timeo;

	x = spl6();
	bp->b_error = 0;
	if (((sc->state & LOCKED) || sc->locks_pending) &&
            (sc->locking_pid != u.u_procp->p_pid) &&
            (sc->owner == bp)) {
		if (info->open_flags & FNDELAY) {
			splx(x);
			bp->b_error = EACCES;
			bp->b_flags |= B_ERROR;
			splx(x);
			return(-1);
		} else {
			if (iob->activity_timeout > 0)
				timeout(shared_buf_timeout, bp, iob->activity_timeout, &timeo);
		}
	}
	if (setjmp(&u.u_qsave)) {
		/* 
		** Request has recieved a signal
		*/
		u.u_eosys = EOSYS_NORESTART;
		bp->b_error = EINTR;
	} else
		while (((sc->state & LOCKED) || sc->locks_pending) &&
		    (sc->locking_pid != u.u_procp->p_pid) &&
		    (sc->owner == bp))
			sleep((caddr_t) &bp->b_queue, DILPRI);
		clear_timeout(&timeo);
	splx(x);
	if (bp->b_error)
		return(-1);
	return(0);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
unlock_close_check(bp)
register struct buf *bp;
{
	struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register struct buf *dbp = (struct buf *) &dil_bufs[0];
	register struct dil_info *dbp_info;
	register struct isc_table_type *sc = (struct isc_table_type *)bp->b_sc;
	register int i;
	register int do_unlock = 1;
	register int x;

	if ((sc->state & LOCKED) == 0)
		return;

	/* if this process doesnt have the sc locked then return */
	if (sc->locking_pid != u.u_procp->p_pid)
		return;

	/*
	** We still have the channel locked.  If we dont have any
	** more open files on this select code then unlock it
	*/
	x = spl6();
	for (i = 0; i < ndilbuffers; i++) {
		dbp_info = (struct dil_info *)dbp->dil_packet;
		if ((dbp != bp) && 
		    (dbp->b_queue->b_flags & B_BUSY) &&
		    (dbp_info->last_pid == sc->locking_pid)
		    && (dbp->b_sc == sc))
			do_unlock = 0;
		dbp++;
	}

	if (do_unlock) {
		sc->lock_count = 0;
		sc->locking_pid = 0;
		sc->state &= ~LOCKED;
		info->locking_pid = 0;
		bp->b_queue->dil_state &= ~D_CHAN_LOCKED;
		drop_selcode(sc->owner);
		wakeup((caddr_t) &bp->b_queue);
		dil_dequeue(bp);
	}
	splx(x);
	return;
}

extern int (*hpib_io_proc)();
extern int (*do_dil_exit)();
int hpib_io();

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**	called once during power up [specifically??]
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_init()
{
	register int i;

	hpib_io_proc = hpib_io;
	do_dil_exit = unlock_close_check;

	/* init the buffers by linking a iobuf to a buf */
	for (i=0; i<ndilbuffers; i++) {
		dil_bufs[i].b_queue = &dil_iobufs[i];
		dil_bufs[i].dil_packet = (long) &dil_info[i];
	}
	/* any final init */
}

