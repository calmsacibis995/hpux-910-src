/* @(#) $Revision: 66.2 $ */     
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
**
**		-D  debug mode		Only provided for amigo devices
**					( Undocumented )
*/

char *usage = "usage: mediainit [-vr] [-f fmt_optn] [-i interleave] [-p partition_size] pathname\n";

char *guru_warning = "\
WARNING:  You have invoked guru mode, a mode requiring  extensive device\n\
command  set  knowledge  in order to  properly  respond to prompts  that\n\
follow, and from which you could seriously  compromise  device integrity\n\
by responding improperly.  Are you SURE you want to proceed?";


/*
** include files
*/
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

/*
** defines
*/
#define CS80_MAJOR	4	/* cs80 character device major number */
#define MF_MAJOR	6	/* minifloppy character device major number */
#define AMIGO_MAJOR    11	/* amigo character device major number */
#define SCSI_MAJOR     47	/* scsi character device major number */
#define SCSITAPE_MAJOR 54	/* scsi tape character device major number */
#define AC_MAJOR       55	/* scsi character device major number */

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

int fd;
int unit;
int volume;


/*
** other globals
*/
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
	int maj;
	int partition = 0;

	extern int optind;
	extern char *optarg;

	cleanenv(&environ, 0);
	/*
	**  parse the options 
        */
	while ((c = getopt(argc, argv, "vrGDf:p:i:s:")) != EOF)
		switch (c) {

		case 'v':
			verbose++;
			break;

		case 'r':
			recertify++;
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
			partition = strtol(optarg, 0, 0);
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
		fprintf(stderr, usage);
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
	unit = m_unit(stat.st_rdev);
	volume = m_volume(stat.st_rdev);
	
	/*
	** check for proper use of the re-certify option
	*/
	if (recertify && maj != CS80_MAJOR)
		err(0, "re-certify option only for cartridge tapes");

	/*
	** check for guru mode and issue warning
	*/
	if (guru && !yes(guru_warning))
		err(0, "guru mode aborted");

	/*
	** call the appropriate mediainit device driver
	*/
	verb("initialization process starting");

	switch (maj) {

	case CS80_MAJOR:
		mi_cs80();
		break;
	case MF_MAJOR:
		mi_mfinit();
		break;
	case AMIGO_MAJOR:
		mi_amigo();
		break;
	case SCSITAPE_MAJOR:
		mi_scsitape(partition);
		break;
	case SCSI_MAJOR:
		mi_scsi();
		break;
	case AC_MAJOR:
		mi_acinit();
		break;

	default:
		err(0, "this type of device unsupported");

	}  /* switch */

	verb("initialization process completed");

	exit(0);
}


/*
** verb: standard procedure for printing verbose info to stdout
*/
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
err(errno, p1, p2, p3, p4)
int errno;
char *p1;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	fprintf(stderr, "mediainit: ");
	fprintf(stderr, p1, p2, p3, p4);
	if (errno > 0)
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
