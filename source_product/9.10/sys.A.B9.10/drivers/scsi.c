/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/drivers/RCS/scsi.c,v $
 * $Revision: 1.2 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/07/14 13:16:10 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987  Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
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
** What string for patches.  When building a patch use:
**
**	"CCOPTS=-DSCSI_PATCH make scsi.o"
*/
#if defined(SCSI_PATCH)
static char scsi_patch_id[] =
"@(#) PATCH_9.0: scsi.c $Revision: 1.2 $ $Date: 92/07/14 13:16:10 $";
#endif /* SCSI_PATCH */

#include <param.h>
#include <_param.h>
#include <buf.h>
#include <timeout.h>
#include <hpibio.h>
#include <dir.h>
#include <_types.h>
#include <user.h>
#include <systm.h>
#include <proc.h>
#include <conf.h>
#include <file.h>
#include <dma.h>
#include <intrpt.h>
#include <iobuf.h>
#include <tryrec.h>
#include <bootrom.h>
#include <scsi.h>
#include <_scsi.h>
#include <diskio.h>
#include <dk.h>
#include <mbuf.h>
#include <malloc.h>

#define SCSI_MALLOC(p, type)	MALLOC(p,type,sizeof(*(p)),M_IOSYS,M_WAITOK);
#define SCSI_FREE(p)		FREE(p, M_IOSYS)

int scsi_max_retries = 2;

#ifdef AUTOCHANGER 
int scsi_bp_lock;
struct buf *scsi_ctl_bp;
#define BP_IN_USE 0x1
#define BP_WANTED 0x2
#endif /* AUTOCHANGER */

/*
** defines for iobuf's io_flags
*/
#define io_flags	b_flags
#define SDTR_DONE 	0x10000000
#define SDTR_ENABLE	0x20000000

/*
** timeout and retry parameters
*/
unsigned SELECT_TICKS = 5 * HZ;
unsigned WATCHDOG_TICKS = HZ;
unsigned DISCONNECT_TICKS = 60 * HZ;

#define MAX_LUN_PER_SCSI_DEV		8
#define MAX_SCSI_DEV_PER_SELCODE 	8
#define MAX_SCSI_SELCODE		32

struct scsi_lun {
	unsigned 	open_cnt;	/* unit open count (block + raw)  */
	unsigned 	raw_open_cnt;	/* raw device open count          */
	dev_t		dev_minor;	/* device minor number            */
	int		flags;		/* driver flags                   */
	long		lunsize;	/* size of lun in logical blocks  */
	char		log2blk;	/* log (base 2) of block size     */
	char		inquiry_sz;	/* Size of inquiry data field     */
	struct inquiry_2 inq_data;	/* inquiry data returned by device*/
	struct stash	*stash;
	struct iobuf	*io_queue;	/* I/O queue head */
	struct scsi_cmd_parms cmd_parms;/* cmd mode parameters            */
	struct sense_2	sense;		/* scsi device status             */
	struct sw_intloc intloc;	/* sw_trigger structure */
#if !defined(NOT_SCSI_MONITOR)
	int		dk_busy_bit;	/* bit mask for disk statistics	*/
	char		dk_index;	/* index for disk statistics	*/
#endif /* NOT_SCSI_MONITOR */
};

/*
** scsi_lun->flags
*/
#define EXCLUSIVE		0x01	/* exclusive open */
#define CMD_MODE		0x02	/* command mode synchronization */
#define CAPACITY_DONE		0x04
#define TEAC_FLOPPY		0x08
#define OPEN_SUCCEEDED		0x10	/* enables change while closed */
#define MEDIUM_CHANGED		0x20	/* for SIOC_MEDIUM_CHANGED ioctl */
#define SCSI_QUIET		0x40
#define NOT_FIRST_SENSE		0x80	/* silence first unit attention */

/*
 * open device table entry - one per physical device (controller)
 */
struct scsi_dev {
	struct scsi_lun	*lun[MAX_LUN_PER_SCSI_DEV];	/* open units */
	unsigned	open_cnt;		/* number of open units */
	struct iobuf	io_queue;	/* I/O queue */
	struct stash {
		int	flags;
		long	bcount;
		long	resid;
		int	(*action2)();
	}		stash;		/* saved from buf for request sense */
};

struct scsi_card {
	struct scsi_dev	*dev[MAX_SCSI_DEV_PER_SELCODE];	/* open devs */
	unsigned	open_cnt;		/* number of open devs */
}	*scsi_card[MAX_SCSI_SELCODE];

int	scsi_blk_major, scsi_raw_major;

#define SCSI_SC_MASK		0x1f0000
#define SCSI_BA_MASK		0x000700
#define SCSI_LU_MASK		0x000070
#define SCSI_SHORT_XFER_CMD	0x000002	/* use 6 byte read/write */

#define SCSI_DEV_MASK	(SCSI_SC_MASK | SCSI_BA_MASK | SCSI_LU_MASK)

/*
** SCSI Commands are contained in a command command set file:
** scsi_ccs.c.  They are declared external below.
*/
extern six_byte_cmd();
extern ten_byte_cmd();
extern scsi_request_sense();
extern scsi_read_capacity();
extern scsi_xfer_cmd();
extern scsi_short_xfer_cmd();
extern scsi_format_unit();
extern scsi_inquiry();
extern scsi_prevent_media();
extern scsi_allow_media();
extern scsi_test_unit();
extern scsi_mode_sense_cmd();
extern scsi_mode_select();

extern scsi_if_dequeue();

struct scsi_lun *m_scsi_up(dev)
register dev_t dev;
{
	VASSERT(major(dev) != 0xff && minor(dev) != 0xffffff);
	VASSERT(major(dev) == scsi_blk_major || major(dev) == scsi_raw_major);
	VASSERT((unsigned)m_selcode(dev) < MAX_SCSI_SELCODE);
	VASSERT(scsi_card[m_selcode(dev)] != NULL);
	VASSERT(scsi_card[m_selcode(dev)]->dev[m_busaddr(dev)] != NULL);
	VASSERT(scsi_card[m_selcode(dev)]->dev[m_busaddr(dev)]
		->lun[m_unit(dev)] != NULL);
	return(scsi_card[m_selcode(dev)]->dev[m_busaddr(dev)]
		->lun[m_unit(dev)]);
}

msg_hex_dmp(sptr, nbytes)
register unsigned char *sptr;
int nbytes;
{
	register int i;

	for (i = 0; i < nbytes; i++)
		msg_printf("%s%02x", i % 16 ? " " : "\n\t", *sptr++);
	msg_printf("\n");
}

scsi_teac_read(bp)
struct buf *bp;
{
	struct scsi_lun *up = m_scsi_up(bp->b_dev);

	SCSI_TRACE(CMDS)("scsi_read\n");

	bp->b_flags |= B_READ;
	bp->b_resid = bp->b_bcount = 1 << up->log2blk;
	bp->b_queue->b_xcount = bp->b_bcount;

	ten_byte_cmd(bp, CMDread_ext, m_unit(bp->b_dev),
		bp->b_parms == -1 ? up->lunsize - 1 : bp->b_parms, 1, 0);
}

/*
 * Decode SCSI Status:
 *	Summary of exit conditions
 *	 (return(0) indicates "no retry")
 *
 *	Return Value Matrix
 *	Condition			   Return value
 *      ---------                          ------------
 *	Hardware Failure			0 (No Retry)
 *	Hardware Recovered from error		0 (Log error and continue)
 *	Too Many Retries (>1)			0 (Give up)
 *	"Soft" Hardware Failure			1 (Indicate retry)
 */
scsi_decode_sense(bp, up)
register struct buf *bp;
register struct scsi_lun *up;
{
	register int error = 0, retry_requested = 0;
	register struct sense_2 *sptr = &up->sense;
	char *un = "";
	
	if (!(up->flags & SCSI_QUIET) && (up->flags & NOT_FIRST_SENSE)) {
	  if (sptr->key != 0x6 || sptr->code != 0x28) {	/* autochanger */
	    msg_printf("SCSI: dev = 0x%x, offset = 0x%x, bcount = 0x%x",
		bp->b_dev, bp->b_offset, up->stash->bcount);
	    msg_printf(" b_flags = 0x%x\n", up->stash->flags);
	    msg_printf("\tsense key = 0x%x, code = 0x%x, qualifier = 0x%x",
		sptr->key, sptr->code, sptr->qualifier);
	    if (sptr->info_valid)
		msg_printf(", info = 0x%x", *((int *)sptr->info));
	    msg_printf("\n");
	    msg_printf("\thexadecimal sense dump:\n\t");
	    msg_hex_dmp(sptr, bp->b_bcount - bp->b_resid);
	  }
	}
	up->flags |= NOT_FIRST_SENSE;


	if (sptr->error_code == SCSI_DEFERRED_ERROR)
	{
		if (sptr->key == 0x01)
		{
			msg_printf("SCSI: deferred error\n");
			retry_requested++;
		}
		else
		{
			msg_printf("SCSI: unrecoverable deferred error\n");
			msg_printf("\tkey = 0x%x, code = 0x%x\n",
					sptr->key, sptr->code);
			panic("SCSI: unrecoverable deferred error");
		}
	}
	else
	{

	switch (sptr->key) {
	case 0x03:
		error = EIO;
		if (up->stash->action2 == scsi_teac_read)
			break;
		un = "un";
	case 0x01: /* Recovered Error */
		if (!(up->flags & SCSI_QUIET) && sptr->info_valid) {
	    		msg_printf("SCSI: %s%s", un, "recoverable I/O error");
			msg_printf(" on dev 0x%x at lba 0x%x\n",
				minor(bp->b_dev), *((int *)sptr->info));
		}
	case 0x00: /* NO SENSE */
		break;
	case 0x0A: /* Copy Aborted */
	case 0x0B: /* Aborted Command */
		retry_requested++;
		break;
	case 0x06: /* Unit Attention */
		switch (sptr->code) {
		case 0x00: /* No Additional Sense */
		case 0x17: /* Recovered Read Data with Target's Read Retries */
		case 0x18: /* Recovered Read Data w. Target's ECC Correction */
		case 0x1E: /* Recovered ID with Target's ECC Correction */
		case 0x2A: /* Mode Select Parameters Changed */
			break;
		case 0x28: /* Medium Changed */
			up->flags |= MEDIUM_CHANGED;
		case 0x29: /* Power On or Reset or Bus Device Reset */
			if (bp->b_scsiflags & POWER_ON &&
					bp->b_flags & S_RETRY_CMD) {
				error = EIO;
				break;
			}
			/* Power on / reset gets two tries! */
			bp->b_scsiflags |= POWER_ON;
			bp->b_newflags &= ~SDTR_SENT;
			up->io_queue->io_flags &= ~SDTR_DONE;
			up->flags &= ~CAPACITY_DONE;
			bp->b_flags &= ~S_RETRY_CMD;
		case 0x47: /* SCSI Interface Parity Error */
		case 0x48: /* Initiator Detected Error */
			retry_requested++;
			break;
		default:
			error = EIO;
			break;
		}
		break;
	default:
		error = EIO;
		break;
	}
	}

	if (error || bp->b_flags & S_RETRY_CMD)
		retry_requested = 0;
	else
		bp->b_flags |= S_RETRY_CMD;

	if (bp->b_error = error)
		bp->b_flags |= B_ERROR;

	return(retry_requested);
}

scsi_cmd(bp)
register struct buf *bp;
{
	register struct scsi_lun *up = m_scsi_up(bp->b_dev);
	register struct scsi_cmd_parms *parms = &up->cmd_parms;

	SCSI_TRACE(CMD_MOD + CMDS)	
	    ("SCSI: scsi_cmd: %d-byte cmd; cmd_mode = 0x%x; clock_ticks = %d:", 
			parms->cmd_type, parms->cmd_mode, parms->clock_ticks);
	if (SCSI_DEBUG & CMDS + CMD_MOD)
		msg_hex_dmp(parms->command, parms->cmd_type);

	bp->b_clock_ticks = parms->clock_ticks;
	scsi_if_transfer(bp,parms->command,parms->cmd_type,B_WRITE,MUST_FHS);
}

/* The device disconnected, and never returned. */
scsi_discon_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: disconnect timed out; dev = 0x%x\n", bp->b_dev);

	scsi_if_remove_busfree(bp);

	TIMEOUT_BODY(iob->intloc,
		scsi_if_dequeue, bp->b_sc->int_lvl, 0, discon_TO);
}

/* The data transfer timed out */
scsi_req_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: request timed out; dev = 0x%x; state = 0x%x\n",
		bp->b_dev, bp->b_queue->b_state);

	TIMEOUT_BODY(iob->intloc,
		scsi_if_dequeue, bp->b_sc->int_lvl, 0, transfer_TO);
}

scsi_select_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: select timed out; dev = 0x%x\n", bp->b_dev);

	scsi_if_term_arbit(bp);  /* Terminate Arbitration phase */
		
	TIMEOUT_BODY(iob->intloc, scsi_if_dequeue,
		bp->b_sc->int_lvl, 0, select_TO);
}

scsi_dequeue(bp)
register struct buf *bp;
{
	/*
	** Some would say it is incorrect to sw_trigger to a level
	** greater than or equal to the current level, but it works just
	** great.  The result being that the triggered routine is called
	** before sw_trigger returns.
	** We could duplicate the level checking code which is called
	** by sw_trigger, but why bother?
	*/
	sw_trigger(&(m_scsi_up(bp->b_dev))->intloc,
			scsi_if_dequeue, bp, bp->b_sc->int_lvl, 0);
}

/*
** these belong in scsi.h
*/
#define BUS_BUSY	1
#define NOT_OWNER	2

/*
 * The SCSI Finite State Machine is a complex beast.
 * It is driven by two mechanisms:
 *	- The phase requested by the target
 *	- by software
 * It is sometimes necessary to remember the previous phase 
 * of the bus.  This is stored in the bp struct: b_phase
 */
scsi_fsm(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct scsi_lun *up = m_scsi_up(bp->b_dev);
	register unsigned char *ptr;
	register int ret, len;
	unsigned char msg[32];
	unsigned char xfr_period, reqack;

retry:	try

	START_FSM;

reenter:

	SCSI_TRACE(STATES)("FSM: dev = 0x%x, state = 0x%x\n",
		bp->b_dev, iob->b_state);

	END_TIME;

	switch (iob->b_state) {
	case initial:
		iob->busy_cnt = 0;
		/* fall through */
	case busy_retry:
		iob->b_state = select;
		get_selcode(bp, scsi_fsm);
		break;
	
	case select:
		/* The decision to select with ATN should 
		 * be made before entering FSM
		 */
		/* Problem: Getting Select Code is done asynchronously
		 *       to the chip's hardware. (Another target may reselect
		 *       initiator to continue transaction.)  Thus with
		 *       multiple devices and overlapped activities, it is
		 *       is quite possible that although we got the select
		 *       code,  the bus is not free. 
		 */
		iob->io_sanity = 0;
		bp->b_phase = SCSI_SELECT;
		START_TIME(scsi_select_timedout, SELECT_TICKS);
		/* If we enter scsi_select and discover that either the bus
		 * is not free, or select code is not owned by us, we return,
		 * expecting the interrupting process (the device reselecting
		 * the host), to drop the select code for us!
		 * If we software time out from the select something
		 * is seriously wrong.  We will escape.
		 */
		bp->b_flags &= ~CONNECTED;
		if (ret = scsi_if_select(bp)) { /* Did not issue Select */
		    END_TIME;
		    if (ret == NOT_OWNER) {
			SCSI_TRACE(STATES)
				("Select (%d): not owner\n", bp->b_ba);
			get_selcode(bp, scsi_fsm);
		    }
		}
		break;

	case select_nodev:
		/* No device */
		iob->b_state = defaul;
		drop_selcode(bp);
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		SCSI_TRACE(DIAGs)("scsi: no device at %x\n",bp->b_dev);
		if (bp->b_flags & ABORT_REQ) {
			bp->b_flags &= ~ABORT_REQ;
			scsi_if_clear_reselect(bp);
		}
		queuedone(bp);
		break;

	case select_TO:
		/* Select has timedout.  Suspect hardware failure.
		 */
		 msg_printf("scsi: select cmd timed out %x\n", bp->b_dev);
		 escape(SELECT_TO);
	
	case discon_TO:
		/* Disconnect timed out - Send Abort Message to clear device 
		 * Do not take off call_to_do list yet - wait until mesg_out.
		 */
		scsi_if_clear_reselect(bp);
		bp->b_flags |= ABORT_REQ | ATN_REQ;
		iob->b_state = (int)initial;
		goto reenter;
	
	case transfer_TO:
		/* Transfer timed out - serious problem? Retry cmd once */
		escape(TIMED_OUT);
	
	case reselect:
		iob->b_state = set_state;
		bp->b_flags |= CONNECTED;	/* Mark buf as connected */
		bp->b_phase = (int)SCSI_RESELECT;
		if (bp->b_flags & ABORT_REQ)
			break;	/* Exceptional case - already got selcode */
		get_selcode(bp, scsi_fsm);
		break;
	
	case set_state:
		if (scsi_set_state(bp) == 0)
			goto reenter;
		set_smart_poll(scsi_dequeue, bp);
		break;

	case phase_mesg_in:
		/* A corner case that must be checked if ABORT msg required */
		if (bp->b_flags & ABORT_REQ)
			bp->b_flags |= ATN_REQ;
		START_TIME(scsi_req_timedout, WATCHDOG_TICKS);
		len = scsi_if_mesg_in(bp, msg);/* Get all message bytes */
		END_TIME;
                iob->b_state = set_state;
		ptr = msg;

		/*
		** Kludge. At least one revision of the SPIFFY controller
		** can get confused due to the Fujitsu
		** ATN glitch and send a garbage byte immediately after
		** reselection.  For all other devices, this first message
		** in byte will be an IDENTIFY.  Since we don't use the
		** info anyway, we just toss the byte and continue.
		*/
		if (bp->b_phase == (int)SCSI_RESELECT) {
			len--;
			ptr++;
		}
		bp->b_phase = (int)SCSI_MESG_IN;

		while (len--) {
		    switch (*ptr)	{
		    case MSGsave_data_ptr:
			SCSI_TRACE(MESGs)("SCSI mesg in: save_data_ptr\n");
			break;

		    case MSGrestore_ptr:
			SCSI_TRACE(MESGs)("SCSI mesg in: restore_ptr\n");
			break;

		    case MSGno_op:
			SCSI_TRACE(MESGs)("SCSI mesg in: no_op\n");
			break;

		    case MSGdisconnect:
			bp->b_flags &= ~CONNECTED;
			SCSI_TRACE(MESGs)("SCSI mesg in: disconnect\n");
			/* Do not set iob->b_state here.  It will 
			 * be initialized by low-level interface routine.
			 */
			bp->b_nextstate = (int)reselect;
			scsi_if_wait_for_reselect(bp, scsi_fsm);
			START_TIME(scsi_discon_timedout, bp->b_clock_ticks);
			break;
			
		    case MSGmsg_reject:
			SCSI_TRACE(MESGs)("SCSI mesg in: Msg Reject\n");
			if (bp->b_newflags & SDTR_SENT) {
			    bp->b_newflags &= ~SDTR_SENT;
			    up->io_queue->io_flags |= SDTR_DONE;
			    iob->io_sync = 0;
			    SCSI_TRACE(MESGs)
				("SCSI: sync negotiation rejected\n"); 
			} else
			    SCSI_TRACE(MESGs)
			      ("SCSI Unknown msg reject (dev: %x)\n",bp->b_dev);
			break;

		    case MSGcmd_complete:
#if !defined(NOT_SCSI_MONITOR)
			if (up->dk_index >= 0) {
			    dk_busy &= ~up->dk_busy_bit;
			    if (bp->b_action2 == scsi_xfer_cmd
				    || bp->b_action2 == scsi_short_xfer_cmd)
				dk_wds[up->dk_index]
					+= (bp->b_bcount - iob->b_xcount) >> 6;
			}
#endif /* NOT_SCSI_MONITOR */
			bp->b_flags &= ~CONNECTED;
			SCSI_TRACE(MESG_v)("SCSI mesg in: cmd complete\n");

			if (!(iob->io_sanity & io_cmd) ||
			    !(iob->io_sanity & io_stat) )
				SCSI_SANITY("MSGcmd_complete");

		/* 'cur_status' is current status returned from this cmd.
		 * If we issue the 'request_sense' command we will reuse
		 * the buf header for this command and use 'save_addr' to
		 * save the buffer address from original cmd. Thus 'save_addr'
		 * is non-zero if and only if we are obtaining additional sense
		 * for the previous command.
		 */
			if (bp->b_status) {
				if (iob->save_addr) {
				    /* We received bad status from 'request
				     * sense'.  Should not happen!
				     */
				    bp->b_bcount  = up->stash->bcount;
				    bp->b_flags   = up->stash->flags;
				    bp->b_un.b_addr = (char *)iob->save_addr;
				    iob->save_addr = 0;
				    msg_printf("SCSI: Bad status from sense\n");
				    escape(NO_SENSE);
				    }
				/* Retry command if we get 'check condition' */
				if (bp->b_status == check_condition) {
				    /* Save information in order to retry cmd */
				    iob->save_addr = (int)bp->b_un.b_addr;
				    bp->b_un.b_addr = (char *)&up->sense;
				    up->stash->bcount = bp->b_bcount;
				    up->stash->resid = bp->b_resid;
				    up->stash->flags = bp->b_flags;
				    up->stash->action2 = bp->b_action2;
				    /* Set up buf for request sense - we
				       need to send a message out */
				    bp->b_flags |= ATN_REQ;
				    iob->b_state = (int)select;
				    bp->b_action2 = scsi_request_sense;
				    goto reenter;
				    }
				else if (bp->b_status == busy) {
				    if (iob->busy_cnt++ < scsi_max_retries) {
					drop_selcode(bp);
					iob->b_state = (int)busy_retry;
					bp->b_flags |= ATN_REQ;
					Ktimeout(scsi_dequeue,bp,HZ,NULL);
					break;
					}
				    else {
					bp->b_error  = EBUSY;
					bp->b_flags |= B_ERROR;
				        }
				    }
				else { /* Do not retry command */
				    bp->b_error  = ENXIO;
				    bp->b_flags |= B_ERROR;
				    }
				}
			if (iob->save_addr) { /* We requested sense */
				bp->b_un.b_addr = (char *)iob->save_addr;
				iob->save_addr = 0;
				/* Just returned from request sense - check */
				if (bp->b_action2 != scsi_request_sense)  {
				   msg_printf("SCSI: inconsistency in sense\n");
				   }
				bp->b_action2 = up->stash->action2;
				bp->b_flags   = up->stash->flags;
				/*
				** Special case code here for CD-ROM install.
				** This whole check condition recovery code
				** needs to be redone.
				*/
				if (up->sense.key == 2) {
				    if (iob->busy_cnt++ < scsi_max_retries) {
					drop_selcode(bp);
					iob->b_state = (int)busy_retry;
					bp->b_bcount = up->stash->bcount;
					bp->b_flags |= ATN_REQ;
					Ktimeout(scsi_dequeue,bp,HZ,NULL);
					break;
				    } else {
					bp->b_error = EBUSY;
					bp->b_flags |= B_ERROR;
				    }
				} else if (scsi_decode_sense(bp, up)) {
				    /* We're retrying command */
				    iob->b_state  = (int)select;
				    bp->b_bcount  = up->stash->bcount;
				    bp->b_resid = up->stash->resid;
				    bp->b_flags |= ATN_REQ;
				    goto reenter;
				}
				/* Use previous status */
				bp->b_sensekey = up->sense.key;
				bp->b_resid = up->stash->resid;
				bp->b_bcount = up->stash->bcount;
			}
			iob->b_state = (int)defaul;
			drop_selcode(bp);
			if (bp->b_error) {
			    bp->b_flags |= B_ERROR;
			    SCSI_TRACE(DIAGs)
				("SCSI: b_error = %d\n", bp->b_error);
			}
			queuedone(bp);
			break;

		    case MSGext_message:
			SCSI_TRACE(MESGs)("SCSI: extended mesg\n");
			if (--len < (ret = *++ptr)) {
				len = 0;
				break;
			}
			len--; ret--;
			if (*++ptr == MSGsync_req
					&& (bp->b_newflags & SDTR_SENT)) {
			    /*
			    ** Compute value for Fujitsu tmod register
			    ** and wether or not to use the 10MHz input
			    ** if available
			    */

			    xfr_period = *++ptr; len--;
			    reqack     = *++ptr; len--;

			    iob->io_sync = 0;
			    if (50 <= xfr_period && xfr_period <= 157
					&& reqack > 0) {
				if (reqack < 8)
					iob->io_sync = reqack << 4;
				iob->io_sync
					|= xfr_period > 157 ? 0
					:  xfr_period > 125 ? 0x8c
#if !defined(NOT_SCSI_5MB)
					:  xfr_period > 100 ? 0x88
#endif /* NOT_SCSI_5MB */
					:  xfr_period >  94 ? 0x188
#if !defined(NOT_SCSI_5MB)
					:  xfr_period >  75 ? 0x84
#endif /* NOT_SCSI_5MB */
					:  xfr_period >  63 ? 0x184
#if !defined(NOT_SCSI_5MB)
					:  xfr_period >  50 ? 0x80
#endif /* NOT_SCSI_5MB */
					:  0x180;
			    }

#if !defined(NOT_SCSI_MONITOR)
			    /*
			    ** s/w = 2 b/w * (xfr_period * 4) ns/b * 10^-9 s/ns
			    */
			    if (up->dk_index >= 0 && reqack)
				dk_mspw[up->dk_index] = xfr_period * 8.0e-9;
#endif /* NOT_SCSI_MONITOR */

			    /* Indicate device has responded to sync data */
			    up->io_queue->io_flags |= SDTR_DONE;
                            bp->b_newflags &= ~SDTR_SENT;
			    SCSI_TRACE(MESGs)
				("SCSI: sync xfr enabled: (io_sync = 0x%x)\n",
					iob->io_sync);
			} else {
				msg_printf("SCSI: unexpected msg:");
				msg_hex_dmp(ptr - 2, ret + 3);
				ptr += ret;
				len -= ret;
			}
			break;
		    default:
			if(*ptr&MSGidentify) {
			    SCSI_TRACE(MESG_v)("SCSI mesg in: identify %x\n", *ptr);
			    }
			else	{
			    msg_printf("SCSI FSM: Unknown msg_in byte: %x\n", *ptr);
			    /* Assume parity error */
			    escape(PARITY_ERR);
			    }
			break;
		     }
		  ptr++;
		}
		if (bp->b_flags & CONNECTED)
			goto reenter;
		else
			break;
	
	case phase_mesg_out:
		len = 0;
		if (bp->b_phase == SCSI_SELECT) {
			if (up->dk_index >= 0) {
			    dk_busy |= up->dk_busy_bit;
			    if (bp->b_action2 == scsi_xfer_cmd
				    || bp->b_action2 == scsi_short_xfer_cmd) {
				dk_xfer[up->dk_index]++;
				dk_seek[up->dk_index]++;
			    }
			}
			msg[len++] = MSGidentify + m_unit(bp->b_dev)
			    + ((bp->b_flags & ATN_REQ) ? MSGident_discon : 0);
			if (bp->b_flags & ABORT_REQ) {
				msg[len++] = MSGabort;
			} else if ((up->io_queue->io_flags
						& (SDTR_DONE+SDTR_ENABLE))
					== SDTR_ENABLE) {
				iob->io_sync = 0;
			 	len += ret = scsi_if_request_sync_data(bp,
					&msg[len]);
				if (ret == 5)
					bp->b_newflags |= SDTR_SENT;
			}
		} else if (bp->b_newflags & REJECT_SDTR) {
			msg[len++] = MSGmsg_reject;
			bp->b_newflags &= ~REJECT_SDTR;
			iob->io_sync = 0;
			iob->b_flags &= ~SDTR_DONE;
		}

		if (len == 0)
			msg[len++] = MSGno_op;

		bp->b_flags &= ~ATN_REQ;
		bp->b_phase = (int)SCSI_MESG_OUT;

		/* scsi_if_mesg_out can fail under one common scenario:
		 * the driver is attempting an SDTR with a device
		 * that does not accept multi-byte message packets.
		 */
		START_TIME(scsi_req_timedout, WATCHDOG_TICKS);
		scsi_if_mesg_out(bp, msg, len);
		END_TIME;

		if (bp->b_flags & ABORT_REQ) {
			bp->b_flags &= ~ABORT_REQ;
			escape(TIMED_OUT);
		}

		iob->b_state = set_state;
		goto reenter;
	
	case phase_cmd:
		bp->b_phase = (int)SCSI_CMD;
		if (iob->io_sanity)
			SCSI_SANITY("phase_cmd");
		else
			iob->io_sanity |= io_cmd;
		/* iob->b_xcount and iob->b_xaddr hold  the number of bytes 
		 * to be transfered and the current address of buffer.
		 * These values are passed to scsi_if_transfer and updated
		 * after completion of command.
		 */
		iob->b_xcount = bp->b_bcount;
		iob->b_xaddr  = bp->b_un.b_addr;
		START_TIME(scsi_req_timedout, WATCHDOG_TICKS);
		(*bp->b_action2)(bp);
		break;
	
	case phase_data_in:
	case phase_data_out:
		if (iob->io_sanity&io_cmd)
			iob->io_sanity |= io_data;
		else 
			SCSI_SANITY("data in / out");
		bp->b_phase = (int)SCSI_DATA_XFR;
		SCSI_TRACE(STATES)("SCSI data xfr count: %x addr: 0x%x\n",
			iob->b_xcount,	iob->b_xaddr);

		START_TIME(scsi_req_timedout, WATCHDOG_TICKS);

		if (!(bp->b_flags & B_READ)
				|| hidden_mode_exists(bp->b_sc->my_isc)
				|| bp->b_action2 == scsi_xfer_cmd
				|| bp->b_action2 == scsi_short_xfer_cmd)
		{
			scsi_if_transfer(bp, iob->b_xaddr, iob->b_xcount,
					bp->b_flags & B_READ, MAX_SPEED);
		}
		else
		{
			scsi_if_transfer(bp, iob->b_xaddr, iob->b_xcount,
					bp->b_flags & B_READ, MUST_FHS);
		}
		break;
	
	case phase_status:
		if (!(iob->io_sanity&io_stat))
			iob->io_sanity |= io_stat;
		else
			SCSI_SANITY("phase_status");
		bp->b_phase = (int)SCSI_STATUS;
		bp->b_resid -= (bp->b_bcount - bp->b_queue->b_xcount);
		START_TIME(scsi_req_timedout, WATCHDOG_TICKS);
		if ((bp->b_status = scsi_if_status(bp)) != -1)
			iob->b_state = set_state;
		END_TIME;
		if (bp->b_status) SCSI_TRACE(DIAGs)
			("SCSI: bad status = 0x%x\n", bp->b_status);
		goto reenter;
	
	case scsi_error:
		escape(PARITY_ERR);

	default:
		bp->b_flags &= ~CONNECTED;
		msg_printf("SCSI: Unknown state = 0x%x, phase = 0x%x,",
			iob->b_state, bp->b_phase);
		msg_printf(" iob->io_sanity = 0x%x\n", iob->io_sanity);
		escape(PHASE_WRONG);
		break;
	}
	END_FSM;
	recover	{
		/*
		 * SCSI Error Recovery: No "normal" entry is ever expected
		 *	Variables upon entry:
		 *	   - escapecode
		 *		PHASE_WRONG
		 *		TIMED_OUT
		 *		S_RETRY_CNT
		 *		PARITY_ERR
		 *		SELECT_TO
		 *	   - S_RETRY_CMD
		 *		boolean flag in bp
		 *	Algorithm:
		 *		ABORT_TIME
		 *		drop select code
		 *		reset bus
		 *		if (PHASE_WRONG || TIMED_OUT || PARITY_ERR) {
		 *			if (S_RETRY_CMD)
		 *				return EIO error & exit
		 *			else  {  reinitialize state variables
		 *				 return to FSM	}
		 *			}
		 *		else (SELECT_TO || S_RETRY_CNT) {
		 *			(Serious hardware problem suspected)
		 *			return EIO error & exit
		 *			}
		 */
		ABORT_TIME;

		msg_printf("SCSI: recover: dev = 0x%x, escapecode = %d\n",
				bp->b_dev, escapecode);
		msg_printf("\toffset = 0x%x, bcount = 0x%x\n",
				bp->b_offset, bp->b_bcount);

#if !defined(NOT_SCSI_MONITOR)
		if (up->dk_index >= 0)
			dk_busy &= ~up->dk_busy_bit;
#endif /* NOT_SCSI_MONITOR */

		/* Remove 'bp' from select code queue of buf's
		 * waiting for a reselection
		 */
		scsi_if_clear_reselect(bp);
		iob->b_state = defaul;
		iob->save_addr = 0;
		/* Always clean up bus & card */
		if (scsi_if_abort(bp))
			msg_printf("SCSI: scsi_if_abort failed; dev = 0x%x\n",
			bp->b_dev);
		drop_selcode(bp);
		if (!(bp->b_flags & S_RETRY_CMD
				|| escapecode == S_RETRY_CNT
				|| escapecode == SELECT_TO
				|| escapecode == NO_SENSE)) {
			bp->b_flags |= S_RETRY_CMD | ATN_REQ; 
			iob->b_state = initial;
			goto retry;
		}
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		queuedone(bp);
	}
}

/*
 *  scsi_nop - control operation for synchronizing closes
 */
scsi_nop(bp)
struct buf *bp;
{
	queuedone(bp);
}

scsi_driver_ctl(bp, order, ptr)
register struct buf *bp;
int order;
register char *ptr;
/* ptr will point to a new struct for communicating
 * between pseudo drivers and drivers
 */
{
	register struct scsi_lun *up = m_scsi_up(bp->b_dev);

	extern scsi_start_stop();
	extern scsi_move_medium();
	extern scsi_release();
	extern scsi_reserve();
	extern scsi_read_element_status();
	extern scsi_init_element_status();

	bp->b_flags |= ATN_REQ;
	bp->b_flags &= ~S_RETRY_CMD;
	bp->b_error = 0;
	bp->b_sc = isc_table[m_selcode(bp->b_dev)];
	bp->b_log2blk = up->log2blk;
	bp->b_ba = m_busaddr(bp->b_dev);

	/* initialize to common values before switch to save code */
	bp->b_action = scsi_fsm;
	bp->b_parms = 0;
	bp->b_clock_ticks = 240 * HZ;
	bp->b_s2 = (long)ptr;

	switch (order) {
	case FLUSH_QUEUE:	/* Flush iobuf queue */
		bp->b_clock_ticks = 0;
		bp->b_action = scsi_nop;
		break;
	case START_STOP:	/* Start / Stop device */
		bp->b_parms = *((int *)ptr);
		bp->b_clock_ticks = 20*HZ;
		bp->b_action2 = scsi_start_stop;
		break;
	case MOVE_MEDIUM:
		bp->b_action2 = scsi_move_medium;
		break;
	case INIT_ELEMENT_STATUS:
		bp->b_s2 = NULL;
		bp->b_clock_ticks = 360*HZ;
		bp->b_action2 = scsi_init_element_status;
		break;
	case READ_ELEMENT_STATUS:
		bp->b_parms = *((int *)ptr);
		bp->b_s2 = NULL;
		bp->b_clock_ticks = 360*HZ;
		bp->b_action2 = scsi_read_element_status;
		break;
	case RESERVE:
		bp->b_action2 = scsi_reserve;
		break;
	case RELEASE:
		bp->b_action2 = scsi_release;
		break;
	default:
		panic("unknown state in driver ctl");
	}
	enqueue(up->io_queue, bp);
}

/*
 *  this is a utility to set up for activities other than read/write.
 *  It is used internally for open activities, and also by ioctl.
 *  It knows scsi specific things.
 */
scsi_control(up, fsm, proc, dev, addr, clock_ticks, atn_flag, parm)
register struct scsi_lun *up;
int (*fsm)();
int (*proc)();
dev_t dev;
char *addr, parm;
{
        register struct buf *bp;
	int bpb;


#ifndef AUTOCHANGER
	bp = (struct buf *)geteblk(SZ_VAR_CNT);
#else
	if (proc ==  scsi_format_unit) {
		/* Don't tie up the buffer waiting for a media init to
                   occur.  This will block any attempted opens or closes */
		bp = (struct buf *)geteblk(SZ_VAR_CNT);
	} else {
		while (scsi_bp_lock & BP_IN_USE) {
			scsi_bp_lock |= BP_WANTED;
			sleep(scsi_bp_lock);
		}
		scsi_bp_lock |= BP_IN_USE;
		if (scsi_ctl_bp == NULL)
			scsi_ctl_bp = geteblk(SZ_VAR_CNT);
		bp = scsi_ctl_bp;
	}
	bp->b_flags = B_BUSY;	/* ? a wholesale assignment ? really ? */
#endif /* AUTOCHANGER */

	if (atn_flag)
		bp->b_flags |= ATN_REQ;
	else
		bp->b_flags &= ~ATN_REQ;
	bp->b_flags &= ~S_RETRY_CMD;
	bp->b_flags |= B_SYNC;	/* scsi_nop for synchronous closes */
	bp->b_scsiflags = 0;
        bp->b_newflags = 0;
	bp->b_dev = dev;
	bp->b_error = 0;
	bp->b_parms = parm;
	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_log2blk = up->log2blk;
	bp->b_clock_ticks = clock_ticks;
	bp->b_action = fsm;
	bp->b_action2 = proc;
	bp->b_s2 = 0;

	if (proc == scsi_mode_select)
		bcopy(addr, bp->b_un.b_addr, (unsigned char)parm);

	enqueue(up->io_queue, bp);
	iowait(bp);

	if (!(bp->b_flags & B_ERROR)) {
		/* Transaction OK - Save Data */
	    if (addr != NULL && proc != scsi_mode_select)
		bcopy(bp->b_un.b_addr, addr, bp->b_bcount - bp->b_resid);
	    if (proc == scsi_inquiry) {
		up->inquiry_sz = bp->b_bcount - bp->b_resid;
		if (SCSI_DEBUG & MISC) {
			msg_printf("SCSI: dev = 0x%x; inquiry data:");
			msg_hex_dmp(&up->inq_data, up->inquiry_sz);
		}
	    }
	}

	if (proc == scsi_read_capacity)	{
	    up->lunsize = 0; up->log2blk = 0;
	    if (!(bp->b_flags & B_ERROR) && bp->b_resid == 0) {
		if (bp->b_resid == 0) {
		    bcopy(&bp->b_un.b_addr[0], &up->lunsize, 4);
		    bcopy(&bp->b_un.b_addr[4], &bpb, 4);
		    up->lunsize++;
		    SCSI_TRACE(MISC)("SCSI: dev 0x%x has %d %d-byte blocks\n",
			bp->b_dev, up->lunsize, bpb);
		    while (bpb >>= 1)
			up->log2blk++;
		}
	    }
	}

#ifdef AUTOCHANGER
	if (proc ==  scsi_format_unit) {
		brelse(bp);
	} else {
        	if (scsi_bp_lock & BP_WANTED)
                	wakeup(scsi_bp_lock);
        	scsi_bp_lock = 0;
	}
#else
	brelse(bp);
#endif
	return geterror(bp);
}

struct scsi_lun *scsi_lun_open(dev)
register dev_t dev;
{
	register struct scsi_card *cp;
	register struct scsi_dev *dp;
	register struct scsi_lun *up;

	if ((cp = scsi_card[m_selcode(dev)]) == NULL) {
		scsi_card[m_selcode(dev)] = (void *) -1;
		SCSI_MALLOC(cp, struct scsi_card *);
		wakeup(&scsi_card[m_selcode(dev)]);
	        bzero(cp, sizeof(*cp));
		scsi_card[m_selcode(dev)] = cp;
	} else if (cp == (void *) -1) {
		sleep(&scsi_card[m_selcode(dev)]);
		cp = scsi_card[m_selcode(dev)];
	}

	if ((dp = cp->dev[m_busaddr(dev)]) == NULL) {
		cp->dev[m_busaddr(dev)] = (void *) -1;
		SCSI_MALLOC(dp, struct scsi_dev *);
		wakeup(&cp->dev[m_busaddr(dev)]);
		bzero(dp, sizeof(*dp));
		cp->dev[m_busaddr(dev)] = dp;
		cp->open_cnt++;
	} else if (dp == (void *) -1) {
		sleep(&cp->dev[m_busaddr(dev)]);
		dp = cp->dev[m_busaddr(dev)];
	}

	if ((up = dp->lun[m_unit(dev)]) == NULL) {
		dp->lun[m_unit(dev)] = (void *) -1;
		SCSI_MALLOC(up, struct scsi_lun *);
		wakeup(&dp->lun[m_unit(dev)]);
		bzero(up, sizeof(*up));
		dp->lun[m_unit(dev)] = up;
		dp->open_cnt++;
		up->io_queue = &dp->io_queue;
		up->stash = &dp->stash;
	} else if (up == (void *) -1) {
		sleep(&dp->lun[m_unit(dev)]);
		up = dp->lun[m_unit(dev)];
	}

	if (up->open_cnt++ == 0) {
		up->dev_minor = minor(dev);
#if !defined(NOT_SCSI_MONITOR)
		up->dk_index = -1;
#endif /* NOT_SCSI_MONITOR */
	}

	return(up);
}

scsi_lun_close(dev)
register dev_t dev;
{
	register struct scsi_card *cp = NULL;
	register struct scsi_dev *dp = NULL;
	register struct scsi_lun *up = NULL;

	if ((cp = scsi_card[m_selcode(dev)])
			&& (dp = cp->dev[m_busaddr(dev)])
			&& (up = dp->lun[m_unit(dev)])
			&& (--up->open_cnt == 0)) {
		if (up->flags & OPEN_SUCCEEDED) {
			up->flags &= ~OPEN_SUCCEEDED;
		} else {
			dp->lun[m_unit(dev)] = NULL;
			dp->open_cnt--;
			SCSI_FREE(up);
		}
	}

	if (dp && dp->open_cnt == 0) {
		cp->dev[m_busaddr(dev)] = NULL;
		cp->open_cnt--;
		SCSI_FREE(dp);
	}

	if (cp && cp->open_cnt == 0) {
		scsi_card[m_selcode(dev)] = NULL;
		SCSI_FREE(cp);
	}
}

unsigned flush_on_all_closes = 1;

scsi_close(dev)
register dev_t	dev;
{
	register struct scsi_lun *up;

	if (m_volume(dev) != 0)
		return sds_close(dev);

	/*
	** flush the queue
	*/
	up = m_scsi_up(dev);
	if (up->open_cnt == 1 || flush_on_all_closes)
		scsi_control(up, scsi_nop, NULL, dev, NULL, 5*HZ, 1, 0);

	if (up->open_cnt == 1) {
#if !defined(NOT_SCSI_MONITOR)
		scsi_free_dk_index(up);
#endif /* NOT_SCSI_MONITOR */
		up->flags &= ~EXCLUSIVE;
		if (up->inq_data.rmb)
			up->flags &= ~CAPACITY_DONE;
		dma_unactive(isc_table[m_selcode(dev)]);
	}

	if (major(dev) == scsi_raw_major) {
		if (--up->raw_open_cnt == 0)
			up->flags &= ~CMD_MODE;
	} else if (up->open_cnt - up->raw_open_cnt == 1 && up->inq_data.rmb) {
		scsi_control(up,scsi_fsm,scsi_allow_media,dev,NULL,5*HZ,1,0);
	}

	scsi_lun_close(dev);

	return(0);
}

scsi_open(dev, flag)
register dev_t	dev;
int	flag;
{
	register struct isc_table_type *sc;
	register struct scsi_lun *up;
	register int err;

	if (m_volume(dev) != 0)
		return sds_open(dev, flag);

	/*
	** validate the minor number (all of it)
	*/
	if (m_selcode(dev) >= MAX_SCSI_SELCODE
	 		|| (sc = isc_table[m_selcode(dev)]) == NULL
			|| sc->card_type != SCUZZY
			|| m_busaddr(dev) >= MAX_SCSI_DEV_PER_SELCODE
			|| m_busaddr(dev) == sc->my_address
			|| m_unit(dev) >= MAX_LUN_PER_SCSI_DEV)
		return(ENXIO);

	/*
	** allocate and initialize necessary structures
	*/
	if ((up = scsi_lun_open(dev)) == NULL)
		return(ENOMEM);

	/*
	**  if command mode or exclusive lock in effect or device opened
	** under another device number then fail
	*/
	if ((up->flags & EXCLUSIVE) || minor(dev) != up->dev_minor) {
		scsi_lun_close(dev);
		return(EBUSY);
	}
	if (major(dev) == scsi_raw_major) {
		if (up->flags & CMD_MODE) {
			scsi_lun_close(dev);
			return(EBUSY);
		}
		up->raw_open_cnt++;
	}

#if !defined(NOT_SCSI_MONITOR)
	/*
	** assign dk_index for disk activity monitoring -- seems like first
	** open stuff, but it is possible that there were no slots available
	** on previous opens and there is one now
	*/
	scsi_assign_dk_index(up);
#endif /* NOT_SCSI_MONITOR */

	/*
	** first open
	*/
	if (up->open_cnt == 1) {

		dma_active(sc);

		/*
		** is there a device out there? -- identify it
		*/
		if (err = scsi_control(up, scsi_fsm, scsi_inquiry,
					dev, &up->inq_data, 5*HZ, 1, 0)) {
			scsi_close(dev);
			return(err);
		}
		if (strncmp(up->inq_data.vendor_id, "TEAC    ", 8) == 0
				&& (strncmp(up->inq_data.product_id,
					"FC-1     HF   07", 16) == 0
				    || strncmp(up->inq_data.product_id,
					"FC-1     HF   00", 16) == 0)) {
			up->flags |= TEAC_FLOPPY;
		} else {
			up->io_queue->io_flags |= SDTR_ENABLE;
		}
	}

	if (major(dev) == scsi_blk_major
			&& up->open_cnt - up->raw_open_cnt == 1) {
	    /*
	    ** first open of the block device, insure that there is media,
	    ** it is spun up, and it is formatted.  Also prevent media removal.
	    */
	    switch (up->inq_data.dev_type) {
	    case 0:	/* Disk  */
	    case 4:	/* WORM  */
	    case 5:	/* CDROM */
	    case 7:	/* MO    */
		if ((err = scsi_control(up, scsi_fsm, scsi_test_unit,
					dev, NULL, 5*HZ, 1, 0))
				|| (err = scsi_init_capacity(dev))) {
			scsi_close(dev);
			return(err);
		}
		if (up->inq_data.rmb)
			scsi_control(up,scsi_fsm,
				scsi_prevent_media, dev, NULL, 5*HZ, 1, 0);
	    default:	/* Give CMD_MODE access to anything! */
		break;
	    }
	}

	up->flags |= OPEN_SUCCEEDED;

	return(0);
}

#if !defined(NOT_SCSI_MONITOR)
/*
** scsi_assign_dk_index - if possible, assign a dk_index for profiling disc
** activity
*/
scsi_assign_dk_index(up)
register struct scsi_lun	*up;
{
	register int	i;

	if (up->dk_index >= 0)	/* already have one */
		return;

	for (i = 0; i < DK_NDRIVE; i++) {
		if (dk_devt[i] == 0) {
			up->dk_index = i;
			up->dk_busy_bit = 1 << i;
			dk_busy &= ~up->dk_busy_bit;
			dk_time[i] = 0;
			dk_xfer[i] = 0;
			dk_seek[i] = 0;
			dk_mspw[i] = 4.0e-6/3.0;	/* 1.5 MBytes/sec */
			dk_devt[i] = (up->dev_minor & SCSI_DEV_MASK)
						+ (scsi_blk_major << 24);
			return;
		}
	}
}

scsi_free_dk_index(up)
register struct scsi_lun	*up;
{
	if (up->dk_index >= 0) {
		dk_devt[up->dk_index] = 0;
		up->dk_index = -1;
	}
}
#endif /* NOT_SCSI_MONITOR */

struct teacModeHeader {
	unsigned char	rsv1;
	unsigned char	mediumType;
	unsigned char	rsv2;
	unsigned char	blockDescriptorLength;
};

struct teacBlockDescriptor {
	unsigned char densityCode;
	unsigned char numberOfBlocks[3];
	unsigned char reserved;
	unsigned char blockLength[3];
};

struct teacModePage5Descriptor {
	unsigned char	pageCode;
	unsigned char	pageLength;
	unsigned short	transferRate;
	unsigned char	numberOfHeads;
	unsigned char	sectorsPerTrack;
	unsigned short	bytesPerSector;
	unsigned short	numberOfCylinders;
	unsigned short	writePrecompCyl;
	unsigned short	reducedWriteCurrentCyl;
	unsigned short	boring[6];
	unsigned char	pin34pin2;
	unsigned char	pin1pin4;
	unsigned short	reserved[2];
};

struct teacModeSenseData {
	struct teacModeHeader hdr;
	struct teacBlockDescriptor block;
	struct teacModePage5Descriptor page5;
};

struct teacModeSelectData {
	struct teacModeHeader hdr;
	struct teacModePage5Descriptor page5;
};

#define MAX_SPT(d,b)	((d)*((b)==256?16:(b)==512?9:5))

scsi_teac_setup(dev, up, bps, cyl)
dev_t dev;
register struct scsi_lun *up;
register int *bps, *cyl;
{
	register int err, lbps, spt, lcyl;
	struct teacModeSelectData modeSelectBuf;
	struct teacModeSenseData modeSenseBuf;

	if (err = scsi_control(up, scsi_fsm, scsi_mode_sense_cmd,
				dev, &modeSenseBuf, HZ, 1, 5))
		return(err);

	bzero(&modeSelectBuf.hdr, sizeof(modeSelectBuf.hdr));
	bcopy(&modeSenseBuf.page5, &modeSelectBuf.page5,
		sizeof(modeSenseBuf.page5));
	modeSelectBuf.page5.pageCode = 5;

	if (bps == NULL || *bps == 0) {
		lbps = modeSenseBuf.page5.bytesPerSector;
		if (bps)
			*bps = lbps;
	} else {
		lbps = *bps;
	}

	if (cyl == NULL || *cyl == 0)
		lcyl = modeSenseBuf.page5.numberOfCylinders;
	else
		lcyl = *cyl;
	if (cyl)
		*cyl = lcyl;

	spt = MAX_SPT(modeSenseBuf.page5.transferRate / 0xfa, lbps);

	modeSelectBuf.page5.bytesPerSector = lbps;
	modeSelectBuf.page5.sectorsPerTrack = spt;
	modeSelectBuf.page5.numberOfCylinders = lcyl;
	if ((err = scsi_control(up, scsi_fsm, scsi_mode_select, dev,
			&modeSelectBuf, HZ, 1, sizeof(modeSelectBuf))) == 0) {
		up->lunsize = spt * lcyl * 2;
		up->log2blk = lbps == 256 ? 8 : lbps == 512 ? 9 : 10;
	}

	return(err);
}

scsi_teac_init_capacity(dev, up)
register dev_t dev;
register struct scsi_lun *up;
{
	register int df_bps, err;
	int bps, cyl;

	/*
	** determine bytes per sector
	*/
	bps = 0; cyl = 80;
	if (err = scsi_teac_setup(dev, up, &bps, &cyl))
		return(err);
	df_bps = bps;
	if (scsi_control(up,scsi_fsm,scsi_teac_read,dev,NULL,20*HZ,1,0)) {
	    for (bps = 512; bps;
			bps = bps == 512 ? 256 : bps == 256 ? 1024 : 0) {
		if (bps == df_bps)
			continue;
		if (scsi_teac_setup(dev, up, &bps, 0) == 0
				&& scsi_control(up, scsi_fsm, scsi_teac_read,
					dev, NULL, 20*HZ, 1, 0) == 0)
			break;
	    }
	}

	/*
	** find number of cylinders
	*/
	if (bps) {
	    if (scsi_control(up, scsi_fsm, scsi_teac_read,
			dev, NULL, 20*HZ, 1, -1)) {
		cyl = 77;
		if (scsi_teac_setup(dev, up, &bps, &cyl)
				|| scsi_control(up, scsi_fsm, scsi_teac_read,
					dev, NULL, 20*HZ, 1, -1))
			cyl = 0;
	    }
	}

	if (bps == 0 || cyl == 0) {
		up->lunsize = 0;
		up->log2blk = 0;
	}

	return(0);
}

scsi_init_capacity(dev)
dev_t dev;
{
	register struct scsi_lun *up = m_scsi_up(dev);

	if ((up->flags & CAPACITY_DONE) && up->lunsize == 0) { 
		/* flush sense data which may reset CAPACITY_DONE */
		scsi_control(up,scsi_fsm,scsi_test_unit,dev,NULL,5*HZ,1,0);
	}

	if (!(up->flags & CAPACITY_DONE)) { 
		up->flags |= CAPACITY_DONE;
		if (up->flags & TEAC_FLOPPY) {
			up->flags |= SCSI_QUIET;
			scsi_teac_init_capacity(dev, up);
			up->flags &= ~SCSI_QUIET;
		} else {
			scsi_control(up, scsi_fsm,
				scsi_read_capacity, dev, NULL, 5*HZ, 1, 0);
		}
	}

	if (up->lunsize == 0)
		return(ENXIO);

	return(0);
}

scsi_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
int *addr;
int flag;
{
	register struct scsi_lun *up;
	register int i, j;
	int bps, cyl;
 
	if (m_volume(dev) != 0)
		return sds_ioctl(dev, order, addr, flag);

	up = m_scsi_up(dev);
	switch (order & IOCCMD_MASK) {
	case SIOC_INQUIRY & IOCCMD_MASK:
		i = (order & IOCSIZE_MASK) >> 16;	/* requested */
		if (up->inquiry_sz < i)			/* available */
			i = up->inquiry_sz;
		bcopy(&up->inq_data, addr, i);
		return(0);
	case SIOC_CAPACITY & IOCCMD_MASK:
		if (i = scsi_init_capacity(dev))
			return(i);
		((struct capacity *)addr)->lba = up->lunsize;
		((struct capacity *)addr)->blksz = 1 << up->log2blk;
		return(0);
	case DIOC_CAPACITY & IOCCMD_MASK:
		if (i = scsi_init_capacity(dev))
			return(i);
		((capacity_type *)addr)->lba
			= (up->lunsize << up->log2blk) / DEV_BSIZE - 1;
		return(0);
	case DIOC_EXCLUSIVE & IOCCMD_MASK:
		if (!(flag & FWRITE) && !suser())
			return(EACCES);
		if (up->open_cnt != 1)
			return(EBUSY);
		if (*addr == 1)
			up->flags |= EXCLUSIVE;
		else if (*addr == 0)
			up->flags &= ~EXCLUSIVE;
		else
			return(EINVAL);
		return(0);
	case SIOC_CMD_MODE & IOCCMD_MASK:
	   	if (!(flag & FWRITE) && !suser())
			return(EACCES);
		if (up->raw_open_cnt != 1)
			return(EBUSY);
		if (*addr)
			up->flags |= CMD_MODE;
		else
			up->flags &= ~CMD_MODE;
		return(0);
	case SIOC_SET_CMD & IOCCMD_MASK:
		if (!(up->flags & CMD_MODE))
			return(EACCES);
		i = ((struct scsi_cmd_parms *)addr)->cmd_type;
		if (!(i == 6 || i == 10 || i == 12))
			return(EINVAL);
		bcopy(addr, &up->cmd_parms, sizeof(struct scsi_cmd_parms));
		return(0);
	case SIOC_XSENSE & IOCCMD_MASK:
		i = MIN((order & IOCSIZE_MASK) >> 16, sizeof(up->sense));
		bcopy(&up->sense, addr, i);
		bzero(&up->sense, sizeof(up->sense));
		return(0);
	case SIOC_FORMAT & IOCCMD_MASK:
	   	if (!(flag & FWRITE) && !suser())
			return(EPERM);
		if (up->flags & TEAC_FLOPPY) {
			switch (((struct sioc_format *)addr)->fmt_optn) {
			case  0:
			case  1: case 4: bps =  256; cyl = 77; break;
			case  2:         bps =  512; cyl = 77; break;
			case  3:         bps = 1024; cyl = 77; break;
			case 16:         bps =  512; cyl = 80; break;
			default: return(EINVAL);
			}
			if (i = scsi_teac_setup(dev, up, &bps, &cyl))
				return(i);
		}
		if (!(up->flags & EXCLUSIVE)) {
			/*
			** We used to require CMD_MODE for formatting, so
			** we do this for compatibility.
			*/
			if ((up->flags & CMD_MODE) && up->open_cnt == 1) {
				/* prevent other opens */
				up->flags |= EXCLUSIVE;
				i = 1;	/* flag indicates reset EXCLUSIVE */
			} else
				return(EACCES);
		} else
			i = 0;
    		j = scsi_control(up, scsi_fsm, scsi_format_unit, dev, NULL,
		    120*60*HZ, 1, ((struct sioc_format *)addr)->interleave);
		if (i == 1)
			up->flags &= ~EXCLUSIVE;
		return(j);
	case SIOC_MEDIUM_CHANGED & IOCCMD_MASK:
		*addr = (up->flags & MEDIUM_CHANGED) ? 1 : 0;
		up->flags &= ~MEDIUM_CHANGED;
		return(0);
	case DIOC_DESCRIBE & IOCCMD_MASK:
		bcopy(up->inq_data.product_id,
			((disk_describe_type *)addr)->model_num, 16);
		((disk_describe_type *)addr)->intf_type = SCSI_INTF;
		if (scsi_init_capacity(dev)) {
		    ((disk_describe_type *)addr)->maxsva = 0;
		    ((disk_describe_type *)addr)->lgblksz = 0;
		} else {
		    ((disk_describe_type *)addr)->maxsva = up->lunsize - 1;
		    ((disk_describe_type *)addr)->lgblksz = 1 << up->log2blk;
		}
		i = up->inq_data.dev_type;
		((disk_describe_type *)addr)->dev_type
				= i == 0 ? DISK_DEV_TYPE
				: i == 4 ? WORM_DEV_TYPE
				: i == 5 ? CDROM_DEV_TYPE
				: i == 7 ? MO_DEV_TYPE
				: UNKNOWN_DEV_TYPE;
		return(0);
	default:
		return(EINVAL);
	}
}

scsi_strategy(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;
	register struct scsi_lun *up;

	VASSERT(!(bp->b_flags & B_DONE));

	if (m_volume(dev) != 0)
		return sds_strategy(bp, NULL);

	up = m_scsi_up(dev);
	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_flags &= ~S_RETRY_CMD;
	bp->b_scsiflags = 0;
        bp->b_newflags = 0;
	bp->b_clock_ticks = DISCONNECT_TICKS;
	bp->b_action  = scsi_fsm;

	/*
	**  CMD_MODE or normal read/write
	*/
	if (major(dev) == scsi_raw_major && (up->flags & CMD_MODE)) {
		/* user land scsi command path */
		if (up->cmd_parms.cmd_mode)
			bp->b_flags |= ATN_REQ; /* Allow target to disconnect */
		else
			bp->b_flags &= ~ATN_REQ;
		bp->b_action2 = scsi_cmd;
		bp->b_resid = bp->b_bcount;
	} else {
		if (scsi_init_capacity(dev) != 0) {
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		bp->b_log2blk = up->log2blk;
		if (bpcheck(bp, up->lunsize, up->log2blk, 0))
			return;
		bp->b_flags |= ATN_REQ;	/* Allow Target to Disconnect */
		bp->b_action2 = (bp->b_dev & SCSI_SHORT_XFER_CMD)
				? scsi_short_xfer_cmd : scsi_xfer_cmd;
	}

#ifdef APEX
	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** Toggle the disk driver led so the front 
	** panel led will indicate disk activity.
	*/
	toggle_led(DISK_DRV_LED);

#endif /* APEX */
	enqueue(up->io_queue, bp);
}

scsi_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	if (m_volume(dev) != 0)
		return sds_write(dev, uio);

	return physio(scsi_strategy, NULL, dev, B_WRITE, minphys, uio);
}

scsi_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	if (m_volume(dev) != 0)
		return sds_read(dev, uio);

	return physio(scsi_strategy, NULL, dev, B_READ,  minphys, uio);
}

scsi_size(dev)
dev_t dev;
{
	struct scsi_lun	*up;
	int		bytes, err;
	static int	snoozetime = 20;

	if (m_volume(dev) != 0)
		return sds_size(dev);

	while (scsi_open(dev, 0x01))
		/* After booting, scsi_init pulls 'RST' which resets each
		 * device on the bus.  Some devices can take a long time.
		 * This algorithm limits TOTAL wait time to 5 seconds.
		 */
		if (snoozetime)	{ 
			/* Once snoozetime equals 0 we never re-enter loop */
			snooze(250000);
			snoozetime--;
			}
		else
			return 0;

	up = m_scsi_up(dev);

	if (err = scsi_init_capacity(dev))
		return(err);

	bytes = up->lunsize << up->log2blk;
	scsi_close(dev);

	return(bytes >> DEV_BSHIFT);
}


int scsi_dump(bp)
struct buf *bp;
{
#ifdef SAVECORE_300
	extern dev_t dumpdev;

	return scsi_if_dump(m_scsi_up(dumpdev)->log2blk);
#endif
}


struct msus (*scsi_saved_msus_for_boot)();
int (*scsi_saved_dev_init)();

/*
 *  code for converting a dev_t to a boot ROM msus
 */
struct msus scsi_msus_for_boot(blocked, dev)
char blocked;
dev_t dev;
{
	extern struct bdevsw bdevsw[];
	extern struct cdevsw cdevsw[];

	int maj = major(dev);
	int log2blk;
	struct msus my_msus;

	/*
	**  does this driver not handle the specified device?
	*/
	if (( blocked && (maj >= nblkdev || bdevsw[maj].d_open != scsi_open)) ||
	    (!blocked && (maj >= nchrdev || cdevsw[maj].d_open != scsi_open)))
		return (*scsi_saved_msus_for_boot)(blocked, dev);

	/*
	**  what's the media's block size?
	*/
	if (scsi_open(dev, FREAD))
	     log2blk = -1;
	else {
	     log2blk = m_scsi_up(dev)->log2blk;
	     scsi_close(dev);
	     }

	/*
	** construct the boot ROM msus
	*/
	my_msus.dir_format = 0;	/* assume LIF */
	my_msus.device_type = (log2blk < 0) ? 31 : 14;
	my_msus.sc = m_selcode(dev);
	my_msus.ba = m_busaddr(dev);
	my_msus.unit = m_unit(dev);
	my_msus.vol = 0;
	msg_printf("MSUS: log2blk %x sc %x unit %x ba %x\n",
		log2blk, my_msus.sc, my_msus.unit, my_msus.ba);
	return my_msus;
}

/*
** clear all nexus state information due to SCSI bus reset
*/
scsi_reset(sc)
int sc;
{
	register struct scsi_card *cp;
	register struct scsi_dev *dp;
	register int i;

	if (cp = scsi_card[sc]) {
	    for (i = 0; i < MAX_SCSI_DEV_PER_SELCODE; i++) {
		if (dp = cp->dev[i]) {
			dp->io_queue.io_flags &= ~SDTR_DONE;
			dp->io_queue.io_sync = 0;
		}
	    }
	}
}

/*
** one-time initialization code
*/
scsi_init()
{
	register struct cdevsw *cswp;
	register struct bdevsw *bswp;
	extern struct cdevsw cdevsw[];
	extern struct bdevsw bdevsw[];
	extern dev_t rootdev;
	extern int (*scsi_reset_callback)();

	/*
	** Initialize scsi_blk_major
	*/
	bswp = bdevsw;
	for (scsi_blk_major = 0; scsi_blk_major < nblkdev; scsi_blk_major++)
		if ((bswp++)->d_open == scsi_open)
			break;
	if (scsi_blk_major >= nblkdev)
		panic("scsi_init: scsi_open not in bdevsw");

	/*
	** Initialize scsi_raw_major
	*/
	cswp = cdevsw;
	for (scsi_raw_major = 0; scsi_raw_major < nchrdev; scsi_raw_major++)
		if ((cswp++)->d_open == scsi_open)
			break;
	if (scsi_raw_major >= nchrdev)
		panic("scsi_init: scsi_open not in cdevsw");

	/*
	** Set rootdev if boot device is SCSI
	*/
	if (msus.device_type == 14 && rootdev == NODEV)
		rootdev = makedev(scsi_blk_major,
			makeminor(msus.sc, msus.ba, msus.unit, msus.vol));

	scsi_reset_callback = scsi_reset;

	(*scsi_saved_dev_init)();
}


/*
**  one-time linking code for the device driver
*/
scsi_link()
{
	extern int (*dev_init)();
	extern struct msus (*msus_for_boot)();

	scsi_saved_dev_init = dev_init;
	dev_init = scsi_init;

	scsi_saved_msus_for_boot = msus_for_boot;
	msus_for_boot = scsi_msus_for_boot;

#ifdef AUTOCHANGER 
        scsi_bp_lock = 0;
        scsi_ctl_bp = NULL;
#endif

}
