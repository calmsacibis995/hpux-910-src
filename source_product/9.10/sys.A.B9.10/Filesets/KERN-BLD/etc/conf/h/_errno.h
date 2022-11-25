/*
 * @(#)_errno.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 17:00:34 $
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

#ifndef _UNSUPP_SYS_ERRNO_INCLUDED
#define _UNSUPP_SYS_ERRNO_INCLUDED

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef	_KERNEL
#  define EPATHREMOTE		133	/* Pathname is remote */
#  define EOPCOMPLETE		134	/* Operation completed at server */

#ifdef	_LVM
	/*
	 * These defines are for OSF's LVM internal use.
	 */
#define ESUCCESS		0
#define	ESOFT			123
#define	EMEDIA			124
#define	ERELOCATED		125
#endif	/* _LVM */
#endif	/* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_ERRNO_INCLUDED */
