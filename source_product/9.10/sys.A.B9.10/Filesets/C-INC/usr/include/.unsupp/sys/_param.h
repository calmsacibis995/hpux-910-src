/*
 * @(#)_param.h: $Revision: 1.2.83.4 $ $Date: 93/12/09 11:48:27 $
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

#ifndef _UNSUPP_SYS_PARAM_INCLUDED
#define _UNSUPP_SYS_PARAM_INCLUDED


#ifdef _WSIO
#ifdef _KERNEL
#define toggle_led(led) {			\
	extern u_char led_activity_bits;        \
	led_activity_bits |= (led);             \
}
#endif /* _KERNEL */
#endif /* _WSIO */

#ifdef __hp9000s300
#ifdef	_KERNEL
#define	MAYBESIG(p) ((p)->p_sig)
#define	ISSIG(p) (MAYBESIG(p) && issig())
#define	ISSIG_EXIT(p, retval) 					\
	(MAYBESIG(p) ? (					\
		((retval = issig()) < 0) ? (			\
			(ipc.ip_req = 0),			\
			wakeup((caddr_t)&ipc),			\
			exit(-retval)				\
			/*NOTREACHED*/				\
		) : retval					\
	) : 0)
#endif	/* _KERNEL */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#if	defined(_KERNEL)
#define	ISSIG_EXIT(p, retval) 					\
	if ((p)->p_sig && ((p)->p_flag&STRC ||			\
	                ((p)->p_sig &~ (p)->p_sigmask))) {	\
		if ((retval = issig()) < 0) {			\
			ipc.ip_req = 0;				\
			wakeup((caddr_t)&ipc);			\
			exit(-retval);				\
		}						\
	} else retval = 0
#endif
#endif /* __hp9000s800 */

#endif /* _UNSUPP_SYS_PARAM_INCLUDED */
