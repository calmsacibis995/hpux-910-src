/*	@(#)ypxdr.c	$Revision: 12.2 $	$Date: 90/08/30 12:06:53 $  */
/*
ypxdr.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "ypxdr.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

/*
 * This contains xdr routines used by the network information service rpc interface.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define xdr_bool 		_xdr_bool
#define xdr_bytes 		_xdr_bytes
#define xdr_datum 		_xdr_datum		/* In this file */
#define xdr_reference 		_xdr_reference
#define xdr_string 		_xdr_string
#define xdr_u_long 		_xdr_u_long
#define xdr_u_short 		_xdr_u_short
#define xdr_union 		_xdr_union
#define xdr_yp_binding 		_xdr_yp_binding		/* In this file */
#define xdr_yp_inaddr 		_xdr_yp_inaddr		/* In this file */
#define xdr_ypall 		_xdr_ypall		/* In this file */
#define xdr_ypbind_resp 	_xdr_ypbind_resp	/* In this file */
#define xdr_ypbind_setdom 	_xdr_ypbind_setdom	/* In this file */
#define xdr_domain_wrap_string 	_xdr_domain_wrap_string	/* In this file */
#define xdr_ypmap_parms 	_xdr_ypmap_parms	/* In this file */
#define xdr_ypmap_wrap_string 	_xdr_ypmap_wrap_string	/* In this file */
#define xdr_ypmaplist 		_xdr_ypmaplist		/* In this file */
#define xdr_ypmaplist_wrap_string _xdr_ypmaplist_wrap_string /* In this file */
#define xdr_ypowner_wrap_string _xdr_ypowner_wrap_string /* In this file */
#define xdr_yppushresp_xfr 	_xdr_yppushresp_xfr	/* In this file */
#define xdr_ypreq_key 		_xdr_ypreq_key		/* In this file */
#define xdr_ypreq_nokey 	_xdr_ypreq_nokey	/* In this file */
#define xdr_ypreq_xfr 		_xdr_ypreq_xfr		/* In this file */
#define xdr_ypresp_key_val 	_xdr_ypresp_key_val	/* In this file */
#define xdr_ypresp_maplist 	_xdr_ypresp_maplist	/* In this file */
#define xdr_ypresp_master 	_xdr_ypresp_master	/* In this file */
#define xdr_ypresp_order 	_xdr_ypresp_order	/* In this file */
#define xdr_ypresp_val 		_xdr_ypresp_val		/* In this file */
#define ypbind_resp_arms 	_ypbind_resp_arms	/* In this file */

#endif /* _NAMESPACE_CLEAN */

#define NULL 0
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

typedef struct xdr_discrim XDR_DISCRIM;
bool xdr_datum();
bool xdr_ypdomain_wrap_string();
bool xdr_ypmap_wrap_string();
bool xdr_ypreq_key();
bool xdr_ypreq_nokey();
bool xdr_ypresp_val();
bool xdr_ypresp_key_val();
bool xdr_ypbind_resp ();
bool xdr_yp_inaddr();
bool xdr_yp_binding();
bool xdr_ypmap_parms();
bool xdr_ypowner_wrap_string();
bool xdr_ypmaplist();
bool xdr_ypmaplist_wrap_string();
bool xdr_ypref();

extern char *malloc();

/*
 * Serializes/deserializes a dbm datum data structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_datum
#pragma _HP_SECONDARY_DEF _xdr_datum xdr_datum
#define xdr_datum _xdr_datum
#endif

bool
xdr_datum(xdrs, pdatum)
	XDR * xdrs;
	datum * pdatum;

{
	return (xdr_bytes(xdrs, &(pdatum->dptr), &(pdatum->dsize),
	    YPMAXRECORD+1));
}


/*
 * Serializes/deserializes a domain name string.  This is a "wrapper" for
 * xdr_string which knows about the maximum domain name size.  
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypdomain_wrap_string
#pragma _HP_SECONDARY_DEF _xdr_ypdomain_wrap_string xdr_ypdomain_wrap_string
#define xdr_ypdomain_wrap_string _xdr_ypdomain_wrap_string
#endif

bool
xdr_ypdomain_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;
{
	return (xdr_string(xdrs, ppstring, YPMAXDOMAIN+1) );
}

/*
 * Serializes/deserializes a map name string.  This is a "wrapper" for
 * xdr_string which knows about the maximum map name size.  
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypmap_wrap_string
#pragma _HP_SECONDARY_DEF _xdr_ypmap_wrap_string xdr_ypmap_wrap_string
#define xdr_ypmap_wrap_string _xdr_ypmap_wrap_string
#endif

bool
xdr_ypmap_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;
{
	return (xdr_string(xdrs, ppstring, YPMAXMAP+1) );
}

/*
 * Serializes/deserializes a ypreq_key structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypreq_key
#pragma _HP_SECONDARY_DEF _xdr_ypreq_key xdr_ypreq_key
#define xdr_ypreq_key _xdr_ypreq_key
#endif

bool
xdr_ypreq_key(xdrs, ps)
	XDR *xdrs;
	struct ypreq_key *ps;

{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) &&
	    xdr_datum(xdrs, &ps->keydat) );
}

/*
 * Serializes/deserializes a ypreq_nokey structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypreq_nokey
#pragma _HP_SECONDARY_DEF _xdr_ypreq_nokey xdr_ypreq_nokey
#define xdr_ypreq_nokey _xdr_ypreq_nokey
#endif

bool
xdr_ypreq_nokey(xdrs, ps)
	XDR * xdrs;
	struct ypreq_nokey *ps;
{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) );
}

/*
 * Serializes/deserializes a ypreq_xfr structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypreq_xfr
#pragma _HP_SECONDARY_DEF _xdr_ypreq_xfr xdr_ypreq_xfr
#define xdr_ypreq_xfr _xdr_ypreq_xfr
#endif

bool
xdr_ypreq_xfr(xdrs, ps)
	XDR * xdrs;
	struct ypreq_xfr *ps;
{
	return (xdr_ypmap_parms(xdrs, &ps->map_parms) &&
	    xdr_u_long(xdrs, &ps->transid) &&
	    xdr_u_long(xdrs, &ps->proto) &&
	    xdr_u_short(xdrs, &ps->port) );
}

/*
 * Serializes/deserializes a ypresp_val structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypresp_val
#pragma _HP_SECONDARY_DEF _xdr_ypresp_val xdr_ypresp_val
#define xdr_ypresp_val _xdr_ypresp_val
#endif

bool
xdr_ypresp_val(xdrs, ps)
	XDR * xdrs;
	struct ypresp_val *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	    xdr_datum(xdrs, &ps->valdat) );
}

/*
 * Serializes/deserializes a ypresp_key_val structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypresp_key_val
#pragma _HP_SECONDARY_DEF _xdr_ypresp_key_val xdr_ypresp_key_val
#define xdr_ypresp_key_val _xdr_ypresp_key_val
#endif

bool
xdr_ypresp_key_val(xdrs, ps)
	XDR * xdrs;
	struct ypresp_key_val *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	    xdr_datum(xdrs, &ps->valdat) &&
	    xdr_datum(xdrs, &ps->keydat) );
}

/*
 * Serializes/deserializes a ypresp_master structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypresp_master
#pragma _HP_SECONDARY_DEF _xdr_ypresp_master xdr_ypresp_master
#define xdr_ypresp_master _xdr_ypresp_master
#endif

bool
xdr_ypresp_master(xdrs, ps)
	XDR * xdrs;
	struct ypresp_master *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	     xdr_ypowner_wrap_string(xdrs, &ps->master) );
}

/*
 * Serializes/deserializes a ypresp_order structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypresp_order
#pragma _HP_SECONDARY_DEF _xdr_ypresp_order xdr_ypresp_order
#define xdr_ypresp_order _xdr_ypresp_order
#endif

bool
xdr_ypresp_order(xdrs, ps)
	XDR * xdrs;
	struct ypresp_order *ps;
{
	return (xdr_u_long(xdrs, &ps->status) &&
	     xdr_u_long(xdrs, &ps->ordernum) );
}

/*
 * Serializes/deserializes a stream of struct ypresp_key_val's.  This is used
 * only by the client side of the transaction.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypall
#pragma _HP_SECONDARY_DEF _xdr_ypall xdr_ypall
#define xdr_ypall _xdr_ypall
#endif

bool
xdr_ypall(xdrs, callback)
	XDR * xdrs;
	struct ypall_callback *callback;
{
	bool more;
	struct ypresp_key_val kv;
	bool s;
	char keybuf[YPMAXRECORD+1];
	char valbuf[YPMAXRECORD+1];

	if (xdrs->x_op == XDR_ENCODE)
		return(FALSE);

	if (xdrs->x_op == XDR_FREE)
		return(TRUE);

	kv.keydat.dptr = keybuf;
	kv.valdat.dptr = valbuf;
	kv.keydat.dsize = YPMAXRECORD+1;
	kv.valdat.dsize = YPMAXRECORD+1;
	
	for (;;) {
		if (! xdr_bool(xdrs, &more) )
			return (FALSE);
			
		if (! more)
			return (TRUE);

		s = xdr_ypresp_key_val(xdrs, &kv);
		
		if (s) {
			s = (*callback->foreach)(kv.status, kv.keydat.dptr,
			    kv.keydat.dsize, kv.valdat.dptr, kv.valdat.dsize,
			    callback->data);
			
			if (s)
				return (TRUE);
		} else {
			return (FALSE);
		}
	}
}

/*
 * This is like xdr_ypmap_wrap_string except that it serializes/deserializes
 * an array, instead of a pointer, so xdr_reference can work on the structure
 * containing the char array itself.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypmaplist_wrap_string
#pragma _HP_SECONDARY_DEF _xdr_ypmaplist_wrap_string xdr_ypmaplist_wrap_string
#define xdr_ypmaplist_wrap_string _xdr_ypmaplist_wrap_string
#endif

bool
xdr_ypmaplist_wrap_string(xdrs, pstring)
	XDR * xdrs;
	char *pstring;
{
	char *s;

	s = pstring;
	return (xdr_string(xdrs, &s, YPMAXMAP+1) );
}

/*
 * Serializes/deserializes a ypmaplist.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypmaplist
#pragma _HP_SECONDARY_DEF _xdr_ypmaplist xdr_ypmaplist
#define xdr_ypmaplist _xdr_ypmaplist
#endif

bool
xdr_ypmaplist(xdrs, lst)
	XDR *xdrs;
	struct ypmaplist **lst;
{
	bool more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	struct ypmaplist **next;

	while (TRUE) {
		more_elements = (*lst != (struct ypmaplist *) NULL);
		
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
			
		if (! more_elements)
			return (TRUE);  /* All done */
			
		if (freeing)
			next = &((*lst)->ypml_next);

		if (! xdr_reference(xdrs, lst, (u_int) sizeof(struct ypmaplist),
		    xdr_ypmaplist_wrap_string))
			return (FALSE);
			
		lst = (freeing) ? next : &((*lst)->ypml_next);
	}
}

/*
 * Serializes/deserializes a ypresp_maplist.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypresp_maplist
#pragma _HP_SECONDARY_DEF _xdr_ypresp_maplist xdr_ypresp_maplist
#define xdr_ypresp_maplist _xdr_ypresp_maplist
#endif

bool
xdr_ypresp_maplist(xdrs, ps)
	XDR * xdrs;
	struct ypresp_maplist *ps;

{
	return (xdr_u_long(xdrs, &ps->status) &&
	   xdr_ypmaplist(xdrs, &ps->list) );
}

/*
 * Serializes/deserializes an in_addr struct.
 * 
 * Note:  There is a data coupling between the "definition" of a struct
 * in_addr implicit in this xdr routine, and the true data definition in
 * <netinet/in.h>.  
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_yp_inaddr
#pragma _HP_SECONDARY_DEF _xdr_yp_inaddr xdr_yp_inaddr
#define xdr_yp_inaddr _xdr_yp_inaddr
#endif

bool
xdr_yp_inaddr(xdrs, ps)
	XDR * xdrs;
	struct in_addr *ps;

{
	return (xdr_u_long(xdrs, &ps->s_addr));
}

/*
 * Serializes/deserializes a ypbind_binding struct.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_yp_binding
#pragma _HP_SECONDARY_DEF _xdr_yp_binding xdr_yp_binding
#define xdr_yp_binding _xdr_yp_binding
#endif

bool
xdr_yp_binding(xdrs, ps)
	XDR * xdrs;
	struct ypbind_binding *ps;

{
	return (xdr_yp_inaddr(xdrs, &ps->ypbind_binding_addr) &&
            xdr_u_short(xdrs, &ps->ypbind_binding_port));
}

/*
 * xdr discriminant/xdr_routine vector for nis binder responses
 */
XDR_DISCRIM ypbind_resp_arms[] = {
	{(int) YPBIND_SUCC_VAL, (xdrproc_t) xdr_yp_binding},
	{(int) YPBIND_FAIL_VAL, (xdrproc_t) xdr_u_long},
	{__dontcare__, (xdrproc_t) NULL}
};

/*
 * Serializes/deserializes a ypbind_resp structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypbind_resp
#pragma _HP_SECONDARY_DEF _xdr_ypbind_resp xdr_ypbind_resp
#define xdr_ypbind_resp _xdr_ypbind_resp
#endif

bool
xdr_ypbind_resp(xdrs, ps)
	XDR * xdrs;
	struct ypbind_resp *ps;

{
	return (xdr_union(xdrs, &ps->ypbind_status, &ps->ypbind_respbody,
	    ypbind_resp_arms, NULL) );
}

/*
 * Serializes/deserializes a peer server's node name
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypowner_wrap_string
#pragma _HP_SECONDARY_DEF _xdr_ypowner_wrap_string xdr_ypowner_wrap_string
#define xdr_ypowner_wrap_string _xdr_ypowner_wrap_string
#endif

bool
xdr_ypowner_wrap_string(xdrs, ppstring)
	XDR * xdrs;
	char **ppstring;

{
	return (xdr_string(xdrs, ppstring, YPMAXPEER+1) );
}

/*
 * Serializes/deserializes a ypmap_parms structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypmap_parms
#pragma _HP_SECONDARY_DEF _xdr_ypmap_parms xdr_ypmap_parms
#define xdr_ypmap_parms _xdr_ypmap_parms
#endif

bool
xdr_ypmap_parms(xdrs, ps)
	XDR *xdrs;
	struct ypmap_parms *ps;

{
	return (xdr_ypdomain_wrap_string(xdrs, &ps->domain) &&
	    xdr_ypmap_wrap_string(xdrs, &ps->map) &&
	    xdr_u_long(xdrs, &ps->ordernum) &&
	    xdr_ypowner_wrap_string(xdrs, &ps->owner) );
}

/*
 * Serializes/deserializes a ypbind_setdom structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_ypbind_setdom
#pragma _HP_SECONDARY_DEF _xdr_ypbind_setdom xdr_ypbind_setdom
#define xdr_ypbind_setdom _xdr_ypbind_setdom
#endif

bool
xdr_ypbind_setdom(xdrs, ps)
	XDR *xdrs;
	struct ypbind_setdom *ps;
{
	char *domain = ps->ypsetdom_domain;
	
	return (xdr_ypdomain_wrap_string(xdrs, &domain) &&
	    xdr_yp_binding(xdrs, &ps->ypsetdom_binding) &&
	    xdr_u_short(xdrs, &ps->ypsetdom_vers));
}

/*
 * Serializes/deserializes a yppushresp_xfr structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_yppushresp_xfr
#pragma _HP_SECONDARY_DEF _xdr_yppushresp_xfr xdr_yppushresp_xfr
#define xdr_yppushresp_xfr _xdr_yppushresp_xfr
#endif

bool
xdr_yppushresp_xfr(xdrs, ps)
	XDR *xdrs;
	struct yppushresp_xfr *ps;
{
	return (xdr_u_long(xdrs, &ps->transid) &&
	    xdr_u_long(xdrs, &ps->status));
}


