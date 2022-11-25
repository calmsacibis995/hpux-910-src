/*
 * $Header: if.c,v 1.26.83.5 93/10/20 11:23:47 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/if.c,v $
 * $Revision: 1.26.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/20 11:23:47 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) if.c $Revision: 1.26.83.5 $";
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
 *	@(#)if.c	7.4 (Berkeley) 6/27/88 plus MULTICAST 1.1
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/netfunc.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../h/mib.h"
#include "../netinet/mib_kern.h"

struct	ifnet *ifnet;
int	ifqmaxlen = IFQ_MAXLEN;
int	MIB_ifNumber=0;		/* # of interfaces */

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */

ifinit()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_init)
			(*ifp->if_init)(ifp);
		if (ifp->if_snd.ifq_maxlen == 0)
			ifp->if_snd.ifq_maxlen = ifqmaxlen;
	}
	/*	register network management get/set routines	*/
	NMREG (GP_if, nmget_if, nmset_if,  mib_unsupp,  mib_unsupp);
	if_slowtimo();
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
if_attach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p = &ifnet;

	ifp->if_index = ++ MIB_ifNumber;
	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifa->ifa_addr, addr))
			return (ifa);
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifa->ifa_broadaddr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}
/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) 
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr.sa_family != addr->sa_family)
				continue;
			if (equal(&ifa->ifa_dstaddr, addr))
				return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is first found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register u_int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= AF_MAX)
		return (0);
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if ((*netmatch)(&ifa->ifa_addr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}

#ifdef notdef
/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family == af)
			return (ifa);
	return ((struct ifaddr *)0);
}
#endif

/*
 * Find an UP interface using a specific address family -- NFS
 */
struct ifnet *
ifa_ifwithafup(af)
      register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if ((ifp->if_flags & IFF_UP) && (ifa->ifa_addr.sa_family == af))
			return ((struct ifnet *)ifa);
	return ((struct ifnet *) 0);
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splnet or eqivalent.
 */
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		pfctlinput(PRC_IFDOWN, &ifa->ifa_addr);
	if_qflush(&ifp->if_snd);
}

/*
 * Flush an interface queue.
 */
if_qflush(ifq)
	register struct ifqueue *ifq;
{
	register struct mbuf *m, *n;

	n = ifq->ifq_head;
	while (m = n) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

#ifdef NEVER_CALLED
/*
 * Resize an interface queue.  Increase ifq_maxlen to size iff ifq_maxlen
 * is currently less than size.
 */
ifq_resize(ifq,size)
      struct ifqueue *ifq;
      int     size;
{
      if (ifq->ifq_maxlen < size)
              ifq->ifq_maxlen = size;
}
#endif /* NEVER_CALLED */

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
if_slowtimo()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp);
	}
	net_timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name)
	register char *name;
{
	register char c, *cp, *dp;
	register struct ifnet *ifp;
	int unit;

	for (cp = name; cp < name + IFNAMSIZ && *cp; cp++) /* scan to end */
		;
	if (cp == name + IFNAMSIZ)
		return ((struct ifnet *)0);
	for (cp--; cp > name; cp--)		/* find first non-numeric */
		if (*cp < '0' || *cp > '9')	
			break;
	unit = 0;
	for (dp = ++cp; c = *dp; dp++)
		unit = unit * 10 + c - '0';
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (bcmp(ifp->if_name, name, (unsigned)(cp - name)))
			continue;
		if (unit == ifp->if_unit)
			break;
	}
	return (ifp);
}

/*
 * Interface ioctls.
 */
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;
	int error;

	switch (cmd) {

	case SIOCGIFCONF:
		return (ifconf(cmd, data));
	case SIOCSARP:
	case SIOCDARP:
	case SIOCTARP:
		if (!suser())
			return (u.u_error);
		/* FALL THROUGH */
	case SIOCGARP:
		return (NETCALL(NET_ARPIOCTL)(cmd, data));
	}
	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0)
		return (ENXIO);
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCSIFFLAGS:
		if (!suser())
			return (u.u_error);
		if (error = (*ifp->if_ioctl)(ifp, cmd, data))
			return(error);
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			int s = splimp();
			if (ifp->if_control)
			   (*ifp->if_control) (ifp, IFC_IFNMEVENT, NMV_LINKDOWN);
			if_down(ifp);
			splx(s);
		}
		if (!(ifp->if_flags & IFF_UP) && ifr->ifr_flags & IFF_UP)
			if (ifp->if_control)
			   (*ifp->if_control) (ifp, IFC_IFNMEVENT, NMV_LINKUP);
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		break;

	case SIOCSIFMETRIC:
		if (!suser())
			return (u.u_error);
		if (error = (*ifp->if_ioctl)(ifp, cmd, data))
			return(error);
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCSACFLAGS:
		if (!suser())
                        return (u.u_error);
	case SIOCGACFLAGS:
		if (error = (*ifp->if_ioctl)(ifp, cmd, data))
			return(error);
                break;

#ifdef MULTICAST
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (!suser())
			return (u.u_error);
		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
#endif MULTICAST

	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
		return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
			cmd, data, ifp));
	}
	return (0);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register struct ifaddr *ifa;
	register char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
	ep = ifr.ifr_name + sizeof (ifr.ifr_name) - 2;
	for (; space > sizeof (ifr) && ifp; ifp = ifp->if_next) {
		bcopy(ifp->if_name, ifr.ifr_name, sizeof (ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
                if (ifp->if_unit<=9)  /* one digit unit number */
                        { *cp++ = '0' + ifp->if_unit; *cp ='\0'; }
                else {
                        int unit_num = ifp->if_unit;
                        char unit_char[6];
                        int i = 1;

                        while (unit_num) {
                                unit_char[i++] = '0' + unit_num % 10 ;
                                unit_num = unit_num / 10;
                        }
                        for (--i; i>0 ; i--)
                                *cp++ = unit_char[i];
                        *cp = '\0';
                }

		if ((ifa = ifp->if_addrlist) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		} else 
		    for ( ; space > sizeof (ifr) && ifa; ifa = ifa->ifa_next) {
			ifr.ifr_addr = ifa->ifa_addr;
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		}
	}
	ifc->ifc_len -= space;
	return (error);
}

/*
 * Interface controls
 */

ifcontrol(cmd, p1, p2, p3)
	int cmd;
	caddr_t p1, p2, p3;
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_control)
			(*ifp->if_control)(ifp, cmd, p1, p2, p3);
	}
}
