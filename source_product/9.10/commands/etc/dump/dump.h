/* @(#)  $Revision: 72.4 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dump.h	5.3 (Berkeley) 5/23/86
 */

#define	NI		16
#if defined(TRUX) && defined(B1)
#define B1MAXINOPB	(MAXBSIZE / sizeof(struct sec_dinode))
#endif /* TRUX & B1 */

#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define MAXNINDIR	(MAXBSIZE / sizeof(daddr_t))

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/inode.h>
#ifdef	hpux
#  include <sys/ino.h>
#  include <unistd.h>
#  define  DIRSIZ_MACRO
#if !defined(TRUX) && !defined(B1)
/* prot.h calls mandatory.h which calls dirent.h .
 * (dirent.h can't coexist w/dir.h (which calls ndir.h)
 */
#  include <ndir.h>
#endif /*(TRUX) && (B1) */
#  include "dumprestore.h"
#else	hpux
#  include <sys/dir.h>
#  include <protocols/dumprestore.h>
#endif	hpux
#include <utmp.h>
#include <sys/time.h>
#include <signal.h>
#ifdef	hpux
#  include <mntent.h>
#  include <string.h>
#else	hpux
#  include <fstab.h>
#endif	hpux

#if defined(TRUX) && defined(B1)
#   include <sys/security.h>
#   include <prot.h>
#   include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/scremdb.h>
#include <sys/m6db.h>
/**/
/*#define TDEBUG		1*/	/* debug printfs */
/*#define FDEBUG		1*/	/* debug printfs */
/*#define DEBUG			1*/	/* debug printfs */
/**/

/* Since one can mount a nonB1 fs on a secure system, dump/restore should 
 * be able to handle both and dump/restore to/from both */

int  it_is_b1fs; 		/*assume b1fs unless the sblock says otherwise*/
int  b1debug;
int  remote_tape, b1fd, found_in_chklst;
char *tapedev;

struct m6rh *rem_host;
struct dev_asg *td; 		/* tape device */
struct mand_config *b1buff;

mand_ir_t       *rmttape_max_ir, *rmttape_min_ir;
mand_ir_t 	*fir; /* inode's ir */

union un_ino{
	struct dinode   nonsec_dinode;
	struct sec_dinode sec_dinode;
};
#endif /*(TRUX) && (B1) */

#define	MWORD(m,i)	(m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)		(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

int	msiz;
char	*clrmap;
char	*dirmap;
char	*nodmap;

/*
 *	All calculations done in 0.1" units!
 */

char	*disk;		/* name of the disk file */
char	*tape;		/* name of the tape file */
char	*increm;	/* name of the file containing incremental 
			   information */
char	*temp;		/* name of the file for doing rewrite of increm */
char	lastincno;	/* increment number of previous dump */
char	incno;		/* increment number */
int	uflag;		/* update flag */
int	fi;		/* disk file descriptor */
int	to;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
ino_t	ino;		/* current inumber; used globally */
int	nsubdir;
int	newtape;	/* new tape flag */
int	nadded;		/* number of added sub directories */
int	dadded;		/* directory added flag */
long	tsize;		/* tape size in 0.1" units */
long	esize;		/* estimated tape size, blocks */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */
time_t	tstart_writing;	/* when started writing the first tape block */
struct fs *sblock;	/* the file system super block */
char	buf[MAXBSIZE];

extern int	density;	/* density in 0.1" units */
extern int	notify;		/* notify operator flag */
extern int	blockswritten;	/* number of blocks written on current tape */
extern int	tapeno;		/* current tape number */
extern char	*processname;
#ifdef ACLS
extern int aclcount;
#endif ACLS

#ifdef	hpux
extern char	*index();
extern char	*rindex();
#endif	hpux

extern char	*ctime();
extern char	*prdate();
extern long	atol();
extern int	mark();
extern int	add();
extern int	dirdump();
extern int	dump();
extern int	tapsrec();
extern int	dmpspc();
extern int	dsrch();
extern int	nullf();
extern char	*getsuffix();
extern char	*rawname();
/*
 * in TRUX & B1's case, getino() returns a pointer that actually point 
 * to a sec_dinode but since both dinode and sec_dinode start at the same 
 * place there is no need to alter this 
 */
extern struct dinode *getino();
					
					
					

extern int	interrupt();		/* in case operator bangs on console */

#define	HOUR	(60L*60L)
#define	DAY	(24L*HOUR)
#define	YEAR	(365L*DAY)

/*
 *	Exit status codes
 */
#define	X_FINOK		0	/* normal exit */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort all of dump; don't attempt 
				   checkpointing */

#ifdef V4FS
#define NINCREM "/var/adm/dumpdates"    /*new format incremental info*/
#define TEMP    "/var/adm/dtmp"         /*output temp file*/
#else /* V4FS */
#define NINCREM "/etc/dumpdates"        /*new format incremental info*/
#define TEMP    "/etc/dtmp"             /*output temp file*/
#endif /* V4FS */

#ifdef	hpux
#  define	TAPE	"/dev/rmt/0m"		/* default tape device */
#  define	DISK	"/dev/rdsk/c0d0s0"	/* default disk */
#else	hpux
#  define	TAPE	"/dev/rmt8"		/* default tape device */
#  define	DISK	"/dev/rrp1g"		/* default disk */
#endif	hpux

#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

#ifdef	hpux
extern struct	mntent *fstabsearch(); /* search in mnt_dir and mnt_fsname */
#else	hpux
extern struct	fstab	*fstabsearch();	/* search in fs_file and fs_spec */
#endif	hpux

/*
 *	The contents of the file NINCREM is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
extern struct	idates {
	char	id_name[MAXNAMLEN+3];
	char	id_incno;
	time_t	id_ddate;
};
extern struct	itime{
	struct	idates	it_value;
	struct	itime	*it_next;
};
extern struct	itime	*ithead;	/* head of the list version */
extern int	nidates;		/* number of records (might be zero) */
extern int	idates_in;		/* we have read the increment file */
extern struct	idates	**idatev;	/* the arrayfied version */
#define	ITITERATE(i, ip) for (i = 0,ip = idatev[0]; i < nidates; i++, ip = idatev[i])

/*
 *	We catch these interrupts
 */
extern int	sighup();
extern int	sigquit();
extern int	sigill();
extern int	sigtrap();
extern int	sigfpe();
extern int	sigkill();
extern int	sigbus();
extern int	sigsegv();
extern int	sigsys();
extern int	sigalrm();
extern int	sigterm();
