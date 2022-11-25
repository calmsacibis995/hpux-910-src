/* @(#)rpc.lockd:	$Revision: 1.6.109.4 $	$Date: 93/12/20 11:09:26 $
*/

#ifdef PATCH_STRING
static char *patch_3453="@(#) PATCH_9.0: pmap.o $Revision: 1.6.109.4 $ 93/12/20 PHNE_3453";
#endif

/* pmap.c -- Support functions for calling the portmapper to get a port.
 * These routines were prompted by the addition of "bind" (named) support,
 * but they really needed to be done anyway.  The problem was that
 * call_udp() and call_tcp() just did clnt*_create()s, which could block for
 * up to a minute while the functions tried to contact the portmapper on
 * the remote system.  With bind, this became up to a minute for EACH
 * address of the remote system returned by gethostbyname!  Thus, it was
 * desirable to somehow do this "in the background" while still servicing
 * other requests.  The solution implemented in these functions is to
 * send out a request to the portmapper, BUT DONT WAIT FOR A RESPONSE.
 * Instead, a special version of svc_run (pmap_svc_run) is used which
 * listens for lockd requests AND possible responses for portmap calls.
 * If a portmap response comes in, then we look up the needy cache entry
 * and then run the queue of waiting requests.
 */

/* ************************************************************** */
/* NOTE: prot_alloc.c, prot_libr.c, prot_lock.c, prot_main.c,     */
/* prot_msg.c, prot_pklm.c, prot_pnlm.c, prot_priv.c, prot_proc.c,*/
/* sm_monitor.c, svc_udp.c, tcp.c, udp.c, AND pmap.c share        */
/* a single message catalog (lockd.cat).  The last three files    */
/* pmap.c, tcp.c, udp.c have messages BOTH in lockd.cat AND       */
/* statd.cat.  For that reason we have allocated message ranges   */
/* for each one of the files.  If you need more than 10 messages  */
/* in this file check the  message numbers used by the other files*/
/* listed above in the NLS catalogs.                              */
/* ************************************************************** */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include "prot_lock.h"
#include "cache.h"
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/types.h>

int pmap_sock = RPC_ANYSOCK;
extern int debug;
extern fd_set svc_fdset;

#define BUFSIZE 512

static struct rpc_msg call_msg;		/* RPC header structure */
static char outbuf[BUFSIZE], inbuf[BUFSIZE];  /* room for a portmap request */
static int addrlen = sizeof ( struct sockaddr_in );

enum clnt_stat getport_wait();

/*
 * Initialize the call_msg structure to all the appropriate values for calling
 * portmap.  The only thing that will need to be filled in later will be the
 * xid for each call.
 */

void
init_call_msg()
{
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = PMAPPROG;
	call_msg.rm_call.cb_vers = PMAPVERS;
	call_msg.rm_call.cb_proc = PMAPPROC_GETPORT;
	call_msg.rm_call.cb_cred.oa_flavor = AUTH_NULL;
	call_msg.rm_call.cb_cred.oa_base = (caddr_t) NULL;
	call_msg.rm_call.cb_cred.oa_length = 0;
	call_msg.rm_call.cb_verf.oa_flavor = AUTH_NULL;
	call_msg.rm_call.cb_verf.oa_base = (caddr_t) NULL;
	call_msg.rm_call.cb_verf.oa_length = 0;
}

/*
 * getport() -- Call the portmapper and get the port# for the requested
 * program, etc., trying all the possible addresses for the named host
 * If timeout is non-zero, then we assume that a response will be waited
 * for, and therefore call the regular pmap_getport() function with the 
 * possible addresses.  If timeout is zero, then we just send out the
 * request and don't wait for it.  Unfortunately, to do this requires our
 * own version of the pmap() function, both so we can send it with zero
 * timeout and so that we can save the transaction ID to map the response
 * to the appropriate cache entry.  Note, most counters/timers are 
 * initialized in add_hash().
 */

enum clnt_stat
getport( cp, prog, vers, proto, timeout)
struct cache *cp;
u_int prog, vers, proto;
int timeout;
{
	u_short port;
	struct sockaddr_in server_addr;
	struct hostent *hp = cp->hp;
	struct timeval now;
	static bool_t firsttime = TRUE;
	static int XID;

	(void) gettimeofday( &now, 0);
	
	if ( firsttime ) {
		XID = getpid() ^ now.tv_sec ^ now.tv_usec;
		init_socket( &pmap_sock );
		init_call_msg();
		firsttime = FALSE;
	}

	/*
	 * Wait for a response?  Then use regular portmap routine, 
	 * trying all possible addresses.
 	 */
	if ( timeout != 0 )
		return(getport_wait(cp, prog, vers, proto));
	/*
	 * Timeout == 0 means don't wait for a response.  Therefore we
	 * format a packet and send it out.  However, we have to keep track
	 * of where we are and possibly try a new address.  If we have tried
	 * for MAXWAITTIME seconds with this address and not got a response,
	 * try the next address if there is one.  On the other hand, if we
	 * just sent a packet in the last MINWAITTIME seconds, just time
	 * out, preventing us from flooding the network with pmap packets.
	 */
	if ( (now.tv_sec - cp->paddrtime.tv_sec) >= MAXWAITTIME ) {
		if (!cp->firsttime)
		    cp->h_index++;
		if ( hp->h_addr_list[cp->h_index] == NULL ) {
			rpc_createerr.cf_stat = RPC_PMAPFAILURE;
			return (RPC_PMAPFAILURE);
		}
		XID++;
		if ( XID == 0 )
			XID++;
		cp->xid = XID;
		cp->paddrtime = now;
		cp->firsttime = FALSE;
	}
	else if ( (now.tv_sec - cp->psendtime.tv_sec) < MINWAITTIME ) {
		return ( RPC_TIMEDOUT);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	memcpy(&server_addr.sin_addr,hp->h_addr_list[cp->h_index],hp->h_length);

	return ( sendto_pmap(cp->xid, &server_addr, prog, vers, proto) );
	
}

/*
 * getport_wait() -- Calls the regular pmap_getport() routine which waits
 * for a response, trying all possible address 
 */

enum clnt_stat
getport_wait( cp, prog, vers, proto)
struct cache *cp;
int prog, vers, proto;
{
	u_short port;
	int index;
	struct sockaddr_in server_addr;
	struct hostent *hp = cp->hp;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	for ( index = 0 ; hp->h_addr_list[index] != NULL ; index++ ) {

	    memcpy(&server_addr.sin_addr, hp->h_addr_list[index], hp->h_length);
	    if ((port = pmap_getport( &server_addr, prog, vers, proto)) != 0 ){
			/* found the port! save information */
			server_addr.sin_port = port;
			cp->server_addr = server_addr;
			cp->xid = 0;
			(void) gettimeofday(&cp->addrtime, 0 );
			return (RPC_SUCCESS);
	    }
	    else if (rpc_createerr.cf_stat != RPC_PMAPFAILURE ) {
		    return ( rpc_createerr.cf_stat );
	    }
	}
		
	/* If got here, we got RPC_PMAPFAILURE on all addresses */
	return ( RPC_PMAPFAILURE );
}

/*
 * sendto_pmap() -- format up the information into an RPC/XDR packet and
 * send it out.  This does essentially a combination of the first halves
 * of pmap_getport and clntudp_call(), without waiting for a response.
 */

sendto_pmap( xid, addr, prog, vers, proto )
int xid;
struct sockaddr_in *addr;
u_int prog, vers, proto;
{
	struct pmap parms;
	XDR xdrs;
	int outlen;

	addr->sin_port = htons(PMAPPORT);
	parms.pm_prog = prog;
	parms.pm_vers = vers;
	parms.pm_prot = proto;
	parms.pm_port = 0;
	call_msg.rm_xid = xid;

	xdrmem_create( &xdrs, outbuf, BUFSIZE, XDR_ENCODE );

	if ( ! xdr_callmsg( &xdrs, &call_msg) || !xdr_pmap( &xdrs, &parms) ) {
		rpc_createerr.cf_stat = RPC_PMAPFAILURE;
		rpc_createerr.cf_error.re_status = RPC_CANTENCODEARGS;
		return ( RPC_PMAPFAILURE );
	}
	outlen = (int)XDR_GETPOS(&xdrs);

	if ( sendto( pmap_sock, outbuf, outlen, 0, addr, addrlen ) != outlen ) {
                rpc_createerr.cf_stat = RPC_PMAPFAILURE;
                rpc_createerr.cf_error.re_status = RPC_CANTSEND;
                return ( RPC_PMAPFAILURE );
	}

	return ( RPC_TIMEDOUT );
}

/*
 * recv_from_pmap() -- called from pmap_svc_run() to handle a response on
 * the socket we used to talk to the portmapper.  It's purpose in life is
 * to decode the incoming packet, and check the cache for a matching XID.
 * if we find a match in the cache, then this means that a request that was
 * waiting for a portmap response was blocked, so we call the function
 * provided to run the queue trying to get a response.  For lockd, this
 * function would normally be xtimer(), and for statd would be sm_try()
 * Essentially this function performs the last half of pmap_getport() and
 * clntudp_call().
 */

void
recv_from_pmap( run_queue )
void (*run_queue)();
{
	int inlen;	
	struct sockaddr_in from;
	int fromlen = sizeof (from);
	extern int errno;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	int port;
	struct cache *cp, *find_hash_xid();
	struct timeval now;
	struct rpc_err rpcerr;
	int oldmask;

	oldmask = sigblock( 1 << (SIGALRM -1));

tryagain:
	if ((inlen = recvfrom(pmap_sock, inbuf,BUFSIZE,0,&from,&fromlen)) < 0){
		if ( errno == EINTR ) 
			goto tryagain;
		if ( errno != EWOULDBLOCK )
			log_perror((catgets(nlmsg_fd,NL_SETN,1000,"Error in recvfrom on socket used for portmap requests")));
		sigsetmask(oldmask);
		return;
	}
	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = (caddr_t) &port;
	reply_msg.acpted_rply.ar_results.proc = (xdrproc_t) xdr_long;

	xdrmem_create( &reply_xdrs, inbuf, inlen, XDR_DECODE);
	
	if ( !xdr_replymsg( &reply_xdrs, &reply_msg) ) {
		/* Decoding error, nothing we can do but log message */
		logclnt_perrno( RPC_CANTDECODERES );
		sigsetmask(oldmask);
		return;
	}
	
	_seterr_reply( &reply_msg, &rpcerr);
	if ( rpcerr.re_status != RPC_SUCCESS ) {
		logclnt_perrno( rpcerr.re_status );
		sigsetmask(oldmask);
		return;
	}

	/*
	 * At this point we know we have a valid message and the port should
	 * be what was returned from the remote portmapper.  Note that even
	 * if the portmapper returned 0, indicating that lockd was not
	 * registered, we still want to run the queue so that we can force
	 * an RPC_PROGNOTREGISTERED to return and abort the request.
	 * However, if find_hash_xid() returns NULL, that indicates that
	 * no one in the cache was waiting for a reply with that XID, so
	 * it must be an old response and we drop the packet. 
	 */

	if ( (cp = find_hash_xid(reply_msg.rm_xid)) == (struct cache *)NULL) {
		if ( debug )
			logmsg((catgets(nlmsg_fd,NL_SETN,1001,"pmap_recvfrom(): xid (%d) not found")), reply_msg.rm_xid);
		sigsetmask(oldmask);
		return;
	}
	
	/* NULL xid so no further responses will be processed */
	cp->xid = 0;
	cp->server_addr.sin_family = AF_INET;
	cp->server_addr.sin_port = port;
	memcpy(&cp->server_addr.sin_addr, cp->hp->h_addr_list[cp->h_index], 
			cp->hp->h_length);

	/* Save time that we got the address so we can know when to time out */
	(void) gettimeofday( &cp->addrtime, 0);

	cp->port_state = PORT_VALID_FIRST;

	if ( debug > 2 )
		logmsg((catgets(nlmsg_fd,NL_SETN,1002,"Port = %1$d, prog = %2$d, host = %3$s")),
			port, cp->prognum, cp->host);

	/* Go run the queue */
	(*run_queue)(cp->host);

	sigsetmask(oldmask);
	return;
}

/*
 * pmap_svc_run() -- similar to svc_run(), except we also select on the
 * socket we use to talk to other portmappers.  This allows us to multiplex
 * between requests to the remote systems portmappers, and servicing 
 * requests ourselves.
 */

pmap_svc_run( run_queue )
void (*run_queue)();
{
	fd_set readfds;
	register fd_set_len, or_mask, *mask_ptr;
        int num_fds;

	while (TRUE) {
		mask_ptr = svc_fdset.fds_bits;
		or_mask = 0;
		fd_set_len = howmany(FD_SETSIZE, NFDBITS);

		do {
			or_mask |= *(mask_ptr++);
		} while (--fd_set_len);

		if (or_mask == 0)
			return;
		memcpy(&readfds,&svc_fdset,sizeof(fd_set));
		if ( pmap_sock != RPC_ANYSOCK ) {
			FD_SET(pmap_sock, &readfds );
		}
		switch (select(FD_SETSIZE, &readfds, (int *)0, (int *)0,
		    (struct timeval *)0)) {

		case -1:
			if (errno == EINTR)
				continue;
			else {
				log_perror((catgets(nlmsg_fd,NL_SETN,1003,"pmap_svc_run(): Select failed")));
				return;
			}
		case 0:
			continue;
		default:
			if ( pmap_sock != RPC_ANYSOCK &&
			     FD_ISSET( pmap_sock, &readfds ) ) 
				recv_from_pmap( run_queue);
			else
                        {
                                num_fds = getnumfds();
				svc_getreqset_ms(&readfds,num_fds);
                        }
		}
	}
}
