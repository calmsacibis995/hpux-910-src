/*	@(#)ypprot_err.c	$Revision: 12.2 $ $Date: 90/08/30 12:07:31 $  */
/*
ypprot_err.c	2.1 86/04/14 NFSSRC
static  char sccsid[] = "ypprot_err.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define ypprot_err 	_ypprot_err	/* In this file */

#endif /* _NAMESPACE_CLEAN */

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

/*
 * Maps a nis protocol error code (as defined in
 * yp_prot.h) to a nis client interface error code (as defined in
 * ypclnt.h).
 */

#ifdef _NAMESPACE_CLEAN
#undef ypprot_err
#pragma _HP_SECONDARY_DEF _ypprot_err ypprot_err
#define ypprot_err _ypprot_err
#endif

int
ypprot_err(yp_protocol_error)
	unsigned int yp_protocol_error;
{
	int reason;

	switch (yp_protocol_error) {
	case YP_TRUE: 
		reason = 0;
		break;
 	case YP_NOMORE: 
		reason = YPERR_NOMORE;
		break;
 	case YP_NOMAP: 
		reason = YPERR_MAP;
		break;
 	case YP_NODOM: 
		reason = YPERR_DOMAIN;
		break;
 	case YP_NOKEY: 
		reason = YPERR_KEY;
		break;
 	case YP_BADARGS:
		reason = YPERR_BADARGS;
		break;
 	case YP_BADDB:
		reason = YPERR_BADDB;
		break;
 	case YP_VERS:
		reason = YPERR_VERS;
		break;
	default:
		reason = YPERR_YPERR;
		break;
	}
	
  	return(reason);
}
