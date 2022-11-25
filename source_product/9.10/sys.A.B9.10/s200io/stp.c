/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/stp.c,v $
 * $Revision: 1.8.84.11 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 10:14:50 $
 */
/* HPUX_ID: @(#)stp.c	55.1		88/12/23 */

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
#include "../wsio/iobuf.h"
#include "../h/systm.h"
#include "../wsio/hpibio.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../wsio/tryrec.h"
#include "../h/uio.h"

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

int stp_config_identify();

extern int (*hpib_config_ident)();
#endif /* IOSCAN */

/*
** mnemonic names for buf scratch registers
*/
#define b_opcode 	b_s0	/* (char) */
#define b_TO_state	b_s1	/* (char) */
#define b_od_ptr 	b_s2	/* (long) used as (struct stp_od *) */

#define OD_PTR(bp) ((struct stp_od *)(bp->b_od_ptr)) /* for proper casting */

/*
** mnemonic names for bits in buf's b_flags
*/
#define B_ABORTING	B_SCRACH1
#define B_PPOLL_TIMEOUT	B_SCRACH2
#define B_DATA_TRANSFER	B_SCRACH3
#define B_RECOVERING	B_SCRACH4

/*
** mnemonic names for bits in iobuf's b_flags
*/
#define IOB_WRITTEN	B_SCRACH1
#define IOB_EOF		B_SCRACH2
#define IOB_BEYOND_EOT	B_SCRACH3
#define CHECK_DENSITY	B_SCRACH4
#define DDS		B_SCRACH5
#define DDS_ACK_SSM	B_SCRACH6

/*
** amigo identify bytes
*/
#define HP7974_IDENT	0x0174		/* Antelope */
#define HP7978_IDENT	0x0178		/* Buckhorn */
#define HP7979_IDENT	0x0179		/* Springbok (Auto-loading) */
#define HP7980_IDENT	0x0180		/* Gnu (Auto-loading) */
#define HPC1501_IDENT	0x0190          /* Foxbox (DDS) */

/*
** listen secondaries
*/
#define LS_WRITE_EXECUTE		(SCG_BASE + 0)
#define LS_TAPE_COMMAND			(SCG_BASE + 1)
#define LS_DOWNLOAD_DIAGNOSTIC		(SCG_BASE + 4)
#define LS_WRITE_FIRMWARE_UPDATE	(SCG_BASE + 6)
#define LS_END_COMMAND			(SCG_BASE + 7)
#define LS_AMIGO_DEVICE_CLEAR		(SCG_BASE + 16)
#define LS_CLEAR_CRC			(SCG_BASE + 17)
#define LS_SEND_UTILITY			(SCG_BASE + 28)
#define LS_WRITE_INTERFACE_LOOPBACK	(SCG_BASE + 30)
#define LS_RUN_SELF_TEST		(SCG_BASE + 31)

/*
** talk secondaries
*/
#define TS_READ_EXECUTE			(SCG_BASE + 0)
#define TS_READ_STATUS			(SCG_BASE + 1)
#define TS_READ_BYTE_COUNT		(SCG_BASE + 2)
#define TS_READ_DIAGNOSTIC_RESULTS	(SCG_BASE + 3)
#define TS_READ_MONITORING_LOG		(SCG_BASE + 5)
#define TS_READ_FIRMWARE_UPDATE		(SCG_BASE + 6)
#define TS_READ_DSJ			(SCG_BASE + 16)
#define TS_READ_CRC			(SCG_BASE + 17)
#define LS_READ_UTILITY			(SCG_BASE + 28)
#define TS_READ_INTERFACE_LOOPBACK	(SCG_BASE + 30)
#define TS_READ_SELF_TEST_STATUS	(SCG_BASE + 31)

/*
** tape commands
*/
enum tape_command {
	CMD_SELECT_UNIT_0,		CMD_COLD_LOAD_UNIT_0_SELECT,
	CMD_2,				CMD_SPACE,
	CMD_WRITE_SETMARK,		CMD_WRITE_RECORD,
	CMD_WRITE_FILE_MARK,		CMD_WRITE_GAP,
	CMD_READ_RECORD,		CMD_FORWARD_SPACE_RECORD,
	CMD_BACKSPACE_RECORD,		CMD_FORWARD_SPACE_FILE,
	CMD_BACKSPACE_FILE,		CMD_REWIND,
	CMD_REWIND_AND_GO_OFFLINE,	CMD_WRITE_GCR_COMPRESSED,
	CMD_WRITE_GCR_FORMAT,		CMD_WRITE_PE_FORMAT,
	CMD_WRITE_NRZI_FORMAT,		CMD_19,
	CMD_START_STOP_ONLY,		CMD_ENABLE_STREAMING,
	CMD_DISABLE_IMMEDIATE_REPORT,	CMD_ENABLE_IMMEDIATE_REPORT,
	CMD_REQUEST_STATUS
};

/*
** END commands / service request register
*/
#define END_COMPLETE	0x08
#define END_IDLE	0x04
#define END_DATA	0x02

/*
** status register bits
*/
#define SR_EOF			0x80000000	/* status register 1 bits */
#define SR_BOT			0x40000000
#define SR_BEYOND_EOT		0x20000000
#define SR_RECOVERED		0x10000000
#define SR_COMMAND_REJECT	0x08000000
#define SR_WRITE_PROTECT	0x04000000
#define SR_UNRECOVERED		0x02000000
#define SR_ON_LINE		0x01000000

#define SR_GCR_DENSITY		0x00800000	/* status register 2 bits */
#define SR_UNKNOWN_DENSITY	0x00400000
#define SR_DATA_PARITY		0x00200000
#define SR_DATA_TIMING		0x00100000
#define SR_TAPE_RUNAWAY		0x00080000
#define SR_DOOR_OPEN		0x00040000
#define SR_2_2			0x00020000
#define SR_IMMEDIATE		0x00010000

#define SR_PE_DENSITY		0x00008000	/* status register 3 bits */
#define SR_NRZI_DENSITY		0x00004000
#define SR_POWERUP		0x00002000
#define SR_COMMAND_PARITY	0x00001000
#define SR_POSITION		0x00000800
#define SR_FORMATTER		0x00000400
#define SR_SERVO		0x00000200
#define SR_CONTROLLER		0x00000100

/*
** fatal EIO errors
*/
#define SR_EIO_ERRORS	( \
			SR_COMMAND_REJECT | \
			SR_UNRECOVERED | \
			SR_DATA_PARITY | \
			SR_DATA_TIMING | \
			SR_TAPE_RUNAWAY | \
			SR_DOOR_OPEN | \
			SR_COMMAND_PARITY | \
			SR_POSITION | \
			SR_FORMATTER | \
			SR_SERVO | \
			SR_CONTROLLER \
			)

/*
** device status
*/
union stp_status {
	long sr_bits;	/* 32 bits from status registers 1-4 */
	struct {
		unsigned sr1: 8;		/* register 1 */
		unsigned sr2: 8;		/* register 2 */
		unsigned sr3: 8;		/* register 3 */
		unsigned cre_detail: 3;		/* part of register 4 */
		unsigned retry_count: 5;	/* part of register 4 */
		unsigned sr5: 8;		/* register 5 */
		unsigned sr6: 8;		/* register 6 */
	} sr_regs;
	struct {
		unsigned dsreg1: 8;		/* "status register 1" */
		unsigned dsreg2: 16;		/* "status register 2" */
		unsigned erreg: 24;		/* "error register" */
	} sr_mtget;
};

/*
**  open device table
*/
#define	ODS	4			/* number of open device entries */

struct stp_od {
	dev_t			od_device;	/* device */
	long			od_resid;	/* most recent resid */
	unsigned short		od_ident;	/* most recent identify */
	union stp_status	od_status;	/* most recent status */
	unsigned short		od_count;	/* most recent count */
	unsigned char		od_dsj;		/* most recent dsj */
	struct iobuf		od_iobuf;	/* device queue */
} stp_odt[ODS];

/*
** pseudo operations so common routines can support both read/write and ioctl
**
** *** NOTE *** assumes MTBSS is the last user ioctl!
**              MTread & MTwrite must be the last pseudo op's!
*/
#define MTstat	MTNOP		/* do a stand-alone status */
#define MTgcr	MTBSS+1		/* write GCR (6250 bpi) format header */
#define MTpe	MTBSS+2		/* write PE (1600 bpi) format header */
#define MTnrzi	MTBSS+3		/* write NRZI (800 bpi) format header */
#define MTcompr	MTBSS+4		/* write GCR (6250 bpi) compressed (GANDALF) */
#define MTdisST	MTBSS+5		/* disable streaming mode */
#define MTenST	MTBSS+6		/* enable streaming mode */
#define MTdisIR	MTBSS+7		/* disable immediate report mode */
#define MTenIR	MTBSS+8		/* enable immediate report mode */
#define MTread	MTBSS+9		/* read one record */
#define MTwrite	MTBSS+10	/* write one record */

#define MTWRITE_UTIL	0	/* write utility psuedo tape command */

/*
** table for translating UNIX operations to device command register operations
**
** *** NOTE *** the array assumes it knows the order of UNIX MT* operations!
*/
enum tape_command stp_command_table[] = {
  CMD_WRITE_FILE_MARK,		/* MTWEOF   0 write an end-of-file record */
  CMD_FORWARD_SPACE_FILE,	/* MTFSF    1 forward space file */
  CMD_BACKSPACE_FILE,		/* MTBSF    2 backward space file */
  CMD_FORWARD_SPACE_RECORD,	/* MTFSR    3 forward space record */
  CMD_BACKSPACE_RECORD,		/* MTBSR    4 backward space record */
  CMD_REWIND,			/* MTREW    5 rewind */
  CMD_REWIND_AND_GO_OFFLINE,	/* MTOFFL   6 rewind and put drive offline */
  CMD_REQUEST_STATUS,		/* MTstat   7 wait pending writes, set status */
  CMD_SPACE,			/* MTEOD    8 (DDS only) seek to EOD point*/
  CMD_WRITE_SETMARK,		/* MTWSS    9 (DDS only) write save_setmarks*/
  CMD_SPACE,			/* MTFSS   10 (DDS only) forward space SMks*/
  CMD_SPACE,			/* MTBSS   11 (DDS only) backward space SMks*/
  CMD_WRITE_GCR_FORMAT,		/* MTgcr   12 */
  CMD_WRITE_PE_FORMAT,		/* MTpe    13 */
  CMD_WRITE_NRZI_FORMAT,	/* MTnrzi  14 */
  CMD_WRITE_GCR_COMPRESSED,	/* MTcompr 15 GCR compressed format */
  CMD_START_STOP_ONLY,		/* MTdisST 16 */
  CMD_ENABLE_STREAMING,		/* MTenST  17 */
  CMD_DISABLE_IMMEDIATE_REPORT,	/* MTdisIR 18 */
  CMD_ENABLE_IMMEDIATE_REPORT,	/* MTenIR  19 */
  CMD_READ_RECORD,		/* MTread  20 */
  CMD_WRITE_RECORD		/* MTwrite 21 */
};

/*
** table specifying which commands get updated status regardless of their dsj
**
** *** NOTE *** the array assumes it knows the order of UNIX MT* operations!
*/
char stp_force_status_table[] = {
  FALSE,	/* MTWEOF   0 write an end-of-file record */
  FALSE,	/* MTFSF    1 forward space file */
  FALSE,	/* MTBSF    2 backward space file */
  FALSE,	/* MTFSR    3 forward space record */
  FALSE,	/* MTBSR    4 backward space record */
  FALSE,	/* MTREW    5 rewind */
  FALSE,	/* MTOFFL   6 rewind and put drive offline */
  TRUE,		/* MTstat   7 wait pending writes, set status */
  FALSE,	/* MTEOD    8 (DDS only) seek to EOD point*/
  FALSE,	/* MTWSS    9 (DDS only) write save_setmarks*/
  FALSE,	/* MTFSS   10 (DDS only) forward space SMks*/
  FALSE,	/* MTBSS   11 (DDS only) backward space SMks*/
  TRUE,		/* MTgcr   12 */
  TRUE,		/* MTpe    13 */
  TRUE,		/* MTnrzi  14 */
  TRUE,		/* MTcompr 15 */
  FALSE,	/* MTdisST 16 */
  FALSE,	/* MTenST  17 */
  FALSE,	/* MTdisIR 18 */
  FALSE,	/* MTenIR  19 */
  FALSE,	/* MTread  20 */
  FALSE		/* MTwrite 21 */
};

/*
** timeout parameters
**
** *** NOTE *** the array assumes it knows the order of UNIX MT* operations!
*/
#define SHORT_MSG_TIME		((   750 * HZ) / 1000)
#define DATA_TRANSFER_TIME	(( 10000 * HZ) / 1000)

int stp_operation_ticks[] = {
	 60 * HZ,	/* MTWEOF   0 */
	720 * HZ,	/* MTFSF    1 */
	720 * HZ,	/* MTBSF    2 */
	 60 * HZ,	/* MTFSR    3 */
	 60 * HZ,	/* MTBSR    4 */
	360 * HZ,	/* MTREW    5 */
	360 * HZ,	/* MTOFFL   6 */
	 60 * HZ,	/* MTstat   7 */
  	360 * HZ,	/* MTEOD    8 (DDS only) */
  	 60 * HZ,	/* MTWSS    9 (DDS only) */
  	720 * HZ,	/* MTFSS   10 (DDS only) */
  	720 * HZ,	/* MTBSS   11 (DDS only) */
  	 60 * HZ,	/* MTgcr   12 */
  	 60 * HZ,	/* MTpe    13 */
  	 60 * HZ,	/* MTnrzi  14 */
	 60 * HZ,	/* MTcompr 15 */
  	 60 * HZ,	/* MTdisST 16 */
	 /* "Enabling Streaming" and "Enabling Immediate" Report takes a 
	  * path similar to "status" and should have identical timeouts
	  */
  	 60 * HZ,	/* MTenST  17 */
  	 60 * HZ,	/* MTdisIR 18 */
  	 60 * HZ,	/* MTenIR  19 */
	 60 * HZ,	/* MTread  20 */
	 60 * HZ	/* MTwrite 21 */
};


#define NO_PARM 256	/* for those tape commands needing no parameter */

stp_configure_dds(bp)
struct buf *bp;
{
	int i, type, util, count;
	unsigned char utl_buffer[10];

	utl_buffer[0] = 2;
	type = T_WRITE + T_EOI;
	util = LS_SEND_UTILITY;

	utl_buffer[1] = 83;
	utl_buffer[2] =  1;
	for (i=3; i<10; i++)
    	    utl_buffer[i] = 0;
	count = 10;

	(*bp->b_sc->iosw->iod_mesg)(bp, type, util, utl_buffer, count);
}

stp_send_tape_cmd(bp, cmd, parm)
struct buf *bp;
enum tape_command cmd;
int parm;
{
	register int message_length = (parm == NO_PARM) ? 1 : 2;
	register int op = bp->b_opcode;
	unsigned char cmd_buffer[8];

	cmd_buffer[0] = (int)cmd;

	if (op == MTEOD || op == MTFSS || op == MTBSS )	{
	    int len = 1;
	    /* SPACE command: 4 bytes for command parms */
	    message_length = 5;
	    /* Set ENTITY CODE */
	    if (op == MTEOD)
		cmd_buffer[1] = 0x8;
	    else if (op == MTFSS)
		cmd_buffer[1] = 0x4;
	    else if (op == MTBSS)
		cmd_buffer[1] = 0x4;

	    /* Send byte count: either +1 or -1 */
	    if (op == MTBSS)
		len = -1;
	    cmd_buffer[2] = 0xff & len>>16;
	    cmd_buffer[3] = 0xff & len>>8;
	    cmd_buffer[4] = 0xff & len;
	    }
	else
	    cmd_buffer[1] = parm;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE + T_EOI, LS_TAPE_COMMAND,
						cmd_buffer, message_length);
}


stp_send_end_cmd(bp, parm)
struct buf *bp;
int parm;
{
	char cmd_buffer = parm;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE + T_EOI, LS_END_COMMAND,
						&cmd_buffer, sizeof cmd_buffer);
}


stp_read_dsj(bp)
struct buf *bp;
{
	(*bp->b_sc->iosw->iod_mesg)(bp, 0, TS_READ_DSJ,
				&OD_PTR(bp)->od_dsj, sizeof(unsigned char));
}


stp_read_status(bp)
struct buf *bp;
{
	struct stp_od *od_ptr = OD_PTR(bp);
	register union stp_status *stat = &od_ptr->od_status;
	register int flags = stat->sr_bits;

	static char *op_name[] = {
		"MTWEOF",
		"MTFSF",
		"MTBSF",
		"MTFSR",
		"MTBSR",
		"MTREW",
		"MTOFFL",
		"MTstat",
  		"MTEOD",
  		"MTWSS",
  		"MTFSS",
  		"MTBSS",
		"MTgcr",
		"MTpe",
		"MTnrzi",
		"MTcompr",
		"MTdisST",
		"MTenST",
		"MTdisIR",
		"MTenIR",
		"MTread",
		"MTwrite"
	};

	static char *bit_name[] = {
		"EOF",
		"BOT",
		"BEYOND_EOT",
		"RECOVERED",
		"COMMAND_REJECT",
		"WRITE_PROTECT",
		"UNRECOVERED",
		"ON_LINE",
		"GCR_DENSITY",
		"UNKNOWN_DENSITY",
		"DATA_PARITY",
		"DATA_TIMING",
		"TAPE_RUNAWAY",
		"DOOR_OPEN",
		"2_2",
		"IMMEDIATE",
		"PE_DENSITY",
		"NRZI_DENSITY",
		"POWERUP",
		"COMMAND_PARITY",
		"POSITION",
		"FORMATTER",
		"SERVO",
		"CONTROLLER"
	};
	char **p;

	(*bp->b_sc->iosw->iod_mesg)(bp, 0, TS_READ_STATUS,
			&OD_PTR(bp)->od_status, sizeof(struct stp_status));

	if (od_ptr->od_iobuf.b_flags & DDS_ACK_SSM)
		goto skip_diag;

/* Don't allow these bits to report errors */
#define NON_ERROR	0x1A7C3FFF

	if(od_ptr->od_dsj && stat->sr_bits & NON_ERROR)	{
	    /* If only error is EOD don't print diagnostics */
	    if (od_ptr->od_iobuf.b_flags & DDS && 
			!(stat->sr_bits & (SR_EIO_ERRORS & ~SR_TAPE_RUNAWAY)))
		return 0;
	    msg_printf("stp: dev 0x%x; ", bp->b_dev); 
	    msg_printf("op %s; ", op_name[bp->b_opcode]);
	    msg_printf("dsj %d; status SR1 0x%x, SR2 0x%x, SR3 0x%x, cmd reject %d, retry cnt %d, SR5 %d, SR6 %d\n",
		od_ptr->od_dsj,
		stat->sr_regs.sr1, stat->sr_regs.sr2, stat->sr_regs.sr3,
		stat->sr_regs.cre_detail, stat->sr_regs.retry_count,
		stat->sr_regs.sr5, stat->sr_regs.sr6);

	    msg_printf("  bits set:");
	    for (p = bit_name; p < bit_name + 24; p++) {
		if (flags & 0x80000000)
			msg_printf(" %s", *p);
		flags <<= 1;
		}
	    msg_printf("\n");
	    }
skip_diag:
	return(0);
}


stp_read_byte_count(bp)
struct buf *bp;
{
	(*bp->b_sc->iosw->iod_mesg)(bp, 0, TS_READ_BYTE_COUNT,
				&OD_PTR(bp)->od_count, sizeof(unsigned short));
}


stp_x_timeout(bp)
struct buf *bp;
{
	stp_fsm(bp);
	do ; while (selcode_dequeue(bp->b_sc) | dma_dequeue());
}


void stp_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,stp_x_timeout,bp->b_sc->int_lvl,0,bp->b_TO_state);
#ifdef STPDEBUG
	msg_printf("stp: tape timed out\n");
#endif
}


stp_fsm(bp)
register struct buf *bp;
{
	enum states {
		initial,
		send_command,
		ppoll_setup,
		ppoll_timeout,
		ppoll_done,
		status_check,
		operation_complete,
		start_data_transfer,
		data_transfer_done,
		timedout,
		start_clear,
		send_clear,
		undefined
	};

	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct stp_od *od_ptr = OD_PTR(bp);
	register int op = bp->b_opcode;

retry:	
	try
		START_FSM;
reswitch:	

		switch((enum states)iob->b_state) {

/* <<<<< entire switch block shifted left to allow more room <<<<< */

case initial:
	if (op == MTwrite && iob->b_flags & IOB_BEYOND_EOT) {
		bp->b_error = ENOSPC;
		iob->b_state = (int)operation_complete;
		goto reswitch;
	}

	if (op != MTstat)
		iob->b_flags &= ~IOB_WRITTEN;
	if (op == MTwrite)
		iob->b_flags |= IOB_WRITTEN;
	bp->b_flags &= ~B_ABORTING;

	iob->b_state = (int)send_command;
	get_selcode(bp, stp_fsm);
	break;

case send_command:
	bp->b_flags &= ~(B_DATA_TRANSFER | B_RECOVERING);

	bp->b_TO_state = (int)timedout;
	START_TIME(stp_timeout, SHORT_MSG_TIME);
	/* stp_send_utility is special for configuring DDS drives ONLY */
	if (od_ptr->od_iobuf.b_flags & DDS_ACK_SSM)
		stp_configure_dds(bp);
	else
	stp_send_tape_cmd(bp, stp_command_table[op],
			    op == MTwrite ? (bp->b_bcount - 1) >> 8 : NO_PARM);
	END_TIME;

	if (op != MTstat)
		iob->b_flags &= ~IOB_EOF;
	/* fall through */

case ppoll_setup:
	bp->b_flags &= ~B_PPOLL_TIMEOUT;

	if (HPIB_test_ppoll(bp, 1)) {
		iob->b_state = (int)status_check;
		goto reswitch;
	}

	bp->b_TO_state = (int)ppoll_timeout;
	START_TIME(stp_timeout, stp_operation_ticks[op]);

	iob->b_state = (int)ppoll_done;
	HPIB_ppoll_drop_sc(bp, stp_fsm, 1);
	break;

case ppoll_timeout:
	HPIB_ppoll_clear(bp);
	bp->b_flags |= B_PPOLL_TIMEOUT;
	/* fall through; sc may have been busy; check below */

case ppoll_done:
	END_TIME;
	iob->b_state = (int)status_check;
	get_selcode(bp, stp_fsm);
	break;

case status_check:
	if (bp->b_flags & B_PPOLL_TIMEOUT)
		if (HPIB_test_ppoll(bp, 1))
			bp->b_flags &= ~B_PPOLL_TIMEOUT;
		else
			escape(TIMED_OUT);

	bp->b_TO_state = (int)timedout;
	START_TIME(stp_timeout, 4 * SHORT_MSG_TIME);

	stp_read_dsj(bp);

	iob->b_flags &= ~IOB_BEYOND_EOT;

	if (od_ptr->od_dsj ||
	    stp_force_status_table[op] ||
	    bp->b_flags & B_RECOVERING) {
		register long sr_bits;

		stp_read_status(bp);

		sr_bits = od_ptr->od_status.sr_bits;

		if (sr_bits & SR_BEYOND_EOT) {
			iob->b_flags |= IOB_BEYOND_EOT;
			if (bp->b_dev & MT_COMPAT_MODE &&
			    op >= MTread &&
			    !bp->b_error)
				bp->b_error = ENOSPC;
		}

		if (sr_bits & SR_EOF) {
			iob->b_flags |= IOB_EOF;
			if (op == MTread && od_ptr->od_dsj != 2)
				bp->b_flags |= B_ABORTING;
		}

		if (sr_bits & SR_EIO_ERRORS) {
			/* If tape is a DDS and only EIO_ERROR bit set is
			 * tape runaway, then we're at EOD.  Non-fatal.
			 */
			if (od_ptr->od_iobuf.b_flags & DDS && 
			      !(sr_bits & (SR_EIO_ERRORS & ~SR_TAPE_RUNAWAY))) {
				iob->b_flags |= IOB_EOF;
				if (op == MTread && od_ptr->od_dsj != 2)
					bp->b_flags |= B_ABORTING;
			} else {
			if (!bp->b_error)
				bp->b_error = EIO;
			if (od_ptr->od_dsj != 2)
				bp->b_flags |= B_ABORTING;
			}
		}

		/*
		** ones still to think about
		**
		SR_POWERUP
		*/
	}

	if (bp->b_flags & B_DATA_TRANSFER)
		if (op == MTread)
			/* Get byte count after READ operations */
			stp_read_byte_count(bp);
		else
			od_ptr->od_count = bp->b_bcount;

	if (bp->b_flags & (B_DATA_TRANSFER | B_ABORTING) ||
	    op < MTread ||
	    od_ptr->od_dsj == 2)
		stp_send_end_cmd(bp, END_COMPLETE);

	END_TIME;

	if (od_ptr->od_dsj == 2) {
		iob->b_state = (int)ppoll_setup;
		goto reswitch;
	}

	if (!(bp->b_flags & B_ABORTING))
		if (op < MTread) {
			if (--bp->b_resid) {
				iob->b_state = (int)send_command;
				goto reswitch;
			}
		} else if (!(bp->b_flags & B_DATA_TRANSFER)) {
			bp->b_flags |= B_DATA_TRANSFER;
			iob->b_state = (int)start_data_transfer;
			goto reswitch;
		} else if (iob->b_flags & IOB_BEYOND_EOT &&
			   bp->b_dev & MT_COMPAT_MODE &&
			   op == MTwrite) {
			bp->b_flags &= ~B_DATA_TRANSFER;
			bp->b_flags |= B_ABORTING;
			op = bp->b_opcode = MTBSR;
			iob->b_state = (int)send_command;
			goto reswitch;
		}

	/* fall through */

case operation_complete:
	if (op == MTread && !bp->b_error)
	/* coerced od_count to long to fix a bug */
		od_ptr->od_resid = ((int) od_ptr->od_count) -
						(bp->b_bcount - bp->b_resid);
	iob->b_state = (int)undefined;
	drop_selcode(bp);
	queuedone(bp);
	break;

case start_data_transfer:
	bp->b_TO_state = (int)timedout;
	START_TIME(stp_timeout, SHORT_MSG_TIME);
	(*sc->iosw->iod_preamb)(bp, op == MTread ?
					TS_READ_EXECUTE : LS_WRITE_EXECUTE);
	END_TIME;

	iob->b_xaddr = bp->b_un.b_addr;
	iob->b_xcount = bp->b_bcount;

	START_TIME(stp_timeout, DATA_TRANSFER_TIME);
	iob->b_state = (int)data_transfer_done;
	(*sc->iosw->iod_tfr)(MAX_SPEED, bp, stp_fsm);
	break;

case data_transfer_done:
	bp->b_resid = sc->resid;
	(*sc->iosw->iod_postamb)(bp);
	END_TIME;

	if (op == MTread)
		stp_send_end_cmd(bp, END_DATA);
	iob->b_state = (int)(op == MTread ? status_check : ppoll_setup);
	goto reswitch;

case timedout:
	escape(TIMED_OUT);

case start_clear:
	iob->b_state = (int)send_clear;
	get_selcode(bp, stp_fsm);
	break;

case send_clear:
	bp->b_flags |= B_RECOVERING;

	bp->b_TO_state = (int)timedout;
	START_TIME(stp_timeout, SHORT_MSG_TIME);
	(*sc->iosw->iod_clear)(bp);
	END_TIME;

	iob->b_state = (int)ppoll_setup;
	goto reswitch;

default:
	panic("stp_fsm");

		}  /* switch */
		END_FSM;
	recover {
		msg_printf("STP: Aborting cmd (0x%x)\n", op);
		ABORT_TIME;
		HPIB_ppoll_clear(bp);
		(*sc->iosw->iod_abort)(bp);
		drop_selcode(bp);
		if (!bp->b_error)
			bp->b_error = EIO;
		iob->b_state = (int)(bp->b_flags & B_RECOVERING ?
					operation_complete : start_clear);
		goto retry;
	}
}


/*
**  dev_opened - if device already opened, return open device pointer
*/
static struct stp_od *dev_opened(dev)
register dev_t dev;
{
	register struct stp_od *p;

	dev &= M_DEVMASK;
	for (p = stp_odt; p < stp_odt + ODS; ++p)
		if (dev == p->od_device)
			return p;
	return NULL;
}


/*
**  dev_open - allocate a new open device entry and return a pointer to it
*/
static struct stp_od *dev_open(dev)
dev_t dev;
{
	register struct stp_od *p;

	if (m_selcode(dev) > 31 || isc_table[m_selcode(dev)] == NULL ||
	    m_busaddr(dev) > 7	||
	    m_flags(dev) & ~(MT_DENSITY_MASK |
			     MT_COMPAT_MODE |
			     MT_SYNC_MODE |
			     MT_UCB_STYLE |
			     MT_NO_AUTO_REW))
		return NULL;

	for (p = stp_odt; p->od_device; )
		if (++p >= stp_odt + ODS) {
			msg_printf("stp_open: open device table overflow\n");
			return NULL;
		}
	p->od_device = dev & M_DEVMASK;
	return p;
}



/*
**  dev_close
*/
static void dev_close(dev, od_ptr)
dev_t dev;
struct stp_od *od_ptr;
{
	struct isc_table_type *sc = isc_table[m_selcode(dev)];

	if (od_ptr == NULL || !od_ptr->od_device)
		panic("dev_close");
	dma_unactive(sc);
	od_ptr->od_device = 0;
}


stp_control(proc, dev, op, count)
int (*proc)();
dev_t dev;
int op;
long count;
{
	register struct stp_od *od_ptr;
	register struct buf *bp;
	int HPIB_identify();
	int identifying = (proc == HPIB_identify);
	
	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("stp_control");

	bp = geteblk(identifying ? 2 : 0);

	bp->b_dev = dev;
	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_od_ptr = (long)od_ptr;
	bp->b_action = proc;
	bp->b_opcode = op;
	bp->b_resid = count;

	enqueue(&od_ptr->od_iobuf, bp);

	iowait(bp);

	if (identifying)
		od_ptr->od_ident = *(unsigned short *)bp->b_un.b_addr;

	brelse(bp);
	return geterror(bp);
}


#define	AT_BOT	(od_ptr->od_status.sr_bits & SR_BOT)

stp_open(dev, flag)
register dev_t dev;
int flag;
{
	register struct stp_od *od_ptr;
	register struct isc_table_type *sc;
	register long sr_bits;
	int error;
	char *message;

	int HPIB_clear();
	int HPIB_identify();

	/*
	**  error if already opened, can't open, or no interface card
	*/
	if (dev_opened(dev) || (od_ptr = dev_open(dev)) == NULL)
		return ENXIO;

	/*
	**  from here on, dev_close must be called if the open fails
	*/
	od_ptr->od_iobuf.b_flags &= ~(IOB_WRITTEN | IOB_EOF | DDS);
	sc = isc_table[m_selcode(dev)];
	dma_active(sc);

	error = ENXIO;	/* possible error for the next few operations */

	/*
	**  check for HP-IB card
	*/
	if (sc->card_type != HP98625 &&
	    sc->card_type != HP98624 &&
	    sc->card_type != INTERNAL_HPIB) {
		goto error_out;
	}

	/*
	**  identify
	*/
	if (stp_control(HPIB_identify, dev, 0, 0)) {
		message = "not responding";
		goto message_out;
	}

	if (od_ptr->od_ident != HP7974_IDENT &&
	    od_ptr->od_ident != HP7978_IDENT &&
	    od_ptr->od_ident != HP7979_IDENT &&
	    od_ptr->od_ident != HP7980_IDENT &&
	    od_ptr->od_ident != HPC1501_IDENT) {
		message = "unrecognized device";
		goto message_out;
	}

	/*
	**  clear
	*/
	if (stp_control(HPIB_clear, dev, 0, 0))
		goto error_out;

	error = EIO;	/* possible error for the next few operations */

	/* Ignore density for DDS drives */
	if (od_ptr->od_ident == HPC1501_IDENT)
		od_ptr->od_iobuf.b_flags |= DDS;
	else /* Mag Tape: Set flag to check density in strategy routine */
		od_ptr->od_iobuf.b_flags |= CHECK_DENSITY;

	/*
	**  check status
	*/
	if (stp_control(stp_fsm, dev, MTstat, 1))
		goto error_out;

	if (!(od_ptr->od_status.sr_bits & SR_ON_LINE)) {
		message = "not online";
		goto message_out;
	}

	if (flag & FWRITE && od_ptr->od_status.sr_bits & SR_WRITE_PROTECT) {
		message = "has no write ring";
		goto message_out;
	}

	/* The HPIB DAT drive defaults to not acknowledging Save Set Marks
	 * during a read operation.  This is incredibly dumb default wreaks
	 * havoc with our driver!  A quick and dirty is performed:  In order
	 * maintain compatibility with SCSI,  we enable this feature.
	 * Set flag: DDS_ACK_SSM, and the fsm checks this bit and calls
	 * the send_utility command to do this.
	 */
	if (od_ptr->od_iobuf.b_flags & DDS) {
		od_ptr->od_iobuf.b_flags |= DDS_ACK_SSM;
		/* Notice that the parameters are dummy! */
		/* We must send command twice, ignoring errors and continuing!*/
		stp_control(stp_fsm, dev, MTWRITE_UTIL, 1);
		stp_control(stp_fsm, dev, MTWRITE_UTIL, 1);
		od_ptr->od_iobuf.b_flags &= ~DDS_ACK_SSM;
	}

	/*
	**  decide upon immediate report and streaming modes
	*/
	{
	int IR_mode_op, ST_mode_op;

	if (flag & FWRITE && dev & (MT_COMPAT_MODE | MT_SYNC_MODE)) {
		IR_mode_op = MTdisIR;
		ST_mode_op = MTdisST;
	} else {
		IR_mode_op = MTenIR;
		ST_mode_op = MTenST;
	}

	if (stp_control(stp_fsm, dev, IR_mode_op, 1) ||
	    stp_control(stp_fsm, dev, ST_mode_op, 1))
		goto error_out;
	}

	return 0;

message_out:
	msg_printf("stp: dev 0x%x %s\n", dev, message);

error_out:
	dev_close(dev, od_ptr);
	return error;
}


stp_close(dev, flag)
register dev_t dev;
int flag;
{
	register struct stp_od *od_ptr;
	register struct iobuf *iob;
	int res1 = 0, res2 = 0, res3 = 0;

	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("stp_close");
	iob = &od_ptr->od_iobuf;

	if (flag & FWRITE) {
		if (iob->b_flags & IOB_WRITTEN) {
			res1 = stp_control(stp_fsm, dev, MTWEOF, 2);
			if (dev & MT_NO_AUTO_REW)
				res2 = stp_control(stp_fsm, dev, MTBSR, 1);
		}
		if (!(dev & MT_NO_AUTO_REW))
			res2 = stp_control(stp_fsm, dev, MTREW, 1);
	} else {
		if (!(dev & MT_NO_AUTO_REW)) {
			res1 = stp_control(stp_fsm, dev, MTREW, 1);
		} else {
			if(!(dev&MT_UCB_STYLE) && !(iob->b_flags&IOB_EOF)) {
				res1 = stp_control(stp_fsm, dev, MTFSF, 1);
			}
		}
	}
	/* Force controller to synchronize with tape */
	res3 = stp_control(stp_fsm, dev, MTstat, 1);

	dev_close(dev, od_ptr);
	return res1 || res2 || res3;
}


stp_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
	register struct stp_od *od_ptr;
	struct io_hpib_ident *id_rec;

	switch (order) {

	case IO_HPIB_IDENT:

		if ((od_ptr = dev_opened(dev)) == NULL)
		    return(EIO);
		else
		{
		    id_rec = (struct io_hpib_ident *)addr;
		    id_rec->ident = od_ptr->od_ident;
		    id_rec->dev_type = IOSCAN_HPIB_TAPE;
		    return(0);
		}
		
		break;
	case MTIOCTOP:	/* control operation */
		{
		struct mtop *mtop = (struct mtop *)addr;
		register unsigned int op = mtop->mt_op;
		register long count;

		if (op > MTBSS) 
			return ENXIO;
		count = mtop->mt_count;
		if (op >= MTREW && op <= MTEOD) 
			count = 1;
		return stp_control(stp_fsm, dev, op, count);
		}

	case MTIOCGET:
		{
		register struct mtget *mtget = (struct mtget *)addr;

		if ((od_ptr = dev_opened(dev)) == NULL)
			panic("stp_ioctl");

		if (od_ptr->od_iobuf.b_flags & DDS)
			mtget->mt_type = MT_ISDDS1;
		else
			mtget->mt_type = MT_ISSTREAM;
		mtget->mt_resid = od_ptr->od_resid;
		mtget->mt_dsreg1 = od_ptr->od_status.sr_mtget.dsreg1;
		mtget->mt_dsreg2 = od_ptr->od_status.sr_mtget.dsreg2;
		/*
		 * Fill in generic status register, status bits are
		 * left justified in a a 32 bit word:
		 *
		 *  sss00s0s  sss00s0s  00000000  00000000
		 *  |||  | |  |||  | |
		 *  |||  | |  |||  | +--- immed report	[sr2 & 0x04]
		 *  |||  | |  |||  +----- door open	[sr2 & 0x01]
		 *  |||  | |  ||+--------  800 bpi	[sr3 & 0x40]
		 *  |||  | |  |+--------- 1600 bpi	[sr3 & 0x80]
		 *  |||  | |  +---------- 6250 bpi	[sr2 & 0x80]
		 *  |||  | +------------- online	[sr1 & 0x01]
		 *  |||  +--------------- write prot	[sr1 & 0x04]
		 *  ||+------------------ EOT		[sr1 & 0x20]
		 *  |+------------------- BOT		[sr1 & 0x40]
		 *  +-------------------- EOF		[sr1 & 0x80]
		 */
		mtget->mt_gstat =
		    ((od_ptr->od_status.sr_regs.sr1 & 0xe5) << 24) |
		    ((od_ptr->od_status.sr_regs.sr2 & 0x85) << 16) |
		    ((od_ptr->od_status.sr_regs.sr3 & 0xc0) << 15);

		/* Special case if both EOF and BOT set, then we must
		 * fix register: indicate setmark, and clear BOT and EOF! 
		 */
		if (GMT_EOF(mtget->mt_gstat) && GMT_BOT(mtget->mt_gstat)) {
			int mask = 0xffffffff;
			mtget->mt_gstat |= GMT_SM(mask);
			mtget->mt_gstat &= ~(GMT_EOF(mask) | GMT_BOT(mask));
			}

		mtget->mt_erreg = od_ptr->od_status.sr_mtget.erreg;
		return 0;
		}

	default:
		return ENXIO;
	}
}


stp_strategy(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;
	register int requested = dev & MT_DENSITY_MASK;
	register struct stp_od *od_ptr;
	register int right_density;

	if ((od_ptr = dev_opened(dev)) == NULL)
		panic("stp_strategy");

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_od_ptr = (long)od_ptr;
	bp->b_resid = bp->b_bcount;
	bp->b_action = stp_fsm;
	bp->b_opcode = bp->b_flags & B_READ ? MTread : MTwrite;

	/* If at BOT 
	 *   If operation == READ 
	 *	and !right_density
	 *	   report error and exit
	 *   Else operation == WRITE, 
	 *	unconditionally write density mark
	 */
	if (od_ptr->od_iobuf.b_flags & CHECK_DENSITY) {
	    register long sr_bits = od_ptr->od_status.sr_bits;
	    int op;

	    od_ptr->od_iobuf.b_flags &= ~CHECK_DENSITY;

	    if (!AT_BOT) /* Not at BOT */ {
		/* If tape head is not at BOT, requested density
		 * must match tape density.  Since we cannot programmatically
		 * distinguish GCR from GCR/Compressed we accept either density.
		 */
		if (!(requested == MT_1600_BPI && sr_bits & SR_PE_DENSITY     ||
		     requested  == MT_6250_BPI && sr_bits & SR_GCR_DENSITY    ||
		     requested == MT_GCR_COMPRESS && sr_bits & SR_GCR_DENSITY ||
		     requested == MT_800_BPI  && sr_bits & SR_NRZI_DENSITY))
			goto error_out;
		}

	    else	{
		/* At BOT.  Determine requested density.  On write operations
		 * at BOT we always write a density burst header.
		 */
	    right_density =
		requested == MT_1600_BPI && sr_bits & SR_PE_DENSITY      ||
		requested == MT_6250_BPI && sr_bits & SR_GCR_DENSITY     ||
		requested == MT_GCR_COMPRESS && sr_bits & SR_GCR_DENSITY ||
		requested == MT_800_BPI && sr_bits & SR_NRZI_DENSITY;

	    switch (requested)	{
		case MT_1600_BPI:     op = MTpe;    break;
		case MT_6250_BPI:     op = MTgcr;   break;
		case MT_800_BPI:      op = MTnrzi;  break;
		case MT_GCR_COMPRESS: op = MTcompr; break;
		}

	   if (bp->b_opcode == MTread && !right_density ||
	       bp->b_opcode == MTwrite && stp_control(stp_fsm, dev, op, 1)) {

error_out:
		   msg_printf("stp: dev 0x%x incorrect density\n", dev);
		   bp->b_error = EIO;
		   bp->b_flags |= B_ERROR;
		   iodone(bp);
		   return;
		   }
		}
	    }

	enqueue(&od_ptr->od_iobuf, bp);
}


stp_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (uio->uio_iovcnt <= 65536) ?
	    physio(stp_strategy, NULL, dev, B_READ, minphys, uio) : EIO;
}


stp_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (uio->uio_iovcnt <= 65536) ?
	    physio(stp_strategy, NULL, dev, B_WRITE, minphys, uio) : EIO;
}

#ifdef IOSCAN
stp_link()
{
        hpib_config_ident = stp_config_identify;
}

stp_config_identify(sinfo)
register struct ioctl_ident_hpib *sinfo;
{
    register struct stp_od *od_ptr;
    register struct isc_table_type *isc;
    dev_t sdev, bdev;
    int busaddr, err;

    if ((isc = isc_table[sinfo->sc]) == NULL)
        return ENODEV;

    if (isc->card_type != HP98625 &&
        isc->card_type != HP98624 &&
        isc->card_type != INTERNAL_HPIB) {
                return ENODEV;
        }

    bdev = 0x04000000 | (sinfo->sc << 16);

    for (busaddr = 0; busaddr < 8; busaddr++) {

        sdev = bdev | (busaddr << 8);

        if (dev_opened(sdev) || (od_ptr = dev_open(sdev)) == NULL)
                return ENXIO;
        od_ptr->od_iobuf.b_flags &= ~(IOB_WRITTEN | IOB_EOF | DDS);

        err =  stp_control(HPIB_identify, sdev, 0, 0);

        if (err) {
	    dev_close(sdev, od_ptr);
            sinfo->dev_info[busaddr].ident = -1;
            continue;
        }       

        sinfo->dev_info[busaddr].ident = od_ptr->od_ident;

	dev_close(sdev, od_ptr);
    }
    return 0;
}
#endif /* IOSCAN */
