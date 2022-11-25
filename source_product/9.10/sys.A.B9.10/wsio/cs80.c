/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/cs80.c,v $
 * $Revision: 1.3.83.12 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/16 10:06:28 $
 */

/* HPUX_ID: @(#)cs80.c	55.1		88/12/23 */

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
/* defines for the front panel led lights */
#define led0            0x01
#define led1            0x02
#define led2            0x04
#define led3            0x08
#define KERN_OK_LED     0x10
#define DISK_DRV_LED    0x20
#define LAN_RCV_LED     0x40
#define LAN_XMIT_LED    0x80



/* 
**  cs80 device driver 
*/

#define CS80ERRLOG  /* comment to turn off error logging */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../h/buf.h"
#include "../wsio/iobuf.h"
#include "../wsio/cs80.h"
#include "../wsio/hshpib.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../wsio/tryrec.h"
#ifdef __hp9000s300
#include "../s200io/bootrom.h"
#endif /* __hp9000s300 */
#include "../h/dk.h"

#ifdef __hp9000s700
#include "../h.800/uio.h"
#endif

#include "../h/diskio.h"


/*
** mnemonic names for buf scratch registers
*/
#define b_log2blk		b_s0	/* (char) */
#define b_TO_state		b_s1	/* (char) */
#define b_exec_msg_ticks	b_s2	/* (long) */
#define b_dk_index		b_s3	/* (char) */
#define b_hpibflags		b_s5	/* (char ) */

#define B_HPIBMUST_INTR		0x1	/* for b_hpibflags */

/*
** defines for the upper half of BUF's b_flags
*/
#define B_PPOLL_INT_TIMEDOUT	B_SCRACH1
#define B_RETRYING_STATUS	B_SCRACH2
#define B_RECONFIGURE_NEEDED	B_SCRACH3
#define B_RETRY_REQUIRED	B_SCRACH4
#define B_CANCEL_FAILED		B_SCRACH5

/*
 * defines for the upper half of IOBUF's b_flags
  */
#define B_MEDIA_CHANGE		B_SCRACH1

/*
** timeout and retry parameters
*/
#define SHORT_MSG_TIME		((   750 * HZ) / 1000)
#define STATUS_TIME		((300000 * HZ) / 1000)
#define RELEASED_TIME		((300000 * HZ) / 1000)
#define MAX_EXEC_MSG_TIME	((300000 * HZ) / 1000)
#define RECONFIGURE_TIME	((  5000 * HZ) / 1000)
#define CANCEL_TIME		((300000 * HZ) / 1000)
#define MAXTRIES 3

/*
**  disc profiling instrumentation
*/
#define DK_DEVT_MASK	0x00fffff0	/* all minor fields except volume */
#define DK_DEVT_MAJ	0xff000000	/* major field */

/*
**  open unit info - linked one per open unit on an open device table entry
*/
struct cs80_ou {
	struct cs80_ou *link;		/* next open unit in list */
	unsigned char uv_o_count[8];	/* unit/vol open count */
	char dk_index;			/* index for profiling disc activity */
	unsigned char unit;		/* unit number */
	unsigned char dev_type;		/* device type field from describe */
	char log2blk;			/* unit block size */
	long command_ticks;		/* R/W C-phase ticks */
	long block_ticks;		/* R/W E-phase "per block" component */
	long extra_ticks;		/* R/W E-phase fixed component */
	long volsize[8];		/* volume size in blocks */
	short ident;			/* HPIB identify data for device */
};

/*
**  open device table entry
*/
struct cs80_od {
	struct iobuf dev_queue;		/* queue head; MUST BE FIRST FIELD */
	dev_t device;			/* selcode/busaddr */
	short dev_o_count;		/* device open count */
	struct cs80_ou *unit_list;	/* open unit list */
	struct cs_status_type status;	/* cs/80 status bytes */
	dev_t cmd_mode_dev;		/* unit/vol with command mode access */
	struct cmd_parms cmd_parms;	/* ioctl command paramters */
};

/*
**  cs80 open device table
*/
#define	ODS	16			/* number of open device entries */
struct cs80_od cs80_odt[ODS]={0};		/* open device table */

/*
**  cs80 open unit table
*/
#define	OUS	16			/* number of open unit entries */
struct cs80_ou cs80_out[OUS]={0};		/* open unit table */


static struct status_mask_type my_status_mask = {
	/* reject errors field */
	status_mask_0,
	/* fault errors field */
	status_mask_1,
	/* access errors field */
	status_mask_2,
	/* information errors field */
	status_mask_3
};


#ifdef CS80ERRLOG

char *error_bit_names[] = {

	/* reject errors field */
	"error_bit_0",
	"error_bit_1",
	"channel_parity_error",
	"error_bit_3",
	"error_bit_4",
	"illegal_opcode",
	"module_addressing",
	"address_bounds",
	"parameter_bounds",
	"illegal_parameter",
	"message_sequence",
	"error_bit_11",
	"message_length",
	"error_bit_13",
	"error_bit_14",
	"error_bit_15",
	"error_bit_16",
	"cross_unit",
	"error_bit_18",
	"controller_fault",
	"error_bit_20",
	"error_bit_21",
	"unit_fault",
	"error_bit_23",
	"diagnostic_result",
	"error_bit_25",
	"operator_release_required",
	"diagnostic_release_required",
	"internal_maint_required",
	"error_bit_29",
	"power_fail",
	"retransmit",

	/* access errors field */
	"illegal_parallel_operation",
	"uninitialized_media",
	"no_spares_available",
	"not_ready",
	"write_protect",
	"no_data_found",
	"error_bit_38",
	"error_bit_39",
	"unrecovb_data_overflow",
	"unrecovb_data",
	"error_bit_42",
	"end_of_file",
	"end_of_volume",
	"error_bit_45",
	"error_bit_46",
	"error_bit_47",

	/* information errors field */
	"operator_request",
	"diagnostic_request",
	"intnl_maint_reqst",
	"media_wear",
	"latency_induced",
	"error_bit_53",
	"error_bit_54",
	"auto_sparing_invoked",
	"error_bit_56",
	"recovb_data_overflow",
	"marginal_data",
	"recovb_data",
	"error_bit_60",
	"maint_track_overflow",
	"error_bit_62",
	"error_bit_63",

	/* pseudo error bit for indicating parameter field owned by no one */
	"none"
};

#endif

/*
**  Miscellaneous routines
*/

void cs80_b_error_escape(bp, error)
register struct buf *bp;
{
	bp->b_error = error;
	escape(1);
}


/*
**  Multi-purpose message-oriented routines
*/

void cs80_ICc(bp, cmd)
register struct buf *bp;
CMD_type cmd;
{
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
		(char *)&cmd, sizeof cmd);
}


#define UCSIZE1	3	/* see uc in next two procs */
void cs80_ICuvc(bp, cmd)
register struct buf *bp;
CMD_type cmd;
{
      struct {
              CMD_type setunit;
              CMD_type setvol;
              CMD_type cmd;
      } uc;

      uc.setunit = m_unit(bp->b_dev)   | UNIT_BASE;
      uc.setvol  = m_volume(bp->b_dev) | VOLUME_BASE;
      uc.cmd = cmd;
      (*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
              (char *)&uc, UCSIZE1);
}


#define UCSIZE2	2	/* see uc in next two procs */
void cs80_ICuc(bp, unit, cmd)
register struct buf *bp;
unsigned char unit;
CMD_type cmd;
{
	struct {
		CMD_type setunit;
		CMD_type cmd;
	} uc;

	uc.setunit = unit | UNIT_BASE;
	uc.cmd = cmd;
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
		(char *)&uc, UCSIZE2);
}
	

void cs80_ICux(bp, trans_cmd)
register struct buf *bp;
CMD_type trans_cmd;
{
	struct {
		CMD_type setunit;
		CMD_type trans_cmd;
	} uc;

	uc.setunit = m_unit(bp->b_dev) | UNIT_BASE;
	uc.trans_cmd = trans_cmd;
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, TRANSPARENT_SEC,
		(char *)&uc, UCSIZE2);
}

#define BYTE0(x)	(((x)&0xff)&(0xff))
#define BYTE1(x)	((((x)&0xff00)>>8)&(0xff))
#define BYTE2(x)	((((x)&0xff0000)>>16)&(0xff))
#define BYTE3(x)	((((x)&0xff000000)>>24)&(0xff))
	
#ifdef __hp9000s700
#define UVCALCSIZE	15	/* see define of uvcalc */
#endif

void cs80_ICuvalc(bp, address, len, cmd)
register struct buf *bp;
daddr_t address;
unsigned len;
CMD_type cmd;
{
#ifdef __hp9000s700

	unsigned char buvalc[UVCALCSIZE];
	buvalc[0]=m_unit(bp->b_dev) | UNIT_BASE;	/* set unit */
	buvalc[1]=m_volume(bp->b_dev) | VOLUME_BASE;	/* set vol */
	buvalc[2]=CMDset_address_1V;	/* set addr */
	buvalc[3]=0;	/* upper two bytes of addr */
	buvalc[4]=0;	/* upper two bytes of addr */
	buvalc[5]=BYTE3(address);	/* byte 3 of addr */
	buvalc[6]=BYTE2(address);	/* byte 2 of addr */
	buvalc[7]=BYTE1(address);	/* byte 1 of addr */
	buvalc[8]=BYTE0(address);	/* byte 0 of addr */
	buvalc[9]= CMDset_length;	/* set length */
	buvalc[10]= BYTE3(len);		/* length byte 3 */
	buvalc[11]= BYTE2(len);		/* length byte 2 */
	buvalc[12]= BYTE1(len);		/* length byte 1 */
	buvalc[13]= BYTE0(len);		/* length byte 0 */
	buvalc[14] = cmd;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC, 
		(char *)&buvalc[0], UVCALCSIZE);
#else

	struct {
		char dummy;	/* not sent! */
		CMD_type setunit;
		CMD_type setvol;
		CMD_type setadd;
		struct sva_type sva;
		CMD_type nop;
		CMD_type setlen;
		unsigned length;
		CMD_type cmd;
	} uvalc;

	uvalc.setunit = m_unit(bp->b_dev) | UNIT_BASE;
	uvalc.setvol  = m_volume(bp->b_dev) | VOLUME_BASE;
	uvalc.setadd  = CMDset_address_1V;
	uvalc.sva.utb = 0;
	uvalc.sva.lfb = address;
	uvalc.nop     = CMDno_op;
	uvalc.setlen  = CMDset_length;
	uvalc.length  = len;
	uvalc.cmd     = cmd;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
		(char *)&uvalc.setunit, sizeof(uvalc)-2);

#endif
}


/*
**  Specific-purpose message-oriented routines
*/

int cs80_qstat(bp)
register struct buf *bp;
{
	char qstat_byte;

	(*bp->b_sc->iosw->iod_mesg)(bp, 0, REPORTING_SEC,
		&qstat_byte, sizeof qstat_byte);
	return qstat_byte;
}


#define	CSSTATUSSIZE	20	/* see CS80 spec */
void cs80_status_exec(bp)
register struct buf *bp;
{
	char old_error = bp->b_error;
	try
		(*bp->b_sc->iosw->iod_mesg)(bp, 0, EXECUTION_SEC,
			(char *)&((struct cs80_od *)bp->b_queue)->status,
			CSSTATUSSIZE);
	recover  /* from premature eoi's, but not timeouts */
		if (escapecode == TIMED_OUT)
			escape(escapecode);
	bp->b_error = old_error;
}



/*
**  Specific-purpose routines suitable as b_action2 in cs80_transfer
**
**    value returned indicates whether execution message is needed or not
*/

#define NEED_EXCUTION_MESSAGE		1
#define NEED_NO_EXCUTION_MESSAGE	0


int cs80_unload_cmd(bp)
register struct buf *bp;
{
	cs80_ICuc(bp, m_unit(bp->b_dev), CMDunload);
	return NEED_NO_EXCUTION_MESSAGE;
}


int cs80_transfer_cmd(bp)
register struct buf *bp;
{
	cs80_ICuvalc(bp,
		bp->b_un2.b_sectno,
		bp->b_bcount,
		(bp->b_flags & B_READ) ? CMDlocate_and_read :
					 CMDlocate_and_write);
	return NEED_EXCUTION_MESSAGE;
}


int cs80_cmd(bp)
register struct buf *bp;
{
	register struct cmd_parms *cpp = &((struct cs80_od *)bp->b_queue)->cmd_parms;
	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC, 
		cpp->cmd_message, cpp->cmd_length);
	bp->b_clock_ticks = cpp->cmd_ticks;
	bp->b_exec_msg_ticks = cpp->exec_ticks;
	return bp->b_bcount ? NEED_EXCUTION_MESSAGE : NEED_NO_EXCUTION_MESSAGE;
}


int cs80_verify_cmd(bp)
register struct buf *bp;
{
	struct verify_parms *vpp = (struct verify_parms *)bp->b_un.b_addr;

	cs80_ICuvalc(bp,
		vpp->start >> bp->b_log2blk,
		vpp->length,
		CMDlocate_and_verify);
	return NEED_NO_EXCUTION_MESSAGE;
}


int cs80_mark_cmd(bp)
register struct buf *bp;
{
	struct mark_parms *mpp = (struct mark_parms *)bp->b_un.b_addr;

	cs80_ICuvalc(bp,
		mpp->start >> bp->b_log2blk,
		0,
		CMDwrite_file_mark);
	return NEED_NO_EXCUTION_MESSAGE;
}


int cs80_describe_cmd(bp)
register struct buf *bp;
{
	cs80_ICuvc(bp, CMDdescribe);
	bp->b_flags |= B_READ;		/* execution message directon */
	bp->b_bcount = DESCRIBE_BYTES;  /* execution message byte count */
	return NEED_EXCUTION_MESSAGE;
}


int cs80_cancel_cmd(bp)
struct buf *bp;
{
	cs80_ICux(bp, XCMDcancel);
	return NEED_NO_EXCUTION_MESSAGE;
}


#define	SRSIZE	3	/* see next routine */
int cs80_set_release_cmd(bp)
register struct buf *bp;
{
  	struct {
  		CMD_type setunit;
  		CMD_type setrel;
  		char     option;
  	} sr;
  
	sr.setunit = 15 | UNIT_BASE;
	sr.setrel  = CMDset_release;
	sr.option  = 0;  /* power-up default's for T & Z */

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
						(char *)&sr, SRSIZE);
	return NEED_NO_EXCUTION_MESSAGE;
}


#define	SOSIZE	3	/* see next proc */
int cs80_set_options_cmd(bp)
register struct buf *bp;
{
  	struct {
  		CMD_type setunit;
  		CMD_type setopt;
  		char     option;
  	} so;
  
	so.setunit = m_unit(bp->b_dev) | UNIT_BASE;
	so.setopt  = CMDset_options;
	so.option  = *(char *)bp->b_un.b_addr;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
						(char *)&so, SOSIZE);
	return NEED_NO_EXCUTION_MESSAGE;
}


#define LSIZE	4	/* see next proc */
int cs80_load_cmd(bp)
register struct buf *bp;
{
  	struct {
  		CMD_type setunit;
  		CMD_type load;
		char	 p_count;
  		char     parm1;
  	} l;
  
	l.setunit = m_unit(bp->b_dev) | UNIT_BASE;
	l.load    = 0x4B;
	l.p_count = 1;
	l.parm1   = *(char *)bp->b_un.b_addr;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
						(char *)&l, LSIZE);
	return NEED_NO_EXCUTION_MESSAGE;
}


#define	SSMSIZE	10	/* see next proc */
int cs80_set_status_mask_cmd(bp)
register struct buf *bp;
{
	struct {
  		CMD_type setunit;
		CMD_type setstsmsk;
		struct status_mask_type stsmsk;
	} ssm;

	ssm.setunit   = m_unit(bp->b_dev) | UNIT_BASE;
	ssm.setstsmsk = CMDset_status_mask;
	ssm.stsmsk    = my_status_mask;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
		(char *)&ssm, SSMSIZE);
	return NEED_NO_EXCUTION_MESSAGE;
}


int cs80_chan_indep_clr_cmd(bp)
struct buf *bp;
{
	cs80_ICux(bp, XCMDchan_indep_clear);
	return NEED_NO_EXCUTION_MESSAGE;
}



#ifdef __hp9000s300
/*
 * temporary solution for 8.0 Software Distribution to get ROMBO's ( C1707A )
 * security number until 9.0
 */

int
cs80_get_security_no_cmd(bp)
struct buf	*bp;
{
#define	GET_ROMBO_SN	0x42

	struct {
		CMD_type	setunit;
		CMD_type	setvol;
		CMD_type	cmd;
		CMD_type	parm;
	} uc;

	/* initialize cmd_parm as in Stuart Bobb's routine */
	uc.setunit	= m_unit(bp->b_dev) | UNIT_BASE;
	uc.setvol	= m_volume(bp->b_dev) | VOLUME_BASE;
	uc.cmd		= CMDinit_util_SEM;
	uc.parm		= GET_ROMBO_SN;

	(*bp->b_sc->iosw->iod_mesg)(bp, T_WRITE+T_EOI, COMMAND_SEC,
		(char *)&uc, sizeof uc);
	bp->b_flags |= B_READ;
	bp->b_bcount = 8;	/* eight bytes data */
	bp->b_clock_ticks = 5 * HZ;
	bp->b_exec_msg_ticks = 5 * HZ;
	return NEED_EXCUTION_MESSAGE;
}

#endif  /* __hp9000s300 */


/*
**  Routine for decoding status;  returns unit requesting release (if any)
*/
int cs80_decode_status(bp)
register struct buf *bp;
{
	struct iobuf *iob = bp->b_queue;
	struct cs_status_type *status_ptr = &((struct cs80_od *)iob)->status;

	char current_error = 0;
	char parameter_field_owner = NO_ERRORS, errors[8], n=0;

	int eb_scan;

	for (eb_scan = 63; eb_scan >= 0; eb_scan--)
	   if (status_ptr->errorbits.s_m[eb_scan/16] & (0x8000>>(eb_scan%16))) {
			if (n<7) errors[n++] = eb_scan;
			switch (eb_scan) {

			case cross_unit:
				parameter_field_owner = eb_scan;
			case channel_parity_error:
			case illegal_parallel_operation:
			case no_spares_available:
			case no_data_found:
				current_error = EIO;
				break;
			
			case illegal_opcode:
			case parameter_bounds:
			case illegal_parameter:
				current_error = EINVAL;
				break;
			
			case module_addressing:
			case address_bounds:
			case not_ready:
			case end_of_volume:
				current_error = ENXIO;
				break;
			
			case end_of_file:
				/* extra byte with eoi doesn't count */
				--iob->b_xcount;
				break;

			case write_protect:
				current_error = EACCES;
				break;
			
			case diagnostic_result:
			case unrecovb_data:
				parameter_field_owner = eb_scan;
			case unrecovb_data_overflow:
			case message_sequence:
			case controller_fault:
			case unit_fault:
			case message_length:
				bp->b_flags |= B_RETRY_REQUIRED;
				break;

			case power_fail:
				if (bp->b_queue->b_flags&B_MEDIA_CHANGE)
					current_error = EINTR;
				else
					bp->b_flags |= B_RECONFIGURE_NEEDED;
			case operator_release_required:
			case diagnostic_release_required:
			case internal_maint_required:
			case retransmit:
			case uninitialized_media: 
				bp->b_flags |= B_RETRY_REQUIRED;
				break;

			case operator_request:
			case diagnostic_request:
			case intnl_maint_reqst:
				parameter_field_owner = eb_scan;
				break;

			/* all should have been masked out */
			case marginal_data:
			case recovb_data:
			case error_bit_53:  /* may own param field */
			case error_bit_54:  /* may own param field */
			case error_bit_56:  /* may own param field */
			case error_bit_60:  /* may own param field */
			case error_bit_62:  /* may own param field */
			case error_bit_63:  /* may own param field */
				parameter_field_owner = eb_scan;
			case media_wear:
			case latency_induced:
			case auto_sparing_invoked:
			case recovb_data_overflow:
			case maint_track_overflow:
			/* ???? add error logging here some day */
				bp->b_flags |= B_RECONFIGURE_NEEDED;
				break;

			default:
				parameter_field_owner = eb_scan;
				current_error = EIO;

			}  /* switch */
		}

#ifdef CS80ERRLOG
	/* We want to printf all diagnostics except media replacement */
	if (n != 1 || errors[0] != 30)	{
	msg_printf("\ncs80 status: major %d minor 0x%06x; \n",
		major(bp->b_dev), minor(bp->b_dev)); 
	msg_printf("  id field - vol: %d  unit: %d  other unit: %d\n",
		status_ptr->vvvvuuuu >> 4, status_ptr->vvvvuuuu & 0x0F,
		status_ptr->requesting_unit);
	msg_printf("  error bit field -");
	while (n)
			msg_printf(" %s", error_bit_names[errors[--n]]);
	msg_printf("\n");
	msg_printf("  parameter field - %x %x %x %x %x %x %x %x %x %x\n",
		status_ptr->st_u.info[0], status_ptr->st_u.info[1],
		status_ptr->st_u.info[2], status_ptr->st_u.info[3],
		status_ptr->st_u.info[4], status_ptr->st_u.info[5],
		status_ptr->st_u.info[6], status_ptr->st_u.info[7],
		status_ptr->st_u.info[8], status_ptr->st_u.info[9]);
	msg_printf("  parameter field owner: %s\n",
		error_bit_names[parameter_field_owner]);
	}
#endif
	
	if (!bp->b_error)
		bp->b_error = current_error;

	return operator_request <= parameter_field_owner &&
	                           parameter_field_owner <= intnl_maint_reqst ?
		status_ptr->st_u.urr[0] : -1;
}



/*
**  Control data transfers to/from a cs80 device
*/

void cs80_x_transfer(bp)
register struct buf *bp;
{
	/* if timeout is called, you still need to dequeue after proceesing
	   the timeout */
	register int i;
	register struct isc_table_type *sc = bp->b_sc;

	cs80_transfer(bp);
	do {
		i = selcode_dequeue(sc);
#ifdef __hp9000s300
		if(sc->card_type!=HP25560)
			i += dma_dequeue();
#endif /* __hp9000s300 */
	} while (i);
}


void cs80_trans_timeout(bp)
register struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc,cs80_x_transfer,bp->b_sc->int_lvl,0,bp->b_TO_state);
#ifdef CS80ERRLOG
	msg_printf("cs80: timeout (major %d minor 0x%06x)\n",
		major(bp->b_dev), minor(bp->b_dev)); 
#endif
}


cs80_transfer(bp)
register struct buf *bp;
{
	/* This routine is called from the interrupt level whenever
	   an appropriate interrupt occurs.  Appropriate: one for which
	   this is the isr service, after decoding.  The buffer structure
	   contains a state, and that state is selected to drive some
	   amount of processing.  The processor is then released and
	   some other interrupt will cause the next action.   During 
	   the actual transfer of data, the processor is released, but
	   ownership of the bus is retained.

	   The reader is reminded that each state is a new call to this
	   procedure, and thus local variables do not survive state 
	   transitions.  Any values must be recalculated in each state.
	   (The necessary memory for this procedure is contained in
	   bp and the things it points to.)
	   (Suggestion: use inner blocks for local variables, one for each
	   state that needs locals.  Make sure that any outermost block
	   variables are initialized on entry, and not changed.)

	   It is also very important that the state variable be set before,
	   not after, the get_xxx routines are called.  If this is not done,
	   it might be possible to go through the same state twice, which
	   guarantees disaster.  (Scenario: running at zero and called
	   from dequeue (which was called from enqueue).  Call get_selcode,
	   and interrupt occurs just after the request is queued, but before
	   the return.  Interrupt will call call selcode_dequeue, which
	   will go through state 0 a second time!).

	   It is also easy to forget that you must own the select code
	   to set up for a parallel poll interrupt.  Once you have
	   done that, then you may drop the select code.

	*/

	enum states {
		initial=0,
		command,
		get1_TO,
		get1,
		execution,
		reporting1,
		reporting2_TO,
		reporting2,
		reporting3, 
		status1,
		status2_TO,
		status2,
		status3,
		release2_TO,
		release2,
		release3,
		check_config,
		timedout,
		cleanup_timeout1,
		cleanup_timeout2,
		defaul
	};

	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register int dk_index = bp->b_dk_index;


retry:
try

	START_FSM;
reswitch:	
		if (dk_index >= 0)
			dk_busy &= ~(1 << dk_index);
		
		switch((enum states)iob->b_state) {

	/* <<<<< entire switch block shifted left to allow more room <<<<< */
	
	case initial:  /* entered once only, to initialize the iobuf */
		iob->b_errcnt = 0;  /* retry count */

		iob->b_state = (int)command;
		get_selcode(bp, cs80_transfer);
		break;
	
	case command:  /* issue command message & break for ppoll wait */
		bp->b_flags &= ~(B_RETRY_REQUIRED | B_PPOLL_INT_TIMEDOUT | B_CANCEL_FAILED);

		if (dk_index >= 0) {
			dk_seek[dk_index]++;
			dk_busy |= 1 << dk_index;
		}

		{
		int exec;

		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		exec = (bp->b_action2)(bp);
		END_TIME;

		bp->b_TO_state = (int)(exec ? get1_TO: reporting2_TO);
		START_TIME(cs80_trans_timeout, bp->b_clock_ticks);

		iob->b_state = (int)(exec ? get1 : reporting2);
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		}
		break;

	case get1_TO:  /* timed out waiting for ppoll; confirm it later */
		HPIB_ppoll_clear(bp);
		bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
		/* drop through */

	case get1:  /* get selectcode again */
		END_TIME;
		iob->b_state = (int)execution;
		get_selcode(bp, cs80_transfer);
		break;
		
	case execution:  /* initiate execution message */
		if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
			if (HPIB_test_ppoll(bp, 1))
				bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
			else
				escape(TIMED_OUT);

		if (dk_index >= 0) {
			dk_busy |= 1 << dk_index;
			dk_xfer[dk_index]++;
			dk_wds[dk_index] += bp->b_bcount >> 6;
		}

		iob->b_xaddr = bp->b_un.b_addr;
		iob->b_xcount = bp->b_bcount;

		bp->b_TO_state = (int)timedout;
		START_TIME(cs80_trans_timeout, bp->b_exec_msg_ticks);

		(*sc->iosw->iod_preamb)(bp, EXECUTION_SEC);

		iob->b_state = (int)reporting1;

#ifdef __hp9000s700
		if(bp->b_hpibflags&B_HPIBMUST_INTR)
			(*sc->iosw->iod_tfr)(MUST_INTR, bp, cs80_transfer);
		else
			(*sc->iosw->iod_tfr)(MAX_SPEED, bp, cs80_transfer);
#else
		(*sc->iosw->iod_tfr)(MAX_SPEED, bp, cs80_transfer);
#endif
		break;
	
	case reporting1:  /* issue postamble & test for ppol */
		iob->b_xcount -= sc->resid;
		(*sc->iosw->iod_postamb)(bp);

		if (dk_index >= 0)
			dk_busy |= 1 << dk_index;

		if (HPIB_test_ppoll(bp, 1)) {
			END_TIME;
			iob->b_state = (int)reporting3;
			goto reswitch;
		}

		bp->b_TO_state = (int)reporting2_TO;
		iob->b_state = (int)reporting2;
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		break;

	case reporting2_TO:  /* timed out waiting for ppoll; confirm it later */
		HPIB_ppoll_clear(bp);
		bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
		/* drop through */

	case reporting2:  /* get select code again */
		END_TIME;
		iob->b_state = (int)reporting3;
		get_selcode(bp, cs80_transfer);
		break;

	case reporting3:  /* receive reporting message */
		if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
			if (HPIB_test_ppoll(bp, 1))
				bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
			else
				escape(TIMED_OUT);
		if (bp->b_flags & B_CANCEL_FAILED) {
			iob->b_state = (int)cleanup_timeout2;
			goto reswitch;
		}

		bp->b_flags &= ~(B_RETRYING_STATUS | B_RECONFIGURE_NEEDED);

		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		if (cs80_qstat(bp)) {
			END_TIME;
			iob->b_state = (int)status1;
			goto reswitch;
		}
		END_TIME;

		if (!bp->b_error)
			if (bp->b_flags & B_RETRY_REQUIRED) {
				if (++iob->b_errcnt >= MAXTRIES)
					cs80_b_error_escape(bp, EIO);
				iob->b_state = (int)command;
				goto reswitch;
			} else
				bp->b_resid -= iob->b_xcount;

		iob->b_state = (int)defaul;
		drop_selcode(bp);
#ifdef CS80ERRLOG
		if (bp->b_error)
			msg_printf("cs80: error %d (major %d minor 0x%06x)\n",
				bp->b_error, major(bp->b_dev),minor(bp->b_dev)); 
#endif
		queuedone(bp);
		break;

	case status1:  /* request status & break for ppoll wait */
		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		cs80_ICc(bp, CMDrequest_status);
		END_TIME;

		bp->b_TO_state = (int)status2_TO;
		START_TIME(cs80_trans_timeout, STATUS_TIME);

		iob->b_state = (int)status2;
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		break;

	case status2_TO:  /* timed out waiting for ppoll; confirm it later */
		HPIB_ppoll_clear(bp);
		bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
		/* drop through */

	case status2:  /* get select code again */
		END_TIME;
		iob->b_state = (int)status3;
		get_selcode(bp, cs80_transfer);
		break;

	case status3:  /* receive full status and its reporting message */
		if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
			if (HPIB_test_ppoll(bp, 1))
				bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
			else
				escape(TIMED_OUT);
		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		cs80_status_exec(bp);
		END_TIME;

		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		HPIB_ppoll(bp, 1);
		if (cs80_qstat(bp)) {
			END_TIME;
			if (bp->b_flags & B_RETRYING_STATUS)
				cs80_b_error_escape(bp, EIO);
			else {
				bp->b_flags |= B_RETRYING_STATUS;
				iob->b_state = (int)status1;
				goto reswitch;
			}
		}
		END_TIME;

		{
		int unit_to_release = cs80_decode_status(bp);

		struct cs_status_type *status_ptr = &((struct cs80_od *)iob)->status;
		if (status_ptr->errorbits.s_m[0])	{
#ifdef CS80ERRLOG
			msg_printf("cs80: command rejected (major %d minor 0x%06x)\n",
				major(bp->b_dev), minor(bp->b_dev)); 
#endif
			cs80_b_error_escape(bp,bp->b_error);
			/* No return */
		}

		if (unit_to_release < 0) {
			iob->b_state = (int)check_config;
			goto reswitch;
		}

		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		cs80_ICuc(bp, unit_to_release, CMDrelease);
		END_TIME;

		}

		bp->b_TO_state = (int)release2_TO;
		START_TIME(cs80_trans_timeout, RELEASED_TIME);

		iob->b_state = (int)release2;
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		break;

	case release2_TO:  /* timed out waiting for ppoll; confirm it later */
		HPIB_ppoll_clear(bp);
		bp->b_flags |= B_PPOLL_INT_TIMEDOUT;
		/* drop through */

	case release2:  /* get select code again */
		END_TIME;
		iob->b_state = (int)release3;
		get_selcode(bp, cs80_transfer);
		break;

	case release3:  /* receive reporting message (ignore value) */
		if (bp->b_flags & B_PPOLL_INT_TIMEDOUT)
			if (HPIB_test_ppoll(bp, 1))
				bp->b_flags &= ~B_PPOLL_INT_TIMEDOUT;
			else
				escape(TIMED_OUT);
		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		(void) cs80_qstat(bp);
		END_TIME;
		/* drop through */

	case check_config:  /* reconfigure if necessary; restore cmd unit */
		START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
		if (bp->b_flags & B_RECONFIGURE_NEEDED)
			(void) cs80_set_status_mask_cmd(bp);
		else
			cs80_ICc(bp, m_unit(bp->b_dev) | UNIT_BASE);
		END_TIME;

		bp->b_TO_state = (int)reporting2_TO;
		START_TIME(cs80_trans_timeout, RECONFIGURE_TIME);

		iob->b_state = (int)reporting2;
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		break;

	case timedout:
		escape(TIMED_OUT);

	case cleanup_timeout1:  /* get select code again */
		iob->b_state = (int)cleanup_timeout2;
		get_selcode(bp, cs80_transfer);
		break;

	case cleanup_timeout2:  /* issue cancel cmd & break for ppoll wait */
		bp->b_flags &= ~(B_PPOLL_INT_TIMEDOUT | B_CANCEL_FAILED);

		try
			START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
			(void) cs80_cancel_cmd(bp);
			END_TIME;
		recover {
			short identify_bytes;
			if (escapecode != TIMED_OUT)
				escape(escapecode);
			(*sc->iosw->iod_abort)(bp);
			bp->b_flags |= B_CANCEL_FAILED;
			iob->b_xaddr = (caddr_t)&identify_bytes;
			START_TIME(cs80_trans_timeout, SHORT_MSG_TIME);
			(*sc->iosw->iod_ident)(bp);
			END_TIME;
#ifdef CS80ERRLOG
			msg_printf("cs80: timeout recovery ident 0x%x (major %d minor 0x%06x)\n",
				identify_bytes,
				major(bp->b_dev), minor(bp->b_dev)); 
#endif
		}

		bp->b_TO_state = (int)reporting2_TO;
		START_TIME(cs80_trans_timeout, CANCEL_TIME);

		iob->b_state = (int)reporting2;
		HPIB_ppoll_drop_sc(bp, cs80_transfer, 1);
		break;

	default:
		panic("Unrecognized cs80_transfer state");

	}  /* switch */
	END_FSM;
	recover {
		if (dk_index >= 0)
			dk_busy &= ~(1 << dk_index);
		iob->b_state = (int)defaul;
		ABORT_TIME;
		if (escapecode == TIMED_OUT) {
			HPIB_ppoll_clear(bp);
			(*sc->iosw->iod_abort)(bp);
			drop_selcode(bp);
			if (bp->b_flags & B_RETRY_REQUIRED &&
			    ++iob->b_errcnt >= MAXTRIES)
				bp->b_error = EIO;
			else {
				bp->b_flags |= B_RETRY_REQUIRED;
				iob->b_state = (int)cleanup_timeout1;
				goto retry;
			}
		} else
			drop_selcode(bp);
		if (!bp->b_error)
			bp->b_error = EIO;
#ifdef CS80ERRLOG
		if (bp->b_error)
			msg_printf("cs80: error %d (major %d minor 0x%06x)\n",
				bp->b_error,
				major(bp->b_dev), minor(bp->b_dev)); 
#endif
		queuedone(bp);
	}
}


/*
**  cs80_nop - control operation for synchronizing closes
*/
cs80_nop(bp)
struct buf *bp;
{
	queuedone(bp);
}


/*
**  unit_opened - if unit already opened, return open unit & device pointers
*/
struct cs80_ou *unit_opened(dev, odp_ptr)
register dev_t dev;
struct cs80_od **odp_ptr;
{
	register int unit = m_unit(dev);
	register struct cs80_od *d;
	register struct cs80_ou *up;

	dev &= M_DEVMASK;
	for (d = cs80_odt; d < cs80_odt + ODS; ++d)
		if (dev == d->device && d->dev_o_count) {
			*odp_ptr = d;
			up = d->unit_list;
			while (up != NULL && up->unit != unit)
				up = up->link;
			return up;
		}
	*odp_ptr = NULL;
	return NULL;
}


/*
**  unit_open - allocate new device/unit entries and return pointers to them
*/
struct cs80_ou *unit_open(dev, odp_ptr)
register dev_t dev;
struct cs80_od **odp_ptr;
{
	register int unit = m_unit(dev);
	register struct cs80_od *d;
	register struct cs80_ou *up, **upp;
	register unsigned char *vocp;
	register long *vsp;

/*
	if (m_selcode(dev) > 31 ||
*/
	if (				/*!!!!!!!!!!python!!!!!!!!!!!!!!*/
	    m_busaddr(dev) > 7	||
	    unit > 14 	||
	    m_volume(dev) > 7 )
		return NULL;

	dev &= M_DEVMASK;

	/*
	**  device already open?
	*/
	for (d = cs80_odt; dev != d->device || !d->dev_o_count; )
		if (++d >= cs80_odt + ODS) {
			/*
			**  allocate a new open device entry
			*/
			for (d = cs80_odt; d->dev_o_count; )
				if (++d >= cs80_odt + ODS) {
#ifdef CS80ERRLOG
					msg_printf("cs80: device table overflow\n");
#endif
					*odp_ptr = NULL;
					return NULL;
				}
			d->device = dev;
			d->unit_list = NULL;
			break;
		}
	*odp_ptr = d;

	/*
	**  unit already open?
	*/
	for (upp = &d->unit_list; (up = *upp) != NULL; upp = &up->link)
		if (unit == up->unit)
			return up;

	/*
	**  allocate a new open unit entry
	*/
	for (up = cs80_out; up < cs80_out + OUS; ++up)
		for (vocp = up->uv_o_count; !*vocp; ++vocp)
			if (vocp == up->uv_o_count + 7) {
				*upp = up;
				up->link = NULL;
				up->dk_index = -1;
				up->unit = unit;
				up->log2blk = -1;
				for (vsp = up->volsize; vsp < up->volsize + 8; ++vsp)
					*vsp = -1;
				return up;
			}
#ifdef CS80ERRLOG
	msg_printf("cs80: unit table overflow\n");
#endif
	return NULL;
}


/*
**  assign_dk_index - if possible, assign a dk_index for profiling disc activity
*/
static void assign_dk_index(dev, up)
dev_t dev;
struct cs80_ou *up;
{
	register dev_t this_dk_devt = (dev & DK_DEVT_MASK) | DK_DEVT_MAJ;
	register int i;

	for (i = 0; i < DK_NDRIVE; i++)
		if (dk_devt[i] == this_dk_devt) {
			up->dk_index = i;
			return;  /* index already associated with this unit */
		}
	for (i = 0; i < DK_NDRIVE; i++)
		if (dk_devt[i] == 0) {
			dk_devt[i] = this_dk_devt;
			dk_time[i] = 0;
			dk_xfer[i] = 0;
			dk_seek[i] = 0;
			dk_busy &= ~(1 << i);
			up->dk_index = i;
			return;  /* new index allocated */
		}
	up->dk_index = -1;
	return;  /* no index associated */
}


static void free_dk_index(up)
struct cs80_ou *up;
{
	if (up->dk_index >= 0) {
		dk_devt[up->dk_index] = 0;
		up->dk_index = -1;
	}
}



/*
**  unit_close - close current unit and deallocate entry if no other units open
*/
void unit_close(dev, up, dp)
dev_t dev;
register struct cs80_ou *up;
register struct cs80_od *dp;
{
	register unsigned char *vocp;
	register struct cs80_ou **upp, *p;
	register struct isc_table_type *sc=isc_table[m_selcode(dev)];

	if (up == NULL || up->uv_o_count[m_volume(dev)]-- == 0 || dp == NULL)
		panic("cs80_close: unopened dev/unit");
#ifdef __hp9000s300
	if(sc->card_type!=HP25560)
		dma_unactive(isc_table[m_selcode(dev)]);
#endif /* __hp9000s300 */
	if (--dp->dev_o_count == 0) {
		for (vocp = up->uv_o_count; vocp < up->uv_o_count + 8; ++vocp)
			if (*vocp != 0)
				panic("cs80_close: open count inconsistancy");

		free_dk_index( up );
	} else {
		for (vocp = up->uv_o_count; vocp < up->uv_o_count + 8; ++vocp)
			if (*vocp != 0)
				return;
		for (upp = &dp->unit_list; (p = *upp) != up ; upp = &p->link)
			if (p == NULL)
				panic("cs80_close: unit not on dev's list");

		free_dk_index( up );

		*upp = p->link;
	}
}


/*
 *  this is a utility to set up for activities other than read/write.
 *  It is used internally for open activities, and also by ioctl.
 *  It knows cs80 specific things.
 */
cs80_control(dp, up, fsm, proc, clock_ticks, exec_msg_ticks, dev, bfr, len)
struct cs80_od *dp;
struct cs80_ou *up;
int (*fsm)();
int (*proc)();
dev_t dev;
char *bfr;
{
	register struct buf *bp = geteblk(len);
	long saved_bcount = bp->b_bcount;

	if (proc != cs80_describe_cmd && bfr != NULL)
		bcopy(bfr, bp->b_un.b_addr, len);

	bp->b_dev = dev;
	bp->b_error = 0;
	bp->b_bcount = len;

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_log2blk = up->log2blk;
	bp->b_dk_index = -1;  /* don't let this request contribute to stats */
	bp->b_action = fsm;
	bp->b_action2 = proc;
	bp->b_clock_ticks = clock_ticks;
	bp->b_exec_msg_ticks = exec_msg_ticks;

#ifdef __hp9000s700
	bp->b_spaddr = KERNELSPACE;
	bp->b_hpibflags |= B_HPIBMUST_INTR;
#endif

	enqueue(&dp->dev_queue, bp);

	iowait(bp);

	if (proc == cs80_describe_cmd) {
		/* copy describe data into C structure describe_type */
		caddr_t describe_buf = bp->b_un.b_addr;
		struct describe_type *db = (struct describe_type *)bfr;
		bcopy(&describe_buf[0],
		      db->controller_tag.cntrl_bytes,
		      CNTRL_BYTES);
		bcopy(&describe_buf[CNTRL_BYTES],
		      db->unit_tag.unit_bytes,
		      UNIT_BYTES);
		bcopy(&describe_buf[CNTRL_BYTES+UNIT_BYTES],
		      db->volume_tag.volume_bytes,
		      VOLUME_BYTES);
	} else if (bfr != NULL)
		bcopy(bp->b_un.b_addr, bfr, len);

	bp->b_bcount = saved_bcount;
	brelse(bp);
	return geterror(bp);
}


/* 
**  Open a cs80 device.
*/
cs80_open(dev, flag)
dev_t dev;
int flag;
{
	register struct cs80_ou *up;
	struct cs80_od *dp;
	register struct isc_table_type *sc;

	register int err = 0;
	char tape_option = SO_AUTO_SPARING;
	short ident;
	char dsj;
	struct describe_type descr_bytes;
	int subset80;
	long atp, ort, msa;

	int HPIB_utility();
#ifdef SDS
	if (m_volume(dev) != 0)
	        return sds_open(dev, flag);

#endif /* SDS */
	/*
	**  if not already opened, then open
	*/
	if ((up = unit_opened(dev, &dp)) == NULL &&
	    (up = unit_open(dev, &dp)) == NULL )
		return ENXIO;

	/*
	**  if command mode lock in effect, then fail
	*/
	if (minor(dp->cmd_mode_dev) == minor(dev))
		return EBUSY;

	/*
	**  check for HP-IB card
	*/
	if ((sc = isc_table[m_selcode(dev)]) == NULL ||
	    sc->card_type != HP98625 &&
	    sc->card_type != HP98624 &&
	    sc->card_type != INTERNAL_HPIB &&
	    sc->card_type != HP25560 )		/**** python *****/
		return ENXIO;

	/*
	**  before any possible sleeps during IO, mark the entries busy...
	**  so other opens can't try to claim...
	**  but from here on, unit_close must be called if the open fails
	*/
	++up->uv_o_count[m_volume(dev)];
	++dp->dev_o_count;
#ifdef __hp9000s300
	if(sc->card_type!=HP25560)
		dma_active(sc);
#endif  /* __hp9000s300 */

	/*
	**  identify, configure, and describe
	*/
	if ((err = cs80_control(dp, up, HPIB_utility, sc->iosw->iod_ident,
		0, 0, dev, (char *)&ident, sizeof ident) ? ENXIO : 0) ||
	    (err = ((ident >> 8) != 2) ? ENXIO : 0) ||
	    (err = cs80_control(dp, up, cs80_transfer, cs80_cancel_cmd,
		5 * HZ, 0, dev, NULL, 0)) ||
	    (err = cs80_control(dp, up, cs80_transfer, cs80_set_release_cmd,
		5 * HZ, 0, dev, NULL, 0)) ||
	    (err = cs80_control(dp, up, cs80_transfer, cs80_chan_indep_clr_cmd,
		5 * HZ, 0, dev, NULL, 0)) ||
	    (err = cs80_control(dp, up, cs80_transfer, cs80_set_status_mask_cmd,
		5 * HZ, 0, dev, NULL, 0)) ||
	    (err = cs80_control(dp, up, cs80_transfer, cs80_describe_cmd,
		60 * HZ, 5 * HZ, dev, &descr_bytes, sizeof descr_bytes)) ||
	/*
	**  if the unit is a cartridge tape, enable auto jump sparing
	*/
	    (up->dev_type = descr_bytes.unit_tag.unit.dt) == 2 &&
	    (err = cs80_control(dp, up, cs80_transfer, cs80_set_options_cmd,
		5 * HZ, 0, dev, &tape_option, sizeof tape_option)))
		goto errout;

	/*
	**  save the log base 2 of the unit's block size
	**
	**  the algorithm ASSUMES that the device's block size is a power of 2!
	*/
	{
	register unsigned bytes_per_block = descr_bytes.unit_tag.unit.nbpb;
	register int block_shift = 0;
	while (bytes_per_block >>= 1)
		block_shift++;
	up->log2blk = block_shift;
	}

        /*
	 * save hpib identify bytes.
	 */
	up->ident = ident;

	subset80 = descr_bytes.controller_tag.controller.ct & CT_SUBSET_80;
	atp = descr_bytes.unit_tag.unit.atp;		/* access time parm */
	ort = descr_bytes.unit_tag.unit.ort;		/* optimal retry time */
	msa = descr_bytes.volume_tag.volume.maxsadd;	/* max sector address */

	/*
	**  command_ticks - from end of command message to ppoll response
	**
	**   A. Subset/80 devices can take up to one worst-case seek (atp).
	**
	**   B. Current & planned cs80 disc drives can take up to two
	**	worst-case seeks (atp's), with a recalibrate in between.
	**	Recalibrates are completely unspecified, and can take up
	**	to one second.
	**
	**   C. Cartridge tapes can take up to one worst-case seek (atp).
	*/
	up->command_ticks = subset80 || up->dev_type == 2 ?
			atp * HZ / 100:
			atp * (2 * HZ) / 100 + HZ;

	/*
	**  execution_ticks - from start of execution message to ppoll response
	**
	**   A. Subset/80 devices can take up to one optimal retry time (ort)
	**	per block, plus up to one extra optimal retry time.
	**
	**   B. Current & planned cs80 disc drives can take up to one optimal
	**	retry time per block, plus up to one command_ticks parameter
	**	per incremental seek (every track crossing).
	**
	**   C. Current cs80 cartridge tapes can take up to one optimal retry
	**	time plus one access time parameter per block.
	**
	**   As a reasonable approximation, we separate the execution_ticks
	**   into two components: one on a per block basis, and another on
	**   a per execution message basis.
	*/
	if (subset80) {						/* subset 80 */
		up->block_ticks = up->extra_ticks = ort * HZ / 100;
	} else if (up->dev_type != 2) {				/* cs80 disc */
		up->block_ticks = ort * HZ / 100 +
				(up->command_ticks + msa) / (msa + 1);
		up->extra_ticks = up->command_ticks;
	} else {						/* cs80 tape */
		up->block_ticks = (ort + atp) * HZ / 100;
		up->extra_ticks = 0;
	}

	/*
	**  save the volume size in blocks
	*/
#if __hp9000s800
	if (descr_bytes.volume_tag.volume.maxsvadd_utb != 0)
#else
	if (descr_bytes.volume_tag.volume.maxsvadd.utb != 0)
#endif
		goto ENXIOout;
	up->volsize[m_volume(dev)] =
#if __hp9000s800
				descr_bytes.volume_tag.volume.maxsvadd_lfb + 1;
#else
				descr_bytes.volume_tag.volume.maxsvadd.lfb + 1;
#endif

	/*
	**  if possible, associate a dk_index for profiling disc activity
	*/
	assign_dk_index(dev, up);

	/*
	**  set the static disc activity profiling parameters
	*/
	if (up->dk_index >= 0)
		dk_mspw[up->dk_index] =
			(descr_bytes.unit_tag.unit.blocktime * 1.0e-6) /
			      (descr_bytes.unit_tag.unit.nbpb / 2);

	goto out;

ENXIOout:
	err = ENXIO;
errout:
	unit_close(dev, up, dp);

out:
	return err;
}


cs80_close(dev)
dev_t dev;
{
	struct cs80_od *dp;
	struct cs80_ou *up;

#ifdef SDS
	if (m_volume(dev) != 0)
	        return sds_close(dev);

#endif /* SDS */
	if ((up = unit_opened(dev, &dp)) == NULL)
		panic("cs80_close: unopened device");

	/* Return to default: be quiet about media changes */
	((struct iobuf *)dp)->b_flags &= ~B_MEDIA_CHANGE;

	(void)cs80_control(dp, up, cs80_nop, NULL, 0, 0, dev, NULL, 0);
	if (dp->cmd_mode_dev == dev)
		dp->cmd_mode_dev = 0;
	unit_close(dev, up, dp);
}


cs80_strategy(bp)
register struct buf *bp;
{
	register dev_t dev = bp->b_dev;
	register struct cs80_ou *up;
	struct cs80_od *dp;
	register long log2blk;

#ifdef SDS
	if (m_volume(dev) != 0)
	        return sds_strategy(bp, NULL);

#endif /* SDS */
	if ((up = unit_opened(dev, &dp)) == NULL)
		panic("cs80_strategy: unopened device");

	bp->b_sc = isc_table[m_selcode(dev)];
	bp->b_ba = m_busaddr(dev);
	bp->b_log2blk = log2blk = up->log2blk;
	bp->b_action = cs80_transfer;

	/*
	**  normal read/write, or command mode request?
	*/
	if (dev != dp->cmd_mode_dev) {
		bp->b_action2 = cs80_transfer_cmd;
		bp->b_dk_index = up->dk_index;
		if (bpcheck(bp, up->volsize[m_volume(dev)], log2blk, 0))
			return;
		bp->b_clock_ticks = up->command_ticks;
		bp->b_exec_msg_ticks = up->extra_ticks +
			up->block_ticks *
			((bp->b_bcount + (1 << log2blk) - 1) >> log2blk);
		if (bp->b_exec_msg_ticks > MAX_EXEC_MSG_TIME)
			bp->b_exec_msg_ticks = MAX_EXEC_MSG_TIME;
#ifdef REQ_MERGING			
		bp->b_flags |= B_MERGE_IO;
#endif		
	} else {
		bp->b_action2 = cs80_cmd;
		bp->b_dk_index = -1;  /* don't contribute to stats */
		bp->b_resid = bp->b_bcount;
#ifdef REQ_MERGING			
		bp->b_flags &= ~B_MERGE_IO;
#endif
	}

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
	enqueue(&dp->dev_queue, bp);
}


cs80_read(dev, uio)
dev_t dev;
struct uio *uio;
{
#ifdef SDS
        if (m_volume(dev) != 0)
	        return sds_read(dev, uio);

#endif /* SDS */
	return physio(cs80_strategy, NULL, dev, B_READ, minphys, uio);
}


cs80_write(dev, uio)
dev_t dev;
struct uio *uio;
{
#ifdef SDS
        if (m_volume(dev) != 0)
	        return sds_write(dev, uio);

#endif /* SDS */
	return physio(cs80_strategy, NULL, dev, B_WRITE, minphys, uio);
}


cs80_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
	struct cs80_od *dp;
	register struct cs80_ou *up;
	register int err;
	char tmp;
	struct describe_type descr;
	register struct isc_table_type *sc;
	struct io_hpib_ident *id_ptr;

#ifdef SDS
        if (m_volume(dev) != 0)
	        return sds_ioctl(dev, order, addr, flag);

#endif /* SDS */
	if ((up = unit_opened(dev, &dp)) == NULL)
		panic("cs80_ioctl: unopened device");

	switch (order) {

         case IO_HPIB_IDENT:
		id_ptr = (struct io_hpib_ident *)addr;
		id_ptr->dev_type = up->dev_type;
		id_ptr->ident = up->ident;
		err = 0;
		break;
	case CIOC_DESCRIBE:

		err = cs80_control(dp, up, cs80_transfer, cs80_describe_cmd,
			60 * HZ, 5 * HZ, dev, addr, DESCRIBE_BYTES);
		break;

	case DIOC_CAPACITY:
		{
		disk_describe_type cs80info;
		register capacity_type *cap = (capacity_type *)addr;
		err = cs80_control(dp, up, cs80_transfer, cs80_describe_cmd,
			60 * HZ, 5 * HZ, dev, &descr, DESCRIBE_BYTES);
		if(err==0)	{
			cs80info.maxsva=(unsigned int)((descr.volume_tag.volume_bytes[8]<<24)+(descr.volume_tag.volume_bytes[9]<<16)+(descr.volume_tag.volume_bytes[10]<<8)+descr.volume_tag.volume_bytes[11]);
			cs80info.lgblksz=(unsigned int)((descr.unit_tag.unit_bytes[4]<<8)+descr.unit_tag.unit_bytes[5]);
			cap->lba=(long)(((cs80info.maxsva+1)*cs80info.lgblksz)>>DEV_BSHIFT);
		}
		}
		break;
	
	case DIOC_DESCRIBE:
	case IOC_OUT|32<<16|'D'<<8|103: /* binary compatibility */

		err = cs80_control(dp, up, cs80_transfer, cs80_describe_cmd,
			60 * HZ, 5 * HZ, dev, &descr, DESCRIBE_BYTES);
		if(err==0)	{
			register disk_describe_type *cs80info=(disk_describe_type *)addr;
			bzero(cs80info->model_num,16);
			cs80info->model_num[0]='h';
			cs80info->model_num[1]='p';
			cs80info->model_num[2]=(unsigned char)((descr.unit_tag.unit_bytes[1]&0xf)+'0');
			cs80info->model_num[3]=(unsigned char)((descr.unit_tag.unit_bytes[2]>>4)+'0');
			cs80info->model_num[4]=(unsigned char)((descr.unit_tag.unit_bytes[2]&0xf)+'0');
			cs80info->model_num[5]=(unsigned char)((descr.unit_tag.unit_bytes[3]>>4)+'0');
			cs80info->intf_type=descr.controller_tag.cntrl_bytes[4];
			cs80info->maxsva=(unsigned int)((descr.volume_tag.volume_bytes[8]<<24)+(descr.volume_tag.volume_bytes[9]<<16)+(descr.volume_tag.volume_bytes[10]<<8)+descr.volume_tag.volume_bytes[11]);
			cs80info->lgblksz=(unsigned int)((descr.unit_tag.unit_bytes[4]<<8)+descr.unit_tag.unit_bytes[5]);
			if(descr.controller_tag.cntrl_bytes[0]==0||descr.controller_tag.cntrl_bytes[0]==1) cs80info->dev_type=DISK_DEV_TYPE;
			else if(descr.controller_tag.cntrl_bytes[0]==2) cs80info->dev_type=CTD_DEV_TYPE;
		if (order == DIOC_DESCRIBE)
			cs80info->flags = 0; /* WRITE_PROTECT */
		}
		break;

	case CIOC_UNLOAD:
		err = up->dev_type != 2 ? ENODEV :
		      cs80_control(dp, up, cs80_transfer, cs80_unload_cmd,
			300 * HZ, 0, dev, NULL, 0);
		break;

	case CIOC_VERIFY:
		err = cs80_control(dp, up, cs80_transfer, cs80_verify_cmd,
			3600 * HZ, 0, dev, addr, VERIFYPARMS_SIZE);
		break;

	case CIOC_MARK:
		err = up->dev_type != 2 ? ENODEV :
		      cs80_control(dp, up, cs80_transfer, cs80_mark_cmd,
			300 * HZ, 0, dev, addr, MARKPARMS_SIZE);
		break;

	case CIOC_SET_OPTIONS:
		err = up->dev_type != 2 ? ENODEV :
		      cs80_control(dp, up, cs80_transfer, cs80_set_options_cmd,
			5 * HZ, 0, dev, addr, sizeof (char));
		break;

	case CIOC_MEDIA_CHNG:
		{
		register struct iobuf *iob = (struct iobuf *)dp;
		if (*(int *)addr)
			iob->b_flags |= B_MEDIA_CHANGE;
		else
			iob->b_flags &= ~B_MEDIA_CHANGE;
		err = 0;
		break;
		}

	case CIOC_LOAD:
		err = up->dev_type != 2 ? ENODEV :
		      cs80_control(dp, up, cs80_transfer, cs80_load_cmd,
			600 * HZ, 0, dev, addr, sizeof (char));
		break;

	case CIOC_CMD_MODE:
		if (!suser()) {
			err = EPERM;
			break;
		}

		if (*(int *)addr) {
			err = dp->cmd_mode_dev ||
			      up->uv_o_count[m_volume(dev)] != 1
					? EBUSY : 0;
			if (!err)
				dp->cmd_mode_dev = dev; 
		} else {
			err = dp->cmd_mode_dev != dev ? EPERM : 0;
			if (!err)
				dp->cmd_mode_dev = 0; 
		}
		break;

	case CIOC_SET_CMD:
	case CIOC_CMD_REPORT:
		err = dp->cmd_mode_dev != dev ? EACCES :
		      ((struct cmd_parms *)addr)->cmd_length <= 0 ||
		      ((struct cmd_parms *)addr)->cmd_length > CMD_LEN ?
				EINVAL : 0;
		if (!err) {
			bcopy(addr, &dp->cmd_parms, CMDPARM_SIZE);
			if (order == CIOC_CMD_REPORT)
				err = cs80_control(dp, up, cs80_transfer,
						cs80_cmd, 0, 0, dev, NULL, 0);
		}
		break;

	case CIOC_CANCEL:
	          err = cs80_control(dp, up, cs80_transfer, cs80_cancel_cmd,
	 	                     5 * HZ, 0, dev, NULL, 0);
	          break;

	case CIOC_CHAN_IND_CLR:
		  tmp = *addr;

		  /*
		     This is a kludge for diagnostics.  Sometimes Sherlock
		     needs to clear all devices attached to the card.  If
		     a unit number of 15 is specified in the channel
		     independent clear command, then all devices are cleared.
		     The caller can set the unit number to 15 by passing the
		     value 0x0f into the ioctl call for CIOC_CHAN_IND_CLR.
		  */
		  if( tmp == 0x0f )
		       dev |= ( (tmp << 4) & 0xf0);

	          err = cs80_control(dp, up, cs80_transfer,
	                cs80_chan_indep_clr_cmd, 5 * HZ, 0, dev, NULL, 0);
	          break;

	case CIOC_GET_STATUS:

		  bcopy( &(dp->status), addr, CSSTATUSSIZE );
		  err = 0;
	          break;

#ifdef __hp9000s300
	/*
	 * temporary solution for software destribution until 9.0
	 */
	case CIOC_GET_SECURITY_ID:
		if (!suser())
			return(EPERM);
		return(cs80_control(dp,up,cs80_transfer, cs80_get_security_no_cmd,
				5 * HZ, 5 * HZ, dev, addr,
				sizeof (struct security_id_info)));
		break;
#endif

	default:
		err = EINVAL;
		break;
	}

	return err;
	
}


int (*cs80_saved_dev_init)();

/*
**  one-time initialization code
*/
cs80_init()
{
#ifdef __hp9000s300
#ifdef SDS_BOOT
	extern int striped_boot;
#endif /* SDS_BOOT */
	/*
	**  set rootdev if not already set and boot device was cs80
	*/
	if (rootdev < 0 && msus.device_type >= 16 && msus.device_type <= 17) {
		register int maj;
		extern struct bdevsw bdevsw[];
		/*
		**  determine the cs80 blocked driver major number
		*/
		for (maj = 0; bdevsw[maj].d_open != cs80_open; )
			if (++maj >= nblkdev)
				panic("cs80_init: cs80_open not in bdevsw");
		/*
		**  construct rootdev from the boot ROM's MSUS
		*/
		rootdev = makedev(maj, makeminor(msus.sc, msus.ba,
							msus.unit, msus.vol));

/*
		rootdev = makedev(maj,((long)((msus.sc)<<16|(msus.ba)<<8|(msus.unit)<<4|msus.vol)));
*/
#ifdef SDS_BOOT
		if (striped_boot)
			rootdev |= 1;
#endif /* SDS_BOOT */
	}
#endif	 /* __hp9000s300 */

	/*
	**  call the next init procedure in the chain
	*/
	(*cs80_saved_dev_init)();
}


#ifdef __hp9000s300
struct msus (*cs80_saved_msus_for_boot)();

/*
**  code for converting a dev_t to a boot ROM msus
*/
struct msus cs80_msus_for_boot(blocked, dev)
char blocked;
dev_t dev;
{
	extern struct bdevsw bdevsw[];
	extern struct cdevsw cdevsw[];

	int maj = major(dev);
	struct cs80_ou *up;
	struct cs80_od *dp;
	int log2blk;
	struct msus my_msus;

	/*
	**  does this driver not handle the specified device?
	*/
	if (( blocked && (maj >= nblkdev || bdevsw[maj].d_open != cs80_open)) ||
	    (!blocked && (maj >= nchrdev || cdevsw[maj].d_open != cs80_open)))
		return (*cs80_saved_msus_for_boot)(blocked, dev);

	/*
	**  what's the media's block size?
	*/
	if (cs80_open(dev, FREAD) || !(up = unit_opened(dev, &dp)))
		log2blk = -1;
	else {
		log2blk = up->log2blk;
		cs80_close(dev);
	}

	/*
	** construct the boot ROM msus
	*/
	my_msus.dir_format = 0;	/* assume LIF */
	my_msus.device_type = (log2blk < 0) ? 31 : (log2blk == 8) ? 16 : 17;
	my_msus.sc = m_selcode(dev);
	my_msus.ba = m_busaddr(dev);
	my_msus.unit = m_unit(dev);
	my_msus.vol = m_volume(dev);
	return my_msus;
}
#endif /* __hp9000s300 */

#ifdef IOSCAN

#include "../h/libio.h"
#include "../wsio/dconfig.h"

int cs80_config_identify();
int cs80_unit1_present();

extern int (*hpib_config_ident)();
extern int (*cs80_find_unit1)();
#endif /* IOSCAN */

/*
**  one-time linking code
*/
cs80_link()
{
	extern int (*dev_init)();
#ifdef __hp9000s300
	extern struct msus (*msus_for_boot)();
#else
	extern struct msus_type (*msus_for_boot)();
#endif

	cs80_saved_dev_init = dev_init;
	dev_init = cs80_init;

#ifdef __hp9000s300
	cs80_saved_msus_for_boot = msus_for_boot;
	msus_for_boot = cs80_msus_for_boot;

#ifdef IOSCAN
        hpib_config_ident = cs80_config_identify;
        cs80_find_unit1 = cs80_unit1_present;
#endif /* IOSCAN */
#endif /* __hp9000s300 */
}


cs80_size(dev)
dev_t dev;
{
	struct cs80_ou *up;
	struct cs80_od *dp;
	int bytes;

#ifdef SDS
        if (m_volume(dev) != 0)
	        return sds_size(dev);

#endif /* SDS */		
	if (cs80_open(dev, FREAD) || !(up = unit_opened(dev, &dp)))
		return 0;
	bytes = up->volsize[m_volume(dev)] << up->log2blk;
	cs80_close(dev);
	return bytes >> DEV_BSHIFT;
}

#ifdef __hp9000s300
int savecore_ppoll(sc)
struct isc_table_type *sc;
{
	extern dev_t dumpdev;

	(*sc->iosw->iod_pplset)(sc,0x80>>m_busaddr(dumpdev),1,0);  
	while (!(*sc->iosw->iod_ppoll)(sc))
		; /* Spin until the disk is responds to parallel poll */
	(*sc->iosw->iod_pplclr)(sc,0x80>>m_busaddr(dumpdev));  
}

/*
 * cs80_dump provides the "Command Set 80" interface for savecore.  It
 * is called with the a buffer pointer to a buf structure that has been
 * set up already.  Also passed in is the DMA channel to be used.  The * starting address to dump to in swap is a global variable.
 */

#endif

int cs80_dump(bp)
	struct buf *bp;
{
#ifdef __hp9000s300
	extern long dumplo;
	extern dev_t dumpdev;
	struct isc_table_type *sc = isc_table[m_selcode(dumpdev)];
	struct simon *cp = (struct simon *) sc->card_ptr;
	caddr_t addr;
	struct cs80_ou *up;
	struct cs80_od *dp;
	int zero = 0;	/* Keep complier from generating clrs */
	int qstat;

	if ((up = unit_opened(dumpdev, &dp)) == NULL) {
		printf ("savecore: can't find device at %x\n", dumpdev);
		return (-1);
	}
	bp->b_log2blk = up->log2blk;
/*
 * Do a parallel poll sequence to wait for the disk to recover from reset
 */
	savecore_ppoll (sc);
/*
 * The DMA channel has been cleared if it was in use on the root device
 * via the iod_abort call previously.  DMA isn't used here.  The disk
 * interface has been reset and the disk itself has been cleared.
 * Issue the seek and write command and return the address of
 * a function to test if an error occurred.
 */

	bp->b_un2.b_sectno = (dumplo << (DEV_BSHIFT - up->log2blk));
	cs80_transfer_cmd (bp);
	savecore_ppoll (sc);		/* Ppoll after the xfer cmd */

/*
** Issue a preamble command for cs80 transfer
*/
	(*sc->iosw->iod_preamb)(bp, EXECUTION_SEC);
/*
** Call the card driver to to the fast handshake
*/
	(*sc->iosw->iod_savecore_xfer)(bp);
	(*sc->iosw->iod_postamb)(bp);	/* Issue a postamble to the drive */
	savecore_ppoll (sc);		/* Ppoll after the postamble */
/*
 * Do a qstat to make the disk happy (otherwise it's light keeps blinking)
 */
	qstat = cs80_qstat (bp);
	return (bp->b_error ? -1 : dumpdev);
#endif
}

#ifdef IOSCAN

cs80_config_identify(sinfo)
register struct ioctl_ident_hpib *sinfo;
{
    register struct cs80_ou *up;
    struct cs80_od *dp;
    register struct isc_table_type *isc;
    short ident;
    dev_t sdev, bdev;
    int busaddr, err;

    if ((isc = isc_table[sinfo->sc]) == NULL)
         return ENOSYS;

    if ( isc->card_type != HP98625 &&
         isc->card_type != HP98624 &&
         isc->card_type != INTERNAL_HPIB )
                return ENODEV;

    bdev = 0x04000000 | (sinfo->sc << 16);

    for (busaddr = 0; busaddr < 8; busaddr++) {

        sdev = bdev | (busaddr << 8);

        if ((up = unit_opened(sdev, &dp)) == NULL &&
            (up = unit_open(sdev, &dp)) == NULL )
                return ENXIO;

        ++up->uv_o_count[m_volume(sdev)];
        ++dp->dev_o_count;
        if(isc->card_type!=HP25560)
                dma_active(isc);

        err = cs80_control(dp, up, HPIB_utility, isc->iosw->iod_ident,
                0, 0, sdev, (char *)&ident, sizeof ident);

        if (err) {
            unit_close(sdev, up, dp);
            sinfo->dev_info[busaddr].ident = -1;
            continue;
        }

        sinfo->dev_info[busaddr].ident = ident;

        unit_close(sdev, up, dp);
    }
    return 0;
}

cs80_unit1_present(sinfo, busaddr)
register struct ioctl_ident_hpib *sinfo;
int busaddr;
{
    dev_t dev;
    register struct cs80_ou *up;
    struct cs80_od *dp;
    register struct isc_table_type *isc;
    struct describe_type descr_bytes;
    int err;

    dev = 0x04000000 | makeminor(sinfo->sc, busaddr, 1, 0);
    isc = isc_table[sinfo->sc];

    if ((up = unit_opened(dev, &dp)) == NULL &&
        (up = unit_open(dev, &dp)) == NULL )
              return ENXIO;

    ++up->uv_o_count[m_volume(dev)];
    ++dp->dev_o_count;
    if (isc->card_type!=HP25560)
        dma_active(isc);

    /* Do appropriate cs80 action here to find unit 1 if it exists */
    err = cs80_control(dp, up, cs80_transfer, cs80_describe_cmd,
              60 * HZ, 5 * HZ, dev, &descr_bytes, sizeof descr_bytes);

    unit_close(dev, up, dp);

    if (err)        /* an error means there was no device */
       return 0;
    else return 1;
}
#endif /* IOSCAN */
