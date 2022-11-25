/* @(#) $Revision: 64.4 $ */

/**********************************************************************

  ftio_rmt.c (original file is dumprmt.c in 4.3BSD dump)

  This file contains functions  which communicate the  remote host  in
  order to access remote device.

**********************************************************************/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "ftio.h"
#include <sys/mtio.h>
#include <sys/ioctl.h>
/*#include <sys/utsname.h>*/
#include <errno.h>
#include <fcntl.h>
#include "BSDmtio.h"

#define	TS_CLOSED	0
#define	TS_OPEN		1
#define	P_RD		0
#define	P_WR		1
#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2

static	int	rmtstate = TS_CLOSED;
static	int	rmtaper, rmtapew;
char	*host = NULL;		/* remote host name */
static	char	dev_name[256];		/* device name */

static int rmthost();
static int rmtopen();
static int rmtcreat();
static int rmtclose();
static int rmtread();
static int rmtwrite();
static int rmtioctl();
static int rmtcall();
static void rmtgets();
static int rmtreply();
static int rmtgetb();
static int is_remote();
static char *get_host();
static char *get_device();
static void disconnect();
static int connect();
static int rmtstatus();
static int rmtfstat();
static void rmtconnaborted();

int rmt_open(path, oflag, mode)
char path[];
int oflag, mode;
{
	if (is_remote(path)) {
		if (connect(get_host(path)) == -1) {
			return (-1);
		}
	} else if (host) {
		disconnect();
	}
	return (host ? rmtopen(get_device(path), oflag) : open(path, oflag, mode));
}

int rmt_creat(path, mode)
char path[];
int mode;
{
	if (is_remote(path)) {
		if (connect(get_host(path)) == -1) {
			return (-1);
		}
	} else if (host) {
		disconnect();
	}
	return (host ? rmtcreat(get_device(path), mode) : creat(path, mode));
}

int rmt_close(filedes)
int filedes;
{
	return (host ? rmtclose() : close(filedes));
}

int rmt_read(fildes, buf, nbyte)
int fildes;
char *buf;
unsigned nbyte;
{
	return (host ? rmtread(buf, nbyte) : read(fildes, buf, nbyte));
}

int rmt_write(fildes, buf, nbyte)
int fildes;
char *buf;
unsigned nbyte;
{
	return (host ? rmtwrite(buf, nbyte) : write(fildes, buf, nbyte));
}

int rmt_ioctl(fildes, request, arg)
int fildes, request;
void *arg;
{
	if (!host)
		return (ioctl(fildes, request, arg));

	switch (request) {
	case MTIOCTOP:
		return (rmtioctl((struct mtop *)arg));

	case MTIOCGET:
		return (rmtstatus((struct mtget *)arg));

	default:
		return (-1);
	}
}

int rmt_fstat(fd, buf)
int fd;
struct stat *buf;
{
	return (host && rmtstate == TS_OPEN ? rmtfstat(buf) : fstat(fd, buf));
}

int rmt_stat(path, buf)
char path[];
struct stat *buf;
{
	int	fd, st;

	if ((fd = rmt_open(path, O_RDONLY)) == -1)
		return (-1);
	st = rmt_fstat(fd, buf);
	rmt_close(fd);
	return (st);
}

static void rmtconnaborted()
{

	(void)ftio_mesg(FM_LOSTC);
	(void)myexit(1);
}

static int rmthost(rhost)
char rhost[];
{
	int	pin[2], pout[2];

	if (pipe(pin) == -1 || pipe(pout) == -1) {
		return (-1);
	}

	switch (fork()) {
	case -1: /* fork fail */
		return (-1);

	case 0:	/* child */
		(void) close(pin[P_RD]);
		(void) close(pout[P_WR]);

		(void) close(STDIN);
		dup(pout[P_RD]);
		(void) close(pout[P_RD]);

		(void) close(STDOUT);
		dup(pin[P_WR]);
		(void) close(pin[P_WR]);

		execl("/usr/bin/remsh", "remsh", rhost, "/etc/rmt", NULL);
		_exit(1);
	}
	/* parent */
	(void) close(pin[P_WR]);
	(void) close(pout[P_RD]);
	rmtaper = pin[P_RD];
	rmtapew = pout[P_WR];
	signal(SIGPIPE, rmtconnaborted);
	return (0);
}

static int rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	strcpy(dev_name, tape);
	sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}

static int rmtcreat(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	sprintf(buf, "o%s\n%o\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}

static int rmtclose()
{

	if (rmtstate != TS_OPEN)
		return (-1);
	rmtstate = TS_CLOSED;
	return (rmtcall("close", "C\n"));
}

static int rmtread(buf, count)
	char *buf;
	unsigned count;
{
	char line[30];
	int n, i, cc;

	sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtaper, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

static int rmtwrite(buf, count)
	char *buf;
	unsigned count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtapew, line, strlen(line));
	write(rmtapew, buf, count);
	return (rmtreply("write"));
}

static int rmtioctl(arg)
struct mtop *arg;
{
	char buf[256];

	if (arg->mt_count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", arg->mt_op, arg->mt_count);
	return (rmtcall("ioctl", buf));
}

static int rmtcall(cmd, buf)
	char *cmd, *buf;
{
	if (write(rmtapew, buf, strlen(buf)) != strlen(buf)) {
		rmtconnaborted();
	}
	return (rmtreply(cmd));
}

static void rmtgets(cp, len)
	char *cp;
	int len;
{

	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = 0;
			return;
		}
		cp++;
		len--;
	}
	rmtconnaborted();
}

static int rmtreply(cmd)
	char *cmd;
{
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		errno = atoi(code + 1);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

static int rmtgetb()
{
	char c;
	int	st;

	if ((st = read(rmtaper, &c, 1)) != 1) {
		rmtconnaborted();
	}
	return (c);
}

static int is_remote(path)
char path[];
{
	return (strchr(path, ':') != NULL);
}

static char *get_host(path)
char path[];
{
	static char hostname[UTSLEN + 1];
	char *p = path;
	int i;

	for (i = 0; i < UTSLEN && *p != ':'; i++, p++) {
		hostname[i] = *p;
	}
	return(hostname);
}

static char *get_device(path)
char path[];
{
	char *p;

	if ((p = strchr(path, ':')) == NULL)
		return (path);
	return (&p[1]);
}

static void disconnect()
{
	(void) close(rmtaper);
	(void) close(rmtapew);
	host = NULL;
}

static int connect(rhost)
char rhost[];
{
	static char hostname[UTSLEN + 1];

	if (host) {
		if (strcmp(rhost, host) == 0) {
			return (0);
		} else {
			disconnect();
		}
	}
	if (rmthost(rhost) == -1) {
		host = NULL;
		return (-1);
	} else {
		host = strncpy(hostname, rhost, UTSLEN);
		return (0);
	}
}

static int rmtstatus(mts)
struct mtget *mts;
{
	register int i, st;
	register char *cp, ch;
	struct BSDmtget BSDmts;

	if (rmtstate != TS_OPEN)
		return (-1);
	if ((st = rmtcall("status", "S")) <= 0)
		return (-1);
	for (i = 0, cp = (char *)&BSDmts; i < st; i++)
		*cp++ = rmtgetb();
	mts->mt_type = BSDmts.mt_type;
	mts->mt_resid = BSDmts.mt_resid;
	mts->mt_gstat = BSDmts.mt_dsreg;
	mts->mt_erreg = BSDmts.mt_erreg;	
	return (0);
}

static int rmtfstat(buf)
struct stat *buf;
{
	char	ch, *cp;
	int	i, st;

	if (rmtstate != TS_OPEN)
		return (-1);
	if ((st = rmtcall("stat", "s")) == -1)
		return (-1);
	for (i = 0, cp = (char *)buf; i < st; i++) {
		ch = rmtgetb();
		if (sizeof(*buf) == st)
			*cp++ = ch;
	}
	return (sizeof(*buf) == st ? 0 : -1);
}
