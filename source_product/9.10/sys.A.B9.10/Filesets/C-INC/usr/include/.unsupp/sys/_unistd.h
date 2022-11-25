/*
 * @(#)_unistd.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 17:01:24 $
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

#ifndef _UNSUPP_SYS_UNISTD_INCLUDED
#define _UNSUPP_SYS_UNISTD_INCLUDED

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _KERNEL_BUILD
       /* Structure for "utime" function: */
       struct utimbuf {
	      time_t	actime;	 /* Date and time of last access */
	      time_t	modtime; /* Date and time of last modification */
       };
#  endif /* _KERNEL_BUILD */
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_UNISTD_INCLUDED */
