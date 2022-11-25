/*   $Revision: 64.1 $  */

/*  plotstat.c
 *  check status and paper change for HP-GL plotter
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/termio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/utsname.h>

#include "plotstat.h"

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 3		/* nl_setn */
#include <msgbuf.h>
#endif NLS

/*  constants  */
#define TIME 10			/*  timeout (SEC)  */

#define PLOTOKS 0x10		/* plotter status "ready for data" for OS */
#define PLOTOKE 0x70		/* plotter status "ready for data" for ESC.O */
#define ERRPAPER 0x20
#define ERRSTATE 0x50

extern int errno;

/*  extern variables */
extern char *fname;			/*  name of output file  */
extern char *cmdname;			/*  name of this program  */
extern char *logname;			/*  name of request id */
extern char *username;			/*  user name  */
extern int infd;			/*  input from  */
extern int outfd;			/*  output to */
extern int tty;				/*  Is output device tty or HP-IB?  */
extern int use_esc;			/*  use ESC.O instruction  */

/*  inqure status command  */
static char *inqstat_s = "OS;\r\n";
static char *inqstat_e = "\033.O\r";
static char *inqposition = "OA;\r\n";

/*  variables for checking status  */
char buff[20];		/*  buffer for device response  */

/*  variables for paper change  */
char prev_buf[20];	/*  buffer for previous pen position  */
char cur_buf[20];	/*  buffer for current pen position  */

/*  message buffer  */
char messages[80];
int message;		/*  sent a message if plotter is not ready */

char *hostname;		/* hostname */

check_status( check_timeout, messp )
	int check_timeout;	/*  check timeout?  */
	int messp;		/*  send message? */
{
	int i, stat;
	int length;		/*  command string length  */
	int stat_byte;		/*  plotter status byte  */
	char *com;		/*  outuput command  */
	char tmp;
	int first;
	int timeout;

	if ( tty ) catch_sig();

/*  make a command for inquire status  */
	if ( use_esc )
		com = inqstat_e;
	else
		com = inqstat_s;
	length = strlen( com );

	message = messp;
	first = 5;
	timeout = 0;
    	for (;;) {
		if ( tty ) {
			ioctl(infd,TCFLSH,0);
			ioctl(outfd,TCFLSH,1);
		}
/*  set timeout count for sending */
		if ( check_timeout ) {
		    if ( tty )
			    alarm( TIME );
		        else
			    io_timeout_ctl(outfd, (long)TIME * 1000000L );
	    	}

/*  send command  */
		stat = write(outfd, com, length);

/*  check timeout  */
		if ( tty )
			alarm( 0 );
		    else
			io_timeout_ctl(outfd, 0);
		if (stat == -1) {
			io_err();
			message = 0;
			timeout++;
			continue;
		}

/*  set timeout count for receiving */
		if ( check_timeout ) {
		    if ( tty )
			    alarm( TIME );
		        else
			    io_timeout_ctl(infd, (long)TIME * 1000000L );
		}

/*  receive command  */
		stat = read(infd, buff, 1);
/*  check timeout  */
		if ( tty )
			alarm( 0 );
		    else
			io_timeout_ctl(infd, 0);
		if (stat == -1) {
			io_err();
			message = 0;
			timeout++;
			continue;
		}

/*  skip control characters */
		if ( timeout && first )	{
			first--;
			continue;
		}
		while ( buff[0] < ' ' ) {
			read(infd, buff, 1);
		}

/*  get status byte */
		i = 0;
		while (buff[i] >= ' ') {
			read(infd, &buff[++i], 1);
		}

		buff[i] = '\0';
		stat_byte = atoi(buff);

/*  check status byte  */
		if ( use_esc ) {
			if ( stat_byte & PLOTOKE ) {
				if ( message ) {
					if ( stat_byte & ERRPAPER )
						send_message( PAPEROUT );
					else if ( stat_byte & ERRSTATE )
						send_message( NOTREADY );
					message = 0;
				}
			} else	break;
		} else {
			if ( stat_byte & PLOTOKS )
				break;
			else {
				if ( message )	send_message( NOTREADY );
				message = 0;
			}
		}
		sleep(1L);
	}
	return;
}

catch_sig()
{
	signal(SIGALRM,catch_sig);
	return;
}

/* paper change
 * If pinch wheel is raised to change a paper, pen position is changed.
 */

page()
{
	int i;
	int length;

/* obtain a pen position before paper change */
	length = strlen( inqposition );
	write(outfd,inqposition,length);
	do {
		read(infd,&prev_buf[0],1);
	} while( prev_buf[0] < ' ' );

	i = 0;
	while( prev_buf[i++] >= ' ' ) {
		read(infd,&prev_buf[i],1);
	}
	i--;

	prev_buf[i] = '\0';

	send_message( PAPERCHANGE );

/* compare current pen position and before one */
	do {
		sleep(1L);
		write(outfd,inqposition,length);
		do {
			read(infd,&cur_buf[0],1);
		} while( cur_buf[0] < ' ' );

		i = 0;
		while( cur_buf[i++] >= ' ' ) {
			read(infd,&cur_buf[i],1);
		}
		i--;

		cur_buf[i] = '\0';

	} while ( !strcmp(prev_buf,cur_buf) );
	return;
}

/* send a message to user */
send_message( mess )
	int mess;
{
	int pid;
	char *tmp_msg;

	get_hostname();

	tmp_msg = NULL;
	switch (mess) {
		case TIMEOUT :
				tmp_msg = nl_msg(1,"<%s> plotter not responding");
				break;
		case NOTREADY :
				tmp_msg = nl_msg(2,"<%s> plotter not ready");
				break;
		case PAPEROUT :
				tmp_msg = nl_msg(3,"<%s> plotter paper out");
				break;
		case PAPERCHANGE :
				tmp_msg = nl_msg(4,"<%s> request for plotter paper change");
				break;
	}
	sprintf(messages,tmp_msg,logname);

	switch (pid=fork()) {
	case 0:
		execl("/usr/lib/rwrite","rwrite",username,hostname,messages,NULL);
		error("cannot send message to user");
		exit(1);
	case -1:
		error("cannot create new process");
		exit(1);
	}
	wait(NULL);

	return;
}

/*  host name */
get_hostname()
{
#ifdef REMOTE
	char  *fbase,*f;
#endif REMOTE
	static char hname[UTSLEN];

#ifdef REMOTE
	/*  filename="dfA%03d%s" or "cA%04d%s" */
	fbase = f = fname;
	while (*f) {
		if (*f++ == '/')	fbase = f;
	}

	if (*fbase == 'd' && (*(fbase+1) == 'A' || *(fbase+2) == 'A')) {
			hostname = fbase + 6;
	}
	else  {
		gethostname(hname,UTSLEN);
		hostname = hname;
	}
#else REMOTE
	gethostname(hname,UTSLEN);
	hostname = hname;
#endif REMOTE

	return;
}

io_err()
{
	if (errno==EINTR || errno==ETIMEDOUT || errno==EIO ) {
		if ( message ) send_message( TIMEOUT );
		return;
	} else {
		error("write error");
		exit(1);
	}
}
