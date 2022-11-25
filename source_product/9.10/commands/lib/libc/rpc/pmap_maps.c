/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.3 $	$Date: 92/02/07 17:08:10 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/pmap_maps.c,v $
 * $Revision: 12.3 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:10 $
 *
 * Revision 12.2  90/08/30  11:57:49  11:57:49  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.1  90/03/20  16:08:42  16:08:42  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:47  16:08:47  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  13:01:34  13:01:34  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.7  89/01/26  12:00:29  12:00:29  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.6  89/01/16  15:15:41  15:15:41  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:45  14:48:45  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.2  88/03/24  14:55:16  14:55:16  chm (Cristina H. Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS
 * libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.5  88/03/24  16:53:56  16:53:56  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS
 * libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.4  87/08/07  14:53:08  14:53:08  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/07/16  22:12:07  22:12:07  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.3  86/07/28  15:12:44  15:12:44  dacosta (Herve' Da Costa)
 * Noted name change: pmap_getmaps.c to pmap_maps.c (12 chars)  D. Pan
 * 
 * Revision 1.2  86/07/28  11:40:33  11:40:33  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

/* USED TO BE CALLED pmap_getmaps.c	D. Pan 86/7/75	*/

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: pmap_maps.c,v 12.3 92/02/07 17:08:10 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * pmap_maps.c   Original comment = next 6 lines, beats me what they mean,
 *               this file WAS pmap_getmaps.c so it's very ambiguous.
 * pmap_getmap.c
 * Client interface to pmap rpc service.
 * contains pmap_getmaps, which is only tcp service involved
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

#define catgets 		_catgets
#define clnt_perror 		_clnt_perror
#define clnttcp_create 		_clnttcp_create
#define close 			_close
#define pmap_getmaps		_pmap_getmaps		/* In this file */
#define xdr_pmaplist 		_xdr_pmaplist
#define xdr_void 		_xdr_void

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 10	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_prot.h> 
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
/* HPNFS Changed sys/time.h to time.h because include files changed */
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#define NAMELEN 255
#define MAX_BROADCAST_SIZE 1400

extern int errno;
static struct sockaddr_in myaddress;

/*
 * Get a copy of the current port maps.
 * Calls the pmap service remotely to do get the maps.
 */

#ifdef _NAMESPACE_CLEAN
#undef pmap_getmaps
#pragma _HP_SECONDARY_DEF _pmap_getmaps pmap_getmaps
#define pmap_getmaps _pmap_getmaps
#endif

struct pmaplist *
pmap_getmaps(address)
	 struct sockaddr_in *address;
{
	struct pmaplist *head = (struct pmaplist *)NULL;
	int socket = -1;
	struct timeval minutetimeout;
	register CLIENT *client;

	nlmsg_fd = _nfs_nls_catopen();

	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	address->sin_port = htons(PMAPPORT);
	client = clnttcp_create(address, PMAPPROG,
	    PMAPVERS, &socket, 50, 500);
	if (client != (CLIENT *)NULL) {
		if (CLNT_CALL(client, PMAPPROC_DUMP, xdr_void, NULL, xdr_pmaplist,
		    &head, minutetimeout) != RPC_SUCCESS) {
			clnt_perror(client, (catgets(nlmsg_fd,NL_SETN,1, "pmap_getmaps rpc problem")));
		}
		CLNT_DESTROY(client);
	}
	(void)close(socket);
	address->sin_port = 0;
	return (head);
}
