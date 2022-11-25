#ifdef _SIO

/* HPUX_ID: @(#) $Revision: 70.2 $  */
/*
** CS/80 mediainit
*/


/*
** include files
*/
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/cs80.h> 
#include <sio/llio.h>
#include "mi_sio_cs80.h"
#include "sio_cs80lib.h"

/* #define DEBUG 1 */

/*
** globals from the parser
*/
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int debug;
extern int blkno;
 

/*
** other globals
*/
extern int errno;
extern int maj;
extern int minr;
extern int fd;
extern int unit;
extern int volume;
extern int iotype;
extern char *name;

/*
 * buffers for use with library routines
 */
union {
	short		icomp[25];  /*complementary buffer*/
	comp_type	comp;
	}u;

char		hdwr_status[HDWR_LEN]; /*buffer for returned hardware status*/
int		hdwr_count;   /*size of hardware status buffer*/
char 		temp_buff[TEMP_LEN]; /*temp buffer for lib routines*/
int		x_msg_len;    /*expected length of execution message*/
struct diag_t	*diag_buff;   /*buffer for diagnostics message*/
struct init_m_t *init_buff;   /*buffer for initialize media message*/
struct util_t	*util_buff;   /*buffer for initiate utility message*/
struct spare_t	*spare_buff;  /*buffer for spare block utility message*/
log_entry_type	single_entry; /*buffer for returned data error entry*/

int sys_call_result;	/* save result of system calls */
int dev_unit;	/*device unit number*/
char qstat;	/*QSTAT returned by device*/
struct trans_report trans_report_struct;	/* so that "sizeof" can
						   be used */
/*
 * Defines necessary for accessing disc0 driver status.
 */
/* Tstat and Qstat in report buffer. */
#define D0_TSTAT  (trans_report_offset + \
		sizeof(trans_report_struct.generic_status) + \
		sizeof(trans_report_struct.report_class))
					/* llio_status offset in buffer */

/* Other error status field offsets in report buffer. */
#define D0_QSTAT	(D0_TSTAT + sizeof(unsigned char))
					/* CS80 qstat offset in buffer */
#define D0_CS80_STATUS  (D0_QSTAT + 1)	/* Start of CS80 status report */
#define D0_CS80_REJECT_2  (D0_QSTAT + 4) /* CS80 reject error field (2nd byte)
					    in buffer */
/*
 * Defines necessary for accessing disc1 driver status.
 */

/* llio_status_type fields in report buffer. */
#define D1_LLIO_STATUS  (trans_report_offset + \
		sizeof(trans_report_struct.generic_status) + \
		sizeof(trans_report_struct.report_class))
					/* llio_status offset in buffer */

/* Other error status field offsets in report buffer. */
#define D1_QSTAT	(D1_LLIO_STATUS + sizeof(llio_status_type))
					/* CS80 qstat offset in buffer */
#define D1_CS80_STATUS  (D1_QSTAT + 1)	/* Start of CS80 status report */
#define D1_CS80_REJECT_2  (D1_QSTAT + 4) /* CS80 reject error field (2nd byte)
/*
 * Defines necessary for accessing disc2 driver status.
 */
/* Tstat and Qstat in report buffer. */
#define D2_TSTAT  (trans_report_offset + \
		sizeof(trans_report_struct.generic_status) + \
		sizeof(trans_report_struct.report_class))
					/* llio_status offset in buffer */

#define D2_STAT_COUNT (D2_TSTAT + sizeof(unsigned char))
					/* Length of status message */
/* Other error status field offsets in report buffer. */
#define D2_QSTAT	(D2_STAT_COUNT + sizeof(unsigned char))
					/* CS80 qstat offset in buffer */
#define D2_CS80_STATUS  (D2_QSTAT)	/* Start of CS80 status report */

void
mi_sio_cs80()
{

	/*
	 * functions used in this routine defined below
	 */
	void do_diagnostic();
	void do_set_format_option();
	void do_initialize();
	void do_preset();
	void do_clear_logs();
	void do_ert();
	void do_read_log();
	void do_spare();
	void set_up_addr();
	void check_lib_stat();
	void print_status();
	void init_variables();
	void copy_hdwr_status();
	int  unimplemented_io();
#ifdef DEBUG
	void print_disc1_status();
	void print_buf();
#endif
	
	/*
	 * setup defaults for media initialization
	 */
	int initialize_option = DEFAULT_INITIALIZE_OPTION;
	int volume_ert_passes = DEFAULT_VOLUME_ERT_PASSES;
	int clear_logs_option = DEFAULT_CLEAR_LOGS_OPTION;
	int sparing_mode = DEFAULT_SPARING_MODE;
	
	/*
	 * data error log type declarations
	 */
	log_info_type	error_log;
	log_info_type	*ptr_error_log;
	int		is_entry;
	int		*ptr_is_entry;
	log_entry_type	*lep;

	/*
 	 * describe type declarations
	 */
	struct describe_type   *ptr_describe;
	struct describe_type describe_bytes;
#define DESCR_CNTRL	describe_bytes.controller_tag.controller
#define DESCR_UNIT	describe_bytes.unit_tag.unit
#define DESCR_VOL	describe_bytes.volume_tag.volume
		
#define CT_SUBSET_80    0x04      /* controller type is subset 80 */
	
	char subset80;
	char cs80disk;
	char diagnostic_to_be_run;
	char format_option_supported;
	int max_interleave;
	 
	short	status;
	short	*ptr_status;
	struct vector_address	v_addr;
	struct vector_address	*ptr_v_addr;
	
	int flag = 1;	  /* used in the CIOC_EXCLUSIVE ioctl */
	int check_fmt_optn = 255; /*used to check if format option supported*/
	int test_area;    /* used as error rate test area parameter*/
	int head, de_log, sparing_tries; /*used as counters for data error tests*/
	char log_util;	  /*used to determine which data error log to read*/
	char x_msg_qual; /*used to determine type of exec message*/

/*
** start cs80 program
*/

	/*
	 * setup pointers to reference passed parameters
	 */
	 ptr_status = &status;
	 ptr_describe = &describe_bytes;
	 ptr_v_addr = &v_addr;
	 ptr_error_log = &error_log;
	 ptr_is_entry = &is_entry;

	/*
	 * setting exclusive lock on device
	 */
	verb("setting exclusive mode lock on device");
	if (ioctl(fd, CIOC_EXCLUSIVE, &flag) < 0)
		err(errno, "can't get exclusive mode lock on device");
	/*
	 * doing describe
	 */
	verb("performing a describe command");
	if (ioctl(fd, CIOC_DESCRIBE, ptr_describe) < 0)
		err(errno, "can't describe device");
	/*
 	 * determine if device is subset80 or cs80disk
	 */

	subset80 = DESCR_CNTRL.ct & CT_SUBSET_80;
	cs80disk = !subset80 && DESCR_UNIT.dt <= 1;

	/*
	 * handle recertify option
	 */
	if (recertify) /*recertification desired*/
	{
		if (DESCR_UNIT.dt == 2 && minr & CTDMASK) /*device is a CTD*/
			initialize_option = RECERTIFY_INITIALIZE_OPTION;
		else /*device is not a CTD*/
			err(0,"re-certify is for cartridge tapes only");
	}
		
	/*
	 * check if fmt_optn is in proper range of values
	 */
	if (fmt_optn < 0 || fmt_optn > 239)
		err(EINVAL, "format option must be in the range 0..239");

	/*
	 * determine if the SS/80 format option is supported
	 */
	format_option_supported = FALSE;/*set format option supported to false*/
	if (subset80)  
	{
		do_set_format_option(ptr_status, check_fmt_optn);
		if (status != SUCCESSFUL && status != MI_IO_UNIMP)
		{
			print_status(status);
			err(errno, "set format option command failed");
		}
		if (status == SUCCESSFUL)
		{
			format_option_supported = TRUE;
			verb("format option supported");
		}
		else   /*status = MI_IO_UNIMP*/
		{
			verb("format option not supported");
		}
	}
	if (!format_option_supported && fmt_optn)
	{
		err(EINVAL,"format option specified, but device supports none");
	}

	/* 
	 * validate the supplied interleave factor
	 * if none supplied use current interleave from ioctl DESCRIBE
	 * CTD's have a default interleave of zero and interleave is
	 * initialized to be zero
	 */
	if (interleave < 0)
		err(EINVAL, "interleave must be a positive number");
	max_interleave = format_option_supported ? 255 : DESCR_UNIT.mif;
	if (interleave > max_interleave)
		err(EINVAL, "maximum interleave for this device is %d", max_interleave);
	if (!interleave && DESCR_VOL.currentif) 
	{
		interleave = DESCR_VOL.currentif;
		verb("interleave factor %d chosen",interleave);
	}

	/* 
	 * diagnostics are to be run on CS80 disk only
	 */
	diagnostic_to_be_run = cs80disk;
	/*
	 * if guru mode, allow modification to certain hidden parameters
	 */
	if (guru)
	{
		if (diagnostic_to_be_run)
			diagnostic_to_be_run = !yes("suppress running diagnostic?");
		allow_modification("initialize option?", &initialize_option, 255);
		if (cs80disk)
		{
			allow_modification("volume ert passes?", &volume_ert_passes, 255);
			allow_modification("clear logs option?", &clear_logs_option, 255);
			allow_modification("sparing mode?", &sparing_mode, 255);
		}
	}

	/*
	 * if flag is set, run self test diagnostic on device
	 */
	if (diagnostic_to_be_run)
	{
		verb("running diagnostics");
		do_diagnostic(ptr_status);
		if (status != SUCCESSFUL && status != MI_POWER_FAIL) 
		{
			print_status(status);
			err(errno, "diagnostics command failed");
		}
		if (status == MI_POWER_FAIL)
		{
			verb("retrying diagnostics due to power fail status");
			do_diagnostic(ptr_status);
			if (status != SUCCESSFUL) 
			{
				print_status(status);
				err(errno, "diagnostics command failed");
			}
			else
			{
				verb("status of diagnostics command = SUCCESSFUL");
			}
		}
		else  /*status == SUCCESSFUL*/
		{
			verb("status of diagnostics command = SUCCESSFUL");
		}
	}
	  
	/*
	 * if applicable, set the format option
	 */
	if (format_option_supported)
	{
		verb("setting format option %d", fmt_optn);
		do_set_format_option(ptr_status, fmt_optn);
		if (status != SUCCESSFUL && status != MI_IO_UNIMP)
		{
			print_status(status);
			err(errno, "set format option command failed");
		}
		if (status == SUCCESSFUL)
		{
			verb("status of set format option command = SUCCESSFUL");
		}
		else   /*status = MI_IO_UNIMP*/
		{
			verb("format option %d rejected", fmt_optn);
		}
	}

	/*
	 * initialize the media
	 */
	verb("initializing media");
	do_initialize(ptr_status, initialize_option);
	if (status != SUCCESSFUL)
	{
		print_status(status);
		err(errno, "initialize media command failed");
	}
	else
	{
		verb("status of initialize media command = SUCCESSFUL");
	}
	if (cs80disk)
	{
		/*
		 * preset the drive to force logging of runtime data errors
		 */
		verb("pre-setting drive");
		do_preset(ptr_status);
		if (status != SUCCESSFUL)
		{
			print_status(status);
			err(errno, "preset drive command failed");
		}
		else
		{
			verb("status of preset drive command = SUCCESSFUL");
		}
		/*
		 * clear the (ert) logs
		 */
		verb("clearing logs");
		do_clear_logs(ptr_status,clear_logs_option);
		if (status != SUCCESSFUL)
		{
			print_status(status);
			err(errno, "clear logs command failed");
		}
		else
		{
			verb("status of clear logs command = SUCCESSFUL");
		}
		/*
		 * run full volume pattern error rate test
		 */
		verb("running a %d pass volume error rate test",volume_ert_passes);
		/*
		 * set up passed parameters for start of volume ert 
		 */
		v_addr.cylinder = 0;
		v_addr.head = 0;
		v_addr.sector = 0;
		test_area = VOLUME;
		x_msg_len = 0;
		x_msg_qual = 0;
		do_ert(ptr_status, volume_ert_passes, test_area, ptr_v_addr, x_msg_qual, ptr_is_entry);
		if (status != SUCCESSFUL)
		{
			print_status(status);
			err(errno, "volume ert command failed");
		}
		else
		{
			verb("status of volume ert command = SUCCESSFUL");
		}
		/*
		 * read ert and runtime logs for each head
		 */
		 for (de_log = ERT; de_log <= RUNTIME; de_log ++)
		 {
			for (head = 0; head <= DESCR_VOL.maxhadd; ++head)
			{
				verb("reading %s log for head %d",
				de_log ? "run time":"error rate test",head); 
				/*
				 * read appropriate data error log for each head
				 */
				log_util = (de_log ? READ_RUN_LOG:READ_ERT_LOG);
				do_read_log (ptr_status, ptr_error_log, head, log_util);
				if (status != SUCCESSFUL)
				{
					print_status(status);
					err(errno, "read log command failed");
				}
				else
				{
					verb("status of read log command = SUCCESSFUL");
				}

				for (lep=error_log.entry; lep<error_log.entry + error_log.entries; lep++)
				{
					/*
					 * address suspect sector and run a
					 * sector ert. An execution message returned implies
					 * that sparing is required
					 * flag is_entry set to true if execution message
					 * returned and execution message stored
					 * in single_entry
					 */
					verb("testing suspect sector %d on head %d, cylinder %d",
					       lep->log_entry.log_sect, lep->log_entry.log_head, 
					       lep->log_entry.log_cyln);
					v_addr.cylinder = lep->log_entry.log_cyln;
					v_addr.head = lep->log_entry.log_head;
					v_addr.sector = lep->log_entry.log_sect;
					test_area = SECTOR;
					x_msg_len = SIZEOF_LET;
					x_msg_qual = 2;
					do_ert(ptr_status, SECTOR_ERT_PASSES, test_area, ptr_v_addr, x_msg_qual, ptr_is_entry);
					if (status != SUCCESSFUL)
					{
						print_status(status);
						err(errno, "sector ert command failed");
					}
					else
					{
						verb("status of sector ert command = SUCCESSFUL");
					}
					/*
					 * if there was a message back then try to
					 * spare the bad sector 
					 */
					for (sparing_tries = 1; is_entry; sparing_tries++)
					{
						if (sparing_tries > MAX_SPARING_TRIES)
							err(0, "sparing attempt limit exceeded");
						/*
						 * spare bad block from execution message
						 * returned from sector ert or track ert
						 */
						verb("sparing sector %d on head %d, cylinder %d",
							 single_entry.log_entry.log_sect,
							 single_entry.log_entry.log_head,
							 single_entry.log_entry.log_cyln);
						v_addr.cylinder = single_entry.log_entry.log_cyln;
						v_addr.head = single_entry.log_entry.log_head;
						v_addr.sector = single_entry.log_entry.log_sect;
						do_spare(ptr_status, sparing_mode, ptr_v_addr);
						if (status != SUCCESSFUL)
						{
							print_status(status);
							err(errno, "spare command failed");
						}
						else
						{
							verb("status of spare command = SUCCESSFUL");
						}
						/*
						 * run a full track ert;
						 * a message back implies sparing required again!!!
						 * flag is_entry is true if execution message returned
						 * and execution message stored in single_entry
						 */
						verb("re-testing entire track on head %d, cylinder %d",
							  lep->log_entry.log_head,lep->log_entry.log_cyln);
						v_addr.cylinder = lep->log_entry.log_cyln;
						v_addr.head = lep->log_entry.log_head;
						v_addr.sector = 0;
						test_area = TRACK;
						x_msg_len = SIZEOF_LET;
						x_msg_qual = 2;
						do_ert(ptr_status, TRACK_ERT_PASSES, test_area, ptr_v_addr, x_msg_qual, ptr_is_entry);
						if (status != SUCCESSFUL)
						{
							print_status(status);
							err(errno, "track ert command failed");
						}
						else
						{
							verb("status of track ert command = SUCCESSFUL");
						}
					} /*for sparing_tries*/
				} /*for lep*/
			} /*for head*/
		} /*for de_log*/
	} /*if cs80disk*/
}







		
/*
 * do_diagnostic - runs the self test diagnostic for the device
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
do_diagnostic(ptr_status) 

 short	*ptr_status;
 {
	int timeout;
	
	timeout = 60;		/*number of seconds to wait for timeout*/
	x_msg_len = 0;		/*no execution message expected*/ 
	diag_buff = (struct diag_t *)temp_buff; /*set diag_buff pointer*/
	diag_buff->loop_count = 1;  	/*set loop count to 1*/
	diag_buff->diag_sec = 0;	/*set section number to 0 self test*/
	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = 15;	/*unit to accept diagnostic must be controller*/
	/*
	 * call xdiag library routine to perform ioctl init_diagnostic
	 */
	sys_call_result = xdiag (fd, &u.comp, diag_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check status of library routine
	 */
	check_lib_stat(sys_call_result, ptr_status);
}

/*
 * do_set_format_option - perform the set format options SS/80 utility
 *
 *	Parameters:
 *		ptr_status - status of the SS/80 transaction
 *
 *		fmt_optn - specified format option for device
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
do_set_format_option(ptr_status, format_option) 

 short	*ptr_status;
 int	format_option;

 {
	int timeout;
	int i;
	
	x_msg_len = 1;	        /*execution message is sent to device*/
	timeout = 10;		/*number of seconds to wait for timeout*/
	util_buff = (struct util_t *)temp_buff; /* set util_buff pointer*/
	util_buff->x_msg_len = x_msg_len;  
	util_buff->x_msg_qual = 1;   /*execution message is sent to device*/
	util_buff->util_no = SET_FORMAT_OP1;    /*do set format option utility*/
	util_buff->param_no = 1;     /*one passed parameters expected*/
	util_buff->params[0] = SET_FORMAT_OP2; /*set second part of opcode*/
	for (i = 1; i < 8; i++)
	{
		util_buff->params[i] = 0;    /*fill params array with 0*/
	}
	/*
	 * set up execution message to send to device
	 */ 
	util_buff->data[0] = format_option; /*pass format option specified*/

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * call xutil library routine to perform ioctl initiate utility
	 */
	sys_call_result = xutil(fd, &u.comp, util_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * Check if the utility is implemented on this drive.
	 * If unimplemented, set status and save hardware
	 * status in buffer.
	 * Otherwise do standard error checking.
	 */
	if (unimplemented_io(sys_call_result, ptr_status))
		copy_hdwr_status();
	else
	{
		/*
		 * check status of library routine
		 */
		check_lib_stat(sys_call_result, ptr_status);
	}
}

/*
 * do_initialize - runs the self test diagnostic for the device
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *		initialize_option - type of initialization parameter
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void 
do_initialize(ptr_status, initialize_option) 

 short	*ptr_status;
 int	initialize_option;
 {
	int timeout;
	
	timeout = 7200;		/*number of seconds to wait for timeout*/
	x_msg_len = 0;		/*no execution message expected*/ 
	init_buff = (struct init_m_t *)temp_buff; /*set init_buff pointer*/
	init_buff->option = (char) initialize_option;/*initialize option set*/
	init_buff->interleave = (char) interleave; /*block interleave factor*/

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * call xinmd library routine to perform ioctl initialize_media
	 */
	sys_call_result = xinmd (fd, &u.comp, init_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check status of library routine
	 */
	check_lib_stat(sys_call_result, ptr_status);
}


/*
 * do_preset - performs preset drive utilty to force drive error logging
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
do_preset(ptr_status) 

 short	*ptr_status;

 {
	int timeout;
	int i;
	
	x_msg_len = 0;	        /*no execution message expected*/
	timeout = 10;		/*number of seconds to wait for timeout*/
	util_buff = (struct util_t *)temp_buff; /* set util_buff pointer*/
	util_buff->x_msg_len = x_msg_len;  
	util_buff->x_msg_qual = 0;   /*no execution message expected*/
	util_buff->util_no = PRESET_DRIVE;    /*do preset drive utility*/
	util_buff->param_no = 0;     /*no passed parameters expected*/
	for (i = 0; i < 8; i++)
	{
		util_buff->params[i] = 0;    /*fill params array with 0*/
	}

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * call xutil library routine to perform ioctl initiate utility
	 */
	sys_call_result = xutil(fd, &u.comp, util_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check if the utility is implemented on this drive by
	 * checking for the parameter bounds error
	 * if parameter bounds error set status and save hardware
	 * status in buffer
	 * otherwise do standard error checking
	 */
	if (!(strncmp(name, "disc0", 5)) || !(strncmp(name, "disc1",5))) {
		if (unimplemented_io(sys_call_result, ptr_status)) 
			copy_hdwr_status();
		else {
			/*
		 	* check status of library routine
		 	*/
			check_lib_stat(sys_call_result, ptr_status);
		}
	}
	else /* alink */ {
		check_lib_stat(sys_call_result, ptr_status);
	}
}


/*
 * do_clear_logs - performs clear logs utilty to clear ert logs 
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *		clear_logs_option - determines if all or just ert logs
 *				    are to be cleared
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void 
do_clear_logs(ptr_status, clear_logs_option) 

short	*ptr_status;
int	clear_logs_option;

{
	int timeout;
	int i;

	x_msg_len = 0;	    /*no execution message expected*/
	timeout = 10;		/*number of seconds to wait for timeout*/
	util_buff = (struct util_t *)temp_buff; /* set util_buff pointer*/
	util_buff->x_msg_len = x_msg_len;
	util_buff->x_msg_qual = 0;   /*no execution message expected*/
	util_buff->util_no = CLEAR_LOGS;    /*do clear logs utility*/
	util_buff->param_no = 1;     /*one passed parameter expected*/
	util_buff->params[0] = (char) clear_logs_option; /*set option*/
	for (i = 1; i < 8; i++)
	{
		util_buff->params[i] = 0;    /*fill rest params array with 0*/
	}

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * call xutil library routine to perform ioctl initiate utility
	 */
	sys_call_result = xutil(fd, &u.comp, util_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check if the utility is implemented on this drive by
	 * checking for the parameter bounds error
	 * if parameter bounds error set status and save hardware
	 * status in buffer
	 * otherwise do standard error checking
	 */
	if (!(strncmp(name, "disc0", 5)) || !(strncmp(name, "disc1",5)))
	{
		if (unimplemented_io(sys_call_result, ptr_status))
			copy_hdwr_status();
		else
		{
			/*
		 	* check status of library routine
		 	*/
			check_lib_stat(sys_call_result, ptr_status);
		}
	}
	else /* alink */
	{
		check_lib_stat(sys_call_result, ptr_status);
	}
}


/*
 * do_ert - performs error rate test for selected data area
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *		loop - error rate test loop parameter
 *
 *		test_area - coverage area for error rate test 
 *
 *		ptr_v_addr - pointer to three vector address to start ert
 *		
 *		x_msg_qual - type of exec msg (0=none 1=to device 2=from device)
 *		
 *		ptr_is_entry - if exec msg returned is true
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
do_ert(ptr_status, loop, test_area, ptr_v_addr, x_msg_qual, ptr_is_entry)

short	*ptr_status;
int	loop;
int	test_area;
struct vector_address	*ptr_v_addr;
char	x_msg_qual;
int	*ptr_is_entry;

{
	int timeout;
	int i;
	char *ptr;
	
	*ptr_is_entry = FALSE;  /*set flag to false til proven otherwise*/
	timeout = 60 + (int) loop * 3600;/*number of seconds for timeout*/
	util_buff = (struct util_t *)temp_buff; /* set util_buff pointer*/
	util_buff->x_msg_len = x_msg_len; /*length execution message*/
	util_buff->x_msg_qual = x_msg_qual;  /*type execution message expected*/
	util_buff->util_no = PATTERN_ERT;   /*do pattern ert utility*/
	util_buff->param_no = 5;     /*five passed parameters expected*/
	util_buff->params[0] = (char) loop; /*set loop parameter*/
	util_buff->params[1] = 0;    /*set offset parameter*/
	util_buff->params[2] = 0;    /*set report mode parameter*/
	util_buff->params[3] = (char) test_area; /*set test area parameter*/
	util_buff->params[4] = 0;    /*set data source parameter*/
	for (i = 5; i < 8; i++)
	{
		util_buff->params[i] = 0;    /*fill rest params array with 0*/
	}

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * set up three vector address in complementary buffer
	 */
	set_up_addr(ptr_v_addr, &u.comp);
	/*
	 * call xutil library routine to perform ioctl initiate utility
	 */
	sys_call_result = xutil(fd, &u.comp, util_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check if the utility is implemented on this drive by
	 * checking for the parameter bounds error
	 * if parameter bounds error set status and save hardware
	 * status in buffer
	 * otherwise do standard error checking
	 */
	if (!(strncmp(name, "disc0", 5)) || !(strncmp(name, "disc1",5)))
	{
		if (unimplemented_io(sys_call_result, ptr_status))
			copy_hdwr_status();
		else
		{
			/*
		 	* check status of library routine
		 	*/
			check_lib_stat(sys_call_result, ptr_status);
		}
	}
	else /* alink */
	{
		check_lib_stat(sys_call_result, ptr_status);
	}
	/*
	 * if status SUCCESSFUL and data error entry is returned
	 * set flag to TRUE and store exec message in
	 * log entry structure
	 */
	if (*ptr_status == SUCCESSFUL && (sys_call_result - 128) > 1)
	{
		*ptr_is_entry = TRUE;
		ptr = temp_buff + 128;
		for (i = 0; i < 10; i++)
		{
			single_entry.log_entry_bytes[i] = (unsigned8) *(ptr++);
		}
	}	
}


/*
 * do_read_log - performs read logs utilty to read data error logs 
 *
 *	Parameters:
 *		ptr_status - status of the cs80 transaction
 *
 *		ptr_error_log - pointer to error log structure
 *
 *		head - head to read data error log for
 *
 *		log_util - which data error log to read
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
do_read_log (ptr_status, ptr_error_log, head, log_util) 

short	*ptr_status;
log_info_type	*ptr_error_log;
int	head;
char	log_util;

{
	int timeout;
	int i,j;
	char *ptr;

	x_msg_len = SIZEOF_LIT;	    /*execution message expected*/
	timeout = 60;		/*number of seconds to wait for timeout*/
	util_buff = (struct util_t *)temp_buff; /* set util_buff pointer*/
	util_buff->x_msg_len = x_msg_len;
	util_buff->x_msg_qual = 2;   /*execution message expected from device*/
	util_buff->util_no = log_util;    /*do logs utility*/
	util_buff->param_no = 1;     /*one passed parameter expected*/
	util_buff->params[0] = (char) head; /*set head to read error log for*/
	for (i = 1; i < 8; i++)
	{
		util_buff->params[i] = 0;    /*fill rest params array with 0*/
	}

	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for initialize_media*/
	/*
	 * call xutil library routine to perform ioctl initiate utility
	 */
	sys_call_result = xutil(fd, &u.comp, util_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check if the utility is implemented on this drive by
	 * checking for the parameter bounds error
	 * if parameter bounds error set status and save hardware
	 * status in buffer
	 * otherwise do standard error checking
	 * and continue
	 */
	if (!(strncmp(name, "disc0", 5)) || !(strncmp(name, "disc1",5)))
	{
		if (unimplemented_io(sys_call_result, ptr_status))
			copy_hdwr_status();
		else
		{
			/*
		 	* check status of library routine
		 	*/
			check_lib_stat(sys_call_result, ptr_status);
		}
	}
	else /* alink */
	{
		check_lib_stat(sys_call_result, ptr_status);
	}
	/*
	 * if status is SUCCESSFUL then copy the error log
	 * data returned in the exec msg into the
	 * error log data structure
	 */
	if (*ptr_status == SUCCESSFUL)
	{
		ptr = temp_buff + 128;
		ptr_error_log->entries = (unsigned8) *(ptr++);
		for (i = 0; i < 8; i++)
		{
			ptr_error_log->other_info[i]=(unsigned8) *(ptr++);
		}
		for (i = 0; i < ptr_error_log->entries; i++)
		{
			for (j = 0; j < 10; j++)
			{
				ptr_error_log->entry[i].log_entry_bytes[j] = (unsigned8) *(ptr++);
			}
		}
	} /*if status SUCCESSFUL*/
}

/*
 * do_spare - spares the block given by the target address
 *
 * 	Parameters :
 *		ptr_status - status of the cs80 transaction
 *
 *		spare_mode - type of sparing to be done
 *
 *		ptr_v_addr - pointer to the three vector address desired
 *
 *	Returns:
 *		none:
 *
 *	Side Effects:
 *		none:
 * 
 */
void 
do_spare(ptr_status, spare_mode, ptr_v_addr)

 short	*ptr_status;
 int	spare_mode;
 struct vector_address	*ptr_v_addr;

 {
	int timeout;

	x_msg_len = 0;	    /*no execution message expected*/
	timeout = 60;		/*number of seconds to wait for timeout*/
	spare_buff = (struct spare_t *)temp_buff; /* set spare_buff pointer*/
	spare_buff->spare_mode = (char) spare_mode; /*set spare mode*/
	/*
	 * initialize variables
	 */
	init_variables(ptr_status);
	/*
	 * set appropriate unit in complementary buffer
	 */
	u.comp.unit = (short) dev_unit;	/*set unit for spare block*/
	/*
	 * set up three vector address is complementary buffer 
	 */
	set_up_addr(ptr_v_addr, &u.comp);
	/*
	 * call xspre library routine to perform spare block command
	 */
	sys_call_result = xspre(fd, &u.comp, spare_buff, x_msg_len, timeout);
#ifdef DEBUG
	print_buf();
#endif
	/*
	 * check status of library routine
	 */
	check_lib_stat(sys_call_result, ptr_status);
}


/*
 * set_up_addr - sets up the complementary buffer for the three vector
 *		 address sent down to it
 *
 *	Parameters:
 *		ptr_v_addr - pointer to the three vector address desired
 *
 *		comp - pointer to the complementary buffer
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */
void
set_up_addr(ptr_v_addr, comp) 

struct vector_address	*ptr_v_addr;
comp_type	*comp;
{

	int i;
	union {
		int	word;
		char	bytes[4];
		} long_u;

	comp->address_mode = 1;		/*set up three vector address mode*/
	/*
	 * comp->address[0 - 2]		cylinder address 
	 * comp->address[3]		head address
	 * comp->address[4 - 5]		sector address
	 */
	long_u.word = ptr_v_addr->cylinder;
	/*
	 * transfer cylinder address to bytes 0 - 2
	 */
	for (i = 0; i < 3; i++)
	{
		comp->address[i] = long_u.bytes[i + 1];
	}
	long_u.word = ptr_v_addr->head;
	/*
	 * transfer head address to byte 3
	 */
	comp->address[3] = long_u.bytes[3];
	long_u.word = ptr_v_addr->sector;
	/*
	 * transfer sector address to bytes 4 - 5
	 */
	comp->address[4] = long_u.bytes[2];
	comp->address[5] = long_u.bytes[3];
}


/*
 * check_lib_stat - checks the return value from the library call and
 *		    sets the status parameter accordingly.
 *
 * 	Parameters:
 *		result - return value of the library call
 *		
 *		ptr_status - status of the cs80 transaction
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		none
 *
 */

void
check_lib_stat (result, ptr_status)

int	 	 result;
short		 *ptr_status;
{

	int		j;
	struct trans_report *trptr;
	extern int	errno;

	/*
	 * First, check for errors where driver is not called at
	 * all, or there is no driver status returned.  These are
	 * indicated by a system call return of -1.
	 */
	if (result == -1) {
		/*    
		 * The library routine was not successful - check errno and
		 * determine what happened and set the status accordingly.
		 */
		switch (errno) {
		case EBADF:		/* Bad file descriptor */
			*ptr_status = MI_BAD_DEV;
			return;

		case ETIMEDOUT:		/* Timeout */
			*ptr_status = MI_TIMEOUT;
			return;

		case EFAULT:		/* Memory protection problem */
			*ptr_status = MI_BAD_BUFF;
			return;

		}
	}
	/*
	 * Next, copy hardware status and qstat, whether we need to or
	 * not (easier)...
	 */
	qstat = 0;
	trptr = (struct trans_report *) (&temp_buff[0] + trans_report_offset);
	switch (trptr->report_class) {
	case NO_STAT:					/* panic */
		printf("Mediainit: Internal problem #1, please report\n");
		printf("Mediainit: Generic report class NO_STAT, should not happen\n");
		exit(1);

	case CS80_CIO:					/* disc0 */
		qstat = temp_buff[D0_QSTAT];
		copy_hdwr_status();
		break;

	case CS80_NIO:					/* disc1 */
		qstat = temp_buff[D1_QSTAT];
		copy_hdwr_status();
		break;

	case CS80_ALINK_1:				/* disc2 */
	case CS80_ALINK_2:
	case CS80_ALINK_3:
	case CS80_ALINK_4:
		if (temp_buff[D2_TSTAT] == 0) {
			qstat = temp_buff[D2_QSTAT];
			copy_hdwr_status(D2_QSTAT + 1);	/* don't copy qstat */
		} else
			copy_hdwr_status(D2_CS80_STATUS);
		break;					/* no qstat to avoid */
	}
	/*
	 * Next, check generic status for generic errors.
	 */
	switch (trptr->generic_status) {
	case TRANS_NORM:			/* everything ok */
		*ptr_status = SUCCESSFUL;
		break;

	case TRANS_TIMEOUT:			/* transaction timeout */
		*ptr_status = MI_TIMEOUT;
		break;

	case TRANS_POWER_ON:			/* power failure */
		*ptr_status = MI_POWER_FAIL;
		break;

	case TRANS_FAIL:			/* generic failure */
		*ptr_status = MI_IO_FAIL;
		break;

	}
}






/*
 * print_status - prints out the status and the hdwr status returned
 *                from the library routine call
 *
 *	 Parameters:
 *		status - status returned from the library routine call
 *
 *	Returns:
 *		none.
 *
 *	Side Effects
 *		none
 *
 */

void
print_status (status)

short 	status;

{

	int i;

	switch (status)
	{
	case SUCCESSFUL:
		printf ("status = SUCCESSFUL\n");
		break;
	case MI_IO_FAIL:
		printf ("status = MI_IO_FAIL\n");
		break;
	case MI_BAD_DEV:
		printf ("status = MI_BAD_DEV\n");
		break;
	case MI_BAD_BUFF:
		printf ("status = MI_BAD_BUFF\n");
		break;
	case MI_TIMEOUT:
		printf ("status = MI_TIMEOUT\n");
		break;
	case MI_IO_UNIMP:
		printf ("status = MI_IO_UNIMP\n");
		break;
	case MI_POWER_FAIL:
		printf ("status = MI_POWER_FAIL\n");
		break;
	case MI_IMS_FAIL:
		printf ("status = MI_IMS_FAIL\n");
		break;

	default:
		printf ("status = unknown number %d\n",status);
		break;
	}
	if(status != MI_BAD_BUFF && status != MI_BAD_DEV && status != MI_TIMEOUT)
	{
		printf("hardware status bytes output in hex\n");
		for (i = 0; i < HDWR_LEN; i++)
		{
			printf("hardware status byte %d = %x\n",i,(unsigned char) hdwr_status[i]);
		}
		printf("QSTAT = %d\n",qstat);
	}
}

/*
 * init_variables - initializes the complementary buffer used in the
 *	            library routines to be all -1            
 *		    it also initializes hdwr_count, status,
 *		    trans_count and hdwr_status
 *
 * 	Parameters:
 *		ptr_status - pointer to the status returned 
 *	
 *	Returns:
 *		none.
 *
 *	Side Effects:
 *		none.
 */

void
init_variables (ptr_status)

short		*ptr_status;

{
	int 	i;

	/*
	 * Initialize complementary buffer to be -l
	 */

	for (i=0; i <= 24; i++)
	{
		*(u.icomp + i) = -1;
	}
	/*
	 * Initialize other variables
	 */

	dev_unit = (minr & UNITMASK) >> 5;
	hdwr_count = HDWR_LEN;
	*ptr_status = SUCCESSFUL;
	for (i = 0; i < HDWR_LEN; i++)
	{
		hdwr_status[i] = 0;
	}
}


/*
 * copy_hdwr_status - copies hardware status from the temporary
 *		      buffer to the hdwr_status buffer.  This is
 *		      dependent on the driver name.
 *
 * 	Parameters:
 *		Index of first byte of status to copy for disc2 only.
 *
 *	Returns:
 *		none.
 *
 *	Side Effects:
 *		The global variable hdwr_status is stored into.
 *		Uses the global variable "name" (driver name).
 */

void
copy_hdwr_status(first_cs80_report_byte)
	int first_cs80_report_byte;
{

	int i;
	if (!(strncmp(name, "disc0", 5)))	/* disc0 driver status */ 
		first_cs80_report_byte = D0_CS80_STATUS;
	else if (!(strncmp(name, "disc1", 5)))  /* disc1 driver status */ 
		first_cs80_report_byte = D1_CS80_STATUS;

	for (i = 0; i < HDWR_LEN; i++)
	{
		hdwr_status[i] = temp_buff[first_cs80_report_byte + i];
	}
}



	


/*
 * unimplemented_io - Returns TRUE if the CS80 status report indicates
 *		      that the io request is not implemented.  This
 *		      operation is dependent on the driver name.
 *		      Also sets it's parameter to the unimplemented
 *		      io operation code.
 *
 *		      Should only be called for disc0 or disc1.
 *
 * 	Parameters:
 *		ptr_status - pointer to returned status.
 *		result - the value returned by the library call
 *			 immediately preceeding this call.  This
 *			 is actually the value returned by the
 *			 system call that was used by the cs80
 *			 library call.
 *
 *	Returns:
 *		TRUE if the io request that preceeded this call was
 *		unimplemented.
 *
 *	Side Effects:
 *		Uses the global variable "name" and "temp_buf".
 *		Sets the global *ptr_status for caller's use.
 */

int
unimplemented_io(result, ptr_status)

	int		result;
	short 		*ptr_status;

{

	int 	qstat_byte;
	int	parm_bounds_byte;

	if (!(strncmp(name, "disc0", 5))) {	/* disc0 driver */ 
		qstat_byte = D0_QSTAT;
		parm_bounds_byte = D0_CS80_REJECT_2;
	} else  if (!(strncmp(name, "disc1", 5))) {	/* disc1 driver */ 
		qstat_byte = D1_QSTAT;
		parm_bounds_byte = D1_CS80_REJECT_2;
	} else {				/* disc2 driver */
		printf("Mediainit: Internal problem #2, please report\n");
		printf("Mediainit: Call to 'unimplemented_io()' for disc2, should not happen\n");
		exit(1);
	}

	if ((result >= 0) && (temp_buff[qstat_byte]) &&
			(temp_buff[parm_bounds_byte] & PARAM_BOUNDS)) {
				/* indicates unimplemented io request */
		*ptr_status = MI_IO_UNIMP;
		return(TRUE);
	}
	else {
		*ptr_status = SUCCESSFUL;
		return(FALSE);
	}
}



#ifdef DEBUG
/*
 * print_disc1_status - prints out the generic and some device
 *			specific status from disc1 in formatted
 *			form.
 *
 *	Parameters-
 *		buf: buffer containing status.
 *
 *	Returns-
 *		none.
 *
 *	Side Effects -
 *		none.
 */

void
print_disc1_status(buf)

	char *buf;

{
	int qstat;
	int i;

	qstat = (int) buf[D1_QSTAT];

	printf("mediainit: Generic status %d, Report class %d, llio status 0x%x, qstat 0x%x\n",
		*(int *) &buf[GENERIC_STATUS],
		*(int *) &buf[REPORT_CLASS],
		*(int *) &buf[D1_LLIO_STATUS],
		qstat);

}



/*
 * print_buf - Prints the resulting buffer in unformatted fashion,
 *	       after a library call.
 *
 *	Parameters:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Returns:
 *		None.
 */

void
print_buf()

{
	int i;
©	printf("mediainit: raw buffer: ");

	for (i = 0; i < 32; i++)
		printf("%x ", temp_buff[64 + i]);

	printf("\n");

}
#endif
#else
int sys_call_result;
void
mi_sio_cs80()
{
	return;
}
#endif /* _SIO */
