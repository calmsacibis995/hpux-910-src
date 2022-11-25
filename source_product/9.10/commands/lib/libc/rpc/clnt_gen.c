/* @(#)$Revision: 12.4.1.1 $	$Date: 94/10/04 10:47:44 $	 clnt_generic.c	*/
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_generic.c 1.4 87/08/11 (C) 1987 SMI";
#endif
/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define clnt_create		_clnt_create		/* In this file */
#define clnttcp_create  	_clnttcp_create
#define clntudp_create  	_clntudp_create
#define gethostbyname   	_gethostbyname
#define getprotobyname  	_getprotobyname
#define memcpy			_memcpy
#define memset			_memset
#define rpc_createerr		_rpc_createerr

#endif /* _NAME_SPACE_CLEAN */

#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <netdb.h>
#include <stdio.h>   /* Just to get NULL defined */

/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can 
 * change using the rpc equivalent of ioctl()'s.
 */

#ifdef _NAMESPACE_CLEAN
#undef clnt_create
#pragma _HP_SECONDARY_DEF _clnt_create clnt_create
#define clnt_create _clnt_create
#endif

CLIENT *
clnt_create(hostname, prog, vers, proto)
	char *hostname;
	unsigned prog;
	unsigned vers;
	char *proto;
{
	struct hostent *h;
	struct protoent *p;
	struct sockaddr_in sin;
	int sock, try_next_ipaddr = 0;
	struct timeval tv;
	CLIENT *client;

	/* Change to fix a bug whereby this wasn't getting reset to 0 */
	rpc_createerr.cf_stat = 0;

	h = gethostbyname(hostname);
	if (h == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
		return (NULL);
	}
	if (h->h_addrtype != AF_INET) {
		/*
		 * Only support INET for now
		 */
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = EAFNOSUPPORT; 
		return (NULL);
	}

	p = getprotobyname(proto);
	if (p == NULL) {
		rpc_createerr.cf_stat 		= RPC_UNKNOWNPROTO;
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT; 
		return (NULL);
	}
next:
	sin.sin_family = h->h_addrtype;
	memset(sin.sin_zero, 0, sizeof(sin.sin_zero));
	sin.sin_port = 0;
	memcpy((char*)&sin.sin_addr, h->h_addr,  h->h_length);

	sock = RPC_ANYSOCK;
	switch (p->p_proto) {
	case IPPROTO_UDP:
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		try_next_ipaddr = 0;
		client = clntudp_create(&sin, prog, vers, tv, &sock);

		if (client == NULL) {
		   if (rpc_createerr.cf_stat == RPC_PMAPFAILURE)
                       {
			try_next_ipaddr = 1; /* try the next ip address; we may
				    	   * have to look for the actual error
					   * condition but for the time being,
					   * this is enough.
					   */
                      break;
                      }
		   else 
			return (NULL);
		}
		tv.tv_sec = 25;
		clnt_control(client, CLSET_TIMEOUT, &tv);
		break;
	case IPPROTO_TCP:
		client = clnttcp_create(&sin, prog, vers, &sock, 0, 0);
		if (client == NULL) {
		  if (rpc_createerr.cf_stat == RPC_SYSTEMERROR) {
		     if ((rpc_createerr.cf_error.re_errno == ENETUNREACH) ||
		         (rpc_createerr.cf_error.re_errno == EHOSTUNREACH))
			  try_next_ipaddr = 1; /* try the next ip address */
		  } else 
			return (NULL);
		} else {
			try_next_ipaddr   = 0;
			tv.tv_sec  = 25;
			tv.tv_usec = 0;
			clnt_control(client, CLSET_TIMEOUT, &tv);
		}
		break;
	default:
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT; 
		return (NULL);
	}
		if (try_next_ipaddr) {
		   if (h && h->h_addr_list[1]) {
		    	    h->h_addr_list++;
			    goto next;
			}
		}
	return (client);
}

