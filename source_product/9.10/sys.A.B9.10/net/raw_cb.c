/*
 * $Header: raw_cb.c,v 1.22.83.5 93/12/09 10:35:38 marshall Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/raw_cb.c,v $
 * $Revision: 1.22.83.5 $		$Author: marshall $
 * $State: Exp $		$Locker:  $
 * $Date: 93/12/09 10:35:38 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) raw_cb.c $Revision: 1.22.83.5 $";
#endif

/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)raw_cb.c	7.6 (Berkeley) 6/27/88 plus MULTICAST 1.1
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../netinet/in.h"

#undef remque
#undef insque

/*
 * Routines to manage the raw protocol control blocks. 
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

/*
 * Allocate a control block and a nominal amount
 * of buffer space for the socket.
 */
raw_attach(so, proto)
	register struct socket *so;
	int proto;
{
	register struct rawcb *rp;

	MALLOC(rp, struct rawcb *, sizeof (struct rawcb), M_PCB, M_NOWAIT);
	if (rp == 0)
		return (ENOBUFS);
 	mbstat.m_mtypes[MT_PCB]++;
 	if (sbreserve(&so->so_snd, (u_long) RAWSNDQ) == 0)
		goto bad;
 	if (sbreserve(&so->so_rcv, (u_long) RAWRCVQ) == 0)
		goto bad2;
	bzero((caddr_t)rp, sizeof (struct rawcb));
	rp->rcb_socket = so;
	so->so_pcb = (caddr_t)rp;
	rp->rcb_proto.sp_family = so->so_proto->pr_domain->dom_family;
	rp->rcb_proto.sp_protocol = proto;
	switch (rp->rcb_proto.sp_family) {
	case AF_INET: 			/* should be generic protocol call!! */
		rp->rcb_pcb = (caddr_t) rip_newripcb();	
		if (rp->rcb_pcb == NULL) {
			FREE(rp, M_PCB);
			return(ENOBUFS);
		}
		break;
	default:
		rp->rcb_pcb = (caddr_t) 0;
		break;
	}
	insque(rp, &rawcb);
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	FREE(rp, M_PCB);
	mbstat.m_mtypes[MT_PCB]--;
	return (ENOBUFS);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */
raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket *so = rp->rcb_socket;

	if (rp->rcb_route.ro_rt)
		rtfree(rp->rcb_route.ro_rt);
	so->so_pcb = 0;
	sofree(so);
	remque(rp);
	if (rp->rcb_options)
		m_freem(rp->rcb_options);
#ifdef MULTICAST
	{
	extern struct socket *ip_mrouter;
	if (so == ip_mrouter)
		ip_mrouter_done();
	if (rp->rcb_proto.sp_family == AF_INET)
		ip_freemoptions(rp->rcb_moptions);
	}
#endif MULTICAST
	if (rp->rcb_pcb)
		FREE(rp->rcb_pcb, M_PCB);
	FREE(rp, M_PCB);
	mbstat.m_mtypes[MT_PCB]--;
}

/*
 * Disconnect and possibly release resources.
 */
raw_disconnect(rp)
	struct rawcb *rp;
{

	rp->rcb_flags &= ~RAW_FADDR;
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);
}

raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

	if (ifnet == 0)
		return (EADDRNOTAVAIL);
/* BEGIN DUBIOUS */
	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */
	switch (addr->sa_family) {

	case AF_IMPLINK:
	case AF_INET: {
		if (((struct sockaddr_in *)addr)->sin_addr.s_addr &&
		    ifa_ifwithaddr(addr) == 0)
			return (EADDRNOTAVAIL);
		break;
	}

	default:
		return (EAFNOSUPPORT);
	}
/* END DUBIOUS */
	rp = sotorawcb(so);
	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_laddr, sizeof (*addr));
	rp->rcb_flags |= RAW_LADDR;
	return (0);
}

/*
 * Associate a peer's address with a
 * raw connection block.
 */
raw_connaddr(rp, nam)
	struct rawcb *rp;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_faddr, sizeof(*addr));
	rp->rcb_flags |= RAW_FADDR;
}
