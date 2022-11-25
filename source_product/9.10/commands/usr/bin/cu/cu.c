#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
static char *HPUX_ID = "@(#) $Revision: 72.2 $";

/*****************************************************************
 *    (c) Copyright 1985 Hewlett Packard Co.
 *        ALL RIGHTS RESERVED
 *****************************************************************/
/* Define HDBuucp to get HDB-UUCP compatible version */
#define ddt
/***************************************************************
 * cu [-s baud] [-l line] [-h] [-t] [-n] [-o | -e] [-q] telno | `dir` | remote
 *
 *	legal baud rates: 110,134,150,300,600,1200,2400,4800,9600.
 *
 *	-l is for specifying a line unit from the file whose
 *		name is defined under LDEVS below.
 *	-h is for half-duplex (local echoing).
 *      -q is used to initiate an enqack handshake instead of xon/xoff.
 *	-t is for adding CR to LF on output to remote (for terminals).
 *	-d can be used (with ddt) to get some tracing & diagnostics.
 *	-o or -e is for odd or even parity on transmission to remote.
 *	-n will request the phone number from the user.
 *	Telno is a telephone number with `=' for secondary dial-tone.
 *	If "-l dev" is used, speed is taken from LDEVS.
 *
 *	Escape with `~' at beginning of line.
 *	Silent output diversions are ~>:filename and ~>>:filename.
 *	Terminate output diversion with ~> alone.
 *	~. is quit, and ~![cmd] gives local shell [or command].
 *	Also ~$ for canned local procedure pumping remote.
 *	Both ~%put from [to]  and  ~%take from [to] invoke built-ins.
 *	Also, ~%break or just ~%b will transmit a BREAK to remote.
 *	~%nostop toggles on/off the DC3/DC1 input control from remote,
 *		(certain remote systems cannot cope with DC3 or DC1).
 *
 *	As a device-lockout semaphore mechanism, create an entry
 *	in the directory #defined as LOCK whose name is LCK..dev
 *	where dev is the device name taken from the "line" column
 *	in the file #defined as LDEVS.  Be sure to trap every possible
 *	way out of cu execution in order to "release" the device.
 *	This entry is `touched' from the dial() library routine
 *	every hour in order to keep uucp from removing it on
 *	its 90 minute rounds.  Also, have the system start-up
 *	procedure clean all such entries from the LOCK directory.
 ***************************************************************/
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/termio.h>
#include <sys/errno.h>
#ifdef HPIRS 
#include "dial.h"
#else
#include <dial.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SIGTSTP
#include <bsdtty.h>
#endif



#define LOW	300		/* default speed for modems */
#define HIGH	1200		/* high speed for modems */
#define HIGHER	4800
#define MID	BUFSIZ/2	/* mnemonic */
#define	RUB	'\177'		/* mnemonic */
#define	XON	'\21'		/* mnemonic */
#define	XOFF	'\23'		/* mnemonic */
#define	TTYIN	0		/* mnemonic */
#define	TTYOUT	1		/* mnemonic */
#define	TTYERR	2		/* mnemonic */
#define	YES	1		/* mnemonic */
#define	NO	0		/* mnemonic */
#define	EQUALS	!strcmp		/* mnemonic */
#define EXIT	0		/* exit code */
#define CMDERR	1		/* exit code */
#define NODIAL	2		/* exit code */
#define HUNGUP	3		/* exit code */
#define IOERR	4		/* exit code */
#define SIGQIT	5		/* exit code */
#define NOFORK	6		/* exit code */
#define	GETID	7		/* exit code */
#define	MAXN	100
#define ENQ '\5'
#define ACK '\6'


#define F_NAME 	0
#define	F_TIME	1
#define	F_LINE	2
#define	F_SPEED 3
#define	F_PHONE	4
#define	F_LOGIN	5

#define	CF_PHONE	-1
#define	SAME		0
#define	MAXPH		60
#define MAXC 	  	BUFSIZ	
#define SYSNSIZE        7
#define CF_SYSTEM	-1

#ifdef NLS
       nl_catd nlmsg_fd;
#endif
int trynum=0;
int sflag=0;
int spelfg=0;
int timeout;
int needack;
int ptim=2;     /* prompt timeout */
char pseq[4];   /* prompt sequence */
#ifdef SIGTSTP
int susp_char = 0x10000;  /* default susp char */
struct ltchars ltchars;
#endif SIGTSTP
char lsuf[4];   /* line suffix */
char modemtype[50]; /* type of acu found by finds() */
int download_flag=0;   /* flags if output diversion is active */
int download_fd;        /* file descriptor for output file */


extern int
	_debug,			/* flag for more diagnostics */
	_Debug,
	errno,			/* supplied by system interface */
	optind,			/* variable in getopt() */
	_dialit,                 /* flag to tell dial to use DIALIT */
	strcmp();		/* c-lib routine */

extern unsigned
	sleep();		/* c-lib routine */

extern char
	ttyname,
	*optarg;		/* variable in getopt() */
#ifdef HDBuucp
extern char
        _protocol[],
        _line_requested[];
#endif

extern char **copyenv();

static FILE *Lsysfp = NULL;

static struct termio tv, tv0;	/* for saving, changing TTY atributes */
static struct termio lv;	/* attributes for the line to remote */
static CALL call;		/* from dial.h */
static char device[15+sizeof(DEVDIR)];

static unsigned char
	cxc,			/* place into which we do character io*/
	tintr,			/* current input INTR */
	tquit,			/* current input QUIT */
	terase,			/* current input ERASE */
	tkill,			/* current input KILL */
	teol,			/* current sencondary input EOL */
	myeof;			/* current input EOF */

static int
	terminal=0,		/* flag; remote is a terminal */
        enqack=0,               /*'-q' option indicator for enqack*/
	echoe,			/* save users ECHOE bit */
	echok,			/* save users ECHOK bit */
	rlfd,			/* fd for remote comm line */
	child,			/* pid for recieve proccess */
	intrupt=NO,		/* interrupt indicator */
	duplex=YES,		/* half(NO), or full(YES) duplex */
	sstop=YES,		/* NO means remote can't XON/XOFF */
	rtn_code=0,		/* default return code */
	takeflag=NO,		/* indicates a ~%take is in progress */
	w_char(),		/* local io routine */
	r_char(),		/* local io routine */
	savuid,			/* save the uid */
	savgid,			/* save gid */
	savmode;		/* save the mode */

static
	onintrpt(),		/* interrupt routine */
	rcvdead(),		/* interrupt routine */
	hangup(),		/* interrupt routine */
	quit(),			/* interrupt routine */
	die(),                  /* interrupt routine */
	bye();			/* interrupt routine */

static void
	flush(),
	shell(),
	dopercen(),
	receive(),
	mode(),
	say(),
#ifdef ddt
	tdmp(),
#endif
	w_str();

char *	sysphone();

char	*msg[]= {
/*  0*/	 "usage: %s [-s baud] [-l line] [-h] [-n] [-t] [-d] [-m] [-o|-e] telno | 'dir' | remote\n",
/*  1*/	"interrupt",
/*  2*/	"dialer hung",
/*  3*/	"no answer",
/*  4*/	"illegal baud-rate",
/*  5*/	"acu problem",
/*  6*/	"line problem",
#ifdef HDBuucp 
/*  7*/	"can't open Devices file",
#else
/*  7*/	"can't open L-devices file",
#endif
/*  8*/	"Requested device not available\r\n",
/*  9*/	"Requested device/system name not known\r\n",
/* 10*/	"No device available at %d baud\r\n",
/* 11*/	"No device known at %d baud\r\n",
/* 12*/	"Connect failed: %s\r\n",
/* 13*/	"Cannot open: %s\r\n",
/* 14*/	"Line gone\r\n",
/* 15*/	"Can't execute shell\r\n",
/* 16*/	"Can't divert %s\r\n",
/* 17*/	"Use `~~' to start line with `~'\r\n",
/* 18*/	"character missed\r\n",
/* 19*/	"after %ld bytes\r\n",
/* 20*/	"%d lines/%ld characters\r\n",
/* 21*/	"File transmission interrupted\r\n",
/* 22*/	"Cannot fork -- try later\r\n",
/* 23*/	"\r\nCan't transmit special character `%#o'\r\n",
/* 24*/	"\nLine too long\r\n",
/* 25*/	"r\nIO error\r\n",
/* 26*/ "Use `~$'cmd \r\n",
/* 27*/ "Cannot obtain status of the terminal\r\n",
/* 28*/ "Sorry you cannot cu from 3B console\r\n",
/* 29*/ "requested speed does not match\r\n",
/* 30*/ "requested system name not known\r\n",
};

/* we need to do this for NLS message catalogs */
#define msg0	(catgets(nlmsg_fd,NL_SETN,1, "usage: %s [-s baud] [-l line] [-h] [-n] [-t] [-d level] [-m] [-o|-e] telno | 'dir' | remote\n"))
#define msg1	(catgets(nlmsg_fd,NL_SETN,2, "interrupt"))
#define msg2	(catgets(nlmsg_fd,NL_SETN,3, "dialer hung"))
#define msg3	(catgets(nlmsg_fd,NL_SETN,4, "no answer"))
#define msg4	(catgets(nlmsg_fd,NL_SETN,5, "illegal baud-rate"))
#define msg5	(catgets(nlmsg_fd,NL_SETN,6, "acu problem"))
#define msg6	(catgets(nlmsg_fd,NL_SETN,7, "line problem"))
#ifdef HDBuucp 
#define msg7	(catgets(nlmsg_fd,NL_SETN,9, "can't open Devices file"))
#else
#define msg7	(catgets(nlmsg_fd,NL_SETN,8, "can't open L-devices file"))
#endif
#define msg8	(catgets(nlmsg_fd,NL_SETN,10, "Requested device not available\r\n"))
#define msg9	(catgets(nlmsg_fd,NL_SETN,11, "Requested device/system name not known\r\n"))
#define msg10	(catgets(nlmsg_fd,NL_SETN,12, "No device available at %d baud\r\n"))
#define msg11	(catgets(nlmsg_fd,NL_SETN,13, "No device known at %d baud\r\n"))
#define msg12	(catgets(nlmsg_fd,NL_SETN,14, "Connect failed: %s\r\n"))
#define msg13	(catgets(nlmsg_fd,NL_SETN,15, "Cannot open: %s\r\n"))
#define msg14	(catgets(nlmsg_fd,NL_SETN,16, "Line gone\r\n"))
#define msg15	(catgets(nlmsg_fd,NL_SETN,17, "Can't execute shell\r\n"))
#define msg16	(catgets(nlmsg_fd,NL_SETN,18, "Can't divert %s\r\n"))
#define msg17	(catgets(nlmsg_fd,NL_SETN,19, "Use `~~' to start line with `~'\r\n"))
#define msg18	(catgets(nlmsg_fd,NL_SETN,20, "character missed\r\n"))
#define msg19	(catgets(nlmsg_fd,NL_SETN,21, "after %ld bytes\r\n"))
#define msg20	(catgets(nlmsg_fd,NL_SETN,22, "%d lines/%ld characters\r\n"))
#define msg21	(catgets(nlmsg_fd,NL_SETN,23, "File transmission interrupted\r\n"))
#define msg22	(catgets(nlmsg_fd,NL_SETN,24, "Cannot fork -- try later\r\n"))
#define msg23	(catgets(nlmsg_fd,NL_SETN,25, "\r\nCan't transmit special character `%#o'\r\n"))
#define msg24	(catgets(nlmsg_fd,NL_SETN,26, "\nLine too long\r\n"))
#define msg25	(catgets(nlmsg_fd,NL_SETN,27, "r\nIO error\r\n"))
#define msg26   (catgets(nlmsg_fd,NL_SETN,28, "Use `~$'cmd \r\n"))
#define msg27   (catgets(nlmsg_fd,NL_SETN,29, "Cannot obtain status of the terminal\r\n"))
#define msg28   (catgets(nlmsg_fd,NL_SETN,30, "Sorry you cannot cu from 3B console\r\n"))
#define msg29   (catgets(nlmsg_fd,NL_SETN,31, "requested speed does not match\r\n"))
#define msg30   (catgets(nlmsg_fd,NL_SETN,32, "requested system name not known\r\n"))

/***************************************************************
 *	main: get command line args, establish connection, and fork.
 *	Child invokes "receive" to read from remote & write to TTY.
 *	Main line invokes "transmit" to read TTY & write to remote.
 ***************************************************************/

char **original_environ;

main(argc, argv, environ)
int argc;
char *argv[];
char **environ;
{
	struct stat bufsave;
	char s[40];
	char *string = NULL;
	char *phdir;          /* Pointer to phone number = -1 if bad */
	int getp;
	int getu;
	int i;
	int errflag=0;
	int nflag=0;
        int syserr=0;
	char *getenv();

	original_environ = copyenv(environ);

        cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", 0 );

	if (original_environ == NULL)
		original_environ = environ;

#ifdef NLS
	nlmsg_fd = catopen("cu",0);
#endif
	ptim=2;
	pseq[0]='\021';
	pseq[1]='\0';
	strcpy(lsuf, "\r");

	lv.c_iflag = (IGNPAR | IGNBRK | IXON | IXOFF);
	ioctl(0, TCGETA, &tv0);
	if (tv0.c_iflag & ISTRIP)
	    lv.c_iflag |= ISTRIP;
	lv.c_cc[VMIN] = '\1';
/*      lv.c_cc[VTIME] = '\4'; */

	_dialit=1;

	call.attr = &lv;
	call.baud = -1;
	call.speed = LOW;
	call.line = NULL;
	call.telno = NULL;
	call.modem = 0;
	call.device = device;
	call.dev_len = sizeof(device);
		
	while((i = getopt(argc, argv, "hqtceomnd:s:l:x:")) != EOF)
		switch(i) {
			case 'd':
			case 'x':
#ifdef	ddt
				_debug = YES;
				_Debug = atoi(optarg);
#else
				++errflag;
				say((catgets(nlmsg_fd,NL_SETN,33, "Cu not compiled with debugging flag\r\n")));
#endif
				break;
			case 'h':
				duplex ^= YES;
				lv.c_iflag &= ~(IXON | IXOFF);
				sstop = NO;
				break;
			case 't':
				terminal = YES;
				lv.c_oflag |= (OPOST | ONLCR);
				break;
			case 'e':
				if(lv.c_cflag & PARENB)
					++errflag;
				else
					goto PAROUT;
				break;
			case 'o':
				if(lv.c_cflag & PARENB)
					++errflag;
				else
					lv.c_cflag = PARODD;
			PAROUT:
					lv.c_cflag |= (CS7 | PARENB);
				break;
			case 's':
				sflag++;
				call.baud = atoi(optarg);
				if(call.baud > LOW)
					call.speed = call.baud;
			  	break;
			case 'l':
				call.line = optarg;
#ifdef HDBuucp
				strcpy(_line_requested,call.line);
#endif
			  	break;
			case 'm':
				call.modem = 1;  /* override modem control */
				break;
			case 'n':
				nflag++;
				printf((catgets(nlmsg_fd,NL_SETN,34, "Please enter the number: ")));
				gets(s);
				break;
                        case 'q':                        /*enqack handshake*/
                                enqack++;
                                break;
			case '?':
				++errflag;
		}

#ifdef  u3b
	if(fstat(1, &buff) < 0) {
		say(msg27);
		exit(1);
	} else if(buff.st_rdev == 0) {
		say(msg28);
		exit(1);
		}
#endif

	if(call.baud == -1 && call.line == NULL)
		call.baud = LOW;

	if((optind < argc && optind > 0) || (nflag && optind > 0)) {  
		if(nflag) 
			string=s;
		else
			string = argv[optind];
		if(strlen(string) == strspn(string, "0123456789=-*:#;f")) {
			call.telno = string;
		} else {
			if(EQUALS(string, "dir")) {
				if(call.line == NULL)
					++errflag;
			} else {                         /* else not direct...*/
				phdir = sysphone(string);
				if (modemtype[0])
				    strcpy(call.device, modemtype);
				if ( ((int) phdir == 0) || ((int) phdir == -1) )
				{
/* if no phone #     */                 errflag++;
                                        syserr++;
/*   then an error   */         }
				else
/* If phone # exists */         {
/* will try numbers  */                 trynum++;
#ifdef HDBuucp
                                        call.telno=phdir;
                                        if (phdir==NULL) errflag++;
                                 }
#else
/* Check number      */                 if(strspn(phdir, "0123456789=-*:#;f"))
/* If this is a #    */                 {
/*   store the #     */                         call.telno=phdir;
/*   If no # stored  */                         if(call.telno == NULL)
/*    then an error  */                                 ++errflag;
/* else we got a line*/                 } else {
                                            if (strlen(phdir)>0)
/*   store line      */                         call.line=phdir;
/*   if bad line     */                         if(phdir==NULL)
/*    then an error  */                                 ++errflag;
/* End normal code   */                 }
/* end if (phdir)    */         }
#endif
/* End NotDirect else*/}
	 	}
	} else
		if(call.line == NULL)
			++errflag;
	
        if (syserr){
                  say(msg30,argv[0]);
                  exit(1);
        }
	if(errflag) {
		say(msg0, argv[0]);
		exit(1);
	}

	(void)ioctl(TTYIN, TCGETA, &tv0); /* save initial tty state */
	tintr = tv0.c_cc[VINTR]? tv0.c_cc[VINTR]: '\377';
	tquit = tv0.c_cc[VQUIT]? tv0.c_cc[VQUIT]: '\377';
	terase = tv0.c_cc[VERASE]? tv0.c_cc[VERASE]: '\377';
	tkill = tv0.c_cc[VKILL]? tv0.c_cc[VKILL]: '\377';
	teol = tv0.c_cc[VEOL]? tv0.c_cc[VEOL]: '\377';
	myeof = tv0.c_cc[VEOF]? tv0.c_cc[VEOF]: '\04';
	echoe = tv0.c_lflag & ECHOE;
	echok = tv0.c_lflag & ECHOK;
#ifdef SIGTSTP
        if (signal(SIGTSTP,SIG_IGN) != SIG_IGN){
               signal(SIGTSTP,SIG_DFL);
	       if(ioctl(TTYIN, TIOCGLTC, &ltchars) != -1){
                        if (ltchars.t_suspc != 0377){
        			susp_char = ltchars.t_suspc;
                        }
                }
        }
#endif

	(void)signal(SIGHUP, hangup);
	(void)signal(SIGQUIT, hangup);
	(void)signal(SIGINT, onintrpt);
	(void)signal(SIGCLD, SIG_DFL);

	if (call.telno == NULL)
		_dialit=0;
	if (_dialit)
		say((catgets(nlmsg_fd,NL_SETN,35, "Autodialing - please wait\r\n")));
        if (!strcmp(call.device,string)) call.line = NULL;
	/*if((rlfd = dial(call)) < 0) { This just try 2 times !! */
	while ((rlfd = dial(call)) < 0) {
		if(trynum && (rlfd != INTRPT)) { /*Try the next possible number from L.sys*/
			phdir=sysphone(string);
			if (modemtype[0])
			    strcpy(call.device, modemtype);
			if ((int) phdir != CF_PHONE) {
				call.telno = NULL;
				call.line = NULL;
				/*strcpy(call.device,"");*/
                                if (!strcmp(call.device,string)) call.line = NULL;
#ifdef HDBuucp
                              call.telno = phdir;
                              if (call.telno == NULL) ++errflag;
#else
				if(strspn(phdir, "0123456789-=*:#;f")) {
					call.telno=phdir;
					if(call.telno == NULL)
						++errflag;
				} else {
                                        if (strlen(phdir)>0) 
						call.line=phdir;
					if (phdir==NULL)
						++errflag;
				}
#endif
				if(errflag) {
					say(msg0, argv[0]);
					exit(1);
				}
				/*rlfd=dial(call);*/
			}else break; /* No more phone numbers or lines */
		}else break; /* user specified a line or a phone.Shouldn't
                              try again */
	}
	if(rlfd < 0) {
			if(rlfd == NO_BD_A || rlfd == NO_BD_K)  {
                              if (rlfd == NO_BD_A)
				say(msg10, call.baud == -1 ? LOW : call.baud);
			      else
				say(msg11, call.baud == -1 ? LOW : call.baud);
			}else if(rlfd == DV_NT_E)
				say(msg29);
			else
				say(msg12, msg[-rlfd]);
				exit(NODIAL);
	}


/*	This section of the code will provide  a security to cu. 
	The device used by cu will have the owner and group id of
	the user. When cu is ready to be disconnected it will set the 
	owner and group id of the device as it was found, done in the
	hangup().
*/

	getu = getuid();
	getp = getgid();
	if(stat(call.device, &bufsave) <0)  {
		say((catgets(nlmsg_fd,NL_SETN,36, "Cannot get the status of the device")));
		undial(rlfd);
		exit(1);
	}
	savmode = bufsave.st_mode;
	savuid = bufsave.st_uid;
	savgid = bufsave.st_gid;
	if(chown(call.device, getu, getp) <0) {
		say((catgets(nlmsg_fd,NL_SETN,37, "Cannot chown on %s\n")), call.device);
		undial(rlfd);
		exit(1);
	}
	if(chmod(call.device, 0600) <0) {
		say((catgets(nlmsg_fd,NL_SETN,38, "Cannot chmod on %s\n")), call.device);
		undial(rlfd);
		exit(1);
	}
	if(setuid(getu) < 0) {
		say((catgets(nlmsg_fd,NL_SETN,39, "Cannot set the uid %d\n")), getu);
		undial(rlfd);
		exit(1);
	}

	/* When we get this far we have an open communication line */
	mode(1);			/* put terminal in `raw' mode */

	if (Lsysfp != NULL)
		(void) fclose(Lsysfp);
	say((catgets(nlmsg_fd,NL_SETN,40, "Connected\007\r\n")));

	{
	    struct termio foo;
	    ioctl(rlfd, TCGETA, &foo);
	    foo.c_cc[VMIN] = 1;
	    foo.c_cc[VTIME] = 1;
	    ioctl(rlfd, TCSETAW, &foo);
	}

	recfork("");              /* checks for child == 0 */
	if(child > 0) {
		(void)signal(SIGUSR1, bye);
		(void)signal(SIGHUP, die);
		(void)signal(SIGQUIT, onintrpt);
		(void) signal(SIGTERM, die);
		(void) signal(SIGPIPE, die);
		rtn_code = transmit();
		quit(rtn_code);
	} else {
		hangup(rlfd);
	}
}

/*
 *	Kill the present child, if it exists, then fork a new one.
 */

recfork(name)
char *name;
{
	if (child) {
	    kill(child, SIGKILL);
	    wait( (int *) 0);
	}
	child = dofork();
	if(child == 0) {
		(void)signal(SIGHUP, rcvdead);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		if (*name) {
			char str[72];
			strcpy(str,"exec ");
			strcat(str, name);
			close(0);
			dup(rlfd);
#ifdef V4FS
			execlp("/usr/bin/sh", "sh", "-c", str, 0);
#else /* V4FS */
			execlp("/bin/sh", "sh", "-c", str, 0);
#endif /* V4FS */
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,41, "Exec of '%s' failed\r\n")),name);
		}
		receive();	/* This should run until killed */
		/*NOTREACHED*/
	}
}

/***************************************************************
 *	transmit: copy stdin to rlfd, except:
 *	~.	terminate
 *	~!	local login-style shell
 *	~!cmd	execute cmd locally
 *	~$proc	execute proc locally, send output to line
 *	~%cmd	execute builtin cmd (put, take, or break)
 ****************************************************************/

int
transmit()
{
	char b[BUFSIZ];
	char prompt[10];
	register char *p;
	register int escape;
	int ret=YES;

#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,42, "transmit started\n\r")));
#endif
	sysname(prompt);
	while(ret == YES) {
		p = b;
		while((ret = r_char(TTYIN)) == YES) {
			if(p == b)  	/* Escape on leading  ~    */
				escape = (cxc == '~');
			if(p == b+1)   	/* But not on leading ~~   */
				escape &= (cxc != '~');
			if(escape) {
				if(cxc == '\n' || cxc == '\r' || cxc == teol) {
					*p = '\0';
					if(tilde(b+1) == YES)
						return(EXIT);
					break;
				}
				if(cxc == tintr || cxc == tkill || cxc == tquit ||
					    (intrupt && cxc == '\0')) {
					if(!(cxc == tkill) || echok)
						say("\r\n");
					break;
				}
				if(p == b+1 && cxc != terase)
					say("[%s]", prompt);
				if(cxc == terase) {
					p = (--p < b)? b:p;
					if(p > b)
						if(echoe)
							say("\b \b");
						else
						 (void)w_char(TTYOUT);
				} else {
					(void)w_char(TTYOUT);
					if(p-b < BUFSIZ) 
						*p++ = cxc;
					else {
						say(msg24);
						break;
					}
				}
			} else {
				if(intrupt && cxc == '\0') {
#ifdef	ddt
					if(_debug == YES)
						say((catgets(nlmsg_fd,NL_SETN,43, "got break in transmit\n\r")));
#endif
					intrupt = NO;
					(void)ioctl(rlfd, TCSBRK, 0);
					flush();
					break;
				}
				if(w_char(rlfd) == NO) {
					say(msg14);
					return(IOERR);
				}
				if(duplex == NO)
					if(w_char(TTYERR) == NO)
						return(IOERR);
				if ( (cxc == tintr) || (cxc == tquit) ) {
#ifdef	ddt
					if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,44, "got a tintr\n\r")));
#endif
					flush();
					break;
				}
				if(cxc == '\n' || cxc == '\r' ||
						cxc == teol || cxc == tkill) {
					takeflag = NO;
					break;
				}
				p = (char*)0;
			}
		}
	}
	return ret;
}

/***************************************************************
 *	routine to halt input from remote and flush buffers
 ***************************************************************/
static void
flush()
{
#ifdef ddt
	if (_Debug == 13)
		return;
#endif
	(void)ioctl(TTYOUT, TCXONC, 0);	/* stop tty output */
	(void)ioctl(rlfd, TCFLSH, 0);		/* flush remote input */
	(void)ioctl(TTYOUT, TCFLSH, 1);	/* flush tty output */
	(void)ioctl(TTYOUT, TCXONC, 1);	/* restart tty output */
	if(takeflag == NO) {
		return;		/* didn't interupt file transmission */
	}
	say(msg21);
	(void)sleep(3);
	w_str("echo '\n~>\n';mesg y;stty echo\n");
	takeflag = NO;
}

/**************************************************************
 *	command interpreter for escape lines
 **************************************************************/
int
tilde(cmd)
char	*cmd;
{
	say("\r\n");
#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,46, "call tilde(%s)\r\n")), cmd);
#endif
#ifdef SIGTSTP
        if (cmd[0] == susp_char ){
                  mode(0);
                  kill(0,SIGTSTP);
                  mode(1);
                  return(NO);
        }
#endif
	switch(cmd[0]) {
		case '.':
			if(call.telno == NULL)
				if(cmd[1] != '.')
					w_str("\04\04\04\04\04");
			return(YES);
		case '!':
			shell(cmd);	/* local shell */
			say("\r%c\r\n", *cmd);
			break;
		case '&':
			shell(cmd);	/* Local shell */
			say("\r%c\r\n", *cmd);
			break;
		case '|':
			recfork(cmd+1); /* fork new receiver */
			break;
		case '$':
			if(cmd[1] == '\0')
				say(msg26);
			else {
				shell(cmd);	/* Local shell */
				say("\r%c\r\n", *cmd);
			}
			break;
		case '%':
			dopercen(++cmd);
			break;
#ifdef ddt
		case 't':
			tdmp(TTYIN);
			break;
		case 'l':
			tdmp(rlfd);
			break;
#endif
		default:
			say(msg17);
	}
	return(NO);
}

/***************************************************************
 *	The routine "shell" takes an argument starting with
 *	either "!" or "$", and terminated with '\0'.
 *	If $arg, arg is the name of a local shell file which
 *	is executed and its output is passed to the remote.
 *	If !arg, we escape to a local shell to execute arg
 *	with output to TTY, and if arg is null, escape to
 *	a local shell and blind the remote line.  In either
 *	case, RUBout or '^D' will kill the escape status.
 **************************************************************/

static void
shell(str)
char	*str;
{
	int	fk, (*xx)(), (*yy)();
	char *shelly;

#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,47, "call shell(%s)\r\n")), str);
#endif
	fk = dofork();
	if(fk < 0)
		return;
	mode(0);	/* restore normal tty attributes */
	xx = signal(SIGINT, SIG_IGN);
	yy = signal(SIGQUIT, SIG_IGN);
	if(fk == 0) {
		/***********************************************
		 * Hook-up our "standard output"
		 * to either the tty or the line
		 * as appropriate for '!' or '$'
		 ***********************************************/
		(void)close(TTYOUT);
		(void)fcntl((*str == '$')? rlfd:TTYERR,F_DUPFD,TTYOUT);
		(void)close(rlfd);
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGHUP, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		(void)signal(SIGUSR1, SIG_DFL);
		(void) signal(SIGTERM, SIG_DFL);
		(void) signal(SIGPIPE, SIG_DFL);
		if ( *str == '&' ) {
			if (child) {
			    kill(child, SIGKILL);
			    wait( (int *) 0);
			}
			child = 0;
		}
		/* at this point our original environment is back */
		if ( (shelly = getenv("SHELL")) == NULL)
#ifdef V4FS
			shelly = "/usr/bin/sh";
#else /* V4FS */
			shelly = "/bin/sh";
#endif /* V4FS */
		if(*++str == '\0')
			(void)execl(shelly,"",(char*)0,(char*)0,0);
		else
#ifdef V4FS
			(void)execl("/usr/bin/sh","sh","-c",str,0);
#else /* V4FS */
			(void)execl("/bin/sh","sh","-c",str,0);
#endif /* V4FS */
		say(msg15);
		exit(0);
	}
	signal(SIGINT, SIG_IGN);
	while(wait((int*)0) != fk);
	(void)signal(SIGINT, xx);
	(void)signal(SIGQUIT, yy);
	mode(1);
	if ( *str == '&' )
		recfork("");
}


/***************************************************************
 *	This function implements the 'put', 'take', 'break', and
 *	'nostop' commands which are internal to cu.
 ***************************************************************/

static void
dopercen(cmd)
register char *cmd;
{
	char	*arg[5];
	char	*getpath, *getenv();
	char	mypath[80];
	int	narg;

	blckcnt((long)(-1));

#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,48, "call dopercen(\"%s\")\r\n")), cmd);
#endif
	arg[narg=0] = strtok(cmd, " \t\n");
	/* following loop breaks out the command and args */
	if (arg[0])
	    while((arg[++narg] = strtok((char*) NULL, " \t\n")) != NULL) {
		    if(narg < 5)
			    continue;
		    else
			    break;
	    }
	else
	    arg[0]=" ";

	if(EQUALS(arg[0],  "take")) {
		if(narg < 2 || narg > 3) {
			say((catgets(nlmsg_fd,NL_SETN,50, "usage: ~%%take from [to]\r\n")));
			return;
		}
		if(narg == 2)
			arg[2] = arg[1];
		w_str("stty -echo;mesg n;echo '~>':");
		w_str(arg[2]);
		w_str(";cat ");
		w_str(arg[1]);
		w_str(";echo '~>';mesg y;stty echo\n");
		takeflag = YES;
		return;
	}
	else if(EQUALS(arg[0], "put")) {
		FILE	*file;
		char	ch, buf[BUFSIZ], spec[NCC+1], *b, *p, *q;
		int	i, j, len, tc=0, lines=0;
		long	chars=0L;

		if(narg < 2 || narg > 3) {
			say((catgets(nlmsg_fd,NL_SETN,55, "usage: ~%%put from [to]\r\n")));
			goto R;
		}
		if(narg == 2)
			arg[2] = arg[1];

		if((file = fopen(arg[1], "r")) == NULL) {
			say(msg13, arg[1]);
R:
			w_str("\n");
			return;
		}
		w_str( "stty -echo; cat - > ");
		w_str(arg[2]);
		w_str( "; stty echo\n");
		intrupt = NO;
		for(i=0,j=0; i < NCC; ++i)
			if((ch=tv0.c_cc[i]) != '\0')
				spec[j++] = ch;
		spec[j] = '\0';
		mode(2);
		(void)sleep(5);
		while(intrupt == NO &&
				fgets(b= &buf[MID],MID,file) != NULL) {
			len = strlen(b);
			chars += len;		/* character count */
			p = b;
			while(q = strpbrk(p, spec)) {
				if(*q == tintr || *q == tquit ||
							*q == teol) {
					say(msg23, *q);
					(void)strcpy(q, q+1);
					intrupt = YES;
				}
				b = strncpy(b-1, b, q-b);
				*(q-1) = '\\';
				p = q+1;
			}
			if((tc += len) >= MID) {
				(void)sleep(1);
				tc = len;
			}
			if(write(rlfd, b, (unsigned)strlen(b)) < 0) {
				say(msg25);
				intrupt = YES;
				break;
			}
			++lines;		/* line count */
			blckcnt((long)chars);
		}
		mode(1);
		blckcnt((long)(-2));		/* close */
		(void)fclose(file);
		if(intrupt == YES) {
			intrupt = NO;
			say(msg21);
			w_str("\n");
			say(msg19, ++chars);
		} else
			say(msg20, lines, chars);
		w_str("\04");
		(void)sleep(3);
		return;
	}
	else if(EQUALS(arg[0], "b") || EQUALS(arg[0], "break")) {
		(void)ioctl(rlfd, TCSBRK, 0);
		return;
	}
	else if(EQUALS(arg[0], "nostop")) {
		(void)ioctl(rlfd, TCGETA, &tv);
		if(sstop == NO)
			tv.c_iflag |= IXOFF;
		else
			tv.c_iflag &= ~IXOFF;
		(void)ioctl(rlfd, TCSETAW, &tv);
		sstop = !sstop;
		mode(1);
		return;
	}
	else if ( arg[0][0] == '<' ) {
	    struct termio foo, bar;
	    char file[50], fbuf[300];
	    FILE *fp;

	    if (narg == 1)
		strcpy(file, arg[0]+1);
	    else
		strcpy(file, arg[1]);
	    fp=fopen(file, "r");
	    if (fp==NULL) {
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,58, "Could not open file %s\r\n")),file);
		return;
	    }
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,59, "Input Diversion starting.\r\n")));
	    kill(child, SIGKILL);
	    wait( (int *) 0);
	    mode(2);
	    if (pseq[0] == '\021') {
		ioctl(rlfd, TCGETA, &foo);
		ioctl(rlfd, TCGETA, &bar);
		bar.c_iflag &= ~IXON;
		ioctl(rlfd, TCSETAW, &bar);
	    }
	    while ( fgets(fbuf, 250, fp) != NULL ) {
		if (intrupt == YES) {
		    intrupt=NO;
		    break;
		}
		sendit(fbuf);       /* send the line */
		readit();           /* read the echo & prompt sequence */
	    }
	    fclose(fp);
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,60, "Input Diversion ended.\r\n")));
	    if (pseq[0] == '\021')
		ioctl(rlfd, TCSETAW, &foo);
	    mode(1);
	    recfork("");
	    return;
	}
	else if(EQUALS(arg[0], "setps")) {
	    char *s;
	    int i;
	    s=arg[1];
	    for (i=0; i<=1; i++, s++) {
		if ( s == NULL )
		    break;
		switch ( *s ) {
		    case '\\':  pseq[i] = *(++s);
				break;
		    case '^':   pseq[i] = *(++s) & 037;
				break;
		    default:    pseq[i] = *s;
		}
	    }
	    pseq[i]='\0';
	    return;
	}
	else if(EQUALS(arg[0], "setel")) {
	    char *s;
	    int i;
	    s=arg[1];
	    for (i=0; i<=1; i++, s++) {
		if ( s == NULL )
		    break;
		switch ( *s ) {
		    case '\\':  lsuf[i] = *(++s);
				break;
		    case '^':   lsuf[i] = *(++s) & 037;
				break;
		    default:    lsuf[i] = *s;
		}
	    }
	    lsuf[i]='\0';
	    return;
	}
	else if(EQUALS(arg[0], "setpt")) {
	    ptim=atoi(arg[1]);
	    return;
	}
	else if(EQUALS(arg[0], "set")) {
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,61, "Prompt = \\%03o \\%03o\r\n")), pseq[0], pseq[1]);
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,62, "Timeout = %d s.\r\n")), ptim);
	    if (*lsuf != '\r')
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,63, "Line suffix = \\%03o \\%03o\r\n")), lsuf[0], lsuf[2]);
	    return;
	}
	else if ( arg[0][0] == '>' ) {
	    char file[50];
	    int append;
	    append = 0;
	    if (arg[0][1] == '>')
		append++;
	    if (narg == 1)
		strcpy(file, arg[0] + 1 + append);
	    else
		strcpy(file, arg[1]);
	    if (!file[0])             /* no arg specified, just ~%> */
		if (download_flag) {    /* terminate diversion */
		    download_flag=0;
		    kill(child, SIGUSR1);
		    close(download_fd);
		    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,64, "Output Diversion ended.\r\n")));
		    return;
		}
		else {          /* got ~%> but no diversion active */
		    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,65, "No Output Diversion active.\r\n")));
		    return;
		}
	    /* at this point, we know a file was specified in ~%>file */
	    if (download_flag) {    /* got ~%>file, and diversion */
				    /* already in progress. so kill old */
		kill(child, SIGUSR1);
		close(download_fd);
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,66, "Output Diversion ended.\r\n")));
	    }
	    if (append)
		download_fd = open(file, O_CREAT | O_APPEND | O_WRONLY, 0777);
	    else
		download_fd=creat(file, 0777);
	    if (download_fd < 0) {
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,67, "Could not open file %s.\r\n")), file);
		download_flag=0;
		return;
	    }
	    download_flag=1;
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,68, "Starting Output Diversion.\r\n")));
	    recfork("");
	    return;
	}
	else if(EQUALS(arg[0], "cd")) {
		/* Change local current directory */
		if (narg < 2) {
			getpath = getenv("HOME");
			strcpy(mypath, getpath);
			if(chdir(mypath) < 0)
				say((catgets(nlmsg_fd,NL_SETN,69, "Cannot change to %s\r\n")), mypath);
	/*              w_str("\r");            */
			say("\r\n");
			return;
		}
		if (chdir(arg[1]) < 0) {
			say((catgets(nlmsg_fd,NL_SETN,70, "Cannot change to %s\r\n")), arg[1]);
			return;
		}
		recfork("");      /* fork a new child so it know about change */
		w_str("\r");
		return;
	}
	say((catgets(nlmsg_fd,NL_SETN,71, "~%%%s unknown to cu\r\n")), arg[0]);
}


sendit(buffer)
char buffer[];
{
    int l1,l2;

    l1=strlen(buffer);
    if (buffer[l1-1] == '\n')
	buffer[--l1]='\0';
    l2=write(rlfd, buffer, l1);
    if (l1 != l2)
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,72, "write failed\r\n")));
    if (*lsuf)
	write(rlfd, lsuf, strlen(lsuf));
    return;
}

int_alarm()
{
    timeout=1;
    return;
}

/*
**	Reads the response of the remote node for the request sent.
**	Returns when there is a time out. If a hangup signal or a 
**	read error occurs, it will hangup the line and exit.
*/
 
readit()
{
    int r;
    char c, buf[300];
    int i, savalarm, (*astat)(), int_alarm();
    int save_err = 0;

    timeout=0;

    while (!timeout) {
	astat = signal(SIGALRM, int_alarm);
	savalarm = alarm(ptim);
	r = read(rlfd, buf, 250);
	if (r <= 0)
		save_err = errno;
	signal(SIGALRM, astat);
	alarm(savalarm);
	if (r <= 0)	{
	    if (timeout)
		return;
	    else if (save_err == EINTR)	{ /* There was a Hangup interrupt */
					  /* while reading.		 */
#ifdef ddt
		if (_Debug == 4)
			fprintf(stderr, "\nGot an Hangup interrupt. errno = %d.\n", save_err);
#endif
		if (child) { /* If a receive process exists, kill it. */
	    		kill(child, SIGKILL);
	    		wait( (int *) 0); /* Wait till child dies */
		}
		hangup(-10); /* Hang up the line. */
	    }  else { 	     /* There was a read error	*/
#ifdef ddt
		if (_Debug == 4)
			fprintf(stderr, "\nRead error. errno = %d.\n", save_err);
#endif
		if (child) { /* Kill the receive process if it exists */
	    		kill(child, SIGKILL);
	    		wait( (int *) 0); /* Wait for the child to die */
		}
		hangup(-10); /* Hang the line */
	    }
	}
	if (enqack) {
	    needack=0;
	    r = chenq(buf, r);
	    if (needack) {
		c=ACK;
		if (write(rlfd, &c, 1) <= 0)	{
#ifdef ddt
			if (_Debug == 4)
				fprintf(stderr, "\nWrite Error. errno = %d.\n", errno);
#endif
			if (child) { /* Could not write to remote node so */
				     /* kill receive process if it exists */
				     /* and hang up the line.		  */

	    			kill(child, SIGKILL);
	    			wait( (int *) 0);
			}
			hangup(-10);
		}
	    }
	}
	r=process_crlf(buf, r);
	write(1, buf, r);
	if (download_flag)
	    write(download_fd, buf, r);

	for (i=0; i<=r; i++)
	    if (got_prompt(buf[i]))
		return(1);
	if (ptim == 0)
		int_alarm();
    }
}

int process_crlf(lines, len)
char *lines; int len;      /* change \n to \r\n */
{
   char plugh[250];
   int i,j;

   j=0;
   for (i=0; i<len; i++) {
	if (lines[i] == '\n')
		plugh[j++] = '\r';
	plugh[j++] = lines[i];
   }
   for (i=0; i<j; i++) lines[i] = plugh[i];
   return(j);
}


int chenq(buf, leng)
char *buf;
int leng;
{
    int m,n;
    char tmpbuf[300];

    for (m=0; m < leng; m++)
	    tmpbuf[m] = buf[m];
    for (m=0,n=0; m < leng ; m++)
	    if (tmpbuf[m] != 5)
		    buf[n++]=tmpbuf[m];
	    else
		    needack++;
    buf[n]='\0';
    leng = n;
    return(leng);
}



int got_prompt(b)
char b;
{
    static int waiting = 0;

    if ( !pseq[0] )
	return(1);
    if ( b == pseq[0] ) {
	if (pseq[1]) {
	    waiting = 1;
	}
	else {
	    waiting = 0;
	    return(1);
	}
    }
    else if ( waiting && (b == pseq[1]) ) {
	waiting = 0;
	return(1);
    }
    else
	waiting = 0;

    return(0);

}

/***************************************************************
 *	receive: read from remote line, write to fd=1 (TTYOUT)
 *	catch:
 *	~>[>]:file
 *	.
 *	. stuff for file
 *	.
 *	~>	(ends diversion)
 ***************************************************************/


static int download_close();

static void
receive()
{
	register silent=NO, file;
	register char *p;
	int	tic;
	char	b[BUFSIZ];
	long	lseek(), count;

#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,73, "receive started\r\n")));
#endif
	file = -1;
	p = b;
	signal(SIGUSR1, download_close);
	while(r_remote() == YES) {
/*      while(r_char(rlfd) == YES) {            */
		if (enqack) {               /*see if enqack turned on*/
			if (cxc == ENQ) {  /*if char is ENQ send ACK*/
				cxc = ACK;
				if (w_char(rlfd) == NO)
					return;
				continue;         /* get next character */
			}
                }
		if(silent == NO)
			if(w_remote(0) == NO)
/*                      if(w_char(TTYOUT) == NO)    */
				rcvdead(IOERR);	/* this will exit */
		/* remove CR's and fill inserted by remote */
		if(cxc == '\0' || cxc == '\177' || cxc == '\r')
			continue;
		*p++ = cxc;
		if(cxc != '\n' && (p-b) < BUFSIZ)
			continue;
		/***********************************************
		 * The rest of this code is to deal with what
		 * happens at the beginning, middle or end of
		 * a diversion to a file.
		 ************************************************/
		if(b[0] == '~' && b[1] == '>') {
			/****************************************
			 * The line is the beginning or
			 * end of a diversion to a file.
			 ****************************************/
			if((file < 0) && (b[2] == ':' || b[2] == '>')) {
				/**********************************
				 * Beginning of a diversion
				 *********************************/
				int	append;

				*(p-1) = NULL; /* terminate file name */
				append = (b[2] == '>')? 1:0;
				p = b + 3 + append;
				if(append && (file=open(p,O_WRONLY))>0)
					(void)lseek(file, 0L, 2);
				else
					file = creat(p, 0666);
				if(file < 0) {
					say(msg16, p);
					perror("");
					(void)sleep(10);
				} else {
					silent = YES; 
					count = tic = 0;
				}
			} else {
				/*******************************
				 * End of a diversion (or queer data)
				 *******************************/
				if(b[2] != '\n')
					goto D;		/* queer data */
				if(silent = close(file)) {
					say(msg16, b);
					silent = NO;
				}
				blckcnt((long)(-2));
				say("~>\r\n");
				say(msg20, tic, count);
				file = -1;
			}
		} else {
			/***************************************
			 * This line is not an escape line.
			 * Either no diversion; or else yes, and
			 * we've got to divert the line to the file.
			 ***************************************/
D:
			if(file > 0) {
				(void)write(file, b, (unsigned)(p-b));
				count += p-b;	/* tally char count */
				++tic;		/* tally lines */
				blckcnt((long)count);
			}
		}
		p = b;
	}
	say((catgets(nlmsg_fd,NL_SETN,74, "\r\nLost Carrier\r\n")));
	rcvdead(IOERR);
}

/***************************************************************
 *	change the TTY attributes of the users terminal:
 *	0 means restore attributes to pre-cu status.
 *	1 means set `raw' mode for use during cu session.
 *	2 means like 1 but accept interrupts from the keyboard.
 ***************************************************************/
static void
mode(arg)
{
#ifdef	ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,75, "call mode(%d)\r\n")), arg);
#endif
	if(arg == 0) {
		(void)ioctl(TTYIN, TCSETAW, &tv0);
	} else {
		(void)ioctl(TTYIN, TCGETA, &tv);
		if(arg == 1) {
			tv.c_iflag &= ~(INLCR | ICRNL | IGNCR |
						IXOFF | IUCLC);
			if (tv0.c_iflag & ISTRIP)
			    tv.c_iflag |= ISTRIP;
			tv.c_oflag |= OPOST;
			tv.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR
						| ONLRET);
			tv.c_lflag &= ~(ICANON | ISIG | ECHO);
			if(sstop == NO)
				tv.c_iflag &= ~IXON;
			else
				tv.c_iflag |= IXON;
			if(terminal) {
				tv.c_oflag |= ONLCR;
				tv.c_iflag |= ICRNL;
			}
			tv.c_cc[VEOF] = '\01';
			tv.c_cc[VEOL] = '\0';
		}
		if(arg == 2) {
			tv.c_iflag |= IXON;
			tv.c_lflag |= ISIG;
		}
		(void)ioctl(TTYIN, TCSETAW, &tv);
	}
}

static int
dofork()
{
	register int x, i=0;

	while(++i < 6)
		if((x = fork()) >= 0) {
			/* Do not give setuid shells */
   /* NOTE: important security issues depend upon dofork() returning with
    *       all uid fields set to be those of the caller for the child. 
    *       DO NOT change this behavior without dealing with clean 
    *       environment issues that have been changed to allow users to
    *       keep their environment during shell escapes
    */
			if (x == 0) {
				if (setuid(getuid())<0 && getuid()!=0) {
					say((catgets(nlmsg_fd,NL_SETN,76, "Cannot set uid\r\n")));
					hangup(GETID);
				}
			if (getgid() != getegid())
				if (setgid(getgid())<0 && getgid()!=0) {
					say((catgets(nlmsg_fd,NL_SETN,77, "Cannot set gid\r\n")));
					hangup(GETID);
				}
			/* the uid is now that of the user.  This is his
			 * process.  Give him back his environment 
			 */
		        for(i=0;original_environ[i];i++)
				putenv(original_environ[i]);
			}
			return(x);
		}
#ifdef	ddt
	if(_debug == YES) perror("dofork");
#endif
	say(msg22);
	return(x);
}

r_remote()		/* read from rlfd (remote system) */
{
	extern	errno;
	int     rtn;
	char	str[30];
	static char buf[512], *bstart=buf, *bend=buf;

	if (bstart>=bend) {
		errno = 0;
		w_remote(1);
		while ( (rtn = read(rlfd, buf, sizeof(buf))) < 0 ){
		    if(errno == EINTR)
			    if(intrupt == YES) {
				    buf[0]='\0';    /* got a BREAK */
				    buf[1]='\0';
				    rtn=1;
			    } else
				    continue;       /* alarm went off */
		    else {
#ifdef  ddt
			    if(_debug == YES)
				    say((catgets(nlmsg_fd,NL_SETN,79, "got read error, not EINTR\n\r")));
#endif
			    buf[0]='\0';
			    buf[1]='\0';
			    rtn=1;
			    break;                  /* something wrong */
		    }
              }
/* This has been commented out 
 * because of customer confusion 
 *
 * #ifdef ddt
 *		if (_Debug > 5)
 *		    write(2, ".", 1);
 * #endif
 */
		  if (rtn < 0 ) {
		    (void) sprintf(str,(catgets(nlmsg_fd,NL_SETN,80, "read from fd=%d returned %d")), rlfd, rtn);
		    perror(str);
		    bstart=bend=buf; buf[0]='\0';	/* fake reading nul */
		}
                if (rtn == 0){ bstart=bend=buf;buf[0]='\0';}
		bstart=buf; bend=bstart+rtn;
	}
	else
		rtn=1;

	cxc = *bstart++;
	return ((rtn >= 1) ? YES : NO);
}



static int
r_char(fd)
{
	int rtn;

	while((rtn = read(fd, &cxc, 1)) < 0)
		if(errno == EINTR)
			if(intrupt == YES) {
				cxc = '\0';	/* got a BREAK */
				return(YES);
			} else
				continue;	/* alarm went off */
		else {
#ifdef	ddt
			if(_debug == YES)
				say((catgets(nlmsg_fd,NL_SETN,81, "got read error, not EINTR\n\r")));
#endif
			break;			/* something wrong */
		}
	return(rtn == 1? YES: NO);
}

static int
w_remote(flag)
int flag;
{
    extern errno;
    int rtn;
    static int bsize=0;
    static char buf[514];

    if (flag)
	if (bsize) {
	    if (download_flag)
		write(download_fd, buf, bsize);
	    while((rtn = write(1, buf, bsize )) < 0){
		    if(errno == EINTR)
			    if(intrupt == YES) {
				    say((catgets(nlmsg_fd,NL_SETN,82, "\ncu: Output blocked\r\n")));
				    quit(IOERR);
			    } else
				    continue;       /* alarm went off */
		    else
			    break;                  /* bad news */
            }
	    rtn = (rtn == bsize ? YES : NO);
	    bsize=0;
	    return(rtn);
	}
	else
	    return(YES);

    buf[bsize++] = cxc;
    return(YES);

}

static int
w_char(fd)
{
	int rtn;

	while((rtn = write(fd, &cxc, 1)) < 0)
		if(errno == EINTR)
			if(intrupt == YES) {
				say((catgets(nlmsg_fd,NL_SETN,83, "\ncu: Output blocked\r\n")));
				quit(IOERR);
			} else
				continue;	/* alarm went off */
		else
			break;			/* bad news */
/* This has been commented
 * because it confuses the 
 * customer
 *
 * #ifdef ddt
 *	if (_Debug > 5)
 *	    write(2, ",", 1);
 *#endif
 */

	return(rtn == 1? YES: NO);
}

/*VARARGS1*/
static void
say(fmt, arg1, arg2, arg3, arg4, arg5)
char	*fmt;
{
	(void)fprintf(stderr, fmt, arg1, arg2, arg3, arg4, arg5);
}

static void
w_str(string)
register char *string;
{
	int len;

	len = strlen(string);
	if(write(rlfd, string, (unsigned)len) != len)
		say(msg14);
}

static
onintrpt()
{
	(void)signal(SIGINT, onintrpt);
	(void)signal(SIGQUIT, onintrpt);
	intrupt = YES;
}

static
download_close()
{
    close(download_fd);
    download_flag=0;
}

static
rcvdead(arg)	/* this is executed only in the receive proccess */
int arg;
{
#ifdef ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,84, "call rcvdead(%d)\r\n")), arg);
#endif
	(void)kill(getppid(), SIGUSR1);
	exit((arg == SIGHUP)? SIGHUP: arg);
	/*NOTREACHED*/
}

static
quit(arg)	/* this is executed only in the parent proccess */
int arg;
{
#ifdef ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,85, "call quit(%d)\r\n")), arg);
#endif
	(void)kill(child, SIGKILL);
	bye(arg);
	/*NOTREACHED*/
}

static
die(arg)        /* this is executed only in the parent proccess */
{
	kill(child, SIGKILL);
	bye(0);
}

static
bye(arg)	/* this is executed only in the parent proccess */
int arg;
{
	int status;
#ifdef ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,86, "call bye(%d)\r\n")), arg);
#endif
	turnoff();
	(void)wait(&status);
	say((catgets(nlmsg_fd,NL_SETN,87, "\r\nDisconnected\007\r\n")));
	hangup((arg == SIGUSR1)? (status >>= 8): arg);
	/*NOTREACHED*/
}

turnoff()
{
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
}


static
hangup(arg)	/* this is executed only in the parent process */
int arg;
{
#ifdef ddt
	if(_debug == YES) say((catgets(nlmsg_fd,NL_SETN,88, "call hangup(%d)\r\n")), arg);
#endif
	if(chmod(call.device, savmode) || chown(call.device, savuid, savgid) <0)
		say(stderr, (catgets(nlmsg_fd,NL_SETN,89, "Cannot chown/chmod on %s\n")), call.device);
	undial(rlfd);
	mode(0);
	exit(arg);	/* restore users prior tty status */
	/*NOTREACHED*/
}

#include <sys/modem.h>

#ifdef ddt
static void
tdmp(arg)
{

        /*mflag status; mflag not defined on s300? */
	struct termio xv;
	int i;

        /*ioctl(rlfd,MCGETA,&status);
        fprintf(stderr,"RS232 %o\n",status);*/
	say((catgets(nlmsg_fd,NL_SETN,90, "\rdevice status for fd=%d\r\n")), arg);
	say("F_GETFL=%o,", fcntl(arg, F_GETFL,1));
	say("F_GETFD=%o,", fcntl(arg, F_GETFD,1));
	if(ioctl(arg, TCGETA, &xv) < 0) {
		char	buf[100];
		i = errno;
		(void)sprintf(buf, "\rtdmp for fd=%d", arg);
		errno = i;
		perror(buf);
		return;
	}
	say("iflag=`%o',", xv.c_iflag);
	say("oflag=`%o',", xv.c_oflag);
	say("cflag=`%o',", xv.c_cflag);
	say("lflag=`%o',", xv.c_lflag);
	say("line=`%o'\r\n", xv.c_line);
	say("cc[0]=`%o',",  xv.c_cc[0]);
	for(i=1; i<8; ++i)
		say("[%d]=`%o', ", i, xv.c_cc[i]);
	say("\r\n");
}
#endif


#include	<sys/utsname.h>

sysname(name)
char * name;
{

	register char *s, *t;
	struct utsname utsn;

	if(uname(&utsn) < 0)
		s = (catgets(nlmsg_fd,NL_SETN,91, "Local"));
	else
		s = utsn.nodename;

	t = name;
	while((*t = *s++ ) &&  (t < name+7))
		*t++;

	*(name+7) = '\0';
	return;
}


#define	NPL	50
blckcnt(count)
long count;
{
	static long lcnt = 0;
	register long c1, c2;
	register int i;
	char c;

	if(count == (long) (-1)) {       /* initialization call */
		lcnt = 0;
		return;
	}
	c1 = lcnt/BUFSIZ;
	if(count != (long)(-2)) {	/* regular call */
		c2 = count/BUFSIZ;
		for(i = c1; i++ < c2;) {
			c = '0' + i%10;
			write(2, &c, 1);
			if(i%NPL == 0)
				write(2, "\n\r", 2);
		}
		lcnt = count;
	}
	else {
		c2 = (lcnt + BUFSIZ -1)/BUFSIZ;
		if(c1 != c2)
			write(2, "+\n\r", 3);
		else if(c2%NPL != 0)
			write(2, "\n\r", 2);
		lcnt = 0;
	}
}

/*
 * Return the phone number or an indication
 * of a failure.
 */
char *
sysphone(sysnam)
char *sysnam;
{

	int nf;
	char *flds[50];
	static char phone[MAXN];

	if (Lsysfp == NULL) {	/* only open once */
#ifdef HDBuucp 
		if((Lsysfp=fopen(SYSTEMS, "r")) == NULL) {
			printf((catgets(nlmsg_fd,NL_SETN,92, "cannot open Systems file\n")));
			exit(1);
		}
#else
		if((Lsysfp=fopen(SYSFILE, "r")) == NULL) {
			printf((catgets(nlmsg_fd,NL_SETN,93, "cannot open L.sys file\n")));
			exit(1);
		}
#endif
	}
	nf=finds(Lsysfp, sysnam, flds, phone);
        call.telno = flds[F_PHONE];
	if ( (nf == 0) || (nf == CF_PHONE) )
		return((char *) nf);
	else
		return((char *) phone);
}

/*
 * set system attribute vector
 * return:
 *	0	-> number of arguments in vector succeeded
 *	CF_PHONE	-> phone number not found
 */
finds(fsys, sysnam, flds, phone)
 FILE *fsys;
char *sysnam;
char *phone;
char *flds[];
{

	register int na;
	register int flg;
	register int s;
	static char info[MAXC];
	char *str;
	char sysn[MAXC];

	/*
	 * format of fields
	 *	0	-> name;
	 *	1	-> time
	 *	2	-> acu/hardwired
	 *	3	-> speed
	 *	4	-> phone number
	 */
	flg = 0;
	if(strlen(sysnam) > 4)
		flg++;
	while (fgets(info, MAXC, fsys) != NULL) {
		if((info[0] == '#') || (info[0] == ' ') || (info[0] == '\t') || 
			(info[0] == '\n'))
			continue;
		if(flg)
			if(sysnam[4] != info[4])
				continue;
		if(info[0] != sysnam[0])
			continue;
		na=getargs(info, flds);
#ifdef HDBuucp 
		sprintf(sysn, "%s", flds[F_NAME]);
		if(strcmp(sysnam, sysn) != SAME)
#else
		sprintf(sysn, "%.7s", flds[F_NAME]);
		if(strncmp(sysnam, sysn, SYSNSIZE) != SAME)
#endif
			continue;
		if (na <= 2)
			continue;       /* ignore passive entries */

		/* find the phone number or the direct line 
		 ** If a baud was specified, look for that baud ONLY!
		*/

#ifdef HDBuucp  /* HDB */
                /* HDB's F_SPEED can have just the number, the
                   number prefixed by a letter, and "Any" */ 
                if (strcmp("Any",flds[F_SPEED]) == SAME){
                   /* if a speed is specified, use it */
                   /* else default to 1200 */
                   if (call.baud != -1 && sflag) s = call.baud;
                   else s = HIGH;
                }else if (!isdigit(flds[F_SPEED][0]))s = atoi(flds[F_SPEED][1]);
                else s = atoi(flds[F_SPEED]);
#else
                s = atoi(flds[F_SPEED]);
#endif
		if(sflag) {
			if(call.baud != -1 && s != call.baud) 
				continue;
		}

		call.baud = s;
		call.speed = s;
		strcpy(phone, flds[F_PHONE]);
#ifdef HDBuucp 
         /* HoneyDanBer allows:
            sys Time sys  speed -     login
            sys Time ACUs speed phone login */ 

        /* get rid of protocol */
        {
           char *p;
           p=strchr(flds[F_LINE],',');
           if (p != NULL){
                    strcpy(_protocol,p+1);
                    *p ='\0';
           }else strcpy(_protocol,"");
        }

         if (strcmp(flds[F_NAME],flds[F_LINE]) == SAME ){
            /* direct connection */
            strcpy(modemtype,flds[F_LINE]);
            strcpy(phone,modemtype);
            return(na);
         }
#endif
		str = strchr(phone, '/');
		if(strcmp(flds[F_LINE], str?str+1:phone) == SAME) {
			if (str)
				strcpy(phone, str+1);
			strcpy(modemtype, "");
			return(na);
		} else {
#ifdef HDBuucp

			strcpy(modemtype, flds[F_LINE]);
			return(na);
		}
#else
			if ( str ) {
				strcpy(phone, str + 1);
				str = strchr(flds[F_PHONE], '/');
				exphone(str + 1, phone);
			}
			else
			    exphone(flds[F_PHONE], phone);

			/* check for "VOID"ed phone numbers */
			if (isupper(phone[0]))
				continue;
			strcpy(modemtype, flds[F_LINE]);
			return(na);
			}
#endif
	}
        
              
	return(CF_PHONE);
}



/*
 * generate a vector of pointers (arps) to the
 * substrings in string "s".
 * Each substring is separated by blanks and/or tabs.
 *	s	-> string to analyze
 *	arps	-> array of pointers
 * returns:
 *	i	-> # of subfields
 * Bug:
 * Should pass # of elements in arps in case s
 * is garbled from file.
 */
getargs(s, arps)
register char *s, *arps[];
{
	register int i;

	i = 0;
	while (1) {
		arps[i] = NULL;
		while (*s == ' ' || *s == '\t')
			*s++ = '\0';
		if (*s == '\n')
			*s = '\0';
		if (*s == '\0')
			break;
		arps[i++] = s++;
		while (*s != '\0' && *s != ' '
			&& *s != '\t' && *s != '\n')
				s++;
	}
	return(i);
}
/*
 * expand phone number for given prefix and number
 * return:
 *	none
 */
exphone(in, out)
register char *in;
char *out;
{
	register FILE *fn;
	register char *s1;
	char pre[MAXPH], npart[MAXPH]; 
	char buf[BUFSIZ], tpre[MAXPH], p[MAXPH];
	char *strcpy(), *strcat();

	if (!isalpha(*in)) {
		strcpy(out, in);
		return;
	}

	s1=pre;
	while (isalpha(*in))
		*s1++ = *in++;
	*s1 = '\0';
	s1 = npart;
	while (*in != '\0')
		*s1++ = *in++;
	*s1 = '\0';

	tpre[0] = '\0';
#ifdef HDBuucp 
	fn = fopen(DIALCODES, "r");
#else
	fn = fopen(DIALFILE, "r");
#endif
	if (fn != NULL) {
		while (fgets(buf, BUFSIZ, fn)) {
		if((buf[0] == '#') || (buf[0] == ' ') || (buf[0] == '\t') || 
			(buf[0] == '\n'))
				continue;
			sscanf(buf, "%s%s", p, tpre);
			if (strcmp(p, pre) == SAME)
				break;
			tpre[0] = '\0';
		}
		fclose(fn);
	}

        if (tpre){
		strcpy(out, tpre);
		strcat(out, npart);
        } else strcpy(out,"");
	return;
}

/* copyenv returns a copy of "env".  This is used to make a copy of the 
 * initail users environment so that subsequent escapes will have all the
 * variables set the way they expect.  This is for defect FSDlj06005
 */

char **
copyenv(env)
char **env;
{
        int num_entries,i,j;
        char **newenv;

        for(num_entries=0;env[num_entries];num_entries++)
                ;
        if ((newenv = (char **)malloc(sizeof(char*)*(num_entries+1))) == NULL)
                return NULL;

        for(i=0;i<num_entries;i++)
                if ((newenv[i] = strdup(env[i])) == NULL)
                        break;

        if (i < num_entries)
        {
                for(j=0;j<i;j++)
                        free(newenv[i]);
                free(newenv);
                return NULL;
        }
        else
                newenv[i] = NULL;

        return newenv;
}
