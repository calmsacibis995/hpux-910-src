#ifdef MODULE_ID
/*
 * @(#)rpc.h: $Revision: 1.6.83.3 $	$Date: 93/09/17 19:27:29 $
 * $Locker:  $
 */
#endif /* MODULE_ID */
/*
 * REVISION: @(#)10.1
 */

/*
 * rpc.h, Just includes the billions of rpc header files necessary to 
 * do remote procedure calling.
 *
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */

#include <rpc/types.h>		/* some typedefs */

#ifndef IPPROTO_ICMP		/* if netinet/in.h has already been included */
#include <netinet/in.h>		/* don't include it again.	*/
#endif /* IPPROTO_ICMP */

/* external data representation interfaces */
#include <rpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <rpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <rpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_unix.h>	/* protocol for unix style cred */

/* Server side only remote procedure callee */
#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */
