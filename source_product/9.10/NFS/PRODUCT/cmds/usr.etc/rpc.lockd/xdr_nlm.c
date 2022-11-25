/* @(#)rpc.lockd:	$Revision: 1.14.109.2 $	$Date: 92/01/21 13:03:43 $
*/
/* (#)xdr_nlm.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)xdr_nlm.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)xdr_nlm.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * modified from nlm_prot.c generated from rpcgen
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


extern char hostname[255];		/* used by xdr_nlm_lock() */

bool_t
xdr_nlm_stats(xdrs,objp)
	XDR *xdrs;
	nlm_stats *objp;
{
	if (! xdr_enum(xdrs, (enum_t *) objp)) {
		return(FALSE);
	}
	return(TRUE);
}



bool_t
xdr_nlm_holder(xdrs,objp)
	XDR *xdrs;
	nlm_holder *objp;
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->svid)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->oh)) {
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
xdr_nlm_testrply(xdrs,objp)
	XDR *xdrs;
	nlm_testrply *objp;
{
	static struct xdr_discrim choices[] = {
		{ (int) nlm_granted, xdr_void },
		{ (int) nlm_denied, xdr_nlm_holder },
		{ (int) nlm_denied_nolocks, xdr_void },
		{ (int) nlm_blocked, xdr_void },
		{ (int) nlm_denied_grace_period, xdr_void },
		{ __dontcare__, NULL }
	};

	if (! xdr_union(xdrs, (enum_t *)  &objp->stat,
			       (char *) &objp->nlm_testrply, choices, NULL)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_stat(xdrs,objp)
	XDR *xdrs;
	nlm_stat *objp;
{
	if (! xdr_nlm_stats(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_res(xdrs,objp)
	XDR *xdrs;
	remote_result *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_stat(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_testres(xdrs,objp)
	XDR *xdrs;
	remote_result *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_testrply(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_lock(xdrs,objp)
	XDR *xdrs;
	struct lock *objp;
{
fhandle_t *fh;

	if (! xdr_string(xdrs, &objp->caller_name, LM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->fh)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->oh)) {
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

#ifdef hpux32
        /* -asynch was removed from fh_opts in hpux's nfs 4.1 
            dmw
        */

	/* This was added for hp.  File handles include fh_opts like
	   the async option.  This will cause file handles for the same
	   file to fail in later comparisons.  So, we must zero the field
        */
	if(strcmp(objp->caller_name, hostname) != 0) {
	   fh = (fhandle_t*)objp->fh.n_bytes;
	   fh->fh_opts = 0;
	}
#endif
	return(TRUE);
}




bool_t
xdr_nlm_lockargs(xdrs,objp)
	XDR *xdrs;
	reclock *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->reclaim)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->state)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_cancargs(xdrs,objp)
	XDR *xdrs;
	reclock *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_testargs(xdrs,objp)
	XDR *xdrs;
	reclock *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_unlockargs(xdrs,objp)
	XDR *xdrs;
	reclock *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}



bool_t
xdr_fsh_mode(xdrs, objp)
	XDR *xdrs;
	fsh_mode *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_fsh_access(xdrs, objp)
	XDR *xdrs;
	fsh_access *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_share(xdrs, objp)
	XDR *xdrs;
	nlm_share *objp;
{
	if (!xdr_string(xdrs, (char **)&objp->caller_name, LM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->fh)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->oh)) {
		return (FALSE);
	}
	if (!xdr_fsh_mode(xdrs, &objp->mode)) {
		return (FALSE);
	}
	if (!xdr_fsh_access(xdrs, &objp->access)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_shareargs(xdrs, objp)
	XDR *xdrs;
	nlm_shareargs *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_share(xdrs, &objp->share)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->reclaim)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_shareres(xdrs, objp)
	XDR *xdrs;
	nlm_shareres *objp;
{
	if (!xdr_netobj(xdrs, &objp->cookie)) {
		return (FALSE);
	}
	if (!xdr_nlm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->sequence)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_nlm_notify(xdrs, objp)
	XDR *xdrs;
	nlm_notify *objp;
{
	if (!xdr_string(xdrs, &objp->name, MAXNAMELEN)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->state)) {
		return (FALSE);
	}
	return (TRUE);
}

