/*	@(#)yperrstrng.c	$Revision: 12.2 $	$Date: 90/08/30 12:08:09 $  */
/*
yperr_string.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "(#)yperr_string.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 	_catgets
#define yperr_string 	_yperr_string		/* In this file */

#endif /* _NAMESPACE_CLEAN */

#define NL_SETN 23	/* set number */
#include <nl_types.h>
char *catgets();
static nl_catd nlmsg_fd;

#include <rpcsvc/ypclnt.h>

/*
 * This returns a pointer to an error message string appropriate to an input
 * nis error code.  An input value of zero will return a success message.
 * In all cases, the message string will start with a lower case chararacter,
 * and will be terminated neither by a period (".") nor a newline.
 */

#ifdef _NAMESPACE_CLEAN
#undef yperr_string
#pragma _HP_SECONDARY_DEF _yperr_string yperr_string
#define yperr_string _yperr_string
#endif

char *
yperr_string(code)
	int code;
{
	nlmsg_fd = _nfs_nls_catopen();
	
	switch (code) {

	case 0:  {
		return((catgets(nlmsg_fd,NL_SETN,1, "YP operation succeeded")));
	}
		
	case YPERR_BADARGS:  {
		return((catgets(nlmsg_fd,NL_SETN,2, "bad args to NIS function")));
	}
	
	case YPERR_RPC:  {
		return((catgets(nlmsg_fd,NL_SETN,3, "RPC failure on NIS operation")));
	}
	
	case YPERR_DOMAIN:  {
		return((catgets(nlmsg_fd,NL_SETN,4, "can't bind to a server that serves NIS domain")));
	}
	
	case YPERR_MAP:  {
		return((catgets(nlmsg_fd,NL_SETN,5, "no such map in server's NIS domain")));
	}
		
	case YPERR_KEY:  {
		return((catgets(nlmsg_fd,NL_SETN,6, "no such key in map")));
	}
	
	case YPERR_YPERR:  {
		return((catgets(nlmsg_fd,NL_SETN,7, "internal NIS server or client error")));
	}
	
	case YPERR_RESRC:  {
		return((catgets(nlmsg_fd,NL_SETN,8, "local resource allocation failure")));
	}
	
	case YPERR_NOMORE:  {
		return((catgets(nlmsg_fd,NL_SETN,9, "no more records in map")));
	}
	
	case YPERR_PMAP:  {
		return((catgets(nlmsg_fd,NL_SETN,10, "can't communicate with portmap")));
	}
		
	case YPERR_YPBIND:  {
		return((catgets(nlmsg_fd,NL_SETN,11, "can't communicate with ypbind")));
	}
		
	case YPERR_YPSERV:  {
		return((catgets(nlmsg_fd,NL_SETN,12, "can't communicate with ypserv")));
	}
		
	case YPERR_NODOM:  {
		return((catgets(nlmsg_fd,NL_SETN,13, "local NIS domain name not set")));
	}

	case YPERR_BADDB:  {
		return((catgets(nlmsg_fd,NL_SETN,14, "YP map is defective")));
	}

	case YPERR_VERS:  {
		return((catgets(nlmsg_fd,NL_SETN,15, "YP client/server version mismatch")));
	}

	default:  {
		return((catgets(nlmsg_fd,NL_SETN,16, "unknown NIS client error code")));
	}
	
	}

}
