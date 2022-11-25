static char *HPUX_ID = "@(#) $Revision: 66.6 $";

/*
 *      getty - sets up speed, various terminal flags, line discipline,
 *	and waits for new prospective user to enter name, before
 *	calling "login".
 *
 *	Usage:	getty [-h] [-t time] line speed_label terminal
 *		    line_disc
 *
 *	-h says don't hangup by dropping carrier during the
 *		initialization phase.  Normally carrier is dropped to
 *		make the dataswitch release the line.
 *	-t says timeout after the number of seconds in "time" have
 *		elapsed even if nothing is typed.  This is useful
 *		for making sure dialup lines release if someone calls
 *		in and then doesn't actually login in.
 *	"line" is the device in "/dev".
 *	"speed_label" is a pointer into the "/etc/getty_defs"
 *			where the definition for the speeds and
 *			other associated flags are to be found.
 *	"terminal" is the name of the terminal type.
 *	"line_disc" is the name of the line discipline.
 *
 *	Usage:  getty -c gettydefs_like_file
 *
 *	The "-c" flag is used to have "getty" check a gettydefs file.
 *	"getty" parses the entire file and prints out its findings so
 *	that the user can make sure that the file contains the proper
 *	information.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<termio.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<utmp.h>
#ifdef MRTS_ON
#include 	<sys/modem.h>
#endif
#ifdef SYS_III
#include	<fcntl.h>
#else
#ifdef UN_DOC
#include	<sys/crtctl.h>
#endif
#endif
#include	<sys/utsname.h>
#include	<ctype.h>
#include 	<fcntl.h>
#ifdef TRUX
#include	<sys/security.h>
#endif

#ifdef SYS_III
#define		UTMP		"/etc/utmp"
#define 	WTMP		"/usr/adm/wtmp"
#define		XONLY		"!"
#endif

#define		TRUE		1
#define		FALSE		0
#define		FAILURE		-1

#define		SUCCESS		0
#define		ID		1
#define		IFLAGS		2
#define		FFLAGS		3
#define		MESSAGE		4
#define		NEXTID		5

#define		ACTIVE		1
#define		FINISHED	0

#define		ABORT		0177		/* Delete */
#define		QUIT		('\\'&037)	/* ^\ */
#define		ERASE		'#'
#define		BACKSPACE	'\b'
#define		KILL		'@'
#define		EOFILE		('D'&037)	/* ^D */

#ifdef OSS
/*	The following three characters are the standard OSS erase,	*/
/*	kill, and abort characters.					*/

#define		STDERASE	'_'
#define		STDKILL		'$'
#define		STDABORT	'&'
#endif

#define		control(x)	(x&037)

#define		GOODNAME	1
#define		NONAME		0
#define		BADSPEED	-1

#ifndef		fioctl
#define		fioctl(x,y,z)	ioctl(fileno(x),y,z)
#endif

struct Gdef {
	char	*g_id;	/* identification for modes & speeds */
	struct termio	g_iflags;	/* initial terminal flags */
	struct termio	g_fflags;	/* final terminal flags */
	char	*g_message;	/* login message */
	char	*g_nextid;	/* next id if this speed is wrong */
};

#define	MAXIDLENGTH	15	/* Maximum length the "g_id" and "g_nextid" \
				 * strings can take.  Longer ones will be \
				 * truncated. \
				 */
#define MAXMESSAGE	79	/* Maximum length the "g_message" string \
				 * can be.  Longer ones are truncated. \
				 */

/*	Maximum length of line in /etc/gettydefs file and the maximum	*/
/*	length of the user response to the "login" message.		*/

#define	MAXLINE		255
#define	MAXARGS		64	/* Maximum number of arguments that can be \
				 * passed to "login" \
				 */

struct Symbols {
	char		*s_symbol;	/* Name of symbol */
	unsigned	s_value	;	/* Value of symbol */
};

/*	The following four symbols define the "SANE" state.		*/

#define	ISANE	(BRKINT|IGNPAR|ICRNL|IXON)
#define	OSANE	(OPOST|ONLCR)
#define	CSANE	(CS8|CREAD)
#define	LSANE	(ISIG|ICANON|ECHO|ECHOK)

/*	Modes set with the TCSETAW ioctl command.			*/

struct Symbols imodes[] = {
	"IGNBRK",	IGNBRK,
	"BRKINT",	BRKINT,
	"IGNPAR",	IGNPAR,
	"PARMRK",	PARMRK,
	"INPCK",	INPCK,
	"ISTRIP",	ISTRIP,
	"INLCR",	INLCR,
	"IGNCR",	IGNCR,
	"ICRNL",	ICRNL,
	"IUCLC",	IUCLC,
	"IXON",	IXON,
	"IXANY",	IXANY,
	"IXOFF",	IXOFF,
#ifdef IENQAKOFF
#else
	"IENQAK",	IENQAK,			/* HP-UX extension */
#endif IENQAKOFF
	NULL,	0
};

struct Symbols omodes[] = {
	"OPOST",	OPOST,
	"OLCUC",	OLCUC,
	"ONLCR",	ONLCR,
	"OCRNL",	OCRNL,
	"ONOCR",	ONOCR,
	"ONLRET",	ONLRET,
	"OFILL",	OFILL,
	"OFDEL",	OFDEL,
	"NLDLY",	NLDLY,
	"NL0",	NL0,
	"NL1",	NL1,
	"CRDLY",	CRDLY,
	"CR0",	CR0,
	"CR1",	CR1,
	"CR2",	CR2,
	"CR3",	CR3,
	"TABDLY",	TABDLY,
	"TAB0",	TAB0,
	"TAB1",	TAB1,
	"TAB2",	TAB2,
	"TAB3",	TAB3,
	"BSDLY",	BSDLY,
	"BS0",	BS0,
	"BS1",	BS1,
	"VTDLY",	VTDLY,
	"VT0",	VT0,
	"VT1",	VT1,
	"FFDLY",	FFDLY,
	"FF0",	FF0,
	"FF1",	FF1,
	NULL,	0
};

struct Symbols cmodes[] = {
	"B0",		B0,
	"B50",	B50,
	"B75", 	B75,
	"B110",	B110,
	"B134",	B134,
	"B150",	B150,
	"B200",	B200,
	"B300",	B300,
	"B600",	B600,
	"B900",	B900,				/* HP-UX extension */
	"B1200",	B1200,
	"B1800",	B1800,
	"B2400",	B2400,
	"B3600",	B3600,			/* HP-UX extension */
	"B4800",	B4800,
	"B7200",	B7200,			/* HP-UX extension */
	"B9600",	B9600,
	"B19200",	B19200,			/* HP-UX extension */
	"B38400",	B38400,			/* HP-UX extension */
	"EXTA",	EXTA,
	"EXTB",	EXTB,
	"CS5",	CS5,
	"CS6",	CS6,
	"CS7",	CS7,
	"CS8",	CS8,
	"CSTOPB",	CSTOPB,
	"CREAD",	CREAD,
	"PARENB",	PARENB,
	"PARODD",	PARODD,
	"HUPCL",	HUPCL,
	"CLOCAL",	CLOCAL,
#ifdef UN_DOC
	"CRTS",		CRTS,			/* HP-UX extension */
#endif
	NULL,	0
};

struct Symbols lmodes[] = {
	"ISIG",	ISIG,
	"ICANON",	ICANON,
	"XCASE",	XCASE,
	"ECHO",	ECHO,
	"ECHOE",	ECHOE,
	"ECHOK",	ECHOK,
	"ECHONL",	ECHONL,
	"NOFLSH",	NOFLSH,
	NULL,	0
};

#if !defined(SYS_III) && defined(UN_DOC)
/*	Terminal types set with the LDSETT ioctl command.		*/
/*	LDSETT is one of the ioctl undocumented features.  Since they are
	undocumented, AT&T may change them anytime.  And HP-UX doesn't support
	these features now (2-11-85).  If HP-UX supports these features in
	the future, simply define UN_DOC to have getty uses them.   -kao*/
struct Symbols terminals[] = {
	"none",		TERM_NONE,
#ifdef	TERM_V10
	"vt100",		TERM_V10,
#endif
#ifdef	TERM_H45
	"hp45",		TERM_H45,
#endif
#ifdef	TERM_C10
	"c100",		TERM_C100,
#endif
#ifdef	TERM_TEX
	"tektronix",	TERM_TEX,
	"tek",		TERM_TEX,
#endif
#ifdef	TERM_D40
	"ds40-1",		TERM_D40,
#endif
#ifdef	TERM_V61
	"vt61",		TERM_V61,
#endif
#ifdef	TERM_TEC
	"tec",		TERM_TEC,
#endif
	NULL,		0
};
#endif

/*	Line disciplines set by the TIOCSETD ioctl command.		*/

#ifndef	LDISC0
#define	LDISC0	0
#endif
#ifndef UN_DOC
#ifndef TERM_NONE
#define	TERM_NONE	0
#endif
#endif

struct Symbols linedisc[] = {
	"LDISC0",		LDISC0,
	NULL,		0
};

/*	If the /etc/gettydefs file can't be opened, the following	*/
/*	default is used.						*/

struct Gdef DEFAULT = {
	"default",
	ICRNL,0,B300+CREAD+HUPCL,0,
	LDISC0,ABORT,QUIT,ERASE,KILL,'\0','\0','\0','\0',
	ICRNL,OPOST+ONLCR+NLDLY+TAB3,B300+CS7+CREAD+HUPCL,
	ISIG+ICANON+ECHO+ECHOE+ECHOK,
	LDISC0,ABORT,QUIT,ERASE,KILL,'\0','\0','\0','\0',
	"LOGIN: ",
	"default"
};

#ifndef	DEBUG
#ifdef SYS_III
char	*CTTY		=	"/dev/console";
#else
char	*CTTY		=	"/dev/syscon";
/*char	*CTTY		=	"/dev/ttyb4";	/* temporary -kao*/
#endif
#else
char	*CTTY		=	"/dev/sysconx";
#endif

char	*ISSUE_FILE	=	"/etc/issue";
char	*GETTY_DEFS	=	"/etc/gettydefs";

int 	check = {
	FALSE
};
char	*checkgdfile;		/* Name of gettydefs file during
				 * check mode.
				 */
#ifdef MRTS_ON
int	mrts_flg = 0;
#endif

main(argc,argv)
int argc;
char **argv;
{
	char *line;
	register struct Gdef *speedef;
	char oldspeed[MAXIDLENGTH+1],newspeed[MAXIDLENGTH+1];
	extern struct Gdef *find_def();
	int termtype,lined;
	extern char *ISSUE_FILE,*GETTY_DEFS;
	extern int check;
	int hangup,timeout;
	extern char *checkgdfile;
	extern struct Symbols *search(),terminals[],linedisc[];
	extern int timedout();
	register struct Symbols *answer;
/* Bug fix to System V code.  See explanation below where "login" */
/* is exec'ed. */
/*	char user[MAXLINE],*largs[MAXARGS],*ptr,buffer[MAXLINE]; */
	char user[MAXLINE],*largs[MAXARGS + 1],*ptr,buffer[MAXLINE];
	FILE *fp;
	FILE *fdup();
	struct utsname utsname;
	struct termio termio;
#ifdef SYS_III
	int exitonly,tempfd;
#else
#ifdef	UN_DOC
	struct termcb termcb;
	static char clrscreen[2] = {
		ESC,CS
	};
#endif
#endif

	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_DFL);

	hangup = TRUE;
	timeout = 0;
#ifdef SYS_III
	exitonly = 0;
#endif
	while(--argc && **++argv == '-') {
		for(ptr = *argv + 1; *ptr;ptr++) switch(*ptr) {
		case 'h':
			hangup = FALSE;
			break;
#ifndef SYS_III
		case 't':
			if(isdigit(*++ptr)) {
				sscanf(ptr,"%d",&timeout);

/* Advance "ptr" so that it is pointing to the last digit of the */
/* timeout argument. */
				while(isdigit(*++ptr));
				ptr--;
			} else if(--argc) {
				if(isdigit(*(ptr = *++argv)))
					sscanf(ptr,"%d",&timeout);
				else error("getty: timeout argument invalid. \"%s\"\n", *argv);
			}
			break;
#endif

/* Check a "gettydefs" file mode. */
		case 'c':
			signal(SIGINT,SIG_DFL);
			if(--argc == 0) {
				fprintf(stderr,
				    "Check Mode Usage: getty -c gettydefs-like-file\n");
				exit(1);
			}
			check = TRUE;
			checkgdfile = *++argv;

/* Attempt to open the check gettydefs file. */
			if((fp = fopen(checkgdfile,"r")) == NULL) {
				fprintf(stderr,"Cannot open %s\n",checkgdfile);
				exit(1);
			}
			fclose(fp);

/* Call "find_def" to check the check file.  With the "check" flag */
/* set, it will parse the entire file, printing out the results. */
			find_def(NULL);
			exit(0);
		default:
			break;
		}
	}

/* There must be at least one argument.  If there isn't, complain */
/* and then die after 20 seconds.  The 20 second sleep is to keep */
/* "init" from working too hard. */
	if(argc < 1) {
		error("getty: no terminal line specified.\n");
		sleep(20);
		exit(1);
	} else line = *argv;

/* If a "speed_label" was provided, search for it in the */
/* "getty_defs" file.  If none was provided, take the first entry */
/* of the "getty_defs" file as the initial settings. */
	if(--argc > 0 ) {
#ifdef SYS_III
/* For System III, need to check for the speed identifier of "!" */
/* and just exit after doing utmp accounting and hangup. */
		if(strcmp(*++argv,XONLY) == 0)
			exitonly = 1;
		else if((speedef = find_def(*argv)) == NULL) {
#else
		if((speedef = find_def(*++argv)) == NULL) {
#endif
			error("getty: unable to find %s in \"%s\".\n",
			    *argv,GETTY_DEFS);

/* Use the default value instead. */
			speedef = find_def(NULL);
		}
	} else speedef = find_def(NULL);

#ifdef SYS_III
/* Get the timeout value. */
	if (--argc > 0) {
		if(isdigit(*(ptr = *++argv)))
			sscanf(ptr,"%d",&timeout);
		else error("getty: timeout argument invalid. \"%s\"\n", *argv);
	}

/* Need to disable terminal type and line discipline for SYS_III */
	termtype = 0;
	lined = LDISC0;

#else
/* If a terminal type was supplied, try to find it in list. */
#if defined(UN_DOC)
	if(--argc > 0) {
		if((answer = search(*++argv,terminals)) == NULL) {
			error("getty: %s is an undefined terminal type.\n",
			    *argv);
			termtype = TERM_NONE;
		} else termtype = answer->s_value;
	} else  termtype = TERM_NONE ;
#else
	/* Ignore user input termtype and set to TERM_NONE */
	termtype = TERM_NONE;
#endif UN_DOC

#if defined(UN_DOC)
/* If a line discipline was supplied, try to find it in list. */
	if(--argc > 0) {
		if((answer = search(*++argv,linedisc)) == NULL) {
			error("getty: %s is an undefined line discipline.\n",
			    *argv);
			lined = LDISC0;
		} else lined = answer->s_value;
	} else  lined = LDISC0;
#else
	/*Ignore user input line discipline, and set to LDISC0 */
	lined = LDISC0;
#endif UN_DOC
#endif

/* Perform "utmp" accounting. */
	account(line);

/* Attempt to open standard input, output, and error on specified */
/* line. */
	chdir("/dev");

#ifdef SYS_III
/* Here we need to check to see if the "!" speed was set, and do the */
/* hangup, if needed, and exit.  The hangup is only done if 0 was not */
/* specified as the timeout value. */
	if (exitonly) {
		if((argc > 0) && isdigit(**argv) && (timeout == 0))
			exit(0);
		signal(SIGALRM,SIG_DFL);
		alarm(timeout?timeout:2);
		tempfd=open(line,O_RDWR);
		ioctl(tempfd,TCGETA,&termio);
		termio.c_cflag = B0|HUPCL;
		ioctl(tempfd,TCSETA,&termio);
		exit(1);
	}
#endif

	openline(line,speedef,termtype,lined,hangup);

/* Loop until user is successful in requesting login. */
	for(;;) {

/* If there is no terminal type, just advance a line. */
#if !defined(SYS_III) && defined(UN_DOC)
		if(termtype == TERM_NONE) {
#endif

/* A bug in the stdio package requires that the first output on */
/* the newly reopened stderr stream be a putc rather than an */
/* fprintf. */
			putc('\r',stderr);
			putc('\n',stderr);

#if !defined(SYS_III) && defined(UN_DOC)
/* If there is a terminal type, clear the screen with the common */
/* crt language.  Note that the characters have to be written in */
/* one write, and hence can't go through standard io, which is */
/* currently unbuffered. */
		} else write(fileno(stderr),clrscreen,sizeof(clrscreen));
#endif

/* If getty is supposed to die if no one logs in after a */
/* predetermined amount of time, set the timer. */
		if(timeout) {
			signal(SIGALRM,timedout);
			alarm(timeout);
		}

#ifdef SYS_NAME
/* Generate a message with the system identification in it. */
		if (uname(&utsname) != FAILURE) {
			sprintf(buffer,"%.9s\r\n", utsname.nodename);

#ifdef	UPPERCASE_ONLY
/* Make all the alphabetics upper case. */
			for (ptr= buffer; *ptr;ptr++) *ptr = tolower(*ptr);
#endif
			fputs(buffer,stderr);
		}

#endif
#ifdef ETC_ISSUE
/* Print out the issue file. */
		if ((fp = fopen(ISSUE_FILE,"r")) != NULL) {
			while ((ptr = fgets(buffer,sizeof(buffer),fp)) != NULL) {
				fputs(ptr,stderr);

/* In "raw" mode, a carriage return must be supplied at the end of */
/* each line. */
				putc('\r',stderr);
			}
			fclose(fp);
		}
#endif

/* Print the login message. */
		fprintf(stderr,"%s",speedef->g_message);

/* BUG FIX: */
/* This fixes the problem where a high speed getty on a long, */
/* unterminated RS-232 line reads trashed line-echo, causing */
/* it to use 100% of the CPU.  Basically, read the terminal */
/* state, and set it back to the same state, using the ioctl */
/* that flushes all typeahead after waiting for output to drain. */
/* Note that I can use &termio here, because getname() is the */
/* next thing called, and the first thing it does is to overwrite */
/* &termio. */
		fioctl(stdin,TCGETA,&termio);
		fioctl(stdin,TCSETAF,&termio);

/* Get the user's typed response and respond appropriately. */
		switch(getname(user,&termio)) {
		case GOODNAME:

/* Parse the input line from the user, breaking it at white */
/* spaces. */
			largs[0] = "login";
			parse(user,&largs[1],MAXARGS-1);

/*
 * Close security hole:  prevent user from spoofing login by running login -r.
 * Do this before making any irreversible changes on the way to exec'ing login.
 */

			if (largs [1] [0] == '-')
			{
			    fputs ("login names may not start with '-'.", stderr);
			    break;
			}

			if (timeout) alarm(0);

#if !defined(SYS_III) && defined(UN_DOC)
/* If a terminal type was specified, keep only those parts of */
/* the gettydef final settings which were not explicitely turned */
/* on when the terminal type was set. */
			if (termtype != TERM_NONE) {
				termio.c_iflag |= (ISTRIP|ICRNL|IXON|IXANY)
					| (speedef->g_fflags.c_iflag
						& ~(ISTRIP|ICRNL|IXON|IXANY));
				termio.c_oflag |= (OPOST|ONLCR)
					| (speedef->g_fflags.c_oflag
						& ~(OPOST|ONLCR));
				termio.c_cflag = speedef->g_fflags.c_cflag;
				termio.c_lflag = (ISIG|ICANON|ECHO|ECHOE|ECHOK)
					| (speedef->g_fflags.c_lflag
						& ~(ISIG|ICANON|ECHO|ECHOE|ECHOK));
			} else {
#endif
				termio.c_iflag |= speedef->g_fflags.c_iflag;
				termio.c_oflag |= speedef->g_fflags.c_oflag;
				termio.c_cflag |= speedef->g_fflags.c_cflag;
				termio.c_lflag |= speedef->g_fflags.c_lflag;
#if !defined(SYS_III) && defined(UN_DOC)
			}
#endif

			termio.c_line = lined;
			fioctl(stdin,TCSETAW,&termio);

/* Bugfix to System V code.  Need to guarantee that last argument */
/* passed to execv is a null string.  If all 64 arguments are passed */
/* to "login", parse() will not leave a null pointer as the last */
/* element in largs.  Fix involves making largs one element larger, */
/* and forcing it to be a null pointer just in case. */
			largs[MAXARGS] = (char *)NULL;

/* Exec "login". */

#ifndef	DEBUG
#ifdef SYS_III
/* For System III, "login" resides in /etc, not /bin.  Also, "login" */
/* takes only two parameters: login name and timeout.  If valid timeout */
/* was specified on "getty" command line, use it.  Otherwise, default to 60. */
			largs[3] = (char *)NULL;
			largs[2] = ((argc > 0) && isdigit(**argv)) ? *argv : "60";
			execv("/etc/login",largs);
#else
#ifdef PFA
 pfa_dump(); 
#endif
#ifdef SecureWare
			if (ISSECURE)
			    execv(getty_login_program(), largs);
			else
			    execv("/bin/login",largs);
#else
			execv("/bin/login",largs);
#endif

/*			execv("/etc/login",largs);  /*temporary -kao*/
#endif
			exit(1);
#else
			exit(0);
#endif

/* If the speed supplied was bad, try the next speed in the list. */
		case BADSPEED:

/* Save the name of the old speed definition incase new one is */
/* bad.  Copy the new speed out of the static so that "find_def" */
/* won't overwrite it in the process of looking for new entry. */
			strcpy(oldspeed,speedef->g_id);
			strcpy(newspeed,speedef->g_nextid);
			if ((speedef = find_def(newspeed)) == NULL) {
				error("getty: pointer to next speed in entry %s is bad.\n",
				oldspeed);

/* In case of error, go back to the original entry. */
				if((speedef = find_def(oldspeed)) == NULL) {

/* If the old entry has disappeared, then quit and let next "getty" try. */
					error("getty: unable to find %s again.\n",
						oldspeed);
					exit(1);
				}
			}

/* Setup the terminal for the new information. */
			setupline(speedef,termtype,lined);
			break;

/* If no name was supplied, not nothing, but try again. */
		case NONAME:
			break;
		}
	}
}

#ifdef SYS_III
utscan(utf,lnam)
register utf;
char *lnam;
{
	register i;
	struct utmp wtmp;

	lseek(utf, (long)0, 0);
	while(read(utf, (char *)&wtmp, sizeof(wtmp)) == sizeof(wtmp))
	{
		for(i=0; i<8; i++)
			if(wtmp.ut_line[i] != lnam[i])
				goto contin;
		lseek(utf, -(long)sizeof(wtmp), 1);
		return(1);
		contin:;
	}
	return(0);
}

account(lname)
char *lname;
{
	int utf, i;
	struct utmp wtmp;
	char lnam[8];

	strncpy(lnam,lname,8);

	for(i=0; i<8; i++)
	{
		wtmp.ut_name[i] = '\0';
		wtmp.ut_line[i] = lnam[i];
	}
	time(&wtmp.ut_time);

	utf = open(UTMP, O_RDWR|O_CREAT, 0644);
	if(utscan(utf,lnam) != 0)
		write(utf, &wtmp, sizeof(wtmp));
	else
	{
		fcntl(utf, F_SETFL, fcntl(utf, F_GETFL, 0) | O_APPEND);
		write(utf, &wtmp, sizeof(wtmp));
	}

	close(utf);
	utf = open(WTMP, O_WRONLY|O_CREAT|O_APPEND);
	write(utf, &wtmp, sizeof(wtmp));
	close(utf);
}
#else
account(line)
char *line;
{
	void endutent();
	extern struct utmp *getutent(), *_pututline();
	register struct utmp *u;
	register int ownpid = getpid();

	/*
	 * Look in "utmp" file for our own entry and change it to
	 * LOGIN.
	 */
	while ((u = getutent()) != NULL) {
		/*
		 * Is this our own entry?
		 */
		if (u->ut_type == INIT_PROCESS && u->ut_pid == ownpid) {
			strncpy(u->ut_line,line,sizeof(u->ut_line));
			strncpy(u->ut_user,"LOGIN",sizeof(u->ut_user));
			u->ut_type = LOGIN_PROCESS;
			_pututline(u); /* Write out the updated entry. */
			break;
		}
	}

	/*
	 * If we were successful in finding an entry for ourself in the
	 * utmp file, then attempt to append to the end of the wtmp
	 * file.
	 * To prevent multi-processes open the wtmp at the same time and
	 * seek to the same place and start write to the same location,
	 * we open it with append mode. (instead of seeking to the end
	 * like the original ATT code did).
	 */
	if (u != NULL) {
                int fd = open(WTMP_FILE, O_WRONLY|O_APPEND);

                if (fd != -1) {
                        write(fd, u, sizeof (struct utmp));
                        close(fd);
                }
        }

/* Close the utmp file. */
	endutent();
}
#endif

/*	"search" scans through a table of Symbols trying to find a	*/
/*	match for the supplied string.  If it does, it returns the	*/
/*	pointer to the Symbols structure, otherwise it returns NULL.	*/

struct Symbols *search(target,symbols)
register char *target;
register struct Symbols *symbols;
{

/* Each symbol array terminates with a null pointer for an */
/* "s_symbol".  Scan until a match is found, or the null pointer */
/* is reached. */
	for (;symbols->s_symbol != NULL; symbols++)
		if (strcmp(target,symbols->s_symbol) == 0) return(symbols);
	return(NULL);
}

error(format,arg1,arg2,arg3,arg4)
char *format;
int arg1,arg2,arg3,arg4;
{
	register FILE *fp;

/*  Fork child process to print out the error message. */
/*  Reason:  getty is group leader.  The first tty it opened become
	     the control terminal of the process group. (If that tty
	     if not the control terminal of other process group)
	     If parent process output the error message directly,
	     then the tty can become the control terminal (it is
	     console in this case).  If then the getty continue,
	     the user login and find out (s)he lose the INT key and
	     QUIT key.  If another user hit INT/BREAK/QUIT key at
	     the console, then all the processes in the same group
	     will then receive INT/QUIT signal. */

	if (fork() == 0) {
		if ((fp = fopen(CTTY,"w")) != NULL)
		{
			fprintf(fp,format,arg1,arg2,arg3,arg4);
			fclose(fp);
		}
		exit(0);
	}
	else {
		wait(0);
	}
}

openline(line,speedef,termtype,lined,hangup)
register char *line;
register struct Gdef *speedef;
int termtype,lined,hangup;
{
	register FILE *fpin,*fp;
	struct stat statb;
	extern int errno;

	close(0);
	close(1);
	close(2);
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

#ifdef SecureWare
	if (ISSECURE)
	    getty_condition_line(line);
	else{
		/* Change the ownership of the terminal line to root and set */
		/* the protections to only allow root to read the line. */
	    stat(line,&statb);
	    chown(line,0,statb.st_gid);
	    chmod(line,0622);
	}
#else
/* Change the ownership of the terminal line to root and set */
/* the protections to only allow root to read the line. */
	stat(line,&statb);
	chown(line,0,statb.st_gid);
	chmod(line,0622);
#endif


#ifdef MRTS_ON
	/* Open the device in O_NDELAY mode to raisw rts, then close (*/
 	if(mrts_flg) 
	{

		int fd;
		int fd2;
 		mflag flag;

 		fd = open(line, O_RDONLY, O_NDELAY);
 		if ( fd == -1 )
 		{
 			error("getty_rts: cannot open \"%s\" in NO_DELAY mode. errno: %d\n", line, errno);
 			sleep(20);
 			exit(1);
 		}
 		ioctl(fd,MCGETA,&flag);
 		flag |= MRTS;
 		if ( ioctl(fd,MCSETA,&flag) == -1 )
 		{
 			error("getty_rts: cannot raise RTS line. errno: %d\n", errno);
 			sleep(20);
 			exit(1);
 		}

		/* dup the line so that all controlling terminal */
		/* information is saved and the fopen of line succeeds. */
		if ((fd2 = dup(fd)) == -1)
		{
			error("getty_rts: cannot dup line. errno: %d\n", errno);
			sleep(20);
			exit(1);
                }

 		close (fd);

           /* Attempt to open the line.  It should become "stdin".  If not, */
           /* then close. */
	        if (fopen(line,"r+") == NULL) 
		{
		        error("getty: cannot open \"%s\". errno: %d\n",line,errno);
		        sleep(20);
		        exit(1);
                }

		close(fd2);

       	}

	else /* mrts_flg == 0 */
           /* Attempt to open the line.  It should become "stdin".  If not, */
           /* then close. */
	        if (fopen(line,"r+") == NULL) 
		{
		        error("getty: cannot open \"%s\". errno: %d\n",line,errno);
		        sleep(20);
		        exit(1);
		}

#else

/* Attempt to open the line.  It should become "stdin".  If not, */
/* then close. */
	if (fopen(line,"r+") == NULL) {
		error("getty: cannot open \"%s\". errno: %d\n",line,errno);
		sleep(20);
		exit(1);
	}

#endif

	fdup(stdin);
	fdup(stdin);
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);

/* Be sure that the opened line became our control terminal */
/* If it didn't, then this getty will be totally useless, so exit. */
	{
	    int devtty = open("/dev/tty", O_RDWR);
	    if (devtty == -1) {
		perror("getty: failed to open /dev/tty");
		sleep(20);
		exit(1);
	    }
	    close(devtty);
	}

/* Unless getty is being invoked by ct, make sure that DTR has been */
/* dropped and reasserted */
	if (hangup) hang_up_line();

/* Set the terminal type and line discipline. */
	setupline(speedef,termtype,lined);
}

#ifdef HANGUP

hang_up_line()
{
	struct termio termio;

	fioctl(stdin,TCGETA,&termio);
	termio.c_cflag &= ~CBAUD;
	termio.c_cflag |= B0;
	fioctl(stdin,TCSETAF,&termio);
	sleep(1);
}
#else
hang_up_line()
{
}
#endif

timedout()
{
	exit(1);
}

setupline(speedef,termtype,lined)
register struct Gdef *speedef;
int termtype,lined;
{
	struct termio termio;
	unsigned short timer;
#if !defined(SYS_III) && defined(UN_DOC)
	struct termcb termcb;

/* Set the terminal type to "none", which will clear all old */
/* special flags, if a terminal type was set from before. */
	termcb.st_flgs = 0;
	termcb.st_termt = TERM_NONE;
	termcb.st_vrow = 0;
	fioctl(stdin,LDSETT,&termcb);
	termcb.st_termt = termtype;
	fioctl(stdin,LDSETT,&termcb);
#endif

/* Get the current state of the modes and such for the terminal. */
	fioctl(stdin,TCGETA,&termio);
#if !defined(SYS_III) && defined(UN_DOC)
	if (termtype != TERM_NONE) {

/* If there is a terminal type, take away settings so that */
/* terminal is "raw" and "no echo".  Also take away the orginal */
/* speed setting. */
		termio.c_iflag = 0;
		termio.c_cflag &= ~(CSIZE|PARENB|CBAUD);
		termio.c_cflag |= CS8|CREAD|HUPCL;
		termio.c_lflag &= ~(ISIG|ICANON|ECHO|ECHOE|ECHOK);

/* Add in the speed. */
		termio.c_cflag |= (speedef->g_iflags.c_cflag & CBAUD);
	} else {
#endif
		termio.c_iflag = speedef->g_iflags.c_iflag;
		termio.c_oflag = speedef->g_iflags.c_oflag;
		termio.c_cflag = speedef->g_iflags.c_cflag;
		termio.c_lflag = speedef->g_iflags.c_lflag;
#if !defined(SYS_III) && defined(UN_DOC)
	}
#endif

/* Make sure that raw reads are 1 character at a time with no */
/* timeout. */
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME] = 0;

/* Add the line discipline. */
	termio.c_line = lined;
	fioctl(stdin,TCSETAF,&termio);

/* Pause briefly while terminal settles. */
#ifdef COMPBUG
	for(timer=0; ++timer != 1000;);
#else
	for(timer=0; ++timer != 0;);
#endif
}

/*	"getname" picks up the user's name from the standard input.	*/
/*	It makes certain						*/
/*	determinations about the modes that should be set up for the	*/
/*	terminal depending upon what it sees.  If it sees all UPPER	*/
/*	case characters, it sets the IUCLC & OLCUC flags.  If it sees	*/
/*	a line terminated with a <return>, it sets ICRNL.  If it sees	*/
/*	the user using the "standard" OSS erase, kill, abort, or line	*/
/*	termination characters ( '_','$','&','/','!' respectively)	*/
/*	it resets the erase, kill, and end of line characters.		*/

int getname(user,termio)
char *user;
struct termio *termio;
{
	register char *ptr,c;
	register int rawc;
	int upper,lower;

/* Get the previous modes, erase, and kill characters and speeds. */
	fioctl(stdin,TCGETA,termio);

/* Set the flags to 0 and the erase and kill to the standard */
/* characters. */
	termio->c_iflag &= ICRNL;
	termio->c_oflag = 0;
	termio->c_cflag = 0;
	termio->c_lflag &= ECHO;
	for (ptr= (char*)termio->c_cc; ptr < (char*)&termio->c_cc[NCC];)
		*ptr++ = NULL;
	termio->c_cc[VINTR] = ABORT;
	termio->c_cc[VQUIT] = QUIT;
	termio->c_cc[VERASE] = ERASE;
	termio->c_cc[VKILL] = KILL;
	termio->c_cc[VEOF] = EOFILE;
	ptr = user;
	upper = 0;
	lower = 0;
	do {

/* If it isn't possible to read line, exit. */
		if ((rawc = getc(stdin)) == EOF) exit(0);

/* If a null character was typed, return 0. */
/* checking for 7 bits - native language work needed ? */
		if ((c = (rawc & 0177)) == '\0') return(BADSPEED);

#ifdef SYS_III
/* Need to exit if ^D is typed. */
		if (c == EOFILE) {
			sleep(6);
			exit(9);
		}
#endif

/* Echo the character if ECHO is off. */
		if( (termio->c_lflag&ECHO) == 0 )
			putc(rawc,stdout);
#ifdef OSS
		if (c == ERASE || c == BACKSPACE) {

/* Store this character as the "erase" character. */
			termio->c_cc[VERASE] = c;
#else
		if (c == ERASE) {
#endif

/* If there is anything to erase, erase a character. */
			if (ptr > user) --ptr;
		}

#ifdef OSS
		else if (c == STDERASE) {
			if (ptr > user) --ptr;

/* Set up the "standard OSS" erase, kill, etc. characters. */
			termio->c_cc[VINTR] = STDABORT;
			termio->c_cc[VERASE] = STDERASE;
			termio->c_cc[VKILL] = STDKILL;
			termio->c_cc[VEOL] = '/';
			termio->c_cc[VEOL2] = '!';
		}
#endif

/* If the character is a kill line or abort character, reset the */
/* line. */
		else if (c == KILL || c == ABORT || c == control('U')) {
			ptr = user;
			fputs("\r\n",stdout);

/* Make sure the erase, kill, etc. are set to the UNIX standard. */
			termio->c_cc[VINTR] = ABORT;
			termio->c_cc[VERASE] = ERASE;
			termio->c_cc[VKILL] = KILL;
			termio->c_cc[VEOL] = '\0';
#if !defined(SYS_III) && defined(UN_DOC)
			termio->c_cc[VEOL2] = '\0';
#endif
		}

#ifdef OSS
		else if (c == STDKILL || c == STDABORT) {
			ptr = user;

/* Set up the "standard OSS" erase, kill, etc. characters. */
			termio->c_cc[VINTR] = STDABORT;
			termio->c_cc[VERASE] = STDERASE;
			termio->c_cc[VKILL] = STDKILL;
			termio->c_cc[VEOL] = '/';
			termio->c_cc[VEOL2] = '!';
		}
#endif

/* If the character is lower case, increment the flag for lower case. */
		else if (islower(c)) {
			lower++;
			*ptr++ = c;
		}

/* If the character is upper case, increment the flag. */
		else if (isupper(c)) {
			upper++;
			*ptr++ = c;
		}

/* Just store all other characters. */
		else *ptr++ = c;
	}

/* Continue the above loop until a line terminator is found or */
/* until user name array is full. */

#ifdef OSS
	while (c != '\n' && c != '\r' && c != '/' && c != '!' &&
	    ptr < (user + MAXLINE));
#else
	while (c != '\n' && c != '\r'
	    && ptr < (user + MAXLINE));
#endif

/* Remove the last character from name. */
	*--ptr = '\0';
	if (ptr == user) return(NONAME);

#ifdef OSS
/* If the line was terminated with one of the printing OSS line */
/* termination characters or is a <cr>, add a <newline>. */
	if (c == '/' || c == '!') {
		putc('\n',stdout);

/* Set up the "standard OSS" erase, kill, etc. characters. */
		termio->c_cc[VINTR] = STDABORT;
		termio->c_cc[VERASE] = STDERASE;
		termio->c_cc[VKILL] = STDKILL;
		termio->c_cc[VEOL] = '/';
		termio->c_cc[VEOL2] = '!';
	} else
#endif
		if (c == '\r') putc('\n',stdout);

/* If the line terminated with a <cr>, put ICRNL and ONLCR into */
/* into the modes. */
	if (c == '\r') {
		termio->c_iflag |= ICRNL;
		termio->c_oflag |= ONLCR;

/* When line ends with a <lf>, then add the <cr>. */
	} else putc('\r',stdout);

/* Set the upper-lower case conversion switchs if only upper */
/* case characters were seen in the login and no lower case. */
/* Also convert all the upper case characters to lower case. */

	if (upper > 0 && lower == 0) {
		termio->c_iflag |= IUCLC;
		termio->c_oflag |= OLCUC;
		termio->c_lflag |= XCASE;
		for (ptr=user; *ptr; ptr++)
			if (*ptr >= 'A' && *ptr <= 'Z' ) *ptr += ('a' - 'A');
	}
	return(GOODNAME);
}

/*	"find_def" scans "/etc/gettydefs" for a string with the		*/
/*	requested "id".  If the "id" is NULL, then the first entry is	*/
/*	taken, hence the first entry must be the default entry.		*/
/*	If a match for the "id" is found, then the line is parsed and	*/
/*	the Gdef structure filled.  Errors in parsing generate error	*/
/*	messages on the system console.					*/

struct Gdef *find_def(id)
char *id;
{
	register struct Gdef *gptr;
	register char *ptr,c;
	FILE *fp;
	int i,input,state,size,rawc,field;
	char oldc,*optr,quoted(),*gdfile;
	char line[MAXLINE+1];
	static struct Gdef def;
	extern struct Gdef DEFAULT;
	static char d_id[MAXIDLENGTH+1],d_nextid[MAXIDLENGTH+1];
	static char d_message[MAXMESSAGE+1];
	extern char *GETTY_DEFS;
	extern char *getword(),*fields(),*speed();
	extern int check;
	extern char *checkgdfile;
	static char *states[] = {
		"","id","initial flags","final flags","message","next id"
	};

/* Decide whether to read the real /etc/gettydefs or the supplied */
/* check file. */
	if (check) gdfile = checkgdfile;
	else gdfile = GETTY_DEFS;

/* Open the "/etc/gettydefs" file.  Be persistent. */
	for (i=0; i < 3;i++) {
		if ((fp = fopen(gdfile,"r")) != NULL) break;
		else sleep(3);	/* Wait a little and then try again. */
	}

/* If unable to open, complain and then use the built in default. */
	if (fp == NULL) {
		error("getty: can't open \"%s\".\n",gdfile);
		return(&DEFAULT);
	}

/* Start searching for the line with the proper "id". */
	input = ACTIVE;
	do {
		for(ptr= line,oldc='\0'; ptr <= &line[sizeof(line)-1] &&
		    (rawc = getc(fp)) != EOF; ptr++,oldc = c) {
			c = *ptr = rawc;

/* Search for two \n's in a row. */
			if (c == '\n' && oldc == '\n') break;
		}

/* If we didn't end with a '\n' or EOF, then the line is too long. */
/* Skip over the remainder of the stuff in the line so that we */
/* start correctly on next line. */
		if (rawc != EOF && c != '\n') {
			for (oldc='\0'; (rawc = getc(fp)) != EOF;oldc=c) {
				c = rawc;
				if (c == '\n' && oldc != '\n') break;
			}
			if (check) fprintf(stdout,"Entry too long.\n");
		}

/* If we ended at the end of the file, then if there is no */
/* input, break out immediately otherwise set the "input" */
/* flag to FINISHED so that the "do" loop will terminate. */
		if (rawc == EOF) {
/* *ptr='\0'; added 030386 by gat: needed if last line in gettydefs is not */
/*                                 a crt and line length is 0 mod 4 and    */
/*                                 register vars are not guarrented        */
		        *ptr='\0';
			if (ptr == line) break;
			else input = FINISHED;
		}

/* If the last character stored was an EOF or '\n', replace it */
/* with a '\0'. */
		if (*ptr == (EOF & 0377) || *ptr == '\n') *ptr = '\0';

/* If the buffer is full, then make sure there is a null after the */
/* last character stored. */
		else *++ptr == '\0';
		if (check) fprintf(stdout,"\n**** Next Entry ****\n%s\n",line);

/* If line starts with #, treat as comment */
		if(line[0] == '#') continue;

/* Initialize "def" and "gptr". */
		gptr = &def;
		gptr->g_id = (char*)NULL;
		gptr->g_iflags.c_iflag = 0;
		gptr->g_iflags.c_oflag = 0;
		gptr->g_iflags.c_cflag = 0;
		gptr->g_iflags.c_lflag = 0;
		gptr->g_fflags.c_iflag = 0;
		gptr->g_fflags.c_oflag = 0;
		gptr->g_fflags.c_cflag = 0;
		gptr->g_fflags.c_lflag = 0;
		gptr->g_message = (char*)NULL;
		gptr->g_nextid = (char*)NULL;

/* Now that we have the complete line, scan if for the various */
/* fields.  Advance to new field at each unquoted '#'. */
		for (state=ID,ptr= line; state != FAILURE && state != SUCCESS;) {
			switch(state) {
			case ID:

/* Find word in ID field and move it to "d_id" array. */
				strncpy(d_id,getword(ptr,&size),MAXIDLENGTH);
				gptr->g_id = d_id;

/* Move to the next field.  If there is anything but white space */
/* following the id up until the '#', then set state to FAILURE. */
				ptr += size;
				while (isspace(*ptr)) ptr++;
				if (*ptr != '#') {
					field = state;
					state = FAILURE;
				} else {
					ptr++;	/* Skip the '#' */
					state = IFLAGS;
				}
				break;

/* Extract the "g_iflags" */
			case IFLAGS:
				if ((ptr = fields(ptr,&gptr->g_iflags)) == NULL) {
					field = state;
					state = FAILURE;
				} else {
					if((gptr->g_iflags.c_cflag & CSIZE) == 0)
						gptr->g_iflags.c_cflag |= CS8;
					gptr->g_iflags.c_cflag |= CREAD|HUPCL;
					ptr++;
					state = FFLAGS;
				}
				break;

/* Extract the "g_fflags". */
			case FFLAGS:
				if ((ptr = fields(ptr,&gptr->g_fflags)) == NULL) {
					field = state;
					state = FAILURE;
				} else {

/* Force the CREAD mode in regardless of what the user specified. */
					gptr->g_fflags.c_cflag |= CREAD;
					ptr++;
					state = MESSAGE;
				}
				break;

/* Take the entire next field as the "login" message. */
/* Follow usual quoting procedures for control characters. */
			case MESSAGE:
				for (optr= d_message; (c = *ptr) != '\0'
				    && c != '#';ptr++) {

/* If the next character is a backslash, then get the quoted */
/* character as one item. */
					if (c == '\\') {
						c = quoted(ptr,&size);
/* -1 accounts for ++ that takes place later. */
						ptr += size - 1;
					}

/* If there is room, store the next character in d_message. */
					if (optr < &d_message[MAXMESSAGE])
						*optr++ = c;
				}

/* If we ended on a '#', then all is okay.  Move state to NEXTID. */
/* If we didn't, then set state to FAILURE. */
				if (c == '#') {
					gptr->g_message = d_message;
					state = NEXTID;

/* Make sure message is null terminated. */
					*optr++ = '\0';
					ptr++;
				} else {
					field = state;
					state = FAILURE;
				}
				break;

/* Finally get the "g_nextid" field.  If this is successful, then */
/* the line parsed okay. */
			case NEXTID:

/* Find the first word in the field and save it as the next id. */
				strncpy(d_nextid,getword(ptr,&size),MAXIDLENGTH);
				gptr->g_nextid = d_nextid;

/* There should be nothing else on the line.  Starting after the */
/* word found, scan to end of line.  If anything beside white */
/* space, set state to FAILURE. */
				ptr += size;
				while (isspace(*ptr)) ptr++;
				if (*ptr != '\0') {
					field = state;
					state = FAILURE;
				} else state = SUCCESS;
				break;
			}
		}

/* If a line was successfully picked up and parsed, compare the */
/* "g_id" field with the "id" we are looking for. */
		if (state == SUCCESS) {

/* If there is an "id", compare them. */
			if (id != NULL) {
				if (strcmp(id,gptr->g_id) == 0) {
					fclose(fp);
					return(gptr);
				}

/* If there is no "id", then return this first successfully */
/* parsed line outright. */
			} else if (check == FALSE) {
				fclose(fp);
				return(gptr);

/* In check mode print out the results of the parsing. */
			} else {
				fprintf(stdout,"id: %s\n",gptr->g_id);
				fprintf(stdout,"initial flags:\niflag- %o oflag- %o cflag- %o lflag- %o\n",
					gptr->g_iflags.c_iflag,
					gptr->g_iflags.c_oflag,
					gptr->g_iflags.c_cflag,
					gptr->g_iflags.c_lflag);
				fprintf(stdout,"final flags:\niflag- %o oflag- %o cflag- %o lflag- %o\n",
					gptr->g_fflags.c_iflag,
					gptr->g_fflags.c_oflag,
					gptr->g_fflags.c_cflag,
					gptr->g_fflags.c_lflag);
				fprintf(stdout,"message: %s\n",gptr->g_message);
				fprintf(stdout,"next id: %s\n",gptr->g_nextid);
			}

/* If parsing failed in check mode, complain, otherwise ignore */
/* the bad line. */
		} else if (check) {
			*++ptr = '\0';
			fprintf(stdout,"Parsing failure in the \"%s\" field\n\
%s<--error detected here\n",
				states[field],line);
		}
	} while (input == ACTIVE);

/* If no match was found, then return NULL. */
	fclose(fp);
	return(NULL);
}

char *getword(ptr,size)
register char *ptr;
int *size;
{
	register char *optr,c;
	char quoted();
	static char word[MAXIDLENGTH+1];
	int qsize;

/* Skip over all white spaces including quoted spaces and tabs. */
	for (*size=0; isspace(*ptr) || *ptr == '\\';) {
		if (*ptr == '\\') {
			c = quoted(ptr,&qsize);
			(*size) += qsize;
			ptr += qsize+1;

/* If this quoted character is not a space or a tab or a newline */
/* then break. */
			if (isspace(c) == 0) break;
		} else {
			(*size)++;
			ptr++;
		}
	}

/* Put all characters from here to next white space or '#' or '\0' */
/* into the word, up to the size of the word. */
	for (optr= word,*optr='\0'; isspace(*ptr) == 0 &&
	    *ptr != '\0' && *ptr != '#'; ptr++,(*size)++) {

/* If the character is quoted, analyze it. */
		if (*ptr == '\\') {
			c = quoted(ptr,&qsize);
			(*size) += qsize;
			ptr += qsize;
		} else c = *ptr;

/* If there is room, add this character to the word. */
		if (optr < &word[MAXIDLENGTH+1] ) *optr++ = c;
	}

/* Make sure the line is null terminated. */
	*optr++ = '\0';
	return(word);
}

/*	"quoted" takes a quoted character, starting at the quote	*/
/*	character, and returns a single character plus the size of	*/
/*	the quote string.  "quoted" recognizes the following as		*/
/*	special, \n,\r,\v,\t,\b,\f as well as the \nnn notation.	*/

char quoted(ptr,qsize)
char *ptr;
int *qsize;
{
	register char c,*rptr;
	register int i;

	rptr = ptr;
	switch(*++rptr) {
	case 'n':
		c = '\n';
		break;
	case 'r':
		c = '\r';
		break;
	case 'v':
		c = '\013';
		break;
	case 'b':
		c = '\b';
		break;
	case 't':
		c = '\t';
		break;
	case 'f':
		c = '\f';
		break;
	default:

/* If this is a numeric string, take up to three characters of */
/* it as the value of the quoted character. */
		if (*rptr >= '0' && *rptr <= '7') {
			for (i=0,c=0; i < 3;i++) {
				c = c*8 + (*rptr - '0');
				if (*++rptr < '0' || *rptr > '7') break;
			}
			rptr--;

/* If the character following the '\\' is a NULL, back up the */
/* ptr so that the NULL won't be missed.  The sequence */
/* backslash null is essentually illegal. */
		} else if (*rptr == '\0') {
			c = '\0';
			rptr--;

/* In all other cases the quoting does nothing. */
		} else c = *rptr;
		break;
	}

/* Compute the size of the quoted character. */
	(*qsize) = rptr - ptr + 1;
	return(c);
}

/*	"fields" picks up the words in the next field and converts all	*/
/*	recognized words into the proper mask and puts it in the target	*/
/*	field.								*/

char *fields(ptr,termio)
register char *ptr;
struct termio *termio;
{
	extern struct Symbols imodes[],omodes[],cmodes[],lmodes[];
	extern struct Symbols *search();
	register struct Symbols *symbol;
	char *word,*getword();
	int size;
	extern int check;

	termio->c_iflag = 0;
	termio->c_oflag = 0;
	termio->c_cflag = CS8;
	termio->c_lflag = 0;
	while (*ptr != '#' && *ptr != '\0') {

/* Pick up the next word in the sequence. */
		word = getword(ptr,&size);

/* If there is a word, scan the two mode tables for it. */
		if (*word != '\0') {

/* If the word is the special word "SANE", put in all the flags */
/* that are needed for SANE tty behavior. */
			if (strcmp(word,"SANE") == 0) {
				termio->c_iflag |= ISANE;
				termio->c_oflag |= OSANE;
				if ((CSANE & CSIZE) != 0)
					termio->c_cflag &= ~CSIZE;
				termio->c_cflag |= CSANE;
				termio->c_lflag |= LSANE;
			} else if ((symbol = search(word,imodes)) != NULL)
				termio->c_iflag |= symbol->s_value;
			else if ((symbol = search(word,omodes)) != NULL)
				termio->c_oflag |= symbol->s_value;
			else if ((symbol = search(word,cmodes)) != NULL) {
				if ((symbol->s_value & CSIZE) != 0)
					termio->c_cflag &= ~CSIZE;
				termio->c_cflag |= symbol->s_value;
			} else if ((symbol = search(word,lmodes)) != NULL)
				termio->c_lflag |= symbol->s_value;
#ifdef MRTS_ON
			else if (strcmp(word, "MRTS") == 0)
				mrts_flg = 1;
#endif
			else if (check) fprintf(stdout,"Undefined: %s\n",word);
		}

/* Advance pointer to after the word. */
		ptr += size;
	}

/* If we didn't end on a '#', return NULL, otherwise return the */
/* updated pointer. */
	return(*ptr != '#' ? NULL : ptr);
}

/*	"parse" breaks up the user's response into seperate arguments	*/
/*	and fills the supplied array with those arguments.  Quoting	*/
/*	with the backspace is allowed.					*/

parse(string,args,cnt)
char *string,**args;
int cnt;
{
	register char *ptrin,*ptrout;
	register int i;
	extern char quoted();
	int qsize;

	for (i=0; i < cnt; i++) args[i] = (char *)NULL;
	for (ptrin = ptrout = string,i=0; *ptrin != '\0' && i < cnt; i++) {

/* Skip excess white spaces between arguments. */
		while(*ptrin == ' ' || *ptrin == '\t') {
			ptrin++;
			ptrout++;
		}

/* Save the address of the argument if there is something there. */
		if (*ptrin == '\0') break;
		else args[i] = ptrout;

/* Span the argument itself.  The '\' character causes quoting */
/* of the next character to take place (except for '\0'). */
		while (*ptrin != '\0') {

/* Is this the quote character? */
			if (*ptrin == '\\') {
				*ptrout++ = quoted(ptrin,&qsize);
				ptrin += qsize;

/* Is this the end of the argument?  If so quit loop. */
			} else if (*ptrin == ' ' || *ptrin == '\t') {
				ptrin++;
				break;

/* If this is a normal letter of the argument, save it, advancing */
/* the pointers at the same time. */
			} else *ptrout++ = *ptrin++;
		}

/* Null terminate the string. */
		*ptrout++ = '\0';
	}
}

FILE *fdup(fp)
register FILE *fp;
{
	register int newfd;
	register char *mode;

/* Dup the file descriptor for the specified stream and then */
/* convert it to a stream pointer with the modes of the original */
/* stream pointer. */
	if ((newfd = dup(fileno(fp))) != FAILURE) {

/* Determine the proper mode.  If the old file was _IORW, then */
/* use the "r+" option, if _IOREAD, the "r" option, or if _IOWRT */
/* the "w" option.  Note that since none of these force an lseek */
/* by "fdopen", the dupped file pointer will be at the same spot */
/* as the original. */
		if (fp->_flag & _IORW) mode = "r+";
		else if (fp->_flag & _IOREAD) mode = "r";
		else if (fp->_flag & _IOWRT) mode = "w";

/* Something is wrong, close dupped descriptor and return NULL. */
		else {
			close(newfd);
			return(NULL);
		}

/* Now have fdopen finish the job of establishing a new file pointer. */
		return(fdopen(newfd,mode));
	} else return(NULL);
}

#ifndef SYS_III
/* LIBRARY ROUTINES ** with no `/etc/utmp.lck' */


/*	Routines to read and write the /etc/utmp file.			*/
/*									*/
#include	<sys/param.h>
#include	<errno.h>

#define	MAXFILE	79	/* Maximum pathname length for "utmp" file */

#ifdef	DEBUG
#undef	UTMP_FILE
#define	UTMP_FILE "utmp"
#endif

static int fd = -1;	/* File descriptor for the utmp file. */
static char utmpfile[MAXFILE+1] = UTMP_FILE;	/* Name of the current
						 * "utmp" like file.
						 */
static long loc_utmp;	/* Where in "utmp" the current "ubuf" was
			 * found.
			 */
static struct utmp ubuf;	/* Copy of last entry read in. */


/* "getutent" gets the next entry in the utmp file.
 */

struct utmp *getutent()
{
	extern int fd;
	extern char utmpfile[];
	extern struct utmp ubuf;
	extern long loc_utmp,lseek();
	extern int errno;
	register char *u;
	register int i;
	struct stat stbuf;

/* If the "utmp" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */

	if (fd < 0) {

/* Make sure file is a multiple of 'utmp' entries long */
		if (stat(utmpfile,&stbuf) == 0) {
			if((stbuf.st_size % sizeof(struct utmp)) != 0) {
				unlink(utmpfile);
			}
		}
		if ((fd = open(utmpfile, O_RDWR|O_CREAT, 0644)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "_pututline()" later will fail the writes.
 */
			if (errno == EACCES
			    && (fd = open(utmpfile, O_RDONLY)) < 0)
				return(NULL);
		}
	}

/* Try to read in the next entry from the utmp file.  */
	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {

/* Make sure ubuf is zeroed. */
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		loc_utmp = 0;
		return(NULL);
	}

/* Save the location in the file where this entry was found. */
	loc_utmp = lseek(fd,0L,1) - (long)(sizeof(struct utmp));
	return(&ubuf);
}

/*	"getutid" finds the specified entry in the utmp file.  If	*/
/*	it can't find it, it returns NULL.				*/

struct utmp *getutid(entry)
register struct utmp *entry;
{
	extern struct utmp ubuf;
	struct utmp *getutent();
	register short type;

/* Start looking for entry.  Look in our current buffer before */
/* reading in new entries. */
	do {

/* If there is no entry in "ubuf", skip to the read. */
		if (ubuf.ut_type != EMPTY) {
			switch(entry->ut_type) {

/* Do not look for an entry if the user sent us an EMPTY entry. */
			case EMPTY:
				return(NULL);

/* For RUN_LVL, BOOT_TIME, OLD_TIME, and NEW_TIME entries, only */
/* the types have to match.  If they do, return the address of */
/* internal buffer. */
			case RUN_LVL:
			case BOOT_TIME:
			case OLD_TIME:
			case NEW_TIME:
				if (entry->ut_type == ubuf.ut_type) return(&ubuf);
				break;

/* For INIT_PROCESS, LOGIN_PROCESS, USER_PROCESS, and DEAD_PROCESS */
/* the type of the entry in "ubuf", must be one of the above and */
/* id's must match. */
			case INIT_PROCESS:
			case LOGIN_PROCESS:
			case USER_PROCESS:
			case DEAD_PROCESS:
				if (((type = ubuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				    && ubuf.ut_id[0] == entry->ut_id[0]
				    && ubuf.ut_id[1] == entry->ut_id[1]
				    && ubuf.ut_id[2] == entry->ut_id[2]
				    && ubuf.ut_id[3] == entry->ut_id[3])
					return(&ubuf);
				break;

/* Do not search for illegal types of entry. */
			default:
				return(NULL);
			}
		}
	} while (getutent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* "getutline" searches the "utmp" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct utmp *getutline(entry)
register struct utmp *entry;
{
	extern struct utmp ubuf,*getutent();
	register struct utmp *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &ubuf;
	do {
/* If the current entry is the one we are interested in, return */
/* a pointer to it. */
		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(entry->ut_line,
		    cur->ut_line,sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = getutent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"_pututline" writes the structure sent into the utmp file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmp file.							*/

struct utmp *_pututline(entry)
struct utmp *entry;
{
	void setutent();
	register int i,type;
	int fc;
	struct utmp *answer;
	struct stat statbuf;
	extern long time();
	extern struct utmp ubuf;
	extern long loc_utmp,lseek();
	extern struct utmp *getutid();
	extern int fd,errno;
	struct utmp tmpbuf;

/* Copy the user supplied entry into our temporary buffer to */
/* avoid the possibility that the user is actually passing us */
/* the address of "ubuf". */
	tmpbuf = *entry;
	getutent();
	if (fd < 0) {
#ifdef	ERRDEBUG
		gdebug("_pututline: Unable to create utmp file.\n");
#endif
		return((struct utmp *)NULL);
	}
/* Make sure file is writable */
	if ((fc=fcntl(fd, F_GETFL, NULL)) == -1
	    || (fc & O_RDWR) != O_RDWR) {
		return((struct utmp *)NULL);
	}

/* Find the proper entry in the utmp file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Making sure the location is an integral number of */
/* utmp structures into the file incase the file is scribbled.) */

	if (getutid(&tmpbuf) == NULL) {
#ifdef	ERRDEBUG
		gdebug("First getutid() failed.  fd: %d",fd);
#endif
		setutent();
		if (getutid(&tmpbuf) == NULL) {
#ifdef	ERRDEBUG
			loc_utmp = lseek(fd, 0L, 1);
			gdebug("Second getutid() failed.  fd: %d loc_utmp: %ld\n",fd,loc_utmp);
#endif
			fcntl(fd, F_SETFL, fc | O_APPEND);
		} else {
			lseek(fd, -(long)sizeof(struct utmp), 1);
		}
	} else {
		lseek(fd, -(long)sizeof(struct utmp), 1);
	}

/* Write out the user supplied structure.  If the write fails, */
/* then the user probably doesn't have permission to write the */
/* utmp file. */
	if (write(fd,&tmpbuf,sizeof(tmpbuf)) != sizeof(tmpbuf)) {
#ifdef	ERRDEBUG
		gdebug("_pututline failed: write-%d\n",errno);
#endif
		answer = (struct utmp *)NULL;
	} else {
/* Copy the user structure into ubuf so that it will be up to */
/* date in the future. */
		ubuf = tmpbuf;
		answer = &ubuf;

#ifdef	ERRDEBUG
		gdebug("id: %c%c loc: %x\n",ubuf.ut_id[0],ubuf.ut_id[1],
		    ubuf.ut_id[2],ubuf.ut_id[3],loc_utmp);
#endif
	}
	fcntl(fd, F_SETFL, fc);
	return(answer);
}

/*	"setutent" just resets the utmp file back to the beginning.	*/

void
setutent()
{
	register char *ptr;
	register int i;
	extern int fd;
	extern struct utmp ubuf;
	extern long loc_utmp;

	if (fd != -1) lseek(fd,0L,0);

/* Zero the stored copy of the last entry read, since we are */
/* resetting to the beginning of the file. */

	for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
	loc_utmp = 0L;
}

/*	"endutent" closes the utmp file.				*/

void
endutent()
{
	extern int fd;
	extern long loc_utmp;
	extern struct utmp ubuf;
	register char *ptr;
	register int i;

	if (fd != -1) close(fd);
	fd = -1;
	loc_utmp = 0;
	for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"utmpname" allows the user to read a file other than the	*/
/*	normal "utmp" file.						*/

utmpname(newfile)
char *newfile;
{
	extern char utmpfile[];

/* Determine if the new filename will fit.  If not, return 0. */
	if (strlen(newfile) > MAXFILE) return (0);

/* Otherwise copy in the new file name. */
	else strcpy(utmpfile,newfile);

/* Make sure everything is reset to the beginning state. */
	endutent();
	return(1);
}

#ifdef	ERRDEBUG
#include	<stdio.h>

gdebug(format,arg1,arg2,arg3,arg4,arg5,arg6)
char *format;
int arg1,arg2,arg3,arg4,arg5,arg6;
{
	register FILE *fp;
	register int errnum;
	extern int errno;

	if ((fp = fopen("/etc/dbg.getut","a+")) == NULL) return;
	fprintf(fp,format,arg1,arg2,arg3,arg4,arg5,arg6);
	fclose(fp);
}
#endif
#endif
