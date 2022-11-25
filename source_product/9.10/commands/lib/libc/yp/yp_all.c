/*	@(#)yp_all.c	$Revision: 12.1.1.1 $	$Date: 94/10/04 10:41:55 $  */
/*
yp_all.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "yp_all.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

#define NULL 0

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define clnt_pcreateerror 	_clnt_pcreateerror
#define clnt_perror 		_clnt_perror
#define clnttcp_create 		_clnttcp_create
#define close 			_close
#define strlen 			_strlen
#define xdr_ypall 		_xdr_ypall
#define xdr_ypreq_nokey 	_xdr_ypreq_nokey
#define yp_all 			_yp_all			/* In this file */
#define yp_unbind 		_yp_unbind

#endif /* _NAMESPACE_CLEAN */

#define NL_SETN 21	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <time.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>


static struct timeval tcp_timout = {
	120,				/* 120 seconds */
	0
	};
extern int _yp_dobind();
extern unsigned int _ypsleeptime;
extern char *malloc();

/*
 * This does the "glommed enumeration" stuff.  callback->foreach is the name
 * of a function which gets called per decoded key-value pair:
 * 
 * (*callback->foreach)(status, key, keylen, val, vallen, callback->data);
 *
 * If the server we get back from _yp_dobind speaks the old protocol, this
 * returns YPERR_VERS, and does not attempt to emulate the new functionality
 * by using the old protocol.
 */

#ifdef _NAMESPACE_CLEAN
#undef yp_all
#pragma _HP_SECONDARY_DEF _yp_all yp_all
#define yp_all _yp_all
#endif

int
yp_all (domain, map, callback)
	char *domain;
	char *map;
	struct ypall_callback *callback;
{
	int domlen;
	int maplen;
	struct ypreq_nokey req;
	int reason;
	struct dom_binding *pdomb;
	enum clnt_stat s;
	struct sockaddr_in dom_server_addr;
	long int yp_version;
	CLIENT *tcp_client;
	int socket_fd;

	nlmsg_fd = _nfs_nls_catopen();

	if ( (map == NULL) || (domain == NULL) ) {
		return(YPERR_BADARGS);
	}
	
	domlen = strlen(domain);
	maplen = strlen(map);
	
	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (callback == (struct ypall_callback *) NULL) ) {
		return(YPERR_BADARGS);
	}

	if (reason = _yp_dobind(domain, &pdomb) ) {
		return(reason);
	}

	if (pdomb->dom_vers == YPOLDVERS) {
		return (YPERR_VERS);
	}
		
	/*
	 * Now, save the server address from the domain binding structure
	 * so we can use it to open a TCP port.
	 */

	dom_server_addr = pdomb->dom_server_addr;
	dom_server_addr.sin_port = 0;
	yp_version = pdomb->dom_vers;

	yp_unbind(domain); /* We don't need to be bound anymore */

	socket_fd = RPC_ANYSOCK;
	
	if ((tcp_client = clnttcp_create(&dom_server_addr,
	    YPPROG, YPVERS, &socket_fd, 0, 0)) ==
	    (CLIENT *) NULL) {
		    clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,1, "yp_all:  TCP channel create failure")));
		    return(YPERR_RPC);
	}

	req.domain = domain;
	req.map = map;
	
	s = clnt_call(tcp_client, YPPROC_ALL, xdr_ypreq_nokey, &req,
	    xdr_ypall, callback, tcp_timout);

	/* At this point, we need to clean up our TCP connection */
	clnt_destroy(tcp_client);

	if (s != RPC_SUCCESS) {
		clnt_perror(pdomb->dom_client,
		    (catgets(nlmsg_fd,NL_SETN,2, "yp_all:  RPC clnt_call (TCP) failure")));
	}

	if (s == RPC_SUCCESS) {
		return(0);
	} else {
		return(YPERR_RPC);
	}
}

