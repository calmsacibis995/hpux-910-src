/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_kudp.c,v $
 * $Revision: 12.0 $	$Author: nfsmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/09/25 16:09:15 $
 */

/* BEGIN_IMS
******************************************************************************
****								
****  svc_kudp.c - NFS kernel only server routines
****								
******************************************************************************
* Description
*	The routines in this file handle server side rpc request handling
*	in the kernel.
*
* Externally Callable Routines
*	svckudp_create	- create an in-kernel UDP tranport handle
*	svckudp_destory - destroy an in-kernel UDP tranport handle
*	svckudp_recv	- recieve and deserialize an rpc call packet
*	svckudp_send	- serialize and send an rpc reply packet
*	svckudp_stat	- return status of the transport handle (idle)
*	svckudp_getargs	- deserialize call arguments
*	svckudp_freeargs- free argument structures
*	svckudp_dupsave	- add a request to the good request cache
*	svckudp_dup	- check to see if a request is already cached
*      
*
* Test Module
*	$scaffold/nfs.bb
*
* To Do List
*
* Notes
*
* Modification History
*	11/04/87	ew	added comments
*	11/19/87	lmb	added global variable nfs_macct
*
******************************************************************************
* END_IMS */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) $Header: svc_kudp.c,v 12.0 89/09/25 16:09:15 nfsmgr Exp $";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../h/ns_diag.h"
#include "../h/trigdef.h"			/* required for triggers */

#define rpc_buffer(xprt) ((xprt)->xp_p1)	/* outbound xdr buffer	*/

/*
 * Routines exported through ops vector.
 */
bool_t		svckudp_recv();
bool_t		svckudp_send();
enum xprt_stat	svckudp_stat();
bool_t		svckudp_getargs();
bool_t		svckudp_freeargs();
void		svckudp_destroy();

/*
 * Server transport operations vector.
 */
struct xp_ops svckudp_op = {
	svckudp_recv,		/* Get requests */
	svckudp_stat,		/* Return status */
	svckudp_getargs,	/* Deserialize arguments */
	svckudp_send,		/* Send reply */
	svckudp_freeargs,	/* Free argument data space */
	svckudp_destroy		/* Destroy transport handle */
};


struct mbuf	*ku_recvfrom();
void		xdrmbuf_init();

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	int	ud_flags;			/* flag bits, see below */
	u_long 	ud_xid;				/* id */
	struct	mbuf *ud_inmbuf;		/* input mbuf chain */
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
};


/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

/*
 * macct for nfs - Used by ku_recvfrom for charging inbound mbufs,
 * created in svckudp_create or clntkudp_create. Both of these
 * routines expand the nfs macct for each new server or client handle.
 */
struct	macct	*nfs_macct = NULL;

#define MBUFS_PER_SERVER ((UDPMSGSIZE/MLEN) * 2)


/* BEGIN_IMS svckudp_create *
 ********************************************************************
 ****
 ****	svckudp_create(sock, port)
 ****	struct socket	*sock;
 ****		u_short		 port;
 ****
 ********************************************************************
 * Input Parameters
 *	sock		- socket to be associated with this handle
 *	port		- port number associated with the socked
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	address		- of the created transport handle
 *	NULL		- failure due to creating macct
 *
 * Globals Referenced
 *
 * Description
 *
 * Algorithm
 *      if ( not enough socket buffers )
 *		expand input socket buffer queue length
 *	if (the nfs macct exists)
 *		expand the macct MBUF_PER_SERVER credits
 *	else {
 *		create the nfs macct	
 *		if (create fails)
 *			return(NULL)
 *	}
 *	allocate memory for xprt structure
 *	allocate a "large buffer" and link it to the xprt
 *	allocate a udp structure and link it to the xprt
 *	fill in ops, port, and socket fields of xprt
 *	return xprt
 *   
 * Modification History
 *	11/06/87	ew	added coments
 *	11/19/87	lmb	added nfs macct code
 *      03/10/88	dds	added expansion of socket inbound queue
 *
 * Notes
 * The transport record, output buffer, and private data structure
 * are allocated.  The output buffer is serialized into using xdrmem.
 * There is one transport record per user process which implements a
 * set of services.
 *
 * To Do List
 *	There is currently no error checking for kmem_alloc failures.
 *	Add trigger and check return value of sbreserve and log a message
 *		in case of failure.
 *
 * External Calls
 *	kmem_alloc		allocate kernel memory
 *	bzero			zero fill memory
 *	
 * Called By
 *	nfs_svc
 *
 ********************************************************************
 * END_IMS svc_kudp_create */

int udp_min_burst_in_nfs = 6;	  /* Six is how many client structs there are*/

SVCXPRT *
svckudp_create(sock, port)
	struct socket	*sock;
	u_short		 port;
{
	register SVCXPRT	 *xprt;
	register struct udp_data *ud;
	register int s;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_create so = %x, port = %d\n", sock, port);
#endif
#ifdef hp9000s200
	/*
	 * Check how many packets can be queued on the inbound socket
	 * buffer.  We want to have at least enough to handle all the
	 * client processes from one client machine to be able to be queued
	 * at once.  Since we and Sun now have 6 clients, we should
	 * allow at least that many packets to be queued to avoid
	 * packets getting dropped unneccesarily. dds.
	 */
	if ( sock->so_rcv.sb_maxmsgs < udp_min_burst_in_nfs ) {
		register struct sockbuf *sb = &sock->so_rcv;

		(void) sbreserve( sb, udp_min_burst_in_nfs, sb->sb_mbpmsg,
				sb->sb_msgsize, sb->sb_flags);
	}
#endif hp9000s200
	if (nfs_macct) {
		if (TRIG2_OR(T_NFS_S_MA_EXPAND, T_NFS_PROC, NULL, NULL)
	           (!ma_expand(nfs_macct, MBUFS_PER_SERVER)))
			NS_LOG(LE_NFS_EXPAND, NS_LC_RESOURCELIM, NS_LS_NFS, 0); 
	}
	else {
		s = splnet();	
		if (TRIG2_OR(T_NFS_S_MA_CREATE, T_NFS_PROC, NULL, NULL)
		    !(nfs_macct = ma_create(MBUFS_PER_SERVER))) {	
			(void) splx(s);
			NS_LOG(LE_NFS_MACCT_CREATE, NS_LC_ERROR, NS_LS_NFS, 0);
			return ((SVCXPRT *)NULL);
		}
		(void) splx(s);
	}

	xprt = (SVCXPRT *)kmem_alloc((u_int)sizeof(SVCXPRT));
	rpc_buffer(xprt) = (caddr_t)kmem_alloc((u_int)UDPMSGSIZE);
	ud = (struct udp_data *)kmem_alloc((u_int)sizeof(struct udp_data));
	bzero((caddr_t)ud, sizeof(*ud));
	xprt->xp_addrlen = 0;
	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svckudp_op;
	xprt->xp_port = port;
	xprt->xp_sock = sock;
	return (xprt);
}


/* BEGIN_IMS svckudp_destroy *
 ********************************************************************
 ****
 ****	void
 ****	svckudp_destroy(xprt)
 ****		register SVCXPRT   *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		handle to destroy
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Returns all memory allocated for a transport handle as
 *	well as any inbound mbuf chain associated with the xprt.
 *
 * Algorithm
 *	shrink the nfs macct MBUF_PER_SERVER credits
 *	if (xprt has na inbound message queued)
 *		free the mbuf chain
 *	free udp struncture
 *	free largebuf
 *	free xprt
 *
 * To Do List
 *	May want to verify freeing the inbound mbuf chain.
 *
 * Notes
 *	Never destroy the NFS macct; clients will need it for requests
 *	even if there are no servers.
 *
 * Modification History
 *	11/06/87	ew	added comments
 *	11/19/87	lmb	added nfs_macct code
 *
 * External Calls
 *	m_freem			free the inbound mbuf chain
 *	kmem_free		return kernel memory
 *
 * Called By
 *	nfs_svc
 *
 ********************************************************************
 * END_IMS svckudp_destroy */

void
svckudp_destroy(xprt)
	register SVCXPRT   *xprt;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

#ifdef RPCDEBUG
	rpc_debug(4, "usr_destroy %x\n", xprt);
#endif

	ma_shrink(nfs_macct, MBUFS_PER_SERVER);
	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	kmem_free((caddr_t)ud, (u_int)sizeof(struct udp_data));
	kmem_free((caddr_t)rpc_buffer(xprt), (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)xprt, (u_int)sizeof(SVCXPRT));
}


/* BEGIN_IMS svckudp_recv *
 ********************************************************************
 ****
 ****	bool_t
 ****	svckudp_recv(xprt, msg)
 ****		register SVCXPRT	 *xprt;
 ****		struct rpc_msg		 *msg;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt	   	- transport handle associated with waiting message
 *	msg		- msg struct to decode into
 *
 * Output Parameters
 *	msg		- decoded message placed here
 *	xprt		- inbount mbuf chain is attached to the xprt
 *
 * Return Value
 *	true		- message ready for processing
 *	false		- couldn't receive or decode message
 *
 * Globals Referenced
 *	rsstat		- global rpc server statistics
 *
 * Description
 *	Pulls a request off the socket, checks if packet is intact, 
 *	and deserializes the call packet.
 *
 * Algorithm
 *	pull inbound mbuf chain from socket
 *	if (no message)
 *		return
 *	if (message too short to be valid)
 *		free mbuf chain
 *		return
 *	create xdr stream from mbuf chain
 *	decode the call message
 *	if (decode fails)
 *		free mbuf chain
 *		return
 *	link mbuf chain to xprt
 *
 * Concurrency
 *	none
 *
 * Modification History
 *	11/06/87	ew	added comments
 *
 * External Calls
 *	splnet			block network interupts
 *	ku_recvfrom		get message from socket
 *	xdrmbuf_init		build xdr stream with mbuf
 *	xdr_callmsg		deserialize the call message
 *	m_freem			free the mbuf chain on error
 *
 * Called By
 *	svc_getreq
 *
 ********************************************************************
 * END_IMS svckudp_recv */

bool_t
svckudp_recv(xprt, msg)
	register SVCXPRT	 *xprt;
	struct rpc_msg		 *msg;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR	 *xdrs = &(ud->ud_xdrin);
	register struct mbuf	 *m;
	int			  s;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_recv %x\n", xprt);
#endif
	rsstat.rscalls++;
	s = splnet();
	m = ku_recvfrom(xprt->xp_sock, &(xprt->xp_raddr));
	(void) splx(s);
	if (m == NULL) {
		rsstat.rsnullrecv++;
		return (FALSE);
	}

	if (m->m_len < 4*sizeof(u_long)) {
		rsstat.rsbadlen++;
		goto bad;
	}
	xdrmbuf_init(&ud->ud_xdrin, m, XDR_DECODE);
	if (! xdr_callmsg(xdrs, msg)) {
		rsstat.rsxdrcall++;
		goto bad;
	}
	ud->ud_xid = msg->rm_xid;
	ud->ud_inmbuf = m;
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_recv done\n");
#endif
	return (TRUE);

bad:
	m_freem(m);
	ud->ud_inmbuf = NULL;
	rsstat.rsbadcalls++;
	return (FALSE);
}


/* BEGIN_IMS buffree *
 ********************************************************************
 ****
 ****	static
 ****	buffree(ud)
 ****		register struct udp_data *ud;
 ****
 ********************************************************************
 * Input Parameters
 *	ud			udp data structure
 *
 * Output Parameters
 *	ud			busy and wanted flags cleared upon return
 *
 * Return Value
 *	????
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine is called to "free" a largebuff for use. It
 *	is called when the mbuf chain to which this largebuff is
 *	linked is being freed after I/O completion.
 *
 * Algorithm
 *	clear the busy flag for this buffer
 *	if (anyone is waiting for this buffer)
 *		wake them up
 *
 * Notes
 *
 * Modification History
 *	11/06/87	ew	added comments
 *
 * External Calls
 *	wakeup			wakeup waiter(s)
 *
 * Called By 
 *	MFREE_SPL	
 *
 ********************************************************************
 * END_IMS buffree */

static
buffree(ud)
	register struct udp_data *ud;
{
	ud->ud_flags &= ~UD_BUSY;
	if (ud->ud_flags & UD_WANTED) {
		ud->ud_flags &= ~UD_WANTED;
		wakeup((caddr_t)ud);
	}
}


/* BEGIN_IMS svckudp_send *
 ********************************************************************
 ****
 ****	bool_t
 ****	svckudp_send(xprt, msg)
 ****		register SVCXPRT *xprt; 
 ****		struct rpc_msg *msg; 
 ****
 ********************************************************************
 * Input Parameters
 *	xprt			transport handle upon which to send
 *				this is also the handle which the 
 *				request came in on.			
 *	msg			reply to be sent
 *	
 * Output Parameters
 *	none
 *
 * Return Value
 *	true/false		if send completed/failed
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine will serialize the reply packet into the
 *	largebuff associated with this xprt handle and call
 *	ku-sendto to "make and mbuf" out of it, and send it.
 *
 * Algorithm
 *	while (the output buffer is still busy)
 *		sleep
 *	point an mbuf at the output buffer (mclgetx)
 *	build an xdr stream out of the output buffer
 *	serialize the reply into the output buffer (possibly
 *		linking in a file system buffer).
 *	call ku_sendto to send the reply
 *	if (file system buffer included in the reply)
 *		call rrokfree to wait for send completion.
 *
 * To Do List
 *
 * Notes
 *	Do we need to go to splimp?
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * External Calls
 *	splimp/splx		block interupts when checking udp busy
 *	sleep			wait for output buffer to be free
 *	mclgetx			point an mbuf at the output buffer
 *	buffree			free the output buffer (on error)
 *	xdrmbuf_init		build an xdr stream from the output buffer
 *	xdr_replymsg		serialize the reply
 *	ku_sendto_mbuf		send the reply
 *	NS_LOG_INFO		log send failure
 *	m_freem			free mbuf associted with the output buff
 *	rrokfree		wait for I/O completion to free buffer
 *
 * Called By
 *	svc_sendreply
 *
 ********************************************************************
 * END_IMS svckudp_send */

bool_t
/* ARGSUSED */
svckudp_send(xprt, msg)
	register SVCXPRT *xprt; 
	struct rpc_msg *msg; 
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR *xdrs = &(ud->ud_xdrout);
	register int slen;
	register int stat = FALSE;
	int s;
	struct mbuf *m, *mclgetx();

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_send %x\n", xprt);
#endif
	s = splimp();
	while (ud->ud_flags & UD_BUSY) {
		ud->ud_flags |= UD_WANTED;
		sleep((caddr_t)ud, PZERO-2);
	}
	ud->ud_flags |= UD_BUSY;
	(void) splx(s);
	m = mclgetx(buffree, (caddr_t)ud, rpc_buffer(xprt), UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		buffree(ud);
		return (stat);
	}

	xdrmbuf_init(&ud->ud_xdrout, m, XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (NOT_UTRIG_AND(T_NFS_XDRREPL, T_NFS_PROC)(xdr_replymsg(xdrs, msg))) {
		slen = (int)XDR_GETPOS(xdrs);
		if (m->m_next == 0) {		/* XXX */
			m->m_len = slen;
		}
		if (!ku_sendto_mbuf(xprt->xp_sock, m, &xprt->xp_raddr))
			stat = TRUE;
	} else {
		NS_LOG_INFO(LE_NFS_XDRREPL, NS_LC_ERROR, NS_LS_NFS, 0, 0, 0, 0);
		m_freem(m);
	}
	/*
	 * This is completely disgusting.  If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff.  (see rrokfree in nfs_xdr.c).
	 */
	if (xdrs->x_public) {
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
	}
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_send done\n");
#endif
	return (stat);
}


/* BEGIN_IMS svckudp_stat *
 ********************************************************************
 ****
 ****	enum xprt_stat
 ****	svckudp_stat(xprt)
 ****		SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt				xprt handle of interest	
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	XPRT_IDLE			always!
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine is only used to make svc_getreq happy. It always
 *	returns XPRT_IDLE.
 *
 * To Do List
 *	
 * Notes
 *	We may wish to consider removing the call to this routine
 *	in svc_getreq when it is used in the kernel.
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * Called By
 *	svc_getreq
 *
 ********************************************************************
 * END_IMS svckudp_stat */

/*ARGSUSED*/
enum xprt_stat
svckudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE); 
}


/* BEGIN_IMS svckudp_getargs *
 ********************************************************************
 ****
 ****	bool_t
 ****	svckudp_getargs(xprt, xdr_args, args_ptr)
 ****		SVCXPRT		*xprt;
 ****		xdrproc_t	 xdr_args;
 ****		caddr_t		 args_ptr;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt			xprt handle tied to request
 *	xdr_args		routine to decode these argumnets
 *	args_ptr		memory to decode args into
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	true/false		decode worked/failed
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine mearly calls the routine passed as a parameter
 *	and returns the results.
 *
 * To Do List
 *	This routine could well be made into a macro to save the 
 *	procedure call.
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * External Calls
 *	appropriate get_args routine
 *
 * Called By
 *	rfa_dispatch | ???
 *
 ********************************************************************
 * END_IMS svckudp_getargs */

bool_t
svckudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{
        bool_t ret;
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	ret = ((*xdr_args)(&(((struct udp_data *)(xprt->xp_p2))->ud_xdrin), args_ptr));

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	ud->ud_inmbuf = (struct mbuf *)0;
        return(ret);

}


/* BEGIN_IMS svckudp_freeargs *
 ********************************************************************
 ****
 ****	bool_t
 ****	svckudp_freeargs(xprt, xdr_args, args_ptr)
 ****		SVCXPRT		*xprt;
 ****		xdrproc_t       xdr_args;
 ****		caddr_t	        args_ptr;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt			xprt handle from which the args came
 *	xdr_args		routine to call to free arguments
 *	args_ptr		address of srguments
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	true/false		if call to xdr_args worked/failed
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	This routine is called to free argument structrues and
 *	the xdr stream (aka inbound mbuf chain) they were decoded
 *	from.
 *
 * Algorithm
 *	reslove addresses of inbound xdr stuct and inbound mbuf chain
 *	if an mbuf chain exists, free it
 *	if there is space allocated to arguments
 *		call the appropiate xdr routine to free the args
 *
 * To Do List
 *
 * Notes
 *	It may be possible/desierable to free the mbuf chain as
 *	soon as the request arguments have beed decoded (svckudp_getargs).
 *	
 * Modification History
 *	11/07/87	ew	added comments
 *	
 * External Calls
 *	m_freem			free inbound mbuf chain
 *	xdr_args		specified routine to free arg structures
 *
 * Called By
 *	rfs_dispatch | ???
 *
 ********************************************************************
 * END_IMS svckudp_freeargs */

bool_t
svckudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{
	register XDR *xdrs =
	    &(((struct udp_data *)(xprt->xp_p2))->ud_xdrin);
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	ud->ud_inmbuf = (struct mbuf *)0;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * the dup cacheing routines below provide a cache of non-failure
 * transaction id's.  rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 */

struct dupreq {
	u_long		dr_xid;
	struct sockaddr_in dr_addr;
	u_long		dr_proc;
	u_long		dr_vers;
	u_long		dr_prog;
	struct dupreq	*dr_next;
	struct dupreq	*dr_chain;
};

/*
 * MAXDUPREQS is the number of cached items.  It should be adjusted
 * to the service load so that there is likely to be a response entry
 * when the first retransmission comes in.
 */
#define	MAXDUPREQS	400

#define	DUPREQSZ	(sizeof(struct dupreq) - 2*sizeof(caddr_t))
#define	DRHASHSZ	32
#define	XIDHASH(xid)	((xid) & (DRHASHSZ-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid

int	ndupreqs;
int	dupreqs;
int	dupchecks;
struct dupreq *drhashtbl[DRHASHSZ];

/*
 * drmru points to the head of a circular linked list in lru order.
 * drmru->dr_next == drlru
 */

struct dupreq *drmru;


/* BEGIN_IMS  svckudp_dupsave *
 ********************************************************************
 ****
 ****	svckudp_dupsave(req)
 ****		register struct svc_req *req;
 ****
 ********************************************************************
 * Input Parameters
 *	req			svc_req to be saved in the cache
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	drmu			head of good request cache
 *	drhashtable		hash table of good requests
 *
 * Description
 *	This routine will add a new dup request entry into the
 *	cache list.
 *
 * Algorithm
 *	if (not all entries allocated)
 *		allocate new entry
 *		if (list not empty)
 *			link entry to tail of list
 *		else
 *			link entry to itself
 *	else
 *		get entry at head of list
 *		remove the entry from the hash table
 *	move head of list to new entry
 *	fill in xid, prog,vers, proc entries
 *	link into hash table
 *
 * Concurrency
 *
 * To Do List
 *	No check is currently made for a failed kmem_alloc
 *
 * Notes
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * External Calls
 *	kmem_alloc		allocate new dureq structure
 *
 * Called By
 *	rfs_[create, remove, rename, link, symlink, mkdir, rmdir]
 *
 ********************************************************************
 * END_IMS svckudp_dupsave */

svckudp_dupsave(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;

	if (ndupreqs < MAXDUPREQS) {
		dr = (struct dupreq *)kmem_alloc(sizeof(*dr));
		if (drmru) {
			dr->dr_next = drmru->dr_next;
			drmru->dr_next = dr;
		} else {
			dr->dr_next = dr;
		}
		ndupreqs++;
	} else {
		dr = drmru->dr_next;
		unhash(dr);
	}
	drmru = dr;

	dr->dr_xid = REQTOXID(req);
	dr->dr_prog = req->rq_prog;
	dr->dr_vers = req->rq_vers;
	dr->dr_proc = req->rq_proc;
	dr->dr_addr = req->rq_xprt->xp_raddr;
	dr->dr_chain = drhashtbl[DRHASH(dr)];
	drhashtbl[DRHASH(dr)] = dr;
}


/* BEGIN_IMS  svckudp_dup *
 ********************************************************************
 ****
 ****	svckudp_dup(req)
 ****		register struct svc_req *req;
 ****
 ********************************************************************
 * Input Parameters
 *	req			svc_req to be searched for in the cache
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	true/false		if request is/isn't found in the cache
 *
 * Globals Referenced
 *	drmu			head of good request cache
 *	drhashtbl		hash table of good requests
 *
 * Description
 *	This routine will search the duplicate request hash table
 *	for a particular requuest and return true if the request
 *	is already in the table.
 *
 * Algorithm
 *	find hash bucket matching the current request id
 *	while (not at the end of the list)
 *		if (current request matches item in list)
 *			return true
 *	return false
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * External Calls
 *	bcmp			compare source address of requests
 *
 * Called By
 *	rfs_[create, remove, rename, link, symlink, mkdir, rmdir]
 *
 ********************************************************************
 * END_IMS svckudp_dup */

svckudp_dup(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	u_long xid;
	 
	dupchecks++;
	xid = REQTOXID(req);
	dr = drhashtbl[XIDHASH(xid)]; 
	while (dr != NULL) { 
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
		     (caddr_t)&req->rq_xprt->xp_raddr,
		     sizeof(dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dupreqs++;
			return (1);
		}
	}
	return (0);
}


/* BEGIN_IMS unhash *
 ********************************************************************
 ****
 ****	static
 ****	unhash(dr)
 ****		struct dupreq *dr;
 ****
 ********************************************************************
 * Input Parameters
 *	dr			pointer to dup request to remove
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	drhashtable		table of hashed dup requests
 *
 * Description
 *	This routine will remove the dup request indicated from
 *	the hash table.
 *
 * Algorithm
 *	calculate correct hash bucket for this request
 *	while (not at end of list)
 *		if (this list item matches item of interest)
 *			unlink it from the list
 *			retrun
 *
 * Concurrency
 *
 * To Do List
 *	May want to add QA code to report item not found.
 *
 * Notes
 *
 * Modification History
 *	11/07/87	ew	added comments
 *
 * External Calls
 *	none
 *
 * Called By
 *	svckudp_dupsave
 *
 ********************************************************************
 * END_IMS unhash */

static
unhash(dr)
	struct dupreq *dr;
{
	struct dupreq *drt;
	struct dupreq *drtprev = NULL;
	 
	drt = drhashtbl[DRHASH(dr)]; 
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				drhashtbl[DRHASH(dr)] = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return; 
		}	
		drtprev = drt;
		drt = drt->dr_chain;
	}	
}

/* BEGIN_IMS svckudp_dupflush *
 ********************************************************************
 ****
 ****	static
 ****	svckudp_dupflush(proc, addr)
 ****		int proc;
 ****		u_long addr;
 ****
 ********************************************************************
 * Input Parameters
 *	proc			procecdure number to flush from dup cache.
 *	addr			IP addr to match, or zero to match all.
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	drmru			head of circular linked listed of dups
 *
 * Description
 *	This routine will remove ALL dup request with matching op from
 *	the hash table by changing the proc number to -1.  This effectively
 *	keeps it from ever being matched as a duplicate request without
 *	having to go through the overhead of kmem_free/kmem_alloc.
 *	If addr it non-zero, then we only flush requests from that IP address.
 *
 * Algorithm
 *	while (not at end of list)
 *		if (this list item matches item of interest)
 *                    Mark it bad
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	04/04/89	dds	Created to fix PC-NFS data corruption problem.
 *
 * External Calls
 *	none
 *
 * Called By
 *	nfs_getfh()
 *
 ********************************************************************
 * END_IMS svckudp_dupflush */

#define BADPROC -1	/* Some bad procedure number */

svckudp_dupflush(proc, addr)
	int proc;
	u_long addr;	/* assumes four byte IP addr */
{
	struct dupreq *dr;
	int firsttime = TRUE;
	int s;
	 
	if (drmru == NULL)	/* Very first time, no requests yet */
		return;
	s = splnet();		/* Keep nfsd from running! */
	dr = drmru;
	while ( firsttime || (dr != drmru) ){
		if (dr->dr_proc == proc && 
		    ( addr == 0 || addr == dr->dr_addr.sin_addr.s_addr )) { 
			dr->dr_proc = BADPROC;
		}	
		dr = dr->dr_next;
		firsttime = FALSE;
	}
	splx(s);
	return;
}
