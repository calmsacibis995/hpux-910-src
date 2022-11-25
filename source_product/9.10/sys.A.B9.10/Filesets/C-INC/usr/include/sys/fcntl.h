/* $Header: fcntl.h,v 1.25.83.5 93/10/19 16:10:30 drew Exp $ */

#ifndef _SYS_FCNTL_INCLUDED
#define _SYS_FCNTL_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

/* Needed types */

#ifdef _INCLUDE_HPUX_SOURCE
#ifdef _KERNEL_BUILD
#    include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#endif /* _KERNEL_BUILD */
#endif /* _INCLUDE_HPUX_SOURCE */


#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef _OFF_T
#    define _OFF_T
     typedef long off_t;   /* For file offsets in fcntl() lock structure */
#  endif /* _OFF_T */

#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;   /* For process ID in fcntl() lock structure */
#  endif /* _PID_T */

#  ifndef _MODE_T
#    define _MODE_T
     typedef unsigned short mode_t; 	/* For file modes in open() */
#  endif /* _MODE_T */


   /* Values for cmd used by fcntl() */
#  define F_DUPFD	0	/* Duplicate fildes */
#  define F_GETFD	1	/* Get file descriptor flags */
#  define F_SETFD	2	/* Set file descriptor flags */
#  define F_GETFL	3	/* Get file status flags */
#  define F_SETFL	4	/* Set file status flags */
#  define F_GETLK	5	/* Get file lock */
#  define F_SETLK	6	/* Set file lock */
#  define F_SETLKW	7	/* Set file lock and wait */


   /* File descriptor flags used with fcntl() */
#  define FD_CLOEXEC 1

   /* Values for l_type used for record locking with fcntl() */
#  define F_RDLCK 01
#  define F_WRLCK 02
#  define F_UNLCK 03

   /* Values for l_whence used with fcntl() record locking */
   /* These values are also found in <sys/unistd.h>. */
#  if defined (_POSIX1_SOURCE) || defined(_XPG4)
#    ifndef SEEK_SET
#      define SEEK_SET	0    /* Set file pointer to "offset" */
#      define SEEK_CUR	1    /* Set file pointer to current plus "offset" */
#      define SEEK_END	2    /* Set file pointer to EOF plus "offset" */
#    endif /* SEEK_SET */
#  endif /* _POSIX1_SOURCE || _XPG4 */

   /* File access modes used with open() and fcntl() */
#  define O_RDONLY	0000000   /* Open for reading only */
#  define O_WRONLY	0000001   /* Open for writing only */
#  define O_RDWR	0000002   /* Open for reading or writing */
#  define O_ACCMODE	0000003   /* Mask for file access modes */

   /* Values for oflag used by open() */
   /* Values of these must match corresponding F* values in <sys/file.h>. */
#  define O_CREAT   0000400   /* Open with file create (uses third open arg) */
#  define O_TRUNC   0001000   /* Open with truncation */
#  define O_EXCL    0002000   /* Refuse to create if it already exists */
#  define O_NOCTTY  0400000   /* Don't assign controlling tty */


   /* File status flags used with open() and fcntl() */

   /* Value of O_APPEND must match that of FAPPEND in <sys/file.h>. */
#  define O_APPEND   0000010	/* Append (all writes happen at end of file) */
#  define O_NONBLOCK 0200000	/* Non-blocking open and/or I/O; POSIX-style */

#  ifdef _INCLUDE_XOPEN_SOURCE
#    define O_SYNC   0100000	/* Synchronous writes */
#  endif /* _INCLUDE_XOPEN_SOURCE */

#  ifdef _INCLUDE_HPUX_SOURCE
   /* Value of O_NDELAY must match that of FNDELAY in <sys/file.h>. */
#    define O_NDELAY  0000004 	/* Non-blocking open and/or I/O */
#    define O_SYNCIO  O_SYNC	/* Do write through caching */
#    define FSYNCIO   O_SYNC	/* Do write through caching */
#  endif /* _INCLUDE_HPUX_SOURCE */


   /* File permission bits for open() and creat() */
   /* These values are also found in <sys/stat.h>. */
#  if defined (_POSIX1_SOURCE) || defined(_XPG4)
#    ifndef S_IRWXU		/* stat.h might already have defined these */
#      define S_ISUID 0004000	/* set user ID on execution */
#      define S_ISGID 0002000	/* set group ID on execution */

#      define S_IRWXU 0000700	/* read, write, execute permission (owner) */
#      define S_IRUSR 0000400	/* read permission (owner) */
#      define S_IWUSR 0000200	/* write permission (owner) */
#      define S_IXUSR 0000100	/* execute permission (owner) */

#      define S_IRWXG 0000070	/* read, write, execute permission (group) */
#      define S_IRGRP 0000040	/* read permission (group) */
#      define S_IWGRP 0000020	/* write permission (group) */
#      define S_IXGRP 0000010	/* execute permission (group) */

#      define S_IRWXO 0000007	/* read, write, execute permission (other) */
#      define S_IROTH 0000004	/* read permission (other) */
#      define S_IWOTH 0000002	/* write permission (other) */
#      define S_IXOTH 0000001	/* execute permission (other) */
#    endif /* S_IRWXU */
#  endif /* _POSIX1_SOURCE || _XPG4 */


   /* File record locking structure used by fcntl() */

   struct flock {
      short	l_type;		/* F_RDLCK, F_WRLCK, or F_UNLCK */
      short	l_whence;	/* SEEK_SET, SEEK_CUR, SEEK_END */
      off_t	l_start;	/* starting offset relative to l_whence */
      off_t	l_len;		/* len == 0 means "until end of file" */
#   ifdef _CLASSIC_ID_TYPES
      short	l_filler;
      short	l_pid;
#   else /* not _CLASSIC_ID_TYPES */
      pid_t	l_pid;		/* Process ID of the process holding the
			           lock, returned with F_GETLK */
#   endif /* not _CLASSIC_ID_TYPES */
   };


   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
       extern int creat(const char *, mode_t);
       extern int open(const char *, int, ...);
       extern int fcntl(int, int, ...);
#  else /* not _PROTOTYPES */
       extern int open();
       extern int fcntl();
       extern int creat();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */
#endif /* _INCLUDE_POSIX_SOURCE */


#ifdef _UNSUPPORTED

	/* 
	 * NOTE: The following header file contains information specific
	 * to the internals of the HP-UX implementation. The contents of 
	 * this header file are subject to change without notice. Such
	 * changes may affect source code, object code, or binary
	 * compatibility between releases of HP-UX. Code which uses 
	 * the symbols contained within this header file is inherently
	 * non-portable (even between HP-UX implementations).
	*/
#ifdef _KERNEL_BUILD
#	include "../h/_fcntl.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_fcntl.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_FCNTL_INCLUDED */
