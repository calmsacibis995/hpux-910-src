/*
 * $Header: route.c,v 1.19.83.4 93/09/17 19:01:35 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/route.c,v $
 * $Revision: 1.19.83.4 $               $Author: kcs $
 * $State: Exp $                $Locker:  $
 * $Date: 93/09/17 19:01:35 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) route.c $Revision: 1.19.83.4 $";
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
 *	@(#)route.c	7.4 (Berkeley) 6/27/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/mib.h"
#include "../netinet/mib_kern.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/route.h"
#include "../net/netmp.h"

struct	rtentry *rthost[RTHASHSIZ];
struct	rtentry *rtnet[RTHASHSIZ];
struct	rtstat	rtstat;
int	rttrash;		/* routes not in table but not freed */
struct	sockaddr wildcard;	/* zero valued cookie for wildcard searches */
int	rthashsize = RTHASHSIZ;	/* for netstat, etc. */

/*
 * Packet routing routines.
 */

/*
 * Allocate a route.  This amounts to finding an existing rtentry which
 * follows this algorithm:
 *	- Find the most specific route (host, then net, then wildcard)
 *	- if a candidate has RTF_DOUBTFUL set AND its a redirect, try
 *	  find another route, even if its less specific.
 *	- Choose the wildcard, if present, regardless of RTF_DOUBTFUL
 *	- Choose a doubtful redirect as a last resort.
 */
rtalloc(ro)
	register struct route *ro;
{
	register struct rtentry *rt;
	register u_long hash;
	struct sockaddr *dst = &ro->ro_dst;
	int (*match)(), doinghost, s;
	struct afhash h;
	u_int af = dst->sa_family;
	struct rtentry **table;
	struct rtentry *drt = 0;
	struct rtentry *ddrt = 0;

	if (ro->ro_rt && ro->ro_rt->rt_ifp && (ro->ro_rt->rt_flags & RTF_UP))
		return;				 /* XXX */
	if (af >= AF_MAX)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	match = afswitch[af].af_netmatch;
	hash = h.afh_hosthash, table = rthost, doinghost = 1;
	NET_SPLNET(s);
again:
	for (rt = table[RTHASHMOD(hash)]; rt; rt = rt->rt_next) {
		if (rt->rt_hash != hash)
			continue;
		if ((rt->rt_flags & RTF_UP) == 0 ||
		    (rt->rt_ifp->if_flags & IFF_UP) == 0)
			continue;
		if (doinghost) {
			if (bcmp((caddr_t)&rt->rt_dst, (caddr_t)dst,
			    sizeof (*dst)))
				continue;
		} else {
			if (rt->rt_dst.sa_family != af ||
			    !(*match)(&rt->rt_dst, dst))
				continue;
		}
		if (rt->rt_flags & RTF_DOUBTFUL) {
			if (rt->rt_flags & RTF_DYNAMIC) {
				if (ddrt == 0) { /* save as last resort */
					ddrt = rt;
					continue;
				}
			} else if (drt == 0) {
				drt = rt;    /* take rather than more general */
				continue;
			}
		}
		rt->rt_refcnt++;
		NET_SPLX(s);
		if (dst == &wildcard)
			rtstat.rts_wildcard++;
		ro->ro_rt = rt;

		/*
		 * NIKE - record interface state info
		 */
		ro->ro_flags = 0;
		ro->ro_flags = rt->rt_ifp->if_flags & (RF_IFCKO|RF_IFNOACC);
		return;
	}
	/*
	 * The only route found during last pass was marked doubtful.
	 * Rather than get less specific, we hope for the best and
	 * allocate this.
	 */
	if (drt) {		
		drt->rt_refcnt++;
		NET_SPLX(s);
		if (dst == &wildcard)
			rtstat.rts_wildcard++;
		ro->ro_rt = drt;

		/*
		 * NIKE - record interface state info
		 */
		ro->ro_flags = 0;
		ro->ro_flags = drt->rt_ifp->if_flags & (RF_IFCKO|RF_IFNOACC);
		return;
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash, table = rtnet;
		goto again;
	}
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
	/*
	 * The only route found during all passes was a doubtful redirect.
	 * A rather interesting impossibility, but bullet proofing is
	 * cheap.  Rather than fail altogether, allocate this.
	 */
	if (ddrt) {		
		ddrt->rt_refcnt++;
		NET_SPLX(s);
		if (dst == &wildcard)		/* ?! */
			rtstat.rts_wildcard++;
		ro->ro_rt = ddrt;

		/*
		 * NIKE - record interface state info
		 */
		ro->ro_flags = 0;
		ro->ro_flags = ddrt->rt_ifp->if_flags & (RF_IFCKO|RF_IFNOACC);
		return;
	}
	NET_SPLX(s);
	rtstat.rts_unreach++;
}

/*
 * Find a route in the tables, without regard to flags.  Use the specificity
 * algorithm when resolving host/net conflicts.
 */

rtlookup(ro)
	register struct route *ro;
{
	register struct rtentry *rt;
	register u_long hash;
	struct sockaddr *dst = &ro->ro_dst;
	int (*match)(), doinghost, s;
	struct afhash h;
	u_int af = dst->sa_family;
	struct rtentry **table;

	if (ro->ro_rt && ro->ro_rt->rt_ifp && (ro->ro_rt->rt_flags & RTF_UP))
		return;				 /* XXX */
	if (af >= AF_MAX)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	match = afswitch[af].af_netmatch;
	hash = h.afh_hosthash, table = rthost, doinghost = 1;
	NET_SPLNET(s);
again:
	for (rt = table[RTHASHMOD(hash)]; rt; rt = rt->rt_next) {
		if (rt->rt_hash != hash)
			continue;
		if ((rt->rt_flags & RTF_UP) == 0 ||
		    (rt->rt_ifp->if_flags & IFF_UP) == 0)
			continue;
		if (doinghost) {
			if (bcmp((caddr_t)&rt->rt_dst, (caddr_t)dst,
			    sizeof (*dst)))
				continue;
		} else {
			if (rt->rt_dst.sa_family != af ||
			    !(*match)(&rt->rt_dst, dst))
				continue;
		}
		rt->rt_refcnt++;
		NET_SPLX(s);
		if (dst == &wildcard)
			rtstat.rts_wildcard++;
		ro->ro_rt = rt;
		return;
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash, table = rtnet;
		goto again;
	}
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
	NET_SPLX(s);
	rtstat.rts_unreach++;
}

rtfree(rt)
	register struct rtentry *rt;
{

	if (rt == 0)
		panic("rtfree");
	rt->rt_refcnt--;
	if (rt->rt_refcnt == 0 && (rt->rt_flags&RTF_UP) == 0) {
		rttrash--;
		if (rt->rt_dst.sa_family == AF_INET)
			MIB_ipDecrRouteNumEnt;
		FREE(rt, M_RTABLE);
		mbstat.m_mtypes[MT_RTABLE]--;
	}
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 *
 * N.B.: must be called at splnet or higher
 *
 */
rtredirect(dst, gateway, flags, src)
	struct sockaddr *dst, *gateway, *src;
	int flags;
{
	struct route ro;
	register struct rtentry *rt;

	/* verify the gateway is directly reachable */
	if (ifa_ifwithnet(gateway) == 0) {
		rtstat.rts_badredirect++;
		return;
	}
	ro.ro_dst = *dst;
	ro.ro_rt = 0;
	rtlookup(&ro);
	rt = ro.ro_rt;
#define	equal(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), sizeof(struct sockaddr)) == 0)
	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong.  If it redirects us to ourselves,
	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 *
	 * HP: But, as in 4.2BSD (HP-UX 7.0), we only compare gateway if
	 * route is __indirect__ (i.e., through a gateway).  If route is
	 * direct, redirect must be coming from a proxy ARP gateway.  In
	 * that case, we want to apply redirect to avoid recurrence.
	 */
	if ((rt && (rt->rt_flags & RTF_GATEWAY) && !equal(src, &rt->rt_gateway)) || ifa_ifwithaddr(gateway)) {
		rtstat.rts_badredirect++;
		if (rt)
			rtfree(rt);
		return;
	}
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if (rt &&
	    (*afswitch[dst->sa_family].af_netmatch)(&wildcard, &rt->rt_dst)) {
		rtfree(rt);
		rt = 0;
	}
	if (rt == 0) {
		rtinit(dst, gateway, (int)SIOCADDRT,
		    (flags & RTF_HOST) | RTF_GATEWAY | RTF_DYNAMIC, NMICMP);
		rtstat.rts_dynamic++;
		return;
	}
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface. 
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
			rtinit(dst, gateway, (int)SIOCADDRT,
			    flags | RTF_DYNAMIC, NMICMP);
			rtstat.rts_dynamic++;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.
			 */
			rt->rt_gateway = *gateway;
			rt->rt_flags |= RTF_MODIFIED;
			rt->rt_proto	= NMICMP;
			rt->rt_upd	= time.tv_sec;
			rtstat.rts_newgateway++;
		}
	} else
		rtstat.rts_badredirect++;
	rtfree(rt);
}

/*
 * Routing table ioctl interface.
 */
rtioctl(cmd, data)
	int cmd;
	caddr_t data;
{

	if (cmd != SIOCADDRT && cmd != SIOCDELRT && cmd != SIOCGTRTADDR) 
		return (EINVAL);
	if (!suser())
		return (u.u_error);
	return (rtrequest(cmd, (struct rtentry *)data));
}

/*
 * Carry out a request to change the routing table.  Called by
 * interfaces at boot time to make their ``local routes'' known,
 * for ioctl's, and as the result of routing redirects.
 */
rtrequest(req, entry)
	int req;
	register struct rtentry *entry;
{
	register struct rtentry *rt, **rtprev, **rtfirst;
	struct afhash h;
	int s, error = 0, (*match)();
	u_int af;
	u_long hash;
	struct ifaddr *ifa;
	struct ifaddr *ifa_ifwithdstaddr();

	if (req == SIOCGTRTADDR) {
	/*
	** Return the addresses of certain route-table variables.
	** This is for /etc/route, so it doesn't have to depend upon
	** a priori knowedge of which kernel file was booted.
	*/
	    struct routeaddrs routeaddrs, *rtptr;
	    rtptr = (struct rtentry *) entry;
	    rtptr->rthost = &rthost;
	    rtptr->rtnet = &rtnet;
	    rtptr->rthashsize = &rthashsize;
	    return(0);
	}
	af = entry->rt_dst.sa_family;
	if (af >= AF_MAX)
		return (EAFNOSUPPORT);
	(*afswitch[af].af_hash)(&entry->rt_dst, &h);
	if (entry->rt_flags & RTF_HOST) {
		hash = h.afh_hosthash;
		rtprev = &rthost[RTHASHMOD(hash)];
	} else {
		hash = h.afh_nethash;
		rtprev = &rtnet[RTHASHMOD(hash)];
	}
	match = afswitch[af].af_netmatch;
	s = splimp();
	for (rtfirst = rtprev; rt = *rtprev; rtprev = &rt->rt_next) {
		if (rt->rt_hash != hash)
			continue;
		if (entry->rt_flags & RTF_HOST) {
			if (!equal(&rt->rt_dst, &entry->rt_dst))
				continue;
		} else {
			if (rt->rt_dst.sa_family != entry->rt_dst.sa_family ||
			    (*match)(&rt->rt_dst, &entry->rt_dst) == 0)
				continue;
		}
		if (equal(&rt->rt_gateway, &entry->rt_gateway))
			break;
	}
	switch (req) {

	case SIOCDELRT:
		if (rt == 0) {
			error = ESRCH;
			goto bad;
		}
		*rtprev = rt->rt_next;
		if (rt->rt_refcnt > 0) {
			rt->rt_flags &= ~RTF_UP;
			rttrash++;
			rt->rt_next = 0;
		} else {
			if (rt->rt_dst.sa_family == AF_INET)
				MIB_ipDecrRouteNumEnt;
			FREE(rt, M_RTABLE);
			mbstat.m_mtypes[MT_RTABLE]--;
		}
		break;

	case SIOCADDRT:
		if (rt) {
			error = EEXIST;
			goto bad;
		}
		if ((entry->rt_flags & RTF_GATEWAY) == 0) {
			/*
			 * If we are adding a route to an interface,
			 * and the interface is a pt to pt link
			 * we should search for the destination
			 * as our clue to the interface.  Otherwise
			 * we can use the local address.
			 */
			ifa = 0;
			if (entry->rt_flags & RTF_HOST) 
				ifa = ifa_ifwithdstaddr(&entry->rt_dst);
			if (ifa == 0)
				ifa = ifa_ifwithaddr(&entry->rt_gateway);
		} else {
			/*
			 * If we are adding a route to a remote net
			 * or host, the gateway may still be on the
			 * other end of a pt to pt link.
			 */
			ifa = ifa_ifwithdstaddr(&entry->rt_gateway);
		}
		if (ifa == 0) {
			ifa = ifa_ifwithnet(&entry->rt_gateway);
			if (ifa == 0) {
				error = ENETUNREACH;
				goto bad;
			}
		}
		MALLOC(rt, struct rtentry *, sizeof (struct rtentry),
			M_RTABLE, M_NOWAIT);
		if (rt == 0) {
			error = ENOBUFS;
			goto bad;
		}
		bzero((caddr_t)rt, sizeof (struct rtentry));
		mbstat.m_mtypes[MT_RTABLE]++;
		rt->rt_next = *rtfirst;
		*rtfirst = rt;
		rt->rt_hash = hash;
		rt->rt_dst = entry->rt_dst;
		rt->rt_gateway = entry->rt_gateway;
		rt->rt_flags = RTF_UP |
		    (entry->rt_flags & (RTF_HOST|RTF_GATEWAY|RTF_DYNAMIC));
		rt->rt_refcnt = 0;
		rt->rt_use = 0;
		rt->rt_ifp = ifa->ifa_ifp;
		rt->rt_proto = entry->rt_proto;
		rt->rt_upd   = time.tv_sec;
		if (rt->rt_dst.sa_family == AF_INET)
			MIB_ipIncrRouteNumEnt;
		break;
	}
bad:
	splx(s);
	return (error);
}

/*
 * Set up a routing table entry, normally
 * for an interface.
 */
rtinit(dst, gateway, cmd, flags, mech)
	struct sockaddr *dst, *gateway;
	int cmd, flags, mech;
{
	struct rtentry route;

	bzero((caddr_t)&route, sizeof (route));
	route.rt_dst = *dst;
	route.rt_gateway = *gateway;
	route.rt_flags = flags;
	route.rt_proto = mech;
	(void) rtrequest(cmd, &route);
}

/*
 * Allow someone (presumably ARP) to tell us that our routes are
 * 'doubtful' or not.
 */
rtnotify(gateway, flags)
struct sockaddr *gateway;
int	flags;
{
	struct rtentry **table;
	struct rtentry *rt, **rtprev, **rtnext_prev;
	int i, s, doinghost = 1;

	table = rthost;

again:
	for (i = 0; i < RTHASHSIZ; i++) {
		rtprev = &table[i];
		s = splimp();
		for (; rt = *rtprev; rtprev = rtnext_prev) {
			rtnext_prev = &rt->rt_next; /* assume no deletion */  
			if ((rt->rt_flags & (RTF_UP|RTF_GATEWAY)) == 0)
				continue;
			if (bcmp((caddr_t)&rt->rt_gateway, (caddr_t)gateway,
			    sizeof (struct sockaddr)))
				continue;

			/* found a route with this as its gateway */

			if ((flags & RTN_NUKEREDIRS) &&
			    (rt->rt_flags & RTF_DYNAMIC)) {
				rtstat.rts_nukedredirects++;
				rtnext_prev = rtprev; /* dont advance rtprev */
				*rtprev = rt->rt_next;
				if (rt->rt_refcnt > 0) {
					rt->rt_flags &= ~RTF_UP;
					rttrash++;
					rt->rt_next = 0;
				} else {
					FREE(rt, M_RTABLE);
					mbstat.m_mtypes[MT_RTABLE]--;
				}
				continue;
			}

			if (flags & RTN_DOUBTFUL) {
				rt->rt_flags |= RTF_DOUBTFUL;
				rtstat.rts_setdoubtful++;

			} else {
				rt->rt_flags &= ~RTF_DOUBTFUL;
				rtstat.rts_cleardoubtful++;
			}
		} /* for rt */
		splx(s);
	} /* for i */
	if (doinghost) {
		doinghost = 0;
		table = rtnet;
		goto again;
	}
}
