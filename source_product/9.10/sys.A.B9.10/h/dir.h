/* $Header: dir.h,v 1.22.83.4 93/09/17 18:25:03 kcs Exp $ */

#ifdef _SYS_DIRENT_INCLUDED
   #error "Can't include both dir.h and dirent.h"
#  define _SYS_DIR_INCLUDED
#endif

#ifndef _SYS_DIR_INCLUDED
#define _SYS_DIR_INCLUDED

/* dir.h: BSD-oriented directory stream stuff; see also dirent.h */

#ifdef _KERNEL_BUILD
#include "../h/types.h"		/* types.h includes stdsyms.h */
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>		/* types.h includes stdsyms.h */
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/*
 * A directory consists of some number of blocks of DIRBLKSIZ
 * bytes, where DIRBLKSIZ is chosen such that it can be transferred
 * to disk in a single atomic operation (e.g. 512 bytes on most machines).
 *
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 *
 * The macro DIRSIZ(dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(dp).  All DIRBLKSIZ bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 */

#if !defined(_KERNEL) && !defined(DEV_BSIZE)
#define	DEV_BSIZE	1024
#endif /* !defined(_KERNEL) && !defined(DEV_BSIZE) */

#define DIRBLKSIZ	DEV_BSIZE

#ifndef MAXNAMLEN		/* watch out for <sys/dirent.h> */
#  define MAXNAMLEN	255
#endif /* not MAXNAMLEN */

struct	direct {
	u_long	d_fileno;		/* file number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};

#ifndef _KERNEL
#define d_ino	d_fileno		/* compatiblity */

/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))

/*
 * Definitions for library routines operating on directories.
 */
typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	long	dd_bbase;
	long	dd_entno;
	long	dd_bsize;
	char	*dd_buf;
} DIR;

#ifndef NULL
#  define NULL 0
#endif

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#undef rewinddir

#ifdef _PROTOTYPES
   extern DIR *opendir(char *);
   extern struct direct *readdir(DIR *);
   extern long telldir(DIR *);
   extern void seekdir(DIR *, long);
   extern void rewinddir(DIR *);
   extern int closedir(DIR *);
   extern int scandir(const char *,
		      struct dirent ***,
		      int (*)(const struct dirent *),
		      int (*)(const struct dirent **,
			      const struct dirent **));
   extern int alphasort(const struct dirent **, const struct dirent **);
#else /* not _PROTOTYPES */
   extern DIR *opendir();
   extern struct direct *readdir();
   extern long telldir();
   extern void seekdir();
   extern void rewinddir();
   extern int closedir();
   extern int scandir();
   extern int alphasort();
#endif /* not _PROTOTYPES */

#ifndef __lint
#  define rewinddir(dirp)	seekdir((dirp), (long)0)
#endif /* not __lint */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_DIR_INCLUDED */
