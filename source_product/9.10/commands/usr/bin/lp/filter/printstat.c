/*  $Revision: 64.1 $  */

/*  printstat.c
 *  check printer status
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/termio.h>
#include <errno.h>
#include <signal.h>
#include <sys/utsname.h>

extern int errno;

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 2		/* nl_setn */
#include <msgbuf.h>
#endif NLS

#define CHK_TIMEOUT	10L	/* timeout (SEC) */
#define WAIT		1L	/* wait time when printer is not ready */
#define WAIT_TIMEOUT	15L	/* wait time after timeout */

#define COMMAND  "\033?\021"	/* PCL command  "ESC ? DC1"  */

#define TIMEOUT	1
#define OFFLINE 2
#define PAPEROUT 3

int infd=0;			/*  input from stadard input  */
int outfd=1;			/*  output to standard output */
char *logname;			/*  request id  */
char *username;			/*  user name  */
char *hostname;			/*  hostname  */
char *cmdname;			/*  name of this command */
char messages[80];		/*  message buffer  */
char *fname;			/*  filename to obtain host name  */
int message;			/*  a flag indicates message has sent  */

void chatch_sig();

main(argc, argv)
	int argc;
	char **argv;
{
	struct termio term,save_term;
	int length;
	int ready;
	int stat;
	char status;
	int timeout;
	int first;
	int i;

#ifdef NLS
	nl_init(getenv("LANG"));
	nl_catopen("lpfilter");
#endif NLS

	cmdname = argv[0];
	if ( ! isatty( infd ) )	{
		fprintf(stderr,"%s: Not a tty device\n",cmdname);
		exit(0);
	}
	logname = username = hostname = NULL;
	for (i=1; i<argc; i++) {
		if ( argv[i][0] == '-' ) {
			switch ( argv[i][1] ) {
			    case 'l' : logname = &argv[i][2];	/* id */
				       break;
			    case 'u' : username = &argv[i][2];	/* user */
				       break;
			    default  :
					fprintf(stderr,
				"usage: %s -llogname -uusername filename\n",
								cmdname);
					exit(1);
			}
		}
		else {
			fname = argv[i];
		}
	}

	if (username == NULL || fname == NULL) {
		fprintf(stderr,"usage: %s -llogname -uusername filename\n",cmdname);
		exit(1);
	}

	message = 1;
	ready = 0;
	first = 1;
	timeout = 0;

	if (ioctl(infd,TCGETA,&term) == -1) {
		fprintf(stderr,"%s: Cannot get device status\n",cmdname);
		exit(1);
	}

	memcpy(&save_term,&term,(sizeof (struct termio)));

	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 1;

	if (ioctl(infd,TCSETAW,&term) == -1) {
		fprintf(stderr,"%s: Cannot set device status\n",cmdname);
		exit(1);
	}

	catch_sig();
	length = strlen(COMMAND);

    	while ( ! ready ) {
/*  flush input/output buffer  */
		ioctl(infd,TCFLSH,0);
		ioctl(outfd,TCFLSH,1);
/*  set timeout count for sending */
		alarm( CHK_TIMEOUT );

/*  send command  */
		stat = write(outfd, COMMAND, length);

/*  check timeout  */
		alarm( 0 );
		if (stat == -1) {
			io_err();
			timeout = 1;
			message = 0;
			first = 1;
			continue;
		}

/*  set timeout count for receiving */
		alarm( CHK_TIMEOUT );

/*  receive command  */
		stat = read(infd, &status, 1);

/*  check timeout  */
		alarm( 0 );
		if (stat == -1) {
			io_err();
			timeout = 1;
			message = 0;
			first = 1;
			continue;
		}
		if ( first ) {
			first = 0;
			continue;
		}

/*  check status byte  */
		if ( !(status & 0x7) ) {
			ready = 1;
			break;
		}
		else {
			if (message) {
				if (status & 0x1)
					send_message( PAPEROUT );
				else if (status & 0x4)
					send_message( OFFLINE );
				message = 0;
			}
		}
		sleep(WAIT);
	}
	if ( timeout )	sleep(WAIT_TIMEOUT);

	ioctl(infd,TCSETAW,&save_term);

	return;
}

io_err()
{
	if (errno==EINTR) {
		if ( message )  send_message( TIMEOUT );
		return;
	} else {
		fprintf(stderr,"I/O error\n");
		exit(1);
	}
}

catch_sig()
{
	signal(SIGALRM,catch_sig);
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
				tmp_msg = nl_msg(1,"<%s> printer not responding");
				break;
		case OFFLINE :
				tmp_msg = nl_msg(2,"<%s> printer off-line");
				break;
		case PAPEROUT :
				tmp_msg = nl_msg(3,"<%s> printer paperout or error");
				break;
	}
	sprintf(messages,tmp_msg,logname);

	switch (pid=fork()) {
	case 0:
		execl("/usr/lib/rwrite","rwrite",username,hostname,messages,NULL);
		fprintf(stderr,"cannot execute");
		exit(1);
	case -1:
		fprintf(stderr,"cannot create new process");
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
	else {
		gethostname(hname,UTSLEN);
		hostname = hname;
	}
#else REMOTE
	gethostname(hname,UTSLEN);
	hostname = hname;
#endif REMOTE

	return;
}

