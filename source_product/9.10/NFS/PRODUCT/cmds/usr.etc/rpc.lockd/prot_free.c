#ifndef lint
static char sccsid[] = "@(#)prot_freeall.c	1.1 88/08/05 NFSSRC4.0 1.3 88/02/07 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_freeall.c consists of subroutines that implement the
	 * DOS-compatible file sharing services for PC-NFS
	 */

#include <stdio.h>
#include <sys/file.h>
#include "prot_lock.h"
#include "priv_prot.h"

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS


extern int debug;
extern int grace_period;
extern char *xmalloc();
extern void xfree();
extern void zap_all_locks_for();
extern bool_t obj_cmp();
char *malloc();

void *
proc_nlm_freeall(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_notify	req;
/*
 * Allocate space for arguments and decode them
 */

	req.name = NULL;
	if (!svc_getargs(Transp, xdr_nlm_notify, &req)) {
		svcerr_decode(Transp);
		return;
	}

	if (debug) {
	    logmsg((catgets(nlmsg_fd,NL_SETN,330,"proc_nlm_freeall from %s\n")),
			req.name);
	}
	destroy_client_shares(req.name);
	zap_all_locks_for(req.name);

	xfree(&req.name);
	svc_sendreply(Transp, xdr_void, NULL);
}
