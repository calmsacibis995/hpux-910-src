/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.2 $	$Date: 92/02/07 17:08:13 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/pmap_port.c,v $
 * $Revision: 12.2 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:13 $
 *
 * Revision 12.1  90/03/20  16:12:30  16:12:30  dlr
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:49  16:08:49  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  13:02:42  13:02:42  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.6  89/01/26  12:01:36  12:01:36  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.5  89/01/16  15:16:21  15:16:21  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:47  14:48:47  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.1  88/02/29  13:52:40  13:52:40  nfsmgr (Nfsmanager)
 * Starting revision level for 6.2 and 3.0.
 * 
 * Revision 10.1  88/02/29  13:35:28  13:35:28  nfsmgr (Nfsmanager)
 * Starting point for 6.2 and 3.0.
 * 
 * Revision 8.2  88/02/04  10:34:41  10:34:41  chm (Cristina H. Mahon)
 * Changed SCCS strings to RCS revision and date strings.
 * 
 * Revision 8.1  87/11/30  13:54:46  13:54:46  nfsmgr (Nfsmanager)
 * revision levels for ic4b
 * 
 * Revision 1.1.10.4  87/08/07  14:54:05  14:54:05  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/07/16  22:12:36  22:12:36  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.3  86/07/28  15:13:28  15:13:28  dacosta (Herve' Da Costa)
 * Noted name change: pmap_getport.c to pmap_port.c (12 chars)  D. Pan
 * 
 * Revision 1.2  86/07/28  11:40:46  11:40:46  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

/* USED TO BE CALLED pmap_getport.c	D. Pan 7/25  */

#if	defined(MODULEID) && !defined(lint)
#endif

/*
 * pmap_port.c
 * Client interface to pmap rpc service.
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

#define clntudp_bufcreate 	_clntudp_bufcreate
#define close 			_close
#define pmap_getport 		_pmap_getport		/* In this file */
#define rpc_createerr 		_rpc_createerr
#define xdr_pmap 		_xdr_pmap
#define xdr_u_short 		_xdr_u_short

#endif /* _NAME_SPACE_CLEAN */

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_prot.h> 
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <arpa/trace.h>

#define NAMELEN 255

static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };

/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 */

#ifdef _NAMESPACE_CLEAN
#undef pmap_getport
#pragma _HP_SECONDARY_DEF _pmap_getport pmap_getport
#define pmap_getport _pmap_getport
#endif

u_short
pmap_getport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	u_short port = 0;
	int socket = -1;
	register CLIENT *client;
	struct pmap parms;

	TRACE("pmap_getport SOP");
	address->sin_port = htons(PMAPPORT);
	TRACE2("pmap_getport sin_port = %d", address->sin_port);
	TRACE4("pmap_getport arguments: program = %d, version = %d, protocol = %d", program, version, protocol);
	client = clntudp_bufcreate(address, PMAPPROG,
	    PMAPVERS, timeout, &socket,  RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	TRACE2("pmap_getport clntudp_bufcreate returns 0x%x", client);
	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		TRACE("pmap_getport about to call CLNT_CALL");
		if (CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, &parms,
		    xdr_u_short, &port, tottimeout) != RPC_SUCCESS){
			rpc_createerr.cf_stat = RPC_PMAPFAILURE;
			TRACE("pmap_getport clnt_call PMAPFAILURE");
			clnt_geterr(client, &rpc_createerr.cf_error);
		} else if (port == 0) {
			TRACE("pmap_getport port = 0, PROGNOTREGISTERED");
			rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
		}
		TRACE("pmap_getport about to clnt_destroy");
		CLNT_DESTROY(client);
	}
	(void)close(socket);
	address->sin_port = 0;
	TRACE2("pmap_getport closed socket, about to return %d", port);
	return (port);
}
