/* $Header: dirent.h,v 1.9.83.4 93/09/17 18:25:09 kcs Exp $ */

#ifdef _SYS_DIR_INCLUDED
   #error "Can't include both dir.h and dirent.h"
#  define _SYS_DIRENT_INCLUDED
#endif

#ifndef _SYS_DIRENT_INCLUDED /* allows multiple inclusion */
#define _SYS_DIRENT_INCLUDED

/*
 * dirent.h: Definitions for library routines operating on directory streams.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

/* types and structures */

#ifdef _INCLUDE_POSIX_SOURCE

#ifndef _INO_T
#  define _INO_T
   typedef unsigned long ino_t;	/* Used for file serial numbers */
#endif /* _INO_T */

#  define _MAXNAMLEN 255

   /* directory entry structure */

   struct dirent {
      ino_t d_ino;			/* file number of entry */
      short d_reclen;			/* length of this record */
      short d_namlen;			/* length of string in d_name */
      char  d_name[_MAXNAMLEN + 1];	/* name must be no longer than this */
   };

   /* DIR (directory stream) structure */

   typedef struct __dirdesc {
      int   __dd_fd;		/* file descriptor */
      long  __dd_loc;
      long  __dd_size;
      long  __dd_bbase;
      long  __dd_entno;		/* directory entry number */
      long  __dd_bsize;		/* buffer size */
      char  *__dd_buf;		/* malloc'ed buffer */
   } DIR;
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  define dd_fd	   __dd_fd
#  define dd_loc   __dd_loc
#  define dd_size  __dd_size
#  define dd_bbase __dd_bbase
#  define dd_entno __dd_entno
#  define dd_bsize __dd_bsize
#  define dd_buf   __dd_buf
#endif /* _INCLUDE_HPUX_SOURCE */


/* Function declarations */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _INCLUDE_POSIX_SOURCE
#  undef rewinddir

#  ifdef _PROTOTYPES
     extern DIR *opendir(const char *);
     extern struct dirent *readdir(DIR *);
     extern void rewinddir(DIR *);
     extern int closedir(DIR *);
#  else /* not _PROTOTYPES */
     extern DIR *opendir();
     extern struct dirent *readdir();
     extern void rewinddir();
     extern int closedir();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifdef _PROTOTYPES
     extern long telldir(DIR *);
     extern void seekdir(DIR *, long int);
#  else /* not _PROTOTYPES */
     extern long telldir();
     extern void seekdir();
#  endif /* not _PROTOTYPES */

#  ifndef __lint
#    define rewinddir(__dirp) seekdir((__dirp),(long)0)
#  endif /* not __lint */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _PROTOTYPES
     extern int scandir(const char *,
			  struct dirent ***,
			  int (*)(const struct dirent *),
			  int (*)(const struct dirent **,
				  const struct dirent **));
     extern int alphasort(const struct dirent **, const struct dirent **);
#  else /* not _PROTOTYPES */
     extern int scandir();
     extern int alphasort();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Miscellaneous HP-UX only stuff */

#ifdef _INCLUDE_HPUX_SOURCE
#  define d_fileno	d_ino		/* file number of entry */
   struct _dirdesc {	/* keep this consistent with __dirdesc */
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	long	dd_bbase;
	long	dd_entno;
	long	dd_bsize;
	char	*dd_buf;
   };
/*
 * A directory consists of some number of blocks of DIRBLKSIZ bytes, where
 * DIRBLKSIZ is chosen such that it can be transferred to disk in a single
 * atomic operation (e.g.  DEV_BSIZE on most machines).
 * 
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has a
 * struct dirent at the front of it, containing its inode number, the
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

#  ifndef _KERNEL
#    ifndef DEV_BSIZE
#      define	DEV_BSIZE	1024
#    endif /* DEV_BSIZE */
#  endif /* _KERNEL */

#  define DIRBLKSIZ	DEV_BSIZE
#  define MAXNAMLEN	_MAXNAMLEN	/* backward compatability */
#  define DIRSIZ_CONSTANT 14      /* equivalent to DIRSIZ */

#  ifndef _KERNEL
#    undef DIRSIZ
#    ifdef DIRSIZ_MACRO
/*
 * The DIRSIZ macro gives the minimum record length which will hold the
 * directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a
 * terminating null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 *
 * NOTE: The DIRSIZ macro is available only if DIRSIZ_MACRO is defined.
 */
#      define DIRSIZ(dp) \
     ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))
#    else /* not DIRSIZ_MACRO */
#      define DIRSIZ 14	/* for System V compatible directories */
#    endif /* DIRSIZ_MACRO */

#    ifndef NULL
#      define NULL 0
#    endif
#  endif  /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_DIRENT_INCLUDED */
