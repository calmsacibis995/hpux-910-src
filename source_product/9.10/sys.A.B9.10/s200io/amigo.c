/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/amigo.c,v $
 * $Revision: 1.5.84.6 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 14:06:18 $
 */

/* HPUX_ID: @(#)amigo.c	55.1		88/12/23 */

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


/* #define AMIGODEBUG  /* uncomment for debugging */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/iobuf.h"
#include "../s200io/amigo.h"
#include "../s200io/dma.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../wsio/tryrec.h"
#include "../s200io/bootrom.h"
#include "../h/dk.h"

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

int amigo_config_identify();

extern int (*hpib_config_ident)();
#endif /* IOSCAN */

/*
**  mnemonic names for iobuf scratch registers
*/
#define iob_current_mask io_s0		/* (char) */
#define iob_resid io_s1			/* (int) */
#define iob_tcount io_s2		/* (int) */

/*
** mediainit(1) uses the most significant bit of this
** scratch reg. as a locking mechanism, via the iobuf's
** b_flags field. If the device is an HP9895_SS IBM formatted
** then specify via the iobuf's b_flags field also.
*/
#define IOB_LOCKED 	B_SCRACH5	/* (long) */
#define IBM_FORMATTED   B_SCRACH4

/*
**  mnemonic names for buf scratch registers
*/
#define b_log2blk		b_s0	/* (char) */
#define b_ioctl_flag		b_s0	/* (char) */
#define b_type			b_s1	/* (char) */
#define b_TO_state		b_s2	/* (long) */
#define b_dk_index		b_s3	/* (char) */

/*
**  mnemonic names for buf's b_flags scratch bits
*/
#define B_BUFFERED_TRANS	B_SCRACH1
#define B_SYNC_UNBUF		B_SCRACH2
#define B_PPOLL_INT_TIMEDOUT	B_SCRACH3

/*
** timeout and retry parameters
*/
#define SHORT_MSG_TIME		((   250 * HZ) / 1000)
#define SEEK_TIME		(( 30000 * HZ) / 1000)
#define TRANSFER_SETUP_TIME	((  5000 * HZ) / 1000)
#define TRANSFER_TIME		(( 60000 * HZ) / 1000)
#define TRANSFER_CLEANUP_TIME	((  5000 * HZ) / 1000)
#define REQ_STATUS_TIME		(( 30000 * HZ) / 1000)
#define PROCESS_STATUS_TIME	((  5000 * HZ) / 1000)
#define CLEAR_TIME		((  5000 * HZ) / 1000)
#define MAXTRIES	10
#define MAXTIMEOUTS	3

/*
**  disc profiling instrumentation
*/
#define DK_DEVT_MASK	0x00fffff0	/* all minor fields except volume */
#define DK_DEVT_MAJ	0xff000000	/* major field */

/*
**  amigo open device table
*/
#define	ODS	8			/* number of open device entries */
struct amigo_od amigo_odt[ODS];		/* open device table */

/*
**  media addressing parameters (and other things)
**
**    cylinders is effective (after sparing), not physical count
*/
struct map_type device_maps [] = {
		 /* cylinders heads   sectors   ident model rps log2blk flags */
{/* HP7906 */		400,	2,	48,	0x0002,	0, 60, 8, 5,
		SURFACE_MODE+SYNC_UNBUF+NOWAIT_STATUS+RECALIBRATES+END_NEEDED},
{/* HP7905_L */		400,	1,	48,	0x0002,	2, 60, 8, 5,
		SURFACE_MODE+SYNC_UNBUF+NOWAIT_STATUS+RECALIBRATES+END_NEEDED},
{/* HP7905_U */		400,	2,	48,	0x0002,	2, 60, 8, 5,
		SURFACE_MODE+SYNC_UNBUF+NOWAIT_STATUS+RECALIBRATES+END_NEEDED},
#define SURF_SECTORS (400*48) /* for surface crossing check */
{/* HP7920 */		800,	5,	48,	0x0002,	1, 60, 8, 7,
		SYNC_UNBUF+NOWAIT_STATUS+RECALIBRATES+END_NEEDED},
{/* HP7925 */		800,	9,	64,	0x0002,	3, 45, 8, 7,
		SYNC_UNBUF+NOWAIT_STATUS+RECALIBRATES+END_NEEDED},
{/* HP9895_SS_BLNK */	73,	1,	30,	0x0081,	1, 6, 8, 0, 0},
{/* HP9895_SS */	73,	1,	30,	0x0081,	2, 6, 8, 0, 0},
{/* HP9895_DS_BLNK */	75,	2,	30,	0x0081,	5, 6, 8, 0, 0},
{/* HP9895_DS */	75,	2,	30,	0x0081,	6, 6, 8, 0, 0},
{/* HP9895_IBM */	77,	1,	26,	0x0081,	8, 6, 7, 0, 0},
{/* HP8290X */		33,	2,	16,	0x0104,	R_BIT_0, 5, 8, 0,
		MUST_BUFFER},
{/* HP9121 */		33,	2,	16,	0x0104,	R_BIT_1, 10, 8, 0, 0},
{/* HP913X_A */		152,	4,	31,	0x0106,	NO_MODEL, 60, 8, 0, 0},
{/* HP913X_B */		305,	4,	31,	0x010A,	NO_MODEL, 60, 8, 0, 0},
{/* HP913X_C */		305,	6,	31,	0x010F,	NO_MODEL, 60, 8, 0, 0},
{/* NOUNIT */		0,	0,	0,	0,	0, 0, 0, 0, 0},
};

/* The following routines are general utilities that do not do IO */


int records_per_medium(map)
register struct map_type *map;
{
	return map->sec_per_trk * map->trk_per_cyl * map->cyl_per_med;
}


struct tva_type coded_addr(bp, record_addr, ibm_format)
struct buf *bp;
daddr_t record_addr;
char ibm_format;
{
	register struct map_type *map;
	short track;
	struct tva_type t_ca;

	map = &device_maps[bp->b_type];
	t_ca.sect = (short)(record_addr % map->sec_per_trk + 
			(ibm_format ? 1 : 0) );
	track	  = (short)(record_addr / map->sec_per_trk + 
			(ibm_format ? 1 : 0) );

	if (map->flag & SURFACE_MODE) {
		t_ca.head = track / map->cyl_per_med + 2 * m_volume(bp->b_dev);
		t_ca.cyl  = track % map->cyl_per_med;
	} else {
		t_ca.head = track % map->trk_per_cyl;
		t_ca.cyl  = track / map->trk_per_cyl;
	}
	return t_ca;
}


int decoded_addr(bp, tva)
struct buf *bp;
struct tva_type *tva;
{
	register struct map_type *map;
	register int track;

	map = &device_maps[bp->b_type];
	track = (map->flag & SURFACE_MODE) ?
		(tva->head-2*m_volume(bp->b_dev))*map->cyl_per_med+tva->cyl :
		tva->cyl*map->trk_per_cyl+tva->head;
	return track*map->sec_per_trk+tva->sect;
}


get_dsj(bp)
register struct buf *bp;
{
	char dsj;

	(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_DSJ, &dsj, sizeof dsj);
	return dsj;
}


static do_dsj(bp)
struct buf *bp;
{
	*(char *)bp->b_un.b_addr = get_dsj(bp);
	return 0; /* no parallel poll */
}


issue_cmd(bp, command, cmd_buf_ptr)
struct buf *bp;
enum command_type command;
struct ftcb_type *cmd_buf_ptr;
{
	static struct ctet /* command table entry type */ {
		short sec;	/* secondary command */
		char  oc;	/* opcode */
		char  nb;	/* number of data bytes */
	} command_table[] = {
		  /*   command			  sec	 oc    nb */
		{ /* seek cmd 		*/	SEC_OP1,  2,	6},
		{ /* buffered read	*/	SEC_OP3,  5,	2},
		{ /* buffered write	*/	SEC_OP2,  8,	2},
		{ /* unbuffered read	*/	SEC_OP1,  5,	2},
		{ /* unbuffered write	*/	SEC_OP1,  8,	2},
		{ /* request status 	*/	SEC_OP1,  3,	2},
		{ /* request log addr 	*/	SEC_OP1, 20,	2},
		{ /* recalibrate cmd 	*/	SEC_OP1,  1,	2},
		{ /* request syndrome 	*/	SEC_OP1, 13,	2},
		{ /* end cmd	 	*/	SEC_OP1, 21,	2},
	};
	
	register struct ctet *ctptr = &command_table[(int)command];

	cmd_buf_ptr->opcode = ctptr->oc;
	cmd_buf_ptr->unit = m_unit(bp->b_dev);
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI+T_PPL, ctptr->sec,
		(char *)cmd_buf_ptr, ctptr->nb);
}


set_file_mask(bp, mask)
struct buf *bp;
int mask;
{
	struct {
		char oc;
		char mask;
	} sfm_cmd;

	sfm_cmd.oc = 15;
	sfm_cmd.mask = mask;
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI+T_PPL, SEC_OP1,
		(char *)&sfm_cmd, sizeof(sfm_cmd));
}


recalibrate(bp)
struct buf *bp;
{
	struct ftcb_type recalibrate_cmd_buf;

	issue_cmd(bp, recalibrate_cmd, &recalibrate_cmd_buf);
}


int request_status(bp)
struct buf *bp;
{
	struct ftcb_type status_cmd_buf;

	issue_cmd(bp, req_status, &status_cmd_buf);
	return !(device_maps[bp->b_type].flag & NOWAIT_STATUS);
}


struct status_type return_status(bp)
struct buf *bp;
{
	struct status_type status_bytes;

	(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_RSTA,
		(char *)&status_bytes, sizeof status_bytes);
	return status_bytes;
}


syndrome(bp, syndrome_bytes)
struct buf *bp;
struct syndrome_type *syndrome_bytes;
{
	struct ftcb_type syndrome_cmd_buf;

	issue_cmd(bp, req_syndrome, &syndrome_cmd_buf);
	(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_RSTA,
		(char *)&syndrome_bytes, sizeof syndrome_bytes);
}


amigoseek(bp, record_addr, possible_ibm)
struct buf *bp;
daddr_t record_addr;
char possible_ibm;
{
	struct {
		struct ftcb_type ftcb;
		struct tva_type  tva;
	} seek_cmd_buf;

	seek_cmd_buf.tva = coded_addr(bp, record_addr, possible_ibm);
	issue_cmd(bp, seek_cmd, (struct ftcb_type *)&seek_cmd_buf);
}


int logical_addr(bp)
struct buf *bp;
{
	struct ftcb_type ladd_cmd_buf;
	struct tva_type tva;

	issue_cmd(bp, req_log_addr, &ladd_cmd_buf);
	if (!(device_maps[bp->b_type].flag & NOWAIT_STATUS))
		HPIB_ppoll(bp,1);
	(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_RSTA, (char *)&tva, sizeof tva);
	return decoded_addr(bp,&tva);
}


do_end(bp)
struct buf *bp;
{
	struct ftcb_type end_cmd_buf;

	issue_cmd(bp, end_cmd, &end_cmd_buf);
	return 0; /* no parallel poll */
}


amigo_x_info(bp)
struct buf *bp;
{
	/* if timeout is called, you still need to dequeue after proceesing
	   the timeout */
	amigo_info(bp);
	do {
	} while (selcode_dequeue(bp->b_sc) | dma_dequeue());
}


amigo_info_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,amigo_x_info,bp->b_sc->int_lvl,0,bp->b_TO_state);
}


/*
** this FSM is similar to HPIB_utility, and is suitable for performing amigo
** status, amigo logical address, and possibly other amigo transactions
*/
amigo_info(bp)
register struct buf *bp;
{

	enum states {
		start = 0,
		request_it,
		ppoll_TO,
		ppoll,
		receive_it,
		defaul
	};

	register struct iobuf *iob = bp->b_queue;
	int take_ppoll;

	try
		START_FSM;

reswitch:	switch ((enum states) iob->b_state) {

		case start:  /* get select code */
			bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
			iob->b_state = (int)request_it;
			get_selcode(bp, amigo_info);
			break;

		case request_it:
			START_TIME(amigo_info_timeout, SHORT_MSG_TIME);
			take_ppoll = (*bp->b_action2)(bp);
			END_TIME;
			if (take_ppoll) {
				bp->b_TO_state = (int)ppoll_TO;
				START_TIME(amigo_info_timeout, bp->b_clock_ticks);
				iob->b_state = (int)ppoll;
				HPIB_ppoll_drop_sc(bp, amigo_info, 1);
				break;
			} else {
				iob->b_state = (int)receive_it;
				goto reswitch;
			}

		case ppoll_TO:  /* timed out waiting ppoll; confirm it later */
			HPIB_ppoll_clear(bp);
			bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
			/* drop through */

		case ppoll:
			END_TIME;
			iob->b_state = (int)receive_it;
			get_selcode(bp, amigo_info);
			break;

		case receive_it:
			if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
				if (HPIB_test_ppoll(bp, 1))
					bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
				else
					escape(TIMED_OUT);

			START_TIME(amigo_info_timeout, SHORT_MSG_TIME);
			(*bp->b_sc->iosw->iod_mesg)(bp, 0, SEC_RSTA,
						bp->b_un.b_addr, bp->b_bcount);
			END_TIME;
			drop_selcode(bp);
			iob->b_state = (int)defaul;
			queuedone(bp);
			break;

		default:
			panic("unknown amigo_info state");
		}
		END_FSM;
	recover {
		ABORT_TIME;
		if (escapecode == TIMED_OUT) {
			(*bp->b_sc->iosw->iod_abort)(bp);
			HPIB_ppoll_clear(bp);
		}
		drop_selcode(bp);
		iob->b_state = (int)defaul;
		bp->b_error = EIO;
		queuedone(bp);
	}
}


amigo_x_transfer(bp)
register struct buf *bp;
{
	/* if timeout is called, you still need to dequeue after proceesing
	   the timeout */
	register int i;
	register struct isc_table_type *sc;

	sc = bp->b_sc;
	amigo_transfer(bp);
	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	}
	while (i);
}

amigo_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,amigo_x_transfer,bp->b_sc->int_lvl,0,bp->b_TO_state);
}

amigo_transfer(bp)
struct buf *bp;
{
	enum states {
		initial=0,
		start,
		seek,
		get1_TO,
		get1,
		get2,
		setup,
		setup2_TO,
		setup2,
		setup3,
		dsj,
		dsj2_TO,
		dsj2,
		dsj3,
		timedout,
		cleanup_timeout,
		cleanup_timeout1,
		cleanup_timeout2_TO,
		cleanup_timeout2,
		defaul
	};

	register struct map_type *map = &device_maps[bp->b_type];
	register struct isc_table_type *sc = bp->b_sc;
	register struct iobuf *iob = bp->b_queue;
	register int dk_index = bp->b_dk_index;
	char ibm_format_flag = 0;

retry:	try
		START_FSM;
reswitch:
		if (dk_index >= 0)
			dk_busy &= ~(1 << dk_index);

		switch ((enum states) iob->b_state) {

/* entire switch clause shifted left to allow more room! */

case initial:  /* entered once only, to initialize the iobuf */
	iob->b_errcnt = 0;  /* retry count */
	iob->b_xaddr = bp->b_un.b_addr;
	iob->iob_tcount = bp->b_bcount;
	bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
	bp->b_TO_state = (int)timedout;

	/*
	** Don't allow anything to happen if the media addressing parameters
	** (map) points to NOUNIT for this "&device_maps[bp->b_type]".
	** This means unformatted media is in the device. The open
	** is allowed to complete because mediainit(1) may be running. So,
	** with potentially unformatted media in the device we have to protect
	** ourselves here. Mediainit doesn't use the same FSM as the driver.
	*/

	if( (map->ident == 0) || (map->rps == 0) || (map->cyl_per_med == 0) )
	{
		iob->b_errcnt = EIO;
		escape(1);
	}


	/* drop through */

case start:  /* get select code */
	iob->b_state = (int)seek;
	get_selcode(bp, amigo_transfer);
	break;

case seek:  /* issue seek command */
	if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
		if (HPIB_test_ppoll(bp, 1))
			bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
		else
			escape(TIMED_OUT);

	if (dk_index >= 0) {
		dk_seek[dk_index]++;
		dk_busy |= 1 << dk_index;
	}

	/*
	** if Chinook is in power-on hold-off,
	** it HANGS instead of sinking command messages!!!
	*/
	if (bp->b_type == (int)HP8290X) {
		START_TIME(amigo_timeout, SHORT_MSG_TIME);
		(void)get_dsj(bp);
		END_TIME;
	}

	if (iob->iob_current_mask != map->file_mask) {
		START_TIME(amigo_timeout, SHORT_MSG_TIME);
		set_file_mask(bp, map->file_mask);
		iob->iob_current_mask = map->file_mask;
		HPIB_ppoll(bp,1);  /* about 1 ms on 7906 */
		END_TIME;
	}
	
	ibm_format_flag = ( iob->b_flags & IBM_FORMATTED ) ? 1 : 0;
	START_TIME(amigo_timeout, SHORT_MSG_TIME);
	amigoseek(bp, bp->b_un2.b_sectno, ibm_format_flag);
	END_TIME;

	/*
	** If Chinook has no media present, and a write is attempted,
	** it HANGS instead of sinking receive_data messages!!!
	** Thus, we need to check the dsj on it's seek.
	*/
	if (bp->b_type == (int)HP8290X) {
		iob->b_xcount = 0;
		bp->b_TO_state = (int)dsj2_TO;
		iob->b_state = (int)dsj2;
	} else {
		bp->b_TO_state = (int)get1_TO;
		iob->b_state = (int)get1;
	}
	START_TIME(amigo_timeout, SEEK_TIME);
	HPIB_ppoll_drop_sc(bp, amigo_transfer, 1);
	break;

case get1_TO:  /* timed out waiting for ppoll; confirm it later */
	HPIB_ppoll_clear(bp);
	bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
	/* drop through */

case get1:  /* pick up the select code again */
	END_TIME;
	bp->b_TO_state = (int)timedout;

	iob->b_state = (bp->b_flags & B_SYNC_UNBUF) ? (int)get2 : (int)setup;
	get_selcode(bp, amigo_transfer);
	break;

case get2:  /* if synchronous unbuffered, pre-allocate the dma channel */
	iob->b_state = (int)setup;
	get_dma(bp, amigo_transfer);
	break;

case setup:  /* issue the command message */
	if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
		if (HPIB_test_ppoll(bp, 1))
			bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
		else
			escape(TIMED_OUT);

	iob->b_xcount = (bp->b_flags & B_BUFFERED_TRANS &&
				iob->iob_tcount >= 256) ? 256 : iob->iob_tcount;

	if (dk_index >= 0) {
		dk_busy |= 1 << dk_index;
		dk_xfer[dk_index]++;
		dk_wds[dk_index] += iob->b_xcount >> 6;
	}

	if (bp->b_flags & B_SYNC_UNBUF) {
		struct ftcb_type cmd_buf;

		cmd_buf.opcode = (bp->b_flags & B_READ) ?
						UNBUF_READ_OC : UNBUF_WRITE_OC;
		cmd_buf.unit = m_unit(bp->b_dev);
		START_TIME(amigo_timeout, TRANSFER_TIME);
		iob->b_state = (int)dsj;
		(*sc->iosw->iod_hst)(bp, SEC_OP1, SEC_xDAT, &cmd_buf, 2,
								amigo_transfer);
	} else {
		struct ftcb_type cmd_buf;

		START_TIME(amigo_timeout, SHORT_MSG_TIME);
		issue_cmd(bp,
		          (bp->b_flags & B_BUFFERED_TRANS) ?
			    ((bp->b_flags & B_READ) ? buf_read : buf_write) :
			    ((bp->b_flags & B_READ) ? unbuf_read : unbuf_write),
			  &cmd_buf);
		END_TIME;

		if (map->flag & NOWAIT_STATUS || HPIB_test_ppoll(bp, 1)) {
			iob->b_state = (int)setup3;
			goto reswitch;
		}

		bp->b_TO_state = (int)setup2_TO;
		START_TIME(amigo_timeout, TRANSFER_SETUP_TIME);
		iob->b_state = (int)setup2;
		HPIB_ppoll_drop_sc(bp, amigo_transfer, 1);
	}
	break;

case setup2_TO:  /* timed out waiting for ppoll; confirm it later */
	HPIB_ppoll_clear(bp);
	bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
	/* drop through */

case setup2:  /* pick up the select code again */
	END_TIME;
	bp->b_TO_state = (int)timedout;

	iob->b_state = (int)setup3;
	get_selcode(bp, amigo_transfer);
	break;

case setup3:  /* initiate the execution message */
	if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
		if (HPIB_test_ppoll(bp, 1))
			bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
		else
			escape(TIMED_OUT);

	START_TIME(amigo_timeout, TRANSFER_TIME);
	(*sc->iosw->iod_preamb)(bp, SEC_xDAT);
	iob->b_state = (int)dsj;
	(*sc->iosw->iod_tfr)(MAX_SPEED, bp, amigo_transfer);
	break;

case dsj:  /* issue postamble & wait for ppoll response */
	iob->iob_resid = sc->resid;
	(*sc->iosw->iod_postamb)(bp);
	END_TIME;

	if (dk_index >= 0)
		dk_busy |= 1 << dk_index;

	if (HPIB_test_ppoll(bp, 1)) {
		iob->b_state = (int)dsj3;
		goto reswitch;
	}

	bp->b_TO_state = (int)dsj2_TO;
	START_TIME(amigo_timeout, TRANSFER_CLEANUP_TIME);
	iob->b_state = (int)dsj2;
	HPIB_ppoll_drop_sc(bp, amigo_transfer, 1);
	break;

case dsj2_TO:  /* timed out waiting for ppoll; confirm it later */
	HPIB_ppoll_clear(bp);
	bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
	/* drop through */

case dsj2:  /* pick up the select code again */
	END_TIME;
	bp->b_TO_state = (int)timedout;

	iob->b_state = (int)dsj3;
	get_selcode(bp, amigo_transfer);
	break;

case dsj3:  /* check the dsj; potentially process status */
	if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
		if (HPIB_test_ppoll(bp, 1))
			bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
		else
			escape(TIMED_OUT);

	START_TIME(amigo_timeout, PROCESS_STATUS_TIME);
	if (get_dsj(bp) == 0 || status_indicates_some_transferred(bp)) {
		iob->b_xaddr += iob->b_xcount;
		iob->iob_tcount -= iob->b_xcount;
		bp->b_un2.b_sectno += iob->b_xcount >> bp->b_log2blk;
		bp->b_resid -= iob->b_xcount;
		if (iob->iob_tcount > 0 && bp->b_flags & B_BUFFERED_TRANS) {
			END_TIME;
			iob->b_state = (int)setup;
			goto reswitch;
		}
	} else if (iob->b_errcnt >= MAXTRIES)
		bp->b_error = EIO;

	if (iob->iob_tcount > 0 && !bp->b_error) {
		END_TIME;
		iob->b_state = (int)seek;
		goto reswitch;
	}

	if (map->flag & END_NEEDED)
		do_end(bp);
	END_TIME;

	drop_selcode(bp);
	iob->b_state = (int)defaul;
#ifdef AMIGODEBUG
	if (bp->b_error)
		printf("amigo: errno %d\n", bp->b_error);
#endif
	queuedone(bp);
	break;

case timedout:
	escape(TIMED_OUT);

case cleanup_timeout:
	iob->b_state = (int)cleanup_timeout1;
	get_selcode(bp, amigo_transfer);
	break;

case cleanup_timeout1:
	bp->b_TO_state = (int)timedout;
	START_TIME(amigo_timeout, SHORT_MSG_TIME);
	(*bp->b_sc->iosw->iod_clear)(bp);
	END_TIME;

	iob->iob_current_mask = 0;

	bp->b_TO_state = (int)cleanup_timeout2_TO;
	START_TIME(amigo_timeout, CLEAR_TIME);
	iob->b_state = (int)cleanup_timeout2;
	HPIB_ppoll_drop_sc(bp, amigo_transfer, 1);
	break;

case cleanup_timeout2_TO:  /* timed out waiting for ppoll; confirm it later */
	HPIB_ppoll_clear(bp);
	bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
	/* drop through */

case cleanup_timeout2:
	END_TIME;
	bp->b_TO_state = (int)timedout;

	iob->b_state = (int)start;
	goto reswitch;

default:
	panic("amigo_transfer: unrecognized state");

		}  /* end switch */

		END_FSM;
	recover {
		if (dk_index >= 0)
			dk_busy &= ~(1 << dk_index);
		iob->b_state = (int)defaul;
		ABORT_TIME;
		if (escapecode == TIMED_OUT) {
			HPIB_ppoll_clear(bp);
			(*bp->b_sc->iosw->iod_abort)(bp);
			drop_selcode(bp);
#ifdef AMIGODEBUG
			printf("amigo: timeout\n");
#endif
			if (++iob->b_errcnt < MAXTIMEOUTS) {
				iob->b_state = (int)cleanup_timeout;
				goto retry;
			}
		} else
			drop_selcode(bp);
		if (!bp->b_error)
			bp->b_error = EIO;
#ifdef AMIGODEBUG
		if (bp->b_error)
			printf("amigo(recover): errno %d\n", bp->b_error);
#endif
		queuedone(bp);
	}
}


int status_indicates_some_transferred(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register int some_transferred = FALSE;  /* initial assumption */

	struct status_type status_bytes;
	register long err_rcrd;

	if (request_status(bp))
		HPIB_ppoll(bp, 1);  /* really should release processor here */
	status_bytes = return_status(bp);
	
#ifdef AMIGODEBUG
	printf("amigo disc status s:%d p:%d d:%d s1:0x%x unit:%d\n",
		status_bytes.s, status_bytes.p, status_bytes.d,
		status_bytes.s1, status_bytes.unit);
	printf("  *:%d xx:%d tttt:%d r:%d a:%d w:%d fmt:%d e:%d f:%d c:%d ss:%d\n",
		status_bytes.star, status_bytes.xx, status_bytes.tttt,
		status_bytes.r, status_bytes.a, status_bytes.w,
		status_bytes.fmt, status_bytes.e, status_bytes.f,
		status_bytes.c, status_bytes.ss);
#endif

	if (get_dsj(bp))
		bp->b_error = EIO;
	else
		switch ((enum s1_type)status_bytes.s1) {

/*
**  The madness caused by MAC/IDC address verification:
**  Due to the non-separated format for address and data, MAC/IDC discs verify
**  the address of the sector physically preceeding the first sector of a
**  transfer, or the first sector of a new track.  The manuals lead you to
**  believe that this is transparent; however, if an error occurs during
**  address verification, the logical address returned is that of the sector
**  whose address is being verified, NOT the sector we are trying to transfer
**  data to/from!!!  This leads to two interesting cases:
**  1)  transfers not starting on a track boundary:  an attempt to read sector
**  N results in an "error" in sector N-1 !  This isn't hard to handle, as it
**  is easily tested for.
**  2)  transfers starting on a track boundary or crossing a track boundary:
**  when attempting to access sector N, an "error" may occur on sector N+47 or
**  N+63, depending on the number of sectors per track.  With reads, we can
**  test the number of bytes transferred, but on writes, I know of nothing we
**  can test, since the remainder of the data byte stream sent by us merely
**  gets sinked by the disc controller.
**
**  Why the MAC/IDC drivers need/want a returned logical address:
**  1)  At the end of a synchronous unbuffered READ, we may not unaddress the
**  disc controller before it has read BEYOND where we actually care about.
**  If errors occur there, such as running into a spare track, we NEED to
**  recognize the fact, and blow off such errors, since any following retries
**  will probably fail for the same reason.
**  2)  For the best shot at recovering data plus the simplest logic in doing
**  error correction via the request syndrome command, we WANT retries to start
**  at the point of the error, not necessarily the original start of the
**  transfer.
**
**  Rules based upon the above hard facts of life:
**  1)  Since the logical address CANNOT be trusted with writes, there can
**      be no partial retries with writes.
**  2)  With reads, we can trust the logical address returned by the controller
**      only if we can distinguish address verification errors, but such means
**      as a transferred byte count or logical address returned by syndrome.
*/

/* retryable error where the address or data is untrustworthy */
case BUS_OVERRUN:
	/* MAC's two sector buffering makes the reported address untrustworthy;
	   also, according to Kent Wilken, data going in/out the PHI may get
	   corrupted when the data overrun hardware interrupt comes */
	/*
	** We purposely do not increment b_errcnt, so that the
	** operation will repeat until no overrun occurs.
	** I hope this won't ever burn us!
	*/
	break;
	
/* Sparrow's power-on hold-off returns dsj=2, but no first status bit */
case NORMAL_COMPLETION:
/* retryable errors */
case END_OF_CYLINDER:
	iob->iob_current_mask = 0;
	iob->b_errcnt++;  /* guarantee we won't loop forever */
	break;

case CYLINDER_COMPARE_ERROR:
	if (!(device_maps[bp->b_type].flag & RECALIBRATES)) {
		bp->b_error = EIO;  /* Small discs retry on their own */
		break;
	}
	/* MAC/IDC recalibrate can take seconds, but on the other hand MAC/IDC
	   are never to be part of the product, and I've never seen it happen,
	   thus its not worth much effort!  It should be error-logged. */
	recalibrate(bp);
	HPIB_ppoll(bp, 1);
	/* drop thru */
case UNCORRECTABLE_DATA_ERROR:
case HEAD_SECTOR_COMPARE_ERROR:
case SYNC_BIT_NOT_RECEIVED_IN_TIME:
case POSSIBLY_CORRECTABLE_DATA_ERROR:
case ILLEGAL_ACCESS_TO_SPARE:
case DEFECTIVE_TRACK:
case ACCESS_NOT_READY_DURING_DATA_OP:
	if ((err_rcrd = logical_addr(bp)) <= bp->b_un2.b_sectno ||
	    !(bp->b_flags & B_READ)) {
		/*
		**  NO partial retry is possible:  the error either occurred on
		**  or before the first sector, or this is a write operation.
		*/
		iob->b_errcnt++;
		if (status_bytes.s1 == (int)POSSIBLY_CORRECTABLE_DATA_ERROR &&
		    iob->b_errcnt > MAXTRIES/2) {
			struct syndrome_type syndrome_bytes;
			/* assumption: any "possibly recoverable" error will
			   have been retried before the syndrome is used.
			   Doubts have been expressed about whether the disc
			   works right doing syndrome.  Thus it is valid to
			   assume that its the first sector read if there is an
			   error. */
			syndrome(bp, &syndrome_bytes);
			if (syndrome_bytes.sb_s1 == (int)POSSIBLY_CORRECTABLE_DATA_ERROR &&
			    decoded_addr(bp, &syndrome_bytes.sb_tva) == err_rcrd &&
			    syndrome_bytes.sb_offset >= 0 &&
			    syndrome_bytes.sb_offset <= 125) {
				int cb_bufoff = syndrome_bytes.sb_offset << 1;
				int cb_index;
				
				/* we can assume whole sector transfers */
				for (cb_index = 0; cb_index < 6; cb_index++)
					iob->b_xaddr[cb_bufoff++] ^= syndrome_bytes.sb_correction_bytes[cb_index];

				err_rcrd++;  /* resume with the next sector */
				iob->b_errcnt = 0;  /* it's had no attempts */
				some_transferred = TRUE;
			}
		}
	} else {
		/*
		**  a partial retry is possible; this section only entered if:
		**  	1)  the error occurred beyond the starting sector
		**  	2)  this is a read operation
		**  Since err_rcrd may be reported as either 47 or 63 records
		**  beyond the actual point of the error if the error occurs
		**  during address verification of a new track, we need to
		**  place an upper limit on it based upon the transferred
		**  byte count.
		*/
		long max_err_rcrd = bp->b_un2.b_sectno +
			((iob->b_xcount - iob->iob_resid) >> bp->b_log2blk);

		if (err_rcrd > max_err_rcrd)
			err_rcrd = max_err_rcrd;
		iob->b_errcnt = 1;  /* the sector has had one attempt */
		some_transferred = TRUE;
	}

	if (some_transferred) {
		/* how much did we really get? */
		long possible_bytes = (err_rcrd - bp->b_un2.b_sectno)
						    << bp->b_log2blk;
		if (possible_bytes < iob->b_xcount)
			iob->b_xcount = possible_bytes;
	}
	break;

case ILLEGAL_DRIVE_TYPE:
case UNIT_UNAVAILABLE:
	bp->b_error = ENXIO;
	break;

case ATTEMPT_TO_WRITE_ON_PROTECTED_TRK:
	bp->b_error = EACCES;
	break;

case STATUS_2_ERROR:
	if (status_bytes.e)
		bp->b_error = EIO;
	else if (status_bytes.ss)
		bp->b_error = ENXIO;
	else if (!(bp->b_flags & B_READ) && status_bytes.w)
		bp->b_error = EACCES;
	else if (status_bytes.f) {
		iob->iob_current_mask = 0;
		++iob->b_errcnt;  /* guarantee we won't loop forever */
	} else
		bp->b_error = ENXIO;
	break;

default:
	bp->b_error = EIO;
	break;

		}  /* switch */

	return some_transferred;
}


/*
**  amigo_nop - control operation for synchronizing closes
*/
amigo_nop(bp)
struct buf *bp;
{
	queuedone(bp);
}


/*
**  dev_opened - if device already opened, return open device pointer
*/
static struct amigo_od *dev_opened(dev)
register dev_t dev;
{
	register struct amigo_od *p;

	dev &= M_DEVMASK;
	for (p = amigo_odt; p < amigo_odt + ODS; ++p)
		if (dev == p->device && p->open_count)
			return p;
	return NULL;
}


/*
**  dev_open - allocate a new open device entry and return a pointer to it
*/
static struct amigo_od *dev_open(dev)
dev_t dev;
{
	register struct amigo_od *p;
	register int i, j;

	if (m_selcode(dev) > 31 ||
	    m_busaddr(dev) > 7	||
	    m_unit(dev) > 7 	||
	    m_volume(dev) > 1 )
		return NULL;

	for (p = amigo_odt; p->open_count; )
		if (++p >= amigo_odt + ODS) {
#ifdef AMIGODEBUG
			printf("amigo_open: open device table overflow\n");
#endif
			return NULL;
		}
	p->device = dev & M_DEVMASK;
	p->dma_reserved = 0;
	for (i = 0; i < 8; ++i) {
		p->dk_index[i] = -1;
		for (j = 0; j < 2; ++j)
			p->unitvol[i][j] = (int)NOUNIT;
	}
	p->dev_queue.iob_current_mask = 0;
	return p;
}


/*
**  assign_dk_index - if possible, assign a dk_index for profiling disc activity
*/
static void assign_dk_index(dev, od_ptr)
dev_t dev;
struct amigo_od *od_ptr;
{
	register dev_t this_dk_devt = (dev & DK_DEVT_MASK) | DK_DEVT_MAJ;
	register char *index_ptr = &od_ptr->dk_index[m_unit(dev)];
	register int i;

	for (i = 0; i < DK_NDRIVE; i++)
		if (dk_devt[i] == this_dk_devt) {
			*index_ptr = i;
			return;  /* index already associated with this unit */
		}
	for (i = 0; i < DK_NDRIVE; i++)
		if (dk_devt[i] == 0) {
			dk_devt[i] = this_dk_devt;
			*index_ptr = i;
			return;  /* new index allocated */
		}
	*index_ptr = -1;
	return;  /* no index asociated */
}


/*
**  dev_close - decrement open_count
*/
static void dev_close(dev, od_ptr)
dev_t dev;
struct amigo_od *od_ptr;
{
	struct isc_table_type *sc = isc_table[m_selcode(dev)];

	if (od_ptr == NULL)
		panic("amigo_close: unopened device");
	if (od_ptr->dma_reserved) {
		dma_unreserve(sc);
		--od_ptr->dma_reserved;
	}
	dma_unactive(sc);
	if (--od_ptr->open_count < 0)
		panic("amigo_close: negative open count");
}


amigo_open(dev, flag)
dev_t dev;
{
	register struct amigo_od *od_ptr;
	register struct isc_table_type *sc;
	register int card_type;
	register unsigned char *unitvol_ptr;

	register int err = 0;
	short id;
	char dsj;
	struct status_type sts;
	register struct map_type *map;
	int dk_index;
	int HPIB_utility();

	/*
	**  if not already opened, then open
	*/
	if ((od_ptr = dev_opened(dev)) == NULL &&
	    (od_ptr = dev_open(dev)) == NULL )
		return ENXIO;

	/*  Now that we have a valid open device table pointer we
	**  have to check if the device was previously opened by
	**  mediainit(1). Mediainit(1) requires that the entire device
	**  be locked to assure synchronous access to the media being
	**  formatted/initialized. This is accomplished by setting the
	**  most significant bit of the open device iobuf's b_flags 
	**  location. See also amigo_close, and amigo_transfer.
	*/
	if( od_ptr->dev_queue.b_flags & IOB_LOCKED )
			return EBUSY;


	/*
	**  check for HP-IB card
	*/
	if ((sc = isc_table[m_selcode(dev)]) == NULL ||
	    (card_type = sc->card_type) != HP98625 &&
	    card_type != HP98624 &&
	    card_type != INTERNAL_HPIB)
		return ENXIO;

	/*
	**  before any possible sleeps during IO, mark the entry busy...
	**  so other opens can't try to claim...
	**  but from here on, dev_close must be called if the open fails
	*/
	++od_ptr->open_count;
	dma_active(sc);

	unitvol_ptr = &od_ptr->unitvol[m_unit(dev)][m_volume(dev)];

	/*
	**  perform an amigo identify
	*/
	if (err = amigo_control(HPIB_utility, sc->iosw->iod_ident,
					0, dev, (char *)&id, sizeof id, 0))
		goto errout;

	/*
	**  permit IDC disks if possible (good luck!)
	*/
	if (id == 0x0003)
		if (card_type == HP98625 && dma_here)
			id = 0x0002;
		else
			goto ENXIOout;

	/*
	**  match up with an amigo controller ident
	*/
	for (map = device_maps; map->ident != id; map++)
		if (map->cyl_per_med == 0)
			 goto ENXIOout;

	/*
	**  we have identified an amigo controller type; set the type field
	**  with what we know thus far, since we need the controller protocol
	**  flags correctly set to perform the status!
	**
	**  Caution!!!  if the controller type is already correct, LEAVE IT
	**    	THAT WAY, since the device may ALREADY BE OPEN, and in fact
	**	have active io transfers, in which case setting the device type
	**	here could possibly screw up those active io transfers!
	*/
	if (device_maps[*unitvol_ptr].ident != id)
		*unitvol_ptr = map - device_maps;

	/*
	**  get status; first dsj just removes any power-on holdoff
	*/
	if ((err = amigo_control(HPIB_utility, do_dsj,
			0, dev, (char *)&dsj, sizeof dsj, 0)) ||
	    (err = amigo_control(amigo_info, request_status,
			REQ_STATUS_TIME, dev, (char *)&sts, sizeof sts, 0)) ||
	    (err = amigo_control(HPIB_utility, do_dsj,
			0, dev, (char *)&dsj, sizeof dsj, 0)))
		goto errout;

	if (dsj || sts.ss) {
		err = EIO;
		goto errout;
	}

	/*
	**  for controllers supporting several models,
	**  find out which one we have here
	*/
	if (id == 0x0104)
		map = device_maps + (sts.r ? (int)HP9121 : (int)HP8290X);
	else if (map->model != NO_MODEL)
		for ( ; map->ident != id || map->model != sts.tttt; map++)
			if (map->cyl_per_med == 0)
				 goto ENXIOout;

	/*
	**  final volume number related push-ups
	*/
	if (map == device_maps + (int)HP7905_L ||
	    map == device_maps + (int)HP7905_U)
		map = device_maps + (int)(m_volume(dev) ? HP7905_L : HP7905_U);
	else if (m_volume(dev) != 0 && map != device_maps + (int)HP7906)
		goto ENXIOout;

	/*
	**  FINALLY, set the (now fine-tuned) device type and open flags
	*/
	*unitvol_ptr = map - device_maps;

	/*
	**  send the end command if needed
	*/
	if (map->flag & END_NEEDED)
		if (err = amigo_control(HPIB_utility, do_end,
							0, dev, NULL, 0, 0))
			goto errout;

	/*
	**  regarding MAC disc/simon interrupt level configuration problems:
	**  Don't allow the open since even the blocked device can't
	**  work reliably because of dma chaining.
	*/
	if (map->flag & SYNC_UNBUF &&
	    card_type == HP98625 && !(sc->state & F_SIMONB) && dma_here &&
	    ((*((char *)sc->card_ptr+3) >> 4) & 0x3) + 3 != 6) {
printf("\namigo_open: synchronous unbuffered disc\n", minor(dev));
printf("            The 98625A card interrupt level must be set to 6,\n");
printf("            or else a 98625B must be used instead.\n\n");
		goto ENXIOout;
	}

	/*
	**  if synchronous unbuffered, try to reserve dma
	*/
	if (map->flag & SYNC_UNBUF && card_type == HP98625)
		od_ptr->dma_reserved += dma_reserve(sc);

	/*
	**  if possible, associate a dk_index for profiling disc activity
	*/
	assign_dk_index(dev, od_ptr);

	/*
	**  set the static disc activity profiling parameters
	*/
	if ((dk_index = od_ptr->dk_index[m_unit(dev)]) >= 0)
		dk_mspw[dk_index] = 1.0 /
		    (map->rps * map->sec_per_trk * (1 << (map->log2blk - 1)));

	/* One last thing to do. If the device is an HP9895_IBM then set a flag
	** in the iobuf's b_flag field. This is req'd to start seeks from sector
	** 1 instead of sector 0 for IBM formatted single-sided 8" floppies
	*/
	if((map->ident==0x0081) && (map->model==8) && (map->sec_per_trk==26))
		od_ptr->dev_queue.b_flags |= IBM_FORMATTED;

	goto out;

ENXIOout:
	err = ENXIO;
errout:
	dev_close(dev, od_ptr);

out:
	return err;
}
	

amigo_close(dev)
dev_t dev;
{
register struct amigo_od *od_ptr;

	/*
	** Prior to closing the previously opened device, we have
	** to first find out if the entire unit was locked by the
	** AMIGO_LOCK_DEVICE ioctl for usage by mediainit(1). 
	** Check here, if the lock was set,  clear it! Also check
	** to see if the IBM_FORMATTED flag was set in the same field.
	*/
	od_ptr = dev_opened(dev);

	if( od_ptr->dev_queue.b_flags & IOB_LOCKED )
		od_ptr->dev_queue.b_flags &= (~IOB_LOCKED);

	if( od_ptr->dev_queue.b_flags & IBM_FORMATTED )
		od_ptr->dev_queue.b_flags &= (~IBM_FORMATTED);

	(void)amigo_control(amigo_nop, NULL, 0, dev, NULL, 0, 0);
	dev_close(dev, dev_opened(dev));
}


/*
**  amigo_minphys - adjust physio b_count requests to avoid surface crossings
*/
amigo_minphys(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;

	register struct amigo_od *od_ptr;
	register struct map_type *map;

	/*
	**  apply fundamental IO archetecture transfer length restriction
	*/
	minphys(bp,0);
	
	/*
	**  logic for 7905/6 only, so log2blk == 8 and SURF_SECTORS is fixed
	**
	**  its not kosher to cross a surface boundary on a surface mode
	**  disc, and in addition, that guarantees the longest possible
	**  seek.  Thus, its broken up here, and physio will have to go
	**  around again to get the rest of it.
	*/
	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("amigo_minphys: unopened device");
	
	map = &device_maps[od_ptr->unitvol[m_unit(dev)][m_volume(dev)]];
	if (map->flag & SURFACE_MODE) {
		register daddr_t sectno = bp->b_offset >> 8;
		register long surf_bytes;

		surf_bytes = (SURF_SECTORS - (sectno % SURF_SECTORS)) << 8;
		if (surf_bytes < bp->b_bcount)
			bp->b_bcount = surf_bytes;
	}

}


amigo_strategy(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;
	register struct amigo_od *od_ptr;
	register struct map_type *map;

	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("amigo_strategy: unopened device");

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_type = od_ptr->unitvol[m_unit(dev)][m_volume(dev)];
	map = &device_maps[bp->b_type];
	bp->b_log2blk = map->log2blk;
	bp->b_dk_index = od_ptr->dk_index[m_unit(dev)];

	/*
	**  check transfer parameters
	*/
	if (bpcheck(bp, records_per_medium(map), map->log2blk, 0))
		return;

	/*
	**  will this be buffered, unbuffered, or synchronous unbuffered?
	*/
	if (map->flag & SYNC_UNBUF)
		bp->b_flags |= od_ptr->dma_reserved ?
					B_SYNC_UNBUF : B_BUFFERED_TRANS;
	else if (map->flag & MUST_BUFFER)
		bp->b_flags |= B_BUFFERED_TRANS;

	/*
	**  surface mode logic for 7905/6 only!!!
	**
	**  requests shouldn't be crossing surface boundaries:
	**    .  amigo_minphys should have cut back physio requests 
	**    .  1K/2K/4K/8K file system blocks fall on surface boundaries
	**    .  USER must restrict swap area to lie on single surface!!!
	*/
	if (map->flag & SURFACE_MODE &&
	    bp->b_un2.b_sectno / SURF_SECTORS !=
	    (bp->b_un2.b_sectno + (bp->b_bcount + 255 >> 8) - 1) / SURF_SECTORS)
		panic("amigo_strategy: IO request crosses surface boundary");

	bp->b_action = amigo_transfer;

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

	enqueue(&od_ptr->dev_queue, bp);
}


amigo_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(amigo_strategy, NULL, dev, B_READ, amigo_minphys, uio);
}


amigo_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(amigo_strategy, NULL, dev, B_WRITE, amigo_minphys, uio);
}




/* 
**	This function is only used by the mediaint(1) code, ioctl's
*/
ioctl_issue_cmd(bp)
struct buf *bp;
{
	(*bp->b_sc->iosw->iod_mesg)(bp,
		((struct combined_buf *)(bp->b_un.b_addr))->mesg_type,
		((struct combined_buf *)(bp->b_un.b_addr))->cmd_sec,
		((struct combined_buf *)(bp->b_un.b_addr))->cmd_buf,
		((struct combined_buf *)(bp->b_un.b_addr))->cmd_nb);

	return bp->b_ioctl_flag; 	/* If 1 then ppoll, if 0 don't */
}




/*
** This function is only used by the mediainit(1) code, ioctl's
*/
ioctl_do_dsj(bp)
struct buf *bp;
{
	(*bp->b_sc->iosw->iod_mesg)(bp,
				     0,
				     SEC_DSJ,
				     (char *)bp->b_un.b_addr,
				     1);

	return bp->b_ioctl_flag;	/* If 1 then ppoll, if 0 don't */
}





/*
** ioctl capability, used by mediainit(1) for amigo devices. 
*/
extern suser();
extern HPIB_identify();

amigo_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
#define	NO_CLOCK_TICKS		0
#define	NO_PPOLL		0
#define YES_DSJ			1
#define DSJ_ERROR		9	/* bogus dsj value	     */

char dsj = DSJ_ERROR;			/* Initially assume an error */
register int err;
register struct amigo_od *od_ptr;
register struct map_type *map;
char dummy = 0;
struct amigo_identify hpib_id;
struct io_hpib_ident *tmp_ptr;

   if(( od_ptr = dev_opened(dev)) == NULL)
	panic("amigo_ioctl: unopened device");


	switch(order)
	{

	case AMIGO_LOCK_DEVICE:
		if(!suser){
			err = EPERM;
			goto err_out;
		}

		/*
		** If the device already has a valid open in affect, then
		** return an error. (e.g. the device is busy)
		*/
		if( od_ptr->open_count != 1 ){
			err = EBUSY;
			goto err_out;
		}

		/* If already locked then exit, else lock it */
		err = (od_ptr->dev_queue.b_flags & IOB_LOCKED ) ? EBUSY : 0;

		if(!err)
			od_ptr->dev_queue.b_flags |= IOB_LOCKED;

		break;


	/* Info from user to driver, addr.element(s) placed on kernel stack */
	/* 1st verify the device is locked before proceeding	            */
	case AMIGO_IOCMD_IN:
		/*
		** In this case the device must be locked, else return an error
		*/
		err=(od_ptr->dev_queue.b_flags & IOB_LOCKED) ? 0 : EPERM;
		if(err)
			goto err_out;

	/*
	 * This ioctl is used by ioscan to read the hpib identify
	 * bytes from a device.
	 * If the identify worked then copy the id
	 * data.  For the moment lets default the dev_type
	 * to a disk
	 */
	case IO_HPIB_IDENT:
		tmp_ptr = (struct io_hpib_ident *)addr;
		err = amigo_control(HPIB_identify,
			       0,
			       NO_CLOCK_TICKS,
			       dev,
			       &hpib_id,
			       sizeof(struct amigo_identify),
			       NO_PPOLL);
	       if( !err )
	       {
		    tmp_ptr->ident = hpib_id.id;
		    tmp_ptr->dev_type = IOSCAN_HPIB_FIXED_DISK;
	       }
	       break;
	case AMIGO_IDENTIFY:
		/*
		** Even if the identify command fails, the amigo_close routine
		** will handle the problem of clearing the iobuf lock that
		** was previously established by the AMIGO_LOCK_DEVICE ioctl
		*/
		if( order == AMIGO_IDENTIFY )
		{
			err = amigo_control(HPIB_identify,
				       0,
				       NO_CLOCK_TICKS,
				       dev,
				       addr,
				       sizeof(struct amigo_identify),
				       NO_PPOLL);
			goto err_out;
		}

		/* Handle the case of order = AMIGO_IOCMD_IN */
		if( err = amigo_control(HPIB_utility,
			  ioctl_issue_cmd,
			  ((struct combined_buf *)addr)->timeout_val,
			  dev,
			  addr,
			  sizeof(struct combined_buf), 
			  ((struct combined_buf *)addr)->ppoll_reqd ) < 0)
				goto err_out;


		if( ((struct combined_buf *)addr)->dsj_reqd == YES_DSJ )
			amigo_control(HPIB_utility,
					ioctl_do_dsj,
					NO_CLOCK_TICKS,
					dev,
					&dsj,
					sizeof dsj,
					NO_PPOLL);

		else
			dsj = 0;	/* assume no err, discover next time */


		err = ( dsj == 0 ) ? 0  : -1 ; 
		break;



	default: /* SVVS states that an unknown ioctl return EINVAL */
		err = EINVAL;
		break;


	}	/* end switch */


err_out:

   return err;

}	/* end amigo_ioctl */






/*
**  this is a utility to set up for activities other than read/write.
**  It is used internally for open activities, and also by ioctl 
**  It knows amigo specific things.
*/
amigo_control(fsm, action2, clock_ticks, dev, bfr, len, flag)
int (*fsm)();
int (*action2)();
int clock_ticks;
dev_t dev;
char *bfr;
int len;
int flag;
{
	register struct amigo_od *od_ptr;
	register struct buf *bp;
	long saved_bcount;
	
	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("amigo_control: unopened device");

	bp = geteblk(len);
	saved_bcount = bp->b_bcount;

	if (bfr != NULL)
		bcopy(bfr, bp->b_un.b_addr, len);

	bp->b_bcount = len;
	bp->b_error = 0;
	bp->b_dev = dev;

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_type = od_ptr->unitvol[m_unit(bp->b_dev)][m_volume(bp->b_dev)];
	bp->b_action = fsm;
	bp->b_action2 = action2;
	bp->b_clock_ticks = clock_ticks;
	bp->b_ioctl_flag = flag > 0;

	enqueue(&od_ptr->dev_queue, bp);
	iowait(bp);

	if (bfr != NULL)
		bcopy(bp->b_un.b_addr, bfr, len);

	bp->b_bcount = saved_bcount;
	brelse(bp);
	return geterror(bp);
}





int (*amigo_saved_dev_init)();

/*
**  one-time initialization code
*/
amigo_init()
{
	/*
	**  set rootdev if not already set and boot device was amigo
	*/
	if (rootdev < 0 && msus.device_type >= 7 && msus.device_type <= 13) {
		register int maj;
		extern struct bdevsw bdevsw[];
		/*
		**  determine the amigo blocked driver major number
		*/
		for (maj = 0; bdevsw[maj].d_open != amigo_open; )
			if (++maj >= nblkdev)
				panic("amigo_init: amigo_open not in bdevsw");
		/*
		**  construct a dev from the boot ROM's MSUS
		*/
		rootdev = makedev(maj, makeminor(msus.sc, msus.ba,
							msus.unit, msus.vol));
		}
	/*
	**  call the next init procedure in the chain
	*/
	(*amigo_saved_dev_init)();
}


struct msus (*amigo_saved_msus_for_boot)();

/*
**  code for converting a dev_t to a boot ROM msus
*/
struct msus amigo_msus_for_boot(blocked, dev)
char blocked;
dev_t dev;
{
	extern struct bdevsw bdevsw[];
	extern struct cdevsw cdevsw[];

	int maj = major(dev);
	struct amigo_od *od_ptr;
	int type;
	struct msus my_msus;

	static char dev_type[] = {
		/* HP7906	*/	11,
		/* HP7905_L	*/	10,
		/* HP7905_U	*/	10,
		/* HP7920	*/	12,
		/* HP7925	*/	13,
		/* HP9895_SS	*/	 4,
		/* HP9895_DS	*/	 4,
		/* HP9895_IBM	*/	 4,
		/* HP8290X	*/	 5,
		/* HP9121	*/	 5,
		/* HP913X_A	*/	 7,
		/* HP913X_B	*/	 8,
		/* HP913X_C	*/	 9,
		/* NOUNIT	*/	31,
	};

	/*
	**  does this driver not handle the specified device?
	*/
	if (( blocked && (maj >= nblkdev || bdevsw[maj].d_open != amigo_open))||
	    (!blocked && (maj >= nchrdev || cdevsw[maj].d_open != amigo_open)))
		return (*amigo_saved_msus_for_boot)(blocked, dev);

	/*
	**  what's the media's block size?
	*/
	if (amigo_open(dev, FREAD) || !(od_ptr = dev_opened(dev)))
		type = (int)NOUNIT;
	else {
		type = od_ptr->unitvol[m_unit(dev)][m_volume(dev)];
		amigo_close(dev);
	}

	/*
	** construct the boot ROM msus
	*/
	my_msus.dir_format = 0;	/* assume LIF */
	my_msus.device_type = dev_type[type];
	my_msus.sc = m_selcode(dev);
	my_msus.ba = m_busaddr(dev);
	my_msus.unit = m_unit(dev);
	my_msus.vol = m_volume(dev);
	return my_msus;
}


/*
**  one-time linking code
*/
amigo_link()
{
	extern int (*dev_init)();
	extern struct msus (*msus_for_boot)();

	amigo_saved_dev_init = dev_init;
	dev_init = amigo_init;

	amigo_saved_msus_for_boot = msus_for_boot;
	msus_for_boot = amigo_msus_for_boot;

#ifdef IOSCAN
        hpib_config_ident = amigo_config_identify;
#endif /* IOSCAN */
}


amigo_size(dev)
dev_t dev;
{
	struct amigo_od *od_ptr;
	int type, bytes;
		
	if (amigo_open(dev, FREAD) || !(od_ptr = dev_opened(dev)))
		return 0;
	type = od_ptr->unitvol[m_unit(dev)][m_volume(dev)];
	bytes = records_per_medium(&device_maps[type]) << 8;
	amigo_close(dev);
	return bytes >> DEV_BSHIFT;
}

#ifdef IOSCAN

amigo_config_identify(sinfo)
register struct ioctl_ident_hpib *sinfo;
{ 
    int HPIB_utility();

    register struct amigo_od *od_ptr;
    register struct isc_table_type *isc;
    short id;
    dev_t sdev, bdev;
    int busaddr, err;

    if ((isc = isc_table[sinfo->sc]) == NULL ||
            isc->card_type != HP98625 &&
            isc->card_type != HP98624 &&
            isc->card_type != INTERNAL_HPIB)
                return ENODEV;

    bdev = 0x04000000 | (sinfo->sc << 16);

    for (busaddr = 0; busaddr < 8; busaddr++) {

        sdev = bdev | (busaddr << 8);

        if ((od_ptr = dev_opened(sdev)) == NULL &&
            (od_ptr = dev_open(sdev)) == NULL )
                return ENXIO;
	++od_ptr->open_count;

        /*
        **  perform an amigo identify
        */
        err = amigo_control(HPIB_utility, isc->iosw->iod_ident,
                                        0, sdev, (char *)&id, sizeof id, 0);

        if (err) {
            /* don't call dev_close, it does a dma_unactive, we don't want to do 
               the active above that open does */
            --od_ptr->open_count;
            sinfo->dev_info[busaddr].ident = -1;
            continue;
        }

        sinfo->dev_info[busaddr].ident = id;

        /* don't call dev_close, it does a dma_unactive, we don't want to do 
           the active above that open does */
        --od_ptr->open_count;
    }
    return 0;
}
#endif /* IOSCAN */
