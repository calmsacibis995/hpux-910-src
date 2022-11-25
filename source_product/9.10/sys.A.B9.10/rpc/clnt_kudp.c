/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/clnt_kudp.c,v $
 * $Revision: 1.13.83.4 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/08 15:21:46 $
 *  Modified for 4.3BSD 8.0
 */

/* BEGIN_IMS_MOD 
******************************************************************************
****								
****	clnt_kudp - client routines for kernel level RPC based on UDP/IP
****								
******************************************************************************
* Description
*	These routines implement the client side of the UDP/IP based RPC
*	protocol. It is used in the kernel as the basis of NFS (Network
*	File System). RPC (Remote Procedure Call) follows a synchronous
*	reply/response paradigm. Most of the client routines in this
*	module are called from rfscall(), the routine for making remote
*	file system calls. The routine clntkudp_callit() is responsible for
*	the bulk of the client RPC code. The principal client data
*	structure is the client handle and its associated private data.
*
* Externally Callable Routines
*	clntkudp_create   - create a new client handle and output buffer
*	clntkudp_init     - (re-)initialize a client handle for a new call
*	clntkudp_freecred - free a user credential structure
*	clntkudp_setint   - allow interrupts at RPC level
*	clntkudp_callit   - send an RPC request and wait for the reply
*	ckuwakeup	  - indicate that remote call has timed out
*	clnkudp_error	 - get the error from the RPC reply packet
*	clnkudp_freeres	  - free results of remote call (NOT USED)
*	clnkudp_abort	  - abort remote procedure call (NOT USED)
*	clnkudp_destroy	  - destroy client handle (NOT USED)
*
* Internal Routines
*	noop	          - a null function called to free an mbuf
*	buffree	          - marks an output buffer not busy
*	bindresvport	  - bind a socket to a reserved port
*
* Test Module
*	$SCAFFOLD/nfs/*
*
* To Do List
*
* Notes
*
* Modification History
*	
*   890726    RM    Reserved buffer space for incomming and outgoing messages
*                   due to the change in default UDP buffer sizes.
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_kudp.c,v 1.13.83.4 93/11/08 15:21:46 craig Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/protosw.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../h/signal.h"	/* Added to support controlled interrupts */
#include "../nfs/nfs.h"
#ifndef sigmask
#define	sigmask(s) (1L<<(s-1))  /* sigmask is not defined in signal.h on s800 */
#endif sigmask

#include "../h/trigdef.h"
/*---------------------------------------------------------------------------
#include "../h/ns_ipc.h"
#include "../h/ns_opt.h"
---------------------------------------------------------------------------*/
#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
/*----------------------------------------------------------------------------
extern int  so_def_so_options;
---------------------------------------------------------------------------*/

struct mbuf	*ku_recvfrom();
int		ckuwakeup();

enum clnt_stat	clntkudp_callit();
void		clntkudp_abort();
void		clntkudp_error();
bool_t		clntkudp_freeres();
void		clntkudp_destroy();

void		xdrmbuf_init();

/*
 * Operations vector for UDP/IP based RPC
 */
static struct clnt_ops udp_ops = {
	clntkudp_callit,	/* do rpc call */
	clntkudp_abort,		/* abort call */
	clntkudp_error,		/* return error status */
	clntkudp_freeres,	/* free results */
	clntkudp_destroy	/* destroy rpc handle */
};

/*
 * Private data per rpc handle.  This structure is allocated by
 * clntkudp_create.  It is deallocated with clntkudp_destroy, but
 * this routine is not used at present.
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	struct socket		*cku_sock;	/* open udp socket */
	struct sockaddr_in	 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr routine for output */
	XDR			 cku_inxdr;	/* xdr routine for input */
	u_int			 cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct mbuf		*cku_inmbuf;	/* input mbuf */
	struct ucred		*cku_cred;	/* credentials */
	u_long			 cku_xid;	/* transaction ID for retrans.*/
						/* This is used for retrans.
						   triggered by the NFS level */
};

struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
} rcstat;

int rpc_sbdrops = 0;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010
#define CKU_INTERRUPT   0x020    /* added to allow interrupts */
#define CKU_RETRANSMIT  0x040    /* This is a retransmission, use same xid */

/* Times to retry */
#define	RECVTRIES	2
#define	SNDTRIES	4

u_long	clntkudpxid;		/* transaction id used by all clients */

/*-----------------------------------------------------------------------------
 * macct for nfs - Used by ku_recvfrom for charging inbound mbufs,
 * created in svckudp_create or clntkudp_create. Both of these
 * routines expand the nfs macct for each new server or client handle.

extern struct	macct	*nfs_macct;

#define MBUFS_PER_CLIENT ((UDPMSGSIZE/MLEN) * 2)
     * Removed references to maccts
-----------------------------------------------------------------------------*/

/* BEGIN_IMS noop *
 ********************************************************************
 ****
 ****		noop()
 ****
 ********************************************************************
 *
 * Description
 *	noop is a null procedure. It is used because mclgetx requires
 *	the m_clfun field of an MF_LARGEBUF mbuf to be initialized to
 *	a routine that is called when the mbuf is freed. This routine
 *	is called by MFREE_SPL() through CL_FUN(). This routine is
 *	typically buffree(), but clntkudp_create uses noop() instead.
 *
 ********************************************************************
 * END_IMS noop */

static
noop()
{
}


/* BEGIN_IMS buffree *
 ********************************************************************
 ****
 ****			buffree(p)
 ****
 ********************************************************************
 * Input Parameters
 *	p	pointer to client handle private data area
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
 *	This routine is called to free an output buffer. It is used
 *	to synchronize access to a large buffer associated with an
 *	mbuf. This routine is called by MFREE_SPL() through CL_FUN()
 *	when the send has completed and the mbuf is freed.
 *
 * Algorithm
 *	mark buffer not busy
 *	if (buffer wanted) {
 *		clear buffer wanted condition
 *		wakeup(buffer address)
 *	}
 *
 * Concurrency
 *	This routine is called at splimp.
 * MOdifications
 *    11/27/91   RB ifdef MP to accomodate MP lan driver
 *
 * External Calls
 *	wakeup
 *
 * Called By
 * 	MFREE_SPL via CL_FUN, clntkudp_callit
 *
 ********************************************************************
 * END_IMS buffree */

static
buffree(p)
	struct cku_private *p;
{
        int old;
#ifdef MP
        old  = sleep_lock();
        p->cku_flags &= ~CKU_BUFBUSY;
        if (p->cku_flags & CKU_BUFWANTED) {
                p->cku_flags &= ~CKU_BUFWANTED;
                wakeup((caddr_t)&p->cku_outbuf);
        }
        sleep_unlock(old);
#else /* not MP */
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		p->cku_flags &= ~CKU_BUFWANTED;
		wakeup((caddr_t)&p->cku_outbuf);
	}
#endif /* MP*/
}


/* BEGIN_IMS clntkudp_create *
 ********************************************************************
 ****
 **** 		clntkudp_create(addr, pgm, vers, retrys, cred)
 ****
 ********************************************************************
 * Input Parameters
 *	addr		internet address of remote node for RPC call
 *	pgm		RPC program number (NFS = 100003)
 *	vers		RPC program version number (currently 2)
 *	retrys		the number of RPC retransmissions
 *	cred		a pointer to the user's credential structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 * 	NULL 		failure to create a client handle
 * 	address		a pointer to a client handle structure
 *
 * Globals Referenced
 *	clntkudpxid	client transaction id
 * 	udp_ops		client kernel UDP operations vector
 * 	nfs_macct	global NFS macct for mbuf accounting
 *
 * Description
 *	clntkudp_create creates and initializes a client rpc handle for
 *	kernel rpc using UDP. Space is allocated for the client handle,
 *	private data, and output buffer. A UDP socket is opened. There is
 *	a one-to-one correspondence between client handles and UDP sockets.
 *	Once a client rpc handle is created, the handle and corresponding
 * 	UDP socket remain in existence for reuse by successive RPC calls.
 *	The maximum number of client handles that can be created is
 *	determined by MAXCLIENTS, the size of chtable (see clget).
 *	
 *		cku_private ----> +-------------------+
 *				  |	flags	      |<---+
 *				  +-------------------+    |
 *				  | CLIENT	      |    |
 *				  |	AUTH	      |    |
 *				  |	clnt_ops      |    |
 *				  |	private    ---+----+
 *				  +-------------------+
 *				  | other client and  |
 *				  | socket information|
 *				  +-------------------+
 *
 * Algorithm
 *	if (no NFS macct) {
 *		create an NFS macct
 *		if (creation attempt fails)
 *			return (NULL)
 *      }
 *	expand the number of credits in the NFS macct
 *	allocate memory for private data
 *		-- client handle is a field of private data
 *	if (clntkudpxid is zero)
 *		assign clntkudpxid the value of the system clock
 *	initialize fields of the client handle
 *	initialize fields of sample rpc call message (for pre-serialization)
 *	initialize transitory fields of private data (clntkudp_init)
 * 	allocate UDPMSGSIZE output buffer
 *	get an mbuf to point to output buffer (mclgetx)
 *	initialize the outbound XDR stream based on the output buffer
 *	pre-serialize the call message header into the XDR stream
 *	if (error) {
 *		free mbuf
 *		free memory and return NULL
 *	}
 *	free mbuf
 *	open UDP socket
 *	bind UDP socket to reserved port number
 *	return (client handle address)
 * 	
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	
 * Modification History
 *	11/2/87		lmb	added triggers, converted to NS logging,
 *	11/2/87		lmb	added soclose() to bind error handling.
 *	11/13/87	lmb	added code to create nfs macct if necessary
 *				and to expand it for each client handle
 *			
 *
 * External Calls
 * 	kmem_alloc, kmem_free
 *	bzero, ptoh
 *	authkern_create, clntkudp_init
 * 	mclgetx, m_freem
 *	xdrmbuf_init, xdr_callhdr, XDR_GETPOS (xdrmbuf_getpos),
 *	pffindproto, socreate, bindresvport, soclose
 * 	rpc_debug, NS_LOG, NS_LOG_INFO
 *
 * Called By
 *	clget	
 *
 ********************************************************************
 * END_IMS clntkudp_create */

CLIENT *
clntkudp_create(addr, pgm, vers, retrys, cred)
	struct sockaddr_in *addr;
	u_long pgm;
	u_long vers;
	int retrys;
	struct ucred *cred;
{
	register CLIENT *h;
	register struct cku_private *p;
	int error = 0;
	struct rpc_msg call_msg;
	struct mbuf *m, *mclgetx();
	extern int nfs_portmon;
	struct protosw *proto;
/*-----------------------------------------------------------------------------
	int    domain, type, soopt;
-----------------------------------------------------------------------------*/
	int    timer = 0;
	int    flags = 0;
/*----------------------------------------------------------------------------
	struct kopt  *opt = NULL;
-----------------------------------------------------------------------------*/
	int s;

#ifdef RPCDEBUG
	rpc_debug(4, "clntkudp_create(%X, %d, %d, %d\n",
	    addr->sin_addr.s_addr, pgm, vers, retrys);
#endif
/*----------------------------------------------------------------------------
	 * Create an nfs_macct, if necessary. This will only occur if
	 * there are no NFS daemons active. Expand the number of mbuf
	 * credits each time a client handle is created.    lmb
       
	if (nfs_macct) {
		if (TRIG2_OR(T_NFS_C_MA_EXPAND, T_NFS_PROC, NULL, NULL)
		    !ma_expand(nfs_macct, MBUFS_PER_CLIENT))
			NS_LOG(LE_NFS_EXPAND, NS_LC_RESOURCELIM, NS_LS_NFS, 0); 
	}
	else {
		s = splnet();	
		if (TRIG2_OR(T_NFS_C_MA_CREATE, T_NFS_PROC, NULL, NULL)
		    !(nfs_macct = ma_create(MBUFS_PER_CLIENT))) {	
			(void) splx(s);
			NS_LOG(LE_NFS_MACCT_CREATE, NS_LC_ERROR, NS_LS_NFS, 0);
			return ((CLIENT *)NULL);
		}
		(void) splx(s);
	}
	* Removed references to Maccts
----------------------------------------------------------------------------*/

	p = (struct cku_private *)kmem_alloc((u_int)sizeof *p);
	bzero((caddr_t)p, sizeof (*p));
	h = ptoh(p);

	if (!clntkudpxid) {
		clntkudpxid = time.tv_usec;
	}

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clntkudp_init(h, addr, retrys, cred);
	p->cku_outbuf = (char *)kmem_alloc((u_int)UDPMSGSIZE);
	m = mclgetx(noop, 0, p->cku_outbuf, UDPMSGSIZE, M_DONTWAIT);
	if (m == NULL)
		goto bad;
	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)
	     OR_UTRIG(T_NFS_XDR_CALLHDR, T_NFS_PROC)){
		NS_LOG(LE_NFS_XDR_CALLHDR, NS_LC_ERROR, NS_LS_NFS, 0);
		(void) m_freem(m);
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	(void) m_free(m);

	/* open udp socket */
/*-----------------------------------------------------------------------------
	domain = PF_INET;
	type = IPPROTO_UDP;
	proto = pffindproto(domain, type);
	if (proto == 0 OR_UTRIG(T_NFS_PROTOFIND, T_NFS_PROC)) {
		NS_LOG_INFO(LE_NFS_PROTO_FIND, NS_LC_ERROR, NS_LS_NFS, 0, 1,
			(int)proto, 0);
		goto bad;
	}
	soopt = SO_GRACEFUL_CLOSE | so_def_so_options;
	error = socreate(&p->cku_sock, SOCK_DGRAM, proto, soopt, 
			 timer, opt, flags);
	  * Changed interface to socreate
----------------------------------------------------------------------------*/

	error = socreate(AF_INET, &p->cku_sock, SOCK_DGRAM, IPPROTO_UDP, 0);

#ifdef NTRIGGER
	if (!error && utry_trigger(T_NFS_SOCREATE, T_NFS_PROC, NULL, NULL)) {
/*-----------------------------------------------------------------------------
		(void) soclose(p->cku_sock, 0);
	  * Changed interface to soclose
-----------------------------------------------------------------------------*/
	        (void) soclose(p->cku_sock);
		error = ENOBUFS;
	}
#endif NTRIGGER

	if (error) {
		NS_LOG_INFO(LE_NFS_SOCREATE, NS_LC_ERROR, NS_LS_NFS, 0, 1,
			(int)error, 0);
		goto bad;
	}
	if (error = bindresvport(p->cku_sock)) {
		NS_LOG_INFO(LE_NFS_SOBIND, NS_LC_ERROR, NS_LS_NFS, 0, 1,
			(int)error, 0);
/*----------------------------------------------------------------------------
		(void) soclose(p->cku_sock, 0);
		* Changed interface to soclose
----------------------------------------------------------------------------*/
	        (void) soclose(p->cku_sock);
		goto bad;
	}
/*
 * these values were picked based on previous (<= 7.0)UDP buffer sizes
 * RM
 */
        error = soreserve( p->cku_sock,NFS_MAXDATA+1024, NFS_MAXDATA+1024 );

        if (error)
		goto bad;

	return (h);

bad:
	/* Make sure cred structure reference count gets decremented on error */
	crfree(p->cku_cred);
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)(caddr_t)p, (u_int)sizeof(struct cku_private));
#ifdef RPCDEBUG
	rpc_debug(4, "create failed\n");
#endif
	return ((CLIENT *)NULL);
}


/* BEGIN_IMS clntkudp_init *
 ********************************************************************
 ****
 ****		clntkudp_init(h, addr, retrys, cred)
 ****
 ********************************************************************
 * Input Parameters
 *	h	address of client handle
 *	addr	internet address of remote node for RPC request
 *	retrys 	the number of RPC retransmissions
 *	cred	address of the user's credential structure (from u area)
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
 *	clntkudp_init initializes certain fields in the client rpc handle.
 *	These are the fields that change for each remote procedure.  They
 *	are the address of the rpc server, the number of times to retransmit
 *	if no reply is received, and the credential structure of the current
 *	user.
 *
 * Algorithm
 *	set the number of retrys
 *	set the address to which the rpc call will be made
 *	increment the reference count on the credential structure
 *	set the credential field
 *	clear the flag field
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 * 	We are adding a pointer to the cred, therefore we MUST increment
 *	the reference count to avoid it being freed. Sun did not have this
 * 	problem, because they did not do a crcopy in bindresvport, which
 *	has other problems.  Also, they were basically LUCKY.  To free
 *	this cred call clntkudp_freecred().
 *	dds	6/2/87.
 *
 * Modification History
 *
 * External Calls
 *	htop
 *	crhold 		macro incrementing credential reference count
 *      bzero
 *
 * Called By 
 *	clget, clntkudp_create
 *
 ********************************************************************
 * END_IMS clntkudp_init */

clntkudp_init(h, addr, retrys, cred)
	CLIENT *h;
	struct sockaddr_in *addr;
	int retrys;
	struct ucred *cred;
{
	struct cku_private *p = htop(h);

	p->cku_retrys = retrys;
	p->cku_addr = *addr;
	/*
	 * NOTE: we are adding a pointer to the cred, therefore we MUST
	 * increment the reference count to avoid it being freed.  
	 */
	crhold(cred);
	p->cku_cred = cred;
	p->cku_flags = 0;
}


/* BEGIN_IMS clntkudp_error *
 ********************************************************************
 ****
 ****			clntkudp_freecred(h)
 ****
 ********************************************************************
 * Input Parameters
 *	h	client handle pointer
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
 *	clntkudp_freecred frees a credential structure.
 *
 * Algorithm
 *	call crfree() to free credential structure
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	See Note in clntkudp_init for why this routine is here.
 *
 * Modification History
 *
 * External Calls
 *	htop
 *	crfree	decrement reference count; if 0, then add to free list
 *
 * Called By
 *	clfree
 *
 ********************************************************************
 * END_IMS clntkudp_error */
void
clntkudp_freecred(h)
	CLIENT *h;
{
	struct cku_private *p = htop(h);

	crfree(p->cku_cred);
	p->cku_cred = (struct ucred *)NULL;

}


/* BEGIN_IMS clntkudp_setint *
 ********************************************************************
 ****
 ****			clntkudp_setint(h)
 ****		
 ********************************************************************
 * Input Parameters
 *	h	pointer to client rpc handle
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
 *	clntkudp_setint sets a flag in the private data area of the client
 *	handle indicating that the remote procedure call can be interrupted
 *	by the user. Whether or not an RPC call can be interrupted is an
 *	option set for each mounted file system. This flag is checked in
 *	clntkudp_callit.
 *
 * Algorithm
 *	set CKU_INTERRUPT in client handle flag
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	This routine was added to HPNFS to allow interrupts at the RPC layer.
 *
 * Modification History
 *
 * External Calls
 *	htop	macro mapping client handle to private data
 *
 * Called By
 *	rfscall
 *
 ********************************************************************
* END_IMS clntkudp_setint */

clntkudp_setint(h)
	CLIENT *h;
{
	register struct cku_private *p = htop(h);

	p->cku_flags |= CKU_INTERRUPT;
}



/*
 * Time out back off function for retransmissions. tim is in hz.
 */
#define MAXTIMO	(60 * hz)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))


/* BEGIN_IMS clntkudp_callit *
 ********************************************************************
 ****
 **** 		clntkudp_callit(h, procnum, xdr_args, argsp,
 ****				xdr_results, resultsp, wait)
 ****
 ********************************************************************
 * Input Parameters
 *	h 		pointer to the client handle structure
 *	procnum 	the number of the NFS procedure to be executed
 *	xdr_args 	XDR procedure for encoding arguments
 *	argsp 		pointer to data to be sent as arguments
 *	xdr_results 	XDR procedure for decoding results
 *	resultsp 	pointer to data area for results
 *	wait 		timeval structure giving RPC timeout value
 *
 * Output Parameters
 *	resultsp 	points to the results of the request
 *
 * Return Value
 *	0 		success
 *	positive	some error
 *
 *	The value returned is a clnt_stat enumerated type giving the 
 *	result of the RPC request.
 *
 * Globals Referenced
 *	rcstat		global client statistics
 *	clntkudpxid	next transaction id to use
 *
 * Description
 * 	Call remote procedure. Most of the work of RPC is done here.
 * 	We serialize what is left of the header (some was pre-seri-
 *	alized in the handle), serialize the arguments, and send it
 *	off. We wait for a reply or a time out. Timeout causes an
 *	immediate return, other packet problems may cause a retry on
 *      the receive.  The number of retry attempts is taken from the
 *	client handle. When a good packet is received, we deserialize
 *      it and check verification.  A bad reply code will cause one retry
 * 	with full (longhand) credentials.
 *
 * Algorithm
 *	number of send tries = maximum retrys
 *	synchronize access to client data structure
 *	put credentials into u structure
 *	if (not a retransmission)
 *		get a new xid
 *	if (interruptable)
 * 		fix signal mask to only allow certain ones.
 *   call again:
 *	clear retransmit flag (CKU_RETRANSMIT)
 *	wait for exclusive access to output buffer
 *	get a special MF_LARGEBUF mbuf (mclgetx)
 *	if (mbuf == NULL) {
 *		status = RPC_SYSTEMERROR
 *		goto done
 *	}
 *	put xid into pre-serialized output buffer
 *	initialize XDR mbuf output stream
 *	encode the procedure number into XDR stream
 *	encode the user's credential into XDR stream (uid, gids)
 *	encode the calling arguments
 *	send the request to the remote system
 *	if (send fails) {
 *		status = RPC_CANTSEND
 *		goto done
 *	}
 *	for (maximum number of receive attempts) {
 *		while (socket buffer receive count == 0) {
 *			schedule timeout wakeup
 *			if (interrupts allowed)
 *				arrange to sleep at interruptible level
 *			sleep (socket receive queue)
 *			cancel timeout wakeup
 *			if (interrupted) {
 *				status = RPC_INTR
 *				goto got interrupted
 *			}
 *			if (timed out) {
 *				status = RPC_TIMEDOUT
 *				goto done
 *			}
 *		}
 *		receive data from socket
 *		if (no_data or data_too_short or xid_does_not_match)
 *			continue (for loop)
 *		flush the rest of the data on the socket input queue
 *	}
 *	if (too many recieve tries) {
 *		status = RPC_CANTRECV
 *		goto done
 *	}
 *	decode reply message
 *	if (decoding succeeds) {
 *		get status from reply
 *		if (status == RPC_SUCCESS {
 *			check authentication
 *			if (authentication not valid)
 *				status = RPC_AUTHERROR
 *		}
 * 	}		
 *	else
 *		status = RPC_CANTDECODERES
 *	free input mbuf chain 
 *   done:
 *	if (status != RPC_SUCCESS and status != RPC_CANTDECODERES and
 *          the number of send tries > 0) {
 *		decrement number of send tries
 *		timeout = backoff(timeout)
 *		if (error is due to lack of resources)
 *			sleep for awhile
 *		goto call again
 *	}
 *   got interrupted:
 *	restore prior credentials in u area
 *	restore signal mask
 *	wait until the output buffer is not busy
 *	release the client handle
 *	return (status)
 *	
 * Concurrency
 *	none
 *
 * To Do List
 *	Replace synchronization flag and code with semaphore operations.
 *
 * Notes
 *
 * Modification History
 *	11/2/87		lmb	added triggers
 *	11/11/87	lmb	moved some splx's around to avoid jumping
 *				up/down in priority for a short time
 *	11/8/88		arm	crhold() and crfree() the u.u_cred
 *				structure. Fix for DTS INDaa03167
 *      11/27/91         RB     ifdef MP sleep_lock to protect from MP
 *                              lan driver 
 *       2/03/92        kls     for write request, the outbound
 *                              xdr_public is set. Add code to free
 *                              the resources referenced by xdr_public.
 *
 * External Calls
 *	sleep, wakeup
 *	nettimeout, netuntimeout
 *	sbdrop
 *	splimp, splnet, splx
 *	mclgetx, m_freem, buffree, mtod
 *	xdrmbuf_init
 *	XDR_SETPOS (xdrmbuf_setpos)
 *	XDR_PUTLONG (xdrmbuf_putlong)
 *	AUTH_MARSHALL (authkern_marshal)
 *	XDR_GETPOS (xdrmbuf_getpos)
 *	(*xdr_args)
 *	ku_sendto_mbuf, ku_recvfrom
 *	xdr_replymsg, _seterr_reply
 *	AUTH_VALIDATE (authkern_validate)
 *	xdr_opaque_auth
 *	backoff 
 *	rpc_debug, utry_trigger, OR_UTRIG
 *
 * Called By
 *	rfscall
 *
 ********************************************************************
 * END_IMS clntkudp_callit */

enum clnt_stat 
clntkudp_callit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
	register CLIENT	*h;
	u_long		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
{
	register struct cku_private *p = htop(h);
	register XDR	   	   *xdrs;
	register struct socket	   *so = p->cku_sock;
	int			   rtries;
	int			   stries = p->cku_retrys;
	struct sockaddr_in	   from;
	struct rpc_msg		   reply_msg;
	int			   s;
	struct ucred		   *tmpcred;
	struct mbuf		   *m;
	int timohz;
	int 			   interruptable = p->cku_flags & CKU_INTERRUPT;
	int			   interrupted;
	int			   save_mask;
        int                        old;

#ifdef RPCDEBUG
	rpc_debug(4, "cku_callit\n");
#endif
	rcstat.rccalls++;

	while (p->cku_flags & CKU_BUSY) {
		rcstat.rcwaits++;
		p->cku_flags |= CKU_WANTED;
		sleep((caddr_t)h, PZERO-2);
	}
	p->cku_flags |= CKU_BUSY;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_cred;
	u.u_cred = p->cku_cred;
	crhold(u.u_cred);

	/* If this is not a retransmission, get a new id */
	if ( (p->cku_flags & CKU_RETRANSMIT) == 0){
	    p->cku_xid = clntkudpxid++;
	    if ( (int) so->so_rcv.sb_cc != 0 ) {
		rpc_sbdrops++;
		(void) sbdrop( &so->so_rcv, (int)so->so_rcv.sb_cc);
	    }
	}
	 

	/*
	 * If we are going to allow interrupts this time around, then set our
	 * signal mask to only allow certain signals.  Ideally, this is the
	 * minimum set of signals.  The reason is that some signals, such
	 * as SIGCLD, can be delayed for processing with no difference as
	 * far as the user is concerned.  The idea is to minimize potential
	 * problems in the system by only allowing those signals which have
	 * to be acted on, rather than those that can wait until the server
	 * wakes up to send us a reply.  Also, protect our access to the
	 * sigmask field just in case.  As long as interrupts are not set
	 * for the first time in, this shouldn't impact performance.
	 */
	if ( interruptable ) {
		s = spl6();
		save_mask = u.u_procp->p_sigmask;
		u.u_procp->p_sigmask  |= ~( sigmask(SIGHUP) | sigmask(SIGINT)
				| sigmask(SIGQUIT) | sigmask(SIGKILL)
				| sigmask(SIGTERM) | sigmask(SIGALRM) );
		(void) splx(s);
	}

	/*
	 * This is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * hz + (wait.tv_usec * hz) / 1000000;

call_again:

	/*
	 * Clear retransmit flag so that we if we manage to succeed on
	 * a retry it doesn't get left around by accident.
	 */
	p->cku_flags &= ~CKU_RETRANSMIT;

	/*
	 * Wait til buffer gets freed then make a type 2 mbuf point at it
	 * The buffree routine clears CKU_BUFBUSY and does a wakeup when
	 * the mbuf gets freed.
	 */

#ifdef MP
        old = sleep_lock();
        while (p->cku_flags & CKU_BUFBUSY) {
                p->cku_flags |= CKU_BUFWANTED;
                net_timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
                sleep_then_unlock((caddr_t)&p->cku_outbuf, PZERO-3,old);
                (void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
                if (!(p->cku_flags & CKU_BUFBUSY))
                { net_untimeout(wakeup, (caddr_t)&p->cku_outbuf);
                  goto loop1_done; }
                old  = sleep_lock();
        }
        sleep_unlock(old);
loop1_done:
        p->cku_flags |= CKU_BUFBUSY;  
#else /* not MP */
	s = splimp();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * loop-back call.  The client can block waiting for
		 * the server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf.  Avoid this
		 * by flushing the input queue every once in a while while
		 * we are waiting.
		 */
		/*
		 * changed p to &p->cku_outbuf to match the fact that we
		 * are waiting for the buffer.  This came along as part of
		 * the fix for data corruption.  dds, 4/8/87
		 */
		net_timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
		sleep((caddr_t)&p->cku_outbuf, PZERO-3);
/*----------------------------------------------------------------------------
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc, 0);
		 * Changed interface to sbdrop
----------------------------------------------------------------------------*/
                 if (!(p->cku_flags & CKU_BUFBUSY)) {
                        /* probably woke up from buffree        */
                        net_untimeout(wakeup, (caddr_t)&p->cku_outbuf);
                }
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
	}
	p->cku_flags |= CKU_BUFBUSY;
	(void) splx(s);
#endif /* MP*/

	m = mclgetx(buffree, (caddr_t)p, p->cku_outbuf, UDPMSGSIZE, M_WAIT);

	 m->m_flags |= MF_NODELAYFREE;
           /* set flag for lan3 to free nfs buffer without delay */

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_MGET_2, T_NFS_PROC, NULL, NULL)) {
		(void) m_freem(m);
		m = NULL;
	}
#endif NTRIGGER

	if (m == NULL) {
		p->cku_err.re_status = RPC_SYSTEMERROR;
		p->cku_err.re_errno = ENOBUFS;
		buffree(p);
		goto done;
	}

	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	(*(u_long *)(p->cku_outbuf)) = p->cku_xid;

	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);
	xdrs = &p->cku_outxdr;
	XDR_SETPOS(xdrs, p->cku_outpos);

	/*
	 * Serialize dynamic stuff into the output buffer.
	 */
	if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, argsp)) OR_UTRIG(T_NFS_ENCODE, T_NFS_PROC)) {
		p->cku_err.re_status = RPC_CANTENCODEARGS;
		(void) m_freem(m);
		goto done;
	}

	if (m->m_next == 0) {		/* XXX */
		m->m_len = XDR_GETPOS(&(p->cku_outxdr));
	}

#ifdef NTRIGGER
	{
	    	int trigval;
		struct mbuf *mm;

	    	if (utry_trigger(T_NFS_BAD_PKT, T_NFS_PROC, &trigval, 0)) {
		    	mm = rpc_badpacket(trigval, procnum,
					p->cku_xid, h->cl_auth);
			if (mm) {
				(void) m_freem(m);
				m = mm;
		    	}
	    	}
	}
#endif NTRIGGER

	if (p->cku_err.re_errno =
	    ku_sendto_mbuf(so, m, &p->cku_addr)) {
		p->cku_err.re_status = RPC_CANTSEND;
		goto done;
	}

	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xdr_results;

	for (rtries = RECVTRIES; rtries; rtries--) {
		s = splnet();
		while (so->so_rcv.sb_cc == 0) {
			/*
			 * Set timeout then wait for input or timeout
			 */
#ifdef RPCDEBUG
			rpc_debug(3, "callit: waiting %d\n", timohz);
#endif
			net_timeout(ckuwakeup, (caddr_t)p, timohz);
			so->so_rcv.sb_flags |= SB_WAIT;

			/* Changed to allow interrupts.  If we are going
			 * to allow interrupts, then sleep at a priority
			 * above PZERO and catch the interrupts, otherwise
			 * sleep at PRIBIO	dds, 3/4/86  HPNFS
			 */
			interrupted = sleep((caddr_t)&so->so_rcv.sb_cc,
				interruptable ? PZERO+1 | PCATCH : PRIBIO);

			net_untimeout(ckuwakeup, (caddr_t)p);
#ifdef NTRIGGER
			if (utry_trigger(T_NFS_INTR_1, T_NFS_PROC, 0, 0)) {
				interrupted = 1;
/*-----------------------------------------------------------------------------
				(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc, 0);
			* Changed interface to sbdrop
			* Should it be sbflush like SUN code?
-----------------------------------------------------------------------------*/
				(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
			}
#endif NTRIGGER
			if (interruptable && interrupted) {
				(void) splx(s);
				p->cku_err.re_status = RPC_INTR;
				p->cku_err.re_errno = EINTR;
				goto got_interrupted;  /* UGH! */
			}
				

			if (p->cku_flags & CKU_TIMEDOUT) {
				p->cku_flags &= ~CKU_TIMEDOUT;
				/*
				 * Set flag to reuse xid on retransmits.
				 */
				p->cku_flags |= CKU_RETRANSMIT;
				(void) splx(s);
				p->cku_err.re_status = RPC_TIMEDOUT;
				p->cku_err.re_errno = ETIMEDOUT;
				rcstat.rctimeouts++;
				goto done;
			}
		}

		if (so->so_error) {
			so->so_error = 0;
			(void) splx(s);
			continue;
		}

		p->cku_inmbuf = ku_recvfrom(so, &from);
		if (p->cku_inmbuf == NULL) {
			(void) splx(s);
			continue;
		}
		p->cku_inbuf = mtod(p->cku_inmbuf, char *);

		if (p->cku_inmbuf->m_len < sizeof(u_long)
			OR_UTRIG(T_NFS_TOO_SHORT, T_NFS_PROC)) {
			m_freem(p->cku_inmbuf);
			(void) splx(s);
			continue;
		}
		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		if (*((u_long *)(p->cku_inbuf))
		    != *((u_long *)(p->cku_outbuf))) {
			rcstat.rcbadxids++;
			m_freem(p->cku_inmbuf);
			(void) splx(s);
			continue;
		}
		/*
		 * Changed to leave at splnet from above, so this is 
 		 * no longer necessary: s = splnet();
		 */
		/*
		 * Flush the rest of the stuff on the input queue
		 * for the socket.
		 */
/*----------------------------------------------------------------------------
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc, 0);
		 * Changed interface to sbdrop
		 * Should it be sbflush like SUN code?
----------------------------------------------------------------------------*/
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
		(void) splx(s);
		break;
	} 

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		goto done;
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmbuf_init(xdrs, p->cku_inmbuf, XDR_DECODE);

	/*
	 * Decode and validate the response.
	 */
	if (xdr_replymsg(xdrs, &reply_msg)) {
		_seterr_reply(&reply_msg, &(p->cku_err));

		if (p->cku_err.re_status == RPC_SUCCESS) {
			/*
			 * Reply is good, check auth.
			 */
			if (! AUTH_VALIDATE(h->cl_auth,
			    &reply_msg.acpted_rply.ar_verf)
			    OR_UTRIG(T_NFS_BAD_AUTH, T_NFS_PROC)) {
				p->cku_err.re_status = RPC_AUTHERROR;
				p->cku_err.re_why = AUTH_INVALIDRESP;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				/* free auth handle */
				xdrs->x_op = XDR_FREE;
				(void)xdr_opaque_auth(xdrs,
				    &(reply_msg.acpted_rply.ar_verf));
			} 
		}
	} else {
		p->cku_err.re_status = RPC_CANTDECODERES;
	}
	m_freem(p->cku_inmbuf);

#ifdef RPCDEBUG
	rpc_debug(4, "cku_callit done\n");
#endif
done:
	if ( (p->cku_err.re_status != RPC_SUCCESS) &&
	     (p->cku_err.re_status != RPC_CANTENCODEARGS) &&
	     (--stries > 0) ) {
		rcstat.rcretrans++;
		timohz = backoff(timohz);
		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack o resources, wait a bit
			 * and try again.
			 */
			/*
			 * Support interruptability so that the user can
			 * abort the program if an "ifconfig down" was done.
			 */
			interrupted =  sleep((caddr_t)&lbolt,
				    interruptable ? PZERO+1 | PCATCH: PZERO-4);
#ifdef NTRIGGER
			if (utry_trigger(T_NFS_INTR_2, T_NFS_PROC, 0, 0)) {
				interrupted = 1;
			}
#endif NTRIGGER
			if ( interruptable && interrupted ) {
				p->cku_err.re_status = RPC_INTR;
				p->cku_err.re_errno = EINTR;
				goto got_interrupted;
			}
		}

		/*
		 * If the outbound xdr x_public field is set it is a pointer to 
		 * a struct whose first field references a function that waits 
		 * for the outbound "user" buffer to be freed by the lower 
		 * level network routines and then frees any related resources 
		 * including the struct referenced by x_public.
		 */
		xdrs = &(p->cku_outxdr);
		if (xdrs->x_public) {
			(**((int (**)())xdrs->x_public))(xdrs->x_public);
			xdrs->x_public = 0;
		}
		goto call_again;
	}

got_interrupted:
	crfree(u.u_cred);
	u.u_cred = tmpcred;
	/* 
	 * Restore the signal mask if we played with it for interrupts.
	 */
	if ( interruptable ) {
		s = spl6();
		u.u_procp->p_sigmask = save_mask;
		(void) splx(s);
	}

	/*
	 * If the outbound xdr x_public field is set it is a pointer to a 
	 * struct whose first field references a function that waits for the
	 * outbound "user" buffer to be freed by the lower level network 
	 * routines and then frees any related resources including the 
	 * struct referenced by x_public.
	 *
	 * Copied from server side read reply handling - see svckudp_send in
	 * rpc/svc_kudp.c - also see wafree, wawakeup and xdr_writeargs in 
	 * nfs/nfs_xdr.c.
	 */
	xdrs = &(p->cku_outxdr);
	if (xdrs->x_public) {
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
		xdrs->x_public = 0;
	}
	/*
	 * Insure that the buffer is not busy prior to releasing the client
	 * handle.  This is done because the low-level network routines may
	 * not be done with the buffer yet.  If we let it go, another client
	 * may grab the buffer and change its contents, causing data corruption.

	 * dds 4/8/87
	 */
#ifdef MP
        old = sleep_lock();
        while (p->cku_flags & CKU_BUFBUSY) {
                p->cku_flags |= CKU_BUFWANTED;
                net_timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
                sleep_then_unlock((caddr_t)&p->cku_outbuf, PZERO-3,old);
                (void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
                if (!(p->cku_flags & CKU_BUFBUSY))
                {  net_untimeout(wakeup, (caddr_t)&p->cku_outbuf);
                   goto loop2_done; }
                old  = sleep_lock();
        }
        sleep_unlock(old);
loop2_done:
#else /* not MP */
	s = splimp();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		net_timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
		sleep((caddr_t)&p->cku_outbuf, PZERO-3);
/*----------------------------------------------------------------------------
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc, 0);
		* Changed interface to sbdrop
		* Should it be sbflush like SUN code?
-----------------------------------------------------------------------------*/
                if (!(p->cku_flags & CKU_BUFBUSY)) {
                        /* probably woke up from buffree        */
                        net_untimeout(wakeup, (caddr_t)&p->cku_outbuf);
                }
		(void) sbdrop(&so->so_rcv, (&so->so_rcv)->sb_cc);
	}
	(void) splx(s);
#endif /* MP*/
	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
		wakeup((caddr_t)h);
	}
	if (p->cku_err.re_status != RPC_SUCCESS) {
		rcstat.rcbadcalls++;
	}
	return (p->cku_err.re_status);
}


/* BEGIN_IMS ckuwakeup *
 ********************************************************************
 ****
 **** 			ckuwakeup(p)		(series 300 and 800)
 ****
 **** 			realckuwakeup(p)	(series 300 only)
 ****
 ********************************************************************
 * Input Parameters
 *	p	client handle private data area
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
 * 	Signals client that an RPC call attempt has timed out by setting
 *	the CKU_TIMEDOUT flag and waking up the client process.
 *
 * Algorithm
 *	set CKU_TIMEDOUT flag
 *	wakeup(client process sleeping on socket receive queue)
 *
 * Concurrency
 *	controlled by using net_timeout()
 *
 * To Do List
 *
 * Modification History
 *	2/18/88		dds		removed 300 specific code due
 *					to change to net_timeout, which
 *					removes timing dependencies.
 *
 * External Calls
 *	sbwakeup	socket buffer wakeup
 *
 * Called By 
 *	scheduled by net_timeout() to be called
 *
 ********************************************************************
 * END_IMS ckuwakeup */


ckuwakeup(p)
	register struct cku_private *p;
{

#ifdef RPCDEBUG
	rpc_debug(4, "cku_timeout\n");
#endif
	p->cku_flags |= CKU_TIMEDOUT;
	sbwakeup(&p->cku_sock->so_rcv);
}


/* BEGIN_IMS clntkudp_error *
 ********************************************************************
 ****
 ****			clntkudp_error(h, err)
 ****
 ********************************************************************
 * Input Parameters
 *	h	client handle pointer
 *	err	pointer to structure for returning error indication
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
 *	clntkudp_error returns the RPC error information for this client
 *	rpc handle. It is called after a reply message is received.
 *
 * Algorithm
 *	get error information from rpc_err structure in client handle
 *	if (status != RPC_SUCCESS and errno == 0)
 *		errno = EIO
 *
 * Concurrency
 * 	none
 *
 * Notes
 *	clntkudp_error is called through the CLNT_GETERR macro. It is
 *	an entry in the udp_ops operations vector.
 *
 * Modification History
 *
 * External Calls
 *	none
 *
 * Called By
 *	rfscall
 *
 ********************************************************************
 * END_IMS clntkudp_error */

void
clntkudp_error(h, err)
	CLIENT *h;
	struct rpc_err *err;
{
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
	/*
	 * if we had an RPC failure, make sure that the errno value
	 * is set to SOMETHING so that NFS does not get confused.
	 */
	if ( err->re_status != RPC_SUCCESS && err->re_errno == 0 )
		err->re_errno = EIO;
}


/* BEGIN_IMS clntkudp_freeres *
 ********************************************************************
 ****
 **** 		clntkudp_freeres(cl, xdr_res, res_ptr)
 ****
 ********************************************************************
 * Input Parameters
 *	cl		client handle for this call
 * 	xdr_res 	XDR routine to FREE arguments
 * 	res_ptr		pointer to the results to free
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	1	success
 *	0	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	After a remote procedure call, this routine can be used to free
 *	any resources used to decode the result parameters. This routine
 *      is currently never called. XDR_FREE is a noop for all cases,
 *	except strings.
 *
 * Algorithm
 *	set XDR operation to FREE
 *	call XDR routine to free results
 *	return (result)
 *
 * Concurrency
 * 	none
 *
 * Notes
 *	This is a routine that is never called. It is included for 
 *	completeness as an entry in the udp_ops operations vector.
 *
 * External Calls
 *	the XDR routine passed as a parameter
 *
 * Called By
 *	never called
 *
 ********************************************************************
 * END_IMS clntkudp_freeres */

static bool_t
clntkudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}


/* BEGIN_IMS clntkudp_abort *
 ********************************************************************
 ****
 ****			clntkudp_abort()
 ****
 ********************************************************************
 * Input Parameters
 *	none
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
 *	This is a noop that is never called. It is included for 
 *	completeness as an entry in the udp_ops operations vector.
 *
 * Notes
 *
 * External Calls
 *	none
 *
 * Called By
 *	never called
 *
 ********************************************************************
 * END_IMS clntkudp_abort */

void 
clntkudp_abort()
{
}


/* BEGIN_IMS clntkudp_destroy *
 ********************************************************************
 ****
 ****			clntkudp_destroy(h)
 ****
 ********************************************************************
 * Input Parameters
 *	h	pointer to client handle to destroy
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
 * 	clntkudp_destroy destroys the client rpc handle. It frees the
 *	space used for output buffer, private data, and handle structure,
 *	and closes the socket for this handle. This routine is currently
 *	never called. A limited number of client handles are recycled for
 *	re-use.
 *
 * Algorithm
 *	shrink the credits in the nfs macct
 *	close the socket associated with client handle
 *	free memory for output buffer
 *	free memory for for private data area (including handle)
 *
 * Concurrency
 * 	none
 *
 * Notes
 *	clntkudp_destroy is included in the udp_ops operations vector
 *	for completeness, but is currently not used.
 *
 * Modification History
 *	11/19/87	lmb	added macct ma_shrink
 *
 * External Calls
 *	soclose, kmem_free
 *
 * Called By
 *	never called
 *
 ********************************************************************
 * END_IMS clntkudp_destroy */

void
clntkudp_destroy(h)
	CLIENT *h;
{
	register struct cku_private *p = htop(h);

#ifdef RPCDEBUG
	rpc_debug(4, "cku_destroy %x\n", h);
#endif



/*----------------------------------------------------------------------------
	ma_shrink(nfs_macct, MBUFS_PER_CLIENT);
	(void) soclose(p->cku_sock, 0);
	*  Removed reference to maccts
	*  Changed interface to soclose
-----------------------------------------------------------------------------*/
	(void) soclose(p->cku_sock);
	kmem_free((caddr_t)(caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)(caddr_t)p, (u_int)sizeof(*p));
}


/* BEGIN_IMS bindresvport *
 ********************************************************************
 ****
 ****			bindresvport(so)
 ****		
 ********************************************************************
 * Input Parameters
 *	so 	UDP socket to which to bind port
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	0 		success
 *	otherwise	error number 
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	bindresvport binds a reserved port number (< 1024) to the UDP socket
 *	passed as a parameter. The purpose of this is to allow the server
 *	receiving the RPC call to at least verify that the call originated
 *	with a process that can assume a root identity. This allows some
 *	minimal measure of security.
 *
 * Algorithm
 *	get an mbuf to hold internet address
 *	if (no mbufs)
 *		return(ENOBUFS)
 *	initialize port to wildcard INADDR_ANY
 *	add address to option structure
 *	duplicate credential structure
 *	assume root identity (uid = 0)
 *	while (error == EADDRINUSE) {	
 *		try next reserved port number
 *		bind socket to port number
 *      }
 *	free duplicate credential structure
 *	restore previous identity
 *	return(error)
 *
 *
 * Concurrency
 * 	none
 *
 * To Do List
 *
 * Notes
 * 	With the possibility of credentials being shared among multiple
 * 	processes, we have to make sure we have our own set of credentials
 * 	before changing the ID to superuser to get a reserved port.
 *
 * 	NOTE: changed to avoid the crcopy and the saving/restoring of
 * 	the uid.  The problem with the crcopy is that some functions may
 * 	be assuming that they have the same set of credentials after the
 * 	call to this function.  The problem with changing using the 
 * 	saved uid to restore it after the call to sobind is that we are
 * 	assuming that sobind did not play with the credentials.  While
 * 	this is probably true, we do not want to rely on this. dds.
 *
 * Modification History
 *			dds	changed to avoid cropy
 *	11/2/87		lmb	added trigger, converted to NS logging
 *
 * External Calls
 *	m_get, mtod, sobind
 *	crdup, crfree
 *	NS_LOG
 *
 * Called By 
 *	clntkudp_create
 *
 ********************************************************************
 * END_IMS bindresvport */

static
bindresvport(so)
	struct socket *so;
{
	struct sockaddr_in *sin;
	struct mbuf *m;
	u_short i;
	int error;
	struct ucred *tcred;

#	define MAX_PRIV	(IPPORT_RESERVED-1)
#	define MIN_PRIV	(IPPORT_RESERVED/2)

/*----------------------------------------------------------------------------
	m = m_get(M_WAIT, MA_NULL, MT_SONAME);
	*  Changed interface to m_get
---------------------------------------------------------------------------*/
	m = m_get(M_WAIT, MT_SONAME);

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_MGET_1, T_NFS_PROC, NULL, NULL)) {
		(void) m_freem(m);
		m = NULL;
	}
#endif NTRIGGER

	if (m == NULL) {
		NS_LOG(LE_NFS_BIND_MGET, NS_LC_RESOURCELIM, NS_LS_NFS, 0);
		return(ENOBUFS);
	}

	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	m->m_len = sizeof(struct sockaddr_in);
/*----------------------------------------------------------------------------
        optinit(opt);
	optadd(opt, NSO_ADDRESS, m);
	*  Removed references to options
----------------------------------------------------------------------------*/
	
	/*
	 * With the possibility of credentials being shared among multiple
	 * processes, we have to make sure we have our own set of credentials
	 * before changing the ID to superuser to get a reserved port.
	 * NOTE: changed to avoid the crcopy and the saving/restoring of the uid
	 */
	/* u.u_cred = crcopy(u.u_cred); */
	tcred = u.u_cred;
	u.u_cred = crdup(tcred);
 
	u.u_uid = 0;
	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
/*----------------------------------------------------------------------------
		error = sobind(so, opt);
	 *  Removed reference to options
-----------------------------------------------------------------------------*/
		error = sobind(so, m);
	}

	crfree(u.u_cred);
	u.u_cred = tcred;
/*----------------------------------------------------------------------------
	optfree(opt);
	 *  Removed reference to options
-----------------------------------------------------------------------------*/
	(void) m_freem(m);
	return (error);
}
