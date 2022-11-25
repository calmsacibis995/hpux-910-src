/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.5 $	$Date: 92/02/07 17:07:57 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/clnt_tcp.c,v $
 * $Revision: 12.5 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:07:57 $
 *
 * Revision 12.4  90/12/10  17:17:28  17:17:28  prabha
 * performance improvements.
 * 
 * Revision 12.3  90/08/30  12:03:51  12:03:51  prabha (Prabha Chadayammuri)
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.2  90/06/08  10:22:57  10:22:57  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/01/31  11:52:26  11:52:26  dlr (Dominic Ruffatto)
 * Cleaned up comments after #else and #endif lines for ANSII-C.  Also made
 * changes for >64 file descriptors.  These changes are currently ifdef'd
 * with the GT_64_FDS flag.
 * 
 * Revision 12.0  89/09/25  16:08:32  16:08:32  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  12:57:32  12:57:32  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.12  89/01/26  11:56:30  11:56:30  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.11  89/01/16  15:11:27  15:11:27  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:25  14:48:25  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.8  88/08/30  13:23:28  13:23:28  mikey (Mike Shipley)
 * Added some comments on timeout fields in cu_data
 * 
 * Revision 1.1.10.10  88/08/30  12:22:32  12:22:32  cmahon (Christina Mahon)
 * Added some comments on timeout fields in cu_data
 * 
 * Revision 1.1.10.9  88/08/05  12:52:58  12:52:58  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.8  88/06/24  13:56:23  13:56:23  cmahon (Christina Mahon)
 * Updated to 3.9 level of functionality.  New code in ifdef's
 * 
 * Revision 1.1.10.7  88/06/22  09:35:47  09:35:47  cmahon (Christina Mahon)
 * Missed part of fix for DTS CNDdm01434 (copying instead of memcpy).  So
 * added fix to that now.
 * 
 * Revision 1.1.10.6  88/06/22  08:24:17  08:24:17  cmahon (Christina Mahon)
 * Changes made to fix DTS CNDdm01434.  These changes allow select to handle
 * more than just 32 file descriptors.
 * 
 * Revision 1.1.10.5  88/03/24  16:37:04  16:37:04  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now the NLS routines are only called once
 *  This improves NIS performance on the s800.
 * 
 * Revision 1.1.10.4  87/12/16  13:22:40  13:22:40  cmahon (Christina Mahon)
 * This change is similar to the change made to clnt_udp.c at revision 7.2.
 * We made sure that file descriptors 0, 1 and 2 are not used as socket
 * descriptors.
 * 
 * Revision 1.1.10.3  87/08/07  14:43:11  14:43:11  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:08:47  22:08:47  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:39:22  11:39:22  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_tcp.c,v 12.5 92/02/07 17:07:57 indnetwk Exp $ (Hewlett-Packard)";
#endif
 
/*
 * clnt_tcp.c, Implements a TCP/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * TCP based RPC supports 'batched calls'.
 * A sequence of calls may be batched-up in a send buffer.  The rpc call
 * return immediately to the client even though the call was not necessarily
 * sent.  The batching occurs iff the results' xdr routine is NULL (0) AND
 * the rpc timeout value is zero (see clnt.h, rpc).
 *
 * Clients should NOT casually batch calls that in fact return results; that is,
 * the server side should be aware that a call is batched and not produce any
 * return message.  Batched calls that produce many result messages can
 * deadlock (netlock) the client and the server....
 *
 * Now go hang yourself.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define authnone_create 	_authnone_create
#define bindresvport 		_bindresvport
#define catgets 		_catgets
#define clnttcp_create 		_clnttcp_create		/* In this file */
#define close 			_close
#define connect 		_connect
#define fcntl 			_fcntl
#define fprintf 		_fprintf
#define getpid 			_getpid
#define gettimeofday 		_gettimeofday
#define memcmp 			_memcmp
#define memcpy 			_memcpy
#define memset 			_memset
#define pmap_getport 		_pmap_getport
#define read 			_read
#define rpc_createerr 		_rpc_createerr
#define select 			_select
#define socket 			_socket
#define write 			_write
#define xdr_callhdr 		_xdr_callhdr
#define xdr_opaque_auth 	_xdr_opaque_auth
#define xdr_replymsg 		_xdr_replymsg
#define xdr_void 		_xdr_void
#define xdrmem_create 		_xdrmem_create
#define xdrrec_create 		_xdrrec_create
#define xdrrec_endofrecord 	_xdrrec_endofrecord
#define xdrrec_skiprecord 	_xdrrec_skiprecord

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 7	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <rpc/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_clnt.h>
#include <fcntl.h>
#include <arpa/trace.h>
#include <sys/types.h>

#define MCALL_MSG_SIZE 24

char *malloc();
extern int errno;

static int	readtcp();
static int	writetcp();

static enum clnt_stat	clnttcp_call();
static void		clnttcp_abort();
static void		clnttcp_geterr();
static bool_t		clnttcp_freeres();
static bool_t		clnttcp_control();
static void		clnttcp_destroy();

static struct clnt_ops tcp_ops = {
	clnttcp_call,
	clnttcp_abort,
	clnttcp_geterr,
	clnttcp_freeres,
	clnttcp_destroy,
	clnttcp_control
};

struct ct_data {
	int		ct_sock;
	bool_t		ct_closeit;
	struct timeval	ct_wait;
	bool_t		ct_waitset;
	struct sockaddr_in  ct_addr;
	struct rpc_err	ct_error;
	char		ct_mcall[MCALL_MSG_SIZE];	/* marshalled callmsg */
	u_int		ct_mpos;			/* pos after marshal */
	XDR		ct_xdrs;
};

/*
 * Some notes on the timeout fields in the ct_data.  The reason why it
 * is confusing is that there is a ct_wait field for tcp and a cu_wait
 * and cu_total field for udp, but the purpose for the ct_wait field
 * is not similar to that of the cu_wait field.  The ct_wait field
 * is similar to the cu_total field.  So strange.
 * Anyway, I looked over the fields and got the following info for
 * tcp
 *
 * TCP-
  
  ct_wait   NOT new for 3.2
  
            JUST the tv_usec portion of cu_wait is set to 0 in clnttcp_create()
	    IF ct_waitset is not set, then ct_wait will get set to timeout
	    in clnttcp_call()
	    set by CLSET_TIMEOUT
	    retrieved by CLGET_TIMEOUT

	    controls how long to wait in a select() before timing out and
	    returning with an error



  ct_waitset  NEW for 3.2
	      
	      Set to FALSE in clnttcp_create()
	      Set to TRUE in clnttcp_control by CLSET_TIMEOUT

	      controls if ct_wait is set to timeout in clnttcp_call()
 */



/*
 * Create a client handle for a tcp/ip connection.
 * If *sockp<0, *sockp is set to a newly created TCP socket and it is
 * connected to raddr.  If *sockp non-negative then
 * raddr is ignored.  The rpc/tcp package does buffering
 * similar to stdio, so the client must pick send and receive buffer sizes,];
 * 0 => use the default.
 * If raddr->sin_port is 0, then a binder on the remote machine is
 * consulted for the right port number.
 * NB: *sockp is copied into a private area.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is set null authentication. Caller may wish to set this
 * something more useful.
 */

#ifdef _NAMESPACE_CLEAN
#undef clnttcp_create
#pragma _HP_SECONDARY_DEF _clnttcp_create clnttcp_create
#define clnttcp_create _clnttcp_create
#endif

CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *h;
	register struct ct_data *ct;
	struct timeval now;
	struct rpc_msg call_msg;

	nlmsg_fd = _nfs_nls_catopen();

	TRACE3("clnttcp_create SOP, prog=%ld, vers=%ld", prog,vers);
	h  = (CLIENT *)mem_alloc(sizeof(*h));
	if (h == NULL) {
		TRACE("clnttcp_create ran out of memory for CLIENT");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "clnttcp_create: out of memory\n")));
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}
	ct = (struct ct_data *)mem_alloc(sizeof(*ct));
	if (ct == NULL) {
		TRACE("clnttcp_create ran out of memory for ct");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "clnttcp_create: out of memory\n")));
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}

	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		TRACE("clnttcp_create no port specified, use pmap_getport");
		if ((port = pmap_getport(raddr, prog, vers, IPPROTO_TCP)) == 0) {
			TRACE("clnttcp_create pmap_getport returned 0");
			mem_free((caddr_t)ct, sizeof(struct ct_data));
			mem_free((caddr_t)h, sizeof(CLIENT));
			return ((CLIENT *)NULL);
		}
		TRACE2("clnttcp_create pmap_getport returns %d", port);
		raddr->sin_port = htons(port);
	}

	/*
	 * If no socket given, open one
	 * There was a change added with the call to bindresvport()
	 * This will fail if the caller is not su, but then lets
	 * connect() do the bind.  If the caller is su, then the
	 * bind will be done to a resvered port.
	 */
	if (*sockp < 0) {
		TRACE("clnttcp_create no sockp specified, get socket");
		*sockp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);     
		(void) bindresvport(*sockp, (struct sockaddr_in *)0);
		if ((*sockp  < 0)
		    || (connect(*sockp, (struct sockaddr *)raddr,
		    sizeof(*raddr)) < 0)) {
			TRACE2("clnttcp_create socket or connect<0, errno = %d", errno);
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			goto fooy;
		}
		/*
		 * Avoid fd's 0,1, and 2, as these may cause conflicts with
		 * calling programs. Only need to check high end (2) because
		 * we checked the low end (<0) above.
		 */
		if( *sockp <= 2 ) {
			int newfd = fcntl(*sockp, F_DUPFD, 3);

			if ( newfd < 0 ) { 
				/* We've got problems...*/
				rpc_createerr.cf_stat = RPC_SYSTEMERROR;
				rpc_createerr.cf_error.re_errno = errno;
				goto fooy;
			}
			(void) close (*sockp);
			*sockp = newfd;
		}
		ct->ct_closeit = TRUE;
	} else {
		ct->ct_closeit = FALSE;
	}

	/*
	 * Set up private data struct
	 */
	ct->ct_sock = *sockp;
	ct->ct_wait.tv_usec = 0;
	ct->ct_waitset = FALSE;
	ct->ct_addr = *raddr;

	/*
	 * Initialize call message
	 */
	(void)gettimeofday(&now, (struct timezone *)0);
	call_msg.rm_xid = getpid() ^  now.tv_sec ^ now.tv_usec;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	TRACE("clnttcp_create initializing, calling xdrmem_create");
	xdrmem_create(&(ct->ct_xdrs), ct->ct_mcall, MCALL_MSG_SIZE,
	    XDR_ENCODE);
	if (! xdr_callhdr(&(ct->ct_xdrs), &call_msg)) {
		if (ct->ct_closeit) {
			(void) close(*sockp);
		}
		TRACE("clnttcp_create xdr_callhdr failed!");
		goto fooy;
	}
	ct->ct_mpos = XDR_GETPOS(&(ct->ct_xdrs));
	XDR_DESTROY(&(ct->ct_xdrs));
	TRACE("clnttcp_create XDR_DESTROY'ed ct->ct_xdrs ...");

	/*
	 * Create a client handle which uses xdrrec for serialization
	 * and authnone for authentication.
	 */
	xdrrec_create(&(ct->ct_xdrs), sendsz, recvsz,
	    (caddr_t)ct, readtcp, writetcp);
	h->cl_ops = &tcp_ops;
	h->cl_private = (caddr_t) ct;
	h->cl_auth = authnone_create();
	TRACE2("clnttcp_create done initializing, now returning 0x%x", h);
	return (h);

fooy:
	/*
	 * Something goofed, free stuff and barf
	 */
	mem_free((caddr_t)ct, sizeof(struct ct_data));
	mem_free((caddr_t)h, sizeof(CLIENT));
	(void)close(*sockp);
	TRACE("clnttcp_create failed, now returning 0x0");
	return ((CLIENT *)NULL);
}

static enum clnt_stat
clnttcp_call(h, proc, xdr_args, args_ptr, xdr_results, results_ptr, timeout)
	register CLIENT *h;
	u_long proc;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
	xdrproc_t xdr_results;
	caddr_t results_ptr;
	struct timeval timeout;
{
	register struct ct_data *ct = (struct ct_data *) h->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);
	struct rpc_msg reply_msg;
	u_long x_id;
	u_long *msg_x_id = (u_long *)(ct->ct_mcall);	/* yuk */
	register bool_t shipnow;
	int refreshes = 2;

	TRACE("clnttcp_call SOP");
	if (!ct->ct_waitset)
	{
	ct->ct_wait = timeout;
	}
	shipnow =
	    (xdr_results == (xdrproc_t)0 && timeout.tv_sec == 0
	    && timeout.tv_usec == 0) ? FALSE : TRUE;

call_again:
	TRACE2("clnttcp_call shipnow=%d, entering call_again loop", shipnow);
	xdrs->x_op = XDR_ENCODE;
	ct->ct_error.re_status = RPC_SUCCESS;
	x_id = ntohl(--(*msg_x_id));
	if ((! XDR_PUTBYTES(xdrs, ct->ct_mcall, ct->ct_mpos)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, args_ptr))) {
		if (ct->ct_error.re_status == RPC_SUCCESS)
			ct->ct_error.re_status = RPC_CANTENCODEARGS;
		(void)xdrrec_endofrecord(xdrs, TRUE);
		TRACE2("clnttcp_call cantencode, returning %d",
				ct->ct_error.re_status);
		return (ct->ct_error.re_status);
	}
	TRACE("clnttcp_call encoding arguments worked OK");
	if (! xdrrec_endofrecord(xdrs, shipnow)) {
		TRACE("clnttcp_call xdrrec_eor failed, return CANTSENT");
		return (ct->ct_error.re_status = RPC_CANTSEND);
	}
	if (! shipnow) {
		TRACE("clnttcp_call shipnow == 0, return SUCCESS");
		return (RPC_SUCCESS);
	}
        /*
         * Hack to provide rpc-based message passing
         */
        if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
                return(ct->ct_error.re_status = RPC_TIMEDOUT);
        }

	xdrs->x_op = XDR_DECODE;
	TRACE("clnttcp_call shipnow != 0, set up for XDR_DECODE");

	/*
	 * Keep receiving until we get a valid transaction id
	 */
	while (TRUE) {
		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = NULL;
		reply_msg.acpted_rply.ar_results.proc = xdr_void;
		if (! xdrrec_skiprecord(xdrs)) {
			TRACE2("clnttcp_call xdrrec_skiprec failed, return %d",
							ct->ct_error.re_status);
			return (ct->ct_error.re_status);
		}
		/* now decode and validate the response header */
		if (! xdr_replymsg(xdrs, &reply_msg)) {
			if (ct->ct_error.re_status == RPC_SUCCESS) {
				TRACE("clnttcp_call status=SUCCESS, continue");
				continue;
			}
			TRACE2("clnttcp_call not SUCCESS, return %d",
							ct->ct_error.re_status);
			return (ct->ct_error.re_status);
		}
		TRACE3("clnttcp_call after replymsg , rm_xid=%d, x_id=%d",
							reply_msg.rm_xid, x_id);
		if (reply_msg.rm_xid == x_id)
			break;
		TRACE("clnttcp_call transaction IDs not the same, try again");
	}

	/*
	 * process header
	 */
	TRACE("clnttcp_call process header ...");
	_seterr_reply(&reply_msg, &(ct->ct_error));
	if (ct->ct_error.re_status == RPC_SUCCESS) {
		TRACE("clnttcp_call re_status == RPC_SUCCESS");
		if (! AUTH_VALIDATE(h->cl_auth, &reply_msg.acpted_rply.ar_verf)) {
			TRACE("clnttcp_call auth_validate failed!");
			ct->ct_error.re_status = RPC_AUTHERROR;
			ct->ct_error.re_why = AUTH_INVALIDRESP;
		} else if (! (*xdr_results)(xdrs, results_ptr)) {
			TRACE("clnttcp_call xdr_results failed...");
			if (ct->ct_error.re_status == RPC_SUCCESS)
				ct->ct_error.re_status = RPC_CANTDECODERES;
			TRACE2("clnttcp_call status=%d",ct->ct_error.re_status);
		}
		/* free verifier ... */
		TRACE("clnttcp_call free verifier ... ");
		if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			TRACE("clnttcp_call call xdr_opaque_auth");
			(void)xdr_opaque_auth(xdrs, &(reply_msg.acpted_rply.ar_verf));
		}
	TRACE("clnttcp_call end successful completion ...");
	}  /* end successful completion */
	else {
		TRACE("clnttcp_call credentials being refreshed ...");
		/* maybe our credentials need to be refreshed ... */
		if (
		    refreshes-- &&
		    AUTH_REFRESH(h->cl_auth))
			goto call_again;
	TRACE("clnttcp_call end of unsuccessful completion");
	}  /* end of unsuccessful completion */
	TRACE2("clnttcp_call returning re_status=%d", ct->ct_error.re_status);
	return (ct->ct_error.re_status);
}

static void
clnttcp_geterr(h, errp)
	CLIENT *h;
	struct rpc_err *errp;
{
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	TRACE("clnttcp_geterr SOP");
	*errp = ct->ct_error;
}

static bool_t
clnttcp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct ct_data *ct = (struct ct_data *)cl->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);

	TRACE("clnttcp_freeres SOP");
	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

static void
clnttcp_abort()
{
	TRACE("clnttcp_abort SOP");
}


static bool_t
clnttcp_control(cl, request, info)
        CLIENT *cl;
        int request;
        char *info;
{
        register struct ct_data *ct = (struct ct_data *)cl->cl_private;

        switch (request) {
        case CLSET_TIMEOUT:
                ct->ct_wait = *(struct timeval *)info;
                ct->ct_waitset = TRUE;
                break;
        case CLGET_TIMEOUT:
                *(struct timeval *)info = ct->ct_wait;
                break;
        case CLGET_SERVER_ADDR:
                *(struct sockaddr_in *)info = ct->ct_addr;
                break;
        default:
                return (FALSE);
        }
        return (TRUE);
}

static void
clnttcp_destroy(h)
	CLIENT *h;
{
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

        if (ct->ct_closeit) {
                (void)close(ct->ct_sock);
        }
	TRACE("clnttcp_destroy SOP");
	XDR_DESTROY(&(ct->ct_xdrs));
	mem_free((caddr_t)ct, sizeof(struct ct_data));
	mem_free((caddr_t)h, sizeof(CLIENT));
}

/*
 * Interface between xdr serializer and tcp connection.
 * Behaves like the system calls, read & write, but keeps some error state
 * around for the rpc level.
 */
static int
readtcp(ct, buf, len)
	register struct ct_data *ct;
	caddr_t buf;
	register int len;
{
	fd_set mask;
	fd_set readfds;
	int ct_sock = ct->ct_sock;
	int nfds = ct_sock + 1;
	int fd_size = (howmany(nfds, NFDBITS) << 2); /* bytes to copy */

	TRACE2("readtcp (clnt_tcp.c) SOP, len=%d", len);
	TRACE3("readtcp (clnt_tcp.c) mask0 = 0x%x, mask1 = 0x%x", mask.fds_bits[0], mask.fds_bits[1]);
	if (len == 0)
		return (0);
	FD_ZERO(&mask);
	FD_SET(ct->ct_sock, &mask);
	while (TRUE) {
		memcpy(&readfds, &mask, fd_size); 
		TRACE3("readtcp (clnt_tcp.c) select readfds[0] = 0x%x, readfds[1] = 0x%x", 
			readfds.fds_bits[0], readfds.fds_bits[0]);
		switch (select(nfds, &readfds, (int*)NULL, (int*)NULL,
		    &(ct->ct_wait))) {

		case 0:
			TRACE("readtcp (clnt_tcp.c) select case 0, return -1");
			ct->ct_error.re_status = RPC_TIMEDOUT;
			return (-1);

		case -1:
			TRACE2("readtcp (clnt_tcp.c) select case -1, errno=%d", errno);
			if (errno == EINTR)
				continue;
			ct->ct_error.re_status = RPC_CANTRECV;
			ct->ct_error.re_errno = errno;
			TRACE("readtcp (clnt_tcp.c) errno != EINTR, return -1");
			return (-1);
		}
		TRACE("readtcp (clnt_tcp.c) out of switch");
		/* if (readfds == mask) */
		if ( memcmp(&readfds, &mask, fd_size) == 0 )
			break;
	}
	TRACE3("readtcp (clnt_tcp.c) read sock=%d, len=%d bytes", ct->ct_sock, len);
	switch (len = read(ct->ct_sock, buf, len)) {

	case 0:
		TRACE("readtcp (clnt_tcp.c) case 0, error");
		/* premature eof */
		ct->ct_error.re_errno = ECONNRESET;
		ct->ct_error.re_status = RPC_CANTRECV;
		len = -1;  /* it's really an error */
		break;

	case -1:
		TRACE2("readtcp (clnt_tcp.c) case -1, errno=%d", errno);
		ct->ct_error.re_errno = errno;
		ct->ct_error.re_status = RPC_CANTRECV;
		break;
	}
	TRACE2("readtcp (clnt_tcp.c) returning %d", len);
	return (len);
}

static int
writetcp(ct, buf, len)
	struct ct_data *ct;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	TRACE("writetcp(clnt_tcp.c) SOP");
	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		TRACE2("writetcp(clnt_tcp.c) try to write %d bytes", cnt);
		if ((i = write(ct->ct_sock, buf, cnt)) == -1) {
			TRACE2("writetcp(clnt_tcp.c) write failed, errno = %d", errno);
			ct->ct_error.re_errno = errno;
			ct->ct_error.re_status = RPC_CANTSEND;
			return (-1);
		}
		TRACE2("writetcp(clnt_tcp.c) wrote %d bytes", i);
	}
	TRACE2("writetcp(clnt_tcp.c) returning len=%d", len);
	return (len);
}
