/*
 * $Header: pxp.c,v 1.3.83.4 93/09/17 19:12:06 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/pxp.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:12:06 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) pxp.c $Revision: 1.3.83.4 $";
#endif

/*
 * This file contains routines which make up the HP-UX implementation of the
 * PXP protocol.
 */


#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/ns_ipc.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/pxp.h"
#include "../nipc/pxp_var.h"


/* PXP protocol global variables */

int pxp_sndseq, pxp_rcvseq;
struct inpcb    xpb;
struct pxpstat	pxpstat;

/* pxp_attach() is called when PXP gets a valid PRU_ATTACH */
/* user request.  It attaches the pxp protocol to the      */
/* socket by allocating an inpcb and a pxpcb head.  It     */
/* also allocates buffer space for the socket's send Q.    */

pxp_attach (so)

struct socket  *so;  /* socket to attach to */

{
	struct inpcb *inp;	/* internet control block for socket */
	struct pxpcb *xp;	/* pxp control block head	     */
	int     error = 0;	/* local error			     */
	u_long	pxp_rcvspace;	/* receive socket buffer space	     */

	/* Reserve space for sockbuf receive queue */
	pxp_rcvspace = PXPQUEUELIMIT * (PXPMAXDATA + PXPIP_HDRSIZE);
	if (!sbreserve (&so->so_rcv, min(pxp_rcvspace,SB_MAX)))
		panic("pxp_attach");

	/* Allocate a new pxp inpcb for the socket */
	if (error = in_pcballoc (so, &xpb))
		return (error);
	inp = sotoinpcb(so);

	/* Allocate a pxpcb head attaching it to the inpcb */
	MALLOC(xp, struct pxpcb *, sizeof(struct pxpcb), M_PCB, M_WAITOK); 
	bzero((caddr_t)xp, sizeof(struct pxpcb));
	mbstat.m_mtypes[MT_PCB]++;
	inp->inp_ppcb = (caddr_t) xp;

	/* initialize the pxpcb head */
	xp->xp_prev = PXPCB_NULL;
	xp->xp_next = PXPCB_NULL;
	xp->xp_type = ((struct nipccb *)(so->so_ipccb))->n_type;
	xp->xp_inpcb = inp;
	xp->xp_num_req = 0;

	return(0);

}  /* pxp_attach */


/* pxp_discard() frees up all resources belonging to  */
/* the specified pxpcb and dequeues it from the list. */

void
pxp_discard (xp)

struct pxpcb  *xp;

{
	struct pxpcb	*xphead;

	/* decrement xp_num_req in the pxpcb head */
	xphead = (struct pxpcb *)(((struct inpcb *)(xp->xp_inpcb))->inp_ppcb);
	xphead->xp_num_req--;

	/* free the request message if there is one */
	if (xp->xp_msg != 0)
		m_freem(xp->xp_msg);

	/* dequeue the pxpcb from the list and free it */
	if (xp->xp_next != PXPCB_NULL)
		xp->xp_next->xp_prev = xp->xp_prev;
	xp->xp_prev->xp_next = xp->xp_next;
	FREE(xp, M_PCB);
	mbstat.m_mtypes[MT_PCB]--;

}  /* pxp_discard */


/* pxp_notify() is called from pxp_ctlinput when the lower */
/* layer (ip) reports an error in communication with a     */
/* remote destination.  All requestors waiting for a reply */
/* from the destination are notified with an error.        */

void
pxp_notify (dst, errno)

struct in_addr *dst;
int     errno;

{
	struct inpcb	*inp;		/* pointer to a pxp inpcb	      */
	struct socket	*so;		/* socket attached to a pxp inpcb     */
	struct pxpcb	*xp, *next_xp;	/* pointers used to search thru pxpcbs*/
	struct mbuf	*m = 0;		/* mbuf used to pass up error replies */
	int		*reply;		/* pointer to error reply data	      */

	/* loop through all pxpcbs searching for those whose foriegn address */
	/* matches the input dst address;  if the pxpcb belongs to a request */
	/* socket then pass an error reply up to the user;  discard the      */
	/* pxpcb in either case (request or reply)			     */

	for (inp = xpb.inp_next; inp != &xpb; inp = inp->inp_next) {
		xp = intopxpcb (inp);
		while (xp != 0) {
	    		next_xp = xp->xp_next;
	    		if (xp->xp_sin_faddr.s_addr == dst->s_addr) {
				so = inp->inp_socket;
				if (xp->xp_type == NS_REQUEST) {
					/* get a mbuf for the error reply */
					if ((m = m_get(M_DONTWAIT, MT_DATA)) == 0) {
						xp = next_xp;
						continue;
					}

					/* set the reply msgid and errno */
					m->m_off = MMINOFF;
					m->m_len = 2 * sizeof(int);
					reply = mtod (m, int *);
					*reply = xp->xp_snd_msgid;
					*(++reply) = errno;

					/* queue the reply and call wakeup */
					sbappendrecord (&so->so_rcv, m);
					sorwakeup(so);
				} /* if xp_type = NS_REQUEST */

				/* discard the pxpcb */
				(void) pxp_discard(xp);

			} /* if internet addresses match */

			xp = next_xp;

		}  /* while (xp != 0) */

	} /* for */

} /* pxp_notify */


/* pxp_timer() is called from pxp_fasttimo when a retransmission */
/* timer has popped for a particular request.  If the maximum    */
/* retransmissions for the request has been exhausted then an    */
/* error reply is queued tothe socket;  otherwise, the request   */
/* data saved in the pxpcb is copied and retransmitted.          */

float   pxp_backoff[PXPMAXTOCNT] =
{
    1.0, 1.2, 1.4, 1.7, 2.0, 3.0, 5.0, 8.0, 16.0, 32.0
};

void
pxp_timer (xp)

struct pxpcb  *xp;

{
	struct inpcb	*inpcb = xp->xp_inpcb;	
	struct socket	*so = inpcb->inp_socket;
	struct mbuf	*m;
	int		*reply;

	/* If the retry count is exhausted then inform the socket user and */
	/* discard the request pxpcb; otherwise retry the transmission     */
	if (xp->xp_retries-- <= 0) {
		/* get a mbuf for the error reply */
		m = m_get(M_WAIT, MT_DATA);

		/* set the reply msgid and errno */
		m->m_off = MMINOFF;
		m->m_len = 2 * sizeof(int);
		reply = mtod(m, int *);
		*reply = xp->xp_snd_msgid;
		*(++reply) = (int)(E_RETRYEXHAUSTED);

		/* queue the reply and wakeup the receiver */
		sbappendrecord (&so->so_rcv, m);
		sorwakeup(so);
		(void) pxp_discard(xp);

	} else {
		/* calculate the next retransmission timeout interval */
		xp->xp_timer = pxp_backoff[++(xp->xp_rxtshift)] * PXPINITTO;

		/* copy the request message for transmission */
		m = m_copy(xp->xp_msg, 0, (int)M_COPYALL);
		if (m != 0)
			ip_output(m, (struct mbuf *)0, (struct route *)0, 0);

		/* note that if m_copy failed we're out of memory so  */
		/* we merely wait until the next timeout to try again */
	}

}  /* pxp_timer */


/* pxp_cltinput() is called by the lower layers     */
/* (ip) through the protocol switch.  It is used to */
/* inform PXP of certain errors (icmp errors).      */

pxp_ctlinput (cmd, arg)

int     cmd;	/* command or error information from layer 3 */
caddr_t arg;	/* optional pointer to icmp structure 	     */

{
	struct  in_addr *dst;
	extern  u_char	inetctlerrmap[];

	/* ensure the command arg is one we know about */
	if (cmd < 0 || cmd > PRC_NCMDS)
		return;

	/* process the command */
	switch (cmd) {

		case PRC_ROUTEDEAD: 
			break;

		case PRC_QUENCH: 
			break;
	
		/* these are handled by ip */
		case PRC_IFDOWN: 
		case PRC_HOSTDEAD: 
		case PRC_HOSTUNREACH: 
			break;

		default: 
			dst = &((struct icmp   *) arg) -> icmp_ip.ip_dst;
			(void) pxp_notify (dst, (int) inetctlerrmap[cmd]);

	}
}  /* pxp_ctlinput */


/* pxp_fastimo() is called through the protocol switch */
/* every 200ms.  It updates all active pxpcb timers    */
/* and calls the internal pxp_timer() routine for each */
/* timer that has expired.                             */

pxp_fasttimo ()

{
	struct inpcb	*inp;
	struct pxpcb	*xp, *next_xp;
	struct socket		*so;

	/* loop through all the pxp inpcbs and for each inpcb whose pxpcbs   */ 
	/* are type NS_REQUEST, traverse through the attached pxpcbs,        */
	/* updating the timers and call pxp_timer() if the timer has expired */

	for (inp = xpb.inp_next; inp != &xpb; inp = inp->inp_next) {
		so = inp->inp_socket;
		xp = intopxpcb (inp);
		if ((xp) && (xp->xp_type == NS_REQUEST)) {
			while (xp != 0) {
			    next_xp = xp->xp_next;
			    if ((--xp->xp_timer) <= 0) 
				(void) pxp_timer(xp);
			    xp = next_xp;
			}
		}
	}


}  /* pxp_fasttimo */


/* pxp_init() is called from nipc_init() */
/* during system initialization.         */

pxp_init ()

{
	/* initialize the global pxp inpcb head to point to itself */
	xpb.inp_next = xpb.inp_prev = &xpb;

	/* initialize the global send and receive sequence numbers */
	pxp_sndseq = 1;
	pxp_rcvseq = 1001;

	/* initialize global stats */
	pxpstat.pxp_hdrops = 0;
	pxpstat.pxp_badsum = 0;
	pxpstat.pxp_badlen = 0;
}


/* pxp_input() is called through the protocol switch  */
/* by the lower layers (ip) when inbound data arrives */
/* for PXP.  If the packet is valid it is sent to the */
/* upper layer.                                       */

pxp_input (m0, ifp)

struct mbuf *m0; 	/* input data chain		    */
struct ifnet *ifp;	/* interface packet was received on */

{
	struct pxpiphdr	*pi;	/* pxp and ip header      */
	struct inpcb		*inp;	/* target inpcb of msg    */
	struct pxpcb		*xphead,/* head of pxpcbs for inp */
				*xp;	/* pxpcb pinter		  */
	struct mbuf		*m = m0;/* pointer to mbuf data   */
	struct in_addr		zeroin_addr; /* used in in_pcblookup */
	int			msgid,	/* msgid to pass up w/msg */
				*msg;   /* msg pointer		  */

	/* get the ip and pxp headers together in first mbuf;	*/
	/* note:  ip leaves ip header in first mbuf		*/
	pi = mtod(m, struct pxpiphdr *);
	if (((struct ip *)pi)->ip_hl > (sizeof(struct ip) >> 2))
		ip_stripoptions((struct ip *)pi, (struct mbuf *)0);

	if  (((m->m_off > MMAXOFF) || (m->m_len < PXPIP_HDRSIZE) ||
	     (m->m_off & WORD_ALIGN))) {
		if ((m = m_pullup(m, PXPIP_HDRSIZE)) == 0)
			goto DROP_MSG;
		pi = mtod(m, struct pxpiphdr *);
	}	

	/* verify the pxp version and class */
	if ((pi->pi_ver != PXPVER) || (pi->pi_cls != PXPCLS) ||
	    (pi->pi_ebit != 0))
		goto DROP_MSG;

	/* locate the inpcb for this message; match on pxp port # */
	zeroin_addr.s_addr = INADDR_ANY;
	inp = in_pcblookup(&xpb, zeroin_addr, 0, pi->pi_dst,
			   pi->pi_dport, INPLOOKUP_WILDCARD);
	if (inp == 0)
		goto DROP_MSG;
	xphead = (struct pxpcb *)inp->inp_ppcb;

	/* process the message based on whether it's a request or a reply */
	if (xphead->xp_type == NS_REQUEST) {
		/* verify this is a reply message */
		if (!pi->pi_rbit) 
			goto DROP_MSG;

		/* find the request pxpcb for this reply */
		for (xp = xphead->xp_next; xp != PXPCB_NULL; xp = xp->xp_next) {
			if (xp->xp_snd_msgid == pi->pi_msgid) 
				break;
		}
		if (xp == PXPCB_NULL)
			goto DROP_MSG;

		/* extract the msgid and discard the request pxpcb */
		msgid = xp->xp_snd_msgid;
		(void) pxp_discard(xp);
	} else {
		/* verify this is a request message */
		if (pi->pi_rbit)
			goto DROP_MSG;

		/* see if it's OK to queue another request */
		if (xphead->xp_num_req == PXPQUEUELIMIT)
			goto DROP_MSG;
	
		/* make sure this is a new request and not a duplicate */
		for (xp = xphead->xp_next; xp != PXPCB_NULL; xp = xp->xp_next) {
			if ((xp->xp_sin_faddr.s_addr == pi->pi_src.s_addr) &&
			    (xp->xp_sin_port == pi->pi_sport) &&
			    (xp->xp_snd_msgid == pi->pi_msgid))
				break;
		}
		if (xp != PXPCB_NULL)  /* duplicate request */
			goto DROP_MSG;

		/* allocate a new pxpcb for the request */
		MALLOC(xp, struct pxpcb *, sizeof(struct pxpcb), M_PCB, M_NOWAIT);
		if (xp == 0) 
			goto DROP_MSG;
		bzero((caddr_t)xp, sizeof(struct pxpcb));
		mbstat.m_mtypes[MT_PCB]++;

		/* initialize the pxpcb, linking it right after the head */
		xphead->xp_num_req++;
		if (xphead->xp_next != PXPCB_NULL)
			xphead->xp_next->xp_prev = xp;
		xp->xp_next		= xphead->xp_next;
		xphead->xp_next		= xp;
		xp->xp_prev		= xphead;
		xp->xp_type		= xphead->xp_type;
		xp->xp_inpcb		= inp;
		xp->xp_sin_port		= pi->pi_sport;
		xp->xp_sin_faddr.s_addr = pi->pi_src.s_addr;
		xp->xp_sin_laddr.s_addr = pi->pi_dst.s_addr;
		xp->xp_snd_msgid	= pi->pi_msgid;

		/* generate a new receive sequence number for the request */
		if (++pxp_rcvseq <= 0)
			pxp_rcvseq = 1;
		msgid = xp->xp_rcv_msgid = pxp_rcvseq;
	}

	/* strip off the pxp and ip headers, leaving room for the  */
	/* message id and error at the front of the mbuf; re-align */
	m_adj(m, PXPIP_HDRSIZE - 2*(sizeof(int)));
	m_pullup(m, m->m_len);

	/* add the msgid and error fields and pass the message up */
	msg = mtod(m, int *);
	msg[0] = msgid;
	msg[1] = 0;
	sbappendrecord(&inp->inp_socket->so_rcv, m);
	sorwakeup(inp->inp_socket);
	return(0);

DROP_MSG:
	m_freem(m);
	return (0);

}  /* pxp_input */


/* pxp_send() is called when a valid PRU_SEND user request */
/* is received.  It builds pxp reqeust and reply datagrams */
/* and sends them to the lower layer.  If the datagram is  */
/* a request, a pxpcb is allocated for the request and the */
/* data is queued to the pxpcb for retransmission.         */

pxp_send (so, m, nam, opt)

struct socket *so;	/* socket doing the send */
struct mbuf *m;		/* send data chain	 */
struct mbuf *nam;	/* optional send address */
struct mbuf *opt;	/* (input/output) msgid	 */

{
	struct inpcb		*inp  = sotoinpcb(so);
	struct sockaddr_in 	*addr = (struct sockaddr_in *)nam;
	struct pxpcb		*xphead = (struct pxpcb *)(inp->inp_ppcb);
       	struct pxpcb 		*xp;		/* pxpcb for the message    */
	struct pxpiphdr 	*pi;		/* pxp ip extended header   */
	struct mbuf		*m0 = 0;	/* mbuf for pxpip header    */
	int 			msgid,		/* local msgid from opt     */
				len,		/* length of send data      */
        			error = 0;	/* local error		    */

	/* depending upon the pxp type , get the pxpcb for the message */
	if (xphead->xp_type == NS_REQUEST) {
		/* allocate a new pxpcb for the request */
		if (xphead->xp_num_req == PXPQUEUELIMIT) {
			m_freem(m);
	    		return (E_MAXUNREPLIEDREQS);
		}
		MALLOC(xp, struct pxpcb *, sizeof(struct pxpcb), M_PCB, M_WAITOK); 
		bzero((caddr_t)xp, sizeof(struct pxpcb));
		mbstat.m_mtypes[MT_PCB]++;

		/* generate a new send message id and return it in opt */
		if (++pxp_sndseq <= 0)
			pxp_sndseq = 1;
		*((int *)opt) = pxp_sndseq;

		/* Initialize the new pxpcb, linking it right after the head */
		xphead->xp_num_req++;
		if (xphead->xp_next != PXPCB_NULL) 
			xphead->xp_next->xp_prev = xp;
		xp->xp_next		= xphead->xp_next;
		xphead->xp_next		= xp;
		xp->xp_prev		= xphead;
		xp->xp_type		= xphead->xp_type;
		xp->xp_inpcb		= inp;
		xp->xp_sin_port		= addr->sin_port;
		xp->xp_sin_faddr.s_addr = addr->sin_addr.s_addr;
		xp->xp_sin_laddr.s_addr = (u_long)0;	
		xp->xp_timer		= PXPINITTO;
        	xp->xp_rxtshift		= -1;
		xp->xp_retries		= PXPMAXRETRY;
		xp->xp_msg		= 0;
		xp->xp_snd_msgid	= pxp_sndseq;

	} else {
		/* this is a reply message;  search for the matching */
		/* request pxpcb using the receive msgid 	     */
		msgid = *((int *)opt);
		for (xp = xphead->xp_next; xp != PXPCB_NULL; xp = xp->xp_next) {
			if (xp->xp_rcv_msgid == msgid)
				break;
		}
		if (xp == PXPCB_NULL) {
			m_freem(m);
			return (E_SEQNUM);

		}
	}

	/* calculate data length and get a mbuf for PXP and IP headers */
	len = m_len(m);
	m0 = m_get(M_WAIT, MT_HEADER);

	/* fill in the mbuf with extended PXP header and */
	/* addresses and length put into network format  */
	m0->m_off = MMAXOFF - sizeof(struct pxpiphdr);
	m0->m_len = PXPIP_HDRSIZE;
	m0->m_next = m;
	pi = mtod (m0, struct pxpiphdr *);
	pi->pi_next = pi->pi_prev = 0;
	pi->pi_x1	= 0;
	pi->pi_pr	= IPPROTO_PXP;
	pi->pi_len	= htons((u_short)len + PXP_HDRSIZE); 
	pi->pi_src	= xp->xp_sin_laddr;
	pi->pi_dst	= xp->xp_sin_faddr;
	pi->pi_sport	= xp->xp_inpcb->inp_lport;
	pi->pi_dport	= xp->xp_sin_port;
	pi->pi_plen	= pi->pi_len;
	pi->pi_cksum	= 0;
	pi->pi_msgid	= xp->xp_snd_msgid;
	pi->pi_ver	= PXPVER;
	pi->pi_cls	= PXPCLS;
	pi->pi_rsved	= 0;
	pi->pi_ebit	= 0;

	/* add ip length and time-to-live and output the message */
	((struct ip *)pi)->ip_len = PXPIP_HDRSIZE + len;
	((struct ip *)pi)->ip_ttl = PXP_TTL;

	/* if this is a reply then set the reply bit and discard */
	/* the request pxpcb; otherwise, (it's a request) make   */
	/* a copy of the request data for retransmission	 */
	if (xp->xp_type == NS_REPLY) {
		pi->pi_rbit = 1;
		(void) pxp_discard(xp); 
	} else {
		pi -> pi_rbit = 0;
		xp->xp_msg = m_copy(m0, 0, (int)M_COPYALL);
		if (xp->xp_msg == 0) {
			m_freem(m0);
			(void) pxp_discard(xp);
			return (ENOBUFS);
		}
	}

	/* send the packet to ip */
	error = ip_output(m0, (struct mbuf *)0, (struct route *)0, 0);
	if ((error) && (xp->xp_type == NS_REQUEST))
		(void) pxp_discard(xp);

	return (error);

}  /* pxp_send */


/* pxp_usrreq() is called through the protocol */
/* switch.  Different actions are taken        */
/* depending on the request type.              */

pxp_usrreq (so, req, m, nam, opt)

struct socket	*so;	/* socket the usrreq pertains to */
int     	req;	/* specific request		 */
struct mbuf	*m;	/* optional mbuf data chain	 */
struct mbuf	*nam;	/* optional sockaddr mbuf 	 */
struct mbuf	*opt;	/* optional pointer to message id*/

{
	struct inpcb	*inp;		/* inpcb attached to the socket */
	struct pxpcb	*xp, *next_xp;	/* pxpcb pointers		*/     
	int     error = 0;		/* local result to be returned	*/
	int	msgid;			/* msgid extracted from opt	*/

	/* attach must be called before processing any other requests */
	inp = sotoinpcb(so);
	if (inp == NULL && req != PRU_ATTACH) {
		return (EINVAL);
	}

	switch (req) {

		case PRU_ATTACH: 
			if (so->so_ipc != SO_NETIPC)
				error = EOPNOTSUPP;	/* that's a no-no   */
			else if (inp != NULL)
				error = EISCONN;	/* already attached */
			else
				error = pxp_attach (so);
			break;

		case PRU_BIND: 
			error = in_pcbbind (inp, nam);
			break;

		case PRU_SEND: 
	    		error = pxp_send (so, m, nam, opt);
			break;

		case PRU_ABORT: 
			/* free all the request pxpcbs for this socket */
	    		xp = intopxpcb(inp);
	    		if ((xp) && (xp->xp_type == NS_REQUEST))
				while (xp != PXPCB_NULL) {
					next_xp= xp->xp_next;
					m_freem(xp->xp_msg);
					FREE(xp, M_PCB);
					mbstat.m_mtypes[MT_PCB]--;
					xp = next_xp;
				}
			else
				while (xp != PXPCB_NULL) {
					next_xp= xp->xp_next;
					FREE(xp, M_PCB);
					mbstat.m_mtypes[MT_PCB]--;
					xp = next_xp;
				}

			/* free the head pxpcb */
			FREE((struct pxpcb *)(inp->inp_ppcb), M_PCB);
			mbstat.m_mtypes[MT_PCB]--;

			/* release the receive sockbuf and detach the inpcb */
	    		sbrelease (&so->so_rcv);
	    		in_pcbdetach (inp);
	    		break;


		case PRU_REQABORT:

			/* find the pxpcb corresponding to the msgid */
			msgid = *((int *) opt);
			xp = intopxpcb (inp);
			if ((xp) && (xp->xp_type == NS_REQUEST)) {
				while (xp != PXPCB_NULL) {
					next_xp = xp->xp_next;
					if (xp->xp_snd_msgid == msgid)
						break;
					xp = next_xp;
				}
			} else {
				while (xp != PXPCB_NULL) {
					next_xp = xp->xp_next;
					if (xp->xp_rcv_msgid == msgid) 
						break;
					xp = next_xp;
				}
			}

			/* if we found it then discard it */
			if (xp != PXPCB_NULL) 
				(void) pxp_discard(xp);
			else 
				error = E_SEQNUM;
			break;


		case PRU_SENDOOB: 
	    		error = EOPNOTSUPP;
			m_freem(m);
			break;

		default: 
	    		error = EOPNOTSUPP;
	}

	return (error);

}  /* pxp_usrreq */

