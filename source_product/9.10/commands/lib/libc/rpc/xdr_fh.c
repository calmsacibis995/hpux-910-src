/* SCCSID strings for NFS/300 group */
/* %Z%%I%	[%E%  %U%] */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr_fh.c,v 12.1 90/03/21 11:27:07 dlr Exp $ (Hewlett-Packard)";
#endif

/* Routines moved from usr/lib/librpcsvc.a to libc.a so that 		*/
/* umount and mount can be compiled without requiring librpcsvc.a 	*/

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define xdr_fhandle 	_xdr_fhandle	/* In this file */
#define xdr_fhstatus 	_xdr_fhstatus	/* In this file */
#define xdr_int 	_xdr_int
#define xdr_opaque 	_xdr_opaque
#define xdr_path 	_xdr_path	/* In this file */
#define xdr_string 	_xdr_string

#endif /* _NAMESPACE_CLEAN */

#include <time.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>


#ifdef _NAMESPACE_CLEAN
#undef xdr_fhstatus
#pragma _HP_SECONDARY_DEF _xdr_fhstatus xdr_fhstatus
#define xdr_fhstatus _xdr_fhstatus
#endif

xdr_fhstatus(xdrs, fhsp)
	XDR *xdrs;
	struct fhstatus *fhsp;
{
	if (!xdr_int(xdrs, &fhsp->fhs_status))
		return FALSE;
	if (fhsp->fhs_status == 0) {
		if (!xdr_fhandle(xdrs, &fhsp->fhs_fh))
			return FALSE;
	}
}

#ifdef _NAMESPACE_CLEAN
#undef xdr_fhandle
#pragma _HP_SECONDARY_DEF _xdr_fhandle xdr_fhandle
#define xdr_fhandle _xdr_fhandle
#endif

xdr_fhandle(xdrs, fhp)
	XDR *xdrs;
	fhandle_t *fhp;
{
	if (xdr_opaque(xdrs, fhp, NFS_FHSIZE)) {
		return (TRUE);
	}
	return (FALSE);
}


#ifdef _NAMESPACE_CLEAN
#undef xdr_path
#pragma _HP_SECONDARY_DEF _xdr_path xdr_path
#define xdr_path _xdr_path
#endif

bool_t
xdr_path(xdrs, pathp)
	XDR *xdrs;
	char **pathp;
{
	if (xdr_string(xdrs, pathp, 1024)) {
		return(TRUE);
	}
	return(FALSE);
}
