/* SCCSID strings for NFS/300 group */
/* %Z%%I%	[%E%  %U%] */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/auth_none.c,v $
 * $Revision: 12.3 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:07:06 $
 *
 * Revision 12.2  90/06/08  09:54:52  09:54:52  dlr
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/03/20  14:42:24  14:42:24  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:09  16:08:09  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.1  89/01/26  12:47:53  12:47:53  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.6  89/01/26  11:46:59  11:46:59  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.5  88/08/05  12:51:07  12:51:07  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.4  88/06/14  15:02:22  15:02:22  cmahon (Christina Mahon)
 * Added updates from the RPC 3.9 code inside ifdef's
 * 
 * Revision 1.1.10.3  87/08/07  14:21:29  14:21:29  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:00:48  22:00:48  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.3  86/09/25  14:15:36  14:15:36  vincent (Vincent Hsieh)
 * Included h/types.h.
 * 
 * Revision 1.2  86/07/28  11:37:08  11:37:08  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: auth_none.c,v 12.3 92/02/07 17:07:06 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * auth_none.c
 * Creates a client authentication handle for passing "null" 
 * credentials and verifiers to remote systems. 
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

#define xdr_opaque_auth 	_xdr_opaque_auth
#define xdrmem_create 		_xdrmem_create
#define marshalled_client 	_marshalled_client	/* In this file */
#define authnone_create 	_authnone_create

#endif /* _NAMESPACE_CLEAN */

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#define NULL ((caddr_t)0)
#define MAX_MARSHEL_SIZE 20

/*
 * Authenticator operations routines
 */
static void	authnone_verf();
static void	authnone_destroy();
static bool_t	authnone_marshal();
static bool_t	authnone_validate();
static bool_t	authnone_refresh();

static struct auth_ops ops = {
	authnone_verf,
	authnone_marshal,
	authnone_validate,
	authnone_refresh,
	authnone_destroy
};

static AUTH	no_client;

#ifdef _NAMESPACE_CLEAN
/*
 * The defines and initialization for marhsalled_client stay in this file
 * because we expect no one to be using marshalled_client unless they are
 * also calling the auth_none routines.
 */
#undef marshalled_client
#pragma _HP_SECONDARY_DEF _marshalled_client marshalled_client
#define marshalled_client _marshalled_client
#endif

char	marshalled_client[MAX_MARSHEL_SIZE] = "";

static u_int	mcnt;

#ifdef _NAMESPACE_CLEAN
#undef authnone_create 
#pragma _HP_SECONDARY_DEF _authnone_create authnone_create
#define authnone_create _authnone_create
#endif

AUTH *
authnone_create()
{
	XDR xdr_stream;
	register XDR *xdrs;

	if (! mcnt) {
		no_client.ah_cred = no_client.ah_verf = _null_auth;
		no_client.ah_ops = &ops;
		xdrs = &xdr_stream;
		xdrmem_create(xdrs, marshalled_client, (u_int)MAX_MARSHEL_SIZE,
		    XDR_ENCODE);
		(void)xdr_opaque_auth(xdrs, &no_client.ah_cred);
		(void)xdr_opaque_auth(xdrs, &no_client.ah_verf);
		mcnt = XDR_GETPOS(xdrs);
		XDR_DESTROY(xdrs);
	}
	return (&no_client);
}

/*ARGSUSED*/
static bool_t
authnone_marshal(client, xdrs)
	AUTH *client;
	XDR *xdrs;
{
	/* 
  	 * This is a 3.9 addition to have a meaningful return if xdrs is not
 	 * defined.
 	 */

	if (! mcnt )
		return(0);
	return ((*xdrs->x_ops->x_putbytes)(xdrs, marshalled_client, mcnt));
}

static void 
authnone_verf()
{
}

static bool_t
authnone_validate()
{

	return (TRUE);
}

static bool_t
authnone_refresh()
{

	return (FALSE);
}

static void
authnone_destroy()
{
}
