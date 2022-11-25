/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dma.c,v $
 * $Revision: 1.8.84.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/22 13:15:19 $
 */

/* HPUX_ID: @(#)dma.c	55.1		88/12/23 */

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

#include "../h/debug.h"
#include "../h/param.h"
#include "../s200/pte.h"
#include "../h/buf.h"
#include "../s200io/dma.h"
#include "../h/user.h"
#include "../wsio/intrpt.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/ti9914.h"
#include "../wsio/dilio.h"
#include "../h/proc.h"
#include "../wsio/iobuf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/pregion.h"

extern int indirect_ptes;
extern int three_level_tables;
extern preg_t *kernpreg;
unsigned short dma_chan0_last_arm;
unsigned short dma_chan1_last_arm;

/******************************routine name******************************
** [short description of routine]
** default dma channel isr for both channels on level 3
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
dma_channel_isr(inf)
struct interrupt *inf;
{
	register struct dma_channel *ch = (struct dma_channel *)inf->temp;

	/*
	** We have just been called from the less than level 7 isr polling code
	** because we asked the dma card if it was interrupting and it said yes.
	** We may be here by mistake if a chain interrupt occured during
	** the polling instruction (the mov.b instruction that read the dma
	** card status register - see s200/locore.s).  We must check to make
	** sure we want to service this interrupt.
	*/

	/*
	** If dma is not enabled or not armed for this 
	** isr's level or is not interrupting then ignore.
	*/
	if (((((ch->card == (struct dma *)DMA_CHANNEL1)
  			? dma_chan1_last_arm : dma_chan0_last_arm) 
				& DMA_CHAN_MASK) != inf->misc) ||
	    (!(ch->card->dma_status & DMA_INT)))
		return; /* this is not our interrupt */

	if (ch->isr == 0) {
		panic("dma_channel_isr: No isr.");
	} else
		(*ch->isr)(ch->arg);
}

/******************************routine name******************************
** [short description of routine]
** initialize channels 0,1
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
dma_initialize()
{
	register struct dma_channel *ch;
	register int dma_id;
	short dummy;

	/*
	** see if dma really exists (can only do byte reads to odd address!)
	*/
	dma_here = testr((caddr_t)(DMA_CHANNEL0 + 7), 1);

	/*
	** if the card exists then set up both channels
	*/
	if (dma_here) {
		/*
		** see if programable dma is here
		*/
		if (testr((caddr_t)(PROGRAMABLE_DMA + 7), 1)) {
			printf("    HP98620A\n");
			panic("Must use a 98620B dma card; not a 98620A");
		} else {
			/* addr of dma id register */
			dma_id = *(int *)(DMA_CARD_BASE + 0x10);
			dma_id >>= 8; /* strip off bottom 2 bytes */
			if (dma_id == 0x323043) {
				dma_here = 3;	/* programmable dma (620C) */
#ifndef QUIETBOOT
				printf("    HP98620C DMA\n");
#endif /* ! QUIETBOOT */
				}
			else	{
				dma_here = 2;	/* non-program. dma (620B) */
#ifndef QUIETBOOT
				printf("    HP98620B DMA\n");
#endif /* ! QUIETBOOT */
				}

			/* 
		 	 * Under certain conditions w/ the revc boot rom
		 	 * preset, channel 0 is left armed after booting.
	 		 * Here we reset the arm word (by reading reg 0).
			 * See DSDe400605.
			 */
			dummy = *(short *)DMA_CHANNEL0;
		}

		free_dma_channels = 2;

		bus_master_count++;

		for (ch = dmatab; ch < dmatab + 2; ch++) {
			/*
			** initialize the dma channel variables
			*/
			ch->card = (struct dma *)
				(ch == dmatab ? DMA_CHANNEL0 : DMA_CHANNEL1);
			ch->card32 = (struct dma32 *)
			       (ch == dmatab ? DMA32_CHANNEL0 : DMA32_CHANNEL1);
			ch->sc = NULL;
			ch->isr = 0;
			ch->arg = 0;
			/*
			** we put these at level 3 for now but we may want to
			** put them at the level of any cards that use dma.
			*/
			isrlink(dma_channel_isr, 3, &ch->card->dma_status,
			    DMA_INT, DMA_INT, dmachan_arm_level(3), (int *)ch);
			/* 
			** Allocate space for the dma chain arrays.
			*/
			if ((ch->base = (struct dma_chain *)calloc(DMACHANSZ))
									== 0)
				panic("dma_initialize");
		}
	}
}


/*
** DMA CHANNEL ALLOCATION CONTROL ROUTINES:
**
**   dma_active/dma_unactive - advise system that this sc will hope to use dma
**   dma_reserve/dma_unreserve - guarantee an available channel for get_dma
**   dma_lock/dma_unlock - get and lock select code ownership of a channel
**
** generic rules:
**   1) successful dma_XXX calls and corresponding dma_unXXX calls must match
**   2) all routines must be called from the synchronous portion of the driver
**   3) dma_reserve must be in effect if get_dma is used
**   4) the routines are hierarchial, in that a dma_lock implies a dma_reserve,
**	and a dma_reserve implies a dma_active.  Thus, a driver does not have
**	to explicitly call each routine in the hierarchy, but rather call one
**	routine at the appropriate level.
**   5) dma_active is intended for the (buffered) disc drivers, where dma is
**	really beneficial because of its speed and low overhead per byte, but
**	is not absolutely necessary.  Dma_active allows the system to forecast
**	the demand for dma, and if dma appears to be a critical resource, the
**	system can deny requests by printers or plotters, since they can be
**	handled almost as well by interrupt transfers.  On the other hand, if
**	dma is not a critical resource, the system can grant all requests
**	for dma, taking advantage of its low overhead per byte characteristic.
**   6) dma_reserve is intended for the drivers which call get_dma, such as
**	amigo synchronous unbuffered discs and unbuffered mag tapes, where
**	a guaranteed transfer rate and limit on the delay between any two
**	bytes in the data stream are necessary.  Without dma_reserve, dma
**	locked by another select code could block a get_dma indefinitely.
**   7) dma_lock is intended for DIL applications, where guaranteed
**	immediate access to the dma is deemed necessary.
*/   


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
check_sc(sc)
struct isc_table_type *sc;
{
	if (sc == NULL)
		panic("check_sc");
}


/******************************routine name******************************
** [short description of routine]
** advise the system of intended dma usage by this select code
**
**   individual select codes keep a count of times active
**   free_dma_channels keeps a count of dma channels minus active select codes
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
dma_active(sc)
struct isc_table_type *sc;
{
	check_sc(sc);

	if (sc->dma_active++ == 0)
		free_dma_channels--;	/* ok to go negative */
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
dma_unactive(sc)
struct isc_table_type *sc;
{
	check_sc(sc);

	if (--sc->dma_active == 0)
		free_dma_channels++;
}

/******************************routine name******************************
** [short description of routine]
** guarantee a suitable dma channel is, and will remain, available for get_dma
**
**   dma channels keep a bit array of select codes reserving them
**   select codes keep a count of times reserved
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value: non-zero (TRUE) return value indicates success
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
dma_reserve(sc)
register struct isc_table_type *sc;
{
	register int my_bit;
	register struct dma_channel *ch;

	check_sc(sc);
	my_bit = 1 << sc->my_isc;

	/*
	** if dma not present, oh well...
	*/
	if (!dma_here)
		return 0;

	/*
	** has a channel already been reserved by this select code?
	*/
	if (dmatab[0].reserved & my_bit || dmatab[1].reserved & my_bit) {
		sc->dma_reserved++;
		dma_active(sc);
		return 1;
	}

	/*
	** channel not already reserved by this sc; if unlocked, can reserve it
	**
	**    note: since a channel is always reserved before locking, the
	**          above test for reserved handles the case of locking or
	**          reserving a channel that's already been locked by this sc
	*/
	for (ch = &dmatab[sc->card_type != INTERNAL_HPIB]; ch >= dmatab; ch--)
		if (!ch->locked) {
			ch->reserved |= my_bit;
			if (dmatab[0].reserved & dmatab[1].reserved)
				panic("dma_reserve");
			sc->dma_reserved++;
			dma_active(sc);
			return 1;
		}

	/*
	** all suitable channels currently locked!
	*/
	return 0;
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
dma_unreserve(sc)
register struct isc_table_type *sc;
{
	register int my_bit;
	register struct dma_channel *ch;

	check_sc(sc);
	my_bit = 1 << sc->my_isc;

	for (ch = &dmatab[sc->card_type != INTERNAL_HPIB]; ch >= dmatab; ch--)
		if (ch->reserved & my_bit) {
			if (--sc->dma_reserved == 0)
				ch->reserved &= ~my_bit;
			dma_unactive(sc);
			return;
		}
	
	panic("dma_unreserve");
}


/*
** get and lock a suitable dma channel
**
**   dma channel keeps a count of times locked and the owning select code
**
**   dma_lock_buf used for synchronizing multiple processes attempting
**     to modify ch->locked, and also for temporarily owning the select
**     code when it's necessary to get or drop dma
**
**   dma_lock_sc and dma_lock_bit are used when performing a get_dma for
**     the purpose of locking.  They prevent dma_assign from possibly
**     assigning the wrong channel.  Only the channel which was reserved
**     by dma_lock should be assigned for the purpose of locking.
**     For synchronization among multiple processes, dma_lock_sc and
**     dma_lock_bit should only be set by the owner of dma_lock_buf.
*/

struct buf dma_lock_buf;
struct isc_table_type *dma_lock_sc;
int dma_lock_bit;

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
dma_lock_wakeup(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	struct dma_channel *ch;

	if ((ch = sc->dma_chan) == NULL || ch->sc != sc)
		panic("dma_lock_wakeup");
	ch->locked++;
	dma_lock_sc = NULL;
	drop_selcode(bp);
	wakeup((caddr_t)bp);
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
dma_lock_get_dma(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc;

	if ((sc = bp->b_sc) == NULL || sc->owner != bp)
		panic("dma_lock_get_dma");
	dma_lock_sc = sc;
	get_dma(bp, dma_lock_wakeup);
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
dma_lock(sc)
register struct isc_table_type *sc;
{
	register struct buf *bp = &dma_lock_buf;
	register int my_bit;
	register struct dma_channel *preferred_ch, *ch, *other_ch;
	int x;

	check_sc(sc);
	my_bit = 1 << sc->my_isc;

	/*
	** if dma not present, oh well...
	*/
	if (!dma_here)
		return 0;

	/*
	** The "preferred" channel to lock for non internal hpib's is
	** channel 1, so that channel 0 can be left available for the
	** internal hpib as well as other card usage.  The "preferred"
	** and in fact the only channel to lock for the internal hpib
	** is channel 0.
	*/
	preferred_ch = &dmatab[sc->card_type != INTERNAL_HPIB];

	/*
	** guarantee exclusive access for possibly modifying ch->locked
	*/
	acquire_buf(bp);
	bp->b_sc = sc;

	/*
	** has a channel already been locked by this select code?
	*/
	for (ch = preferred_ch; ch >= dmatab; ch--)
		if (ch->locked && ch->sc == sc) {
			if (dma_reserve(sc) == 0)
				panic("dma_lock: can't reserve existing lock");
			ch->locked++;
			release_buf(bp);
			return 1;
		}

	/*
	** try to come up with a lockable channel, then lock it
	*/
	for (ch = preferred_ch; ch >= dmatab; ch--) {
		/*
		** first off, it mustn't already be locked
		*/
		if (ch->locked)
			continue;
		/*
		** if this channel is reserved by other select codes,
		** we'll have to see about moving their reservations to
		** the other channel
		*/
		other_ch = dmatab + !(ch - dmatab);
		if (ch->reserved & ~my_bit) {
			if (other_ch->locked)
				continue;
			if (sc->my_isc != 7 &&
			    ch->reserved & (1 << 7) &&
			    isc_table[7]->card_type == INTERNAL_HPIB)
				continue;
			other_ch->reserved |= ch->reserved;
		}
		/*
		** regardless of any previous reservations, the other channel
		** won't be reserved by this select code, and this channel will
		**
		** if this select code hadn't reserved a channel previously,
		** there will be a harmless momentary inconsistency between the
		** time we set the reserved bit here and the time the formal
		** reserve occurs, but this is necessary since we must force
		** the reserve to occur on the correct channel
		*/
		other_ch->reserved &= ~my_bit;
		ch->reserved = my_bit;
		if (dma_reserve(sc) == 0)
			panic("dma_lock: can't formally reserve for new lock");
		/*
		** let asynchronous procedures get ownership
		** of the channel and lock it
		*/
		dma_lock_bit = my_bit;
		get_selcode(bp, dma_lock_get_dma);
		do; while ((selcode_dequeue(sc) | dma_dequeue()));
		x = splx(sc->int_lvl);
		while (!ch->locked)
			sleep((caddr_t)bp, PRIBIO+1);
		splx(x);
		release_buf(bp);
		return 1;
	}

	/*
	** all suitable channels currently locked or reserved!
	*/
	release_buf(bp);
	return 0;
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
dma_unlock_drop_everything(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc;
	register struct dma_channel *ch;

	if ((sc = bp->b_sc) == NULL ||
	    sc->owner != bp ||
	    (ch = sc->dma_chan) == NULL ||
	    ch->sc != sc ||
	    --ch->locked != 0)
		panic("dma_unlock_drop_everything");
	drop_dma(bp);
	drop_selcode(bp);
	wakeup((caddr_t)bp);
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
dma_unlock(sc)
register struct isc_table_type *sc;
{
	register struct buf *bp = &dma_lock_buf;
	register struct dma_channel *ch;
	int x;

	check_sc(sc);

	/*
	** guarantee exclusive access for possibly modifying ch->locked
	*/
	acquire_buf(bp);
	bp->b_sc = sc;

	for (ch = &dmatab[sc->card_type != INTERNAL_HPIB]; ch >= dmatab; ch--)
		if (ch->locked && ch->sc == sc) {
			if (ch->locked == 1) {
				get_selcode(bp, dma_unlock_drop_everything);
				do; while ((selcode_dequeue(sc) | dma_dequeue()));
				x = splx(sc->int_lvl);
				while (ch->locked)
					sleep((caddr_t)bp, PRIBIO+1);
				splx(x);
			} else
				--ch->locked;
			release_buf(bp);
			dma_unreserve(sc);
			return;
		}

	panic("dma_unlock");
}

/******************************routine name******************************
** [short description of routine]
** try to get a dma channel
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
try_dma(sc, transfer_request)
register struct isc_table_type *sc;
enum transfer_request_type transfer_request;
{
	register struct dma_channel *ch;
	int s;

	/*
	** does the select code already own a dma channel?
	*/
	if ((ch = sc->dma_chan) != NULL) {
		if (ch->sc != sc)
			panic("try_dma: channel half-owned");
		return 1;
	}

	/*
	** for now we check if we have the programable dma card (dma_here = 2).
	** If we do then all is well, otherwise we return 0 if we are talking
	** to other than a simon card since the dma transfers for these need to
	** operate at two interrupt levels.
	*/
	if (dma_here == 0 ||
	/*
	** if the transfer request isn't for maximum speed and there's a
	** shortage of free dma channels, be more stingy with them
	*/
	    transfer_request != MAX_SPEED &&
				(free_dma_channels < 0 ||
				 free_dma_channels == 0 && !sc->dma_active)
			) {
		return 0;
	}

	s = spl6();
	/*
	** if the selcode for this buffer is the internal hpib then
	** we will only want to try to get channel 0 else we will try
	** for channel 1 and then 0.
	*/
	for (ch = &dmatab[sc->card_type != INTERNAL_HPIB]; ch >= dmatab; ch--)
		if (ch->sc == NULL) {	/* channel available */
			ch->sc = sc;
			sc->dma_chan = ch;
			splx(s);
			return 1;	/* got a channel */
		}
	/*
	** suitable channel unavailable
	*/
	splx(s);
	return 0;	/* did not get a channel */
}

/******************************routine name******************************
** [short description of routine]
** get on a queue to allocate a dma channel
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
get_dma(bp, func)
register struct buf *bp;
int (*func)();
{
	/* due to disgusting hardware design, there is one card that
	   can only use one DMA channel.  (The internal HPIB can only
	   use channel 0.)  Because of this, there are two DMA queues;
	   if only channel 0 is acceptable, it goes on one queue.  If
	   either is acceptable, it goes on another.  During dequeuing,
	   the channel 0 queue is interrogated first, on the assumption
	   that the 'eithers' will get their shot at channel 1 if channel
	   0 is hogged.  This assumption may not be totally valid.  It 
	   might be desireable to permit 'eithers' to force in ahead of
	   channel 0 requests, but for now...
	   (An argument of 0 implies zero only, one implies both).
	*/
	register struct isc_table_type *sc = bp->b_sc;
	register int my_bit = 1 << sc->my_isc;

	register struct dma_channel *ch;
	register struct buf *dma_queue;
	int s;

	/*
	** is a dma channel reserved by this select code?
	*/
	if (!(dmatab[0].reserved & my_bit || dmatab[1].reserved & my_bit))
		panic("get_dma: dma not reserved");

	/*
	** Does the select code already own a dma channel?
        **
        ** In most cases, it shouldn't, because calling get_dma
        ** twice without a drop_dma in between is an error, and
        ** a bad one, because it means either a driver's state
        ** machine is screwed up or a drop_dma is missing from
        ** dma cleanup in a driver.  The exception is if this
        ** guy has previously locked the dma channel.  If he
        ** has, he will also own it at this point.  Thus, this
        ** check is: if this channel is owned, has someone 
        ** called us twice (an error) or is it owned because the 
        ** channel is locked (OK)?
	*/
	if ((ch = sc->dma_chan) != NULL) { /* I think I own a channel */
		if (ch->sc != sc) 
			panic("get_dma: channel half-owned");
		if (!ch->locked) /* I own the channel, but it's not locked */
			panic("get_dma: channel not locked");
		(*func)(bp);
		return;
	}

	/*
	** note: even if/when channel 1 gets locked, requests can still
	**       be handled by its queue since channel 1 requests can
	**       be assigned to channel 0
	*/
	ch = &dmatab[sc->card_type != INTERNAL_HPIB];

	bp->b_action = func;
	bp->av_forw = NULL;

	/*
	** this buf already on the queue?
	*/
	for (dma_queue = ch->b_actf; dma_queue != NULL; dma_queue = dma_queue->av_forw)
		if (dma_queue == bp)
			panic("get_dma: already on dma queue");

	/*
	** put this buf on the tail of the queue
	*/
        s = spl6();
        if (ch->b_actf == NULL) 
                ch->b_actf = bp;
        else
                ch->b_actl->av_forw = bp;
        ch->b_actl = bp;
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
dma_assign(dmaq, ch)
struct dma_channel *dmaq;
struct dma_channel *ch;
{
	struct buf *bp;
	struct isc_table_type *sc;
	int s;

	s = spl6();
	/*
	** if there's not a request, or the channel's unavailable...
	** or if this request is for locking and this isn't the right channel..
	** then we can't assign
	*/
	if ((bp = dmaq->b_actf) == NULL || ch->sc != NULL ||
	    (sc = bp->b_sc) == dma_lock_sc && !(ch->reserved & dma_lock_bit)) {
		splx(s);
		return 0;
	}

	dmaq->b_actf = bp->av_forw;
	sc->dma_chan = ch;
	ch->sc = sc;
	splx(s);

	(*bp->b_action)(bp);
	return 1;
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
dma_dequeue()
{
	/*
	** if nothing queued, take a short cut
	*/
	if (dmatab[0].b_actf == NULL && dmatab[1].b_actf == NULL)
		return 0;
	/*
	** assign channel 0 to requests for zero first,
	** then assign either channel to requests in channel 1's queue
	*/
	return 
		dma_assign(&dmatab[0], &dmatab[0]) +
		dma_assign(&dmatab[1], &dmatab[1]) +
		dma_assign(&dmatab[1], &dmatab[0]);
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
drop_dma(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct dma_channel *ch;
	register struct buf *current, *previous;
	int x;

	/* this is needed because DCACHE is enabled for the networking */
	/* buffers.  however, it is not enabled for the file system */
	/* buffer cache.  this should be looked at more carefully in */
	/* the future. */
	/* flush internal and external dcache after inbound dma */
	if (bp->b_flags & B_READ) {
		/*
		** This will flush the dcache for the S320 and S350.
		** It will also flush the on-chip dcache for Wolverine
		** and Weasel but it does not flush Wolverines external
		** dcache.
		*/
		purge_dcache(); 

		/*
		** This will flush the external cache on wolverine.
		** It will also flush the cache on the S320 and S350
		** again but we dont care.
		*/
		purge_dcache_physical();
	}
	/*
	** if the sc is not owned or the dma channel is locked, then do nothing
	*/
	if (sc == NULL || (ch = sc->dma_chan) != NULL && ch->locked)
		return;

	/*
	** if owned, free the channel
	*/
	if (ch != NULL && ch->sc == sc) {
		x = spl6();
		sc->dma_chan = NULL;
		ch->sc = NULL;
		ch->isr = 0;
		ch->arg = 0;
		splx(x);
	} else { /* take off the list if it is there, should not happen often */
		ch = &dmatab[bp->b_sc->card_type != INTERNAL_HPIB];
		previous = NULL;
		x = spl6();	/* can`t have the queue change now */
		current = ch->b_actf;
		while (current != NULL) {
			if (current == bp)
				break;	/* found it */
			previous = current;
			current = current->av_forw;
		}
		if (current != NULL) {	/* take if off list */
			if (previous == NULL) /* first on list */
				ch->b_actf = current->av_forw;
			else {
				previous->av_forw = current->av_forw;
				/* end of list check */
				if (ch->b_actl == current)
					ch->b_actl = previous;
			}
		}
		splx(x);
	}
}

/******************************routine name******************************
** [short description of routine]
** This routine fills in the dma chain array.
** It will build one for either dma channel and
** for either the simon, ti9914, or gpio io card.
** For the HP-IB's, it assumes that EOI will always be tagged on 
** the last byte out and an input transfer will always
** terminate on an EOI (maybe prematurely).
**
** N.B. There is now the assumption for *all* I/O that the
** buffer address is a kernel logical address and that
** the buffer is logically contiguous.
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
dma_build_chain(bp)
struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	struct dma_channel *ch = sc->dma_chan;
	int reading = bp->b_flags & B_READ;
	struct iobuf *iob = bp->b_queue;
	register unsigned laddr = (int)iob->b_xaddr;
	register int len = iob->b_xcount;
	register struct dma_chain *base;
	register caddr_t io_reg;
	register int i, j;
	register unsigned char io_byte;
	register unsigned short dma_arm;
	register struct pte *pte;
	struct dma_chain dma_setup_struct;
	struct dma_chain *dma_setup = &dma_setup_struct;
	int count_shift;
#ifdef OSDEBUG
	int nchain = 0;
#endif

	/* This routine works in several steps:
	   1) calculate the various hardware control values needed.
	   2) calculate the actual DMA chain based on 1 and 2 above
	   3) Fix up the last entry in the chain for proper termination
	      conditions.

	A fixup is simpler than trying to handle all the special cases
	of termination while calculating addresses and byte counts.
	(Ahh... what I wouldn't give for a MASSBUS.)
	*/

	/* NOTE: It is the responsibility of the calling routine to
	**       properly setup the IO card for DMA, etc.  This 
	**	 routine only sets up those things required to continue/end
	**	 a chained DMA transfer.
	*/

	if (ch == NULL || ch->sc != sc)
		panic("dma_build_chain: channel not owned");
	base = ch->base;

	/*
	** card-specific code
	*/
	(*sc->iosw->iod_dma_setup)(bp, dma_setup, FIRST_ENTRIES);
	io_byte = dma_setup->card_byte;
	io_reg = dma_setup->card_reg;
	dma_arm = dma_setup->arm;

	/*
	** common dma arm word adjustments
	*/
	if (!reading)
		dma_arm |= DMA_IO;

	if (sc->state & F_PRIORITY_DMA)
		dma_arm |= DMA_PRI;

	if (sc->state & F_LWORD_DMA)	{
		count_shift = 2;
		dma_arm |= DMA32_LWORD | DMA32_START;
		if (laddr & 3)
			panic("dma_build_chain: bad address in long word mode");
		if (len & 3)
			panic("dma_build_chain: bad length in long word mode");
	}
	else if (count_shift = ((sc->state & F_WORD_DMA) != 0)) {
		dma_arm |= DMA_WB;
		if (laddr & 1)
			panic("dma_build_chain: odd address with word mode");
		if (len & 1)
			panic("dma_build_chain: odd length with word mode");
	}

	if (len <= 0)
		panic("dma_build_chain: negative or zero count");

	if ((bp->b_spaddr == KERNELSPACE) &&
				(laddr >= (unsigned int)tt_region_addr)) {
		i = (unsigned int)laddr >> PGSHIFT;
	} else {
		pte = vastopte((vas_t *)bp->b_spaddr, laddr);
		VASSERT(pte);
		if (!pte->pg_pfnum)
				panic("dma_build_chain: bad address -sys");
		i = PG_PFNUM(*((int *)pte++));
	}
		
	/*
	** first chain entry
	*/
	j = laddr & PGOFSET; /* get page offset bits */ 
	base->buffer = (caddr_t)(i << PGSHIFT) + j; /* physical bufad */ 
	j = NBPG - j; /* calculate bytes remaining in page */
	base->count = (j >> count_shift) - 1;
	base->arm = dma_arm;
	base->card_reg = io_reg;
	base->card_byte = io_byte;
	base->level = 7;
	laddr += j;
	len -= j;
	j = ++i; /* j = next sequential physical page frame number */

	/*
	** fill the dma array with entries
	*/
	while (len > 0) {
		if ((bp->b_spaddr == KERNELSPACE) &&
				(laddr >= (unsigned int)tt_region_addr)) {
			i = (unsigned int)laddr >> PGSHIFT;
		} else {
			/* When walk of current page table, calculate new one */
			if ((((unsigned long)pte & (NBPG-1)) == 0L) || three_level_tables || indirect_ptes) {
				pte = vastopte((vas_t *)bp->b_spaddr, laddr);
				VASSERT(pte);
			}
			if (!pte->pg_pfnum)
				panic("dma_build_chain: bad address -sys");
			i = PG_PFNUM(*((int *)pte++));
		}
		if (i == j++) { /* if sequential, just add to count */
			base->count += NBPG >> count_shift;
		} else {
			base++;
#ifdef OSDEBUG
			nchain += 1;
			VASSERT(nchain < (DMA_PAGES+3));
#endif
			base->buffer = (caddr_t)(i << PGSHIFT);
			base->count = (NBPG >> count_shift) - 1;
			base->arm = dma_arm;
			base->card_reg = io_reg;
			base->card_byte = io_byte;
			base->level = 7;
			j = ++i;
		}
		laddr += NBPG;
		len -= NBPG;
	}
	/* now take care of the fractional last page
	   of this part (if any) */
	if (len < 0) 
		base->count += len >> count_shift;

	/*
	** Fix up the last entry(s)
	*/
	ch->extent = (*sc->iosw->iod_dma_setup)(bp, base, LAST_ENTRY);
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
dma_start(ch)
struct dma_channel *ch;
{
	register struct dma_chain *base = ch->base;
	register struct dma *dma = ch->card;
	register struct isc_table_type *sc = ch->sc;

	/* This tells us which DMA channel we are starting DMA on.
	 * If "dma" == "DMA_CHANNEL1", then the boolean compare
	 * will result in a 1 in "channel", else "channel" is 0 
 	 */
	register channel = dma == (struct dma *)DMA_CHANNEL1;

        purge_dcache_physical();

	/* pick up the pointer to the channel and increment it.
	 * Everything must be ready before the arm word is written
	 * as the full processor bandwith may go to DMA until a chain
	 * interrupt occurs
	 */
	dmachain[channel] = base + 1;

	/* remember the arm word */
	if (channel) dma_chan1_last_arm = base->arm;
	else         dma_chan0_last_arm = base->arm;

	/* the transfer chain has been built by now */
	if (sc->state&F_LWORD_DMA) {
		register struct dma32 *dma32 = ch->card32;
		register char dummy = *(base->card_reg); /* for Simon's sake! */
		if (dma == (struct dma *)DMA_CHANNEL0)
			dma32_chan0++;
		else
			dma32_chan1++;
		*(base->card_reg) = base->card_byte;
		dma32->dma32_address = base->buffer;
		dma32->dma32_count = (unsigned long)base->count;
		dma32->dma32_control = base->arm;
	}
	else {
		register char dummy = *(base->card_reg); /* for Simon's sake! */
		if (dma == (struct dma *)DMA_CHANNEL0)
			dma32_chan0 = 0;
		else
			dma32_chan1 = 0;
		*(base->card_reg) = base->card_byte;
		dma->dma_address = base->buffer;
		dma->dma_count = base->count;
		dma->dma_control = base->arm;
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
dmaend_isr(sc)
register struct isc_table_type *sc;
{
	register unsigned short resid;
	struct dma_channel *ch;
	struct dma_chain *chain;

	if (sc->owner == NULL || (ch = sc->dma_chan) == NULL)
		return;

	resid = (int)ch->card->dma_reset;  /* reset DMA */
	resid = ch->card->dma_count;
	if (resid == 0xffff) 
		resid = 0;
	else
		resid += 1;
	if (ch->card == (struct dma *)DMA_CHANNEL0)
		chain = dmachain[0];
	else
		chain = dmachain[1];
	for (;chain <= ch->extent; chain++)
		resid += chain->count+1;

	sc->resid = resid;
}

wait_for_dma(sc)
register struct isc_table_type *sc;
{
	register unsigned short currcount, newcount;
	struct dma_channel *ch;

	if (sc->owner == NULL || (ch = sc->dma_chan) == NULL)
		return;

	currcount = ch->card->dma_count;

	/* if the count is -1 then dma is done */
	if (currcount == 0xffff) 
		return;

	/* dma hasn't completed it's count yet */
	while (1) {
		/* give dma some time to complete */
		snooze(10);
		newcount = ch->card->dma_count;

		/* if the count is -1 then dma is done */
		if (newcount == 0xffff) 
			return;

		/* dma still not done, did it make any progress? */
		if (newcount != currcount)
			/* yes, so give it more time */
			currcount = newcount;
		else
			/* no progress, give up */
			return;
	}
}
