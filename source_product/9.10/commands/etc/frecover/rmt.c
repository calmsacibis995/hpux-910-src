/* @(#) $Revision: 72.1 $ */

/**********************************************************************

  rmt.c (original file is dumprmt.c in 4.3BSD dump)

  This file contains functions  which communicate the  remote host  in
  order to access remote device.

**********************************************************************/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#ifdef hpux
#include <signal.h>
#include <sys/utsname.h>
#include <string.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "frecover.h"

#define	TS_CLOSED	0
#define	TS_OPEN		1
#define	P_RD		0
#define	P_WR		1
#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2

static	int	rmtstate = TS_CLOSED;
static	int	rmtaper, rmtapew;
static	char	*host = NULL;		/* remote host name */
extern int machine_type ;         /* machine type of where tape is*/

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
static int connect();
static int rmtstatus();
static int rmtfstat();
static void disconnect();

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
	return (0);
}

static int rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

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
			return (-1);
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
		return (-1);
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
	return;
}

static int rmtreply(cmd)
	char *cmd;
{
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		errno = atoi(cmd + 1);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		return (-1);
	}
	return (atoi(code + 1));
}

static int rmtgetb()
{
	char c;

	if (read(rmtaper, &c, 1) != 1) {
		return (EOF);
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
	hostname[i] = '\0';
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

static void disconnect()
{
	(void) close(rmtaper);
	(void) close(rmtapew);
	host = NULL;
}

static int rmtstatus(mtget_buf)
struct mtget *mtget_buf;
{
	register int n, cc, i, st;
	char buf[BUFSIZ];
	char *ans;
	char *tok;
	char *s;
	char field[100], value[100];

	if (rmtstate != TS_OPEN)
		return (-1);
	if ((st = rmtcall("status", "m")) == -1)
		return (-1);
	for (i = 0 ; i < st ; i += cc) {
	  cc = read(rmtaper, buf+i, st - i);
	  if (cc <= 0) {
	    return (-1);
	  }
	}
    
  tok = "\n";
  ans = buf;
  
  while ((s = strtok(ans, tok)) != 0) {
    ans = 0;
    n = sscanf(s, "%s %s", field, value);

    if (strcmp(field, "machine") == 0) {
      if (strncmp(value, "9000/3", 6) == 0) {
	machine_type = 300;
      } else  if (strncmp(value, "9000/4", 6) == 0) {
	machine_type = 300;
      } else  if (strncmp(value, "9000/7", 6) == 0) {
	machine_type = 700;
      } else  {
	machine_type = 800;
      }
    }
    else  if (strcmp(field, "mt_type") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_type));
    }
    else  if (strcmp(field, "mt_resid") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_resid));
    }
    else  if (strcmp(field, "mt_dsreg1") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_dsreg1));
    }
    else  if (strcmp(field, "mt_dsreg2") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_dsreg2));
    }
    else  if (strcmp(field, "mt_gstat") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_gstat));
    }
    else  if (strcmp(field, "mt_erreg") == 0) {
      sscanf(value, "%li", &(mtget_buf->mt_erreg));
    }
#ifdef __hp9000s800
    else  if (machine_type == 800) {
      if (strcmp(field, "mt_fileno") == 0) {
	sscanf(value, "%li", &(mtget_buf->mt_fileno));
      }
      else  if (strcmp(field, "mt_blkno") == 0) {
	sscanf(value, "%li", &(mtget_buf->mt_blkno));
      }
    }
#endif /* __hp9000s800 */
    
  } /* while strtok */
  
	return (st);
}

static int rmtfstat(stat_buf)
struct stat *stat_buf;
{
	int	n, i, st, cc;
	unsigned int f;
	short z;
	char buf[BUFSIZ];
	char *ans;
	char *tok;
	char *s;
	char field[100], value[100];

	if (rmtstate != TS_OPEN)
		return (-1);
	if ((st = rmtcall("stat", "f")) == -1)
		return (-1);
	for (i = 0; i < st; i += cc) {
		cc = read(rmtaper, buf+i, st - i);
		if (cc <= 0) {
			return (-1);
		}
	}
    
  tok = "\n";
  ans = buf;
  
  while ((s = strtok(ans, tok)) != 0) {
    ans = 0;
    n = sscanf(s, "%s %s", field, value);

    if (strcmp(field, "machine") == 0) {
      if (strncmp(value, "9000/3", 6) == 0) {
	machine_type = 300;
      } else if (strncmp(value, "9000/4", 6) == 0) {
	machine_type = 300;
      } else if (strncmp(value, "9000/7", 6) == 0) {
	machine_type = 700;
      }
      else {
	machine_type = 800;
      }
    }
    else  if (strcmp(field, "st_dev") == 0) {
      sscanf(value, "%li", &(stat_buf->st_dev));
    }
    else  if (strcmp(field, "st_ino") == 0) {
      sscanf(value, "%lu", &(stat_buf->st_ino));
    }
    else  if (strcmp(field, "st_mode") == 0) {
      sscanf(value, "%hi", &(stat_buf->st_mode));
    }
    else  if (strcmp(field, "st_nlink") == 0) {
      sscanf(value, "%hi", &(stat_buf->st_nlink));
    }
    else  if (strcmp(field, "st_reserved1") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_reserved1));
    }
    else  if (strcmp(field, "st_reserved2") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_reserved2));
    }
    else  if (strcmp(field, "st_rdev") == 0) {
      sscanf(value, "%li", &(stat_buf->st_rdev));
    }
    else  if (strcmp(field, "st_size") == 0) {
      sscanf(value, "%li", &(stat_buf->st_size));
    }
    else  if (strcmp(field, "st_atime") == 0) {
      sscanf(value, "%li", &(stat_buf->st_atime));
    }
    else  if (strcmp(field, "st_mtime") == 0) {
      sscanf(value, "%li", &(stat_buf->st_mtime));
    }
    else  if (strcmp(field, "st_ctime") == 0) {
      sscanf(value, "%li", &(stat_buf->st_ctime));
    }
    else  if (strcmp(field, "st_spare1") == 0) {
      sscanf(value, "%i", &(stat_buf->st_spare1));
    }
    else  if (strcmp(field, "st_spare2") == 0) {
      sscanf(value, "%i", &(stat_buf->st_spare2));
    }
    else  if (strcmp(field, "st_spare3") == 0) {
      sscanf(value, "%i", &(stat_buf->st_spare3));
    }
    else  if (strcmp(field, "st_blksize") == 0) {
      sscanf(value, "%li", &(stat_buf->st_blksize));
    }
    else  if (strcmp(field, "st_pad") == 0) {
      sscanf(value, "%hi", &f);
      stat_buf->st_pad = f;
    }
    else  if (strcmp(field, "st_acl") == 0) {
      sscanf(value, "%hi", &f);
      stat_buf->st_acl = f;
    }
    else  if (strcmp(field, "st_remote") == 0) {
      sscanf(value, "%hi", &f);
      stat_buf->st_remote = f;
    }
    else  if (strcmp(field, "st_netdev") == 0) {
      sscanf(value, "%li", &(stat_buf->st_netdev));
    }
    else  if (strcmp(field, "st_netino") == 0) {
      sscanf(value, "%lu", &(stat_buf->st_netino));
    }
    else  if (strcmp(field, "st_cnode") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_cnode));
    }
    else  if (strcmp(field, "st_rcnode") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_rcnode));
    }
    else  if (strcmp(field, "st_netsite") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_netsite));
    }
    else  if (strcmp(field, "st_fstype") == 0) {
      sscanf(value, "%hi", &z);
      stat_buf->st_fstype = z;
    }
    else  if (strcmp(field, "st_realdev") == 0) {
      sscanf(value, "%li", &(stat_buf->st_realdev));
    }
    else  if (strcmp(field, "st_basemode") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_basemode));
    }
    else  if (strcmp(field, "st_spareshort") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_spareshort));
    }
#ifdef _CLASSIC_ID_TYPES
    else  if (strcmp(field, "st_filler_uid") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_filler_uid));
    }
    else  if (strcmp(field, "st_uid") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_uid));
    }
    else  if (strcmp(field, "st_filler_gid") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_filler_gid));
    }
    else  if (strcmp(field, "st_gid") == 0) {
      sscanf(value, "%hu", &(stat_buf->st_gid));
    }
#else /*  _CLASSIC_ID_TYPES */
    else  if (strcmp(field, "st_uid") == 0) {
      sscanf(value, "%li", &(stat_buf->st_uid));
    }
    else  if (strcmp(field, "st_gid") == 0) {
      sscanf(value, "%li", &(stat_buf->st_gid));
    }
#endif /*  _CLASSIC_ID_TYPES */
  } /* while strtok */
  
	return (st);
}
