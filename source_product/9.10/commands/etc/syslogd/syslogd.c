#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.2 $";
static char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * define KERNEL_LOGGING to enable processing log requests
 * from /dev/klog.
 */

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * NLOGS   -- the maximum number of simultaneous log files.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 */

#ifdef hpux
#define LOG_FIFO
#ifndef MAXHOSTNAMELEN
#   define MAXHOSTNAMELEN 64
#endif
#define index	strchr
#define bzero(a,b)	memset(a,0,b)
#define bcopy(a,b,c)	memcpy(b,a,c)
#define strcpyn		strncpy
#endif

#include <sys/stdsyms.h>
#include <limits.h>	/* for LINE_MAX */

#define	NLOGS		20		/* max number of log files */
#ifdef LINE_MAX
#   define	MAXLINE		LINE_MAX	/* maximum line length */
#else /* LINE_MAX */
#   define 	MAXLINE		2048		/* default value */
#endif
#define DEFUPRI		(LOG_USER|LOG_NOTICE)
#define DEFSPRI		(LOG_KERN|LOG_CRIT)
#define MARKCOUNT	10		/* ratio of minor to major marks */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <utmp.h>
#include <ctype.h>
#include <signal.h>
#ifndef hpux
#include <sysexits.h>
#endif
#include <string.h>
#ifdef hpux
#include <fcntl.h>
#endif

#ifdef hpux
#include <syslog.h>
#else
#include <sys/syslog.h>
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/msgbuf.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <netdb.h>

#define GETMAX(a, b) ((a) >= (b) ? (a) : (b))

char	*LogName  = "/dev/log";
char	*ConfFile = "/etc/syslog.conf";
#ifdef V4FS
char	*PidFile  = "/var/run/syslog.pid";
#else /* V4FS */
char	*PidFile  = "/etc/syslog.pid";
#endif /* V4FS */
char	ctty[]    = "/dev/console";

#define FDMASK(fd)	((fd) < 0 ? 0 : 1 << (fd))

#define	dprintf		if (Debug) printf

#define UNAMESZ		8	/* length of a login name */
#define MAXUNAMES	20	/* maximum number of user names */
#define MAXFNAME	200	/* max file pathname length */

#define NOPRI		0x10	/* the "no priority" priority */
#define	LOG_MARK	(LOG_NFACILITIES << 3)	/* mark "facility" */

/*
 * Flags to logmsg().
 */

#define IGN_CONS	0x001	/* don't print on console */
#define SYNC_FILE	0x002	/* do fsync on file after printing */
#define NOCOPY		0x004	/* don't suppress duplicate messages */
#define ADDDATE		0x008	/* add a date to the message */
#define MARK		0x010	/* this message is a mark */

/*
 * This structure represents the files that will have log
 * copies printed.
 */
struct filed {
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	union {
		char	f_uname[MAXUNAMES][UNAMESZ+1];
		struct {
			char	f_hname[MAXHOSTNAMELEN+1];
			struct sockaddr_in	f_addr;
		} f_forw;		/* forwarding address */
		char	f_fname[MAXFNAME];
	} f_un;
};

/* values for f_type */
#define F_UNUSED	0		/* unused entry */
#define F_FILE		1		/* regular file */
#define F_TTY		2		/* terminal */
#define F_CONSOLE	3		/* console terminal */
#define F_FORW		4		/* remote machine */
#define F_USERS		5		/* list of users */
#define F_WALL		6		/* everyone logged on */

char	*TypeNames[7] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL"
};

struct filed	Files[NLOGS];

int	Debug;			/* debug flag */
char	LocalHostName[MAXHOSTNAMELEN+1];	/* our hostname */
char	*LocalDomain;		/* our local domain name */
int	InetInuse = 1;		/* non-zero if INET sockets are being used */
int	LogPort;		/* port number for INET connections */
char	PrevLine[MAXLINE + 1];	/* copy of last line to supress repeats */
char	PrevHost[MAXHOSTNAMELEN+1];		/* previous host */
int	PrevFlags;
int	PrevPri;
int	PrevCount = 0;		/* number of times seen */
int	Initialized = 0;	/* set when we have initialized ourselves */
int	MarkInterval = 20;	/* interval between marks in minutes */
int	MarkSeq = 0;		/* mark sequence number */

extern	int errno, sys_nerr;
extern	char *sys_errlist[];
extern	char *ctime(), *index();
extern	struct utmp *getutent();

#ifdef hpux
extern int domark();
struct sigvec alrm_vec = {(void(*)())domark, 0, 0};
#endif

#ifdef LOG_FIFO
extern int no_networking();
struct sigvec sigsys_vec = {(void(*)())no_networking, 0, 0};
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *p;
	int funix, unixm, finet, inetm, fklog, klogm, len;
	struct sockaddr_un sun, fromunix;
	struct sockaddr_in sin, frominet;
	FILE *fp;
	char line[MSG_BSIZE + 1];
	extern int die(), domark(), reapchild();
	int c;
	extern char *optarg;
	extern int optind;
	struct stat sbuf;
	int maxfds;

	while ((c = getopt(argc, argv, "f:dm:p:")) != EOF)
		switch (c) {
		case 'f':		/* configuration file */
			ConfFile = optarg;
			break;

		case 'd':		/* debug */
			Debug++;
			break;

		case 'p':		/* path */
			LogName = optarg;
			break;

		case 'm':		/* mark interval */
			MarkInterval = atoi(optarg);
			break;

		case '?':
			usage();
		}

	/* We need to make a check of LogName before the fork
	 * so if there's a problem, we can tell somebody about it
	 */
	if(!stat(LogName,&sbuf)) 
		if(!S_ISFIFO(sbuf.st_mode)){
		    fprintf(stderr,"Syslogd: Cannot create %s as a FIFO.\n\t Syslog daemon not started.\n",LogName);
		    exit(1);
		}

	if (!Debug) {
		if (fork())
			exit(0);
		for (i = 0; i < 10; i++)
			(void) close(i);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		untty();
	} else
#ifndef hpux
		setlinebuf(stdout);
#else
		setvbuf(stdout, NULL, BUFSIZ);
#endif


	(void) gethostname(LocalHostName, sizeof LocalHostName);
	if (p = index(LocalHostName, '.')) {
		*p++ = '\0';
		LocalDomain = p;
	}
	else
		LocalDomain = "";
	(void) signal(SIGTERM, die);
	(void) signal(SIGINT, Debug ? (void(*)())die : SIG_IGN);
	(void) signal(SIGQUIT, Debug ? (void(*)())die : SIG_IGN);
#ifdef hpux
	(void) signal(SIGCLD, SIG_IGN);
	(void) sigvector(SIGALRM, &alrm_vec, 0);
#else
	(void) signal(SIGCHLD, reapchild);
	(void) signal(SIGALRM, domark);
#endif
#ifdef LOG_FIFO
	(void) sigvector(SIGSYS, &sigsys_vec, 0);
#endif
	(void) alarm(MarkInterval * 60 / MARKCOUNT);

#ifdef LOG_FIFO
	if (mknod(LogName, S_IFIFO, (dev_t)0) == 0 || errno == EEXIST) {
		chmod(LogName, 0666);
		if ((funix = open(LogName, O_RDONLY|O_NONBLOCK)) >= 0) {
		    int flags;

		    if ((flags = fcntl(funix, F_GETFL, 0)) < 0)
			dprintf("errno = %d\n", errno);
		    dprintf("flags = 0%o\n", flags);
		    flags &= ~O_NDELAY;
		    dprintf("flags = 0%o\n", flags);
		    if (fcntl(funix, F_SETFL, flags) < 0)
			dprintf("errno = %d\n", errno);
		}
		/*
		 * We can't open the FIFO for O_RDWR (P1003.1 disallows
		 * it), but we need the FIFO open for writing so that
		 * our select() call will block on the pipe.  So, we
		 * open the file for O_WRONLY [but never use it].
		 */
		if (open(LogName, O_WRONLY) < 0)
		{
		    /*
		     * If the open for writing fails, we must close the
		     * funix file.  If we don't, we will go into an
		     * infinite loop when we select() on it.
		     */
		    dprintf("errno = %d, can't open %s for O_WRONLY\n",
			errno, LogName);
		    close(funix);
		    funix = -1;
		}
	}
	else {
		fprintf(stderr, "syslogd: cannot create FIFO %s\n",
		    LogName);
		exit(1);
	}
	unixm = FDMASK(funix);
#else
	sun.sun_family = AF_UNIX;
	(void) strncpy(sun.sun_path, LogName, sizeof sun.sun_path);
	funix = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (funix < 0 || bind(funix, (struct sockaddr *) &sun,
	    sizeof(sun.sun_family)+strlen(sun.sun_path)) < 0 ||
	    chmod(LogName, 0666) < 0) {
		(void) sprintf(line, "cannot create %s", LogName);
		logerror(line);
		dprintf("cannot create %s (%d)\n", LogName, errno);
		die(0);
	}
#endif
	finet = socket(AF_INET, SOCK_DGRAM, 0);
	if (InetInuse && (finet >= 0)) {
		struct servent *sp;

		sp = getservbyname("syslog", "udp");
		if (sp == NULL) {
			errno = 0;
			logerror("syslog/udp: unknown service");
			dprintf("getservbyname failed\n");
			close(finet);
			finet = -1;
			InetInuse = 0;
		} else {
			sin.sin_family = AF_INET;
			sin.sin_port = LogPort = sp->s_port;
			sin.sin_addr.s_addr = INADDR_ANY;
			if (bind(finet, &sin, sizeof(sin)) < 0) {
				logerror("bind");
				dprintf("bind failed (%d)\n",errno);
				close(finet);
				finet = -1;
				InetInuse = 0;
			}
		}
	} else if (finet < 0) { /* socket call failed */
		logerror("socket");
		dprintf("socket failed (%d)\n",errno);
		InetInuse = 0;
	} else {  /* no networking in kernel */
		errno = 0;
		logerror("no networking in kernel");
		finet = -1;
	};
	inetm = FDMASK(finet);
#ifdef KERNEL_LOGGING
	if ((fklog = open("/dev/klog", O_RDONLY)) >= 0)
		klogm = FDMASK(fklog);
	else {
		dprintf("can't open /dev/klog (%d)\n", errno);
		klogm = 0;
	}
#else
	klogm = 0;
	fklog = -1;
#endif

#ifdef LOG_FIFO
	/* make sure we have valid FIFO fd as minimum */
	if (!unixm) {
		errno = 0;
		logerror("no input source");
		die(0);
	}
#endif

	/* tuck my process id away */
	fp = fopen(PidFile, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	dprintf("off & running....\n");

	init();
	(void) signal(SIGHUP, init);

	maxfds = GETMAX(fklog, GETMAX(finet, funix)) + 1;
	for (;;) {
		int nfds;
		int readfds = unixm | inetm | klogm;

		errno = 0;
#ifdef KERNEL_LOGGING
		dprintf("readfds = %#x %#x %#x %#x\n",
			readfds, funix, finet, fklog);
#else
		dprintf("readfds = %#x %#x %#x\n",
			readfds, funix, finet);
#endif
		nfds = select(maxfds,
			     (fd_set *)&readfds, (fd_set *)NULL,
			     (fd_set *)NULL, (struct timeval *)NULL);
		dprintf("got a message (%d, %#x)\n", nfds, readfds);
		if (nfds == 0)
			continue;
		if (nfds < 0) {
			if (errno != EINTR)
				logerror("select");
			continue;
		}
#ifdef KERNEL_LOGGING
		if (readfds & klogm) {
			i = read(fklog, line, sizeof(line) - 1);
			if (i > 0) {
				line[i] = '\0';
				printsys(line);
			} else if (i < 0 && errno != EINTR) {
				logerror("klog");
				fklog = -1;
				klogm = 0;
			}
		}
#endif
		if (readfds & unixm) {
#ifndef hpux
			len = sizeof fromunix;
#endif
#ifdef hpux
			i = read(funix, line, MAXLINE);
#else
			i = recvfrom(funix, line, MAXLINE, 0,
				     (struct sockaddr *)&fromunix, &len);
#endif
			if (i > 0) {
				line[i] = '\0';
				printline(LocalHostName, line);
			} else if (i < 0 && errno != EINTR)
#ifdef hpux
				logerror("read funix");
#else
				logerror("recvfrom unix");
#endif
		}
		if (readfds & inetm) {
			len = sizeof frominet;
			i = recvfrom(finet, line, MAXLINE, 0, &frominet, &len);
			if (i > 0) {
				extern char *cvthname();

				line[i] = '\0';
				printline(cvthname(&frominet), line);
			} else if (i < 0 && errno != EINTR)
				logerror("recvfrom inet");
		} 
	}
}

usage()
{
	fprintf(stderr, "usage: syslogd [-d] [-m markinterval] [-p path] [-f conffile]\n");
	exit(1);
}

untty()
{
	int i;

	if (!Debug) {
		i = open("/dev/tty", O_RDWR | O_NOCTTY);
		if (i >= 0) {
#ifdef  TIOCNOTTY
			(void) ioctl(i, (int) TIOCNOTTY, (char *)0);
#else
			setpgrp();
#endif

			(void) close(i);
		}
	}
}

/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */

printline(hname, msg)
	char *hname;
	char *msg;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri;

	/* test for special codes */
	pri = DEFUPRI;
	p = msg;
	if (*p == '<') {
		pri = 0;
		while (isdigit(*++p))
			pri = 10 * pri + (*p - '0');
		if (*p == '>')
			++p;
		if (pri <= 0 || pri >= (LOG_NFACILITIES << 3))
			pri = DEFUPRI;
	}

	/* don't allow users to log kernel messages */
	if ((pri & LOG_FACMASK) == LOG_KERN)
		pri |= LOG_USER;

	q = line;

	while ((c = *p++ & 0177) != '\0' && c != '\n' &&
	    q < &line[sizeof(line) - 1]) {
		if (iscntrl(c)) {
			*q++ = '^';
			*q++ = c ^ 0100;
		} else
			*q++ = c;
	}
	*q = '\0';

	logmsg(pri, line, hname, 0);
}

#ifdef KERNEL_LOGGING
/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */

printsys(msg)
	char *msg;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri, flags;
	char *lp;
	time_t now;

	(void) time(&now);
	(void) sprintf(line, "%.15s vmunix: ", ctime(&now) + 4);
	lp = line + strlen(line);
	for (p = msg; *p != '\0'; ) {
		flags = SYNC_FILE;	/* fsync file after write */
		pri = DEFSPRI;
		if (*p == '<') {
			pri = 0;
			while (isdigit(*++p))
				pri = 10 * pri + (*p - '0');
			if (*p == '>')
				++p;
			if (pri <= 0 || pri >= (LOG_NFACILITIES << 3))
				pri = DEFSPRI;
		} else {
			/* kernel printf's come out on console */
			flags |= IGN_CONS;
		}
		q = lp;
		while (*p != '\0' && (c = *p++) != '\n' &&
		    q < &line[MAXLINE])
			*q++ = c;
		*q = '\0';
		logmsg(pri, line, LocalHostName, flags);
	}
}
#endif /* KERNEL_LOGGING */

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */

logmsg(pri, msg, from, flags)
	int pri;
	char *msg, *from;
	int flags;
{
	register struct filed *f;
	register int l;
	int fac, prilev;
	time_t now;
	int omask;
	struct iovec iov[6];
	register struct iovec *v = iov;
	char line[MAXLINE + 1];

	dprintf("logmsg: pri %o, flags %x, from %s, msg %s\n", pri, flags, from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	/*
	 * Check to see if msg looks non-standard.
	 */
	if (strlen(msg) < 16 || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	if (!(flags & NOCOPY)) {
		if (flags & (ADDDATE|MARK))
			flushmsg();
		else if (!strcmp(msg + 16, PrevLine + 16)) {
			/* we found a match, update the time */
			(void) strncpy(PrevLine, msg, 15);
			PrevCount++;
			(void) sigsetmask(omask);
			return;
		} else {
			/* new line, save it */
			flushmsg();
			(void) strcpy(PrevLine, msg);
			(void) strcpy(PrevHost, from);
			PrevFlags = flags;
			PrevPri = pri;
		}
	}

	(void) time(&now);
	if (flags & ADDDATE)
		v->iov_base = ctime(&now) + 4;
	else
		v->iov_base = msg;
	v->iov_len = 15;
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	v->iov_base = from;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	if (flags & ADDDATE)
		v->iov_base = msg;
	else
		v->iov_base = msg + 16;
	v->iov_len = strlen(v->iov_base);
	v++;

	/* extract facility and priority level */
	fac = (pri & LOG_FACMASK) >> 3;
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	prilev = pri & LOG_PRIMASK;

	/* log the message to the particular outputs */
	if (!Initialized) {
		int cfd = open(ctty, O_WRONLY | O_NOCTTY);

		if (cfd >= 0) {
			v->iov_base = "\r\n";
			v->iov_len = 2;
			(void) writev(cfd, iov, 6);
			(void) close(cfd);
		}
		untty();
		(void) sigsetmask(omask);
		return;
	}
	for (f = Files; f < &Files[NLOGS]; f++) {
		/* skip messages that are incorrect priority */
		if (f->f_pmask[fac] < prilev || f->f_pmask[fac] == NOPRI)
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < (MarkInterval * 60 / 2))
			continue;

		dprintf("Logging to %s", TypeNames[f->f_type]);
		f->f_time = now;
		switch (f->f_type) {
		case F_UNUSED:
			dprintf("\n");
			break;

		case F_FORW:
			dprintf(" %s\n", f->f_un.f_forw.f_hname);
			(void) sprintf(line, "<%d>%.15s %s", pri,
				iov[0].iov_base, iov[4].iov_base);
			l = strlen(line);
			if (l > MAXLINE)
				l = MAXLINE;
			if (sendto(f->f_file, line, l, 0,
			    &f->f_un.f_forw.f_addr,
			    sizeof f->f_un.f_forw.f_addr) != l) {
				int e = errno;
				(void) close(f->f_file);
				f->f_type = F_UNUSED;
				errno = e;
				logerror("sendto");
			}
			break;

		case F_CONSOLE:
			if (flags & IGN_CONS) {
				dprintf(" (ignored)\n");
				break;
			}

		case F_TTY:
		case F_FILE:
			dprintf(" %s\n", f->f_un.f_fname);
			if (f->f_type != F_FILE) {
				v->iov_base = "\r\n";
				v->iov_len = 2;
			} else {
				v->iov_base = "\n";
				v->iov_len = 1;
			}
			if (writev(f->f_file, iov, 6) < 0) {
				int e = errno;
				(void) close(f->f_file);
				/*
				 * Check for EBADF on TTY's due to vhangup() XXX
				 */
				if (e == EBADF && f->f_type != F_FILE) {
					f->f_file = open(f->f_un.f_fname, O_WRONLY|O_APPEND);
					if (f->f_file < 0) {
						f->f_type = F_UNUSED;
						logerror(f->f_un.f_fname);
					}
				} else {
					f->f_type = F_UNUSED;
					errno = e;
					logerror(f->f_un.f_fname);
				}
			} else if (flags & SYNC_FILE)
				(void) fsync(f->f_file);
			break;

		case F_USERS:
		case F_WALL:
			dprintf("\n");
			v->iov_base = "\r\n";
			v->iov_len = 2;
			wallmsg(f, iov);
			break;
		}
	}

	(void) sigsetmask(omask);
}


/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */

wallmsg(f, iov)
	register struct filed *f;
	struct iovec *iov;
{
	register int i;
	struct utmp *ut;
	char p[6 + sizeof(ut->ut_line)]; /* space for "/dev/" + ut_line + NULL */
	int ttyf, len;
	static int reenter = 0;
	time_t now;
	char greetings[200];

	if (reenter++)
		return;

	/* open the user login file */
	if ((ut = getutent()) == NULL) {
		logerror("/etc/utmp");
		reenter = 0;
		return;
	}

	(void) time(&now);
	(void) sprintf(greetings,
	    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
		iov[2].iov_base, ctime(&now));
	len = strlen(greetings);

	/* scan the user login file */
	do {
		/* is this slot used? */
		if (ut->ut_type != USER_PROCESS)
			continue;

		/* should we send the message to this user? */
		if (f->f_type == F_USERS) {
			for (i = 0; i < MAXUNAMES; i++) {
				if (!f->f_un.f_uname[i][0]) {
					i = MAXUNAMES;
					break;
				}
				if (strncmp(f->f_un.f_uname[i], ut->ut_name,
				    UNAMESZ) == 0)
					break;
			}
			if (i >= MAXUNAMES)
				continue;
		}

		/* compute the device name */
		strcpyn(p,"/dev/",sizeof(p));
		strncat(p, ut->ut_line, sizeof (ut->ut_line));

		/*
		 * Might as well fork instead of using nonblocking I/O
		 * and doing notty().
		 */
		if (fork() == 0) {
			if (f->f_type == F_WALL) {
				iov[0].iov_base = greetings;
				iov[0].iov_len = len;
				iov[1].iov_len = 0;
			}
			(void) signal(SIGALRM, SIG_DFL);
			(void) alarm(30);
			/* open the terminal */
			ttyf = open(p, O_WRONLY | O_NOCTTY);
			if (ttyf >= 0)
				(void) writev(ttyf, iov, 6);
			exit(0);
		}
	} while ((ut = getutent()) != NULL);
	/* close the user login file */
	(void) endutent();
	reenter = 0;
}

reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, (struct rusage *) NULL) > 0)
		;
}

/*
 * Return a printable representation of a host address.
 */
char *
cvthname(f)
	struct sockaddr_in *f;
{
	struct hostent *hp;
	register char *p;
	extern char *inet_ntoa();

	dprintf("cvthname(%s)\n", inet_ntoa(f->sin_addr));

	if (f->sin_family != AF_INET) {
		dprintf("Malformed from address\n");
		return ("???");
	}
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0) {
		dprintf("Host name for your address (%s) unknown\n",
			inet_ntoa(f->sin_addr));
		return (inet_ntoa(f->sin_addr));
	}
	if ((p = index(hp->h_name, '.')) && strcmp(p + 1, LocalDomain) == 0)
		*p = '\0';
	return (hp->h_name);
}

domark()
{
	int pri;

	if (Debug)
		printf("In domark\n");

	if ((++MarkSeq % MARKCOUNT) == 0)
		logmsg(LOG_INFO, "-- MARK --", LocalHostName, ADDDATE|MARK);
	else
		flushmsg();
	alarm(MarkInterval * 60 / MARKCOUNT);
}

flushmsg()
{
	if (PrevCount == 0)
		return;
	if (PrevCount > 1)
		(void) sprintf(PrevLine+16, "last message repeated %d times", PrevCount);
	PrevCount = 0;
	logmsg(PrevPri, PrevLine, PrevHost, PrevFlags|NOCOPY);
	PrevLine[0] = '\0';
}

/*
 * Print syslogd errors some place.
 */
logerror(type)
	char *type;
{
	char buf[100];

	if (errno == 0)
		(void) sprintf(buf, "syslogd: %s", type);
	else if ((unsigned) errno > sys_nerr)
		(void) sprintf(buf, "syslogd: %s: error %d", type, errno);
	else
		(void) sprintf(buf, "syslogd: %s: %s", type, sys_errlist[errno]);
	errno = 0;
	dprintf("%s\n", buf);
	logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
}

die(sig)
{
	char buf[100];

	if (sig) {
		dprintf("syslogd: going down on signal %d\n", sig);
		flushmsg();
		(void) sprintf(buf, "going down on signal %d", sig);
		logerror(buf);
	}
	exit(0);
}

#ifdef LOG_FIFO
no_networking(sig)
{
	InetInuse = 0;
}
#endif

/*
 *  INIT -- Initialize syslogd from configuration table
 */

init()
{
	register int i;
	register FILE *cf;
	register struct filed *f;
	register char *p;
	char cline[BUFSIZ];

	/*  Reset signal, so next SIGHUP won't terminate  */
	signal(SIGHUP, init);

	dprintf("init\n");

	/* flush any pending output */
	flushmsg();

	/*
	 *  Close all open log files.
	 */
	for (f = Files; f < &Files[NLOGS]; f++) {
		if (f->f_type == F_FILE || f->f_type == F_TTY || f->f_type == F_CONSOLE || f->f_type == F_FORW)
			(void) close(f->f_file);
		f->f_type = F_UNUSED;
	}

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL) {
		dprintf("cannot open %s\n", ConfFile);
		cfline("*.ERR\t/dev/console", &Files[0]);
		cfline("*.PANIC\t*", &Files[1]);
		return;
	}

	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = Files;
	while (fgets(cline, sizeof cline, cf) != NULL && f < &Files[NLOGS]) {
		/* check for end-of-section */
		if (cline[0] == '\n' || cline[0] == '#')
			continue;

		/* strip off newline character */
		p = index(cline, '\n');
		if (p)
			*p = '\0';

		cfline(cline, f++);
	}

	/* close the configuration file */
	(void) fclose(cf);

	Initialized = 1;

	if (Debug) {
		for (f = Files; f < &Files[NLOGS]; f++) {
			for (i = 0; i <= LOG_NFACILITIES; i++)
				if (f->f_pmask[i] == NOPRI)
					printf("X ");
				else
					printf("%d ", f->f_pmask[i]);
			printf("%s: ", TypeNames[f->f_type]);
			switch (f->f_type) {
			case F_FILE:
			case F_TTY:
			case F_CONSOLE:
				printf("%s", f->f_un.f_fname);
				break;

			case F_FORW:
				printf("%s", f->f_un.f_forw.f_hname);
				break;

			case F_USERS:
				for (i = 0; i < MAXUNAMES && *f->f_un.f_uname[i]; i++)
					printf("%s, ", f->f_un.f_uname[i]);
				break;
			}
			printf("\n");
		}
	}

	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
}

/*
 * Crack a configuration file line
 */

struct code {
	char	*c_name;
	int	c_val;
};

struct code	PriNames[] = {
	"panic",	LOG_EMERG,
	"emerg",	LOG_EMERG,
	"alert",	LOG_ALERT,
	"crit",		LOG_CRIT,
	"err",		LOG_ERR,
	"error",	LOG_ERR,
	"warn",		LOG_WARNING,
	"warning",	LOG_WARNING,
	"notice",	LOG_NOTICE,
	"info",		LOG_INFO,
	"debug",	LOG_DEBUG,
	"none",		NOPRI,
	NULL,		-1
};

struct code	FacNames[] = {
	"kern",		LOG_KERN,
	"user",		LOG_USER,
	"mail",		LOG_MAIL,
	"daemon",	LOG_DAEMON,
	"auth",		LOG_AUTH,
	"security",	LOG_AUTH,
	"mark",		LOG_MARK,
	"syslog",	LOG_SYSLOG,
	"lpr",		LOG_LPR,
	"lp",		LOG_LPR,
	"local0",	LOG_LOCAL0,
	"local1",	LOG_LOCAL1,
	"local2",	LOG_LOCAL2,
	"local3",	LOG_LOCAL3,
	"local4",	LOG_LOCAL4,
	"local5",	LOG_LOCAL5,
	"local6",	LOG_LOCAL6,
	"local7",	LOG_LOCAL7,
	NULL,		-1
};

cfline(line, f)
	char *line;
	register struct filed *f;
{
	register char *p;
	register char *q;
	register int i;
	char *bp;
	int pri;
	struct hostent *hp;
	char buf[MAXLINE];

	dprintf("cfline(%s)\n", line);

	/* clear out file entry */
	bzero((char *) f, sizeof *f);
	for (i = 0; i <= LOG_NFACILITIES; i++)
		f->f_pmask[i] = NOPRI;

	/* scan through the list of selectors */
	for (p = line; *p && *p != '\t';) {

		/* find the end of this facility name list */
		for (q = p; *q && *q != '\t' && *q++ != '.'; )
			continue;

		/* reaching end of line is caused by spaces instead of
		 * tabs separating the selector from the action so
		 * bail out
		 */
		if (!*q)
		    break;

		/* collect priority name */
		for (bp = buf; *q && !index(" \t,;", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (index(", ;", *q))
			q++;

		/* decode priority name */
		pri = decode(buf, PriNames);
		if (pri < 0) {
			char xbuf[200];

			(void) sprintf(xbuf, "unknown priority name \"%s\"", buf);
			logerror(xbuf);
			return;
		}

		/* scan facilities */
		while (*p && !index("\t.;", *p)) {
			int i;

			for (bp = buf; *p && !index("\t,;.", *p); )
				*bp++ = *p++;
			*bp = '\0';
			if (*buf == '*')
				for (i = 0; i < LOG_NFACILITIES; i++)
					f->f_pmask[i] = pri;
			else {
				i = decode(buf, FacNames);
				if (i < 0) {
					char xbuf[200];

					(void) sprintf(xbuf, "unknown facility name \"%s\"", buf);
					logerror(xbuf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	while (*p == '\t')
		p++;

	switch (*p)
	{
	case '@':
		if (!InetInuse)
			break;
		(void) strcpy(f->f_un.f_forw.f_hname, ++p);
		hp = gethostbyname(p);
		if (hp == NULL) {
			char buf[100];

			(void) sprintf(buf, "unknown host %s", p);
			errno = 0;
			logerror(buf);
			break;
		}
		bzero((char *) &f->f_un.f_forw.f_addr,
			 sizeof f->f_un.f_forw.f_addr);
		f->f_un.f_forw.f_addr.sin_family = AF_INET;
		f->f_un.f_forw.f_addr.sin_port = LogPort;
		bcopy(hp->h_addr, (char *) &f->f_un.f_forw.f_addr.sin_addr, hp->h_length);
		f->f_file = socket(AF_INET, SOCK_DGRAM, 0);
		if (f->f_file < 0) {
			logerror("socket");
			break;
		}
		f->f_type = F_FORW;
		break;

	case '/':
		(void) strcpy(f->f_un.f_fname, p);
		if ((f->f_file = open(p, O_WRONLY|O_APPEND|O_CREAT|O_NOCTTY, 0444)) < 0) {
			logerror(p);
			break;
		}
		if (isatty(f->f_file)) {
			f->f_type = F_TTY;
			untty();
		}
		else
			f->f_type = F_FILE;
		if (strcmp(p, ctty) == 0)
			f->f_type = F_CONSOLE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		for (i = 0; i < MAXUNAMES && *p; i++) {
			for (q = p; *q && *q != ','; )
				q++;
			(void) strncpy(f->f_un.f_uname[i], p, UNAMESZ);
			if ((q - p) > UNAMESZ)
				f->f_un.f_uname[i][UNAMESZ] = '\0';
			else
				f->f_un.f_uname[i][q - p] = '\0';
			while (*q == ',' || *q == ' ')
				q++;
			p = q;
		}
		f->f_type = F_USERS;
		break;
	}
}


/*
 *  Decode a symbolic name to a numeric value
 */

decode(name, codetab)
	char *name;
	struct code *codetab;
{
	register struct code *c;
	register char *p;
	char buf[40];

	if (isdigit(*name))
		return (atoi(name));

	(void) strcpy(buf, name);
	for (p = buf; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}
