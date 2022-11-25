/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dil_hpib.c,v $
 * $Revision: 1.10.84.8 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/08/16 13:19:38 $
 */

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
**
*/

#include "../h/param.h"
#include "../h/buf.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/ti9914.h"
#include "../h/user.h"
#include "../s200io/dma.h"
#include "../wsio/intrpt.h"
#include "../wsio/iobuf.h"
#include "../wsio/tryrec.h"
#include "../h/systm.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../wsio/dil.h"	/* may want to move these to h */
#include "../s200io/gpio.h"
#include "../wsio/hpib.h"
#include "../wsio/dilio.h"
#include "../h/iomap.h"	/* this is for the speedy version */

#include "../wsio/o_dil.h"

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

int dil_config_identify();

extern int (*hpib_config_ident)();
#endif /* IOSCAN */

/* XXX move to wsio/dilio.h */
#define PREAMB_SENT     0x800

#define M_DMA_ACTIVE(x)		((int)((unsigned)(x)&1) == 1)
#define M_DMA_RESERVE(x)	((int)((unsigned)(x)&2) == 2)
#define M_DMA_LOCK(x)		((int)((unsigned)(x)&3) == 3)
#define M_NO_NACK(x)		((int)((unsigned)(x) & 0x04))

int dil_priority;
int started_ppoll = 0;

int hpib_strategy();
int hpib_transfer();
int hpib_transfer_timeout();
int hpib_wait_status();
int dil_getsc();
int status_timeout();
int hpib_ppoll();
int hpib_spoll_isr();
int hpib_ppoll_int();
int dil_io_lock();
int dil_utility();
int dil_util();
int dil_intr();
struct buf * get_dilbuf();

int dil_utility_timeout();
int dil_util_timeout();
int dil_intr_timeout();
int dil_lock_timeout();
int ppoll_timeout();
int ppoll_int_timeout();
int wait_status_timeout();
int rel_dilbuf_check();

int hpib_first_open = 1;

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
hpib_open(dev, flag)
dev_t dev;
{
	register struct isc_table_type *sc;
	register struct buf *bp;
	register struct iobuf *iob;
	register int sc_num, ba;
	register struct dil_info *info;

	/*
	 * Here we do some device driver dependent intialization.  We use to
	 * to do this at power up, but this prevented DIL from being 
	 * configurable.  So, it was placed here.  Note that for interface
	 * drivers there is a special init function that can be called, but
	 * device drivers do not work this way.
	 */
	if (hpib_first_open) {
		hpib_first_open = 0;
		dil_init();
	}

	/* Extract the select code and (hpib) bus address */
	sc_num = m_selcode(dev);
	ba = m_busaddr(dev);

	/* Check for valid selectcodes.  Since HPIB and GPIO cards are DIO   */
	/* cards, there select code range is 0-31.                           */
	if (sc_num > 31)
		return(ENXIO);

	/*
	 * Check to make sure that the card exists.  The sc struct   
	 * is filled in during power up for each card present in the system. 
	 * The user could try to open a device file w/ a select code for 
	 * which there is no card.
	 */
	if ((sc = isc_table[sc_num]) == NULL)
		return(ENXIO);

	/*
	 * Make sure we have a card supported by DIL.  The user could create
	 * a special file w/ the major number corresponding to the DIL driver,
	 * but for the wrong card (ie the card type determined at power up
	 * for this select code is not an HPIB or GPIO card).
	 */
	if ((sc->card_type != HP98624) &&      /* TI9914 HPIB */
	    (sc->card_type != HP98625) &&      /* SIMON HPIB */
	    (sc->card_type != HP98622) &&      /* GPIO */
	    (sc->card_type != HP986PP) &&      /* Centronics */
            (sc->card_type != INTERNAL_HPIB))  /* INTERNAL TI9914 HPIB */
		return(ENXIO);

	/* Check for valid HPIB bus address. */
	if ((sc->card_type == HP98624) || (sc->card_type == HP98625) || 
            (sc->card_type == INTERNAL_HPIB)) {
	        /* 
		 * The second part of the or checks to make sure that we 
	         * have a valid bus address.  But what does the first
		 * part of the OR do? @@.
		 */
		if ((sc->my_address == ba) || (ba < 0) || (ba > 0x1f))
			return(ENXIO);
	}

	/*
	 * Assign a dil buffer to this open request.  DIL maintains a pool
	 * of bufs, iobufs, and dil_info structs to be allocated for use on 
	 * a per-open basis.  Thus, there is a maximum number of opens that
	 * a user can do.  This number, however, is configurable.
	 */
	if ((bp = get_dilbuf()) == NULL) {
		return(ENFILE);
	}

	/* 
	 * The buf structure contains pointers to the iobuf and dil_info
	 * structs allocated by get_dilbuf.  Here we are just extracting
	 * these fields for easier manipulation.
	 */
	iob = bp->b_queue;     
	info = (struct dil_info *) bp->dil_packet;

	bp->b_sc = sc;		/* save selectcode this will be using */
	bp->b_ba = ba;		/* keep address */
	iob->term_reason = 0;	/* initialize to abnormal termination */

	iob->b_flags |= NO_TIMEOUT | NO_RUPT_HANDLER | PARITY_CONTROL;

	iob->activity_timeout = 0;   /* # of clockticks used for timeouts (0 = infinite) */

	info->dil_procp = u.u_procp;       /* Save the process pointer and    */
	info->last_pid = u.u_procp->p_pid; /* process id for later use.       */

	/* need to be aware of open flags (eg FNDELAY) in other routines */
	info->open_flags = u.u_fp->f_flag;

	info->map_addr = 0;

	/* Set DIL interrupt signal to default, SIGIO. */
	if (!(u.u_procp->p_dil_signal))
		u.u_procp->p_dil_signal = SIGIO;

	iob->dil_state = USE_DMA | EIR_CONTROL;

	/* Are we opening a raw bus?  (ie hpib addr = 31 or gpio card) */
	if ((ba == NO_ADDRESS) ||             /* hpib raw open                */
	    (sc->card_type == HP98622) ||     /* GPIO is considered raw       */
	    (sc->card_type == HP986PP))       /* Centronics is considered raw */
		iob->dil_state |= D_RAW_CHAN;

	if ((sc->card_type == HP986PP) && (dev & 1))
		iob->dil_state |= PC_HANDSHK;

	if ((sc->card_type == HP986PP) && (M_NO_NACK(dev)))
		iob->dil_state |= NO_NACK;

	/* reserve any DMA requested resources */
	if (M_DMA_ACTIVE(dev)) {
		dma_active(sc);
		iob->b_flags |= ACTIVE_DMA;
	}
	if (M_DMA_RESERVE(dev)) {
		if (dma_reserve(sc) == 0)
			uprintf("DIL open: unable to reserve DMA channel\n");
		else
			iob->b_flags |= RESERVED_DMA;
	}
	if (M_DMA_LOCK(dev)) {
		if (dma_lock(sc) == 0)
			uprintf("DIL open: unable to lock DMA channel\n");
		else
			iob->b_flags |= LOCKED_DMA;
	}

	u.u_fp->f_buf = (caddr_t) bp;  /* save buf in the open file structure */

	return 0;	/* made it so far */
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
hpib_close(dev)
dev_t dev;
{
	register struct buf *bp;
	register struct iobuf *iob;
	register struct isc_table_type *sc;
	register int temp;

	bp = (struct buf *)u.u_fp->f_buf; 	/* get buffer */
	sc = bp->b_sc;
	iob = bp->b_queue;

	/* check for remaining io_burst */
	if (iob->dil_state & D_MAPPED) {
		if (sc->card_type == INTERNAL_HPIB)
			temp = 71;
		else
			temp = m_selcode(dev) + 96;
		temp = (temp << 8) + 1;/* only want 64k mapped */
		if ((bp->b_error=iomap_ioctl(temp,IOMAPUNMAP,
					&sc->mapped,3))!=0) {
			/* may not have been mapped */
			/* I doubt it */
		}
		if ((bp->b_error = iomap_close(temp)) != 0) {
			/* this should not happen */
		}
		bp->return_data = 0;
		(*sc->iosw->iod_abort_io)(bp);
		sc->intcopy |= TI_M_BO;	/* save for next guy */
		bp->b_flags |= B_DONE;
		iob->dil_state &= ~D_MAPPED;
	}
	iob->dil_state &= ~D_CHAN_LOCKED;

	/* remove any event or ppoll request and mask bits */
	HPIB_ppoll_clear(bp);
	HPIB_event_clear(bp);
	HPIB_status_clear(bp);

	/* clear out any timeouts */
	clear_timeout((caddr_t)&iob->dil_timeout);

	/* release any DMA resources */
	if (iob->b_flags & ACTIVE_DMA) {
		dma_unactive(sc);
		iob->b_flags &= ~ACTIVE_DMA;
	}
	if (iob->b_flags & RESERVED_DMA) {
		dma_unreserve(sc);
		iob->b_flags &= ~RESERVED_DMA;
	}
	if (iob->b_flags & LOCKED_DMA) {
		dma_unlock(sc);
		iob->b_flags &= ~LOCKED_DMA;
	}

        /* Make sure we don't leave an interface locked */;
        unlock_close_check(bp);

	/* make the buffer available if it is not being shared */
	release_dilbuf(bp);
	u.u_fp->f_buf = (caddr_t) NULL;
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
hpib_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct buf *bp;
	register struct dil_info *info;
	int return_value;
	int dilsigmask = (1 << (SIGDIL - 1));

	bp = (struct buf *)u.u_fp->f_buf; 	/* get buffer */

	if (shared_buf_check(bp))
		return(bp->b_error); 
	info = (struct dil_info *) bp->dil_packet;
	info->dil_procp = u.u_procp;
	info->last_pid = u.u_procp->p_pid;
	info->full_tfr_count = uio->uio_resid;

	if ((u.u_procp->p_sigmask & dilsigmask) == 0)
		u.u_procp->p_sigmask |= dilsigmask;
	else
		dilsigmask = 0;

	bp->b_queue->dil_state &= ~PREAMB_SENT;

	return_value = physio(hpib_strategy, bp, dev, B_READ, minphys, uio);
	u.u_procp->p_sigmask &= ~dilsigmask;
	return return_value;
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
hpib_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct buf *bp;
	register struct dil_info *info;
	int return_value;
	int dilsigmask = (1 << (SIGDIL - 1));

	bp = (struct buf *)u.u_fp->f_buf; 	/* get buffer */

	if (shared_buf_check(bp))
		return(bp->b_error); 
	info = (struct dil_info *) bp->dil_packet;
	info->dil_procp = u.u_procp;
	info->last_pid = u.u_procp->p_pid;
	info->full_tfr_count = uio->uio_resid;

	if ((u.u_procp->p_sigmask & dilsigmask) == 0)
		u.u_procp->p_sigmask |= dilsigmask;
	else
		dilsigmask = 0;

	bp->b_queue->dil_state &= ~PREAMB_SENT;

	return_value = physio(hpib_strategy, bp, dev, B_WRITE, minphys, uio);
	u.u_procp->p_sigmask &= ~dilsigmask;
	return return_value;
}

selcode_busy(sc, dil_info)
struct isc_table_type *sc;
struct dil_info *dil_info;
{
	/*
	 * If the select code is lock but I don't own the lock
	 * or it is not locked but owned then it is busy.
	 *
	 */
	if (((sc->state & LOCKED) || (sc->locks_pending)) && 
            (sc->locking_pid != dil_info->last_pid))
		return 1;
	else
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
hpib_strategy(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;

	bp->b_flags |= B_DIL; /* mark the buffer */
	bp->b_error = 0; /* clear errors */

	/* set up any buffer stuff */
	/* initialize this here in case we get a signal before any
	   data transfer occurs (physstrat uses this to return partial
	   transfer count on a signal)
	*/
	bp->b_resid = bp->b_bcount;

	iob->b_xaddr = bp->b_un.b_addr;
	iob->b_xcount = bp->b_bcount;

	info->dil_timeout_proc = hpib_transfer_timeout;
	bp->b_action = hpib_transfer;
	enqueue(iob, bp);
}


enum dil_transfer_state {get_dil_sc = 0,
			 check_transfer,
			 re_get_dil_sc,
			 do_transfer_preamb, 
			 do_get_dma,
			 do_transfer, 
			 end_transfer, 
			 tfr_timedout, 
			 tfr_defaul};

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:  iod_io_check, iod_save_state, iod_preamb_intr,
**		iod_tfr, iod_restore_state, ....
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	
**  Determining transfer mechanism algo:
**  --------------------------------------------------
**  The type of transfer is determined by 1) the user's requested speed, 2)
**  size of the transfer, 3) whether dma is present, and 4) the read
**  termination mode in effect.  If dma is not present as indicated by the
**  global kernel var 'dma_here', then interrupts must be used.  If pattern
**  matching is in effect, then interrupts must be used.  If one byte only is
**  being transfered, then interrupts are used.  If the above conditions are
**  false, and the user requested maximum transfer speed, then dma is used; if
**  the user requested a speed less than the maximum, then interrupts are used.
**  Finally, if the user did not request a speed, dma is used (the default).
**  Note that whether or not dma is actually used is determined by other
**  factors (see external doc on s300 dma usage).
**  
************************************************************************/
hpib_transfer(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register struct isc_table_type *sc = bp->b_sc;
	register unsigned int state = 0;
	register enum transfer_request_type control;
	register int x;

	state = iob->dil_state;

	try
		START_FSM;
re_switch:
		switch ((enum dil_transfer_state)iob->b_state) {
		case get_dil_sc:
			iob->b_state = (int)check_transfer;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) tfr_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(hpib_transfer_timeout)
			if (!dil_get_selcode(bp, hpib_transfer))
				break;

		case check_transfer:
			END_TIME
			x = spl6();
			switch ((*sc->iosw->iod_io_check)(bp)) {
				case TRANSFER_READY:
					iob->b_state = (int)do_transfer_preamb;
					splx(x);
					goto re_switch;
				case TRANSFER_ERROR:
					iob->b_state = (int) tfr_defaul;
					splx(x);
					dil_drop_selcode(bp);
					queuedone(bp);
					break;
				case TRANSFER_WAIT:
					iob->b_state = (int)re_get_dil_sc;
					HPIB_status_int(bp, hpib_transfer);
					dil_drop_selcode(bp);
					DIL_START_TIME(hpib_transfer_timeout)
					splx(x);
					break;
			}
			break;

		case re_get_dil_sc:
			END_TIME
			iob->b_state = (int)do_transfer_preamb;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) tfr_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(hpib_transfer_timeout)
			if (!dil_get_selcode(bp, hpib_transfer))
				break;

		case do_transfer_preamb:
			END_TIME
			iob->b_state = (int)do_get_dma;
			DIL_START_TIME(hpib_transfer_timeout)
			if (state & D_RAW_CHAN)
				(*sc->iosw->iod_save_state)(sc);
			else {
				if ((iob->dil_state & PREAMB_SENT) == 0) {
					iob->dil_state |= PREAMB_SENT;
					if (!(*sc->iosw->iod_preamb_intr)(bp))
						break;
				}
			}

		case do_get_dma:
			END_TIME
			iob->b_state = (int)do_transfer;

			DIL_START_TIME(hpib_transfer_timeout)
			if (iob->b_flags & RESERVED_DMA) {
				get_dma(bp, hpib_transfer);
				break;
			}
			goto re_switch;

		case do_transfer:
			END_TIME
			iob->b_state = (int)end_transfer;

			/* set the transfer mechanism; see notes above */
			if (state & (READ_PATTERN | USE_INTR)) {
				control = MUST_INTR;
				sc->pattern = iob->read_pattern;
			}
			else if (bp->b_bcount == 1)
				control = MUST_INTR;
			else if (!dma_here)
				control = MUST_INTR;
			else if (state & USE_DMA) {
				if (iob->b_flags & ACTIVE_DMA)
					control = MAX_SPEED;
				else
					control = MAX_OVERLAP;
			} else
				control = MAX_OVERLAP;
			/* set up transfer control info here */
			sc->tfr_control = state;
			sc->resid = bp->b_bcount;
			DIL_START_TIME(hpib_transfer_timeout)
			(*sc->iosw->iod_tfr)(control, bp, hpib_transfer);
			break;

		case end_transfer:
			iob->b_state = (int) tfr_defaul;
			END_TIME
			bp->b_resid = bp->b_sc->resid;
			info->full_tfr_count -= (bp->b_bcount - bp->b_resid);
			iob->term_reason = sc->tfr_control; /* set this now */

			if (iob->term_reason & (TR_MATCH | TR_END))
			        bp->b_flags |= B_END_OF_DATA;

			if (bp->b_error) {
				bp->b_error = EIO;
				escape(1);
			}
			if (sc->state & STATE_SAVED)
				(*sc->iosw->iod_restore_state)(sc);
			dil_drop_selcode(bp);	/* get rid of channel */
			queuedone(bp);
			break;

		case tfr_timedout:
			escape(TIMED_OUT);
		default:
			panic("bad dil transfer state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) tfr_defaul;
		if (escapecode == TIMED_OUT)
			bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		(*sc->iosw->iod_abort_io)(bp);

                /* If we own the sc, update the resid for physstrat to
                   use in partial transfer count calculations */
                if (sc->owner == bp)
                   bp->b_resid = sc->resid;

		HPIB_status_clear(bp);
		iob->term_reason = TR_ABNORMAL;
		dil_drop_selcode(bp);
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
**  a note on copying dil_state for IO_MAP
**  --------------------------------------
**  Dil_state (in the iobuf) was changed from a char to an int.  The library
**  code does not make use of all of the bits which are currently being used in
**  dil_state.  In order to avoid modifying the library code to accept an
**  integer 'info->state' and to avoid binary incompatibility, all bits which
**  the library uses must be placed in lower char of dil_state (so that we 
**  can continue to pass just one char to the library).
************************************************************************/
hpib_ioctl(dev, cmd, arg, mode, uio)
dev_t dev;
char *arg;
struct uio *uio;
{
	register struct buf *bp;
	register struct iobuf *iob;
	register struct isc_table_type *sc;
	register struct dil_info *dil_info;
	register struct dil_info *sc_dil_info;
	struct buf *p;
	struct ioctl_intr_type *intr_info;
	register unsigned int state;
	register int status;
	register int status_result;
	register int temp;
	unsigned char data;
	unsigned char data1;
	int x, return_info = 1;
	struct fd_info *info;
	struct o_fd_info *o_info;
	struct D515_fd_info *D515_info;

	int newlib, arg_type, arg_data0, arg_data1, arg_data2;
	register struct o_ioctl_type *old_arg;
	register struct ioctl_type *new_arg;

	switch (cmd) {
		case O_IO_CONTROL:
		case O_IO_STATUS:
		case O_HPIB_CONTROL:
		case O_HPIB_STATUS:
		case O_HPIB_SEND_CMD:
		case O_HPIB_MAP:
		case O_HPIB_UNMAP:
		case O_HPIB_REMAP:
		case O_HPIB_UNMAP_MARK:
			newlib = D5141LIB;
			old_arg = (struct o_ioctl_type *) arg;
			arg_type = old_arg->type;
			arg_data0 = old_arg->data[0];
			arg_data1 = old_arg->data[1];
			arg_data2 = old_arg->data[2];
			break;
		case D515_HPIB_MAP:
		case D515_HPIB_UNMAP:
		case D515_HPIB_REMAP:
		case D515_HPIB_UNMAP_MARK:
			newlib = D515LIB;
			break;
		default:
			newlib = D52LIB;
			new_arg = (struct ioctl_type *) arg;
			arg_type = new_arg->type;
			arg_data0 = new_arg->data[0];
			arg_data1 = new_arg->data[1];
			arg_data2 = new_arg->data[2];
	}

	bp = (struct buf *)u.u_fp->f_buf; 	/* get buffer */
	if (shared_buf_check(bp))
		return(bp->b_error); 
	acquire_buf(bp);
	dil_info = (struct dil_info *) bp->dil_packet;
	dil_info->dil_procp = u.u_procp;
	dil_info->last_pid = u.u_procp->p_pid;

	iob = bp->b_queue; 	/* get dil iobuf*/
	/* mark the buffer */
	bp->b_flags &= ~(B_DONE | B_ERROR);
	bp->b_flags |= B_DIL;
	bp->b_error = 0;	/* clear errors */
	bp->return_data = 0;		/* defualt return value */
	bp->pass_data1 = arg_data1;	/* first parameter (if any) */
	bp->pass_data2 = arg_data2;	/* second parameter (if any) */

	sc = bp->b_sc;
	if (newlib && new_arg->sc_state & PASS_IN)
		if (new_arg->sc_state & TI9914_HOLDOFF)
			sc->state |= IN_HOLDOFF;
		else
			sc->state &= ~IN_HOLDOFF;

	/*
	** Find out what our current bus controller status is.
	** If this is a simon card we must get the select code
	** since touching simon during a DMA transfer can cause
	** simon to lose data.
	*/
	if ((sc->card_type == HP98625) &&
	    ((cmd != IO_CONTROL) || 
	     ((cmd == IO_CONTROL) && 
	      ((arg_type == IO_INTERRUPT) ||
	       (arg_type == IO_LOCK) ||
	       (arg_type == IO_RESET))))) {
		/* this is simon so lets do it the hard way */
		dil_info->dil_action = sc->iosw->iod_ctlr_status;
		dil_info->dil_timeout_proc = dil_utility_timeout;
		bp->b_action = dil_utility;
		enqueue(iob, bp);

		/* wait for this request to complete */
		x = spl6();
		if (setjmp(&u.u_qsave)) {
			/* 
			** Request has recieved a signal cleanup and return. 
			*/
			if (!(bp->b_flags & B_DONE))
				(*dil_info->dil_timeout_proc)(bp);
			u.u_eosys = EOSYS_NORESTART;
			bp->b_error = EINTR;
		} else
			while ((bp->b_flags & B_DONE) == 0)
				sleep((caddr_t) &bp->b_flags, DILPRI);
		splx(x);

		if (bp->b_flags & B_ERROR) {
			release_buf(bp);
			return(bp->b_error);
		}
		bp->b_flags &= ~B_DONE;
	} else 
		(*sc->iosw->iod_ctlr_status)(bp);

	state = iob->dil_state & 0xffff;

	switch( cmd ) {
	case HPIB_CONTROL:
	case O_HPIB_CONTROL:
	   switch( arg_type ) {
	      case HPIB_RESET:
	   	switch( arg_data0 ) {
	      		case BUS_CLR:
				SYS_CTLR_ONLY
				RAW_CHAN_ONLY
				dil_info->dil_action = sc->iosw->iod_dil_abort;
				dil_info->dil_timeout_proc = dil_utility_timeout;
				bp->b_action = dil_utility;
				enqueue(iob, bp);
				break;
			default:
				bp->b_error = EINVAL;	/* bad value */
				bp->b_flags |= B_ERROR | B_DONE;
			}
		break;

		case HPIB_EOI:
			if (arg_data0 != 0)
				iob->dil_state |= EOI_CONTROL;
			else
				iob->dil_state &= ~EOI_CONTROL;
			bp->b_flags |= B_DONE;
			break;

		case HPIB_PPOLL_IST:
			RAW_CHAN_ONLY
			if (arg_data0 == 0)
				sc->state &= ~IST;
			else
				sc->state |= IST;

			dil_info->dil_action = sc->iosw->iod_set_ppoll_resp;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_PPOLL_RESP:
			RAW_CHAN_ONLY
			if (arg_data0 == 0) {
				bp->ppoll_data = 0;
				sc->ppoll_resp = 0;
			} else {
				if (sc->card_type == HP98625) {
					bp->ppoll_data = arg_data1 & 0x7;
					if (bp->ppoll_data == 0)
						bp->ppoll_data = 0xff;
				} else
					bp->ppoll_data = 1 << (arg_data1 & 0x7);
				if (arg_data2 == 0)
					sc->ppoll_resp = (unsigned char) bp->ppoll_data;
				else
					sc->ppoll_resp = bp->ppoll_data << 8;
			}
			dil_info->dil_action = sc->iosw->iod_set_ppoll_resp;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_REN:
			SYS_CTLR_ONLY
			RAW_CHAN_ONLY
			bp->pass_data1 = arg_data0;
			dil_info->dil_action = sc->iosw->iod_ren;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_SRQ:	/* assert SRQ */
			RAW_CHAN_ONLY
			bp->pass_data1 = arg_data0;
			dil_info->dil_action = sc->iosw->iod_srq;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_PASS_CONTROL:
			ACT_CTLR_ONLY
			RAW_CHAN_ONLY
			if (sc->ppoll_f) {
				bp->b_error = EIO;	/* can't do */
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			bp->pass_data1 = arg_data0;
			if ((bp->pass_data1 < 0) || (bp->pass_data1 > 30) ||
			    (bp->pass_data1 == sc->my_address)) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			dil_info->dil_action = sc->iosw->iod_pass_control;
			dil_info->dil_timeout_proc = dil_util_timeout;
			bp->b_action = dil_util;
			enqueue(iob, bp);
			break;

		case HPIB_SET_ADDR:
			bp->pass_data1 = arg_data0;
			if ((bp->pass_data1 < 0) || (bp->pass_data1 > 30)) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			dil_info->dil_action = sc->iosw->iod_set_addr;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_ATN:
			ACT_CTLR_ONLY
			bp->pass_data1 = arg_data0;
			dil_info->dil_action = sc->iosw->iod_atn_ctl;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;

		case HPIB_PARITY:
			if (sc->card_type == HP98625) {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			if (arg_data0)
				iob->b_flags |= PARITY_CONTROL;
			else
				iob->b_flags &= ~PARITY_CONTROL;
			bp->b_flags |= B_DONE;
			break;

		default:
			bp->b_error = EINVAL;	/* bad value */
			bp->b_flags |= B_ERROR | B_DONE;
	   }	/* control */
		break;
	case HPIB_STATUS:	/* status */
	case O_HPIB_STATUS:	/* status */
	   switch( arg_type ) {
		case HPIB_PPOLL:	/* called from dillib -- hpib_ppoll */
			RAW_CHAN_ONLY
			ACT_CTLR_ONLY
			dil_info->dil_timeout_proc = ppoll_timeout;
			bp->b_action = hpib_ppoll;
			enqueue(iob, bp);
			break;

		case HPIB_SPOLL:
			ACT_CTLR_ONLY
			RAW_CHAN_ONLY
			if ((bp->pass_data1 < 0) || (bp->pass_data1 > 30) ||
			    (bp->pass_data1 == sc->my_address)) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			bp->b_ba = bp->pass_data1;
			dil_info->dil_action = sc->iosw->iod_spoll;
			dil_info->dil_timeout_proc = dil_util_timeout;
			bp->b_action = dil_util;
			enqueue(iob, bp);
			break;

		case HPIB_ADDRESS:
			if ((sc->card_type == HP98625) && 
                            (sc->state & ACTIVE_CONTROLLER))
					bp->return_data = 30;
			else
				bp->return_data = sc->my_address;
			bp->b_flags |= B_DONE;
			break;

		case HPIB_BUS_STATUS:
			RAW_CHAN_ONLY
			bp->pass_data1 = arg_data0;
			/*
			** Get bus status info from the card.
			** If this is a simon card we must get the select
			** code since touching simon during a DMA transfer
			** can cause simon to lose data.
			*/
			if (sc->card_type == HP98625) {
				dil_info->dil_action = sc->iosw->iod_status;
				dil_info->dil_timeout_proc = dil_utility_timeout;
				bp->b_action = dil_utility;
				enqueue(iob, bp);
			} else {
				(*sc->iosw->iod_status)(bp);
				bp->b_flags |= B_DONE;
			}
			break;

		case HPIB_WAIT_ON_PPOLL:
			ACT_CTLR_ONLY
			RAW_CHAN_ONLY

			/* return error for zero mask; see FSDlsxxxxxx */
			if (arg_data1 == 0) {
				bp->b_flags |= B_DONE | B_ERROR;
				bp->b_error = EINVAL;
				break;
			}
			bp->b_ba = arg_data1;
			dil_info->dil_timeout_proc = ppoll_int_timeout;
			bp->b_action = hpib_ppoll_int;
			enqueue(iob, bp);
			break;
		case HPIB_WAIT_ON_STATUS:
			/** WARNING: INT_SRQ is defined already */
			RAW_CHAN_ONLY
			bp->pass_data1 = arg_data0;
			if (bp->pass_data1 & ~(STATE_SRQ|STATE_ACTIVE_CTLR|
						STATE_TALK|STATE_LISTEN)) {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			if ((bp->pass_data1 & STATE_TALK) && 
                            (sc->state & ACTIVE_CONTROLLER)) {
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			if ((bp->pass_data1 & STATE_LISTEN) && 
                            (sc->state & ACTIVE_CONTROLLER)) {
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			/* clr old wait bits */
			dil_info->intr_wait &= INTR_MASK;
			dil_info->dil_timeout_proc = wait_status_timeout;
			bp->b_action = hpib_wait_status;
			enqueue(iob, bp);
			break;
		default:
			bp->b_error = EINVAL;	/* bad value */
			bp->b_flags |= B_ERROR | B_DONE;
	   }	/* status */
	   break;

	case HPIB_SEND_CMD:
	case O_HPIB_SEND_CMD:
		ACT_CTLR_ONLY
		RAW_CHAN_ONLY
		/* check for a reasonable length */
		if ((arg_type < 1) || (arg_type > 122)) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			break;
		}
		bp->pass_data1 = arg_type;	/* length */
		if (newlib)
			bcopy(&new_arg->data[0], dil_info->cmd_buf, arg_type);
		else
			bcopy(&old_arg->data[0], dil_info->cmd_buf, arg_type);
		bp->pass_data2 = (int)dil_info->cmd_buf;
		dil_info->dil_action = sc->iosw->iod_send_cmd;	
		dil_info->dil_timeout_proc = dil_util_timeout;
		bp->b_action = dil_util;
		enqueue(iob, bp);
		break;

	case IO_REG_ACCESS:
		bp->pass_data1 = arg_data0;
		bp->pass_data2 = arg_data1;
		dil_info->register_data = arg_data2;
		dil_info->dil_action = sc->iosw->iod_reg_access;
		dil_info->dil_timeout_proc = dil_utility_timeout;
		bp->b_action = dil_utility;
		enqueue(iob, bp);
		break;

	case IO_CONTROL:
	case O_IO_CONTROL:
	   switch( arg_type ) {
		case IO_DMA:
			switch (arg_data0) {
			case DMA_ACTIVE:
				if ((iob->b_flags & ACTIVE_DMA) == 0) {
					dma_active(sc);
					iob->b_flags |= ACTIVE_DMA;
				}
				break;
			case DMA_UNACTIVE:
				if (iob->b_flags & ACTIVE_DMA) {
					dma_unactive(sc);
					iob->b_flags &= ~ACTIVE_DMA;
				}
				break;
			case DMA_RESERVE:
				if ((iob->b_flags & RESERVED_DMA) == 0) {
					if (dma_reserve(sc) == 0) {
						bp->b_error = EINVAL;
						bp->b_flags |= B_ERROR;
						break;
					}
					iob->b_flags |= RESERVED_DMA;
				}
				break;
			case DMA_UNRESERVE:
				if (iob->b_flags & RESERVED_DMA) {
					dma_unreserve(sc);
					iob->b_flags &= ~RESERVED_DMA;
				}
				break;
			case DMA_LOCK:
				if ((iob->b_flags & LOCKED_DMA) == 0) {
					if (dma_lock(sc) == 0) {
						bp->b_error = EINVAL;
						bp->b_flags |= B_ERROR;
						break;
					}
					iob->b_flags |= LOCKED_DMA;
				}
				break;
			case DMA_UNLOCK:
				if (iob->b_flags & LOCKED_DMA) {
					dma_unlock(sc);
					iob->b_flags &= ~LOCKED_DMA;
				}
				break;
			default:
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			}
			bp->b_flags |= B_DONE;
			break;

		case IO_INTERRUPT:
			RAW_CHAN_ONLY
			/* check to see if interrupts were set up */
			if (iob->b_flags & NO_RUPT_HANDLER) {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}

			if (arg_data0 == 0) {
				dil_info->intr_wait &= ~INTR_MASK;
				bp->b_flags |= B_DONE;
				break;
			}

			/* make sure this buf is already on the list */
			x = spl6();
			p = sc->event_f;
			while (p != NULL) {
				if (p == bp) {
					break;
				} else
					p = ((struct dil_info *) p->dil_packet)->dil_forw;
			}

			if (p == NULL) {
				/*
				** put this process on the list of those waiting
				** for an event on this select code
				*/
				dil_info->dil_forw = NULL;
				if (sc->event_f == NULL) {
					sc->event_f = bp;
					sc->event_l = NULL;
				} else
					((struct dil_info *)sc->event_l->dil_packet)->dil_forw = bp;
				dil_info->dil_back = sc->event_l;
				sc->event_l = bp;
			}
			splx(x);

			/* enable appropriate interrupting conditions */
			dil_info->dil_timeout_proc = dil_intr_timeout;
			bp->b_action = dil_intr;
			enqueue(iob, bp);
			break;

	      case IO_LOCK:
	   	switch( arg_data0 ) {
			case LOCK:
				x = spl6();
				if ((sc->state & LOCKED) && 
				    (sc->locking_pid == dil_info->last_pid)) {
					/* we already have it locked */
					bp->return_data = ++sc->lock_count;
					splx(x);
					bp->b_flags |= B_DONE;
					break;
				}
				if (!sc->active) {
					/* nobodys got it - we will take it */
					sc->active++;
					sc->owner = bp;
					dil_info->locking_pid = dil_info->last_pid;
					sc->locking_pid = dil_info->locking_pid;
					iob->dil_state |= D_CHAN_LOCKED;
					sc->state |= LOCKED;
					bp->return_data = 1;
					sc->lock_count = 1;
					splx(x);
					bp->b_flags |= B_DONE;
					break;
				}
				if ((sc->state & LOCKED) && 
				    (u.u_fp->f_flag & FNDELAY)) {
					/* this guy doesn't want to wait */
					splx(x);
					bp->b_error = EACCES;
					bp->b_flags |= B_ERROR | B_DONE;
					break;
				}
				splx(x);
				dil_info->dil_timeout_proc = dil_lock_timeout;
				bp->b_action = dil_io_lock;
				enqueue(iob, bp);
				break;
			case DIL_UNLOCK:
				x = spl6();
				if (((sc->state & LOCKED) == 0) && 
				    (sc->locking_pid == 0)) {
					/* no one has it locked */
					bp->return_data = 0;
					sc->lock_count = 0;
					splx(x);
					bp->b_error = EINVAL;
					bp->b_flags |= B_ERROR | B_DONE;
					break;
				}
				if (sc->locking_pid != dil_info->last_pid) {
					/* someone else has it locked */
					splx(x);
					bp->b_error = EPERM;
					bp->b_flags |= B_ERROR | B_DONE;
					break;
				}
				if ((--sc->lock_count) == 0) {
					iob->dil_state &= ~D_CHAN_LOCKED;
					dil_info->locking_pid = 0;
					sc->state &= ~LOCKED;
					sc->locking_pid = 0;
					drop_selcode(sc->owner);
					wakeup((caddr_t) &bp->b_queue);
				}
				bp->return_data = sc->lock_count;
				splx(x);
				dil_dequeue(bp);
				bp->b_flags |= B_DONE;
				break;
			default:
				bp->b_error = EINVAL;	/* bad value */
				bp->b_flags |= B_ERROR | B_DONE;
			}	/* HPIB_LOCK */
		    break;

		case IO_TIMEOUT:	/* set the activity timeout */
			temp = arg_data0;
			if (temp > 0)
				/* get the ticks */
				temp = ((temp / 1000) * HZ) / 1000 + 2; 
			else
				temp = 0;
			if (temp == 0) {
				iob->b_flags |= NO_TIMEOUT;
			} else {
				iob->b_flags &= ~NO_TIMEOUT;
				iob->activity_timeout = temp;
			}
			bp->b_flags |= B_DONE;
			break;
		case IO_SET_MAP_ADDR:	/* set the address to map ioburst segment*/
			dil_info->map_addr = (caddr_t) arg_data0;
			bp->b_flags |= B_DONE;
			break;
		case IO_READ_PATTERN:
			if (arg_data0 != 0) {
				iob->dil_state |= READ_PATTERN;
				iob->read_pattern = arg_data1 & 
				  ((iob->dil_state & D_16_BIT) ? 0xffff : 0xff);
			} else {
				iob->dil_state &= ~READ_PATTERN;
				iob->read_pattern = 0;
			}
			bp->b_flags |= B_DONE;
			break;
		case IO_WIDTH:
			RAW_CHAN_ONLY
			bp->pass_data1 = arg_data0;
			if ((bp->pass_data1 == 16) && 
                            (sc->card_type == HP98622)) {
				iob->dil_state |= D_16_BIT;
				bp->b_flags |= B_DONE;
				break;
			}
			if (bp->pass_data1 == 8) {
				iob->dil_state &= ~D_16_BIT;
				bp->b_flags |= B_DONE;
				break;
			}
			bp->b_error = EINVAL;	/* bad value */
			bp->b_flags |= B_DONE | B_ERROR;
			break;

	      	case IO_RESET:
			RAW_CHAN_ONLY
			dil_info->dil_action = sc->iosw->iod_dil_reset;
			dil_info->dil_timeout_proc = dil_utility_timeout;
			bp->b_action = dil_utility;
			enqueue(iob, bp);
			break;
		case IO_SPEED:	/* set the transfer speed */
			temp = arg_data0;
			iob->dil_state &= ~(USE_DMA|USE_INTR);
			if (temp > 140)
				iob->dil_state |= USE_DMA;
			else if (temp < 7)
				iob->dil_state |= USE_INTR;
			bp->b_flags |= B_DONE;
			break;
		default:
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR | B_DONE;
			break;
		} /* IO_CONTROL switch */
		break;
	case IO_STATUS:
	case O_IO_STATUS:
	   switch( arg_type ) {
		case IO_RAW_DEV:
			RAW_CHAN_ONLY
			break;
		case IO_TERM_REASON:
			bp->return_data = iob->term_reason & 0x07;
			break;
		case CHANNEL_TYPE:
			if (state & D_RAW_CHAN)
				bp->return_data = HPIB_CHAN;
			else
				bp->return_data = HPIB_DEVICE;
			break;
		default:
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
		}	/* IO_STATUS switch */
		bp->b_flags |= B_DONE;
		break;
	case IO_MAP:
	case O_HPIB_MAP:
	case D515_HPIB_MAP:
		/* must make a dev that is what iomap wants */
		if (sc->card_type == INTERNAL_HPIB)
			temp = 71;
		else
			temp = m_selcode(dev) + 96;
		temp = (temp << 8) + 1;/* only want 64k mapped */
		if ((bp->b_error = iomap_open(temp, 1)) != 0) {
			bp->b_flags |= B_ERROR | B_DONE;
			break;
		}
		switch (newlib) {
		case D52LIB:
			info = (struct fd_info *)arg;	/* get pointer to structure */
			info->cp = dil_info->map_addr;	
			if ((bp->b_error=iomap_ioctl(temp, IOMAPMAP, &info->cp, 3))!=0){
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			/* the map was successfull, now get the other information */
			info->ba = m_busaddr(dev);
			info->state = iob->dil_state;
			info->pattern = iob->read_pattern;
			info->card_address = sc->my_address;
			info->card_type = sc->card_type;
			info->dev = dev;
			info->temp = (((sc->state & IN_HOLDOFF) >> 11) | 
				      ((iob->b_flags & PARITY_CONTROL) >> 24));
			sc->mapped = (int)info->cp;	/* keep copy */
			break;
		case D515LIB:
			D515_info = (struct D515_fd_info *)arg;	/* get pointer to structure */
			D515_info->cp = 0;	/* let the system find the place */
			if ((bp->b_error=iomap_ioctl(temp, IOMAPMAP, &D515_info->cp, 3))!=0){
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			/* the map was successfull, now get the other information */
			D515_info->ba = m_busaddr(dev);
			D515_info->state = iob->dil_state;
			D515_info->pattern = iob->read_pattern;
			D515_info->card_address = sc->my_address;
			D515_info->dev = dev;
			D515_info->temp = sc->state & IN_HOLDOFF;
			sc->mapped = (int)D515_info->cp;	/* keep copy */
			break;
		case D5141LIB:
			o_info = (struct o_fd_info *)arg;	/* get pointer to structure */
			o_info->cp = 0;	/* let the system find the place */
			if ((bp->b_error=iomap_ioctl(temp, IOMAPMAP, &o_info->cp, 3))!=0){
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			/* the map was successfull, now get the other information */
			o_info->ba = m_busaddr(dev);
			o_info->state = iob->dil_state;
			o_info->pattern = iob->read_pattern;
			o_info->card_address = sc->my_address;
			o_info->dev = dev;
			o_info->temp = sc->state & IN_HOLDOFF;
			sc->mapped = (int)o_info->cp;	/* keep copy */
			break;
		}
		if (bp->b_flags & B_ERROR)
			break;
		bp->b_flags |= B_DONE;
		iob->dil_state |= D_MAPPED;
		return_info = 0;	/* don't want to stick back anything */
		break;
	case IO_REMAP:
	case O_HPIB_REMAP:
	case D515_HPIB_REMAP:
		switch (newlib) {
		case D52LIB:
			info = (struct fd_info *)arg;	/* get pointer to structure */
			info->state = iob->dil_state;
			info->pattern = iob->read_pattern;
			info->card_address = sc->my_address;
			info->card_type = sc->card_type;
			info->dev = dev;
			info->temp = (((sc->state & IN_HOLDOFF) >> 11) | 
				      ((iob->b_flags & PARITY_CONTROL) >> 24));
			break;
		case D515LIB:
			D515_info = (struct D515_fd_info *)arg;	/* get pointer to structure */
			D515_info->state = iob->dil_state;
			D515_info->pattern = iob->read_pattern;
			D515_info->card_address = sc->my_address;
			D515_info->dev = dev;
			D515_info->temp = sc->state & IN_HOLDOFF;
			break;
		case D5141LIB:
			o_info = (struct o_fd_info *)arg;	/* get pointer to structure */
			o_info->state = iob->dil_state;
			o_info->pattern = iob->read_pattern;
			o_info->card_address = sc->my_address;
			o_info->dev = dev;
			o_info->temp = sc->state & IN_HOLDOFF;
			break;
		}
		iob->dil_state |= D_MAPPED;
		bp->b_flags |= B_DONE;
		return_info = 0;	/* don't want to stick back anything */
		break;
	case IO_UNMAP:
	case O_HPIB_UNMAP:
	case D515_HPIB_UNMAP:
		if (sc->card_type == INTERNAL_HPIB)
			temp = 71;
		else
			temp = m_selcode(dev) + 96;
		temp = (temp << 8) + 1;/* only want 64k mapped */
		switch (newlib) {
		case D52LIB:
			info = (struct fd_info *)arg;	/* get pointer to structure */
			if (sc->card_type == INTERNAL_HPIB)
				info->cp -= 0x8011;
			if (sc->card_type == HP98624)
				info->cp -= 0x11;
			if ((bp->b_error=iomap_ioctl(temp,IOMAPUNMAP,&info->cp,3))!=0) {
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			break;
		case D515LIB:
			D515_info = (struct D515_fd_info *)arg;	/* get pointer to structure */
			if (sc->card_type == INTERNAL_HPIB)
				o_info->cp -= 0x8011;
			if (sc->card_type == HP98624)
				o_info->cp -= 0x11;
			if ((bp->b_error=iomap_ioctl(temp,IOMAPUNMAP,&o_info->cp,3))!=0) {
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			break;
		case D5141LIB:
			o_info = (struct o_fd_info *)arg;	/* get pointer to structure */
			if (sc->card_type == INTERNAL_HPIB)
				o_info->cp -= 0x8011;
			if (sc->card_type == HP98624)
				o_info->cp -= 0x11;
			if ((bp->b_error=iomap_ioctl(temp,IOMAPUNMAP,&o_info->cp,3))!=0) {
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
			break;
		}

		if (bp->b_flags & B_ERROR)
			break;

		if ((bp->b_error = iomap_close(temp)) != 0) {
			bp->b_flags |= B_ERROR | B_DONE;
			break;
		}
		bp->return_data = 0;
		(*sc->iosw->iod_abort_io)(bp);
		sc->intcopy |= TI_M_BO;	/* save for next guy */
		bp->b_flags |= B_DONE;
		iob->dil_state &= ~D_MAPPED;
		dil_drop_selcode(bp);
		break;

	case IO_UNMAP_MARK:
	case O_HPIB_UNMAP_MARK:
	case D515_HPIB_UNMAP_MARK:
		bp->return_data = 0;
		iob->dil_state &= ~D_MAPPED;
		bp->b_flags |= B_DONE;
		break;

	case HPIB_SET_DIL_SIG:
		RAW_CHAN_ONLY  
		dil_info->dil_procp->p_dil_signal = SIGDIL;
		bp->return_data = SIGDIL;
		bp->b_flags |= B_DONE;
		break;

	case HPIB_INTERRUPT_SET:
		RAW_CHAN_ONLY

		/* get interrupt info */
		intr_info = (struct ioctl_intr_type *) arg;

                if ((intr_info->cause << 8) & INTR_PPOLL && 
                    !(sc->state & ACTIVE_CONTROLLER)) { 
                                bp->b_error = EINVAL; 
                                bp->b_flags |= B_ERROR | B_DONE;
                                break; 
                }

		dil_info->eid 	    = intr_info->eid;
		dil_info->cause     = (intr_info->cause << 8) & INTR_MASK;
		dil_info->ppl_mask  = intr_info->mask;
		dil_info->handler   = intr_info->proc;

		/* if this buf is already on the list we are done */
		x = spl6();
		p = sc->event_f;
		while (p != NULL)
			if (p == bp) break;
			else p = ((struct dil_info *) p->dil_packet)->dil_forw;

		if (p == NULL) {
			/*
			** put this process on the list of those waiting
			** for an event on this select code
			*/
			dil_info->dil_forw = NULL;
			if (sc->event_f == NULL) {
				sc->event_f = bp;
				sc->event_l = NULL;
			} else
				((struct dil_info *)sc->event_l->dil_packet)->dil_forw = bp;
			dil_info->dil_back = sc->event_l;
			sc->event_l = bp;
		}
		splx(x);
		iob->b_flags &= ~NO_RUPT_HANDLER;
		dil_info->dil_procp->p_flag2 |= S2SENDDILSIG;

		/* If we've gotten here and this field is 0, this is an old
		   application (pre-6.5) which was exec'ed from a new one and
		   is using the fd to do an io_on_interrupt, without reopening
		   the device.
		*/
		if (!(u.u_procp->p_dil_signal))
			u.u_procp->p_dil_signal = SIGIO;

		return_info = 0;
		bp->b_flags |= B_DONE;
		break;

	case HPIB_INTERRUPT_ENABLE:	/* called by dillib -- io_on_interrupt */
		/* enable appropriate interrupting conditions */
		dil_info->dil_timeout_proc = dil_intr_timeout;
		bp->b_action = dil_intr;
		enqueue(iob, bp);
		break;

	case HPIB_SET_STATE:
		sc->state |= IN_HOLDOFF;
		return_info = 0;	/* don't want to stick back anything */
		bp->b_flags |= B_DONE;
		break;

	case HPIB_CLEAR_STATE:
		sc->state &= ~IN_HOLDOFF;
		return_info = 0;	/* don't want to stick back anything */
		bp->b_flags |= B_DONE;
		break;

	case HPIB_GET_STATE:
		if (sc->state & IN_HOLDOFF)
			bp->b_error = 2;
		else
			bp->b_error = 1;
		return_info = 0;	/* don't want to stick back anything */
		bp->b_flags |= B_DONE;
		break;

        case GPIO_CONTROL:
	   switch(arg_type) {
   	       case GPIO_EIR_CONTROL:
		        if (sc->card_type != HP98622) {
			        bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR | B_DONE;
				break;
			}
		        if (arg_data0)
			        iob->dil_state |= EIR_CONTROL;
		        else
			        iob->dil_state &= ~EIR_CONTROL;
		        bp->b_flags |= B_DONE;
		        break;

		default:
			bp->b_error = EINVAL;	/* bad value */
			bp->b_flags |= B_ERROR | B_DONE;
			break;
	   }	/* end switch GPIO_CONTROL type */
	   break;
	
	default:
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR | B_DONE;
	} /* switch */

	x = spl6();
	if (setjmp(&u.u_qsave)) {
		/* 
		** Request has recieved a signal cleanup and return. 
		*/
		if (!(bp->b_flags & B_DONE))
			(*dil_info->dil_timeout_proc)(bp);
		u.u_eosys = EOSYS_NORESTART;
		bp->b_error = EINTR;
	} else
		while ((bp->b_flags & B_DONE) == 0)
			sleep((caddr_t) &bp->b_flags, DILPRI);
	splx(x);

	if (bp->b_error)
		bp->return_data = 0;		/* default return value */

	if (return_info)
		if (newlib)
			new_arg->data[0] = bp->return_data;
		else
			old_arg->data[0] = bp->return_data;

	if (newlib && new_arg->sc_state & PASS_IN) {
		new_arg->sc_state |= PASS_OUT;
		if (sc->state & IN_HOLDOFF)
			new_arg->sc_state |= TI9914_HOLDOFF;
		else
			new_arg->sc_state &= ~TI9914_HOLDOFF;
	}

	release_buf(bp);
	return(bp->b_error);
}


enum dil_util_state     {udil_start = 0, udil_action, udil_end, udil_timedout, udil_defaul};

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
**	Global variables/fields referenced: bp->b_sc->int_lvl, bp->b_queue
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
dil_util_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, dil_util, bp->b_sc->int_lvl, 0, udil_timedout);
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
**	Kernel routines called:  iod_abort_io, 
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
dil_util(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register struct isc_table_type *sc = bp->b_sc;
	int (*test_func)(); /* ptr to serial poll function */

	try
		START_FSM;
		switch ((enum dil_util_state) iob->b_state) {
		case udil_start:
			iob->b_state = (int) udil_action;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) udil_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(dil_util_timeout)
			if (!dil_get_selcode(bp, dil_util))
				break;
		case udil_action:
			END_TIME
			iob->b_state = (int) udil_end;
			DIL_START_TIME(dil_util_timeout)
			if (!(*info->dil_action)(bp))
				break;
		case udil_end:
			END_TIME
			iob->b_state = (int) udil_defaul;
			dil_drop_selcode(bp);/* drop channel */
			queuedone(bp);
			break;
		case udil_timedout:
			if (((sc->card_type == INTERNAL_HPIB) ||
			   (sc->card_type == HP98624)) &&	
			   (sc->transaction_state == 0x6))
			   test_func = sc->iosw->iod_spoll;

			/* Check if spoll timed out waiting for a response
			   from a device powered off or no such device */

 			/* See TI9914_spoll FSM states :
			   0x6 get_spoll_byte
			   0xa resend_atn */

			if (((((sc->card_type == INTERNAL_HPIB) ||
			   (sc->card_type == HP98624)) &&	
			   (sc->transaction_state == 0x6)) &&
			   (sc->transaction_proc == test_func)) &&
			   (info->dil_action == test_func)) {
				ABORT_TIME;
				iob->b_state = (int) udil_timedout;

				/* Serial poll disable */
				sc->transaction_state = 0xa;
				DIL_START_TIME(dil_util_timeout)
				if (!(*info->dil_action)(bp))
				   escape(TIMED_OUT);
 				}
			escape(TIMED_OUT);
		default:
			panic("unrecognized dil_util state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) udil_defaul;
		if (escapecode == TIMED_OUT)
			bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		(*sc->iosw->iod_abort_io)(bp);
		dil_drop_selcode(bp);
		queuedone(bp);
		dil_dequeue(bp);
	}
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
dil_utility_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, dil_utility, bp->b_sc->int_lvl, 0, dil_timedout);
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
dil_utility(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register struct isc_table_type *sc = bp->b_sc;

	try
		START_FSM;
		switch ((enum dil_ioctl_state) iob->b_state) {
		case dil_start:
			iob->b_state = (int) dil_end;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) dil_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(dil_utility_timeout)
			if (!dil_get_selcode(bp, dil_utility))
				break;
		case dil_end:
			END_TIME
			iob->b_state = (int) dil_defaul;
			DIL_START_TIME(dil_utility_timeout)
			(*info->dil_action)(bp);
			END_TIME
			dil_drop_selcode(bp);/* drop channel */
			queuedone(bp);
			break;
		case dil_timedout:
			escape(TIMED_OUT);
		default:
			panic("unrecognized dil_utility state");
		}
		END_FSM;
	recover
		{                                 
		ABORT_TIME;                       
		iob->b_state = (int) dil_defaul;  
		if (escapecode == TIMED_OUT)      
			bp->b_error = EIO;        
		/* B_ERROR set in queuedone */
		(*sc->iosw->iod_abort_io)(bp);    
		dil_drop_selcode(bp);             
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
ppoll_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, hpib_ppoll, bp->b_sc->int_lvl, 0, dil_timedout);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by: 	HPIB_PPOLL case in hpib_ioctl 
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:  iod_pplset, iod_ppoll, iod_pplclr,
**		iod_abort_io, iod_pplclr, 
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
hpib_ppoll(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;

	try
		START_FSM;
		switch ((enum dil_ioctl_state) iob->b_state) {
		case dil_start:
			iob->b_state = (int) dil_end;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) dil_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(ppoll_timeout)
			if (!dil_get_selcode(bp, hpib_ppoll))
				break;
		case dil_end:
			END_TIME
			iob->b_state = (int) dil_defaul;
			DIL_START_TIME(ppoll_timeout)
			dil_priority = splx(sc->int_lvl);
			(*sc->iosw->iod_pplset)(sc, 0xff, 0, 0);
			END_TIME
			bp->return_data = (*sc->iosw->iod_ppoll)(sc);
			(*sc->iosw->iod_pplclr)(sc, 0xff);
			splx(dil_priority);
			dil_drop_selcode(bp);/* drop channel */
			queuedone(bp);
			break;
		case dil_timedout:
			escape(TIMED_OUT);
		default:
			panic("unrecognized hpib_ppoll state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) dil_defaul;
		if (escapecode == TIMED_OUT) 
			bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		(*sc->iosw->iod_abort_io)(bp);
		(*sc->iosw->iod_pplclr)(sc, 0xff);
		splx(dil_priority);
		dil_drop_selcode(bp);/* drop channel */
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
**	Kernel routines called:  iod_pplclr, 
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
dil_ppoll_check(sc, resp)
register struct isc_table_type *sc;
register unsigned char resp;
{
	register struct buf *this_buf, *next_buf;
	for (this_buf = sc->ppoll_f; this_buf != NULL; this_buf = next_buf) {
		next_buf = this_buf->av_forw;
		if (resp & this_buf->b_ba) {
			if (this_buf->av_back != NULL)
				this_buf->av_back->av_forw = this_buf->av_forw;
			else
				sc->ppoll_f = this_buf->av_forw;
			if (this_buf->av_forw != NULL)
				this_buf->av_forw->av_back = this_buf->av_back;
			else
				sc->ppoll_l = this_buf->av_back;
			this_buf->b_bufsize = resp;
			(*this_buf->b_action)(this_buf);
		}
	}
	(*sc->iosw->iod_pplclr)(sc, resp);
}

enum ppoll_int_state	{get_sc = 0,
			 start_ppoll_int,
			 end_ppoll_int,
			 ppoll_int_timedout,
			 ppoll_int_defaul};

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
ppoll_int_timeout(bp)
struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, hpib_ppoll_int, bp->b_sc->int_lvl, 
						0, ppoll_int_timedout);
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
**	Kernel routines called: iod_pplset, iod_pplclr, iod_ppoll, 
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
hpib_ppoll_int(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register int response;
	int x;

	try
		START_FSM;
		switch ((enum ppoll_int_state) iob->b_state) {
		case get_sc:
			iob->b_state = (int) start_ppoll_int;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) ppoll_int_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(ppoll_int_timeout)
			if (!dil_get_selcode(bp, hpib_ppoll_int))
				break;
		case start_ppoll_int:
			END_TIME
			iob->b_state = (int) end_ppoll_int;

			x = spl6();
			/* check to see if the callers ppoll request is
			   already satisfied */
			(*sc->iosw->iod_pplset)(sc, 0xff, 0, 0);
			response = (*sc->iosw->iod_ppoll)(sc);
			(*sc->iosw->iod_pplclr)(sc, 0xff);
			if ((response ^ bp->pass_data2) & bp->pass_data1) {
				/* we have a match */
				iob->b_state = (int) ppoll_int_defaul;
				splx(x);
				bp->return_data = (response ^ bp->pass_data2) 
						  & bp->pass_data1;
				dil_drop_selcode(bp);
				queuedone(bp);
				break;
			}

			/* check if anyone else got satisfied */
			dil_ppoll_check(sc, ((response ^ sc->ppoll_sense) 
					     & sc->ppoll_mask));

			/* set this guy up for a ppoll interrupt */
			DIL_START_TIME(ppoll_int_timeout)
			HPIB_ppoll_int(bp, hpib_ppoll_int, bp->pass_data2);
			dil_drop_selcode(bp);/* drop channel */
			splx(x);
			break;
		case end_ppoll_int:
			END_TIME
			iob->b_state = (int) ppoll_int_defaul;
			x = spl6();
			if (bp->b_flags & B_DONE) {
				splx(x);
				break;
			}
			queuedone(bp);
			splx(x);
			break;
		case ppoll_int_timedout:
			escape(TIMED_OUT);
		default:
			panic("unrecognized ppoll_int_state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) ppoll_int_defaul;
		x = spl6();
		if (bp->b_flags & B_DONE) {
			splx(x);
			return;
		}
		HPIB_ppoll_clear(bp);
		bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		(*sc->iosw->iod_pplclr)(sc, 0xff);
		dil_drop_selcode(bp);
		queuedone(bp);
		splx(x);
		dil_dequeue(bp);
	}
}

enum wait_status_state	{status_get_sc = 0,
			 start_wait_status,
			 end_wait_status,
			 wait_status_timedout,
			 wait_status_defaul};

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
wait_status_timeout(bp)
struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, hpib_wait_status, bp->b_sc->int_lvl, 
						0, wait_status_timedout);
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
**	Kernel routines called:  iod_wait_set, 
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
hpib_wait_status(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct dil_info *info = (struct dil_info *) bp->dil_packet;
	register int x;

	try
		START_FSM;
		switch ((enum wait_status_state) iob->b_state) {
		case status_get_sc:
			iob->b_state = (int) start_wait_status;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) wait_status_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(wait_status_timeout)
			if (!dil_get_selcode(bp, hpib_wait_status))
				break;
		case start_wait_status:
			END_TIME
			iob->b_state = (int) end_wait_status;
			x = spl6();
			if ((*sc->iosw->iod_wait_set)(sc, &(info->intr_wait), 
				bp->pass_data1)) {
				splx(x);
				iob->b_state = (int) wait_status_defaul;
				dil_drop_selcode(bp);
				queuedone(bp);
				break;
			}
			HPIB_status_int(bp, hpib_wait_status);
			dil_drop_selcode(bp);
			DIL_START_TIME(wait_status_timeout);
			splx(x);
			break;
		case end_wait_status:
			END_TIME
			iob->b_state = (int) wait_status_defaul;
			dil_drop_selcode(bp);
			queuedone(bp);
			break;
		case wait_status_timedout:
			escape(TIMED_OUT);
		wait_status_default:
			panic("unrecognized wait_status_state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) wait_status_defaul;
		if (escapecode == TIMED_OUT)
			bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		HPIB_status_clear(bp);
		dil_drop_selcode(bp);
		queuedone(bp);
		dil_dequeue(bp);
	}
}

enum dil_intr_state	{dil_intr_start = 0,
			 dil_intr_set,
			 dil_intr_timedout,
			 dil_intr_defaul};

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
dil_intr_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, dil_intr, bp->b_sc->int_lvl, 0, dil_intr_timedout);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  HPIB_INTERRUPT_ENABLE case in hpib_ioctl
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called: iod_pplset, iod_ppoll, iod_pplclr, iod_intr_set,
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
dil_intr(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct dil_info *info = (struct dil_info *)bp->dil_packet;
	register int response;

	try
		START_FSM;
dil_reswitch:
		switch ((enum dil_intr_state) iob->b_state) {
		case dil_intr_start:
			iob->b_state = (int) dil_intr_set;
			if (selcode_busy(sc,info) && info->open_flags&FNDELAY) {
				iob->b_state = (int) dil_intr_defaul;
				bp->b_error = EACCES;
				/* B_ERROR set in queuedone */
				queuedone(bp);
				break;
			}
			DIL_START_TIME(dil_intr_timeout)
			if (!dil_get_selcode(bp, dil_intr))
				break;
		case dil_intr_set:
			END_TIME
			iob->b_state = (int) dil_intr_defaul;
			info->event = 0;
			if (info->cause & INTR_PPOLL) {
				if (!(sc->state & ACTIVE_CONTROLLER)) {
					iob->b_state = (int) dil_intr_defaul;
					bp->b_error = EINVAL;
					bp->b_flags |= B_ERROR;
					dil_drop_selcode(bp);/* drop channel */
					queuedone(bp);
					break;
				}
				sc->intr_wait |= INTR_PPOLL;
				info->intr_wait |= INTR_PPOLL;
				DIL_START_TIME(dil_intr_timeout);
				dil_priority = splx(sc->int_lvl);
				(*sc->iosw->iod_pplset)(sc, 0xff, 0, 0);
				END_TIME
				response = (*sc->iosw->iod_ppoll)(sc);
				(*sc->iosw->iod_pplclr)(sc, 0xff);

				/* check if anyone else got satisfied */
				dil_ppoll_check(sc, ((response ^ sc->ppoll_sense) 
						     & sc->ppoll_mask));

				/* set this guy up for a ppoll interrupt */
				DIL_START_TIME(dil_intr_timeout);	
				(*sc->iosw->iod_pplset)(sc, info->ppl_mask, 0, 1);
				END_TIME
				splx(dil_priority);
			}
			dil_priority = splx(sc->int_lvl);
			(*sc->iosw->iod_intr_set)(sc, &(info->intr_wait), info->cause);
			splx(dil_priority);
			dil_drop_selcode(bp);/* drop channel */
			queuedone(bp);
			break;
		case dil_intr_timedout:
			escape(TIMED_OUT);
		default:
			panic("unrecognized dil_intr state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		iob->b_state = (int) dil_intr_defaul;
		if (escapecode == TIMED_OUT)
			bp->b_error = EIO;
		/* B_ERROR set in queuedone */
		if (info->cause & INTR_PPOLL)
			(*sc->iosw->iod_pplclr)(sc, 0xff);
		splx(dil_priority);
		dil_drop_selcode(bp);/* drop channel */
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
hpib_x_transfer(bp)
struct buf *bp;
{
	register int i;
	register struct isc_table_type *sc = bp->b_sc;

	hpib_transfer(bp);
	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	} while (i);
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
hpib_transfer_timeout(bp)
struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, hpib_x_transfer, 
			bp->b_sc->int_lvl, 0, tfr_timedout);
}

#ifdef IOSCAN

dil_config_identify(sinfo)
register struct ioctl_ident_hpib *sinfo;
{
    register struct isc_table_type *isc;
    register struct buf *bp;
    register struct iobuf *iob;
    register struct dil_info *info;
    int busaddr, x;
    short ident;

    if ((isc = isc_table[sinfo->sc]) == NULL)
         return ENOSYS;

    if ( isc->card_type != HP98625 &&
         isc->card_type != HP98624 &&
         isc->card_type != INTERNAL_HPIB )
                return ENODEV;

    if (hpib_first_open) {
        hpib_first_open = 0;
        dil_init();
    }

    /* device open stuff that can be done only once */
    if ((bp = get_dilbuf()) == NULL) {
         return(ENOSYS);
    }

    iob = bp->b_queue;
    info = (struct dil_info *) bp->dil_packet;

    bp->b_sc = isc;          /* save selectcode this will be using */
    info->dil_procp = u.u_procp;       /* Save the process pointer and    */
    info->last_pid = u.u_procp->p_pid; /* process id for later use.       */
    info->open_flags = u.u_fp->f_flag;
    u.u_fp->f_buf = (caddr_t) bp;  /* save buf in the open file structure */
    iob->b_xaddr = (caddr_t) &ident;

    acquire_buf(bp);

    for (busaddr = 0; busaddr < 8; busaddr++) {

        /* device open stuff, dependent on busaddr */
        bp->b_ba = busaddr;          /* keep address */
        bp->b_error = 0;        /* clear errors */
        bp->b_flags &= ~(B_DONE | B_ERROR);
        bp->b_flags |= B_DIL;

        /* do the identify */
        info->dil_action = isc->iosw->iod_ident;
        info->dil_timeout_proc = dil_utility_timeout; 
        iob->activity_timeout = (250*HZ)/1000;  /* grabbed timeout from cs80 */
        bp->b_action = dil_utility;
        enqueue(iob, bp);

        x = spl6();
        if (setjmp(&u.u_qsave)) {
                /*
                ** Request has recieved a signal cleanup and return.
                */
                if (!(bp->b_flags & B_DONE))
                        (*info->dil_timeout_proc)(bp);
                u.u_eosys = EOSYS_NORESTART;
                bp->b_error = EINTR;
        } else
                while ((bp->b_flags & B_DONE) == 0)
                        sleep((caddr_t) &bp->b_flags, DILPRI);
        splx(x);

        if (bp->b_error) {
            sinfo->dev_info[busaddr].ident = -1;
        } else sinfo->dev_info[busaddr].ident = *(short *)iob->b_xaddr;

        /* device close (busaddr dependent) */
    }
    /* device close, busaddr independent, can be done once */
    release_buf(bp);
    release_dilbuf(bp);

    return 0;
}
#endif /* IOSCAN */
