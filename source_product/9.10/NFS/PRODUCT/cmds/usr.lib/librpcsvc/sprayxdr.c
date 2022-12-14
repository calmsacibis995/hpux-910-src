/* 	@(#)sprayxdr.c	$Revision: 1.18.109.1 $	$Date: 91/11/19 14:25:37 $
sprayxdr.c	2.1 86/04/14 NFSSRC
static  char sccsid[] = "sprayxdr.c 1.1 86/02/05 Copyr 1985 Sun Micro";
*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <time.h>
#include <rpc/rpc.h>
#include <rpcsvc/spray.h>

xdr_sprayarr(xdrsp, arrp)
	XDR *xdrsp;
	struct sprayarr *arrp;
{
	if (!xdr_bytes(xdrsp, &arrp->data, &arrp->lnth, SPRAYMAX))
		return(0);
}

xdr_spraycumul(xdrsp, cumulp)
	XDR *xdrsp;
	struct spraycumul *cumulp;
{
	if (!xdr_u_int(xdrsp, &cumulp->counter))
		return(0);
	if (!xdr_timeval(xdrsp, &cumulp->clock))
		return(0);
}
