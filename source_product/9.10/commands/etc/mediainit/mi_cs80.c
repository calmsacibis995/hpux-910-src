/* @(#) $Revision: 66.1 $ */      
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
#include <sys/cs80.h>


/*
** defines
*/
#define	MAX_SEC	(24 * 60 * 60)	/* one day should be "infinite" enough */


/*
** globals from the parser
*/
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int fd;
extern int unit;
extern int volume;


/*
** other globals
*/
extern int errno;


/*
** CS/80 driver ioctl interface
*/


/*
** execute supplied command (no execution message); return errno
*/
int cmd(c_ticks, c_ptr, c_len)
int c_ticks;
register char *c_ptr;
register int c_len;
{
	struct cmd_parms cmd_parms;
	register char *cp = cmd_parms.cmd_message;

	if (c_len <= 0 || c_len > CMD_LEN)
		return EINVAL;
	cmd_parms.cmd_ticks = c_ticks;
	cmd_parms.exec_ticks = 0;
	cmd_parms.cmd_length = c_len;
	while (c_len--)
		*cp++ = *c_ptr++;
	return ioctl(fd, CIOC_CMD_REPORT, &cmd_parms) < 0 ? errno : 0;
}


/*
** execute supplied command (write execution message); return errno
*/
int cmd_write(c_ticks, c_ptr, c_len, e_ticks, e_ptr, e_len)
int c_ticks;
register char *c_ptr;
register int c_len;
int e_ticks;
char *e_ptr;
int e_len;
{
	struct cmd_parms cmd_parms;
	register char *cp = cmd_parms.cmd_message;

	if (c_len <= 0 || c_len > CMD_LEN)
		return EINVAL;
	cmd_parms.cmd_ticks = c_ticks;
	cmd_parms.exec_ticks = e_ticks;
	cmd_parms.cmd_length = c_len;
	while (c_len--)
		*cp++ = *c_ptr++;
	return ioctl(fd, CIOC_SET_CMD, &cmd_parms) < 0 ||
	       write(fd, e_ptr, e_len) < 0
			? errno : 0;
}


/*
** execute supplied command (read execution message); return errno & bytes read
*/
int cmd_read(c_ticks, c_ptr, c_len, e_ticks, e_ptr, e_len_ptr)
int c_ticks;
register char *c_ptr;
register int c_len;
int e_ticks;
char *e_ptr;
int *e_len_ptr;
{
	struct cmd_parms cmd_parms;
	register char *cp = cmd_parms.cmd_message;

	if (c_len <= 0 || c_len > CMD_LEN)
		return EINVAL;
	cmd_parms.cmd_ticks = c_ticks;
	cmd_parms.exec_ticks = e_ticks;
	cmd_parms.cmd_length = c_len;
	while (c_len--)
		*cp++ = *c_ptr++;
	return ioctl(fd, CIOC_SET_CMD, &cmd_parms) < 0 ||
	       (*e_len_ptr = read(fd, e_ptr, *e_len_ptr)) < 0
			? errno : 0;
}


/*
** CS/80 diagnostic command set structures
*/

typedef struct {	/* three vector address type */
	unsigned	cyln: 24;
	unsigned8	head;
	unsigned16	sect;
} tva_type;

/* data error log types */
#define ERT	0
#define RUNTIME 1

typedef struct {	/* data error log entry */
	unsigned16	phy_cyln;
	unsigned8	phy_head;
	unsigned8	phy_sect;
	unsigned16	log_cyln;
	unsigned8	log_head;
	unsigned8	log_sect;
	unsigned8	error_byte;
	unsigned8	occurrences;
} log_entry_type;

typedef struct {	/* READ_DATA_ERROR_LOGS info */
	unsigned8	my_special_pad;   /* not sent by the device!!! */
	unsigned8	entries;
	char		other_info[5+2+1];
	log_entry_type	entry[256];
} log_info_type;
#define SIZEOF_LIT (sizeof(log_info_type) - 1)

typedef enum {		/* types of error rate tests */
	sector_ert,
	track_ert,
	cylinder_ert,
	surface_ert,
	volume_ert
} ert_area_type;

typedef struct {	/* SET_UNIT/SET_VOLUME command pair */
	unsigned8	setunit;
	unsigned8	setvol;
} setunitvol_type;


/*
** routine for computing and returning a set_unit/set_volume command pair
*/
setunitvol_type suv_CMD_pair()
{
	setunitvol_type value;

	value.setunit = UNIT_BASE | unit;
	value.setvol = VOLUME_BASE | volume;
	return value;
}


/*
** set format option (SS/80 only)
*/
int set_format_option(option)
unsigned8 option;
{
	struct {
		setunitvol_type	setunitvol;
		unsigned8	initutil;
		unsigned8	setformatoption;
		unsigned8	parameter;
	} fo;
#define	SIZEOF_FO 5	/* mustn't count padding bytes at end */

	unsigned8 ob = option;

	fo.setunitvol      = suv_CMD_pair();
	fo.initutil        = CMDinit_util_REM;
	fo.setformatoption = 243;
	fo.parameter       = 95;

	return cmd_write(5 * HZ, &fo, SIZEOF_FO, 5 * HZ, &ob, sizeof ob);
}


/*
** initiate diagnostic with specified unit
*/
int initiate_diagnostic(specific_unit, loops, section)
int specific_unit;
signed16 loops;
unsigned8 section;
{
	struct {
		unsigned8	setunit;
		unsigned8	initdiag;
		signed16	loops;
		unsigned8	section;
	} id;
#define	SIZEOF_ID 5	/* mustn't count padding bytes at end */

	id.setunit  = UNIT_BASE | specific_unit;
	id.initdiag = CMDinit_diagnostic;
	id.loops    = loops;
	id.section  = section;

	return cmd(300 * HZ, &id, SIZEOF_ID);
}


int initialize_media(options, interleave)
unsigned8 options;
unsigned8 interleave;
{
	struct {
		setunitvol_type	setunitvol;
		unsigned8	initmedia;
		unsigned8	options;
		unsigned8	interleave;
	} im;
#define	SIZEOF_IM 5	/* mustn't count padding bytes at end */

	im.setunitvol = suv_CMD_pair();
	im.initmedia  = CMDinit_media;
	im.options    = options;
	im.interleave = interleave;

	return cmd(MAX_SEC * HZ, &im, SIZEOF_IM);
}


int preset_drive()
{
	struct {
		setunitvol_type	setunitvol;
		unsigned8	initutil;
		unsigned8	presetdrive;
	} pd;

	pd.setunitvol  = suv_CMD_pair();
	pd.initutil    = CMDinit_util_NEM;
	pd.presetdrive = 206;

	return cmd(5 * HZ, &pd, sizeof pd);
}


int clear_logs(logcode)
unsigned8 logcode;
{
	struct {
		setunitvol_type	setunitvol;
		unsigned8	initutil;
		unsigned8	clearlogs;
		unsigned8	logcode;
	} cl;
#define	SIZEOF_CL 5	/* mustn't count padding bytes at end */

	cl.setunitvol = suv_CMD_pair();
	cl.initutil   = CMDinit_util_NEM;
	cl.clearlogs  = 205;
	cl.logcode    = logcode;

	return cmd(300 * HZ, &cl, SIZEOF_CL);
}

 
int data_error_log(de_log, head, log_ptr)
int de_log;
int head;
log_info_type *log_ptr;
{
	static unsigned8 read_de_log_micro_op[] = {
		198,	/* ert */
		197	/* runtime */
	};

	struct {
		setunitvol_type setunitvol;
		unsigned8 initutil;
		unsigned8 readlogs;
		unsigned8 head;
	} del;
#define	SIZEOF_DEL 5	/* mustn't count padding bytes at end */

	int bytes_read = SIZEOF_LIT;

	del.setunitvol = suv_CMD_pair();
	del.initutil   = CMDinit_util_SEM;
	del.readlogs   = read_de_log_micro_op[de_log];
	del.head       = head;

	return cmd_read(5 * HZ, &del, SIZEOF_DEL,
			5 * HZ, &log_ptr->entries, &bytes_read) ||
	       bytes_read != 9 + log_ptr->entries * sizeof(log_entry_type)
			    ? EIO : 0;
}


int ert(cyln, head, sect, ert_area, loops, exec_len_ptr, log_entry_ptr)
unsigned long cyln;
unsigned8 head;
signed16 sect;
ert_area_type ert_area;
unsigned8 loops;
int *exec_len_ptr;
log_entry_type *log_entry_ptr;
{
	struct {
		setunitvol_type setunitvol;
		unsigned8 nop;
		unsigned8 setadd;
		tva_type tva;
		unsigned8 initutil;
		unsigned8 patternert;
		unsigned8 loops;
		signed8 offset;
		unsigned8 report;
		unsigned8 testarea;
		unsigned8 datasource;
	} e;
#define	SIZEOF_ERT 17	/* mustn't count padding bytes at end */

	e.setunitvol	= suv_CMD_pair();
	e.nop		= CMDno_op;
	e.setadd	= CMDset_address_3V;
	e.tva.cyln	= cyln;
	e.tva.head	= head;
	e.tva.sect	= sect;
	e.initutil	= *exec_len_ptr ? CMDinit_util_SEM : CMDinit_util_NEM;
	e.patternert	= 200;			/* pattern ert micro opcode */
	e.loops		= loops;		/* as specified */
	e.offset	= 0;			/* no offset */
	e.report	= 0;			/* data error log info only */
	e.testarea	= (int)ert_area;	/* as specified */
	e.datasource	= 0;			/* internal pattern table */

	return *exec_len_ptr ?
			cmd_read(MAX_SEC * HZ, &e, SIZEOF_ERT, 5 * HZ,
						log_entry_ptr, exec_len_ptr) :
			cmd(MAX_SEC * HZ, &e, SIZEOF_ERT);
}


int spare_block(cyln, head, sect, sparingmode)
unsigned long cyln;
unsigned8 head;
signed16 sect;
unsigned8 sparingmode;
{
	struct {
		setunitvol_type setunitvol;
		unsigned8 nop;
		unsigned8 setadd;
		tva_type tva;
		unsigned8 spareblock;
		unsigned8 sparingmode;
	} sb;

	sb.setunitvol	= suv_CMD_pair();
	sb.nop		= CMDno_op;
	sb.setadd	= CMDset_address_3V;
	sb.tva.cyln	= cyln;
	sb.tva.head	= head;
	sb.tva.sect	= sect;
	sb.spareblock	= CMDspare_block;
	sb.sparingmode	= sparingmode;

	return cmd(MAX_SEC * HZ, &sb, sizeof sb);
}


/*
** mediainit a CS/80 or SS/80 device
*/

#define	DEFAULT_INITIALIZE_OPTIONS	0
#define	RECERTIFY_INITIALIZE_OPTION	1	/* force re-certification */
#define	DEFAULT_VOLUME_ERT_PASSES	2
#define	DEFAULT_CLEAR_LOGS_OPTION	1	/* clear ert logs only */
#define	DEFAULT_SPARING_MODE		1	/* do not retain data */

#define	SECTOR_ERT_PASSES	20	/* suspect sector scans */
#define	TRACK_ERT_PASSES	20	/* spared track scans */
#define	MAX_SPARING_TRIES	5	/* maximum attempts at sparing */


/*
** NOTE: due to the depth of control structure nesting, the following procedure
**	 uses a shift width of 4
*/
void mi_cs80()
{
    int flag = 1;	/* used in the CIOC_SET_CMD_MODE ioctl */

    int initialize_options = DEFAULT_INITIALIZE_OPTIONS;
    int volume_ert_passes = DEFAULT_VOLUME_ERT_PASSES;
    int clear_logs_option = DEFAULT_CLEAR_LOGS_OPTION;
    int sparing_mode = DEFAULT_SPARING_MODE;

    struct describe_type describe_bytes;
#define DESCR_CNTRL	describe_bytes.controller_tag.controller
#define DESCR_UNIT	describe_bytes.unit_tag.unit
#define DESCR_VOL	describe_bytes.volume_tag.volume

    char subset80;
    char cs80disc;
    char diagnostic_to_be_run;
    char format_option_supported;
    int max_interleave;

    /* Clear errno */
    errno = 0;
    /*
    ** put the CS/80 driver into command mode
    */
    verb("locking the volume");
    if (ioctl(fd, CIOC_CMD_MODE, &flag) < 0)
	err(errno, "can't lock volume");

    /*
    ** describe
    */
    verb("performing a describe command");
    if (ioctl(fd, CIOC_DESCRIBE, &describe_bytes) < 0) 
	err(errno, "describe command failed");

    subset80 = DESCR_CNTRL.ct & CT_SUBSET_80;
    cs80disc = !subset80 && DESCR_UNIT.dt <= 1;
    
    /*
    ** handle of the re-certify option
    */
    if (recertify)
	if (DESCR_UNIT.dt == 2)
	    initialize_options = RECERTIFY_INITIALIZE_OPTION;
	else
	    err(0, "re-certify option only for cartridge tapes");

    /*
    ** determine if the SS/80 format option is supported
    */
    format_option_supported = 0;	/* until proven otherwise */
    if (subset80) {
	verb("testing for format option support");
	if (set_format_option(255) && errno != EINVAL)
	    err(errno, "format option test failed");
	if (errno)
	    verb("no format option supported");
	else {
	    verb("format option supported");
	    format_option_supported = 1;
	}
    }

    if (!format_option_supported && fmt_optn)
	err(EINVAL, "format option specified, but device supports none");
    if (fmt_optn < 0 || fmt_optn > 239)
	err(EINVAL, "format option must be in the range 0..239");

    /*
    ** validate the supplied interleave factor or come up with a default
    */
    if (interleave < 0)
	err(EINVAL, "interleave must be a positive number");
    max_interleave = format_option_supported ? 255 : DESCR_UNIT.mif;
    if (interleave > max_interleave)
	err(EINVAL, "maximum interleave for this device is %d", max_interleave);
    if (!interleave && DESCR_VOL.currentif) {
	interleave = DESCR_VOL.currentif;
	verb("interleave factor %d chosen", interleave);
    }

    /*
    ** diagnostics are to be run on CS80 discs only
    */
    diagnostic_to_be_run = cs80disc;

    /*
    ** if guru mode, allow modification to certain hidden parameters
    */
    if (guru) {
	if (diagnostic_to_be_run)
	    diagnostic_to_be_run = !yes("suppress running diagnostic?");
	allow_modification("initialize options?", &initialize_options, 255);
	if (cs80disc) {
	    allow_modification("volume ert passes?", &volume_ert_passes, 255);
	    allow_modification("clear logs option?", &clear_logs_option, 255);
	    allow_modification("sparing mode byte?", &sparing_mode, 255);
	}
    }

    /*
    ** if appropriate, run diagnostics
    */
    if (diagnostic_to_be_run) {
	/*
	** first try the individual unit (old CS/80 devices will reject)
	*/
	verb("running diagnostics");
	if (initiate_diagnostic(unit, 1, 0) && errno != EINVAL)
	    err(errno, "diagnostics failed");
	/*
	** if diagnostic to the individual unit rejected, send it to unit 15
	*/
	if (errno)
	    if (initiate_diagnostic(15, 1, 0))
		err(errno, "diagnostics failed");
    }

    /*
    ** if applicable, set the format option
    */
    if (format_option_supported) {
	verb("setting format option %d", fmt_optn);
	if (set_format_option(fmt_optn))
	    err(errno, "specified format option rejected");
    }

    /*
    ** initialize the media
    */
    verb("initializing media");
    if (initialize_media(initialize_options, interleave))
	err(errno, "initialize media command failed");

    /*
    ** for CS80 discs, perform testing and sparing
    */
    if (cs80disc) {

	int exec_len;
	int de_log;
	int head;
	log_info_type log;
	log_entry_type *lep;
	int sparing_tries;

	/*
	** preset the drive to force logging of runtime data errors
	*/
	verb("pre-setting drive");
	if (preset_drive())
	    err(errno, "preset drive command failed");

	/*
	** clear the (ert) logs
	*/
	verb("clearing logs");
	if (clear_logs(clear_logs_option))
	    err(errno, "clear logs command failed");

	/*
	** if appropriate, run a full volume ert
	*/
	if (volume_ert_passes) {
	    verb("running a %d pass volume error rate test", volume_ert_passes);
	    exec_len = 0;  /* no execution message */
	    if (ert(0, 0, 0, volume_ert, volume_ert_passes, &exec_len, NULL))
		err(errno, "volume error rate test failed");
	}

	for (de_log = ERT; de_log <= RUNTIME; ++de_log)
	    for (head = 0; head <= DESCR_VOL.maxhadd; ++head) {

		/*
		** read the appropriate data error log
		*/
		verb("reading %s log for head %d",
				de_log ? "run time" : "error rate test", head);
		if (data_error_log(de_log, head, &log))
		    err(errno, "read data error log command failed");

		for (lep = log.entry; lep < log.entry + log.entries; ++lep) {

		    /*
		    ** address the suspect sector and run a sector ert;
		    ** a message back implies that sparing is required
		    */
		    verb("testing suspect sector %d on head %d, cylinder %d",
				lep->log_sect, lep->log_head, lep->log_cyln);
		    exec_len = sizeof(log_entry_type);
		    if (ert(lep->log_cyln, lep->log_head, lep->log_sect,
				sector_ert, SECTOR_ERT_PASSES, &exec_len, lep))
			err(errno, "sector error rate test failed");

		    for (sparing_tries = 1; exec_len > 1; ++sparing_tries) {

			if (sparing_tries > MAX_SPARING_TRIES)
			    err(0, "sparing attempt limit exceeded");

			/*
			** spare bad block (or track if necessary) 
			*/
			verb("sparing sector %d on head %d, cylinder %d",
				 lep->log_sect, lep->log_head, lep->log_cyln);
			if (spare_block(lep->log_cyln, lep->log_head,
					lep->log_sect, sparing_mode))
			    err(errno, "spare block command failed");

			/*
			** run a track ert;
			** a message back implies sparing required again!!!
			*/
			verb("re-testing entire track on head %d, cylinder %d",
						 lep->log_head, lep->log_cyln);
			exec_len = sizeof(log_entry_type);
			if (ert(lep->log_cyln, lep->log_head, 0,
				track_ert, TRACK_ERT_PASSES, &exec_len, lep))
			    err(errno, "track error rate test failed");

		    }  /* for (sparing_tries) */

		}  /* for (lep) */

	    }  /* for (head) */

	/*
	** if guru mode, allow manual sparing
	*/
	if (guru) {
	    int head = 0;
	    int cylinder = 0;
	    int sector = 0;

	    while (yes("additional manual sparing?")) {
		allow_modification("sparing mode byte?", &sparing_mode, 255);
		allow_modification("head?", &head, 255);
		allow_modification("cylinder?", &cylinder, 65535);
		allow_modification("sector?", &sector, 255);
		printf("head %d, cylinder %d, sector %d, sparing mode %d",
				    head, cylinder, sector, sparing_mode);
		if (yes(" correct?"))
		    if (spare_block(cylinder, head, sector, sparing_mode))
			err(errno, "spare block command failed");
	    }
	}

    }  /* if (cs80disc) */

}

