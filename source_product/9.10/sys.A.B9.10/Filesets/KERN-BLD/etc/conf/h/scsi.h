/*
 * @(#)scsi.h: $Revision: 1.8.83.4 $ $Date: 93/09/17 18:33:39 $
 * $Locker:  $
 */

#ifndef _SYS_SCSI_INCLUDED
#define _SYS_SCSI_INCLUDED

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

/*
** SCSI and SCSI-2 inquiry structures
*/

struct inquiry {
    unsigned char	dev_type;
    unsigned char	rmb:1;
    unsigned char	dtq:7;
    unsigned char	iso:2;
    unsigned char	ecma:3;
    unsigned char	ansi:3;
    unsigned char	resv:4;
    unsigned char	rdf:4;
    unsigned char	added_len;
    unsigned char	dev_class[3];
	     char	vendor_id[8];
	     char	product_id[16];
	     char	rev_num[4];
    unsigned char	vendor_spec[20];
    unsigned char	resv4[40];
    unsigned char	vendor_parm_bytes[32];
};

struct inquiry_2 {
    unsigned char	periph_qualifier:3;
    unsigned char	dev_type:5;
    unsigned char	rmb:1;
    unsigned char	dtq:7;
    unsigned char	iso:2;
    unsigned char	ecma:3;
    unsigned char	ansi:3;
    unsigned char	aenc:1;
    unsigned char	trmiop:1;
    unsigned char	resv1:2;
    unsigned char	rdf:4;
    unsigned char	added_len;
    unsigned char	resv2[2];
    unsigned char	reladr:1;
    unsigned char	wbus32:1;
    unsigned char	wbus16:1;
    unsigned char	sync:1;
    unsigned char	linked:1;
    unsigned char	resv3:1;
    unsigned char	cmdque:1;
    unsigned char	sftre:1;
	     char	vendor_id[8];
	     char	product_id[16];
	     char	rev_num[4];
    unsigned char	vendor_spec[20];
    unsigned char	resv4[40];
    unsigned char	vendor_parm_bytes[32];
};

/*
** data structure for SIOC_INQUIRY
*/
union inquiry_data {
    struct inquiry	inq1;		/* SCSI-1 inquiry	*/
    struct inquiry_2	inq2;		/* SCSI-2 inquiry	*/
};

/*
** data structure for SIOC_VPD_INQUIRY
*/
struct vpd_inquiry {
    char		page_code;	/* VPD page code		*/
    char		page_buf[126];	/* buffer for VPD page info	*/
};

/*
** bits in parms field of xsense_aligned and xsense
*/
#define	FM	0x08
#define	EOM	0x04
#define	ILI	0x02

/*
** struct xsense_aligned is for examining the sense data of SCSI-1
** and CCS devices.
*/
struct xsense_aligned {
	unsigned char	valid:1;
	unsigned char	error_class:3;
	unsigned char	error_code:4;
	unsigned char	seg_num;
	unsigned char	parms:4;
	unsigned char	sense_key:4;
	unsigned char	lba[4];
	unsigned char	add_len;
	unsigned char	copysearch[4];	/* Unused by HP-UX */
	unsigned char	sense_code;
	unsigned char	resv;
	unsigned char	fru;
	unsigned char	field;
	unsigned char	field_ptr[2];
	unsigned char	dev_error[4];
	unsigned char	misc_bytes[106];
};

/*
** struct sense_2_aligned is for examining the sense data of SCSI-2 devices.
*/
struct sense_2_aligned {
	unsigned char	info_valid:1;
	unsigned char	error_code:7;
	unsigned char	seg_num;
	unsigned char	filemark:1;
	unsigned char	eom:1;
	unsigned char	ili:1;
	unsigned char	resv:1;
	unsigned char	key:4;
	unsigned char	info[4];
	unsigned char	add_len;
	unsigned char	cmd_info[4];
	unsigned char	code;
	unsigned char	qualifier;
	unsigned char	fru;
	unsigned char	key_specific[3];
	unsigned char	add_sense_bytes[113];
};

#ifdef _WSIO
/*
** struct xsense is provided for backwards source code compatibility only.
** struct xsense_aligned is the appropriate struct for examining the sense
** data of SCSI-1 and CCS devices.
*/
struct xsense {
	unsigned char	valid:1;
	unsigned char	error_class:3;
	unsigned char	error_code:4;
	unsigned char	seg_num;
	unsigned char	parms:4;
	unsigned char	sense_key:4;
	unsigned char	lba[4];
	unsigned char	add_len;
	unsigned char	copysearch[4];	/* Unused by HP-UX */
	unsigned char	sense_code;
	unsigned char	resv;
	unsigned char	fru;
	unsigned char	field;
	unsigned short	field_ptr;
	unsigned long	dev_error;
	unsigned char	misc_bytes[106];
};

/*
** struct sense_2 is provided for backwards source code compatibility only.
** struct sense_2_aligned is the appropriate struct for examining the sense
** data of SCSI-2 devices.
*/
struct sense_2 {
	unsigned char	info_valid:1;
	unsigned char	error_code:7;
	unsigned char	seg_num;
	unsigned char	filemark:1;
	unsigned char	eom:1;
	unsigned char	ili:1;
	unsigned char	resv:1;
	unsigned char	key:4;
	unsigned char	info[4];
	unsigned char	add_len;
	unsigned int	cmd_info;
	unsigned char	code;
	unsigned char	qualifier;
	unsigned char	fru;
	unsigned char	key_specific[3];
	unsigned char	add_sense_bytes[113];
};
#endif /* _WSIO */

/*
** data structure for SIOC_XSENSE
*/
union sense_data {
	struct xsense_aligned r_sense1a;	/* SCSI and CCS devices */
	struct sense_2_aligned r_sense2a;	/* SCSI-2 devices */
#ifdef _WSIO
	struct xsense  r_sense1; /* do not use; for compatibility only */
	struct sense_2 r_sense2; /* do not use; for compatibility only */
#endif /* _WSIO */
};

/*
** SCSI-2 error_code
*/
#define S_CURRENT_ERROR	0x70
#define S_DEFERRED_ERROR	0x71

/*
** SCSI-2 sense_key
*/
#define S_NO_SENSE		0x00
#define S_RECOVERRED_ERROR	0x01
#define S_NOT_READY		0x02
#define S_MEDIUM_ERROR		0x03
#define S_HARDWARE_ERROR	0x04
#define S_ILLEGAL_REQUEST	0x05
#define S_UNIT_ATTENTION	0x06
#define S_DATA_PROTECT		0x07
#define S_BLANK_CHECK		0x08
#define S_VENDOR_SPECIFIC	0x09
#define S_COPY_ABORTED		0x0a
#define S_ABORTED_COMMAND	0x0b
#define S_EQUAL			0x0c
#define S_VOLUME_OVERFLOW	0x0d
#define S_MISCOMPARE		0x0e
#define S_RESERVED		0x0f

/*
** data structure for SIOC_CAPACITY
*/
struct capacity {
	int	lba;
	int	blksz;
};

#ifndef _WSIO
/*
** data structure for SIOC_BLANK_SECTOR_SEARCH and SIOC_WRITTEN_SECTOR_SEARCH
*/
struct sector_search {
	unsigned char	error;
	unsigned char	found;
	unsigned char	addr_msb;
	unsigned char	addr_2nd;
	unsigned char	addr_3rd;
	unsigned char	addr_lsb;
};
#endif /* _WSIO */

#ifdef _WSIO
struct sctl_io
{
	unsigned flags;			/* IN: SCTL_READ */
	unsigned cdb_length;		/* IN */
	unsigned char cdb[16];		/* IN */
	void *data;			/* IN */
	unsigned data_length;		/* IN */
	unsigned max_msecs;		/* IN: milli-seconds before abort */
	unsigned data_xfer;		/* OUT */
	unsigned cdb_status;		/* OUT: SCSI status */
	unsigned char sense[256];	/* OUT */
	unsigned sense_status;		/* OUT: SCSI status */
	unsigned sense_xfer;		/* OUT: bytes of sense data received */
	unsigned char reserved[64];
};

/*
** values for sctl_io->cdb_status and sctl_io->sense_status
*/
#define SCTL_INVALID_REQUEST	0x0100
#define SCTL_SELECT_TIMEOUT	0x0200
#define SCTL_INCOMPLETE		0x0400

/*
** sctl_io->flags bits
*/
#define SCTL_READ	0x00000001
#define SCTL_INIT_WDTR	0x00000002
#define SCTL_INIT_SDTR	0x00000004
#define SCTL_NO_ATN	0x00000008 /* select without ATN -- no SCSI messages */
#endif /* _WSIO */

#ifdef _WSIO
/*
** data structure for SIOC_FORMAT
*/
struct sioc_format {
	short fmt_optn;
	short interleave;
};

#define	SCSI_CMD_LEN	12	/* max # data bytes in the cmd message */
#undef	CMD_LEN
#define CMD_LEN		SCSI_CMD_LEN	/* use SCSI_CMD_LEN */

/*
** data structure for SIOC_SET_CMD
*/
struct scsi_cmd_parms {
	unsigned char	cmd_type;	/* command type (6, 10, or 12 byte) */
	unsigned char	cmd_mode;	/* environment (select with ATN) */
	unsigned long	clock_ticks;	/* timeout: maximum disconnect time
					 * (in 20 msec ticks) */
	unsigned char	command[SCSI_CMD_LEN];	/* SCSI Commnd to be sent */
};
#endif /* _WSIO */

/*
** data structure for SIOC_ERASE
*/
struct scsi_erase {
	unsigned int	start_lba;
	unsigned short	block_cnt;
};

/*
** data structure for SIOC_VERIFY and SIOC_VERIFY_BLANK
*/
struct scsi_verify {
	unsigned int	start_lba;
	unsigned short	block_cnt;
};

#ifdef _WSIO
/*
** data structure for SIOC_GET_BLOCK_LIMITS
*/
struct scsi_block_limits {
	unsigned int	min_blk_size;
	unsigned int	max_blk_size;
};

/*
** data structure for SIOC_INIT_ELEM_STAT
*/
struct element_addresses {
	unsigned short  first_transport;
	unsigned short  num_transports;
	unsigned short  first_storage;
	unsigned short  num_storages;
	unsigned short  first_import_export;
	unsigned short  num_import_exports;
	unsigned short  first_data_transfer;
	unsigned short  num_data_transfers;
};
#endif /* _WSIO */

/*
** data structure for SIOC_ELEMENT_STATUS
*/
struct element_status {
	unsigned short  element;          /* element address */

	unsigned char   resv1:2;
	unsigned char   import_enable:1;  /* allows media insertion (load) */
	unsigned char   export_enable:1;  /* allows media removal (eject) */
	unsigned char   access:1;         /* transport element accessible */
	unsigned char   except:1;         /* is in an abnormal state */
	unsigned char   operator:1;       /* medium positioned by operator */
	unsigned char   full:1;           /* holds a a unit of media */

	unsigned char   resv2;
	unsigned char   sense_code;       /* info. about abnormal state */
	unsigned char   sense_qualifier;  /* info. about abnormal state */

	unsigned char   not_bus:1;        /* transfer device SCSI bus differs */
	unsigned char   resv3:1;
	unsigned char   id_valid:1;       /* bus_address is valid */
	unsigned char   lu_valid:1;       /* lun is valid */
	unsigned char   sublu_valid:1;    /* sub_lun is valid */
	unsigned char   lun:3;            /* transfer device SCSI LUN */

	unsigned char   bus_address;      /* transfer device SCSI address */
	unsigned char   sub_lun;          /* sub-logical unit number */

	unsigned char   source_valid:1;   /* source_element is valid */
	unsigned char   invert:1;         /* media in element was inverted */
	unsigned char   resv4:6;

	unsigned short  source_element;   /* last storage location of medium */
	char            pri_vol_tag[36];  /* volume tag (device optional) */
	char            alt_vol_tag[36];  /* volume tag (device optional) */
	unsigned char   misc_bytes[168];  /* device specific */
};

#ifdef _WSIO
/*
** data structure for SIOC_RESERVE and SIOC_RELEASE
*/
struct reservation_parms {
	unsigned short  element;
	unsigned char   identification;
	unsigned char   all_elements;
};

/*
** data structure for SIOC_MOVE_MEDIUM
*/
struct move_medium_parms {
	unsigned short  transport;
	unsigned short  source;
	unsigned short  destination;
	unsigned char   invert;
};

/*
** data structure for SIOC_EXCHANGE_MEDIUM
*/
struct exchange_medium_parms {
	unsigned short  transport;
	unsigned short  source;
	unsigned short  first_destination;
	unsigned short  second_destination;
	unsigned char   invert_first;
	unsigned char   invert_second;
};

struct sioc_lun_parms {
	unsigned int flags;
	unsigned int max_q_depth;	/* maximum active I/O's */
	unsigned int reserved[4];	/* reserved for future use */
};

/*
** for sioc_lun_parms.flags only
*/
#define SCTL_TAGS_ACTIVE	0x00000001

/*
** for both sioc_lun_parms.flags and sioc_lun_limits.flags
*/
#define SCTL_ENABLE_TAGS	0x80000000

struct sioc_tgt_parms {
	unsigned int flags;
	unsigned int width;		/* bits per word */
	unsigned int xfer_rate;		/* words per second */
	unsigned int reqack_offset;	/* REQ/ACK offset */
	unsigned int reserved[4];	/* reserved for future use */
};

/*
** sioc_tgt_parms.flags only
*/
#define SCTL_SDTR_DONE		0x01
#define SCTL_WDTR_DONE		0x02

/*
** for both sioc_tgt_parms.flags and sioc_tgt_limits.flags
*/
#define SCTL_ENABLE_SDTR	0x80000000
#define SCTL_ENABLE_WDTR	0x40000000

struct sioc_bus_parms {
	unsigned int flags;		/* reserved for future use */
	unsigned int max_width;		/* bits per word */
	unsigned int max_xfer_rate;	/* words per second */
	unsigned int max_reqack_offset;	/* REQ/ACK offset */
	unsigned int reserved[4];	/* reserved for future use */
};

struct sioc_lun_limits {
	unsigned int flags;
	unsigned int max_q_depth;
	unsigned int reserved[4];	/* reserved for future use */
};

struct sioc_tgt_limits {
	unsigned int flags;
	unsigned int max_width;		/* bits per word */
	unsigned int max_xfer_rate;	/* words per second */
	unsigned int max_reqack_offset;	/* REQ/ACK offset */
	unsigned int reserved[4];	/* reserved for future use */
};

struct sioc_bus_limits {
	unsigned int flags;		/* reserved for future use */
	unsigned int max_width;		/* bits per word */
	unsigned int max_xfer_rate;	/* words per second */
	unsigned int max_reqack_offset;	/* REQ/ACK offset */
	unsigned int reserved[4];	/* reserved for future use */
};
#endif /* _WSIO */

/*
** SCSI ioctl's
*/
#define	SIOC_INQUIRY			_IOR('S', 2, union inquiry_data)
#ifdef _WSIO
#define	SIOC_CAPACITY			_IOR('S', 3, struct capacity)
#define	SIOC_CMD_MODE			_IOW('S', 4, int)
#define	SIOC_SET_CMD			_IOW('S', 5, struct scsi_cmd_parms)
#endif /* _WSIO */
#define	_WSIO_SIOC_FORMAT		_IOW('S', 6, struct sioc_format)
#define	_SIO_SIOC_FORMAT		_IOW('S', 6, int)
#ifdef _WSIO
#define SIOC_FORMAT			_WSIO_SIOC_FORMAT
#else /* _WSIO */
#define SIOC_FORMAT			_SIO_SIOC_FORMAT
#endif /* _WSIO */
#define	SIOC_XSENSE			_IOR('S', 7, union sense_data)
#ifdef _WSIO
#define	SIOC_RETURN_STATUS		_IOR('S', 8, int)
#define	SIOC_RESET_BUS			_IO('S', 9)
#define	SIOC_RESET_DEV			_IO('S', 16)
#endif /* _WSIO */
#ifndef _WSIO
#define SIOC_VPD_INQUIRY		_IOWR('S', 10, struct vpd_inquiry)
#define SIOC_DISK_EJECT			_IO('S', 11)
#define SIOC_BLANK_SECTOR_SEARCH	_IOWR('S', 12, struct sector_search)
#define SIOC_WRITTEN_SECTOR_SEARCH	_IOWR('S', 13, struct sector_search)
#endif /* _WSIO */
#define SIOC_GET_IR	                _IOR('S', 14, int)
#define SIOC_SET_IR	                _IOW('S', 15, int)
#ifndef _WSIO
#define SIOC_SYNC_CACHE			_IOW('S', 16, int)
#endif /* _WSIO */
#define SIOC_WRITE_WOE			_IOW('S', 17, int)
#define SIOC_VERIFY_WRITES		_IOW('S', 18, int)
#define SIOC_ERASE			_IOW('S', 19, struct scsi_erase)
#define SIOC_VERIFY_BLANK		_IOW('S', 20, struct scsi_verify)
#define SIOC_VERIFY			_IOW('S', 21, struct scsi_verify)
#ifdef _WSIO
#define SIOC_IO				_IOWR('S', 22, struct sctl_io)
#define SIOC_GET_BLOCK_SIZE		_IOR('S', 30, int)
#define SIOC_SET_BLOCK_SIZE		_IOW('S', 31, int)
#define SIOC_GET_BLOCK_LIMITS		_IOR('S', 32, struct scsi_block_limits)
#define SIOC_GET_POSITION		_IOR('S', 33, int)
#define SIOC_SET_POSITION		_IOW('S', 34, int)
#define SIOC_MEDIUM_CHANGED             _IOR('S', 42, int)
#define SIOC_ABORT			_IO('S', 44)
#define SIOC_INIT_ELEM_STAT		_IO('S', 51)
#define SIOC_ELEMENT_ADDRESSES		_IOR('S', 52, struct element_addresses)
#define SIOC_ELEMENT_STATUS		_IOWR('S', 53, struct element_status)
#define SIOC_RESERVE			_IOW('S', 54, struct reservation_parms)
#define SIOC_RELEASE			_IOW('S', 55, struct reservation_parms)
#define SIOC_MOVE_MEDIUM		_IOW('S', 56, struct move_medium_parms)
#define SIOC_EXCHANGE_MEDIUM		_IOW('S', 57, struct exchange_medium_parms)
#define SIOC_GET_LUN_PARMS		_IOR('S', 58, struct sioc_lun_parms)
#define SIOC_GET_TGT_PARMS		_IOR('S', 59, struct sioc_tgt_parms)
#define SIOC_GET_BUS_PARMS		_IOR('S', 60, struct sioc_bus_parms)
#define SIOC_GET_LUN_LIMITS		_IOR('S', 61, struct sioc_lun_limits)
#define SIOC_GET_TGT_LIMITS		_IOR('S', 62, struct sioc_tgt_limits)
#define SIOC_GET_BUS_LIMITS		_IOR('S', 63, struct sioc_bus_limits)
#define SIOC_SET_LUN_LIMITS		_IOW('S', 64, struct sioc_lun_limits)
#define SIOC_SET_TGT_LIMITS		_IOW('S', 65, struct sioc_tgt_limits)
#define SIOC_SET_BUS_LIMITS		_IOW('S', 66, struct sioc_bus_limits)
#define SIOC_PRIORITY_MODE		_IOW('S', 67, int)
#define SIOC_EXCLUSIVE			_IOW('S', 68, int)
#endif /* _WSIO */

#ifndef _WSIO
/* offset of options byte in ten byte commands */
#define OPTIONS_BYTE_OFFSET	1
/* offset of control byte in ten byte commands */
#define CONTROL_BYTE_OFFSET	9
#endif /* _WSIO */

#ifdef _WSIO
/*
** SCSI device types
*/
#define SCSI_DIRECT_ACCESS	0
#define SCSI_SEQUENTIAL_ACCESS	1
#define SCSI_PROCESSOR		3
#define SCSI_WORM		4
#define SCSI_CDROM		5
#define SCSI_MO			7
#define SCSI_AUTOCHANGER	8
#define SCSI_UNKNOWN_DEV_TYPE	0x1F
#endif /* _WISO */

/*
** Opcodes for all SCSI device types
*/
#define CMDtest_unit_ready	0x00
#define CMDrequest_sense	0x03
#define	CMDinquiry		0x12
#define CMDsend_diagnostic	0x1D

/*
** Opcodes for Direct-Access Devices
*/
#define	CMDrezero_unit		0x01
#define CMDformat_unit		0x04
#define CMDreassign_blocks	0x07
#define CMDread			0x08
#define CMDread6		0x08
#define CMDwrite		0x0A
#define CMDwrite6		0x0A
#define CMDmode_select		0x15
#define CMDreserve		0x16
#define SCSI_CMDrelease		0x17
#undef	CMDrelease
#define CMDrelease		SCSI_CMDrelease
#define CMDmode_sense		0x1A
#define CMDstart_stop_unit	0x1B
#define CMDsend_diag		0x1D
#define CMDprevent_allow_media  0x1E
#define CMDread_capacity	0x25
#define CMDread_ext		0x28	/* Seek is redundant; use read */
#define CMDread10		0x28
#define CMDwrite_ext		0x2A
#define CMDwrite10		0x2A
#define CMDerase		0x2C
#define CMDwriteNverify		0x2E
#define CMDverify		0x2F
#define CMDsync_cache		0x35
#define CMDread_defect_data	0x37
#define CMDwrite_buffer		0x3B
#define CMDread_buffer		0x3C
#define CMDlog_select		0x4C
#define CMDlog_sense		0x4D
#define CMDread12		0xA8
#define CMDwrite12		0xAA
#define CMDread_full		0xF0
#define CMDmedia_test		0xF1
#define CMDaccess_log		0xF2
#define CMDwrite_full		0xFC
#define CMDmanage_primary	0xFD
#define CMDexecute_data		0xFE

/*
** Opcodes for Sequential-Access Devices
*/
#define	CMDrewind		0x01
#define	CMDreq_block_address	0x02
#define CMDread_block_limits	0x05
#define CMDseek_block		0x0C
#define CMDwrite_filemarks	0x10
#define	CMDspace		0x11
#define CMDload			0x1B
#define CMDlocate		0x2B
#define CMDread_position	0x34

/*
** Values for code field in the Space command
*/
#define CMDspace_record         0x0
#define CMDspace_seq_file       0x2
#define CMDspace_file           0x1
#define CMDspace_eod            0x3
#define CMDspace_setmark        0x4
#define CMDspace_seq_setmark    0x5

/*
** Opcodes for Printer Devices
*/
#define CMDformat               0x04
#define CMDprint                0x0A
#define CMDslew_print           0x0B
#define CMDsync_buf             0x10
#define CMDread_buf             0x3C
#define CMDwrite_buf            0x3B
#define CMDreserve_unit         0x16
#define CMDrelease_unit         0x17
#define CMDstop_print           0x1B
#define CMDmode_select_10       0x55
#define CMDmode_sense_10        0x5A
#define CMDread_6               0x08

#ifdef _WSIO
/*
** Opcodes for Medium-Changer Devices
*/
#define	CMDinit_element_status	0x07
#define	CMDread_element_status	0xB8
#define	CMDmove_medium		0xA5
#define CMDexchange_medium	0xA6
#endif /* _WSIO */

/*
** SCSI Bus Phases
*/
#define	DATA_OUT_PHASE	0x00
#define	DATA_IN_PHASE	0x01
#define	CMD_PHASE	0x02
#define	STATUS_PHASE	0x03
#define	MESG_OUT_PHASE	0x06
#define	MESG_IN_PHASE	0x07

/*
** SCSI Messages
*/

#define	MSGcmd_complete		0x00
#define MSGext_message		0x01
#define	MSGsave_data_ptr	0x02
#define	MSGrestore_ptr		0x03
#define	MSGdisconnect		0x04
#define	MSGinit_detect_error	0x05
#define	MSGabort		0x06
#define	MSGmsg_reject		0x07
#define	MSGno_op		0x08
#define	MSGmsg_parity_error	0x09

#define MSGlinked_cmd_complete  0x0A
#define MSGwith_flag		0x01	/* for MSGlinked_cmd_complete */

#define	MSGbus_device_reset	0x0C
#define MSGabort_tag            0x0D
#define MSGclear_queue          0x0E
#define MSGinitiate_recovery    0x0F
#define MSGrelease_recovery     0x10
#define MSGterminate_io_proc    0x11

#define MSGsimple_queue_tag     0x20
#define MSGhead_of_queue_tag    0x21
#define MSGordered_queue_tag    0x22
#define MSGignore_wide_residue  0x23

/* extended message codes */
#define MSGmodify_data_ptr      0x00
#define MSGsync_req             0x01
#define MSGwide_req             0x03

#define	MSGidentify		0x80
#define	MSGident_discon		0x40	/* okay to disconnect */

/*
** SCSI Status
*/
#define S_GOOD			0x00
#define S_CHECK_CONDITION	0x02
#define S_CONDITION_MET		0x04
#define S_BUSY			0x08
#define S_INTERMEDIATE		0x10
#define S_RESV_CONFLICT		0x18
#define S_COMMAND_TERMINATED	0x22
#define S_QUEUE_FULL		0x28
#define S_I_CONDITION_MET	(S_INTERMEDIATE + S_CONDITION_MET)


/*
** Mode Sense / Select
*/

/*
** Page Control (PC) values
*/
#define CURRENT    0 
#define CHANGEABLE 1
#define DEFAULT    2
#define SAVED      3

/*
** Page Codes  
*/
#define ERR_RECOV_PAGE            1
#define DISCONNECT_RECONNECT_PAGE 2
#define FORMAT_DEV_PAGE           3
#define DISK_GEOMETRY_PAGE        4
#define CACHE_PAGE                8
#define ALL_PAGES              0x3F  

#define WRITE_PROTECT_BIT      0x80	/* WP bit in Mode Sense header */

struct sense_hdr {
	unsigned char	length;
	unsigned char	medium_type;
	unsigned char	misc;
	unsigned char	bd_len;
};

struct block_desc {
	unsigned char	density_code;
	unsigned char	blocks[3];
	unsigned char	resv;
	unsigned char	block_len[3];
};

struct page_desc {
	unsigned char	page_code;
	unsigned char	page_len;
};

/*
** Bit fields for error recovery parameter: error_bits
*/
#define	DCR	0x01
#define	DTE	0x02
#define	PER	0x04
#define	EEC	0x08
#define	RC	0x10
#define	TB	0x20
#define	ARRE	0x40
#define	AWRE	0x80

struct error_recovery {
	struct	page_desc page_desc;
	unsigned char	error_bits;
	unsigned char	retry_cnt;
	unsigned char	correction;
	char		head_offset;
	char		data_strobe_offset;
	unsigned char	resv;
	unsigned short	recovery_time;
};

struct disc_recon {
	struct	page_desc page_desc;
	unsigned char	bfr_full_ratio;
	unsigned char	bfr_empty_ratio;
	unsigned short	bus_inactive;
	unsigned short	discon_time;
	unsigned short	connect_time;
	unsigned char	resv[2];
};

struct dad_form_parms {
	struct	page_desc page_desc;
	unsigned short	tracks_per_zone;
	unsigned short	alt_sectors_per_zone;
	unsigned short	alt_tracks_per_zone;
	unsigned short	alt_tracks_per_vol;
	unsigned short	sectors_per_track;
	unsigned short	bytes_per_phys_sector;
	unsigned short	interleave;
	unsigned short	track_skew_factor;
	unsigned short	cyl_skew_factor;
	unsigned char	parms;
	unsigned char	resv[3];
};

struct geometry_parms {
	struct	page_desc page_desc;
	unsigned char	num_cyl[3];
	unsigned char	num_heads;
	unsigned char	write_precomp[3];
	unsigned char	write_current[3];
	unsigned short	drive_step_rate;
	unsigned char	land_zone_cyl[3];
	unsigned char	resv[3];
};

/*
** Bit fields for cache_parms: control_bits
*/
#define	RCD	0x01
#define	MF	0x02
#define	WCE	0x04

#ifdef _WSIO
struct cache_parms {
	struct	page_desc page_desc;
	unsigned char	control_bits;
	unsigned char	rd_wr_priority;
	unsigned short	disable_prefetch_len;
	unsigned short	min_prefetch;
	unsigned short	max_prefetch;
	unsigned short	max_prefetch_ceiling;
};
#else /* _WSIO */
struct caching_page {
	struct page_desc page_desc;
	char   cache_control_bits;
	char   retention_priorities;
	short  disable_prefetch_len;
	short  min_prefetch;
	short  max_prefetch;
	short  max_pref_ceiling;
};
#endif /* _WSIO */

#ifndef _WSIO
struct mode_6_buff_no_bd {
	struct sense_hdr  mode_parms;
	union {
	    struct error_recovery err_recovery_page;
	    struct caching_page   caching_page;
	    struct disc_recon     disc_reconnect_page;
	    struct dad_form_parms format_device_page;
	    struct geometry_parms disk_geometry_page;
	    } u;
};

struct mode_6_buff {
	struct sense_hdr  mode_parms;
	struct block_desc block_desc;
	union {
	    struct error_recovery err_recovery_page;
  	    struct caching_page   caching_page;
	    struct disc_recon     disc_reconnect_page;
            struct dad_form_parms format_device_page;
	    struct geometry_parms disk_geometry_page;
            } u;
};
#endif /* _WSIO */

#ifdef __hp9000s300
#ifdef _UNSUPPORTED

        /*
         * NOTE: The following header file contains information specific
         * to the internals of the HP-UX implementation. The contents of
         * this header file are subject to change without notice. Such
         * changes may affect source code, object code, or binary
         * compatibility between releases of HP-UX. Code which uses
         * the symbols contained within this header file is inherently
         * non-portable (even between HP-UX implementations).
        */
#ifndef _KERNEL_BUILD
#include <.unsupp/sys/_scsi.h>
#endif /* ! _KERNEL_BUILD */
#endif /* _UNSUPPORTED */
#endif /* __hp9000s300 */

#endif /* _SYS_SCSI_INCLUDED */
