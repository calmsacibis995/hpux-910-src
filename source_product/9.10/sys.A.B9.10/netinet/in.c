/*
 * $Header: in.c,v 1.20.83.4 93/09/17 19:02:36 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/in.c,v $
 * $Revision: 1.20.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:02:36 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) in.c $Revision: 1.20.83.4 $ $Date: 93/09/17 19:02:36 $";
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
 *	@(#)in.c	7.9 (Berkeley) 6/29/88 plus MULTICAST 1.2
 */

#include "../h/param.h"
#include "../h/ioctl.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../netinet/in_systm.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/af.h"
#include "../netinet/in.h"
#include "../net/cko.h"
#include "../netinet/in_pcb.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/in_var.h"
#include "../h/mib.h"
#include "../netinet/mib_kern.h"

inet_hash(sin, hp)
	register struct sockaddr_in *sin;
	struct afhash *hp;
{
	register u_long n;

	n = in_netof(sin->sin_addr);
	if (n)
	    while ((n & 0xff) == 0)
		n >>= 8;
	hp->afh_nethash = n;
	hp->afh_hosthash = ntohl(sin->sin_addr.s_addr);
}

inet_netmatch(sin1, sin2)
	struct sockaddr_in *sin1, *sin2;
{

	return (in_netof(sin1->sin_addr) == in_netof(sin2->sin_addr));
}

/*
 * Formulate an Internet address from network + host.
 */
struct in_addr
in_makeaddr(net, host)
	u_long net, host;
{
	register struct in_ifaddr *ia;
	register u_long mask;
	u_long addr;

	if (IN_CLASSA(net))
		mask = IN_CLASSA_HOST;
	else if (IN_CLASSB(net))
		mask = IN_CLASSB_HOST;
	else
		mask = IN_CLASSC_HOST;
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if ((ia->ia_netmask & net) == ia->ia_net) {
			mask = ~ia->ia_subnetmask;
			break;
		}
	addr = htonl(net | (host & mask));
	return (*(struct in_addr *)&addr);
}

/*
 * Return the network number from an internet address.
 */
u_long
in_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;
	register struct in_ifaddr *ia;

	if (IN_CLASSA(i))
		net = i & IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		net = i & IN_CLASSB_NET;
	else if (IN_CLASSC(i))
		net = i & IN_CLASSC_NET;
#ifdef MULTICAST
	else if (IN_CLASSD(i))
		net = i & IN_CLASSD_NET;
#endif MULTICAST
	else
		return (0);

	/*
	 * Check whether network is a subnet;
	 * if so, return subnet number.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (net == ia->ia_net)
			return (i & ia->ia_subnetmask);
	return (net);
}

/*
 * Return the host portion of an internet address.
 */
u_long
in_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net, host;
	register struct in_ifaddr *ia;

	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = i & IN_CLASSB_NET;
		host = i & IN_CLASSB_HOST;
	} else if (IN_CLASSC(i)) {
		net = i & IN_CLASSC_NET;
		host = i & IN_CLASSC_HOST;
#ifdef MULTICAST
	} else if (IN_CLASSD(i)) {
		net = i & IN_CLASSD_NET;
		host = i & IN_CLASSD_HOST;
#endif MULTICAST
	} else
		return (i);

	/*
	 * Check whether network is a subnet;
	 * if so, use the modified interpretation of `host'.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (net == ia->ia_net)
			return (host &~ ia->ia_subnetmask);
	return (host);
}

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif
int subnetsarelocal = SUBNETSARELOCAL;
/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
in_localaddr(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register struct in_ifaddr *ia;

	if (subnetsarelocal) {
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if ((i & ia->ia_netmask) == ia->ia_net)
				return (1);
	} else {
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if ((i & ia->ia_subnetmask) == ia->ia_subnet)
				return (1);
	}
	return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
in_canforward(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;

	if (IN_EXPERIMENTAL(i))
		return (0);
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == IN_LOOPBACKNET)
			return (0);
	}
	return (1);
}

int	in_interfaces;		/* number of external internet interfaces */
extern	struct ifnet loif;

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
in_control(so, cmd, data, ifp)
	struct socket *so;
	int cmd;
	caddr_t data;
	register struct ifnet *ifp;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct in_ifaddr *ia = 0, *nia;
	struct sockaddr oldaddr;
	struct sockaddr_in netaddr;
	struct ifaddr *ifa;
	struct mbuf *m;
	int error;

	/*
	 * Find address for this interface, if it exists.
	 */
	if (ifp)
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;

	switch (cmd) {

	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFDSTADDR:
		if (!suser())
			return (u.u_error);

		if (ifp == 0)
			panic("in_control");
		if (ia == (struct in_ifaddr *)0) {
			MALLOC(nia, struct in_ifaddr *,
				sizeof (struct in_ifaddr), M_IFADDR, M_WAITOK);
			if (nia == 0)
				return (ENOBUFS);
			bzero((caddr_t)nia, sizeof (struct in_ifaddr));
			mbstat.m_mtypes[MT_IFADDR]++;
			if (ia = in_ifaddr) {
				for ( ; ia->ia_next; ia = ia->ia_next)
					;
				ia->ia_next = nia;
			} else
				in_ifaddr = nia;
			ia = nia;
			if (ifa = ifp->if_addrlist) {
				for ( ; ifa->ifa_next; ifa = ifa->ifa_next)
					;
				ifa->ifa_next = (struct ifaddr *) ia;
			} else
				ifp->if_addrlist = (struct ifaddr *) ia;
			ia->ia_ifp = ifp;
			IA_SIN(ia)->sin_family = AF_INET;
			if (ifp != &loif)
				in_interfaces++;
		}
		break;

	case SIOCSIFBRDADDR:
		if (!suser())
			return (u.u_error);
		/* FALLTHROUGH */

	default:
		if (ia == (struct in_ifaddr *)0)
			return (EADDRNOTAVAIL);
		break;
	}

	switch (cmd) {

	case SIOCGIFADDR:
		ifr->ifr_addr = ia->ia_addr;
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		ifr->ifr_dstaddr = ia->ia_broadaddr;
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		ifr->ifr_dstaddr = ia->ia_dstaddr;
		break;

	case SIOCGIFNETMASK:
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		satosin(&ifr->ifr_addr)->sin_family = AF_INET;
		satosin(&ifr->ifr_addr)->sin_addr.s_addr = htonl(ia->ia_subnetmask);
		break;

	case SIOCSIFDSTADDR:
	    {
		struct sockaddr oldaddr;

		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = ifr->ifr_dstaddr;
		if (error = (*ifp->if_ioctl)(ifp, SIOCSIFDSTADDR, ia)) {
			ia->ia_dstaddr = oldaddr;
			return (error);
		}
		if (ia->ia_flags & IFA_ROUTE) {
			rtinit(&oldaddr, &ia->ia_addr, (int)SIOCDELRT,
			    RTF_HOST, NMLOCAL);
			rtinit(&ia->ia_dstaddr, &ia->ia_addr, (int)SIOCADDRT,
			    RTF_HOST|RTF_UP, NMLOCAL);
		}
	    }
		break;

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		if (error = (*ifp->if_ioctl)(ifp, cmd, data))
			return(error);
		ia->ia_broadaddr = ifr->ifr_broadaddr;
		break;

	case SIOCSIFADDR:
		return (in_ifinit(ifp, ia,
		    (struct sockaddr_in *) &ifr->ifr_addr));

	case SIOCSIFNETMASK:
		if (error = (*ifp->if_ioctl)(ifp, cmd, data))
			return(error);

		/*
	 	 * Delete any previous route for an old address before
		 * setting the subnetmask.
	 	 *
	 	 * Fix DTS INDaa006371 : Move this code from in_ifinit() 
		 * into here.  Otherwise, if subnetmask is set first, 
		 * in_ifinit() will pick up the new mask while attempting 
		 * to delete the old route. 
	 	 */
		bzero((caddr_t)&netaddr, sizeof (netaddr));
		netaddr.sin_family = AF_INET;
		oldaddr = ia->ia_addr;
		if (ia->ia_flags & IFA_ROUTE) {
			if (ifp->if_flags & IFF_LOOPBACK)
				rtinit(&oldaddr, &oldaddr, (int)SIOCDELRT, RTF_HOST, NMLOCAL);
			else if (ifp->if_flags & IFF_POINTOPOINT) {
				rtinit(&ia->ia_dstaddr, &oldaddr, 
					(int)SIOCDELRT, RTF_HOST, NMLOCAL);
				/*
				 * If we added a net route in in_ifinit, we
				 * should delete the route here.
				 */
				netaddr.sin_addr = in_makeaddr(ia->ia_subnet,
					INADDR_ANY);
				rtinit((struct sockaddr *)&netaddr, &oldaddr,
					(int)SIOCDELRT, 0, NMLOCAL);
			} else {
				netaddr.sin_addr = in_makeaddr(ia->ia_subnet,
			    		INADDR_ANY);
				rtinit((struct sockaddr *)&netaddr, &oldaddr, 
			    		(int)SIOCDELRT, 0, NMLOCAL);
			}
			ia->ia_flags &= ~IFA_ROUTE;
		}

		ia->ia_subnetmask = ntohl(satosin(&ifr->ifr_addr)->sin_addr.s_addr);
		break;

	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
	return (0);
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
in_ifinit(ifp, ia, sin)
	register struct ifnet *ifp;
	register struct in_ifaddr *ia;
	struct sockaddr_in *sin;
{
	register u_long i = ntohl(sin->sin_addr.s_addr);
	struct sockaddr oldaddr;
	struct sockaddr_in netaddr;
	int s = splimp(), error;

	oldaddr = ia->ia_addr;
	ia->ia_addr = *(struct sockaddr *)sin;

	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, ia)) {
		splx(s);
		ia->ia_addr = oldaddr;
		return (error);
	}

	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	ia->ia_net = i & ia->ia_netmask;
	/*
	 * The subnet mask includes at least the standard network part,
	 * but may already have been set to a larger value.
	 */
	ia->ia_subnetmask |= ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	if (ifp->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sa_family = AF_INET;
		((struct sockaddr_in *)(&ia->ia_broadaddr))->sin_addr =
			in_makeaddr(ia->ia_subnet, INADDR_BROADCAST);
		ia->ia_netbroadcast.s_addr =
		    htonl(ia->ia_net | (INADDR_BROADCAST &~ ia->ia_netmask));
	}
	/*
	 * Add route for the network.
	 */
	bzero((caddr_t)&netaddr, sizeof (netaddr));
	netaddr.sin_family = AF_INET;
	if (ifp->if_flags & IFF_LOOPBACK)
		rtinit(&ia->ia_addr, &ia->ia_addr, (int)SIOCADDRT,
		    RTF_HOST|RTF_UP, NMLOCAL);
	else if (ifp->if_flags & IFF_POINTOPOINT) {
		/*
		 * We should add a net entry to the POINTOPOINT link
		 * so that loopback over that address will work.
		 * Subbu 5/18/90.
		 */
		rtinit(&ia->ia_dstaddr, &ia->ia_addr, (int)SIOCADDRT,
		    RTF_HOST|RTF_UP, NMLOCAL);
		netaddr.sin_addr = in_makeaddr(ia->ia_subnet, INADDR_ANY);
		rtinit((struct sockaddr *)&netaddr, &ia->ia_addr,
		    (int)SIOCADDRT, RTF_UP, NMLOCAL);
	} else {
		netaddr.sin_addr = in_makeaddr(ia->ia_subnet, INADDR_ANY);
		rtinit((struct sockaddr *)&netaddr, &ia->ia_addr,
		    (int)SIOCADDRT, RTF_UP, NMLOCAL);
	}
	ia->ia_flags |= IFA_ROUTE;
#ifdef MULTICAST
	/*
	 * If the interface supports multicast, join the "all hosts"
	 * multicast group on that interface.
	 */
	if (ifp->if_flags & IFF_MULTICAST) {
		struct in_addr addr;

		addr.s_addr = htonl(INADDR_ALLHOSTS_GROUP);
		in_addmulti(addr, ifp);
	}
#endif MULTICAST
	splx(s);
	return (0);
}

/*
 * Return address info for specified internet network.
 */
struct in_ifaddr *
in_iaonnetof(net)
	u_long net;
{
	register struct in_ifaddr *ia;

	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_subnet == net)
			return (ia);
	return ((struct in_ifaddr *)0);
}

#define  IN_MASK(i) \
((IN_CLASSC(i)) ? IN_CLASSC_NET \
    : ((IN_CLASSB(i)) ? IN_CLASSB_NET \
          : IN_CLASSA_NET))
/*
 * Return 1 if the address might be a local broadcast address.
 */
in_broadcast(in)
	struct in_addr in;
{
	register struct in_ifaddr *ia;
	u_long netmask, t = ntohl(in.s_addr);

  /*
   *  If the non-network part is all ones, this is a broadcast.
   */
  netmask = IN_MASK(t);
  if ( (t | netmask) == 0xffffffff )
    return(1);

	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
	    if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
		if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr == t)
		     return (1);
		/*
		 * Check for old-style (host 0) broadcast.
		 */
		if (t == ia->ia_subnet || t == ia->ia_net)
		    return (1);
	}

	if (t == INADDR_BROADCAST || t == INADDR_ANY)
		return (1);
	return (0);
}

#ifdef MULTICAST
/*
 * Add an address to the list of IP multicast addresses for a given interface.
 */
struct in_multi *
in_addmulti(addr, ifp)
	register struct in_addr addr;
	register struct ifnet *ifp;
{
	register struct in_multi *inm;
	struct ifreq ifr;
	struct in_ifaddr *ia;
	struct mbuf *m;
	int s;

	NET_SPLNET(s);

	/*
	 * See if address already in list.
	 */
	IN_LOOKUP_MULTI(addr, ifp, inm);
	if (inm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		++inm->inm_refcount;
	}
	else {
		/*
		 * New address; get an mbuf for a new multicast record
		 * and link it into the interface's multicast list.
		 */
		if ((m = m_getclr(M_DONTWAIT, MT_IPMADDR)) == NULL) {
			NET_SPLX(s);
			return(NULL);
		}
		inm = mtod(m, struct in_multi *);
		inm->inm_addr = addr;
		inm->inm_ifp = ifp;
		inm->inm_refcount = 1;
		IFP_TO_IA(ifp, ia);
		if (ia == NULL) {
			m_free(m);
			NET_SPLX(s);
			return(NULL);
		}
		inm->inm_ia = ia;
		inm->inm_next = ia->ia_multiaddrs;
		ia->ia_multiaddrs = inm;
		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr = addr;
		if (ifp->if_ioctl == NULL ||
		    (*ifp->if_ioctl)(ifp, SIOCADDMULTI,(caddr_t)&ifr) !=  0) {
			m_free(m);
			NET_SPLX(s);
			return(NULL);
		}
		/*
		 * Let IGMP know that we have joined a new IP multicast group.
		 */
		igmp_joingroup(inm);
	}
	NET_SPLX(s);
	return(inm);
}

/*
 * Delete a multicast address record.
 */
in_delmulti(inm)
	register struct in_multi *inm;
{
	register struct in_multi **p;
	struct ifreq ifr;
	int s;

	NET_SPLNET(s);

	if (--inm->inm_refcount == 0) {
		/*
		 * No remaining claims to this record; let IGMP know that
		 * we are leaving the multicast group.
		 */
		igmp_leavegroup(inm);
		/*
		 * Unlink from list.
		 */
		for (p = &inm->inm_ia->ia_multiaddrs;
		     *p != inm;
		     p = &((*p)->inm_next));
		*p = (*p)->inm_next;
		/*
		 * Notify the network driver to update its multicast reception
		 * filter.
		 */
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr =
								inm->inm_addr;
		(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI,
							     (caddr_t)&ifr);
		m_free(dtom(inm));
	}
	NET_SPLX(s);
}
#endif MULTICAST

copy_fixup(m,sb)
	struct mbuf *m;
	struct sockbuf *sb;
{
	return(EOPNOTSUPP);
}

/*
 * HP : Checksum Offload
 *
 * cko_cksum : called by tcp_output, udp_output in place of in_cksum.
 *
 *	If interface supports checksum offload,
 *		Set up checksum assist info in the mquad area
 *		Seed the protocol header with pseudo header checksum
 *	else
 *		call in_cksum.
 *
 *	Assumption : Standard IP header without IP options
 *		If there are IP options, ip_insertoptions will adjust mquad
 */
cko_cksum(m, len, inp, insert, cntl)
	struct mbuf  *m;	/* start at ip hdr */
	int	len;		/* ip hdr + tcp/udp hdr + data */
	struct inpcb *inp;	
	int	insert;		/* Checksum insert offset */
	int	cntl;		/* Checksum assist control flags */
{
	struct ifnet	*ifp;
	struct rtentry	*rt;
	struct ipovly	*ih;
	u_short		sum;

	rt = inp->inp_route.ro_rt;
	if((rt && (ifp = rt->rt_ifp) && (ifp->if_flags&IFF_CKO)) ||
	   ((rt == 0) && (inp->inp_socket->so_snd.sb_flags & SB_COPYAVOID_SUPP))) {
		ih = mtod(m, struct ipovly *);
		sum  = in_3sum(ih->ih_src.s_addr, ih->ih_dst.s_addr,
					  (ih->ih_pr<<16) + ih->ih_len);
	    	CKO_SET((struct cko_info *)&(m->m_quad[MQ_CKO_OUT0]), 
			sizeof(struct ipovly), len-1, insert, cntl);
		m->m_flags |= MF_CKO_OUT;
	} else {
		sum  = in_cksum(m,len);
	}
	(*(u_short *) (mtod(m,int) + insert)) = sum;
	return(0);
}

/* 
 * HP : Checksum Offload.  It is possible that we freed a route
 *	and allocate a new route.  If checksum assist requested 
 *	and the new interface does not support checksum offload, 
 *	call in_cksum()
 *
 *	By the time we reach here, we are assured that there
 *	is a route and interface.
 */
cko_fixup(m)
	struct mbuf *m;
{
	int	offset, cklen;
	int 	hlen;
	u_short algo;
	u_short sum;

	hlen = ((struct cko_info*) &(m->m_quad[MQ_CKO_OUT0]))->cko_start;
	cklen  = ((struct cko_info*) &(m->m_quad[MQ_CKO_OUT0]))->cko_stop +1 
			- hlen;
	/* 
	 * For in_cksum, make m_offset point to TCP/UDP header 
	 * then restore it to point to IP header.
	 */
	m->m_off += hlen;
	m->m_len -= hlen;
	sum    = in_cksum(m, cklen);
	m->m_off -= hlen;
	m->m_len += hlen;

	algo   = ((struct cko_info*) &(m->m_quad[MQ_CKO_OUT0]))->cko_type;
	offset = ((struct cko_info*) &(m->m_quad[MQ_CKO_OUT0]))->cko_insert;
	if ((algo & CKO_ALGO_UDP) && (sum ==0)) {
		(*(u_short *) (mtod(m,int) +offset)) = 0xffff;
	} else {
		(*(u_short *) (mtod(m,int) +offset)) = sum;
	}
	m->m_flags &= ~MF_CKO_OUT;
	return(0);
}
