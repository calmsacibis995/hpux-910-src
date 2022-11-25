/* $Header: signal.h,v 1.52.83.6 93/12/08 18:24:18 marshall Exp $ */

#ifndef _SYS_SIGNAL_INCLUDED /* allows multiple inclusion */
#define _SYS_SIGNAL_INCLUDED

/* signal.h: Definitions for signals and their management */

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/* __harg: signal handler arguments type */

#if defined(__STDC__) && !defined(_INCLUDE_HPUX_SOURCE) || defined(__cplusplus)
#  define __harg	int
#else /* not __STDC__ && not _INCLUDE_HPUX_SOURCE || __cplusplus */
#  define __harg	/* nothing */
#endif /* not __STDC__ && not _INCLUDE_HPUX_SOURCE || __cplusplus */


/* Types needed for this file */

#ifdef _INCLUDE__STDC__
  typedef unsigned int sig_atomic_t;  /* Settable type in a signal handler */
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef _PID_T
#    define _PID_T
     typedef long pid_t;
#  endif /* not _PID_T */

   /* Structures used with POSIX signal facilities. */

#  ifdef _INCLUDE_HPUX_SOURCE
     typedef struct __sigset_t {
          long sigset[8];
     } sigset_t;
#  else /* not _INCLUDE_HPUX_SOURCE */
     typedef struct __sigset_t { 
	  long __sigset[8];
     } sigset_t;
#  endif /* _INCLUDE_HPUX_SOURCE */

   struct sigaction {
      void  (*sa_handler)(__harg); /* SIG_DFL, SIG_IGN, or pointer to a func */
      sigset_t   sa_mask;	   /* Additional set of signals to be blocked
				     during execution of signal-catching
				     function. */
      int	sa_flags;	  /* Special flags to affect behavior of sig */
   };
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */

   struct sigstack {		/* Used with sigstack(). */
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
   };

   struct sigvec {		/* Used with sigvec() and sigvector(). */
      void    (*sv_handler)(__harg);	/* signal handler */
      int	sv_mask;		/* signal mask to apply */
      int	sv_flags;		/* see below */
   };

#  define 	sv_onstack  sv_flags /* for compat with no BSDJOBCTL hpux */

#endif /* _INCLUDE_HPUX_SOURCE */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _INCLUDE__STDC__
#  ifdef _PROTOTYPES
     extern void (*signal(int, void (*) (__harg)))(__harg);
     extern int raise(int);
#  else /* not _PROTOTYPES */
     extern void (*signal())();
     extern int raise();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifdef _PROTOTYPES
     extern int kill(pid_t, int);
     extern int sigemptyset(sigset_t *);
     extern int sigfillset(sigset_t *);
     extern int sigaddset(sigset_t *, int);
     extern int sigdelset(sigset_t *, int);
     extern int sigismember(const sigset_t *, int);
     extern int sigaction(int, const struct sigaction *, struct sigaction *);
     extern int sigprocmask(int, const sigset_t *, sigset_t *);
     extern int sigsuspend(const sigset_t *);
     extern int sigpending(sigset_t *);
#  else /* not _PROTOTYPES */
     extern int kill();
     extern int sigemptyset();
     extern int sigfillset();
     extern int sigaddset();
     extern int sigdelset();
     extern int sigismember();
     extern int sigaction();
     extern int sigprocmask();
     extern int sigsuspend();
     extern int sigpending();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _PROTOTYPES
     extern long sigblock(long);
     extern long sigsetmask(long);
     extern int sigstack(const struct sigstack *, struct sigstack *);
     extern int sigvector(int, const struct sigvec *, struct sigvec *);
     extern int (*ssignal(int, int (*) (__harg)))(__harg);
#   ifdef _SVID2
     extern void (*sigset(int, void (*)(__harg)))(__harg);
     extern int sighold(int);
     extern int sigrelse(int);
     extern int sigignore(int);
     extern int sigpause(int);
#   else /* not _SVID2 */
     extern long sigpause(long);
#   endif /* not _SVID2 */
     extern ssize_t sigspace(size_t);
     extern int gsignal(int);
#  else /* not _PROTOTYPES */
     extern long sigblock();
     extern long sigsetmask();
     extern int sigstack();
     extern int sigvector();
#   ifdef _SVID2
     extern void (*sigset())();
     extern int sighold();
     extern int sigrelse();
     extern int sigignore();
     extern int sigpause();
#   else /* not _SVID2 */
     extern long sigpause();
#   endif /* not _SVID2 */
#   ifdef _CLASSIC_TYPES
     extern int sigspace();
#   else /* not _CLASSIC_TYPES */
     extern ssize_t sigspace();
#   endif /* not _CLASSIC_TYPES */
     extern int gsignal();
     extern int (*ssignal())();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Signal numbers */

#  define _SIGHUP	1	/* floating point exception */
#  define SIGINT	2	/* Interrupt */
#  define _SIGQUIT	3	/* quit */
#  define SIGILL	4	/* Illegal instruction (not reset when 
				   caught) */
#  define _SIGTRAP	5	/* trace trap (not reset when caught) */
#  define SIGABRT	6	/* Process abort signal */
#  define _SIGIOT	SIGABRT	/* IOT instruction */
#  define _SIGEMT	7	/* EMT instruction */
#  define SIGFPE	8	/* Floating point exception */
#  define _SIGKILL	9	/* kill (cannot be caught of ignored) */
#  define _SIGBUS	10	/* bus error */
#  define SIGSEGV	11	/* Segmentation violation */
#  define _SIGSYS	12	/* bad argument to system call */
#  define _SIGPIPE	13	/* write on a pipe with no one to read it */
#  define _SIGALRM	14	/* alarm clock */
#  define SIGTERM	15	/* Software termination signal from kill */
#  define _SIGUSR1	16	/* user defined signal 1 */
#  define _SIGUSR2	17	/* user defined signal 2 */
#  define _SIGCHLD	18	/* Child process terminated or stopped */
#  define _SIGCLD	_SIGCHLD	/* death of a child */
#  define _SIGPWR	19	/* power state indication */
#  define _SIGVTALRM	20 	/* virtual timer alarm */
#  define _SIGPROF	21	/* profiling timer alarm */
#  define _SIGIO	22	/* asynchronous I/O */
#  define _SIGPOLL	_SIGIO	/* for HP-UX hpstreams signal */
#  define _SIGWINCH	23 	/* window size change signal */
#  define _SIGWINDOW    _SIGWINCH /* added for compatibility reasons */
#  define _SIGSTOP	24	/* Stop signal (cannot be caught or ignored) */
#  define _SIGTSTP	25	/* Interactive stop signal */
#  define _SIGCONT	26	/* Continue if stopped */
#  define _SIGTTIN	27	/* Read from control terminal attempted by a
			  	   member of a background process group */
#  define _SIGTTOU	28	/* Write to control terminal attempted by a 
				   member of a background process group */
#  define _SIGURG	29	/* urgent condition on IO channel */
#  define _SIGLOST	30      /* remote lock lost  (NFS)        */
#  define _SIGRESERVE	31	/* Save for future use */
#  define _SIGDIL 	32      /* DIL signal */

#ifdef _INCLUDE_POSIX_SOURCE
#  define SIGHUP	_SIGHUP
#  define SIGQUIT	_SIGQUIT
#  define SIGKILL	_SIGKILL
#  define SIGPIPE	_SIGPIPE
#  define SIGALRM	_SIGALRM
#  define SIGUSR1	_SIGUSR1
#  define SIGUSR2	_SIGUSR2
#  define SIGCHLD	_SIGCHLD
#  define SIGSTOP	_SIGSTOP
#  define SIGTSTP	_SIGTSTP
#  define SIGCONT	_SIGCONT
#  define SIGTTIN	_SIGTTIN
#  define SIGTTOU	_SIGTTOU
#endif /* _INCLUDE_POSIX_SOURCE */

#if defined(_XPG2) || defined(_INCLUDE_HPUX_SOURCE)
#  define SIGTRAP	_SIGTRAP
#  define SIGSYS	_SIGSYS
#endif /* _XPG2 || _INCLUDE_HPUX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  define SIGIOT	_SIGIOT
#  define SIGEMT	_SIGEMT
#  define SIGBUS	_SIGBUS
#  define SIGCLD	_SIGCLD
#  define SIGPWR	_SIGPWR
#  define SIGVTALRM	_SIGVTALRM
#  define SIGPROF	_SIGPROF
#  define SIGIO		_SIGIO
#  define SIGWINCH	_SIGWINCH
#  define SIGWINDOW	_SIGWINCH

#    define SIGURG	_SIGURG
#    define SIGLOST	_SIGLOST

#  ifdef _WSIO
#    define SIGDIL	_SIGDIL
#  endif /* _WSIO */

/* hpstreams signal used on 800,700,300 */
#    define SIGPOLL	_SIGPOLL

#endif /* _INCLUDE_HPUX_SOURCE */


/* NSIG is 1 greater than the highest defined signal */

#ifdef __hp9000s300
#  define _NSIG	33   
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#  ifndef _KERNEL
#    define _NSIG	31
#  endif /* not _KERNEL */
#endif /* __hp9000s800 */

#if defined(_XPG2) || defined(_XPG3) || defined(_INCLUDE_HPUX_SOURCE)
#  define NSIG 	_NSIG
#endif /* _XPG2 || _XPG3 || _INCLUDE_HPUX_SOURCE */


/* Value returned from signal() and sigset() to indicate an error */

#define SIG_ERR ((void (*) (__harg))-1)


/* Values used as the "func" argument or s?_handler member in signal(),
   sigaction(), sigvec(), sigvector(), and System V sigset*()
   functions:

      SIG_DFL	Default action upon receipt of signal
      SIG_IGN	Ignore the signal
 */

#  define SIG_DFL ((void (*) (__harg))0)

#  ifdef __lint		/* lint doesn't like non-null pointers to be cast */
#    ifdef __hp9000s300
#      define SIG_IGN ((void (*) (__harg))0)
#    endif /* __hp9000s300 */

#    ifdef __hp9000s800
#      define SIG_IGN ((void (*) (__harg)) (_NSIG+10))
#    endif /* __hp9000s800 */

#  else /* not __lint */

#    define SIG_IGN	((void (*) (__harg))1)
#  endif /* __lint */


#ifdef _INCLUDE_POSIX_SOURCE

/* Values for sigaction() sa_flags */

#  define SA_NOCLDSTOP    0x00000008      /* if set, don't send SIGCLD or 
				 	     SIGCHLD for stopped processes. */
#  ifdef _INCLUDE_HPUX_SOURCE
#    define SA_ONSTACK      0x00000001  /* if set, take on signal stack */
#    define SA_RESETHAND    0x00000004  /* if set, use old signal semantics */
					/* (reset after catching,           */
					/*          don't block in handler) */
#  endif /* _INCLUDE_HPUX_SOURCE */

/* Values for sigprocmask() argument */

#  define SIG_BLOCK	000	/* Resulting set is union of current set and 
				   the signal set pointed to by the argument 
				   set */
#  define SIG_UNBLOCK	001	/* Resulting set is intersection of current set
				   and the signal set pointed to by the argument
				   set */
#  define SIG_SETMASK	002	/* Resulting set is the signal set pointed to 
				   by the argument set */
#endif /* _INCLUDE_POSIX_SOURCE */


/* HP-UX specific stuff */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/syscall.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/syscall.h>
#endif /* _KERNEL_BUILD */

#    define	_sigset_t __sigset_t /* backward compatability */
/* 
   The following define should be >= the KERNEL NSIG ; otherwise, the
   the sizeof(struct user) will be different for non-KERNEL user programs.
   For more information, look at the u_signal[] and u_sigmask[] arrays
   in user.h.
*/

#    define	SIGARRAYSIZE	32

#    ifdef __hp9000s800
#      define	    ILL_RESAD_FAULT  0x0    /* reserved addressing fault */
#      define	    ILL_PRIVIN_FAULT 0x1    /* privileged instruction fault */
#      define	    ILL_RESOP_FAULT  0x2    /* reserved operand fault */
#    endif /* __hp9000s800 */
/* CHME, CHMS, CHMU are not yet given back to users reasonably */
#    ifdef __hp9000s800
#      define	    FPE_INTOVF_TRAP 0x1	/* integer overflow */
#      define	    FPE_INTDIV_TRAP 0x2	/* integer divide by zero */
#      define	    FPE_FLTOVF_TRAP 0x3	/* floating overflow */
#      define	    FPE_FLTDIV_TRAP 0x4	/* floating/decimal divide by zero */
#      define	    FPE_FLTUND_TRAP 0x5	/* floating underflow */
#      define	    FPE_DECOVF_TRAP 0x6	/* decimal overflow */
#      define	    FPE_SUBRNG_TRAP 0x7	/* subscript out of range */
#      define	    FPE_FLTOVF_FAULT 0x8  /* floating overflow fault */
#      define	    FPE_FLTDIV_FAULT 0x9  /* divide by zero floating fault */
#      define	    FPE_FLTUND_FAULT 0xa  /* floating underflow fault */
#    endif /* __hp9000s800 */
#    if (!defined(__hp9000s300) || defined(_KERNEL))
	/*
         * Defines used when a plock()ed process tries to get more
 	 * memory.
 	 */
#      define SRR_EXP_STACK       0x010   /*  Stack expanded */
#      define SRR_EXP_DATA        0x020   /*  Data expanded */
#      define SRR_EXP_SHMEM       0x040   /*  Shared memory expanded */
#      define SRR_LOCK_MEM        0x100   /*  Currently out of lockable memory */
#      define SRR_TLOCK_MEM       0x200   /*  Not enough lockable memory
					 *  in system for this process
					 */
#    endif /* (!__hp9000s300 || _KERNEL) */

#    define	KILL_ALL_OTHERS	((pid_t) 0x7fff)

/* HP-UX values for sigvector() and sigvec() sv_flags */

#    define SV_ONSTACK    SA_ONSTACK
#    define SV_BSDSIG	  0x00000002  /* if set, use BSD signal semantics */
#    define SV_RESETHAND  SA_RESETHAND

#    ifdef	__hp9000s300
/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 */
       struct	sigcontext {
	    int	  sc_syscall;		/* interrupted system call if any */
	    char  sc_syscall_action;	/* what to do after system call */
	    char  sc_pad1;
	    char  sc_pad2;
	    char  sc_onstack;		/* sigstack state to restore */
	    int	  sc_mask;		/* signal mask to restore */
	    int	  sc_sp;		/* sp to restore */
	    short sc_ps;		/* ps to restore */
	    int	  sc_pc;		/* pc to retore */
       };
#    endif	/* __hp9000s300 */

#    ifdef	__hp9000s800

#ifdef _KERNEL_BUILD
#      include "../machine/frame.h"
#      include "../machine/save_state.h"
#else  /* ! _KERNEL_BUILD */
#      include <machine/frame.h>
#      include <machine/save_state.h>
#endif /* _KERNEL_BUILD */

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed, in which the register
 * values for ss_flags, sc_pcoq_head, sc_gr31, sc_sp, sc_dp
 * and the values of sc_onstack and sc_mask may be of concern.
 */

       struct siglocal {
		int	sl_syscall;	/* interrupted system call if any */
		int	sl_onstack;	/* sigstack state to restore */
		int	sl_mask;	/* signal mask to restore */
		char	sl_syscall_action;	/* what to do after sys call */
		char	sl_eosys;
		unsigned short	sl_error;
		int	sl_rval1;
		int	sl_rval2;
		int	sl_arg[NUMARGREGS];
		struct save_state sl_ss;	/* user saved state */
       };

       struct sigcontext {
		struct siglocal	sc_sl;	/* local frame containing context */
		int	sc_args[NUMARGREGS];	/* arguments to handler */
		struct frame_marker sc_sfm;
       };

/* Useful Aliases */
					/* sigcontext structure size */
#      define	SC_SIZE	(sizeof(struct sigcontext))
#      define	sc_syscall sc_sl.sl_syscall /* interrupted system call if any */
#      define	sc_onstack sc_sl.sl_onstack /* previous on stack indicator */
#      define	sc_mask	sc_sl.sl_mask	    /* previous signal mask */
					    /* what to do after system call */
#      define	sc_syscall_action sc_sl.sl_syscall_action
#      define	sc_eosys sc_sl.sl_eosys
#      define	sc_error sc_sl.sl_error
#      define	sc_rval1 sc_sl.sl_rval1
#      define	sc_rval2 sc_sl.sl_rval2
#      define	sc_arg sc_sl.sl_arg
#      define	sc_flags sc_sl.sl_ss.ss_flags	/* save state flags */

/* Registers in the Siglocal of the Sigcontext */
#      define	sc_sp	sc_sl.sl_ss.ss_sp		/* sp to restore */
#      define	sc_ret1	sc_sl.sl_ss.ss_ret1		/* ret1 to restore */
#      define	sc_ret0	sc_sl.sl_ss.ss_ret0		/* ret0 to restore */
#      define	sc_dp	sc_sl.sl_ss.ss_dp		/* dp to restore */
#      define	sc_arg0	sc_sl.sl_ss.ss_arg0		/* arg0 to restore */
#      define	sc_arg1	sc_sl.sl_ss.ss_arg1		/* arg1 to restore */
#      define	sc_arg2	sc_sl.sl_ss.ss_arg2		/* arg2 to restore */
#      define	sc_arg3	sc_sl.sl_ss.ss_arg3		/* arg3 to restore */
#      define	sc_rp	sc_sl.sl_ss.ss_rp		/* rp to restore */
#      define	sc_sar	sc_sl.sl_ss.ss_sar		/* sar to restore */
#      define	sc_ipsw	sc_sl.sl_ss.ss_ipsw
#      define	sc_psw	sc_sl.sl_ss.ss_ipsw		/* psw to restore */
#      define	sc_pcsq_head sc_sl.sl_ss.ss_pcsq_head /* pcsq head to restore */
#      define	sc_pcoq_head sc_sl.sl_ss.ss_pcoq_head /* pcoq head to restore */
#      define	sc_pcsq_tail sc_sl.sl_ss.ss_pcsq_tail /* pcsq tail to restore */
#      define	sc_pcoq_tail sc_sl.sl_ss.ss_pcoq_tail /* pcoq tail to restore */

#      define	sc_gr1	sc_sl.sl_ss.ss_gr1
#      define	sc_gr2	sc_sl.sl_ss.ss_gr2
#      define	sc_gr3	sc_sl.sl_ss.ss_gr3
#      define	sc_gr4	sc_sl.sl_ss.ss_gr4
#      define	sc_gr5	sc_sl.sl_ss.ss_gr5
#      define	sc_gr6	sc_sl.sl_ss.ss_gr6
#      define	sc_gr7	sc_sl.sl_ss.ss_gr7
#      define	sc_gr8	sc_sl.sl_ss.ss_gr8
#      define	sc_gr9	sc_sl.sl_ss.ss_gr9
#      define	sc_gr10	sc_sl.sl_ss.ss_gr10
#      define	sc_gr11	sc_sl.sl_ss.ss_gr11
#      define	sc_gr12	sc_sl.sl_ss.ss_gr12
#      define	sc_gr13	sc_sl.sl_ss.ss_gr13
#      define	sc_gr14	sc_sl.sl_ss.ss_gr14
#      define	sc_gr15	sc_sl.sl_ss.ss_gr15
#      define	sc_gr16	sc_sl.sl_ss.ss_gr16
#      define	sc_gr17	sc_sl.sl_ss.ss_gr17
#      define	sc_gr18	sc_sl.sl_ss.ss_gr18
#      define	sc_gr19	sc_sl.sl_ss.ss_gr19
#      define	sc_gr20	sc_sl.sl_ss.ss_gr20
#      define	sc_gr21	sc_sl.sl_ss.ss_gr21
#      define	sc_gr22	sc_sl.sl_ss.ss_gr22
#      define	sc_gr23	sc_sl.sl_ss.ss_gr23
#      define	sc_gr24	sc_sl.sl_ss.ss_gr24
#      define	sc_gr25	sc_sl.sl_ss.ss_gr25
#      define	sc_gr26	sc_sl.sl_ss.ss_gr26
#      define	sc_gr27	sc_sl.sl_ss.ss_gr27
#      define	sc_gr28	sc_sl.sl_ss.ss_gr28
#      define	sc_gr29	sc_sl.sl_ss.ss_gr29
#      define	sc_gr30	sc_sl.sl_ss.ss_gr30
#      define	sc_gr31	sc_sl.sl_ss.ss_gr31
#      define	sc_sr4	sc_sl.sl_ss.ss_sr4
#    endif	/* __hp9000s800 */

#    define BADSIG          ((void (*)(__harg))-1)


/* SIG_HOLD: For use with V.3 sigset() only */

#    ifdef	__lint
#      ifdef __hp9000s300
#        define SIG_HOLD        ((void (*)(__harg))0)
#      endif /* __hp9000s300 */
#      ifdef __hp9000s800
#        define SIG_HOLD        ((void (*)(__harg)) (_NSIG+11))
#      endif /* __hp9000s800 */
#    else
#      define SIG_HOLD        ((void (*)(__harg))3)
#    endif

/* values for sc_syscall_action in sigcontext structure */
#    ifdef __hp9000s800
#      define SIG_RETURN	1
#      define SIG_RESTART	0
#    endif /* __hp9000s800 */
#    ifdef __hp9000s300
#      define SIG_RETURN	0
#      define SIG_RESTART	1
#    endif /* __hp9000s300 */


#    ifdef __hp9000s300
/* macro to find proper bit in a signal mask */
#      define	sigmask(signo)	(1L << (signo-1))
#    endif /* __hp9000s300 */

#    ifdef __hp9000s800
#      ifndef _KERNEL
/* macro to find proper bit in a signal mask */
#        define	sigmask(signo)	(1L << (signo-1))
#      endif /* _KERNEL */
#    endif /* __hp9000s800 */

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
#ifdef _KERNEL_BUILD
#	include "../h/_signal.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_signal.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_SIGNAL_INCLUDED */
