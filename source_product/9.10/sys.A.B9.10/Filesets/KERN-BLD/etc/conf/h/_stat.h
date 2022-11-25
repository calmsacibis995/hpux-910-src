/*
 * @(#)_stat.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 17:01:13 $
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

#ifndef _UNSUPP_SYS_STAT_INCLUDED
#define _UNSUPP_SYS_STAT_INCLUDED

#ifdef _INCLUDE_POSIX_SOURCE
#  ifdef _KERNEL
#  define SPARE4_SIZE _SPARE4_SIZE
#  endif /* _KERNEL */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _KERNEL
#  define st_site st_cnode
#  endif /* _KERNEL */
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_STAT_INCLUDED */
