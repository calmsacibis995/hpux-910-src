/*
 * $Header: nipc_srd.c,v 1.6.83.4 93/09/17 19:11:36 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_srd.c,v $
 * $Revision: 1.6.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:11:36 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_srd.c $Revision: 1.6.83.4 $";
#endif

/* 
 * The routines contained in this file make up the kernel socket registry
 * daemon process.
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/malloc.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_name.h"
#include "../nipc/nipc_hpdsn.h"
#include "../netinet/in.h"
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */
#include "../h/ki_calls.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

extern struct mbuf * sou_sbdqmsg();
struct socket	*srd_pxpso;  /* global sockregd pxp socket pointer */

/* srd_buildreply() is called from srd_main(). */
/* It parses a received request, drops it if   */
/* it's not valid, otherwise it generates a    */
/* reply from the request and returns it.      */

struct mbuf *
srd_buildreply(m)

struct mbuf *m;  /* mbuf data chain of request */

{
	struct nm_lookuphdr	*hdr;	  /* pointer to fixed header          */
	struct nm_lookupreq	*req;	  /* pointer to request data          */
	char			*name;    /* pointer to request socket name   */
	int			nlen;     /* length of request socket name    */
	int			msglen;   /* length of request message        */
	struct nm_lookupreply   *reply;	  /* pointer to reply data            */
	struct mbuf		*cspr;    /* csite path report for reply      */
	int			protocol; /* protocol for reply		      */
	int			sockkind; /* socket kind for reply	      */
	int 			error;	  /* error code for reply	      */

	/* ensure the request message is contiguous in a single mbuf */
	msglen = m_len(m);
	if (msglen > NM_MAX_REQ_LEN)
		goto ERROR_RETURN;
	if ((m->m_off > MMAXOFF) || (m->m_len < msglen) || 
	    (m->m_off & BYTE_ALIGN)) {
		if ((m = m_pullup(m, msglen)) == 0)
			goto ERROR_RETURN;
		if (m->m_next) {
			m_freem(m->m_next);
			m->m_next = 0;
		}
	}

	/* validate the request message type and length */
	req = mtod(m, struct nm_lookupreq *);
	hdr = &req->req_hdr;
	if ((hdr->hdr_msgtype != NM_REQUEST) ||	   /* bad type      */
	    (hdr->hdr_msglen != msglen) ||	   /* bad msg length*/
	    (hdr->hdr_msglen <= NM_REQ_LEN))       /* msg too small */
		goto ERROR_RETURN;

	/* verify the version id */
	if (hdr->hdr_version != NM_VERSION) {
		error = NM_VERSION_RESULT;
		goto BUILD_REPLY;
	}

	/* get the socket name from the request and do a local lookup on  */
	/* the name to get the protocol, sockkind, and cspr for the reply */
	nlen = req->req_endptr - req->req_nameptr;
	name = ((char *)req) + req->req_nameptr;
	nm_downshift(name, nlen);
	error = nm_locallookup(name, nlen, PATH_NOLOOP, &protocol, &sockkind, &cspr);
	if (error == E_NAMENOTFOUND) {
		error = NM_NOTFOUND_RESULT;
	} else
		if (error) 
			goto ERROR_RETURN;
		else
			error = NM_SUCCESS_RESULT;

BUILD_REPLY:
	/* convert the request header into a reply header */
	reply = mtod(m, struct nm_lookupreply *);
	hdr->hdr_msgtype = NM_REPLY;
	hdr->hdr_error	 = error;

	/* depending on the error code, add the reply data */
	/* and compute the reply message length		   */
	if (error == NM_SUCCESS_RESULT) {
		reply->reply_count = 1;  /* reply contains only 1 path report */
		reply->reply_sockkind = sockkind;
		m->m_len = NM_REPLY_LEN;
		m_cat(m, cspr);
		hdr->hdr_msglen = m_len(m);
	} else {  /* this is an error reply */
		m->m_len = NM_HDR_LEN;
		hdr->hdr_msglen = NM_HDR_LEN; 
	}

	return (m);

ERROR_RETURN:
	m_freem(m);
	return (0);

}  /* srd_buildreply */


/* srd_init() is called from srd_main.  It creates the    */
/* PXP reply socket with the well-known sockregd address. */

srd_init()

{
	struct nipc_protosw *proto; /* pxp protoswitch			      */
	struct sockaddr_in  *addr;  /* sockaddr for binding well-known addr   */
	struct mbuf	*nam;	/* mbuf for well-known addr to bind to socket */
	int		error;	/* local error				      */
	struct socket *sou_create();

	/* get a pointer to the pxp protosw */
	if (error = mi_findproto(HPDSN_DOM, &proto, NSP_HPPXP, NS_REPLY))
		return (error);

	/* allocate a socket structure and nipccb for the reply socket */
	srd_pxpso = sou_create(proto);

	/* attach the protocol to the socket */
	if (error = (*proto->np_bsd_protosw->pr_usrreq)(srd_pxpso, PRU_ATTACH, 
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0))
		goto ERROR_RETURN;

	/* get a sockaddr mbuf and initialize it to the sockregd pxp port */
	nam = m_get(M_WAIT, MT_SONAME);
	nam->m_len = sizeof(struct sockaddr);
	bzero((caddr_t)nam, sizeof(struct sockaddr));
	addr = mtod(nam, struct sockaddr_in *);
	addr->sin_family = AF_INET;
	addr->sin_port   = NM_SERVER;

	/* bind the well-known port address to the socket */
	error = (*proto->np_bsd_protosw->pr_usrreq)(srd_pxpso, PRU_BIND, 
				(struct mbuf *)0, nam, (struct mbuf *)0);
	m_free(nam);  /* finished with sockaddr mbuf */
	if (error)
		goto ERROR_RETURN;
	
	return (0);

ERROR_RETURN:
	/* free the socket resources */
	(void) sou_delete(srd_pxpso, (struct file *) 0);
	return (error);

}  /* srd_init */


struct mbuf *
srd_recvreq (msgid)

int		*msgid; /* (output) pxp message id of the received request */

{
	struct mbuf	*m;	/* request message mbuf data chain */
	int		*req;	/* pointer to reply message data   */
	int		s;	/* spl setting			   */

	/* wait for a message to arrive on the pxp reply socket */
	if (srd_pxpso->so_rcv.sb_cc == 0) {
		srd_pxpso->so_rcv.sb_flags |= SB_WAIT;
		sleep((caddr_t)&srd_pxpso->so_rcv.sb_cc, PZERO-1);
	}

	/* a message has arrived - take it off the receive queue */
	m = sou_sbdqmsg(&srd_pxpso->so_rcv, 0);

	/* extract the message id from the beginning of the request mbuf */
	req = mtod(m, int *);
	*msgid = *req;

	/* drop the msgid and error code from the request mbuf chain;      */
	/* if this leaves an empty mbuf in front of the chain then free it */
	m_adj(m, 2*(sizeof(int)));
	if (m->m_len == 0)
		m = m_free(m);

	return (m);

}  /* srd_recvreq */


/* srd_main() is the main routine for the socket registry daemon process.    */
/* It creates a PXP reply socket with a well-known address and loops         */
/* forever sleeping on the socket, receiving requests and sending replies.   */
/* Sockregd replies to requests in one of four ways:                         */
/*	1) No reply - the request was invalid or ENOBUFS was encountered.    */
/*	2) NM_VERSION_RESULT - the version # of the request was bad.         */
/*	3) NM_NOTFOUND_RESULT - the socket name was not in the socket reg    */
/*	4) NM_SUCCESS_RESULT  - the lookup succeeded and reply contains data.*/

void
srd_main()

{
	struct mbuf	*m;	/* pointer to request/reply data chain    */
	int		msgid;	/* pxp message id of the received request */
	struct protosw	*proto;	/* bsd protosw for pxp			  */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/* MPNET: turn MP protection on.  Note: this routine is
	** NEVER supposed to exit (short of srd_init() failing), but
	** the calling process is put to sleep at times, which "gives
	** up" the MP exclusion.  It will be re-acquired, automagically,
	** by the kernel when it wakes up the calling process.
	*/
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* create the well-known pxp reply socket */
	if (srd_init() != 0) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		return; 
	}
	proto = ((struct nipccb *)(srd_pxpso->so_ipccb))->n_protosw->np_bsd_protosw;

	/* loop forever handling incoming requests */
	while (TRUE) {
		m = srd_recvreq(&msgid);
		m = srd_buildreply(m);
		if (!m) 
			(*proto->pr_usrreq)(srd_pxpso, PRU_REQABORT, (struct mbuf *)0,
				(struct mbuf *)0, (struct mbuf *)&msgid);
		else
			(*proto->pr_usrreq)(srd_pxpso, PRU_SEND, m, (struct mbuf *)0,
						(struct mbuf *)&msgid);
	}

}  /* srd_main */


/* srd_create() is called at system initialization. */
/* It spawns off the kernel socket registry daemon. */

void
srd_create()

{
	int i = 0;
	char *proc_name = "sockregd";
	struct proc *pp, *allocproc();
	extern void pstat_cmd();
	
	pp = allocproc(S_DONTCARE);
	proc_hash(pp, 0, PID_SOCKREGD, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_DAEMON, pp)) {
		u.u_procp->p_flag |= SSYS;

		/* let pstat know about us */
		pstat_cmd(u.u_procp, proc_name, 1, proc_name);
		u.u_syscall = KI_SOCKREGD;

		srd_main();
		panic("srd_create");
	}
}  /* srd_create */
