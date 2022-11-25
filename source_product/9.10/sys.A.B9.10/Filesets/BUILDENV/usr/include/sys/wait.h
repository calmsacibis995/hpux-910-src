/* $Header: wait.h,v 1.19.83.5 93/12/09 10:35:24 marshall Exp $ */

#ifndef _SYS_WAIT_INCLUDED
#define _SYS_WAIT_INCLUDED

/*
 * This file holds definitions relevent to the three system calls
 * wait, waitpid, and wait3.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE

#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;
#  endif /* _PID_T */

    /*
     * Option bits for the waitpid and wait3 calls.  WNOHANG causes the
     * wait to not hang if there are no stopped or terminated processes, rather
     * returning an error indication in this case (pid==0).  WUNTRACED
     * indicates that the caller should receive status about untraced children
     * which stop due to signals.  If children are stopped and a wait without
     * this option is done, it is as though they were still running... nothing
     * about them is returned.
     */
#  define WNOHANG		1	/* don't hang in wait */
#  define WUNTRACED	2	/* tell about stopped, untraced children */

#  define _WAITMASK	(WNOHANG | WUNTRACED)	/* all legal flags */

#  define WIFEXITED(_X)		(((int)(_X)&0377)==0)
#  define WIFSTOPPED(_X)	(((int)(_X)&0377)==0177)
#  define WIFSIGNALED(_X)	((((int)(_X)&0377)!=0)&&(((_X)&0377)!=0177))
#  define WEXITSTATUS(_X)	(((int)(_X)>>8)&0377)
#  define WTERMSIG(_X)		((int)(_X)&0177)
#  define WSTOPSIG(_X)		(((int)(_X)>>8)&0377)

#  ifndef _KERNEL
#  ifdef __cplusplus
    extern "C" {
#  endif /* __cplusplus */

#  ifdef _CLASSIC_POSIX_TYPES
     extern int wait();
     extern int waitpid();
#  else /* not _CLASSIC_POSIX_TYPES */
#    ifdef _PROTOTYPES
       extern pid_t wait(int *);
       extern pid_t waitpid(pid_t, int *, int);
#    else /* not _PROTOTYPES */
       extern pid_t wait();
       extern pid_t waitpid();
#    endif /* not _PROTOTYPES */
#  endif /* not _CLASSIC_POSIX_TYPES */

#  ifdef __cplusplus
    }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

#endif /* _INCLUDE_POSIX_SOURCE */


#if defined (_INCLUDE_HPUX_SOURCE) || defined (_BSD)

#  ifndef _KERNEL
#    ifdef __cplusplus
      extern "C" {
#    endif /* __cplusplus */
#    ifdef _CLASSIC_ID_TYPES
       extern int wait3();
#    else /* not _CLASSIC_ID_TYPES */
#      ifdef _PROTOTYPES
         extern pid_t wait3(int *, int, int *);
#      else /* not _PROTOTYPES */
         extern pid_t wait3();
#      endif /* not _PROTOTYPES */
#    endif /* not _CLASSIC_ID_TYPES */

#    ifdef __cplusplus
      }
#    endif /* __cplusplus */
#  endif /* not _KERNEL */

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */
   union wait	{
	int	w_status;		/* used in syscall */
	/*
	 * Terminated process status.
	 */
	struct {
		unsigned short	w_pad;		/* pad to low order 16 bits */
		unsigned short	w_Retcode:8;	/* exit code if w_termsig==0 */
		unsigned short	w_Coredump:1;	/* core dump indicator */
		unsigned short	w_Termsig:7;	/* termination signal */
	} w_T;
	/*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
	struct {
		unsigned short	w_pad;		/* pad to low order 16 bits */
		unsigned short	w_Stopsig:8;	/* signal that stopped us */
		unsigned short	w_Stopval:8;	/* == W_STOPPED if stopped */
	} w_S;
   };
#  define w_termsig	w_T.w_Termsig
#  define w_coredump	w_T.w_Coredump
#  define w_retcode	w_T.w_Retcode
#  define w_stopval	w_S.w_Stopval
#  define w_stopsig	w_S.w_Stopsig

#  define WSTOPPED	0177	/* value of s.stopval if process is stopped */

#  define WCOREDUMP(_X)	((int)(_X)&0200)
#endif	/* _INCLUDE_HPUX_SOURCE || _BSD */

#ifdef _BSD
#  undef  WIFEXITED
#  define WIFEXITED(_X)		((_X).w_stopval != WSTOPPED && (_X).w_termsig == 0)
#  undef  WIFSTOPPED
#  define WIFSTOPPED(_X)	((_X).w_stopval == WSTOPPED)
#  undef  WIFSIGNALED
#  define WIFSIGNALED(_X)	((_X).w_stopval != WSTOPPED && (_X).w_termsig != 0)
#  undef  WEXITSTATUS
#  define WEXITSTATUS(_X) 	((_X).w_retcode)
#  undef  WTERMSIG
#  define WTERMSIG(_X)    	((_X).w_termsig)
#  undef  WCOREDUMP
#  define WCOREDUMP(_X)   	((_X).w_coredump)
#  undef  WSTOPSIG
#  define WSTOPSIG(_X)    	((_X).w_stopsig)
#endif  /* _BSD */

#endif	/* _SYS_WAIT_INCLUDED */
