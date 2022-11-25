/* @(#) $Revision: 27.1 $ */      
#include "dir.h"

/* This file defines the major and minor device fields */
/* This version is for the Series 200 */

/* major part of a device */
#define	major(x)	(int)((unsigned)(x)>>8)

/* minor part of a device */
#define	minor(x)	(int)((x)&0xff)

/* make a device number */
#define	makedev(x,y)	(dev_t)(((x)<<8) | (y))

/* the standard print format for a minor device */
#define MINOR_FORMAT	"%3.3d"


/* <sys/filsys.h> */


/* fundamental constants of the filesystem */

#define	BSIZE	1024		/* or 512; size of block (bytes) */
#define	BMASK	(BSIZE-1)	/* BSIZE-1 */

#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	NMASK	(NINDIR-1)	/* NINDIR-1 */

#define	INOPB	(BSIZE/sizeof(struct dinode))	/* inodes per block */
#define	INOMOD	(INOPB-1)		/* value to give offset */

#define	ROOTINO	((ushort)2)	/* i number of all roots */
#define	SUPERB	((daddr_t)1)	/* block number of the super block */

#define	DIRSIZ	14		/* max characters per directory */

#if (BSIZE==1024)
#define	INOSHFT	4 		/* LOG2(INOPB) */
#define	BSHIFT	10		/* LOG2(BSIZE) */
#define	NSHIFT	8		/* LOG2(NINDIR) */
#define	NICINOD	250		/* number of superblock inodes */
#define	NICFREE	100		/* number of superblock free blocks */
#endif

#if (BSIZE==512)
#define	INOSHFT	3 		/* LOG2(INOPB) */
#define	BSHIFT	9		/* LOG2(BSIZE) */
#define	NSHIFT	7		/* LOG2(NINDIR) */
#define	NICINOD	100		/* number of superblock inodes */
#define	NICFREE	50		/* number of superblock free blocks */
#endif

/* Some macros for units conversion */

/* pages to disk blocks */
#define	ctod(x)	(x*(BSIZE/PAGESIZE))

/* inumber to disk address */
#define	itod(x)	(daddr_t)((((unsigned)x+INOPB*2-1)>>INOSHFT))

/* inumber to disk offset */
#define	itoo(x)	(int)(((unsigned)x+INOPB*2-1)&INOMOD)



/* Structure of the super-block */

struct	filsys
{
	long  	s_isize;	/* size in blocks of i-list */
	daddr_t	s_fsize;	/* size in blocks of entire volume */
	long 	s_nfree;	/* number of addresses in s_free */
	daddr_t	s_free[NICFREE];/* free block list */
	long 	s_ninode;	/* number of i-nodes in s_inode */
	ushort	s_inode[NICINOD];/* free i-node list */
	char	s_flock;	/* lock during free list manipulation */
	char	s_ilock;	/* lock during i-list manipulation */
	char  	s_fmod; 	/* super block modified flag */
	char	s_ronly;	/* mounted read-only flag */
	time_t	s_time; 	/* last super block update */
	short	s_dinfo[4];	/* device information */
	daddr_t	s_tfree;	/* total free blocks*/
	ushort	s_tinode;	/* total free inodes */
	char	s_fname[6];	/* file system name */
	char	s_fpack[6];	/* file system pack name */
};

/* "ino.h"	*/

	/* Inode structure as it appears on a disk block. */
struct dinode
{
	ushort di_mode;		/* mode and type of file */
	short	di_nlink;    	/* number of links to file */
	ushort	di_uid;      	/* owner's user id */
	ushort	di_gid;      	/* owner's group id */
	off_t	di_size;     	/* number of bytes in file */
	union {
		char  	di_a[40];	/* disk block addresses */
		dev_t	di_r;
	} di_un;
	time_t	di_atime;   	/* time last accessed */
	time_t	di_mtime;   	/* time last modified */
	time_t	di_ctime;   	/* time created */
};
#define di_addr di_un.di_a
#define di_rdev di_un.di_r
/*
 * the 40 address bytes:
 *	39 used; 13 addresses
 *	of 3 bytes each.
 */




/* our own stuff */
typedef int boolean;
#define true (1)
#define false (0)
