/* @(#)  $Revision: 72.2 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * rmt
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mtio.h>
#include <errno.h>
#ifdef  hpux
#  include <sys/stat.h>
#  include <fcntl.h>
#  include "BSDmtio.h"
#  include <sys/utsname.h>
#endif  hpux

int	tape = -1;

char	*record;
int	maxrecsize = -1;
char	*checkbuf();
void    getstring(), error();
#if defined(TRUX) && defined(B1)
#include <sys/security.h> 
#include <prot.h>
static mand_ir_t 	*tape_max_ir, *tape_min_ir;
char  			*dev;
int 			 lev, im_ex_port, fd,j, ret;
char 			*level, *direction, *tapedev;
union min_max_pair {
	mand_ir_t min_ir;
	mand_ir_t max_ir;
}un_ir;
#endif /* (TRUX) && (B1) */

#define	SSIZE	64
char	device[SSIZE];
char	count[SSIZE], mode[SSIZE], pos[SSIZE], op[SSIZE];

extern  char    *sys_errlist[];
char	resp[BUFSIZ];

long	lseek();

FILE	*debug;
#define	DEBUG(f)	if (debug) fprintf(debug, f)
#define	DEBUG1(f,a)	if (debug) fprintf(debug, f, a)
#define	DEBUG2(f,a1,a2)	if (debug) fprintf(debug, f, a1, a2)

main(argc, argv)
	int argc;
	char **argv;
{
	int rval;
	char c;
	int n, i, cc;
#ifdef  hpux
	int octal_mode;
	char buf[BUFSIZ];
	char ans[BUFSIZ];
	struct utsname name;
#endif  hpux

#if defined(TRUX) && defined(B1)
/*#define B1DEBUG		1 */
#ifdef B1DEBUG
	j = fopen("/rmt.b1dbg","w+");
	fprintf(j,"pid=%d ", getpid());
	fflush(j);
	/*fclose(j);*/
	/* while (1) ; */
#endif B1DEBUG
	ie_init(argc, argv, 1);
        forcepriv(SEC_ALLOWMACACCESS);
        forcepriv(SEC_ALLOWDACACCESS);

        forcepriv(SEC_NETPRIVADDR);
        forcepriv(SEC_REMOTE);
        forcepriv(SEC_NETNOAUTH);
        forcepriv(SEC_NETBROADCAST);
        forcepriv(SEC_NETSETID);
        forcepriv(SEC_NETRAWACCESS);

	if(!authorized_user("backup")){ 
		errno = EPERM;
		goto ioerror;
	}
#endif /* (TRUX) && (B1) */

	argc--, argv++;
	if (argc > 0) {
		debug = fopen(*argv, "w");
		if (debug == 0)
			exit(1);
		(void) setbuf(debug, (char *)0);
	}
top:
	errno = 0;
	rval = 0;
	if (read(0, &c, 1) != 1)
		exit(0);
	switch (c) {

	case 'O':
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
		fprintf(j,"tape=%d device=%s mode=%s\n", tape, device, mode);
		fflush(j);
#endif 
		if (tape >= 0)
			(void) close(tape);
		getstring(device); getstring(mode);
		DEBUG2("rmtd: O %s %s\n", device, mode);
		tape = open(device, atoi(mode));

#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
		fprintf(j,"device=%s mode=%d tape=%d err=%d\n", 
			device, atoi(mode),tape,errno);
		fflush(j);
#endif 
		if (tape < 0)
			goto ioerror;
		goto respond;

#if defined(TRUX) && defined(B1)
	case 'T':

		getstring(dev); getstring(level);
		getstring(direction); getstring(tapedev);
		sscanf(level, "%d", &lev);
		sscanf(direction, "%d", &im_ex_port);
		sscanf(tapedev, "%d", &fd);

		/* Watch out!! ie_check_device does not return anything.
		 * It exits upon failure
		*/
		ie_check_device(dev, lev, im_ex_port, fd);

		if (get_dev_range(dev) == 0){
			errno = EACCES;
			goto ioerror;
		}
		/* got the max and min IR for the tape, now send it */

		(void) sprintf(resp, "A%d\n", sizeof(un_ir.min_ir));
		(void) write(1, resp, strlen(resp));
		(void) sprintf(resp, "%0ld\n", un_ir.min_ir);
		(void) write(1, resp, strlen(resp));
		(void) sprintf(resp, "%0ld\n", sizeof(un_ir.max_ir));
		(void) write(1, resp, strlen(resp));

		goto top;
#endif /* (TRUX) && (B1) */

#ifdef  hpux
	case 'o':
		if (tape >= 0)
			(void) close(tape);
		getstring(device); getstring(mode);
		DEBUG2("rmtd: o %s %s\n", device, mode);
		sscanf(mode, "%o", &octal_mode);
		tape = open(device, O_RDWR | O_CREAT, octal_mode);
		if (tape < 0)
		    goto ioerror;
		goto respond;
#endif  hpux

	case 'C':
		DEBUG("rmtd: C\n");
		getstring(device);		/* discard */
		if (close(tape) < 0)
			goto ioerror;
		tape = -1;
		goto respond;

	case 'L':
		getstring(count); getstring(pos);
		DEBUG2("rmtd: L %s %s\n", count, pos);
		rval = lseek(tape, (long) atoi(count), atoi(pos));
		if (rval == -1)
			goto ioerror;
		goto respond;

	case 'W':
		getstring(count);
		n = atoi(count);
		DEBUG1("rmtd: W %s\n", count);
		record = checkbuf(record, n, c);
		for (i = 0; i < n; i += cc) {
			cc = read(0, &record[i], n - i);
			if (cc <= 0) {
				DEBUG("rmtd: premature eof\n");
				exit(2);
			}
		}
		rval = write(tape, record, n);
		if (rval < 0)
			goto ioerror;
		goto respond;

	case 'R':
		getstring(count);
		DEBUG1("rmtd: R %s\n", count);
		n = atoi(count);
		record = checkbuf(record, n, c);
		rval = read(tape, record, n);
		if (rval < 0)
			goto ioerror;
		(void) sprintf(resp, "A%d\n", rval);
		(void) write(1, resp, strlen(resp));
		(void) write(1, record, rval);
		goto top;

	case 'I':
		getstring(op); getstring(count);
		DEBUG2("rmtd: I %s %s\n", op, count);
		{ struct mtop mtop;
		  mtop.mt_op = atoi(op);
		  mtop.mt_count = atoi(count);
		  if (ioctl(tape, MTIOCTOP, (char *)&mtop) < 0)
			goto ioerror;
		  rval = mtop.mt_count;
		}
		goto respond;

	case 'S':		/* status */
		DEBUG("rmtd: S\n");
		{ struct mtget mtget;
		  struct BSDmtget BSDmtget;
		  if (ioctl(tape, MTIOCGET, (char *)&mtget) < 0)
			goto ioerror;
		  BSDmtget.mt_type = mtget.mt_type;
		  BSDmtget.mt_resid = mtget.mt_resid;
		  BSDmtget.mt_dsreg = mtget.mt_gstat;
		  BSDmtget.mt_erreg = mtget.mt_erreg;
		  rval = sizeof (BSDmtget);
		  (void) sprintf(resp, "A%d\n", rval);
		  (void) write(1, resp, strlen(resp));
		  (void) write(1, (char *)&BSDmtget, sizeof (BSDmtget));
		  goto top;
		}

#ifdef  hpux
	case 's':		/* disk status */
		DEBUG1("rmtd: s %s\n", device);
		{ struct stat statbuf;
		  if (fstat(tape, &statbuf) < 0)
			goto ioerror;
		  rval = sizeof (statbuf);
		  (void) sprintf(resp, "A%d\n", rval);
		  (void) write(1, resp, strlen(resp));
		  (void) write(1, (char *)&statbuf, sizeof (statbuf));
		  goto top;
		}
#endif  hpux

#ifdef  hpux
	case 'f':		/* disk status  ascii return*/
		DEBUG1("rmtd: f %s\n", device);
		{ struct stat statbuf;
		  if (fstat(tape, &statbuf) < 0)
			goto ioerror;

		  ans[0] = '\0';
		  uname(&name);
		  buf[0] = '\0';
		  sprintf(buf, "machine ");
		  strcat(buf, name.machine);
		  strcat(buf, "\n");
		  strcat(ans, buf);
		  sprintf(buf, "st_dev %li\n", statbuf.st_dev);
		  strcat(ans, buf);
		  sprintf(buf, "st_ino %lu\n", statbuf.st_ino);
		  strcat(ans, buf);
		  sprintf(buf, "st_mode %hi\n", statbuf.st_mode);
		  strcat(ans, buf);
		  sprintf(buf, "st_nlink %hi\n", statbuf.st_nlink);
		  strcat(ans, buf);
		  sprintf(buf, "st_reserved1 %hu\n", statbuf.st_reserved1);
		  strcat(ans, buf);
		  sprintf(buf, "st_reserved2 %hu\n", statbuf.st_reserved2);
		  strcat(ans, buf);
		  sprintf(buf, "st_rdev %li\n", statbuf.st_rdev);
		  strcat(ans, buf);
		  sprintf(buf, "st_size %li\n", statbuf.st_size);
		  strcat(ans, buf);
		  sprintf(buf, "st_atime %li\n", statbuf.st_atime);
		  strcat(ans, buf);
		  sprintf(buf, "st_spare1 %i\n", statbuf.st_spare1);
		  strcat(ans, buf);
		  sprintf(buf, "st_mtime %li\n", statbuf.st_mtime);
		  strcat(ans, buf);
		  sprintf(buf, "st_spare2 %i\n", statbuf.st_spare2);
		  strcat(ans, buf);
		  sprintf(buf, "st_ctime %li\n", statbuf.st_ctime);
		  strcat(ans, buf);
		  sprintf(buf, "st_spare3 %i\n", statbuf.st_spare3);
		  strcat(ans, buf);
		  sprintf(buf, "st_blksize %li\n", statbuf.st_blksize);
		  strcat(ans, buf);
#ifdef ACLS
		  sprintf(buf, "st_pad %hi\n", statbuf.st_pad);
		  strcat(ans, buf);
		  sprintf(buf, "st_acl %hi\n", statbuf.st_acl);
		  strcat(ans, buf);
#else /* not ACLS */
		  sprintf(buf, "st_pad %hi\n", statbuf.st_pad);
		  strcat(ans, buf);
#endif /* ACLS */
		  sprintf(buf, "st_remote %hi\n", statbuf.st_remote);
		  strcat(ans, buf);
		  sprintf(buf, "st_netdev %li\n", statbuf.st_netdev);
		  strcat(ans, buf);
		  sprintf(buf, "st_netino %lu\n", statbuf.st_netino);
		  strcat(ans, buf);
		  sprintf(buf, "st_cnode %hu\n", statbuf.st_cnode);
		  strcat(ans, buf);
		  sprintf(buf, "st_rcnode %hu\n", statbuf.st_rcnode);
		  strcat(ans, buf);
		  sprintf(buf, "st_netsite %hu\n", statbuf.st_netsite);
		  strcat(ans, buf);
		  sprintf(buf, "st_fstype %hi\n", statbuf.st_fstype);
		  strcat(ans, buf);
		  sprintf(buf, "st_realdev %li\n", statbuf.st_realdev);
		  strcat(ans, buf);
#ifdef ACLS
		  sprintf(buf, "st_basemode %hu\n", statbuf.st_basemode);
		  strcat(ans, buf);
		  sprintf(buf, "st_spareshort %hu\n", statbuf.st_spareshort);
		  strcat(ans, buf);
#else /* not ACLS */
		  sprintf(buf, "st_spare5 %hu\n", statbuf.st_spare5);
		  strcat(ans, buf);
		  sprintf(buf, "st_spare6 %hu\n", statbuf.st_spare6);
		  strcat(ans, buf);
#endif /* ACLS */
#ifdef  _CLASSIC_ID_TYPES
		  sprintf(buf, "st_filler_uid %hu\n", statbuf.st_filler_uid);
		  strcat(ans, buf);
		  sprintf(buf, "st_uid %hu\n", statbuf.st_uid);
		  strcat(ans, buf);
		  sprintf(buf, "st_filler_gid %hu\n", statbuf.st_filler_gid);
		  strcat(ans, buf);
		  sprintf(buf, "st_gid %hu\n", statbuf.st_gid);
		  strcat(ans, buf);
#else
		  sprintf(buf, "st_uid %li\n", statbuf.st_uid);
		  strcat(ans, buf);
		  sprintf(buf, "st_gid %li\n", statbuf.st_gid);
		  strcat(ans, buf);
#endif

		  rval = strlen(ans) + 1;
		  (void) sprintf(resp, "A%d\n", rval);
		  (void) write(1, resp, strlen(resp));
		  (void) write(1, ans, rval);
		  goto top;
		}
#endif  hpux

#ifdef  hpux
	case 'm':		/* mtiocget  ascii return*/
		DEBUG("rmtd: m\n");
		{ struct mtget mtget;
		  if (ioctl(tape, MTIOCGET, (char *)&mtget) < 0)
		    goto ioerror;
		
		  ans[0] = '\0';
		  uname(&name);
		  buf[0] = '\0';
		  sprintf(buf, "machine ");
		  strcat(buf, name.machine);
		  strcat(buf, "\n");
		  strcat(ans, buf);
		  sprintf(buf, "mt_type %li\n", mtget.mt_type);
		  strcat(ans, buf);
		  sprintf(buf, "mt_resid %li\n", mtget.mt_resid);
		  strcat(ans, buf);
		  sprintf(buf, "mt_dsreg1 %li\n", mtget.mt_dsreg1);
		  strcat(ans, buf);
		  sprintf(buf, "mt_dsreg2 %li\n", mtget.mt_dsreg2);
		  strcat(ans, buf);
		  sprintf(buf, "mt_gstat %li\n", mtget.mt_gstat);
		  strcat(ans, buf);
		  sprintf(buf, "mt_erreg %li\n", mtget.mt_erreg);
		  strcat(ans, buf);
#ifdef __hp9000s800
		  sprintf(buf, "mt_fileno %li\n", mtget.mt_fileno);
		  strcat(ans, buf);
		  sprintf(buf, "mt_blkno %li\n", mtget.mt_blkno);
		  strcat(ans, buf);
#endif /* __hp9000s800 */

		  rval = strlen(ans) + 1;
		  (void) sprintf(resp, "A%d\n", rval);
		  (void) write(1, resp, strlen(resp));
		  (void) write(1, ans, rval);
		  goto top;
		}
#endif  hpux

	default:
		DEBUG1("rmtd: garbage command %c\n", c);
		exit(3);
	}
respond:
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
	fprintf(j,"Returning a good response, rval= %d\n", rval);
	fflush(j);
#endif 
	DEBUG1("rmtd: A %d\n", rval);
	(void) sprintf(resp, "A%d\n", rval);
	(void) write(1, resp, strlen(resp));
	goto top;
ioerror:
	error(errno);
	goto top;
}

void
getstring(bp)
	char *bp;
{
	int i;
	char *cp = bp;

	for (i = 0; i < SSIZE; i++) {
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
		fprintf(j,"i=%d cp=%c ssize=%d\n", i, cp[i], SSIZE);
		fflush(j);
#endif 

		if ((read(0, cp+i, 1)) != 1){
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
			fprintf(j,"read rtrnd err %d\n", errno);
			fflush(j);
#endif 
			exit(0);
		}
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
		fprintf(j,"read was ok & %c was read \n",  cp[i]);
		fflush(j);
#endif 
		if (cp[i] == '\n')
			break;
	}
	cp[i] = '\0';
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
	fprintf(j,"getstring completed: %s\n", cp);
	fflush(j);
#endif 
}

char *
checkbuf(record, size, dir)
	char *record;
	int size;
	char dir;
{
	if (size <= maxrecsize)
		return (record);
	if (record != 0)
		free(record);
	record = malloc(size);
	if (record == 0) {
		DEBUG("rmtd: cannot allocate buffer space\n");
		exit(4);
	}
	maxrecsize = size;

	DEBUG1("rmtd: buffer size is %d\n", size);

#ifdef	FASTTCPIP
#ifdef	hpux
	if (size > 32767)
		size = 32767;
#endif	hpux

	if (dir == 'W')
		while (size > 1024 &&
		       setsockopt(0, SOL_SOCKET, SO_RCVBUF, &size, 
				  sizeof (size)) < 0)
			size -= 1024;
	else
		while (size > 1024 &&
		       setsockopt(0, SOL_SOCKET, SO_SNDBUF, &size, 
				  sizeof (size)) < 0)
			size -= 1024;
	DEBUG2("rmtd: %c TCP buffer size is %d\n", dir, size);
#endif	FASTTCPIP

	return (record);
}

void
error(num)
	int num;
{

	DEBUG2("rmtd: E %d (%s)\n", num, sys_errlist[num]);
	(void) sprintf(resp, "E%d\n%s\n", num, sys_errlist[num]);
	(void) write(1, resp, strlen (resp));
}

#if defined(TRUX) && defined(B1)
get_dev_range(dev)
char *dev;
{
	struct dev_asg *d;
	char **alias;
	mand_ir_t *ir;
	static mand_ir_t *max_ir;
	static mand_ir_t *min_ir;
	static int is_single = 0;
	union {
		mand_ir_t	tape_max_ir;
		mand_ir_t	tape_min_ir;
	}tape_ir;
	int size_er;
	static char *max_er;
	static char *min_er;

	  setdvagent(); /* to reset to the first record */
	  while( (d = getdvagent()) != NULL) {	/* get a device record. */
   		if(d->uflg.fg_devs) {
			/* Get pointer to the list of valid device pathnames	
			 * for this device.
			 */
      			alias = d->ufld.fd_devs;
	
			/* Search the list for a match for our device. */
      			while(*alias != NULL) {
         			if(strcmp(dev,*alias) == 0) break;
	 			alias++; 
      			}
       		}
		/* break out of loop if we found a match. */
   		if(*alias != NULL) break;
	  }
	  setdvagent(); /* to reset to the first record */
	  if (d == (struct dev_asg *) 0) 
		   return (0);

	  /*********************************************
	   * Get the device sensitivity label range.
	   ********************************************/

	    /* Is it a multi-level device? */
	    if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_MULTI)) {
		/* Can only assign it one way. */
		if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE))
		   return (0);

		if (d->uflg.fg_max_sl) 
			max_ir = d->ufld.fd_max_sl;
		else if (d->sflg.fg_max_sl) {
			max_ir = d->sfld.fd_max_sl;
		}
		else
		   return (0);

		if (d->uflg.fg_min_sl) 
			min_ir = d->ufld.fd_min_sl;
		else if (d->sflg.fg_min_sl) {
			min_ir = d->sfld.fd_min_sl;
		}
		else  
		   return (0);
	  }
	  /* Is it a single level device? */
	  else if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE)) {
		if (d->uflg.fg_cur_sl) {
			ir = d->ufld.fd_cur_sl;
			is_single = 1;
		}
		/* Is it a system default single level? */
		else if (d->sflg.fg_cur_sl) {
			ir = d->sfld.fd_cur_sl;
			    is_single = 1;
		    }
		    else 
		       return (0);
	      }

	  /* The SSO must specify the device in one of the options above, else
	   * we have an error, so notify the user and exit. 
	   */
	  else  
		   return (0);

	  /* If single-level, only files that match the designated level
	   * exactly can be printed.  Restrict the min/max_ir to the 
	   * designated device level.
	   */
	  if (is_single) {
		   return (0);
	  }

	  /*
	   * Now make sure that the specified sensitivity range is valid.
	   */
	  /* No wildcards allowed. */
	  if(max_ir == (mand_ir_t *)0) 
		   return (0);
	  if(min_ir == (mand_ir_t *)0)  
		   return (0);

	un_ir.max_ir = *max_ir;
	un_ir.min_ir = *min_ir;

}
#endif /* (TRUX) && (B1) */

