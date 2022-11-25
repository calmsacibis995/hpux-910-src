/*  $Revision: 72.1 $  */

/*	cgmtohpgl.c
 *   CGM file to HP-GL plotter  filter program
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termio.h>
#include <starbase.c.h>
#include <errno.h>

#include "plotstat.h"

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 3		/* nl_setn */
#include <msgbuf.h>
#endif NLS

#define HANDSHAKE "\033.I;;17:\r\033.N;19:\r"

/*  exported variables */
int infd;				/* input fd */
int outfd;				/* output fd */
int tty;				/* Is device tty? */
int use_esc;				/* use ESC command */
char *logname;				/* request id */
char *username;				/* user name */

char *fname;				/* inputfile */
char *cmd;				/* command name */
char *usage = "%s: Usage -ddevicefile -lrequestid -uusername [-e -f] filename\n";

char *driver;
int  cgmne;				/* if starbase error then return (0)
					   code */

/*  for Starbase error handling */
void sb_error();

main(argc,argv)
	int argc;
	char **argv;
{
	struct stat stat_buf;			/* for stat() */
	struct termio term,save_term;		/* for ioctl() */
	char *devname;				/* output device name */
	int fd;					/* file descriptor */
	int encoding, num_pic;			/* cgm file info. */
	int no_stop;				/* don't stop for paper change*/
	int i;					/* loop counter */
	char c;
	extern char *optarg;
	extern int optind;

#ifdef NLS
	nl_init(getenv("LANG"));
	nl_catopen("lpfilter");
#endif NLS

/*  check command argument */
	cmd = argv[0];
	fname = logname = devname = username = NULL;
	use_esc = no_stop = cgmne = 0;


	while ((c = getopt(argc,argv,"ecfd:l:u:")) != EOF) {
		switch ( c ) {
			case 'd' : devname = optarg;
				   break;
			case 'l' : logname = optarg;
				   break;
			case 'u' : username = optarg;
				   break;
			case 'e' : use_esc = 1;
				   break;
			case 'f' : no_stop = 1;
				   break;
			case 'c' : cgmne = 1;
				   break;
			case '?' : fprintf(stderr,usage,cmd);
				   exit(1);
		}
	}
	fname = argv[optind];

	if (fname == NULL || devname == NULL || username == NULL) {
		fprintf(stderr,usage,cmd);
		exit(1);
	}

/*  Is output devfile device special?  */
	if ( stat(devname,&stat_buf) == -1 ||
			! (stat_buf.st_mode & 020000) ) {
		error("output file must be device special");
		exit(1);
	}

/*  Does specified file exist? */
	if ( stat(fname,&stat_buf) == -1) {
		error("cannot open file");
		exit(1);
	}

/*  set starbase error handling  */
	gerr_print_control(PRINT_ERRORS);
	gerr_procedure(sb_error);

/*  get CGM file information */
	inquire_cgm(fname,&encoding,&num_pic);
	if (num_pic == 0)  exit(0);		/* no picture data */

/*  open device for obtaining device status  */
	if ( (outfd = infd = open(devname,O_RDWR)) == -1) {
		error("cannot open ");
		exit(1);
	}

/*  if tty device  set termio  */
	if ( isatty( infd ) ) {
		tty = 1;
		if (ioctl(infd,TCGETA,&term) == -1) {
		fprintf(stderr,"%s: Cannot get device status\n",cmd);
			exit(1);
		}
		memcpy(&save_term,&term,(sizeof (struct termio)));
		term.c_lflag &= ~(ICANON | ECHO);
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 1;
		if (ioctl(infd,TCSETAW,&term) == -1) {
			fprintf(stderr,"%s: Cannot set device status\n",cmd);
			exit(1);
		}
		driver = "hpgls";
	}
	else {
		tty = 0;
		driver = "hpgl";
	}

/*  check device status */
	check_status(TRUE, TRUE);
	if ( tty ) {
		write(outfd,HANDSHAKE,strlen(HANDSHAKE));
		ioctl(infd,TCSETAW,&save_term);
	}
	write(outfd,"IN;\r\n",5);

/*  open starbase */
/*  spool HP-GL data for each page */
	for (i=1; i<=num_pic; i++) {
		if ( (fd=gopen(devname,OUTDEV,driver,
					INIT | RESET_DEVICE )) == -1) {
			error("cannot open output device");
			exit(1);
		}
		cgm_to_starbase(fd,fname,i);
		gclose(fd);
		if ( !use_esc && !no_stop && i != num_pic) {
			if ( tty ) {
				write(outfd,"\033.R\r\n",strlen("\033.R\r\n"));
				write(outfd,HANDSHAKE,strlen(HANDSHAKE));
				ioctl(infd,TCSETAW,&term);
			}
			page();
			check_status(FALSE, FALSE);
			if ( tty ) ioctl(infd,TCSETAW,&save_term);
		}
	}

	if ( tty ) {
		write(outfd,"\033.R\r\n",strlen("\033.R\r\n"));
		if ( tty ) ioctl(infd,TCSETAW,&save_term);
	}
	close(infd);
	close(outfd);
}

/*  error message */
error(msg)
	char *msg;
{
	fprintf(stderr,"%s: %s\n",cmd,msg);
	if ( !cgmne )
	   exit(1);
}

/*  stabase error handling procedure  */
void sb_error()
{
	int  errornumber,errorfd;
	char *errmessage;
	char *gerr_message();

	inquire_gerror(&errornumber,&errorfd);
	errmessage = gerr_message(errornumber);

	error( errmessage );

	if ( !cgmne )
	   exit(1);
}

