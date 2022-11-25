/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.5 $	$Date: 92/02/07 17:08:31 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_tcp.c,v $
 * $Revision: 12.5 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:31 $
 *
 * Revision 12.4  90/12/10  17:21:18  17:21:18  prabha
 * TRACE changes, select changes for perfoprmance.
 * 
 * Revision 12.3  90/08/30  11:59:18  11:59:18  prabha (Prabha Chadayammuri)
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.2  90/06/11  08:16:50  08:16:50  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/01/31  11:54:37  11:54:37  dlr (Dominic Ruffatto)
 * Cleaned up comments after #else and #endif lines for ANSII-C.  Also made
 * changes for >64 file descriptors.  These changes are currently ifdef'd
 * with the GT_64_FDS flag.
 * 
 * Revision 12.0  89/09/25  16:09:23  16:09:23  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  13:13:22  13:13:22  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.13  89/01/26  12:12:29  12:12:29  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.12  89/01/16  15:22:51  15:22:51  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:54:41  14:54:41  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.9  88/09/22  08:52:48  08:52:48  chm (Cristina H. Mahon)
 * Fixed DTS CNDdm01702.  This fix requires that the definition of 
 * full_fd_set be inside #ifndef NFS3_2 since with this change that is
 * defined in svc.h which is included by this file.
 * 
 * Revision 1.1.10.11  88/09/22  07:52:10  07:52:10  cmahon (Christina Mahon)
 * Fixed DTS CNDdm01702.  This fix requires that the definition of 
 * full_fd_set be inside #ifndef NFS3_2 since with this change that is
 * defined in svc.h which is included by this file.
 * 
 * Revision 1.1.10.10  88/08/05  12:56:22  12:56:22  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.9  88/08/02  12:16:00  12:16:00  cmahon (Christina Mahon)
 * Added the ability to do a bind to a reserved port if the caller is
 * super-user.
 * 
 * Revision 1.1.10.8  88/06/22  09:37:26  09:37:26  cmahon (Christina Mahon)
 * Missed part of fix for DTS CNDdm01434 (copying instead of memcpy).  So
 * added fix to that now.
 * 
 * Revision 1.1.10.7  88/06/22  08:26:10  08:26:10  cmahon (Christina Mahon)
 * Changes made to fix DTS CNDdm01434.  These changes allow select to handle
 * more than just 32 file descriptors.
 * 
 * Revision 1.1.10.6  88/03/24  17:16:17  17:16:17  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use an NLS front end for all NFS
 * libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.5  87/10/06  12:52:40  12:52:40  cmahon (Christina Mahon)
 * Fixed bug which made NIS hang when talking to the portmapper -- YP was fixed
 * to not do a lingering, graceful close when it made its TCP connection to the
 * portmapper to determine if portmap is up (gross by design).  This caused the
 * accept() in rendezvous_request() to get an error and hang occasionally.  The
 * fix is to make the listen() socket NON-BLOCKING (FIOSNBIO), and explicitly
 * make the accept() sockets BLOCKING (they're non-blocking by default).
 * HPNFS	jad	87.10.06
 * 
 * Revision 1.1.10.4  87/08/07  15:07:10  15:07:10  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  11:13:22  11:13:22  cmahon (Christina Mahon)
 * Merged to latest 300 version.  Changed perror() calls.
 * 
 * Revision 1.1.10.2  87/07/16  22:25:20  22:25:20  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:44:20  11:44:20  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_tcp.c,v 12.5 92/02/07 17:08:31 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * svc_tcp.c, Server side for TCP/IP based RPC. 
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define abort 			_abort
#define accept 			_accept
#define bind 			_bind
#define bindresvport 		_bindresvport
#define catgets 		_catgets
#define close 			_close
#define fprintf 		_fprintf
#define getsockname 		_getsockname
#define ioctl 			_ioctl
#define listen 			_listen
#define memcmp 			_memcmp
#define memcpy 			_memcpy
#define memset 			_memset
#define perror 			_perror
#define read 			_read
#define select 			_select
#define socket 			_socket
#define svcfd_create 		_svcfd_create		/* In this file */
#define svctcp_create 		_svctcp_create		/* In this file */
#define write 			_write
#define xdr_callmsg 		_xdr_callmsg
#define xdr_replymsg 		_xdr_replymsg
#define xdrrec_create 		_xdrrec_create
#define xdrrec_endofrecord 	_xdrrec_endofrecord
#define xdrrec_eof 		_xdrrec_eof
#define xdrrec_skiprecord 	_xdrrec_skiprecord
#define xprt_register 		_xprt_register
#define xprt_unregister 	_xprt_unregister

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 15	/* set number */
#include <nl_types.h>

#include <stdio.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <sys/types.h>

/*	HPNFS	jad	87.08.28
**	added TRACE() code to track down xdr_rec problems ...
*/
#ifdef	TRACEON
#define	LIBTRACE
#endif
#include <arpa/trace.h>

char *mem_alloc();
extern bool_t abort();
extern errno;

/*
 * Ops vector for TCP/IP based rpc service handle
 */
static bool_t		svctcp_recv();
static enum xprt_stat	svctcp_stat();
static bool_t		svctcp_getargs();
static bool_t		svctcp_reply();
static bool_t		svctcp_freeargs();
static void		svctcp_destroy();

static struct xp_ops svctcp_op = {
	svctcp_recv,
	svctcp_stat,
	svctcp_getargs,
	svctcp_reply,
	svctcp_freeargs,
	svctcp_destroy
};

/*
 * Ops vector for TCP/IP rendezvous handler
 */
static bool_t		rendezvous_request();
static enum xprt_stat	rendezvous_stat();

static struct xp_ops svctcp_rendezvous_op = {
	rendezvous_request,
	rendezvous_stat,
	abort,
	abort,
	abort,
	svctcp_destroy
};

static int readtcp(), writetcp();
static SVCXPRT *makefd_xprt();

struct tcp_rendezvous { /* kept in xprt->xp_p1 */
	u_int sendsize;
	u_int recvsize;
};

struct tcp_conn {  /* kept in xprt->xp_p1 */
	enum xprt_stat strm_stat;
	u_long x_id;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
};

static nl_catd nlmsg_fd;

/*
 * Usage:
 *	xprt = svctcp_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svctcp_create
 * binds it to an arbitrary port.  The routine then starts a tcp
 * listener on the socket's associated port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 *
 * Since tcp streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
**
**	HPNFS	jad	87.08.05
**	a note to the perplexed: the choosing of the "system default"
**	occurs in xdrrec_create -- don't look for it in this module!!
 */

#ifdef _NAMESPACE_CLEAN
#undef svctcp_create
#pragma _HP_SECONDARY_DEF _svctcp_create svctcp_create
#define svctcp_create _svctcp_create
#endif

SVCXPRT *
svctcp_create(sock, sendsize, recvsize)
	register int sock;
	u_int sendsize;
	u_int recvsize;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int on = 1;

	nlmsg_fd = _nfs_nls_catopen();

	TRACE("svctcp_create SOP");
	if (sock == RPC_ANYSOCK) {
		TRACE("svctcp_create RPC_ANYSOCK, getting socket");
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			perror((catgets(nlmsg_fd,NL_SETN,1, "svctcp_create: udp socket creation problem")));
			return ((SVCXPRT *)NULL);
		}
		TRACE2("svctcp_create got socket %d", sock);
		madesock = TRUE;
	}
	/*
	 * For NFS3.2, try to do a bindresvport.  If the called is super-user
	 * it should work.  Otherwise just do a normal bind.   mds 
	 */
	memset((char *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        if (bindresvport(sock, &addr)) {
                addr.sin_port = 0;
                (void)bind(sock, (struct sockaddr *)&addr, len);
	}
	TRACE("svctcp_create bound to socket, now trying to listen()");
	if ((getsockname(sock, (caddr_t)&addr, &len) != 0)  ||
	    (listen(sock, 2) != 0)) {
		TRACE2("svctcp_create listen failed, errno=%d", errno);
		perror((catgets(nlmsg_fd,NL_SETN,2, "svctcp_create: cannot getsockname or listen")));
		if (madesock)
		       (void)close(sock);
		return ((SVCXPRT *)NULL);
	}

	/*	HPNFS	jad	87.10.06
	**	Make the listen() socket non-blocking, so our accept()
	**	call never hangs ... with a blocking socket, if there is
	**	an error with the accept() it will block until a valid
	**	request comes in -- which causes whoever called the
	**	accept() (see rendezvous_request()) to HANG!
	*/
	(void) ioctl(sock, FIOSNBIO, &on);

	r = (struct tcp_rendezvous *)mem_alloc(sizeof(*r));
	if (r == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "svctcp_create: out of memory\n")));
		return ((SVCXPRT *)NULL);
	}
	r->sendsize = sendsize;
	r->recvsize = recvsize;
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "svctcp_create: out of memory\n")));
		return (NULL);
	}
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)r;
	xprt->xp_verf = _null_auth;
	xprt->xp_ops = &svctcp_rendezvous_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	TRACE("svctcp_create allocated memory, set up xprt, now registering");
	xprt_register(xprt);
	TRACE2("svctcp_create returning xprt 0x%x", xprt);
	return (xprt);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */

#ifdef _NAMESPACE_CLEAN
#undef svcfd_create
#pragma _HP_SECONDARY_DEF _svcfd_create svcfd_create
#define svcfd_create _svcfd_create
#endif

SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	TRACE("svcfd_create SOP, calling makefd_xprt");
	return (makefd_xprt(fd, sendsize, recvsize));
}

static SVCXPRT *
makefd_xprt(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	register SVCXPRT *xprt;
	register struct tcp_conn *cd;
 
	TRACE("makefd_xprt SOP");
	nlmsg_fd = _nfs_nls_catopen();
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == (SVCXPRT *)NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "svc_tcp: makefd_xprt: out of memory\n")));
		goto done;
	}
	cd = (struct tcp_conn *)mem_alloc(sizeof(struct tcp_conn));
	if (cd == (struct tcp_conn *)NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "svc_tcp: makefd_xprt: out of memory\n")));
		mem_free(xprt, sizeof(SVCXPRT));
		xprt = (SVCXPRT *)NULL;
		goto done;
	}
	cd->strm_stat = XPRT_IDLE;
	TRACE("makefd_xprt got memory, calling xdrrec_create");
	xdrrec_create(&(cd->xdrs), sendsize, recvsize,
	    (caddr_t)xprt, readtcp, writetcp);
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)cd;
	xprt->xp_verf.oa_base = cd->verf_body;
	xprt->xp_addrlen = 0;
	xprt->xp_ops = &svctcp_op;  /* truely deals with calls */
	xprt->xp_port = 0;  /* this is a connection, not a rendezvouser */
	xprt->xp_sock = fd;
	TRACE("makefd_xprt set up xprt, calling xprt_register");
	xprt_register(xprt);
    done:
	TRACE2("makefd_xprt returning xprt 0x%x", xprt);
	return (xprt);
}

static bool_t
rendezvous_request(xprt)
	register SVCXPRT *xprt;
{
	int sock;
	struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len;
	int off = 0;

	TRACE("rendezvous_request SOP");
	r = (struct tcp_rendezvous *)xprt->xp_p1;
    again:
	len = sizeof(struct sockaddr_in);
	TRACE("rendezvous_request calling accept()");
	if ((sock = accept(xprt->xp_sock, (struct sockaddr *)&addr,
	    &len)) < 0) {
		TRACE2("rendezvous_request accept failed, errno=%d", errno);
		if (errno == EINTR)
			goto again;
		TRACE("rendezvous_request returns FALSE");
		return (FALSE);
	}

	/*	HPNFS	jad	87.10.06
	**	since non-blocking passive socket gives us non-blocking
	**	active socket, we have to make sure to set it to blocking
	**	before doing anything with it!
	*/
	(void) ioctl(sock, FIOSNBIO, &off);

	/*
	 * make a new transporter (re-uses xprt)
	 */
	TRACE("rendezvous_request calling makefd_xprt");
	xprt = makefd_xprt(sock, r->sendsize, r->recvsize);
	xprt->xp_raddr = addr;
	xprt->xp_addrlen = len;
	TRACE("rendezvous_request returning FALSE");
	return (FALSE); /* there is never an rpc msg to be processed */
}

static enum xprt_stat
rendezvous_stat()
{
	TRACE("rendezvous_stat SOP, returning XPRT_IDLE"); 
	return (XPRT_IDLE);
}

static void
svctcp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct tcp_conn *cd = (struct tcp_conn *)xprt->xp_p1;

	TRACE2("svctcp_destroy SOP, calling unregister on 0x%x", xprt);
	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	if (xprt->xp_port != 0) {
		/* a rendezvouser socket */
		xprt->xp_port = 0;
	} else {
		/* an actual connection socket */
		XDR_DESTROY(&(cd->xdrs));
	}
	mem_free((caddr_t)cd, sizeof(struct tcp_conn));
	mem_free((caddr_t)xprt, sizeof(SVCXPRT));
	TRACE("svctcp_destroy returning");
}

/*
 * All read operations timeout after 35 seconds.
 * A timeout is fatal for the connection.
 */
static struct timeval wait_per_try = { 35, 0 };

/*
 * reads data from the tcp conection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 */
static int
readtcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	register int len;
{
	register int sock = xprt->xp_sock;
	int nfds = sock+1;
	int fd_size = (howmany(nfds, NFDBITS) << 2); 
	fd_set mask;
	fd_set readfds;

	FD_ZERO(&mask);
	FD_SET(sock, &mask);

	TRACE4("readtcp (svc_tcp.c) SOP: sock = %d, nfds = %d, fd-size = %d",sock, nfds,fd_size);
	do {
		memcpy(&readfds, &mask, fd_size);
		TRACE3("readtcp (svc_tcp.c) calling select, readfds[0]=0x%x, readfds[1]=0x%x",
					readfds.fds_bits[0],readfds.fds_bits[1]);
		if (select(nfds, &readfds, NULL, NULL, &wait_per_try) <= 0) {
			TRACE2("readtcp (svc_tcp.c) select errno=%d", errno);
			if (errno == EINTR)
				continue;
			goto fatal_err;
		}
		TRACE3("readtcp (svc_tcp.c): readfds after select: readfds[0]=0x%x, readfds[1]=0x%x",
				readfds.fds_bits[0],readfds.fds_bits[1]);
	} while (memcmp(&readfds, &mask, fd_size) != 0);
	TRACE4("readtcp (svc_tcp.c) calling read on fd %d for %d bytes (buf addr = 0x%x)",sock,len,buf);
	if ((len = read(sock, buf, len)) > 0) {
		TRACE2("readtcp (svc_tcp.c) read got %d bytes", len);
		return (len);
	}
fatal_err:
	TRACE3("readtcp (svc_tcp.c) read got len=%d, errno=%d", len, errno);
	((struct tcp_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	TRACE("readtcp (svc_tcp.c) fatal error, XPRT_DIED and return -1");
	return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
static int
writetcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	TRACE2("writetcp (svc_tcp.c) SOP, len=%d", len);
	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		TRACE2("writetcp (svc_tcp.c) trying to write %d", cnt);
		if ((i = write(xprt->xp_sock, buf, cnt)) < 0) {
			TRACE2("writetcp (svc_tcp.c) XPRT_DIED, errno=%d", errno);
			((struct tcp_conn *)(xprt->xp_p1))->strm_stat =
			    XPRT_DIED;
			return (-1);
		}
		TRACE2("writetcp (svc_tcp.c) wrote %d bytes", i);
	}
	TRACE2("writetcp (svc_tcp.c) returning %d", len);
	return (len);
}

static enum xprt_stat
svctcp_stat(xprt)
	SVCXPRT *xprt;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);

	TRACE("svctcp_stat SOP");
	if (cd->strm_stat == XPRT_DIED)
		return (XPRT_DIED);
	TRACE("svctcp_stat not dead");
	if (! xdrrec_eof(&(cd->xdrs)))
		return (XPRT_MOREREQS);
	TRACE("svctcp_stat no more reqs, return idle");
	return (XPRT_IDLE);
}

static bool_t
svctcp_recv(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);

	TRACE("svctcp_recv SOP");
	xdrs->x_op = XDR_DECODE;
	(void)xdrrec_skiprecord(xdrs);
	TRACE("svctcp_recv skiprecord done, now callmsg");
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		TRACE2("svctcp_recv xid=0x%x return TRUE", cd->x_id);
		return (TRUE);
	}
	TRACE("svctcp_recv returns FALSE");
	return (FALSE);
}

static bool_t
svctcp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	TRACE("svctcp_getargs SOP, return funky expr");
	return ((*xdr_args)(&(((struct tcp_conn *)(xprt->xp_p1))->xdrs), args_ptr));
}

static bool_t
svctcp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs =
	    &(((struct tcp_conn *)(xprt->xp_p1))->xdrs);

	xdrs->x_op = XDR_FREE;
	TRACE("svctcp_freeargs SOP, return funky expr");
	return ((*xdr_args)(xdrs, args_ptr));
}

static bool_t
svctcp_reply(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);
	register bool_t stat;

	TRACE("svctcp_reply SOP");
	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	TRACE2("svctcp_reply trying to replymsg to xid=0x%x", cd->x_id);
	stat = xdr_replymsg(xdrs, msg);
	TRACE2("svctcp_reply replymsg returned %d, call endofrecord", stat);
	(void)xdrrec_endofrecord(xdrs, TRUE);
	TRACE2("svctcp_reply returning %d", stat);
	return (stat);
}
