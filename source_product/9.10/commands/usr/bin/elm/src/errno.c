/**			errno.c				**/

/*
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This routine maps error numbers to error names and error messages.
 *  These are all directly ripped out of the include file errno.h, and
 *  are HOPEFULLY standardized across the different breeds of Unix!!
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN  14
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char           *err_name[] = {
			     /* 0 */ "NOERROR", "No error status currently",
			     /* 1 */ "EPERM", "Not super-user",
			     /* 2 */ "ENOENT", "No such file or directory",
			     /* 3 */ "ESRCH", "No such process",
			     /* 4 */ "EINTR", "Interrupted system call",
			     /* 5 */ "EIO", "I/O error",
			     /* 6 */ "ENXIO", "No such device or address",
			     /* 7 */ "E2BIG", "Arg list too long",
			     /* 8 */ "ENOEXEC", "Exec format error",
			     /* 9 */ "EBADF", "Bad file number",
			     /* 10 */ "ECHILD", "No children",
			     /* 11 */ "EAGAIN", "No more processes",
			     /* 12 */ "ENOMEM", "Not enough core",
			     /* 13 */ "EACCES", "Permission denied",
			     /* 14 */ "EFAULT", "Bad address",
			     /* 15 */ "ENOTBLK", "Block device required",
			     /* 16 */ "EBUSY", "Mount device busy",
			     /* 17 */ "EEXIST", "File exists",
			     /* 18 */ "EXDEV", "Cross-device link",
			     /* 19 */ "ENODEV", "No such device",
			     /* 20 */ "ENOTDIR", "Not a directory",
			     /* 21 */ "EISDIR", "Is a directory",
			     /* 22 */ "EINVAL", "Invalid argument",
			     /* 23 */ "ENFILE", "File table overflow",
			     /* 24 */ "EMFILE", "Too many open files",
			     /* 25 */ "ENOTTY", "Not a typewriter",
			     /* 26 */ "ETXTBSY", "Text file busy",
			     /* 27 */ "EFBIG", "File too large",
			     /* 28 */ "ENOSPC", "No space left on device",
			     /* 29 */ "ESPIPE", "Illegal seek",
			     /* 30 */ "EROFS", "Read only file system",
			     /* 31 */ "EMLINK", "Too many links",
			     /* 32 */ "EPIPE", "Broken pipe",
			     /* 33 */ "EDOM", "Math arg out of domain of func",
			     /* 34 */ "ERANGE", "Math result not representable",
			     /* 35 */ "ENOMSG", "No message of desired type",
			     /* 36 */ "EIDRM", "Identifier removed"
};


char           *strcpy();


char  
*error_name( errnumber )

	int             errnumber;

{

	if ( errnumber < 0 || errnumber > 36 )
		sprintf( err_name_buf, catgets(nl_fd,NL_SETN,1,"ERR-UNKNOWN (%d)"), errnumber );
	else
		strcpy( err_name_buf, err_name[2 * errnumber] );

	return ( (char *) err_name_buf );
}


char  
*error_description( errnumber )

	int             errnumber;

{

	if ( errnumber < 0 || errnumber > 36 )
		sprintf( err_desc_buf, catgets(nl_fd,NL_SETN,2,"Unknown error - %d - No description"), 
				 errnumber );
	else
		strcpy( err_desc_buf, err_name[2 * errnumber + 1] );

	return ( (char *) err_desc_buf );
}
