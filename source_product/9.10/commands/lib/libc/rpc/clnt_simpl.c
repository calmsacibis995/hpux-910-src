/* SCCSID strings for NFS/300 group */
/* %Z%%I%	[%E%  %U%] */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/clnt_simpl.c,v $
 * $Revision: 12.3 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:07:53 $
 *
 * Revision 12.2  90/05/15  11:07:38  11:07:38  dlr
 * Fixed problem with address cylcing when only one internet address is returned
 * by gethostbyname() and the corresponding interface is not active.
 * 
 * Revision 12.1  90/03/20  14:47:55  14:47:55  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:30  16:08:30  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.4  89/04/13  16:24:07  16:24:07  prabha (Prabha Chadayammuri)
 * added bind changes
 * 
 * Revision 1.1.10.9  89/04/13  15:23:14  15:23:14  cmahon (Christina Mahon)
 * added bind changes
 * 
 * Revision 1.1.10.8  89/02/13  14:03:26  14:03:26  cmahon (Christina Mahon)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.7  89/01/26  11:55:23  11:55:23  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.6  89/01/16  15:10:34  15:10:34  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:22  14:48:22  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.2  88/07/15  12:01:30  12:01:30  chm (Cristina H. Mahon)
 * Changed the name of the global variable socket to sock since now socket.h
 * defines the call socket.
 * 
 * Revision 1.1.10.5  88/07/15  11:00:51  11:00:51  cmahon (Christina Mahon)
 * Changed the name of the global variable socket to sock since now socket.h
 * defines the call socket.
 * 
 * Revision 1.1.10.4  87/08/07  14:41:27  14:41:27  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/07/16  22:08:18  22:08:18  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.3  86/07/28  15:10:32  15:10:32  dacosta (Herve' Da Costa)
 * Noted name change from clnt_simple.c to clnt_simpl.c (12 chars)  D. Pan
 * 
 * Revision 1.2  86/07/28  14:49:24  14:49:24  dacosta (Herve' Da Costa)
 * Header added.  
 * 
 * $Endlog$
 */

/* USED TO BE CALLED clnt_simple.c,  D. Pan 7/25/86	*/

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_simpl.c,v 12.3 92/02/07 17:07:53 indnetwk Exp $ (Hewlett-Packard)";
#endif

/* 
 * clnt_simpl.c
 * Simplified front end to rpc.
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

#define callrpc 		_callrpc		/* In this file */
#define clntudp_create 		_clntudp_create
#define close 			_close
#define gethostbyname 		_gethostbyname
#define memcpy 	        	_memcpy
#define rpc_createerr 		_rpc_createerr
#define strcmp 			_strcmp
#define strcpy 			_strcpy

#ifdef _ANSIC_CLEAN
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAME_SPACE_CLEAN */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
/* HPNFS Changed sys/time.h to time.h because include file changed */
#include <time.h>
#include <netdb.h>
#include <arpa/trace.h>

char *malloc();
static CLIENT *client;
static int sock;
static int oldprognum, oldversnum, valid;
static char *oldhost;

#ifdef _NAMESPACE_CLEAN
#undef callrpc
#pragma _HP_SECONDARY_DEF _callrpc callrpc
#define callrpc _callrpc
#endif

callrpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char	*host;
	int	prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char	*in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout;

	TRACE2("callrpc SOP, oldhost=0x%x", oldhost);
	TRACE5("callrpc  host=%s, prog=%ld, vers=%ld, proc=%ld",
			 host,    prognum,  versnum,  procnum);
	TRACE2("callrpc  in=%s", in);
	TRACE2("callrpc out=%s", out);
	if (oldhost == NULL) {
		oldhost = malloc(256);
		oldhost[0] = 0;
		sock = RPC_ANYSOCK;
	}
	if (valid && oldprognum == prognum && oldversnum == versnum
		&& strcmp(oldhost, host) == 0) {
		TRACE("callrpc reusing old client");
		/* reuse old client */		
	} else {
		TRACE("callrpc need new client");
		(void) close(sock);
		if (client) {
			TRACE("callrpc destroying old client");
			clnt_destroy(client);
			client = NULL;
		}
		if ((hp = gethostbyname(host)) == NULL) {
			TRACE2("callrpc no /etc/hosts entry for %s", host);
			return ((int) RPC_UNKNOWNHOST);
		}
next_ipaddr:
		valid = 0;
		sock = RPC_ANYSOCK;
		timeout.tv_usec = 0;
		timeout.tv_sec = 5;
		memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;

		TRACE("callrpc about to clntudp_create our new client");
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &sock)) == NULL) {
			TRACE2("callrpc clntudp_create failed, stat=%d",
						(int) rpc_createerr.cf_stat);
			if ( (rpc_createerr.cf_stat == RPC_PMAPFAILURE) && 
			                      hp && hp->h_addr_list[1]) {
				hp->h_addr_list++;
				goto next_ipaddr;
			} else
				return ((int) rpc_createerr.cf_stat);
		}
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		strcpy(oldhost, host);
		TRACE("callrpc new client structure is set up");
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	TRACE("callrpc about to clnt_call");
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    			      outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	TRACE3("callrpc clnt_call returned %d, valid=%d", clnt_stat, valid);
	return ((int) clnt_stat);
}
