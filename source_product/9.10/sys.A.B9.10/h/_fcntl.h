/*
 * @(#)_fcntl.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 17:00:52 $
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

#ifndef _UNSUPP_SYS_FCNTL_INCLUDED /* multiple inclusion protection */
#define _UNSUPP_SYS_FCNTL_INCLUDED

/* Miscellaneous HP-UX only stuff */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _KERNEL
/* 
 * The following defines were added to support the NFS lock manager
 * via the nfs_fcntl() call (see nfs_server.c).  They are intended to
 * be used STRICTLY by the KERNEL ONLY.  See nfs_fcntl() and
 * the locking routines in ufs_lockf.c for more details.
 */
#      define F_SETLK_NFSCALLBACK   (-1) /* Call back on blocking locks */
#      define F_QUERY_ANY_LOCKS     (-2) /* This pid have any locks? */
#      define F_QUERY_ANY_NFS_CALLBACKS (-3) /* Any NFS callbacks on locks? */
#  endif /* _KERNEL */
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _UNSUPP_SYS_FCNTL_INCLUDED */
