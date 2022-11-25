/*
 * @(#)_signal.h: $Revision: 1.2.83.4 $ $Date: 93/12/08 18:23:30 $
 * $Locker:  $
 */

/* 
 * NOTE: This header file contains information specific to the 
 * internals of the HP-UX implementation. The contents of 
 * this header file are subject to change without notice. Such
 * changes may affect source code, object code, or binary
 * compatibility between releases of HP-UX. Code which uses 
 * the symbols contained within this header file is inherently
 * non-portable (even between HP-UX implementations).
*/

#ifndef _UNSUPP_SYS_SIGNAL_INCLUDED
#define _UNSUPP_SYS_SIGNAL_INCLUDED

#ifdef _INCLUDE_HPUX_SOURCE
# ifdef _WSIO
#   ifdef _KERNEL
#     define SIGRESERVE _SIGRESERVE
#   endif /* __KERNEL */
# endif /* _WSIO */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __hp9000s800
# ifdef _KERNEL
#  define _NSIG	32
# endif /* _KERNEL */
#endif /* __hp9000s800 */


#ifdef _INCLUDE_HPUX_SOURCE

# ifdef _KERNEL
#  ifdef __hp9000s300
# 	define NUSERSIG 33
#  endif /* __hp9000s300 */
#  ifdef __hp9000s800
#	define NUSERSIG 31
#  endif /* __hp9000s800 */
# endif /* _KERNEL */

#ifdef __hp9000s800
# ifdef _KERNEL
       struct pa83_siglocal {
		int	sl_syscall;	/* interrupted system call if any */
		int	sl_onstack;	/* sigstack state to restore */
		int	sl_mask;	/* signal mask to restore */
		char	sl_syscall_action;	/* what to do after sys call */
		char	sl_eosys;
		unsigned short	sl_error;
		int	sl_rval1;
		int	sl_rval2;
		int	sl_arg[NUMARGREGS];
		struct pa83_save_state sl_ss;	/* user saved state */
       };

       struct pa83_sigcontext {
		struct pa83_siglocal	sc_sl;	/* local frame containing context */
		int	sc_args[NUMARGREGS];	/* arguments to handler */
		struct frame_marker sc_sfm;
       };
# endif /* _KERNEL */
#endif      /* __hp9000s800 */

# ifdef _KERNEL
#      define SIG_CATCH       ((void (*)(__harg))2)
# endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_SIGNAL_INCLUDED */
