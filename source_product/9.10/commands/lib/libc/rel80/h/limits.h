/* @(#) $Revision: 70.1 $ */
#ifndef _LIMITS_INCLUDED
#define _LIMITS_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _INCLUDE__STDC__
#  define CHAR_BIT	8	    /* Number of bits in a char */
#  define CHAR_MAX	127	    /* Max integer value of a char */
#  define CHAR_MIN      (-128)      /* Min integer value of a char */
#  define MB_LEN_MAX	2	    /* Max bytes in a multibyte character */
#  define INT_MAX	2147483647  /* Max decimal value of an int */
#  define INT_MIN	(-2147483647 - 1)  /* Min decimal value of an int */
#  define LONG_MAX	2147483647L 	   /* Max decimal value of a long */
#  define LONG_MIN	(-2147483647L - 1) /* Min decimal value of a long */
#  define SCHAR_MAX     127        /* max value of a signed char */
#  define SCHAR_MIN     (-128)     /* Min value of a signed char */
#  define SHRT_MAX      32767      /* max decimal value of a short */
#  define SHRT_MIN      (-32768)   /* Min decimal value of a short */
#  define UCHAR_MAX     255        /* max value of unsigned char */
#  if defined(__STDC__) || defined(__cplusplus)
#    define UINT_MAX	4294967295U  /* max value of an unsigned integer */
#    define ULONG_MAX	4294967295UL /* max value of a unsigned long int */
#  else /* not __STDC__ || __cplusplus */
#    define UINT_MAX	4294967295  /* max value of an unsigned integer */
#    define ULONG_MAX	4294967295  /* max value of a unsigned long int */
#  endif /* else not __STDC__ || __cplusplus */
#  define USHRT_MAX     65535      /* max value of a unsigned short int */
#endif /* _INCLUDE__STDC__ */

#if defined(_INCLUDE_POSIX_SOURCE) || defined(_INCLUDE_POSIX2_SOURCE)
#  define _POSIX_ARG_MAX  	4096	/* The length of the arguments for 
					   one of the exec functions in bytes, 
					   including environment data. */
#  define _POSIX_CHILD_MAX	6	/* The number of simultaneous
					   processes per real user ID */
#  define _POSIX_LINK_MAX	8	/* The value of a files link count */
#  define _POSIX_MAX_CANON  	255     /* Max number of bytes in a terminal 
					   canonical input line */
#  define _POSIX_MAX_INPUT	255	/* The number of bytes for which space 
					   will be available in aterminal 
					   input queue */
#  define _POSIX_NAME_MAX	14	/* The number of bytes in a filename */
#  define _POSIX_NGROUPS_MAX	0	/* The number of simultaneous 
					   supplementary group IDs per
					   process. */
#  define _POSIX_OPEN_MAX	16	/* The number of files that one process
					   can have open at one time */
#  define _POSIX_PATH_MAX	255 	/* The number of bytes in a pathname */
#  define _POSIX_PIPE_BUF	512	/* The number of bytes that can be 
					   written atomically when writing to 
					   a pipe. */
#  define _POSIX_SSIZE_MAX	32767	/* The value that can be stored in
					   an object of type ssize_t */
#  define _POSIX_TZNAME_MAX	3	/* The maximum number of bytes
					   supported for the name of a time
					   zone (not of the TZ variable). */
#  define _POSIX_STREAM_MAX	8	/* The number of streams that one 
					   process can have open at one
					   time */

/*
 * The following limit is also available using the sysconf() function.
 * Use of sysconf() is advised over use of the constant value defined
 * here, since it should pose fewer portability and forward-compatability
 * problems.
 */
#  define NGROUPS_MAX		20	/* Maximum number of simultaneous
					   supplementary group IDs per 
					   process */

#  if !defined(_POSIX1_1988) && !defined(_XPG3)
#    define SSIZE_MAX		INT_MAX /* The maximum value that can be
					   stored in an object of type 
					   ssize_t */

/*
 * The following limit is also available using the sysconf() function.
 * Use of sysconf() is advised over use of the constant value defined
 * here, since it should pose fewer portability and forward-compatability
 * problems.
 */
#    define TZNAME_MAX		19	/* Maximum number of bytes
					   supported for the name of the 
					   time zone (not of the TZ 
					   variable). */
#  endif /* !defined(_POSIX1_1988) && !defined(_XPG3) */

#endif /* defined(_INCLUDE_POSIX_SOURCE) || defined(_INCLUDE_POSIX2_SOURCE) */

/* the following limits are from POSIX 1003.2 (draft 8) section 2.14 */
#ifdef _INCLUDE_POSIX2_SOURCE

#  define BC_BASE_MAX		99	/* largest ibase and obase for bc */
#  define BC_DIM_MAX		2048	/* max no. of elems in bc array */
#  define BC_SCALE_MAX		99	/* largest scale value for bc */
#  define COLL_ELEM_MAX		4	/* largest no. of bytes to define
					   one collation element */
#  define EXPR_NEST_MAX		32	/* max no. of expressions nested 
					   within parenteses in expr */
#  define LINE_MAX		2048	/* Expected length in bytes of a 
					   utility's input line when input
					   is from text files */
#  define PASTE_FILES_MAX	12	/* largest no. of file operands 
					   allowed by paste */
#  define RE_DUP_MAX		255	/* Max no. of repeated occurrences
					   of an RE when using \{m,n\}
					   notation */
#  define SED_PATTERN_MAX	20480	/* max size in bytes of sed pattern */
#  define SENDTO_MAX		90000	/* Max length in bytes of message
					   accepted by sendto, not
					   including header info. */
#  define SORT_LINE_MAX		20480	/* Max length in bytes of sort
					   input line */
#endif /* _INCLUDE_POSIX2_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifndef DBL_DIG
#    define DBL_DIG   15	/* Digits of precision of a double */  
#  endif /* DBL_DIG */

#  ifndef DBL_MAX
#    define DBL_MAX   1.7976931348623157e+308	/* Max decimal value of a 
						   double */
#  endif /* DBL_MAX */

#  ifndef FLT_DIG
#    define FLT_DIG   6		/* Digits of precision of a float */  
#  endif /* FLT_DIG */

#  ifndef FLT_MAX
#    define FLT_MAX   3.40282347e+38	/* Max decimal value of a float */
#  endif /* FLT_MAX */
#  ifndef DBL_MIN
#    define DBL_MIN   2.2250738585072014e-308	/* Min decimal value of a 
						   double */
#  endif /* DBL_MIN */

#  ifndef FLT_MIN
#    define FLT_MIN   1.17549435e-38	/* Min decimal value of a float */
#  endif /* FLT_MIN */

#  define LONG_BIT    32	/* Number of bits in a long */
#  define WORD_BIT    32	/* number of bits in a "word" (int) */
#  define NL_LANGMAX  (3*_POSIX_NAME_MAX+2)  /* Max number of bytes in a LANG name */
#  define NL_ARGMAX   9     	/* max value of "digit" in calls to the NLS
				   printf() and scanf() functions */

/* message catologue limits */
#  define NL_MSGMAX     65534 /* max message number */
#  define NL_NMAX       2	/* max number of bytes in N-to-1 mapping 
				   characters */
#  define NL_SETMAX     255   /* max set number */
#  define NL_TEXTMAX    8192  /* max number of bytes in a message string */
#  define NZERO		20	/* default process priority */
#  define PASS_MAX      8     /* max number of significant characters in a 
				   password (not including terminating null) */

#  ifndef TMP_MAX
#    define TMP_MAX     17576  /* min number of unique names generated by 
				   tmpnam() */
#  endif /* TMP_MAX */

#  define _SYS_NMLN	9	/* length of strings returned by
				   uname(OS) */

#  ifdef _XPG2
/*
 * The following limts are not actually invariant, but are configurable.
 * The correct values can be determined using the sysconf() function.
 * The default values are provided here because the constants are specified
 * by several publications including XPG (X/Open Portability Guide) Issue 2
 * and SVID (System V Interface Definition) Issue 2.
 */
#    define ARG_MAX		20478	/* Maximum length of arguments for the 
					   exec function in bytes, including 
					   environment data */
#    define CHILD_MAX	   	25	/* Maximum number of simultaneous
					   processes per real user ID */
#    define OPEN_MAX	   	60	/* Maximum number of files that one 
					   process can have open at any given 
					   time */

/*
 * The following limits are not actually invariant, but can vary by file
 * system or device.  the correct values can be determined using the 
 * pathconf() function.  The default values are provided here because the
 * constants are specified by several publications including XPG Issue 2
 * and SVID Issue 2.
 */
#    define LINK_MAX   	32767	/* Max number of links to a single file */
#    define MAX_CANON  	512     /* Max number of bytes in a terminal canonical 
			   	   input line */
#    define MAX_INPUT  	512 	/* Max number of bytes allowed in a terminal 
				   input queue */ 
#    define NAME_MAX   	14	/* Max number of characters in a filename
				   (not including terminating null) */
#    define PATH_MAX   	1023	/* max number of characters in a pathname (not 
				   including terminating null) */
#    define PIPE_BUF   	8192	/* max number bytes that is guaranteed
				   to be atomic when writing to a pipe */ 

/*
 * The following limits are not actually invariant, but are configurable.
 * The values are not generally useful for portable applications due to
 * the system-wide nature of these limits.  The default values are provided
 * here because the constants are specified by several publications
 * including XPG Issue 2 and SVID Issue 2.
 */
#    define LOCK_MAX	32	/* max number of entries in system lock table */
#    define PROC_MAX	84	/* max number of simultaneous processes on 
				   system */
#    define SYS_OPEN	120	/* max number of files open on system */

/* 
 * The following limits are obsolescent.  Some are not generally
 * useful for portable applications.  Some have been replaced by 
 * limits with different names defined by industry standards.  Some
 * have values that can vary.  The values (defaults for variable limits)
 * are provided here because the constants are specified by several 
 * publications including XPG Issue 2 and SVID Issue 2.
 */
#    define FCHR_MAX	INT_MAX		/*max file offset in bytes */
#    define MAX_CHAR	MAX_INPUT	/* max size of a character input 
					   buffer */
#    define SYS_NMLN	_SYS_NMLN	/* length of strings returned by
					   uname(OS) */
#    define SYSPID_MAX	4		/* max pid of system processes */
#    define USI_MAX	UINT_MAX	/* max decimal value of an unsigned 
					   int */
#  endif /* _XPG2 */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

#  include <sys/param.h>

#  define UCHAR_MIN   0       	/* Min value for unsigned char */

#  ifndef CLK_TCK
#    ifdef __hp9000s300
#      define CLK_TCK  50	/* Number of clock ticks per second */
#    endif /* __hp9000s300 */
#    ifdef __hp9000s800
#      define CLK_TCK  100	/* Number of clock ticks per second */
#    endif /* __hp9000s800 */
#  endif /* CLK_TCK */

#  define ATEXIT_MAX	32	/* Max # of functions that can be called by
				   atexit() */
/* message catologue limits */
#  define NL_SAFEFD	3	/* smallest available file no. */
#  define STD_BLK       512	/* number of bytes in a physical I/O block */

/* constants for nl_ascxtime(), et al. */
#  define NL_MAXDATE	200	/* maximum length of a time/date string */

/* constants for nl_init() */
#  define NLSDIR "/usr/lib/nls/"	/* root of language directories */
#  define MAX_INFO_MSGS	80	/* max number of nl_langinfo messages */
#  define MAX_DIGIT_ALT	35	/* max alt digit string(17 char) + NULL */
#  define MAX_PUNCT_ALT	67	/* max alt punct string(33 char) + NULL */
#  define MAX_ERA_FMTS	16	/* max number of era formats */

/* constants for nl_langinfo() and buildlang(1) */
#  define LEN_INFO_MSGS	80	/* max length of any one message */
#  define LEN_ERA_FMTS	40	/* max length of any one era format */

/* constants for the language configuration file: */
#  define NL_WHICHLANGS "/usr/lib/nls/config"	/* where its located */
#  define NL_SPECMAX	96
#  define MAXLNAMELEN 	14	/* max length of a language name */


/* constants for setlocale() and buildlang() */
#define _LC_ALL_SIZE		256	/* max size of LC_ALL data area */
#define _LC_COLLATE_SIZE       1024	/* max size of LC_COLLATE data area */

#ifdef EUC
#define _LC_CTYPE_SIZE	       2168	/* max size of LC_CTYPE data area */
#else /* EUC */
#define _LC_CTYPE_SIZE	       1400	/* max size of LC_CTYPE data area */
#endif /* EUC */

#define _LC_MONETARY_SIZE	128	/* max size of LC_MONETARY data area */
#define _LC_NUMERIC_SIZE	128	/* max size of LC_NUMERIC data area */
#define _LC_TIME_SIZE		912	/* max size of LC_TIME data area */

/*
 * misc. constants
 */
#  define PID_MAX	MAXPID		/* max value for a process id */
#  define PIPE_MAX	INT_MAX		/* max number of bytes writable to a
					   pipe in one write */
#  define UID_MAX	MAXUID		/* smallest unattainable value for a 
					   user or group ID */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _LIMITS_INCLUDED */
