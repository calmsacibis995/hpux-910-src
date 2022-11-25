/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.5 $	$Date: 92/02/07 17:08:35 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_udp.c,v $
 * $Revision: 12.5 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:35 $
 *
 * Revision 12.4  90/08/30  12:00:29  12:00:29  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.3  90/06/11  08:19:20  08:19:20  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.2  90/04/19  10:58:35  10:58:35  dlr (Dominic Ruffatto)
 * Made change to set the send and receive sizes for the UDP socket via
 * set sockopt.  This was necessary because the default was changed to 2k
 * in 8.0.
 * 
 * Revision 12.1  90/01/31  11:55:08  11:55:08  dlr (Dominic Ruffatto)
 * Cleaned up comments after #else and #endif lines for ANSII-C.  Also made
 *  changes for >64 file descriptors.  These changes are currently ifdef'd
 * with the GT_64_FDS flag.
 * 
 * Revision 12.0  89/09/25  16:09:26  16:09:26  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/06  08:20:26  08:20:26  dds (Darren Smith)
 * Fixed typo in NAME SPACE clean up define, xdr_memcreate ==> xdrmem_create.
 * dds
 * 
 * Revision 1.1.10.9  89/02/06  07:19:42  07:19:42  cmahon (Christina Mahon)
 * Fixed typo in NAME SPACE clean up define, xdr_memcreate ==> xdrmem_create.
 * dds
 * 
 * Revision 1.1.10.8  89/01/26  12:13:27  12:13:27  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.7  89/01/16  15:23:45  15:23:45  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:54:45  14:54:45  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.4  88/08/05  13:58:10  13:58:10  dds (Darren Smith)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.6  88/08/05  12:57:17  12:57:17  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.5  88/08/02  12:23:18  12:23:18  cmahon (Christina Mahon)
 * Added the ability for a super-user to bind to a reserved port.
 * 
 * Revision 1.1.10.4  88/03/24  17:19:14  17:19:14  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use an NLS front end for all NFS
 * libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.3  87/08/07  15:07:57  15:07:57  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:25:51  22:25:51  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:44:32  11:44:32  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_udp.c,v 12.5 92/02/07 17:08:35 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * svc_udp.c,
 * Server side for UDP/IP based RPC.  (Does some caching in the hopes of
 * achieving execute-at-most-once semantics.)
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

#define bind 			_bind
#define bindresvport 		_bindresvport
#define catgets 		_catgets
#define close 			_close
#define fprintf 		_fprintf
#define getsockname 		_getsockname
#define memset 			_memset
#define perror 			_perror
#define recvfrom 		_recvfrom
#define sendto 			_sendto
#define setsockopt		_setsockopt
#define socket 			_socket
#define svcudp_bufcreate 	_svcudp_bufcreate	/* In this file */
#define svcudp_create 		_svcudp_create		/* In this file */
#define xdr_callmsg 		_xdr_callmsg
#define xdr_replymsg 		_xdr_replymsg
#define xdrmem_create 		_xdrmem_create
#define xprt_register 		_xprt_register
#define xprt_unregister 	_xprt_unregister

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 16	/* set number */
#include <nl_types.h>
static	nl_catd nlmsg_fd;

#include <stdio.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>

char *mem_alloc();

#define rpc_buffer(xprt) ((xprt)->xp_p1)
#define MAX(a, b)     ((a > b) ? a : b)

static bool_t		svcudp_recv();
static bool_t		svcudp_reply();
static enum xprt_stat	svcudp_stat();
static bool_t		svcudp_getargs();
static bool_t		svcudp_freeargs();
static void		svcudp_destroy();

static struct xp_ops svcudp_op = {
	svcudp_recv,
	svcudp_stat,
	svcudp_getargs,
	svcudp_reply,
	svcudp_freeargs,
	svcudp_destroy
};

extern int errno;

/*
 * kept in xprt->xp_p2
 */
struct svcudp_data {
	u_int   su_iosz;	/* byte size of send.recv buffer */
	u_long	su_xid;		/* transaction id */
	XDR	su_xdrs;	/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
};
#define	su_data(xprt)	((struct svcudp_data *)(xprt->xp_p2))

/*
 * Usage:
 *	xprt = svcudp_create(sock);
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svcudp_create
 * binds it to an arbitrary port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 * Once *xprt is initialized, it is registered as a transporter;
 * see (svc.h, xprt_register).
 * The routines returns NULL if a problem occurred.
 */

#ifdef _NAMESPACE_CLEAN
#undef svcudp_bufcreate
#pragma _HP_SECONDARY_DEF _svcudp_bufcreate svcudp_bufcreate
#define svcudp_bufcreate _svcudp_bufcreate
#endif

SVCXPRT *
svcudp_bufcreate(sock, sendsz, recvsz)
	register int sock;
	u_int sendsz, recvsz;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct svcudp_data *su;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);

	nlmsg_fd = _nfs_nls_catopen();

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror((catgets(nlmsg_fd,NL_SETN,1, "svcudp_create: socket creation problem")));
			return ((SVCXPRT *)NULL);
		}
		madesock = TRUE;
	}
        if (setsockopt(sock, SOL_SOCKET,SO_RCVBUF,&recvsz,sizeof(u_int))==-1){
		perror((catgets(nlmsg_fd,NL_SETN,1, 
				"svcudp_create: setsockopt problem")));
		return ((SVCXPRT *)NULL);
	}
	if (setsockopt(sock, SOL_SOCKET,SO_SNDBUF,&sendsz,sizeof(u_int))==-1){
		perror((catgets(nlmsg_fd,NL_SETN,1, 
				"svcudp_create: setsockopt problem")));
		return ((SVCXPRT *)NULL);
	}

        memset((char *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        if (bindresvport(sock, &addr)) {
                addr.sin_port = 0;
                (void)bind(sock, (struct sockaddr *)&addr, len);
        }
	if (getsockname(sock, (caddr_t)&addr, &len) != 0) {
		perror((catgets(nlmsg_fd,NL_SETN,2, "svcudp_create - cannot getsockname")));
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "svcudp_create: out of memory\n")));
		return (NULL);
	}
	su = (struct svcudp_data *)mem_alloc(sizeof(*su));
	if (su == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "svcudp_create: out of memory\n")));
		return (NULL);
	}
	su->su_iosz = ((MAX(sendsz, recvsz) + 3) / 4) * 4;
	if ((rpc_buffer(xprt) = mem_alloc(su->su_iosz)) == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "svcudp_create: out of memory\n")));
		return (NULL);
	}
	xdrmem_create(
	    &(su->su_xdrs), rpc_buffer(xprt), su->su_iosz, XDR_DECODE);
	xprt->xp_p2 = (caddr_t)su;
	xprt->xp_verf.oa_base = su->su_verfbody;
	xprt->xp_ops = &svcudp_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

#ifdef _NAMESPACE_CLEAN
#undef svcudp_create
#pragma _HP_SECONDARY_DEF _svcudp_create svcudp_create
#define svcudp_create _svcudp_create
#endif

SVCXPRT *
svcudp_create(sock, sendsz, recvsz)
	int sock;
{

	return(svcudp_bufcreate(sock, UDPMSGSIZE, UDPMSGSIZE));
}
 
static enum xprt_stat
svcudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE); 
}

static bool_t
svcudp_recv(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int rlen;

    again:
	xprt->xp_addrlen = sizeof(struct sockaddr_in);
	rlen = recvfrom(xprt->xp_sock, rpc_buffer(xprt), su->su_iosz,
	    0, (struct sockaddr *)&(xprt->xp_raddr), &(xprt->xp_addrlen));
	if (rlen == -1 && errno == EINTR)
		goto again;
	if (rlen < 4*sizeof(u_long))
		return (FALSE);
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
		return (FALSE);
	su->su_xid = msg->rm_xid;
	return (TRUE);
}

static bool_t
svcudp_reply(xprt, msg)
	register SVCXPRT *xprt; 
	struct rpc_msg *msg; 
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int slen;
	register bool_t stat = FALSE;

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	msg->rm_xid = su->su_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (sendto(xprt->xp_sock, rpc_buffer(xprt), slen, 0,
		    (struct sockaddr *)&(xprt->xp_raddr), xprt->xp_addrlen)
		    == slen)
			stat = TRUE;
	}
	return (stat);
}

static bool_t
svcudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr));
}

static bool_t
svcudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs = &(su_data(xprt)->su_xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static void
svcudp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct svcudp_data *su = su_data(xprt);

	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	XDR_DESTROY(&(su->su_xdrs));
	mem_free(rpc_buffer(xprt), su->su_iosz);
	mem_free((caddr_t)su, sizeof(struct svcudp_data));
	mem_free((caddr_t)xprt, sizeof(SVCXPRT));
}
