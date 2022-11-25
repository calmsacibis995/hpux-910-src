/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/parallel.c,v $
 * $Revision: 1.5.84.7 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/22 13:19:24 $
 */

/* HPUX_ID: @(#)parallel.c	55.1		88/12/23 */

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


********************************************************************************
********************************************************************************
  
	HP986PP Parallel Printer Interface Driver

     This is the interface driver for  the  HP986PP  Parallel  Printer
     interface.   It  is  a Centronics compatible bi-directional 8 bit
     parallel interface.  This interface is intended to be a low cost,
     general  purpose  output interface for HP and 3rd party printers,
     plotters  and  other  output  devices.   The  interface  is  also
     designed   to   support   the   bi-directional  ScanJet  parallel
     interface.

     Due to the relatively low data transfer rates (it is  anticipated
     that  Centronics  devices  will  not  exceed  100 Kbytes/sec) the
     interface only supports 8 bit data transfers.  DMA transfers  are
     supported.

     This driver  provides  a  general  purpose  interface  to  device
     drivers  via  the  iosw entry in the isc_table_type.  This driver
     supports poweron linking and initialization, data transfers,  and
     device  I/O.   Note  that  many  routines  assume  pre-conditions
     garanteed by the caller, so caller beware!

     This module is divided into four major sections, poweron  linking
     and initialization, data transfer support, interrupt service, and
     miscellaneous iosw support.

  
********************************************************************************
********************************************************************************
*/

#include "../h/debug.h"
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
#include "../s200io/parallel.h"
#include "../wsio/dilio.h"

#define BUSY_END_TFR BURST_TFR
#define PENDING_ACK     SYSTEM_CONTROLLER
#define PC_HANDSHK_TFR BUS_TRANSACTION
#define CONTINUE_TRANSFER       0
#define TRANSFER_COMPLETED      1
#define MAXPHYS (64 * 1024)


/*
********************************************************************************
********************************************************************************
  
	Powerup Linking and Initialization
  
********************************************************************************
********************************************************************************
*/

/*
** The following routine are exported to upper level device drivers via the iosw
*/
int parallel_init();
int parallel_transfer();
int parallel_save_state();
int parallel_restore_state();
int parallel_iocheck();
int parallel_reset();
int parallel_dil_reset();
int parallel_abort();
int parallel_intr_set();
struct dma_chain * parallel_dma_setup();
int parallel_reg_access();

/* stub for unimplemented services in the iosw */
int parallel_nop() { }

/* for forward references */
int parallel_isr();
int parallel_do_isr();

/*
** allocate and initialize the iosw
*/

struct drv_table_type parallel_iosw = {
	parallel_init,		/* iod_init() 		*/
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_transfer,	/* iod_tfr()		*/
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_save_state,	/* iod_save_state()	*/
	parallel_restore_state,	/* iod_restore_state()	*/
	parallel_iocheck,	/* iod_io_check()	*/
	parallel_dil_reset,	/* iod_dil_reset()	*/
	parallel_nop,
	parallel_nop,
	parallel_abort,		/* iod_abort_io()	*/
	parallel_nop,
	parallel_nop,
	parallel_nop,
	parallel_intr_set,	/* iod_intr_set()	*/
	parallel_dma_setup,	/* iod_dma_setup()	*/
	parallel_nop,
	parallel_nop,
	parallel_reg_access,	/* iod_reg_access()	*/
	parallel_nop,
	parallel_nop
};

/* linking declarations */
extern int (*make_entry)();
int (*parallel_saved_make_entry)();


/*******************************************************************************
**
**   parallel_make_entry
**
**     This routine offers driver services to Parallel Printer interface
**     cards.   It  is  called once at powerup for every interface found
**     scanning the backplane.  If the interface is not recognized  then
**     it  is  passed on to the next driver in the make entry chain.  If
**     it is recognized then  the  iosw  is  setup,  the  card  type  is
**     initialized,  and  the  system  is notified that this driver will
**     service the calling interface.
**
**	Called by:  
**	   kernel_initialize (see s200io/kernel.c) via make_entry
**
**	Parameters:
**	   id  - value of id register for interface at this select code
**	   sc  - pointer to the isc_table_type for this select code
**
**	Return value:
**         If the interface is not recognized then a call to the next
**         driver in the system driver make entry chain is returned.
**
**         If the interface is recognized then a call to io_inform is 
**         returned notifying the system of acceptance of the card.
**
**	Kernel routines called:
**         parallel_saved_make_entry()  in this file
**         io_inform()                  in s200io/kernel.c
**
*******************************************************************************/

parallel_make_entry(id, sc)
int id;
struct isc_table_type *sc;
{
	/* we only service Parallel Printer interface cards */
	if (id != PARALLEL_PRINTER_INTERFACE_ID)
		return (*parallel_saved_make_entry)(id, sc);

	/* setup the iosw now so the system can call out powerup init routine */
	sc->iosw = &parallel_iosw;

	/* save the card type */
	sc->card_type = HP986PP;

	/* 
	** let the system know we will service this card 
	** at whatever interrupt level it is set at
	*/
	return io_inform("Parallel Interface", sc, sc->int_lvl);
}


/*******************************************************************************
**
**   parallel_link
**
**     This routine is called once at powerup to configure  this  driver
**     into  the  system.  The driver must save a pointer to the head of
**     the make entry chain and then insert itself at the head  of  this
**     chain.   The  system calls each interface drivers link routine to
**     build a system chain of  interface  driver  make_entry  routines.
**     This  list  represents  the interface drivers configured into the
**     system.
**
**	Called by:  
**	   link_drivers (see mach.300/machdep.c)
**
**	Global variables accessed:
**         parallel_saved_make_entry
**         make_entry
**
*******************************************************************************/

parallel_link()
{
	/* save the head of the list */
	parallel_saved_make_entry = make_entry;

	/* put our routine at the head of the list */
	make_entry = parallel_make_entry;
}


/*******************************************************************************
**
**   parallel_init
**
**     This routine does poweron initialization.  It is called  once  at
**     powerup  for  each  parallel interface found in the system.  This
**     routine must not be called more than once  per  interface  or  it
**     will  screw  up  the  interrupt  service  routine  chain  at that
**     interfaces interrupt level.
**
**     Both the parallel bus and the interface are reset.  The INIT line
**     is  driven low on the bus for 50 us. and the reset line is yanked
**     in the  interface  card.   Necessary  isc_table_type  fields  are
**     initialized.   Finally, an interrupt service routine is installed
**     for the interface.
**
**      Called by: 
**	   kernel_initialize (see s200io/kernel.c) via the iosw (iod_init)
**
**      Parameters:  
**	   sc - pointer to the isc_table_type for this select code
**
**      Kernel routines called:
**         isrlink()                  in mach.300/machdep.c
**         parallel_reset()           in this file
**
*******************************************************************************/

parallel_init(sc)
register struct isc_table_type *sc;
{
        struct parallel_if *cp = (struct parallel_if *)sc->card_ptr;

        /* initialize select code fields */
        sc->locking_pid = 0;
        sc->locks_pending = 0;
        sc->parallel_rptmask = 0;
        sc->intr_wait = 0;
        sc->b_actf = NULL;
        sc->b_actl = NULL;
        sc->event_f = NULL;
        sc->event_l = NULL;
        sc->owner = NULL;
        sc->transfer = NO_TFR;
        sc->state = 0;

        /* reset the hardware */
        parallel_reset(sc);

        /* install our interrupt service routine */
        isrlink(parallel_isr, sc->int_lvl, &cp->dio_control, 0xc0, 0xc0,
                                                                0, (int)sc);
}


/*
********************************************************************************
********************************************************************************
  
	Data Transfer Routines
  
********************************************************************************
********************************************************************************
*/


/*******************************************************************************
**
**   parallel_save_state
**
**     This routine disables interrupts on the  interface.   We  do  not
**     have  to  save  the  interrupt  mask  as we  have a copy of it in
**     sc->intr_wait.  This routine is typically  called  in preparation
**     for data transfers.  Since we will  allow  interrupts during data
**     transfers we will not clear the interrupt mask.   Interrupts  are
**     disabled  here,  they  should  be  enabled  when  a  transfer  is
**     initiated.  It is assumed that the caller owns the select code.
**
**	Called by: 
**	   device drivers via the iosw (iod_save_state)
**
**	Parameters:  
**	   sc	- pointer to the isc_table_type for this select code
**
**	Interface registers:	
**	   dio_control
**
*******************************************************************************/

parallel_save_state (sc)
register struct isc_table_type *sc;
{
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;

	/* indicate that we are done */
	sc->state |= STATE_SAVED;
}


/*******************************************************************************
**
**   parallel_restore_state           
**
**     This routine restores the interface interrupt mask  as  saved  in
**     sc->intr_wait and then enables interrupts on the interface.  This
**     routine is typically called after data transfers to restore  user
**     interrupt  requests.   It  is  assumed  that  the caller owns the
**     select code.
**
**      Called by:
**         device drivers via the iosw (iod_restore_state)
**
**      Parameters:
**         sc   - pointer to the isc_table_type for this select code
**
**	Kernel routines called:
**         parallel_rupt_mask()  	in this file
**
**      Interface registers:
**         interrupt_control, dio_control
**
*******************************************************************************/

parallel_restore_state(sc)
register struct isc_table_type *sc;
{
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;

	/* disable the card from interrupting */
	cp->dio_control &= ~PARALLEL_INT_ENABLE;

	/*disable all interrupts */
	parallel_ruptmask(sc, 0xFF, INT_OFF);

	/* restore user interrupts */
	parallel_ruptmask(sc, (sc->intr_wait >> 8) & 0xFF, INT_ON);

	/* indicate that we are done */
	sc->state &= ~STATE_SAVED;

	/* enable the card */
	cp->dio_control |= PARALLEL_INT_ENABLE;
}


/*******************************************************************************
**
**   parallel_iocheck
**
**     This routine tests for any device  specific  pre-conditions  that
**     must  be met prior to a data transfer.  For this driver there are
**     none.
**
**	Called by:  
**         device drivers via the iosw (iod_iocheck)
**
**	Parameters:
**	   bp - pointer to the buf describing this I/O request
**
**	Return value:
**         Possible return  values for iod_iocheck are TRANSFER_WAIT meaning
**         wait for a device specific event to occur, TRANSFER_ERROR meaning
**         that  there  is a problem with the I/O request, or TRANSFER_READY
**         meaning  that  the  interface  is  ready  to do the data transfer
**         specified by bp.
**
*******************************************************************************/

parallel_iocheck(bp)
register struct buf *bp;
{
	/* we are always ready! */
	return TRANSFER_READY;
} 


/*******************************************************************************
**
**    parallel_transfer
**
**     This routine initiates data transfers  on  the  parallel  printer
**     bus.   Three  methods  of  data transfer are supported, DMA, fast
**     handshake, and interrupt per byte.
**
**     First the transfer method is determined.  Interrupt per  byte  is
**     used  if  the user requested pattern matching or if interrupt per
**     byte transactions were  requested  via  io_speed_ctl().   If  the
**     buffer  is  too  small we won't bother with DMA.  Also, if we are
**     writing and the count is less than  the  outbound  fifo  size  we
**     won't bother with DMA.
**
**     Once the transfer method has been determined,  the  interface  is
**     prepared for I/O.  The SELECTIN line is set, the direction of the
**     transfer in setup, and error  interrupts  are  enabled.   If  the
**     transfer  method  is DMA then the transfer chain is built and DMA
**     is initiated.  For interrupt  or  fast  handshake  transfers  the
**     appropriate interrupting conditions are enabled.
**
**     It is assumed that the caller owns the select code.  The  routine
**     proc is setup to be called on I/O completion.
**
**	Called by:
**         device drivers via the iosw (iod_tfr)
**
**	Parameters:
**	   request - requested transfer type 
**	   bp      - pointer to the buf describing this I/O request
**	   proc    - pointer to procedure to call on I/O completion
**
**	Kernel routines called:
**         VASSERT()                  in h/debug.h
**         parallel_rupt_mask()	      in this file
**         try_dma()                  in s200io/dma.c
**         dma_build_chain()          in s200io/dma.c
**         start_dma()                in s200io/dma.c
**         panic()                    in sys/subr_prf.c
**
**      Interface registers:
**         ext_control, interrupt_control, dio_control
**
*******************************************************************************/

parallel_transfer(request, bp, proc)
enum transfer_request_type request;	/* unused */
register struct buf *bp;
int (*proc)();
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;
	register int state = iob->dil_state;
	register struct dma_channel *dma;

	VASSERT(sc->owner == bp);
	VASSERT(sc->transfer == NO_TFR);

	/* disable the card for now */
	cp->dio_control &= ~PARALLEL_INT_ENABLE;

	bp->b_action = proc;

	/*
	**  For speed during interrupt and fast handshake transfers
	*/
	sc->iob_dil_state = state;
	sc->iob_read_pattern = iob->read_pattern;
	sc->bp_b_flags = bp->b_flags;

	/* select type of transfer */
	if (state & PC_HANDSHK)
		sc->transfer = FHS_TFR;
	else if ((state & READ_PATTERN) || (state & USE_INTR))
			sc->transfer = INTR_TFR;
	else {
		if ((bp->b_bcount <= 32) ||
		    (bp->b_flags & B_READ) ||
		    (!(try_dma(sc, MAX_SPEED))))  
			sc->transfer = FHS_TFR;
		else
			sc->transfer = DMA_TFR;
	}

	/* get the buffer address and the count */
	sc->buffer = bp->b_queue->b_xaddr;
	sc->count = bp->b_queue->b_xcount;

	/* prepare the parallel card */
	cp->ext_control |= PARALLEL_SELECTIN;

	if (bp->b_flags & B_READ) {
		/* the order of the next 4 statements is critical */
		cp->dio_control |= PARALLEL_IO;
		/* enable nack interrupt for input - don't miss leading edge! */
		parallel_ruptmask(sc, PARALLEL_ACK_EI, INT_ON);
		if (sc->transfer == INTR_TFR)
                        /*
                        ** Set the slow bit so we will get an interrupt on
                        ** every byte.  This is primarily for devices that
                        ** do not assert NACK to indicate the end on
                        ** inbound data transfers
                        */
			cp->dio_control |= PARALLEL_SLOW;
		cp->ext_control &= ~PARALLEL_WRRD;

	} else {
		/* the order of the next 2 statements is critical */
		cp->ext_control |= PARALLEL_WRRD;
		cp->dio_control &= ~PARALLEL_IO;

                /**
		 **  If we are using only BUSY and NSTROBE for handshaking 
		 **  set the slow bit.  This is for devices that do not
		 **  use NACK to indicate when they are ready for the next 
		 **  byte.
		 **/
		if( iob->dil_state & NO_NACK ) {
			sc->transfer = INTR_TFR;
			cp->dio_control |= PARALLEL_SLOW;
		}
	}

	switch(sc->transfer) {
		case DMA_TFR:
			/*
			** Kick off a dma transfer.
			*/
			dma = sc->dma_chan;

			/* build the transfer chain */
			dma_build_chain(bp);

			/* have dma card call us when it is finished */
			dma->isr = parallel_do_isr;
			dma->arg = (int) sc;

			/* 
			** The hardware requires this.  It will eventually
			** lock up if this bit isn't cleared.
			*/
			if (bp->b_flags & B_READ)
				cp->dio_control &= ~PARALLEL_IO;

			/* start the dma transfer */
			dma_start(dma);
			break;

		case INTR_TFR:
		case FHS_TFR:
                        /* 
                        ** If dma is reserved and we're here, we decided
                        ** not to use it, so give it back 
                        */
                        if (iob->b_flags & RESERVED_DMA)
                                drop_dma(sc->owner);

			/* 
			** enable fifo boundry condition interrupt
			*/
			if (bp->b_flags & B_READ) {
				if (sc->state & PENDING_ACK)
					return parallel_fakeisr(sc);
				parallel_ruptmask(sc, PARALLEL_FIFO_FULL_EI, 
									INT_ON);
			} else
				parallel_ruptmask(sc, PARALLEL_FIFO_EMPTY_EI, 
									INT_ON);
			break;

		default:
			panic("parallel_transfer: transfer of unknown type");
	}

	/* enable interrupts */
	cp->dio_control |= PARALLEL_INT_ENABLE;
}


/*******************************************************************************
**
**   parallel_dma_setup
**
**     This routine sets up interface specific info  in  the  dma  chain
**     structure.  It is called twice by dma_build_chain, first to setup
**     the card specific fields for all but the last  dma  chain  entry,
**     and  then  once  again to setup the last entry.  This driver will
**     chain at level 7 and then interrupt at the interrupt level of the
**     interface for the last entry.
**
**     This routine assumes that the caller owns  the  select  code  and
**     that the select code owns a DMA channel.
**
**	Called by:  
**	   dma_build_chain (see s200io/dma.c) via the iosw (iod_dma_setup)
**
**	Parameters:
**	   bp        - pointer to the buf describing this I/O request
**	   dma_chain - pointer to the dma chain being setup
**	   entry     - flag indicating which entries we are doing
**
**	Return value:
**         This routine returns a pointer to the modified dma chain structure.
**
**	Kernel routines called:
**         VASSERT()     in h/debug.h
**         panic()                    in sys/subr_prf.c
*******************************************************************************/

struct dma_chain *parallel_dma_setup(bp, dma_chain, entry)
struct buf *bp;
register struct dma_chain *dma_chain;
int entry;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;

	VASSERT(sc->owner == bp);
	VASSERT(sc->dma_chan != NULL);

	switch (entry) {
	case FIRST_ENTRIES:
		dma_chain->card_byte = cp->dio_control & 0xFC;
		dma_chain->card_byte |= 
                           (sc->dma_chan->card == (struct dma *) DMA_CHANNEL0) ?
				     PARALLEL_DMA_0 : PARALLEL_DMA_1;
		dma_chain->card_byte |= PARALLEL_INT_ENABLE;
		dma_chain->card_reg = 
                  (caddr_t) &((struct parallel_if *)sc->card_ptr)->dio_control;
		dma_chain->arm = DMA_ENABLE | DMA_LVL7;
		break;

	case LAST_ENTRY:
		/*
		** last transfer adjustments
		*/
		dma_chain->card_byte |= PARALLEL_INT_ENABLE;
		/* drop the dma int level to the int level of the interface */
		dma_chain->arm -= DMA_LVL7 - ((sc->int_lvl - 3) << 4);
		(dma_chain+1)->level = sc->int_lvl;
		break;

	default:
		panic("parallel_dma_setup: unknown dma setup entry");
	}

	return(dma_chain);
}


/*
********************************************************************************
********************************************************************************
  
	Interrupt Service Routines
  
********************************************************************************
********************************************************************************
*/

int parallel_do_isr();

/*******************************************************************************
**
**   parallel_pc_handshk_isr
**
**      Called by:
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**         cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER
**
**      Kernel routines called:
**
*******************************************************************************/

parallel_pc_handshk_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
	/* make sure we have fifo empty */
	if (!(cp->bus_status & PARALLEL_FIFO_EMPTY))
		return CONTINUE_TRANSFER;

	/* make sure STROBE is low */
	while (!(cp->bus_status & PARALLEL_STROBE));

	/* reset the interface card */
	cp->reset = 1;

	/* enable the ACK interrupt */
	cp->interrupt_control = PARALLEL_ACK_EI | PARALLEL_FIFO_EMPTY_EI;
	/* enable interrupts */
	cp->dio_control |= PARALLEL_INT_ENABLE;

	/* reset the transfer type */
	sc->transfer = FHS_TFR;
					
	if (sc->count)
		return CONTINUE_TRANSFER;
	else
		return parallel_output_end_isr(sc, cp);
}
/*******************************************************************************
**
**   parallel_busy_end_isr
**
**     This is the interrupt service common to  the  completion  of  all
**     transfers.   It  waits  for  the interface to clear the busy line
**     before indicating that the transfer is completed.
**
**      Called by:
**         parallel_fhs()                 in this file
**         parallel_dma_isr()             in this file
**         parallel_intr_isr()            in this file
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**         cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER if the transfer
**         should be continued or TRANSFER_COMPLETED if I/O was completed.
**
**      Kernel routines called:
**         VASSERT()     	      in h/debug.h
**         parallel_rupt_mask()	      in this file
**
*******************************************************************************/

parallel_busy_end_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
	/*
	** check to see if I/O has completed
	*/

	/* don't miss the leading edge */
	parallel_ruptmask(sc, PARALLEL_BUSY_EI, INT_ON);

	/* check for busy status */
	if (cp->bus_status & PARALLEL_BUSY) {
		/*
		** The bus is still busy so wait for an interrupt
		*/
		sc->transfer = BUSY_END_TFR;
		cp->dio_control |= PARALLEL_INT_ENABLE;

		return CONTINUE_TRANSFER;
	} else {
		/*
		** The bus is no longer busy so we know that all our
		** bytes have gotten out accross the bus.  Indicate that
		** a transfer is no longer in progress and disable the
		** busy interrupt.
		*/
		sc->transfer = NO_TFR;
		parallel_ruptmask(sc, PARALLEL_BUSY_EI, INT_OFF);

		return TRANSFER_COMPLETED;
	}
}

/*******************************************************************************
**
**   parallel_ack_isr
**
**     This  is  the  interrupt  service  routine  for   handeling   ACK
**     interrupts during inbound transfers.  Some peripherals assert ACK
**     to indicate the  end  of  a  transfer.  This  routine  is  called
**     whenever  one  of the inbound transfer routines detects a pending
**     ACK interrupt.
**
**     To service the ACK interrupt, the inbound data fifo  is  emptied,
**     pattern   matching   is   done  if  necessary,  transfer  related
**     interrupts are cleared, the resid  and  termination  reasons  are
**     updated, and the transfer is marked as completed.  Note that this
**     routine always returns TRANSFER_COMPLETED.
**
**      Called by:
**         parallel_fhs()                 in this file
**         parallel_dma_isr()             in this file
**         parallel_intr_isr()            in this file
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**         cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value TRANSFER_COMPLETED.
**
**      Kernel routines called:
**         VASSERT()     in h/debug.h
**
*******************************************************************************/

parallel_ack_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
        register unsigned char data = 0;
	register term_reason = 0;
	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;

       /* make sure the inbound data fifo is empty */
	while (!(cp->bus_status & PARALLEL_FIFO_EMPTY) && sc->count) {

                /* transfer a byte to temp buf for pattern matching */
                data = cp->fifo;

                /* stick it in the I/O buffer */
                *sc->buffer++ = data;

                /* update the count */
                sc->count--;

                /* see if we need to do pattern matching */
                if ((sc->tfr_control&READ_PATTERN) && (data == sc->iob_read_pattern)) {
			/* set the termination reason for DIL */
			term_reason |= TR_MATCH;
			break;
		}
	}

	/* clear transfer related interrupts */
	parallel_ruptmask(sc, PARALLEL_ACK_EI | PARALLEL_FIFO_FULL_EI, INT_OFF);

	/* save number of bytes that didn't get transfered */
	sc->resid = sc->count;

	/* set the termination reasons for DIL */
	if (cp->bus_status & PARALLEL_FIFO_EMPTY) {
		term_reason |= TR_END;
		sc->state &= ~PENDING_ACK;
	} else {
		sc->state |= PENDING_ACK;
	}
	if ((sc->count == 0) && (dil_info->full_tfr_count <= MAXPHYS)) 
		term_reason |= TR_COUNT;
	sc->tfr_control = term_reason;

	sc->transfer = NO_TFR;

	return TRANSFER_COMPLETED;
}


/*******************************************************************************
**
**   parallel_output_end_isr
**
**     This is the interrupt service common to  the  completion  of  all
**     outbound  transfers.   The resid and termination reasons are set.
**     If the outbound fifo is not empty we will wait for a  fifo  empty
**     interrupt to complete the transfer.
**
**      Called by:
**         parallel_fhs()                 in this file
**         parallel_dma_isr()             in this file
**         parallel_intr_isr()            in this file
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**         cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER if the transfer
**         should be continued or TRANSFER_COMPLETED if I/O was completed.
**
**      Kernel routines called:
**         VASSERT()     in h/debug.h
**
*******************************************************************************/

parallel_output_end_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
	/*
	** if we got here, then we have satisfied our count
	*/
	sc->resid = 0;

	/*
	** check to see if I/O has completed
	*/

	/* don't miss the leading edge */
	parallel_ruptmask(sc, PARALLEL_FIFO_EMPTY_EI, INT_ON);

	/* check for fifo empty status */
	if (cp->bus_status & PARALLEL_FIFO_EMPTY) {
		/*
		** The outbound fifo is empty so we know that all our
		** bytes have gotten out accross the bus.  Disable the
		** fifo empty interrupt and wait for busy to clear.
		*/
		parallel_ruptmask(sc, PARALLEL_FIFO_EMPTY_EI, INT_OFF);

		/* wait for the interface to drop busy */
		return parallel_busy_end_isr(sc, cp);
	}

	/*
	** There is still something in the outbound fifo so
	** wait for a fifo empty interrupt to complete I/O.
	*/
	sc->transfer = BO_END_TFR;
	cp->dio_control |= PARALLEL_INT_ENABLE;

	return CONTINUE_TRANSFER;
}


/*******************************************************************************
**
**   parallel_fhs_isr
**
**     This is the interrupt service  routine  for  the  fast  handshake
**     transfer  method.   It  is  called  whenever  the  interface card
**     interrupts and the transfer type is fast handshake.
**
**     Only  interrupting  conditions  related  to  the   transfer   are
**     serviced.   On  input  transfers the fifo full and ACK interrupts
**     are serviced.  On output transfers the fifo  empty  interrupt  is
**     serviced.  Other interrupting conditions are ignored.
**
**     Once this routine is entered, data  bytes  are  transfered  until
**     either  the count is satisfied or the fifo is exhausted (empty on
**     input/full on output).  An input transfer is  terminated  if  the
**     ACK  line  is  asserted  or  if  we  came  in  here due to an ACK
**     interrupt.
**
**     This routine does not do pattern matching.   Transfer  status  is
**     returned  to  the  caller  indicating  whether  the  transfer has
**     completed or should be continued.
**
**      Called by:
**         parallel_do_isr()  		in this file
**
**      Parameters:
**	   sc - pointer to the isc_table_type for this select code
**	   cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER if the transfer
**         should be continued or TRANSFER_COMPLETED if I/O was completed.
**
**      Kernel routines called:
**         VASSERT()                        in h/debug.h
**         parallel_ack_isr()               in this file
**         parallel_output_end_isr()        in this file
**
*******************************************************************************/

parallel_fhs_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
        register unsigned char data = 0;

	VASSERT(sc->transfer == FHS_TFR);
	VASSERT(sc->owner != NULL);

        if (sc->bp_b_flags & B_READ) {
		struct dil_info *dil_info = 
				(struct dil_info *)sc->owner->dil_packet;

		/* empty the inbound data fifo */
		while (!(cp->bus_status & PARALLEL_FIFO_EMPTY) && sc->count) {

			/* transfer a byte */
			*sc->buffer++ = cp->fifo;

			/* update the count */
			sc->count--;
		}

		if ((cp->interrupt_status & PARALLEL_ACK_IR) || 
		    (sc->state & PENDING_ACK))
			return parallel_ack_isr(sc, cp);

                if (sc->count)
                        return CONTINUE_TRANSFER;

		
		/* 
		** if we got here, then we have satisfied our count
		*/
		sc->resid = 0;

		/* this is the termination reason for DIL */
		if (dil_info->full_tfr_count <= MAXPHYS) 
			sc->tfr_control = TR_COUNT;

		sc->transfer = NO_TFR;

		/* clear transfer related interrupts */
		parallel_ruptmask(sc, PARALLEL_ACK_EI | PARALLEL_FIFO_FULL_EI, INT_OFF);

		return TRANSFER_COMPLETED;

	} else {

                /* output transfers */

		while (sc->count) {
			/*
			** If the fifo is full then there is no room for us
			** to write our data.  Either we have been through 
			** the loop enough time to full up the fifo or this 
			** is not our interrupt.  In either case we have not
			** exhausted our count so we indicate that the transfer 
			** should be continued.
			*/
			if (cp->bus_status & PARALLEL_FIFO_FULL)
				return CONTINUE_TRANSFER;

			/* transfer a byte */
			cp->fifo = *sc->buffer++;

			/* update the count */
			sc->count--;

			if (sc->iob_dil_state & PC_HANDSHK) {
				int patience = 5;

				/* wait for fifo empty */
				if (!(cp->bus_status & PARALLEL_FIFO_EMPTY)) {
					sc->transfer = PC_HANDSHK_TFR;
					return CONTINUE_TRANSFER;
				}
				/* wait for STROBE to go low */
				for (patience = 0; patience < 5; patience++) {
					if (cp->bus_status & PARALLEL_STROBE)
						break;
					snooze(10);
				}

				if (!(cp->bus_status&PARALLEL_STROBE)) {
					cp->reset = 1;
					return CONTINUE_TRANSFER;
				}

				/* reset the interface card */
				cp->reset = 1;

				/* enable the ACK interrupt */
				cp->interrupt_control = PARALLEL_ACK_EI | PARALLEL_FIFO_EMPTY_EI;
				                                
				/* wait for NACK */
				if (!(cp->interrupt_status & PARALLEL_ACK_IR)) {
					/* enable interrupts */
					cp->dio_control |= PARALLEL_INT_ENABLE;
					return CONTINUE_TRANSFER;
				}
			}
		}

		return parallel_output_end_isr(sc, cp);

        }
}


/*******************************************************************************
**
**   parallel_intr_isr
**
**     This is the interrupt service routine for the interrupt per  byte
**     transfer  method.   It  is  called  whenever  the  interface card
**     interrupts and the transfer type is interrupt per byte.
**
**     Only  interrupting  conditions  related  to  the   transfer   are
**     serviced.   On  input  transfers the fifo full and ACK interrupts
**     are serviced.  On output transfers the fifo  empty  interrupt  is
**     serviced.  Other interrupting conditions are ignored.
**
**     This routine does pattern matching for  device  I/O  support.  An
**     input  transfer  is   terminated  if  the ACK  line  is  asserted
**     or  if  we  came  in  here due to  an  ACK  interrupt.   Transfer
**     status  is returned to the caller indicating whether the transfer
**     has completed or should be continued.
**
**      Called by:
**         parallel_do_isr()  		in this file
**
**      Parameters:
**	   sc - pointer to the isc_table_type for this select code
**	   cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER if the transfer
**         should be continued or TRANSFER_COMPLETED if I/O was completed.
**
**      Kernel routines called:
**         VASSERT()                        in h/debug.h
**         parallel_ack_isr()               in this file
**         parallel_output_end_isr()        in this file
**
*******************************************************************************/

parallel_intr_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
        register unsigned char data = 0;

	VASSERT(sc->transfer == INTR_TFR);
	VASSERT(sc->owner != NULL);

        if (sc->bp_b_flags & B_READ) {
		struct dil_info *dil_info = 
				(struct dil_info *)sc->owner->dil_packet;
		/* input transfers */
		if ((cp->interrupt_status & PARALLEL_ACK_IR) ||
		    (sc->state & PENDING_ACK))
			return parallel_ack_isr(sc, cp);

		/* 
		** If the fifo is empty then there is no data for us
		** to read.  This is not our interrupt.  Indicate that
		** the transfer should be continued.
		*/
		if (cp->bus_status & PARALLEL_FIFO_EMPTY)
			return CONTINUE_TRANSFER;

		/* transfer a byte to temp buf for pattern matching */
		data = cp->fifo;

		/* stick it in the I/O buffer */
		*sc->buffer++ = data;

		/* update the count */
		sc->count--;

		/* see if we need to do pattern matching */
		if (sc->tfr_control & READ_PATTERN) {
			/* did we match? */
			if (data == sc->iob_read_pattern) {
				/*
				** Save the number of bytes that
				** didn't get transfered.
				*/
				sc->resid = sc->count;

				/* set the termination reason for DIL */
				sc->tfr_control = TR_MATCH;

				/* did we satisfy count too? */
				if ((sc->resid == 0) && 
					(dil_info->full_tfr_count <= MAXPHYS))
					sc->tfr_control |= TR_COUNT;

				sc->transfer = NO_TFR;

				/* clear transfer related interrupts */
				parallel_ruptmask(sc, PARALLEL_ACK_EI | PARALLEL_FIFO_FULL_EI, INT_OFF);

				return TRANSFER_COMPLETED;
			}
		}
		
		if (sc->count)
			return CONTINUE_TRANSFER;

		/* 
		** if we got here, then we have satisfied our count
		*/
		sc->resid = 0;

		/* this is the termination reason for DIL */
		if (dil_info->full_tfr_count <= MAXPHYS)
			sc->tfr_control = TR_COUNT;

		sc->transfer = NO_TFR;

		/* clear transfer related interrupts */
		parallel_ruptmask(sc, PARALLEL_ACK_EI | PARALLEL_FIFO_FULL_EI, INT_OFF);

		return TRANSFER_COMPLETED;

	} else {

                /* output transfers */

		/*
		** If the fifo is full then there is no room for us
		** to write our data.  This is not our interrupt.  
		** Indicate that the transfer should be continued.
		*/
		if (cp->bus_status & PARALLEL_FIFO_FULL)
			return CONTINUE_TRANSFER;

		/* transfer a byte */
		cp->fifo = *sc->buffer++;

		/* update the count */
		sc->count--;

		if (sc->count)
			return CONTINUE_TRANSFER;

		return parallel_output_end_isr(sc, cp);
        }
}


/*******************************************************************************
**
**   parallel_dma_isr
**
**     This is the  interrupt  service  routine  for  the  DMA  transfer
**     method.   It is called whenever the interface card or an attached
**     DMA channel interrupts and the transfer type is DMA. Note that on
**     outbound  transfers  there will be a race between the DMA channel
**     trying  to  deliver  it's  count  satisfied  interrupt  and   the
**     interface  trying  to  deliver  it's  fifo  idle interrupt.  This
**     routine must be prepared to handle both cases.
**
**     Only  interrupting  conditions  related  to  the   transfer   are
**     serviced.   On  input  the  ACK interrupt is serviced.  On output
**     transfers the fifo empty interrupt is serviced.  If we  got  here
**     due  to  a  DMA interrupt, then that interrupt is serviced by the
**     DMA channel isr.  Other interrupting conditions are ignored.
**
**     This routine terminates the current DMA transaction and  releases
**     DMA  resources.   An input transfer is terminated if the ACK line
**     is asserted or if we came in here due to an ACK interrupt. Output
**     transfers  are  not  considered complete until we have fifo empty
**     status indicating that all bytes have  actually  been  transfered
**     accross  the  bus.   If  we don't have fifo empty in this case we
**     will wait  for  a  fifo  empty  interrupt.   Transfer  status  is
**     returned  to  the  caller  indicating  whether  the  transfer has
**     completed or should be continued.
**
**      Called by:
**         parallel_do_isr()            in this file
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**         cp - pointer to the interface card
**
**      Return value:
**         This routine returns the value CONTINUE_TRANSFER if the transfer
**         should be continued or TRANSFER_COMPLETED if I/O was completed.
**
**      Kernel routines called:
**         parallel_ack_isr()               in this file
**         parallel_output_end_isr()        in this file
**         dmaend_isr()                     in s200io/dma.c
**         drop_dma()                       in s200io/dma.c
**
*******************************************************************************/

parallel_dma_isr(sc, cp)
register struct isc_table_type *sc;
register struct parallel_if *cp;
{
	register struct dma_channel *ch = sc->dma_chan;
	int dma_interrupted;
	extern unsigned short dma_chan0_last_arm;
	extern unsigned short dma_chan1_last_arm;
	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;

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
	** This isr only services dma_channel interrupts and NACK
	** interrupts.  Others are passed along for someone else
	** to service.  We will continue the transfer if its not
	** out interrupt.
	*/
	if (!dma_interrupted && 
	    !(cp->interrupt_status & PARALLEL_ACK_IR) && 
	    !(sc->state & PENDING_ACK))
		/* this is not our interrupt */
		return CONTINUE_TRANSFER;

	/* make sure dma is complete */
	if (!dma_interrupted)
		wait_for_dma(sc);

	/* clear up dma channel */
	dmaend_isr(sc);

	/* disable DMA for PARALLEL */
	cp->dio_control &= ~(PARALLEL_DMA_0 | PARALLEL_DMA_1);

	/* get rid of channel */
	drop_dma(sc->owner);

	
	if (!(sc->owner->b_flags & B_READ))
		/* finish off the outbound transfer */
		return parallel_output_end_isr(sc, cp);

	/* did we get an ACK interrupt? */
	if ((cp->interrupt_status & PARALLEL_ACK_IR) || 
	    (sc->state & PENDING_ACK)) {
		/* dmaend_isr set resid, update count for ack_isr */
		sc->count = sc->resid;
		return parallel_ack_isr(sc, cp);
	}

	/* clear transfer related interrupts */
	parallel_ruptmask(sc, PARALLEL_ACK_EI | PARALLEL_FIFO_FULL_EI, INT_OFF);

	/* this is the termination reason for DIL */
	if (dil_info->full_tfr_count <= MAXPHYS)
		sc->tfr_control = TR_COUNT;

	/* all done */
	sc->transfer = NO_TFR;
	return TRANSFER_COMPLETED;
}


/*******************************************************************************
**
**   parallel_isr
**
**     This routine is a  front  end  for  the  real  interrupt  service
**     routine,  parallel_do_isr().  It extracts the select code pointer
**     out of the interrupt structure and then  calls  parallel_do_isr()
**     to  do  the  work.  This is necessary because the polling code in
**     mach.300/locore.s  wants  to  call  the  isr  with  an  interrupt
**     structure  and the dma channel isr wants to call it with a select
**     code pointer.
**
**	Called by:  
**	   This routine is called by the interrupt polling code in 
**	   mach.300/locore.s whenever the parallel interface interrupts.
**
**	Parameters:
**         inf - pointer to the interrupt structure for this interrupt
**
**	Kernel routines called:
**         parallel_do_isr()         in this file
**
*******************************************************************************/

parallel_isr(inf)
struct interrupt *inf;
{
	register struct isc_table_type *sc = (struct isc_table_type *)inf->temp;

	parallel_do_isr(sc);
}


/*******************************************************************************
**
**   parallel_do_isr
**
**     This is the primary interrupt service routine  for  the  parallel
**     interface  driver.  All parallel interface interrupts are handled
**     by this routine.  Transfer related interrupts are  handled  first
**     if  a  transfer  is  in progress.  Note that non-transfer related
**     interrupts are allowed during transfers.  On transfer completion,
**     the   upper  level  device  driver  is  called.   After  transfer
**     interrupts are serviced, other DIL related and  error  interrupts
**     are  serviced.  Finally, if we are not continuing a transfer then
**     select code and DMA dequeueing are done.
**
**      Called by:
**	   This routine is called by parallel_isr() when the interface 
**	   card interrupts.  It is also called by dma_channel_isr() on 
**	   dma completion.
**
**      Parameters:
**         sc - pointer to the isc_table_type for this select code
**
**      Kernel routines called:
**         parallel_dma_isr()         in this file
**         parallel_fhs_isr()         in this file
**         parallel_intr_isr()        in this file
**         deliver_dil_interrupt()    in s200io/hpib.c
**         selcode_dequeue()          in s200io/selcode
**         dma_dequeue()              in s200io/dma.c
**         panic()                    in sys/subr_prf.c
**
*******************************************************************************/

parallel_do_isr(sc)
register struct isc_table_type *sc;
{
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;
	register struct buf *thisbp, *nextbp;
	register struct dil_info *dil_pkt;
	register unsigned char interrupts;
	int transfer_completed = 0;
	int i;

	switch(sc->transfer) {
		case DMA_TFR:
			transfer_completed = parallel_dma_isr(sc, cp);
			break;

		case FHS_TFR:
			transfer_completed = parallel_fhs_isr(sc, cp);
                        break;

		case INTR_TFR:
			transfer_completed = parallel_intr_isr(sc, cp);
			break;

		case PC_HANDSHK_TFR:
			transfer_completed = parallel_pc_handshk_isr(sc, cp);
			break;

		case BUSY_END_TFR:
                        if (!(cp->bus_status & PARALLEL_BUSY)) {
                                /*
                                ** the bus is no longer busy so we know that
                                ** all our bytes have gotten out accross the bus
                                */
				parallel_ruptmask(sc, PARALLEL_BUSY_EI, INT_OFF);

				/* indicate transfer is no longer in progress */
				sc->transfer = NO_TFR;
                                transfer_completed = TRANSFER_COMPLETED;
                                break;
                        }
                        transfer_completed = CONTINUE_TRANSFER;
                        break;

		case BO_END_TFR:
			/* check for fifo empty status */
			if (cp->bus_status & PARALLEL_FIFO_EMPTY) {
				/*
				** The outbound fifo is empty so we know that 
				** all our bytes have gotten out accross the bus
				*/
				parallel_ruptmask(sc, PARALLEL_FIFO_EMPTY_EI, INT_OFF);

				/* wait for the interface to drop busy */
				transfer_completed = 
					          parallel_busy_end_isr(sc, cp);
				break;
			}
			transfer_completed = CONTINUE_TRANSFER;
			break;

		case NO_TFR:
			break;

		default:
			panic("parallel_do_isr: unrecognized transfer type");
	}

	/* call the device driver on I/O completion */
	if (transfer_completed)
		(sc->owner->b_action)(sc->owner);

	/* service any remaining interrupts */
	if (interrupts = (cp->interrupt_status & ((sc->intr_wait >> 8) & 0xFF))) {
		/* service (clear) the hardware interrupts */
		parallel_ruptmask(sc, interrupts, INT_OFF);

                /* clear the software's view of the interrupts */
                sc->intr_wait &= ~(interrupts << 8);

                /* find all the DIL processes waiting for these events */
                for (thisbp = sc->event_f; thisbp != NULL; thisbp = nextbp) {
                        dil_pkt = (struct dil_info *) thisbp->dil_packet;
                        nextbp = dil_pkt->dil_forw;
                        if (interrupts & dil_pkt->intr_wait) {
                                dil_pkt->event |= interrupts & dil_pkt->intr_wait;
                                dil_pkt->intr_wait &= ~interrupts;
                                deliver_dil_interrupt(thisbp);
                        }
                }
        }

        /* dequeue on transfer completions */
        if (transfer_completed) {
		/* keep the world going */
		do {
			i = selcode_dequeue(sc);
			i += dma_dequeue();
		}
		while (i);
	}
}


/*
********************************************************************************
********************************************************************************
  
	Misc iosw Services
  
********************************************************************************
********************************************************************************
*/


/*******************************************************************************
**
**   parallel_reg_access
**
**     This routine provides read/write access  to the parallel  control
**     register  interface.   This routine is exported to device drivers
**     through the iosw and is primarily intended for device I/O (DIL).
**
**     It is assumed that the caller owns the  select  code.   Undefined
**     register  offsets will return the EINVAL error.  Registers should
**     be accessed at the interrupt  level of  the  interface  to  avoid
**     collisions with isr's.
**
**	Called by: 
**	   device drivers via the iosw (iod_reg_access)
**
**	Parameters: 
**	   bp - pointer to the buf describing this I/O request
**
**	Kernel routines called:
**         splx()                  in ../s200/locore.s
**
*******************************************************************************/

parallel_reg_access(bp)
register struct buf *bp;
{
	register struct parallel_if *cp = 
        			      (struct parallel_if *) bp->b_sc->card_ptr;
	register int access_mode = bp->pass_data1;
	register int offset = bp->pass_data2;
	register int data;
	register int x;

	x = splx(bp->b_sc->int_lvl);

	if (access_mode & READ_ACCESS) {
		switch (offset) {
		case 1:
			bp->return_data = cp->id & 0xff;
			break;
		case 3:
			bp->return_data = cp->dio_status & 0xff;
			break;
		case 5:
			bp->return_data = cp->bus_status & 0xff;
			break;
		case 7:
			bp->return_data = cp->ext_status & 0xff;
			break;
		case 9:
			bp->return_data = cp->interrupt_status & 0xff;
			break;
		case 11:
			bp->return_data = cp->fifo & 0xff;
			break;
		default:
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
		}
	} else {
		switch (offset) {
		case 1:
			cp->reset = data & 0xff;
			break;
		case 3:
			cp->dio_control = data & 0xff;
			break;
		case 5:
			/*
			** writes to register 5 have no effect
			** should we return an error?
			*/
			cp->bus_status = data & 0xff;
			break;
		case 7:
			cp->ext_control = data & 0xff;
			break;
		case 9:
			cp->interrupt_control = data & 0xff;
		case 11:
			cp->fifo = data & 0xff;
		default:
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
		}
	}
	splx(x);
}


/*******************************************************************************
**
**   parallel_reset
**
**     This routine resets both the bus and the  interface  and  returns
**     them  to  their  poweron  conditions.  To reset the bus, the INIT
**     line is driven low for 50 us.  The interface is reset by  writing
**     any  value  to  register  one.   The caller is assumed to own the
**     select code.  This routine may be called under interrupt.
**
**      Called by: 
**	   directly by other routines in this module
**
**	Parameters:  
**	   sc	- pointer to the isc_table_type for this select code
**
**	Kernel routines called:
**         snooze()                  in mach.300/clocks.s
**         parallel_restore_state()  in this file
**
*******************************************************************************/

parallel_reset(sc)
register struct isc_table_type *sc;
{
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;

	/* disable the card */
	cp->dio_control = 0;

	/* reset the parallel bus */
	cp->ext_control |= PARALLEL_INIT;
	snooze(50);
	cp->ext_control &= ~PARALLEL_INIT;

	/* reset the interface card */
	cp->reset = 1;

	/* restore any pending user interrupts */
	parallel_restore_state(sc);
}


/*******************************************************************************
**
**   parallel_dil_reset
**
**     This routine is an onion skin for parallel_reset
**
**      Called by: 
**	   device drivers via the iosw (iod_reset) 
**
**	Parameters:  
**	   bp - pointer to the buf describing this I/O request
**
**	Kernel routines called:
**         parallel_reset()           in this file
**         VASSERT()     	      in h/debug.h
**
*******************************************************************************/

parallel_dil_reset(bp)
register struct buf *bp;
{
	VASSERT(bp == bp->b_sc->owner);
	parallel_reset(bp->b_sc);
}


/*******************************************************************************
**
**   parallel_abort
**
**     This routine aborts any I/O transaction that may be in  progress.
**     If  a  data  transfer  is  in  progress  it is terminated and any
**     allocated dma resources are returned to the system.  Both the bus
**     and  the  interface  are  reset  and  returned  to  their poweron
**     conditions.  It is assumed that the caller owns the select  code.
**     Note that this routine  may  be (and  usually  is)  called  under
**     interrupt.
**
**	Called by:  
**	   device drivers via the iosw (iod_intr_set)
**
**	Parameters:
**	   bp - pointer to the buf describing this I/O request
**
**	Kernel routines called:
**         dmaend_isr()               in s200io/dma.c
**         drop_dma()                 in s200io/dma.c
**         parallel_reset()           in this file
**
**	Interface registers accessed:
**	   dio_control
**
*******************************************************************************/

parallel_abort(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct parallel_if *cp = (struct parallel_if *) sc->card_ptr;

	VASSERT(bp == bp->b_sc->owner);


        /*
	 * before we disable the gpio card from interrupts by clearing
	 * the intr_control register, we need to reset the interface 
	 * card by calling dmaend_isr.
	 */
	/* cleanup dma if necessary */
	if (sc->transfer == DMA_TFR) {	
		dmaend_isr(sc);
		drop_dma(sc->owner);
	}

	/* disable the card */
	cp->dio_control = 0;

	/* if we were doing a transfer, we are done now */
	sc->transfer = NO_TFR;

	/* reset the bus and the interface */
	parallel_reset(bp->b_sc);
}


/*******************************************************************************
**
**   parallel_intr_set
**
**     This routine enables the requested interrupting conditions on the
**     parallel  interface  bus.  It is assumed that the caller owns the
**     select code and has placed one or more bufs on  the  select  code
**     event  queue  that  wish to handle the requested interrupts. This
**     routine may be called under interrupt.
**
**     This routine is exported to device  drivers  through   the   iosw
**     and  is primarily intended for device I/O (DIL).
**
**	Called by:  
**	   device drivers via the iosw (iod_intr_set)
**
**	Parameters:
**	   sc		- pointer to the isc_table_type for this select code
**
**	   active_waits	- address of bit field to be updated with the newly
**                        enabled interrupting conditions
**
**	   request	- bit field indicating the requested interrupting
**                        conditions
**
**	Kernel routines called:
**         parallel_rupt_mask()	      in this file
**
**	Interface registers accessed:
**	   dio_control, interrupt_control
**
*******************************************************************************/

parallel_intr_set(sc, active_waits, request)
register struct isc_table_type *sc;
register int *active_waits;
register int request;
{
	register struct parallel_if *cp = (struct parallel_if *)sc->card_ptr;
	register int interrupts;

	/* disable interrupts until we are ready */
	cp->dio_control &= ~PARALLEL_INT_ENABLE;

	/* mask off the allowable user interrupts */
	interrupts = request & INTR_PARALLEL_MASK;

	/* mark the ones we are doing */
	sc->intr_wait |= interrupts;

	/* "set" don't "or" to clear old bits */
	*active_waits = interrupts;

	/* enable the appropriate interrupts */
	parallel_ruptmask(sc, interrupts, INT_ON);

	/* enable the interface card to interrupt */
	cp->dio_control |= PARALLEL_INT_ENABLE;
}

/*******************************************************************************
**
**   parallel_ruptmask
**
**     This routine sets and  clears  bits  in  the  parallel  interface
**     interrupt  control  register.   Since this register has different
**     meaning depending on whether you are writing or reading  we  must
**     treat  is  as a write only register when writing.  Thus the 'and'
**     and 'or' operators cannot be used to clear and set bits  in  this
**     register.
**
**	Called by:  
**         routines in this driver
**
**	Parameters:
**	   sc        - pointer to the isc_table_type for this select code
**	   mask      - bit mask containing desired interrupts
**	   int_flag  - boolean indication for enabling or disabling interrupts
**
*******************************************************************************/

parallel_ruptmask(sc, mask, int_flag)
register struct isc_table_type *sc;
register unsigned char mask;
register int int_flag;
{
	register struct parallel_if *cp = (struct parallel_if *)sc->card_ptr;
        register int x;

        x = spl6();

	/* "service" interrupts */
        cp->interrupt_control = 0;

	/* calculate new mask */
        if (int_flag)
                sc->parallel_rptmask |= mask;
        else
                sc->parallel_rptmask &= ~mask;

	/* make sure we don't loose any user interrupts */
	sc->parallel_rptmask |= sc->intr_wait;

	/* write out new interrupt mask */
        cp->interrupt_control = sc->parallel_rptmask;

        splx(x);
}

/*******************************************************************************
**
**   parallel_fakeisr
**
**     This routine causes the driver to be invoked as if a parallel interrupt
**     had occurred.
**
**	Called by:  
**         routines in this driver
**
**	Parameters:
**	   sc        - pointer to the isc_table_type for this select code
**
*******************************************************************************/

parallel_fakeisr(sc)
register struct isc_table_type *sc;
{
        register int x;

        x = spl6();
	sw_trigger(&sc->intloc, parallel_do_isr, sc, sc->int_lvl, 0);

        splx(x);
}

