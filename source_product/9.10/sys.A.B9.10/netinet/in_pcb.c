/*
 * $Header: in_pcb.c,v 1.26.83.7 93/10/27 18:08:13 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/in_pcb.c,v $
 * $Revision: 1.26.83.7 $		$Author: donnad $
 * $State: Exp $		$Locker: donnad $
 * $Date: 93/10/27 18:08:13 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) in_pcb.c $Revision: 1.26.83.7 $ $Date: 93/10/27 18:08:13 $";
#endif

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
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
 *	@(#)in_pcb.c	7.7 (Berkeley) 6/29/88 plus MULTICAST 1.0
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_var.h"
#include "../h/protosw.h"
#ifdef MULTICAST
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#endif MULTICAST

#undef insque
#undef remque

struct	in_addr zeroin_addr;

in_pcballoc(so, head)
	struct socket *so;
	struct inpcb *head;
{
	register struct inpcb *inp;

	MALLOC(inp, struct inpcb *, sizeof (struct inpcb), M_PCB, M_NOWAIT);
	if (inp == NULL)
		return (ENOBUFS);
	bzero((caddr_t)inp, sizeof (struct inpcb));
	mbstat.m_mtypes[MT_PCB]++;
	inp->inp_head = head;
	inp->inp_socket = so;
	insque(inp, head);
	so->so_pcb = (caddr_t)inp;
	return (0);
}

in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	register struct inpcb *head = inp->inp_head;
	register struct sockaddr_in *sin;
	u_short lport = 0;
	int	antiloop;	/* HP : flag to prevent infinite loop */
	int     wild;           /* HP INDaa14129 */

	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if (nam == 0)
		goto noname;
	sin = mtod(nam, struct sockaddr_in *);
	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_addr.s_addr != INADDR_ANY) {
		int tport = sin->sin_port;

		sin->sin_port = 0;		/* yech... */
		if (in_ifaddr == 0)
			return (EADDRNOTAVAIL);
		if (ifa_ifwithaddr((struct sockaddr *)sin) == 0)
			return (EADDRNOTAVAIL);
		sin->sin_port = tport;
	}
	lport = sin->sin_port;
	if (lport) {
		u_short aport = ntohs(lport);
		int wild = 0;

		/* GROSS */
		/*HP* if SS_NOUSER, "u." might not be real user context
		 *
		 * NB: we are tempted to simplify the 3rd test to just
		 * SS_PRIV, but we are careful to preserve the 4.3BSD
		 * semantics.  4.3BSD tested only u_uid, not SS_PRIV.
		 * thus, it was sufficient for user to be (or not to be)
		 * uid 0 at bind time, regardless of uid at open time
		 * (SS_PRIV).
		 */
		/* if not superuser, disallow reserve port allocation */
		if (aport < IPPORT_RESERVED && 
		    !(u.u_uid == 0 && !(so->so_state & SS_NOUSER)) &&
		    (so->so_state & (SS_NOUSER|SS_PRIV)) != (SS_NOUSER|SS_PRIV)) {
			return (EACCES);
		}

		/* even GROSSER, but this is the Internet */
		if ((so->so_options & SO_REUSEADDR) == 0 &&
		    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
		     (so->so_options & SO_ACCEPTCONN) == 0))
			wild = INPLOOKUP_WILDCARD;

		if (in_pcblookup(head,
		    zeroin_addr, 0, sin->sin_addr, lport, wild))
			return (EADDRINUSE);
	}
	inp->inp_laddr = sin->sin_addr;

noname:
	if (lport == 0) {
		/* HP : antiloop flag to prevent infinite loop 
			if all ports are in use 
		*/
		antiloop = 0;
		do {
			wild = 0;                                    /* HP INDaa14129 */
			if ((so->so_options & SO_REUSEADDR) == 0 &&
			   ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
			    (so->so_options & SO_ACCEPTCONN) == 0 ))
				wild = INPLOOKUP_WILDCARD;

			if (head->inp_lport++ < IPPORT_RESERVED ||
			    head->inp_lport > IPPORT_USERRESERVED) {
				if (antiloop == 0) {	
					head->inp_lport = IPPORT_RESERVED;
					antiloop ++;
				} else 
					return (EADDRNOTAVAIL);
			}
			lport = htons(head->inp_lport);
		} while (in_pcblookup(head,
			zeroin_addr, 0, inp->inp_laddr, 
			lport, (wild | INPLOOKUP_SETLOCAL) )); /* HP INDaa11614 */
	}
	inp->inp_lport = lport;
	return (0);
}

#ifdef	HPXTI

in_pcbunbind(inp)
	register struct inpcb *inp;
{
	inp->inp_laddr.s_addr = INADDR_ANY;
	inp->inp_lport = 0;
	return 0;
}

#endif

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
in_pcbconnect(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	struct in_ifaddr *ia;
	struct sockaddr_in *ifaddr;
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (in_ifaddr) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use the primary local address.
		 * If the supplied address is INADDR_BROADCAST,
		 * and the primary interface supports broadcast,
		 * choose the broadcast address for that interface.
		 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		if (sin->sin_addr.s_addr == INADDR_ANY)
		    sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
		else if (sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST &&
		  (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
		    sin->sin_addr = satosin(&in_ifaddr->ia_broadaddr)->sin_addr;
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;
		struct ifnet *ifp;

		ia = (struct in_ifaddr *)0;
		/* 
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr !=
			sin->sin_addr.s_addr || 
		    inp->inp_socket->so_options & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0 && /*XXX*/
		    (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
			/* No route yet, so try to acquire one */
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				sin->sin_addr;
			rtalloc(ro);
		}
		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 * unless it is the loopback (in case a route
		 * to our address on another net goes to loopback).
		 */
		if (ro->ro_rt && (ifp = ro->ro_rt->rt_ifp) &&
		    (ifp->if_flags & IFF_LOOPBACK) == 0)
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp)
					break;
		if (ia == 0) {
			int fport = sin->sin_port;

			sin->sin_port = 0;
			ia = (struct in_ifaddr *)
			    ifa_ifwithdstaddr((struct sockaddr *)sin);
			sin->sin_port = fport;
			if (ia == 0)
				ia = in_iaonnetof(in_netof(sin->sin_addr));
			if (ia == 0)
				ia = in_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}

		/*
		 * NIKE - call the protocol to determine if the protocol and
		 *	  route can support copy avoidance.
		 */
		if (ro->ro_rt) {
			struct socket *so;
			so = ro->ro_socket;
			if ((so->so_proto->pr_flags & PR_COPYAVOID) &&
			    ((*so->so_proto->pr_usrreq)(so, PRU_COPYAVOID, 0, 0, 0) == 0))
				so->so_snd.sb_flags |= SB_COPYAVOID_SUPP;
		}
				
#ifdef MULTICAST
		/*
		 * If the destination address is multicast and an outgoing
		 * interface has been set as a multicast option, use the
		 * address of that interface as our source address.
		 */
		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr)) &&
					inp->inp_moptions != NULL) {
			struct ip_moptions *imo;

			imo = mtod(inp->inp_moptions, struct ip_moptions *);
			if (imo->imo_multicast_ifp != NULL) {
				ifp = imo->imo_multicast_ifp;
				for (ia = in_ifaddr; ia; ia = ia->ia_next)
					if (ia->ia_ifp == ifp)
						break;
				if (ia == 0)
					return (EADDRNOTAVAIL);
			}
		}
#endif MULTICAST
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;
	}
	if (in_pcblookup(inp->inp_head,
		sin->sin_addr,
		sin->sin_port,
		inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
		inp->inp_lport,
		0))
			return (EADDRINUSE);

	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void)in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

/*
 * NFS
 *
 * in_pcbsetaddr -- set the pcb addr to connect to.
 * Same thing as in_pcbconnect, but with a different
 * parameter.  Currently called only by
 * ku_sendto_mbuf() in the NFS/RPC code.
 * 
 * NFS
 */

in_pcbsetaddr(inp, sin)
	struct inpcb *inp;
	register struct sockaddr_in *sin;
{
	struct ifnet *ifp, *if_ifwithafup();
	struct sockaddr_in *ifaddr;
	struct ifaddr *ifap;
	struct in_ifaddr *ia;
	extern struct in_ifaddr *ifa_ifwithafup();

	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_addr.s_addr == INADDR_ANY || sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;
		struct ifnet *ifp;
		ia = (struct in_ifaddr *)0;
		/*
		 * Try to set the route in the in_pcb, if we succeed we take
		 * the address from the interface.  NOTE: we assume there
		 * is no current route set in the in_pcb (this should be
		 * the case with NFS), but check it anyway. 
		 */
                ro = &inp->inp_route;
		if (ro->ro_rt != (struct rtentry *)0)
			RTFREE(ro->ro_rt);
                ro->ro_dst.sa_family = AF_INET;
		((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
			sin->sin_addr;
                rtalloc(ro);

		/*
                 * If we found a route, use the address corresponding to the
		 * outgoing interface  unless it is the loopback (in case a 
		 * route to our address on another net goes to loopback).
		 */
                if (ro->ro_rt && (ifp = ro->ro_rt->rt_ifp) &&
		    (ifp->if_flags & IFF_LOOPBACK) == 0)
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp)
					break;
                if (ia == 0) {
			int fport = sin->sin_port;
			sin->sin_port = 0;
			ia = (struct in_ifaddr *)
			    ifa_ifwithdstaddr((struct sockaddr *)sin);
			sin->sin_port = fport;
			if (ia == 0)
				ia = in_iaonnetof(in_netof(sin->sin_addr));
			if (ia == 0)
				ia = in_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
                }
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;
	}
	/* HP INDaa09596: do not call in_pcblookup2() here since
	 * a INADDR_ANY local host address is not currently passed in
	 * in practice.
	 */
#ifdef NET_QA
	if (inp->inp_laddr == 0 && (ifaddr->sin_addr == 0))
		panic("in_pcbsetaddr");
#endif
	if (in_pcblookup(inp->inp_head, sin->sin_addr, sin->sin_port,
	    inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport, 0))
		return (EADDRINUSE);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void) in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

in_pcbdisconnect(inp)
	struct inpcb *inp;
{

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;

	/*
	 * NIKE - clear out the route and the copy avoidance flags so we
	 *	  start fresh on the next call to connect (udp sockets).
	 */
	if (inp->inp_route.ro_rt) {
		RTFREE(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		inp->inp_route.ro_flags = 0;
	}
	inp->inp_socket->so_snd.sb_flags &= ~SB_COPYAVOID_SUPP;

	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;

	so->so_pcb = 0;
	sofree(so);
	if (inp->inp_options)
		(void)m_free(inp->inp_options);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
#ifdef MULTICAST
	ip_freemoptions(inp->inp_moptions);
#endif MULTICAST
	remque(inp);
	FREE(inp, M_PCB);
	mbstat.m_mtypes[MT_PCB]--;
}

in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

in_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  Call the protocol specific
 * routine (if any) to handle each connection.
 */
in_pcbnotify(head, sin, errno, notify)
	struct inpcb *head;
	register struct sockaddr_in *sin;
	int errno, (*notify)();
{
	register struct inpcb *inp, *oinp;
	int s = splimp();

	for (inp = head->inp_next; inp != head;) {
		if (inp->inp_faddr.s_addr != sin->sin_addr.s_addr ||
		    inp->inp_socket == 0) {
			inp = inp->inp_next;
			continue;
		}
		if (errno) {
			/* check for a local port.  If zero then this
			   error applies to all ports for the dest ip addr.
			   Otherwise the error applies only to the port that
			   sent the packet. */
                        if (sin->sin_port == 0) {
			         inp->inp_socket->so_error = errno;
#ifdef _WSIO      
			         sohaspollexception(inp->inp_socket);
#endif
		        }
		        else
				 if (sin->sin_port == inp->inp_lport) {
					 inp->inp_socket->so_error = errno;
#ifdef _WSIO      
			                 sohaspollexception(inp->inp_socket);
#endif
                                 }
		}

		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify)(oinp);
	}
	splx(s);
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
in_losing(inp)
	struct inpcb *inp;
{
	register struct rtentry *rt;

	if ((rt = inp->inp_route.ro_rt)) {
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest((int)SIOCDELRT, rt);
		rtfree(rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
in_rtchange(inp)
	register struct inpcb *inp;
{
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
}

struct inpcb *
in_pcblookup(head, faddr, fport, laddr, lport, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_short fport, lport;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_lport != lport)
			continue;

		if (flags & INPLOOKUP_SETLOCAL) { /* HP : INDaa11614 */
			if ((laddr.s_addr == INADDR_ANY)
				|| (inp->inp_laddr.s_addr == INADDR_ANY))
			return(inp);
		}
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
#ifndef MULTICAST
			if (laddr.s_addr != INADDR_ANY)
#endif MULTICAST
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	return (match);
}
