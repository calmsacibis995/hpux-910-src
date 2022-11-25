/* HPUX_ID: @(#) $Revision: 66.2 $ */

/* mi_sio_cs80.h*/
/*
** defines
*/
#define	MAX_SEC	(24 * 60 * 60)	/* one day should be "infinite" enough */
#define CTDMASK    0x0400000    /* mask for ctd bit in minor number */
#define UNITMASK   0x00000e0    /* mask for unit bits in minor number */
#define PARAM_BOUNDS 0x80	/* mask for parameter bounds status error*/
#define FALSE 0
#define TRUE  1

/* data error log types */
#define ERT	0
#define RUNTIME 1

struct log_struct {	/* data error log entry */
	unsigned16	phy_cyln;
	unsigned8	phy_head;
	unsigned8	phy_sect;
	unsigned16	log_cyln;
	unsigned8	log_head;
	unsigned8	log_sect;
	unsigned8	error_byte;
	unsigned8	occurrences;
	};

typedef union { 		/*union for above log entry structure*/
	unsigned8		log_entry_bytes[10];
	struct log_struct	log_entry;
	} log_entry_type;

#define SIZEOF_LET (sizeof(log_entry_type))

typedef struct {	/* READ_DATA_ERROR_LOGS info */
	unsigned8	my_special_pad;   /* not sent by the device!!! */
	unsigned8	entries;
	char		other_info[5+2+1];
	log_entry_type	entry[256];
	} log_info_type;

#define SIZEOF_LIT (sizeof(log_info_type) - 1)

/*
 * three vector address structure
 */
struct vector_address {
	int	cylinder;
	int	head;
	int 	sector;
	};

/*
 * ert test area pass parameter defines
 */
#define SECTOR		0
#define TRACK		1
#define VOLUME		4

/*
 * buffer lengths defined
 */
#define TEMP_LEN 4096
#define HDWR_LEN 20

/*
 * buffer structures for library routines
 */

struct diag_t {			/*Init_diagnostic buffer structure*/
	char	buffer[64];	/*buff [0-63] = hpib message*/
	short	loop_count;	/*buff[64-65] = loop_count*/
	char	diag_sec;	/*buff[66] = diagnostic section*/
	char	extra[89];	/*Total diag_buff is 256 bytes*/
	};

struct init_m_t {
	char	buffer[64];	/*buff[0-63] = hpib message*/
	char	option;		/*buff[64] = initialize option*/
	char	interleave;	/*buff[65] = block interleave byte*/
	char	extra[90];	/*Total init_media_buff is 256 bytes*/
	};

struct util_t {
	char	buffer[64];	/*buff[0-63] = hpib message*/
	int	x_msg_len;	/*buff[64-67] = execution message length*/
	char 	x_msg_qual;	/*buff[68] = exec. message qualifier*/
	char	util_no;	/*buff[69] = utility number*/
	char	params[8];	/*buff[70-77] = parameter bytes*/
	char	param_no;	/*buff[78] = number of parameter bytes*/
	char	extra[49];	/*buff[79-127] = rest of report phase*/
	char	data[TEMP_LEN-128];	/*buff[128-4095] = exec message*/
	};

struct spare_t {
	char	buffer[64];	/*buff[0-63] = hpib message*/
	char	spare_mode;	/*buff[64] = spare mode*/
	char	extra[91];	/*Total spare_buff is 256 bytes*/
	};

/*
 * defines for utility numbers used for initiate utility command
 */
#define READ_RUN_LOG	197
#define READ_ERT_LOG	198
#define PATTERN_ERT	200
#define CLEAR_LOGS	205
#define PRESET_DRIVE	206
#define SET_FORMAT_OP1	243
#define SET_FORMAT_OP2	 95


/*
 * status messages returned by library routine checker
 */
#define	SUCCESSFUL  	0
#define MI_IO_FAIL  	1
#define MI_BAD_DEV  	2
#define MI_BAD_BUFF 	3
#define MI_TIMEOUT  	4
#define MI_IO_UNIMP 	5
#define MI_POWER_FAIL	6
#define MI_IMS_FAIL	7

/*
** mediainit a CS/80 or SS/80 device
*/

#define	DEFAULT_INITIALIZE_OPTION	0
#define RECERTIFY_INITIALIZE_OPTION	1	/* force re-certification */
#define	NO_RECERTIFY_INITIALIZE_OPTION	4	/* force no re-certification */
#define	DEFAULT_VOLUME_ERT_PASSES	2
#define	DEFAULT_CLEAR_LOGS_OPTION	1	/* clear ert logs only */
#define	DEFAULT_SPARING_MODE		1	/* do not retain data */

#define	SECTOR_ERT_PASSES	20	/* suspect sector scans */
#define	TRACK_ERT_PASSES	20	/* spared track scans */
#define	MAX_SPARING_TRIES	5	/* maximum attempts at sparing */

