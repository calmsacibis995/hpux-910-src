/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/msdos.c,v $
 *
 * $Revision: 66.1 $
 *
 * msdos.c - functions to make things works under MS-DOS
 *
 * DESCRIPTION
 *
 *	These routines provide the necessary functions to make pax work
 *	under MS-DOS.
 *
 *	NOTE: Before these routines can be used to read/write directly to
 *	the disk, bypassing the logical file structure, MSDOS MUST know
 *	what kind of disk is in the drive you intend to write to.  This can
 *	be accomplished by putting a formatted disk in the drive of
 *	interest and doing a DIR on it.  MSDOS then remembers the disk type
 *	for a while.
 *
 *	WARNING: DISABLING THE BUILT IN CHECK AND CALLING THESE ROUTINES
 *	WITH THE DRIVE SET TO CORRESPOND TO YOUR HARD DISK WILL PROBABLY
 *	TRASH IT COMPLETELY!
 * 
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *	Harold Walters, Oklahoma State University (walters@1.ce.okstate.edu)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	msdos.c,v $
 * Revision 66.1  90/05/11  08:58:11  08:58:11  michas
 * inital checkin
 * 
 * Revision 1.4  89/12/16  10:35:35  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 1.3  89/10/30  07:50:09  mark
 * Added dio_to_binary to support real character devices (from Harold Walters)
 * 
 * Revision 1.2  89/10/13  02:35:19  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char *ident = "$Id: msdos.c,v 1.4 89/12/16 10:35:35 mark Exp Locker: mark $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.";
#endif /* not lint */

/* Headers */

#include "pax.h"
#ifdef MSDOS
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <dos.h>


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

static int	    dio_adw P((unsigned int, char *, unsigned int, 
			       unsigned int, unsigned int *)); 
static int 	    dio_adr P((unsigned int, char *, unsigned int, 
			       unsigned int, unsigned int *));
static int 	    dio_drive_check P((int));
static int	    dio_err_hand P((unsigned int, int, unsigned int));
static int 	    dio_read1 P((int, char *, unsigned int));
static int 	    dio_write1 P((int, char *, unsigned int));

#undef P


/* Defines */

#define SECSIZ 512


/* Local Variables */

static unsigned long fptr = 0L;
static char         secbuf[SECSIZ];
static int          rwsec = 0;

static union REGS   reg;
static union REGS   rreg;

#ifdef M_I86LM
static struct SREGS sreg;
#endif /* !M_I86LM */


void 
dio_str(s)
    char	   *s;
{
    DBUG_ENTER("dio_str");
    for ( ; *s; s++) {
	if (*s == '\\') {
	    *s = '/';
	} else if (isupper(*s)) {
	    *s = tolower(*s);
	}
    }
    DBUG_VOID_RETURN;
}
	

static int 
dio_adw(drive, buf, secnum, secknt, err)
    unsigned int        drive;
    unsigned int        secnum;
    unsigned int	secknt;
    unsigned int       *err;
    char               *buf;
{
    DBUG_ENTER("dio_adw");
    rwsec = secnum;
    reg.x.ax = drive;
    reg.x.dx = secnum;
    reg.x.cx = secknt;
#ifdef M_I86LM
    reg.x.bx = FP_OFF(buf);
    sreg.ds = FP_SEG(buf);
    int86x(0x26, &reg, &rreg, &sreg);
#else /* !M_I86LM */
    reg.x.dx = (int) buf;
    int86(0x26, &reg, &rreg);
#endif /* !M_I86LM */
    *err = rreg.x.ax;
    if (rreg.x.cflag) {
	DBUG_RETURN (-1);
    } else {
	DBUG_RETURN (0);
    }
}


static int 
dio_adr(drive, buf, secnum, secknt, err)
    unsigned int        drive;
    unsigned int        secnum;
    unsigned int        secknt;
    unsigned int       *err;
    unsigned int       *buf;
{
    DBUG_ENTER("dio_adr");
    rwsec = secnum;
    reg.x.ax = drive;
    reg.x.dx = secnum;
    reg.x.cx = secknt;
#ifdef M_I86LM
    reg.x.bx = FP_OFF(buf);
    sreg.ds = FP_SEG(buf);
    int86x(0x25, &reg, &rreg, &sreg);
#else /* !M_I86LM */
    reg.x.dx = (int) buf;
    int86(0x25, &reg, &rreg);
#endif /* !M_I86LM */
    *err = rreg.x.ax;
    if (rreg.x.cflag) {
	DBUG_RETURN (-1);
    } else {
	DBUG_RETURN (0);
    }
	
}


static char        *doserr[] = {
    "write-protect error",
    "unknown unit",
    "drive not ready",
    "unknown command",
    "data error (bad CRC)",
    "bad request structure length",
    "seek error",
    "unknown media type",
    "sector not found",
    "printer out of paper",
    "write fault",
    "read fault",
    "general failure",
    " ",
    " ",
    "invalid disk change (DOS 3.x)",
    ""
};

static char        *bioserr[] = {
    "general error",
    "",
    "bad address mark",
    "write-protect error",
    "sector not found",
    "", "", "",
    "DMA failure",
    "", "", "", "", "",
    "", "",
    "data error (bad CRC)",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "",
    "controller failed",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "",
    "seek error",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "",
    "", "", "",
    "disk time out",
    ""
};


static int 
dio_drive_check(d)
    int                 d;
{
    DBUG_ENTER("dio_drive_check");
    d = -(d + 1);
    switch (d) {
	
    case 0:			/* a */
    case 1:			/* b */
	break;
	
    default:
	fprintf(stderr, "dio: dio to drive %1c: not supported\n",
		'a' + d);
	exit(1);
    }
    DBUG_RETURN (d);
}


static int 
dio_err_hand(drive, rw, err)
    unsigned int        drive;
    int                 rw;
    unsigned int        err;
{
    unsigned int        high,
                        low;

    DBUG_ENTER("dio_err_hand");
    low = err & 0x000f;
    high = (err >> 8) & 0x00ff;
    if (!(high == 0x04 && low == 0x08)) {
	fprintf(stderr,
		"dio: sector %d: %s error 0x%x\ndio: dos: %s: bios: %s\n",
		(rw == (int) 'r') ? rwsec : rwsec,
		(rw == (int) 'r') ? "read" : "write", err,
		doserr[low], bioserr[high]);
    }
    if (high == 0x04 && low == 0x08) {	/* sector not found */
	if (rw == (int) 'r') {
	    rwsec = 0;
	    DBUG_RETURN (0);
	} else {
	    errno = ENOSPC;
	    rwsec = 0;
	    DBUG_RETURN (-1);
	}
    }
    exit(1);
}


static int 
dio_read1(drive, buf, sec)
    int                 drive;
    char               *buf;
    unsigned int        sec;
{
    unsigned int        err;

    if (dio_adr(drive, buf, sec, 1, &err) == -1) {
	if (dio_adr(drive, buf, sec, 1, &err) == -1) {
	    if (dio_adr(drive, buf, sec, 1, &err) == -1) {
		if (dio_err_hand(drive, (int) 'r', err) == 0) {
		    DBUG_RETURN (0);
		}
	    }
	}
    }
    DBUG_RETURN (SECSIZ);
}

static int 
dio_write1(drive, buf, sec)
    int                 drive;
    char               *buf;
    unsigned int        sec;
{
    unsigned int        err;

    DBUG_ENTER("dio_write1");
    if (dio_adw(drive, buf, sec, 1, &err) == -1) {
	if (dio_adw(drive, buf, sec, 1, &err) == -1) {
	    if (dio_adw(drive, buf, sec, 1, &err) == -1) {
		if (dio_err_hand(drive, (int) 'w', err) == -1) {
		    DBUG_RETURN (-1);
		}
	    }
	}
    }
    DBUG_RETURN (SECSIZ);
}


int 
dio_write(drive, from_buf, from_cnt)
    int                 drive;	/* a -> -1, b -> -2, etc */
    char               *from_buf;	/* buffer containing bytes to be
					 * written */
    unsigned int        from_cnt;	/* number of bytes to write */
{
    unsigned int        amt:
    unsigned int        err;
    unsigned int        nn;
    unsigned int        fquo;
    unsigned int        frem;
    unsigned int        cquo;
    unsigned int        crem;

    DBUG_ENTER("dio_write");
    drive = dio_drive_check(drive);
    amt = 0;
    err = 0;
    cquo = 0;
    crem = 0;
    DBUG_PRINT("msdos", ("W drive %d from_cnt %d fptr %ld\n", 
			 drive, from_cnt, fptr)); 
    fquo = (unsigned int) (fptr / SECSIZ);
    frem = (unsigned int) (fptr % SECSIZ);
    if (frem > 0) {
	if (dio_read1(drive, secbuf, fquo) == 0) {
	    DBUG_RETURN (-1);
	}
	if ((nn = SECSIZ - frem) > from_cnt) {
	    nn = from_cnt;
	}
	memcpy(&secbuf[frem], from_buf, nn);
	if (dio_write1(drive, secbuf, fquo) == -1) {
	    DBUG_RETURN (-1);
	}
	amt += nn;
	fptr += nn;
	if (SECSIZ - frem <= from_cnt) {
	    fquo++;
	}
	from_buf += nn;
	from_cnt -= nn;
	DBUG_PRINT("msdos", ("W fr fptr %ld fquo %d frem %d cquo %d crem %d amt %d from_cnt %d\n",
			     fptr, fquo, frem, cquo, crem, amt, from_cnt));
    }
    cquo = from_cnt / SECSIZ;
    crem = from_cnt % SECSIZ;
    if (cquo > 0) {
	if (dio_adw(drive, from_buf, fquo, cquo, &err) == -1) {
	    if (dio_adw(drive, from_buf, fquo, cquo, &err) == -1) {
		if (dio_adw(drive, from_buf, fquo, cquo, &err) == -1) {
		    if (dio_err_hand(drive, (int) 'w', err) == -1) {
			DBUG_RETURN (-1);
		    }
		}
	    }
	}
	nn = cquo * SECSIZ;
	amt += nn;
	fptr += nn;
	fquo += cquo;
	from_buf += nn;
	from_cnt -= nn;
	DBUG_PRINT("msdos", ("W cq fptr %ld fquo %d frem %d cquo %d crem %d amt %d from_cnt %d\n",
			     fptr, fquo, frem, cquo, crem, amt, from_cnt));
    }
    if (crem > 0) {
	if (dio_read1(drive, secbuf, fquo) == 0) {
	    DBUG_RETURN (-1);
	}
	nn = crem;
	memcpy(&secbuf[0], from_buf, nn);
	if (dio_write1(drive, secbuf, fquo) == -1) {
	    DBUG_RETURN (-1);
	}
	amt += nn;
	fptr += nn;
	from_buf += nn;
	from_cnt -= nn;
	DBUG_PRINT("msdos", ("W cr fptr %ld fquo %d frem %d cquo %d crem %d amt %d from_cnt %d\n",
			     fptr, fquo, frem, cquo, crem, amt, from_cnt));
    }
    DBUG_RETURN (amt);
}


/* read data directly from the disk using INT 25 */

int 
dio_read(drive, to_buf, to_cnt)
    int                 drive;	/* a -> -1, b -> -2, etc */
    char               *to_buf;	/* buffer containing bytes to be written */
    unsigned int        to_cnt;	/* number of bytes to write */
{
    unsigned int        amt;
    unsigned int        err;
    unsigned int        nn;
    unsigned int        fquo;
    unsigned int        frem;
    unsigned int        cquo;
    unsigned int        crem;

    DBUG_ENTER("dio_read");
    drive = dio_drive_check(drive);
    amt = 0;
    err = 0;
    cquo = 0;
    crem = 0;
    DBUG_PRINT("msdos", ("R drive %d to_cnt %d fptr %ld\n", drive, to_cnt,
			 fptr));
    fquo = (unsigned int) (fptr / SECSIZ);
    frem = (unsigned int) (fptr % SECSIZ);
    if (frem > 0) {
	if (dio_read1(drive, secbuf, fquo) == 0) {
	    DBUG_RETURN (0);
	}
	if ((nn = SECSIZ - frem) > to_cnt) {
	    nn = to_cnt;
	}
	memcpy(to_buf, &secbuf[frem], nn);
	amt += nn;
	fptr += nn;
	if (SECSIZ - frem <= to_cnt) {
	    fquo++;
	}
	to_buf += nn;

	to_cnt -= nn;
	DBUG_PRINT("msdos", ("R fr fptr %ld fquo %d frem %d cquo %d crem %d amt %d to_cnt %d\n",
			     fptr, fquo, frem, cquo, crem, amt, to_cnt));
    }
    cquo = to_cnt / SECSIZ;
    crem = to_cnt % SECSIZ;
    if (cquo > 0) {
	if (dio_adr(drive, to_buf, fquo, cquo, &err) == -1) {
	    if (dio_adr(drive, to_buf, fquo, cquo, &err) == -1) {
		if (dio_adr(drive, to_buf, fquo, cquo, &err) == -1) {
		    if (dio_err_hand(drive, (int) 'r', err) == 0) {
			DBUG_RETURN (0);
		    }
		}
	    }
	}
	nn = cquo * SECSIZ;
	amt += nn;
	fptr += nn;
	fquo += cquo;
	to_buf += nn;
	to_cnt -= nn;
	DBUG_PRINT("msdos", ("R cq fptr %ld fquo %d frem %d cquo %d crem %d amt %d to_cnt %d\n",
			fptr, fquo, frem, cquo, crem, amt, to_cnt));
    }
    if (crem > 0) {
	if (dio_read1(drive, secbuf, fquo) == 0) {
	    DBUG_RETURN (0);
	}
	nn = crem;
	memcpy(to_buf, &secbuf[0], nn);
	amt += nn;
	fptr += nn;
	to_buf += nn;
	to_cnt -= nn;
	DBUG_PRINT("msdos", ("R cr fptr %ld fquo %d frem %d cquo %d crem %d amt %d to_cnt %d\n",
			fptr, fquo, frem, cquo, crem, amt, to_cnt));
    }
    DBUG_RETURN (amt);
}

/* changes true character devices to binary mode
 * ignores dio and dos files but will change all of
 * stdin, stdout, stderr to binary mode if called with
 * h = to 0, 1, or 2
 * inspired by John B. Thiel (jbthiel@ogc.cse.edu)
 */
 
int
dio_to_binary(h)
   int h;
{
   union REGS           regs;
   DBUG_ENTER("dio_to_binary");
   if (h < 0) {
	   DBUG_RETURN(0);
   }
   regs.h.ah = 0x44;
   regs.h.al = 0x00;
   regs.x.bx = h;
   intdos(&regs, &regs);
   if (regs.x.cflag || regs.x.ax == 0xff) {
	   DBUG_RETURN(-1);
   }
   if (regs.h.dl & 0x80) {
       regs.h.ah = 0x44;
       regs.h.al = 0x01;
       regs.x.bx = h;
       regs.h.dh = 0;
       regs.h.dl |= 0x20;
       intdos(&regs, &regs);
   }
   if (regs.x.cflag || regs.x.ax == 0xff) {
   	   DBUG_RETURN(-1);
   } else {
   	   DBUG_RETURN(0);
   }
}

int 
dio_open_check(s)
    char               *s;
{
    DBUG_ENTER("dio_open_check");
    if (!stricmp(s, "a:dio") || !stricmp(s, "b:dio")) {
	DBUG_RETURN (-(*s - 'a' + 1));
    } else {
	DBUG_RETURN (0);
    }
}


int 
dio_open2(p, f)
    char               *p;
    int                 f;
{
    int                 h;

    DBUG_ENTER("dio_open2");
    h = dio_open_check(p);
    if (h < 0) {
	fptr = 0L;
    }
    DBUG_RETURN (h);
}


int 
dio_open3(p, f, m)
    char               *p;
    int                 f,
                        m;
{
    int                 h;

    DBUG_ENTER("dio_open3");
    h = dio_open_check(p);
    if (h < 0) {
	fptr = 0L;
    }
    DBUG_RETURN (h);
}


int 
dio_close(h)
    int                 h;
{
    return(0);
}


long 
dio_lseek(h, o, r)
    int                 h;
    long                o;
    int                 r;
{
    long                check;

    DBUG_ENTER("dio_lseek");
    if (h >= 0) {
	errno = EBADF;
	DBUG_RETURN (-1L);
    }
    check = fptr;
    switch (r) {
	
    case 0:
	check = o;
	break;
	
    case 1:
	check += o;
	break;
	
    case 2:
    default:
	errno = EINVAL;
	fprintf(stderr, "dio: origin %d not supported\n", r);
	DBUG_RETURN (-1L);
    }
    
    if (check < 0L) {
	errno = EINVAL;
	DBUG_RETURN (-1L);
    }
    fptr = check;
    DBUG_RETURN (fptr);
}


static struct passwd npwd = {"", "", 0, 0, 0, "", "", "", ""};
static char gmem1[] = "";
static char *gmem2[] = {gmem1, (char *) NULL};
static struct group ngrp = {"", "", 0, gmem2};

struct passwd *
getpwuid(x)
    int			x;
{
    return(&npwd);
}


struct passwd *
getpwnam(s)
    char	       *s;
{
    return(&npwd);	
}


struct group *
getgrgid(x)
    int			x;
{
    return(&ngrp);
}


struct group *
getgrnam(s)
    char	       *s;
{
    return(&ngrp);
}


int 
setgrent()
{
    return(0);
}


int 
getuid()
{
    return(0);
}


int 
getgid()
{
    return(0);
}


int 
link(from, to)
    char	       *from;
    char	       *to;
{
    return(-1);
}


int 
chown(name, uid, gid)
    char       	       *name;
    int			uid;
    int			gid;
{
    return(0);
}

#endif /* MSDOS */
