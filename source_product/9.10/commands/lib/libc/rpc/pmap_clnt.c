/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.9.1.1 $	$Date: 94/02/03 08:02:22 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/pmap_clnt.c,v $
 * $Revision: 12.9.1.1 $	$Author: ssa $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/02/03 08:02:22 $
 *
 * Revision 12.8  90/12/12  10:50:57  10:50:57  dlr
 * Terminated an unterminated comment string in the previous version.
 * 
 * Revision 12.7  90/12/10  17:13:53  17:13:53  prabha (Prabha Chadayammuri)
 * added INADDR_LOOPBACK, backed out but left it in for 9.0
 * 
 * Revision 12.6  90/09/18  10:29:43  10:29:43  dlr (Dominic Ruffatto)
 * Cleaned up strcmp namespace polution.
 * 
 * Revision 12.5  90/08/30  15:08:01  15:08:01  dlr (Dominic Ruffatto)
 * The SO_LINGER socket option now requires a linger structure instead of
 * a long integer.  
 * 
 * Revision 12.4  90/08/30  11:56:48  11:56:48  prabha (Prabha Chadayammuri)
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.3  90/08/06  16:22:05  16:22:05  dlr (Dominic Ruffatto)
 * Changed the get_myaddress routine to skip the loopback address when
 * looking for the local internet address.
 * 
 * Revision 12.2  90/04/19  10:58:13  10:58:13  dlr (Dominic L. Ruffatto)
 * Made change to set the send and receive sizes for the UDP socket via
 * set sockopt.  This was necessary because the default was changed to 2k
 * in 8.0.
 * 
 * Revision 12.1  90/03/20  15:37:35  15:37:35  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:43  16:08:43  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/13  15:08:01  15:08:01  dds (Darren Smith)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.14  89/02/13  14:07:04  14:07:04  cmahon (Christina Mahon)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.13  89/01/26  11:59:26  11:59:26  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.12  89/01/16  15:13:57  15:13:57  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:40  14:48:40  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.5  88/07/25  11:03:07  11:03:07  chm (Cristina H. Mahon)
 * Fixed bus error with the way my_addr was being assigned.
 * 
 * Revision 1.1.10.11  88/07/25  10:02:37  10:02:37  cmahon (Christina Mahon)
 * Fixed bus error with the way my_addr was being assigned.
 * 
 * Revision 1.1.10.10  88/07/07  14:44:39  14:44:39  cmahon (Christina Mahon)
 * Fixed DTS CNDdm01548.  Nfsd would core dump when trying to do a pmap_unset
 * and the portmap was not ready to handle it.  The reason for that was that
 * it was trying to use an integer that had no space allocated for it.
 * 
 * Revision 1.1.10.9  88/05/11  14:39:08  14:39:08  cmahon (Christina Mahon)
 * Fixed enhancement request CNDdm01191.  If portmap is down pmap_unset
 * will return immediately instead of waiting for a long time to timeout.
 * 
 * 
 * Revision 1.1.10.8  88/03/24  16:50:48  16:50:48  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use a front end for all NFS libc
 * routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.7  87/11/06  15:17:54  15:17:54  cmahon (Christina Mahon)
 * Added check to see if sockp is NULL and return in that case (instead
 * of core dumping).
 * Resolved CNOdm00772.
 * 
 * Revision 1.1.10.6  87/10/14  09:53:05  09:53:05  cmahon (Christina Mahon)
 * Removed an error message that should not be printed since the
 * programs that invoke the library routine can handle the error gracefully.
 * Changed the numbers of the messages for the message catalog so they are
 * continuous.
 * This fixes DTS CNOdm00660.
 * 
 * Revision 1.1.10.5  87/09/22  12:06:27  12:06:27  cmahon (Christina Mahon)
 * Added NLS message catalog support to two different messages that didn't
 * have it.
 * 
 * Revision 1.1.10.4  87/08/07  14:51:34  14:51:34  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  10:55:53  10:55:53  cmahon (Christina Mahon)
 * Update to latest version from CND.  Added gettransient() function.
 * 
 * Revision 1.1.10.2  87/07/16  22:11:14  22:11:14  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:40:03  11:40:03  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: pmap_clnt.c,v 12.9.1.1 94/02/03 08:02:22 ssa Exp $ (Hewlett
-Packard)";
#endif

/*
 * pmap_clnt.c
 * Client interface to pmap rpc service.
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
#define catgets 		_catgets
#define clnt_perror 		_clnt_perror
#define clntudp_bufcreate 	_clntudp_bufcreate
#define close 			_close
#define connect 		_connect
#define get_myaddress 		_get_myaddress		/* In this file */
#define getpid 			_getpid
#define getsockname 		_getsockname
#define gettransient 		_gettransient		/* In this file */
#define inet_addr 		_inet_addr
#define ioctl 			_ioctl
#define memset 			_memset
#define perror 			_perror
#define pmap_set 		_pmap_set		/* In this file */
#define pmap_unset 		_pmap_unset		/* In this file */
#define setsockopt 		_setsockopt
#define socket 			_socket
#define strcmp			_strcmp
#define xdr_bool 		_xdr_bool
#define xdr_pmap 		_xdr_pmap

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 9	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <rpc/rpc.h>		/* bring in lots of definitions */
#include <rpc/pmap_prot.h> 
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>
#define NAMELEN 255

static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };
static struct sockaddr_in myaddress;

void clnt_perror();
bool check_pmap_up();

/*
 * Set a mapping between program,version and port.
 * Calls the pmap service remotely to do the mapping.
 */

#ifdef _NAMESPACE_CLEAN
#undef pmap_set
#pragma _HP_SECONDARY_DEF _pmap_set pmap_set
#define pmap_set _pmap_set
#endif

bool_t
pmap_set(program, version, protocol, port)
	u_long program;
	u_long version;
	u_long protocol;
	u_short port;
{
	struct sockaddr_in myaddress;
	int socket = -1;
	register CLIENT *client;
	struct pmap parms;
	bool_t rslt;

	nlmsg_fd = _nfs_nls_catopen();

        /* setup your own address rather than calling get_myaddress 10.0 */

        myaddress.sin_family = AF_INET;
        myaddress.sin_addr.s_addr = INADDR_LOOPBACK;
        myaddress.sin_port =  htons(PMAPPORT);


	client = clntudp_bufcreate(&myaddress, PMAPPROG, PMAPVERS,
	    timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);

	if (client == (CLIENT *)NULL)
		return (FALSE);

	parms.pm_prog = program;
	parms.pm_vers = version;
	parms.pm_prot = protocol;
	parms.pm_port = port;
	if (CLNT_CALL(client, PMAPPROC_SET, xdr_pmap, &parms, xdr_bool, &rslt,
	    tottimeout) != RPC_SUCCESS) {
		clnt_perror(client, (catgets(nlmsg_fd,NL_SETN,1, "Cannot register service")));
		return (FALSE);
	}
	CLNT_DESTROY(client);
	(void)close(socket);
	return (rslt);
}

/*
 * Remove the mapping between program,version and port.
 * Calls the pmap service remotely to do the un-mapping.
 */

#ifdef _NAMESPACE_CLEAN
#undef pmap_unset
#pragma _HP_SECONDARY_DEF _pmap_unset pmap_unset
#define pmap_unset _pmap_unset
#endif

bool_t
pmap_unset(program, version)
	u_long program;
	u_long version;
{
	struct sockaddr_in myaddress;
	int socket = -1;
	register CLIENT *client;
	struct pmap parms;
	bool_t rslt;
	int err;

	if (check_pmap_up(&myaddress, &err))
	{

        /* setup your own address rather than calling get_myaddress 10.0 */
		myaddress.sin_family = AF_INET; 
		myaddress.sin_addr.s_addr = INADDR_LOOPBACK;
                myaddress.sin_port =  htons(PMAPPORT);


		client = clntudp_bufcreate(&myaddress, PMAPPROG, PMAPVERS,
		    timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
		if (client == (CLIENT *)NULL)
			return (FALSE);

		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_port = parms.pm_prot = 0;
		CLNT_CALL(client, PMAPPROC_UNSET, xdr_pmap, &parms, xdr_bool, 
			  &rslt, tottimeout);
		CLNT_DESTROY(client);
		(void)close(socket);
		return (rslt);
	}
	else
		return(TRUE);
}

/* 
 * don't use gethostbyname, which would invoke network information service
 */

#define LOOPBACKNAME "lo0"

#ifdef _NAMESPACE_CLEAN
#undef get_myaddress
#pragma _HP_SECONDARY_DEF _get_myaddress get_myaddress
#define get_myaddress _get_myaddress
#endif

get_myaddress(addr)
	struct sockaddr_in *addr;
{
	int s;
	char buf[1024];	     /* Was BUFSIZ == 8K which is more than we need */
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	int len;

	nlmsg_fd = _nfs_nls_catopen();
	/*
	**	clear the array in case we run into an error -- we will
	**	return an empty struct sockadd_in instead of exiting!!
	*/
	memset((char *)addr, 0, sizeof(struct sockaddr_in));

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    return;
	}
	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,2, "get_myaddress: ioctl (get interface configuration)")));
		return;
	}
	ifr = ifc.ifc_req;
	for (len = ifc.ifc_len; len; len -= sizeof ifreq) {
		ifreq = *ifr;
		if (ioctl(s, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
			perror((catgets(nlmsg_fd,NL_SETN,3, "get_myaddress: ioctl")));
			return;
		}
		if ((ifreq.ifr_flags & IFF_UP) &&
		    (ifr->ifr_addr.sa_family == AF_INET) &&
		    (strcmp(ifr->ifr_name,LOOPBACKNAME) != 0)) {
			*addr = *((struct sockaddr_in *)&ifr->ifr_addr);
			addr->sin_port = htons(PMAPPORT);
		}
		ifr++;
	}
	close(s);
	return;
}



/*	HPNFS	jad	87.08.03	value-added
**	added gettransient() to pmap_clnt.c; already documented ...
****
**	gettransient(protocol, version, *sockp)
**	returns 0 or a valid program number registered with the given
**	version, using the given protocol.  The socket itself is returned
**	in *sockp, if it is passed in as RPC_ANYSOCK.
*/

#ifdef _NAMESPACE_CLEAN
#undef gettransient
#pragma _HP_SECONDARY_DEF _gettransient gettransient
#define gettransient _gettransient
#endif

gettransient(proto, vers, sockp)
int proto, vers, *sockp;
{
    static int prognum = 0;
    int s, len, socktype, msgsize;
    struct sockaddr_in addr;
    /*
    **	get a socket of the appropriate type (DGRAM or STREAM),
    **	bind it, pmap set the newly bound socket and return the
    **	prognum of the new service ...
    */
    switch (proto) {
	case IPPROTO_UDP:			/* UDP datagram socket	*/
	    socktype = SOCK_DGRAM;
	    break;
	case IPPROTO_TCP:			/* TCP stream socket	*/
	    socktype = SOCK_STREAM;
	    break;
	default:				/* unknown socket type	*/
	    return(0);
    }
    if (sockp == NULL)
	return(0);
    if (*sockp == RPC_ANYSOCK) {
	if ((s = socket(AF_INET, socktype, 0)) < 0) {
	    perror((catgets(nlmsg_fd,NL_SETN,4, "socket")));
	    return(0);
	}
	*sockp = s;
    } else
	s = *sockp;

    msgsize = UDPMSGSIZE;
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &msgsize, sizeof(int)) == -1){
	perror((catgets(nlmsg_fd,NL_SETN,4, "setsockopt")));
	close(s);
	return (0);
    }
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &msgsize, sizeof(int)) == -1){
	perror((catgets(nlmsg_fd,NL_SETN,4, "setsockopt")));
	close(s);
	return (0);
    }

    /*
    **	set up the socket address structure using any address, the
    **	Internet address family, and let the system choose the port.
    */
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    len = sizeof(addr);
    /*
    **	the socket may already be bound, so don't worry about errors
    */
    (void) bind(s, &addr, len);
    if (getsockname(s, &addr, &len) < 0) {
	perror((catgets(nlmsg_fd,NL_SETN,5, "getsockname")));
	return(0);
    }
    /*
    **	if prognum has not been initialized, then set it to the base of
    **	the transient range plus 64*pid; this gives each pid a range of
    **	64 prognum's it can register before it has to worry about a hit
    **	of an already registered transient port.
    */
    if (prognum == 0)
	prognum = 0x40000000 + 64*getpid();
    /*
    **	keep looping until we get a port number we can use ...
    */
    while (!pmap_set(++prognum, vers, proto, addr.sin_port))
	continue;
    /*
    **	if we fall out we have finally pmap_set a valid prog,vers,proto
    **	triple; return the prognum to the user.
    */
    return (prognum);
}

/*
 * This checks whether the portmapper is up.  If the connect fails, the
 * portmapper is dead.  As a side effect, the pmapper sockaddr_in is
 * initialized.  The connection is not used for anything else, so it is
 * immediately closed.  
 * NOTE: This routine is very similar to the routine with the same name 
 *	 in the file yp_bind.c.  Any changes made to this routine should 
 *	 be made to the one in yp_bind.c
 */
static bool
check_pmap_up(pmapper, err)
	struct sockaddr_in *pmapper;
	int *err;
{
	int sokt, len = sizeof(struct linger);
	struct linger linger;

#ifdef NOTDEF
	pmapper->sin_addr.s_addr = INADDR_THISHOST;
#else /* not NOTDEF */
	pmapper->sin_addr.s_addr = inet_addr("127.0.0.1");
#endif /* NOTDEF */
	pmapper->sin_family = AF_INET;
	pmapper->sin_port = PMAPPORT;
	memset(pmapper->sin_zero, 0, 8);
	sokt =  socket(AF_INET, SOCK_STREAM, 0);

	if (sokt == -1) {
		*err = YPERR_RESRC;
		return (FALSE);
	} else {
		/*	HPNFS	jad	87.07.07
		**	added SO_LINGER(0) to force a close of the socket
		**	without a TIME_WAIT state; this function gets called
		**	a lot, and can leave *many* TIME_WAIT states ...
		*/
		linger.l_onoff = 1;
		linger.l_linger = 0;
		setsockopt(sokt,SOL_SOCKET,SO_LINGER,&linger,len);
	}

	if (connect(sokt, (struct sockaddr *) pmapper,
	    sizeof(struct sockaddr_in)) < 0) {
		(void) close(sokt);
		*err = YPERR_PMAP;
		return (FALSE);
	}

	(void) close(sokt);
	return (TRUE);
}

