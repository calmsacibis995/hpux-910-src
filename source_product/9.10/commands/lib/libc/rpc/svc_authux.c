/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_authux.c,v $
 * $Revision: 12.3 $	$Author: dlr $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/05/03 15:23:00 $
 */

/* BEGIN_IMS_MOD   
******************************************************************************
****								
****	svc_authux.c - Handles UNIX style authentication for the server.
****								
******************************************************************************
* Description
* 	Handles UNIX flavor authentication parameters on the service side of
*	RPC. There are two svc auth implementations here: AUTH_UNIX and
*	AUTH_SHORT. _svcauth_unix does full blown UNIX style uid,gid+gids
* 	auth, _svcauth_short uses a shorthand auth to index into a cache of
*	longhand auths.
*
* Externally Callable Routines
*	_svcauth_unix		handle un*x stylue authentication
*	_svcauth_short		a placeholder - always returns an error
*
* Test Module
*	$scaffold/nfs/*
* Notes
*
* Modification History
*	7/28		D. Pan  the shorthand has been gutted for efficiency.
*	 			USED TO BE CALLED svc_auth_unix.c 
*	11/08/87	ew	added comments
*
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_authux.c,v 12.3 90/05/03 15:23:00 dlr Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#ifdef KERNEL
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/auth_unix.h"
#include "../rpc/svc_auth.h"
#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#else /* not KERNEL */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define memcpy			_memcpy
#define nl_printf		_nl_printf
#define xdr_authunix_parms 	_xdr_authunix_parms
#define xdrmem_create 		_xdrmem_create

#endif /* _NAME_SPACE_CLEAN */


#define NL_SETN 13	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <rpc/types.h>
#include <time.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <rpc/auth_unix.h>
#include <rpc/svc_auth.h>
char *mem_alloc();
#endif /* KERNEL */


/* BEGIN_IMS _svcauth_unix *
 ********************************************************************
 ****
 ****	enum auth_stat
 ****	_svcauth_unix(rqst, msg)
 ****		register struct svc_req *rqst;
 ****		register struct rpc_msg *msg;
 ****
 ********************************************************************
 * Input Parameters
 *	rqst		service request being built
 *	msg		incomming message from the wire
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	auth_stat	AUTH_OK if decode ok, else AUTH_BADCRED
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine decodes un*x style authentication credentials
 *	into the request being built. It also builds a null
 *	authentication reply.
 *
 * Algorithm
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	11/08/87	ew	added comments
 *	11/08/87	ew	converted to NS logging
 *	11/24/87	arm	fixed panic caused by call to 
 *				xdr_authunix_parms with x_op set to XDR_FREE
 *
 * External Calls
 *	xdrmem_create		create xdr stream from raw credentials
 *	XDR_INLINE		check for available data in xdr stream
 *	IXDR_xxx		pull data directly from xdr stream
 *	bcopy/memcopy		copy machine name from raw credentials
 *	NS_LOG_INFO5		log decode error
 *	nl_printf/catgets 	NLS logging in user space
 *	xdr_authunix_parms	xdr the authentication the hard way
 *	XDR_DESTROY		cleanup xdr stream
 *
 * Called By
 *	_authenticate		
 *
 ********************************************************************
 * END_IMS _svcauth_unix */

enum auth_stat
_svcauth_unix(rqst, msg)
	register struct svc_req *rqst;
	register struct rpc_msg *msg;
{
	register enum auth_stat stat;
	XDR xdrs;
	register struct authunix_parms *aup;
	register long *buf;
	struct area {
		struct authunix_parms area_aup;
		char area_machname[MAX_MACHINE_NAME];
		int area_gids[NGRPS];
	} *area;
	u_int auth_len;
	u_long str_len, gid_len;
	register int i;

#ifndef KERNEL
	nlmsg_fd = _nfs_nls_catopen();
#endif /* not KERNEL */

	area = (struct area *) rqst->rq_clntcred;
	aup = &area->area_aup;
	aup->aup_machname = area->area_machname;
	aup->aup_gids = area->area_gids;
	auth_len = (u_int)msg->rm_call.cb_cred.oa_length;
	xdrmem_create(&xdrs, msg->rm_call.cb_cred.oa_base, auth_len,XDR_DECODE);
	buf = XDR_INLINE(&xdrs, auth_len);
	if (buf != NULL) {
		aup->aup_time = IXDR_GET_LONG(buf);
		str_len = IXDR_GET_U_LONG(buf);
		if ( str_len > MAX_MACHINE_NAME ) {
			stat = AUTH_BADCRED;
			goto done;
		}
#ifdef	KERNEL
		bcopy(buf, aup->aup_machname, str_len);
#else	/* KERNEL */
		memcpy(aup->aup_machname, buf, str_len);
#endif	/* KERNEL */
		aup->aup_machname[str_len] = 0;
		str_len = RNDUP(str_len);
		buf += str_len / sizeof (long);
		aup->aup_uid = IXDR_GET_LONG(buf);
		aup->aup_gid = IXDR_GET_LONG(buf);
		gid_len = IXDR_GET_U_LONG(buf);
		if (gid_len > NGRPS) {
			stat = AUTH_BADCRED;
			goto done;
		}
		aup->aup_len = gid_len;
		for (i = 0; i < gid_len; i++) {
			aup->aup_gids[i] = IXDR_GET_LONG(buf);
		}
		/*
		 * five is the smallest unix credentials structure -
		 * timestamp, hostname len (0), uid, gid, and gids len (0).
		 */
		if ((5 + gid_len) * BYTES_PER_XDR_UNIT + str_len > auth_len) {
#ifdef KERNEL
			/* NFS_LOG3("bad auth_len gid %d str %d auth %d\n",
			       gid_len, auth_len, auth_len); */
			NS_LOG_INFO5(LE_NFS_AUTH_LEN, NS_LC_WARNING,
				     NS_LS_NFS, 0, 4,
				     rqst->rq_xprt->xp_raddr.sin_addr.s_addr,
				     gid_len, str_len, auth_len, 0);

#else
			nl_printf((catgets(nlmsg_fd,NL_SETN,1, "bad auth_len gid %1$d str %2$d auth %3$d\n")),
			       gid_len, str_len, auth_len);
#endif /* KERNEL */
			stat = AUTH_BADCRED;
			goto done;
		}
	} else if (! xdr_authunix_parms(&xdrs, aup)) {
		/*
		 * This has been removed since memory has already been
 		 * allocated by svc_getreq which calls _authenticate
 		 * which calls this routine. Hence it is the responsibility
 		 * of svc_getreq to return the allocated memory. If we call
 		 * xdr_authunix_parms here, it calls kmem_free to return the
 		 * memory pointed to by char pointer aup->machname causing
 		 * kmem-free to panic.  11/24/87 -arm
 		 *
		 * xdrs.x_op = XDR_FREE;
		 * (void)xdr_authunix_parms(&xdrs, aup);
 		 */
		stat = AUTH_BADCRED;
		goto done;
	}
	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_NULL;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	stat = AUTH_OK;
done:
	XDR_DESTROY(&xdrs);
	return (stat);
}


/* BEGIN_IMS _svcauth_short *
 ********************************************************************
 ****
 ****	enum auth_stat 
 ****	_svcauth_short(rqst, msg)
 ****		struct svc_req *rqst;
 ****		struct rpc_msg *msg;
 ****
 ********************************************************************
 * Input Parameters
 *	rqst		service request being built
 *	msg		message from the wire		
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	auth_stat	always returns AUTH_REJECTED
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	A placeholder routine which always returns an error.
 *
 * Algorithm
 *	return AUTH_REJECTEDCRED
 *
 * Modification History
 *	11/08/87	ew	added comments
 *
 * External Calls
 *  	none
 *
 ********************************************************************
 * END_IMS _svcauth_short */

/*ARGSUSED*/
enum auth_stat 
_svcauth_short(rqst, msg)
	struct svc_req *rqst;
	struct rpc_msg *msg;
{
	return (AUTH_REJECTEDCRED);
}
