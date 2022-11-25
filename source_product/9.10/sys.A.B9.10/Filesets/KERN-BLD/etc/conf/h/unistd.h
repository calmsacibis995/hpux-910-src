/* $Header: unistd.h,v 1.19.83.10 93/11/02 08:58:26 root Exp $ */

#ifndef _SYS_UNISTD_INCLUDED /* allows multiple inclusion */
#define _SYS_UNISTD_INCLUDED

/*
 * unistd.h
 *
 * Symbolic constants, structures, and function declarations
 * not belonging in any other more appropriate header.
 *
 * These support the /usr/group standard, the IEEE POSIX standards,
 * the X/Open Portability Guide, HP-UX, and other standards.
 */

#ifndef _SYS_STDSYMS_INCLUDED
#  ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#  else /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/* Types */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef _GID_T
#    define _GID_T
     typedef long gid_t;	/* For group IDs */
#  endif /* _GID_T */

#  ifndef _UID_T
#    define _UID_T
     typedef long uid_t;	/* For user IDs */
#  endif /* _UID_T */

#  ifndef _OFF_T
#    define _OFF_T
     typedef long off_t;	/* For file sizes */
#  endif /* _OFF_T */

#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;	/* For process IDs and process group IDs */
#  endif /* _PID_T */

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t; /* For memory object sizes and counts */
#  endif /* _SIZE_T */

#  ifndef _SSIZE_T
#     define _SSIZE_T
      typedef int ssize_t;	/* Signed version of size_t */
#  endif /* _SSIZE_T */

#  ifndef NULL
#    define NULL	0
#  endif
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  if defined(__hp9000s300) || defined(__hp9000s800)
#    ifdef _KERNEL_BUILD
#      include "../h/types.h"
       /* Structure for "utime" function moved to the unsupported section */
#    else /* ! _KERNEL_BUILD */
#      include <sys/types.h>
#      include <utime.h>
#    endif /* _KERNEL_BUILD */
#  endif /* 300 or 800 */
#endif /* _INCLUDE_HPUX_SOURCE */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifdef _PROTOTYPES
     extern void _exit(int);
     extern int access(const char *, int);
     extern int chdir(const char *);
     extern int chown(const char *, uid_t, gid_t);
     extern int close(int);
     extern char *ctermid(char *);
     extern char *cuserid(char *);
     extern int dup(int);
     extern int dup2(int, int);
     extern int execl(const char *, const char *, ...);
     extern int execle(const char *, const char *, ...);
     extern int execlp(const char *, const char *, ...);
     extern int execv(const char *, char *const []);
     extern int execve(const char *, char *const [], char *const []);
     extern int execvp(const char *, char *const []);
     extern long fpathconf(int, int);
#   if defined(_XPG3) || defined(_POSIX1_1988)
     extern char *getcwd(char *, int);
#   else /* not (_XPG3 || _POSIX1_1988) */
     extern char *getcwd(char *, size_t);
#   endif /* not (_XPG3 || _POSIX1_1988) */
     extern int getgroups(int, gid_t []);
     extern char *getlogin(void);
     extern int isatty(int);
     extern int link(const char *, const char *);
     extern off_t lseek(int, off_t, int);
     extern long pathconf(const char *, int);
     extern int pause(void);
     extern int pipe(int *);
#   if defined(_XPG3) || defined(_POSIX1_1988)
     extern int read(int, char *, unsigned int);
#   else /* not (_XPG3 || _POSIX1_1988) */
     extern ssize_t read(int, void *, size_t);
#   endif /* not (_XPG3 || _POSIX1_1988) */
     extern int rmdir(const char *);
     extern int setgid(gid_t);
     extern int setpgid(pid_t, pid_t);
     extern pid_t setsid(void);
     extern int setuid(uid_t);
     extern unsigned int sleep(unsigned int);
     extern long sysconf(int);
     extern pid_t tcgetpgrp(int);
     extern int tcsetpgrp(int, pid_t);
     extern char *ttyname(int);
     extern int unlink(const char *);
#   if defined(_XPG3) || defined(_POSIX1_1988)
     extern int write(int, const char *, unsigned int);
#   else /* not (_XPG3 || _POSIX1_1988) */
     extern ssize_t write(int, const void *, size_t);
#   endif /* not (_XPG3 || _POSIX1_1988) */
#  else /* not _PROTOTYPES */
     extern void _exit();
     extern int access();
     extern int chdir();
     extern int chown();
     extern int close();
     extern char *ctermid();
     extern char *cuserid();
     extern int dup();
     extern int dup2();
     extern int execl();
     extern int execle();
     extern int execlp();
     extern int execv();
     extern int execve();
     extern int execvp();
     extern long fpathconf();
     extern char *getcwd();
     extern char *getlogin();
     extern int getgroups();
     extern int isatty();
     extern int link();
     extern off_t lseek();
     extern long pathconf();
     extern int pause();
     extern int pipe();
#   if defined(_XPG3) || defined(_POSIX1_1988)
     extern int read();
#   else /* not (_XPG3 || _POSIX1_1988) */
     extern ssize_t read();
#   endif /* not (_XPG3 || _POSIX1_1988) */
     extern int rmdir();
     extern int setgid();
     extern int setpgid();
     extern pid_t setsid();
     extern int setuid();
     extern unsigned int sleep();
     extern long sysconf();
     extern pid_t tcgetpgrp();
     extern int tcsetpgrp();
     extern char *ttyname();
     extern int unlink();
#   if defined(_XPG3) || defined(_POSIX1_1988)
     extern int write();
#   else /* not (_XPG3 || _POSIX1_1988) */
     extern ssize_t write();
#   endif /* not (_XPG3 || _POSIX1_1988) */
#  endif /* not _PROTOTYPES */

#  ifdef _CLASSIC_POSIX_TYPES
     extern unsigned long alarm();
     extern int fork();
     extern unsigned short getuid();
     extern unsigned short geteuid();
     extern unsigned short getgid();
     extern unsigned short getegid();
     extern int getpid();
     extern int getpgrp();
     extern int getppid();
#  else
#    ifdef _PROTOTYPES
        extern unsigned int alarm(unsigned int);
        extern pid_t fork(void);
	extern gid_t getegid(void);
	extern uid_t geteuid(void);
	extern gid_t getgid(void);
	extern pid_t getpgrp(void);
	extern pid_t getpid(void);
	extern pid_t getppid(void);
	extern uid_t getuid(void);
#    else /* not _PROTOTYPES */
	extern unsigned int alarm();
	extern pid_t fork();
	extern gid_t getegid();
	extern uid_t geteuid();
	extern gid_t getgid();
	extern pid_t getpgrp();
	extern pid_t getpid();
	extern pid_t getppid();
	extern uid_t getuid();
#    endif /* not _PROTOTYPES */
#  endif /* _CLASSIC_POSIX_TYPES */
#endif /* _INCLUDE_POSIX_SOURCE */


#ifdef _INCLUDE_POSIX2_SOURCE
     extern char *optarg;
     extern int opterr;
     extern int optind;
     extern int optopt;
#  ifdef _PROTOTYPES
     /* fnmatch() has moved to <fnmatch.h> */
     extern int getopt(int, char * const [], const char *);/* was <stdio.h> */
     extern size_t confstr(int, char *, size_t);
#  else /* not _PROTOTYPES */
     /* fnmatch() has moved to <fnmatch.h> */
     extern int getopt(); 				   /* was <stdio.h> */
     extern size_t confstr();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_POSIX2_SOURCE */


#ifdef _INCLUDE_XOPEN_SOURCE
#    ifdef _PROTOTYPES
        extern int chroot(const char *);
        extern char *crypt(const char *, const char *);
        extern void encrypt(char [64], int);
        extern int fsync(int);
        extern char *getpass(const char *);
        extern int nice(int);
#      if defined(_XPG3) || defined(_INCLUDE_HPUX_SOURCE)
        extern int rename(const char *, const char *);	/* now in <stdio.h> */
#      endif /* _XPG3 || _INCLUDE_HPUX_SOURCE */
#      if !defined(_INCLUDE_AES_SOURCE) || defined(_INCLUDE_HPUX_SOURCE)
        extern void swab(const void *, void *, ssize_t);
#      endif /* not _INCLUDE_AES_SOURCE || _INCLUDE_HPUX_SOURCE */
#    else /* not _PROTOTYPES */
        extern int chroot();
        extern char *crypt();
        extern void encrypt();
        extern int fsync();
        extern char *getpass();
        extern int nice();
#      if defined(_XPG3) || defined(_INCLUDE_HPUX_SOURCE)
        extern int rename();				/* now in <stdio.h> */
#      endif /* _XPG3 || _INCLUDE_HPUX_SOURCE */
        extern void swab();
#    endif /* not _PROTOTYPES */
#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_AES_SOURCE
     extern char **environ;
#  ifdef _PROTOTYPES
#    ifdef _INCLUDE_HPUX_SOURCE
       extern int readlink(const char *, char *, size_t);
#    else /* ! _INCLUDE_HPUX_SOURCE */
       extern int readlink(const char *, char *, int);
#    endif /* _INCLUDE_HPUX_SOURCE */
     extern int fchown(int, uid_t, gid_t);
     extern int ftruncate(int, off_t);
     extern int truncate(const char *, off_t);
     extern int setgroups(int, gid_t []);
     extern int symlink(const char *, const char *);
#  else
     extern int readlink();
     extern int fchown();
     extern int ftruncate();
     extern int truncate();
     extern int setgroups();
     extern int symlink();
#  endif /* _PROTOTYPES */
#endif /*  _INCLUDE_AES_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _PROTOTYPES
     extern int brk(const void *);
     extern void endusershell(void);
     extern char *getcdf(const char *, char *, size_t);
     extern int getcontext(char *, size_t);
     extern char *gethcwd(char *, size_t);
     extern int gethostname(char *, size_t);
     extern int getpgrp2(pid_t);
     extern char *getusershell(void);
     extern char *hidecdf(const char *, char *, size_t);
     extern int initgroups(const char *, gid_t);
     extern int ioctl(int, int, ...);
#   ifdef __hp9000_s300
     extern int is_68010_present(void);
     extern int is_68881_present(void);
     extern int is_98248A_present(void);
     extern int is_98635A_present(void);
#   endif /* __hp9000_s300 */
     extern int lockf(int, int, off_t);
     extern char *logname(void);
     extern void lsync(void);
     extern int mkstemp(char *);
     extern char *mktemp(char *);
     extern int prealloc(int, off_t);
#   ifdef _CLASSIC_XOPEN_TYPES
     extern char *sbrk(int);
#   else /* not _CLASSIC_XOPEN_TYPES */
     extern void *sbrk(int);
#   endif /* not _CLASSIC_XOPEN_TYPES */
     extern int sethostname(const char *, size_t);
     extern int setpgrp2(pid_t, pid_t);
     extern int setresgid(gid_t, gid_t, gid_t);
     extern int setresuid(uid_t, uid_t, uid_t);
     extern void setusershell(void);
     extern long sgetl(const char *);
     extern void sputl(long, char *);
     extern int swapon(const char *, ...);
     extern void sync(void);
     extern char *ttyname(int);
     extern int ttyslot(void);
#  else /* not _PROTOTYPES */
     extern int brk();
     extern void endusershell();
     extern int fchown();
     extern char *getcdf();
     extern int getcontext();
     extern char *gethcwd();
     extern int gethostname();
     extern int getpgrp2();
     extern char *getusershell();
     extern char *hidecdf();
     extern int initgroups();
     extern int ioctl();
#   ifdef __hp9000_s300
     extern int is_68010_present();
     extern int is_68881_present();
     extern int is_98248A_present();
     extern int is_98635A_present();
#   endif /* __hp9000_s300 */
     extern int lockf();
     extern char *logname();
     extern void lsync();
     extern int mkstemp();
     extern char *mktemp();
     extern int prealloc();
#   ifdef _CLASSIC_XOPEN_TYPES
     extern char *sbrk();
#   else /* not _CLASSIC_XOPEN_TYPES */
     extern void *sbrk();
#   endif /* not _CLASSIC_XOPEN_TYPES */
     extern int sethostname();
     extern int setpgrp2();
     extern int setresgid();
     extern int setresuid();
     extern void setusershell();
     extern long sgetl();
     extern void sputl();
     extern int swapon();
     extern void sync();
     extern char *ttyname();
     extern int ttyslot();
#  endif /* not _PROTOTYPES */

#  ifdef _CLASSIC_ID_TYPES
     extern int setpgrp();
     extern int vfork();
#  else /* not _CLASSIC_ID_TYPES */
#    ifdef _PROTOTYPES
	extern pid_t setpgrp(void);
	extern pid_t vfork(void);
#    else /* not _PROTOTYPES */
	extern pid_t setpgrp();
	extern pid_t vfork();
#    endif /* not _PROTOTYPES */
#  endif /* not _CLASSIC_ID_TYPES */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Symbolic constants */

#if defined(_INCLUDE_POSIX_SOURCE) || defined(_INCLUDE_POSIX2_SOURCE)

/* Symbolic constants for the access() function */
/* These must match the values found in <sys/file.h> */
#  ifndef R_OK
#    define R_OK	4	/* Test for read permission */
#    define W_OK	2	/* Test for write permission */
#    define X_OK	1	/* Test for execute (search) permission */
#    define F_OK	0	/* Test for existence of file */
#  endif /* R_OK */

/* Symbolic constants for the lseek() function */
#  ifndef SEEK_SET
#    define SEEK_SET	0	/* Set file pointer to "offset" */
#    define SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#    define SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#  endif /* SEEK_SET */

/* Versions of POSIX.1 we support */
#  define _POSIX1_VERSION_88	198808L	   /* We support POSIX.1-1988 */
#  define _POSIX1_VERSION_90	199009L	   /* We support POSIX.1-1990 */

#  ifdef _POSIX1_1988
#    define _POSIX_VERSION	_POSIX1_VERSION_88
#  else /* not _POSIX1_1988 */
#    define _POSIX_VERSION	_POSIX1_VERSION_90
#  endif /* not _POSIX1_1988 */

#    define STDIN_FILENO	0
#    define STDOUT_FILENO	1
#    define STDERR_FILENO	2

/* Compile-time symbolic constants */
#  define _POSIX_SAVED_IDS	1	/* If defined, each process has a
					   saved set-user-ID and a saved 
					   set_group-ID */
#  define _POSIX_JOB_CONTROL	2	/* If defined, it indicates that
					   the implementation supports job
					   control */
#if defined(_POSIX_VDISABLE)
#undef _POSIX_VDISABLE
#endif /* defined(_POSIX_VDISABLE) */
#  define _POSIX_VDISABLE       0xff    /* Character which disables local
					   TTY control character functions */
#  define _POSIX2_LOCALEDEF	(-1)	/* If defined, it indicates that the
					   implementation supports creation
					   of locales by the localedef
					   utility. */


/* _POSIX_CHOWN_RESTRICTED and _POSIX_NO_TRUNC are not
 * defined here since they are dynamic.	 Use the pathconf() or fpathconf()
 * functions to query for these values.
 */


/* Symbolic constants for sysconf() variables defined by POSIX.1-1988: 0-7 */

#  define _SC_ARG_MAX	      0	 /* ARG_MAX: Max length of argument to exec()
				    including environment data */
#  define _SC_CHILD_MAX	      1	 /* CHILD_MAX: Max # of processes per userid */
#  define _SC_CLK_TCK	      2	 /* Number of clock ticks per second */
#  define _SC_NGROUPS_MAX     3	 /* NGROUPS_MAX: Max # of simultaneous
				    supplementary group IDs per process */
#  define _SC_OPEN_MAX	      4	 /* OPEN_MAX: Max # of files that one process 
				    can have open at any one time */
#  define _SC_JOB_CONTROL     5	 /* _POSIX_JOB_CONTROL: 1 iff supported */
#  define _SC_SAVED_IDS	      6	 /* _POSIX_SAVED_IDS: 1 iff supported */
#  define _SC_1_VERSION_88    7	 /* _POSIX_VERSION: Date of POSIX.1-1988 */

/* Symbolic constants for sysconf() variables added by POSIX.1-1990: 100-199 */

#  define _SC_STREAM_MAX     100 /* STREAM_MAX: Max # of open stdio FILEs */
#  define _SC_TZNAME_MAX     101 /* TZNAME_MAX: Max length of timezone name */
#  define _SC_1_VERSION_90   102 /* _POSIX_VERSION: Date of POSIX.1-1990 */

/* Pick appropriate value for _SC_VERSION symbolic constant */

#  ifdef _POSIX1_1988
#    define _SC_VERSION	_SC_1_VERSION_88
#  else /* not _POSIX1_1988 */
#    define _SC_VERSION	_SC_1_VERSION_90
#  endif /* not _POSIX1_1988 */


/* Symbolic constants for sysconf() variables added by POSIX.2: 200-299 */

#  define _SC_BC_BASE_MAX	200  /* largest ibase & obase for bc */
#  define _SC_BC_DIM_MAX	201  /* max array elements for bc */
#  define _SC_BC_SCALE_MAX	202  /* max scale value for bc */
#  define _SC_EXPR_NEST_MAX	204  /* max nesting of (...) for expr */
#  define _SC_LINE_MAX		205  /* max length in bytes of input line */
#  define _SC_RE_DUP_MAX	207  /* max regular expressions permitted */
#  define _SC_2_VERSION		211  /* Current version of POSIX.2 */
#  define _SC_2_C_BIND		212  /* C Language Bindings Option */
#  define _SC_2_C_DEV		213  /* C Development Utilities Option */
#  define _SC_2_FORT_DEV	214  /* FORTRAN Dev. Utilities Option */
#  define _SC_2_SW_DEV		215  /* Software Dev. Utilities Option */
#  define _SC_2_C_VERSION	216  /* version of POSIX.2 CLB supported */
#  define _SC_2_CHAR_TERM	217  /* termianls exist where vi works */
#  define _SC_2_FORT_RUN	218  /* FORTRAN Runtime Utilities Option */
#  define _SC_2_LOCALEDEF	219  /* localedef(1M) can create locales */
#  define _SC_2_UPE		220  /* User Portability Utilities Option */
#  define _SC_BC_STRING_MAX	221  /* max scale value for bc */
#  define _SC_COLL_WEIGHTS_MAX  222  /* max collation weights in locale */

    /* The following are obsolete and will be removed in a future release */
#  define _SC_COLL_ELEM_MAX	203  /* max bytes in collation element */
#  define _SC_PASTE_FILES_MAX	206  /* max file operands for paste */
#  define _SC_SED_PATTERN_MAX	208  /* max bytes of pattern space for sed */
#  define _SC_SENDTO_MAX	209  /* max bytes of message for sendto */
#  define _SC_SORT_LINE_MAX	210  /* max bytes of input line for sort */

/* Symbolic constants for sysconf() variables defined by X/Open: 2000-2999 */

#  define _SC_CLOCKS_PER_SEC   2000  /* CLOCKS_PER_SEC: Units/sec of clock() */
#  define _SC_XPG3_VERSION	  8  /* 3 */
#  define _SC_XPG4_VERSION     2001  /* 4 */
#  define _SC_PASS_MAX		  9  /* Max # of bytes in password */
#  define _SC_XOPEN_CRYPT      2002  /* Encryption feature group supported */
#  define _SC_XOPEN_ENH_I18N   2003  /* Enhanced I18N feature group  "	   */
#  define _SC_XOPEN_SHM	       2004  /* Shared memory feature group  "	   */
# ifdef _XPG3
#  define _SC_XOPEN_VERSION  _SC_XPG3_VERSION  /* Issue of XPG supported */
# else /* not _XPG3 */
#  define _SC_XOPEN_VERSION  _SC_XPG4_VERSION  /* Issue of XPG supported */
# endif /* not _XPG3 */

/* Symbolic constants for sysconf() variables defined by OSF: 3000-3999 */

#  define _SC_AES_OS_VERSION   3000 /* AES_OS_VERSION: Version of OSF/AES OS */
#  define _SC_PAGE_SIZE	       3001 /* PAGE_SIZE: Software page size */
#  define _SC_ATEXIT_MAX       3002 /* ATEXIT_MAX: Max # of atexit() funcs */

/* Symbolic constants for sysconf() variables defined by HP-UX: 10000-19999 */

#  define _SC_SECURITY_CLASS  10000 /* SECURITY_CLASS: DoD security level */
#  define _SC_CPU_VERSION     10001 /* CPU type this program is running on */
#  define _SC_IO_TYPE	      10002 /* I/O system type this system supports */
#  define _SC_MSEM_LOCKID     10003 /* msemaphore lock unique identifier */
#  define _SC_MCAS_OFFSET     10004 /* Offset on gateway page of mcas_util() */

/* Symbolic constants for pathconf() defined by POSIX.1: 0-99 */

#  define _PC_LINK_MAX		0  /* LINK_MAX: Max # of links to a single
				      file */
#  define _PC_MAX_CANON		1  /* MAX_CANON: Max # of bytes in a terminal 
				     canonical input line */
#  define _PC_MAX_INPUT		2  /* MAX_INPUT: Max # of bytes allowed in
				     a terminal input queue */ 
#  define _PC_NAME_MAX		3  /* NAME_MAX: Max # of bytes in a filename */

#  define _PC_PATH_MAX		4  /* PATH_MAX: Max # of bytes in a pathname */

#  define _PC_PIPE_BUF		5  /* PIPE_BUF: Max # of bytes for which pipe
				      writes are atomic */ 
#  define _PC_CHOWN_RESTRICTED	6  /* _POSIX_CHOWN_RESTRICTED: 1 iff only a
				      privileged process can use chown() */
#  define _PC_NO_TRUNC		7  /* _POSIX_NO_TRUNC: 1 iff an error is
				      detected when exceeding NAME_MAX */
#  define _PC_VDISABLE		8  /* _POSIX_VDISABLE: character setting which
				      disables TTY local editing characters */

#endif /* _INCLUDE_POSIX_SOURCE || _INCLUDE_POSIX2_SOURCE */


/* Issue(s) of X/Open Portability Guide we support */

#ifdef _INCLUDE_XOPEN_SOURCE

#  ifdef _XPG3
#    define _XOPEN_VERSION	3 
#  else /* not _XPG3 */
#    define _XOPEN_VERSION	4
#  endif /* not _XPG3 */

#  ifdef _XPG2
#    define _XOPEN_XPG2		1
#  else /* not _XPG2 */
#    ifdef _XPG3
#      define _XOPEN_XPG3	1
#    else /* not _XPG3 */
#      define _XOPEN_XPG4	1
#    endif /* not _XPG3 */
#  endif /* not _XPG2 */

#  define _XOPEN_XCU_VERSION	3	/* X/Open Commands & Utilities */

    /* XPG4 Feature Groups */

#  define _XOPEN_CRYPT		1	/* Encryption and Decryption */

    /* _XOPEN_SHM is not defined because the Shared Memory routines can be
    configured in and out of the system.  See uxgen(1M) or config(1M).  */

    /* _XOPEN_ENH_I18N is not defined because the Enhanced Internationalization
       functions are not yet supported, but they will be in the future. */

#endif /* _INCLUDE_XOPEN_SOURCE */


/* Revision of AES OS we support */

#ifdef _INCLUDE_AES_SOURCE
#  if defined(__hp9000s700) || defined(__hp9000s300)
#    define _AES_OS_VERSION	1
#  else /* not (__hp9000s700 || __hp9000s300) */
#    define _AES_OS_VERSION	0
#  endif /* not (__hp9000s700 || __hp9000s300) */
#endif /* _INCLUDE_AES_SOURCE */


#ifdef _INCLUDE_POSIX2_SOURCE

/* Conformance and options for POSIX.2 */

#  define _POSIX2_VERSION   199209L  /* IEEE POSIX.2-1992 base standard */
#  define _POSIX2_C_VERSION 199209L  /* IEEE POSIX.2-1992 C language binding */

#  define _POSIX2_C_BIND     1	     /* c89 finds POSIX.2 funcs by default */
#  define _POSIX2_C_DEV	     1	     /* c89, lex, yacc, etc. are provided */
#  define _POSIX2_FORT_DEV   1	     /* fort77 is provided */
#  define _POSIX2_FORT_RUN   1	     /* asa is provided */
#  define _POSIX2_SW_DEV     1	     /* make, ar, etc. are provided */
#  define _POSIX2_CHAR_TERM  1	     /* terminals exist where vi works */

    /* _POSIX2_UPE is not defined because the User Portability Utilities
       are not yet supported, but they will be in the future. */

/* Symbolic constants for confstr() defined by POSIX.2: 200-299 */

#  define _CS_PATH	200	/* Search path that finds all POSIX.2 utils */

/* Symbolic constants for use with fnmatch() have been moved to <fnmatch.h> */

#endif /*  _INCLUDE_POSIX2_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE

/* Symbolic constants for the "lockf" function: */

#  define F_ULOCK	0	/* Unlock a previously locked region */
#  define F_LOCK	1	/* Lock a region for exclusive use */
#  define F_TLOCK	2	/* Test and lock a region for exclusive use */
#  define F_TEST	3	/* Test a region for a previous lock */


/* Symbolic constants for the passwd file and group file */

#  define GF_PATH	"/etc/group"	/* Path name of the "group" file */
#  define PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */
#  define IN_PATH	"/usr/include"	/* Path name for <...> files */


/* Path on which all POSIX.2 utilities can be found */

#  define CS_PATH	  "/bin/posix:/bin:/usr/bin:"


/* Symbolic constants for values of sysconf(_SC_SECURITY_LEVEL) */

#  define SEC_CLASS_NONE	0  /* default secure system */
#  define SEC_CLASS_C2		1  /* C2 level security */
#  define SEC_CLASS_B1		2  /* B1 level security */

/* Symbolic constants for values of sysconf(_SC_IO_TYPE) */

#  define IO_TYPE_WSIO	  01
#  define IO_TYPE_SIO	  02

/* Symbolic constants for values of sysconf(_SC_CPU_VERSION) */
/* These are the same as the magic numbers defined in <sys/magic.h> */
/* Symbolic constants for values of sysconf(_SC_CPU_VERSION)
   are not monotonic. If any new constants are added, checks should
   be made for equality, and not monotonicity */

#  define CPU_HP_MC68020  0x20C /* Motorola MC68020 */
#  define CPU_HP_MC68030  0x20D /* Motorola MC68030 */
#  define CPU_HP_MC68040  0x20E /* Motorola MC68040 */
#  define CPU_PA_RISC1_0  0x20B /* HP PA-RISC1.0 */
#  define CPU_PA_RISC1_1  0x210 /* HP PA-RISC1.1 */
#  define CPU_PA_RISC1_2  0x211 /* HP PA-RISC1.2 */
#  define CPU_PA_RISC2_0  0x214 /* HP PA-RISC2.0 */
#  define CPU_PA_RISC_MAX 0x2FF	/* Maximum value for HP PA-RISC systems */

/* Macro for detecting whether a given CPU version is an HP PA-RISC machine */

#  define CPU_IS_PA_RISC(__x)           \
       ((__x) == CPU_PA_RISC1_0 ||      \
        (__x) == CPU_PA_RISC1_1 ||      \
        (__x) == CPU_PA_RISC1_2 ||      \
        (__x) == CPU_PA_RISC2_0)

/* Macro for detecting whether a given CPU version is an HP MC680x0 machine */

#  define CPU_IS_HP_MC68K(__x)          \
        ((__x) == CPU_HP_MC68020 ||     \
         (__x) == CPU_HP_MC68030 ||     \
         (__x) == CPU_HP_MC68040)


#endif /* _INCLUDE_HPUX_SOURCE */

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
# ifdef _KERNEL_BUILD
#	include "../h/_unistd.h"
# else /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_unistd.h>
# endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* ! _SYS_UNISTD_INCLUDED */
