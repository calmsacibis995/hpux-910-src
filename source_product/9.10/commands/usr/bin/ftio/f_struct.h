/* HPUX_ID: @(#) $Revision: 56.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : ftio_struct.h 
 *	Author ................ : David Williams. 
 *
 *	Hewlett-Packard Australian Software Operation.
 *
 *	(C) 1986 Hewlett Packard Australia.
 *
 *	Description:
 *		Ftio structures of publishable nature. 
 *
 *	Contents:
 *
 *
 *-----------------------------------------------------------------------------
 */

#include <sys/param.h>

struct cpio_f_hdr 
{
	short	h_magic,	/* cpio magic number */
		h_dev;		/* device no that file existed on */
	ushort	h_ino,		/* inode no of file */
		h_mode,		/* access permission mask */
		h_uid,		/* user id of file's owner */
		h_gid;		/* group id of file's owner */
	short	h_nlink,	/* no of links */
		h_rdev,		/* Machine specific LOCALMAGIC no */
		h_mtime[2],	/* last modification time */
		h_namesize,	/* number of characters in file name */
		h_filesize[2];	/* size of file in bytes */
	char	h_name[MAXPATHLEN];	/* file full pathname */
};

#include <sys/utsname.h>	/* needed for uname */

struct ftio_t_hdr
{
/* 
#define	FTIO_MAGIC "FTIO LABEL"
   eg:
   FTIO LABEL.9030X.HP-UX.05.11.C.hpausla.djw.16384.c.12:20.21/08/86.#001.123.

   all fields are null terminated character strings
 */
	char	magic[ 11 ]; /* ftio tape identifier */
	char	machine[UTSLEN];	/* uname -m eg: 9030X */
	char	sysname[UTSLEN];	/* uname -s eg: HP-UX */
	char	release[UTSLEN];	/* uname -r eg: 05.05 */
/*	char	version[UTSLEN];	/* uname -v eg: C */
	char	nodename[UTSLEN];	/* uname -n eg: hpausla */
	char	username[UTSLEN];	/* unix user name of creator: djw */
	char	blocksize[6];		/* 5 digit decimal: 16384 */
	char	headertype[2];		/* 1 char specifying hdr type: c*/
	char	time[20];		/* trad'n hrs:min:sec eg:12:20:31 */
/*	char	date[8];		/* day/month/year eg: 21/08/86 */
	char	tapeuse[5];		/* no times tape used, 0 = not valid */
	char	tapeno[5];		/* # then 3 digit dec eg: #001 */
	char	check[4];		/* 3 digit decimal checksum */
	char	offset[12];		/* offset to first file. */
	char	comment[128];		/* comment field */	
};

