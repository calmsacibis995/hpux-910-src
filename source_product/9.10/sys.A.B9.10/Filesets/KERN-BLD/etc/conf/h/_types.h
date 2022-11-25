/*
 * @(#)_types.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 17:01:08 $
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

#ifndef _UNSUPP_SYS_TYPES_INCLUDED
#define _UNSUPP_SYS_TYPES_INCLUDED

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL
   typedef struct mbuf *dm_message;
#endif /* _KERNEL */


#ifndef MAXFUPLIM
#  ifdef _KERNEL
#      define FD_ZERO(p)     bzero((char *)(p), sizeof(*(p)))
#  endif /* _KERNEL */
#endif /* not MAXFUPLIM */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_TYPES_INCLUDED */
