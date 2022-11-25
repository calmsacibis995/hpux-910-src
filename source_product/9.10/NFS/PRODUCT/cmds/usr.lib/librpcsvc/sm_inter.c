/*	@(#)sm_inter.c	$Revision: 1.3.109.1 $	$Date: 91/11/19 14:25:27 $
sm_inter.c	2.1 86/04/14 NFSSRC
*/
/* (#)sm_inter.c	1.4 87/08/13 3.2/4.3NFSSRC */
#ifndef lint
static char sccsid[] = "(#)sm_inter.c 1.1 86/09/25 Copyright 1984, 1987 Sun Microsystems, Inc.";
#endif
/*
 *
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 *
 */

	/*
	 * Copyright (c) 1984, 1987 by Sun Microsystems, Inc.
	 */
#include <rpc/rpc.h>
#include "sm_inter.h"


bool_t
xdr_sm_name(xdrs,objp)
	XDR *xdrs;
	sm_name *objp;
{
	if (! xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_my_id(xdrs,objp)
	XDR *xdrs;
	my_id *objp;
{
	if (! xdr_string(xdrs, &objp->my_name, SM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->my_prog)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->my_vers)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->my_proc)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_mon_id(xdrs,objp)
	XDR *xdrs;
	mon_id *objp;
{
	if (! xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_my_id(xdrs, &objp->my_id)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_mon(xdrs,objp)
	XDR *xdrs;
	mon *objp;
{
	if (! xdr_mon_id(xdrs, &objp->mon_id)) {
		return(FALSE);
	}
	if (! xdr_opaque(xdrs, objp->priv, 16)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_sm_stat(xdrs,objp)
	XDR *xdrs;
	sm_stat *objp;
{
	if (! xdr_int(xdrs, &objp->state)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_res(xdrs,objp)
	XDR *xdrs;
	res *objp;
{
	if (! xdr_enum(xdrs, (enum_t *) objp)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_sm_stat_res(xdrs,objp)
	XDR *xdrs;
	sm_stat_res *objp;
{
	if (! xdr_res(xdrs, &objp->res_stat)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->state)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_status(xdrs,objp)
	XDR *xdrs;
	status *objp;
{
	if (! xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->state)) {
		return(FALSE);
	}
	if (! xdr_opaque(xdrs, objp->priv, 16)) {
		return(FALSE);
	}
	return(TRUE);
}



/* 
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 */
