/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/scsi.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:17:42 $
 */

#ifndef _S200IO_SCSI_INCLUDED
#define _S200IO_SCSI_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#define SZ_CMDread_capacity	0x08
#define	SZ_CMDread_block_limits	0x06
#define SZ_INQUIRY		100

/*
** The target drives the bus phases, hence drives the states in the FSM.
** This macro is part of the mechanism to insure it is doing this "sanely".
*/
#define SCSI_SANITY(str)	\
	msg_printf("SCSI_SANITY: %s\n", str), escape(PHASE_WRONG)

/* Status Codes */
#define	good		0
#define	check_condition	0x2
#define	busy		0x8
#define	resv_conflict	0x18

#define SCSI_DEFERRED_ERROR	0x71	/* error code */

/* Sense Keys */
#define no_sense	0
#define recovered_err	1
#define not_ready	2
#define medium_err	3
#define hardware_err	4
#define illegal_request	5
#define unit_attention	6
#define data_protect	7
#define aborted_cmd	0xB

/*
 * defines for the upper half of BUF's b_flags
 */
#define ATN_REQ 	B_SCRACH1
#define CONNECTED	B_SCRACH2
#define S_RETRY_CMD 	B_SCRACH3
#define TO_SET		B_SCRACH4
#define ABORT_REQ	B_DIL

/*
 * defines for the b_scsiflags
 */
#define POWER_ON	0x1

/*
 * defines for the b_newflags - may not be necessary, can use b_scsiflags
 */
#define SDTR_SENT       0x0001
#define REJECT_SDTR	0x0004


/* Bits in integer indicate which diagnostic mesgs will be printed */
extern int SCSI_DEBUG;

/* Defines for setting the type of diagnostic output               */
#define	DIAGs	0x0001	/* Print general important diagnostic data */
#define	MESGs	0x0002	/* Print info on SCSI mesg system          */
#define	MESG_v	0x0004	/* Print verbose info on SCSI mesg system  */
#define	STATES	0x0008	/* Track states in SCSI                    */
#ifndef __hp9000s700    /* ISR = Interrupt Space Reg on s700 */
#define	ISR	0x0010	/* ISR status                              */
#endif /* __hp9000s700 */
#define	ISR_v	0x0020	/* ISR status (verbose)                    */
#define	WAITING	0x0040	/* Waiting for an event                    */
#define	CMDS	0x0080	/* Commands issued to SCSI                 */
#define	CMD_MOD	0x0100	/* Command Mode Path	                   */
#define DEBUG	0x0200	/* Temporary flag for debugging		   */
#define	MISC	0x0800	/* Miscellaneous Information               */
#define SENSE	0x1000	/* Print sense info on check condition     */

#define	SCSI_TRACE(x)	if (SCSI_DEBUG&(x)) msg_printf


/* Defines for scsi_flags
 *    Allows specially configured environments
 *    (Intended for testing and debugging)
 */
#define FHS_REQ		0x01
#define DMA16_REQ	0x02
#define DISCON_OK	0x04
#define NOSYNC		0x08	/* This flag should ONLY be set at powerup */
#define SCSI_DMA_PRI	0x10
#define BRISTOL		0x20	/* Special requirements for BRISTOL */

extern int scsi_flags;

/* escape codes */
#define NO_SENSE	0x01
#define S_RETRY_CNT	0x04
#define SELECT_TO	0x05
#define PHASE_WRONG	0x06
#define PARITY_ERR	0x07

#define	CLEAR_reselect		HPIB_ppoll_clear
/*
 * mnemonic names for 'buf' scratch registers
 */
#define b_log2blk		b_s0	/* log2 (size of logical block) */
#define b_phase			b_s1	/* reqested phase of bus */
#define b_parms			b_s3	/* status of request */
#define b_nextstate		b_s4	/* next state for ISR */
#define b_scsiflags		b_s5	/* special scsi flags */
#define b_newflags              b_s6    /* more special scsi flags */

#define	b_status		b_s4	/* Status returned by command */
#define	b_sensekey		b_s5	/* Sense key returned in sense */

#define io_sanity		io_s0  /* SCSI devices control the bus.
					     * This is a sanity check */
#define io_sync			io_s1  /* sync transfer data */
#define save_addr		io_s2  /* saved address for sense cmd */
#define blkssz_log2		io_s3  /* log2 of tape blk size */
#define	busy_cnt		io_s3	/* retry cnt for busy status */

#define retry_cnt		b_errcnt	/* retry count */
#define cur_status		b_errcnt	/* current status returned */

/* Bit positions for io_sanity */
#define	io_cmd	0x01	/* Exactly once */
#define	io_data	0x02	/* 0, 1 or many times */
#define	io_stat	0x04	/* Exactly once */

/*
** definitions for isc_table_type fields
*/
#define set_state_cnt   lock_count	/* retry count for set_state */

extern char *status_code_values[];

#define PSNS_ACK	0x40  /* same ACK bit as in psns register */

enum states {
	initial=0,
	busy_retry,
	select,
	select_nodev,
	select_TO,
	transfer_TO,
	discon_TO,
	reselect,
	set_state,
	scsi_error,
	defaul,
	/* Special states assigned by ISR
	 * Lower three bits correspond to psns register
	 */
	phase_data_out  = PSNS_ACK + DATA_OUT_PHASE,
	phase_data_in   = PSNS_ACK + DATA_IN_PHASE,
	phase_cmd    	= PSNS_ACK + CMD_PHASE,
	phase_status 	= PSNS_ACK + STATUS_PHASE,
	phase_mesg_out  = PSNS_ACK + MESG_OUT_PHASE,
	phase_mesg_in   = PSNS_ACK + MESG_IN_PHASE
};

enum SPC_states {
	SCSI_SELECT,
	SCSI_IDENT_MSG,
	SCSI_MESG_IN,
	SCSI_MESG_OUT,
	SCSI_CMD,
	SCSI_STATUS,
	SCSI_DATA_XFR,
	SCSI_RESELECT
};

#if defined(__hp9000s300) && defined(_KERNEL)
/*
** scsi_driver_ctl
*/
#define FLUSH_QUEUE		0
#define	MOVE_MEDIUM		1
#define	INIT_ELEMENT_STATUS	2
#define	READ_ELEMENT_STATUS	3
#define	START_STOP		4
#define	RESERVE			5
#define	RELEASE			6

#define SZ_VAR_CNT		0xFF
#endif /* __hp9000s300 && _KERNEL */

#endif /* _S200IO_SCSI_INCLUDED */
