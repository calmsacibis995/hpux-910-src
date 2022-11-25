/* HPUX_ID: @(#) $Revision: 70.3 $ */
/*
** mediainit - initialize hard disc, flexible disc, or cartridge tape media
**
**   USAGE: mediainit [ options ] pathname
**     options: -v			verbose mode
**		-r			re-certify CS/80 cartridge tape
**		-G			guru mode (undocumented)
**		-f  fmt_optn		specify format option
**		-i  interleave		specify interleave factor
**		-s  blkno		SCSI verify and reassign if bad
**		-n			no-verify pass
**
**		-D  debug mode		Only provided for amigo devices
**					( Undocumented )
*/

char *wsio_usage = "usage: mediainit [-vrn] [-f fmt_optn] [-i interleave] [-p partition_size] pathname\n";

char *sio_usage = "usage: mediainit [-vr] [-f fmt_optn] [-i interleave] pathname\n";

char *guru_warning = "\
WARNING:  You have invoked guru mode, a mode requiring  extensive device\n\
command  set  knowledge  in order to  properly  respond to prompts  that\n\
follow, and from which you could seriously  compromise  device integrity\n\
by responding improperly.  Are you SURE you want to proceed?";


/*
** include files
*/
#ifdef _WSIO
#include <sys/sysmacros.h>
#else
#define _WSIO
#include <sys/sysmacros.h>
#undef _WSIO
#endif /* _WSIO */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __hp9000s800
#include <sys/libIO.h>
#endif /* __hp9000s800 */
#include <unistd.h>

char *get_driver_name();

/*
** defines
*/
#define TRANSMASK       0x0800000 /* mask for transparent bit in minor number */
#define VMUNIX "/hp-ux"		/* the kernel */
#define VERSION -3		/* I/O tree version */
#define streq(a,b)      (strcmp((char *)(a), (char *)(b)) == 0)
#define CS80_MAJOR	4	/* cs80 character device major number */
#define MF_MAJOR	6	/* minifloppy character device major number */
#define AMIGO_MAJOR    11	/* amigo character device major number */
#define SCSI_MAJOR     47	/* scsi character device major number */
#define SCSITAPE_MAJOR 54	/* scsi tape character device major number */
#define AC_MAJOR       55	/* scsi character device major number */
#define SCSIFLOPPY_MAJOR 106	/* scsi floppy raw device major number */

/*
** global variables
*/
int verbose = 0;
int recertify = 0;
int guru = 0;
int fmt_optn = 0;
int interleave = 0;
int debug = 0;
int blkno = -1;
int verify_pass = 1;

int maj;
int minr;
int fd;
int unit;
int volume;
int iotype;
char	*name;	/* the name of the driver for the character major number */


/*
** other globals
*/
extern int sys_call_result;	/* system call result...mi_cs80.c */
extern int errno;
extern char **environ;

/*
** mediainit
*/
main(argc, argv)
int argc;
char **argv;
{
	char error = 0;
	int c;
	char *pathname;
	struct stat stat;
	int partition = 0;

	extern int optind;
	extern char *optarg;

	cleanenv(&environ, 0);

	if ( (iotype = _sysconf(_SC_IO_TYPE)) == -1 )
		err(errno, "sysconf failed");
	
	/*
	**  parse the options 
        */
	while ((c = getopt(argc, argv, "vrnGDf:p:i:s:?")) != EOF)
		switch (c) {

		case 'v':
			verbose++;
			break;

		case 'r':
			recertify++;
			break;

		case 'n':
			verify_pass = 0;
			break;

		case 'G':
			if (getuid())
				err(0, "only super-user can invoke guru mode");
			guru++;
			break;

		case 'D':
			if (getuid())
				err(0, "only super-user can invoke debug mode");
			debug++;
			break;

		case 'f':
			fmt_optn = strtol(optarg, 0, 0);
			break;

		case 'i':
			interleave = strtol(optarg, 0, 0);
			break;

		case 'p':
			if (iotype == IO_TYPE_WSIO) {
				partition = strtol(optarg, 0, 0);
			} else {
				error++;
			}
			break;

		case 's':
			blkno = strtol(optarg, 0, 0);
			break;

		case '?':
			error++;

		}  /* switch */

	/*
	** there should now be a single remaining file name parameter
	*/
	if (error || optind != argc - 1) {
		if (iotype == IO_TYPE_WSIO) {
			fprintf(stderr, wsio_usage);
		} else {
			fprintf(stderr, sio_usage);
		}
		exit(2);
	}
	pathname = argv[optind];

	/*
	** check for file read/write access permissions under real ID
	*/
	if (access(pathname, 06) < 0)
		err(errno, "can't access file %s", pathname);

	/*
	** open and stat the file
	*/
	if ((fd = open(pathname, O_RDWR)) < 0)
		err(errno, "can't open file %s", pathname);
	if (fstat(fd, &stat) < 0)
		err(errno, "can't stat file %s", pathname);

	/*
	** perform some file status processing
	*/
	if ((stat.st_mode & S_IFMT) != S_IFCHR)
		err(0, "character (raw) device special file required");
	maj = major(stat.st_rdev);
	minr = minor(stat.st_rdev);
	unit = m_unit(stat.st_rdev);
	volume = m_volume(stat.st_rdev);
	
	/*
	** check for proper use of the re-certify option
	*/
	if (recertify && maj != CS80_MAJOR)
		err(0, "re-certify option only for cartridge tapes");
	
	/*
	** check for proper use of the reassign and verify option
	*/
	if (blkno != -1 && maj != SCSI_MAJOR)
		err(0, "reassign/verify option only for direct-access SCSI");

	/*
	** check for guru mode and issue warning
	*/
	if (guru && !yes(guru_warning))
		err(0, "guru mode aborted");

	/*
	** call the appropriate mediainit device driver
	*/
	verb("initialization process starting");

	if (iotype == IO_TYPE_SIO) { /* s800 machine */
		if ((name = get_driver_name(maj)) == NULL) 
	    	exit(1);


		if (streq(name, "disc1") || streq(name, "disc2")) { 
			/*
			** check for transparent bit in minor number
			*/
			if (!(minr & TRANSMASK))
				err(0, "transparent device special file required");
				mi_sio_cs80();
		} else if (streq(name, "disc3")) {
			mi_scsi();
		} else if (streq(name, "autoch")) {
			mi_acinit();
		} else {
			err(0, "this type of device unsupported");
		} 
	} else  { /* s700 or s300 machine */
		switch (maj) {

		case CS80_MAJOR:
			mi_wsio_cs80();
			break;
#ifdef __hp9000s300
		case MF_MAJOR:
			mi_mfinit();
			break;
		case AMIGO_MAJOR:
			mi_amigo();
			break;
#endif /* __hp9000s300 */
		case SCSITAPE_MAJOR:
			if (iotype == IO_TYPE_WSIO) {
				mi_scsitape(partition);
			} else {
				err(0, "this type of device unsupported");
			}
			break;
		case SCSI_MAJOR:
		case SCSIFLOPPY_MAJOR:
			mi_scsi();
			break;
		case AC_MAJOR:
			mi_acinit();
			break;

		default:
			err(0, "this type of device unsupported");

		}  /* switch */
	}

	verb("initialization process completed");

	exit(0);
}


/*
** verb: standard procedure for printing verbose info to stdout
*/
/* VARARGS */
verb(p1, p2, p3, p4)
char *p1;
{
	if (verbose) {
		printf("mediainit: ");
		printf(p1, p2, p3, p4);
		printf("\n");
	}
}


/*
** err: standard procedure for printing error info to stderr, then exiting
*/
/* VARARGS */
err(errno, p1, p2, p3, p4)
int errno;
char *p1;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	fprintf(stderr, "mediainit: ");
	fprintf(stderr, p1, p2, p3, p4);
#ifdef __hp9000s800
	if (errno > 0 || sys_call_result < 0)
#else
	if (errno > 0)
#endif /* __hp9000s800 */
		if (errno <= sys_nerr)
			fprintf(stderr, " - %s", sys_errlist[errno]);
		else
			fprintf(stderr, " - errno %d", errno);
	fprintf(stderr, "\n");
	exit(1);
}


/*
** yes: interactive procedure asking for affirmative or negative response
*/
int yes(prompt)
char *prompt;
{
	char response, c;

	if (isatty(1) != 1)
		err(0, "stdout must be a tty to be interactive");
	if (isatty(0) != 1)
		err(0, "stdin must be a tty to be interactive");

	printf("%s (y/n) ", prompt);

	response = c = getchar();
	while (c != '\n' && c > 0)
		c = getchar();

	return response == 'y';
}


/*
** allow_modification: interactive procedure to allow parameter modification
*/
void allow_modification(prompt, parm_ptr, maxvalue)
char *prompt;
int *parm_ptr;
int maxvalue;
{
#define RESPONSE_LEN 10
	char response[RESPONSE_LEN + 1];
	char *cp;
	int newvalue;

	if (isatty(1) != 1)
		err(0, "stdout must be a tty to be interactive");
	if (isatty(0) != 1)
		err(0, "stdin must be a tty to be interactive");

	do {
		int c;

		printf("%s (defaults to %d) ", prompt, *parm_ptr);

		cp = response;
		while ((c = getchar()) != '\n' && c > 0)
			if (cp < response + RESPONSE_LEN)
				*cp++ = c;
		*cp = '\0';

	} while (cp > response &&
		 ((newvalue = atoi(response)) < 0 || newvalue > maxvalue));

	if (cp > response)
		*parm_ptr = newvalue;
}

/*
 * This routine will scan the iotree for the given character major number and
 * return the name of the driver associated with that character major number
 */
char	*get_driver_name(maj_num)
int	maj_num;
{
#ifdef __hp9000s800
io_mgr_type 		*mgr_table;
int          		 status, num_mgrs, i=0;

	if ((status = io_init(O_RDONLY)) != SUCCESS) {
	    (void)print_libIO_status("mediainit", _IO_INIT, status, O_RDONLY);
	    return(NULL);
	}

        if ((num_mgrs = io_get_table(T_IO_MGR_TABLE, (void **)&mgr_table))
	    < 0) {
	    (void)print_libIO_status("mediainit", _IO_GET_TABLE, status,
		    T_IO_MGR_TABLE, (void *)mgr_table);
	    io_end();
	    return(NULL);
	}

	for (i = 0; i < num_mgrs; i++) {
	    if (mgr_table[i].c_major == maj_num) {
	        static char driver_buf[MAX_ID];

	        strncpy(driver_buf, mgr_table[i].name, MAX_ID);
		io_free_table(T_IO_MGR_TABLE, (void **)&mgr_table);
		io_end();
	        return(driver_buf);
	    }
	}
	io_free_table(T_IO_MGR_TABLE, (void **)&mgr_table);
	io_end();
#endif /* __hp9000s800 */
	return(NULL);
}
