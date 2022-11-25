/* @(#) $Revision: 72.1 $ */

/**********************************************************************

  rmt.c (original file is dumprmt.c in 4.3BSD dump)

  This file contains functions  which communicate the  remote host  in
  order to access remote device.

  Additional functions for the support of the VDI_REMOTE parts were
  put into this file so that all calls having to do with remote
  tapes are here and the old tape.c module from fbackup/frecover
  only deals with local tapes.  This allows usage by hsbackup of
  the vdi interface.  (Dan Matheson, CSB R&D, 28 Nov 1990)

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
#include "head.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define	TS_CLOSED	0
#define	TS_OPEN		1
#define	P_RD		0
#define	P_WR		1
#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2

#ifdef NLS
#include <locale.h>
#define NL_SETN 3       /* message set number */
nl_catd nlmsg_fd;
#endif NLS

static	int	rmtstate = TS_CLOSED;
static	int	rmtaper, rmtapew;
char	*host;		/* remote host name */
extern	void	wrtrabort();

extern int machine_type;         /* machine type where the tape device is, rmt.c
			     can overwrite */

PADTYPE	*pad;
RECTYPE *rec;

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
static int rconnect();
static int rmtstatus();
static int rmtfstat();

/*
 *  A number of routines have been added here to get all remote
 *  mag tape routines into one file.
 */

/*************************************************************************
    This function will turn a tape drive off line; however, because of a bug
    in the tape driver (it ALWAYS prints an error message no matter how one
    turns the drive off line), it is commented out until this bug is fixed.
***************************************************************************/
mtoffl(outfptr)
   char *outfptr;
{
    /*********** remove this comment when the bug is fixed **************
    char cmd[MAXPATHLEN];

    msg((catgets(nlmsg_fd,NL_SETN,302,"fbackup(3302): taking the drive off line\n")));
    (void) sprintf(cmd, "mt -t %s offl", outfptr);
    (void) system(cmd);
    *********** remove this comment when the bug is fixed **************/
}



/***************************************************************************
    This function determines if the open file descriptor outfd is attached
    to a magnetic tape drive.  It returns TRUE if so, FALSE if not.
***************************************************************************/
int
isamagtape(fd)
     int fd;
{
    struct mtget mtget_buf;

/*
    printf("tape.c:isamagtape rmt_ioctl call\n");
*/
    
    if (rmt_ioctl(fd, MTIOCGET, &mtget_buf) == -1)
	return(FALSE);
    else
	return(TRUE);
}

/***************************************************************************
    This function queries the magnetic tape drive attached to the file
    descriptor outfd.  It returns an integer which is the status of the
    drive.  (Such things as EOT, off line, write protection, etc may be
    used by other parts of the writer process.)

    Calling the rmt_ioctl function has the side effect of setting the
    global variable machine_type.  This is used to indicate
    what type the remote machine is.  The S300 and S800 return status
    in different fields of the mtget structure.  (Dan Matheson, 9 July 1990)
***************************************************************************/
int
getstat(fd, mtget_buf)
     int fd;
     struct mtget *mtget_buf;
{

/*
    printf("tape.c:getstat rmt_ioctl call\n");
*/

    if (rmt_ioctl(fd, MTIOCGET, mtget_buf) == -1) {
	perror("fbackup(9999)");
	msg((catgets(nlmsg_fd,NL_SETN,303, "fbackup(3303): ioctl error, can't query outfile\n")));
	wrtrabort();
    }
    return(0);
}

/***************************************************************************
    This function is called to perform operations on the open magnetic tape
    file descriptor, outfd.  The operations it may perform are all the legal
    operations that ioctl may perform, such as REWIND, OFFLINE, write and
    EOF, etc.  It returns FALSE if there was a problem, otherwise TRUE.
***************************************************************************/
int
mtoper(fd, operation, count)
     int fd;
     int operation;
     int count;
{
    struct mtop mtop_buf;
    int r;

    mtop_buf.mt_op = operation;
    mtop_buf.mt_count = count;
    r = rmt_ioctl(fd, MTIOCTOP, &mtop_buf);
    if (r == -1) {
	perror("fbackup(9999)");
	msg((catgets(nlmsg_fd,NL_SETN,304, "fbackup(3304): magtape operation error\n")));
	return(FALSE);
    } else
	return(TRUE);
}


/***************************************************************************
    This function is called to open the output file.  It returns TRUE if it
    succeeds, FALSE if it fails.
***************************************************************************/
int openoutfd(path, fd)
     char *path;
     int *fd;
{
    if ((*fd = rmt_open(path, O_RDWR|O_CREAT|O_TRUNC, PROTECT)) < 0) {
	msg((catgets(nlmsg_fd,NL_SETN,305, "fbackup(3305): could not open output file %s\n")), path);
	*fd = CLOSED;
	return(FALSE);
    } else
	return(TRUE);
}

int creatoutfd(path, fd)
     char *path;
     int *fd;
{
    if ((*fd = rmt_creat(path, PROTECT)) < 0) {
	msg((catgets(nlmsg_fd,NL_SETN,301, "fbackup(3301): could not open output file %s\n")), path);
	*fd = CLOSED;
	return(FALSE);
    } else
	return(TRUE);
}


/***************************************************************************
    This function is called to close the output file, and, if it is a magtape
    drive in UCB mode with no rewind-on-close, it rewinds it after the close.
***************************************************************************/
close_magtape(outfptr, outfd, mttype)
     char *outfptr;
     int *outfd;
     int mttype;
{
    (void) rmt_close(outfd);
    *outfd = CLOSED;
    if (mttype == UCBNOREW) {
	rmt_rewind(outfptr);
    }
}


extern char *recbuf;

/***************************************************************************
    This function checks to see if a tape volume is "OK".  It tries to read
    (a label and) a volume header.  If this volume was last used for something
    other than fbackup, we know nothing of the history of the volume, and it
    is treated as being new (the number of uses is set to 1).  If it was last
    used by fbackup, it is first checked to see if it is a volume of this
    session.  If it is, the volume is rejected.  Next, n_uses (th number of
    times this volume has been used by fbackup) is extracted and incremented.
    If the number is too high, newtapeok asks if you want to use it anyway.
    If not, the new tape is rejected.  Otherwise, the tape is accepted.
***************************************************************************/
int
rmt_newtapeok(fd,n_uses)
     int fd;
     int *n_uses;
{
    VHDRTYPE *volhdr = (VHDRTYPE*)recbuf;
    int n=0, retval=TRUE;

    if ((rmt_read(fd,recbuf,(unsigned)sizeof(LABELTYPE)+1) == sizeof(LABELTYPE))
							&& (mtoper(fd, MTFSF, 1))
			&& (rmt_read(fd,volhdr,(unsigned)sizeof(VHDRTYPE)+1)
						== sizeof(VHDRTYPE))) {

	/* got a label and vol header */

	if ((atoi(volhdr->backupid.ppid) == pad->pid) &&
			    (atoi(volhdr->backupid.time) == pad->begtime)) {
	    n = atoi(volhdr->volno);
	    msg((catgets(nlmsg_fd,NL_SETN,306, "fbackup(3306): this is volume %d OF THIS SESSION!\n	rejecting this volume\n")), n);
	    retval = FALSE;
	} else {
	    *n_uses = atoi(volhdr->mediause);
	    msg((catgets(nlmsg_fd,NL_SETN,307, "fbackup(3307): volume %d has been used %d time(s) (maximum: %d)\n")),
					    pad->vol, *n_uses, pad->maxvoluses);
	    if ((*n_uses >= pad->maxvoluses) &&
		(!query((catgets(nlmsg_fd,NL_SETN,308, "fbackup(3308): do you want to use this volume anyway?\n")))))
		retval = FALSE;
	}
    } else {		/* didn't get a label and/or a vol header */
	msg((catgets(nlmsg_fd,NL_SETN,309, "fbackup(3309): unable to read a volume header\n")));
	*n_uses = 0;
    }
    (*n_uses)++;
    (void) mtoper(fd, MTREW, 1);
    return(retval);
}  /* end rmt_newtapeok */



int rmt_open(path, oflag, mode)
char path[];
int oflag, mode;
{
/*
  printf("entered rmt_open\n");
*/
  
	if (is_remote(path)) {
		if (rconnect(get_host(path)) == -1) {
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
		if (rconnect(get_host(path)) == -1) {
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
/*
  printf("entered rmt_read\n");
*/
  
	return (host ? rmtread(buf, nbyte) : read(fildes, buf, nbyte));
}

int rmt_write(fildes, buf, nbyte)
int fildes;
char *buf;
unsigned nbyte;
{
/*
  printf("entered rmt_write\n");
*/
  
	return (host ? rmtwrite(buf, nbyte) : write(fildes, buf, nbyte));
}

int rmt_ioctl(fildes, request, arg)
int fildes, request;
void *arg;
{
/*
  printf("entered rmt_ioctl\n");
*/
  
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
/*
  printf("entered rmt_fstat\n");
*/
  
	return (host && rmtstate == TS_OPEN ? rmtfstat(buf) : fstat(fd, buf));
}

int rmt_stat(path, buf)
char path[];
struct stat *buf;
{
 	int	fd, st;

/*
  printf("entered rmt_stat\n");
*/
  
	if ((fd = rmt_open(path, O_RDONLY, PROTECT)) == -1)
		return (-1);
	st = rmt_fstat(fd, buf);
	rmt_close(fd);
	return (st);
}

rmt_rewind(path)
char *path;
{
	char s[128];

	if (is_remote(path)) {
		(void) sprintf(s, "/usr/bin/remsh %s mt -t %s rewind &",
			       get_host(path), get_device(path));
	} else {
		(void) sprintf(s, "mt -t %s rewind &", path);
	}
	(void) system(s);
}

static int rmthost(rhost)
char rhost[];
{
	int	pin[2], pout[2];

/*
	printf("entered rmthost\n");
*/
	
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
	signal(SIGPIPE, wrtrabort);
	return (0);
}

static int rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

/*
	printf("entered rmtopen\n");
*/
	
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
			wrtrabort();
		}
	}
	return (n);
}

static int rmtwrite(buf, count)
	char *buf;
	unsigned count;
{
	char line[30];

/*
	printf("debug: in rmtwrite");
*/
	
	sprintf(line, "W%d\n", count);
	write(rmtapew, line, strlen(line));
	write(rmtapew, buf, count);
	return (rmtreply("write"));
}

static int rmtioctl(arg)
struct mtop *arg;
{
	char buf[256];

/*
	printf("entered rmtioctl\n");
*/
	
	if (arg->mt_count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", arg->mt_op, arg->mt_count);
	return (rmtcall("ioctl", buf));
}

static int rmtcall(cmd, buf)
	char *cmd, *buf;
{
	if (write(rmtapew, buf, strlen(buf)) != strlen(buf)) {
		wrtrabort();
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
	wrtrabort();
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
		wrtrabort();
	}
	return (atoi(code + 1));
}

static int rmtgetb()
{
	char c;

	if (read(rmtaper, &c, 1) != 1) {
		wrtrabort();
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

static void disconnect()
{
	(void) close(rmtaper);
	(void) close(rmtapew);
	host = NULL;
}

static int rconnect(rhost)
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

static int rmtstatus(mtget_buf)
struct mtget *mtget_buf;
{
	register int i, st, n, cc;
	char buf[BUFSIZ];
	char *ans;
	char *tok;
	char *s;
	char field[100], value[100];

/*
	printf("entered rmtstatus\n");
*/

	if (rmtstate != TS_OPEN)
		return (-1);
	if ((st = rmtcall("status", "m")) <= 0)
		return (-1);

/*
	printf("rmtstatus: st = %d\n", st);
*/

	for (i = 0 ; i < st ; i += cc) {
	  cc = read(rmtaper, buf+i, st - i);
	  if (cc <= 0) {
	    return (-1);
	  }
	}

/*
	printf("rmtstatus: cc = %d\n", cc);
	printf("rmtstatus: before while strtok\n");
	printf("rmtstatus: buf\n %s\n", buf);
*/
    
  tok = "\n";
  ans = buf;
  
  while ((s = strtok(ans, tok)) != 0) {
    ans = 0;
    n = sscanf(s, "%s %s", field, value);

/*
    printf("rmtstatus: field = %s : value = %s \n", field, value);    
*/

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
  

	return(0);
}

static int rmtfstat(stat_buf)
struct stat *stat_buf;
{
	int	i, st, n, cc;
	unsigned int f;
	short z;
	char buf[BUFSIZ];
	char *ans;
	char *tok;
	char *s;
	char field[100], value[100];

/*
	printf("entered rmtfstat\n");
*/
	
	if (rmtstate != TS_OPEN)
		return (-1);
	
	if ((st = rmtcall("stat", "f")) == -1)
		return (-1);

/*
	printf("rmtfstat: st = %d\n", st);
*/
	
	for (i = 0; i < st; i += cc) {
		cc = read(rmtaper, buf+i, st - i);
		if (cc <= 0) {
			return (-1);
		}
	}


/* debug 
	printf("rmtfstat: cc = %d\n", cc);
printf("rmtfstat before while strtok\n");
printf("rmtfstat: buf\n %s\n", buf);
*/	
    
  tok = "\n";
  ans = buf;
  
  while ((s = strtok(ans, tok)) != 0) {
    ans = 0;
    n = sscanf(s, "%s %s", field, value);

/* debug
printf("rmtfstat: field = %s : value = %s \n", field, value); 
*/
    

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
      sscanf(value, "%i", &(stat_buf->st_spare1));
    }
    else  if (strcmp(field, "st_spare3") == 0) {
      sscanf(value, "%i", &(stat_buf->st_spare1));
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
      sscanf(value, "%hi", &(z));
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
