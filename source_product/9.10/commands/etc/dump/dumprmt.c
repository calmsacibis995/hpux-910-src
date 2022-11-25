/* @(#)  $Revision: 72.4 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/inode.h>

#ifdef	hpux
#  include "BSDmtio.h"
#  include <sys/ino.h>
#  include <signal.h>
#  include <sys/stat.h>
#endif	hpux

#include <netinet/in.h>
#include <stdio.h>
#include <pwd.h>
#include <netdb.h>
#ifdef	hpux
#  include "dumprestore.h"
#else	hpux
#  include <protocols/dumprestore.h>
#endif	hpux

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
int	rmtape;
int	rmtconnaborted();
char	*rmtpeer;

extern int ntrec;		/* blocking factor on tape */

rmthost(host, how)
	char *host;
	char how;
{

	rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn(how);
	if (rmtape < 0)
		return (0);
	return (1);
}

rmtconnaborted()
{

	fprintf(stderr, "rdump: Lost connection to remote host.\n");
	exit(1);
}

rmtgetconn(how)
	char how;
{
	static struct servent *sp = 0;
	struct passwd *pw;
	char *name = "root";
	int size;

	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			fprintf(stderr, "rdump: shell/tcp: unknown service\n");
			exit(1);
		}
	}
#if defined(TRUX) && defined(B1)
	pw = getpwuid((uid_t) getluid());
#else
	pw = getpwuid(getuid());
#endif
	if (pw && pw->pw_name)
		name = pw->pw_name;
#ifdef V4FS
	rmtape = rcmd(&rmtpeer, sp->s_port, name, name, "/usr/sbin/rmt", 0);
#else /* V4FS */
	rmtape = rcmd(&rmtpeer, sp->s_port, name, name, "/etc/rmt", 0);
#endif /* V4FS */
	size = ntrec * TP_BSIZE;
#ifdef	FASTTCPIP
#ifdef	hpux
	/* HP-UX 6.x signed short bug */
	if (size > 32767)
		size = 32767;
#endif	hpux
	if (how = 'W')
		while (   size > TP_BSIZE 
		       && setsockopt(rmtape, SOL_SOCKET, SO_SNDBUF, &size, 
				     sizeof (size)) < 0)
			size -= TP_BSIZE;
	else
		while (   size > TP_BSIZE 
		       && setsockopt(rmtape, SOL_SOCKET, SO_RCVBUF, &size, 
				     sizeof (size)) < 0)
			size -= TP_BSIZE;
#endif	FASTTCPIP
}

#ifdef  hpux
rmtopen(tape, mode, errno)
	char *tape;
	int mode;
	int *errno;
{
	char buf[256];
	char code[30], emsg[BUFSIZ];

#if defined(TRUX) && defined(B1)
	msg_b1("rmtopen started, tape=%s, mode=%d\n", tape, mode);
#endif /* (TRUX) && (B1) */
	sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		*errno = atoi(code + 1);
		return (-1);
	}
	if (*code != 'A') {
		msg("Protocol to remote tape server botched (code %s?).\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}
#else   hpux
rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}
#endif  hpux

rmtclose()
{

	if (rmtstate != TS_OPEN)
		return;
	rmtcall("close", "C\n");
	rmtstate = TS_CLOSED;
}

rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;
	extern errno;

	sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		errno = n;
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

#if defined(TRUX) && defined(B1)
/* get the min and max level (in the case of multilevel device) */

rmtgetlevel(dev, level, direction, tapedev)
int                    level, direction, tapedev;
char                    *dev;
{
	/* 200 is a arbitrary number, should be big enough */
/*Q
	char line[200]; 	

	sprintf(line, "T%s\n%d\n%d\n%d\n", dev, level, direction, tapedev);
	return (rmtcall("getlevel", line));
*/
}
#endif /* (TRUX) && (B1) */


rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	int i,j;
	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}


rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	sprintf(line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}

struct	mtget mts;

struct mtget *
rmtstatus()
{
	register int i;
	register char *cp;
#ifdef  hpux
	register int n;
	struct BSDmtget BSDmts;
#endif  hpux

	if (rmtstate != TS_OPEN)
		return (0);
#ifdef  hpux
        n = rmtcall("status", "S");
        if (n <= 0)
                return (struct mtget *)NULL;
        for (i = 0, cp = (char *)&BSDmts; i < n; i++)
                *cp++ = rmtgetb();
        mts.mt_type = BSDmts.mt_type;
        mts.mt_resid = BSDmts.mt_resid;
        mts.mt_gstat = BSDmts.mt_dsreg;
        mts.mt_erreg = BSDmts.mt_erreg;
#else   hpux
	i = rmtcall("status", "S");
        if (i != sizeof(mts))
                return (0);
        for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
                *cp++ = rmtgetb();
#endif  hpux
        return (&mts);
}

rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	return (rmtreply(cmd));
}

rmtreply(cmd)
	char *cmd;
{
	register int c;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		msg("%s: %s\n", cmd, emsg);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		msg("Protocol to remote tape server botched (code %s?).\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1){
		rmtconnaborted();
	}
	return (c);
}

rmtgets(cp, len)
	char *cp;
	int len;
{

	char *scp;

	scp = cp;
	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = 0;
			return;
		}
		cp++;
		len--;
	}
	msg("Protocol to remote tape server botched (in rmtgets).\n");
	rmtconnaborted();
}


#ifdef  hpux
rmtfilecreate(file, mode)
        char *file;
        int mode;
{
        char buf[256];

	rmtstate = TS_OPEN;
        sprintf(buf, "o%s\n0%o\n", file, mode);
        write(rmtape, buf, strlen(buf));
        return (rmtreply("rmtfileopen"));
}

struct	stat statbuf;

struct stat *
rmtfilestatus()
{
    char buf[256];
    register int i;
    register char *cp;

        sprintf(buf, "s");
        write(rmtape, buf, strlen(buf));
        rmtreply("rmtfilestatus");
        for (i = 0, cp = (char *)&statbuf; i < sizeof(statbuf); i++)
                *cp++ = rmtgetb();
	return (&statbuf);
}

#endif  hpux
