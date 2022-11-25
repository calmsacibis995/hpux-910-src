/* $Header: errno.h,v 1.32.83.5 93/12/06 22:19:58 marshall Exp $ */

#ifndef _SYS_ERRNO_INCLUDED
#define _SYS_ERRNO_INCLUDED

/*
 * Error codes
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */


#ifdef _INCLUDE__STDC__
   /* External variable errno */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */
         extern int errno;
#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

   /* Possible errno values */

#  define EDOM		33	/* Math arg out of domain of func */
#  define ERANGE	34	/* Math result not representable */

#endif /* _INCLUDE__STDC__ */

/* Things in POSIX IEEE 1003.1 beyond ANSI C */
#ifdef _INCLUDE_POSIX_SOURCE
#define	EPERM		1	/* Not super-user		*/
#define	ENOENT		2	/* No such file or directory	*/
#define	ESRCH		3	/* No such process		*/
#define	EINTR		4	/* interrupted system call	*/
#define	EIO		5	/* I/O error			*/
#define	ENXIO		6	/* No such device or address	*/
#define	E2BIG		7      	/* Arg list too long		*/
#define	ENOEXEC		8	/* Exec format error		*/
#define	EBADF		9	/* Bad file number		*/
#define	ECHILD		10	/* No children			*/
#define	EAGAIN		11	/* No more processes		*/
#define	ENOMEM		12	/* Not enough core		*/
#define	EACCES		13      /* Permission denied		*/
#define	EFAULT		14	/* Bad address			*/
#define	EBUSY		16	/* Mount device busy		*/
#define	EEXIST		17	/* File exists			*/
#define	EXDEV		18	/* Cross-device link		*/
#define	ENODEV		19	/* No such device		*/
#define	ENOTDIR		20	/* Not a directory		*/
#define	EISDIR		21	/* Is a directory		*/
#define	EINVAL		22	/* Invalid argument		*/
#define	ENFILE		23	/* File table overflow		*/
#define	EMFILE		24	/* Too many open files		*/
#define	ENOTTY		25	/* Not a typewriter		*/
#define	EFBIG		27	/* File too large		*/
#define	ENOSPC		28	/* No space left on device	*/
#define	ESPIPE		29	/* Illegal seek			*/
#define	EROFS		30	/* Read only file system	*/
#define	EMLINK		31	/* Too many links		*/
#define	EPIPE		32	/* Broken pipe			*/
#define EDEADLK 	45	/* A deadlock would occur	*/
#define	ENOLCK  	46	/* System record lock table was full */
#define EILSEQ		47	/* Illegal byte sequence        */
#define	ENOTEMPTY   	247	/* Directory not empty          */
#define	ENAMETOOLONG 	248	/* File name too long           */
#define	ENOSYS 	  	251     /* Function not implemented     */
#endif /* _INCLUDE_POSIX_SOURCE */

/* Things in XPG3 not in POSIX or ANSI C */
#ifdef _INCLUDE_XOPEN_SOURCE
#define	ENOTBLK		15	/* Block device required	*/
#define	ETXTBSY		26	/* Text file busy		*/
#if defined( _INCLUDE_HPUX_SOURCE) || ! defined( _INCLUDE_AES_SOURCE)
#define	ENOMSG  	35      /* No message of desired type   */
#define	EIDRM		36	/* Identifier removed		*/
#endif /* defined( _INCLUDE_HPUX_SOURCE) || ! defined( _INCLUDE_AES_SOURCE) */
#endif /* _INCLUDE_XOPEN_SOURCE */

/* Things in AES not in  XPG3, POSIX or ANSI C */
#ifdef _INCLUDE_AES_SOURCE
#define	ELOOP	    		249	/* Too many levels of symbolic links */
#endif /* _INCLUDE_AES_SOURCE */

/* Things in HP-UX not in XPG3, POSIX or ANSI C */
#ifdef _INCLUDE_HPUX_SOURCE
/* The error numbers between 37 and 44 are not produced by HP-UX. They
   will track whatever the UNIX(tm) system does in the future */
#define	ECHRNG	37	/* Channel number out of range		*/
#define	EL2NSYNC 38	/* Level 2 not synchronized		*/
#define	EL3HLT	39	/* Level 3 halted			*/
#define	EL3RST	40	/* Level 3 reset			*/
#define	ELNRNG	41	/* Link number out of range		*/
#define	EUNATCH 42	/* Protocol driver not attached		*/
#define	ENOCSI	43	/* No CSI structure available		*/
#define	EL2HLT	44	/* Level 2 halted			*/

#define ENONET		50	/* Machine is not on the network	*/
#define ENODATA		51	/* no data (for no delay io)		*/
#define ETIME		52	/* timer expired			*/
#define ENOSR		53	/* out of streams resources		*/
#define ENOSTR		54	/* Device not a stream			*/
#define ENOPKG		55	/* Package not installed                */
#define ENOLINK		57	/* the link has been severed */
#define EADV		58	/* advertise error */
#define ESRMNT		59	/* srmount error */
#define	ECOMM		60	/* Communication error on send		*/
#define EPROTO		61	/* Protocol error			*/
#define	EMULTIHOP 	64	/* multihop attempted */
#define	EDOTDOT 	66	/* Cross mount point (not really error)*/
#define EBADMSG 	67	/* trying to read unreadable message	*/


#define	ENOSYM		215	/* symbol does not exist in executable  */

/* disk quotas errors */
#  define	EUSERS	 68	/* For Sun compatibilty, will not occur.*/
#  define	EDQUOT	 69	/* Disc quota exceeded 		*/

/* Network File System */
#  define	ESTALE		70	/* Stale NFS file handle */
#  define	EREMOTE		71	/* Too many levels of remote in path */

/* ipc/network software */

/* argument errors */
#  define ENOTSOCK		216	/* Socket operation on non-socket */
#  define EDESTADDRREQ		217	/* Destination address required */
#  define EMSGSIZE		218	/* Message too long */
#  define EPROTOTYPE		219	/* Protocol wrong type for socket */
#  define ENOPROTOOPT		220	/* Protocol not available */
#  define EPROTONOSUPPORT	221	/* Protocol not supported */
#  define ESOCKTNOSUPPORT	222	/* Socket type not supported */
#  define EOPNOTSUPP	 	223	/* Operation not supported */
#  define EPFNOSUPPORT 		224	/* Protocol family not supported */
#  define EAFNOSUPPORT 		225 	/*Address family not supported by 
			  		  protocol family*/
#  define EADDRINUSE		226	/* Address already in use */
#  define EADDRNOTAVAIL 	227	/* Can't assign requested address */

	/* operational errors */
#  define ENETDOWN		228	/* Network is down */
#  define ENETUNREACH		229	/* Network is unreachable */
#  define ENETRESET		230	/* Network dropped connection on 
					   reset */
#  define ECONNABORTED		231	/* Software caused connection abort */
#  define ECONNRESET		232	/* Connection reset by peer */
#  define ENOBUFS		233	/* No buffer space available */
#  define EISCONN		234	/* Socket is already connected */
#  define ENOTCONN		235	/* Socket is not connected */
#  define ESHUTDOWN		236	/* Can't send after socket shutdown */
#  define ETOOMANYREFS		237	/* Too many references: can't splice */
#  define ETIMEDOUT		238	/* Connection timed out */
#  define ECONNREFUSED		239	/* Connection refused */
#  ifdef __hp9000s800
#    define EREFUSED		ECONNREFUSED	/* Double define for NFS*/
#  endif /* __hp9000s800 */
#  define EREMOTERELEASE	240	/* Remote peer released connection */
#  define EHOSTDOWN		241	/* Host is down */
#  define EHOSTUNREACH		242	/* No route to host */

#define	EALREADY    		244	/* Operation already in progress */
#define	EINPROGRESS 		245	/* Operation now in progress */
#define	EWOULDBLOCK 		246	/* Operation would block */

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
#	include "../h/_errno.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_errno.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_ERRNO_INCLUDED */
