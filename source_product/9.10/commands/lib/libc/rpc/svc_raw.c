/* SCCSID strings for NFS/300 group */
/* %Z%%I%	[%E%  %U%] */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_raw.c,v $
 * $Revision: 12.2 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:24 $
 *
 * Revision 12.1  90/03/21  10:43:09  10:43:09  dlr
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:09:18  16:09:18  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.1  89/01/26  13:11:22  13:11:22  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.5  89/01/26  12:10:33  12:10:33  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.4  87/08/07  15:05:19  15:05:19  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  11:35:25  11:35:25  cmahon (Christina Mahon)
 * Merge with 300 version.  Added call to xprt_register() to make it work at all.
 * 
 * Revision 1.1.10.2  87/07/16  22:24:19  22:24:19  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:43:57  11:43:57  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_raw.c,v 12.2 92/02/07 17:08:24 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * svc_raw.c,   This a toy for simple testing and timing.
 * Interface to create an rpc client and server in the same UNIX process.
 * This lets us similate rpc and get rpc (round trip) overhead, without
 * any interference from the kernal.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define svcraw_create 		_svcraw_create		/* In this file */
#define xdr_callmsg 		_xdr_callmsg
#define xdr_replymsg 		_xdr_replymsg
#define xdrmem_create 		_xdrmem_create
#define xprt_register 		_xprt_register

#endif /* _NAMESPACE_CLEAN */

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>

#define NULL ((caddr_t)0)

/*
 * This is the "network" that we will be moving data over
 */
extern char _raw_buf[UDPMSGSIZE];

static bool_t		svcraw_recv();
static enum xprt_stat 	svcraw_stat();
static bool_t		svcraw_getargs();
static bool_t		svcraw_reply();
static bool_t		svcraw_freeargs();
static void		svcraw_destroy();

static struct xp_ops server_ops = {
	svcraw_recv,
	svcraw_stat,
	svcraw_getargs,
	svcraw_reply,
	svcraw_freeargs,
	svcraw_destroy
};

static SVCXPRT server;
static XDR xdr_stream;
static char verf_body[MAX_AUTH_BYTES];

#ifdef _NAMESPACE_CLEAN
#undef svcraw_create
#pragma _HP_SECONDARY_DEF _svcraw_create svcraw_create
#define svcraw_create _svcraw_create
#endif

SVCXPRT *
svcraw_create()
{

	server.xp_sock = 0;
	server.xp_port = 0;
	server.xp_ops = &server_ops;
	server.xp_verf.oa_base = verf_body;
	xdrmem_create(&xdr_stream, _raw_buf, UDPMSGSIZE, XDR_FREE);

	/*	HPNFS	jad	87.08.06
	**	Nasty bug from Sun -- they forgot to xprt_register the
	**	New SVCXPRT, so there way NO WAY raw RPC could work ...
	*/
	xprt_register(&server);

	return (&server);
}

static enum xprt_stat
svcraw_stat()
{

	return (XPRT_IDLE);
}

static bool_t
svcraw_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register XDR *xdrs = &xdr_stream;

	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
	       return (FALSE);
	return (TRUE);
}

static bool_t
svcraw_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register XDR *xdrs = &xdr_stream;

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_replymsg(xdrs, msg))
	       return (FALSE);
	(void)XDR_GETPOS(xdrs);  /* called just for overhead */
	return (TRUE);
}

static bool_t
svcraw_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&xdr_stream, args_ptr));
}

static bool_t
svcraw_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{ 
	register XDR *xdrs = &xdr_stream;

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
} 

static void
svcraw_destroy()
{
}
