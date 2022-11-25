/* @(#)  $Revision $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)restore.h	5.1 (Berkeley) 5/28/85
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#ifdef	hpux
#  include <sys/ino.h>
#  define  DIRSIZ_MACRO
#if !defined(TRUX) && !defined(B1)
/* prot.h calls mandatory.h which calls dirent.h .
 * (dirent.h can't coexist w/dir.h (which calls ndir.h)
 */
#  include <ndir.h>
#endif /*(TRUX) && (B1) */
#else	hpux
#  include <sys/dir.h>
#endif	hpux

#if defined(TRUX) && defined(B1)
/*#define	B1DEBUG 	1*/
#   include <sys/security.h>
#   include <prot.h>
#   include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/scremdb.h>
#include <sys/m6db.h>

struct m6rh *rem_host;
/* Since one can mount a nonB1 fs on a secure system, dump/restore should
 * be able to handle both and dump/restore to/from both.
 * Any pointers to dinode can stay the same. But anything of the type
 * struct dinode should be a union of struct dinode or struct sec_dinode 
 */
int  it_is_b1fs;     /* assume b1fs unless the sblock says otherwise */
mand_ir_t       *ir; /* global, so we don't have to mand_alloc_ir everytime */

struct direct {  /* Otherwise can't build. A byproduct of excluding ndir.h */ 
      ino_t d_ino;                      /* file number of entry */
      short d_reclen;                   /* length of this record */
      short d_namlen;                   /* length of string in d_name */
      char  d_name[_MAXNAMLEN + 1];     /* name must be no longer than this */
   };
struct sec_attr {
        priv_t  di_gpriv[SEC_SPRIVVEC_SIZE];    /* granted priv vector*/
        priv_t  di_ppriv[SEC_SPRIVVEC_SIZE];    /* potntial priv vector*/
        tag_t   di_tag[SEC_TAG_COUNT];          /* security policy tags*/
        ushort  di_type_flags;      /* type flags, e.g., multilevel dir*/
        ushort  di_checksum;        /* per-inode checksum, always = 0 */
};
/* made global for B1 security use */
struct inotab {
        struct inotab *t_next;
        ino_t   t_ino;
        daddr_t t_seekpt;
	struct sec_attr secinfo; /* for B1 only */
        long t_size;
};
#define HASHSIZE 1000
#define INOHASH(val) (val % HASHSIZE)
static struct inotab *inotab[HASHSIZE];
extern struct inotab *inotablookup();
extern struct inotab *allocinotab();

#endif /*(TRUX) && (B1) */

/*
 * Flags
 */
extern int	cvtflag;	/* convert from old to new tape format */
extern int	bflag;		/* set input block size */
extern int	dflag;		/* print out debugging info */
extern int	hflag;		/* restore heirarchies */
extern int	mflag;		/* restore by name instead of inode number */
extern int	vflag;		/* print out actions taken */
extern int	yflag;		/* always try to recover from tape errors */
/*
 * Global variables
 */
extern char	*dumpmap; 	/* map of inodes on this dump tape */
extern char	*clrimap; 	/* map of inodes to be deleted */
extern ino_t	maxino;		/* highest numbered inode in this file system */
extern long	dumpnum;	/* location of the dump on this tape */
extern long	volno;		/* current volume being read */
extern long	ntrec;		/* number of TP_BSIZE records per tape block */
extern time_t	dumptime;	/* time that this dump begins */
extern time_t	dumpdate;	/* time that this dump was made */
extern char	command;	/* opration being performed */
extern FILE	*terminal;	/* file descriptor for the terminal input */

/*
 * Each file in the file system is described by one of these entries
 */
struct entry {
	char	*e_name;		/* the current name of this entry */
	u_char	e_namlen;		/* length of this name */
	char	e_type;			/* type of this entry, see below */
	short	e_flags;		/* status flags, see below */
	ino_t	e_ino;			/* inode number in previous file sys */
	int	e_CDF;			/* file system informations */
	long	e_index;		/* unique index (for dumpped table) */
	struct	entry *e_parent;	/* pointer to parent directory (..) */
	struct	entry *e_sibling;	/* next element in this directory (.) */
	struct	entry *e_links;		/* hard links to this inode */
	struct	entry *e_entries;	/* for directories, their entries */
	struct	entry *e_next;		/* hash chain list */
#if defined(TRUX) && defined(B1)
	struct  sec_attr secinfo;	/* for tagged file this'll be saved */
#endif /* (TRUX) && (B1) */
};

/* types */
#define	LEAF 1			/* non-directory entry */
#define NODE 2			/* directory entry */
#define LINK 4			/* synthesized type, stripped by addentry */
/* flags */
#define EXTRACT		0x0001	/* entry is to be replaced from the tape */
#define NEW		0x0002	/* a new entry to be extracted */
#define KEEP		0x0004	/* entry is not to change */
#define REMOVED		0x0010	/* entry has been removed */
#define TMPNAME		0x0020	/* entry has been given a temporary name */
#define EXISTED		0x0040	/* directory already existed during extract */
/* e_CDF */
#define C_UNKNOWN	0
#define C_CDF		1
#define C_NOCDF 	2

/*
 * functions defined on entry structs
 */
extern struct entry *lookupino();
extern struct entry *lookupname();
extern struct entry *lookupparent();
extern struct entry *addentry();
extern char *myname();
extern char *savename();
extern char *gentempname();
extern char *flagvalues();
extern ino_t lowerbnd();
extern ino_t upperbnd();
extern DIR *rst_opendir();
extern struct direct *rst_readdir();
#define NIL ((struct entry *)(0))
/*
 * Constants associated with entry structs
 */
#define HARDLINK	1
#define SYMLINK		2
#define TMPHDR		"RSTTMP"

/*
 * The entry describes the next file available on the tape
 */
struct context {
	char	*name;		/* name of file */
	ino_t	ino;		/* inumber of file */
	struct	dinode *dip;	/* pointer to inode */
	int	action;		/* action being taken on this file */
	int	ts;		/* TS_* type of tape record */
} curfile;
/* actions */
#define	USING	1	/* extracting from the tape */
#define	SKIP	2	/* skipping */
#define UNKNOWN 3	/* disposition or starting point is unknown */

/*
 * Other exported routines
 */
extern ino_t psearch();
extern ino_t dirlookup();
extern long listfile();
extern long deletefile();
extern long addfile();
extern long nodeupdates();
extern long verifyfile();
extern char *rindex();
extern char *index();
extern char *strcat();
extern char *strncat();
extern char *strcpy();
extern char *strncpy();
extern char *fgets();
extern char *mktemp();
extern char *malloc();
extern char *calloc();
extern char *realloc();
extern long lseek();

/*
 * Useful macros
 */
#define	MWORD(m,i) (m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)	(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

#define dprintf		if (dflag) fprintf
#define vprintf		if (vflag) fprintf

#define GOOD 1
#define FAIL 0

#ifdef  hpux
#define TRUE  1
#define FALSE 0
#endif  hpux
