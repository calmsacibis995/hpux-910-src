/*
 * @(#)_ioctl.h: $Revision: 1.2.83.4 $ $Date: 93/12/09 11:48:20 $
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

#ifndef _UNSUPP_SYS_IOCTL_INCLUDED
#define _UNSUPP_SYS_IOCTL_INCLUDED

#ifdef _INCLUDE_HPUX_SOURCE        
#  ifdef __hp9000s800

#ifdef	_KERNEL
#define	FIOCLEX		_IO('f', 1)		/* set exclusive use on fd */
#define	FIONCLEX	_IO('f', 2)		/* remove exclusive use */
#endif /* _KERNEL */

#  endif /* __hp9000s800 */
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_IOCTL_INCLUDED */
