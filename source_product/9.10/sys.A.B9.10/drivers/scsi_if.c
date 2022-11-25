/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/drivers/RCS/scsi_if.c,v $
 * $Revision: 1.2 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/07/14 13:17:36 $
 */

/*
 *	SCSI Interface driver
 *	   Current Capabilities
 *	   	- 16/32 bit DMA
 *		- Full disconnect/reconnect
 *		- Synchronous support
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
**	"CCOPTS=-DSCSI_IF_PATCH make scsi_if.o"
*/
#if defined(SCSI_IF_PATCH)
static char scsi_if_patch_id[] =
"@(#) PATCH 9.0: scsi_if.c $Revision: 1.2 $ $Date: 92/07/14 13:17:36 $";
#endif /* SCSI_IF_PATCH */

#include <param.h>
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
#include <callout.h>
#include <debug.h>

/* DIO Registers Definitions */
/* ID and Reset Register */
#define DIO_DIFFER	0x40
#define DIO_16BIT_DMA	0x20
#define DIO_PRIMARY_ID	0x1F
#define SCSI_ID		0x07

/* Control & Status Register */
#define DIO_IE		0x80
#define DIO_IR		0x40
#define DIO_LEVMSK	0x30
#define SELECT_32DMA	0x08
#define SCSI_DMA1	0x02
#define SCSI_DMA0	0x01

/* Hardware Configuration Register */
#define MAX_SYNC_XFR	0x60
#define PARITY		0x08
#define SCSI_ADDR	0x07

/* Fujitsu Specific Registers */
/* SCTL Register */
#define RST_SPC		0x80
#define CTRL_RST	0x40
#define DIAG		0x20
#define ARBT_ENAB	0x10
#define SPC_PARITY	0x08
#define SEL_ENAB	0x04
#define RESEL_ENAB	0x02
#define INTR_ENAB	0x01

/* SCSI register commands for SPC */
#define BUS_REL		0x00	/* Bus Release */
#define SELECT		0x20
#define SET_ATN		0x60
#define RST_ATN		0x40
#define TRANSFER	0x80
#define XFR_PAUSE		/* Not applicatable for host */
#define RST_ACK		0xC0	/* Reset ACK/REQ command     */
#define SET_ACK		0xE0	/* Set ACK/REQ command       */

/* SCMD Register */
#define RST_OUT		0x10
#define INT_XFR		0x08	/* Intercept Transfer */
#define PROG_XFR	0x04	/* Program Transfer   */
#define DMA_XFR		0x00	/* DMA  Transfer      */
#define TERM_MODE	0x01	/* Termination Mode   */

/* TMOD Register */
#define sync_mode	0x80	/* Sychronous Mode for data transfer */

/* PCTL Register */
#define BF_INT_ENAB	0x80	/* Bus Free Interrupt Enable */

/* PSNS Register */
#define REQ		0x80	/* REQ Line on SCSI */
#define ACK		0x40	/* ACK Line on SCSI */
#define ATN		0x20	/* ATN Line on SCSI */
#define SEL		0x10	/* SEL Line on SCSI */
#define BSY		0x08	/* BSY Line on SCSI */
#define MSG		0x04	/* MSG Line on SCSI */
#define CD		0x02	/* CD  Line on SCSI */
#define IO		0x01	/* IO  Line on SCSI */
#define PHASE		0x07	/* Determine phase (use with both PSNS & PCTL)*/

/* SSTS Register */
#define STATE		0x03	/* State of SPC (right shift register first) */
#define SPC_BSY		0x20	/* SPC Busy. Command being executed */
#define XFR_N_PROG	0x10	/* Transfer in Progress */
#define TCZ		0x02	/* Transfer Counter is Zero (TCH, TCM, TCL has
				 * reached zero 
				 */
#define DREG_FULL	0x02	/* Internal data buffer is full (8 bytes) */
#define DREG_EMPTY	0x01	/* Internal data buffer is full (8 bytes) */
#define DREG_1_7	0x01	/* Internal data buffer is full (8 bytes) */

/* INTS Register */
#define SELECTED	0x80	/* SELECTION Phase: Selected as Target (N/A) */
#define RESELECTED	0x40	/* SPC Reselected */
#define DISCONNECTED	0x20	/* Indicates Transition to BUS FREE (see bit 7
				 * (BUS FREE Interrupt Enable) in PCTL Reg.
				 */
#define CMD_COMPLETE	0x10	/* Select or Transfer Command Completed */
#define SRV_REQ		0x08	/* Service Required */
#define TIMEOUT		0x04
#define HARD_ERR	0x02	/* Hard Error: Read SERR Register */
#define RST_INT		0x01	/* Reset Interrupt */

/* SERR Register */
/* The "MB87030 Synchronous/Asynchronous Protocol Controller Users Manual"
 * is required to fully understand the details of the errors described below.
 */
#define	S_PARITY_ERR	0xc0	/* Parity error detected */
#define	S_TC_PARITY	0x08	/* Parity error detected during hardware xfr */
#define	S_PHASE_ERR	0x04	/* Transfer phase error */
#define	S_SHORT_XFR_ERR	0x02	/* REQ/ACK signal has a cycle exceeding range */
#define	S_OFFSET_ERR	0x01	/* Sync xfr offset error */

/* SCSI Register definitions */
struct scsi {
	unsigned char
		sf0,  scsi_id,
		sf2,  scsi_csr,
		sf4,  scsi_wrap,
		sf6,  scsi_hconf,
		sf8,  scsi_pstat,
		sf10, scsi_pbyte0,
		sf12, scsi_pbyte1,
		sf14, scsi_pbyte2,
		sf16, scsi_dma,
		sfa[14],
		sf32, scsi_bdid,
		sf34, scsi_sctl,
		sf36, scsi_scmd,
		sf38, scsi_tmod,
		sf40, scsi_ints,
		sf42, scsi_psns,
		sf44, scsi_ssts,
		sf46, scsi_serr,
		sf48, scsi_pctl,
		sf50, scsi_mbc,
		sf52, scsi_dreg,
		sf54, scsi_temp,
		sf56, scsi_tch,
		sf58, scsi_tcm,
		sf60, scsi_tcl;
};

#define	scsi_sync	scsi_csr	/* SCSI hidden register 3 */

int		SCSI_DEBUG = 0;		/* specifies interesting diagnostics */
int		scsi_flags = SCSI_DMA_PRI + DISCON_OK;
unsigned 	min_xfr_period = 63;		/* 4 MB/s */
unsigned 	req_ack_offset = 8;
int (*scsi_reset_callback)();	/* inform top half of bus reset */

/* define's for Select Code Structure */
#define s_flags		tfr_control
#define sync		ppoll_resp /* Select code structure holds maximum
				    * synchronous transfer rate (divided by 4)
				    * available on interface card.  (Per device
				    * data transfer rate is contained in iobuf.)
				    */
#define trying_select	ppoll_flag /* Flag to indicate that a process has
				    * attempted to select. Someone else must
				    * drop his select code and initialize
				    * their state, and get_selcode
				    */
#define	hard_error	ppoll_mask /* Fujitsu SPC detected error */

/*
** state variables for scsi_set_state
*/
#define set_state_state	lock_count
#define start_lbolt	intr_wait
#define sc_flags	locks_pending

#define FUJITSU_10_MHz	0x01

/* define's for state flag in select code structure */
#define	DMA32_INSTALLED	DO_BOBCAT_PPOLL
#define	RST_REQ		BOBCAT_HPIB	/* Reset req'd - not currently used */

/* Flags used by SCSI card (per selcode) in int_flags */
#define SCSI_RST	0x01	/* Set after SCSI reset until Request Sense */

#define	CONNECTED_TO_SCSI	(cp->scsi_ssts&0x80)

/****************************************************************
 *                                                              *
 * Specialized SCSI routines for queuing / dequeuing requests   *
 * for the select code.                                         *
 * See hpib.c, get_selcode.c for related activities.            *
 *                                                              *
 ****************************************************************/

scsi_if_dequeue(bp)
register struct buf *bp;
{
	int i;
	struct isc_table_type *sc = bp->b_sc;

	(*bp->b_action)(bp);
	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	} while(i);
}


/*
 * resp = response from the SCSI card indicating
 * which target re-selected the initiator.
 * 'resp' is a bit significant value (e.g. 0x20 is bus addr 5)
 */
scsi_if_call_isr(sc, resp)
register struct isc_table_type *sc;
register unsigned char resp;
{
	register struct buf *bp;
	register unsigned char addr;

	/*
	** determine bus address of target
	*/
	if (resp != (addr = 1 << sc->my_address))
		resp &= ~addr;
	for (addr = 0; resp >>= 1; addr++) ;

	/*
	 * determine the next buf now, since calling b_action
	 * will place the current buf on some other queue,
	 * thus losing its pointers to this queue!
	 */
	for (bp = sc->ppoll_f; bp != NULL; bp = bp->av_forw) {
	    if (addr == bp->b_ba ) {
		if (bp->av_back != NULL)
			bp->av_back->av_forw = bp->av_forw;
		else
 			sc->ppoll_f = bp->av_forw;
		if (bp->av_forw != NULL)
			bp->av_forw->av_back = bp->av_back;
		else
			sc->ppoll_l = bp->av_back;
		/* Put process on queue to determine next phase
		 * requested by SCSI and get select code 
		 * Assumption: must return immediately
		 */
		bp->b_queue->b_state = (int)bp->b_nextstate;
		(*bp->b_action)(bp);
		/* Only one target can reselect (and interrupt) initiator 
		 * at a time.  Thus, we can break after calling the
		 * appropriate b_action routine.
		 */
		return(0);	/* Just return */
		}
	}
	/* There is one known reason for getting here:  
	 * A request has timed out; the timeout routine must immediately
	 * remove the request from the 'sc->ppoll_f' list, and then a narrow
	 * window exists before we handle the timeout and the device for
	 * unknown reasons finally reselects.  This window could be handled
	 * in software.  The rationale for resetting the bus is:
	 *	- extremely small window (highly unlikely)
	 *	- complex code to handle the problem
	 *	- difficulty in testing and debugging software
	 */
	msg_printf("SCSI call_isr: unknown reselect (isc: %d addr: %d)\n", 
		sc->my_isc, addr);
	snooze(3000000);	/* flush write cache */
	scsi_if_reset_bus(sc);
	snooze(1000000);	/* let devices recover */
	return(1);	/* Just return */
}


scsi_if_call_busfree(sc)
register struct isc_table_type *sc;
{
	register struct buf *bp = sc->event_f;

	if (bp == NULL)
		return(0);
 	if ((sc->event_f = bp->av_forw) != NULL)
		bp->av_forw->av_back = NULL;

	bp->b_queue->b_state = (int)bp->b_nextstate;
	(*bp->b_action)(bp);
	return(0);
}

/* Put process on list when bus unavailable for use. */
scsi_if_wait_for_busfree(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc;
	int s;

	sc = bp->b_sc;
		
	bp->av_forw = NULL;
        s = spl6();
        if (sc->event_f == NULL) {
                sc->event_f = bp;
                sc->event_l = NULL;
		}
        else
                sc->event_l->av_forw = bp;
	bp->av_back = sc->event_l;
        sc->event_l = bp;
	drop_selcode(bp);
	splx(s);
}

/* Remove 'bp' from scsi_if_wait_for_busfree list */
scsi_if_remove_busfree(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct buf *ptr = sc->event_f;

	while (ptr != NULL)	{
		if (bp == ptr)	{
			if (bp->av_back != NULL)
				bp->av_back->av_forw = bp->av_forw;
			else
 				sc->event_f = bp->av_forw;
			if (bp->av_forw != NULL)
				bp->av_forw->av_back = bp->av_back;
			else
				sc->event_l = bp->av_back;
			return(0);
			}
		else
			ptr = ptr->av_forw;
		}
	return(1);
}

/* put bp on queue to wait for reselect */
scsi_if_wait_for_reselect(bp, proc)
register struct buf *bp;
int (*proc)();
{
	register struct isc_table_type *sc;
	int s;

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

	unprotected_drop_selcode(bp);
	splx(s);
}

/* clear reselect */
scsi_if_clear_reselect(bp)
register struct buf *bp;
{
	register struct buf *cur_buf;
	register struct isc_table_type *sc = bp->b_sc;
	int s;

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


/****************************************************************
 *                                                              *
 * SCSI Interface Driver.                                       *
 * Contains:                                                    *
 *           Initialization Routines                            *
 *           Interface specific routines                        *
 *           Chip controller dependent routines                 *
 *                                                              *
 ****************************************************************/

scsi_if_reset_bus(sc)
register struct isc_table_type *sc;
{
	register struct scsi *cp = (struct scsi *)sc->card_ptr;
	unsigned char s_addr;

	msg_printf("SCSI Bus: hard reset (isc: %d)\n", sc->my_isc);
	/* Reset SCSI */
	cp->scsi_csr  = 0;	/* Disable interrupts while resetting SCSI */
	cp->scsi_sctl = 0;
	cp->scsi_scmd = RST_OUT;
	snooze(30);
	sc->int_flags = SCSI_RST;

	cp->scsi_sctl = RST_SPC | CTRL_RST;
	cp->scsi_scmd = 0;
	cp->scsi_tmod = 0;
	cp->scsi_pctl = 0;
	cp->scsi_temp = 0;
	cp->scsi_tch  = 0;
	cp->scsi_tcm  = 0;
	cp->scsi_tcl  = 0;
	cp->scsi_ints = 0;

	cp->scsi_csr  = DIO_IE;		/* Enable interrupts */
	cp->scsi_sctl = RST_SPC | ARBT_ENAB | RESEL_ENAB | INTR_ENAB;

	/* Set Parity if switch set */
	if (cp->scsi_hconf&PARITY)
		cp->scsi_sctl |= SPC_PARITY;

	/* Setup SCSI address for card */
	s_addr = SCSI_ADDR&(~cp->scsi_hconf);
	cp->scsi_bdid = s_addr;

	/* Reconnect SPC to SCSI */
	cp->scsi_sctl &= ~RST_SPC;

	/* Save address in sc */
	sc->my_address = s_addr;

	/* Inform top half of bus reset */
	if (scsi_reset_callback)
		(*scsi_reset_callback)(sc->my_isc);
}


#define LSHIFT(x,amt) (int)(x<<amt)

scsi_if_dmaisr(sc)
struct isc_table_type *sc;
{
	struct scsi *cp = (struct scsi *) sc->card_ptr;
	struct buf *bp = sc->owner;
	struct iobuf *iob = bp->b_queue;
	int resid;
	caddr_t ptr;

	/*
	** Update sc->resid from dma channel hardware.
	*/
	dmaend_isr(sc);

	/*
	** Disown the dma channel.
	*/
	cp->scsi_csr &= ~(SCSI_DMA0 | SCSI_DMA1);
	drop_dma(sc->owner);

	/*
	** Determine resid w.r.t. SCSI.
	*/
	resid = (cp->scsi_tch << 16) + (cp->scsi_tcm << 8) + cp->scsi_tcl;

	/*
	** Translate sc->resid from a word count to a byte count.
	*/
	sc->resid <<= (sc->state & F_LWORD_DMA) ? 2
			: (sc->state & F_WORD_DMA) ? 1 : 0;

	/*
	** Manage discrepancies in DMA channel counter, sc->resid,
	** and the fujitsu transfer byte counter, resid, due to the DMA
	** packer.
	**
	** If the counts differ on a write, the data left in the packer
	** is unneeded and the packer can be flushed, but sc->resid
	** needs to be updated to reflect the amount of data actually
	** transferred on SCSI rather than the amount of data DMA'd into
	** the packer from memory.
	**
	** If the counts differ on a read, there is data left in the
	** packer which should be moved to memory if possible.  The data
	** is read from the packer and written to memory and
	** sc->resid is updated to reflect the amount of data actually
	** transferred on SCSI.  On older hardware, it is impossible to
	** read the data stuck in the packer, so the best we can do, short
	** of retrying the I/O without using DMA is to
	** leave sc->resid indicating the amount of data that actually
	** reached memory via DMA and pretend that the last few bytes
	** were never transferred.  In this case, we cannot forget to
	** clear the packer so the residual data is not used in the
	** next DMA transfer.
	*/
	if (resid != sc->resid)
	{
		if (bp->b_flags & B_READ)
		{
			VASSERT(sc->resid > resid && sc->resid - resid < 4);

			if (hidden_mode_exists(sc->my_isc))
			{
				VASSERT(((cp->scsi_pstat & 1) != 0)
						== (sc->resid - resid == 1));
				VASSERT(((cp->scsi_pstat & 2) != 0)
						== (sc->resid - resid == 2));
				VASSERT(((cp->scsi_pstat & 4) != 0)
						== (sc->resid - resid == 3));

				ptr = iob->b_xaddr + iob->b_xcount - sc->resid;

				if (sc->resid-- > resid)
					*ptr++ = cp->scsi_pbyte0;
				if (sc->resid-- > resid)
					*ptr++ = cp->scsi_pbyte1;
				if (sc->resid-- > resid)
					*ptr++ = cp->scsi_pbyte2;
			}
			else /* old, less functional hardware */
			{
				/*
				** This problem must be dealt with at a
				** higher level.
				*/
			}
		}
		else /* B_WRITE */
		{
			VASSERT(sc->resid < resid);
			sc->resid = resid;
		}
	}

	/*
	** Clear the packer.
	*/
	cp->scsi_hconf = 0;


	/*
	** Return the number of bytes of data lost due to limited
	** hardware functionality.
	*/

	return sc->resid - resid;
}


scsi_if_isr(inf)
struct interrupt *inf;
{
    register struct isc_table_type *sc = (void *)inf->temp;
    register struct scsi *cp = (void *)sc->card_ptr;
    register unsigned char mask, resp;

    SCSI_TRACE(ISR)("Enter ISR\n");

    if (mask = cp->scsi_ints) {

	SCSI_TRACE(ISR)("ISR: ints = 0x%02x\n", mask);

	resp = cp->scsi_temp;	/* reselecting target id */

	/* Capture error before resetting ints */
	sc->hard_error = cp->scsi_serr;	
	cp->scsi_ints = mask; /* Reset interrupts */

	if (mask&RST_INT)	{
		/* Hard RESET on SCSI.   Origin is unknown.
		 * We need to notify all devices waiting for a reselect.
		 */

		/* Inform top half of bus reset */
		if (scsi_reset_callback)
			(*scsi_reset_callback)(sc->my_isc);

		snooze(100);	/* Make sure IGOR has time to set bit. */
		if (cp->scsi_hconf & 0x80)	{ /* Fuse OK. Who pulled RST? */
		     printf("SCSI: Spurious RST Interrupt: Check Hardware\n");
		     printf("SCSI: Possible DATA LOST!\n");
		     }
		else	{
		     cp->scsi_csr &= ~DIO_IE;
		     printf("SCSI: HARD ERROR in Hardware (CHECK FUSE)\n");
		     printf("SCSI: WARNING: ALL SCSI interrupts turned off!\n");
		     }
		}

	if (sc->hard_error || mask&HARD_ERR)	{
		msg_printf("SCSI: (select code: %d)", sc->my_isc);
		if (!(mask&HARD_ERR))
			/* Bits 6 & 7 do NOT generate interrupt */
			cp->scsi_ints = HARD_ERR; /* Reset SERR Register */
		if (sc->hard_error&S_PARITY_ERR)
		    msg_printf("Parity error detected\n");
		if (sc->hard_error&S_TC_PARITY)
		    msg_printf("Parity error detected in hardware xfr\n");
		if (sc->hard_error&S_PHASE_ERR)
		    msg_printf("Transfer phase error\n");
		if (sc->hard_error&S_SHORT_XFR_ERR)
		    msg_printf("REQ/ACK signal cycle period exceeds range\n");
		if (sc->hard_error&S_OFFSET_ERR)
		    msg_printf("Sync xfr offset error\n");
		/* Increment sc->hard_error to guarantee its non-zero */
		sc->hard_error++;
		/* call FSM if not a TIMEOUT */
		if (!(mask&TIMEOUT) && sc->active)	{
			struct buf *bp = sc->owner;
			bp->b_queue->b_state = (int)scsi_error;
			(*bp->b_action)(bp);	/* Call FSM's error handler */
			mask = 0;    /* Do not process remaining interrupts */
			}
		else
			printf("SCSI: Spurious parity error detected\n");
		do;
		while (selcode_dequeue(sc) | dma_dequeue());
		}

	if (mask&DISCONNECTED && (sc->event_f != NULL))	{
		/* Find the appropriate process and call
		 * its b_action() routine (from call_isr)
		 */
		scsi_if_call_busfree(sc);

		do;
		while (selcode_dequeue(sc) | dma_dequeue());
		}

	if (mask&RESELECTED)	{
		SCSI_TRACE(ISR)("SPC Reselected resp %x psns %x\n",
			resp, cp->scsi_psns);

		sc->set_state_state = 0;

		if (cp->scsi_psns&ATN) {
			cp->scsi_hconf = 0;
 			cp->scsi_scmd = RST_ATN;

			}

		/* Find the appropriate process and call
		 * its b_action() routine (from call_isr)
		 */
		scsi_if_call_isr(sc, resp);

		/*
		 * dequeue routines
		 */
		if (sc->active)	 {
		    struct buf *bp = sc->owner;
		    /* Unconditionally drop select code for 'bp' */
		    drop_selcode(bp);
		    /* Put 'bp' on the select code only if he is
		     * "trying to select" device (and failed).
		     */
		    if (sc->trying_select) {
			bp->b_queue->b_state = (int)initial;
			(*bp->b_action)(bp);  /* Must return immediately */
			}
		    sc->trying_select = 0;
		    }
		do;
		while (selcode_dequeue(sc) | dma_dequeue());
		}

	if (mask&TIMEOUT)	{
		struct buf *bp = sc->owner;

		if (bp == NULL)	
			panic("scsi_if_isr: Timeout and no owner");
		bp->b_queue->b_state = (int)select_nodev;
		sc->trying_select = 0;
		(*bp->b_action)(bp);
		while (selcode_dequeue(sc) | dma_dequeue())
			/* Pending interrupt; break  */
			if (cp->scsi_ints)
				break;
		}

	if (mask&CMD_COMPLETE || mask&SRV_REQ)	{
	/* 'Service Required' uses almost identical ISR processing */
		struct buf *bp = sc->owner;

		if (bp == NULL)	
			if (mask&SRV_REQ)
			   panic("scsi_if_isr: Service Req'd and no owner");
			else
			   panic("scsi_if_isr: command complete and no owner");

		if (mask&SRV_REQ)	{
			cp->scsi_scmd |=  PROG_XFR;
			cp->scsi_sctl |=  CTRL_RST;
			cp->scsi_sctl &= ~CTRL_RST;
		}

		if (!CONNECTED_TO_SCSI)
			panic("scsi_if_isr: Not connected with SCSI\n");

		sc->set_state_state = 0;
		bp->b_queue->b_state = (int)set_state;

		sc->resid = bp->b_queue->b_xcount;
		if (bp->b_phase == (int)SCSI_DATA_XFR)	{
			register struct iobuf *iob = bp->b_queue;
			/* Compute residual count.
			 * We also have to clean up SPC.
			 */
			switch (sc->transfer)	{
			    case FHS_TFR:
				sc->resid = LSHIFT(cp->scsi_tch,16)+
				LSHIFT(cp->scsi_tcm,8)+LSHIFT(cp->scsi_tcl,0);
				break;
			    case DMA_TFR:
				if (scsi_if_dmaisr(sc) != 0)
					bp->b_flags |= ABORT_REQ;
				break;
			    default:
				panic("SCSI: Unknown transfer type");
			   }
			SCSI_TRACE(ISR_v)("(after scsi_if_dmaisr):resid: %x\n",
					sc->resid);
			/* Update iobuf pointers */
			iob->b_xaddr += iob->b_xcount - sc->resid;
			iob->b_xcount = sc->resid;
			}
		else if (bp->b_phase == (int)SCSI_SELECT)	{
			sc->trying_select = 0;
			bp->b_flags |= CONNECTED;  /* Mark buf as connected */
			bp->b_queue->retry_cnt = 0;
			}

		/* SCSI Drives Next State.  If CONNECTED call FSM */
		if (bp->b_flags & CONNECTED)
			(*bp->b_action)(bp);
		do;
		while (selcode_dequeue(sc) | dma_dequeue());
		}
    }

    SCSI_TRACE(ISR)("Exit ISR\n");

}

/*
** these belong in scsi.h
*/
#define	BUS_BUSY	1
#define	NOT_OWNER	2

/*
 * Select LUN
 */
scsi_if_select(bp)
register struct buf *bp;
{
	register struct scsi *cp = (struct scsi *)bp->b_sc->card_ptr;
	register struct isc_table_type *sc = bp->b_sc;
	int scsi_priority, ret=0;

	/* Prevent card from interrupting */
	scsi_priority = splx(sc->int_lvl);

	if (cp->scsi_ssts & 0xE0) {
		/* SCSI bus not free */
		ret = BUS_BUSY;
		bp->b_nextstate = (int)initial;
	  	scsi_if_wait_for_busfree(bp);	
		SCSI_TRACE(STATES)("Select (%d): bus not free\n",
							 bp->b_ba);
		}
	else if (bp != sc->owner) {
		ret = NOT_OWNER;
		}
	else	{
		sc->trying_select++;	/* We will try to select device */
		/* Clear ints register for DISCONNECT */
		if (cp->scsi_ints&0x20)
			cp->scsi_ints=0x20;
	
		cp->scsi_pctl = 0;
		cp->scsi_temp = 1 << bp->b_ba | 1 << sc->my_address;
	
		if ( bp->b_flags & ATN_REQ)	/* Set ATN during selection */
			cp->scsi_scmd = SET_ATN;
	
		/* Set timeout for 10 millisecs. Winchesters take less than 
		 * 1 millisec while SONY may take several millisecs.
		 */
		cp->scsi_tch  = 0x3;
		cp->scsi_tcm  = 0xE; /* Allow 50 msecs for select to complete */
		cp->scsi_tcl  = 4;   /* Hard coded by clock signal on card */
	
		cp->scsi_scmd = SELECT;
		}
	
	/* Re-enable interrupts */
	splx(scsi_priority);

	return(ret);  		/* Does not guarantee success */
}

/*
** tmod must be set prior to target's first REQ of data transfer phase
** independent of when the transfer command is issued
*/
scsi_if_set_tmod(sc, tmod)
struct isc_table_type *sc;
int tmod;
{
	register struct scsi *cp = (struct scsi *)sc->card_ptr;

	if (hidden_mode_exists(sc->my_isc)) {
	    if ((tmod & 0x100) && !(sc->sc_flags & FUJITSU_10_MHz)
		    || !(tmod & 0x100) && (sc->sc_flags & FUJITSU_10_MHz)) {
		enable_hidden_mode(sc->my_isc);
		if (tmod & 0x100) {
			cp->scsi_sync |= 0x10;	/* 10 MHz */
			sc->sc_flags |= FUJITSU_10_MHz;
		} else {
			cp->scsi_sync &= ~0x10;		/* 8 MHz */
			sc->sc_flags &= ~FUJITSU_10_MHz;
		}
		disable_hidden_mode(sc->my_isc);
	    }
	}

	cp->scsi_tmod = (unsigned char)tmod;
}

#define RSHIFT(x,amt) (char)((x)>>(amt)&0xFF)

/*
 * Fast handshake routine
 * Uses FUJI programmed transfer
 * Assumptions:
 *	Addressing and count set up by calling procedure
 *	All parameters for transfer already setup in sc struct
 *		(iobuf & buf independent)
 * 	Isr will clean up and update iobuf
 *
 *      One additional assumption: this routine is being called
 *      out of the card's ISR.  Thus, we are handshaking bytes
 *      at the card's interrupt level.  If an interrupt occurs,
 *      we will simple exit and wait to service the interrupt.
 */
scsi_if_prog_xfr(bp)
struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct scsi *cp = (struct scsi *)sc->card_ptr;
	struct iobuf *iob = bp->b_queue;
	register int count = sc->resid;
	register unsigned char *bfr = (void*)sc->buffer;
	register unsigned char *ssts = &cp->scsi_ssts;
	register unsigned char *dreg = &cp->scsi_dreg;

	/* Set up FUJI registers for transfer */
	cp->scsi_tch = RSHIFT(count,16);
	cp->scsi_tcm = RSHIFT(count,8);
	cp->scsi_tcl = RSHIFT(count,0);

	if ((cp->scsi_pctl = (cp->scsi_psns & 0x07)) == CMD_PHASE)
		scsi_if_set_tmod(sc, iob->io_sync);

	/* Start transfer */
	cp->scsi_scmd = TRANSFER + PROG_XFR;

	START_POLL;

	while ((*ssts & 0xF0) != 0xB0) ;

	if (sc->s_flags & B_READ) {	/* Read */
		while (count) {
			if (*ssts & 0x2) {
				switch (count) {
				case 7:	*bfr++ = *dreg;
				case 6:	*bfr++ = *dreg;
				case 5:	*bfr++ = *dreg;
				case 4:	*bfr++ = *dreg;
				case 3:	*bfr++ = *dreg;
				case 2:	*bfr++ = *dreg;
				case 1:	*bfr++ = *dreg;
					count = 0;
					break;
				default:
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					*bfr++ = *dreg;
					count -= 8;
					break;
				}
			} else {
				while (*ssts & 0x1)
					if (cp->scsi_ints)
						goto cleanup;
				*bfr++ = *dreg;
				count--;
			}
		}
	} else {	/* Write */
		while (count) {
			if (*ssts & 0x1) {
				switch (count) {
				case 7:	*dreg = *bfr++;
				case 6:	*dreg = *bfr++;
				case 5:	*dreg = *bfr++;
				case 4:	*dreg = *bfr++;
				case 3:	*dreg = *bfr++;
				case 2:	*dreg = *bfr++;
				case 1:	*dreg = *bfr++;
					count = 0;
					break;
				default:
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					*dreg = *bfr++;
					count -= 8;
					break;
				}
			} else {
				while (*ssts & 0x2)
					if (cp->scsi_ints)
						goto cleanup;
				*dreg = *bfr++;
				count--;
			}
		}
	}
	/* A Service Required interrupt may have occurred
	 * Cleanup SPC and adjust count accordingly
	 */
cleanup:
	END_POLL;
	return(0);
}

int dmajunk;

struct dma_chain *scsi_if_dma_setup(bp, dma_chain, entry)
struct buf *bp;
register struct dma_chain *dma_chain;
int entry;
{
	dma_chain->card_reg = (caddr_t)&dmajunk;
	
	switch (entry)	{
		case FIRST_ENTRIES:
		     dma_chain->arm = DMA_ENABLE | DMA_LVL7;
		     break;
		case LAST_ENTRY:
		     dma_chain->arm &= ~DMA_ENABLE;
		     break;
		default:
		     panic("scsi_if_dma_setup: Unknown dma setup entry");
	}
	return(dma_chain);
}

/*
 * Terminate Arbitration phase of SCSI
 * Return success (1) if arbitration successfully terminated
 */
scsi_if_term_arbit(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct scsi *cp = (struct scsi *)sc->card_ptr;

	cp->scsi_ints = TIMEOUT;	/* Terminate Arbitration */
}

void scsi_set_state_done(bp)
register struct buf *bp;
{
	register struct scsi *cp = (struct scsi *)bp->b_sc->card_ptr;
	register struct iobuf *iob = bp->b_queue;

	if (cp->scsi_serr) {
	   	iob->b_state = scsi_error;
		cp->scsi_ints = HARD_ERR;
		msg_printf("SCSI: hardware (probably parity) error\n");
	} else {
		iob->b_state = CONNECTED_TO_SCSI
				? ACK + (cp->scsi_psns & 7) : defaul;
	}
}

/*
** Find next phase requested by SCSI
*/
scsi_if_set_state(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct scsi *cp = (struct scsi *)sc->card_ptr;

	START_POLL;
	while (CONNECTED_TO_SCSI) {
		if (cp->scsi_psns & REQ || (cp->scsi_ssts & 0x90) == 0x90)
			break;
	}
	END_POLL;

	scsi_set_state_done(bp);
}

/* 
** Assumptions:
**	- target is asserting REQ for message out phase
*/
scsi_if_mesg_out(bp, bfr, len)
register struct buf *bp;
char *bfr;
register int len;
{
	register struct scsi *cp = (struct scsi *)bp->b_sc->card_ptr;

	START_POLL;

	bp->b_flags &= ~ATN_REQ;

	cp->scsi_scmd = (len > 1) ? SET_ATN : RST_ATN;

	do {
		if (len <= 1)
			cp->scsi_scmd = RST_ATN;

		/* scsi_if_man_xfr_out */
		cp->scsi_temp = *bfr++;
		cp->scsi_scmd = SET_ACK;
		while (cp->scsi_psns & REQ) ;
		cp->scsi_scmd = RST_ACK;

		/*
		** Wait for the next phase.
		*/
		while (!(cp->scsi_psns & REQ)
				&& (cp->scsi_ssts & 0x90) != 0x90) ;

	} while (--len && (cp->scsi_psns & 0x07) == MESG_OUT_PHASE) ;

	cp->scsi_scmd = RST_ATN;

	END_POLL;

	return(len);
}

/*
** Assumptions:
**	- target has REQ asserted for message in phase
*/
scsi_if_mesg_in(bp, bfr)
register struct buf *bp;
char *bfr;
{
	int length = 0;
	register struct scsi *cp = (struct scsi *)bp->b_sc->card_ptr;

	START_POLL;

	scsi_if_set_tmod(bp->b_sc, bp->b_queue->io_sync);

	do {
		/* scsi_if_man_xfr_in */
		cp->scsi_scmd = SET_ACK;
		while (cp->scsi_psns & REQ) ;
		bfr[length] = cp->scsi_temp;
		cp->scsi_scmd = RST_ACK;

		/*
		** reject all extended messages except target responses to SDTR
		*/
		if (!(bp->b_newflags & SDTR_SENT)
				&& bfr[length] == MSGext_message) {
			cp->scsi_scmd = SET_ATN;
			bp->b_newflags |= REJECT_SDTR;
		}

		length++;

		while (!(cp->scsi_psns & REQ)
				&& (cp->scsi_ssts & 0x90) != 0x90)
		{
			if (!CONNECTED_TO_SCSI)
				goto break2;
		}

	} while ((cp->scsi_psns & 0x07) == MESG_IN_PHASE) ;

break2:
	END_POLL;

	if (!length)
		bfr[length++] = MSGno_op;
	return(length);
}

/*
** Assumptions:
**	- target has asserted REQ for status phase
*/
scsi_if_status(bp)
struct buf *bp;
{
	register struct scsi *cp = (struct scsi *)bp->b_sc->card_ptr;
	char status;

	START_POLL;

	/* scsi_if_man_xfr_in */
	cp->scsi_scmd = SET_ACK;
	while (cp->scsi_psns & REQ) ;
	status = cp->scsi_temp;
	cp->scsi_scmd = RST_ACK;

	END_POLL;

	return(status);
}

#define	BUS_NOT_FREE	3
#define	TICKS		HZ

/* flag_timeout() merely ESCAPEs */
scsi_if_escape(bp)
register struct buf *bp;
{
	int flag_timeout();

	SCSI_TRACE(DIAGs)("scsi_if_abort: scsi_escape\n");
	*bp->b_queue->markstack = flag_timeout;
}

/*
 * Free up bus.  
 *	Algorithm:
 *	   abort_mesg_sent = error = 0
 *	   If (bus_free)
 *		return (0)
 *	   Turn off card
 *	   If (SPC interrupting) {
 *	   	if (SPC reselected by another device)
 *	   	    return (0)
 *	   	    }
 *	   Else
 *		print(interrupt status)
 *		clean up interrupts
 *	   set ATN
 *	   while (not bus_free)
 *	   begin
 *		if (abort_mesg_sent)
 *			error++
 *		if (PHASE == mesg_out)
 *			send ABORT_MSG
 *			abort_mesg_sent++
 *		else
 *			handshake in/out byte for that phase
 *	   end
 *	   Reset transfer control circuits
 *	   Turn on card
 *	   return (error)
 *
 *		We'll use scsi->ssts register (SPC internal status)
 *		to verify SPC has disconnected from bus.
 * 
 *   We send the ABORT message to the target to get to bus free as
 *   quickly as possible.  We assume all pending data and status for 
 *   the command is cleared and the target will go *immediately* to 
 *   the BUS FREE phase.   No status or ending message will be sent 
 *   for the operation.  Only the logical unit identified is effected.
 */
scsi_if_abort(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct scsi *cp = (struct scsi *)sc->card_ptr;
	struct iobuf *iob = bp->b_queue;
	unsigned char bfr;
	int error=0, cnt=0;
	int id_needed;

	if (bp != sc->owner) {
		SCSI_TRACE(DIAGs)("scsi_if_abort not_owner: bp %x owner %x\n",
			(int)bp, (int)sc->owner);
		return(0);
                }

	/* Release DMA resources */
	if (bp->b_phase == (char)SCSI_DATA_XFR && sc->transfer == DMA_TFR) {
		scsi_if_dmaisr(sc);
		}

	/* This is a patch for Jaws: if the disconnect interrupt is not reset,
	 * apparently the lines may "float" and PSNS register is non-zero.
	 */
	if ((bfr = cp->scsi_ints) & 0x20) {
		cp->scsi_ints = bfr;    /* Reset the disconnect interrupt */
		snooze(10);
	}

	if (!cp->scsi_psns)	/* If bus free return */	{
		SCSI_TRACE(DIAGs)("scsi_if_abort: bus free\n");
		return(0);
		}


	SCSI_TRACE(DIAGs)("scsi_if_abort (bus not free) psns %x ssts %x ints %x\n",
		cp->scsi_psns, cp->scsi_ssts, cp->scsi_ints);

	/* Turn off card */
	cp->scsi_csr &= ~DIO_IE;
	if ((bfr=cp->scsi_ints)&RESELECTED)	{
		register unsigned char addr, resp = cp->scsi_temp;

		sc->set_state_state = 0;

		if (resp != (addr = 1 << sc->my_address))
			resp &= ~addr;
		for (addr = 0; resp >>= 1; addr++) ;

		if (addr != bp->b_ba)	{ /* Another device is reselecting.
					   * Just exit */
			cp->scsi_csr |= DIO_IE;
			return(0);
			}
		else	{	/* Device is reselecting - too late */
			CLEAR_reselect(bp);
			}
		}
	cp->scsi_ints = bfr;	/* Clean up interrupts */

	/* Start routine where we actually free up bus 
	 * The dilemma we have is that we are outside of a START_FSM / END_FSM
	 * yet we require a START_POLL / END_POLL in order to loop on some
	 * hardware conditions.  We use a very simplified approach to timeouts.
	 *
	 * Look at selcode.c for general discussion.
	 */
        iob->timeflag = FALSE;  /* Unused, but initialize to FALSE for safety */
try

	bp->b_queue->markstack = (int (**)())get_sp();

	timeout(scsi_if_escape, bp, TICKS, &iob->timeo);

	id_needed = 1;	/* send identify before abort */

	while (cp->scsi_ssts & 0x80)	{
		unsigned char sense;

		/* Wait for REQ to be asserted */
		while (!(cp->scsi_psns & REQ)
				&& (cp->scsi_ssts & 0x90) != 0x90)
		{
			if (!(cp->scsi_ssts & 0x80))
				goto notcon;
		}

		sense = cp->scsi_psns & 0x07;

		if ((cp->scsi_psns&0x07) == MESG_OUT_PHASE)	{
			/* Send abort message */

			if (id_needed)
			{
				bfr = MSGidentify + m_unit(bp->b_dev);
				id_needed = 0;
			}
			else
			{
				bfr = MSGabort;
			}

			    /* Turn off ATN so target will not expect 
			     * additional bytes after the abort message.
			     */
			bp->b_flags &= ~ATN_REQ;
			}
		else	{
			bfr = 0;
			/* Ask Target for message out phase */
			bp->b_flags |= ATN_REQ; 
			}
		cnt++;

		cp->scsi_pctl = sense;

		/* Manually transfer data */
		
		if (cp->scsi_psns & 0x1)	{	/* Input */
			cp->scsi_scmd = SET_ACK;
			while (cp->scsi_psns & REQ)
			/* Spin */ ;
			bfr = cp->scsi_temp;
			}
		else {				/* Output */
			cp->scsi_temp = bfr;
			cp->scsi_scmd = SET_ACK;
			while (cp->scsi_psns & REQ)
				 /* Spin */ ;
		     }
		cp->scsi_scmd = RST_ACK;
		}


notcon:
	if (cp->scsi_psns)	/* Return error if bus not free */
		escape(BUS_NOT_FREE);

	/* Clear up timeout and polling parameters */
	bp->b_queue->markstack = NULL;
	clear_timeout(&iob->timeo);

recover	{
	bp->b_queue->markstack = NULL;	
	clear_timeout(&iob->timeo);
	if (escapecode == TIMED_OUT)
		msg_printf("scsi_if_abort: timed out\n");
	else if (escapecode == BUS_NOT_FREE)
		msg_printf("scsi_if_abort: bus not free\n");

	/* Give up.  Reset bus! */
	snooze(3000000);	/* flush write cache */
	scsi_if_reset_bus(sc);

	/* Temporary.  Future: get_selcode (lock select code) then software
	 * trigger drop_selcode 5 seconds later.
	 */
	msg_printf("SCSI BUS RESET\n");
	snooze(1000000); /* give hardware time (1 sec) to recover from reset */
	}

	/* Clear FIFO's */
	cp->scsi_hconf = 0;

	/* Reset the data transfer control circuit in SPC */
	if (sc->hard_error && !(cp->scsi_ints&RESELECTED)) {
		msg_printf("Resetting transfer control circuitry\n");
		cp->scsi_sctl |=  CTRL_RST;
		cp->scsi_sctl &= ~CTRL_RST;
		sc->hard_error=0;
		}

	/* Re-enable interrupts */
	cp->scsi_csr |= DIO_IE;
	SCSI_TRACE(DIAGs)("scsi_if_abort return status: psns: 0x%x (cnt %d)\n",
			cp->scsi_psns, cnt);
	return(error);
}

#define PHASE_WRONG	0x06

/*
 * Front-end for information transfer phases
 * Handles both standard length and variable length data packets,
 * Assumptions:
 *	select code owned
 */
scsi_if_transfer(bp, bfr, count, flag, transfer_request)
struct buf *bp;
register char *bfr;	/* base address for I/O */
int count;
char flag;		/* read / write flag    */
enum transfer_request_type transfer_request;	
{
	struct isc_table_type *sc = bp->b_sc;
	struct scsi *cp = (struct scsi *)sc->card_ptr;
	struct iobuf *iob = bp->b_queue;
	int residual;

	/* Quick security check */
	if ((cp->scsi_psns & 0x01) != flag)
		SCSI_SANITY("scsi_if_transfer: confused state");

	/* Several SCSI commands (such as 'request_sense') return a variable
	 * length data packet.   Flag is set in command setup. 
	 *  - 'count' (value passed to routine) is fixed length of base packet;
	 *  - 'residual' is remaining length.
	 *  - 'transfer_request' parm is ignored if transfer is variable length
	 */
	switch(transfer_request)	{
	case MUST_FHS:
		sc->transfer = FHS_TFR;
		break;
	case MAX_SPEED:
		/*
		 * Byte aligned transfers, or odd-count
		 * transfers  must use fast hand-shake.
		 */
			if ((int)bfr & 1 || count & 1)
			    sc->transfer = FHS_TFR;
			else
			    sc->transfer = try_dma(sc, transfer_request) 
				? DMA_TFR : FHS_TFR;
		break;
	default:
		panic("scsi_if_transfer: unknown transfer request");
	}
	/*
	 * Common setup of select code structure for all transfers
	 */
	sc->buffer  = bfr;
	sc->resid   = count;
	sc->s_flags = flag;

	/* Added a sanity check for bad hardware */
	if (count <= 0)
	       SCSI_SANITY("scsi_if_transfer:zero or negative transfer count");

	/* DEBUGGING Tool used to force fhs */
	if (scsi_flags & FHS_REQ)
		sc->transfer = FHS_TFR;

	/*
	 * Kick off actual transfer
	 */
	switch(sc->transfer)	{
	case FHS_TFR:
		scsi_if_prog_xfr(bp);
		break;
	case DMA_TFR:
		{
		struct dma_channel *dma = sc->dma_chan;
		register unsigned char card_ctrl;

		/*
		 * 32 bit DMA requests requires:
		 *  - that 32 bit DMA is installed;
		 *  - that transfers are long word aligned;
		 *  - the count is a multiple of 4 bytes.
		 */
		if ((sc->state & DMA32_INSTALLED) &&
		    !((int)bfr & 3) && !(count & 3))
			sc->state |= F_LWORD_DMA;
		else
			sc->state &= ~F_LWORD_DMA;

		/* Special Environment Flags */
		if (scsi_flags & DMA16_REQ)
			sc->state &= ~F_LWORD_DMA;
		if (scsi_flags & SCSI_DMA_PRI)
			sc->state |= F_PRIORITY_DMA;
		else
			sc->state &= ~F_PRIORITY_DMA;

		dma_build_chain(bp);	/* build the transfer chain */

		/* Set up FUJI registers for transfer */
		cp->scsi_tch = RSHIFT(count,16);
		cp->scsi_tcm = RSHIFT(count,8);
		cp->scsi_tcl = RSHIFT(count,0);

		cp->scsi_pctl = cp->scsi_psns&0x07;

		card_ctrl = (unsigned char) 
                            DIO_IE | ((bp->b_flags & B_READ) ? 0x04 :0);
		if (sc->state & F_LWORD_DMA)
			card_ctrl |= SELECT_32DMA;
		cp->scsi_csr = card_ctrl;
		if ((dma->card == (struct dma *)DMA_CHANNEL0))
			cp->scsi_csr |= SCSI_DMA0;
		else
			cp->scsi_csr |= SCSI_DMA1;
		/* Start transfer */
		cp->scsi_scmd = TRANSFER;
		dma_start(dma);
		}
		break;
	default:
		panic("scsi: Unknown transfer request");
	}
}

/*
** Return length and contents of SDTR message to driver, or zero for
** asynchronous
*/
scsi_if_request_sync_data(bp, msg)
struct buf		*bp;
register unsigned char	*msg;
{
	if (scsi_flags & NOSYNC)
		min_xfr_period = 0;

	if ((msg[3] = MAX(bp->b_sc->sync, min_xfr_period)) > 157
			|| min_xfr_period == 0)
		return(0);			/* Async only */

	*msg++ = MSGext_message;
	*msg++ = 3;
	*msg++ = MSGsync_req;
	 msg++;		/* MAX(bp->b_sc->sync, min_xfr_period) */
	*msg++ = req_ack_offset;

	return(5);	/* Return length of extended message */
}


scsi_if_init(sc)
register struct isc_table_type *sc;
{
	register struct scsi *cp = (struct scsi *)sc->card_ptr;
	int level = ((cp->scsi_csr & DIO_LEVMSK) >> 4) + 3;

	/* Link isr before resetting the bus */
	isrlink(scsi_if_isr, level, &cp->scsi_csr, (DIO_IE | DIO_IR), 
			(DIO_IE | DIO_IR), 0, (int)sc);

	/* Reset bus and setup Fujitsu registers */
	scsi_if_reset_bus(sc);
	snooze(1000000);	/* let devices recover from reset */

	sc->state = F_WORD_DMA;
	/* Determine 32/16 bit DMA operation */
	if (!(cp->scsi_id & DIO_16BIT_DMA))
		if (dma_here == 3)
			sc->state |= DMA32_INSTALLED;
		else
			panic("Illegal SCSI configuration: no DMA");
	else
		if (!dma_here)
			panic("Unsupported configuration");
}

/*
** Reset the SCSI bus to prevent reselections during scsi_dump.
** Quantum disk drives may get confused by a bus reset,
** so if there are no outstanding I/O's don't do the reset.
*/
scsi_if_clear(bp)
struct buf *bp;
{
	static reset_done = 0;

	if (reset_done)
		return;

	snooze(3000000);	/* flush write cache */
	scsi_if_reset_bus(bp->b_sc);
	reset_done = 1;
	snooze(10000000);	/* let devices recover from reset */
}

struct drv_table_type scsi_if_iosw = {
	scsi_if_init,
	NULL,
	NULL,
	scsi_if_clear,
	NULL,
	scsi_if_isr,
	scsi_if_transfer,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	scsi_if_abort,		/* iod_abort */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	scsi_if_dma_setup,
};


/*
 * linking and initialization routines
 */

extern int (*make_entry)();
int (*scsi_if_saved_make_entry)();

scsi_if_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	int ret_val;
	struct scsi *cp = (struct scsi *)isc->card_ptr; /* Base Addr of Card */
	int req_lvl = isc->int_lvl;	/* Hardware interrupt level */
	char *card_name;

	if ((id&0x1f) != SCSI_ID)
		return(*scsi_if_saved_make_entry)(id, isc);
		
	req_lvl = 4;
	isc->iosw = &scsi_if_iosw;
	isc->card_type = SCUZZY;
	ret_val=io_inform("HP98265A (SCSI Interface)", isc, req_lvl);

	/*
	** determine maximum synchronous data transfer rate
	*/
	if (hidden_mode_exists(isc->my_isc)) {
		isc->sync = 50;		/* xfr_period = 200 ns */
		printf("        5 MB/s; ");
	} else if (cp->scsi_hconf & MAX_SYNC_XFR) {
		isc->sync = 93;
		printf("        2.67 MB/s; ");
	} else {
		isc->sync = 62;		/* xfr_period = 248 ns */
		printf("        4 MB/s; ");
	}

	if (cp->scsi_hconf&PARITY)
		printf("parity enabled.\n");
	else
		printf("parity NOT enabled.\n");

	if ((SCSI_ADDR&(~cp->scsi_hconf)) != 7)
		printf("Warning:  HP98265A initiator address is %d. \n", 
			SCSI_ADDR&(~cp->scsi_hconf));
	return(ret_val);
}

/*
 *  one-time linking code
 */
scsi_if_link()
{
	extern struct msus (*msus_for_boot)();

	scsi_if_saved_make_entry = make_entry;
	make_entry = scsi_if_make_entry;
}

/*
** scsi_set_state operational parameters
*/
unsigned spin_usecs = 2000;	/* don't make bigger than 65535 */
unsigned set_state_ticks = HZ;

scsi_set_state(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct iobuf *iob = bp->b_queue;
	register struct scsi *cp = (struct scsi *)sc->card_ptr;
	unsigned short start, delta;
	extern unsigned short get_4_usec_tick();

#ifdef OSDEBUG
	{ int x; splx(x = spl6()); VASSERT(x == sc->int_lvl); }
#endif /* OSDEBUG */

	do {
		if (!CONNECTED_TO_SCSI || cp->scsi_psns & REQ
				|| (cp->scsi_ssts & 0x90) == 0x90) {
			sc->set_state_state = 0;
			scsi_set_state_done(bp);
			return(0);
		}

		if (sc->set_state_state == 0) {
			start = get_4_usec_tick();
			sc->start_lbolt = lbolt;
			sc->set_state_state = 1;
		}

		delta = get_4_usec_tick() - start;

	}
	while (delta < (spin_usecs >> 2) && sc->set_state_state != 2) ;

	sc->set_state_state = 2;

	if ((unsigned)(lbolt - sc->start_lbolt) >= set_state_ticks) {
		iob->b_state = transfer_TO;
		return(0);
	}

	return(1);
}

#ifdef SAVECORE_300
/*
** SCSI bus has been reset by scsi_if_clear.
*/
scsi_if_dump(log2blk)
int log2blk;
{
	extern dev_t dumpdev;
	extern long dumplo, dumpsize;

	struct isc_table_type *isc = isc_table[m_selcode(dumpdev)];
	struct scsi *cp = (void *) isc->card_ptr;

	register u_char *ptr;
	register u_char *dreg = &cp->scsi_dreg;
	register u_char *ssts = &cp->scsi_ssts;
	register u_char *end;

	u_char *addr;
	u_int lba;
	int resid;

	u_char phase, message, status, cdb[10];
	u_int cnt, blkcnt;
	u_int i, cdblen;
	int reject_message;

	if (cp->scsi_ints != 0)
		return -1;

	addr = (void *) (physmembase << PGSHIFT);
	resid = dumpsize << PGSHIFT;

	if (DEV_BSHIFT > log2blk)
		lba = dumplo << (DEV_BSHIFT - log2blk);
	else
		lba = dumplo >> (log2blk - DEV_BSHIFT);

	reject_message = 0;

	bzero(cdb, 10);
	cdb[0] = CMDtest_unit_ready;
	cdblen = 6;
	cnt = 0;
	blkcnt = 0;

select:

	/*
	** Set up for select.
	*/
	cp->scsi_pctl = 0;
	cp->scsi_temp = 1 << m_busaddr(dumpdev) | 1 << isc->my_address;
	cp->scsi_tch = ((250 * 1000000) / (125 * 2) - 15) / 256 / 256;
	cp->scsi_tcm = ((250 * 1000000) / (125 * 2) - 15) / 256 % 256;
	cp->scsi_tcl = 0x04;
	cp->scsi_scmd = SET_ATN;

	/*
	** Select.
	*/

	cp->scsi_scmd = SELECT;

	while (cp->scsi_ints == 0) ;

	if (cp->scsi_ints != CMD_COMPLETE)
		return -1;

	cp->scsi_ints |= CMD_COMPLETE;

	/*
	** Selected.
	*/

next_phase:

	while (!(cp->scsi_psns & REQ)) ;

	phase = cp->scsi_psns & 0x07;
	cp->scsi_pctl = phase;

	switch (phase)
	{

	case MESG_OUT_PHASE:

		if (reject_message)
		{
			message = MSGmsg_reject;
			reject_message = 0;
		}
		else
		{
			message = MSGidentify;
		}

		cp->scsi_scmd = RST_ATN;
		cp->scsi_temp = message;
		cp->scsi_scmd = SET_ACK;
		while (cp->scsi_psns & REQ) ;
		cp->scsi_scmd = RST_ACK;
		goto next_phase;

	case CMD_PHASE:

		if (cdb[0] == CMDwrite_ext)
		{
			if (resid > 65536)
				cnt = 65536;
			else
				cnt = resid;

			blkcnt = cnt >> log2blk;

			cdb[3] = lba >> 16;
			cdb[4] = lba >> 8;
			cdb[5] = lba;
			cdb[7] = blkcnt >> 8;
			cdb[8] = blkcnt;

			cdblen = 10;
		}

		for (i = 0; i < cdblen; i++)
		{
			while (!(cp->scsi_psns & REQ)) ;
			cp->scsi_temp = cdb[i];
			cp->scsi_scmd = SET_ACK;
			while (cp->scsi_psns & REQ) ;
			cp->scsi_scmd = RST_ACK;
		}

		goto next_phase;

	case STATUS_PHASE:

		status = cp->scsi_temp;
		cp->scsi_scmd = SET_ACK;
		while (cp->scsi_psns & REQ) ;
		cp->scsi_scmd = RST_ACK;
		goto next_phase;

	case DATA_OUT_PHASE:

		cp->scsi_tch = cnt >> 16;
		cp->scsi_tcm = cnt >> 8;
		cp->scsi_tcl = cnt;
		cp->scsi_tmod = 0;
		cp->scsi_scmd = TRANSFER + PROG_XFR;
		while ((*ssts & 0xf0) != 0xb0) ;

		ptr = addr;
		end = ptr + cnt;
		while (ptr != end)
		{
			while (!(*ssts & DREG_EMPTY)) ;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
			*dreg = *ptr++;
		}

		while (!(cp->scsi_ints & CMD_COMPLETE)) ;
		cp->scsi_ints |= CMD_COMPLETE;

		goto next_phase;

	case MESG_IN_PHASE:

		message = cp->scsi_temp;
		cp->scsi_scmd = SET_ACK;
		while (cp->scsi_psns & REQ) ;
		cp->scsi_scmd = RST_ACK;

		switch (message)
		{

		case MSGcmd_complete:

			while (!(cp->scsi_ints & DISCONNECTED)) ;
			cp->scsi_ints = DISCONNECTED;
			if (cp->scsi_ints != 0)
				return -1;

			if (status != S_GOOD)
				goto select;

			if (cdb[0] == CMDtest_unit_ready)
				cdb[0] = CMDwrite_ext;

			addr += cnt;
			lba += blkcnt;
			resid -= cnt;

			if (resid != 0)
				goto select;

			return 0;

		default:

			cp->scsi_scmd = SET_ATN;
			reject_message = 1;
			goto next_phase;
		}

	default:

		return -1;

	}
}
#endif
