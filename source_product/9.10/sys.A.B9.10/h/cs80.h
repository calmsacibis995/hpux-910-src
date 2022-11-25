/* @(#) $Revision: 1.21.83.5 $ */     
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/cs80.h,v $
 * $Revision: 1.21.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 22:19:49 $
 */
#ifndef _SYS_CS80_INCLUDED /* allows multiple inclusion */
#define _SYS_CS80_INCLUDED

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

typedef long signed32;
typedef unsigned long unsigned32;
typedef short signed16;
typedef unsigned short unsigned16;
typedef char signed8;
typedef unsigned char unsigned8;


#ifdef _WSIO
struct sva_type {
	unsigned16 utb;		/* upper two bytes */
	unsigned32 lfb;		/* lower four bytes */
};


/*
**  describe's controller type field masks
*/
#define CT_MULTI_UNIT	0x01
#define CT_MULTI_PORT	0x02
#define CT_SUBSET_80	0x04
#endif /* _WSIO */

/*
**  describe field byte counts
*/
#define	DESCRIBE_BYTES	37	/* total bytes returned by the describe cmd */
#define	CNTRL_BYTES	 5	/* controller description byte count */
#define	UNIT_BYTES	19	/* unit description byte count */
#define	VOLUME_BYTES	13	/* volume description byte count */

struct cntrl_desc {
	unsigned16	iv;	/* installed unit word, bits */
	unsigned16	mitr;	/* max inst. xfr rate (Kbytes) */
	unsigned8	ct;	/* controller type */
};

struct unit_desc {
	unsigned8	dt;	/* generic device type */
	unsigned8	dn[3];	/* device number (6 BCD digits) */
	unsigned16	nbpb;	/* # bytes per block */
	unsigned8	nbb;	/* # of blocks which can be buffered */
	unsigned8	rbs;	/* reccomendcd burst size */
	unsigned16	blocktime; /* block time in micorseconds */
	unsigned16	catr;	/* continuous avg xfr rate (Kbytes) */
	unsigned16	ort;	/* optimal retry time in centiseconds */
	unsigned16	atp;	/* access time parameter in centiseconds */
	unsigned8	mif;	/* maximum interleave factor */
	unsigned8	fvb;	/* fixed volume byte: 1/volume */
	unsigned8	rvb;	/* removeable volume byte: 1/volume */
};

struct volume_desc {
	unsigned	maxcadd:24; /* maximum cylinder address */
	unsigned8	maxhadd;    /* maximum head address */
	unsigned16	maxsadd;    /* maximum sector address */
#ifdef __hp9000s800
	unsigned16	maxsvadd_utb; /* max single vector address bytes 0-1 */
	unsigned32	maxsvadd_lfb; /* max single vector address bytes 2-5 */
#else /* s300 */
	struct sva_type	maxsvadd;   /* maximum single vector address */
#endif /* __hp9000s800 */
	unsigned8	currentif;  /* current interleave factor */
};

struct describe_type {
		/* controller description field */
	union {
		unsigned8 cntrl_bytes[CNTRL_BYTES];
		struct cntrl_desc controller;
	} controller_tag;
		/* unit description field */
	union {
		unsigned8 unit_bytes[UNIT_BYTES];
		struct unit_desc unit;
	} unit_tag;
		/* volume description field */
	union {
		unsigned8 volume_bytes[VOLUME_BYTES];
		struct volume_desc volume;
	} volume_tag;
};


#ifdef _WSIO
/* reject errors field */
#define channel_parity_error		2
#define illegal_opcode			5
#define module_addressing		6
#define address_bounds			7
#define parameter_bounds		8
#define illegal_parameter		9
#define message_sequence		10
#define message_length			12

/* fault errors field */
#define cross_unit			17
#define controller_fault		19
#define unit_fault			22
#define diagnostic_result		24
#define operator_release_required 	26
#define diagnostic_release_required 	27
#define internal_maint_required		28
#define power_fail			30
#define retransmit			31

/* access errors field */
#define illegal_parallel_operation 	32
#define uninitialized_media		33
#define no_spares_available		34
#define not_ready			35
#define write_protect			36
#define no_data_found			37
#define unrecovb_data_overflow		40
#define unrecovb_data			41
#define end_of_file			43
#define end_of_volume			44

/* information errors field */
#define operator_request		48
#define diagnostic_request		49
#define intnl_maint_reqst		50
#define media_wear			51
#define latency_induced			52
#define error_bit_53			53
#define error_bit_54			54
#define auto_sparing_invoked		55
#define error_bit_56			56
#define recovb_data_overflow		57
#define marginal_data			58
#define recovb_data			59
#define error_bit_60			60
#define maint_track_overflow		61
#define error_bit_62			62
#define error_bit_63			63

/* pseudo error bit */
#define NO_ERRORS			64


struct status_mask_type {
	unsigned16 s_m[4];
	};

struct cs_status_type {
	unsigned8  vvvvuuuu;
	signed8	requesting_unit;
	struct status_mask_type errorbits;
	union {
		unsigned8 info[10];		/* generic way to index */
		struct { 
			struct sva_type nta;	/* new target address */
			int faulting;		/* fault log */
		} nta_fault;
		struct { 
			struct sva_type aaa;	/* affected area address */
			int afl;		/* affected field length */
		} aaa_fault;
		signed8 uee[6];			/* units experiencing errors */
		unsigned8 dor[6];		/* diagnostic results */
		struct sva_type ta;		/* target address */
		struct sva_type bba;		/* bad block address */
		signed8 urr[6];			/* units requesting release */
		struct sva_type btbs;		/* block to be spared */
		struct sva_type rba;		/* recovb block address */
	} st_u;
};

typedef enum { allow_release_timeout, suppress_release_timeout } t_type;

typedef enum { disable_auto_release, enable_auto_release} z_type;

#define CMDlocate_and_read 	0x00
#define CMDlocate_and_write	0x02
#define CMDlocate_and_verify	0x04
#define CMDspare_block		0x06
#define CMDcopy_data		0x08
#define CMDcold_load_read	0x0A
#define CMDrequest_status	0x0D
#define CMDrelease		0x0E
#define CMDrelease_denied	0x0F
#define CMDset_address_1V	0x10
#define CMDset_address_3V	0x11
#define CMDset_block_disp	0x12
#define CMDset_length		0x18
#define UNIT_BASE		0x20
#define CMDinit_util_NEM	0x30
#define CMDinit_util_REM	0x31
#define CMDinit_util_SEM	0x32
#define CMDinit_diagnostic	0x33
#define CMDno_op		0x34
#define CMDdescribe		0x35
#define CMDinit_media		0x37
#define CMDset_options		0x38
#define CMDset_rps		0x39
#define CMDset_retry_time	0x3A
#define CMDset_release		0x3B
#define CMDset_burst_LBO	0x3C
#define CMDset_burst_ABT	0x3D
#define CMDset_status_mask	0x3E
#define VOLUME_BASE		0x40
#define CMDset_retadd_mode	0x48
#define CMDwrite_file_mark	0x49
#define CMDunload		0x4A

#define XCMDchan_indep_clear	0x08
#define XCMDcancel		0x09

typedef unsigned8 CMD_type;

#define TRANSPARENT_SEC SCG_BASE + 18
#define COMMAND_SEC	SCG_BASE +  5
#define EXECUTION_SEC	SCG_BASE + 14
#define REPORTING_SEC	SCG_BASE + 16


/* status masks */

#define B(error) (0x8000>>(error % 16))

/* reject errors field */
#define status_mask_0 0

/* fault errors field */
#define status_mask_1 0

/* access errors field */
#define status_mask_2 0

/* information errors field */
#define status_mask_3	B(media_wear) |\
			B(latency_induced) |\
			B(error_bit_53) |\
			B(error_bit_54) |\
			B(auto_sparing_invoked) |\
			B(error_bit_56) |\
			B(recovb_data_overflow) |\
			B(marginal_data) |\
			B(recovb_data) |\
			B(error_bit_60) |\
			B(maint_track_overflow) |\
			B(error_bit_62) |\
			B(error_bit_63)
#endif /* _WSIO */

#ifndef _WSIO /* 800 only */
/* define the table types used by AMUX Eagle */

#define CTRL_TBL_TYPE	0x03
#define UV_TBL_TYPE	0x04
#define AV_TBL_TYPE	0x06
#define END_TBL_TYPE	0xff

/* define maximum table sizes for AMUX Eagle */
/*  MAX_CACHE_LINE defined in llio.h         */

#define MAX_CACHE_LINE   64
#define MAX_MSG_LEN	(MAX_CACHE_LINE * 2) 
#define TBL_SIZE	0x06
#define EAGLE_CTRL_HDR	0x08
#define CASCADE_CTRL_HDR 0x10
#define EAGLE_UV_HDR	0x12
#define EAGLE_UV_REC	0x0e
#define INSTLD_UNIT_BYTE  1
#define HDR_SIZE_BYTE     3
#define CTRL_TYPE_BYTE    4

struct eagle_extend_desc {
	unsigned char		msg_len[2];
	/* controller information */
	unsigned char		ctrl_tbl_desc[TBL_SIZE];
	unsigned char		ctrl_hdr_bytes[EAGLE_CTRL_HDR];
	/* unit volume information */
	unsigned char		uv_tbl_desc[TBL_SIZE];
	unsigned char		uv_hdr_bytes[EAGLE_UV_HDR];
	unsigned char		uv_rec_bytes[EAGLE_UV_REC];
	/* end of information table */
	unsigned char		end_tbl_desc[TBL_SIZE];
};

/*
 * Note: table types used by CASCADE are shared with AMUX Eagle
 * (see above).
 * ALSO: This struct assumes a SINGLE Unit is being accessed.
 * If performing ext describe on Controller, all units' info returned.
 */

typedef struct {
	unsigned16	iv;	/* installed unit word, bits */
	unsigned16	mitr;	/* max inst. xfr rate (Kbytes) */
	unsigned8	ct;	/* controller type */
#define SINGLE_UNIT  0
#define MULTI_UNIT   1
#define MULTI_PORT   2
#define PBUS_ONLY    3
#define MULTI_U_PORT 4
	unsigned8	host_id;    /* Host port ID */
	unsigned8	host_ports; /* Number of Host ports provided */
	unsigned8	cont_mode;  /* Controller mode */
#define SINGLE_CTRL_UNIT 0
#define INDEPENDENT  1
#define TWO_PLUS_TWO 2
#define ONE_W_PARITY 3
#define TWO_STRIPED  4
#define TWO_W_PARITY 5
#define FOUR_STRIPED 6
#define FOUR_W_PARITY 7
	unsigned8	cn[3];   /* Controller Prod. ID (6 BCD digits) */
	unsigned8	csi[4];  /* Controller Specific info */
	unsigned8	resrvd;  /* reserved */
}cascade_cntrl_desc;

typedef struct {
	unsigned8	unit_no;/* Unit Number */
	unsigned8	dt;	/* generic device type */
#define FIXED_DISK_GENERIC_DEV_TYPE  0
#define REMOVE_DISK_GENERIC_DEV_TYPE 1
#define TAPE_GENERIC_DEV_TYPE        2
	unsigned8	dn[3];	/* device number (6 BCD digits) */
	unsigned8	nbpb[2];/* # bytes per block */
	unsigned8	nbb;	/* # of blocks which can be buffered */
	unsigned8	rbs;	/* recommended burst size */
	unsigned8	blocktime[2]; /* block time in micorseconds */
	unsigned8	catr[2];/* continuous avg xfr rate (Kbytes) */
	unsigned8	ort[2];	/* optimal retry time in centiseconds */
	unsigned8	atp[2];	/* access time parameter in centiseconds */
	unsigned8	mif;	/* maximum interleave factor */
}fl_unit_desc;

typedef struct {
	unsigned	maxcadd:24; /* maximum cylinder address */
	unsigned8	maxhadd;    /* maximum head address */
	unsigned8	maxsadd[2]; /* maximum sector address */
#ifdef __hp9000s800
	unsigned8	maxsvadd_utb[2]; /* max single vector addr bytes 0-1 */
	unsigned8	maxsvadd_lfb[4]; /* max single vector addr bytes 2-5 */
#else /* not __hp9000s800 */
	struct sva_type maxsvadd;   /* maximum single vector address */
#endif /* else not __hp9000s800 */
	unsigned8	currentif;  /* current interleave factor */
	unsigned8	volume_no;  /* Volume number */
}fl_volume_desc;

struct cascade_extend_desc {
	unsigned char		msg_len[2];
	/* controller information */
	unsigned char		ctrl_tbl_desc[TBL_SIZE];
	union {
		unsigned char		ctrl_hdr_bytes[CASCADE_CTRL_HDR];
		cascade_cntrl_desc  ctrl_hdr;
        } ctrl_u;
	/* unit volume information */
	unsigned char		uv_tbl_desc[TBL_SIZE];
	union {
		unsigned char		uv_hdr_bytes[EAGLE_UV_HDR];
		fl_unit_desc		 unit_hdr;
        } unit_u;
	union {
		unsigned char		uv_rec_bytes[EAGLE_UV_REC];
		fl_volume_desc		volume_rec;
        } vol_u;
	/* end of information table */
	unsigned char		end_tbl_desc[TBL_SIZE];
};

/* Excalibur cartridge tape extended describe tables. */

/*
 * Note: table types used by Excalibur are shared with AMUX Eagle
 * (see above).
 */

/* Sizes of pieces of extended describe structure specific to Excalibur. */
#define EXCAL_CTRL_HDR		8
#define EXCAL_UV_HDR		18
#define EXCAL_UV_REC		14

struct excal_extend_desc {
	/* controller information */
	unsigned char		ctrl_tbl_desc[TBL_SIZE];
	unsigned char		ctrl_hdr_bytes[EXCAL_CTRL_HDR];
	/* unit volume information */
	unsigned char		uv_tbl_desc[TBL_SIZE];
	unsigned char		uv_hdr_bytes[EXCAL_UV_HDR];
	unsigned char		uv_rec_bytes[EXCAL_UV_REC];
	/* end of information table */
	unsigned char		end_tbl_desc[TBL_SIZE];
};

union extend_desc_type {
	unsigned char			data_bytes[MAX_MSG_LEN];
	struct eagle_extend_desc	eagle_desc;
	struct cascade_extend_desc	cascade_desc;
	struct excal_extend_desc	excal_desc;
};
#endif /* !_WSIO */

/*
 *  CIOC_VERIFY parameter structure
 */

#define VERIFYPARMS_SIZE        8
struct verify_parms {
	long	start;		/* start address in bytes */
	long	length;		/* length in bytes */
};


/*
 *  CIOC_MARK parameter structure
 */

#define MARKPARMS_SIZE  4
struct mark_parms {
	long	start;		/* start address in bytes */
};


/*
 * CIOC_GET_SECURITY_ID structure
 */
struct security_id_info {
	unsigned char security_num[8];
};


/*
 *  CIOC_SET_OPTIONS bit definitions  -  allowed only with cartridge tapes
 */

#define	SO_CHARACTER_COUNT	0x01	/* enable character count */
#define	SO_SKIP_SPARING		0x02	/* skip vs jump auto sparing */
#define	SO_AUTO_SPARING		0x04	/* enable auto sparing */
#define	SO_IMMEDIATE_REPORT	0x08	/* enable immediate report (Buf/Mer) */
#define	SO_MEDIA_UNLOAD		0x80	/* media vs cartridge unload (Mer) */

#ifdef _WSIO
/*
 *  CIOC_SET_CMD parameter structure
 */

#define	CMD_LEN	20	/* maximum # data bytes in the cmd message */
#define CMDPARM_SIZE    29      /* max total CMD length */

struct cmd_parms {
	long	cmd_ticks;		/* timeout for C-phase */
	long	exec_ticks;		/* timeout for E-phase if appl. */
	char	cmd_length;		/* actual command message length */
	char	cmd_message[CMD_LEN];	/* comand message byte string */
};


/*
**  currently undocumented CS80 ioctl functions used by tcio and mediainit
*/
#define	CIOC_DESCRIBE		_IOR('C', 1, struct describe_type)
#define	CIOC_UNLOAD		_IO ('C', 2)
#define	CIOC_VERIFY		_IOW('C', 3, struct verify_parms)
#define	CIOC_MARK		_IOW('C', 4, struct mark_parms)
#define	CIOC_SET_OPTIONS	_IOW('C', 5, char)
#define	CIOC_CMD_MODE		_IOW('C', 6, int)
#define	CIOC_SET_CMD		_IOW('C', 7, struct cmd_parms)
#define	CIOC_CMD_REPORT		_IOW('C', 8, struct cmd_parms)
#define	CIOC_LOAD		_IOW('C', 9, char)
#define CIOC_GET_STATUS     _IOR('C', 10, struct cs_status_type)

/*
  These defines were added for diagnosic support in the cs80_ioctl routine
*/
#define CIOC_CANCEL     _IO('C', 11 )
#define CIOC_CHAN_IND_CLR   _IOW('C', 12, char )

/*  Undocumented CS80 ioctl call used by RMB-UX to detect media change.
 *  A non-zero 'arg' to ioctl(fd, req, arg) turns on "detect media change"
 */
#define	CIOC_MEDIA_CHNG		_IOW('C', 10, int)

#define CIOC_GET_SECURITY_ID _IOR('C',11,struct security_id_info)
#endif /* _WSIO */

#ifndef _WSIO /* 800 only */
struct size_info {
	int	blocks;		/* blocks in section (partition) */
	int	block_size;	/* bytes per block (DEV_BSIZE)   */
};

/* Structures for specific Excalibur ioctls. */

struct excal_tape_info {		/* Return_excalibur_tape_information */
	unsigned char msg_len;
	unsigned char media_loaded_flag;
	unsigned char media_protect_flag;
	unsigned char cartridge_code;
	unsigned char cartridge_format;
	unsigned char compress_code;
	unsigned char compress_algorithm;
	unsigned char EORD_known_flag;
	unsigned char EORD_host_addr[4];
	unsigned char EORD_addr[4];
	unsigned char DABS_extent[4];
};

struct excal_pfail_status {		/* Return_powerfail_status */
	unsigned char msg_len;		
	unsigned char status_flag;
	unsigned char data_loss_flag;
	unsigned char first_host_block_not_written_addr[4];
};

/*
 *  currently undocumented CS80 ioctl functions used by tcio

 */

#define CIOC_DESCRIBE		_IOR('d',1,struct describe_type)
#define CIOC_UNLOAD		_IO('d',2) 
#define CIOC_MARK		_IOW('d',3,struct mark_parms)
#define CIOC_VERIFY		_IOW('d',4,struct verify_parms)
#define CIOC_AMIGO_ID 		_IOR('d',5,short)
#define CIOC_EXCLUSIVE 		_IOW('d',6,int)	/* 0/1 = non/exclusive */
#define CIOC_SET_OPTIONS	_IOW('d',7,char)
#define CIOC_SIZE		_IOR('d',8,struct size_info)
#define CIOC_LOAD		_IOW('d',9,char)
#define CIOC_EXT_DESCRIBE	_IOR('d',10,union extend_desc_type)
#define CIOC_GET_TAPE_INFO	_IOR('d',11,struct excal_tape_info)
#define CIOC_GET_PFAIL_STATUS	_IOR('d',12,struct excal_pfail_status)



#define CIOC_GET_SECURITY_ID _IOR('d',13,struct security_id_info)

/* Declarations for the generic transparent mode interface. */

struct trans_report {		/* structure of status report */
	int generic_status;		/* worked, timed out, or failed */
	int report_class;		/* descriptor for device_status */
	char device_status[56];	/* real status returned by device */
};

/* Position of generic status report in user's buffer. */
#define trans_report_offset 64

/* Generic statuses. */
#define TRANS_NORM 0		/* generic status for "okay" */
#define TRANS_FAIL 1		/* generic status for failure */
#define TRANS_TIMEOUT 2		/* generic status for timeout */
#define TRANS_POWER_ON 4	/* generic status for power on */
#define TRANS_WARN 8		/* generic status for C-bit set (disks) */


/* Report classes. */
#define NO_STAT 0		/* no status available */
#define CS80_CIO 1		/* device status for CIO bus driver device */
#define CS80_NIO 2		/* device status for NIO bus driver device */
				/* device statuses for Alink driver device */
#define CS80_ALINK_1 3		/*	if tstat = 0        */
#define CS80_ALINK_2 4		/* 	if tstat = 2 or 8   */
#define CS80_ALINK_3 5		/*	if tstat = 10 or 11 */
#define CS80_ALINK_4 6		/*	all other tstats    */
#endif /* !_WSIO */

#endif /* _SYS_CS80_INCLUDED */
