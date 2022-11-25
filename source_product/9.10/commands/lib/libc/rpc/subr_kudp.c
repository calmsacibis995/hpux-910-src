/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/subr_kudp.c,v $
 * $Revision: 12.0 $	$Author: nfsmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/09/25 16:09:04 $
 */

/* BEGIN_IMS
******************************************************************************
****								
****   subr_kudp - udp send and receive routines for kernel level rpc
****								
******************************************************************************
* Description
*	The routines in this module are used to do kernel sends or
*	receives of an mbuf chain.
*
* Externally Callable Routines
*	ku_sendto	- send mbuf chain via udp to the specified address
*	ku_recvfrom	- pull next inbound udp message off specified socket
*	rpc_debug	- print debug information
*
* Test Module
*	$SCAFFOLD/nfs/*
*
* Notes
*	none
*
*
******************************************************************************
* END_IMS */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: subr_kudp.c,v 12.0 89/09/25 16:09:04 nfsmgr Exp $ (Hewlett-Packard)";
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
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../h/ns_diag.h"	/* needed for logging		     */
#include "../h/trigdef.h"	/* needed in order to use triggers   */
#include "../h/ns_ipc.h"	/* needed to use the NS_ASSERT macro */

struct mbuf     *mclgetx();
extern	struct macct	*nfs_macct;


/* BEGIN_IMS ku_recvfrom *
 ********************************************************************
 ****
 ****	struct mbuf *
 ****	ku_recvfrom(so, from)
 ****		register struct socket *so;
 ****		struct sockaddr_in *from;
 ****
 ********************************************************************
 * Input Parameters
 *	so	socket from which we want a message
 *
 * Output Parameters
 *	from	source address for this message
 *
 * Return Value
 *	address	of the requested message (mbuf chain)	
 *	null	if no message present
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine is called when a message has been detected on
 *	the specified socket. It will remove the mbuf chain which
 *	contains the message from the socket and pass it back to
 *	the caller.
 *
 * Algorithm
 *	Lock the socket (sblock)
 *	if there really isn't a message on the socket
 *		unlock the socket
 *		return NULL
 *	extract source address from the first mbuf
 *	if there are no more mbufs
 *		log and error
 *		unlock the socket
 *		return NULL
 *	for each mbuf in the chain
 *		free space on the socket for this mbuf (sbfree)
 *	remove mbuf from macct
 *	unlock the socket
 *	(attempt to align data - see notes)
 *	return pointer to mbuf chain
 *
 * Concurrency
 *
 *   This routine is called at splnet
 *
 * To Do List
 *	Handling of the socket pointers and maccts needs to be reworked.
 *	When this is done, we may remove extra checks in svc_run.
 *
 *	We also need to fix the pullup code. Currently the code
 *	only assures that the data in each mbuf begins on a word
 *	boundry. It does not assure that an xdr unit does not cross
 *	an mbuf boundry. We really want to assure that both of these
 *	conditions are met or else remove this code completely and
 *	be sure that all xdr routines correctly deal with misaligned
 *	data. We'll need to do more investigation to see which 
 *	option gives us the biggest payoff.
 *
 * Notes 
 *	none
 *				
 * Modification History
 *	10/22/87	ew	added comments
 *
 * External Calls
 *	sblock		lock the socket to guarentee exclusive access
 *	sbunlock	unlock the socket
 *	sbfree		decrement socket usage count
 *	MFREE		free an mbuf
 *	m_pullup	move data such that n bytes are contiguous
 *
 * Called By 
 *	clntkudp_callit	client side
 *	svckudp_recv	server side
 *
 ********************************************************************
 * END_IMS ku_recvfrom */

struct mbuf *
ku_recvfrom(so, from)
	register struct socket *so;
	struct sockaddr_in *from;
{
	register struct mbuf	*m;
	register struct mbuf	*m0;
	register struct mbuf	*m1;
	int		len = 0, error = 0;
	int             first;
	int             s;

#ifdef RPCDEBUG
	rpc_debug(4, "ku_recvfrom so=%X\n", so);
#endif
	/* Lock the socket and be sure there is really a message */
	sblock(&so->so_rcv, &error);
	if ( error ) {
		return((struct mbuf *)NULL);
	}
	/* 
	 * NB: Since multiple processes may be blocked by the previous
	 * sblock(), each one has to insure that their is still something
	 * there for it to work on.  In particular, we need to check that
	 * the first macct and mbufs are good.  dds  9/21/87
	 */
	if ( so->so_rcv.sb_macct == NULL ) {
		sbunlock(&so->so_rcv);
		return ((struct mbuf *)NULL);
	}
	/* change sb_mb to sb_macct.ma_mb */
	m = so->so_rcv.sb_macct->ma_mb;
	if (m == NULL) {
		sbunlock(&so->so_rcv);
		return (m);
	}

	/* The first mbuf should contain the address of the sender */
	/* Grab the remote socket address and free the mbuf	   */
	*from = *mtod(m, struct sockaddr_in *);
	sbfree(&so->so_rcv, m);
	MFREE(m, m0);  /* free up sockaddr */
#ifdef NTRIGGER
	/* This trigger will simulate a truncated message	*/
	if (utry_trigger(T_NFS_TRUNCATED, T_NFS_PROC, NULL, NULL)) {
		m = m0;
		for (;;) {
			sbfree(&so->so_rcv, m);
			if ((m->m_flags & MF_EOM) || m->m_next == NULL) {
				NS_ASSERT((m->m_flags & MF_EOM),
					  "ku_recvfrom: no MF_EOM");
				break;
			}
			m = m->m_next;
		}
		m_freem(m0);
		m0 = NULL;
	}
#endif NTRIGGER
	if (m0 == NULL) {
		/* NFS_LOG("cku_recvfrom: no body!\n");*/	
		NS_LOG_INFO(LE_NFS_NO_BODY, NS_LC_WARNING, NS_LS_NFS, 0,
		    1, (int)from->sin_addr.s_addr, 0); 

		/* If we get here, the macct has already been freed */
		/* from the socket, just return an error!	    */
		/* so->so_rcv.sb_macct->ma_mb = m0;		    */

		sbunlock(&so->so_rcv);
		return (m0);
	}

	/*
	 * Walk down mbuf chain till MF_EOM set (end of packet) or
	 * end of chain freeing socket buffer space as we go.
	 * With the current 300/800 network implementation we
	 * should always encounter the MF_EOM flag. If we don't
	 * there are serious problems with the lower network layers.
	 * After the loop m points to the last mbuf in the packet.
	 */
	m = m0;
	for (;;) {
		sbfree(&so->so_rcv, m);
		len += m->m_len;
		if ((m->m_flags & MF_EOM) || m->m_next == NULL) {
			NS_ASSERT((m->m_flags & MF_EOM),
				  "ku_recvfrom: no MF_EOM");
			break;
		}
		m = m->m_next;
	}
	if (TRIG2_OR(T_NFS_CHARGE, T_NFS_PROC, 0, 0) ma_charge(nfs_macct, m0)) {
		NS_LOG(LE_NFS_OVERDRAW, NS_LC_RESOURCELIM, NS_LS_NFS, 0);
		m_freem(m0);
		sbunlock(&so->so_rcv);
		return ((struct mbuf *)NULL);
	}
	m->m_next = NULL;
#ifdef NTRIGGER
	/* This trigger will simulate a message which is too long */       
	if (utry_trigger(T_NFS_TOO_LONG, T_NFS_PROC, NULL, NULL)) {
		len = UDPMSGSIZE + 1;
	}
#endif NTRIGGER
	if (len > UDPMSGSIZE) {
		/* NFS_LOG1("ku_recvfrom: len = %d\n", len);*/
		NS_LOG_INFO(LE_NFS_BAD_LEN, NS_LC_WARNING, NS_LS_NFS, 0,
			    2, len, (int)from->sin_addr.s_addr);
		m_freem(m0);
		m0 = NULL;
	}
#ifdef RPCDEBUG
	rpc_debug(4, "ku_recvfrom %d from %X\n", len, from->sin_addr.s_addr);
#endif

	sbunlock(&so->so_rcv);
	/* We will now attempt to align the data on word boundries */
	/* As implemented, this really only guarentees that the	   */
	/* first mbufs worth of data is aligned.                   */

	m = m1 = m0;
	first = TRUE;
	while(m) {
	    if (m->m_off%4) {
		if (m->m_len <= MLEN) m = m_pullup(m, m->m_len);
		else m = m_pullup(m, MLEN);
		if (first) {       
		   m0 = m;
		   first = FALSE;
                }
		else m1->m_next = m;
	    }
	    m1 = m;
            m = m->m_next;
	    first = FALSE;
        }

	return (m0);
}

int Sendtries = 0;
int Sendok = 0;



/* BEGIN_IMS ku_sendto *
 ********************************************************************
 ****
 ****	int
 ****	ku_sendto_mbuf(so, m, addr)
 ****		struct socket *so;
 ****		struct mbuf *m;
 ****		struct sockaddr_in *addr;
 ****
 ********************************************************************
 * Input Parameters
 *	so		socket to use for send
 *	m		mbuf chain to be sent
 *	addr		address of remote node
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	int		error returned by ku_fastsend or udp_output
 *
 * Globals Referenced
 *	Sendtries	number of calls to this routine
 *	Sendok		number of calls completed without error
 *
 * Description
 *	This routine attempt to send an mbuf chain via udp to the address
 *	specified.
 *
 * Algorithm
 *	if ku_fastsend suceeds
 *		return(0);
 *	else
 *		if message still intact
 *		       send via udp_output
 *		else
 *			retrun(error);
 *	fi
 *
 *
 * Concurrency
 *
 *
 * To Do List
 *	We would like to be able to remove the use of the fastpath
 *	code if we can tune the normal outbound path to achieve the
 *	same throughput.
 *
 *	We may also want to investigate the gains achieved by using
 *	the new routine in_pcbsetaddr as opposed to the existing
 *	routine in_pcbconnect.
 *
 * Notes
 *
 * Modification History
 *	10/29/87	ew	added comments
 *	10/29/87	ew	removed unused sugg_offset
 *
 * External Calls
 *	ku_fastsend	nfs "faster" output routine
 *	splnet		hold off network interupts
 *	splx		restore previous interupt level
 *	in_pcbsetaddr	set up address in the protocol control block
 *	m_freem		free an mbuf chain
 *	in_pcbdisconnect detach a protocol control block
 *
 * Called By
 *	clntkudp_callit	client side requests
 *	svckudp_send	server side replies
 *
 ********************************************************************
 * END_IMS ku_sendto_mbuf */

int
ku_sendto_mbuf(so, m, addr)
	struct socket *so;
	struct mbuf *m;
	struct sockaddr_in *addr;
{
	register struct inpcb *inp = sotoinpcb(so);
	int error;
	int s;
        struct in_addr laddr;

        m->m_acct = ((struct sockbuf *)(&so->so_snd))->sb_macct;
#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto_mbuf %X\n", addr->sin_addr.s_addr);
#endif
	Sendtries++;
	if ((error = ku_fastsend(so, m, addr)) == 0) {
		Sendok++;
		return (0);
	}

	/*
	 *  if ku_fastsend returns -2, then we can try to send m the
	 *  slow way.  else m was freed and we return ENOBUFS.
	 */
	if (error != -2) {
#ifdef RPCDEBUG
		rpc_debug(3, "ku_sendto_mbuf: fastsend failed\n");
#endif
		return (ENOBUFS);
	}
	s = splnet();
        laddr = inp->inp_laddr;
	if (TRIG2_OR(T_NFS_PCBSETADDR, T_NFS_PROC, NULL, NULL)
	    (error = in_pcbsetaddr(inp, addr))) {
		/* NFS_LOG1("pcbsetaddr failed %d\n", error); */
		NS_LOG_INFO(LE_NFS_PCBADDR, NS_LC_WARNING, NS_LS_NFS, 0,
			    2, error, addr->sin_addr.s_addr);
		(void) splx(s);
		m_freem(m);
		return (error);
	}
	error = udp_output(inp, m);
	in_pcbdisconnect(inp);
        inp->inp_laddr = laddr;
#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto returning %d\n", error);
#endif
	Sendok++;
	(void) splx(s);
	return (error);
}

#ifdef RPCDEBUG

/* BEGIN_IMS rpc_debug *
 ********************************************************************
 ****
 ****	rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
 ****		int level;
 ****		char *str;
 ****		int a1, a2, a3, a4, a5, a6, a7, a8, a9;
 ****
 ********************************************************************
 * Input Parameters
 *	level		debug level of this message
 *	str		string to be output
 *	a1-a9		variables to be included in the string
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	rpcdebug	debug level currently in effect
 *
 * Description
 *	This routine may be called to output debugging information
 *	to the system console.
 *
 * Algorithm
 *
 *   if severity of this message is less than current severity
 *	print the message
 *
 * To Do List
 *	
 * Notes
 *	This routine is only complied and called when RPC_DEBUG
 *	is defined.
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	printf
 *
 ********************************************************************
 * END_IMS rpc_debug */


int rpcdebug = 2;

/*VARARGS2*/
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{

        if (level <= rpcdebug)
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif
