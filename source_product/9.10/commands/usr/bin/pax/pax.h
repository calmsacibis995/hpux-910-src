/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/pax.h,v $
 *
 * $Revision: 66.3 $
 *
 * pax.h - defnitions for entire program
 *
 * DESCRIPTION
 *
 *	This file contains most all of the definitions required by the PAX
 *	software.  This header is included in every source file.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
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
 * $Log:	pax.h,v $
 * Revision 66.3  90/11/27  04:49:58  04:49:58  kawo
 * changes done for cnode handling
 * 
 * Revision 66.2  90/07/10  08:39:00  08:39:00  kawo (#Wolfgang Kapellen)
 * New constant LOCALMAGIC defined.
 * New global variable f_hidden declared.
 * Additional defines in structure Stat.
 * 
 *
 */

#ifndef _PAX_H
#define _PAX_H

/* Headers */

#include "config.h"
#include "limits.h"
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include "regexp.h"
#include "dbug.h"
#ifdef __STDC__
#  include <string.h>
#  include <stdlib.h>
#endif /* __STDC__ */
#ifdef MSDOS
#  include <sys/ioctl.h>
#endif /* MSDOS */

#define DEF_TAR_FILE	"/dev/rmt0"
#define TTY	        "/dev/tty"

#ifdef IOCTL
#  include <sys/ioctl.h>
#endif

#ifdef	TMINSYS
#  include <sys/time.h>
#else
#  include <time.h>
#endif

#ifdef FCNTL
#  include <fcntl.h>
#endif /* FCNTL */

#ifdef XENIX
#  include <sys/inode.h>
#endif /* XENIX */

#ifndef MSDOS
#  include <pwd.h>
#  include <grp.h>
#endif /* MSDOS */

#ifndef XENIX_286
#  include <sys/file.h>
#endif /* XENIX_286 */

/* Defines */

#define	STDIN	0		/* Standard input  file descriptor */
#define	STDOUT	1		/* Standard output file descriptor */

/*
 * Open modes; there is no <fcntl.h> with v7 UNIX and other versions of
 * UNIX may not have all of these defined...
 */

#ifndef O_RDONLY
#  define	O_RDONLY	0
#endif

#ifndef O_WRONLY
#  define	O_WRONLY	1
#endif

#ifndef O_RDWR
#   define	O_WRONLY	2
#endif

#ifndef	O_BINARY
#  define	O_BINARY	0
#endif

#ifndef NULL
#  define 	NULL 		0
#endif

#define TMAGIC		"ustar"	/* ustar and a null */
#define TVERSION	"00"	/* 00 and no null */

#ifdef __hp9000s300
#  define	LOCALMAGIC	0xdddd	/* unique for series 300 */
#endif

#ifdef __hp9000s800
#  define	LOCALMAGIC	0xcccc	/* unique for series 800 */
#endif

/* Values used in typeflag field */
#define REGTYPE		'0'	/* Regular File */
#define AREGTYPE	'\0'	/* Regular File */
#define LNKTYPE		'1'	/* Link */
#define SYMTYPE		'2'	/* Reserved */
#define CHRTYPE		'3'	/* Character Special File */
#define BLKTYPE		'4'	/* Block Special File */
#define DIRTYPE		'5'	/* Directory */
#define FIFOTYPE	'6'	/* FIFO */
#define CONTTYPE	'7'	/* Reserved */

#define BLOCKSIZE	512	/* all output is padded to 512 bytes */
#define	uint	unsigned int	/* Not always in types.h */
#define	ushort	unsigned short	/* Not always in types.h */
#define	BLOCK	5120		/* Default archive block size */
#define	H_COUNT	10		/* Number of items in ASCII header */
#define	H_PRINT	"%06o%06o%06o%06o%06o%06o%06o%011lo%06o%011lo"
#define	H_SCAN	"%6lo%6lo%6ho%6lo%6lo%6ho%6lo%11lo%6o%11lo"
#define	H_STRLEN 70		/* ASCII header string length */
#define	M_ASCII "070707"	/* ASCII magic number */
#define	M_BINARY 070707		/* Binary magic number */
#define	M_STRLEN 6		/* ASCII magic number length */
#define	PATHELEM 256		/* Pathname element count limit */
#define	S_IPERM	07777		/* File permission bits (shb in stat.h) */
#define	S_IPEXE	07000		/* Special execution bits (shb in stat.h) */
#define	S_IPOPN	0777		/* Open access bits (shb in stat.h) */

/*
 * Trailer pathnames. All must be of the same length. 
 */
#define	TRAILER	"TRAILER!!!"	/* Archive trailer (cpio compatible) */
#define	TRAILZ	11		/* Trailer pathname length (including null) */

#include "port.h"


#define	TAR		1
#define	CPIO		2
#define	PAX		3

#define AR_READ 	0
#define AR_WRITE 	1
#define AR_APPEND 	4

/*
 * Header block on tape. 
 */
#define	TUNMLEN		32
#define	TGNMLEN		32

/*
 * Exit codes from the "tar" program 
 */
#define	EX_ARGSBAD	1	/* invalid args */

#define	ROUNDUP(a,b) 	(((a) % (b)) == 0 ? (a) : ((a) + ((b) - ((a) % (b)))))

/*
 * Mininum value. 
 */
#ifdef MIN
#undef MIN
#endif
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))

/*
 * Remove a file or directory. 
 */
#define	REMOVE(name, asb) \
	(((asb)->sb_mode & S_IFMT) == S_IFDIR ? rmdir(name) : unlink(name))

/*
 * Cast and reduce to unsigned short. 
 */
#define	USH(n)		(((ushort) (n)) & 0177777)


/* Type Definitions */

/*
 * Binary archive header (obsolete). 
 */
typedef struct {
    short               b_dev;	/* Device code */
    ushort              b_ino;	/* Inode number */
    ushort              b_mode;	/* Type and permissions */
    ushort              b_uid;	/* Owner */
    ushort              b_gid;	/* Group */
    short               b_nlink;/* Number of links */
    short               b_rdev;	/* Real device */
    ushort              b_mtime[2];	/* Modification time (hi/lo) */
    ushort              b_name;	/* Length of pathname (with null) */
    ushort              b_size[2];	/* Length of data */
} Binary;

/*
 * File status with symbolic links. Kludged to hold symbolic link pathname
 * within structure. 
 */
typedef struct {
    struct stat         sb_stat;
    char                sb_link[PATH_MAX + 1];
} Stat;

#define	STAT(name, asb)		stat(name, &(asb)->sb_stat)
#define	FSTAT(fd, asb)		fstat(fd, &(asb)->sb_stat)

#define	sb_dev		sb_stat.st_dev
#define	sb_ino		sb_stat.st_ino
#define	sb_mode		sb_stat.st_mode
#define	sb_nlink	sb_stat.st_nlink
#define	sb_uid		sb_stat.st_uid
#define	sb_gid		sb_stat.st_gid
#define	sb_rdev		sb_stat.st_rdev
#define	sb_size		sb_stat.st_size
#define	sb_atime	sb_stat.st_atime
#define	sb_mtime	sb_stat.st_mtime
#define	sb_ctime	sb_stat.st_ctime
#define	sb_acl  	sb_stat.st_acl  
#define	sb_rcnode  	sb_stat.st_rcnode  

#ifdef	S_IFLNK
#	define	LSTAT(name, asb)	lstat(name, &(asb)->sb_stat)
#	define	sb_blksize	sb_stat.st_blksize
#	define	sb_blocks	sb_stat.st_blocks
#else				/* S_IFLNK */
/*
 * File status without symbolic links. 
 */
#	define	LSTAT(name, asb)	stat(name, &(asb)->sb_stat)
#endif /* S_IFLNK */

/*
 * Hard link sources. One or more are chained from each link structure. 
 */
typedef struct name {
    struct name        *p_forw;	/* Forward chain (terminated) */
    struct name        *p_back;	/* Backward chain (circular) */
    char               *p_name;	/* Pathname to link from */
} Path;

/*
 * File linking information. One entry exists for each unique file with with
 * outstanding hard links. 
 */
typedef struct link {
    struct link        *l_forw;	/* Forward chain (terminated) */
    struct link        *l_back;	/* Backward chain (terminated) */
    dev_t               l_dev;	/* Device */
    ino_t               l_ino;	/* Inode */
    ushort              l_nlink;/* Unresolved link count */
    OFFSET              l_size;	/* Length */
    char               *l_name;	/* pathname to link from */
    Path               *l_path;	/* Pathname which link to l_name */
} Link;

/*
 * Structure for ed-style replacement strings (-s option).
*/
typedef struct replstr {
    regexp             *comp;	/* compiled regular expression */
    char               *replace;/* replacement string */
    char                print;	/* >0 if we are to print replacement */
    char global;		/* >0 if we are to replace globally */
    struct replstr     *next;	/* pointer to next record */
} Replstr;


/*
 * This has to be included here to insure that all of the type 
 * delcarations are declared for the prototypes.
 */
#include "func.h"


#ifndef NO_EXTERN
/* Globally Available Identifiers */

extern char        *ar_file;
extern char        *bufend;
extern char        *bufstart;
extern char        *bufidx;
extern char        *myname;
extern int          archivefd;
extern int          blocking;
extern uint         blocksize;
extern GIDTYPE      gid;
extern int          head_standard;
extern int          ar_interface;
extern int          ar_format;
extern int          mask;
extern int          ttyf;
extern UIDTYPE      uid;
extern OFFSET       total;
extern short        areof;

/*
 * All of the f_* variables are controlled by command line options
 */
extern short        f_append;
extern short        f_create;
extern short        f_extract;
extern short        f_follow_links;
extern short        f_interactive;
extern short        f_unresolved;
extern short        f_hidden;
extern short        f_list;
extern short        f_modified;
extern short        f_verbose;
extern short        f_link;
extern short        f_owner;
extern short        f_access_time;
extern short        f_pass;
extern short        f_pass;
extern short        f_disposition;
extern short        f_reverse_match;
extern short        f_mtime;
extern short        f_dir_create;
extern short        f_unconditional;
extern short        f_newer;
extern time_t       now;
extern uint         arvolume;
extern int          names_from_stdin;
extern Replstr     *rplhead;
extern Replstr     *rpltail;
extern char       **n_argv;
extern int          n_argc;
extern FILE        *msgfile;
extern short        tar_interface;
extern short        cpio_interface;
extern short        pax_interface;

#endif /* NO_EXTERN */

extern char        *optarg;
extern int          optind;
extern int          sys_nerr;
extern char        *sys_errlist[];
extern int          errno;

#endif /* _PAX_H */
