/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/klm_kprot.c,v $
 * $Revision: 1.2.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 22:48:19 $
 */

/*
 * Copyright (c) 1988 by Hewlett Packard
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * XDR routines for Kernel<->Lock-Manager communication
 *
 * Generated from ../rpcsvc/klm_prot.x
 */

#ifdef hpux
#include "../h/types.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"	
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/svc_auth.h"
#else
#include "../rpc/rpc.h"
#endif
#include "../nfs/klm_prot.h"

#ifndef NULL
#define NULL 0
#endif

bool_t
xdr_klm_stats(xdrs,objp)
	XDR *xdrs;
	klm_stats *objp;
{
	if (! xdr_enum(xdrs, (enum_t *) objp)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lock(xdrs,objp)
	XDR *xdrs;
	klm_lock *objp;
{
	if (! xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->fh)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->pid)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_holder(xdrs,objp)
	XDR *xdrs;
	klm_holder *objp;
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->svid)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_stat(xdrs,objp)
	XDR *xdrs;
#ifdef _KERNEL
	klm_testrply *objp;
#else
	klm_stat *objp;
#endif
{
	if (! xdr_klm_stats(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_testrply(xdrs,objp)
	XDR *xdrs;
	klm_testrply *objp;
{
	static struct xdr_discrim choices[] = {
		{ (int) klm_granted, xdr_void },
		{ (int) klm_denied, xdr_klm_holder },
		{ (int) klm_denied_nolocks, xdr_void },
		{ (int) klm_working, xdr_void },
		{ __dontcare__, NULL }
	};

	if (! xdr_union(xdrs, (enum_t *) &objp->stat, (char *) &objp->klm_testrply, choices, NULL_xdrproc_t)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lockargs(xdrs,objp)
	XDR *xdrs;
	klm_lockargs *objp;
{
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_klm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_testargs(xdrs,objp)
	XDR *xdrs;
#ifdef _KERNEL
	klm_lockargs *objp;
#else
	klm_testargs *objp;
#endif
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_klm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_unlockargs(xdrs,objp)
	XDR *xdrs;
#ifdef _KERNEL
	klm_lockargs *objp;
#else
	klm_unlockargs *objp;
#endif
{
	if (! xdr_klm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}


