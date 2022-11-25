/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.8 $	$Date: 92/02/07 17:08:20 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/pmap_rmt.c,v $
 * $Revision: 12.8 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:20 $
 *
 * Revision 12.7  90/12/10  17:20:21  17:20:21  prabha
 * nfds changed to select on only the required socket.
 * 
 * Revision 12.6  90/08/30  11:58:24  11:58:24  prabha (Prabha Chadayammuri)
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.5  90/06/11  08:07:59  08:07:59  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.4  90/04/19  12:17:46  12:17:46  dlr (Dominic Ruffatto)
 * Used setsockopt to increase the UDP receive size to UDPMSGSIZE.  This
 * was required because the default value was changed to 2kbytes in 8.0.
 * 
 * Revision 12.3  90/01/31  11:53:31  11:53:31  dlr (Dominic Ruffatto)
 * Cleaned up comments after #else and #endif lines for ANSII-C.  Also made
 * changes for >64 file descriptors.  These changes are currently ifdef'd
 * with the GT_64_FDS flag.
 * 
 * Revision 12.2  90/01/11  16:16:17  16:16:17  dlr (Dominic Ruffatto)
 * Added a call to setsockopt in order to turn on broadcast.
 * 
 * Revision 12.1  89/11/27  11:48:06  11:48:06  dlr (Dominic Ruffatto)
 * Changed reference of baddr.sin_addr.S_un.S_addr to baddr.sin_addr.s_addr
 * to work with the BSD 4.3 version of netinet/in.h.
 * 
 * Revision 12.0  89/09/25  16:08:54  16:08:54  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.4  89/02/06  12:02:29  12:02:29  dds (Darren Smith)
 * Added inet_makeaddr and inet_netof to #defines for NAME SPACE cleanup. dds.
 * 
 * Revision 1.1.10.16  89/02/06  11:01:39  11:01:39  cmahon (Christina Mahon)
 * Added inet_makeaddr and inet_netof to #defines for NAME SPACE cleanup. dds.
 * 
 * Revision 1.1.10.15  89/02/01  11:24:02  11:24:02  cmahon (Christina Mahon)
 * Added secondary defs for clnt_broadcast and pmap_rmtcall -- missed in last
 * pass at NAME SPACE cleanup. dds.
 * 
 * Revision 1.1.10.14  89/01/26  12:03:37  12:03:37  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.13  89/01/16  15:18:17  15:18:17  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:52  14:48:52  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.10  88/10/04  11:32:14  11:32:14  chm (Cristina H. Mahon)
 * Changed size of inbuf so that results of recvfrom fit.
 * Fix DTS CNDdm01796.
 * 
 * Revision 1.1.10.12  88/10/04  10:31:20  10:31:20  cmahon (Christina Mahon)
 * Changed size of inbuf so that results of recvfrom fit.
 * Fix DTS CNDdm01796.
 * 
 * Revision 1.1.10.11  88/09/09  11:42:17  11:42:17  cmahon (Christina Mahon)
 * Localized error messages that had been added (user level code only).
 * 
 * Revision 1.1.10.10  88/08/05  12:54:54  12:54:54  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.9  88/06/22  09:40:01  09:40:01  cmahon (Christina Mahon)
 * Fixed DTS CNDdm01434 (select can now deal with file descriptors greater
 * than 32).
 * 
 * Revision 1.1.10.8  88/06/20  15:27:53  15:27:53  cmahon (Christina Mahon)
 * Added changes to update to RPC 3.9  inside ifdef's
 * 
 * Revision 1.1.10.7  88/06/03  10:31:03  10:31:03  cmahon (Christina Mahon)
 * Finally able to checkin the fix for broadcast problem.  
 * 
 * Revision 10.2  88/03/24  14:59:44  14:59:44  chm (Cristina H. Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS libc
 * routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.6  88/03/24  16:57:33  16:57:33  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS libc
 * routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.5  87/10/26  08:40:56  08:40:56  cmahon (Christina Mahon)
 * Fixed DTS CNOdm00724.  Fixed discrepancy between pmap_rmt and portmap.
 * pmap_rmt had a timeout of 3 seconds but portmap had a timeout of 5 seconds.
 * Increased pmap_rmt timeout to 6 seconds.
 * 
 * Revision 1.1.10.4  87/08/07  14:56:49  14:56:49  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  11:00:55  11:00:55  cmahon (Christina Mahon)
 * Update to latest 300 version.  Added default value for stat = TIMEDOUT
 * 
 * Revision 1.1.10.2  87/07/16  22:14:08  22:14:08  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.2  86/07/28  11:41:23  11:41:23  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: pmap_rmt.c,v 12.8 92/02/07 17:08:20 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * pmap_rmt.c
 * Client interface to pmap rpc service.
 * remote call and broadcast service
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

#define authunix_create_default 	_authunix_create_default
#define catgets 			_catgets
#define clnt_broadcast 			_clnt_broadcast	    /* In this file */
#define clntudp_create 			_clntudp_create
#define close 				_close
#define getpid 				_getpid
#define gettimeofday 			_gettimeofday
#define inet_makeaddr			_inet_makeaddr
#define inet_netof			_inet_netof
#define ioctl 				_ioctl
#define memcpy 				_memcpy
#define memset 				_memset
#define perror 				_perror
#define pmap_rmtcall 			_pmap_rmtcall	    /* In this file */
#define recvfrom 			_recvfrom
#define select 				_select
#define sendto 				_sendto
#define setsockopt			_setsockopt
#define socket 				_socket
#define xdr_callmsg 			_xdr_callmsg
#define xdr_reference 			_xdr_reference
#define xdr_replymsg 			_xdr_replymsg
#define xdr_u_long 			_xdr_u_long
#define xdr_void 			_xdr_void
#define xdrmem_create 			_xdrmem_create

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 11	/* set number */
#include <nl_types.h>

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_prot.h> 
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#define MAX_BROADCAST_SIZE 1400

#include <sys/types.h>

int inet_netof();
struct in_addr inet_makeaddr();
extern int errno;

/* HPNFS  
** 	The timeout used to be 3 seconds instead of 6.  The problem 
** with that is that the timeout for portmap is 5 seconds and so portmap
** would not catch all the retransmissions and there would be problems.
*/

static struct timeval timeout = { 6, 0 };

static nl_catd nlmsg_fd;

/*
 * Structures and XDR routines for parameters to and replys from
 * the pmapper remote-call-service.
 */

struct rmtcallargs {
	u_long prog, vers, proc, arglen;
	caddr_t args_ptr;
	xdrproc_t xdr_args;
};
static bool_t xdr_rmtcall_args();

struct rmtcallres {
	u_long *port_ptr;
	u_long resultslen;
	caddr_t results_ptr;
	xdrproc_t xdr_results;
};
static bool_t xdr_rmtcallres();

/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
*/

#ifdef _NAMESPACE_CLEAN
#undef pmap_rmtcall
#pragma _HP_SECONDARY_DEF _pmap_rmtcall pmap_rmtcall
#define pmap_rmtcall _pmap_rmtcall
#endif

enum clnt_stat
pmap_rmtcall(addr, prog, vers, proc, xdrargs, argsp, xdrres, resp, tout, port_ptr)
	struct sockaddr_in *addr;
	u_long prog, vers, proc;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	u_long *port_ptr;
{
	int socket = -1;
	register CLIENT *client;
	struct rmtcallargs a;
	struct rmtcallres r;
	enum clnt_stat stat;

	addr->sin_port = htons(PMAPPORT);
	client = clntudp_create(addr, PMAPPROG, PMAPVERS, timeout, &socket);
	if (client != (CLIENT *)NULL) {
		a.prog = prog;
		a.vers = vers;
		a.proc = proc;
		a.args_ptr = argsp;
		a.xdr_args = xdrargs;
		r.port_ptr = port_ptr;
		r.results_ptr = resp;
		r.xdr_results = xdrres;
		stat = CLNT_CALL(client, PMAPPROC_CALLIT, xdr_rmtcall_args, &a,
		    xdr_rmtcallres, &r, tout);
		CLNT_DESTROY(client);
	} else {
		stat = RPC_FAILED;
	}
	(void)close(socket);
	addr->sin_port = 0;
	return (stat);
}

/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
static bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR *xdrs;
	register struct rmtcallargs *cap;
{
	u_int lenposition, argposition, position;

	if (xdr_u_long(xdrs, &(cap->prog)) &&
	    xdr_u_long(xdrs, &(cap->vers)) &&
	    xdr_u_long(xdrs, &(cap->proc))) {
		lenposition = XDR_GETPOS(xdrs);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		argposition = XDR_GETPOS(xdrs);
		if (! (*(cap->xdr_args))(xdrs, cap->args_ptr))
		    return (FALSE);
		position = XDR_GETPOS(xdrs);
		cap->arglen = (u_long)position - (u_long)argposition;
		XDR_SETPOS(xdrs, lenposition);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		XDR_SETPOS(xdrs, position);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
static bool_t
xdr_rmtcallres(xdrs, crp)
	register XDR *xdrs;
	register struct rmtcallres *crp;
{

	caddr_t port_ptr;

        port_ptr = (caddr_t)crp->port_ptr;
	if (xdr_reference(xdrs, &port_ptr, sizeof (u_long), xdr_u_long) &&
		xdr_u_long(xdrs, &crp->resultslen)) {
                crp->port_ptr = (u_long *)port_ptr;
		return ((*(crp->xdr_results))(xdrs, crp->results_ptr));
	}
	return (FALSE);
}

/*
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial 
 * routines which only support udp/ip .
 */

static int
getbroadcastnets(addrs, sock, buf)
	struct in_addr *addrs;
	int sock;  /* any valid socket will do */
	char *buf;  /* why allocxate more when we can use existing... */
{
	struct ifconf ifc;
        struct ifreq ifreq, *ifr;
	struct sockaddr_in *sin;
        int n, i;

	nlmsg_fd = _nfs_nls_catopen();

	ifc.ifc_len = UDPMSGSIZE;
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
                perror((catgets(nlmsg_fd,NL_SETN,1, "broadcast: ioctl (get interface configuration)")));
                return (0);
        }
        ifr = ifc.ifc_req;
        for (i = 0, n = ifc.ifc_len/sizeof (struct ifreq); n > 0; n--, ifr++) {
                ifreq = *ifr;
                if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
                        perror((catgets(nlmsg_fd,NL_SETN,2, "broadcast: ioctl (get interface flags)")));
                        continue;
                }
                if ((ifreq.ifr_flags & IFF_BROADCAST) &&
		    (ifreq.ifr_flags & IFF_UP) &&
		    ifr->ifr_addr.sa_family == AF_INET) {
#ifdef SIOCGIFBRDADDR
                	if (ioctl(sock, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
                        	perror((catgets(nlmsg_fd,NL_SETN,8, "broadcast: ioctl (get broadcast addr)")));
                        	continue;
                	}
                        sin = (struct sockaddr_in *)&ifreq.ifr_broadaddr;
			addrs[i++] = sin->sin_addr;
#else
                        sin = (struct sockaddr_in *)&ifr->ifr_addr;
			addrs[i++] = inet_makeaddr(inet_netof
			    (sin->sin_addr.s_addr), INADDR_ANY);
#endif /* SIOCGIFBRDADDR */
                }
        }
	return (i);
}

typedef bool_t (*resultproc_t)();

#ifdef _NAMESPACE_CLEAN
#undef clnt_broadcast
#pragma _HP_SECONDARY_DEF _clnt_broadcast clnt_broadcast
#define clnt_broadcast _clnt_broadcast
#endif

enum clnt_stat 
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	enum clnt_stat stat = RPC_TIMEDOUT;	/* default return value */
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, fromlen, nets;
	int on = 1, nfds, fd_size;
	fd_set mask;
	fd_set readfds;
	register int sock, i;
	bool_t done = FALSE;
	register u_long xid;
	u_long port;
	struct in_addr addrs[20];
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	struct timeval t; 
	char outbuf[MAX_BROADCAST_SIZE], inbuf[UDPMSGSIZE];
/*	bool_t sum; */
	int cnt, msgsize;

	nlmsg_fd = _nfs_nls_catopen();

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,3, "Cannot create socket for broadcast rpc")));
		stat = RPC_CANTSEND;
		goto done_broad;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		/* 
		 *  NEED TO NLSize THIS PERROR
		 */
		perror("Cannot set broadcast socket option");
	}
	msgsize = UDPMSGSIZE;
	if (setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&msgsize,sizeof(int))==-1){
		/* 
		 *  NEED TO NLSize THIS PERROR
		 */
                perror("Cannot set socket receive size");
        }
	
	FD_ZERO(&mask);
	FD_SET(sock, &mask);
	nfds = sock+1;
	fd_size = (howmany(nfds, NFDBITS) << 2); /* num of bytes to copy */
	nets = getbroadcastnets(addrs, sock, inbuf);
	memset(&baddr, 0, sizeof (baddr));
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(PMAPPORT);
#ifdef SIOCGIFBRDADDR
	baddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
#else
	baddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif /* SIOCGIFBRDADDR */
	(void)gettimeofday(&t, (struct timezone *)0);
	msg.rm_xid = xid = getpid() ^ t.tv_sec ^ t.tv_usec;
	t.tv_usec = 0;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = xargs;
	a.args_ptr = argsp;
	r.port_ptr = &port;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	xdrmem_create(xdrs, outbuf, MAX_BROADCAST_SIZE, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);
	/*
	 * Basic loop: broadcast a packet and wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
		for (i = 0; i < nets; i++) {
			baddr.sin_addr = addrs[i];
			if (sendto(sock, outbuf, outlen, 0,
				(struct socketaddr *)&baddr,
				sizeof (struct sockaddr)) != outlen) {
				perror((catgets(nlmsg_fd,NL_SETN,4, "Cannot send broadcast packet")));
				stat = RPC_CANTSEND;
				goto done_broad;
			}
			/*
		 	 * A check to keep you from trying to use eachresult
		 	 * dereferenced as a function pointer.  MDS
		 	 */
                	if (eachresult == NULL) {
                        	stat = RPC_SUCCESS;
                        	goto done_broad;
                	}
		}
	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.where = (caddr_t)&r;
                msg.acpted_rply.ar_results.proc = xdr_rmtcallres;
		memcpy(&readfds, &mask, fd_size);
		switch (select(nfds, &readfds, (int *)NULL, (int *)NULL, &t)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			continue;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			perror((catgets(nlmsg_fd,NL_SETN,5, "Broadcast select problem")));
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */
/*
 *		HPNFS This checks what all the elements of readfds 	
 *		      and mask and'ed together will produce.  This 
 *		      should never happen since select only selects
 *		      on sock and if it returns other than in error
 *		      it is because there is something in sock.
 *		      This has been left here in case of a future need.
 *
 *		for (cnt = 0, sum = 0; cnt < FD_ARRAYSIZE; cnt++)
 *			sum = sum || (FD_ELEMENT(cnt, &readfds) & 
 *					FD_ELEMENT(cnt, &mask));
 *		if (sum == 0)
 *			goto recv_again;
 */
	try_again:
		fromlen = sizeof(struct sockaddr);

	/* HPNFS The code assumes that sock is the file descriptor ready */

		inlen = recvfrom(sock, inbuf, UDPMSGSIZE, 0,
			(struct sockaddr *)&raddr, &fromlen);
		if (inlen < 0) {
			if (errno == EINTR)
				goto try_again;
			perror((catgets(nlmsg_fd,NL_SETN,6, "Cannot receive reply to broadcast")));
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_long))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short)port);
				done = (*eachresult)(resultsp, &raddr);
			}
			/* otherwise, we just ignore the errors ... */
		} else {
#ifdef notdef
			/* some kind of deserialization problem ... */
			if (msg.rm_xid == xid)
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "Broadcast deserialization problem")));
			/* otherwise, just random garbage */
#endif
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		(void)(*xresults)(xdrs, resultsp);
		xdr_destroy(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
done_broad:
	(void)close(sock);
	AUTH_DESTROY(unix_auth);
	return (stat);
}
