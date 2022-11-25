/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ti9914.c,v $
 * $Revision: 1.9.84.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/22 13:16:36 $
 */
/* HPUX_ID: @(#)ti9914.c	55.1		88/12/23 */

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
**  TI9914 driver
*/

#include "../h/param.h"
#include "../h/buf.h"
#include "../s200/vmparam.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/ti9914.h"
#include "../wsio/dil.h"
#include "../wsio/hpib.h"
#include "../wsio/dilio.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../s200io/dma.h"
#include "../wsio/intrpt.h"
#include "../wsio/iobuf.h"
#include "../wsio/tryrec.h"
#include "../h/malloc.h"

int want_ppoll = 0;

/* 
 * Special data for PIL/DIL co-existence.  If PIL is present, PIL may
 * munge the below variables to cause the desired actions.
 */
int TI9914_link_called = 0;     /* set when TI9914_link is called */
int PIL_events = 0;             /* PIL interrupt mask; see ucg_isr */
int (*PIL_dil_isr)() = 0;       /* ptr to PIL isr; see ucg_isr */
int PIL_imask = 0;              /* always enable these unless STATE_SAVED */

extern fhs_out();	/* these three are in ti9914s.s */
extern fhs_in();
extern TI9914_RHDF();
int TI9914_set_addr();
int TI9914_atn_ctl();
int TI9914_isr();
int TI9914_fhs();
int TI9914_dma();
int TI9914_dma_start();
int TI9914_do_isr();
int TI9914_ucg_isr();
int TI9914_dil_isr();

extern char odd_partab[];

#define	BI_DMA_TFR	BURST_TFR
#define MAXPHYS		(64 * 1024)

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (IO_REG_ACCESS) via dil_utility (dil_action)
**		via enqueue (b_action)
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
TI9914_reg_access(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int access_mode = bp->pass_data1;
	register int offset = bp->pass_data2;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register int data;
	register char reg_data;
	int x;

	x = splx(sc->int_lvl);

	if (access_mode & WORD_MODE) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		return;
	}

	if (access_mode & READ_ACCESS) {
		switch(offset) {
		case 1:
			bp->return_data = cp->card_id;
			break;
		case 3:
			bp->return_data = cp->iostatus;
			break;
		case 5:
			bp->return_data = cp->extstatus;
			break;
		case 7:
			bp->return_data = cp->ppoll_int;
			break;
		case 9:
			bp->return_data = cp->ppoll_mask;
			break;
		case 17:
			reg_data = cp->intstat0;
			sc->intcopy |= reg_data;
			bp->return_data = reg_data;
			break;
		case 19:
			reg_data = cp->intstat1;
			sc->intcopy |= (reg_data << 8);
			bp->return_data = reg_data;
			break;
		case 21:
			bp->return_data = cp->addrstate;
			break;
		case 23:
			bp->return_data = cp->busstat;
			break;
		case 25:
			bp->return_data = cp->address_reg;
			break;
		case 27:
			bp->return_data = cp->spoll;
			break;
		case 29:
			bp->return_data = cp->cmdpass;
			break;
		case 31:
			bp->return_data = cp->datain;
			break;
		default:
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
		}
	} else {
		data = info->register_data;
		switch(offset) {
		case 1:
			/* If it's the internal HP-IB, don't write
			   the reset register due to bug where writing
			   this reg on the internal can glitch the DMA
			   lines on the backplane!!  Since writing this
			   reg is theoretically a nop, just return as if
			   we had written it.
			*/
 			if (sc->card_type != INTERNAL_HPIB) 
				cp->card_id = data & 0xff;
			break;
		case 3:
			cp->iocontrol = data & 0xff;
			break;
		case 5:
			cp->extstatus = data & 0xff;
			break;
		case 7:
			cp->ppoll_int = data & 0xff;
			break;
		case 9:
			cp->ppoll_mask = data & 0xff;
			break;
		case 17:
			cp->intmask0 = data & 0xff;
			break;
		case 19:
			cp->intmask1 = data & 0xff;
			break;
		case 21:
			cp->addrstate = data & 0xff;
			break;
		case 23:
			cp->auxcmd = data & 0xff;
			break;
		case 25:
			cp->address_reg = data & 0xff;
			break;
		case 27:
			cp->spoll = data & 0xff;
			break;
		case 29:
			cp->ppoll = data & 0xff;
			break;
		case 31:
			cp->dataout = data & 0xff;
			break;
		default:
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
		}
	}
	splx(x);
}

/******************************routine name******************************
** [short description of routine]
** Set the HPIB card to a known reset state
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
TI9914_init(sc)
struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	if (sc->state & BOBCAT_HPIB)
		isrlink(TI9914_isr, sc->int_lvl, &cp->ppoll_int,
						0xc0, 0xc0, 0, (int *)sc);
	sc->locking_pid = 0;
	sc->locks_pending = 0;
	isrlink(TI9914_isr, sc->int_lvl, &cp->iostatus,
						0xc0, 0xc0, 0, (int *)sc);

	TI9914_hw_clear(sc);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  TI9914_init, TI9914_dil_reset,
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
TI9914_hw_clear(sc)
struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	/* see if we are system controller */
	if ((cp->extstatus & 0x80) == 0)
		sc->state &= ~SYSTEM_CONTROLLER;
	else
		sc->state |= SYSTEM_CONTROLLER;

	/* see if we are active controller */
	if (cp->extstatus & 0x40)
		sc->state &= ~ACTIVE_CONTROLLER;
	else
		sc->state |= ACTIVE_CONTROLLER;

	/* set REN false     */
	cp->auxcmd = H_SRE0;

	TI9914_reset(sc, cp);	/* reset the card */

	if (sc->state & ACTIVE_CONTROLLER)
		cp->auxcmd = H_RQC;

	/* clear ppoll mask */
	if (sc->state & BOBCAT_HPIB) {
		cp->ppoll_int = 0;
		cp->ppoll_mask = 0;
	}
	sc->ppoll_flag = 0;
	sc->ppoll_f = NULL;
	sc->event_f = NULL;
	sc->ppoll_l = NULL;
	sc->event_l = NULL;
	sc->ppoll_mask = 0;
	sc->ppoll_sense = 0;
	sc->ppoll_resp = 0;
	sc->intmsksav = 0;
	sc->intmskcpy = 0;
	sc->state &= ~IST;
	sc->transfer = NO_TFR;
	cp->iocontrol = TI_ENI;		/* card enable for external cards */

	if (sc->state & SYSTEM_CONTROLLER) {
		cp->auxcmd = H_TCA;	/* Yank on IFC for ~100 micro seconds */
		do_ti9914_ifc(cp);
	}

	/* setup the interrupt mask (and copies) */
	cp->auxcmd = H_DAI1;
	TI9914_ruptmask(sc, TI_S_UCG, INT_ON);
	cp->auxcmd = H_DAI0;
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

TI9914_reset(sc, cp)
register struct isc_table_type *sc;
register struct TI9914 *cp;
{
	unsigned char card_address;
	int dummy;  /* used to clear datain register, data never used */

	cp->auxcmd = H_SWRST1; /* start software reset */

	/* set T1 delay: 1200 nsec for 9914, 600 nsec for 9914A */
	cp->auxcmd = H_STDL1;
	cp->auxcmd = H_VSTDL1;

	/* clear both interrupt mask registers */
	cp->intmask0 = 0;
	cp->intmask1 = 0;
	/* clear the dma bits */
	cp->iostatus &= ~(TI_DMA_0 | TI_DMA_1);

	/* determine card address */
	if ((char *)sc->card_ptr == (char *) INTERNAL_CARD) {
		if (sc->state & SYSTEM_CONTROLLER)
			card_address = 21;
		else
			card_address = 20;
	} else	 /* read card address */
		card_address = cp->extstatus & 0x1f;	/* want lower 5 bits */
	if (card_address == 31) 
		card_address = 0;
	/* let card know its address */
	cp->address_reg = card_address;
	sc->my_address = card_address;

	/* keep track of holdoff state */
	sc->state &= ~IN_HOLDOFF;

	/* save a bo */
	sc->intcopy = TI_M_BO;

	cp->auxcmd = H_HDFA1;	/* set holdoff on all */
	cp->auxcmd = H_HDFE0;	/* clear holdoff on end */
	cp->auxcmd = H_RPP0;	/* clear ppoll response */
	sc->ppoll_flag = 0;
	cp->auxcmd = H_DAI0;	/* enable interrupts */
	cp->auxcmd = H_SHDW0;	/* disable shadow handshake */
	cp->spoll = 0;		/* clear spoll response */
	cp->ppoll = 0;		/* unconfigure ppoll response */
	cp->auxcmd = H_SWRST0; 	/* end software reset */
	dummy = cp->datain;		/* throw away any old data */
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

TI9914_identify(bp)
register struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;
	struct iobuf *iob = bp->b_queue;

	TI9914_save_state(sc);
	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = UNL;

	TI9914_bo(bp);
	cp->dataout = odd_partab[LAG_BASE + sc->my_address];
	cp->auxcmd = H_LON1;

	TI9914_bo(bp);
	cp->dataout = odd_partab[TAG_BASE + 31];

	TI9914_bo(bp);
	snooze(65);		/* delay for chinook (>55 us) */
	cp->dataout = odd_partab[SCG_BASE + bp->b_ba];

	TI9914_bo(bp);
	snooze(65);		/* delay again for chinook */
	cp->auxcmd = H_HDFA0;
	cp->auxcmd = H_HDFE1;
	TI9914_RHDF(sc);
	cp->auxcmd = H_GTS;

	TI9914_bi(bp);
	iob->b_xaddr[0] = cp->datain;

	if (sc->intcopy & TI_M_END) {	/* premature EOI */
		cp->auxcmd = H_TCA;	/* Yank on IFC for ~100 micro seconds */
		do_ti9914_ifc(cp);
		cp->auxcmd = H_HDFA1;		/* set holdoff */
		cp->auxcmd = H_HDFE0;
		cp->dataout = odd_partab[TAG_BASE + sc->my_address];
		sc->intcopy &= ~TI_M_END;
		bp->b_error = EIO;
		TI9914_restore_state(sc);
		escape(1);
	}

	TI9914_bi(bp);
	iob->b_xaddr[1] = cp->datain;

	sc->intcopy &= ~TI_M_END;
	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = odd_partab[TAG_BASE + sc->my_address];

	TI9914_bo(bp);
	cp->auxcmd = H_HDFA1;		/* set holdoff */
	cp->auxcmd = H_HDFE0;
	sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
	TI9914_restore_state(sc);
	return 0; 	/* no parallel poll */
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
TI9914_clear_wopp(bp)
struct buf *bp;
{
	TI9914_clear(bp);
	return 0;	/* no parallel poll */
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
TI9914_clear(bp)
struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;

	TI9914_save_state(sc);
	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = UNL;

	TI9914_bo(bp);
	cp->dataout = odd_partab[TAG_BASE + sc->my_address];
	cp->auxcmd = H_TON1;

	TI9914_bo(bp);
	cp->dataout = odd_partab[LAG_BASE + bp->b_ba];

	TI9914_bo(bp);
	cp->dataout = odd_partab[SCG_BASE + 16];	/* amigo clear */

	TI9914_bo(bp);
	cp->auxcmd = H_GTS;	/* off ATN */
	cp->auxcmd = H_FEOI;	/* assert EOI with this byte */
	cp->dataout = 0;
	TI9914_bo(bp); /* wait until we get this byte out */

	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = SDC;

	TI9914_bo(bp);
	cp->dataout = UNL;

	TI9914_bo(bp);
	sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
	TI9914_restore_state(sc);
	snooze(100);	/* so chinook won't hang */
	return 1;	/* do a parallel poll */
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
TI9914_abort(bp)
struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	int x,i, none_found = 1;

	if (sc->owner != bp)
		return;

	cp->auxcmd =H_DAI1;
	switch (sc->transfer){
	case DMA_TFR:
			dma_isr(sc, cp);
			/* dma_isr may leave a BO interrupt pending */
	case BO_END_TFR:
			TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
			break;
	case FHS_TFR:
			sc->state &= ~(FHS_ACTIVE | FHS_TIMED_OUT);
			
			/* Note: it may be possible to remove this resid update */
			sc->resid = sc->count;
			break;
	case INTR_TFR:
			intr_end(sc, 0);
			break;
	case NO_TFR:
			break;
	case BUS_TRANSACTION:
			if (sc->state & WAIT_BO) {
				sc->state &= ~WAIT_BO;
				TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
			}
			if (sc->state & WAIT_BI) {
				sc->state &= ~WAIT_BI;
				TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
			}
			break;
	case BI_DMA_TFR:
			TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
			break;
	default:
			panic("ti9914: Unrecognized TI9914 abort type");
	}
	sc->transfer = NO_TFR;

	TI9914_reset(sc, cp);		/* reset the card */
	do_ti9914_ifc(cp);		/* clear it */
	cp->auxcmd = H_TCA;	/* Yank on IFC for ~100 micro seconds */
	if (sc->state & STATE_SAVED) 
		TI9914_restore_state(sc);
	else {
		sc->intmsksav = 0;
		sc->intmskcpy = 0;
		TI9914_ruptmask(sc, TI_S_UCG, INT_ON);	/* restore interrupts */
		cp->auxcmd = H_DAI0;		/* enable interrupts */
		/* check for a parallel poll on anyone else */

		/* if someone is waiting for a ppoll on this interface... */
		if (sc->ppoll_f || (sc->intr_wait & INTR_PPOLL)) {
			if (sc->ppoll_flag++ == 0) {  /* start a ppoll */
				TI9914_atn(sc);
				TI9914_bo(bp);
				sc->intcopy |= TI_M_BO;
				cp->auxcmd = H_RPP1;
				snooze(25);	/* wait 25 usecs */
			}
			if ((sc->state & BOBCAT_HPIB) && (sc->state & DO_BOBCAT_PPOLL)) 
				cp->ppoll_int |= 0x80;	/* enable ppoll intr */
			else {
				/* sneak in a ppoll check for this select code */
				x = spl6();
				if (TI9914_ppoll(sc))
					/* we have a match */
					ppoll_TI9914_fakeisr(sc);
				splx(x);
			}
		/* else, noone is waiting for ppoll on this interface */
		}  else 
		        TI9914_ppoll_off(sc);
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
TI9914_mesg(bp,type,sec,bfr,count)
register struct buf *bp;
char type,sec;
register char *bfr;
register short count;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;
	register int temp;

	TI9914_save_state(sc);
	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = UNL;

	TI9914_bo(bp);
	if (type & T_WRITE) {
		cp->dataout = odd_partab[TAG_BASE + sc->my_address];
		cp->auxcmd = H_TON1; /* I'm a talker */

		TI9914_bo(bp);
		cp->dataout = odd_partab[LAG_BASE + bp->b_ba];
	}
	else {
		cp->dataout = odd_partab[LAG_BASE + sc->my_address];
		cp->auxcmd = H_LON1; /* I'm a listner */

		TI9914_bo(bp);
		cp->dataout = odd_partab[TAG_BASE + bp->b_ba];
	}

	if (sec) {
		TI9914_bo(bp);
		snooze(65);		/* yes another delay for chinook */
		cp->dataout = odd_partab[sec];
	}

	TI9914_bo(bp);
	temp = count;
	if (count) {
		if (type & T_WRITE) {
			cp->auxcmd = H_GTS;	/* off ATN */
			while (--temp) {
				cp->dataout = *bfr++;  /* write out byte */
				TI9914_bo(bp);
			}
			if (type & T_EOI) cp->auxcmd = H_FEOI; /* set EOI */
			cp->dataout = *bfr;
			TI9914_bo(bp); /* wait until the last byte is sent */
		}
		else {
			snooze(65);		/* for Chinook's sake */
			cp->auxcmd = H_HDFA0;	/* off holdoff */
			cp->auxcmd = H_HDFE1;	/* holdoff on end */
			TI9914_RHDF(sc);
			cp->auxcmd = H_GTS;
			while (--temp) {	/* loop for n-1 bytes */ 
				TI9914_bi(bp);
				if (sc->intcopy & TI_M_END) {
					if (type & T_EOI) {
						/* make sure the bi completes */
						sc->intcopy |= TI_M_BI;
						break;
					} else {
						bp->b_error = EIO;
						escape(1);
					}
				}
				*bfr++ = cp->datain;
			}
			cp->auxcmd = H_HDFA1;		/* set holdoff */
			cp->auxcmd = H_HDFE0;		/* clear end holdoff */

			TI9914_bi(bp);
			*bfr = cp->datain;		/* get last byte */

			sc->intcopy &= ~TI_M_END;	/* clear EOI flag */
			count = count - temp;
		}
	}

	TI9914_atn(sc);

	TI9914_bo(bp);
	if (type & T_WRITE) {
		/* workaround for transparent message bug CS80.
		** may be fixed in future disks. see simon_mesg
		*/
		if ((sec & 0177) == 0162) snooze(150);
		cp->dataout = UNL;
	}
	else
		cp->dataout = UNT;

	TI9914_bo(bp);
	sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
	TI9914_restore_state(sc);

	return count;
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
TI9914_save_state(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	if (sc->state & BOBCAT_HPIB) {
		cp->ppoll_int = 0;	/* disable ppoll intr */
	}
	cp->auxcmd = H_RPP0;
	sc->ppoll_flag = 0;
	sc->intmskcpy = sc->intmsksav;

	sc->state |= STATE_SAVED;	/* keep track of current state */
	TI9914_ruptmask(sc, sc->intmsksav, INT_OFF);	/* off interrupts */
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  TI9914_postamb, 
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
TI9914_restore_state(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	/* we will asume that attention is already set and that
	** a previous byte going out have completed the handshake
	*/

	sc->state &= ~STATE_SAVED;	/* keep track of current state */

	/* restore interrupts */
	TI9914_ruptmask(sc, sc->intmskcpy | TI_S_UCG, INT_ON);
	TI9914_restore_ppoll_state(sc);
}


/******************************TI9914_restore_ppoll_state****************
** [short description of routine]
**
**	Called by:  TI9914_restore, TI9914_ucg_isr
**
**	Parameters: turn parallel poll interrupts back on
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
TI9914_restore_ppoll_state(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	int x;

	if (sc->state & ACTIVE_CONTROLLER) {
		/* if someone was waiting for ppoll on this interface,
		   we'll turn ppoll's back on
		*/
		if (sc->ppoll_f || (sc->intr_wait & INTR_PPOLL)) {
			if (sc->ppoll_flag++ == 0) {
				TI9914_atn(sc);
				TI9914_bo(sc->owner);
				sc->intcopy |= TI_M_BO;
				cp->auxcmd = H_RPP1;
				snooze(25);	/* wait 25 usecs */
			}
			if ((sc->state & BOBCAT_HPIB) && (sc->state & DO_BOBCAT_PPOLL)) 
				cp->ppoll_int |= 0x80;	/* enable ppoll intr */
			else {   /* sneak in a ppoll check for this select code */
				x = spl6();
				if (TI9914_ppoll(sc)) 
					/* we have a match */
					ppoll_TI9914_fakeisr(sc);
				splx(x);
			}
		/* else, noone was waiting for ppolls on this interface */
		} else 
		        TI9914_ppoll_off(sc);
	}	/* end if active controller */
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
**	 for all select codes:
**   		1) if it is busy then ignore it for now
**   		2) if it is not if a parallel poll state then forget it
**   		3) if the state is saved then forget it
**   		4) if we have a match then we need to fake an interrupt
************************************************************************/
TI9914_ppoll_intr()
{
	register struct isc_table_type *sc;
	register int i;
	register int temp_want_ppoll = want_ppoll;

	/* if nobody wants ppoll's anymore turn off this polling routine */
	if (want_ppoll == 0)
		return;

	i = 0;
	while (temp_want_ppoll) {  /* while there are bits set */
		if (temp_want_ppoll & (1 << i)) {  /* if this sc is waiting for ppoll */
			temp_want_ppoll &= ~(1 << i);
			sc = isc_table[i];
			if ((sc->active == 0)  			      && 
		     	(sc->ppoll_f || (sc->intr_wait & INTR_PPOLL)) && 
                     	(sc->ppoll_flag)                              &&
		     	(!((sc->state & BOBCAT_HPIB)                  && 
                     	(sc->state & DO_BOBCAT_PPOLL)))) {
				if (TI9914_ppoll(sc))
					ppoll_TI9914_fakeisr(sc); /* we have a match */
			}
		}
		i++;  /* check next sc */
	}
	timeout(TI9914_ppoll_intr, 1, 1, NULL);
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
TI9914_pplset(sc, mask, sense, enab)
register struct isc_table_type *sc;
register unsigned char mask;
int sense,	/* Sense of bit to look for */
    enab;	/* Is this an interupt ppoll */
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	int x;  

	sc->ppoll_mask |= mask;
	if (sc->owner->b_flags & B_DIL)
		sc->ppoll_sense = (sc->ppoll_sense & (~mask)) | (sense & mask);
	else 
		if (sense == 1)
			sc->ppoll_sense &= ~mask;
		else
			sc->ppoll_sense |= mask;

	/* if this interface has ppoll interrupt hardware, we will
	   use it iff this request is for regular sense.  Since the
	   hardware has no sense register, it won't work for inverse
	   sense.
	*/
	if ((!(sc->ppoll_sense)) && (sc->state & BOBCAT_HPIB))
		sc->state |= DO_BOBCAT_PPOLL;
	else
		sc->state &= ~DO_BOBCAT_PPOLL;

	/* Start the ppoll */
	if (sc->ppoll_flag++ == 0) {
		TI9914_atn(sc);
		TI9914_bo(sc->owner);
		sc->intcopy |= TI_M_BO;
		cp->auxcmd = H_RPP1;
		snooze(25);		/* 25 us delay */
	}
	/* set system ppoll interrupt flag */
	if (enab) {
		if ((sc->state & BOBCAT_HPIB) && (sc->state & DO_BOBCAT_PPOLL)) {
			cp->ppoll_mask = sc->ppoll_mask;
			cp->ppoll_int |= 0x80;
		}
		else {
			x = spl6();
			/* if first ppoll request, start clock to fake it */
			if ( ! want_ppoll) 
				timeout(TI9914_ppoll_intr, 1, 1, NULL);
			want_ppoll |= (1 << sc->my_isc);	/* bit field to do timeouts */
			splx(x);
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
TI9914_pplclr(sc, mask)
register struct isc_table_type *sc;
register unsigned char mask;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register struct buf *cur_buf, *next_buf;
	register struct dil_info *dil_info;
	register int ppoll_rupts = 0;
	int x;

	if ((sc->ppoll_mask & mask) == 0) return;	/* not on list */

	x = spl6();
	/* recompute the cumulative select code mask */
	sc->ppoll_mask = 0;
	for (cur_buf = sc->ppoll_f; cur_buf != NULL; cur_buf = next_buf) {
		next_buf = cur_buf->av_forw;
		if (cur_buf->b_flags & B_DIL)
			sc->ppoll_mask |= cur_buf->b_ba;
		else
			sc->ppoll_mask |= 0x80 >> cur_buf->b_ba;
	}
	for (cur_buf = sc->event_f; cur_buf != NULL; cur_buf = next_buf) {
		dil_info = (struct dil_info *) cur_buf->dil_packet;
		next_buf = dil_info->dil_forw;
		if (dil_info->intr_wait & INTR_PPOLL) {
			sc->ppoll_mask |= dil_info->ppl_mask;
			ppoll_rupts = 1;
		}
	}

	if (!ppoll_rupts)
		sc->intr_wait &= ~INTR_PPOLL;

	if ((sc->state & BOBCAT_HPIB) && (sc->state & DO_BOBCAT_PPOLL)) 
		cp->ppoll_mask = sc->ppoll_mask;

	if ((sc->ppoll_f == NULL) && ((sc->intr_wait & INTR_PPOLL) == 0)) 
		/* no processes are waiting for ppoll on this selcode */
	        TI9914_ppoll_off(sc);
	splx(x);
}

/******************************TI9914_ppoll_off******************************
** Turn parallel poll interrupts off.  You should have active control.
**
**	Called by:  TI9914_send_cmd, TI9914_restore_ppoll_state
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
TI9914_ppoll_off(sc)
register struct isc_table_type *sc;

{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

        if (sc->ppoll_flag) {
	        if (sc->state & BOBCAT_HPIB)
		    cp->ppoll_int = 0;	/* turn off ppoll intr */
		cp->auxcmd = H_RPP0;	/* off ppoll -- need active control */
		sc->ppoll_flag = 0;
		want_ppoll &= ~(1 << sc->my_isc);
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
TI9914_ppoll(sc)
struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	return ((cp->cmdpass ^ sc->ppoll_sense) & sc->ppoll_mask);
}


/*****************************TI9914_spoll****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SPOLL) via dil_util (dil_action)
**		via enqueue (b_action)
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
**  Comment:
**	TI9914_spoll only needs bp to pass to TI9914_bo_intr and 
**	TI9914_bi_intr (asm routines), is it needed?  Can we get rid 
**	of bp as a parameter to this routine?
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_spoll(bp)
struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;
	long flags = bp->b_queue->b_flags;

enum spoll_transaction_state { set_atn = 0,
                               send_unl,
                               send_mla,
                               send_dta,
                               send_spe,
                               drop_atn,
                               get_spoll_byte,
                               send_spd,
                               send_unt,
                               spoll_end,
			       resend_atn,
			       resend_spd,
			       resend_unt,
			       resend_end,
                               spoll_default};

	if (sc->transfer != BUS_TRANSACTION) {
		TI9914_save_state(sc);
		sc->transfer = BUS_TRANSACTION;
		sc->transaction_state = (int) set_atn;
		sc->transaction_proc = TI9914_spoll;
	}

	switch (sc->transaction_state) {
	case set_atn:
		sc->transaction_state = (int) send_unl;
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp))
			break;
	case send_unl:
		sc->transaction_state = (int) send_mla;
		cp->dataout = UNL;
		if (!TI9914_bo_intr(bp))
			break;
	case send_mla:
		sc->transaction_state = (int) send_dta;
		if (flags & PARITY_CONTROL)
			cp->dataout = odd_partab[LAG_BASE + sc->my_address];
		else
			cp->dataout = LAG_BASE + sc->my_address;
		cp->auxcmd = H_LON1;
		if (!TI9914_bo_intr(bp))
			break;
	case send_dta:
		sc->transaction_state = (int) send_spe;
		if (flags & PARITY_CONTROL)
			cp->dataout = odd_partab[TAG_BASE + bp->b_ba];
		else
			cp->dataout = TAG_BASE + bp->b_ba;
		if (!TI9914_bo_intr(bp))
			break;
	case send_spe:
		sc->transaction_state = (int) drop_atn;
		cp->dataout = SPE;	/* enable serial poll */
		if (!TI9914_bo_intr(bp))
			break;
	case drop_atn:
		sc->transaction_state = (int) get_spoll_byte;
		TI9914_RHDF(sc);
		cp->auxcmd = H_GTS;	/* off ATN */
		if (!TI9914_bi_intr(bp))
			break;
	case get_spoll_byte:
		sc->transaction_state = (int) send_spd;
		sc->spoll_byte = cp->datain;	/* get the byte */
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp)) {
			break;
		}
	case send_spd:
		sc->transaction_state = (int) send_unt;
		cp->dataout = SPD;
		if (!TI9914_bo_intr(bp)) {
			break;
		}
	case send_unt:
		sc->transaction_state = (int) spoll_end;
		cp->dataout = UNT;
		if (!TI9914_bo_intr(bp))
			break;
	case spoll_end:
		sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
		sc->transfer = NO_TFR;
		TI9914_restore_state(sc);
		bp->return_data = sc->spoll_byte;
		sc->transaction_state = (int) spoll_default;
		return(1);
	case resend_atn:
		sc->transaction_state = (int) resend_spd;
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp)) {
			break;
		}
	case resend_spd:
		sc->transaction_state = (int) resend_unt;
		cp->dataout = SPD;
		if (!TI9914_bo_intr(bp)) {
			break;
		}
	case resend_unt:
		sc->transaction_state = (int) resend_end;
		cp->dataout = UNT;
		if (!TI9914_bo_intr(bp)) {
			break;
		}
	case resend_end:
		 /* sc->intcopy |= TI_M_BO; */ /* yes we already got a BO */
		sc->transaction_state = (int) 0;
	        return(0);
	default:
		panic("ti9914: bad ti9914 spoll transaction state");
	}
	return(0);
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:   TI9914_postamb, TI9914_atn_ctl,
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
TI9914_atn(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	/* if ATN is already set then we are done */
	if (cp->addrstate & TI_A_ATN) {
		/* make sure the caller gets a byte out */
		sc->intcopy |= TI_M_BO;
		return;
	}
		
	sc->intcopy &= ~TI_M_BO;

	if ((cp->addrstate & TI_A_LADS) && (sc->state & IN_HOLDOFF)) {
		cp->auxcmd = H_TCS;
		return;
	}
	cp->auxcmd = H_TCA;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  bo_intr, bi_intr,.....
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
TI9914_ruptmask(sc, mask, int_flag)
register struct isc_table_type *sc;
register unsigned short mask;
register int int_flag;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int x;

	union {
		struct {
			unsigned char intstat0;
			unsigned char intstat1;
		} byte;
		unsigned short status_mask;
	} ruptmask;

	x = spl6();
	if (int_flag)
		sc->intmsksav |= mask;
	else
		sc->intmsksav &= ~mask;

        /*
         * PIL wants to leave certain interrupt enabled except when a 
         * save_state has occurred (otherwise DIL can panic).  Thus we
         * only add the PIL bits when we are not in STATE_SAVED.  By default,
         * PIL_imask contains nothing.
         */
        if (sc->state & STATE_SAVED)
                ruptmask.status_mask = TI9914_IMASK | sc->intmsksav;
        else
                ruptmask.status_mask = TI9914_IMASK | PIL_imask | sc->intmsksav;

	cp->intmask0 = ruptmask.byte.intstat0;
	cp->intmask1 = ruptmask.byte.intstat1;
	splx(x);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:  cp
**
**	Kernel routines called:  snooze
**
**	Registers modified:  auxcmd
**
************************************************************************/
do_ti9914_ifc(cp)
register struct TI9914 *cp;
{
	/* send IFC for 100 ms. */
	cp->auxcmd = H_SIC1;
	snooze(100);	/* allow card to settle after reset */
	cp->auxcmd = H_SIC0;

	/* make sure ren is asserted */
	cp->auxcmd = H_SRE1;
}


/************************************************************************
** TI9914_transfer assumes that the hpib
** is addressed properly for the transfer.
** Should have been done by the higher level
** device driver.
** NOTE: transfers can only terminate for the
**       following reasons:
**		1) count satisfied (input/output)
**		2) EOI seen (input only)
**		3) timeout expired (input/output) (enforced at higher level)
**
** 'TI9914_preamb' leaves the bus in hold off on all
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
**  byte-in interrupt transfer algo
**  ---------------------------------------------------------
**  The byte-in (BI) interrupt indicates that the card has a byte in
**  its data-in register and is used for reads.  This routine sets
**  things up to get the transfer started and then returns.  When 
**  a byte becomes available, a byte-in interrupt is generated and
**  control is transfered to the isr (TI9914_do_isr).  The isr processes
**  the new data byte by calling 'intr_isr' which also sets up for the
**  next byte to be transfered.  When the last data byte is read, the
**  routine 'intr_end' is called (from intr_isr) which completes special 
**  processing for the last byte red, and the isr terminates the transfer.  
**  See comments for these other routines for more details.
**
**  byte-out interrupt transfer algo
**  ---------------------------------------------------------
**  The byte-out interrupt indicates that the card's data-out register is 
**  empty and ready for another byte.  Since we are starting the write,
**  we turn the TI_S_BO flag on in our s/w interrupt status register and
**  call intr_isr to get things started (we have to start the write to get
**  the interrupt process started).  Intr_isr will examine the s/w interrupt
**  status register, and seeing the BO is set, will put another byte in 
**  the data-out register, thus clearing the BO condtion on the card.  
**  After this, the process continues similar to the byte-in interrupt 
**  transfer algo.  
**
************************************************************************/
TI9914_transfer(transfer_request, bp, proc)
enum transfer_request_type transfer_request;
register struct buf *bp;
int (*proc)();
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	int x;

	
       /* 
	* Sanity check; we set sc->transfer inside this routine and
	* set it back to NO_TFR at the end of the transfer.  So if
	* we enter this routine and and sc->transfer is not set to
	* NO_TFR, we probably have an FSM error somewhere.
	*/
	if (sc->transfer != NO_TFR) {
		panic("ti9914: reentered transfer");
	}
	if (sc->owner != bp)
		panic("ti9914: transfer called when not owned!");

	bp->b_action = proc;    /* call this proc when transfer is complete */

        /* set sc->transfer to values the interface driver understands */
	switch(transfer_request) {
		case MUST_FHS:
			sc->transfer = FHS_TFR;
			break;
		case MUST_INTR:
			sc->transfer = INTR_TFR;
			break;
		case MAX_OVERLAP:
		case MAX_SPEED:

			/*
			** If this is a read and we already have byte in but
			** we are no longer addressed to listen then someone
			** addressed us to listen, sent us one byte and then
			** unaddressed us.  We know there is only one byte for
			** this transaction so no sense using DMA.
			*/
			if ((bp->b_flags & B_READ) &&
			    (sc->intcopy & TI_M_BI) &&
			    ((cp->addrstate & TI_A_LADS) == 0)) {
				sc->transfer = INTR_TFR;
				break;
			}

			if (try_dma(sc, transfer_request))
				sc->transfer = DMA_TFR;
			else {		/* failed to get dma channel */
				switch(transfer_request) {
					case MAX_OVERLAP:
						sc->transfer = INTR_TFR;
						break;
					case MAX_SPEED:
						sc->transfer = FHS_TFR;
						break;
				}
			}
			break;
		default:
			panic("ti9914: Bad transfer request");
	}

	/* set up and initiate the transfer */
	switch(sc->transfer) {

	        /* use byte-in/byte-out interrupts */
		case INTR_TFR:
			/* put info from iobuf into sc struct */
			sc->buffer = bp->b_queue->b_xaddr;
			sc->count = bp->b_queue->b_xcount;

                        /* If dma is reserved and we're here, we decided
                           not to use it, so give it back */
                        if (bp->b_queue->b_flags & RESERVED_DMA)
                                drop_dma(sc->owner);

			if (bp->b_flags & B_READ) {
			        /* disable interrupts */
				cp->auxcmd = H_DAI1;   
				
			        /* holdoff on all bytes */
				cp->auxcmd = H_HDFA1;  /* holdoff on all on  */

				/* no need to holdoff on end (ie EOI) */
				cp->auxcmd = H_HDFE0;  /* holdoff on end off */

			       /* 
				* This routine releases any holdoff so that
				* the read may proceed.  The current holdoff
				* state is maintained in s/w thru the
				* use of sc->state.
				*/
				TI9914_RHDF(sc);   

			        /* byte-in interrupt on in mask */
				TI9914_ruptmask(sc, TI_S_BI, INT_ON);

				/* drop ATN */
				cp->auxcmd = H_GTS;              
				
				/* if we already have a byte, fake interrupt */
				if (sc->intcopy & TI_M_BI)
					TI9914_fakeisr(sc);

				/* enable interrupts; we'll interrupt on BI */
				cp->auxcmd = H_DAI0;
			}
			else {   /* WRITE */
			        /* turn off interrupts; drop ATN */
				cp->auxcmd = H_DAI1;
				cp->auxcmd = H_GTS;

			       /* 
				* BO is added here so the intr_isr will see it.
				* We don't have to worry about this because we
				* are the talker and the first BO is a 
				* don't care.
				*/
				sc->intcopy |= (cp->intstat0 | TI_M_BO);

				/* byte-out interrupt on in mask */
				TI9914_ruptmask(sc, TI_S_BO, INT_ON);

				intr_isr(sc);	/* send first byte */

				/* enable interrupts; we'll interrupt on BO */
				cp->auxcmd = H_DAI0;
			}
			break;
			
		/* fast-handshake transfer */
		case FHS_TFR:
			/* set up transfer information */
			sc->buffer = bp->b_queue->b_xaddr;
			sc->count = bp->b_queue->b_xcount;

			x = spl6();
			sw_trigger (&sc->intloc1,TI9914_fhs, bp, 0, sc->int_lvl);
			sc->state |= FHS_ACTIVE;
			splx(x);

			break;

	        /* dma transfer */
		case DMA_TFR:
			TI9914_dma(sc, bp->b_flags);
			break;

		/* illegal sc->transfer values */
		case BURST_TFR:
		case NO_TFR:
		default:
			panic("ti9914: transfer of unknown type");
	}
}

/****************************ppoll_do_isr****************************
** [short description of routine]
**
**	Called by:  This routine is software triggered by ppoll_TI9914_fakeisr
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
**  Don't ever try to combine this routine and ppoll_TI9914_fakeisr into
**  one routine.  The order and timing of events is critical and very
**  touchy!
************************************************************************/
ppoll_do_isr(sc)
register struct isc_table_type *sc;
{
	sc->int_flags |= INT_PPL;
	TI9914_do_isr(sc);
}
/***************************ppoll_TI9914_fakeisr***********************
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
**  Don't ever try to combine this routine and ppoll_do_isr into
**  one routine.  The order and timing of events is critical and very
**  touchy!
************************************************************************/
ppoll_TI9914_fakeisr(sc)
register struct isc_table_type *sc;
{
	int x;

	x = spl6();
	sw_trigger(&sc->intloc2, ppoll_do_isr, sc, sc->int_lvl, 0);
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
TI9914_fakeisr(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int x;

	x = spl6();
	sc->intcopy = ((cp->intstat0) | (cp->intstat1 << 8) | (sc->intcopy)) &
			~(TI_M_INT0 | TI_M_INT1);	/* clear int flags */
	sw_trigger(&sc->intloc, TI9914_do_isr, sc, sc->int_lvl, 0);
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
TI9914_isr(inf)
struct interrupt *inf;
{
	register struct isc_table_type *sc = (struct isc_table_type *)inf->temp;

	TI9914_do_isr(sc);
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
**  interrupt transfer notes
**  ----------------------------------------------
**  If we get a byte-in or byte-out interrupt (and we're doing a 
**  transfer), 'intr_isr' is called to process the byte.  Intr_isr's 
**  return value indicates whether or not the transfer should continue.
**
**  special transfer state for writes (BO_END_TFR)
**  ----------------------------------------------
**  For writes, a special transfer state (BO_END_TFR) is used to indicate
**  to the isr that a transfer has terminated; that is, for writes, intr_isr
**  never returns 0 (don't continue).  This reason for this is that after
**  we write the last byte to card's data-out register, we need to wait
**  for the final BO interrupt to signal that the byte has been transfered.
**  If intr_isr returned 1, the isr would continue as though the transfer
**  completed (when it really hasn't) and would later receive a BO interrupt
**  which it did not expect.  
**  
************************************************************************/
TI9914_do_isr(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int i;
	register int cont_transfer = 0;
	int this_event;
	register struct dma_channel *ch = sc->dma_chan;
	int dma_interrupted;
	extern unsigned short dma_chan0_last_arm;
	extern unsigned short dma_chan1_last_arm;


	cp->auxcmd = H_DAI1;		/* disable interrupts */

	if ((sc->state & BOBCAT_HPIB) && (sc->state & DO_BOBCAT_PPOLL)) {
		if ((cp->ppoll_int & 0xc0) == 0xc0) {	/* got a ppoll intr */
			cp->ppoll_int = 0x80;	/* clear intr */
			sc->int_flags = INT_PPL;
		}
		else
			sc->int_flags = 0;
	}
	else
		/* if ppoll_fakeisr set this bit, it will remain set,
		   otherwise, it will be cleared 
		*/
		sc->int_flags &= INT_PPL;	/* clear all but ppoll intr */
					/* kick off another routine at end */

	sc->intcopy = ((cp->intstat0) | (cp->intstat1 << 8) | (sc->intcopy)) &
			~(TI_M_INT0 | TI_M_INT1);	/* clear int flags */
	if (sc->intcopy & TI_M_SRQ) {
		sc->intcopy &= ~TI_M_SRQ;
		if (cp->busstat & TI_B_SRQ)
			sc->int_flags |= INT_SRQ;
	}

	/* if a ppoll software trigger went off during a transfer,
	   delay handling it until the transfer is complete */
	if ((sc->int_flags & INT_PPL) && (sc->transfer != NO_TFR))  {
		sc->int_flags &= ~INT_PPL;
		cp->auxcmd = H_DAI0;		/* re-enable interrupts */
		return(1);
	}
		
reswitch:
	switch(sc->transfer) {
		case INTR_TFR:
			if ((cont_transfer = intr_isr(sc)) == 0)
				(sc->owner->b_action)(sc->owner);
			break;
		case DMA_TFR:
			/*
			** If dma is not enabled or not armed for this
			** isr's level or is not interrupting then 
			** this is not a dma_channel interrupt.
			*/
			dma_interrupted = 
				(((((ch->card == (struct dma *)DMA_CHANNEL1)
				     ? dma_chan1_last_arm : dma_chan0_last_arm)
				     & DMA_CHAN_MASK) != sc->int_lvl) ||
				     (!(ch->card->dma_status & DMA_INT)));

			/*
			** This isr only services dma_channel interrupts
			*/
			cont_transfer = 1;
			if (dma_interrupted &&
			    ((cont_transfer = dma_isr(sc, cp)) == 0))
					(sc->owner->b_action)(sc->owner);
			break;
		case BUS_TRANSACTION:
			if (sc->state & WAIT_BO) {
				if (sc->intcopy & TI_M_BO) {
					TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
					sc->state &= ~WAIT_BO;
					sc->intcopy &= ~TI_M_BO;
					if ((sc->transaction_proc)(sc->owner) == 1) {
						cont_transfer = 0;
						(sc->owner->b_action)(sc->owner);
					} else {
						cont_transfer = 1;
					}
				}
			} else if (sc->state & WAIT_BI) {
				if (sc->intcopy & TI_M_BI) {
					TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
					sc->state |= IN_HOLDOFF;
					sc->state &= ~WAIT_BI;
					sc->intcopy &= ~TI_M_BI;
					if ((sc->transaction_proc)(sc->owner) == 1) {
						cont_transfer = 0;
						(sc->owner->b_action)(sc->owner);
					} else {
						cont_transfer = 1;
					}
				}
			} /* else not our isr */
			break;
		case FHS_TFR:
		case NO_TFR:
			break;
		/*
		** this dance is so transfer of control will go back
		** to the calling routine when the last byte is actually
		** handshaked out
		*/
		case BO_END_TFR:
			if (sc->intcopy & TI_M_BO) {
				sc->transfer = NO_TFR;
				TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
				sc->intcopy |= TI_M_BO;
				(sc->owner->b_action)(sc->owner);
			}
			break;
		case BI_DMA_TFR:
			if (sc->intcopy & TI_M_BI) {
				if (sc->intcopy & TI_M_END) {
					/* 
					 * We got EOI on the first byte.
					 * Forget DMA and just finish up
					 * with an interrupt transfer.
					 */
					drop_dma(sc->owner);
					sc->buffer=sc->owner->b_queue->b_xaddr;
					sc->count=sc->owner->b_queue->b_xcount;
					sc->transfer = INTR_TFR;
					goto reswitch;
				}
				sc->transfer = DMA_TFR;
				TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
				TI9914_dma_start(sc);
				cont_transfer = 1;
			}
			break;
		default:
			panic("ti9914: Unrecognized TI9914 transfer type");
	}

	TI9914_ucg_isr(sc, &this_event);

	/* this field will only be set when dil is present, else nop */
	TI9914_dil_isr(sc, this_event);

	if (sc->int_flags)
		HPIB_call_isr(sc);
	cp->auxcmd = H_DAI0;		/* enable interrupts */
	if (cont_transfer) return 1;	/* we have not finished */

	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	}
	while (i);
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
TI9914_preamb(bp, sec)
register struct buf *bp;
char sec;
{
	/*
	** send a message preamble consisting of an unlisten,
	** bus addressing, and a secondary (optional)
        */
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;

	TI9914_save_state(sc);
	TI9914_atn(sc);

	TI9914_bo(bp);
	cp->dataout = UNL;

	TI9914_bo(bp);
	if (bp->b_flags & B_READ) {
		cp->dataout = odd_partab[LAG_BASE + sc->my_address];
		cp->auxcmd = H_LON1;	/* I'm a listener */

		TI9914_bo(bp);
		cp->dataout = odd_partab[TAG_BASE + bp->b_ba];
	}
	else {
		cp->dataout = odd_partab[TAG_BASE + sc->my_address];
		cp->auxcmd = H_TON1;	/* I'm a talker */

		TI9914_bo(bp);
		cp->dataout = odd_partab[LAG_BASE + bp->b_ba];
	}
	cp->auxcmd = H_HDFA1;	/* Hold off on all */
	if (sec) {
		TI9914_bo(bp);
		snooze(65);	/* delay for chinook (>55) */
		cp->dataout = odd_partab[sec];
	}
	TI9914_bo(bp);	/* wait for last byte to get out */
	sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
	snooze(65);		/* delay again for chinook */
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
TI9914_preamb_intr(bp)
register struct buf *bp;
{
	/*
	** send a message preamble consisting of an unlisten,
	** bus addressing, and a secondary (optional)
        */
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;

enum preamb_state { preamb_set_atn = 0,
                    preamb_send_unl,
                    preamb_send_myaddress,
                    preamb_send_devaddress,
                    preamb_end};

	if (sc->transfer != BUS_TRANSACTION) {
		TI9914_save_state(sc);
		sc->transfer = BUS_TRANSACTION;
		sc->transaction_state = (int) preamb_set_atn;
		sc->transaction_proc = TI9914_preamb_intr;
	}

	switch (sc->transaction_state) {
	case preamb_set_atn:
		sc->transaction_state = (int) preamb_send_unl;
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp))
			break;
	case preamb_send_unl:
		sc->transaction_state = (int) preamb_send_myaddress;
		cp->dataout = UNL;
		if (!TI9914_bo_intr(bp))
			break;
	case preamb_send_myaddress:
		sc->transaction_state = (int) preamb_send_devaddress;
		if (bp->b_flags & B_READ) {
			cp->dataout = odd_partab[LAG_BASE + sc->my_address];
			cp->auxcmd = H_LON1;	/* I'm a listener */
			if (!TI9914_bo_intr(bp))
				break;
		} else {
			cp->dataout = odd_partab[TAG_BASE + sc->my_address];
			cp->auxcmd = H_TON1;	/* I'm a talker */
			if (!TI9914_bo_intr(bp))
				break;
		}
	case preamb_send_devaddress:
		sc->transaction_state = (int) preamb_end;
		if (bp->b_flags & B_READ)
			cp->dataout = odd_partab[TAG_BASE + bp->b_ba];
		else
			cp->dataout = odd_partab[LAG_BASE + bp->b_ba];
		cp->auxcmd = H_HDFA1;	/* Hold off on all */
		if (!TI9914_bo_intr(bp))
			break;
	case preamb_end:
		sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
		snooze(65);		/* delay again for chinook */
		sc->transfer = NO_TFR;
		sc->transaction_state = (int) 0;
		return(1);
	default:
		panic("ti9914: bad ti9914 preamb transaction state");
	}
	return(0);
}


/******************************TI9914_postamb***************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:  bp
**
**	Kernel routines called:  TI9914_atn, TI9914_bo, TI9914_restore_state
**
**	Global variables/fields referenced:  bp->b_flags, bp->b_sc, 
**		sc->card_ptr
**
**	Global vars/fields modified:  sc->intcopy
**
**	Registers modified:  dataout
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_postamb(bp)
register struct buf *bp;
{
	/* send an untalk/unlisten at the end of a message */

	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	TI9914_atn(sc);

	TI9914_bo(bp);
	if (bp->b_flags & B_READ)
		cp->dataout = UNT;
	else
		cp->dataout = UNL;

	TI9914_bo(bp);
	sc->intcopy |= TI_M_BO;	/* yes we already got a BO */
	TI9914_restore_state(sc);
}


/************************************************************************
** TI9914_fhs
**
**     This is the fast hand shake transfer routine for the TI9914 based
**     HPIB interface cards.  Data is transfered via programmed I/O in a
**     tight loop.  Transfers will terminate when the requested count is
**     satisfied  or  a  timeout  occurs.   Inbound  transfers will also
**     terminate if an EOI is  detected  or  a  specified  pattern  (see
**     io_eol_ctl() manual page in section 3) is detected.
**
**     Since this routine hogs the CPU for large transfers it is run  at
**     level zero (via sw_trigger) so the keyboard wont loose characters
**     and other interfaces running at or below the interrupt  level  of
**     the TI9914 interface cards wont get locked out.
**
** Called by: Software triggered by TI9914_transfer() 
**
** Parameters: bp - pointer to the buf describing this I/O request
**
** Return value: None
**
** Kernel routines called:
** 	fhs_in()		in ../s200io/ti9914s.s
** 	fhs_out()		in ../s200io/ti9914s.s
** 	spl6()			in ../s200/locore.s
** 	splx()			in ../s200/locore.s
** 	selcode_dequeue()	in ../s200io/selcode.c
** 	dma_dequeue()		in ../s200io/dma.c
**
** Notes:
**
**     This routine is mostly an onion skin for  the  routines  fhs_in()
**     and  fhs_out()  in  ../s200io/ti9914s.s  which do the actual data
**     transfers.  The routines were coded in assembly to  maximize  the
**     fast   hand   shake  transfer  rate.   This  transfer  method  is
**     approximately an order of magnitude faster than the interrupt per
**     byte method.
**
************************************************************************/

TI9914_fhs(bp)
struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register int i;

	sc->tfr_control = 0;
	if (bp->b_flags & B_READ)
		sc->resid = fhs_in(sc);
	else
		sc->resid = fhs_out(sc);

	i = spl6();

	if (sc->state & FHS_TIMED_OUT) {
		splx(i);
		return;
	}

	/* Must be in holdoff when we leave. The device is still addressed */
	sc->intcopy &= ~TI_M_END;
	sc->intcopy |= TI_M_BO;
	sc->transfer = NO_TFR;
	sc->state &= ~FHS_ACTIVE;

	/* call routine */
	(*bp->b_action)(bp);

	splx(i);

	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	}
	while (i);
}

/******************************routine name******************************
** Intr_isr is used to process bytes transfered using interrupts (byte-in
** byte-out).  In the byte-in case, it reads in the data byte and sets
** up for the next byte to read.  Similarly for byte-out, the routine
** writes the next byte out to the card.  When the routine detects that
** a transfer has reached its last byte, it calls 'intr_end'.
**
** Clean Up:  pattern transfer should call intr_end
**
**	Called by:  
**          TI9914_do_isr   (to process byte-in and byte-out interrupts)
**          TI9914_transfer (called directly for writes)
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
**  special processing for pattern matched reads -- IMPORTANT!
**  ----------------------------------------------------------
**  If a pattern match is encountered in this routine, intr_end is not
**  called since we must read the data-in register to get the byte in
**  order to compare it with the pattern, and since intr_end also reads
**  the data-in register.  That is, we don't want to call intr_end since 
**  that will result in reading the data-in register twice.  So, much of
**  the processing for terminated reads must be duplicated in this routine
**  and in intr_end!
************************************************************************/
intr_isr(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *)sc->card_ptr;
	register ushort inttemp = sc->intcopy; /* register for performance */
	register unsigned char data;

	if (--sc->count) {		/* not at end of count */
		if (inttemp & TI_M_BI) {  /* read */
		       /* 
			* We must keep track of the card's holdoff state in
			* hardware since there is no way to determine it
		        * by looking at the card.  Since we just read a byte
			* and it's in our data-in register, we have a holdoff.
			*/
			sc->state |= IN_HOLDOFF;

			/* if we got EOI on this byte we're done */
			if (inttemp & TI_M_END)
				intr_end(sc, inttemp);

			/* we have more bytes to transfer */
			else {
			        /* copy byte to temp reg var */
				data = cp->datain;

				/* check for pattern termination */
				if (sc->tfr_control & READ_PATTERN) {
					if (data == sc->pattern) {
						TI9914_ruptmask(sc,
							TI_S_BI, INT_OFF);
						*sc->buffer = data;
						sc->intcopy &=
							~(TI_M_BI | TI_M_END);
						sc->transfer = NO_TFR;
						sc->tfr_control = TR_MATCH;
						/* left over (if any) */
						sc->resid = sc->count;
						return 0; /* end of transfer */
					}			
				}

				/* no pattern match; set up for another byte */

				/* copy byte to buffer */
				*sc->buffer++ = data;

				/* release holdoff so read can continue */
				TI9914_RHDF(sc);

			       /* 
				* We must manually clear conditions; we 
				* always bitwise-or the card's status 
				* register into intcopy when we read the 
				* card's status register.
				*/
				sc->intcopy &= ~TI_M_BI;

				/* continue the transfer */
				return 1;
			}
		}
		else if (inttemp & TI_M_BO) {           /* write */
		        /* put new byte in data-out register on card */
			cp->dataout = *sc->buffer++;

			/* 
			 * We must manually clear conditions; we always 
			 * bitwise-or the card's status register into intcopy 
			 * when	we read the card's status register.
			 */
			sc->intcopy &= ~TI_M_BO;

			/* continue the transfer */
			return 1;
		}
		else {		/* not our isr */
			sc->count++;		/* adjust count */
			return 0;
		}
	}
	else
		return (intr_end(sc, inttemp));     /* count reached */
}


/******************************routine name******************************
** Intr_end is called to process the last byte of a data transfer and
** to terminate data transfers (the latter case is indicated by a 
** zero status).
**
**	Called by:  
**          intr_isr        (to process the last byte of a data transfer)
**          TI9914_abort    (to abort a transfer)
**          TI9914_abort_io (to abort a transfer)
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
**  read termination handling -- IMPORTANT!
**  --------------------------------------
**  See the note regarding pattern matched read termination inside
**  the intr_isr header.  Some of the processing for read termination
**  must be duplicated inside intr_isr!
**
**  special processing/isr interaction for last byte of a write
**  -----------------------------------------------------------
**  Note that intr_end sets sc->transfer to BO_END_TFR and returns 1
**  after writing the last byte to the card's data-out register.  This
**  is so the isr can correctly catch the final BO interrupt and
**  terminate the transfer; if we returned 0 the isr would think it's done
**  and later get an unexpected BO.  See the notes in isr's comment header.
**  
************************************************************************/
intr_end(sc, status)
register struct isc_table_type *sc;
register unsigned short status;
{
	register struct TI9914 *cp = (struct TI9914 *)sc->card_ptr;
	int dummy;  /* used to throw away a byte, read but not used */

	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;

	if (status & TI_M_BI) {                            /* read */
	        /* turn byte-in interrupt off */
		TI9914_ruptmask(sc, TI_S_BI, INT_OFF);

                /* read byte in data-in register to user's buffer */
		*sc->buffer = cp->datain;

                /* we may also have a pattern match */
		if (sc->tfr_control & READ_PATTERN) 
			if (*sc->buffer == sc->pattern)
				/* last character matched */
				sc->tfr_control = TR_MATCH;
		        else
                                sc->tfr_control = 0;
		else
		        sc->tfr_control = 0;
		if ((sc->count == 0) && (dil_info->full_tfr_count <= MAXPHYS))
			sc->tfr_control |= TR_COUNT; /* full count */
		if (status & TI_M_END)	/* had EOI */
			sc->tfr_control |= TR_END;

		/* we always bitwise-or in the card's status reg so we must  */
		/* clear events by hand when they become false               */
		sc->intcopy &= ~(TI_M_BI | TI_M_END);

		/* mark transfer state complete */
		sc->transfer = NO_TFR;
 
		sc->resid = sc->count;	/* left over (if any) */

	       /* 
		* We must keep track of the card's holdoff state in
		* hardware since there is no way to determine it
		* by looking at the card.  Since we just read a byte
		* and it's in our data-in register, we have a holdoff.
		*/
		sc->state |= IN_HOLDOFF;

		return 0;	/* end of the interrupt transfer */
	}
	else if (status & TI_M_BO) {                 /* write */
		sc->transfer = BO_END_TFR;

	       /* 
		* If this is for DIL, the user controls EOI mode.  Also, we
		* don't want to send EOI until the last byte of the entire
		* write which may be broken up into several strategy calls
		* inside physio.  Yes, this a kludge to fix a bug; see
		* DSDxxxxx
		*/
		if (sc->owner->b_flags & B_DIL) {
			if ((sc->owner->b_queue->dil_state &  EOI_CONTROL) 
				&& (dil_info->full_tfr_count <= MAXPHYS))
				cp->auxcmd = H_FEOI;
		} else
			cp->auxcmd = H_FEOI; /* non-DIL write always use EOI */

		/* put last byte in card's data register */
		cp->dataout = *sc->buffer;

		sc->resid = sc->count;	/* left over (if any) */

		sc->tfr_control = 0;

	       /* Why do we store a write termination reason? */
		if (sc->count == 0)	/* full count */
			sc->tfr_control |= TR_COUNT;

		/* we always bitwise-or in the card's status reg so we must  */
		/* clear events by hand when they become false               */
		sc->intcopy &= ~TI_M_BO;

		return 1;	/* wait till this byte is transfered */
	}
	/* Test is for aborting a interrupt transfer.
	** Otherwise the status byte Must have a value
	*/
	else if (status == 0) {	/* called from TI9914_abort */
		sc->intcopy |= cp->intstat0;	/* remove intr (if any) */
		sc->owner->b_error = EIO;
		if (sc->intmsksav & TI_S_BI) {
			TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
			sc->intcopy &= ~TI_M_BI;
			dummy = cp->datain;	/* ignore byte */
		}
		else {
			TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
			/* find resid count */
			sc->resid = sc->count;
		}
	}
	return 0;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  called from TI9914_transfer and TI9914_highspeed.
**
**	Parameters: sc = select code structure
**	            flags = bp->b_flags, used to determine whether read or write
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
TI9914_dma(sc, flags)
register struct isc_table_type *sc;
register int flags;			/* bp->b_flags */
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int do_interrupt = 0;
	int dummy;  /* used to throw away a byte, read but not used */

	cp->auxcmd = H_DAI1;	/* disable card */
	if (flags & B_READ) {
		sc->intcopy |= cp->intstat0;
		if ((sc->state & IN_HOLDOFF) && ((sc->intcopy & TI_M_BI) == 0)) {
			dummy = cp->datain;
		} else {
			do_interrupt = 1;
			sc->transfer = BI_DMA_TFR;
			TI9914_ruptmask(sc, TI_S_BI, INT_ON);
		}
		cp->auxcmd = H_HDFA0;	/* holdoff on all off */
		cp->auxcmd = H_HDFE1;	/* holdoff on end on */
		if ((sc->state & IN_HOLDOFF) && ((sc->intcopy & TI_M_BI) == 0))
			TI9914_RHDF(sc);
		cp->auxcmd = H_GTS;	/* off ATN */
		if (do_interrupt) {
			if (sc->intcopy & TI_M_BI)
				TI9914_fakeisr(sc);
			cp->auxcmd = H_DAI0;	/* enable card */
			return;
		}
	}
	TI9914_dma_start(sc);
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
TI9914_dma_start(sc)
register struct isc_table_type *sc;
{
	register struct buf *bp = sc->owner;
	register struct dma_channel *dma = sc->dma_chan;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int chan_flag;

	dma_build_chain(bp);	/* build the transfer chain */

	/* check if the dma channel is correct for internal hpib */
	if ((sc->card_type == INTERNAL_HPIB) &&
		(dma->card != (struct dma *) DMA_CHANNEL0))
			panic("ti9914: INTERNAL HPIB: wrong dma channel");

	cp->auxcmd = H_DAI1;	/* disable card */
	if (bp->b_flags & B_READ)
		TI9914_ruptmask(sc, TI_S_END, INT_ON);
	if (dma->card == (struct dma *) DMA_CHANNEL0)
		chan_flag = TI_DMA_0;
	else
		chan_flag = TI_DMA_1;

	/* we want the dmacard to call us when it is finished */
	dma->isr = TI9914_do_isr;
	dma->arg = (int) sc;
	sc->intcopy &= ~TI_M_BO;	/* clear bo copy */

	/* insure that we have DMA request for writes */
	if (((sc->state & ACTIVE_CONTROLLER) == 0) && 
	    ((bp->b_flags & B_READ) == 0)) {
		cp->auxcmd = H_TON0;
		snooze(1);
		cp->auxcmd = H_TON1;
        }

	sc->tfr_control = 0;	/* set this up */

	/* arm the card */
	cp->iostatus |= chan_flag;
	cp->auxcmd = H_GTS;	/* off ATN */

	/* start the dma transfer */
	dma_start(dma);

	cp->auxcmd = H_DAI0;	/* enable card */
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
dma_isr(sc, cp)
register struct isc_table_type *sc;
register struct TI9914 *cp;
{
	register struct dma_channel *dma = sc->dma_chan;

	/*
	** There may be a problem on how we got here:
	** is this from dma_isr or EOI isr?
	*/
	

	/* clear EOI interrupt */
	TI9914_ruptmask(sc, TI_S_END, INT_OFF);

	dmaend_isr(sc);		/* clear up dma channel */

        /* disable the card for DMA */
	if (dma->card == (struct dma *) DMA_CHANNEL0)
		cp->iostatus &= ~TI_DMA_0;
	else
		cp->iostatus &= ~TI_DMA_1;

	drop_dma(sc->owner);	/* get rid of channel */

	cp->auxcmd = H_HDFA1;	/* go to holdoff state */
	cp->auxcmd = H_HDFE0;	/* off holdoff state */

	if (sc->owner->b_flags & B_READ) {
		struct dil_info *dil_info = 
				(struct dil_info *)sc->owner->dil_packet;
		if (sc->intcopy & TI_M_END)
			sc->tfr_control |= TR_END;	/* mark EOI */
		if ((sc->resid == 0) && (dil_info->full_tfr_count <= MAXPHYS))
			sc->tfr_control |= TR_COUNT;	/* complete transfer */
		sc->state |= IN_HOLDOFF;
		sc->intcopy &= ~(TI_M_END | TI_M_BI);
		sc->transfer = NO_TFR;
		return 0;
	}

	if (sc->intcopy & TI_M_BO) {
		sc->transfer = NO_TFR;
		sc->tfr_control = TR_COUNT;	/* complete transfer */
		return 0;
	}

	/* make sure last byte is handshaked out */
	sc->transfer = BO_END_TFR;
	sc->tfr_control = TR_COUNT;	/* complete transfer */
	TI9914_ruptmask(sc, TI_S_BO, INT_ON);
	return 1;
}


/******************************routine name******************************
** [short description of routine]
**	TI9914_highspeed is used for the tape drive for now.
**
**	Assumptions:	1) selectcode already owned
**			2) dma channel already owned
**
**	see simon.c for more explanation of this routine.
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
TI9914_highspeed(bp, sec1, sec2, bfr, count, proc)
register struct buf *bp;
char sec1, sec2;
register char *bfr;
register short count;
int (*proc)();
{
	register struct isc_table_type *sc = bp->b_sc;

	if (!dma_here) {	/* can't do without good card */
		bp->b_error = EIO;
		escape(1);
	}
	if (sc->owner != bp)
		panic("ti9914: highspeed, started without select code");

 	/* disable interrupts, TI9914_dma does this and enables them again so
	** we do not have to.
	*/
	((struct TI9914 *)sc->card_ptr)->auxcmd = H_DAI1;

	TI9914_mesg(bp, T_WRITE, sec1, bfr, count);

	TI9914_preamb(bp, sec2);	/* address the device */

	sc->transfer = DMA_TFR;

	TI9914_dma(sc, bp->b_flags);			/* start the dma transfer */
}


/***************************TI9914_status*****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_BUS_STATUS) thru iosw (iod_status)
**		and via dil_utility (dil_action) via enqueue (b_action)
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
TI9914_status(bp)
register struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register int status;
	register int status_result;
	register ushort state = bp->b_sc->state;

	status = bp->pass_data1;
	status_result = 0;
	if (status & STATE_REN)
		if (cp->addrstate & TI_A_REN)
			status_result |= STATE_REN;
	if (status & STATE_SRQ)
		if (cp->busstat & TI_B_SRQ)
			status_result |= STATE_SRQ;
	if (status & STATE_NDAC)
		if (cp->busstat & TI_B_NDAC)
			status_result |= STATE_NDAC;
	if (status & STATE_SYSTEM_CTLR)
		if (state & SYSTEM_CONTROLLER)
			status_result |= STATE_SYSTEM_CTLR;
	if (status & STATE_ACTIVE_CTLR)
		if (state & ACTIVE_CONTROLLER)
			status_result |= STATE_ACTIVE_CTLR;
	if (status & STATE_TALK)
		if (cp->addrstate & TI_A_TADS)
			status_result |= STATE_TALK;
	if (status & STATE_LISTEN)
		if (cp->addrstate & TI_A_LADS)
			status_result |= STATE_LISTEN;
	if (status & ~STATUS_BITS) { /* check for bad value */
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
	}
	else
		/* pass back value */
		bp->return_data = status_result;
}

/******************************TI9914_ren*****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_REN) via dil_utility (dil_action)
**		via enqueue (b_action)
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
TI9914_ren(bp)
register struct buf *bp;
{
	register unsigned int dil_state = bp->b_queue->dil_state;
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;

	if (dil_state & D_RAW_CHAN) {
		if (bp->pass_data1)
			cp->auxcmd = H_SRE1;
		else
			cp->auxcmd = H_SRE0;
	/** The else path won't be hit until ren is supported for non-raw
	 ** DIL device files
	 **/
	} else { /* address the bus */
		TI9914_preamb(bp, 0);
		cp->auxcmd = H_GTS; /* drop ATN */
		cp->auxcmd = H_FEOI; /* set EOI */
		if (bp->pass_data1)
			cp->auxcmd = H_SRE1; /* assert REN */
		else
			cp->dataout = GTL; /* send GTL */
		TI9914_postamb(bp);
	}
}

/************************TI9914_set_ppoll_resp**************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_PPOLL_IST and HPIB_PPOLL_RESP) via
**		dil_utility (dil_action) and enqueue (b_action)
**
**	Parameters:  bp  (needs sc)
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
TI9914_set_ppoll_resp(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int listener, talker;

	if (sc->state & IST)
		cp->ppoll = sc->ppoll_resp >> 8;
	else
		cp->ppoll = (unsigned char) sc->ppoll_resp;

	if (!(sc->state & ACTIVE_CONTROLLER)) {
		if (!(cp->addrstate & TI_A_TADS))
			if ((!(cp->addrstate & TI_A_ATN)) || 
			    (!(cp->busstat & TI_B_EOI))) {
				return;
			}
		listener = cp->addrstate & TI_A_LADS;
		talker = cp->addrstate & TI_A_TADS;
		/* begin software reset */
		cp->auxcmd = H_DAI1;
		cp->auxcmd = H_SWRST1;
		cp->auxcmd = H_STDL1;
		cp->auxcmd = H_VSTDL1;
		cp->intmask0 = 0;
		cp->intmask1 = 0;
		cp->iostatus = ~(TI_DMA_0 | TI_DMA_1);
		cp->address_reg = sc->my_address;
		sc->state &= ~IN_HOLDOFF;
		sc->intcopy = TI_M_BO;
		cp->auxcmd = H_HDFA1;
		cp->auxcmd = H_HDFE0;
		cp->auxcmd = H_SHDW0;
		if (sc->state & IST)
			cp->ppoll = sc->ppoll_resp >> 8;
		else
			cp->ppoll = (unsigned char) sc->ppoll_resp;
		cp->auxcmd = H_SWRST0;
		if (listener)
			cp->auxcmd = H_LON1;
		if (talker)
			cp->auxcmd = H_TON1;
		TI9914_ruptmask(sc, sc->intmsksav, INT_ON);
		cp->auxcmd = H_DAI0;
	}
}

/***************************TI9914_dil_abort***************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_RESET) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs cp)
**
**	Kernel routines called:  do_ti9914_ifc
**
**	Global variables/fields referenced:  bp->b_sc->card_ptr
**
**	Registers modified:  auxcmd
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_dil_abort(bp)
register struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;

	do_ti9914_ifc(cp);	/* set ifc */
	cp->auxcmd = H_GTS;	/* clear atn */
}

/****************************TI9914_dil_reset***************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (IO_RESET) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:	bp (needs sc)
**
**	Kernel routines called:  TI9914_hw_clear, snooze
**
**	Global variables/fields referenced:  bp->b_sc, sc->card_ptr
**
**	Registers modified:  card_id
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_dil_reset(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	/* do a hard reset to the card */
	/* internal hpib corrupts dma channel 0 when this is done
	cp->card_id = 1;
	snooze(100);
	*/
	TI9914_hw_clear(sc);
}


extern int pct_flag;

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_PASS_CONTROL) via dil_util (dil_action)
**		via enqueue (b_action)
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
TI9914_pass_control(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register char listener;

	enum pct_state { pct_set_atn = 0,
                    pct_send_unl,
                    pct_send_talkaddr,
                    pct_send_tct,
                    pct_release_control};

	if (sc->transfer != BUS_TRANSACTION) {
		TI9914_save_state(sc);
		sc->transfer = BUS_TRANSACTION;
		sc->transaction_state = (int) pct_set_atn;
		sc->transaction_proc = TI9914_pass_control;
	}

	switch (sc->transaction_state) {
	case pct_set_atn:
		sc->transaction_state = (int) pct_send_unl;
		TI9914_ppoll_off(sc);
		pct_flag = 1;
		listener = cp->addrstate & TI_A_LADS;
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp))
			break;
	case pct_send_unl:
		sc->transaction_state = (int) pct_send_talkaddr;
		cp->dataout = UNL;
		if (!TI9914_bo_intr(bp))
			break;
	case pct_send_talkaddr:
		sc->transaction_state = (int) pct_send_tct;
		cp->auxcmd = H_TON0;
		cp->dataout = 0x40 + (bp->pass_data1 & 0x1f);
		if (!TI9914_bo_intr(bp))
			break;
	case pct_send_tct:
		sc->transaction_state = (int) pct_release_control;
		cp->dataout = TCT;
		if (!TI9914_bo_intr(bp))
			break;
	case pct_release_control:
		cp->auxcmd = H_RLC; /* we have passed control */
		cp->auxcmd = H_TON0;
		if (!listener)
			cp->auxcmd = H_LON0;
		sc->state &= ~ACTIVE_CONTROLLER;/* keep track */
		sc->intcopy |= TI_M_BO;	/* already got a BO */
		TI9914_ruptmask(sc, TI_S_UCG, INT_ON);
		pct_flag = 0;
		sc->transfer = NO_TFR;
		TI9914_restore_state(sc);
		sc->transaction_state = (int) 0;
		return(1);
	default:
		panic("ti9914: bad ti9914 pass control transaction state");
	}
	return(0);
}


/***************************TI9914_send_cmd****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SEND_CMD) and do_hpib_io via
**		dil_util (dil_action) via enqueue (b_action)
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
TI9914_send_cmd(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	long flags = bp->b_queue->b_flags;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register unsigned char data;
	extern int pct_flag;

	enum send_transaction_state { set_atn = 0,
				      send_bytes,
				      release_control,
				      send_end};

	if (sc->transfer != BUS_TRANSACTION) {
		TI9914_save_state(sc);
		sc->transfer = BUS_TRANSACTION;
		sc->transaction_state = (int) set_atn;
		sc->transaction_proc = TI9914_send_cmd;
	}

re_switch:
	switch (sc->transaction_state) {
	case set_atn:
		sc->transaction_state = (int) send_bytes;
		TI9914_atn(sc);
		if (!TI9914_bo_intr(bp))
			break;
	case send_bytes:
		pct_flag = 1;

		while (bp->pass_data1--) {
			data = *(unsigned char *)bp->pass_data2;
			if (flags & PARITY_CONTROL)
				cp->dataout = (unsigned char) odd_partab[*(char *)bp->pass_data2++];
			else
				cp->dataout = *(unsigned char *)bp->pass_data2++;

			if (data == (sc->my_address | LAG_BASE)) 
				cp->auxcmd = H_LON1;
			else if (data == (sc->my_address | TAG_BASE)) 
				cp->auxcmd = H_TON1;
			else if (data == 0x3f)  /* UNL */
				cp->auxcmd = H_LON0;
			else if (data == 0x5f)  /* UNT */
				cp->auxcmd = H_TON0;
			else if ((data == 0x9) && ((cp->addrstate & TI_A_TADS) == 0)) {
				sc->transaction_state = (int) release_control;
				if (!TI9914_bo_intr(bp))
					return(0);
				TI9914_ppoll_off(sc);
				goto re_switch;
			} else if ((data & 0x60) == TAG_BASE) 
				cp->auxcmd = H_TON0;

			if (bp->pass_data1 <= 0)
				sc->transaction_state = (int) send_end;

			if (!TI9914_bo_intr(bp))
				return(0);
		}
	case send_end:
		sc->intcopy |= TI_M_BO;	/* save a BO */
		sc->transfer = NO_TFR;
		TI9914_restore_state(sc);
		pct_flag = 0;
		sc->transaction_state = (int) 0;
		return(1);
	case release_control:
		cp->auxcmd = H_RLC;
		sc->intcopy |= TI_M_BO;	/* save a BO */
		sc->state &= ~ACTIVE_CONTROLLER;
		sc->transfer = NO_TFR;
		TI9914_restore_state(sc);
		pct_flag = 0;
		sc->transaction_state = (int) 0;
		return(1);
	default:
		panic("ti9914: bad ti9914 send transaction state");
	}
	return(0);
}

/*************************TI9914_ctlr_status***************************
** [short description of routine]
**
**	Called by:  hpib_ioctl and do_hpib_io via dil_utility (dil_action)
**		via enqueue (b_action) and by do_hpib_io thru iosw (iod_ctlr_status)
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
TI9914_ctlr_status(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	/* see if we are system controller */
	if ((cp->extstatus & 0x80) == 0)
		sc->state &= ~SYSTEM_CONTROLLER;
	else
		sc->state |= SYSTEM_CONTROLLER;

	/* are we active controller? */
	if (cp->extstatus & 0x40)
		sc->state &= ~ACTIVE_CONTROLLER;
	else
		sc->state |= ACTIVE_CONTROLLER;
}

/******************************TI9914_srq*****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SRQ) via dil_utility (dil_action)
**		via enqueue (b_action)
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
TI9914_srq(bp)
register struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	cp->spoll = 0;
	cp->spoll = bp->pass_data1;
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
TI9914_iocheck(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register struct dil_info *dil_info = (struct dil_info *) bp->dil_packet;
	register unsigned int dil_state = bp->b_queue->dil_state & 0xffff;

	/* always look at our current status */
	if (cp->extstatus & 0x40)
		sc->state &= ~ACTIVE_CONTROLLER;
	else
		sc->state |= ACTIVE_CONTROLLER;

	if ( (!(dil_state & D_RAW_CHAN)) && 
	     (!(sc->state & ACTIVE_CONTROLLER))) {
		bp->b_error = EINVAL;	/* can't do */
		bp->b_flags |= B_ERROR;
		return TRANSFER_ERROR;
	}
	if (dil_state & D_RAW_CHAN) {
		if ((bp->b_flags & B_READ) && (!(cp->addrstate & TI_A_LADS))) {
			if (sc->intcopy & TI_M_BI)
				return TRANSFER_READY;
			if (sc->state & ACTIVE_CONTROLLER) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR;
				return TRANSFER_ERROR;
			} else {
				/* wait until addressed to listen */
				sc->intr_wait |= WAIT_READ;
				dil_info->intr_wait |= WAIT_READ;
				TI9914_ruptmask(sc, TI_S_MA, INT_ON);
				return TRANSFER_WAIT;
			}
		}
		if ((!(bp->b_flags & B_READ)) && (!(cp->addrstate & TI_A_TADS))) {
			if (sc->state & ACTIVE_CONTROLLER) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR;
				return TRANSFER_ERROR;
			} else {
				/* wait until addressed to talk */
				sc->intr_wait |= WAIT_WRITE;
				dil_info->intr_wait |= WAIT_WRITE;
				TI9914_ruptmask(sc, TI_S_MA, INT_ON);
				return TRANSFER_WAIT;
			}
		}
	}
	return TRANSFER_READY;
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
talk_check(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	if (cp->addrstate & TI_A_ATN)
		timeout(talk_check, sc, 1, NULL);
	else  {
		if (cp->addrstate & TI_A_TADS) {
			/* someone really addressed us to talk */
			sc->int_flags |= INT_WRITE;
			HPIB_call_isr(sc);
		} else {
			sc->intr_wait |= WAIT_WRITE;
			TI9914_ruptmask(sc, TI_S_MA, INT_ON);
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
listen_check(sc)
register struct isc_table_type *sc;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	if (cp->addrstate & TI_A_ATN)
		timeout(listen_check, sc, 1, NULL);
	else  {
		if (cp->addrstate & TI_A_LADS) {
			/* someone really addressed us to listen */
			sc->int_flags |= INT_READ;
			HPIB_call_isr(sc);
		} else {
			sc->intr_wait |= WAIT_READ;
			TI9914_ruptmask(sc, TI_S_MA, INT_ON);
		}
	}
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by: TI9914_do_isr
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
**	Algorithm:  
**	Don't try to restore the parallel poll state before passing
**	control is completed.  If the user passes control via
**	hpib_send_cmnd(TI9914_send_cmd), we receive the "TCT as an
**	unrecognized command.  We tried to restore the parallel poll state
**	based on the pct_flag.  The controller passing command (via 
**	hpib_send_cmnd) was waiting for a BO interrupt and the pct_flag
**	was not set.
**
**	The driver assumed that the passing of control was completed or
**	no passing of control was going on.  This assumption was based
**	on the pct_flag not being set so TI9914_restore_ppoll_state was
**	invoked.
**
**	A fake bp was created for the TI9914_restore_ppoll_state.  If
**	someone was waiting the a parallel poll, TI9914_restore_ppoll_state
**	would (sw_trigger the re-enabling ppolls). TI9914_do_isr would
**	eventually be called with the "fake" bp, which was invalid.
**
**	The pcg_flag should not be "depended" on if the user is passing
**	control via hpib_send_cmnd.
************************************************************************/
TI9914_ucg_isr(sc, this_event)
register struct isc_table_type *sc;
int *this_event;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register unsigned char command;
	register unsigned short copy;
	int interesting_events;
	int release_dac_holdoff = 0;
	int x;
	int was_CIC = 0;
	struct buf *fake_bp;
	struct iobuf *fake_iob;
        int saved_interesting_events;

	*this_event = 0;
	copy = sc->intcopy;
	interesting_events = sc->intr_wait;

        /* cause code below to process events for PIL->DIL */
        saved_interesting_events = interesting_events;
        interesting_events |= PIL_events;
	
	if (copy & TI_M_UCG) {	/* unrecognized command */
		command = cp->cmdpass & 0x7f;	/* get command */
		if (command == (TCT &0x7f)) {
			/* control is being passed to us */
			if (pct_flag == 0) {
				cp->auxcmd = H_DACR0; /* release DAC holdoff */
				while (cp->addrstate & TI_A_ATN)
					;
				cp->auxcmd = H_RQC;	/* request control */
			} else {
				cp->auxcmd = H_RQC;	/* request control */
				cp->auxcmd = H_DACR0; /* release DAC holdoff */
			}
			cp->auxcmd = H_GTS;

			/* We were previously active controller? */
			if (sc->state & ACTIVE_CONTROLLER)
			   was_CIC = 1;

			/* assume nothing can stop us now */
			sc->state |= ACTIVE_CONTROLLER;
			if (interesting_events & WAIT_CTLR) {
				sc->int_flags |= INT_CTLR;
				sc->intr_wait &= ~WAIT_CTLR;
			}
			if (interesting_events & INTR_CTLR) {
				*this_event |= INTR_CTLR;
				sc->intr_wait &= ~INTR_CTLR;
			}
			if (interesting_events & INTR_UAC) {
				*this_event |= INTR_UAC;
				sc->intr_wait &= ~INTR_UAC;
			}

			/* 
			 * Turn ppoll ints on if they were on before we
			 * lost control and another card on our system
			 * isn't passing control, else disable them.  
			 * See DSDe400224. We have rec'd a TCT
			 * but the passing of control via TI9914_send_cmd
			 * has not yet been completed.  We won't restore
			 * parallel polls before passing control is
			 * completed.  This part of the fix to DSDe400224
			 * has been changed to check if we were previously
			 * ACTIVE CONTROLLER, if not then we don't restore
			 * the parallel poll state.
	                 */
			if (pct_flag == 0) {

				/* Restore ppolls only if we were CIC before */
				if ((was_CIC) && (sc->owner == NULL)) {
					MALLOC(fake_bp, struct buf *, sizeof(struct buf),
							M_IOSYS, M_NOWAIT);
					if (fake_bp == NULL)
						panic("ti9914: cannot allocate kernel memory");
					MALLOC(fake_iob, struct iobuf *, sizeof(struct iobuf),
							M_IOSYS, M_NOWAIT);
					if (fake_iob == NULL)
						panic("ti9914: cannot allocate kernel memory");
			       		fake_bp->b_queue = fake_iob;
			       		fake_bp->b_sc    = sc; 
					sc->owner       = fake_bp;
					TI9914_restore_ppoll_state(sc);
					FREE(fake_bp, M_IOSYS);	
					FREE(fake_iob, M_IOSYS);	
					}
			} else {
			        sc->ppoll_f = NULL;
				sc->ppoll_l = NULL;
				sc->intr_wait &= ~INTR_PPOLL;
			}
		}
		if (cp->addrstate & TI_A_LADS) {
			if (interesting_events & INTR_UAC) {
				*this_event |= INTR_UAC;
				sc->intr_wait &= ~INTR_UAC;
			}
		} else {
			if (command != (TCT &0x7f)) {
				if (interesting_events & INTR_UUC) {
					*this_event |= INTR_UUC;
					sc->intr_wait &= ~INTR_UUC;
				}
			}
		}
		cp->auxcmd = H_DACR0;	/* release DAC holdoff */
		if (command == (PPC & 0x7f)) {
			/* ppoll configure */
			cp->auxcmd = H_PTS;	/* to get ppoll enable */
		}
		if (command == (PPU & 0x7f)) {
			/* ppoll unconfigure */
			sc->ppoll_resp = 0;
			cp->ppoll = 0;
			if (interesting_events & INTR_PPCC) {
				*this_event |= INTR_PPCC;
				sc->intr_wait &= ~INTR_PPCC;
			}
		}
		if ((command & 0x70) == 0x60) {
			/* ppoll enable */
			/* format: 0110SPPP */
			sc->ppoll_resp = 0x0001 << (command & 0xf);
			if (sc->state & IST)
				cp->ppoll = sc->ppoll_resp >> 8;
			else
				cp->ppoll = (unsigned char) sc->ppoll_resp;
			if (interesting_events & INTR_PPCC) {
				*this_event |= INTR_PPCC;
				sc->intr_wait &= ~INTR_PPCC;
			}
		} else if ((command & 0x70) == 0x70) {
			/* ppoll disable */
			sc->ppoll_resp = 0;
			cp->ppoll = 0;
			if (interesting_events & INTR_PPCC) {
				*this_event |= INTR_PPCC;
				sc->intr_wait &= ~INTR_PPCC;
			}
		}
		sc->intcopy &= ~TI_M_UCG;
	}

	if (sc->int_flags & INT_SRQ) {
		if (interesting_events & WAIT_SRQ) {
			sc->int_flags |= INT_SRQ;
			sc->intr_wait &= ~WAIT_SRQ;
		}
		if (interesting_events & INTR_SRQ) {
			*this_event |= INTR_SRQ;
			sc->intr_wait &= ~INTR_SRQ;
		}
		TI9914_ruptmask(sc, TI_S_SRQ, INT_OFF);
	}

	/* addressed to talk? */
	if (cp->addrstate & TI_A_TADS) {
                if (sc->intmsksav & TI_S_MA)
			cp->auxcmd = H_DACR0;

		if (interesting_events & WAIT_TALK) {
			release_dac_holdoff = 1;
			sc->int_flags |= INT_TADS;
			sc->intr_wait &= ~WAIT_TALK;
		}
		if (interesting_events & WAIT_WRITE) {
			release_dac_holdoff = 1;
			if (cp->addrstate & TI_A_ATN) {
				sc->intr_wait &= ~WAIT_WRITE;
				timeout(talk_check, sc, 1, NULL);
			} else {
				sc->int_flags |= INT_WRITE;
				sc->intr_wait &= ~WAIT_WRITE;
			}
		}
		if (interesting_events & INTR_TALK) {
			release_dac_holdoff = 1;
			*this_event |= INTR_TALK;
			sc->intr_wait &= ~INTR_TALK;
		}
	}

	if ((sc->intcopy & TI_M_BI) || (cp->addrstate & TI_A_LADS)) {
		if ((cp->addrstate & TI_A_LADS) && (sc->intmsksav & TI_S_MA))
			cp->auxcmd = H_DACR0;

		if (interesting_events & WAIT_LISTEN) {
			release_dac_holdoff = 1;
			sc->int_flags |= INT_LADS;
			sc->intr_wait &= ~WAIT_LISTEN;
		}
		if (interesting_events & WAIT_READ) {
			release_dac_holdoff = 1;
			if (cp->addrstate & TI_A_ATN) {
				sc->intr_wait &= ~WAIT_READ;
				timeout(listen_check, sc, 1, NULL);
			} else {
				sc->int_flags |= INT_READ;
				sc->intr_wait &= ~WAIT_READ;
			}
		}
		if (interesting_events & INTR_LISTEN) {
			release_dac_holdoff = 1;
			*this_event |= INTR_LISTEN;
			sc->intr_wait &= ~INTR_LISTEN;
		}
	}

	/* if no one else is interested in a MA interrupt turn it off */
	if (!(sc->intr_wait & (WAIT_TALK | WAIT_LISTEN | 
			       WAIT_READ | WAIT_WRITE |
			       INTR_TALK | INTR_LISTEN))) {
		TI9914_ruptmask(sc, TI_S_MA, INT_OFF);
	}
	
	if (interesting_events & INTR_MASK) {
		if (copy & TI_M_DCAS) {
			cp->auxcmd = H_DACR0;	/* release DAC holdoff */
			*this_event |= INTR_DCL;
			sc->intr_wait &= ~INTR_DCL;
			TI9914_ruptmask(sc, TI_S_DCAS, INT_OFF);
		}

		if (copy & TI_M_IFC) {
			*this_event |= INTR_IFC;
			sc->intr_wait &= ~INTR_IFC;
			TI9914_ruptmask(sc, TI_S_IFC, INT_OFF);
		}

		if (copy & TI_M_MAC) {
			*this_event |= INTR_MAC;
			sc->intr_wait &= ~INTR_MAC;
			TI9914_ruptmask(sc, TI_S_MAC, INT_OFF);
		}

		if (copy & TI_M_GET) {
			cp->auxcmd = H_DACR0;	/* release DAC holdoff */
			*this_event |= INTR_GET;
			sc->intr_wait &= ~INTR_GET;
			TI9914_ruptmask(sc, TI_S_GET, INT_OFF);
		}

		if (copy & TI_M_REN) {
			*this_event |= INTR_REN;
			sc->intr_wait &= ~INTR_REN;
			TI9914_ruptmask(sc, TI_S_RLC, INT_OFF);
		}

		if (copy & TI_M_ERR) {
			*this_event |= INTR_ERR;
			sc->intr_wait &= ~INTR_ERR;
			TI9914_ruptmask(sc, TI_S_ERR, INT_OFF);
		}

		if (copy & TI_M_SPAS) {
			*this_event |= INTR_SPAS;
			sc->intr_wait &= ~INTR_SPAS;
			TI9914_ruptmask(sc, TI_S_SPAS, INT_OFF);
		}

		if (copy & TI_M_APT) {
			/* may not want to do this yet? */
			cp->auxcmd = H_DACR0;	/* release DAC holdoff */

			*this_event |= INTR_APT;
			sc->intr_wait &= ~INTR_APT;
			TI9914_ruptmask(sc, TI_S_APT, INT_OFF);
		}

	}
	if ((copy & TI_M_MA) || release_dac_holdoff)
		cp->auxcmd = H_DACR0;
	sc->intcopy &= ~(TI_M_ERR | TI_M_APT | TI_M_SPAS | TI_M_MA | TI_M_SRQ | TI_M_REN | TI_M_IFC | TI_M_GET | TI_M_DCAS);

        /* call PIL-DIL isr if wanted events occurred (and PIL present) */
        if (PIL_dil_isr != 0) {
                if (*this_event & PIL_events) 
                        (*PIL_dil_isr)(sc, *this_event);        
        }

        /* remove any bits that were PIL only */
        *this_event &= saved_interesting_events;
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
TI9914_dil_isr(sc, this_event)
register struct isc_table_type *sc;
{
	register struct dil_info *dil_info;
	register struct buf *cur_buf, *next_buf;

	if (this_event)	/* get the world going again */
		for (cur_buf = sc->event_f; cur_buf != NULL; cur_buf = next_buf) {
			dil_info = (struct dil_info *) cur_buf->dil_packet;
			next_buf = dil_info->dil_forw;
			if (this_event & dil_info->intr_wait) {
			        /* 
				 * Store any events that need to be returned to the
				 * user in 'event'.  'Event' will be cleared when
				 * a signal is sent to the user.  See FSDls02304.
				 */
				dil_info->event |= this_event & dil_info->intr_wait;

				dil_info->intr_wait &= ~this_event;
				deliver_dil_interrupt(cur_buf);
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
TI9914_wait_set(sc, active_waits, request)
register struct isc_table_type *sc;
register int *active_waits;
register int request;
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int status = 0;
	int enabled;
	int x;

	x = spl6();
	if (request & STATE_ACTIVE_CTLR) {
		if (sc->state & ACTIVE_CONTROLLER)
			status |= STATE_ACTIVE_CTLR;
		else {
			sc->intr_wait |= WAIT_CTLR;
			*active_waits |= WAIT_CTLR;
		}
	}
	if (request & STATE_SRQ) {
		enabled = sc->intr_wait & (WAIT_SRQ | INTR_SRQ);
		*active_waits |= WAIT_SRQ;
		sc->intr_wait |= WAIT_SRQ;
		TI9914_ruptmask(sc, TI_S_SRQ, INT_ON);

		/* check first for SRQ */
		if (cp->busstat & TI_B_SRQ) {
			status |= STATE_SRQ;
			*active_waits &= ~WAIT_SRQ;
			if (!enabled) {
				sc->intr_wait &= ~WAIT_SRQ;
				TI9914_ruptmask(sc, TI_S_SRQ, INT_OFF);
			}
		}
	}
	if (request & STATE_TALK) {
		if (cp->addrstate & TI_A_TADS)
			status |= STATE_TALK;
		else if (status == 0) {
			sc->intr_wait |= WAIT_TALK;
			*active_waits |= WAIT_TALK;
			TI9914_ruptmask(sc, TI_S_MA, INT_ON);
		}
	}
	if (request & STATE_LISTEN) {
		if (cp->addrstate & TI_A_LADS)
			status |= STATE_LISTEN;
		else if (status == 0) {
			sc->intr_wait |= WAIT_LISTEN;
			*active_waits |= WAIT_LISTEN;
			TI9914_ruptmask(sc, TI_S_MA, INT_ON);
		}
	}
	splx(x);
	return(status);
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
TI9914_wait_clear(sc)
register struct isc_table_type *sc;
{
	if ((sc->intr_wait & (WAIT_SRQ | INTR_SRQ)) == 0)
		TI9914_ruptmask(sc, TI_S_SRQ, INT_OFF);

	/* if no one else is interested in a MA interrupt turn it off */
        if (!(sc->intr_wait & (WAIT_TALK | WAIT_LISTEN |
                               WAIT_READ | WAIT_WRITE |
                               INTR_TALK | INTR_LISTEN))) {
                TI9914_ruptmask(sc, TI_S_MA, INT_OFF);
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
**
**      -------------------------------------------------------------
**      Bug Fix:
**
**      We simply need to clear the old event bits out of the
**      per-open mask and bitwise-or in the new mask. 
**
**      A cleanup attempt made turned out to be incorrect.  It was
**      thought that the per-open mask manipulation could be moved
**      to a higher level piece of software (ie the device driver).
**      However, if an event came in after the mask was fixed but
**      before the spl level is raised in this routine, we would 
**      mark the event to be delivered and turn the interrupt off on
**      the card in the isr, but then turn the interrupt back on 
**      in this routine.  So the per-open mask manipulation must be
**      done AFTER the spl level is raised.
**
**      -------------------------------------------------------------
**      Bug Fix: FSDls02301
**
**      We need to read the status register to clear leftover edge
**      triggered event bits that occurred before the user enabled those
**      events.  Otherwise, when we enable the interrupt, the user will
**      get it right away (even though it occurred in the past -- ie 
**      once an edge-triggered event occurs, it's status bit is not
**      cleared until the status register is read).  
**   
**      If we read the status register we have to handle any other
**      events that have occurred as well.  After we raise the spl
**      level, any card interrupts are held off; if we read the
**      status register and later lower the spl level, we won't get
**      the interrupts held off.  So, we must handle any interrupts
**      that came in after the spl level is raised; this task is
**      rather complicated, therefore the isr is called directly to
**      process any held off interrupts and consequently clear out
**      old edge-triggered event bits in the status register.
**
**      Note that we can't use a software trigger mechanism to call
**      the isr since the routine would continue w/out the status 
**      register being read.
**
**      We must also clear out the old per-open interrupt mask BEFORE
**      calling the isr.  If we don't, interrupts that were held off
**      or interrupts that are faked in the isr depending on certain
**      conditions (see LTN) will be delivered "after" the call to
**      io_on_interrupt (ie that's the way it appears to the user).
**      Since a call to io_on_interrupt resets the user's interrupt
**      mask, we claim the old mask is in effect until we ENTER 
**      this routine.
**
**      Another small problem came up as a result of clearing the
**      per-open mask before calling the isr.  Suppose the user
**      calls io_on_interrupt w/ a mask with an event that
**      was turned on in a previous call.  If that event occurs
**      after we clear the mask and before the call to the isr is
**      made, the user won't get the interrupt.  So, we claim that
**      there will be a period of time during io_on_interrupt 
**      processing where the user will cannot catch any events.  Note
**      that if we were to set up the new mask before the call we
**      would have the same problem as with moving the mask
**      manipulation to higher level software (see other bug fix
**      notes).  
************************************************************************/
TI9914_intr_set(sc, per_open_mask, mask)
register struct isc_table_type *sc;
register int *per_open_mask;
register int mask;			/* new interrupt bits */
{
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	int this_event;
	int x;

	x = spl6();

	*per_open_mask &= ~INTR_MASK;
	TI9914_do_isr(sc);

	sc->intr_wait |= mask;
     
	/* (software) enable appropriate interrupting conditions */
	*per_open_mask |= mask;
	
	if (mask & INTR_DCL) 
		TI9914_ruptmask(sc, TI_S_DCAS, INT_ON);
	if (mask & INTR_IFC) 
		TI9914_ruptmask(sc, TI_S_IFC, INT_ON);
	if (mask & INTR_MAC)
		TI9914_ruptmask(sc, TI_S_MAC, INT_ON);
	if (mask & INTR_GET)
		TI9914_ruptmask(sc, TI_S_GET, INT_ON);
	if (mask & INTR_REN)
		TI9914_ruptmask(sc, TI_S_RLC, INT_ON);
	if (mask & INTR_ERR) 
		TI9914_ruptmask(sc, TI_S_ERR, INT_ON);
	if (mask & INTR_APT)
		TI9914_ruptmask(sc, TI_S_APT, INT_ON);
	if (mask & INTR_SPAS) 
		TI9914_ruptmask(sc, TI_S_SPAS, INT_ON);

	if (mask & INTR_SRQ) {
		TI9914_ruptmask(sc, TI_S_SRQ, INT_ON);
		if (cp->busstat & TI_B_SRQ) {
			this_event = INTR_SRQ;
			sc->intr_wait &= ~INTR_SRQ;
			if (!(sc->intr_wait & WAIT_SRQ))
				TI9914_ruptmask(sc, TI_S_SRQ, INT_OFF);
			TI9914_dil_isr(sc, this_event);
		}
	}
	if (mask & INTR_TALK) {
		TI9914_ruptmask(sc, TI_S_MA, INT_ON);
		if (cp->addrstate & TI_A_TADS) {
			this_event = INTR_TALK;
			sc->intr_wait &= ~INTR_TALK;
			if (!(sc->intr_wait & (WAIT_TALK | WAIT_WRITE |
					INTR_LISTEN | WAIT_LISTEN | WAIT_READ)))
				TI9914_ruptmask(sc, TI_S_MA, INT_OFF);
			TI9914_dil_isr(sc, this_event);
		}
	}
	if (mask & INTR_LISTEN) {
		TI9914_ruptmask(sc, TI_S_MA, INT_ON);
		if (cp->addrstate & TI_A_LADS) {
			this_event = INTR_LISTEN;
			sc->intr_wait &= ~INTR_LISTEN;
			if (!(sc->intr_wait & (WAIT_LISTEN | WAIT_READ |
					INTR_TALK | WAIT_TALK | WAIT_WRITE)))
				TI9914_ruptmask(sc, TI_S_MA, INT_OFF);
			TI9914_dil_isr(sc, this_event);
		}
	}


	/* The following are UNC interrupts.  This interrupt is always on */
	/* if (mask & INTR_CTLR) */
	/* if (mask & INTR_PPCC) */
	/* if (mask & INTR_UAC)  */
	/* if (mask & INTR_UUC)  */

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
TI9914_abort_io(bp)
struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;
	register int x;
	register int i_am_controller;
	int dummy;  /* for data read from card, never used */

	if (sc->owner != bp)
		return;

	cp->auxcmd =H_DAI1;
	switch (sc->transfer){
	case DMA_TFR:
			dma_isr(sc, cp);
			/* dma_isr may leave a BO interrupt pending */
	case BO_END_TFR:
			TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
			break;
	case FHS_TFR:
			sc->state &= ~(FHS_ACTIVE | FHS_TIMED_OUT);
			
			/* Note: it may be possible to remove this resid update */
			sc->resid = sc->count;
			break;
	case INTR_TFR:
			intr_end(sc, 0);
			break;
	case NO_TFR:
			break;
	case BUS_TRANSACTION:
			if (sc->state & WAIT_BO) {
				sc->state &= ~WAIT_BO;
				TI9914_ruptmask(sc, TI_S_BO, INT_OFF);
			}
			if (sc->state & WAIT_BI) {
				sc->state &= ~WAIT_BI;
				TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
			}
			break;
	case BI_DMA_TFR:
			TI9914_ruptmask(sc, TI_S_BI, INT_OFF);
			break;
	default:
			panic("ti9914: Unrecognized TI9914 abort type");
	}
	sc->transfer = NO_TFR;
	i_am_controller = sc->state & ACTIVE_CONTROLLER;

	/* begin software reset */
	cp->auxcmd = H_SWRST1;
	cp->auxcmd = H_STDL1;
	cp->auxcmd = H_VSTDL1;
	cp->intmask0 = 0;
	cp->intmask1 = 0;
	cp->iostatus = ~(TI_DMA_0 | TI_DMA_1);
	sc->state &= ~IN_HOLDOFF;
	sc->intcopy = TI_M_BO;
	cp->auxcmd = H_HDFA1;
	cp->auxcmd = H_HDFE0;
	cp->auxcmd = H_SHDW0;
	cp->auxcmd = H_SWRST0;
	dummy = cp->datain;

	if (i_am_controller)
		cp->auxcmd = H_RQC; /* get control back */

	if (sc->state & STATE_SAVED)
		TI9914_restore_state(sc);
	else {
		sc->intmsksav = 0;
		sc->intmskcpy = 0;
		TI9914_ruptmask(sc, TI_S_UCG, INT_ON);
	}
	cp->auxcmd = H_DAI0;		/* enable interrupts */
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
struct dma_chain *TI9914_dma_setup(bp, dma_chain, entry)
struct buf *bp;
register struct dma_chain *dma_chain;
int entry;
{
	register struct isc_table_type *sc = bp->b_sc;
	register unsigned int dil_state = bp->b_queue->dil_state;
	int reading = bp->b_flags & B_READ;
	struct dil_info *dil_info = (struct dil_info *)bp->dil_packet;

	switch (entry) {
	case FIRST_ENTRIES:
		dma_chain->arm = DMA_ENABLE | DMA_LVL7;
		dma_chain->card_byte = reading ? H_HDFE1 : H_HDFE0;
		dma_chain->card_reg = (caddr_t) &((struct TI9914 *)sc->card_ptr)->auxcmd;
		break;
	case LAST_ENTRY:
		if (dma_chain->count != 1-1) {
			*(dma_chain+1) = *dma_chain;	/* first, just copy */
			dma_chain->count -= 1;	/* and adjust count */
			dma_chain++;			/* point to next one */
			dma_chain->buffer += dma_chain->count << (sc->state & F_WORD_DMA);
			dma_chain->count = 1-1;	/* one transfer */
		}
		dma_chain->card_byte = reading ? H_HDFA1 : ((!(bp->b_flags & B_DIL)) || 
		((dil_state & EOI_CONTROL) && (dil_info->full_tfr_count <= MAXPHYS))) 
				? H_FEOI : H_HDFE0;
		dma_chain->arm -= DMA_LVL7 - ((sc->int_lvl - 3) << 4);
		(dma_chain+1)->level = sc->int_lvl;
		break;
	default:
		panic("ti9914: ti9914_dma_setup: unknown dma setup entry");
	}
	return(dma_chain);
}

/***************************TI9914_set_addr*****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SET_ADDR) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs sc, pass_data1)
**
**	Global variables/fields referenced: bp->b_sc, sc->card_ptr, 
**		bp->pass_data1
**
**	Global vars/fields modified:  sc->my_address
**
**	Registers modified:  address_reg
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_set_addr(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	sc->my_address = bp->pass_data1 & 0x1F;
	cp->address_reg = sc->my_address;
}

/***************************TI9914_atn_ctl****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_ATN) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp
**
**	Kernel routines called:  TI9914_atn, TI9914_bo
**
**	Global variables/fields referenced:  bp->pass_data1, bp->b_sc,
**		sc->card_ptr, bp
**
**	Registers modified:  auxcmd
**
**	Algorithm:  [?? if needed]
************************************************************************/
TI9914_atn_ctl(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct TI9914 *cp = (struct TI9914 *) sc->card_ptr;

	if (bp->pass_data1 == 0) {
		/* drop atn */
		cp->auxcmd = H_GTS;
	} else {
		/* set atn */
		TI9914_atn(sc);
		TI9914_bo(bp);
	}
}

TI9914_savecore_xfer (bp)
struct buf *bp;
{
	register struct TI9914 *cp = (struct TI9914 *) bp->b_sc->card_ptr;
	register caddr_t addr;
	register int i;
	extern int physmembase, dumpsize;
	extern caddr_t dio2_map_page();
	long page, pagecnt = 0;

        cp->auxcmd = H_DAI1;    /* disable card from interrupting */
	cp->auxcmd = H_GTS;	/* off ATN */

	for (page = physmembase; pagecnt < dumpsize; 
	     pagecnt++, page++) {
/*
 * Map in the page of physical memory so it can be referenced
 */
		for (addr = dio2_map_page((caddr_t)(page << PGSHIFT)),
		     i = 0; i < (NBPG - (pagecnt == (dumpsize - 1))); i++) {

			cp->dataout = *addr++;  /* write out byte */
			TI9914_bo(bp);		/* wait for byte out */
		}
	}

	cp->auxcmd = H_FEOI;	/* set EOI */
	cp->dataout = *addr;	/* do the last byte */
	TI9914_bo(bp);		/* wait until the last byte is sent */
}

TI9914_nop()
{
	return(0);
}



/*  Table of routines for drivers to use */

struct drv_table_type ti9914_iosw = {
	TI9914_init,
	TI9914_identify,
	TI9914_clear_wopp,
	TI9914_clear,
	TI9914_nop,		/* used to be TI9914_ifc */
	TI9914_nop,		/* used to be TI9914_isr */
	TI9914_transfer,
	TI9914_pplset,
	TI9914_pplclr,
	TI9914_ppoll,
	TI9914_spoll,
	TI9914_preamb,
	TI9914_postamb,
	TI9914_mesg,
	TI9914_highspeed,
	TI9914_abort,
	TI9914_ren,
	TI9914_send_cmd,
	TI9914_dil_abort,
	TI9914_ctlr_status,
	TI9914_set_ppoll_resp,
	TI9914_srq,
	TI9914_save_state,
	TI9914_restore_state,
	TI9914_iocheck,
	TI9914_dil_reset,
	TI9914_status,
	TI9914_pass_control,
	TI9914_abort_io,
	TI9914_nop,		/* used to be TI9914_dil_init */
	TI9914_wait_set,
	TI9914_wait_clear,
	TI9914_intr_set,
	TI9914_dma_setup,
	TI9914_set_addr,
	TI9914_atn_ctl,
	TI9914_reg_access,
	TI9914_preamb_intr,
	TI9914_nop,
	TI9914_savecore_xfer	/* handle savecore FHS transfer */
};


/*
** linking and inititialization routines
*/

extern int (*make_entry)();

int (*ti9914_saved_make_entry)();

extern int (*fhs_timeout_proc)();
int fhs_timeout_exit();


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
ti9914_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	register struct TI9914 *cp = (struct TI9914 *) isc->card_ptr;
	unsigned char ppoll_enabled = 0;
	extern int processor;
	int ret_val;

	if (id == -1) {
		isc->iosw = &ti9914_iosw;
		isc->card_type = INTERNAL_HPIB;
		fhs_timeout_proc = fhs_timeout_exit;

		if (dma_here == 3) {
			isc->state = BOBCAT_HPIB;
			ppoll_enabled = 1;
			cp->ppoll_mask = 0;
		} else {
			/* test to see if this is an internal BOBCAT 10 */
			cp->ppoll_int = 0;	/* clear out ints */
			cp->ppoll_mask = 0xaa;	/* write mask value */
			cp->ppoll_int = 0;	/* delay for BOBCAT */
			if ((cp->ppoll_mask == 0xaa) && (processor == M68010)) {
				/* This is a BOBCAT 10 */
				isc->state = BOBCAT_HPIB;
				ppoll_enabled = 1;
				cp->ppoll_mask = 0;
			} else
				isc->state = 0;
		}

		if ((cp->extstatus & 0x80) == 0)
			ret_val = io_inform("Internal HP-IB Interface - non-system controller", isc, 3);
		else
			ret_val = io_inform("Internal HP-IB Interface - system controller", isc, 3);
		if (ppoll_enabled)
			msg_printf("        Parallel poll interrupts enabled.\n");
		return ret_val;
	} else if (id == 1) {
		isc->iosw = &ti9914_iosw;
		isc->card_type = HP98624;
		fhs_timeout_proc = fhs_timeout_exit;
		isc->state = 0;
		if ((cp->extstatus & 0x80) == 0)
			return io_inform("HP98624 HP-IB Interface - non-system controller", isc, 3);
		else
			return io_inform("HP98624 HP-IB Interface - system controller", isc, 3);
	} else
		return (*ti9914_saved_make_entry)(id, isc);
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
ti9914_link()
{
	ti9914_saved_make_entry = make_entry;
	make_entry = ti9914_make_entry;
        TI9914_link_called = 1;
}
