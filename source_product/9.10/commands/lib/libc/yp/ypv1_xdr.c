/*	@(#)ypv1_xdr.c	$Revision: 12.2 $	$Date: 90/08/30 12:06:19 $  */
/*
ypv1_xdr.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "ypv1_xdr.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

/*
 * This contains the xdr functions needed by ypserv and the yp
 * administrative tools to support the previous version of the nis protocol.
 * Note that many "old" xdr functions are called, with the assumption that
 * they have not changed between the v1 protocol (which this module exists
 * to support) and the current v2 protocol.  
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */
#define xdr_union _xdr_union
#define xdr_yp_binding _xdr_yp_binding
#define xdr_ypdomain_wrap_string _xdr_ypdomain_wrap_string
#define xdr_ypmap_parms _xdr_ypmap_parms
#define xdr_ypreq_key _xdr_ypreq_key
#define xdr_ypreq_nokey _xdr_ypreq_nokey
#define xdr_ypresp_key_val _xdr_ypresp_key_val
#define xdr_ypresp_val _xdr_ypresp_val

#endif /* _NAMESPACE_CLEAN */

#define NULL 0
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>
typedef struct xdr_discrim XDR_DISCRIM;

extern bool xdr_ypreq_key();
extern bool xdr_ypreq_nokey();
extern bool xdr_ypresp_val();
extern bool xdr_ypresp_key_val();
extern bool xdr_ypmap_parms();

/*
 * xdr discriminant/xdr_routine vector for nis requests.
 */
XDR_DISCRIM _yprequest_arms[] = {
	{(int) YPREQ_KEY, (xdrproc_t) xdr_ypreq_key},
	{(int) YPREQ_NOKEY, (xdrproc_t) xdr_ypreq_nokey},
	{(int) YPREQ_MAP_PARMS, (xdrproc_t) xdr_ypmap_parms},
	{__dontcare__, (xdrproc_t) NULL}
};

/*
 * Serializes/deserializes a yprequest structure.
 */
bool
_xdr_yprequest (xdrs, ps)
	XDR * xdrs;
	struct yprequest *ps;

{
	return(xdr_union(xdrs, &ps->yp_reqtype, &ps->yp_reqbody,
	    _yprequest_arms, NULL) );
}


/*
 * xdr discriminant/xdr_routine vector for nis responses
 */
XDR_DISCRIM _ypresponse_arms[] = {
	{(int) YPRESP_VAL, (xdrproc_t) xdr_ypresp_val},
	{(int) YPRESP_KEY_VAL, (xdrproc_t) xdr_ypresp_key_val},
	{(int) YPRESP_MAP_PARMS, (xdrproc_t) xdr_ypmap_parms},
	{__dontcare__, (xdrproc_t) NULL}
};

/*
 * Serializes/deserializes a ypresponse structure.
 */
bool
_xdr_ypresponse (xdrs, ps)
	XDR * xdrs;
	struct ypresponse *ps;

{
	return(xdr_union(xdrs, &ps->yp_resptype, &ps->yp_respbody,
	    _ypresponse_arms, NULL) );
}

/*
 * Serializes/deserializes a ypbind_oldsetdom structure.
 */
bool
_xdr_ypbind_oldsetdom(xdrs, ps)
	XDR *xdrs;
	struct ypbind_setdom *ps;
{
	char *domain = ps->ypsetdom_domain;
	
	return(xdr_ypdomain_wrap_string(xdrs, &domain) &&
	    xdr_yp_binding(xdrs, &ps->ypsetdom_binding) );
}
