/*
 * @(#)fsdir.h: $Revision: 1.7.83.3 $ $Date: 93/09/17 20:20:21 $
 * $Locker:  $
 */
#ifndef _SYS_FSDIR_INCLUDED /* allows multiple inclusion */
#define _SYS_FSDIR_INCLUDED


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
/* so user programs can just include dir.h */
#if !defined(_KERNEL) && !defined(DEV_BSIZE)
#if defined(__hp9000s300) || defined(__hp9000s800)
#define	DEV_BSIZE	1024
#endif /* __hp9000s300 || __hp9000s800 */
#endif /* !defined(_KERNEL) && !defined(DEV_BSIZE) */
#define DIRBLKSIZ	DEV_BSIZE

#define DIRSIZ_CONSTANT	14	/* valid for System V-style directories */
#define DIR_PADSIZE	10
#define MAXNAMLEN	255
#define PADSIZ		DIR_PADSIZE

#undef DIRSIZ
#ifdef DIRSIZ_MACRO
/*
 * The DIRSIZ macro gives the minimum record length which will hold the
 * directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a
 * terminating null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 *
 * NOTE: The DIRSIZ macro is only available if DIRSIZ_MACRO is defined.
 */
#define DIRSIZ(dp) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))
#else /* not DIRSIZ_MACRO */
#define DIRSIZ 14	/* compatible with System V-style directories */
#endif /* not DIRSIZ_MACRO */

/*
 * WARNING:
 *
 * The following definition of the directory structure is included
 * only for compatibility.  It is valid only for HP-UX file systems
 * that support System V style directories that limit file names to
 * 14 characters in length.  Use <ndir.h> and the directory(3C)
 * routines for portability across all HP-UX implementations and
 * file systems.
 */

struct	direct {
	u_long	d_ino;			/* inode number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
#if defined(_KERNEL)
	union {
		char	d_un_longname[MAXNAMLEN + 1];	/* name must be no longer than this */
		struct {
			char	sn_name[DIRSIZ_CONSTANT];
			char	sn_pad[DIR_PADSIZE];
		} d_un_shortname;
	} d_un;
#define d_name d_un.d_un_longname
#define d_pad d_un.d_un_shortname.sn_pad
#else /* not (defined LONGFILENAMES && defined _KERNEL) */
	char	d_name[DIRSIZ_CONSTANT];/* name must be no longer than this */
	char	d_pad[DIR_PADSIZE];
#endif /* not (defined LONGFILENAMES && defined _KERNEL) */
};
#if defined(_KERNEL)
/*
 * DIRSTRCTSIZ is the size of the structure representing
 * the default 14-character maximum file name length HFS
 * directory format.
 */

#define DIRSTRCTSIZ \
	(sizeof(struct direct) - (MAXNAMLEN+1) + DIRSIZ_CONSTANT + DIR_PADSIZE)
#else /* not (defined LONGFILENAMES && defined _KERNEL) */
/*
 * DIRSTRCTSIZ is the size (in bytes) of the structure representing
 * the System V-compatible 14-character maximum file name length
 * HFS directory format.
 */

#define DIRSTRCTSIZ 32		/* sizeof(struct direct) */
#endif /* not (defined LONGFILENAMES && defined _KERNEL) */

#ifndef _KERNEL
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
extern	DIR *opendir();
extern	struct direct *readdir();
extern	long telldir();
extern	void seekdir();
#define rewinddir(dirp)	seekdir((dirp), (long)0)
extern	int closedir();
#endif /* not _KERNEL */

/*
 * Template for manipulating directories.
 * Should use struct direct's. The name field
 * for non-hpux is MAXNAMLEN - 1, and this just won't do.
 * Fortunately, for hpux systems, the name field is much
 * smaller and it will do.  Sorry for the confusion.
 */

struct dirtemplate {
	u_long	dot_ino;
	short	dot_reclen;
	short	dot_namlen;
	char	dot_name[DIRSIZ_CONSTANT];
	char 	dot_pad[DIR_PADSIZE];
	u_long	dotdot_ino;
	short	dotdot_reclen;
	short	dotdot_namlen;
	char	dotdot_name[DIRSIZ_CONSTANT];
	char	dotdot_pad[DIR_PADSIZE];
};
/*
 * dirtemplate for long file name file system
 */

struct dirtemplate_lfn {
	u_long	dot_ino;
	short	dot_reclen;
	short	dot_namlen;
	char	dot_name[4];		/* must be multiple of 4 */
	u_long	dotdot_ino;
	short	dotdot_reclen;
	short	dotdot_namlen;
	char	dotdot_name[4];		/* ditto */
};
#endif /* _SYS_FSDIR_INCLUDED */
