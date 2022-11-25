/*
 * @(#)_syscall.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 17:01:19 $
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

#ifndef _UNSUPP_SYS_SYSCALL_INCLUDED
#define _UNSUPP_SYS_SYSCALL_INCLUDED

#ifdef	_KERNEL
/* The following values are used internally by the kernel.       */
/* They should not overlap with any of the SYS_xxx values above. */
#define	SYS_NOSYS	0
#define	SYS_SIGCLEANUP	139
#endif	/* _KERNEL */


#ifdef __hp9000s300

#ifdef _KERNEL
extern int syscalltrace_rev;
extern struct sct syscalltrace;
#endif /* _KERNEL */

#endif /* __hp9000s300 */

#endif /* _UNSUPP_SYS_SYSCALL_INCLUDED */
