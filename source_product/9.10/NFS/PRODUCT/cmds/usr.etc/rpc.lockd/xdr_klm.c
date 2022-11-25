/* @(#)rpc.lockd:	$Revision: 1.10.109.2 $	$Date: 92/01/21 13:02:45 $
*/
/* (#)xdr_klm.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)xdr_klm.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)xdr_klm.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * xdr_klm.c
	 * modified from klm_prot.c generated from rpcgen
	 */

#ifndef NULL
#define NULL 0
#endif

#include "prot_lock.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fs.h>
#include <sys/vfs.h>
/* #include <sys/vnode.h> */
#include <sys/stat.h>
#define NFSSERVER
#undef fh_len	/* define in prot_lock.h conflicts with nfs.h */
#include <nfs/nfs.h>

extern char hostname[255];		/* used by remote_data() */

bool_t
xdr_klm_stats(xdrs,objp)
	XDR *xdrs;
	klm_stats *objp;
{
	if (! xdr_enum(xdrs, objp)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lock(xdrs,objp)
	XDR *xdrs;
	struct lock *objp;
{
fhandle_t *fh;

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

#ifdef hpux32
        /* -asynch was removed from fh_opts in hpux's nfs 4.1 
            dmw
        */

	/* This was added for hp.  File handles include fh_opts like
	   the async option.  This will cause file handles for the same
	   file to fail in later comparisons.  So, we must zero the field
        */
	if(strcmp(objp->server_name, hostname) == 0) {
	   fh = (fhandle_t*)objp->fh.n_bytes;
	   fh->fh_opts = 0;
	}
#endif
	return(TRUE);
}




bool_t
xdr_klm_holder(xdrs,objp)
	XDR *xdrs;
	nlm_holder *objp;
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

	if (! xdr_union(xdrs, &objp->stat, &objp->klm_testrply, choices,NULL)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_stat(xdrs,objp)
	XDR *xdrs;
	klm_testrply *objp;
{
	if (! xdr_klm_stats(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lockargs(xdrs,objp)
	XDR *xdrs;
	reclock *objp;
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
	reclock *objp;
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
	reclock *objp;
{
	if (! xdr_klm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}
