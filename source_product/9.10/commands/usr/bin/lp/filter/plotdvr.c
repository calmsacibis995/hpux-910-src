/*  $Revision: 66.2 $  */

/*  plotdvr.c
 *  filter program for HP-GL plotters
 */

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termio.h>
#include <errno.h>

#include "plotstat.h"

extern int errno;

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 3		/* nl_setn */
#include <msgbuf.h>
#endif NLS

#define BLKSIZ 512 		/*  block size of reading a file  */
#define TERM ';'		/*  terminate character of HP-GL  */
#define LF '\n'
#define CR '\r'
#define ETX '\003'		/*  term char for 'WD' 'BL' 'LB' command  */
#define HANDSHAKE "\033.I;;17:\r\033.N;19:\r"

/*  export variables  */
int infd=0;			/*  input from stadard input  */
int outfd=1;			/*  output to standard output */
int tty;			/*  Is device tty?  */
int use_esc;			/*  use escape instruction for obtaining stat*/
char *logname;			/*  request id  */
char *username;			/*  user name  */

/*  error message */
char *usage = "usage: %s -lrequest_id -uusername [-e -f -i] filename\n";

/*  local variables  */
char *cmdname;		/*  name of this command  */
char *fname;		/*  input file name  */
int fd;			/*  for input file  */

char *in_buf, *inp;	/*  input buffer  */
char *out_buf, *outp;	/*  output buffer  */

int blk_num;		/*  How many blocks in input file  */
int blk_remain;		/*  remaindar of input_file_size/BLKSIZ  */
int blk_cur;		/*  current block number  */

struct termio term, save_term;

main(argc, argv)
	int argc;
	char **argv;
{
	int no_init;	/*  initialize before any output?  */
	int no_stop;		/*  do not stop for paper change */
	int i;
	char c;
	extern char *optarg;
	extern int optind;

#ifdef NLS
	nl_init(getenv("LANG"));
	nl_catopen("lpfilter");
#endif NLS

	cmdname = argv[0];
	fname = logname = username = NULL;
	no_init = use_esc = no_stop = 0;

	while ((c = getopt(argc,argv,"efil:u:")) != EOF) {
		switch ( c ) {
		    case 'l' : logname = optarg;	/* id */
			       break;
		    case 'u' : username = optarg;	/* user */
			       break;
		    case 'i' : no_init = 1;	/* do not initialize? */
			       break;
		    case 'e' : use_esc = 1;	/* use ESC instruction*/
			       break;
		    case 'f' : no_stop = 1;	/*  do not stop */
			       break;
		    case '?' :
			       fprintf(stderr,usage,cmdname);
				exit(1);
		}
	}
	fname = argv[optind];		/* input file */

	if (fname == NULL || username == NULL) {
		fprintf(stderr,usage,cmdname);
		exit(1);
	}
	if ( isatty( infd ) ) {
		tty = 1;
		if (ioctl(infd,TCGETA,&term) == -1) {
			fprintf(stderr,"%s: Cannot get device status\n",cmdname);
			exit(1);
		}
		save_term = term;
		term.c_lflag &= ~(ICANON | ECHO);
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 1;
		if (ioctl(infd,TCSETAW,&term) == -1) {
			fprintf(stderr,"%s: Cannot set device status\n",cmdname);
			exit(1);
		}
	}
	else
		tty = 0;

	get_buffer();
	check_status( TRUE, TRUE );
	if ( tty )
	{
	    if (write(outfd,HANDSHAKE,strlen(HANDSHAKE)) == -1)
		error("write error");
	}

	if ( ! no_init )
	{
	    if (write(outfd,"IN;\r\n",5) == -1)
		error("write error");
	}
	
	if ( no_stop || use_esc )
		send_data();
	else
		send_hpgl();

	free(out_buf);
	free(in_buf);
	close(fd);

	if ( tty )	ioctl(infd,TCSETAW,&save_term);
}


get_buffer()
{
	struct stat stat_buf;
	int file_size;		/*  input file size  */

	if ( (fd = open(fname,O_RDONLY)) == -1) {
		error("cannot open file");
		exit(1);
	}
	if ( fstat(fd,&stat_buf) == -1 ) {
		error("cannot read file");
		exit(1);
	}
	file_size = stat_buf.st_size;

	blk_remain = file_size % BLKSIZ;
	blk_num = ( file_size + BLKSIZ - 1 ) / BLKSIZ;
	blk_cur = 0;				/* current block number */

	in_buf = malloc(BLKSIZ); 	/* get input buffer */
	out_buf = malloc(BLKSIZ * 2);	/* get output buffer */
	if ( in_buf == NULL || out_buf == NULL ) {
		error("cannot get buffer");
		exit(1);
	}
	return;
}

read_block()
{
	int stat;
	int tmp;

	if (blk_num <= ++blk_cur) {		/* Is it last block? */
		if (blk_remain == 0)
			return(0);
		else {
			stat = read(fd,in_buf,blk_remain);
			tmp = blk_remain;
			blk_remain = 0;
		}
	}
	else {
		stat = read(fd,in_buf,BLKSIZ);		/* not last block */
		tmp = BLKSIZ;
	}
	if ( stat == -1 ) {
		error("cannot read file");
		exit(1);
	}
	return( tmp );
}

/*  Send HPGL instruction to plotter.
 *  If "PG" instruction(Page) occurs, "Paper change request" message is
 *  sent to user when Plotter does not have autofeed capability.
 *  But when "PG" occurs between "LB" instruction and ETX,
 *  it is string data. 
 */
 
send_hpgl()
{
	int state;
	char *in_maxp;		/* location of last input data  */
	char save_p,save_g;	/* char is saved here when "p" or "g" occurs */
	int in_bytes;		/* how many data are read into input buffer */

	state = 0;
	outp = out_buf;
	while ( in_bytes = read_block() ) {
	    in_maxp = &in_buf[in_bytes-1];
	    for(inp=in_buf; inp<=in_maxp; inp++) {
		switch( state ) {
		case 0 :			/* normal state */
			switch( *inp ) {
			case 'P' :
			case 'p' :  state = 1;
				    save_p = *inp;
				    break;
			case 'L' :
			case 'l' :  state = 2;
				    *outp++ = *inp;
				    break;
			default  :  *outp++ = *inp;
			}
			break;
		case 1 :		/* state after "P" is read */
			if (*inp=='G' || *inp=='g') {
				state = 3;
				save_g = *inp;
			} else {
				state = 0;
				*outp++ = save_p;
				*outp++ = *inp;
			}
			break;
		case 2 :		/* state after "L" is read */
			*outp++ = *inp;
			if (*inp=='B' || *inp=='b')
				state = 4;
			else
				state = 0;
			break;
		case 3 :		/* state after "PG" are read */
			if (*inp==TERM || *inp==LF) {
				if (out_buf != outp)	/*send remaining data*/
					if (write(outfd,out_buf,(outp-out_buf))
							== -1 ) {
						error("write error");
					}
				outp = out_buf;
				page();
				check_status( FALSE, FALSE  ); /* wait for ready */
				state = 0;
			}
			break;
		case 4 :		/* state after string */
			if (*inp == ETX)	state = 0;
			*outp++ = *inp;
			break;
		}
	    }
	    if (out_buf != outp)		/* send data to plotter */
		if (write(outfd,out_buf,(outp - out_buf)) == -1)
		    error("write error");
	    outp = out_buf;
	}
}

send_data()
{
	int in_bytes;

	while ( in_bytes = read_block() )
		if (write(outfd,in_buf,in_bytes) == -1)
		    error("write error");
	return;
}

error(msg)
	char *msg;
{
	fprintf(stderr,"%s: %d %s\n",cmdname,errno,msg);
	exit(1);
}

