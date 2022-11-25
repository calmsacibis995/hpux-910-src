static char *scsitape_id =
"$Header: scsitape.c,v 1.2 92/07/14 13:17:43 root Exp $";
/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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
 *	SCSI tape driver
 *	   for Sequential Access Drives
 *	   Current Capabilities
 *		- Full Channel Implementation (full disconnect/reconnect)
 *		- Ioctl()
 *			rewind
 *			space
 *		- Ioctl pass-thru for Userland Access to SCSI Command Set
 */

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
#include <diskio.h>
#include <uio.h>
#include <scsi.h>
#include <_scsi.h>
#include <mtio.h>

/*
 * mnemonic names for bits in iobuf's b_flags
 */
#define IOB_WRITTEN	B_SCRACH1
#define IOB_EOF		B_SCRACH2
#define IOB_EOT_SEEN	B_SCRACH3
#define HP		B_SCRACH4
#define NEED_SYNC_DATA	B_SCRACH5
#define DRIVE_OFFLINE	B_SCRACH6
#define NOT_AT_BOT	B_DIL
#define UNLOADED	B_END_OF_DATA	/* Used for suppressing tape action
					 * at close.
					 */

/*
 * timeout and retry parameters
 */
#define SELECT_TIME	(( 5000 * HZ) / 1000)
#define TRANSFER_TIME	(( 120000 * HZ) / 1000)
#define COMMAND_TIME	(( 1000 * HZ) / 1000)
#define MESSAGE_TIME	(( 1000 * HZ) / 1000)

char *tape_sense_key[] = {
		"No_sense",
		"Recovered_error",
		"Not_ready",
		"Medium_error",
		"Hardware_error",
		"Illegal_request",
		"Unit_attention",
		"Data_protect",
		"Blank_check",
		"Vendor_unique",
		"Copy_aborted",
		"Aborted_command",
		"Equal",
		"Volume_overflow",
		"Miscompare",
		"Reserved_sense_key"
		};

#define MAX_CODES	0x39

/* The sense_codes are numbered only to insure code correctly
 * correlating the index with the code_fnct[][] below.
 */
char *tape_sense_code[] = {
	/* 0x0  */	"No sense",
	/* 0x1  */	"FM detected",
	/* 0x2  */	"End of partition/medium",
	/* 0x3  */	"Save-set mark detected",
	/* 0x4  */	"BO partition/med detected",
	/* 0x5  */	"End of data",
	/* 0x6  */	"Error rate warning",
	/* 0x7  */	"Humidity warning",
	/* 0x8  */	"Recovered error",
	/* 0x9  */	"Drive offline",
	/* 0xA  */	"Drive getting ready",
	/* 0xB  */	"Medium not present",
	/* 0xC  */	"Medium error (reached phys EOP/M)",
	/* 0xD  */	"Write error",
	/* 0xE  */	"Read error",
	/* 0xF  */	"Incompat. media",
	/* 0x10 */	"Unknown format",
	/* 0x11 */	"Positioning error",
	/* 0x12 */	"Error append during write",
	/* 0x13 */	"Hard failure during write",
	/* 0x14 */	"Servo failure",
	/* 0x15 */	"Hardware diag. error",
	/* 0x16 */	"Internal hardware failure",
	/* 0x17 */	"Media load/eject failed",
	/* 0x18 */	"Moisture detected",
	/* 0x19 */	"Illegal request",
	/* 0x1A */	"Invalid command op code",
	/* 0x1B */	"Invalid field in CDB",
	/* 0x1C */	"LUN not supported",
	/* 0x1D */	"Invalid test number",
	/* 0x1E */	"Parm page not supported",
	/* 0x1F */	"Invalid bits in Identify",
	/* 0x20 */	"Media changed",
	/* 0x21 */	"Device Reset",
	/* 0x22 */	"Power-on failure",
	/* 0x23 */	"Mode parms changed",
	/* 0x24 */	"Log parms changed",
	/* 0x25 */	"Write protected",
	/* 0x26 */	"Blank check",
	/* 0x27 */	"EOD found",
	/* 0x28 */	"<unused>",
	/* 0x29 */	"Copy aborted",
	/* 0x2A */	"Copy aborted / Host can't discon",
	/* 0x2B */	"Aborted cmd",
	/* 0x2C */	"Unexpected phase sequence",
	/* 0x2D */	"Unexpected message",
	/* 0x2E */	"Select / reselect error",
	/* 0x2F */	"Parity error",
	/* 0x30 */	"Init detected error msg",
	/* 0x31 */	"Illegal scsi message",
	/* 0x32 */	"Unexpected Cmd phas",
	/* 0x33 */	"Unexpected Data phas",
	/* 0x34 */	"Cmd currently processin",
	/* 0x35 */	"Equal",
	/* 0x36 */	"Volume overflow (write_FM)",
	/* 0x37 */	"Mis-compare",
	/* 0x38 */	"EOD mark not found"
};

unsigned char code_fnct[MAX_CODES][4] = {
/*      Key	Byte12	Byte13 Errno                    */
     {	0x0,	 0x0,	 0x0,	0      }, /* 0x0  No sense */
     {	0x0,	 0x0,	 0x1,	0      }, /* 0x1  FM detected */
     {	0x0,	 0x0,	 0x2,	0      }, /* 0x2  End of partition/medium */
     {	0x0,	 0x0,	 0x3,	0      }, /* 0x3  Save-set mark detected */
     {	0x0,	 0x0,	 0x4,	0      }, /* 0x4  BO partition/med detected */
     {	0x0,	 0x0,	 0x5,	ENXIO  }, /* 0x5  End of data */
     {	0x0,	 0x0A,	 0x0,	0      }, /* 0x6  Error rate warning */
     {	0x0,	 0x81,	 0x0,	0      }, /* 0x7  Humidity warning */
     {	0x1,	 0x0,	 0x0,	0      }, /* 0x8  Recovered error */
     {	0x2,	 0x4,	 0x0,	ENXIO  }, /* 0x9  Drive offline */
     {	0x2,	 0x4,	 0x1,	ENXIO  }, /* 0xA  Drive getting ready */
     {	0x2,	 0x3A,	 0x0,	ENXIO  }, /* 0xB  Medium not present */
     {	0x3,	 0x0,	 0x2,	EIO    }, /* 0xC  Medium error (reached phys EOP/M) */
     {	0x3,	 0xC,	 0x0,	EIO    }, /* 0xD  Write error */
     {	0x3,	 0x11,	 0x0,	EIO    }, /* 0xE  Read error */
     {	0x3,	 0x30,	 0x0,	EIO    }, /* 0xF  Incompat. media */
     {	0x3,	 0x30,	 0x1,	EIO    }, /* 0x10 Unknown format */
     {	0x3,	 0x3B,	 0x0,	EIO    }, /* 0x11 Positioning error */
     {	0x3,	 0x50,	 0x0,	EIO    }, /* 0x12 Error append during write */
     {	0x4,	 0x03,	 0x0,	EIO    }, /* 0x13 Hard failure during write */
     {	0x4,	 0x09,	 0x0,	EIO    }, /* 0x14 Servo failure */
     {	0x4,	 0x40,	 0x0,	EIO    }, /* 0x15 Hardware diag. error */
     {	0x4,	 0x44,	 0x0,	EIO    }, /* 0x16 Internal hardware failure */
     {	0x4,	 0x53,	 0x0,	EIO    }, /* 0x17 Media load/eject failed */
     {	0x4,	 0x82,	 0x80,	EIO    }, /* 0x18 Moisture detected */
     {	0x5,	 0x1A,	 0x0,	EINVAL }, /* 0x19 Illegal request */
     {	0x5,	 0x20,	 0x0,	EINVAL }, /* 0x1A Invalid command op code */
     {	0x5,	 0x24,	 0x0,	EINVAL }, /* 0x1B Invalid field in CDB */
     {	0x5,	 0x25,	 0x0,	EINVAL }, /* 0x1C LUN not supported */
     {	0x5,	 0x26,	 0x0,	EINVAL }, /* 0x1D Invalid test number */
     {	0x5,	 0x26,	 0x1,	EINVAL }, /* 0x1E Parm page not supported */
     {	0x5,	 0x3D,	 0x0,	EINVAL }, /* 0x1F Invalid bits in Identify */
     {	0x6,	 0x28,	 0x0,	EINVAL }, /* 0x20 Media changed */
     {	0x6,	 0x29,	 0x0,	EINVAL }, /* 0x21 Device Reset */
     {	0x6,	 0x29,	 0x80,	EINVAL }, /* 0x22 Power-on failure */
     {	0x6,	 0x2A,	 0x1,	EINVAL }, /* 0x23 Mode parms changed */
     {	0x6,	 0x2A,	 0x2,	EINVAL }, /* 0x24 Log parms changed */
     {	0x7,	 0x27,	 0x0,	EACCES }, /* 0x25 Write protected */
     {	0x8,	 0x0,	 0x0,	ENXIO  }, /* 0x26 Blank check */
     {	0x8,	 0x0,	 0x5,	ENXIO  }, /* 0x27 EOD found */
     {	0x9,	 0x0,	 0x0,	ENXIO  }, /* 0x28 <unused> */
     {	0xA,	 0x0,	 0x0,	EIO    }, /* 0x29 Copy aborted */
     {	0xA,	 0x2B,	 0x0,	EIO    }, /* 0x2A Copy aborted / Host can't discon */
     {	0xB,	 0x0,	 0x0,	EIO    }, /* 0x2B Aborted cmd */
     {	0xB,	 0x2C,	 0x0,	EIO    }, /* 0x2C Unexpected phase sequence */
     {	0xB,	 0x43,	 0x0,	EIO    }, /* 0x2D Unexpected message */
     {	0xB,	 0x45,	 0x0,	EIO    }, /* 0x2E Select / reselect error */
     {	0xB,	 0x47,	 0x0,	EIO    }, /* 0x2F Parity error */
     {	0xB,	 0x48,	 0x0,	EIO    }, /* 0x30 Init detected error msg */
     {	0xB,	 0x49,	 0x0,	EINVAL }, /* 0x31 Illegal scsi message */
     {	0xB,	 0x4A,	 0x0,	EINVAL }, /* 0x32 Unexpected Cmd phase*/
     {	0xB,	 0x4B,	 0x0,	EINVAL }, /* 0x33 Unexpected Data phase*/
     {	0xB,	 0x4E,	 0x0,	EINVAL }, /* 0x34 Cmd currently processing*/
     {	0xC,	 0x0,	 0x0,	EIO    }, /* 0x35 Equal */
     {	0xD,	 0x0,	 0x2,	EIO    }, /* 0x36 Volume overflow (write_FM) */
     {	0xE,	 0x0,	 0x0,	EIO    }, /* 0x37 Mis-compare */
     {  0x8,     0x14,   0x3,   ENXIO  }  /* 0x38 EOD mark not found */
};

/*
 * open device table entry - one per physical device (controller)
 */
struct scsitape_od {
	struct  iobuf    dev_queue;	/* queue head */
	dev_t   od_device;		/* selcode/busaddr */
	struct  xsense   xsense;	/* most recent device status */
	long	od_resid;		/* most recent resid */
	char    inquiry_sz;		/* Size of inquiry data field	*/
	char    name[SZ_INQUIRY];	/* Name from Inquiry command	*/
	char    flags;			/* flags for special use	*/
	int	last_status;		/* last scsi status returned	*/
	dev_t   cmd_mode_dev;		/* Used for syncronizing command mode
					 * access.  When set, it's in use.
					 */
	struct scsi_cmd_parms cmd_parms;/* ioctl command parameters */

	/* Following is for saved values in buf header
	 * for request sense
	 */
	unsigned int	save_resid;
	long	save_cnt;
	int	save_flags;
	int	(*save_action2)();
	char	save_status;
};

/*
 * mnemonic names for bits in scsitape_od flags
 */
#define FLAG_QUIET	0x01	/* disable obnoxious diagnostics */
#define FLAG_EOT_ACK	0x02	/* MTIOCGET has acknowledged EOT */
#define FLAG_OPEN_DISK	0x04	/* scsi disk opened */

/*
 * SCSI disk device major number
 */
int scsi_major = 47;

/*
 * open device table entry
 */
#define STOD 8
struct scsitape_od scsitape_odt[STOD];

int scsitape_ticks[] = {
	120 * HZ,	/* MTWEOF   0 */
	720 * HZ,	/* MTFSF    1 */
	720 * HZ,	/* MTBSF    2 */
	120 * HZ,	/* MTFSR    3 */
	120 * HZ,	/* MTBSR    4 */
	360 * HZ,	/* MTREW    5 */
	120 * HZ,	/* MTOFFL   6 */
	 60 * HZ,	/* MTNOP (Generic timeout) 7 */
 	120 * HZ,	/* MTEOD    8 */
	120 * HZ,	/* MTWSS    9 */
	120 * HZ,	/* MTFSS   10 */
	120 * HZ,	/* MTBSS   11 */
	 60 * HZ,	/* Mode Sense   12 */
	 60 * HZ 	/* Mode Select  13 */
};

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
scsitape_decode_sense(bp, dp)
register struct buf *bp;
struct scsitape_od *dp;
{
	register struct xsense *sptr;
	register int index;
	int i, key, byte12, byte13, found=0, quiet=0;
	int scsitape_xfr();

	sptr = (struct xsense *)&dp->xsense;

	key    = sptr->sense_key;
	byte12 = sptr->sense_code; /* SCSI II: add'l sense code */
	byte13 = sptr->resv;	   /* SCSI II: add'l sense code qualifier */
	for (index=0; index < MAX_CODES; index++)
		if (code_fnct[index][0] == key && code_fnct[index][1] == byte12
			&& code_fnct[index][2] == byte13)
			{found++; break;}

	if (!found) {
		if (key != 0 && key != 1 && key != 6) {
			msg_printf("Cannot decode sense information.\n");
			bp->b_error = EIO; /* We flag unknown sense with EIO */
			}
		}
	else
		bp->b_error = code_fnct[index][3];

	if (sptr->valid)	{
		/* Logical Block Address field contains valid info */
		bcopy(sptr->lba, &dp->od_resid, 4);
		SCSI_TRACE(DIAGs)("SCSI: info bytes (residue): %d\n",
				dp->od_resid);
		}

	if (dp->flags & FLAG_QUIET)
		quiet++;

	/* Don't log message for media change and first time power-up */
	if (key == 0x6 && byte13 == 0)	{
		if (byte12 == 0x28) {
			dp->dev_queue.b_flags &= ~(NOT_AT_BOT | IOB_EOT_SEEN);
			quiet++; 
			}
		else if (byte12 == 0x29) {
			bp->b_newflags &= ~SDTR_SENT;
			dp->dev_queue.b_flags &= ~IOB_EOT_SEEN;
			if (!(dp->dev_queue.b_flags & NOT_AT_BOT))
				quiet++;
			}
		}

	if ((bp->b_error && !quiet) || SCSI_DEBUG&DIAGs) 	{
		unsigned char *ptr = (unsigned char *)sptr;

		msg_printf("SCSI: Status bytes: ");
		for (i=0; i<bp->b_bcount-bp->b_resid; i++)
			msg_printf("%x ",*ptr++);
		msg_printf("\n");

		msg_printf("SCSI: (dev 0x%x) sense_key=%s  ",
			bp->b_dev, tape_sense_key[sptr->sense_key]);

		if (found)  {
			msg_printf("sense_code=%s\n", tape_sense_code[index]);
			if (index == 0x09)
			    dp->dev_queue.b_flags |= DRIVE_OFFLINE;
			}
		else
			msg_printf("(Unknown code)\n");

		if (sptr->fru)	{
			if (sptr->fru == 1)
				msg_printf("SCSI: FRU: Interface\n");
			else if (sptr->fru == 2)
				msg_printf("SCSI: FRU: Buffer\n");
			else	/* Assume fru == 3 */
				msg_printf("SCSI: FRU: Drive electronics\n");
			bp->b_error = EIO;
			}
		if (sptr->field & 0x80)	{
			msg_printf("SCSI: sense key specific info\n");
			if (sptr->field & 0x40)
				msg_printf("illegal parm in CDB");
			else
				msg_printf("illegal parm during DATA OUT");
			msg_printf(" byte %d ", sptr->field_ptr);
			if (sptr->field & 0x08)
				msg_printf("(bit %d)", sptr->field & 0x07);
			msg_printf("\n");
			}
		else
		    if (sptr->field_ptr)
			msg_printf("SCSI: CCL error: %d status\n",
			    (sptr->field_ptr>>8) & 0xff, sptr->field_ptr & 0xff);
		}

	if (!quiet && sptr->parms)	{
		if (sptr->parms&FM)
		    SCSI_TRACE(DIAGs)("SCSI: Current Cmd read Filemark\n");
		if (sptr->parms&EOM) {
		    msg_printf("SCSI: End of Medium Found\n");
		    /* Notify EOM condition after write operations only */
		    if(bp->b_action2 == scsitape_xfr){
			dp->dev_queue.b_flags |= IOB_EOT_SEEN;
		        SCSI_TRACE(DIAGs)("EOT Seen\n");
			}
		    }
		if (sptr->parms&ILI)
		    SCSI_TRACE(DIAGs)("Illegal Length Indicated\n");
		}

	return(0);
}

/*
 * Command Commands shared by all device drivers.
 * Found in scsi_ccs.c  (Command Command Set)
 */
extern six_byte_cmd();
extern ten_byte_cmd();
extern scsi_mode_sense_cmd();
extern scsi_inquiry();
extern scsi_request_sense();
extern scsi_test_unit();
extern scsi_if_dequeue();

/*
 * Request unit to load tape (if offline) or rewind and unload tape
 */
scsi_load(bp)
register struct buf *bp;
{
	int load_bit;

	load_bit = bp->b_parms ? 0 : 1;	/* Assumption: MTOFFL != 0 */
if (SCSI_DEBUG&CMDS)	{
	if (load_bit)
		msg_printf("load_tape\n");
	else
		msg_printf("unload_tape\n");
	}

	if (!load_bit)
		bp->b_queue->b_flags |= UNLOADED;

	bp->b_flags |= B_READ;	/* transfer direction */
	bp->b_bcount = 0;

	six_byte_cmd(bp, CMDload, 0, 0, load_bit, 0);
}

/*
 * Request Block Limits of logical unit
 * Returns: Maximum block length and Minimum block length (in bytes) of LUN
 */
scsi_read_block_limits(bp)
register struct buf *bp;
{
	SCSI_TRACE(CMDS)("scsi_read_block_limits\n");

	bp->b_flags |= B_READ;	/* transfer direction */
	bp->b_bcount = bp->b_queue->b_xcount = 
					SZ_CMDread_block_limits;

	six_byte_cmd(bp, CMDread_block_limits, 0, 0, 0, 0);
}

#define SILI	0x2
/*
 * READ  6-byte CDB format
 * WRITE 6-byte CDB format
 * Assumptions: The drive has been set to 'variable block mode'
 *		either via default or 'mode_select' command.
 *		The SILI bit is set to indicate normal status for short reads.
 */
scsitape_xfr(bp)
register struct buf *bp;
{
	register int length = bp->b_bcount;
	register struct iobuf *iob = bp->b_queue;

	struct {
		unsigned char	opcode;
		unsigned char	lunSf;
		unsigned char	len_msb;
		unsigned char	len_isb;
		unsigned char	len_lsb;
		unsigned char	control;
	} cdb; /* command descriptor block */

	cdb.opcode  = bp->b_flags & B_READ ? CMDread : CMDwrite;
	/* If Block type drive, modify bytecount */
	if (iob->blkssz_log2) {
		/* Set fixed block bit, and do not set SILI */
		cdb.lunSf   = 0x01;
		length >>= iob->blkssz_log2;
		}
	else
		/* We no longer set the SILI bit, in order to
		 * report back mt_resid for users who need this information
		 */
		/* cdb.lunSf   = bp->b_flags & B_READ ? SILI : 0; */
		cdb.lunSf   = 0;

	cdb.len_msb = (unsigned char) (length>>16);
	cdb.len_isb = (unsigned char) (length>>8);
	cdb.len_lsb = (unsigned char) (length&0xFF);
	cdb.control = 0;


if (SCSI_DEBUG&CMDS)	{
	if (bp->b_flags&B_READ)
		msg_printf("scsitape_read  ");
	else
		msg_printf("scsitape_write ");
	msg_printf("busaddr 0x%x byte_count 0x%x\n", bp->b_ba, length);
	}

	if (!(bp->b_flags&B_READ))
		iob->b_flags |= IOB_WRITTEN;
	scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}

rewind(bp)
register struct buf *bp;
{
	struct scsitape_od *dp = (struct scsitape_od *)bp->b_queue;

	dp->dev_queue.b_flags &= ~IOB_EOT_SEEN;
	dp->flags &= ~FLAG_EOT_ACK;

	SCSI_TRACE(CMDS)("rewind\n");

	bp->b_flags |= B_READ;	/* transfer direction */
	bp->b_bcount = 0;

	six_byte_cmd(bp, CMDrewind, 0,0,0,0);
}

/*
 * space()
 * Motion commands to device:
 *	Forward and backward space a block or filemark.
 *	Negative values imply reverse motion (2's complement)
 *	bp->b_bcount holds count (number of items to position)
 *	bp->b_parms  holds 'space code' (whether records or filemarks)
 */
space(bp)
register struct buf *bp;
{
	struct scsitape_od *dp = (struct scsitape_od *)bp->b_queue;
	register int parm = (int)bp->b_parms;
	int count = bp->b_bcount;
	struct {
		char    opcode;
		char    code;
		char    lba_msb;
		char    lba_mid;
		char    lba_lsb;
		char    control;
	} cdb;  /* command descriptor block */

	if (parm == MTBSR || parm == MTBSF || parm == MTBSS)
	{
	    dp->dev_queue.b_flags &= ~IOB_EOT_SEEN;
	    dp->flags &= ~FLAG_EOT_ACK;
	}

	cdb.opcode  = CMDspace;

	/* Sort out the code for byte 1 */
	if (parm == MTFSR || parm == MTBSR)
		cdb.code = 0;
	if (parm == MTFSF || parm == MTBSF)
		cdb.code = 1;
	if (parm == MTFSS || parm == MTBSS)
		cdb.code = 4;
	if (parm == MTEOD)
		cdb.code = 3;

	/* If motion command is reverse direction, count is negative.
	 * Must use 2's complement notation
	 */
	if (parm == MTBSR || parm == MTBSF || parm == MTBSS)
		count = -count;

	SCSI_TRACE(CMDS)("space (parm %d code %d cnt 0x%x)\n",
		bp->b_parms, cdb.code, (unsigned int)count);

	cdb.lba_msb = 0xFF & count>>16;
	cdb.lba_mid = 0xFF & count>>8;
	cdb.lba_lsb = 0xFF & count;
	cdb.control = 0;

	scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}

#define WSmk	0x02

write_filemarks(bp)
register struct buf *bp;
{
	int count=bp->b_bcount;
	char *str = "";
	struct {
		char    opcode;
		char    code;
		char    lba_msb;
		char    lba_mid;
		char    lba_lsb;
		char    control;
	} cdb;  /* command descriptor block */


	cdb.opcode  = CMDwrite_filemarks;
	if (bp->b_parms == MTWSS) {
		cdb.code = WSmk;
		str = " (WSmk)";
		}
	else
		cdb.code = 0;
	cdb.lba_msb = 0xFF & count>>16;
	cdb.lba_mid = 0xFF & count>>8;
	cdb.lba_lsb = 0xFF & count;
	cdb.control = 0;

	SCSI_TRACE(CMDS)("write_filemarks(%d): cnt %d\n",
		cdb.code, count);

	scsi_if_transfer(bp, &cdb, 6, B_WRITE, MUST_FHS);
}

scsitape_cmd(bp)
register struct buf *bp;
{
	register struct scsi_cmd_parms *parms =
			&((struct scsitape_od *)bp->b_queue)->cmd_parms;

if (SCSI_DEBUG&CMD_MOD)	{
	int i;
	msg_printf("scsitape_cmd: %d-byte cmd; clock_ticks: %d",
		parms->cmd_type, parms->clock_ticks);
	if (parms->cmd_mode)
		msg_printf(" with ATN\n");
	else
		msg_printf(" without ATN\n");
	msg_printf("Cmd: ");
	for (i=0; i< parms->cmd_type; i++)
		msg_printf("%x ", (unsigned char)parms->command[i]);
	msg_printf("\n");
	}

	bp->b_clock_ticks = parms->clock_ticks;
	scsi_if_transfer(bp, parms->command, parms->cmd_type,B_WRITE,MUST_FHS);
}

#define PF 0x100000

scsi_mode_select_cmd(bp)
register struct buf *bp;
{
	struct scsitape_od *dp = (struct scsitape_od *)bp->b_queue;
	int pf, page_code = (int)bp->b_parms;

	SCSI_TRACE(CMDS)("scsi_mode_select: Pg %d (cnt %d)\n",
			page_code, bp->b_bcount);

	bp->b_flags |= B_WRITE;	/* data transfer direction,
				 * Variable length data
				 */
	/* count is set by calling process */
	bp->b_queue->b_xcount = bp->b_bcount;
	pf = dp->dev_queue.b_flags &  HP ? PF : 0;

	six_byte_cmd(bp, CMDmode_select, 0, pf, bp->b_bcount, 0);
}

/*
 * table for translating HPIB tape operations to SCSI cmds
 *
 * * NOTE * the array assumes it knows the order of UNIX MT* operations!
 */
int (*tape_cmd_table[])() = {
  write_filemarks,	/* MTWEOF   0 write an end-of-file record */
  space,		/* MTFSF    1 forward space file */
  space,		/* MTBSF    2 backward space file */
  space,		/* MTFSR    3 forward space record */
  space,		/* MTBSR    4 backward space record */
  rewind,		/* MTREW    5 rewind */
  scsi_load,		/* MTOFFL   6 rewind and put drive offline */
  write_filemarks,	/* MTNOP    7 wait pending writes */
  space,		/* MTEOD    8 DDS only. seek to end-of-data */
  write_filemarks,	/* MTWSS    9 DDS only. write SETMARKS */
  space,		/* MTFSS    a DDS only. space forward SETMARK(s) */
  space			/* MTBSS    b DDS only. space backward SETMARK(s) */
  /* Not Implemented From HPIB ...
			 * MTgcr    *
			 * MTpe     *
			 * MTnrzi   *
			 * MTcompr  *
			 * MTdisST  *
			 * MTenST   *
			 * MTdisIR  *
			 * MTenIR   *
			 * MTread   *
			 * MTwrite  *
   */
};

/* The device disconnected, and never returned. */
scsitape_discon_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: disconnect timed out; dev = 0x%x\n", bp->b_dev);
	scsi_if_remove_busfree(bp);
	TIMEOUT_BODY(iob->intloc, scsi_if_dequeue, 
		bp->b_sc->int_lvl, 0, discon_TO);
}

scsitape_select_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: select timed out; dev = 0x%x\n", bp->b_dev);
	scsi_if_term_arbit(bp);  /* Terminate Arbitration phase */
	TIMEOUT_BODY(iob->intloc, scsi_if_dequeue,
		bp->b_sc->int_lvl, 0, select_TO);
}

scsitape_req_timedout(bp)
register struct buf *bp;
{
	msg_printf("SCSI: request timed out; dev = 0x%x; state = 0x%x\n",
		bp->b_dev, bp->b_queue->b_state);
    scsi_if_remove_busfree(bp);
	TIMEOUT_BODY(iob->intloc, scsi_if_dequeue, 
		bp->b_sc->int_lvl, 0, transfer_TO);
}


#define	BUS_BUSY	1
#define	NOT_OWNER	2
/*
 * The SCSI Finite State Machine is a complex beast.
 * It is driven by two mechanisms:
 *	- The phase requested by the target
 *	- by software
 * It is sometimes necessary to remember the previous phase
 * of the bus.  This is stored in the bp struct: b_phase
 */
scsitape_fsm(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	struct scsitape_od *dp = (struct scsitape_od *)iob;

retry:	try

	START_FSM;
reenter:

	SCSI_TRACE(STATES)("FSM: %x ba %d\n",
				iob->b_state, bp->b_ba);

	if (bp->b_flags&TO_SET)	{
		END_TIME;
		bp->b_flags &= ~TO_SET;
		}

	switch ((enum states)iob->b_state)	{
	case initial:
		iob->b_state = (int)select;
		get_selcode(bp, scsitape_fsm);
		break;

	case select:
		{
		int ret;
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
		bp->b_phase = (int)SCSI_SELECT;
		bp->b_flags |= TO_SET;
		START_TIME(scsitape_select_timedout, SELECT_TIME);
		/* If we enter scsi_if_select and discover that either the bus
		 * is not free, or select code is not owned by us, we return,
		 * expecting the interrupting process (the device reselecting
		 * the host) to drop the select code for us!
		 * If we software time out from the select something
		 * is seriously wrong.  We will escape.
		 */
		bp->b_flags &= ~CONNECTED;
		if (ret=scsi_if_select(bp))  { /* Did not Select device */
			END_TIME;
			/* 'iob->b_errcnt' limits the number of times we
			 *  attempt to select a device.
			 */
			if (iob->b_errcnt++ > 2000)	{
			   	iob->b_errcnt = 0;
			   	escape(S_RETRY_CNT);
			   	}
			if (ret == NOT_OWNER)	{
				SCSI_TRACE(STATES)("Tselect: not owner\n");
				drop_selcode(bp);
				iob->b_state = (int)initial;
				goto reenter;
				}
			}
		}
		break;

	case select_nodev:
		/* No device */
		iob->b_state =(int)defaul;
		drop_selcode(bp);
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		SCSI_TRACE(DIAGs)("scsi: no device at %x\n",bp->b_dev);
		if (bp->b_flags & ABORT_REQ)	{
			bp->b_flags &= ~ABORT_REQ;
			scsi_if_clear_reselect(bp);
			}
		bp->b_status = -1;
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
		/* Transfer timed out - Give up */
		escape(TIMED_OUT);

	case reselect:
		START_TIME(scsitape_req_timedout, MESSAGE_TIME);
		scsi_if_set_state(bp);
		END_TIME;
		bp->b_flags |= CONNECTED;	/* Mark buf as connected */
		bp->b_phase = (int)SCSI_RESELECT;
		if (bp->b_flags & ABORT_REQ)
			break;	/* Exceptional case - already got selcode */
		get_selcode(bp, scsitape_fsm);
		break;

	case set_state:
		START_TIME(scsitape_req_timedout, MESSAGE_TIME);
		scsi_if_set_state(bp);
		END_TIME;
		goto reenter;

	case phase_mesg_in:
		{
		unsigned char bfr[32], *ptr, save_len;
		char length;

		/* A corner case that must be checked if ABORT msg required */
		if (bp->b_flags & ABORT_REQ)
			bp->b_flags |= ATN_REQ;
		START_TIME(scsitape_req_timedout, MESSAGE_TIME);
		length = scsi_if_mesg_in(bp, bfr);  /* Get all message bytes */
		scsi_if_set_state(bp);
		END_TIME;
		ptr=bfr;

		/*
		** Kludge. At least one revision of the SPIFFY controller
		** can get confused due to the Fujitsu
		** ATN glitch and send a garbage byte immediately after
		** reselection.  For all other devices, this first message
		** in byte will be an IDENTIFY.  Since we don't use the
		** info anyway, we just toss the byte and continue.
		*/
		if (bp->b_phase == (int)SCSI_RESELECT) {
			length--;
			ptr++;
		}
		bp->b_phase = (int)SCSI_MESG_IN;
		while (length--) {
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
			scsi_if_wait_for_reselect(bp, scsitape_fsm);
			bp->b_flags |= TO_SET;
			START_TIME(scsitape_discon_timedout, bp->b_clock_ticks);
			break;

		    case MSGmsg_reject:
			{
			SCSI_TRACE(MESGs)("SCSI mesg in: Msg Reject\n");
			if (dp->dev_queue.b_flags & NEED_SYNC_DATA)	{
			    dp->dev_queue.b_flags &= ~NEED_SYNC_DATA;
			    iob->io_sync = 0;
			    SCSI_TRACE(MESGs)("SCSI: SDTR rejected\n");
			    }
			else
			    SCSI_TRACE(MESGs)
			      ("SCSI (dev: %x) Unknown msg reject\n",bp->b_dev);
			}
			break;

		    case MSGcmd_complete:
			{
			bp->b_flags &= ~CONNECTED;
			SCSI_TRACE(MESG_v)("SCSI mesg in: cmd complete\n");

			if (!(iob->io_sanity&io_cmd) ||
			    !(iob->io_sanity&io_stat))
				SCSI_SANITY("MSGcmd_complete");

		/* 'cur_status' is current status returned from this cmd.
		 * If we issue the 'request_sense' command we will reuse
		 * the buf header for this command and use 'save_addr' to
		 * save the buffer address from original cmd. Thus 'save_addr'
		 * is non-zero if and only if we are obtaining additional sense
		 * for the previous command.
		 */
			if (bp->b_status = iob->b_errcnt)	{
				if (iob->save_addr) {
				    /* We received bad status from 'request
				     * sense'.  Should not happen!
				     */
				    bp->b_bcount  = dp->save_cnt;
				    bp->b_flags   = dp->save_flags;
				    bp->b_un.b_addr = (char *)iob->save_addr;
				    iob->save_addr = 0;
				    msg_printf("SCSI: Bad status from sense\n");
				    escape(NO_SENSE);
				    }
				/* Retry command if we get 'check condition' */
				if (iob->b_errcnt == check_condition) {
				    /* Save information in order to retry cmd */
				    iob->save_addr = (int)bp->b_un.b_addr;
				    bp->b_un.b_addr  = (char *)&dp->xsense;
				    dp->save_cnt     = bp->b_bcount;
				    dp->save_resid   = bp->b_resid;
				    dp->save_flags   = bp->b_flags;
				    dp->save_action2 = bp->b_action2;
				    dp->save_status  = iob->b_errcnt;
				    /* Set up buf for request sense - we
				       need to send a message out */
				    bp->b_flags  |= ATN_REQ;
				    iob->b_state  = (int)select;
				    bp->b_action2 = scsi_request_sense;
				    goto reenter;
				    }
				else { /* Do not retry command */
				    bp->b_error  = EBUSY;
				    bp->b_flags |= B_ERROR;
				    }
				}
			if (iob->save_addr) { /* We requested sense */
				bp->b_un.b_addr = (char *)iob->save_addr;
				iob->save_addr = 0;
				/* Just returned from request sense - check */
				if (bp->b_action2 != scsi_request_sense)  {
				   printf("SCSI: inconsistency in sense\n");
				   }
				bp->b_action2 = dp->save_action2;
				bp->b_flags   = dp->save_flags;
				if (scsitape_decode_sense(bp, dp))	{
				    /* We're retrying command */
				    iob->b_state  = (int)select;
				    bp->b_bcount  = bp->b_resid = dp->save_cnt;
				    bp->b_flags  |= ATN_REQ;
				    goto reenter;
				    }
				/* Use previous status */
				bp->b_status = dp->save_status;
				bp->b_sensekey = (char)dp->xsense.sense_key;
				bp->b_resid   = dp->save_resid;
				bp->b_bcount  = dp->save_cnt;
				}
			}
			iob->b_state = (int)defaul;
			drop_selcode(bp);
			if (bp->b_error)	{
			    bp->b_flags |= B_ERROR;
			    SCSI_TRACE(DIAGs)("scsi: error %d\n", bp->b_error);
			    }
			queuedone(bp);
			break;

		    case MSGext_message:
			SCSI_TRACE(MESGs)("SCSI: extended mesg\n");
			if (--length < (save_len = *++ptr)) {
				length = 0;
				break;
			}
			length--;
			if (*++ptr == MSGsync_req
					&& (bp->b_newflags & SDTR_SENT)) {
			    unsigned char xfr_period, reqack;

			    /*
			    ** Compute value for Fujitsu tmod register
			    ** and wether or not to use the 10MHz input
			    ** if available
			    */

			    xfr_period = *++ptr; length--;
			    reqack     = *++ptr; length--;

			    iob->io_sync = 0;
			    if (dp->dev_queue.b_flags & NEED_SYNC_DATA
					&& reqack 
					&& xfr_period <= 157
					&& xfr_period >= 50) {
				if (reqack < 8)
					iob->io_sync = reqack << 4;
				iob->io_sync
					|= xfr_period > 157 ? 0
					:  xfr_period > 125 ? 0x8c
					:  xfr_period > 100 ? 0x88
					:  xfr_period >  94 ? 0x188
					:  xfr_period >  75 ? 0x84
					:  xfr_period >  63 ? 0x184
					:  xfr_period >  50 ? 0x80 : 0x180;
			    }

			    /* Indicate device has responded to sync data */
			    dp->dev_queue.b_flags &= ~NEED_SYNC_DATA;
			    bp->b_newflags &= ~SDTR_SENT;
			    SCSI_TRACE(MESGs)
				("SCSI: sync xfr enabled: (io_sync = 0x%x)\n",
					iob->io_sync);
			} else {
			        msg_printf("SCSI msg in: unknown xtd msg:");
				length -= save_len - 1;
				while (save_len--)
					msg_printf(" 0x%x", *ptr++);
			        msg_printf("\n");
			}
			break;
		    default:
			if(*ptr&MSGidentify) {
			    SCSI_TRACE(MESG_v)("SCSI mesg in: identify %x\n", *ptr);
			    }
			else	{
			    msg_printf("SCSI: Unknown mesg_in byte=%x\n", *ptr);
			    /* Assume parity error */
			    escape(PARITY_ERR);
			    }
			break;
		     }
		  ptr++;
		  }
		}
		if (bp->b_flags & CONNECTED)
			goto reenter;
		else
			break;

	case phase_mesg_out:
		{
		unsigned char msg[8], len=0;

		 /*
		  * Synchronous transfer negotiation is required just once
		  * after a device is opened.  Remember: it is both device
		  * and interface specific.
		  */
		switch ((int)bp->b_phase)	{
		   case SCSI_SELECT:
			/* Send Identify */
			/* We do not support units for tape drivers */
			msg[0] = MSGidentify;
			if (bp->b_flags & ATN_REQ)	{
				if (scsi_flags&DISCON_OK)
					msg[0] |= MSGident_discon;
				bp->b_flags &= ~ATN_REQ;
				}
			len = 1;
			if (dp->dev_queue.b_flags & NEED_SYNC_DATA) {
				len += scsi_if_request_sync_data(bp, &msg[len]);
				iob->io_sync = 0;
				if (len == 1)
				     {
				     dp->dev_queue.b_flags &= ~NEED_SYNC_DATA;
				     bp->b_newflags &= ~SDTR_SENT;
				     }
				else
				    bp->b_newflags |= SDTR_SENT;
				}
			break;
		   case SCSI_MESG_IN:
			if (bp->b_newflags & REJECT_SDTR) {
			    msg[0] = MSGmsg_reject;
			    bp->b_flags &= ~ATN_REQ;
			    bp->b_newflags &= ~REJECT_SDTR;
			    iob->io_sync = 0;
			    iob->b_flags |= NEED_SYNC_DATA;
			    len++;
			    break;
			}
			/* Drop through */
		   case SCSI_RESELECT:
			/* Drop through */
		   default:
			msg[0] = MSGno_op;
			bp->b_flags &= ~ATN_REQ;
			len++;
			break;
		   }

		if (bp->b_flags & ABORT_REQ)
			msg[len++] = MSGabort;

		bp->b_phase = (int)SCSI_MESG_OUT;

		/* scsi_if_mesg_out can fail under one common scenario:
		 * the driver is attempting an SDTR with a device
		 * that does not accept multi-byte message packets.
		 */
		START_TIME(scsitape_req_timedout, MESSAGE_TIME);
		if (scsi_if_mesg_out(bp, msg, len))	{
			if (dp->dev_queue.b_flags & NEED_SYNC_DATA)
				{
				dp->dev_queue.b_flags &= ~NEED_SYNC_DATA;
				bp->b_newflags &= ~SDTR_SENT;
				}
			else	{
				int i;
				msg_printf("SCSI: msg out failed (dev: 0x%x) [",
					bp->b_dev);
				for (i=0; i<len; i++)
					msg_printf("0x%x ", msg[i]);
				msg_printf("]\n");
				}
			}
		scsi_if_set_state(bp);
		END_TIME;
		}

		/* If we had to send an abort message to clear
		 * target, escape to recovery routine to process status.
		 */
		if (bp->b_flags & ABORT_REQ)	{
			bp->b_flags &= ~ABORT_REQ;
			escape(TIMED_OUT);
			}

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
		if (bp->b_action2 != scsi_request_sense)
			iob->b_flags &= ~IOB_WRITTEN;
		iob->b_xcount = bp->b_bcount;
		iob->b_xaddr  = bp->b_un.b_addr;
		SCSI_TRACE(WAITING)("tics %d\n", bp->b_clock_ticks);
		START_TIME(scsitape_req_timedout, COMMAND_TIME);
		bp->b_flags |= TO_SET;
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

		START_TIME(scsitape_req_timedout, TRANSFER_TIME);
		bp->b_flags |= TO_SET;
		if (bp->b_flags & B_READ)
		    scsi_if_transfer(bp, iob->b_xaddr, iob->b_xcount,
			bp->b_flags&B_READ, MUST_FHS);
		else
		    scsi_if_transfer(bp, iob->b_xaddr, iob->b_xcount,
			bp->b_flags&B_READ, MAX_SPEED);
		/* Variable length data requests are not interrupt driven */
		break;

	case phase_status:
		if ((iob->io_sanity&io_cmd) && (!(iob->io_sanity&io_stat)))
			iob->io_sanity |= io_stat;
		else
			SCSI_SANITY("phase_status");
		bp->b_phase = (int)SCSI_STATUS;
		bp->b_resid = bp->b_queue->b_xcount;
		START_TIME(scsitape_req_timedout, MESSAGE_TIME);
		if ((iob->b_errcnt = (char)scsi_if_status(bp)) != -1)
			scsi_if_set_state(bp);
		END_TIME;
		if (iob->b_errcnt)
			SCSI_TRACE(DIAGs)("SCSI: bad status: 0x%x\n",
				iob->b_errcnt);
		goto reenter;

	case scsi_error:
		escape(PARITY_ERR);
	default:
		bp->b_flags &= ~CONNECTED;
		msg_printf("SCSI: Unknown state: Phase %x\n", bp->b_phase);
		escape(PHASE_WRONG);
		break;
	}
	END_FSM;
	recover	{
		/*
		 * SCSI Error Recovery: No "normal" entry is ever expected
		 * It's purpose will be to leave the bus free, and clean
		 *	up the software resources,  reporting errors.
		 *	Variables upon entry:
		 *	   - escapecode
		 *		PHASE_WRONG
		 *		TIMED_OUT
		 *		S_RETRY_CNT
		 *		PARITY_ERR
		 *		SELECT_TO
		 *
		 *	Algorithm:
		 *		ABORT_TIME
		 *		drop select code
		 *		free bus (reset if necessary)
		 *		report appropriate error and
		 *		exit
		 */
		ABORT_TIME;
		/* Remove 'bp' from select code queue of buf's
		 * waiting for a reselection
		 */
		scsi_if_clear_reselect(bp);
		iob->b_state =(int)defaul;
		iob->save_addr = 0;
		/* Always clean up bus & card */
		if (scsi_if_abort(bp))
		    msg_printf("SCSI: Recover: unable to free bus (dev 0x%x)\n",
			bp->b_dev);
		drop_selcode(bp);
		bp->b_flags |= B_ERROR;
		if (escapecode == SELECT_TO || escapecode == S_RETRY_CNT)
			bp->b_error = ENXIO;
		if (!bp->b_error)
			bp->b_error = EIO;
		msg_printf("SCSI: error %d (dev 0x%x) code %d\n",
			bp->b_error, bp->b_dev, escapecode);
		bp->b_status = -1;
		queuedone(bp);
	}
}

/*
 *  scsitape_control()
 *  Description: set up activities other than standard reads/writes.
 *               knows scsi tape specific things (synchronous call to fsm)
 *
 *  Algorithm:
 *	get buf header
 *	initialize buf structure
 *	enqueue activity
 *	iowait (synchronize request)
 *	if no error then
 *		if cmd == inquiry_cmd
 *			save inquiry data
 *		check that device is sequential access
 *	release buf header
 *	return any error that occurred
 */
scsitape_control(dp, cmd, addr, count, parm, clock)
struct scsitape_od *dp;
int  (*cmd)();
char *addr;
int  count;
char parm;
{
	register struct buf *bp = (struct buf *)geteblk(SZ_VAR_CNT);
	long saved_bcount = bp->b_bcount;
	int flag=0;

retry:
	bp->b_flags |= ATN_REQ;	/* Temporarily always require ATN */
	bp->b_dev    = dp->od_device;
	bp->b_error  = 0;
	bp->b_bcount = count;	/* For motion commands, holds count */
	bp->b_parms  = parm;	/* Holds special codes  */
	bp->b_sc     = isc_table[m_selcode(bp->b_dev)];
	bp->b_ba     = m_busaddr(bp->b_dev);
	bp->b_clock_ticks = scsitape_ticks[clock];
	bp->b_action = scsitape_fsm;
	bp->b_action2 = cmd;

	if (cmd == scsi_mode_select_cmd)	/* bcopy (from, to, cnt) */
		bcopy(addr, bp->b_un.b_addr, count);

	enqueue(&dp->dev_queue, bp);
	iowait(bp);

	if (!(bp->b_flags & B_ERROR))	{
		/* Transaction OK - Save Data */
		if (cmd == scsi_inquiry)	{
			dp->inquiry_sz = bp->b_bcount-bp->b_resid;
		   }
		if (addr != NULL && cmd != scsi_mode_select_cmd)
			bcopy(bp->b_un.b_addr, addr, bp->b_bcount-bp->b_resid);
		/* Check that the device is a sequential access drive */
		if (cmd == scsi_inquiry)	{
			struct inquiry *inq = (struct inquiry *)(dp->name);
			if (inq->dev_type != 1)
				bp->b_error = ENODEV;
			else	{
				/* Determine if it is an HP device */
				if (inq->vendor_id[0] == 'H' &&
				    inq->vendor_id[1] == 'P' &&
				    inq->vendor_id[2] == ' ')
					dp->dev_queue.b_flags |=  HP;
				else
					dp->dev_queue.b_flags &= ~HP;
				}
			}
		}

	bp->b_bcount = saved_bcount;
	brelse(bp);
errout:
	return geterror(bp);
}

/*
 *  scsitape_opened_dev()
 *  Description: return pointer to per device structure
 *  Algorithm:
 *	if device already opened
 *		return open device pointer
 *	else
 *		return NULL
 */
struct scsitape_od *scsitape_opened_dev(dev)
register dev_t dev;
{
	register struct scsitape_od *p;

	dev &= M_DEVMASK;
	for (p = scsitape_odt; p < scsitape_odt + STOD; ++p)
		if (dev == p->od_device)
			return p;
	return NULL;
}


/*
 *  scsitape_open_dev()
 *  Description: allocate NEW per device entry
 *  Algorithm:
 *	misc error checking:
 *		check select code, bus address for correctness;
 *	find first unallocated per device structure
 *		return pointer
 *		  or   NULL in case of error
 */
struct scsitape_od *scsitape_open_dev(dev)
register dev_t dev;
{
	register struct scsitape_od *d;

	if (m_selcode(dev) > 31 || m_busaddr(dev) > 7)
		return NULL;

	dev &= M_DEVMASK;
	/*
	**  device already open?
	*/
	for (d = scsitape_odt; d->od_device; )
		if (++d >= scsitape_odt + STOD) {
			msg_printf("SCSI: tape device table overflow\n");
			return NULL;
			}

	d->od_device = dev;
	return d;
}

void scsitape_close_dev(dev, od_ptr)
dev_t dev;
struct scsitape_od *od_ptr;
{
	struct isc_table_type *sc = isc_table[m_selcode(dev)];

	if (od_ptr == NULL || !od_ptr->od_device)
		panic("dev_close: inconsistency");
	od_ptr->od_device = 0;
	/* Close device for command mode */
	od_ptr->cmd_mode_dev = 0;
	dma_unactive(sc);
}

#define	WSMK	0x08

scsitape_close(dev, flag)
register dev_t dev;
int flag;
{
	register struct scsitape_od *od_ptr;
	register struct iobuf *iob;
	int res1 = 0, res2 = 0, res3 = 0;

	if ((od_ptr = scsitape_opened_dev(dev)) == NULL)
		panic("scsitape_close: unopened device");

	SCSI_TRACE(DIAGs)("===\n");
	/* Get iobuf associated with device */
	iob = &od_ptr->dev_queue;

	if (iob->b_flags & UNLOADED)
		goto skip_motion_cmds;

	if (flag & FWRITE) {
		if (iob->b_flags & IOB_WRITTEN) {
			SCSI_TRACE(DIAGs)("IOB_WRITE & FWRITE\n");
			res1 = scsitape_control(od_ptr,tape_cmd_table[MTWEOF],
						NULL,2, MTWEOF, MTWEOF);
			if (dev & MT_NO_AUTO_REW)
				res2 = scsitape_control(od_ptr,
				     tape_cmd_table[MTBSR],NULL,1,MTBSR,MTBSR);
			}
		if (!(dev & MT_NO_AUTO_REW))
			res2 = scsitape_control(od_ptr, tape_cmd_table[MTREW],
					NULL, 0, MTREW, MTREW);
	} else {
		if (!(dev & MT_NO_AUTO_REW))
			res1 = scsitape_control(od_ptr, tape_cmd_table[MTREW],
						NULL, 0, MTREW, MTREW);
		else {
			if(!(dev&MT_UCB_STYLE) && !(iob->b_flags&IOB_EOF))
			  res1 = scsitape_control(od_ptr, tape_cmd_table[MTFSF],
						NULL, 1, MTFSF, MTFSF);
		}
	}

	if (dev & MT_NO_AUTO_REW)
		iob->b_flags |=  NOT_AT_BOT;
	else
		iob->b_flags &= ~NOT_AT_BOT;
		
	/* Flush controller's buffer */
/* 	res3 = scsitape_control(od_ptr,tape_cmd_table[MTWEOF],NULL,0,0,MTWEOF);
 */

skip_motion_cmds:
	if (od_ptr->flags & FLAG_OPEN_DISK)
	{
		int disk_dev = makedev(scsi_major, minor(dev) & ~0xFF);
		SCSI_TRACE(DIAGs)("Release disk lock dev = %x\n", disk_dev);
		scsi_close(disk_dev);
	}

	scsitape_close_dev(dev, od_ptr);
	return res1 || res2 || res3;
}

/*
 *  scsi_nop - control operation for synchronizing closes
 */
scsitape_nop(bp)
struct buf *bp;
{
	queuedone(bp);
}

typedef struct {
	char	length;
	char	medium_type;
	char    WP:1;
	char    buffered_mode:3;
	char    speed:4;
	char    descriptor_len;
	char	density;
	char    num_blks[3];
	char	reserved;
	char    blk_len[3];
	unsigned char	add_bytes[SZ_VAR_CNT];
} MODE_DATA;

#define LSHIFT(x,amt) (int)(x<<amt)

/*
 * scsitape_open()
 * description: primary entry point into driver from O/S
 * Algorithm:
 *	if device already opened
 *		return error
 *	else
 *		get an scsitape_od structure
 *	check interface is scsi
 *	dma_active()   (advise the system of intended dma usage by this device)
 */
scsitape_open(dev, flag)
dev_t dev;
int flag;
{
	struct scsitape_od *dp;
	register struct isc_table_type *sc;
	int length, number, err=0, select_req=0, select_cnt, DCenabled = 0;
	struct blk_limits {
		int	max_blk_len;
		short	min_blk_len;
	} limits;
	char page_code;
	MODE_DATA mode_data;
	dev_t disk_dev;
	int exclusive_flag;

	/* Exclusive Open */
	if (scsitape_opened_dev(dev))
		return EBUSY;

	if ((dp = scsitape_open_dev(dev)) == NULL )
		return ENXIO;

	/*
	 * check that we have a card and it is SCUZZY!
	 */
	if ((sc = isc_table[m_selcode(dev)]) == NULL ||
		sc->card_type != SCUZZY)
		return ENXIO;

	dma_active(sc);
	dp->dev_queue.b_flags |= NEED_SYNC_DATA;
	dp->dev_queue.b_flags &= ~(IOB_WRITTEN | IOB_EOF | 
					IOB_EOT_SEEN | DRIVE_OFFLINE);
	dp->flags = 0;
	/*
	 * Test Unit, Identify, and Read Capacity
	 *
	 * test_unit_ready has two chances of passing - a second chance is
	 * given for new media or bus_device_reset diagnostics
	 * If both fail, we test to see if drive is offline (not loaded)
	 * and will try to load tape.
	 */
	if (err=scsitape_control(dp, scsi_test_unit, NULL, 0, 0, MTNOP)) {
	    if (dp->dev_queue.b_flags & NOT_AT_BOT) {
	    		dp->dev_queue.b_flags &= ~NOT_AT_BOT;
			goto open_failed;
			}
	    if (err=scsitape_control(dp, scsi_test_unit, NULL, 0, 0, MTNOP))
		if (dp->dev_queue.b_flags & DRIVE_OFFLINE) {
	           if(err=scsitape_control(dp, scsi_load, NULL, 0, 0, MTOFFL))
		     goto open_failed;
		   }
		else
		    goto open_failed;
		}

	 dp->dev_queue.b_flags &= ~UNLOADED;
	 if (err=scsitape_control(dp, scsi_inquiry, dp->name, 0, 0, MTNOP))
		goto open_failed;
	 /* Check that it is a sequential access device */
	 if ((dp->name[0]&0x1f) != 1) { 
		err = ENODEV;
		msg_printf("SCSI: Not a sequential access device\n");
		goto open_failed;
		}

	 /* Use disk dev driver to check/gain exclusive access */
	 disk_dev = makedev(scsi_major, minor(dev) & ~0xFF);

	 SCSI_TRACE(DIAGs)("Disk lock dev = %x\n", disk_dev);
	 if (scsi_open(disk_dev) != 0) {
		err = ENXIO;
		goto open_failed;
		}

	 dp->flags |= FLAG_OPEN_DISK;

	 exclusive_flag = 1;
	 SCSI_TRACE(DIAGs)("Exclusive disk lock dev = %x\n", disk_dev);
	 if (scsi_ioctl(disk_dev, DIOC_EXCLUSIVE, &exclusive_flag, FWRITE)) {
		err = EBUSY;
		goto open_failed;
		}
	 SCSI_TRACE(DIAGs)("Disk locked out dev = %x\n", disk_dev);

	 if (err=scsitape_control(dp, scsi_read_block_limits, (char *)&limits, 
								0, 0, MTNOP))
		goto open_failed;

	 /* Setup / Disable Data Compression */
	 /* Compression must be checked before other configurations issues
	  * Algorithm:
	  * if (Mode_Sense (DC Page) failed)
	  *	*** Drive does not support DC ***
	  *	if (dev & MT_GCR_COMPRESS)
	  *	    return ERROR
	  * if (mode_data.add_bytes[2]&0xc0)
	  *    *** drive has DC enabled ***
	  *    DCenabled = TRUE
	  * if ( dev & MT_GCR_COMPRESS && !DCenabled )
	  *	*** Enable DC - set DCE bit to one ***
	  *	mode_data.add_bytes[2] |= 0xc0)
	  * 	if (Mode_Select (DC Page) failed)
	  *	    return ERROR
	  * else( !(dev & MT_GCR_COMPRESS) && DCenabled )
	  *	*** Disable DC - set DCE bit to zero ***
	  *	mode_data.add_bytes[2] &= ~0x80)
	  * 	if (Mode_Select (DC Page) failed)
	  *	    return ERROR
	  */

	 dp->flags |= FLAG_QUIET;
	 page_code = 0x0f;
	 if  (err=scsitape_control(dp, scsi_mode_sense_cmd,
			(char *)&mode_data, 0, page_code, 12)) {
	     dp->flags &= ~FLAG_QUIET;
	     /* Device does not support compression page */
	     if ((dev & MT_DENSITY_MASK) == MT_GCR_COMPRESS) {
		msg_printf("SCSITAPE: DC Not supported (dev 0x%x)\n", dev);
		goto open_failed;
		}
	     else
		goto post_compression;
	     }
	 dp->flags &= ~FLAG_QUIET;
	 if (mode_data.add_bytes[2]&0x80)
	 	DCenabled++;
	 if ((dev & MT_DENSITY_MASK) == MT_GCR_COMPRESS && 
			!(mode_data.add_bytes[2]&0x40) ) {
		msg_printf("SCSITAPE: DC Unsupported (dev 0x%x)\n", dev);
		err = EINVAL;
		goto open_failed;
		}
	 if ((dev & MT_DENSITY_MASK) == MT_GCR_COMPRESS && !DCenabled ) {
	  	mode_data.add_bytes[2] |= 0xc0;
		select_req++;
		SCSI_TRACE(DIAGs)("Tape: Enabling DC Compression\n");
	 }
	 if ((dev & MT_DENSITY_MASK) != MT_GCR_COMPRESS && DCenabled ) {
	  	mode_data.add_bytes[2] &= ~0x80;
		select_req++;
		SCSI_TRACE(DIAGs)("Tape: Disabling DC Compression\n");
	 }
	 mode_data.length  = 0;	/* Mode Data Len must be zero */
	 select_cnt=28;
	 if (select_req && (err=scsitape_control(dp, scsi_mode_select_cmd,
			(char *)&mode_data, select_cnt, page_code, 13)))
		goto open_failed;

post_compression:
	 select_req = 0;
	 page_code = dp->dev_queue.b_flags & HP ? 0x10 : 0;
	 if  (err=scsitape_control(dp, scsi_mode_sense_cmd,
				(char *)&mode_data, 0, page_code, 12))
		goto open_failed;

	/*
	 * Configure device according to bits in minor number:
	 * Variable mode is default
	 * A sequence of decisions are made based on these issues:
	 *	block vs. variable mode
	 *		(variable is default, however if device comes up in blk,
	 *		 and minor bit is set, allow for block mode)
	 *	buffered vs. non-buffered
	 *	partition requested (0 or 1)
	 */
	number = LSHIFT(mode_data.num_blks[2], 0) +
	         LSHIFT(mode_data.num_blks[1], 8) +
		 LSHIFT(mode_data.num_blks[0],16);
	length = LSHIFT(mode_data.blk_len[2], 0) +
	         LSHIFT(mode_data.blk_len[1], 8) +
		 LSHIFT(mode_data.blk_len[0],16);

	mode_data.length         = 0;
	mode_data.medium_type    = 0;
	mode_data.WP             = 0;
	mode_data.speed          = 0;
	mode_data.descriptor_len = 8;
	mode_data.num_blks[0]    = 0;
	mode_data.num_blks[1]    = 0;
	mode_data.num_blks[2]    = 0;
	mode_data.reserved       = 0;

	/* If blkssz_log2 is non-zero, we will assume block mode */
	dp->dev_queue.blkssz_log2 = 0;

	if ((dev & MT_DENSITY_MASK) == 0x80)	{
		int bytes_per_block, block_shift = 0;
		if (!length) {
		/* If user requests block mode, and device indicates variable
		 * mode (length == 0), set to 256 bytes/record (for Bootrom)
		 */
		    if (dp->dev_queue.b_flags & NOT_AT_BOT) {
			err = ENXIO;
			goto open_failed;
			}
		    mode_data.blk_len[2] = 0;
		    mode_data.blk_len[1] = 1;
		    mode_data.blk_len[0] = 0;
		    length = 0x100;
		    select_req++; select_cnt=12;
		    }
		bytes_per_block = length;
		while (bytes_per_block >>= 1)
			block_shift++;
		dp->dev_queue.blkssz_log2 = block_shift;
		SCSI_TRACE(DIAGs)("Blk Mode: %x\n", dp->dev_queue.blkssz_log2);
		}
	else if (length) {
		mode_data.blk_len[0] = 0;
		mode_data.blk_len[1] = 0;
		mode_data.blk_len[2] = 0;
		select_req++;
		select_cnt=12;
		}
	if (dev & MT_SYNC_MODE || mode_data.buffered_mode == 0) {
		mode_data.buffered_mode = dev & MT_SYNC_MODE ? 0 : 1;
		select_req++;
		select_cnt=12;
		}

	/* Select proper partition */
	if (dp->dev_queue.b_flags & HP &&
		mode_data.add_bytes[3] != (dev>>4 & 1))	{
		int k;

		for (k=4; k<16; k++)
			mode_data.add_bytes[k] = 0;
		mode_data.add_bytes[0]     = 0x10; /* dev config page */
		mode_data.add_bytes[1]     = 0x0E; /* add'l bytes     */
		mode_data.add_bytes[2]     = 0x40; /* Chge partition  */
		mode_data.add_bytes[3]     = dev & 0x10 ? 1 : 0;
		mode_data.add_bytes[7]     = 0x32;
		mode_data.add_bytes[8]     = 0x20;
		mode_data.add_bytes[10]    = 0x08;
		select_req++;
		select_cnt=28;
	}

	if (select_req && (err=scsitape_control(dp, scsi_mode_select_cmd,
			(char *)&mode_data, select_cnt, 0x10, 13)))
		goto open_failed;

if (SCSI_DEBUG&DIAGs)	{
	int i;

	msg_printf("block limits: max blk len %d  min blk len %d\n",
		limits.max_blk_len&0xffffff, limits.min_blk_len);

	msg_printf("mode sense data: ");
	if (mode_data.medium_type&0x80)
		msg_printf("VU: ");
	msg_printf("medium_type: %d; ", mode_data.medium_type&0x7F);
	msg_printf("WP: %d buffered: %d speed: %d\n",
		mode_data.WP, mode_data.buffered_mode,
		mode_data.speed);
	msg_printf("descriptor_len %d; density: %d\n",
		mode_data.descriptor_len, mode_data.density);
	msg_printf("number %d  length %d\n", number, length);
	if (mode_data.length - 12 > 0)	{
		msg_printf("add bytes: ");
		for (i=0; i<=mode_data.length-12; i++)
			msg_printf("%x ", mode_data.add_bytes[i]);
		msg_printf("\n");
		}
	msg_printf("---\n");
	}

	goto out;

open_failed:
	/* Suppress any tape close action since open failed */
	dp->dev_queue.b_flags |= UNLOADED;
	scsitape_close(dev);
out:
	return err;
}

scsitape_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
	struct scsitape_od *dp;
	register int err=0;
	int i, blksize=1;

	if ((dp = scsitape_opened_dev(dev)) == NULL)
		panic("scsi_ioctl: unopened device");

	switch	(order)	{
		case SIOC_INQUIRY:
			bcopy(dp->name,addr,dp->inquiry_sz);
			break;
		case SIOC_CMD_MODE:
			/* CMD_MODE requires super-user permission */
		  	if (!suser()) {
				err = EPERM;
				break;
			}
			/* If addr is non-zero, turn on CMD_MODE */
			if (*(int *)addr) {
				dp->cmd_mode_dev = dev;
			} else { /* else addr is zero, turn off CMD_MODE */
				dp->cmd_mode_dev = 0;
			}
			break;
		case SIOC_SET_CMD:
		   i = (int)((struct scsi_cmd_parms *)addr)->cmd_type;
		   err = dp->cmd_mode_dev != dev ? EACCES :
			(i != 6 && i != 10 && i != 12) ?
				       EINVAL : 0;
		   if (!err)
		     bcopy(addr, &dp->cmd_parms, sizeof(struct scsi_cmd_parms));
		   break;
		case SIOC_RESET_BUS:
		   err = (dp->cmd_mode_dev != dev) ? EACCES :
			    scsi_if_reset_bus(isc_table[m_selcode(dev)]);
		   break;
		case SIOC_RETURN_STATUS:
		   err = (dp->cmd_mode_dev != dev) ? EPERM : 0;
		   if (!err)
			    bcopy(&(dp->last_status),addr,sizeof(int));
		   break;
		case SIOC_XSENSE:
			bcopy(&dp->xsense, addr, sizeof(struct xsense));
			((char *)&dp->xsense)[0] = 0;
		   break;

		case MTIOCTOP:	/* control operation */
			{
			struct mtop *mtop = (struct mtop *)addr;
			register unsigned int op = mtop->mt_op;
			register long count;

			/* Algorithm for converting HPIB opcodes into SCSI
			 * commands:  two table are required:
			 *	tape_cmd_table[] to look up the command;
			 *	scsi_tape_ticks[] to compute proper timeout.
			 * 'count' is passed directly to commands that use it.
			 * 'op' is used by SPACE cmd and WRITE_FILEMARK to
			 * decode type of tape mark;
			 * Backspace cmds are converted to negatives (2's comp)
			 */
			if (op > MTBSS)
				return ENXIO;
			count = mtop->mt_count;
			if (op == MTREW)
				dp->dev_queue.b_flags &= ~NOT_AT_BOT;
			if (op == MTNOP)
				count = 0;
			if (op == MTNOP && !(dp->dev_queue.b_flags & HP))

			    err = scsitape_control(dp, space, NULL,
			                           count, MTFSF, MTFSF);
			else
			    err = scsitape_control(dp, tape_cmd_table[op], NULL,
						   count, op, op);
			break;
			}

		case MTIOCGET:
			{
			/* Caveat: the status registers defined for this
			 * call are HPIB specific.  An attempt is made to
			 * have these registers accurate - where possible.
			 * 'gstat' reg is used for generic status (cf. mtio.h)
			 */
			struct mtget *mtget = (struct mtget *)addr;
			struct xsense xsense;
			int err, i, write_protect, online = 1, mask=0xffffffff;
			MODE_DATA mode_sense_data;

			/* Construct status registers 1 and 2 manually */

			/* Determine tape position from sense data
			 * (and mt_resid field from info bytes)
			 */
			err=scsitape_control(dp, scsi_request_sense,
					&xsense, 0, 0, MTNOP);

		        SCSI_TRACE(DIAGs)
			   ("MTIOCGET: sense_key 0x%x code 0x%x qual 0x%x\n",
			      xsense.sense_key, xsense.sense_code, xsense.resv);

			/* If test_unit_ready returns good status,
			 * unit is online
			 */
			if (err=scsitape_control(dp, scsi_test_unit, 
							NULL, 0, 0, MTNOP))
				online = 0;

			/* Determine whether media is write-protected */
			if (dp->dev_queue.b_flags & HP)
			    err=scsitape_control(dp, scsi_mode_sense_cmd,
				(char *)&mode_sense_data, 0,
				    dp->dev_queue.b_flags & HP ? 0x3f : 0, 12);
			mtget->mt_type = MT_ISDDS2;
			if (dp->od_resid >= 0)
				mtget->mt_resid = 0;
			else
				mtget->mt_resid = -dp->od_resid;

			write_protect = (int)mode_sense_data.WP;
			mtget->mt_dsreg1 = online | (write_protect<<2);

/* HPIB Status Register Definitions */
#define	EOT	0x20	/* Passed Logical End of Tape */
#define	BOT	0x40	/* Beginning of Tape          */
#define	EOF	0x80	/* Filemark Detected          */
#define	LRS	0x2	/* Long records supported */

			/* Set up 'gstat' register for generic tape status */
			mtget->mt_gstat = 0;

			/* GMT_DR_OPEN(mask) -> Unknown field */
			if (!(dev & MT_SYNC_MODE))
				mtget->mt_gstat  |= GMT_IM_REP_EN(mask);
			if (online)
				mtget->mt_gstat  |= GMT_ONLINE(mask);
			if (write_protect)
				mtget->mt_gstat  |= GMT_WR_PROT(mask);
			if (xsense.parms&FM && xsense.sense_key == 0 &&
			    			xsense.sense_code == 0)	{
			    if (xsense.resv == 1)
				mtget->mt_dsreg1 |= EOF,
				mtget->mt_gstat  |= GMT_EOF(mask);
			    else if (xsense.resv == 3)
				mtget->mt_gstat  |= GMT_SM(mask);
				}

			if (xsense.sense_key == 0 && xsense.sense_code == 0 && 
				xsense.resv == 4) {
					mtget->mt_dsreg1 |= BOT,
					mtget->mt_gstat  |= GMT_BOT(mask);
				}

			/* A Blank Check is returned after a read operation at
			 * EOD,  while an unsolicited request sense will
			 * return NO_SENSE
			 */
			if (xsense.sense_key == 8 || (xsense.sense_key == 0 &&
			    xsense.sense_code == 0 && xsense.resv == 5)) {
					mtget->mt_gstat  |= GMT_EOD(mask);
				}

			if (xsense.sense_key == 0 && xsense.sense_code == 0 && 
				xsense.resv == 2) {
					mtget->mt_dsreg1 |= EOT;
					mtget->mt_gstat  |= GMT_EOT(mask);
					}

			mtget->mt_dsreg2 = LRS;

			mtget->mt_erreg  = 0;
			/* Future writes beyond EOT will be permitted */
			if (dp->dev_queue.b_flags & IOB_EOT_SEEN)
			    dp->flags |= FLAG_EOT_ACK;
		        SCSI_TRACE(DIAGs)
			   ("stat regs: gstat 0x%x dsreg1 0x%x dsreg2 0x%x\n",
			   mtget->mt_gstat, mtget->mt_dsreg1, mtget->mt_dsreg2);
			break;
			}

		default:
			err = EINVAL;
			break;
	}
	return err;
}

scsitape_strategy(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;
	struct scsitape_od *dp;
	int err=0;

	if ((dp = scsitape_opened_dev(dev)) == NULL)
		panic("scsitape_strategy: unopened device");

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_clock_ticks = 5*TRANSFER_TIME;	/* Hard coded */
	bp->b_action  = scsitape_fsm;

	/*
	 *  standard read/write
	 */
	if (dev != dp->cmd_mode_dev)	{
		if ((dp->dev_queue.b_flags & IOB_EOT_SEEN) &&
		    !(dp->flags & FLAG_EOT_ACK) &&
		    !(bp->b_flags & B_READ)) {
			err++;
			bp->b_error = ENOSPC;
			bp->b_resid = bp->b_bcount;
			goto error_out;
			}
		bp->b_flags |= ATN_REQ;		/* Allow target to disconnect */
		bp->b_action2 = scsitape_xfr;
		dp->od_resid = 0;
		if (dp->dev_queue.blkssz_log2) /* Check request size */
			if (bp->b_bcount & ((1<<dp->dev_queue.blkssz_log2)-1)) {
				err++;
				bp->b_error = ENXIO;
				goto error_out;
				}
		}
	else	{
		if (dp->cmd_parms.cmd_mode)
			bp->b_flags |= ATN_REQ; /* Allow target to disconnect */
		else
			bp->b_flags &= ~ATN_REQ;
		bp->b_action2 = scsitape_cmd;
		bp->b_resid = bp->b_bcount;
		}

error_out:
	if (err) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
		}
	enqueue(&dp->dev_queue, bp);
}

scsitape_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(scsitape_strategy, NULL, dev, B_WRITE, minphys, uio);
}

scsitape_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return physio(scsitape_strategy, NULL, dev, B_READ,  minphys, uio);
}
