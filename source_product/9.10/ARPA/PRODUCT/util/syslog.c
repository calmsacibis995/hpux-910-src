/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslog.c	5.9 (Berkeley) 5/7/86";
#endif LIBC_SCCS and not lint

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by /etc/syslogd.
 *
 * Author: Eric Allman
 * Modified to use UNIX domain IPC by Ralph Campbell
 */
#ifdef _NAMESPACE_CLEAN
#define strchr    _strchr
#define strncpy   _strncpy
#define sys_nerr  _sys_nerr
#define sys_errlist _sys_errlist
#define syslog    _syslog
#define sprintf   _sprintf
#define ctime     _ctime
#define getpid    _getpid
#define strlen    _strlen
#define time      _time
#define strcpy    _strcpy
#define write     _write
#define sendto    _sendto
#define vfork     _vfork
#define signal    _signal
#define sigsetmask __sigsetmask
#define sigblock   __sigblock
#define alarm     _alarm
#define open      _open
#define strcat    _strcat
#define close     _close
#define wait      _wait
#define fcntl     _fcntl
#define socket    _socket
#define closelog  _closelog
#define openlog _openlog
#define setlogmask _setlogmask
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <syslog.h>
#include <netdb.h>
#include <string.h>
#include <sys/param.h>


#define index   strchr
#ifdef hpux
#define bzero(a,b)      memset(a,0,b)
#define bcopy(a,b,c)    memcpy(b,a,c)
#define strcpyn         strncpy
/* sigmask also defined in sys/signal.h */
#ifndef __hp9000s300
#ifndef __hp9000s800
#define sigmask(x)      (1 << (x - 1))
#endif  /* __hp9000s800 */
#endif  /* __hp9000s300 */
#endif  /* hpux */

#define	MAXLINE	256			/* max message size */
#define NULL	0			/* manifest */

#define PRIMASK(p)	(1 << ((p) & LOG_PRIMASK))
#define PRIFAC(p)	(((p) & LOG_FACMASK) >> 3)
#define IMPORTANT 	LOG_ERR

static char	logname[] = "/dev/log";
static char	ctty[] = "/dev/console";

static int	LogFile = -1;		/* fd for log */
static int	LogStat	= 0;		/* status bits, set by openlog() */
static char	*LogTag = "syslog";	/* string to tag the entry with */
static int	LogMask = 0xff;		/* mask of priorities to be logged */
static int	LogFacility = LOG_USER;	/* default facility code */

static struct sockaddr SyslogAddr;	/* AF_UNIX address of local logger */

extern	int errno, sys_nerr;
extern	char *sys_errlist[];
#ifdef _NAMESPACE_CLEAN
#undef syslog
#pragma _HP_SECONDARY_DEF _syslog syslog
#define syslog _syslog
#endif
syslog(pri, fmt, p0, p1, p2, p3, p4)
	int pri;
	char *fmt;
{
	char buf[MAXLINE + 1], outline[MAXLINE + 1];
	register char *b, *f, *o;
	register int c;
	long now;
	int pid, olderrno = errno;

	/* see if we should just throw out this message */
	if (pri <= 0 || PRIFAC(pri) >= LOG_NFACILITIES || (PRIMASK(pri) & LogMask) == 0)
		return;
	if (LogFile < 0)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* build the message */
	o = outline;
	sprintf(o, "<%d>", pri);
	o += strlen(o);
	time(&now);
	sprintf(o, "%.15s ", ctime(&now) + 4);
	o += strlen(o);
	if (LogTag) {
		strcpy(o, LogTag);
		o += strlen(o);
	}
	if (LogStat & LOG_PID) {
		sprintf(o, "[%d]", getpid());
		o += strlen(o);
	}
	if (LogTag) {
		strcpy(o, ": ");
		o += 2;
	}

	b = buf;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && b < &buf[MAXLINE]) {
		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}
		if ((unsigned)olderrno > sys_nerr)
			sprintf(b, "error %d", olderrno);
		else
			strcpy(b, sys_errlist[olderrno]);
		b += strlen(b);
	}
	*b++ = '\n';
	*b = '\0';
	sprintf(o, buf, p0, p1, p2, p3, p4);
	c = strlen(outline);
	if (c > MAXLINE)
		outline[MAXLINE-1]=0;

	/* output the message to the local logger */
#ifdef hpux
	if (write(LogFile, outline, MAXLINE) >= 0)
#else
	if (sendto(LogFile, outline, c, 0, &SyslogAddr, sizeof SyslogAddr) >= 0)
#endif
		return;
	if (!(LogStat & LOG_CONS))
		return;

	/* output the message to the console */
	pid = vfork();
	if (pid == -1)
		return;
	if (pid == 0) {
		int fd;

		signal(SIGALRM, SIG_DFL);
		sigsetmask(sigblock(0) & ~sigmask(SIGALRM));
		alarm(5);
		fd = open(ctty, O_WRONLY);
		alarm(0);
		strcat(o, "\r");
		o = index(outline, '>') + 1;
		write(fd, o, c + 1 - (o - outline));
		close(fd);
		_exit(0);
	}
	if (!(LogStat & LOG_NOWAIT))
		while ((c = wait((int *)0)) > 0 && c != pid)
			;
}

/*
 * OPENLOG -- open system log
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef openlog
#pragma _HP_SECONDARY_DEF _openlog openlog
#define openlog _openlog
#endif

openlog(ident, logstat, logfac)
	char *ident;
	int logstat, logfac;
{
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0)
		LogFacility = logfac & LOG_FACMASK;
	if (LogFile >= 0)
		return;
#ifdef hpux
	if (LogStat & LOG_NDELAY) {
		if((LogFile = open("/dev/log", O_WRONLY | O_NONBLOCK)) < 0)
		      return(-1);
		fcntl(LogFile, F_SETFD, 1);
	}
#else
	SyslogAddr.sa_family = AF_UNIX;
	strncpy(SyslogAddr.sa_data, logname, sizeof SyslogAddr.sa_data);
	if (LogStat & LOG_NDELAY) {
		LogFile = socket(AF_UNIX, SOCK_DGRAM, 0);
		fcntl(LogFile, F_SETFD, 1);
	}
#endif
}

/*
 * CLOSELOG -- close the system log
 */
#ifdef _NAMESPACE_CLEAN
#undef closelog
#pragma _HP_SECONDARY_DEF _closelog closelog
#define closelog _closelog
#endif

closelog()
{

	(void) close(LogFile);
	LogFile = -1;
}

/*
 * SETLOGMASK -- set the log mask level
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setlogmask
#pragma _HP_SECONDARY_DEF _setlogmask setlogmask
#define setlogmask _setlogmask
#endif

setlogmask(pmask)
	int pmask;
{
	int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
