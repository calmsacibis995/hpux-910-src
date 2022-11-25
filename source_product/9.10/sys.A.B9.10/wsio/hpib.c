/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/hpib.c,v $
 * $Revision: 1.3.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 12:21:48 $
 */
/* HPUX_ID: @(#)hpib.c	55.1		88/12/23 */

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
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/iobuf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../wsio/tryrec.h"
#include "../wsio/dil.h"
#include "../wsio/dilio.h"


int HPIB_utility();

enum state {start = 0, doit, ppoll, hpib_timedout, hpib_defaul};

int pct_flag;

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
HPIB_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,HPIB_utility,bp->b_sc->int_lvl,0,hpib_timedout);
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
** a note on b_action2 & b_clock_ticks: these act as arguments to HPIB_utility.
** So far there has been no indication that b_action2 need be of other than
** type procedure, thus it is not generalized.  If this should change, there
** is no strong argument against making b_action2 "untyped".   
************************************************************************/
HPIB_utility(bp)
struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	try
		START_FSM;
		switch ((enum state)iob->b_state) {
		case start:
			iob->b_state = (int)doit;
			get_selcode(bp, HPIB_utility);
			break;

		case doit:
			START_TIME(HPIB_timeout, (250*HZ)/1000);
			iob->b_xaddr = bp->b_un.b_addr;
			iob->b_xcount = bp->b_bcount;
			/* the result is an indication from the 
			   particular function whether or not 
			   it wants a parallel poll afterward */
			if ((*bp->b_action2)(bp)) {
				END_TIME;
				START_TIME(HPIB_timeout, bp->b_clock_ticks);
				iob->b_state = (int)ppoll;
				HPIB_ppoll_drop_sc(bp, HPIB_utility, 1);
			} else {
				drop_selcode(bp);
				END_TIME;
				iob->b_state = (int)hpib_defaul;
				queuedone(bp);
			}
			break;

		case ppoll:
			END_TIME;
			iob->b_state = (int)hpib_defaul;
			queuedone(bp);
			break;

		case hpib_timedout: 
			escape(TIMED_OUT);
			break;

		hpib_default:
			panic("bad identify state");
		}
		END_FSM;
	recover {
		iob->b_state = (int)hpib_defaul;
		ABORT_TIME;
		if (escapecode == TIMED_OUT) {
			(*bp->b_sc->iosw->iod_abort)(bp);
			HPIB_ppoll_clear(bp);
		}
		drop_selcode(bp);
		bp->b_error = EIO;
		queuedone(bp);
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
#ifdef NEVER_CALLED
HPIB_clear_wopp(bp)
struct buf *bp;
{
	bp->b_action2 = bp->b_sc->iosw->iod_clear_wopp;
	bp->b_clock_ticks = 0;  /* actually a don't care */
	HPIB_utility(bp);
}
#endif /* NEVER_CALLED */

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
HPIB_clear(bp)
struct buf *bp;
{
	bp->b_action2 = bp->b_sc->iosw->iod_clear;
	bp->b_clock_ticks = 5*HZ;
	HPIB_utility(bp);
}

/******************************HPIB_identify***************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:  bp
**
**	Kernel routines called:  HPIB_utility (iod_ident)
**
**	Global variables/fields referenced:  b_sc->iosw->iod_ident
**
**	Global vars/fields modified: bp->b_action2, bp->b_clock_ticks
**
**	Algorithm:  [?? if needed]
************************************************************************/
HPIB_identify(bp)
struct buf *bp;
{
	bp->b_action2 = bp->b_sc->iosw->iod_ident;
	bp->b_clock_ticks = 0;  /* actually a don't care */
	HPIB_utility(bp);
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
**	Kernel routines called:  iod->pplclr,
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
HPIB_call_isr(sc)
register struct isc_table_type *sc;
{
	register struct buf *cur_buf, *next_buf;
	register struct dil_info *dil_info;
	register int x;

	if (sc->int_flags & INT_PPL) {
		register int resp = (*sc->iosw->iod_ppoll)(sc);
		for (cur_buf = sc->ppoll_f; cur_buf != NULL; cur_buf = next_buf) {
			/*
			** determine the next buf now, since calling b_action
			** will place the current buf on some other queue,
			** thus losing its pointers to this queue!
			*/
			next_buf = cur_buf->av_forw;
			if (resp & (cur_buf->b_flags & B_DIL ? cur_buf->b_ba : 0x80 >> cur_buf->b_ba)) {
				if (cur_buf->av_back != NULL)
					cur_buf->av_back->av_forw = cur_buf->av_forw;
				else
					sc->ppoll_f = cur_buf->av_forw;
				if (cur_buf->av_forw != NULL)
					cur_buf->av_forw->av_back = cur_buf->av_back;
				else
					sc->ppoll_l = cur_buf->av_back;
				if (cur_buf->b_flags & B_DIL)
					cur_buf->b_bufsize = resp;
				(*cur_buf->b_action)(cur_buf);
			}
		}
		for (cur_buf = sc->event_f; cur_buf != NULL; cur_buf = next_buf) {
			dil_info = (struct dil_info *)cur_buf->dil_packet;
			next_buf = dil_info->dil_forw;
			if (dil_info->intr_wait & INTR_PPOLL) {
				if (resp & dil_info->ppl_mask) {
					dil_info->event |= INTR_PPOLL;
					dil_info->ppl_mask = resp;
					dil_info->intr_wait &= ~INTR_PPOLL;
					deliver_dil_interrupt(cur_buf);
				}
			}
		}
		(*sc->iosw->iod_pplclr)(sc, resp);
		sc->int_flags &= ~INT_PPL;
	}

	if (sc->int_flags) {
		for (cur_buf = sc->status_f; cur_buf != NULL; cur_buf = next_buf) {
			/*
			** determine the next buf now, since calling b_action
			** will place the current buf on some other queue,
			** thus losing its pointers to this queue!
			*/
			dil_info = (struct dil_info *)cur_buf->dil_packet;
			next_buf = cur_buf->av_forw;
			if (dil_info->intr_wait & sc->int_flags) {
				if (cur_buf->av_back != NULL)
					cur_buf->av_back->av_forw = cur_buf->av_forw;
				else
					sc->status_f = cur_buf->av_forw;
				if (cur_buf->av_forw != NULL)
					cur_buf->av_forw->av_back = cur_buf->av_back;
				else
					sc->status_l = cur_buf->av_back;
				(*cur_buf->b_action)(cur_buf);
			}
		}
		sc->int_flags = 0;
	}
}

/******************************routine name******************************
** [short description of routine]
** put this buffer on the linked list of status candidates 
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
HPIB_status_int (bp, proc)
register struct buf *bp;
int (*proc)();
{
	register struct isc_table_type *sc;
	int s;

	sc = bp->b_sc;
	bp->b_action = proc;
		
	bp->av_forw = NULL;
        s = spl6();
        if (sc->status_f == NULL) {
                sc->status_f = bp;
		sc->status_l = NULL;
	}
        else
                sc->status_l->av_forw = bp;
	bp->av_back = sc->status_l;
        sc->status_l = bp;
	splx(s);
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
**	Kernel routines called: iod_wait_clr,
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
HPIB_status_clear(my_buf)
register struct buf *my_buf;
{
	register struct buf *this_buf, *next_buf;
	register struct isc_table_type *sc = my_buf->b_sc;
	register struct dil_info *my_dil_info = (struct dil_info *) my_buf->dil_packet;
	register struct dil_info *this_dil_info;
	register int new_wait_mask = 0;
	int x;

	x = spl6();
	this_buf = sc->status_f;
	while (this_buf != NULL) {
		this_dil_info = (struct dil_info *)this_buf->dil_packet;
		next_buf = this_buf->av_forw;
		if (my_buf == this_buf) {
			if (this_buf->av_back != NULL)
				this_buf->av_back->av_forw = this_buf->av_forw;
			else
				sc->status_f = this_buf->av_forw;
			if (this_buf->av_forw != NULL)
				this_buf->av_forw->av_back = this_buf->av_back;
			else
				sc->status_l = this_buf->av_back;
		} else 
			new_wait_mask |= this_dil_info->intr_wait;
		this_buf = next_buf;
	}
	sc->intr_wait = new_wait_mask | (sc->intr_wait & INTR_MASK); /* don't touch intr bits */
	(*sc->iosw->iod_wait_clr)(sc);
	splx(x);
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
**	Kernel routines called:  iod_pplset,
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
HPIB_ppoll_int (bp, proc, sense)
register struct buf *bp;
int (*proc)();
int sense;
{
	register struct isc_table_type *sc;
	register unsigned char mask;
	int s;

	/* dil needs this to be ok
	if (bp->b_ba > 7) {
		panic("bad ppoll bus address");
	} */
	sc = bp->b_sc;
	bp->b_action = proc;
		
	bp->av_forw = NULL;
        s = spl6();
        if (sc->ppoll_f == NULL) {
                sc->ppoll_f = bp;
		sc->ppoll_l = NULL;
	}
        else
                sc->ppoll_l->av_forw = bp;
	bp->av_back = sc->ppoll_l;
        sc->ppoll_l = bp;
	splx(s);
	if (bp->b_flags & B_DIL) /* handle dil ppoll right */
		mask = bp->b_ba;
	else
		mask = 0x80 >> bp->b_ba;
	/* the interrupt could occur at the end of this next call */
	(*sc->iosw->iod_pplset)(sc, mask, sense, 1);
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
**	Kernel routines called: iod_pplset, 
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
HPIB_ppoll_drop_sc(bp, proc, sense)
register struct buf *bp;
int (*proc)();
int sense;
{
	register struct isc_table_type *sc;
	register unsigned char mask;
	int s;

	/* dil needs this to be ok
	if (bp->b_ba > 7) {
		panic("bad ppoll bus address");
	} */
	sc = bp->b_sc;
	bp->b_action = proc;
		
	bp->av_forw = NULL;
        s = spl6();
        if (sc->ppoll_f == NULL) {
                sc->ppoll_f = bp;
		sc->ppoll_l = NULL;
	}
        else
                sc->ppoll_l->av_forw = bp;
	bp->av_back = sc->ppoll_l;
        sc->ppoll_l = bp;
	if (bp->b_flags & B_DIL) /* handle dil ppoll right */
		mask = bp->b_ba;
	else
		mask = 0x80 >> bp->b_ba;
	(*sc->iosw->iod_pplset)(sc, mask, sense, 1);
	unprotected_drop_selcode(bp);
	splx(s);
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
HPIB_ppoll_clear(bp)
register struct buf *bp;
{
	register struct buf *cur_buf;
	register struct isc_table_type *sc = bp->b_sc;
	int s;

	/* must see if a parallel poll was outstanding before removed */

        s = spl6();
	cur_buf = sc->ppoll_f;	/* get list */
	while (cur_buf != NULL) {
		if (cur_buf == bp) {	/* found it */
			if (bp->av_back != NULL)
				bp->av_back->av_forw = bp->av_forw;
			else
				sc->ppoll_f = bp->av_forw;
			if (bp->av_forw != NULL)
				bp->av_forw->av_back = bp->av_back;
			else
				sc->ppoll_l = bp->av_back;
			break;
		}
		else
			cur_buf = cur_buf->av_forw;
	}
	splx(s);
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
**	Kernel routines called:, iod_pplset, iod_ppoll, iod_pplclr
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
HPIB_ppoll(bp, sense)
register struct buf *bp;
{
	register int mask;
	register struct isc_table_type *sc = bp->b_sc;
	register struct iobuf *iob = bp->b_queue;

	if (bp->b_flags & B_DIL) /* handle dil ppoll right */
		mask = bp->b_ba;
	else
		mask = 0x80 >> bp->b_ba;
	/* synchronously wait for parallel poll */
	(*sc->iosw->iod_pplset)(sc, mask, sense, 0);
	while (((*sc->iosw->iod_ppoll)(sc) & mask) == 0)
		if (iob->timeflag) {
			iob->timeflag = FALSE;
			escape(TIMED_OUT);
		}
	(*sc->iosw->iod_pplclr)(sc, mask);
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
**	Kernel routines called: iod_pplset, iod_ppoll, iod_pplclr
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
int HPIB_test_ppoll(bp, sense)
register struct buf *bp;
{
	register int mask, response;
	register struct isc_table_type *sc = bp->b_sc;

	mask = (bp->b_flags & B_DIL) ? bp->b_ba : 0x80 >> bp->b_ba;
	(*sc->iosw->iod_pplset)(sc, mask, sense, 0);
	response = (*sc->iosw->iod_ppoll)(sc) & mask;
	(*sc->iosw->iod_pplclr)(sc, mask);
	return response;
}


/* These two routines live in ws_utl.c on the 700.  We should probably
   pick one place for them to live for both machines.  Take your pick.
*/
#ifdef __hp9000s300
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
acquire_buf(bp)
register struct buf *bp;
{
	int s;
	s = spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO+1);
	}
	bp->b_flags = B_BUSY;
	bp->b_error = 0;
	splx(s);
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
release_buf(bp)
register struct buf *bp;
{
	int s;
	s = spl6();
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	splx(s);
	bp->b_flags &= ~(B_BUSY|B_WANTED);
}
#endif /* __hp9000s300 */

/* The following two routines were moved here from dil.c to allow dil
	to be configurable -- pam 02/29/88				*/

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
HPIB_event_clear(my_buf)
register struct buf *my_buf;
{
	register struct buf *this_buf;
	register struct isc_table_type *sc = my_buf->b_sc;
	register struct dil_info *my_dil_info = (struct dil_info *)my_buf->dil_packet;
	register int new_rupt_mask = 0;
	register struct dil_info *this_dil_info;
	int x;

	my_dil_info->intr_wait = 0;
	my_dil_info->eid = 0;
	my_dil_info->event = 0;
	my_dil_info->cause = 0;
	my_dil_info->intr_wait &= ~INTR_MASK;   /* clear all my intr bits */
	my_dil_info->ppl_mask = 0;
	x = spl6();
	this_buf = sc->event_f;
	while (this_buf != NULL) {
		this_dil_info = (struct dil_info *)this_buf->dil_packet;
		if (my_buf == this_buf) {
			if (my_dil_info->dil_back != NULL)
				((struct dil_info *)
				  my_dil_info->dil_back->dil_packet)->dil_forw = 
								  my_dil_info->dil_forw;
			else
				sc->event_f = my_dil_info->dil_forw;
			if (my_dil_info->dil_forw != NULL)
				((struct dil_info *)
				  my_dil_info->dil_forw->dil_packet)->dil_back = 
								  my_dil_info->dil_back;
			else
				sc->event_l = my_dil_info->dil_back;
			this_buf = this_dil_info->dil_forw;
		} else {
			new_rupt_mask = this_dil_info->intr_wait;
			this_buf = this_dil_info->dil_forw;
		}
	}

	sc->intr_wait = new_rupt_mask | (sc->intr_wait & ~INTR_MASK); /*don't touch wait bits */
	splx(x);
}

/*************************deliver_dil_interrupt**************************
** Links the buf pointed to by 'bp' to the list of bufs which need
** to have their io_on_interrupt handlers called for this process.
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
deliver_dil_interrupt(bp)
register struct buf *bp;
{
	register struct dil_info *dil_info = (struct dil_info *)bp->dil_packet;
	register struct proc *procp = dil_info->dil_procp;
	register struct buf *cur_buf;
	register int x;

	/* if this buf has no interrupt handler (forked proc), don't send
		him an interrupt or put him on the event list */
	if (procp->p_flag2 & S2SENDDILSIG) {

		/* if this buf is already on the list we are done */
		x = spl6();
		cur_buf = procp->p_dil_event_f;
		while (cur_buf != NULL) {
			if (cur_buf == bp) {
				splx(x);
				psignal(procp, procp->p_dil_signal);
				return;
			} else
				cur_buf = ((struct dil_info *) cur_buf->dil_packet)->event_forw;
		}

		/*
		** put this buf on the list of events pending
		** for this process.
		*/
		dil_info->event_forw = NULL;
		if (procp->p_dil_event_f == NULL) {
			procp->p_dil_event_f = bp;
			procp->p_dil_event_l = NULL;
		} else
			((struct dil_info *) 
				procp->p_dil_event_l->dil_packet)->event_forw = bp;
		dil_info->event_back = procp->p_dil_event_l;
		procp->p_dil_event_l = bp;
		splx(x);
		psignal(procp, procp->p_dil_signal);
	}
}
