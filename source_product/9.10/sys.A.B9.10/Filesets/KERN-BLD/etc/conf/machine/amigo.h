/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/amigo.h,v $
 * $Revision: 1.5.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 16:42:03 $
 */
/* @(#) $Revision: 1.5.84.4 $ */       
#ifndef _SYS_AMIGO_INCLUDED /* allows multiple inclusion */
#define _SYS_AMIGO_INCLUDED

#ifdef __hp9000s300
#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#include "../h/types.h"
#include "../machine/iobuf.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/iobuf.h>
#endif /* _KERNEL_BUILD */

enum amigo_dev_type  { HP7906, HP7905_L, HP7905_U, HP7920, HP7925,
		       HP9895_SS_BLNK, HP9895_SS, HP9895_DS_BLNK,
                       HP9895_DS, HP9895_IBM, HP8290X, HP9121,
		       HP913X_A, HP913X_B, HP913X_C, NOUNIT };

enum command_type {
	seek_cmd,		/* seek 			*/
	buf_read,		/* buffered read		*/
	buf_write,		/* buffered write		*/
	unbuf_read,		/* unbuffered read		*/
	unbuf_write,		/* unbuffered write		*/
	req_status,		/* request status 		*/
	req_log_addr,		/* request logical address 	*/
	recalibrate_cmd,	/* recalibrate 			*/
	req_syndrome,		/* request syndrome 		*/
	end_cmd 		/* end		 		*/
	};

struct ftcb_type {
	char opcode;
	char unit;
	};

struct tva_type {
	short cyl;
	char  head;
	char  sect;
	};

struct status_type {
	unsigned int	s:1;
	unsigned int	p:1;
	unsigned int	d:1;
	unsigned int	s1:5; /* compiler bug: should be eunm s1_type */
	unsigned int	unit:8;
	unsigned int	star:1;
	unsigned int	xx:2;
	unsigned int	tttt:4;
	unsigned int	r:1;
	unsigned int	a:1;
	unsigned int	w:1;
	unsigned int	fmt:1;
	unsigned int	e:1;
	unsigned int	f:1;
	unsigned int	c:1;
	unsigned int	ss:2;
	};

struct syndrome_type {
	int		sb_pad1:3;
	int		sb_s1:5;
	int		sb_pad2:8;
	struct tva_type sb_tva;
	short		sb_offset;
	char		sb_correction_bytes[6];
	};

struct map_type {  /* media addressing parameters */
	short	cyl_per_med;
	short	trk_per_cyl;
	short	sec_per_trk;
	short	ident;
	short	model;	/* if NO_MODEL, value is not to be checked */
	char	rps;
	char	log2blk;
	char	file_mask;
	char	flag;
	};

#define NO_MODEL	255
#define R_BIT_1		254
#define R_BIT_0		253

#define SYNC_UNBUF	0x01	/* unbuffered transfers must be synchronous */
#define SURFACE_MODE	0x02	/* as opposed to cylinder mode */
#define MUST_BUFFER	0x04	/* supports buffered transfer protocol only */
#define NOWAIT_STATUS	0x08	/* status does not need a ppol wait */
#define RECALIBRATES	0x10	/* necessary after certain errors (MAC/IDC) */
#define END_NEEDED	0x20	/* needs an end command after access */

#define SEC_xDAT	SCG_BASE+0
#define SEC_DSJ		SCG_BASE+16
#define SEC_RSTA	SCG_BASE+8
#define SEC_OP1		SCG_BASE+8
#define SEC_OP2		SCG_BASE+9
#define SEC_OP3		SCG_BASE+10
#define SEC_OP4		SCG_BASE+12

#define UNBUF_READ_OC	5	/* unbuffered read op code */
#define UNBUF_WRITE_OC	8	/* unbuffered write op code */

enum s1_type {
    NORMAL_COMPLETION,			ILLEGAL_OPCODE,
    UNIT_AVAILABLE,			ILLEGAL_DRIVE_TYPE,
    S1_4,				S1_5,
    S1_6,				CYLINDER_COMPARE_ERROR,
    UNCORRECTABLE_DATA_ERROR,		HEAD_SECTOR_COMPARE_ERROR,
    IO_PROGRAM_ERROR,			S1_11,
    END_OF_CYLINDER,			SYNC_BIT_NOT_RECEIVED_IN_TIME,
    BUS_OVERRUN,			POSSIBLY_CORRECTABLE_DATA_ERROR,
    ILLEGAL_ACCESS_TO_SPARE,		DEFECTIVE_TRACK,
    ACCESS_NOT_READY_DURING_DATA_OP,	STATUS_2_ERROR,
    S1_20,				S1_21,
    ATTEMPT_TO_WRITE_ON_PROTECTED_TRK,	UNIT_UNAVAILABLE,
    S1_24,				S1_25,
    S1_26,				S1_27,
    S1_28,				S1_29,
    S1_30,				DRIVE_ATTENTION
    };


#ifdef _KERNEL
/*
**  open device table entry
*/
struct amigo_od {
	dev_t device;			/* selcode/busaddr */
	short open_count;		/* device open count */
	short dma_reserved;		/* count of successful dma_reserve's */
	char dk_index[8];		/* index for profiling disc activity */
	unsigned char unitvol[8][2];	/* unitvol type */
	struct iobuf dev_queue;		/* queue head */
};
#endif /* _KERNEL */



#define	MAX_IO_BUF	20
struct combined_buf {
	int command_name;		/* ioctl_command_type	   */
	short cmd_sec;			/* ctet.sec		   */
	char cmd_nb;			/* ctet.nb		   */
	unsigned int timeout_val;	/* How long should it take?*/
	short ppoll_reqd;		/* Parallel poll req'd?    */
					/*  if yes then 1, else 0  */
	int mesg_type;			/* 2nd arg of iod_mesg     */
	short dsj_reqd;			/* Command requires a dsj? */

	unsigned char cmd_buf[MAX_IO_BUF];
};


struct amigo_identify {
	short id;
};


/*
** undocumented ioctl's for amigo devices, used by mediainit
*/
#define	AMIGO_IOCMD_IN		_IOWR('A', 1, struct combined_buf)
#define AMIGO_IDENTIFY		_IOR('A', 2, struct amigo_identify)
#define AMIGO_LOCK_DEVICE	_IO('A',3)
#endif /* __hp9000s300 */
#endif /* _SYS_AMIGO_INCLUDED */
