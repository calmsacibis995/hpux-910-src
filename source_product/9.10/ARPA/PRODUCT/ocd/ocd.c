/*
 * Copyright (c) 1983, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * 00.02 : 11-Dec-91 - alpha test version.
 * 00.03 : 12-Dec-91 - fixed bug in pc_file_parser, improved parser perf.
 *                     errno output on log messages common to syslog.
 *                     pty no longer duped to stdin as this was getting syslog
 *                     in a knot.
 * 00.04 : 13-Dec-91 - initialized net and p when connection closed so as not to
 *                     mess up the select.
 *                     Included outgoing binary functionality.
 *                     Improved received ioctl debug dump.
 *                     Closeup modified to send SIGHUP to slave.
 *                     Signal handling corrected.
 * 00.05 : 16-Dec-91 - maintains a file called dfa.devices containing info
 *                     <pseudonym><pty><pid> for use by dfa_tidy.
 *                     Allows disabling of telnet_mode.
 *                     Corrected bug in opening telnet negotiation
 *                     Now transmits ioctl BRK through telnet
 * 00.06 : 18-Dec-91 - integrates full binary negotiation.
 * 00.07 : 08-Jan-92 - uses utmp file instead of dfa.devices. Logs
 *                     <pty><pseudonym><pid>
 *                     properly analyses pseudonym ioctls
 *                     pty and net buffs increased from 512 => 1024 bytes
 *                     ioctl FLUSHes tentatively integrated
 *                     open to device no longer queued
 *                     corrected bug in close when close_timer=0
 *                     fixed bug when dpp passes x or X as board number
 * 00.08 : 08-Jan-92 - main select i/o/xbit processing modified
 *                     ioctl fail sends SIGHUP instead of SIGTERM
 *                     fixed pass-thru bug when status_timer=0              
 * 00.09 : 09-Jan-92 - main select i/o/xbit processing modified
 *                     fixed new bug when dpp passes x or X as board number
 *                     ioctl flush processing removed because not good
 *                     fixed SR bug when time < SR_SLEEP
 * 00.10 : 10-Jan-92 - sends SIGTERM if slave application ignore SIGHUP
 * 00.11 : 10-Jan-92 - to see whats happening with the modems
 *                     uses personal utmp file
 *                     corrected closing account call (slave)
 *                     close failure no longer sends a signal
 *                     removed sleep in SR. Calculates time left after select.
 *                     Modified BN to use same process as SR for timeout.
 *                     fixed bug when dpp passes a port number = x or X
 * 00.12 : 15-Jan-92 - printer name added to pcf for disable command
 *                     shuts pty and optionally disables spooler on error
 *                     modified account call not to register slave
 * 00.13 : 20-Jan-92 - final version?
 *                     stat of pty corrected: statbuf => &statbuf
 * 00.14 : 30-Jan-92 - cleaned up using lint
 * 00.15 : 05-Feb-92 - included mechanism to continually verify the existence
 *                     of the pseudonym.
 * 00.16 : 06-Feb-92 - final build
 * 01.00 : 23-Mar-92 - Version change to 1.01
 * 01.01 : 29-Apr-92 - Fixed a bug of ocd abort using dpp w/o -k option  
 *                     Fixed a bug to cleanup utmp file after dpp -k option
 *       : 17-Aug-92 - Special build for customer Carrefour.
 *                     A1 - Add a new parameter in pcf file: number of SR
 *                          retries
 *                     A2 - Removed second (post-print) printer status check
 *                     A3 - Disable printer when SR check is unsuccessful
 *       : 15-Sep-92 - A4 - Modified code to detect stty 'EXTA' setting at the
 *                          'treat TIOIOCTL' section.
 *                     A5 - Added handling of second status request at the
 *                           'treat TIOIOCTL' section under 'TCSETAW'.
 *                     A6 - Do timing mark processing in TIOIOCTL after each
 *                          file and not in closedown()
 *                     A7 - Removed new parameter from pcf file for
 *                          number of SR retries.  Ocd still has the
 *                          capability.  Just need to add the parameter to
 *                          pcf file.  Hidden option.
 */

char    copyright_o[]= "Copyright (c) 1992, HEWLETT-PACKARD COMPANY";
static char rcsid[]="@(#)$Header: ocd.c,v 1.1.109.9 95/01/10 10:04:05 craig Exp $";
#ifdef PATCH_STRING
static char *PHNE_4818_patch =
	"@(#) PATCH_9.0:        ocd.c    1.1.109.8   94/10/07 PHNE_4818";
#endif
#ifdef DEBUG
static char id[] = "@(#)$Debug version                                   $";
#endif

#define FALSE 0
#define TRUE 1
#ifdef DEBUG
#define TELOPTS TRUE
#endif

/*
 * Telnet server.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef MYTELNET
#include "../ARPA/telnet.h"
#else
#include <arpa/telnet.h>
#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#include <ctype.h>

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <termio.h>
#include <sys/ptyio.h>
#include <utmp.h>
#include <pwd.h>
#include <grp.h>
#include <sys/modem.h>

#define STRSIZ 256		/* max log message string size */
#define MAXARPANAML 256		/* max length for an node-name */
#define LINESIZ 80		/* max line size for pc_file */
#define TRACE 0x1		/* procedure tracing flag */
#define TRACK 0x2		/* tracking flag */
#define DUMPS 0x4		/* data structure dump flag */
#define IOCTL 0x8		/* ioctl data structure dump flag */
#define MIN_BOARD 0		/* minimum board number of server */
#define MAX_BOARD 7		/* maximum board number of server */
#define MIN_PORT 0		/* minimum port number of server */
#define MAX_PORT 32		/* maximum port number of server */
#define MAX_BACKOFF 32		/* max backoff time (s) for expo backoff */
#define FOREVER 0x7FFFFFFF	/* seconds => roughly 68 years */
#define TM_FAILED 999		/* errno for TM negotiation failure */
#define SR_FAILED 998		/* errno for SR negotiation failure */
#define SR_SLEEP 30		/* sleep for 30 secs between status reqs */
#define LP_OK 0x30
#define LP_NO_PAPER 0x31
#define LP_BUSY 0x32
#define LP_OFF_LINE 0x34
#define LP_DATA_ERROR 0x38
#ifndef IPPORT_TELNET
#define IPPORT_TELNET 23
#endif


/* redefine BUFSIZ for use with NVS ioctl */
#undef BUFSIZ
#define BUFSIZ 1024

#define	OPT_NO			0		/* won't do this option */
#define	OPT_YES			1		/* will do this option */
#define	OPT_YES_BUT_ALWAYS_LOOK	2
#define	OPT_NO_BUT_ALWAYS_LOOK	3
char	hisopts[256];
char	myopts[256];

char	doopt[] = { IAC, DO, '%', 'c', 0 };
char	dont[] = { IAC, DONT, '%', 'c', 0 };
char	will[] = { IAC, WILL, '%', 'c', 0 };
char	wont[] = { IAC, WONT, '%', 'c', 0 };

/*
 * I/O data buffers, pointers, and counters.
 */
char	ptyibuf[BUFSIZ], *ptyip = ptyibuf;

char	ptyobuf[BUFSIZ], *pfrontp = ptyobuf, *pbackp = ptyobuf;

char	netibuf[BUFSIZ], *netip = netibuf;
#define	NIACCUM(c)	{   *netip++ = c; \
			    ncc++; \
			}

char	netobuf[BUFSIZ], *nfrontp = netobuf, *nbackp = netobuf;
char	*neturg = 0;		/* one past last bye of urgent data */
	/* the remote system seems to NOT be an old 4.2 */
int	not42 = 1;

		/* buffer for sub-options */
char	subbuffer[100], *subpointer= subbuffer, *subend= subbuffer;
#define	SB_CLEAR()	subpointer = subbuffer;
#define	SB_TERM()	{ subend = subpointer; SB_CLEAR(); }
#define	SB_ACCUM(c)	if (subpointer < (subbuffer+sizeof subbuffer)) { \
				*subpointer++ = (c); \
			}
#define	SB_GET()	((*subpointer++)&0xff)
#define	SB_EOF()	(subpointer >= subend)

int	pcc = 0;
int	ncc = 0;

int	pty, p, net;
int	inter;
extern	char **environ;
extern	int errno;
int	SYNCHing = 0;		/* we are in TELNET SYNCH mode */

static char reason_msg[80];
char enable[] = "enable";
char t_c = 'T';
char f_c = 'F';
/* Set up ocd option default values */
short	n_m_opt = 2;		/* nb mandatory options */
char	node_name[MAXARPANAML]; /* server name, no default */
char	pseudonym[MAXPATHLEN+1]; /* pseudo-device pseudonym, no default */
char	*p_name;		/* pointer to name part of pseudonym */
char	board_port = FALSE;     /* no board => assume port_no is a tcp_port */
short	board = -1;		/* server board number */
short	port = IPPORT_TELNET;	/* server or tcp port nb, default => telnet */
unsigned short	deb_level = 0;	/* debug level, default => disabled */
char	pc_file_ok = FALSE;	/* assume no port_config file */
char	pc_file_path[MAXPATHLEN+1];	/* pc file path, no default */
/* Set up pc file parameter defaults */
char	log_file_path[MAXPATHLEN+1] = "/dev/console";	/* log file */
char	telnet_mode = TRUE;	/* telnet mode enabled */
char	timing_mark = TRUE;	/* TM before TCP FIN enabled */
char    TM_sent = FALSE;
unsigned int	telnet_timer = 120;	/* telnet neg time_out */     
char 	binary_mode = FALSE;	/* binary mode disabled */
unsigned int	open_tries = 0;	/* nb times to try connect, 0=> infinite */
unsigned int	c_tries;	/* number of connect tries left */
unsigned int	s_tries;	/* number of socket tries left */
char get_out;			/* connect loop flag */
unsigned int	open_timer = 0;	/* period between tries, 0=> exponential */
unsigned int	close_timer = 0;/* time between pseudonym close & cnxn close */
char	status_request = FALSE;	/* status request disabled */
unsigned int	status_timer = 30;	/* status request timeout */
unsigned int    status_tries = 1;       /* nb times to try status request */
char	eightbit = FALSE;	/* eightbit mode */
char	tcp_nodelay = TRUE;	/* socket optiont */
int	bin_neg_state = 0;	/* binary negotiation status */
#ifdef DEBUG
/* debug toggles */
char trace_on = FALSE;		/* procedure tracing off */
char track_on = FALSE;		/* tracking off */
char dumps_on = FALSE;		/* data structure dump off */
char ioctl_on = FALSE;		/* special for ioctl dumps */
#endif
/* received pseudonym ioctl request params */
int r_val;
char r_who;
/* socket things */
struct hostent *hp;
struct sockaddr_in sin;
char cnxn_open = FALSE;
/* telnet binary counter */
/* misc */
int on=1;
int off=0;
struct utmp *utptr, utent;
struct request_info req_info;
char msg[STRSIZ];
char time_out = FALSE;
int my_pid;
int ret_val;		/* for general use */
dev_t slave_rdev;

char	*pc_params[] = {	/* pc_file parameter names */
		"telnet_m\0",	/* telnet_mode */
		"timing_m\0",	/* do TM before TCP fin */
		"telnet_t\0",	/* telnet_timer */
		"binary_m\0",	/* binary_mode */
		"open_tri\0",	/* open_tries */
		"open_tim\0",	/* open_timer */
		"close_ti\0",	/* close_timer */
		"status_r\0",	/* status_request */
		"status_t\0",	/* status_timer */
                "status.t\0",   /* status_tries */
		"eightbit\0",	/* eightbit mode */
		"tcp_nodelay\0",/* socket option */
		"\0"		/* end */
	};
#define NCPERP 8                /* size of the above fields */

/*
 * The following are some clocks used to decide how to interpret
 * the relationship between various variables.
 */

struct {
    int
	system,			/* what the current time is */
	echotoggle,		/* last time user entered echo character */
	modenegotiated,		/* last time operating mode negotiated */
	didnetreceive,		/* last time we read data from network */
	ttypeopt,		/* ttype will/won't received */
	ttypesubopt,		/* ttype subopt is received */
	getterminal,		/* time started to get terminal information */
	gotDM;			/* when did we last see a data mark */
} clocks;

#define	settimer(x)	(clocks.x = ++clocks.system)
#define	sequenceIs(x,y)	(clocks.x < clocks.y)


/*
 *Print errors to stderr of parent, until we get the syslog going
 */
usage()
{
	sprintf(msg,
#ifdef DEBUG
"usage: ocdebug -n<node_name> -f<pseudonym> [-b<board_no>]\
          [-p<port_no>] [-c<pc_file_path>] [-d<level>]\0");
#else
"usage: ocd -n<node_name> -f<pseudonym> [-b<board_no>]\
          [-p<port_no>] [-c<pc_file_path>]\0");
#endif
	syslog(LOG_ERR, msg);
	exit(1);
}

int pseudonym_available();
int check_pseudonym();
void cleanup();
void closeup();
void closedown();
void alarm_call();
void log();
void parse_pc_file();
void toggle_tracx();
void dont_retry();
int do_status_req();
void shakeup_pty();
void Dump();
void printoption();
void disable_printer();


#define bcopy(a,b,c)    memcpy(b,a,c)  /*!!! shouldn't be necessary */

main(argc, argv)

	char *argv[];
{
	char *cp;
	int fd;

	/*
	 * Fork off to be sure that we are not already a process group leader
	 */
	if(fork() !=0){
#ifdef DEBUG
		fprintf(stderr,"Parent process exit OK\n");
#endif
		exit(0);
	}

#ifdef DEBUG
	fprintf(stderr,"child forked OK\n");
#endif

	/*
	 * Set the child up as a process group leader and lose
	 * controlling terminal.
	 * It must not be part of the parent's process group,
	 * because signals issued in the child's process group
	 * must not affect the parent and vv.
	 */
	if(( my_pid = setsid() ) == -1){
#ifdef DEBUG
		fprintf(stderr,"setsid failed: %d\n",errno);
#endif
		exit(1);
	}
#ifdef DEBUG
	fprintf(stderr,"setsid OK, pid: %d\n", my_pid);
#endif

	/*
	 * open syslog
	 */
	openlog(strcat("ocd",pseudonym),
                LOG_PID | LOG_ODELAY | LOG_CONS, LOG_DAEMON);

	/*
	 * Set up file creation modes
	 */
	(void)umask(0133);

	/*
	 * Close inherited file descriptors
	 */
	for (fd = 0; fd < NOFILE; fd++){
#ifdef DEBUG
		if (fd != 2){			/* don't close stderr */
#endif
			if (close(fd)==0){
#ifdef DEBUG
				sprintf(msg,"File descriptor %d closed\0", fd);
				if(track_on)log(msg);
#endif
			}
#ifdef DEBUG
		}
#endif
	}
#ifdef DEBUG
	/*
	 * Open log file 
	 */
	sprintf(log_file_path,"/usr/adm/ocd%d\0",my_pid);
	if( (fd = open( log_file_path, O_RDWR | O_CREAT )) ==-1){
		syslog(LOG_ERR,"Can't open log file '%s': %m");
		exit(1);
	}
	fprintf(stderr,"Debug logging to %s. See syslog also.\n",log_file_path);

	if( dup2(fd,2) == -1){
		syslog(LOG_ERR,"Can't dup stderr: %m");
		exit(1);
	}
#endif
	
	/*
	 * Become immune from process group leader death and become a
	 * non process-group leader.
	 */
	signal(SIGHUP, SIG_IGN);
	signal(SIGCLD, SIG_IGN);

	/*
	 * Ignore terminal stop signals
	 * !!! perhaps no need, since we don't have a terminal 
	 */
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	/*
	 * The following signals are not ignored
	 */
	signal(SIGTERM, cleanup,
		sigmask(SIGUSR1) | sigmask(SIGUSR2) | sigmask(SIGTERM) |
		sigmask(SIGALRM) | sigmask(SIGPIPE));

	signal(SIGPIPE, closeup,
		 sigmask(SIGUSR1) | sigmask(SIGUSR2) | sigmask(SIGTERM) |
		 sigmask(SIGALRM) | sigmask(SIGPIPE));
#ifdef DEBUG
	/*
	 * toggle trace & track
	 */
	signal(SIGUSR1, toggle_tracx,
		sigmask(SIGUSR1) | sigmask(SIGUSR2) | sigmask(SIGTERM) |
		sigmask(SIGALRM) | sigmask(SIGPIPE));
#else
	signal(SIGUSR1, SIG_IGN);
#endif
	/*
	 * get out of socket/connect loop
	 */
	signal(SIGUSR2, dont_retry,
	  sigmask(SIGUSR1)|sigmask(SIGUSR2)|sigmask(SIGTERM)|sigmask(SIGPIPE));

	/*
	 * Process any options.
	 */
	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		cp = &argv[0][1];
		switch (*cp) {

		case 'n':			/* node_name */
			n_m_opt--;
			memset(node_name, 0, sizeof(node_name));
			if (*++cp)
			    strcpy(node_name, cp);
			else if (argc > 1 && argv[1][0] != '-') {
			    strcpy(node_name, argv[1]);
			    argc--, argv++;
                        }
			break;

		case 'f':
			n_m_opt--;
			memset(pseudonym, 0, sizeof(pseudonym));
			if (*++cp)
			    strcpy(pseudonym, cp);
			else if (argc > 1 && argv[1][0] != '-') {
			    strcpy(pseudonym, argv[1]);
			    argc--, argv++;
                        }
			break;

		case 'b':
			board_port = TRUE;	/* => port !TCP_telnet */
			if (*++cp){
			    if ((*cp == 'x')||(*cp == 'X'))
				/*
				 * -bX or -bx from dpp
				 */
				board_port = FALSE;
			    else
				board = atoi(cp);
			}
			else if (argc > 1 && argv[1][0] != '-') {
			    board = atoi(argv[1]);
			    argc--, argv++;
                        }
			break;

		case 'p':
			if (*++cp){
			    if ((*cp != 'x')&&(*cp != 'X'))
				/*
				 * not -pX or -px from dpp
				 */
				port = atoi(cp);
			}
			else if (argc > 1 && argv[1][0] != '-') {
			    port = atoi(argv[1]);
			    argc--, argv++;
                        }
			break;

#ifdef DEBUG
		case 'd':
			if (*++cp)
			    deb_level = atoi(cp);
			else if (argc > 1 && argv[1][0] != '-') {
			    deb_level = atoi(argv[1]);
			    argc--, argv++;
                        }
			break;
#endif

		case 'c':
			pc_file_ok = TRUE;
			memset(pc_file_path, 0, sizeof(pc_file_path));
			if (*++cp)
			    strcpy(pc_file_path, cp);
			else if (argc > 1 && argv[1][0] != '-') {
			    strcpy(pc_file_path, argv[1]);
			    argc--, argv++;
                        }
			break;

		default:
			sprintf(msg, "ocd: Unknown option '%s' ignored\0",
                               cp);
			syslog(LOG_INFO, msg);
			log(msg);
			break;
		}
		argc--, argv++;
	}

	if (n_m_opt) usage();

#ifdef DEBUG
	/*
	 * see if we're to debug
	 */
	trace_on = (deb_level & TRACE);
	track_on = (deb_level & TRACK) >> 1;
	dumps_on = (deb_level & DUMPS) >> 2;
	ioctl_on = (deb_level & IOCTL) >> 3;
	if (trace_on) syslog(LOG_INFO,"trace_on");
	if (track_on) syslog(LOG_INFO,"track_on");
	if (dumps_on) syslog(LOG_INFO,"dumps_on");
	if (trace_on) fprintf(stderr,"=>ocd\n");

        if (dumps_on){
                sprintf(msg,"-n %s  -f %s  -b %d  -p %d  -d %x  \
-c %s\0",
                node_name, pseudonym, board, port, deb_level, pc_file_path);
		log(msg);
	}
#endif

	/*
	 * Parse pc_file
	 */
	if (pc_file_ok)
		parse_pc_file();
	else
		syslog(LOG_WARNING,"pcf undefined, using default values");

	/*
	 * Verify parameter values
	 */

	if( (!telnet_mode) && timing_mark ){
	    timing_mark = FALSE;
	    sprintf(msg,"timing_mark disabled because telnet_mode disabled\0");
	    syslog(LOG_ERR,msg);
#ifdef DEBUG
	    if(track_on) log(msg);
#endif
	}
	utmpname("/etc/utmp.dfa");
	if (pseudonym_available() == FALSE) exit(1);

	if( board != -1 ){
		if((board < MIN_BOARD) || (board > MAX_BOARD)){
			sprintf(msg,"Bad board number: %d\0",board);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
			exit(1);
		}
		if( (port < MIN_PORT) || (port > MAX_PORT)){
			sprintf(msg,"Bad port number: %d\0",port);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
			exit(1);
		}
	}
	/*
	 * set up socket ... first translate node_name to INET address
	 */
	if ((sin.sin_addr.s_addr=inet_addr(node_name)) != (unsigned long) -1)
		sin.sin_family = AF_INET;
	else{
		if ((hp = gethostbyname(node_name)) != NULL){
#ifdef DEBUG
			if (track_on)
				sprintf(msg,"gethostbyname OK %s => %x\0",
					node_name,hp->h_addr);
#endif
			sin.sin_family = hp->h_addrtype;
			memcpy((caddr_t)&sin.sin_addr,hp->h_addr,hp->h_length);
		}
		else{
			sprintf(msg,"Unknown host %s: %d-%%m\0", node_name,errno);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
			exit(1);
		}
	}
	/*
	 * Now translate board/port to tcp_service_port
	 */
#if !defined(htons)
	u_short htons();
#endif  /* !defined(htons) */
	if(board == -1)				/* Board number omitted =>  */
		sin.sin_port = htons((int)port);/* port = tcp_service_port  */
	else{
/*??? verify that ok to use 0x17 direct or should servbyname "telnet" "tcp" */
	    sin.sin_port = htons(IPPORT_TELNET | (board << 13) | ((port+1)<<8));
	}
	sprintf(msg, "Node name: %s, Board %d, Port %d, TCP port %x\0",
		node_name,board,port,sin.sin_port);
	syslog(LOG_INFO, msg);
#ifdef DEBUG
	if (track_on) log(msg);
#endif
	getit();
        dfa_account(USER_PROCESS,my_pid,p_name,"",node_name,&sin);
	telnet();
}


char master[MAXPATHLEN], slave[MAXPATHLEN];

/*
 * Get a pty, scan input lines.
 */
getit()
{
	mode_t mode;
	struct stat  statbuf;


	/*
	 * get a pty pair
	 */

#if DEBUG
	if (trace_on) fprintf(stderr,"=>getit\n");
#endif
	if (getpty(&p, master, slave) != 0) {
		sprintf(msg,"FATAL EROR - Cannot allocate pty\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		exit(1);
        }

#ifdef DEBUG
	if (track_on){
		sprintf(msg,"getpty OK\0");
		log(msg);
	}
#endif
	if ( stat (slave, &statbuf) != 0){
		sprintf(msg,"FATAL ERROR - Cannot stat %s: %d-%%m\0",slave, errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		chmod(master,0666);
		chown(master, 0, 0);
		chmod(slave,0666);
		chown(slave, 0, 0);
		exit(1);
	}
	slave_rdev = statbuf.st_rdev;
	if ( mknod(pseudonym, S_IFCHR, statbuf.st_rdev) != 0 ){
		sprintf(msg,"FATAL ERROR - Cannot mknod %s: %d-%%m\0",pseudonym,errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		chmod(master,0666);
		chown(master, 0, 0);
		chmod(slave,0666);
		chown(slave, 0, 0);
		exit(1);
	}
#ifdef DEBUG
	if (track_on){
		sprintf(msg,"mknod %s OK\0",pseudonym);
		log(msg);
	}
#endif
	sprintf(msg,"dev:%d pty master:%s slave:%s<=>%s\0",
		p,master,slave,pseudonym);
	syslog(LOG_INFO, msg);
#ifdef DEBUG
	if (track_on) log(msg);
#endif
	/*
	 * Change access permissions for pseudonym side of device file
	 */
	mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	if(chmod(pseudonym,mode) != 0){
		sprintf(msg,"Cannot chmod of %s: %d-%%m\n",pseudonym,errno);
		syslog(LOG_WARNING, msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
	}

	/*
	 * Enable open/close exception traps
	 */
	/*
	 * Enable non-termio(7) ioctl(2) request trapping
	 * Enable termio(7) ioctl(2) request trapping
	 */
	(void)ioctl(p, TIOCTTY, (caddr_t)&on);
	(void)ioctl(p, TIOCTRAP, (caddr_t)&on);
	(void)ioctl(p, TIOCMONITOR, (caddr_t)&on);

	/*
	 * Enable packet mode, if defined
	 */
	if (ioctl(p, TIOCPKT, (caddr_t)&on) == -1){
#ifdef DEBUG
		if(track_on)fprintf(stderr,
			"ioctl(p, TIOCPKT, &on) failed. errno: %d\n",errno);
#endif
	}
	if (ioctl(p, FIONBIO, (caddr_t)&on) == -1){
#ifdef DEBUG
		if(track_on)fprintf(stderr,
			"ioctl(p, FIONBIO, &on) failed. errno: %d\n",errno);
#endif
	}

	pty = p;
#if DEBUG
	if (trace_on) fprintf(stderr,"<=getit\n");
#endif
}


/*
 * Main loop.  Select from pty and network, and
 * hand data to telnet receiver finite state machine.
 */
telnet()
{
	int toggle_binary();
	static char brkbuf[] = { IAC, BREAK};
	int ret_code;
	void cleanup();
	int send_TM();
	struct termio b;
#ifndef NCCS
#	define NCCS 16
#endif
#	define VSTART 14
#	define VSTOP 15
/*	typedef unsigned int tcflag_t;
 *	typedef unsigned char cc_t;
 */
	struct termios {
        tcflag_t        c_iflag;        /* Input modes */
        tcflag_t        c_oflag;        /* Output modes */
        tcflag_t        c_cflag;        /* Control modes */
        tcflag_t        c_lflag;        /* Local modes */
        tcflag_t        c_reserved;     /* Reserved for future use */
        cc_t            c_cc[NCCS];             /* Control characters */
	};
	struct termios bs;
	int io_arg = 0;
	int i;
	int f;
	tcflag_t bix;
	static struct timeval main_to;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>telnet\n");
#endif
	/*
	 * net is not open yet, so we don't want to select on it.
	 * Put it equal to pty+1.
	 * To be improved on later.
	 */
	net = pty+1;
	f = net;

	for (;;) {
		fd_set ibits, obits, xbits;
		register int c;

		if (ncc < 0 && pcc < 0){
			/*
			 * Catastrophic but keep on running
			 */
			closeup();
			continue;
		}

		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		FD_ZERO(&xbits);
		/*
		 * Never look for input if there's still
		 * stuff in the corresponding output buffer
		 */
		if(cnxn_open){
		    if (nfrontp - nbackp || pcc > 0)
			FD_SET(f, &obits);
		    else
			FD_SET(p, &ibits);
		    if (pfrontp - pbackp || ncc > 0)
			FD_SET(p, &obits);
		    else
			FD_SET(f, &ibits);
		}

			FD_SET(p, &xbits);
		if (!SYNCHing) {
			FD_SET(f, &xbits);
		}
		if (!cnxn_open)
			FD_CLR(f, &xbits);

#ifdef DEBUG
	 	if (dumps_on){
			fprintf(stderr,"before main select:\t\
p:%x  f:%x  ibits:%x  obits:%x  xbits:%x\n", p, f,
			ibits.fds_bits[0],obits.fds_bits[0], xbits.fds_bits[0]);
			fprintf(stderr, "Buffer pointers before select:\n\
ptyibuf:%x  ptyobuf:%x  ptyip:%x  pfrontp:%x  pbackp:%x\n\
netibuf:%x  netobuf:%x  netip:%x  nfrontp:%x  nbackp:%x\n\
ncc:%d pcc:%d\n",
ptyibuf,ptyobuf,ptyip,pfrontp,pbackp,netibuf,netobuf,netip,nfrontp,nbackp,ncc,pcc);
			sprintf(msg,"CPU time used: %d %ds\n",clock(),clock()/CLOCKS_PER_SEC);
			log(msg);
		}
#endif
		/*
		 * set up main select time-out values
		 */
		main_to.tv_sec = 30;
		main_to.tv_usec = 0;
		/*
		 * pty always has a lower fd than the socket since pty was
		 * opened first, so select up to the socket fd
		 */
		c = select(net+1,&ibits,&obits,&xbits,&main_to);
#ifdef DEBUG
	 	if (dumps_on){
			sprintf(msg,"main select returns:\t\
nb_fd:%d  ibits:%x  obits:%x  xbits:%x  errno:%d\0",
		c,ibits.fds_bits[0],obits.fds_bits[0], xbits.fds_bits[0],errno);
			log(msg);
      			sprintf(msg,"CPU time used: %d %ds\n",clock(),clock()/CLOCKS_PER_SEC);
			log(msg);
		}
#endif
		if (c == 0) {
		/*
		 * select timed-out. Take the opportunity to check if the
		 * pseudonym still exists
		 */
			if ( check_pseudonym() == -1 ){
				sprintf(msg,"FATAL ERROR - pseudonym no longer valid\0");
				syslog(LOG_ERR, msg);
#ifdef DEBUG
				if (trace_on) log(msg);
#endif
				if(cnxn_open){
					sprintf(msg,"Shutting down network cnxn\0");
					syslog(LOG_INFO,msg);
#ifdef DEBUG
					if (track_on) log(msg);
#endif
					shutdown(net, 2);
					close(net);
					cnxn_open = FALSE;
				}
			chmod(master,0666);
			chown(master, 0, 0);
			chmod(slave,0666);
		 	chown(slave, 0, 0);
                        dfa_account(DEAD_PROCESS,my_pid,"","",NULL,NULL);
			exit(1);
			}
			/*
			 * pseudonym is OK. Check that we didn't get a close
			 * alarm call between times
			 */
			if (time_out){
				time_out = FALSE;
				closedown();
			}
			continue;
		}

		if (c == -1) {
			if (errno == EINTR) {
				if(time_out){
					time_out = FALSE;
					closedown();
				}
				continue;
			}
			sprintf(msg, "select failed: %d-%%m\0",errno);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
			continue;
		}

		alarm(0);		/* cancel close alarm */

		/*
		 * Any urgent data?
		 */
		if (FD_ISSET(net, &xbits)) {
#ifdef DEBUG
			if(track_on){
			    sprintf(msg, "net exception\0");
			    log(msg);
			}
#endif
		    SYNCHing = 1;
		}

		/*
		 * Something to read from the network...
		 */
		if (FD_ISSET(net, &ibits)) {
#if	!defined(SO_OOBINLINE)
			/*
			 * In 4.2 (and 4.3 beta) systems, the
			 * OOB indication and data handling in the kernel
			 * is such that if two separate TCP Urgent requests
			 * come in, one byte of TCP data will be overlaid.
			 * This is fatal for Telnet, but we try to live
			 * with it.
			 *
			 * In addition, in 4.2 (and...), a special protocol
			 * is needed to pick up the TCP Urgent data in
			 * the correct sequence.
			 *
			 * What we do is:  if we think we are in urgent
			 * mode, we look to see if we are "at the mark".
			 * If we are, we do an OOB receive.  If we run
			 * this twice, we will do the OOB receive twice,
			 * but the second will fail, since the second
			 * time we were "at the mark", but there wasn't
			 * any data there (the kernel doesn't reset
			 * "at the mark" until we do a normal read).
			 * Once we've read the OOB data, we go ahead
			 * and do normal reads.
			 *
			 * There is also another problem, which is that
			 * since the OOB byte we read doesn't put us
			 * out of OOB state, and since that byte is most
			 * likely the TELNET DM (data mark), we would
			 * stay in the TELNET SYNCH (SYNCHing) state.
			 * So, clocks to the rescue.  If we've "just"
			 * received a DM, then we test for the
			 * presence of OOB data when the receive OOB
			 * fails (and AFTER we did the normal mode read
			 * to clear "at the mark").
			 */
		    if (SYNCHing) {
			int atmark;

			ioctl(net, SIOCATMARK, (char *)&atmark);
			if (atmark) {
			    ncc = recv(net, netibuf, sizeof (netibuf), MSG_OOB);
#ifndef hpux
			    if ((ncc == -1) && (errno == EINVAL)) {
				ncc = read(net, netibuf, sizeof (netibuf));
				if (sequenceIs(didnetreceive, gotDM)) {
				    SYNCHing = stilloob(net);
				}
			    }
#endif /* ~hpux */
			} else {
			    ncc = read(net, netibuf, sizeof (netibuf));
			}
		    } else {
			ncc = read(net, netibuf, sizeof (netibuf));
		    }
		    settimer(didnetreceive);
#else	/* !defined(SO_OOBINLINE)) */
		    ncc = read(net, netibuf, sizeof (netibuf));
#endif	/* !defined(SO_OOBINLINE)) */
		    if (ncc < 0 && errno == EWOULDBLOCK)
			ncc = 0;
		    else {
			if (ncc <= 0){
			/*
			 * Catastrophic ... but keep going
			 */
				closeup();
				continue;
			}
			netip = netibuf;
		    }
#ifdef DEBUG
		    if(track_on){
			sprintf(msg,"Got %d bytes from net\0",ncc);
			log(msg);
		    }
		    if(dumps_on)Dump(netibuf,ncc,1);
#endif
		}

		/*
		 * Something to read from the pty
		 */
		if (FD_ISSET(p, &ibits)) {
			pcc = read(p, ptyibuf, BUFSIZ);
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else {
				if (pcc <= 0){
					sprintf(msg,"pty read failed: %d-%%m\0",errno);
					syslog(LOG_ERR,msg);
#ifdef DEBUG
					if(track_on)log(msg);
					if(dumps_on)
					 fprintf("p=%d  ptyibuf=%x\n",p,ptyibuf);
#endif
					closeup();
					continue;
				}
				/* Skip past "packet" */
				pcc--;
				ptyip = ptyibuf+1;
			}
#ifdef DEBUG
			if(track_on){
		    		sprintf(msg,"Got %d bytes from pty\0",pcc);
		    		log(msg);
			}
			if(dumps_on)Dump(ptyibuf,pcc+1,0);   /* + TIOCPKT byte */
#endif
		}
		if(telnet_mode){
			if ((ptyibuf[0] & TIOCPKT_FLUSHWRITE) && pcc > 0) {
				netclear();	/* clear buffer back */
				*nfrontp++ = IAC;
				*nfrontp++ = DM;
				neturg = nfrontp-1;  /* off by one XXX */
				ptyibuf[0] = 0;
			}

			while (pcc > 0) {
				if ((&netobuf[BUFSIZ] - nfrontp) < 2)
					break;
				c = *ptyip++ & 0377, pcc--;
				if (c == IAC)
					*nfrontp++ = c;
				*nfrontp++ = c;
				/* Don't do CR-NUL if we are in binary mode */
				if ((c=='\r')&&(myopts[TELOPT_BINARY]==OPT_NO)){
					if (pcc>0 && ((*ptyip & 0377)=='\n')) {
						*nfrontp++ = *ptyip++ & 0377;
						pcc--;
					} else
						*nfrontp++ = '\0';
				}
			}
		}else{
			while (pcc > 0) {
				if ((&netobuf[BUFSIZ] - nfrontp) == 0)
					break;
				*nfrontp++ = *ptyip++ & 0377, pcc--;
			}
		}
		if (FD_ISSET(f, &obits) && (nfrontp - nbackp) > 0){
#ifdef DEBUG
			if(track_on){
			    sprintf(msg,
				"Sending %d bytes to net\0",nfrontp-nbackp);
			    log(msg);
			}
			if(dumps_on)Dump(nbackp,nfrontp-nbackp,1);
#endif
			netflush();
		}
		if (ncc > 0){
			if (telnet_mode) 
			   telrcv();
			else{
				while (ncc > 0){
					if( (&ptyobuf[BUFSIZ] - pfrontp) == 0 )
						break;
					*pfrontp++ = *netip++ & 0377, ncc--;
				}
			}
		}
		if (FD_ISSET(p, &obits) && (pfrontp - pbackp) > 0){
#ifdef DEBUG
			if(track_on){
			    sprintf(msg,
				"Sending %d bytes to pty\0",pfrontp-pbackp);
			    log(msg);
			}
			if(dumps_on)Dump(pbackp,pfrontp-pbackp,0);
#endif
			ptyflush();
		}

		/*
		 * pty exception ... treat the open, close or ioctl
		 */
		if (FD_ISSET(p, &xbits)) {
			/*
			 * Analyse exception
			 */
#ifdef DEBUG
			if(track_on){
			    sprintf(msg, "pty exception\0");
			    log(msg);
			}
#endif
			if((ioctl(p, TIOCREQCHECK, (caddr_t)&req_info))==-1){
				if (errno==EINVAL)
					continue;
				else{
			      	 sprintf(msg,"pty TIOCREQCHECK failed: %d-%%m\0",errno);
				 syslog(LOG_ERR,msg);
#ifdef DEBUG
				 if(track_on) log(msg);
#endif
				 closeup();
				 continue;
				}
			}
			/*
			 * decortiquer the ioctl request
			 */
			r_val = req_info.request&0xff;
			r_who =  (req_info.request&0xff00)>>8 ;
#ifdef DEBUG
			if(track_on){
				sprintf(msg, "pty x ioctl OK\0");
				log(msg);
			}
		        if (dumps_on|ioctl_on){
			 fprintf(stderr,"pty ioctl returns:\n\
\trequest: %x => ",req_info.request);
			 if (IOC_VOID&req_info.request)
			  fprintf(stderr, "_IO('%c', %d)", r_who, r_val);
			 else
			  if (IOC_OUT&req_info.request)
			   fprintf(stderr,"_IOR('%c', %d, arg)",r_who,r_val);
			  else
			   if (IOC_IN&req_info.request)
			    fprintf(stderr,"_IOW('%c', %d, arg)",r_who,r_val);
			   else
			    if (IOC_INOUT&req_info.request)
			     fprintf(stderr,"_IOWR('%c', %d, arg)",r_who,r_val);
			    else fprintf(stderr,"_IO?('%c', %d)",r_who,r_val);
			 fprintf(stderr,"\n\targget:%x\n\
\targset:%x\n\
\tpgrp:%d\n\
\tpid:%d\n\
\terrno_error:%d\n\
\treturn_value:%d\n",
				req_info.argget,
				req_info.argset,req_info.pgrp,req_info.pid,
				req_info.errno_error,req_info.return_value);
			}
#endif
			if ( req_info.request == TIOCOPEN ) {
			/*
			 * Treat TIOCOPEN
			 */
			    if(!cnxn_open){
				sprintf(msg,
			"pty exception => open on slave by [%d]\0",req_info.pid);
				syslog(LOG_INFO,msg);
#ifdef DEBUG
				if(track_on) log(msg);
#endif

				/*
				 * re-initialize I/O data buffer pointers,
				 * and counters.
				 */
#ifdef DEBUG
				if(dumps_on)fprintf(stderr,
"Buffer pointers before connect:\n\
ptyibuf:%x  ptyobuf:%x  ptyip:%x  pfrontp:%x  pbackp:%x\n\
netibuf:%x  netobuf:%x  netip:%x  nfrontp:%x  nbackp:%x\n\
ncc:%d pcc:%d\n",
ptyibuf,ptyobuf,ptyip,pfrontp,pbackp,netibuf,netobuf,netip,nfrontp,nbackp,ncc,pcc);
#endif
				ptyip = ptyibuf;
				netip = netibuf;

/*				pfrontp = ptyobuf;
 *				pbackp = ptyobuf;
 *				nfrontp = netobuf;
 *				nbackp = netobuf;
 */
				neturg = (char *) 0;
				not42 = 1;

		/* pointers for sub-options */
				subpointer= subbuffer;
				subend= subbuffer;

/*				pcc = 0;
 *				ncc = 0;
 */
				for ( i=0;i<256;i++ ) {
					hisopts[i] = OPT_NO;
					myopts[i]  = OPT_NO;
				}

				(void) ioctl(p, TCGETA, &b);
#ifdef DEBUG
				if(dumps_on|ioctl_on){
					sprintf(msg,
"original pty termio structure :\n\
c_iflag:%x c_oflag:%x c_cflag:%x c_lflag:%x\n",
b.c_iflag, b.c_oflag, b.c_cflag, b.c_lflag);
					log(msg);
					for (i=0;i<NCC;i++)
						fprintf(stderr,"%x ",b.c_cc[i]);
					fprintf(stderr,"\n");
					fflush(stderr);
				}
#endif
				b.c_oflag = OPOST|ONLCR|TAB3;
				b.c_iflag = BRKINT|IGNPAR|ISTRIP|ICRNL|IXON;
				b.c_lflag = ISIG|ICANON;

				/*
				** Let the administrator violate the telnet RFC
				** and initialize 8-bit settings.
				*/
				if (eightbit) {
					b.c_iflag &= ~ISTRIP;
					b.c_cflag &= ~PARENB;
					b.c_cflag |= CS8;

				} else {
 				b.c_iflag |= ISTRIP;
					b.c_cflag |= CS7 | PARENB;
				}

				(void) ioctl(p, TCSETA, &b);
				(void) ioctl(p, TCGETA, &b);

#ifdef DEBUG
				if(dumps_on|ioctl_on){
					sprintf(msg,
"pty termio structure reset to:\n\
c_iflag:%x c_oflag:%x c_cflag:%x c_lflag:%x\n",
b.c_iflag, b.c_oflag, b.c_cflag, b.c_lflag);
					log(msg);
					for (i=0;i<NCC;i++)
						fprintf(stderr,"%x ",b.c_cc[i]);
					fprintf(stderr,"\n");
					fflush(stderr);
				}
#endif
				/*
				 * handshake the open here, even though we're
				 * not sure if the net connection is going to
				 * work. We have to be quick ,otherwise the lp
				 * spooler will disable the printer.
				 */
				if (handshake(p, req_info, 0) <0){
					/*
					 * slave gave up
					 */
#ifdef DEBUG
					if (track_on){
					  sprintf(msg,"slave cancelled open\0");
					  log(msg);
					}
#endif
					continue;
				}
				f = open_cnxn ( sin,open_tries,open_timer );
				if (f>=0){
					/* Success, go and send SR and TM if
					 * enabled. Shake up pty if things go
					 * wrong.
					 * Set cnxn_open flag here. It will
					 * be cleared if SR or TM or handshake
					 * fails.
					 */
					cnxn_open = TRUE;
					net = f;

					ret_code = ioctl(f, FIONBIO, &on);
					if(ret_code == -1){
					    sprintf(msg,
					    "ioctl(f, FIONBIO, &on) failed.\
 errno: %d-%%m\n,errno");
					    syslog(LOG_WARNING,msg);
#ifdef DEBUG
					    if(track_on)log(msg);
#endif
					}
					if(telnet_mode){
					    /*
					     * Request to do remote echo and
					     * to suppress go ahead.
					     * send WILL_SGA DO_SGA WILL_ECHO
					     */
		   			    dooption(TELOPT_SGA);
					    willoption(TELOPT_SGA);
					    dooption(TELOPT_ECHO);
					    
					    /*
					     * Optionally enable binary
					     */
					    if ( binary_mode ){
						/*
						 * binary mode desired
						 */
						if(!toggle_binary(on)){
                                            		sprintf(msg,
"BIN_NEG failed => open failed\0");
                                            		syslog(LOG_WARNING, msg);
#ifdef DEBUG
                                            		if (track_on) log(msg);
#endif
                                            		closeup();
                                            		continue;
                                        	}

					    }else netflush();
					}

					/* Clear ptybuf[0] - where the packet
					 * information is received
					 */
					ptyibuf[0] = 0;

					/*
					 * clean out channels
					 * !!! watch out for garbage
					 * !!! specially when !telnet_mode
					 */ 
					netflush();
					ptyflush();

					/*
					 * do a select to see if we received
					 * any replies to the telnet 
					 * negotiations
					 */


					/*
					 * Optionally do status request
					 */
					if (status_request) 
					  {
					   if (telnet_mode)
					      /*
					       * Check for receipt of any
					       * telnet negotiations.  We
					       * check it here because if
					       * we don't it will interfere
					       * with the status req check
                                               * But, we need to make sure
                                               * it's a new connection or
                                               * it will also interfere
                                               * with the status request
					       */
					      check_tel_neg_reply();
					   ret_val = do_status_req();
					  }
					else 
					   ret_val = 0;
					/*
					 * check status reply OK
					 */
					if (ret_val != 0){
					    sprintf(msg, "SR failed => closing/re-opening pty master\0");
					    syslog(LOG_WARNING, msg);
#ifdef DEBUG
					    if (track_on) log(msg);
#endif
					    closeup();
					    continue;
					}
				} else {
					sprintf(msg,"Connection failed. Closing/re-opening pty master\0");
					syslog(LOG_WARNING,msg);
#ifdef DEBUG
					if(track_on) log(msg);
#endif
                                        sprintf(reason_msg, "Error connecting to remote printer.\n");
                                        disable_printer();
					shakeup_pty();
					/*
					 * f is now invalid but we put
					 * it equal to it's old value
					 * so as not to balls up the select
					 */
					f = net;
					continue;	/* stay in loop */
				}
			    }else{
#ifdef DEBUG
				sprintf(msg,"Connection already open ... OK\0");
				if(track_on) log(msg);
#endif
				handshake(p, req_info, 0);
				continue;
			    }
			} /* END TIOOPEN */
			else{

			 if (req_info.request == TIOCCLOSE){
			 /*
			  * Treat TIOCCLOSE
			  */
#ifdef DEBUG
			  if(track_on){
				sprintf(msg,
				 "pty exception => close on slave\0");
				log(msg);
			  }

			  if(dumps_on)fprintf(stderr,
"Buffer pointers on close:\n\
ptyibuf:%x  ptyobuf:%x  ptyip:%x  pfrontp:%x  pbackp:%x\n\
netibuf:%x  netobuf:%x  netip:%x  nfrontp:%x  nbackp:%x\n\
ncc:%d pcc:%d\n",
ptyibuf,ptyobuf,ptyip,pfrontp,pbackp,netibuf,netobuf,netip,nfrontp,nbackp,ncc,pcc);
#endif
			  if(cnxn_open){
			     if( (!FD_ISSET(net,&obits)) &&
				 (!FD_ISSET(p,&ibits))){

			        /*
                                 * A2 - 
                                 * Removed status checking.  The status
                                 * checking was disabling printers.
                                 *
				 */
				 handshake(p, req_info, 0);
				 if (close_timer == 0){
				    /*
				     * Close lan cnxn immediately
				     */
				     closedown();
				 }else{
				    /* or set up cnxn close alarm timer
				     *
				     */
				    signal(SIGALRM, alarm_call);
				    alarm(close_timer);
				 }
				 continue;
			      }else{
				 /*
				  * still stuff to write ... pass thru
				  */
			 	 sprintf(msg,"Close retarded because still I/O\0");
#ifdef DEBUG
				 if(track_on)log(msg);
#endif
			      }
			  } else {
			 	sprintf(msg,"Close ignored because cnxn not open\0");
				syslog(LOG_WARNING,msg);
#ifdef DEBUG
				if(track_on)log(msg);
#endif
				handshake(p, req_info, 0);
				continue;
			  }
			}	/* END TIOCCLOSE */

			/*
			 * treat TIOIOCTL
			 */
			else {
#ifdef DEBUG
			    if(track_on){
				sprintf(msg, "pty exception =>  slave ioctl\0");
				log(msg);
			    }
#endif
			    if(!cnxn_open){
#ifdef DEBUG
				fprintf(stderr,
				"ioctl rejected because cnxn closed. Closing/re-opening pty master\0");
				if(track_on)log(msg);
#endif
				shakeup_pty();
				handshake(p, req_info, ENOTCONN);

			    }else{
				if(req_info.argget != 0){
				/*
				 * check if termio ioctl
				 */
				if ( r_who == 'T' ){

				 /*
				  * Get the arg passed by the ioctl.
				  * If it's a termios, use 
				  * our personal termios structure. This
				  * is horrific.
				  * If we want to modify the termio struct,
				  * pass the structure back like this
				  * being careful to verify if termio or termios
				  */
				 /* if(ioctl(p, req_info.argset, &b) != 0){
				  *  sprintf(msg,"ioctl(argset) failed: %d-%%m\0,errno"); 
				  *  syslog(LOG_WARNING,msg);
				  * }
				  */
				    switch ( r_val ){
				    case 1:   /* TCGETA */
					ret_val = 0;
					break;

				    case 4:	/* TCSETAF */ 
					/*
					 * put flush code here
					 */

				    case 3:	/* TCSETAW */
				 	if(FD_ISSET(p, &ibits)){
#ifdef DEBUG
					    if(track_on){
						sprintf(msg,
						"Flushing out net\0");
						log(msg);
					    }
#endif
					    /*
					     * put flush code here
					     */
				 	}

                                        /********************************** 
                                         *
                                         * A4 - Detect stty EXTA coming from
                                         *      the interface script.  This
                                         *      is a workaround to try and
                                         *      delay the spooler so that we
                                         *      can process status checking
                                         *      and timing mark.  This will
                                         *      prevent the printer from
                                         *      being disabled while we check
                                         *      for timing mark and status.
                                         *
                                         ***********************************/

                                        (void) ioctl(p, req_info.argget, &b);
                                        if ((b.c_cflag & CBAUD) == EXTA) {

                                                /*****************************
                                                 *
                                                 * A5 - Handle second status
                                                 *      request checking here
                                                 *      instead of in
                                                 *      TIOCCLOSE.
                                                 *
                                                 ****************************/
                                                ret_val = 0;
                                                if (status_request)
                                                        ret_val = do_status_req();
                                                if (ret_val != 0) {
                                                        sprintf(msg, "Closing/re-opening pty\0");
                                                        syslog(LOG_INFO, msg);
#ifdef DEBUG
                                                        if (track_on) log(msg);
#endif
                                                        closeup();
                                                        continue;
                                                }

                                                /****************************
                                                 *
                                                 * A6 -
                                                 * Send timing mark here.  It
                                                 * will be sent after each
                                                 * file.  It's a drawback to
                                                 * doing it only before the
                                                 * connection goes down but
                                                 * it's better than the printer
                                                 * being disabled.
                                                 *
                                                 * No need to send timing mark
                                                 * if status request is enabled
                                                 * since they are virtually
                                                 * serving the same purpose.
                                                 * If a reply to the status
                                                 * request is received that
                                                 * means the DTC port buffer
                                                 * has been emptied, which is
                                                 * the purpose of timing mark.
                                                 *
                                                 *****************************/
                                                if (timing_mark == TRUE) {
                                                /*   && (!status_request)   */
                                                        if( send_TM() < 0 )
                                                                shakeup_pty();
                                                        TM_sent = TRUE;
                                                }
                                        }
                                        break;

				    case 2:	/* TCSETA */
				 if (ioctl(p, req_info.argget, &b) != 0){
				   sprintf(msg,"ioctl(argget) failed: %d-%%m\0",errno); 
				   syslog(LOG_WARNING,msg);
#ifdef DEBUG
				   if(track_on)log(msg);
#endif
				   ret_val = EINVAL;
				   break;
				 }
#ifdef DEBUG
				 if(dumps_on|ioctl_on){
				  fprintf(stderr,"ioctl(argget) returns\n\
iflag:%x  oflag:%x  cflag:%x  lflag:%x\n",
b.c_iflag, b.c_oflag, b.c_cflag, b.c_lflag);
					for (i=0;i<NCC;i++) fprintf(stderr,"%x ",b.c_cc[i]);
					fprintf(stderr,"\n");
					fflush(stderr);
				 }
#endif
			    if(telnet_mode){
#ifdef DEBUG
				sprintf(msg, "Checking binary state\0");
			   	if(track_on)log(msg);
#endif
				/*
				 * see if binary mode has been enabled on the
				 * pty slave-side. Toggle unless binary is
				 * permanently on, in which case it will have
				 * been enabled long ago.
				 */
				if (!binary_mode){
				    /*
				     * TELNET Binary mode will have to be
				     * enabled if the DTC special chars are to
				     * be ignored i.e. XON/XOFF and ^K
				     * OK for ^K which is always ignored for
				     * an outgoing cnxn.
				     */

				    /* bi =  b.c_iflag & (BRKINT|ISTRIP|INLCR|ICRNL|IUCLC);
				     * bo =  b.c_oflag & (OPOST);
				     * bl =  b.c_lflag & (ISIG|ICANON|ECHO);
				     */
				    bix = b.c_iflag & (IXON);
				    if (bix){
	
				    /* if (( bix )&&( bi | bo | bl )){
				     */
#ifdef DEBUG
			 		if(track_on){
					    sprintf(msg,"toggle_binary(off) if necessary\0");
					    log(msg);
					}
#endif
					    if(!toggle_binary(off)){
						closeup();
						ret_val = ENOPROTOOPT;
						break;
                                       	    }
				    }else{
#ifdef DEBUG
					if(track_on){
					    sprintf(msg,"toggle_binary(on) if necessary\0");
					    log(msg);
					}
#endif
					    if(!toggle_binary(on)){
						closeup();
						ret_val = ENOPROTOOPT;
						break;
                                       	    }
				    } /* endif flags */
				}else{
				    sprintf(msg,"Binary mode permanently on\0");
				    syslog(LOG_INFO,msg);
#ifdef DEBUG
				    if(track_on)log(msg);
#endif
				}
			    }	/* if (telnet_mode) */
				  ret_val = 0;
				  break;

				    case 5:	/* TCSBRK */
#ifdef DEBUG
				   sprintf( msg, "ioctl request: TCSBRK\0" );
				   if ( track_on ) log( msg );
#endif
                                        /* IND 1.1.109.11 5000-699355 */
                                        if ( req_info.argget != 0 )
                                                if ( ioctl( p, req_info.argget, &io_arg ) == 0 )
                                                        if ( io_arg == 0 ) {
#ifdef DEBUG
                                   sprintf( msg, "ioctl arg: %d\0", io_arg );
				   if ( track_on ) log( msg );
                                   sprintf( msg, "Sending telnet break\0" );
				   if ( track_on ) log( msg );
#endif
			 		bcopy(brkbuf, nfrontp, sizeof brkbuf);
					nfrontp += sizeof brkbuf;
                                                        }
                                                        else {
#ifdef DEBUG
                                   sprintf( msg, "ioctl arg: %d\0", io_arg );
				   if ( track_on ) log( msg );
                                   sprintf( msg, "Not sending telnet break\0" );
				   if ( track_on ) log( msg );
#endif
                                                         }
                                                 else {
                                   sprintf( msg, "ioctl request: argget failed: %d-%%m\0", errno );
                                   syslog( LOG_WARNING, msg );
#ifdef DEBUG
                                   if ( track_on ) log( msg );
#endif
                                   ret_val = EINVAL;
                                   break;
                                                 }
                                         else {
#ifdef DEBUG
                                   sprintf( msg, "ioctl request: argget = %d\0", req_info.argget );
                                   if ( track_on ) log( msg );
#endif
                                        }
					netflush();
					ret_val = 0;
					break;

				    case 6:	/* TCXONC */

				 if (ioctl(p, req_info.argget, io_arg) != 0){
				   sprintf(msg,"ioctl(argget) failed: %d-%%m\0",errno); 
				   syslog(LOG_WARNING,msg);
#ifdef DEBUG
				   if(track_on) log(msg);
#endif
				   ret_val = EINVAL;
				   break;
				 }
				 if (io_arg == 2)
                                   {
			 	    bcopy(bs.c_cc[VSTOP], nfrontp, 1);
				    nfrontp ++;
#ifdef DEBUG
				    sprintf(msg, "ioctl=TCXONC(%d).\
 Sending VSTOP: %c\0",io_arg,bs.c_cc[VSTOP]);
				    if(track_on) log(msg);
#endif
				    netflush();
				   }
                                 else
                                   {
				    if (io_arg == 3) 
                                      {
			    	       bcopy(bs.c_cc[VSTART], nfrontp, 1);
				       nfrontp ++;
#ifdef DEBUG
				       sprintf(msg,"ioctl=TXCONC(%d).\
 Sending VSTART: %c\0",io_arg,bs.c_cc[VSTART]);
					if(track_on) log(msg);
#endif
					netflush();
				       }
		 		    }
					ret_val = 0;
					break;

				    case 7:	/* TCFLSH */
					ret_val = EINVAL;
					break;
					/*
					 * the following is bad
					 */

/*                              if (ioctl(p, req_info.argget, io_arg) != 0)
 *                                {
 *                                 sprintf(msg,"ioctl(argget) failed: %d-%%m\0",errno); 
 *                                 syslog(LOG_WARNING,msg);
 * #ifdef DEBUG
 *                                 if(track_on)log(msg);
 * #endif
 *                                 ret_val = EINVAL;
 *                                 break;
 *                                }
 *                              if ( io_arg == 0 )
 *                                {
 * #ifdef DEBUG
 *                                 sprintf(msg, "ioctl=TCFLSH(%d).\
 * Flushing input\0",io_arg);
 *                                 if(track_on) log(msg);
 * #endif
 *                                       *
 *                                       * put flush code here
 *                                       * 
 *                                 }
 *                               if ( io_arg == 1 )
 *                                 {
 * #ifdef DEBUG
 *                                  sprintf(msg, "ioctl=TCFLSH(%d).\
 * Flushing output\0",io_arg);
 *                                  if(track_on) log(msg);
 * #endif
 *                                       *
 *                                       * put flush code here
 *                                       * 
 *                                  }
 *                                if ( io_arg == 2 )
 *                                  {
 * #ifdef DEBUG
 *                                   sprintf(msg, "ioctl=TCFLSH(%d).\
 * Flushing input then output\0",io_arg);
 *                                   if(track_on) log(msg);
 * #endif
 *                                       *
 *                                       * put flush code here
 *                                       * 
 *                                    }
 *                                 ret_val = 0;
 *                                 break;
 */

				    default:
#ifdef DEBUG
				      sprintf(msg,
				      "ioctl was TC<%d> ... ignored\0", r_val);
				      if(track_on) log(msg);
#endif
				      ret_val = 0;
				      break;

				    }  /*switch */
				    /*
				     * clean the following up!!!!
				     */
				    handshake(p, req_info, ret_val);
				} else handshake(p, req_info, 0);  /* if 'T' */
				} else handshake(p, req_info, 0);
			    }
			} /* END TIOIOCTL */
		    }
		} /* end pty exception */
	} /* for */
/*    cleanup();	 * emergency exit *
 * #ifdef DEBUG
 *    if (trace_on) fprintf(stderr,"<=telnet\n");
 * #endif
 */
}


/*
 * State for recv fsm
 */
#define	TS_DATA		0	/* base state */
#define	TS_IAC		1	/* look for double IAC's */
#define	TS_CR		2	/* CR-LF ->'s CR */
#define	TS_SB		3	/* throw away begin's... */
#define	TS_SE		4	/* ...end's (suboption negotiation) */
#define	TS_WILL		5	/* will option negotiation */
#define	TS_WONT		6	/* wont " */
#define	TS_DO		7	/* do " */
#define	TS_DONT		8	/* dont " */


static int state = TS_DATA;


telrcv()
{
	register int c;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>telrcv\n");
#endif
	while (ncc > 0) {
		if ((&ptyobuf[BUFSIZ] - pfrontp) < 2)
			return;
		c = *netip++ & 0377, ncc--;
		switch (state) {

		case TS_CR:
			state = TS_DATA;
			/* Strip off \n or \0 after a \r */
			if ((c == 0) || (c == '\n')) {
				break;
			}
			/* FALL THROUGH */

		case TS_DATA:
			if (c == IAC) {
				state = TS_IAC;
				break;
			}
			if (inter > 0)
				break;
			/*
			 * We now map \r\n ==> \r for pragmatic reasons.
			 * Many client implementations send \r\n when
			 * the user hits the CarriageReturn key.
			 *
			 * We USED to map \r\n ==> \n, since \r\n says
			 * that we want to be in column 1 of the next
			 * printable line, and \n is the standard
			 * unix way of saying that (\r is only good
			 * if CRMOD is set, which it normally is).
			 */
			if ((c == '\r') && (hisopts[TELOPT_BINARY] == OPT_NO)) {
				state = TS_CR;
			}
			*pfrontp++ = c;
			break;

		case TS_IAC:
			switch (c) {

			/*
			 * Send the process on the pty side an
			 * interrupt.  Do this with a NULL or
			 * interrupt char; depending on the tty mode.
			 */
			case IP:
				interrupt();
				break;

			case BREAK:
				sendbrk();
				break;

			/*
			 * Are You There?
			 */
			case AYT:
				strcpy(nfrontp, "\r\n[Yes]\r\n");
				nfrontp += 9;
				break;

			/*
			 * Abort Output
			 */
			case AO: {
#ifndef hpux
					struct ltchars tmpltc;
#endif
					ptyflush();	/* half-hearted */
#ifndef hpux
					ioctl(pty, TIOCGLTC, &tmpltc);
					if (tmpltc.t_flushc != '\377') {
						*pfrontp++ = tmpltc.t_flushc;
					}
#endif
					netclear();	/* clear buffer back */
					*nfrontp++ = IAC;
					*nfrontp++ = DM;
					neturg = nfrontp-1; /* off by one XXX */
					break;
				}

			/*
			 * Erase Character and
			 * Erase Line
			 */
			case EC:
			case EL: {
					struct termio b;
					int ch;

					ptyflush();	/* half-hearted */

					ioctl(pty, TCGETA, &b);
					ch = (c == EC) ?
						b.c_cc[VERASE] : b.c_cc[VKILL];
					if (ch != -1) {
						*pfrontp++ = ch;
					}
					break;
				}

			/*
			 * Check for urgent data...
			 */
			case DM:
				SYNCHing = stilloob(net);
				settimer(gotDM);
				break;


			/*
			 * Begin option subnegotiation...
			 */
			case SB:
				state = TS_SB;
				continue;

			case WILL:
				state = TS_WILL;
				continue;

			case WONT:
				state = TS_WONT;
				continue;

			case DO:
				state = TS_DO;
				continue;

			case DONT:
				state = TS_DONT;
				continue;

			case IAC:
				*pfrontp++ = c;
				break;
			}
			state = TS_DATA;
			break;

		case TS_SB:
			if (c == IAC) {
				state = TS_SE;
			} else {
				SB_ACCUM(c);
			}
			break;

		case TS_SE:
			if (c != SE) {
				if (c != IAC) {
					SB_ACCUM(IAC);
				}
				SB_ACCUM(c);
				state = TS_SB;
			} else {
				SB_TERM();
				suboption();	/* handle sub-option */
				state = TS_DATA;
			}
			break;

		case TS_WILL:
#ifdef DEBUG
			if(track_on)printoption(">RCVD",will,c,!hisopts[c]);
#endif
			if (c == TELOPT_BINARY) bin_neg_state++;
			if (hisopts[c] != OPT_YES){
				willoption(c);
			}
			state = TS_DATA;
			continue;

		case TS_WONT:
#ifdef DEBUG
			if(track_on)printoption(">RCVD",wont,c,hisopts[c]);
#endif
			if (c == TELOPT_BINARY) bin_neg_state--;
			if (hisopts[c] != OPT_NO)
				wontoption(c);
			state = TS_DATA;
			continue;

		case TS_DO:
#ifdef DEBUG
			if(track_on)printoption(">RCVD",doopt,c,!myopts[c]);
#endif
			if (c == TELOPT_BINARY) bin_neg_state++;
			if (myopts[c] != OPT_YES)
				dooption(c);
			state = TS_DATA;
			continue;

		case TS_DONT:
#ifdef DEBUG
			if(track_on)printoption(">RCVD",dont,c,myopts[c]);
#endif
			if (c == TELOPT_BINARY) bin_neg_state--;
			if (myopts[c] != OPT_NO) {
				dontoption(c);
			}
			state = TS_DATA;
			continue;

		default:
#ifdef DEBUG
			if(track_on)printoption(">RCVD","???",c,0);
#endif
			sprintf(msg,"panic state=%d\0", state);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if(track_on) log(msg);
#endif
			closeup();
		}
	}
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=telrcv\n");
#endif
}

willoption(option)
	int option;
{
	char *fmt;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>willoption\n");
#endif
	switch (option) {

	case TELOPT_BINARY:
		mode(0, 0, ISTRIP);
		fmt = doopt;
		break;

	case TELOPT_ECHO:
		not42 = 0;		/* looks like a 4.2 system */
		/*
		 * Now, in a 4.2 system, to break them out of ECHOing
		 * (to the terminal) mode, we need to send a "WILL ECHO".
		 * Kludge upon kludge!
		 */
		if (myopts[TELOPT_ECHO] == OPT_YES) {
		    dooption(TELOPT_ECHO);
		}
		fmt = dont;
		break;

	case TELOPT_TTYPE:
		settimer(ttypeopt);
		if (hisopts[TELOPT_TTYPE] == OPT_YES_BUT_ALWAYS_LOOK) {
		    hisopts[TELOPT_TTYPE] = OPT_YES;
		    return;
		}
		fmt = doopt;
		break;

	case TELOPT_SGA:
		fmt = doopt;
		break;

	case TELOPT_TM:
		fmt = dont;
		break;

	default:
		fmt = dont;
		break;
	}
	if (fmt == doopt) {
		hisopts[option] = OPT_YES;
	} else {
		hisopts[option] = OPT_NO;
	}
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (dont) - 2;
#ifdef DEBUG
        if (track_on) printoption(">SENT", fmt, option, 0);
	if (trace_on) fprintf(stderr,"<=willoption\n");
#endif
}

wontoption(option)
	int option;
{
	char *fmt;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>wontoption\n");
#endif
	switch (option) {
	case TELOPT_ECHO:
		not42 = 1;		/* doesn't seem to be a 4.2 system */
		break;

	case TELOPT_BINARY:
		mode(0, ISTRIP, 0);
		break;

	case TELOPT_TTYPE:
	    settimer(ttypeopt);
	    break;
	}

	fmt = dont;
	hisopts[option] = OPT_NO;
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (doopt) - 2;
#ifdef DEBUG
        if (track_on) printoption(">SENT", fmt, option, 0);
	if (trace_on) fprintf(stderr,"<=wontoption\n");
#endif
}

dooption(option)
	int option;
{
	char *fmt;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>dooption\n");
#endif
	switch (option) {

	case TELOPT_TM:
		fmt = wont;
		break;

	case TELOPT_ECHO:
		mode(3, ECHO, 0);
		fmt = will;
		break;

	case TELOPT_BINARY:
		mode(1, 0, OPOST);		/* no output processing */
		mode(2, CS8, CSIZE|PARENB);	/* 8bit, disable parity */
		fmt = will;
		break;

	case TELOPT_SGA:
		fmt = will;
		break;

	default:
		fmt = wont;
		break;
	}
	if (fmt == will) {
	    myopts[option] = OPT_YES;
	} else {
	    myopts[option] = OPT_NO;
	}
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (doopt) - 2;
#ifdef DEBUG
	if(track_on)printoption(">SENT",fmt,option,0);
	if (trace_on) fprintf(stderr,"<=dooption\n");
#endif
}


dontoption(option)
int option;
{
    char *fmt;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>dontoption\n");
#endif
    switch (option) {
    case TELOPT_ECHO:		/* we should stop echoing */
	mode(3, 0, ECHO);
	fmt = wont;
	break;

    case TELOPT_BINARY:
	mode(1, OPOST, 0);
	mode(2, CS7|PARENB, CSIZE);

    default:
	fmt = wont;
	break;
    }

    if (fmt = wont) {
	myopts[option] = OPT_NO;
    } else {
	myopts[option] = OPT_YES;
    }
    (void) sprintf(nfrontp, fmt, option);
    nfrontp += sizeof (wont) - 2;
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=dontoption\n");
	if(track_on)printoption(">SENT",fmt,option,0);
#endif
}
char	*terminaltype = 0;

/*
 * suboption()
 *
 *	Look at the sub-option buffer, and try to be helpful to the other
 * side.
 *
 *	Currently we recognize:
 *
 *	Terminal type is
 */

suboption()
{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>suboption\n");
#endif
    switch (SB_GET()) {
    case TELOPT_TTYPE: {		/* Yaaaay! */
	static char terminalname[5+41] = "TERM=";

	settimer(ttypesubopt);

	if (SB_GET() != TELQUAL_IS) {
	    return;		/* ??? XXX but, this is the most robust */
	}

	terminaltype = terminalname+strlen(terminalname);

	while ((terminaltype < (terminalname + sizeof terminalname-1)) &&
								    !SB_EOF()) {
	    register int c;

	    c = SB_GET();
	    if (isupper(c)) {
		c = tolower(c);
	    }
	    *terminaltype++ = c;    /* accumulate name */
	}
	*terminaltype = 0;
	terminaltype = terminalname;
	break;
    }

    default:
	;
    }
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=suboption\n");
#endif
}


/*
 * parse_pc_file : gets the port_configuration parameters
 */

void
parse_pc_file()
{
int i;
char	buff[LINESIZ];		/* input buffer */
char	name[LINESIZ], value[LINESIZ];
FILE	*pc_file;		/* pc_file descriptor */
char	*cp,found;

#if DEBUG
if (trace_on) fprintf(stderr,"=>parse_pc_file\n");
#endif
if ((pc_file = fopen(pc_file_path, "r")) == NULL)
	syslog(LOG_WARNING,"can't open pc_file, using default values: %m ");
else {
	/*
	 * Keep reading lines until eof
	 */
	while ( fgets(buff, sizeof(buff), pc_file) != NULL ) {
		/*
		 * Strip comments by inserting null char at each #
		 */
		if(buff[0] != '#'){	/* ignore comment lines */
		    if ((cp = strchr(buff, '#')) != NULL)
			*cp = '\0';	/* ignore everything after # */
		    i = sscanf(buff,"%s %s", name,value);
		    if (i<0) continue;
		    found = FALSE;
		    for ( i=0; (*pc_params[i] && !found) ; i++) {
			found = !strncmp ( name, pc_params[i], NCPERP);
		    }
		    if (found){
			/*
			 * Handle parameter accordingly
			 * Modify boolean parameters iff different from
			 * program default value
			 */
			switch (i)
			{
			case 1:		/* telnet_mode */
				if (strcmp (value, enable) != 0)
					telnet_mode = FALSE;
				break;	

			case 2:		/* timing_mark before TCP EOF */
				if (strcmp (value, enable) != 0)
					timing_mark = FALSE;
				break;	

			case 3:		/* telnet_timer */
				telnet_timer = atoi (value);
				break;	

			case 4:		/* binary_mode */
				if (strcmp (value, enable) == 0)
					binary_mode = TRUE;
				break;	

			case 5:		/* open_tries */
				open_tries = atoi (value);
				break;	

			case 6:		/* open_timer */
				open_timer = atoi (value);
				break;	

			case 7:		/* close_timer */
				close_timer = atoi (value);
				break;	

			case 8:		/* status_request */
				if (strcmp (value, enable) == 0)
					status_request = TRUE;
				break;	

			case 9:		/* status_timer */
				status_timer = atoi (value);
				break;	

                        case 10:        /* status_tries */
                                status_tries = atoi (value);
                                break;

			case 11:	/* eightbit mode */
				if (strcmp (value, enable) == 0)
					eightbit = TRUE;
				break;	

			case 12:	/* tcp_nodelay socket opt */
				if (strcmp (value, enable) != 0)
					tcp_nodelay = FALSE;
				break;	

			default:	/* this shouldn't happen */
				syslog(LOG_ERR,"error while parsing pc_file");
			break;	
			}
		    }
		    else {
			syslog(LOG_WARNING,"pc_file parameter '%s' invalid",name);
		    }
		}
	}
}
#if DEBUG
if (dumps_on) {
	sprintf(msg,"\ntelnet_mode:    %c\n\
timing_mark:    %c\n\
telnet_timer:   %d\n\
binary_mode:    %c\n\
open_tries:     %d\n\
open_timer:     %d\n\
close_timer:    %d\n\
status_request: %c\n\
status_timer:   %d\n\
status.tries:   %d\n\
eightbit:       %c\n\
tcp_nodely:     %c\0",
(telnet_mode?t_c:f_c),(timing_mark?t_c:f_c),telnet_timer,
(binary_mode?t_c:f_c),open_tries,
open_timer,close_timer,(status_request?t_c:f_c),
status_timer, status_tries, (eightbit?t_c:f_c), (tcp_nodelay?t_c:f_c));
	log(msg);
	}
if (trace_on) fprintf(stderr,"<=parse_pc_file\n");
#endif
}


int
open_cnxn(s_in, nb_tries, retry_period)
struct sockaddr_in s_in;
unsigned int nb_tries;
unsigned int retry_period;
{
unsigned int
snooze();
int sd;
char expo_backoff;
unsigned int g_time;
	/*
	 * Get a socket & connect to it.
	 * If 1st connect fails, retry (nb_tries-1) times, sleeping for
	 * the backoff_period between tries. If nb_tries on entry is 0
         * try for 63 years. If retry_period on entry is 0, use
	 * exponential back-off up to MAX_BACKOFF seconds, then constant.
	 * Return socket descriptor if all OK, otherwise -1 with corresponding
	 * errno.
	 */
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>open_cnxn\n");
#endif
	if (nb_tries == 0) nb_tries = FOREVER;	/* try for 68 years */
	if (retry_period == 0) expo_backoff = TRUE;
	else expo_backoff = FALSE;

#ifdef DEBUG
	if (dumps_on){
		sprintf(msg,
		"open_cnxn: nb_tries=%d, retry_period=%d exp_backoff=%c\0",
		nb_tries,retry_period,expo_backoff?t_c:f_c);
		log(msg);
	}
#endif
	get_out = FALSE;
	if (expo_backoff) g_time = 1;	/* global retry period */
	else g_time = retry_period;
	s_tries = nb_tries;		/* nb socket retries left */


    while ( !get_out ) {

	if (expo_backoff) retry_period = 1;
	while (s_tries > 0){
#ifdef DEBUG
		sprintf(msg,"Going to get new socket\0");
	        if (track_on) log(msg);
#endif
		if ((ret_val = socket(AF_INET, SOCK_STREAM, 0)) >= 0 ){
			sd = ret_val;
#ifdef DEBUG
	    		if (track_on){
				sprintf(msg,"socket OK, sd = %d\0", sd);
				log(msg);
			}
#endif
	       		break; /* out of while with s_tries > 0, ret_val>=0 */
	    	}
		sprintf(msg, "socket [%d] failed: %d-%%m\0",s_tries,errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		--s_tries;
		switch (errno){
	   	case EHOSTDOWN:		/* host is down */
		case EMFILE:		/* too many open files */
		case ENOBUFS:		/* no buffer space */
		case ENFILE:		/* file table overflow */
		case ETIMEDOUT:		/* connection timed out */
#ifdef DEBUG
			sprintf(msg,"Try again for the socket\0");
	        	if (track_on) log(msg);
#endif
			break;		/* retry socket for all of above */ 

		default:
			s_tries = 0;	/* give up completely */
#ifdef DEBUG
			sprintf(msg,"Give up on the socket & the connect\0");
	        	if (track_on) log(msg);
#endif
			break;
		}
		retry_period=snooze(retry_period,expo_backoff); /* try later */
	} /* while */
	if (s_tries <= 0) get_out = TRUE;	/* give up completely */

	c_tries = s_tries;
	if (expo_backoff) retry_period = 1;
	while (c_tries > 0){
#ifdef DEBUG
		sprintf(msg,"Going to connect\0");
	        if (track_on) log(msg);
#endif
	        if ((ret_val=
	        connect(sd,(struct sockaddr *)&s_in,sizeof(s_in)))==0){
	            sprintf(msg,"connect OK on sd = %d\0", sd);
	            syslog(LOG_INFO,msg);
#ifdef DEBUG
		    if (track_on) log(msg);
#endif

		    /*
		     * set up KEEPALIVE on our socket
	 	     */
		    if (setsockopt(sd,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on))<0) {
			sprintf(msg, "setsockopt (SO_KEEPALIVE): %d-%%m\0",errno);
			syslog(LOG_WARNING, msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
		    }

		    /* 
		     * set up TCP_NODELAY for this socket, if available so that
		     * small outbound packets will not wait for delayed ACKs
	 	     * before being sent
		     */
	 	    if (tcp_nodelay) {
			int optval;
			struct protoent *pp;

			optval = 1;
			pp = getprotobyname("tcp");
			if (!pp) {
				sprintf(msg, "getprotobyname: %d-%%m\0",errno);
				syslog(LOG_WARNING, msg);
#ifdef DEBUG
				if (track_on) log(msg);
#endif
			}
		    else if (setsockopt(sd, pp->p_proto, TCP_NODELAY, &optval,
		    sizeof(optval)) < 0) {
				sprintf(msg, "setsockopt (TCP_NODELAY): %d-%%m\0",errno);
				syslog(LOG_WARNING, msg);
#ifdef DEBUG
				if (track_on) log(msg);
#endif
		        }
		    }

#if	defined(SO_OOBINLINE)
		    setsockopt(sd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof on);
#endif	/* defined(SO_OOBINLINE) */


		    ret_val = sd;	/* return socket fd if all OK */
		    get_out = TRUE;
	            break;		/* break from connect while */
	        }

	        sprintf(msg, "connect [%d] failed: %d-%%m\0",c_tries,errno);
	        syslog(LOG_ERR, msg);
#ifdef DEBUG
	        if (track_on) log(msg);
#endif
		--c_tries;
/*!!! attention: perhaps switch is innefficient */
		switch (errno) {
		case EISCONN:		/* socket already connected */
			ret_val = 0;	/* modif ret_val to negate error state*/
			get_out = TRUE;	/* stop trying */
#ifdef DEBUG
			sprintf(msg,"Socket already connected OK\0");
	        	if (track_on) log(msg);
#endif
			break;
		case EBADF:		/* bad file number */
		case EINVAL:		/* invalid argument */
		case ETIMEDOUT:		/* connection timed out */
		case ECONNREFUSED:	/* connection refused */
		case ENETUNREACH:	/* network is unreachable */
		case EADDRINUSE:	/* address already in use */
		case ENOSPC:		/* no space left on device */
		case ENETDOWN:		/* network is down */
#ifdef DEBUG
			sprintf(msg,"Connect failed, retry with new socket\0");
	        	if (track_on) log(msg);
#endif
			s_tries = c_tries;
			c_tries = 0;
			break;

		case EINPROGRESS:	/* operation now in progress */
			ret_val = 0;	/* modify in case last iteration */
#ifdef DEBUG
			sprintf(msg,"Connect in progress, retry connect\0");
	        	if (track_on) log(msg);
#endif
			break;		/* try to connect again */

		default:		/* no point retrying anything */
			get_out = TRUE;
			c_tries = 0;
#ifdef DEBUG
			sprintf(msg,"Fatal connect error, give up\0");
	        	if (track_on) log(msg);
#endif
			break;
		}
	    if (c_tries>0) retry_period=snooze(retry_period, expo_backoff);
	} /* while */

	/*
	 * If the connect failed, and we need a new socket, back off first
	 */
	if(!get_out){
#ifdef DEBUG
		sprintf(msg,"Closing socket and globally snoozing\0");
	       	if (track_on) log(msg);
#endif
		(void)close(sd);	/* in case it's open */
		g_time=snooze(g_time, expo_backoff);/* try later */
	}
    } /* while  !get_out */

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=open_cnxn\n");
#endif
	return(ret_val);
}


unsigned int
snooze(t, expo)
unsigned int t;
char expo;
{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>snooze\n");
#endif
        sleep( t );
        if( expo ){
		if( t < MAX_BACKOFF ){
			t <<= 1;
		}
        }
#ifdef DEBUG
	if (track_on){
	    sprintf(msg,"New retry_period: %d\0", t);
		log(msg);
	}
	if (trace_on) fprintf(stderr,"<=snooze\n");
#endif
	return(t);
}


int
handshake(fd, r_info, local_errno)
int fd;
struct request_info r_info;
int local_errno;
{
	/*
	 * Complete exception interrupt handshake with slave
	 */
	req_info.errno_error = local_errno;
	req_info.return_value = 0;
/*	if ( errno == 0 )req_info.return_value = 0;
 *	else req_info.return_value = -1;
 */
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>handshake\n");
	if (dumps_on){
		sprintf(msg,"handshaking with:\n\
                     request:%x\n\
		     argget:%x\n\
                     argset:%x\n\
                     pgrp:%d\n\
                     pid:%d\n\
                     errno_error:%d\n\
                     return_value:%d\0",
		req_info.request,req_info.argget,
		req_info.argset,req_info.pgrp,req_info.pid,
		req_info.errno_error,req_info.return_value);
		log(msg);
	}
#endif
        if(( ioctl(fd, TIOCREQSET, (caddr_t)&r_info)) == -1){
		/*!!! should check that it's truly an EINVAL */
		sprintf(msg,
		"ioctl failed to complete p exception handshake: %d-%%m\0",errno);
		syslog(LOG_WARNING, msg);
#ifdef DEBUG
		if (track_on) log(msg);
		if (trace_on) fprintf(stderr,"<=handshake\n");
#endif
		return(-1);
	}
#ifdef DEBUG
	if (track_on){
		sprintf(msg,"p exeception handshake OK\0");
		log(msg);
	}
	if (trace_on) fprintf(stderr,"<=handshake\n");
#endif
	return (0);
}


/*
 * Check a descriptor to see if out of band data exists on it.
 */


stilloob(s)
int	s;		/* socket number */
{
    static struct timeval timeout = { 0 };
    fd_set	excepts;
    int value;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>stilloob\n");
#endif
    do {
	FD_ZERO(&excepts);
	FD_SET(s, &excepts);
	value = select(s+1, (fd_set *)0, (fd_set *)0, &excepts, &timeout);
    } while ((value == -1) && (errno == EINTR));

    if (value < 0) {
	sprintf(msg, "stilloob select failed: %d-%%m",errno);
	syslog(LOG_ERR, msg);
#ifdef DEBUG
	if(track_on)log(msg);
#endif
    }
    if (FD_ISSET(s, &excepts)) {
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=stilloob\n");
#endif
	return 1;
    } else {
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=stilloob\n");
#endif
	return 0;
    }
}


int
send_TM()
{
static char sbuf[] = { IAC, DO, TELOPT_TM };
unsigned int TM_timer;
register int c;
static struct timeval timeout;
fd_set ibits,obits,xbits;
int tmm_state;
int i;

#ifdef DEBUG
	if(trace_on)fprintf(stderr,"=>send_TM\n");
#endif
	bcopy(sbuf, nfrontp, sizeof sbuf);
	nfrontp += sizeof sbuf;
	if ( telnet_timer == 0 ) 
           TM_timer = FOREVER;
        else
	   TM_timer = telnet_timer;
	timeout.tv_sec = TM_timer;
	timeout.tv_usec = 0;
	sprintf(msg,"Sending TM, wait %d s for reply\0",TM_timer);
	syslog(LOG_INFO,msg);
#ifdef DEBUG
	if(track_on) log(msg);
#endif
        i = 100;   /* IND 1.1.109.10  1653-061465 */
        while (( nfrontp - nbackp > 0 ) && ( i )) {
	   netflush();
           i--;
           sleep( 1 );
        }
	FD_ZERO(&ibits);
	FD_ZERO(&obits);
	FD_ZERO(&xbits);
	FD_SET(net, &ibits);
#ifdef DEBUG
	if(track_on){
		sprintf(msg,"Going to select for TM neg reply\0");
		log(msg);
	}
 	if (dumps_on){
    	     sprintf(msg,"ibits:%x  obits:%x  xbits:%x\0",
	     ibits.fds_bits[0], obits.fds_bits[0], xbits.fds_bits[0]);
	     log(msg);
	}
#endif
	ret_val=select(net+1,&ibits,&obits,&xbits,&timeout);
#ifdef DEBUG
	if(track_on){
		sprintf(msg,"TM select done\0");
		log(msg);
	}
 	if (dumps_on){
	     sprintf(msg,"TM select [t=%d] returns:  \
             nb_fd:%d ibits:%x obits:%x xbits:%x errno:%d\0",
	     timeout.tv_sec,ret_val,ibits.fds_bits[0],obits.fds_bits[0],
	     xbits.fds_bits[0],errno);
	     log(msg);
	}
#endif
   	if (ret_val < 0) {
		sprintf(msg,"TM select failed: %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=send_TM\n");
#endif
                disable_printer();
		return(TM_FAILED);
   	}
	if (ret_val == 0){
		sprintf(msg,"TM select timed-out\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=send_TM\n");
#endif
                disable_printer();
		return(TM_FAILED);
	}
	if (FD_ISSET(net, &ibits)) (ncc = read(net, netibuf, sizeof netibuf));
	else{
		sprintf(msg, "select IT not due to input from network\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=send_TM\n");
#endif
                disable_printer();
		return(TM_FAILED);
	}
	if (ncc < 0) {
		sprintf(msg, "TM read failed : %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=send_TM\n");
#endif
                disable_printer();
		return(TM_FAILED);
	}
	/*
	 * get the TM neg reply
	 */
	netip = netibuf;
	tmm_state = TS_DATA;
#ifdef DEBUG
	if(dumps_on)Dump(netibuf,ncc,1);
#endif
	while(ncc > 0){
		c = *netip++ & 0377, ncc--;
		switch (tmm_state){
		case TS_DATA:
			if (c == IAC) tmm_state = TS_IAC;
			break;
		case TS_IAC:
			switch ( c){
			case WILL:
				tmm_state = TS_WILL;
				continue;
			case WONT:
				tmm_state = TS_WONT;
				continue;
			case DO:
				tmm_state = TS_DO;
				continue;
			case DONT:
				tmm_state = TS_DONT;
				continue;
			}
			tmm_state = TS_DATA;
			break;
		case TS_WILL:
		case TS_WONT:
			if ( c == TELOPT_TM){
				sprintf(msg, "TM negotiation received\0");
				syslog(LOG_INFO,msg);
#ifdef DEBUG
				if(track_on)log(msg);
				if(trace_on)fprintf(stderr, "<=send_TM\n");
#endif
				/*
				 * ignore the rest
				 */
				ncc = 0;
				netip = netibuf;
				return(0);
			}/* fall through */
		case TS_DO:
		case TS_DONT:
			tmm_state = TS_DATA;
			continue;
		default:
			sprintf(msg,"panic state=%d\0", tmm_state);
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if(track_on) log(msg);
#endif
			closeup();
		}
	}
	sprintf(msg,"No TM negotiation in data stream\0");
	syslog(LOG_ERR,msg);
#ifdef DEBUG
	if(track_on) log(msg);
	if(trace_on)fprintf(stderr,"<=send_TM\n");
#endif
        disable_printer();
	return(TM_FAILED);
}



/*
* do a select to see if we received any replies to the telnet 
* negotiations
*/
int
check_tel_neg_reply()
{
fd_set ibits, obits, xbits;
static struct timeval timeout;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>check_tel_neg_reply\n");
#endif
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;

	FD_ZERO(&ibits);
	FD_ZERO(&obits);
	FD_ZERO(&xbits);
	FD_SET(net, &ibits);

#ifdef DEBUG
	if (track_on)
	  {
 	   sprintf(msg,"Going to select for telnet neg reply check\0");
	   log(msg);
	  }
 	if (dumps_on)
	  {
    	     sprintf(msg,"ibits:%x  obits:%x  xbits:%x\0",
		 ibits.fds_bits[0], obits.fds_bits[0], xbits.fds_bits[0]);
	     log(msg);
		fprintf(stderr, "Buffer pointers before select:\n\
ptyibuf:%x  ptyobuf:%x  ptyip:%x  pfrontp:%x  pbackp:%x\n\
netibuf:%x  netobuf:%x  netip:%x  nfrontp:%x  nbackp:%x\n\
ncc:%d pcc:%d\n",
ptyibuf,ptyobuf,ptyip,pfrontp,pbackp,netibuf,netobuf,netip,nfrontp,nbackp,ncc,pcc);
	  }
#endif

	ret_val = select(net+1,&ibits,&obits,&xbits,&timeout);

#ifdef DEBUG
	if (track_on)
	  {
	   sprintf(msg,"telnet neg select done\0");
	   log(msg);
	  }
 	if (dumps_on)
	  {
	   sprintf(msg,"telnet neg select returns:  \
           nb_fd:%d ibits:%x obits:%x xbits:%x errno:%d\0", ret_val,
           ibits.fds_bits[0],obits.fds_bits[0], xbits.fds_bits[0],errno);
	   log (msg);
	  }
#endif
   	if (ret_val < 0) 
	  {
	   sprintf(msg,"telnet neg select failed: %d-%%m\0",errno);
	   syslog(LOG_ERR, msg);
#ifdef DEBUG
	   if (track_on) log(msg);
	   if (trace_on) fprintf(stderr,"<=check_tel_neg_reply\n");
#endif
	   return;
   	  }
	if (ret_val == 0)
	  {
	   sprintf(msg,"telnet neg select timed-out\0");
	   syslog(LOG_ERR, msg);
#ifdef DEBUG
	   if(track_on)log(msg);
	   if(trace_on) fprintf(stderr,"<=check_tel_neg_reply\n");
#endif
	   return;
	  }
        if (FD_ISSET(net, &ibits))
          {
	   ncc = read(net, netibuf, sizeof netibuf);
	   if (ncc < 0) 
	     {
	      sprintf(msg, "telnet neg reply read failed : %d-%%m\0",errno);
	      syslog(LOG_ERR, msg);
#ifdef DEBUG
	      if (track_on) log(msg);
	      if (trace_on) fprintf(stderr,"<=check_tel_neg_reply\n");
#endif
	      return;
	     }
#ifdef DEBUG
	   if (dumps_on) Dump(netibuf, ncc, 1);
#endif
	   if (telnet_mode) 
	      telrcv();	 	/* filter out the data */
          }
        else
          { /* if ibits */
	   sprintf(msg,"telnet neg select IT not due to net input\0");
	   syslog(LOG_WARNING,msg);
#ifdef DEBUG
	   log(msg);
#endif
          }
#ifdef DEBUG
        if (trace_on) fprintf(stderr,"<=check_tel_neg_reply \n");
#endif
}


int
do_status_req()
{
static char sbuf[] = { '\033', '?', '\021' };
unsigned int SR_timer;
int status_reply;
int n_data, tries;
fd_set ibits, obits, xbits;
static struct timeval timeout;
time_t sr_sleep, then;
char log_msg = TRUE;

/*
 * Send status request, accept 1st non-telnet char received from net as reply
 * Could be more efficient but no time.
 */
#ifdef DEBUG
	if(trace_on) fprintf(stderr,"=>do_status_req\n");
#endif

for (tries = 1; tries <= status_tries; tries++)
   {
	if ( status_timer == 0 ) 
	   SR_timer = FOREVER;
	else 
           SR_timer = status_timer;

	timeout.tv_sec = SR_timer;
	timeout.tv_usec = 0;

    while (timeout.tv_sec>0)
         {
	bcopy(sbuf, nfrontp, sizeof sbuf);
	nfrontp += sizeof sbuf;
	netip = netibuf;
        if (log_msg) {
	        sprintf(msg,"Sending SR, wait %d s for reply\0",timeout.tv_sec);
	        syslog(LOG_INFO,msg);
        }
#ifdef DEBUG
	if(track_on) log(msg);
#endif
	netflush();
	FD_ZERO(&ibits);
	FD_ZERO(&obits);
	FD_ZERO(&xbits);
	FD_SET(net, &ibits);

#ifdef DEBUG
	if(track_on){
 		sprintf(msg,"Going to select for SR reply\0");
		log(msg);
	}
 	if (dumps_on){
    	     sprintf(msg,"ibits:%x  obits:%x  xbits:%x\0",
		 ibits.fds_bits[0], obits.fds_bits[0], xbits.fds_bits[0]);
	     log(msg);
		fprintf(stderr, "Buffer pointers before select:\n\
ptyibuf:%x  ptyobuf:%x  ptyip:%x  pfrontp:%x  pbackp:%x\n\
netibuf:%x  netobuf:%x  netip:%x  nfrontp:%x  nbackp:%x\n\
ncc:%d pcc:%d\n",
ptyibuf,ptyobuf,ptyip,pfrontp,pbackp,netibuf,netobuf,netip,nfrontp,nbackp,ncc,pcc);
	}
#endif
	ret_val=select(net+1,&ibits,&obits,&xbits,&timeout);
#ifdef DEBUG
	if(track_on){
		sprintf(msg,"SR select done\0");
		log(msg);
	}
 	if (dumps_on){
	     sprintf(msg,"SR select [t=%d] returns:  \
             nb_fd:%d ibits:%x obits:%x xbits:%x errno:%d\0",
	     timeout.tv_sec,ret_val,ibits.fds_bits[0],obits.fds_bits[0],
	     xbits.fds_bits[0],errno);
	     log(msg);
	}
#endif
   	if (ret_val < 0) {
		sprintf(msg,"SR select failed: %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on) fprintf(stderr,"<=do_status_req\n");
#endif
                sprintf(reason_msg, "Error receiving status from printer.\n");
                disable_printer();
		return(SR_FAILED);
   	}
	if (ret_val == 0){
		sprintf(msg,"SR select timed-out\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on) fprintf(stderr,"<=do_status_req\n");
#endif
                if (tries >= status_tries)
                  {
                  sprintf(reason_msg, "Printer did not respond to status request.");
                  disable_printer();
	   	  return(SR_FAILED);
                  }
                else
                  {
                   timeout.tv_sec = 0;
                   continue;
                  }
	}
   if (FD_ISSET(net, &ibits)){
	ncc = read(net, netibuf, sizeof netibuf);
	if (ncc < 0) {
		sprintf(msg, "SR read failed : %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on) fprintf(stderr,"<=do_status_req\n");
#endif
                sprintf(reason_msg, "Error trying to read status request.\n");
                disable_printer();
		return(SR_FAILED);
	}
#ifdef DEBUG
	if(dumps_on)Dump(netibuf, ncc, 1);
#endif
	if (telnet_mode) telrcv();	/* filter out the data */
	else{
		while (ncc > 0){
			if( (&ptyobuf[BUFSIZ] - pfrontp) == 0 )
				break;
			*pfrontp++ = *netip++ & 0377, ncc--;
		}
	}
	n_data = pfrontp-pbackp;   /* nb non-telnet chars */
	if(n_data == 0){
		sprintf(msg,"No status reply in data stream\0");
		syslog(LOG_WARNING, msg);
#ifdef DEBUG
		if(track_on)log(msg);
#endif
                if (tries < status_tries)
                   {
                    timeout.tv_sec = 0;
                    continue;
                   }
	}
	if(n_data > 1){
		sprintf(msg, "SR got %d chars\0",n_data);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(dumps_on)Dump(pbackp,n_data,0);
#endif
		/*
		 * If garbage, take the last char in the stream
		 */
		pbackp = pfrontp - 1;
		n_data = 1;
	}
	if(n_data == 1){
		--pfrontp;	/* eat the SR reply */
		status_reply  = *pfrontp & 0377;
#ifdef DEBUG
		if(track_on)log(msg);
#endif
		/*
		 * analyse status reply
		 */
		switch (status_reply){
		case LP_OK:
			sprintf(msg,"SR: Printer OK\0");
			syslog(LOG_INFO,msg);
#ifdef DEBUG
			if(track_on)log(msg);
			if(trace_on) fprintf(stderr,"<=do_status_req\n");
#endif
			return(0);		/* lp ready ... success */

		case LP_BUSY:
			sprintf(msg,"SR: Printer busy\0");
                        if (log_msg)
			       syslog(LOG_WARNING,msg);
#ifdef DEBUG
			if(track_on)log(msg);
#endif
                        sleep(SR_timer);
                        log_msg = FALSE;
			break;

		case LP_NO_PAPER:
			sprintf(msg,"SR: No paper\0");
                        if (log_msg)
		   	        syslog(LOG_WARNING,msg);
#ifdef DEBUG
			if(track_on)log(msg);
#endif
                        sleep(SR_timer);
                        log_msg = FALSE;
			break;

		case LP_OFF_LINE:	/* fat chance of getting this */
			sprintf(msg,"SR: Printer off line\0");
                        if (log_msg)
			        syslog(LOG_WARNING,msg);
#ifdef DEBUG
			if(track_on)log(msg);
#endif
                        sleep(SR_timer);
                        log_msg = FALSE;
			break;

		case LP_DATA_ERROR:
			sprintf(msg,"SR: Printer data error\0");
			syslog(LOG_WARNING,msg);
#ifdef DEBUG
			if(track_on) log(msg);
#endif
                        sprintf(reason_msg,"SR: Printer data error\n");
                        disable_printer();
                        return(SR_FAILED);

		default:
			sprintf(msg,"SR got reply: %x\0", status_reply);
                        syslog(LOG_INFO, msg);
                        sprintf(msg,"SR: Unknown status reply\0");
			syslog(LOG_WARNING,msg);
                        sleep (SR_timer);
#ifdef DEBUG
			if(track_on) log(msg);
#endif
			break;
		} /* switch */
	} /* if n_data == 1 */
    }else{ /* if ibits */
	sprintf(msg,"SR select IT not due to net input\0");
	syslog(LOG_WARNING,msg);
#ifdef DEBUG
	log(msg);
#endif
    }
/*********************************************************
 *
 *  This section removed.  If the status request failed, it
 *  re-sent the status request.  This loop was occuring way
 *  too fast and filling up the syslog file.  Now it will
 *  only retry the number of retries configured.  If the
 *  status request fails, it's because the printer is not
 *  at a ready state.
 *
 **********************************************************
 *  Need the line then = time(NULL); right before the
 *  select.
 *
 *  sr_sleep = time(NULL) - then;
 *  if (timeout.tv_sec > sr_sleep)
 *  	timeout.tv_sec -= sr_sleep;
 *  else timeout.tv_sec = 0;
 *
 ***********************************************************/
    timeout.tv_sec = SR_timer;
#ifdef DEBUG
	if(track_on)fprintf(stderr,
		"timeout.tv_sec=%d\n",timeout.tv_sec);
#endif
     } /* while */
   } /* for */
    if (log_msg) {
       sprintf(msg,"SR: problem not fixed on time. Giving up\0");
       syslog(LOG_ERR,msg);
    }
#ifdef DEBUG
    if(track_on)log(msg);
    if(trace_on) fprintf(stderr,"<=do_status_req\n");
#endif
    sprintf(reason_msg, "Printer not ready.  Check printer status.\n");
    disable_printer();
    return(SR_FAILED);
}

int
toggle_binary(flag)
int flag;
{
unsigned int BN_timer;
static struct timeval timeout;
fd_set ibits,obits,xbits;
time_t then, bn_sleep;

#ifdef DEBUG
	if(trace_on) fprintf(stderr,"=>toggle_binary\n");
#endif
	if ( telnet_timer == 0 ) 
           BN_timer = FOREVER;
        else
	   BN_timer = telnet_timer;
	timeout.tv_sec = BN_timer;
	timeout.tv_usec = 0;
	/*
	 * toggle binary on/off
	 */
	if( flag == off ){
		if ( (hisopts[TELOPT_BINARY] == OPT_NO) &&
		     (myopts[TELOPT_BINARY]  == OPT_NO) ){
#ifdef DEBUG
			if(trace_on) fprintf(stderr,"<=toggle_binary\n");
#endif
			 return(TRUE);
		}
		sprintf(msg,"Sending BIN-NEG, wait %d s for reply\0",BN_timer);
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if(track_on) log(msg);
#endif
		dontoption(TELOPT_BINARY);
		wontoption(TELOPT_BINARY);
		bin_neg_state = 2;
		netflush();
	} else {
		if ( (hisopts[TELOPT_BINARY] == OPT_YES) &&
		     (myopts[TELOPT_BINARY]  == OPT_YES) ) return(TRUE);
		sprintf(msg,"Sending BIN-NEG, wait %d s for reply\0",BN_timer);
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if(track_on) log(msg);
#endif
		dooption(TELOPT_BINARY);
   		willoption(TELOPT_BINARY);
		bin_neg_state = -2;
		netflush();
	}

	while (timeout.tv_sec > 0){
	    FD_ZERO(&ibits);
	    FD_ZERO(&obits);
	    FD_ZERO(&xbits);
	    FD_SET(net, &ibits);
#ifdef DEBUG
	    if(track_on){
		sprintf(msg,"Going to select [%ds] for BIN-NEG reply\0",
			timeout.tv_sec);
		log(msg);
	    }
 	    if (dumps_on){
    		sprintf(msg,"ibits:%x  obits:%x  xbits:%x\0",
		ibits.fds_bits[0], obits.fds_bits[0], xbits.fds_bits[0]);
	 	log(msg);
	    }
#endif
	    then = time(NULL);
	    ret_val=select(net+1,&ibits,&obits,&xbits,&timeout);
#ifdef DEBUG
	    if(track_on){
		sprintf(msg,"BIN-NEG select done\0");
		log(msg);
	    }
 	    if (dumps_on){
	     sprintf(msg,"BIN-NEG select [t=%d] returns:  \
             nb_fd:%d ibits:%x obits:%x xbits:%x errno:%d\0",
	     timeout.tv_sec,ret_val,ibits.fds_bits[0],obits.fds_bits[0],
	     xbits.fds_bits[0],errno);
	     log(msg);
	    }
#endif
   	    if (ret_val < 0) {
		sprintf(msg,"BIN-NEG select failed: %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
		return(FALSE);
   	    }
	    if (ret_val == 0){
		sprintf(msg,"BIN-NEG select timed-out\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
		return(FALSE);
	    }
	    if (FD_ISSET(net, &ibits)) (ncc=read(net, netibuf, sizeof netibuf));
	    else{
		sprintf(msg, "select IT not due to input from network\0");
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
		return(FALSE);
	    }
	    if (ncc < 0) {
		sprintf(msg, "BIN-NEG  read failed : %d-%%m\0",errno);
		syslog(LOG_ERR, msg);
#ifdef DEBUG
		if(track_on)log(msg);
		if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
		return(FALSE);
	    }
	    /*
	     * analyse data received from the lan
	     */
	    netip = netibuf;
#ifdef DEBUG
	    if(dumps_on)Dump(netibuf,ncc,0);
#endif
	    telrcv();
	    if (bin_neg_state == 0){
			sprintf(msg, "BINARY negotiation accepted by remote\0");
			syslog(LOG_INFO, msg);
#ifdef DEBUG
			if(track_on)log(msg);
			if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
			return(TRUE);
	    }
	    if (bin_neg_state == -4){
			sprintf(msg, "DO BINARY refused by remote\0");
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if(track_on)log(msg);
			if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
			return(FALSE);
	    }
	    if (bin_neg_state == 4){
			sprintf(msg, "DONT BINARY refused by remote\0");
			syslog(LOG_ERR, msg);
#ifdef DEBUG
			if(track_on)log(msg);
			if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
			return(FALSE);
	    }
	    /*
	     * sorry about this
	     */
	    bn_sleep = time(NULL) - then;
	    if (timeout.tv_sec > bn_sleep)
	    	timeout.tv_sec -= bn_sleep;
	    else timeout.tv_sec = 0;
#ifdef DEBUG
	    if(trace_on)sprintf(msg,
			"bin_neg_state = %d ... try again\0",bin_neg_state);
	    if(track_on)log(msg);
	    fprintf(stderr,"timeout.tv_sec=%d\n",timeout.tv_sec);
#endif
	}	/* while */
	sprintf(msg,"Remote didn't confirm binary negotiation on time\0");
	syslog(LOG_ERR, msg);
#ifdef DEBUG
	if(track_on)log(msg);
	if(trace_on)fprintf(stderr,"<=toggle_binary\n");
#endif
	return(FALSE);
}


mode(flag, m_on, m_off)
int flag, m_on, m_off;
{
	struct termio b;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>mode\n");
#endif
	if (ioctl(pty, TCGETA, &b) < 0)
		syslog(LOG_WARNING, "ioctl(TCGETA): %m");

	switch (flag)
	{
	case 0: b.c_iflag &= ~m_off;
		b.c_iflag |= m_on;
		break;
	case 1: b.c_oflag &= ~m_off;
		b.c_oflag |= m_on;
		break;
	case 2: b.c_cflag &= ~m_off;
		b.c_cflag |= m_on;
		break;
	case 3: b.c_lflag &= ~m_off;
		b.c_lflag |= m_on;
		break;
	default: b.c_cc[m_on] = m_off;
	}

	if (ioctl(pty, TCSETA, &b) < 0)
		syslog(LOG_WARNING, "ioctl(TCSETA): %m");
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=mode\n");
#endif
}


/*
 * Send interrupt to process on other side of pty.
 */
interrupt()
{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>interrupt\n");
#endif
	 if(ioctl(pty, TIOCSIGSEND, SIGINT) < 0)
                syslog(LOG_WARNING, "ioctl(TIOCSIGSEND): %m");
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=interrupt\n");
#endif
}

/*
 * Send break to process on other side of pty.
 */
sendbrk()
{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>sendbrk\n");
#endif
	if (ioctl(pty, TIOCBREAK, 0) < 0)
		syslog(LOG_WARNING, "ioctl(TIOCBREAK): %m");
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=sendbrk\n");
#endif
}

ptyflush()
{
	int n;

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>ptyflush\n");
#endif
	if ((n = pfrontp - pbackp) > 0)
		n = write(pty, pbackp, n);
	if (n < 0)
		return;
	pbackp += n;
	if (pbackp == pfrontp)
		pbackp = pfrontp = ptyobuf;
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=ptyflush\n");
#endif
}


/*
 * nextitem()
 *
 *	Return the address of the next "item" in the TELNET data
 * stream.  This will be the address of the next character if
 * the current address is a user data character, or it will
 * be the address of the character following the TELNET command
 * if the current address is a TELNET IAC ("I Am a Command")
 * character.
 */

char *
nextitem(current)
char	*current;
{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>nextitem\n");
#endif
    if ((*current&0xff) != IAC) {
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=nextitem\n");
#endif
	return current+1;
    }
    switch (*(current+1)&0xff) {
    case DO:
    case DONT:
    case WILL:
    case WONT:
	{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=nextitem\n");
#endif
	return current+3;
	}
    case SB:		/* loop forever looking for the SE */
	{
	    register char *look = current+2;

	    for (;;) {
		if ((*look++&0xff) == IAC) {
		    if ((*look++&0xff) == SE) {
#ifdef DEBUG
			if (trace_on) fprintf(stderr,"<=nextitem\n");
#endif
			return look;
		    }
		}
	    }
	}
    default:
	{
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=nextitem\n");
#endif
	return current+2;
	}
    }
}


/*
 * netclear()
 *
 *	We are about to do a TELNET SYNCH operation.  Clear
 * the path to the network.
 *
 *	Things are a bit tricky since we may have sent the first
 * byte or so of a previous TELNET command into the network.
 * So, we have to scan the network buffer from the beginning
 * until we are up to where we want to be.
 *
 *	A side effect of what we do, just to keep things
 * simple, is to clear the urgent data pointer.  The principal
 * caller should be setting the urgent data pointer AFTER calling
 * us in any case.
 */

netclear()
{
    register char *thisitem, *next;
    char *good;
#define	wewant(p)	((nfrontp > p) && ((*p&0xff) == IAC) && \
				((*(p+1)&0xff) != EC) && ((*(p+1)&0xff) != EL))

#ifdef DEBUG
    if (trace_on) fprintf(stderr,"=>netclear\n");
#endif
    thisitem = netobuf;

    while ((next = nextitem(thisitem)) <= nbackp) {
	thisitem = next;
    }

    /* Now, thisitem is first before/at boundary. */

    good = netobuf;	/* where the good bytes go */

    while (nfrontp > thisitem) {
	if (wewant(thisitem)) {
	    int length;

	    next = thisitem;
	    do {
		next = nextitem(next);
	    } while (wewant(next) && (nfrontp > next));
	    length = next-thisitem;
	    bcopy(thisitem, good, length);
	    good += length;
	    thisitem = next;
	} else {
	    thisitem = nextitem(thisitem);
	}
    }

    nbackp = netobuf;
    nfrontp = good;		/* next byte to be sent */
    neturg = 0;
#ifdef DEBUG
    if (trace_on) fprintf(stderr,"<=netclear\n");
#endif
}


/*
 *  netflush
 *		Send as much data as possible to the network,
 *	handling requests for urgent data.
 */


netflush()
{
    int n;

#ifdef DEBUG
    if (trace_on) fprintf(stderr,"=>netflush\n");
#endif
    if ((n = nfrontp - nbackp) > 0) {
	/*
	 * if no urgent data, or if the other side appears to be an
	 * old 4.2 client (and thus unable to survive TCP urgent data),
	 * write the entire buffer in non-OOB mode.
	 */
	if ((neturg == 0) || (not42 == 0)) {
	    n = write(net, nbackp, n);	/* normal write */
	} else {
	    n = neturg - nbackp;
	    /*
	     * In 4.2 (and 4.3) systems, there is some question about
	     * what byte in a sendOOB operation is the "OOB" data.
	     * To make ourselves compatible, we only send ONE byte
	     * out of band, the one WE THINK should be OOB (though
	     * we really have more the TCP philosophy of urgent data
	     * rather than the Unix philosophy of OOB data).
	     */
	    if (n > 1) {
		n = send(net, nbackp, n-1, 0);	/* send URGENT all by itself */
	    } else {
		n = send(net, nbackp, n, MSG_OOB);	/* URGENT data */
	    }
	}
    }
    if (n < 0) {
	if (errno == EWOULDBLOCK){
#ifdef DEBUG
	    if (trace_on) fprintf(stderr,"<=netflush\n");
#endif
	    return 0;
	}
	/* should blow this guy away... */
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=netflush\n");
#endif
	return n;
    }
    nbackp += n;
    if (nbackp >= neturg) {
	neturg = 0;
    }
    if (nbackp == nfrontp) {
	nbackp = nfrontp = netobuf;
    }
#ifdef DEBUG
    if (trace_on) fprintf(stderr,"<=netflush\n");
#endif
    return 0;
}

int
check_pseudonym()
{
struct stat statbuf;

	if(stat(pseudonym,&statbuf) == -1){
	    /*
	     * see why stat failed
	     */
	    if (errno == ENOENT){
		/*
		 * stat failed because file doesn't exist
		 */
		return(-1);
	    }else{
		/*
		 * stat failed badly
		 */
		sprintf(msg, "stat on %s failed, errno: %d-%%m\0",pseudonym);
		syslog(LOG_ERR,msg);
#ifdef debug
		if (track_on) log(msg);
#endif
		return(0);
	    }
	}
	/*
	 * pseudonym exists, check if it belongs to this ocd or if it has
	 * been adopted by another
	 */
	if (slave_rdev == statbuf.st_rdev)
		/*
		 * it's ours
		 */
		return(0);
	else
		/*
		 * someone else has nicked it
		 */
		return(-1);
}

int
pseudonym_available()
{
struct stat statbuf;

	/*
	 * get pure name of pseudonym
	 */
	if( (p_name = strrchr ( pseudonym,'/')) != NULL ) p_name++;
	else p_name = pseudonym;
	if(stat(pseudonym,&statbuf) == -1){
	    /*
	     * see why stat failed
	     */
	    if (errno == ENOENT){
		/*
		 * stat failed because file doesn't exist
		 */
		return(TRUE);
	    }else{
		/*
		 * stat failed badly
		 */
		sprintf(msg, "stat on %s failed, errno: %d-%%m\0",pseudonym);
		syslog(LOG_ERR,msg);
#ifdef debug
		if (track_on) log(msg);
#endif
		return(FALSE);
	    }
	}
		/*
		 * pseudonym already exists, check if it belongs
		 * to a process killed using -9
		 */
		setutent();
		bcopy(p_name,utent.ut_line,sizeof(utent.ut_line));
		utptr = (struct utmp *)getutline(&utent);
		bcopy(utptr,&utent,sizeof(utent));
		endutent();
		utptr = &utent;
#ifdef DEBUG
	            sprintf(msg,"utmp returns: pid: %d  ut_line: %s  ut_id: %s\0",utptr->ut_pid, utptr->ut_line, utptr->ut_id);
            	    if (track_on) log(msg);
#endif
		if ( utptr == (struct utmp *) 0 ){
	            sprintf(msg,"pseudonym '%s' already in use but not by ocd\0",pseudonym);
		    syslog(LOG_ERR,msg);
#ifdef DEBUG
            	    if (track_on) log(msg);
#endif
		    return(FALSE);
		}
		if( kill(utptr->ut_pid, 0) == 0 ){
	    	sprintf(msg,"pseudonym '%s' already in use by ocd[%d]\0",
						pseudonym, utptr->ut_pid);
			syslog(LOG_ERR,msg);
#ifdef DEBUG
			if (track_on) log(msg);
#endif
			return(FALSE);
		}
		if (errno != ESRCH){
    		sprintf(msg,"Unable to free %s using kill(%d, 0), errno: %d-%%m\0", pseudonym, utptr->ut_pid, errno );
			syslog(LOG_ERR,msg);
#ifdef debug
			if (track_on) log(msg);
#endif
			return(FALSE);
		}
		/*
		 * Check that device is a pty slave ( network special file )
		 */
		if ((statbuf.st_rdev & 0xFF000000) != 0x11000000){
			sprintf(msg, "pseudonym %s is not a pty slave\0",
				pseudonym);
			syslog(LOG_ERR,msg);
#ifdef debug
			if (track_on) log(msg);
#endif
			return(FALSE);
		}
		if ( unlink(pseudonym) == -1){
			sprintf(msg, "Can't delete unowned pseudonym %s: %d-%%m\0",pseudonym,errno);
			syslog(LOG_ERR,msg);
#ifdef debug
			if (track_on) log(msg);
#endif
			return(FALSE);
		}
		sprintf(msg, "Deleted unowned pseudonym %s\0",pseudonym);
		syslog(LOG_INFO,msg);
#ifdef debug
		if (track_on) log(msg);
#endif
	       	dfa_account(DEAD_PROCESS,utptr->ut_pid,"","",NULL,NULL);
		return(TRUE);
}

void
cleanup()
{
/*
 * tidy things up before getting out
 */

#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>cleanup\n");
#endif
	sprintf(msg,"FATAL ERROR OCCURRED - closing down ocd for\
-n %s -f %s -b %d -p %d\0", node_name, pseudonym, board, port);
	syslog(LOG_CRIT,msg);
	if(cnxn_open){
		sprintf(msg,"Shutting down network cnxn\0");
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		shutdown(net, 2);
		close(net);
		cnxn_open = FALSE;
	}
	chmod(master,0666);
	chown(master, 0, 0);
	chmod(slave,0666);
	chown(slave, 0, 0);
/*
 * remove pseudonym
 */
	if ( unlink(pseudonym) == 0){
		sprintf(msg, "Unlinked from %s\0",pseudonym);
		bcopy(p_name,utent.ut_line,sizeof(utent.ut_line));
   		dfa_account(DEAD_PROCESS,my_pid,"","",NULL,NULL);
	}else{
		sprintf(msg, "Can't unlink from %s: %d-%%m\0",pseudonym,errno);
		syslog(LOG_WARNING,msg);
#ifdef DEBUG
		if(track_on)log(msg);
#endif
	}
#ifdef DEBUG
	if(trace_on)fprintf(stderr,"<=cleanup");
	fflush(stderr);
#endif
	exit(1);

}

void
shakeup_pty()
{
/*
 * Closes and re-opens the pty master to indicate to slave process that we
 * have problems. Causes a SIGHUP to be sent to controlling terminal's group
 * Causes further application open/close/ioctl/write to return error. read
 * returns 0 bytes
 */
#ifdef DEBUG
	if(trace_on)fprintf(stderr,"=>shakeup_pty");
#endif
	sprintf(msg, "Closing/re-opening pty: %s dev: %d\0",master,pty);
	syslog(LOG_WARNING,msg);
#ifdef DEBUG
	if(track_on)log(msg);
#endif
/*
 * remove pseudonym
 */
	if ( unlink(pseudonym) == 0){
		sprintf(msg, "Unlinked from %s\0",pseudonym);
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if(track_on)log(msg);
#endif
	}else{
		sprintf(msg, "Can't unlink from %s: %d-%%m\0",pseudonym,errno);
		syslog(LOG_ERR,msg);
#ifdef DEBUG
		if(track_on)log(msg);
#endif
	}
	/*
	 * get a new pty and remake pseudonym
	 */
	chmod(master,0666);
	chown(master, 0, 0);
	chmod(slave,0666);
	chown(slave, 0, 0);
	close(pty);
	getit();
#ifdef DEBUG
	if(trace_on)fprintf(stderr,"<=shakeup_pty");
#endif
}

void
closeup()
{
	/*
	 * close connection
	 */
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>closeup\n");
	sprintf(msg,"Connection state at closeup = %d\0",cnxn_open);
	if (dumps_on) log(msg);
#endif
	if(cnxn_open){
		sprintf(msg,"Shutting down network cnxn\0");
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
		shutdown(net, 2);
		close(net);
		shakeup_pty();
		cnxn_open = FALSE;
	}
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=closeup\n");
#endif

}

void
alarm_call()
{
	time_out = TRUE;
}

void
closedown()
{
	/*
	 * close down connection cleanly
	 */
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"=>closedown\n");
#endif
	if(cnxn_open){
		sprintf(msg,"Closing down network cnxn\0");
		syslog(LOG_INFO,msg);
#ifdef DEBUG
		if (track_on) log(msg);
#endif
                /*********************************************
                 *
                 * A6 -
                 * Send timing mark only if it hasn't been
                 * sent yet.  Normally, the timing mark will be
                 * sent after each file in 'TIOIOCTL' section.
                 *
                 *********************************************
                 *
		 *if ((timing_mark==TRUE) && (!TM_sent )) {
		 *     if( send_TM() < 0 )
                 *             shakeup_pty();
                 *     TM_sent = FALSE;
		 *}
                 *
                 ********************************************/
		shutdown(net, 2);
		close(net);
		cnxn_open = FALSE;
	}
#ifdef DEBUG
	if (trace_on) fprintf(stderr,"<=closedown\n");
#endif

}

void
log(log_msg)
char	*log_msg;
{
char *ascii_time;
time_t now;

	/*
	 * Optionally send message to log file
	 */
	now=time(NULL);
        ascii_time = ctime(&now);
        ascii_time += 4;
        *(ascii_time+16)= '\0';
	fprintf(stderr,"%s - %s\n",ascii_time , log_msg);
	fflush(stderr);
}

#ifdef DEBUG
void
toggle_tracx()
{
	trace_on = !trace_on;
	track_on = !track_on;
	dumps_on = !dumps_on;
	if (trace_on) syslog(LOG_INFO,"trace_on");
	else syslog(LOG_INFO,"trace_off");
	if (track_on) syslog(LOG_INFO,"track_on");
	else syslog(LOG_INFO,"track_off");
	if (dumps_on) syslog(LOG_INFO,"dumps_on");
	else syslog(LOG_INFO,"dumps_off");
}
#endif

void
dont_retry()
/*
 * Get out of connect or socket retry loop
 */
{
	s_tries = 0;		/* don't retry socket */
	c_tries = 0;		/* don't retry connect */
	get_out = TRUE;		/* get out of big loop */
}

void
Dump(buffer, length, ascii)
char    *buffer;
int     length;
char	ascii;
{
#   define BYTES_PER_LINE       32
#   define min(x,y)     ((x<y)? x:y)
    char *pThisc, *pThis;
    int offset;

    offset = 0;

    while (length) {
        /* print one line */
        fprintf(stderr, "0x%x\t", offset);
        pThis = buffer;
	pThisc = pThis;
        buffer = buffer+min(length, BYTES_PER_LINE);

        while (pThis < buffer) {
            fprintf(stderr, "%.2x", (*pThis)&0xff);
            pThis++;
        }
        fprintf(stderr, "\n");

	if (ascii){
        	fprintf(stderr, "0x%x\t", offset);
		while (pThisc < buffer) {
			fprintf(stderr, "%c", *pThisc);
			pThisc++;
		}
		fprintf(stderr, "\n");
	}

        length -= BYTES_PER_LINE;
        offset += BYTES_PER_LINE;
        if (length < 0) {
            fflush(stderr);
            return;
        }
        /* find next unique line */
    }
    fflush(stderr);
}

#ifdef DEBUG
void
printoption(direction, fmt, option, what)
        char *direction, *fmt;
        int option, what;
{
        fprintf(stderr, "%s ", direction+1);
        if (fmt == doopt)
                fmt = "do";
        else if (fmt == dont)
                fmt = "dont";
        else if (fmt == will)
                fmt = "will";
        else if (fmt == wont)
                fmt = "wont";
        else
                fmt = "???";
        if (option < (sizeof telopts/sizeof telopts[0]))
                fprintf(stderr, "%s %s", fmt, telopts[option]);
        else
                fprintf(stderr, "%s %d", fmt, option);
        if (*direction == '<') {
                fprintf(stderr, "\r\n");
                return;
        }
        fprintf(stderr, " (%s)\r\n", what ? "reply" : "don't reply");
}
#endif

/*********************************************************************
 |
 | A3 - Function to disable printers.  Called within do_status_request
 |      and send_TM().
 |
 *********************************************************************/

void
disable_printer()
{
 char printer_name[80];
 int system_error;

 sprintf(printer_name, "`lpstat -v | grep \'");
 strcat(printer_name, pseudonym);
 strcat(printer_name, "$\' | cut -d\":\" -f1 | cut -c12-50`");

 syslog(LOG_ERR, "Disabling printer ....");
 sprintf(msg,"/usr/bin/disable -r\"%s\" %s >/dev/null",reason_msg, printer_name);
 system_error = system(msg);

#ifdef DEBUG

 sprintf(msg, "system returned: %d  with errno = %d", system_error,errno);
 syslog(LOG_ERR, msg);

#endif
}

