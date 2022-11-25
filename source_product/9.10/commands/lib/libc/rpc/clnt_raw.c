/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.4 $	$Date: 92/02/07 17:07:48 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/clnt_raw.c,v $
 * $Revision: 12.4 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:07:48 $
 *
 * Revision 12.3  90/08/30  11:54:16  11:54:16  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.2  90/06/08  10:16:06  10:16:06  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/03/20  14:47:09  14:47:09  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:28  16:08:28  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/03  10:51:39  10:51:39  dds (Darren Smith)
 * Added "#ifndef NULL" around the #define NULL to prevent warning on 800. dds.
 * 
 * Revision 1.1.10.9  89/02/03  09:50:25  09:50:25  cmahon (Christina Mahon)
 * Added "#ifndef NULL" around the #define NULL to prevent warning on 800. dds.
 * 
 * Revision 1.1.10.8  89/01/26  11:54:33  11:54:33  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.7  89/01/16  15:09:49  15:09:49  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:19  14:48:19  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.4  88/08/05  13:52:56  13:52:56  dds (Darren Smith)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.6  88/08/05  12:52:08  12:52:08  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.5  88/06/16  10:35:57  10:35:57  cmahon (Christina Mahon)
 * Updated to RPC 3.9 standards.   The code in inside ifdef's.
 * 
 * Revision 1.1.10.4  88/03/24  16:35:02  16:35:02  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now the NLS routines are only called once
 *  This improves NIS performance on the s800.
 * 
 * Revision 1.1.10.3  87/08/07  14:39:21  14:39:21  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:07:51  22:07:51  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:38:57  11:38:57  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_raw.c,v 12.4 92/02/07 17:07:48 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * clnt_raw.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Memory based rpc for simple testing and timing.
 * Interface to create an rpc client and server in the same process.
 * This lets us similate rpc and get round trip overhead, without
 * any interference from the kernal.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define authnone_create 	_authnone_create
#define catgets 		_catgets
#define clntraw_create 		_clntraw_create		/* In this file */
#define perror 			_perror
#define svc_getreq		 _svc_getreq
#define xdr_callhdr 		_xdr_callhdr
#define xdr_opaque_auth 	_xdr_opaque_auth
#define xdr_replymsg 		_xdr_replymsg
#define xdrmem_create 		_xdrmem_create

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 6	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <rpc/types.h>
#include <time.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>

#ifndef NULL
#define NULL ((caddr_t)0)
#endif /* NULL */
#define MCALL_MSG_SIZE 24

/*
 * This is the "network" we will be moving stuff over.
 */
char _raw_buf[UDPMSGSIZE];

static char	mashl_callmsg[MCALL_MSG_SIZE];
static u_int	mcnt;

static enum clnt_stat	clntraw_call();
static void		clntraw_abort();
static void		clntraw_geterr();
static bool_t		clntraw_freeres();
static bool_t           clntraw_control();
static void		clntraw_destroy();

static struct clnt_ops client_ops = {
	clntraw_call,
	clntraw_abort,
	clntraw_geterr,
	clntraw_freeres,
	clntraw_destroy,
	clntraw_control
};

static CLIENT	client_object;
static XDR	xdr_stream;

void	svc_getreq();

/*
 * Create a client handle for memory based rpc.
 */

#ifdef _NAMESPACE_CLEAN
#undef clntraw_create
#pragma _HP_SECONDARY_DEF _clntraw_create clntraw_create
#define clntraw_create _clntraw_create
#endif

CLIENT *
clntraw_create(prog, vers)
	u_long prog;
	u_long vers;
{
	struct rpc_msg call_msg;
	XDR *xdrs = &xdr_stream;
	CLIENT	*client = &client_object;

	nlmsg_fd = _nfs_nls_catopen();

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;
	xdrmem_create(xdrs, mashl_callmsg, MCALL_MSG_SIZE, XDR_ENCODE); 
	if (! xdr_callhdr(xdrs, &call_msg)) {
		perror((catgets(nlmsg_fd,NL_SETN,1, "clnt_raw.c - Fatal header serialization error.")));
	}
	mcnt = XDR_GETPOS(xdrs);
	XDR_DESTROY(xdrs);

	/*
	 * Set xdrmem for client/server shared buffer
	 */
	xdrmem_create(xdrs, _raw_buf, UDPMSGSIZE, XDR_FREE);

	/*
	 * create client handle
	 */
	client->cl_ops = &client_ops;
	client->cl_auth = authnone_create();
	return (client);
}

static enum clnt_stat 
clntraw_call(h, proc, xargs, argsp, xresults, resultsp, timeout)
	CLIENT *h;
	u_long proc;
	xdrproc_t xargs;
	caddr_t argsp;
	xdrproc_t xresults;
	caddr_t resultsp;
	struct timeval timeout;
{
	register XDR *xdrs = &xdr_stream;
	struct rpc_msg msg;
	enum clnt_stat status;
	struct rpc_err error;

call_again:
	/*
	 * send request
	 */
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	((struct rpc_msg *)mashl_callmsg)->rm_xid ++ ;
	if ((! XDR_PUTBYTES(xdrs, mashl_callmsg, mcnt)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp))) {
		return (RPC_CANTENCODEARGS);
	}
	(void)XDR_GETPOS(xdrs);  /* called just to cause overhead */

	/*
	 * We have to call server input routine here because this is
	 * all going on in one process. Yuk.
	 */
	svc_getreq(1);

	/*
	 * get results
	 */
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = resultsp;
	msg.acpted_rply.ar_results.proc = xresults;
	if (! xdr_replymsg(xdrs, &msg))
		return (RPC_CANTDECODERES);
	_seterr_reply(&msg, &error);
	status = error.re_status;

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
	}  /* end successful completion */
	else {
		if (AUTH_REFRESH(h->cl_auth))
			goto call_again;
	}  /* end of unsuccessful completion */

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
		if (msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void)xdr_opaque_auth(xdrs, &(msg.acpted_rply.ar_verf));
		}
	}

	return (status);
}

static void
clntraw_geterr()
{
}


static bool_t
clntraw_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register XDR *xdrs = &xdr_stream;

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

static void
clntraw_abort()
{
}

static void
clntraw_destroy()
{
}

static bool_t
clntraw_control()
{
	return (FALSE);
}
