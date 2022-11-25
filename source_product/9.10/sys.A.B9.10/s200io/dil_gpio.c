/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dil_gpio.c,v $
 * $Revision: 1.8.84.8 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/22 13:18:10 $
 */
/* HPUX_ID: @(#)dil_gpio.c	55.1		88/12/23 */
/* ifdef GPIO_DDB is for the s800 debugger. We ship this driver with 
   -UGPIO_DDB 
*/
#ifdef	GPIO_DDB
static char GPIO_DDBid[] = "$Header: dil_gpio.c,v 1.8.84.8 94/12/22 13:18:10 marshall Exp $";
#endif	GPIO_DDB

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
**
**	DIL kernel driver
**	GPIO driver
**
*/
#include "../h/param.h"
#include "../h/buf.h"
#include "../s200io/dma.h"
#include "../h/user.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/ti9914.h"
#include "../h/proc.h"
#include "../wsio/iobuf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../wsio/intrpt.h"
#include "../wsio/tryrec.h"
#include "../h/file.h"
#include "../wsio/dil.h"
#include "../s200io/gpio.h"
#include "../wsio/dilio.h"

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

extern int dil_config_identify();

extern int (*hpib_config_ident)();
#endif /* IOSCAN */

#define MAXPHYS (64 * 1024)

int gpio_do_isr();
int gpio_dma();
int gpio_abort();
int gpio_isr();
int dil_getsc();
int lock_timeout();
int dil_io_lock();
struct buf * get_dilbuf();

gpio_nop()
{
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
gpio_reg_access(bp)
register struct buf *bp;
{
	register struct GPIO *cp = (struct GPIO *) bp->b_sc->card_ptr;
	register int access_mode = bp->pass_data1;
	register int offset = bp->pass_data2;
	struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register int data;
	int x;

	x = splx(bp->b_sc->int_lvl);

	if (access_mode & READ_ACCESS) {
		if (access_mode & WORD_MODE) {
			if (offset == 4)
				bp->return_data = cp->data.word & 0xffff;
			else {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			}
		} else {
			switch (offset) {
			case 0:
				bp->return_data = cp->ready & 0xff;
				break;
			case 1:
				bp->return_data = cp->id_reg & 0xff;
				break;
			/* register 2 is a write-only register */
			case 3:
				bp->return_data = cp->intr_control & 0xff;
				break;
			case 4:
				bp->return_data = cp->data.byte.upper & 0xff;
				break;
			case 5:
				bp->return_data = cp->data.byte.lower & 0xff;
				break;
			case 7:
				bp->return_data = cp->p_status & 0xff;
				break;
			default:
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			}
		}
	} else {
		data = info->register_data;
		if (access_mode & WORD_MODE) {
			if (offset == 4)
				cp->data.word = data & 0xffff;
			else {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			}
		} else {
			switch (offset) {
			case 0:
				cp->set_pctl = data & 0xff;
				break;
			case 1:
				cp->reset_gpio = data & 0xff;
				break;
			case 2:
				cp->intr_mask = data & 0xff;
				break;
			case 3:
				cp->intr_status = data & 0xff;
				break;
			case 4:
				cp->data.byte.upper = data & 0xff;
				break;
			case 5:
				cp->data.byte.lower = data & 0xff;
				break;
			case 7:
				cp->p_control = data & 0xff;
				break;
			default:
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			}
		}
	}
	splx(x);
}

/******************************gpio_save_state**************************
**
**	Called by:  hpib_transfer via iosw
**
**	Parameters:  sc	
**
**	Global vars/fields modified: sc->state
**
************************************************************************/
gpio_save_state(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	cp->intr_mask = 0;
	cp->intr_control = 0;
	sc->state |= STATE_SAVED;
}

/******************************gpio_restore_state***********************
** [short description of routine]
**
**	Called by:  hpib_transfer via iosw
**
**	Parameters:	sc
**
**	Global variables/fields referenced:  sc->intr_wait, sc->card_ptr
**
**	Global vars/fields modified:	sc->state
**
**	Registers modified:	intr_mask, intr_control
**
**	Algorithm:  [?? if needed]
************************************************************************/
gpio_restore_state(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	if ((sc->intr_wait & INTR_GPIO_EIR) &&
	    (sc->intr_wait & INTR_GPIO_RDY)) {
		cp->intr_mask = GP_EIR1 | GP_RDYEN1;
		cp->intr_control = GP_ENAB;
		sc->state &= ~STATE_SAVED;
		return;
	}
	if (sc->intr_wait & INTR_GPIO_EIR) {
		cp->intr_mask = GP_EIR1;
		cp->intr_control = GP_ENAB;
	}
	if (sc->intr_wait & INTR_GPIO_RDY) {
		cp->intr_mask = GP_RDYEN1;
		cp->intr_control = GP_ENAB;
	}
	sc->state &= ~STATE_SAVED;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	bp
**
**	Global variables/fields referenced:  bp->pass_data1, b_sc->card_ptr
**
**	Registers modified:	p_control
**
**	Algorithm:  [?? if needed]
************************************************************************/
gpio_set_control(bp)
struct buf * bp;
{
	register struct GPIO *cp = (struct GPIO *) bp->b_sc->card_ptr;
	register int data;

	/* make sure we get the lower bits */
	data = bp->pass_data1 & 0x03;
	cp->p_control = data;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	bp
**
**	Global variables/fields referenced:  b_sc->card_ptr
**
**	Global vars/fields modified:	bp->return_data
**
**	Algorithm:  [?? if needed]
************************************************************************/
gpio_status(bp)
struct buf * bp;
{
	register struct GPIO *cp = (struct GPIO *) bp->b_sc->card_ptr;

	bp->return_data = cp->p_status;
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
gpio_iocheck(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;

	/*
	**  odd buffer address?
	*/
	if ((iob->dil_state & D_16_BIT) && 
	    ((int)bp->b_un.b_addr & 1)) {
		bp->b_error = EFAULT;
		bp->b_flags |= B_ERROR;
		return TRANSFER_ERROR;
	}

	/*
	**  odd transfer count in 16 bit width? 
	**  (transfer count always in bytes)
	*/
	if ((iob->dil_state & D_16_BIT) && 
	    ((int)bp->b_bcount & 1)) {
		bp->b_error = EFAULT;
		bp->b_flags |= B_ERROR;
		return TRANSFER_ERROR;
	}
	return TRANSFER_READY;	/* all ok for this transfer */
} 


/******************************routine name******************************
** [short description of routine]
**	This routine is responsible for the following:
**		1) selecting the type of transfer
**		2) kicking off the transfer
**
**	Called by:  hpib_transfer via iosw (iod_tfr)
**
**	Parameters:	request -- unused here, used in other iod_tfr
**				   routines
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
gpio_driver(request, bp, proc)
enum transfer_request_type request;	/* unused */
register struct buf *bp;
int (*proc)();
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register unsigned int state = iob->dil_state;

	bp->b_action = proc;

#ifndef	GPIO_DDB
	if (!(cp->p_status & GP_PSTS))
	{	sc->tfr_control = TR_END;
		sc->resid = bp->b_bcount;
		(*bp->b_action)(bp);
		return;
	}
#endif	not GPIO_DDB
	/* set up other control info here */
	sc->tfr_control = state;	/* transfer info */

	/*
	**  For speed during interrupt transfers the following fields
	**  are used:
	**	sc->intcopy  => iob->dil_state
	**	sc->intmsksav  => gp->termination_pattern
	**	sc->intmskcpy  => bp->b_flags
	*/
	sc->intcopy = iob->dil_state;
	sc->intmsksav = iob->read_pattern;
	sc->intmskcpy = bp->b_flags;

	/* selcect type of transfer */
	if ((bp->b_bcount == 1) ||
	    ((state & D_16_BIT) && (bp->b_bcount == 2)) ||
	    (state & READ_PATTERN) ||
	    (state & USE_INTR) ||
	    (! (try_dma(sc, MAX_SPEED))))  
		sc->transfer = INTR_TFR;	/* do interrupt transfer */
	else
		sc->transfer = DMA_TFR;		/* Otherwise do dma */

	sc->buffer = bp->b_queue->b_xaddr;
	sc->count = bp->b_queue->b_xcount;
	switch(sc->transfer) {
		case DMA_TFR:
			if (iob->dil_state & D_16_BIT) {
				sc->state |= F_WORD_DMA;	/* 16 bit */
				sc->buffer = (caddr_t)((int)sc->buffer + sc->count - 2);
			}
			else {
				sc->state &= ~F_WORD_DMA;	/* 8 bit */
				sc->buffer = (caddr_t)((int)sc->buffer + sc->count - 1);
			}
			sc->count = 1;

			if (cp->intr_control & GP_BURST)
				sc->state |= F_PRIORITY_DMA;	/* do burst */
			else
				sc->state &= ~F_PRIORITY_DMA;

			gpio_dma(bp);
			break;
		case INTR_TFR:
                        /* If dma is reserved and we're here, we decided
                         * not to use it, so give it back */
                        if (iob->b_flags & RESERVED_DMA)
                                drop_dma(sc->owner);

			gpio_intr(bp);
			break;
		case FHS_TFR:
		case BURST_TFR:
		case NO_TFR:
		default:
			panic("GPIO: transfer of unknown type");
	}
}


/******************************routine name******************************
** [short description of routine]
**	do a dma transfer
**
**	assume:
**		- selectcode already owned (by bp)
**		- dma channel already owned (by sc)
**		- transfer chain set up for 8 or 16 bit transfers
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
gpio_dma(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register struct dma_channel *dma = sc->dma_chan;
	register char dummy;  /* used for reading and writing card
				 registers when value is unused or doesn't matter */

	dma_build_chain(bp);

#ifdef	GPIO_DDB
	cp->intr_mask = 0;		/* Enable ready interrupt */
#else	GPIO_DDB
	if (bp->b_queue->dil_state & EIR_CONTROL)
	        cp->intr_mask = GP_EIR1;	/* enable on EIR only */
#endif	GPIO_DDB

	/* we want the dmacard to call us when it is finished */
	dma->isr = gpio_do_isr;
	dma->arg = (int) sc;

	/* prepare the gpio card for an input transfer */
	if (bp->b_flags & B_READ) {
#ifdef	GPIO_DDB
/***
 **
 *    The following violates the GPIO spec,
 *    but then so does the 800 MID-bus parallel
 *    board. --LG
 **
 ***/
		cp->set_pctl = 0;		/* set pctl on card, value written irrelevant */
		dummy = cp->data.byte.lower;	/* do a read */
#else	GPIO_DDB
		dummy = cp->data.byte.lower;	/* do a read, value not used */
		cp->set_pctl = dummy;		/* set pctl on card, value written irrelevant */
#endif
	}
	/* start the dma transfer */
	dma_start(dma);
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
gpio_intr(bp)
register struct buf *bp;
{
	register char dummy;	/* used for kicking card, value written doesn't matter */
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	if (sc->tfr_control & D_16_BIT) /* we have a 16 bit transfer */
		sc->count = sc->count / 2;	/* adjust count */

	/* interrupts must be disabled at this point on a read (until PCTL is set) */
#ifdef	GPIO_DDB
	cp->intr_mask = GP_RDYEN1;		/* enable on ready */
#else	GPIO_DDB
	if (bp->b_queue->dil_state & EIR_CONTROL)
	        cp->intr_mask = GP_RDYEN1 | GP_EIR1;	/* enable on ready and EIR */
	else
	        cp->intr_mask = GP_RDYEN1;              /* enable on ready only */
#endif	GPIO_DDB

	/* On a read, set I/O line to indicate an input and set PCTL --
	   this causes an interrupt when the device sends data and PFLG
	   is pulsed.  On a write, leave PCTL line low and this will 
	   cause a RDY interrupt to occur -- we start the I/O in the 
	   isr.
	*/
	if (bp->b_flags & B_READ) {
#ifdef	GPIO_DDB
/***
 **
 *    The following violates the GPIO spec,
 *    but then so does the 800 MID-bus parallel
 *    board. --LG
 **
 ***/
		cp->set_pctl = 0;		/* set PCTL, value written is irrelevant */
		dummy = cp->data.byte.lower;	/* set IO line, value read not used */
#else	GPIO_DDB
		dummy = cp->data.byte.lower;	/* set IO line, value read not used */
		cp->set_pctl = dummy;		/* set PCTL, value written is irrelevant,
						 * this causes the GPIO I/O line to 
						 * indicate an input transfer       */
#endif	GPIO_DDB
	}
	cp->intr_control = GP_ENAB;	/* enable card */
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
gpio_intr_isr(sc, cp, ext_intr)
register struct isc_table_type *sc;
register struct GPIO *cp;
int ext_intr;
{
	char dummy;		/* used for writing to card register, value written 
				   doesn't matter */
	register unsigned char control;
	register unsigned short data = 0;
	register unsigned short status;

	/* terminate the transfer on EIR? */
	if (ext_intr) {
		gpio_resid(sc);		/* find out how much is left */
		cp->intr_mask = 0;	/* disable interrupts */
		cp->intr_control = 0;
		sc->tfr_control = TR_END;
		sc->transfer = NO_TFR;
		gpio_reset(sc);		/* now clear the EIR line */
		return 0;
	}

	status = sc->intcopy;		/* for fast access */
	control = sc->tfr_control;	/* for fast access */

	if (sc->intmskcpy & B_READ) {	/* input transfers */
		struct dil_info *dil_info = 
				(struct dil_info *) sc->owner->dil_packet;
		if (status & D_16_BIT)	{ /* word transfers */
			data = cp->data.word;
			*(unsigned short *)sc->buffer = data;
			sc->buffer += 2;
		}
		else {	/* byte transfer */
			data = cp->data.byte.lower;
			*sc->buffer++ = data;
		}
		if (control & READ_PATTERN) {
			if (data == sc->intmsksav) {
				sc->tfr_control = TR_MATCH;
				sc->resid = --sc->count;
				if (status & D_16_BIT)
					sc->resid += sc->resid;
				if ((sc->count == 0) && 
					(dil_info->full_tfr_count <= MAXPHYS))
					sc->tfr_control |= TR_COUNT;
				cp->intr_mask = 0;	/* disable interrupts */
				cp->intr_control = 0;
				sc->transfer = NO_TFR;
				return 0;
			}
		}
		if (--sc->count) {	/* still more to go */
			cp->set_pctl = dummy;	/* set PCTL */
			return 1;	/* keep going */
		}
		else {	/* finished */
			/* don't want to reset PCTL now */
			gpio_resid(sc);		/* find out how much is left */
			cp->intr_mask = 0;	/* disable interrupts */
			cp->intr_control = 0;
			sc->tfr_control = 0;
			if (dil_info->full_tfr_count <= MAXPHYS)
				sc->tfr_control = TR_COUNT;
			sc->transfer = NO_TFR;
			return 0;
		}
	}
	else {	/* output transfers */
		if (status & D_16_BIT) {/* word transfers */
			cp->data.word = *(unsigned short *)sc->buffer;
			sc->buffer += 2;
		}
		else	/* byte transfer */
			cp->data.byte.lower = *sc->buffer++;
		if (--sc->count) {	/* still more to go */
			cp->set_pctl = dummy;
			return 1;	/* keep going */
		}
		else {	/* finished */
			gpio_resid(sc);		/* find out how much is left */
			cp->intr_mask = 0;	/* disable interrupts */
			cp->intr_control = 0;
			sc->tfr_control = TR_COUNT;
			cp->set_pctl = dummy;	/* set PCTL for last byte */
			if (cp->ready & GP_READY) {
				sc->transfer = NO_TFR;
				return 0;
			} else {
				sc->transfer = BO_END_TFR;
				cp->intr_mask = GP_RDYEN1 | GP_EIR1;
				cp->intr_control = GP_ENAB;
				return 1;
			}
		}
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
gpio_dma_isr(sc, cp, ext_intr)
register struct isc_table_type *sc;
register struct GPIO *cp;
register int ext_intr;
{

	dmaend_isr(sc);		/* clear up dma channel */
	cp->intr_control &= ~(GP_DMA0 | GP_DMA1); /* disable DMA for GPIO */
	drop_dma(sc->owner);	/* get rid of channel */

	/* terminate the transfer on EIR? */
	if (ext_intr) {
		if (cp->intr_control & GP_WORD)
			sc->resid += sc->resid;
		cp->intr_mask = 0;	/* disable interrupts */
		cp->intr_control = 0;
		sc->tfr_control = TR_END;
		sc->transfer = NO_TFR;
		gpio_reset(sc);		/* now clear the EIR lint */
		return 0;
	}

	if (sc->owner->b_flags & B_READ) {	/* input transfer */
		/* we must do interrupt transfer for the last byte */
		cp->intr_control = 0;	/* turn off for now */
		sc->transfer = INTR_TFR;
#ifdef	GPIO_DDB
		cp->intr_mask = GP_RDYEN1;		/* enable on ready */
#else	GPIO_DDB
		cp->intr_mask = GP_RDYEN1 | GP_EIR1;	/* enable on ready */
#endif	GPIO_DDB
		cp->intr_control = GP_ENAB;	/* enable the card */
		return 1;	/* finish last next isr */
	}
	if (cp->intr_control & GP_WORD) 	/* adjust resid from dmaend */
		sc->resid += sc->resid;		/* double it */
	sc->tfr_control = TR_COUNT;
	cp->intr_control = 0;	/* disable gpio from dma */
	if (cp->ready & GP_READY) {
		sc->transfer = NO_TFR;
		return 0;
	} else {
		sc->transfer = BO_END_TFR;
		cp->intr_mask = GP_RDYEN1 | GP_EIR1;	/* enable on ready */
		cp->intr_control = GP_ENAB;	/* enable the card */
		return 1;
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
gpio_isr(inf)
struct interrupt *inf;
{
	register struct isc_table_type *sc = (struct isc_table_type *)inf->temp;

	gpio_do_isr(sc);
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
gpio_do_isr(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	int i;
	register int ext_int = 0;
	register struct buf *this_buf, *next_buf;
	register struct dil_info *dil_pkt;
	unsigned char status;

	status = cp->p_status;
#ifndef	GPIO_DDB
	if (status & GP_INT_EIR)	/* external interrupt */
		ext_int= 1;
#endif	not GPIO_DDB

	switch(sc->transfer) {
		case DMA_TFR:
				if (gpio_dma_isr(sc, cp, ext_int)) return;
				(sc->owner->b_action)(sc->owner);
				break;
		case INTR_TFR:
				if (gpio_intr_isr(sc, cp, ext_int)) return;
				(sc->owner->b_action)(sc->owner);
				break;
		case NO_TFR:

		/* did we get an EIR interrupt? */
		if (ext_int) {
			/* service the EIR interrupt */
                        if (sc->intr_wait & INTR_GPIO_RDY)
			    cp->intr_mask = GP_RDYEN1 | GP_EIR0;
                        else
                            cp->intr_mask = 0;

			/* see if anyone was waiting for it */
			if (sc->intr_wait & INTR_GPIO_EIR) {
				cp->intr_control = 0;
				sc->intr_wait &= ~INTR_GPIO_EIR;
				for (this_buf = sc->event_f; this_buf != NULL; 							 this_buf = next_buf) {
					dil_pkt = (struct dil_info *) 
							this_buf->dil_packet;
					next_buf = dil_pkt->dil_forw;
					if (INTR_GPIO_EIR&dil_pkt->intr_wait) {
					       dil_pkt->event |= INTR_GPIO_EIR
							  & dil_pkt->intr_wait;
					       dil_pkt->intr_wait &= 
								~INTR_GPIO_EIR;
					       deliver_dil_interrupt(this_buf);
					}
				}
			}
		}

		/* did we get a READY interrupt? */
		if (cp->ready & GP_READY) {
			/* service the READY interrupt */
                        if (sc->intr_wait & INTR_GPIO_EIR)
			    cp->intr_mask = GP_EIR1;
                        else
                            cp->intr_mask = 0;

			/* see if anyone was waiting for it */
			if (sc->intr_wait & INTR_GPIO_RDY) {
				cp->intr_control = 0;
				sc->intr_wait &= ~INTR_GPIO_RDY;

				for (this_buf = sc->event_f; this_buf != NULL; 
							 this_buf = next_buf) {
					dil_pkt = (struct dil_info *) 
							this_buf->dil_packet;
					next_buf = dil_pkt->dil_forw;
					if (INTR_GPIO_RDY&dil_pkt->intr_wait) {
					       dil_pkt->event |= INTR_GPIO_RDY
							  & dil_pkt->intr_wait;
					       dil_pkt->intr_wait &= 
								~INTR_GPIO_RDY;
					       deliver_dil_interrupt(this_buf);
					}
				}
			}
		}
		break;

		case BO_END_TFR:
			cp->intr_mask = 0;	/* disable interrupts */
			cp->intr_control = 0;
                        sc->transfer = NO_TFR;
			(sc->owner->b_action)(sc->owner);
			break;

		case FHS_TFR:
		case BURST_TFR:
		default:
			panic("Unrecognized GPIO transfer type");
	}
	/* interrupts should be off now.  Only active during transfer */

	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	}
	while (i);
	return;
}


/******************************routine name******************************
** [short description of routine]
** this is used by any transfer routine other than DMA 
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
gpio_resid(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *)sc->card_ptr;

	sc->resid = sc->count;
	if (sc->tfr_control & D_16_BIT)
		sc->resid += sc->resid;
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
gpio_init(sc)
register struct isc_table_type *sc;
{
	struct GPIO *cp = (struct GPIO *)sc->card_ptr;

	/* reset the hardware */
	gpio_reset(sc);

	/* initialize for select code locking */
	sc->locking_pid = 0;
	sc->locks_pending = 0;

	/* install our interrupt service routine */
	isrlink(gpio_isr, sc->int_lvl, &cp->intr_control, 0xc0, 0xc0,
								0, (int)sc);
}


/******************************gpio_reset******************************
** [short description of routine]
**
**	Called by:  dil_gpio routines directly and via iosw 
**
**	Parameters:	sc
**
**	Kernel routines called:	 snooze		
**
**	Global variables/fields referenced: sc->card_ptr
**
**	Registers modified:  reset_gpio
**
**	Algorithm:  [?? if needed]
************************************************************************/
gpio_reset(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	cp->reset_gpio = 1;
	snooze(15);	/* let card settle after reset */
}

/******************************gpio_dil_reset**************************
** [short description of routine]
**
**	Called by:  dil_utility (dil_action) thru iod_dil_reset
**
**	Parameters:	bp
**
**	Kernel routines called:  snooze
**
**	Global variables/fields referenced: b_sc->card_ptr
**
**	Registers modified:  reset_gpio
**
**	This routine could go away and be replaced in the iosw with gpio_reset
** 	if dil_action is fixed so that it can pass a sc instead of a bp.
************************************************************************/
gpio_dil_reset(bp)
register struct buf *bp;
{
	register struct GPIO *cp = (struct GPIO *) bp->b_sc->card_ptr;

	cp->reset_gpio = 1;
	snooze(15);	/* let card settle after reset */
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
gpio_abort(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;

        /*
	 * before we disable the gpio card from interrupts by clearing
	 * the intr_control register, we need to reset the interface 
	 * card by calling dmaend_isr.
	 */
	cp->intr_mask = 0;		/* disable interrupts */
	if (sc->transfer == DMA_TFR) 
	{	dmaend_isr(sc);
		if (cp->intr_control & GP_WORD)
			sc->resid += sc->resid;
		drop_dma(sc->owner);
	} else {
		gpio_resid(sc);		/* find out how much is left */
	}
	cp->intr_control = 0;
	sc->transfer = NO_TFR;
	if (((sc->state & LOCKED) || (sc->locks_pending)) &&
	    (sc->locking_pid != info->last_pid)) {
	    /* Someone else has the card locked - don't reset */
	    }
        else
	   gpio_reset(bp->b_sc);		/* now clear the EIR line */
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
gpio_intr_set(sc, active_waits, request)
register struct isc_table_type *sc;
register int *active_waits;
register int request;
{
	register struct GPIO *cp = (struct GPIO *)sc->card_ptr;

	sc->intr_wait |= request;
	*active_waits = 0;

	if ((request & INTR_GPIO_EIR) &&
	    (request & INTR_GPIO_RDY)) {
		*active_waits = INTR_GPIO_EIR | INTR_GPIO_RDY;
#ifdef	GPIO_DDB
		cp->intr_mask = GP_RDYEN1;
#else	GPIO_DDB
		cp->intr_mask = GP_EIR1 | GP_RDYEN1;
#endif	GPIO_DDB
		cp->intr_control = GP_ENAB;
		return;
	}
	if (request & INTR_GPIO_EIR) {
		*active_waits = INTR_GPIO_EIR;
#ifdef	GPIO_DDB
		cp->intr_mask = 0;
#else	GPIO_DDB
		cp->intr_mask = GP_EIR1;
#endif	GPIO_DDB
		cp->intr_control = GP_ENAB;
	}
	if (request & INTR_GPIO_RDY) {
		*active_waits = INTR_GPIO_RDY;
		cp->intr_mask = GP_RDYEN1;
		cp->intr_control = GP_ENAB;
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
struct dma_chain *gpio_dma_setup(bp, dma_chain, entry)
struct buf *bp;
register struct dma_chain *dma_chain;
int entry;
{
	register struct isc_table_type *sc = bp->b_sc;

	switch (entry) {
	case FIRST_ENTRIES:
		dma_chain->card_byte = (sc->dma_chan->card == (struct dma *) DMA_CHANNEL0) ?
							GP_DMA0 : GP_DMA1;
		if (sc->state & F_WORD_DMA)
			dma_chain->card_byte |= GP_WORD;
		dma_chain->card_byte |= GP_ENAB;
		dma_chain->card_reg = (caddr_t) &((struct GPIO *)sc->card_ptr)->intr_control;
		dma_chain->arm = DMA_ENABLE | DMA_LVL7;
		break;
	case LAST_ENTRY:
		/* 
		** If count == 1 then just format as last transfer.
		** Otherwise make two transfers -- count-1 and
		** the last one.  Gpio reads never let the dma do
		** the last transfer; the processor must do it
		** to avoid having the gpio request an extra transfer.
		*/
		if (bp->b_flags & B_READ) {
			if (dma_chain->count != 1-1) {
				*(dma_chain+1) = *dma_chain;	/* first, just copy */
				dma_chain->count -= 1;	/* and adjust count */
				dma_chain++;			/* point to next one */
				dma_chain->buffer += dma_chain->count << (sc->state & F_WORD_DMA);
				dma_chain->count = 1-1;	/* one transfer */
			}
			dma_chain--;  /* make previous entry the last */
		}
		/*
		** last transfer adjustments
		*/
		dma_chain->card_byte |= GP_ENAB;
		dma_chain->arm -= DMA_LVL7 - ((sc->int_lvl - 3) << 4);	/* drop the dma int level */
		(dma_chain+1)->level = sc->int_lvl;
		break;
	default:
		panic("gpio_dma_setup: unknown dma setup entry");
	}
	return(dma_chain);
}


/*
** linking and inititialization routines
*/

struct drv_table_type gpio_iosw = {
	gpio_init,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_driver,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_set_control,
	gpio_save_state,
	gpio_restore_state,
	gpio_iocheck,
	gpio_dil_reset,
	gpio_status,
	gpio_nop,
	gpio_abort,
	gpio_nop,
	gpio_nop,
	gpio_nop,
	gpio_intr_set,
	gpio_dma_setup,
	gpio_nop,
	gpio_nop,
	gpio_reg_access,
	gpio_nop,
	gpio_nop,
	gpio_nop	/* no action for savecore_xfer() */
};

extern int (*make_entry)();

int (*gpio_saved_make_entry)();


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
gpio_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	if (id != 3)
		return (*gpio_saved_make_entry)(id, isc);

	isc->iosw = &gpio_iosw;
	isc->card_type = HP98622;
	return io_inform("HP98622 GPIO Interface", isc, 3);
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
#ifdef IOSCAN
int gpio_link_called = 0;
#endif /* IOSCAN */
gpio_link()
{
#ifdef IOSCAN
   if (! gpio_link_called) {
#endif /* IOSCAN */
	gpio_saved_make_entry = make_entry;
	make_entry = gpio_make_entry;

#ifdef IOSCAN
        hpib_config_ident = dil_config_identify;
        gpio_link_called = 1;
   }
#endif /* IOSCAN */
}

