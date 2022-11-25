/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.6 $	$Date: 92/02/07 17:08:02 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/clnt_udp.c,v $
 * $Revision: 12.6 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:02 $
 *
 * Revision 12.5  90/12/10  17:16:16  17:16:16  prabha
 * changed to select on the required sock only.
 * 
 * Revision 12.4  90/08/30  11:55:30  11:55:30  prabha (Prabha Chadayammuri)
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.3  90/06/08  10:27:45  10:27:45  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.2  90/04/19  10:57:39  10:57:39  dlr (Dominic Ruffatto)
 * Made change to set the send and receive sizes for the UDP socket via
 * set sockopt.  This was necessary because the default was changed to 2k
 * in 8.0.
 * 
 * Revision 12.1  90/01/31  11:53:03  11:53:03  dlr (Dominic Ruffatto)
 * Cleaned up comments after #else and #endif lines for ANSII-C.  Also made
 * 
 * changes for >64 file descriptors.  These changes are currently ifdef'd
 * with the GT_64_FDS flag.
 * 
 * Revision 12.0  89/09/25  16:08:35  16:08:35  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  12:58:41  12:58:41  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.15  89/01/26  11:57:43  11:57:43  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.14  89/01/16  15:12:35  15:12:35  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:29  14:48:29  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.9  88/08/30  13:25:36  13:25:36  mikey (Mike Shipley)
 * Added some comments on timeout fields in cu_data
 * 
 * Revision 1.1.10.13  88/08/30  12:24:25  12:24:25  cmahon (Christina Mahon)
 * Added some comments on timeout fields in cu_data
 * 
 * Revision 1.1.10.12  88/08/05  12:53:59  12:53:59  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.11  88/06/24  16:18:41  16:18:41  cmahon (Christina Mahon)
 * Updating to the 3.9 level of functionality.  Changes are in ifdef's.
 * 
 * Revision 1.1.10.10  88/06/22  09:36:36  09:36:36  cmahon (Christina Mahon)
 * Missed part of fix for DTS CNDdm01434 (copying instead of memcpy).  So
 * added fix to that now.
 * 
 * Revision 1.1.10.9  88/06/22  08:25:18  08:25:18  cmahon (Christina Mahon)
 * Changes made to fix DTS CNDdm01434.  These changes allow select to handle
 * more than just 32 file descriptors.
 * 
 * Revision 1.1.10.8  88/05/06  16:02:43  16:02:43  cmahon (Christina Mahon)
 * added ifdef NFS3_2 around last additions
 * 
 * Revision 1.1.10.7  88/05/06  09:43:06  09:43:06  cmahon (Christina Mahon)
 * added Sun NFS 3.2 change to provide rpc-based message passing
 * by specifying a timeout of zero
 * 
 * Revision 1.1.10.6  88/03/24  16:46:37  16:46:37  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS
 * libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.5  87/11/17  09:58:30  09:58:30  cmahon (Christina Mahon)
 * Made clntudp_bufcreate() avoid the use of 0,1, or 2 as socket decriptors.
 * This is becuase this was causing a problem with csh/yp, and it seems to be
 * generally a good idea to avoid the use of stdin, stdout, and stderr.  dds.
 * 
 * Revision 1.1.10.4  87/09/25  13:14:37  13:14:37  cmahon (Christina Mahon)
 * fixed message in TRACE statement from "decode" to "encode"
 * 
 * Revision 1.1.10.3  87/08/07  14:47:30  14:47:30  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:09:18  22:09:18  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:39:36  11:39:36  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_udp.c,v 12.6 92/02/07 17:08:02 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * clnt_udp.c, Implements a UPD/IP based, client side RPC.
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

#define authnone_create 	_authnone_create
#define bindresvport 		_bindresvport
#define catgets 		_catgets
#define clntudp_bufcreate 	_clntudp_bufcreate	/* In this file */
#define clntudp_create 		_clntudp_create		/* In this file */
#define close 			_close
#define fcntl 			_fcntl
#define fprintf 		_fprintf
#define getpid 			_getpid
#define gettimeofday 		_gettimeofday
#define ioctl 			_ioctl
#define memcpy 			_memcpy
#define memset 			_memset
#define pmap_getport 		_pmap_getport
#define recvfrom 		_recvfrom
#define rpc_createerr 		_rpc_createerr
#define select 			_select
#define sendto 			_sendto
#define setsockopt		_setsockopt
#define socket 			_socket
#define xdr_callhdr 		_xdr_callhdr
#define xdr_opaque_auth 	_xdr_opaque_auth
#define xdr_replymsg 		_xdr_replymsg
#define xdrmem_create 		_xdrmem_create

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 8	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <rpc/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_clnt.h>
#include <fcntl.h>
#include <sys/types.h>

#include <arpa/trace.h>

char *malloc();
extern int errno;

/*
 * UDP bases client side rpc operations
 */
static enum clnt_stat	clntudp_call();
static void		clntudp_abort();
static void		clntudp_geterr();
static bool_t		clntudp_freeres();
static bool_t		clntudp_control();
static void		clntudp_destroy();

static struct clnt_ops udp_ops = {
	clntudp_call,
	clntudp_abort,
	clntudp_geterr,
	clntudp_freeres,
	clntudp_destroy,
	clntudp_control
};

/* 
 * Private data kept per client handle
 */
struct cu_data {
	int		   cu_sock;
	bool_t	           cu_closeit;
	struct sockaddr_in cu_raddr;
	int		   cu_rlen;
	struct timeval	   cu_wait;
	struct timeval	   cu_total;
	struct rpc_err	   cu_error;
	XDR		   cu_outxdrs;
	u_int		   cu_xdrpos;
	u_int		   cu_sendsz;
	char		   *cu_outbuf;
	u_int		   cu_recvsz;
	char		   cu_inbuf[1];
};

/*
 * Some notes on the timeout fields in the cu_data.  The reason why it
 * is confusing is that there is a ct_wait field for tcp and a cu_wait
 * and cu_total field for udp, but the purpose for the ct_wait field
 * is not similar to that of the cu_wait field.  The ct_wait field
 * is similar to the cu_total field.  So strange.
 * Anyway, I looked over the fields and got the following info for
 * udp
 *
 * UDP-
  cu_wait  NOT a new field
           set in clntudp_bufcreate() as a user specified value from
	   clntudp_create()
	   set by CLSET_RETRY_TIMEOUT
	   retrieved by CLGET_RETRY_TIMEOUT

	   tells select() how long to wait before returning without
	   getting something on the sockets


  cu_total  NEW for 3.2
	    initialized to -1 in clntudp_bufcreate()
	    set by CLSET_TIMEOUT  
	    retrieved by CLGET_TIMEOUT

	    in clntudp_call(), if -1 then timeout is set to the value passed
	    into clntudp_call().  Otherwise the value of timeout is set
	    to that of cu_total.  Once set by clnt_control, values passed in
	    to clntudp_call have no effect

	    controls how long the clntudp_call should wait before quitting
	    it is compared with the sum of all of the timeouts from the select()

 */

/*
 * Create a UDP based client handle.
 * If *sockp<0, *sockp is set to a newly created UPD socket.
 * If raddr->sin_port is 0 a binder on the remote machine
 * is consulted for the correct port number.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is initialized to null authentication.
 *     Caller may wish to set this something more useful.
 *
 * wait is the amount of time used between retransmitting a call if
 * no response has been heard;  retransmition occurs until the actual
 * rpc call times out.
 *
 * sendsz and recvsz are the maximum allowable packet sizes that can be
 * sent and received.
 */

#ifdef _NAMESPACE_CLEAN
#undef clntudp_bufcreate
#pragma _HP_SECONDARY_DEF _clntudp_bufcreate clntudp_bufcreate
#define clntudp_bufcreate _clntudp_bufcreate
#endif

CLIENT *
clntudp_bufcreate(raddr, program, version, wait, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl;
	register struct cu_data *cu;
	struct timeval now;
	struct rpc_msg call_msg;

	nlmsg_fd = _nfs_nls_catopen();

	TRACE("clntudp_bufcreate SOP");
	cl = (CLIENT *)mem_alloc(sizeof(CLIENT));
	if (cl == NULL) {
		TRACE("clntudp_bufcreate first mem_alloc failed");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "clntudp_create: out of memory\n")));
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}
	sendsz = ((sendsz + 3) / 4) * 4;
	recvsz = ((recvsz + 3) / 4) * 4;
	TRACE3("clntudp_bufcreate sendsz = %d, recvsz = %d", sendsz, recvsz);
	cu = (struct cu_data *)mem_alloc(sizeof(*cu) + sendsz + recvsz);
	if (cu == NULL) {
		TRACE("clntudp_bufcreate second mem_alloc failed");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "clntudp_create: out of memory\n")));
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto fooy;
	}
	cu->cu_outbuf = &cu->cu_inbuf[recvsz];

	(void)gettimeofday(&now, (struct timezone *)0);
	TRACE2("clntudp_bufcreate timenow = %ld", now.tv_sec);
	if (raddr->sin_port == 0) {
		u_short port;
		TRACE("clntudp_bufcreate raddr->sin_port = 0, call getport");
		if ((port =
		    pmap_getport(raddr, program, version, IPPROTO_UDP)) == 0) {
			TRACE("clntudp_bufcreate pmap_getport returned port=0");
			goto fooy;
		}
		TRACE2("clntudp_bufcreate pmap_getport returned port=%d",port);
		raddr->sin_port = htons(port);
	}
	cl->cl_ops = &udp_ops;
	cl->cl_private = (caddr_t)cu;
	cu->cu_raddr = *raddr;
	cu->cu_rlen = sizeof (cu->cu_raddr);
	cu->cu_wait = wait;
        cu->cu_total.tv_sec = -1;
        cu->cu_total.tv_usec = -1;
	cu->cu_sendsz = sendsz;
	cu->cu_recvsz = recvsz;
	call_msg.rm_xid = getpid() ^ now.tv_sec ^ now.tv_usec;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = program;
	call_msg.rm_call.cb_vers = version;
	TRACE("clntudp_bufcreate about to call xdrmem_create");
	xdrmem_create(&(cu->cu_outxdrs), cu->cu_outbuf,
	    sendsz, XDR_ENCODE);
	TRACE("clntudp_bufcreate about to call xdr_callhdr");
	if (! xdr_callhdr(&(cu->cu_outxdrs), &call_msg)) {
		TRACE("clntudp_bufcreate xdr_callhdr failed");
		goto fooy;
	}
	cu->cu_xdrpos = XDR_GETPOS(&(cu->cu_outxdrs));
	if (*sockp < 0) {
		int dontblock = 1;
		*sockp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		TRACE2("clntudp_bufcreate sockp<0, got new socket %d", *sockp);
		if (*sockp < 0) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			goto fooy;
		}
		if (setsockopt(*sockp, SOL_SOCKET, SO_RCVBUF, &recvsz, sizeof(u_int)) == -1) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			goto fooy;
                }
		if (setsockopt(*sockp, SOL_SOCKET, SO_SNDBUF, &sendsz, sizeof(u_int)) == -1) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			goto fooy;
                }
                /* 
		 * Attempt to bind to prov port 
		 * If the caller is a non-su, then the bindresvport just
   		 * fails quietly 
		 */
                (void)bindresvport(*sockp, (struct sockaddr_in *)0);
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
				close (*sockp);
				goto fooy;
			}
			(void) close (*sockp);
			*sockp = newfd;
		}
		/* the sockets rpc controls are non-blocking */
		(void)ioctl(*sockp, FIONBIO, &dontblock);
		cu->cu_closeit = TRUE;
	} else {
		cu->cu_closeit = FALSE;
	}
	cu->cu_sock = *sockp;
	cl->cl_auth = authnone_create();
	TRACE2("clntudp_bufcreate returning OK with 0x%x", cl);
	return (cl);
fooy:
	if (cu)
		mem_free((caddr_t)cu, sizeof(*cu) + sendsz + recvsz);
	if (cl)
		mem_free((caddr_t)cl, sizeof(CLIENT));
	TRACE("clntudp_bufcreate returning NOTOK");
	return ((CLIENT *)NULL);
}

#ifdef _NAMESPACE_CLEAN
#undef clntudp_create
#pragma _HP_SECONDARY_DEF _clntudp_create clntudp_create
#define clntudp_create _clntudp_create
#endif

CLIENT *
clntudp_create(raddr, program, version, wait, sockp)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
{
	return(clntudp_bufcreate(raddr, program, version, wait, sockp,
	    UDPMSGSIZE, UDPMSGSIZE));
}


static enum clnt_stat 
clntudp_call(cl, proc, xargs, argsp, xresults, resultsp, utimeout)
	register CLIENT	*cl;		/* client handle */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	struct timeval	utimeout;	/* seconds to wait before giving up */
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs;
	register int outlen;
	register int inlen;
	int fromlen, cu_sock = cu->cu_sock;
	int nfds = cu_sock+1;
	int fd_size = (howmany(nfds, NFDBITS) << 2);
	fd_set readfds;
	fd_set mask;
	struct sockaddr_in from;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	struct timeval time_waited;
	bool_t ok;
	bool_t sum;
	int cnt;
	int nrefreshes = 2;
	struct timeval timeout;
        if (cu->cu_total.tv_usec == -1) {
                timeout = utimeout;     /* use supplied timeout */
        } else {
                timeout = cu->cu_total; /* use default timeout */
        }

	TRACE("clntudp_call SOP");
	time_waited.tv_sec = 0;
	time_waited.tv_usec = 0;
call_again:
	xdrs = &(cu->cu_outxdrs);
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, cu->cu_xdrpos);
	TRACE2("clntudp_call initialized, timeout=%f",
		(float)timeout.tv_sec + (float)timeout.tv_usec/1000000.0);
	/*
	 * the transaction is the first thing in the out buffer
	 */
	(*(u_short *)(cu->cu_outbuf))++;
	if ((! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp))) {
		TRACE2("clntudp_call can't encode args(%d)",RPC_CANTENCODEARGS);
		return (cu->cu_error.re_status = RPC_CANTENCODEARGS);
	}
	outlen = (int)XDR_GETPOS(xdrs);
	while (TRUE) {
		int	selected;
		/*	HPNFS	jad	87.04.29
		**	added "selected" variable to check the return
		**	value of select -- seems to be returning
		**	something other than 0, -1, or readfds ...
		*/
		if (sendto(cu_sock, cu->cu_outbuf, outlen, 0,
		    (struct sockaddr *)&(cu->cu_raddr), cu->cu_rlen)
		    != outlen) {
			cu->cu_error.re_errno = errno;
			TRACE2("clntudp_call can't send (%d)",RPC_CANTSEND);
			return (cu->cu_error.re_status = RPC_CANTSEND);
		}
		TRACE2("clntudp_call sendto sent %d bytes", outlen);
		TRACE2("clntudp_call to in_addr=0x%lx",
			(cu->cu_raddr).sin_addr);
		TRACE3("clntudp_call with sin_family=%d, sin_port=%d",
			(cu->cu_raddr).sin_family, (cu->cu_raddr).sin_port);
          	/*
 	         * SUN Hack to provide rpc-based message passing.
		 * This allows us to send a message and not wait for 
		 * a response.
 	        */
          	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
 	         	return (cu->cu_error.re_status = RPC_TIMEDOUT);
          	}
		/*
		 * sub-optimal code appears inside the loop because we have
		 * some clock time to spare while the packets are in flight.
		 * (We assume that this is actually only executed once.)
		 */
		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = resultsp;
		reply_msg.acpted_rply.ar_results.proc = xresults;
		FD_ZERO(&mask);
		FD_SET(cu_sock, &mask);
rcv_again:
		memcpy(&readfds, &mask, fd_size);

		/*
		 * These trace statemsnts should be modified to print the
		 * whole fds_bits mask; not just two integers worth
		 */

		TRACE3("clntudp_call *enter select, readfds1=0x%x, readfds2=0x%", readfds.fds_bits[1], readfds.fds_bits[2]);
		selected = select(nfds, &readfds, (int *)NULL, 
					(int *)NULL, &(cu->cu_wait));
		TRACE3("clntudp_call *exit select, selected=%d, readfds1=0x%x",
	    		selected, readfds.fds_bits[1]);
		TRACE2("clntudp_call *exit select, readfds2=0x%x", 
			readfds.fds_bits[2]);

		switch (selected) {

		case 0:
			TRACE("clntudp_call select timed out");
			time_waited.tv_sec += cu->cu_wait.tv_sec;
			time_waited.tv_usec += cu->cu_wait.tv_usec;
			while (time_waited.tv_usec >= 1000000) {
				time_waited.tv_sec++;
				time_waited.tv_usec -= 1000000;
			}
			TRACE2("clntudp_call time_waited=%f",
			    (float)time_waited.tv_sec +
			    (float)time_waited.tv_usec/1000000.0);
			if ((time_waited.tv_sec < timeout.tv_sec) ||
				((time_waited.tv_sec == timeout.tv_sec) &&
				(time_waited.tv_usec < timeout.tv_usec)))
				continue;
			TRACE2("clntudp_call timed out (%d)", RPC_TIMEDOUT);
			return (cu->cu_error.re_status = RPC_TIMEDOUT);

		case -1:
			TRACE2("clntudp_call select error, errno=%d",errno);
			if (errno == EINTR)
				goto rcv_again;
			cu->cu_error.re_errno = errno;
			TRACE2("clntudp_call returning %d", RPC_CANTRECV);
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}

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
 *			goto rcv_again;
*/
tryagain:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(cu->cu_sock, cu->cu_inbuf, cu->cu_recvsz, 0,
		    (struct sockaddr *)&from, &fromlen);
		TRACE2("clntudp_call recvfrom got %d bytes", inlen);
		if (inlen < 0) {
			TRACE2("clntudp_call recvfrom<0 errno=%d", errno);
			if (errno == EINTR)
				goto tryagain;
			if (errno == EWOULDBLOCK)
				goto rcv_again;
			cu->cu_error.re_errno = errno;
			TRACE2("clntudp_call returning %d", RPC_CANTRECV);
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}
		if (inlen < sizeof(u_long))
			goto rcv_again;
		TRACE2("clntudp_call from in_addr=0x%lx", from.sin_addr);
		TRACE3("clntudp_call with sin_family=%d, sin_port=%d",
				from.sin_family, from.sin_port);
		/* see if reply transaction id matches sent id */
		if (*((u_long *)(cu->cu_inbuf)) != *((u_long *)(cu->cu_outbuf)))
			goto rcv_again;
		/* we now assume we have the proper reply */
		TRACE("clntudp_call transaction ID matches sent ID");
		break;
	}

	/*
	 * now decode and validate the response
	 */
	TRACE("clntudp_call about to call xdrmem_create");
	xdrmem_create(&reply_xdrs, cu->cu_inbuf, (u_int)inlen, XDR_DECODE);
	TRACE("clntudp_call about to call xdr_replymsg");
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	TRACE2("clntudp_call return from xdr_replymsg=%d", ok);
	/* XDR_DESTROY(&reply_xdrs);  save a few cycles on noop destroy */
	if (ok) {
		_seterr_reply(&reply_msg, &(cu->cu_error));
		if (cu->cu_error.re_status == RPC_SUCCESS) {
			if (! AUTH_VALIDATE(cl->cl_auth,
				&reply_msg.acpted_rply.ar_verf)) {
				cu->cu_error.re_status = RPC_AUTHERROR;
				cu->cu_error.re_why = AUTH_INVALIDRESP;
				TRACE("clntudp_call AUTHERROR INVALIDRESP");
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				xdrs->x_op = XDR_FREE;
				(void)xdr_opaque_auth(xdrs,
				    &(reply_msg.acpted_rply.ar_verf));
			} 
		TRACE("clntudp_call OK, end successful completion");
		}  /* end successful completion */
		else {
			TRACE("clntudp_call creds may need to be refreshed");
			if (
		             nrefreshes-- &&
			    AUTH_REFRESH(cl->cl_auth))
				goto call_again;
			TRACE("clntudp_call end of unsuccessful completion");
		}  /* end of unsuccessful completion */
		TRACE("clntudp_call end of valid reply message");
	}  /* end of valid reply message */
	else {
		TRACE("clntudp_call re_status = RPC_CANTDECODERES");
		cu->cu_error.re_status = RPC_CANTDECODERES;
	}
	TRACE2("clntudp_call returning %d", cu->cu_error.re_status);
	return (cu->cu_error.re_status);
}

static void
clntudp_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	TRACE("clntudp_geterr SOP");
	*errp = cu->cu_error;
}


static bool_t
clntudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs = &(cu->cu_outxdrs);

	TRACE("clntudp_freeres SOP");
	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

static void 
clntudp_abort(/*h*/)
	/*CLIENT *h;*/
{
	TRACE("clntudp_abort SOP");
}

static bool_t
clntudp_control(cl, request, info)
        CLIENT *cl;
        int request;
        char *info;
{
        register struct cu_data *cu = (struct cu_data *)cl->cl_private;

        switch (request) {
        case CLSET_TIMEOUT:
                cu->cu_total = *(struct timeval *)info;
                break;
        case CLGET_TIMEOUT:
                *(struct timeval *)info = cu->cu_total;
                break;
        case CLSET_RETRY_TIMEOUT:
                cu->cu_wait = *(struct timeval *)info;
                break;
        case CLGET_RETRY_TIMEOUT:
                *(struct timeval *)info = cu->cu_wait;
                break;
        case CLGET_SERVER_ADDR:
                *(struct sockaddr_in *)info = cu->cu_raddr;
                break;
        default:
                return (FALSE);
        }
        return (TRUE);
}

static void
clntudp_destroy(cl)
	CLIENT *cl;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

        if (cu->cu_closeit) {
                (void)close(cu->cu_sock);
        }
	TRACE("clntudp_destroy SOP");
	XDR_DESTROY(&(cu->cu_outxdrs));
	mem_free((caddr_t)cu, (sizeof(*cu) + cu->cu_sendsz + cu->cu_recvsz));
	mem_free((caddr_t)cl, sizeof(CLIENT));
}
