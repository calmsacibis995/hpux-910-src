/*	@(#)util.c	$Revision: 1.19.109.1 $	$Date: 91/11/19 14:25:42 $
util.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "util.c 1.1 86/02/05 Copyr 1985 Sun Micro";
*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>
extern u_short	pmap_getport();

getrpcport(host, prognum, versnum, proto)
	char *host;
{
	struct sockaddr_in addr;
	struct hostent *hp;
	u_short port = 0;

	if ((hp = gethostbyname(host)) == NULL)
		return (0);
next_ipaddr:
	memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port =  0;
	port = pmap_getport (&addr, prognum, versnum, proto);
	if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
	    if (hp && hp->h_addr_list[1]) {
	        hp->h_addr_list++;
	        goto next_ipaddr;
	    }
	}
	return ((int)port);
}
