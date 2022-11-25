/* @(#) $Revision: 66.2 $ */

/*
 *	Fbackup/frecover common include file
 *
 *	Hewlett-Packard ITG and Australian Software Operation.
 *
 *	(C) 1987 Hewlett Packard.
 *	(C) 1987 Hewlett Packard Australia.
 *
 *	The files <sys/types.h>
 *		  <sys/param.h>
 *		  <sys/stat.h>
 *		  <sys/utsname.h>
 *	must be included before this file is included
 *
 */
#ifndef _FBACKUP_INCLUDED /* allow multiple inclusions */
#define _FBACKUP_INCLUDED

#define EOB "EOB"
#define BOH "BOH"
#define COH "COH"
#define EOH "EOH"
#define BOT "BOT"
#define COT "COT"
#define EOT "EOT"
#define MOR "MOR"
#define FILLCHAR '\0'

#define MAXSTRLEN    1024
#define TYPESIZE      4
#define VOLMAGIC     "FBACKUP LABEL"
#define MAGICSTR     "BLOCK MAGIC"
#define INTSTRFMT    "%11d"
#define INTSTRSIZE   12
#define MAXCOMPONENTS 512     /* max no of components in path name */

#define BLOCKSIZE  1024
#define BLOCKSHIFT 9

#define ASCIIGOOD  "G"
#define ASCIIBAD   "B"

typedef struct {
    char type[TYPESIZE];                /* block type (header or trailer */
    char last[TYPESIZE];                /* last block in header /trailer */
    char magic[INTSTRSIZE];             /* magic number */
    char checksum[INTSTRSIZE];          /* block checksum */
} BLKID;

typedef struct {
    char ppid[INTSTRSIZE];               /* unique backup */
    char time[INTSTRSIZE];              /* identification tag */
} BKUPID;

typedef struct {			/* file header data structure */
    BLKID com;                          /* header/trailer common area */
    BKUPID id;                          /* unique backup identification tag */
    char filenum[INTSTRSIZE];           /* file number sanity check */
} FHDRTYPE;

typedef struct {			/* file trailer data structure */
    BLKID com;                          /* header/trailer common area */
    char filenum[INTSTRSIZE];           /* file number sanity check */
    char status[2];                     /* state of flux for this file */
} FTRLTYPE;

typedef struct {			/* volume label data structure */
    char data[1024];
} LABELTYPE;

typedef union {				/* pointers to various fields within */
    char *ch;				/* a block. used for checksum, etc. */
    int  *integ;
    BLKID *id;
    FHDRTYPE *hdr;
    FTRLTYPE *trl;
} PBLOCKTYPE;

/* It is expected by both frecover and fbackup that VHDRTYPE be aligned
 * to 512 byte block boundaries (hence the peculiar number for pad).
 * If fields are added to VHDRTYPE, adjust pad accordingly. Note that
 * the size can increase, but must be a multiple of 512.
 */
typedef struct {			/* volume header data structure */
	char	magic[14];		/* fbackup media identifier */
	char	machine[UTSLEN];	/* uname -m eg: 9030X */
	char	sysname[UTSLEN];	/* uname -s eg: HP-UX */
	char	release[UTSLEN];	/* uname -r eg: 05.05 */
	char	nodename[UTSLEN];	/* uname -n eg: hpausla */
	char	username[UTSLEN];	/* unix user name of creator: djw */
	char	recsize[8];		/* 5 digit decimal: 16384 */
	char	time[64];		/* ctime */
	char	mediause[5];		/* no times tape used, 0 = not valid */
	char	volno[5];		/* # then 3 digit dec eg: #001 */
	char	check[5];		/* checkpoint frequency */
	char	indexsize[8];		/* size of index in bytes */
	BKUPID	backupid;   	    	/* backup identification */
	char	checksum[12];		/* volume header checksum */
	char	numfiles[12];		/* number of files in backup */
	char	lang[32];		/* language used for this session */
	char	fsmfreq[5];		/* fast search mark frequency */
	char	pad[1809];		/* reserved for future use */
} VHDRTYPE;

typedef struct {			/* checkpoint data structure */
    char filenum[INTSTRSIZE];
    char trecnum[INTSTRSIZE];
    char retrynum[INTSTRSIZE];
    char datasize[INTSTRSIZE];
    char blknum[INTSTRSIZE];
    char blktype;
    char fill_pad;                      /* byte alignment */
} CKPTTYPE;

#endif /* _FBACKUP_INCLUDED */
