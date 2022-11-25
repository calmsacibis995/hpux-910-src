/* @(#) $Revision: 66.4 $ */      

/*
 * A directory consists of some number of blocks of DIRBLKSIZ bytes, where
 * DIRBLKSIZ is chosen such that it can be transferred to disk in a single
 * atomic operation (e.g.  DEV_BSIZE on most machines).
 * 
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has a
 * struct direct at the front of it, containing its inode number, the
 * length of the entry, and the length of the name contained in the entry.
 * These are followed by the name padded to a 4 byte boundary with null
 * bytes.  All names are guaranteed null terminated.  The maximum length of
 * a name in a directory is MAXNAMLEN.
 */
/*
 * The macro DIRSIZ(dp) gives the amount of space required to represent a
 * directory entry.  Free space in a directory is represented by entries
 * which have dp->d_reclen > DIRSIZ(dp).  All DIRBLKSIZ bytes in a
 * directory block are claimed by the directory entries.  This usually
 * results in the last entry in a directory having a large dp->d_reclen.
 * When entries are deleted from a directory, the space is returned to the
 * previous entry in the same directory block by increasing its
 * dp->d_reclen.  If the first entry of a directory block is free, then its
 * dp->d_fileno (aka dp->d_ino) is set to 0.  Entries other than the first
 * in a directory do not normally have dp->d_fileno set to 0.
 */

#ifndef	_NDIR_INCLUDED
#define	_NDIR_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

/* so user programs can just include ndir.h */
# include <sys/types.h>
# ifndef DEV_BSIZE
#   define	DEV_BSIZE	1024
# endif /* DEV_BSIZE */

#define DIRBLKSIZ	DEV_BSIZE
#define MAXNAMLEN	255
#define DIRSIZ_CONSTANT 14      /* equivalent to DIRSIZ */

struct	direct {
	long	d_fileno;		/* file number of entry */
	short	d_reclen;		/* length of this record */
	short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};

#ifndef ATT3B2
#define d_ino	d_fileno		/* compatibility */

#undef DIRSIZ
#ifdef DIRSIZ_MACRO
/*
 * The DIRSIZ macro gives the minimum record length which will hold the
 * directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a
 * terminating null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 *
 * NOTE: The DIRSIZ macro is available only if DIRSIZ_MACRO is defined.
 */
#define DIRSIZ(dp) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))
#else /* not DIRSIZ_MACRO */
#define DIRSIZ 14	/* for System V compatible directories */
#endif /* DIRSIZ_MACRO */

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
#define NULL 0
#endif

#if defined(__STDC__) || defined(__cplusplus)
   extern	DIR *opendir(const char *);
   extern	struct direct *readdir(DIR *);
   extern	long telldir(DIR *);
   extern	void seekdir(DIR *, long);
   extern	int closedir(DIR *);
   extern	int getdirentries(int, struct direct *, size_t, off_t *);
#else /* not __STDC__ || __cplusplus */
   extern	DIR *opendir();
   extern	struct direct *readdir();
   extern	long telldir();
   extern	void seekdir();
   extern	int closedir();
   extern	int getdirentries();
#endif /* else not __STDC__ || __cplusplus */

#  define rewinddir(dirp)	seekdir((dirp), (long)0)

#endif /* not ATT3B2 */

#ifdef __cplusplus
}
#endif

#endif	/* _NDIR_INCLUDED */
