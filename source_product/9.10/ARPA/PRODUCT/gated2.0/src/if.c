/*
 *  $Header: if.c,v 1.1.109.5 92/02/28 15:53:58 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


/*
 * if.c
 *
 */

#include "include.h"
#ifdef	SYSV
#include <sys/sioctl.h>
#include <sys/stropts.h>
#else	/* SYSV */
#include <sys/ioctl.h>
#endif	/* SYSV */
#include "snmp.h"

/* Some systems (SunOS 3.x where x > 2) do not define ifr_broadaddr */
#if	defined(SIOCGIFBRDADDR) && !defined(ifr_broadaddr)
#define	ifr_broadaddr	ifr_addr
#endif				/* defined(SIOCGIFBRDADDR) && !defined(ifr_broadaddr) */

task *if_task = (task *) 0;
if_entry *ifnet;			/* direct internet interface list */
int n_interfaces;			/* # internet interfaces */
int int_index_max;
int if_rtactive = FALSE;		/* Set to true if any protocol is broadcasting */

bits if_flag_bits[] =
{
    {IFS_UP, "Up"},
    {IFS_BROADCAST, "Broadcast"},
    {IFS_POINTOPOINT, "PointoPoint"},
    {IFS_SUBNET, "Subnet"},
    {IFS_LOOPBACK, "Loopback"},
    {IFS_INTERFACE, "Interface"},
    {IFS_REMOTE, "Remote"},
    {IFS_NOAGE, "NoAge"},
    {IFS_NORIPOUT, "NoRipOut"},
    {IFS_NORIPIN, "NoRipIn"},
    {IFS_NOHELLOOUT, "NoHelloOut"},
    {IFS_NOHELLOIN, "NoHelloIn"},
    {IFS_NOICMPIN, "NoIcmpIn"},
    {IFS_METRICSET, "MetricSet"},
    {IFS_MULTICAST, "Multicast"},
    {IFS_SIMPLEX, "Simplex"},
    {0}
};


/*  Find the subnetmask for the specified address, returns it in */
/*  host byte order.  Do not match the whole network of a subnetted network.	*/
u_long
if_subnetmask(addr)
struct in_addr addr;
{
    u_long mask = (u_long) 0;
    if_entry *ifp;

    IF_LIST(ifp) {
	if (!(ifp->int_state & IFS_UP)) {
	    continue;
	}
	/* Ignore the netmasks of P2P links */
	/* XXX - What if we want to specify the netmask of remote net when we configure a P2P link? */
	if (ifp->int_state & IFS_POINTOPOINT) {
	    continue;
	}
	/* XXX - Won't work with variable length subnet masks */
	if ((ifp->int_netmask.in.sin_addr.s_addr & addr.s_addr) == ifp->int_net.in.sin_addr.s_addr) {
	    if (ifp->int_state & IFS_LOOPBACK) {
		mask = ifp->int_netmask.in.sin_addr.s_addr;
	    } else {
		mask = ifp->int_subnetmask.in.sin_addr.s_addr;
	    }
	    break;
	}
    } IF_LISTEND(ifp) ;

    return (ntohl(mask));
}


/*
 * Find the interface with specified address.
 *
 *	For point-to-point interfaces, the destination address is checked
 *	for other interfaces, the interface address is checked
 *	for broadcast interfaces, the broadcast address is also checked
 */

if_entry *
if_withaddr(addr)
sockaddr_un *addr;
{
    register if_entry *ifp;

    IF_LIST(ifp) {
	if (!(ifp->int_state & IFS_UP)) {
	    continue;
	}
	if (ifp->int_state & IFS_REMOTE) {
	    continue;
	}
	if (ifp->int_addr.a.sa_family != addr->a.sa_family) {
	    continue;
	}
	if (ifp->int_state & IFS_POINTOPOINT) {
	    if (equal_in(ifp->int_dstaddr.in.sin_addr, addr->in.sin_addr)) {
		break;
	    } else {
		continue;
	    }
	}
	if (equal_in(ifp->int_addr.in.sin_addr, addr->in.sin_addr)) {
	    break;
	}
	if (ifp->int_state & IFS_BROADCAST) {
	    if (equal_in(ifp->int_broadaddr.in.sin_addr, addr->in.sin_addr)) {
		break;
	    }
	}
    } IF_LISTEND(ifp) ;
    return (ifp);
}



 /* Find the interface on the network of the specified address.  On a	*/
 /* point-to-point interface only the destination address is compared.	*/
 /* On all other interfaces, the network/subnet is compared against the	*/
 /* network/subnet of the interface address.				*/

if_entry *
if_withdst(dstaddr)
sockaddr_un *dstaddr;
{
    register if_entry *ifp;

    if (dstaddr->a.sa_family != AF_INET)
	return (0);

    /* Scan the interface list.  For P2P interfaces look for an exact	*/
    /* match of the specified address and the destination of this link.	*/
    /* For other types of interfaces search for interfaces with the same	*/
    /* (whole) netmask.  On these interfaces, compare the specified	*/
    /* address with the interface address under the subnetmask.		*/

    switch (dstaddr->a.sa_family) {
	case AF_INET:
	    IF_LIST(ifp) {
		if (!(ifp->int_state & IFS_UP)) {
		    continue;
		}
		if (ifp->int_state & IFS_POINTOPOINT) {
		    if (dstaddr->in.sin_addr.s_addr == ifp->int_dstaddr.in.sin_addr.s_addr) {
			break;
		    }
		} else if (ifp->int_state & IFS_LOOPBACK) {
		    if (dstaddr->in.sin_addr.s_addr == ifp->int_addr.in.sin_addr.s_addr) {
			break;
		    }
		} else if (!((dstaddr->in.sin_addr.s_addr ^ ifp->int_addr.in.sin_addr.s_addr) &
			     ifp->int_subnetmask.in.sin_addr.s_addr)) {
		    break;
		}
	    } IF_LISTEND(ifp) ;
	    break;
	default:
	    ifp = (if_entry *) 0;
    }

    return (ifp);
}


/*
 *	Lookup an interface by name
 */
if_entry *
if_withname(name)
char *name;
{
    if_entry *ifp;

    IF_LIST(ifp) {
	if (!strcasecmp(name, ifp->int_name)) {
	    break;
	}
    } IF_LISTEND(ifp) ;

    return (ifp);
}


/*
 *	if_display():
 *		Log the configuration of the interface
 */
void
if_display(name, ifp)
const char *name;
if_entry *ifp;
{

    tracef("%s: interface %s: %s  addr %A  metric %d  index %d  preference %d",
	   name,
	   ifp->int_name,
	   (ifp->int_state & IFS_UP) ? "up" : "down",
	   &ifp->int_addr,
	   ifp->int_metric,
	   ifp->int_index,
	   ifp->int_preference);
    trace(TR_INT, 0, NULL);
    if (ifp->int_state & IFS_BROADCAST) {
	tracef("%s: interface %s: broadaddr %A",
	       name,
	       ifp->int_name,
	       &ifp->int_broadaddr);
    }
    if (ifp->int_state & IFS_POINTOPOINT) {
	tracef("%s: interface %s: dstaddr %A",
	       name,
	       ifp->int_name,
	       &ifp->int_dstaddr);
    }
    if (ifp->int_state & (IFS_BROADCAST | IFS_POINTOPOINT)) {
	trace(TR_INT, 0, NULL);
    }
    trace(TR_INT, 0, "%s: interface %s: net %A  netmask %A",
	  name,
	  ifp->int_name,
	  &ifp->int_net,
	  &ifp->int_netmask);

    trace(TR_INT, 0, "%s: interface %s: subnet %A  subnetmask %A",
	  name,
	  ifp->int_name,
	  &ifp->int_subnet,
	  &ifp->int_subnetmask);

    trace(TR_INT, 0, "%s: interface %s: flags <%s>",
	  name,
	  ifp->int_name,
	  trace_bits(if_flag_bits, ifp->int_state));
    trace(TR_INT, 0, NULL);
}


#ifdef notdef
/* used for DEBUGing */
static void
if_print()
{
    register if_entry *ifp;

    IF_LIST(ifp) {
	if_display("if_print", ifp);
    } IF_LISTEND;
}

#endif				/* notdef */


/*	if_flags():
 *		Convert kernel interface flags to gated interface flags
 */
flag_t
if_flags(k_flags)
int k_flags;
{
    flag_t flags = 0;

    if (k_flags & IFF_UP) {
	flags |= IFS_UP;
    }
    if (k_flags & IFF_BROADCAST) {
	flags |= IFS_BROADCAST;
    }
    if (k_flags & IFF_POINTOPOINT) {
	flags |= IFS_POINTOPOINT;
    }
#ifdef	IFF_LOOPBACK
    if (k_flags & IFF_LOOPBACK) {
	flags |= IFS_LOOPBACK;
    }
#endif				/* IFF_LOOPBACK */
#ifdef	IFF_MULTICAST
    if (k_flags & IFF_MULTICAST) {
	flags |= IFS_MULTICAST;
    }
#endif				/* IFF_MULTICAST */
#ifdef	IFF_SIMPLEX
    if (k_flags & IFF_SIMPLEX) {
	flags |= IFS_SIMPLEX;
    }
#endif				/* IFF_SIMPLEX */
    return (flags);
}


/* Verify that no two non-POINTOPOINT interfaces have the same address and */
/* that no two interfaces have the same destination route */
static void
if_dupcheck()
{
    if_entry *ifp, *ifp2;

    IF_LIST(ifp) {
	if (ifp->int_state & IFS_UP) {
	    IF_LIST(ifp2) {
		if ((ifp2->int_state & IFS_UP) && (ifp != ifp2)) {
		    if (ifp->int_state & IFS_LOOPBACK) {
			/* Loopback interfaces only have a route to the */
			/* loopback host so we only need check the */
			/* destination.  But we also need to check that */
			/* someone did not configure the other end of a */
			/* POINTOPOINT link to be the loopback host */
			if (equal(&ifp->int_addr, IF_ADDR(ifp2))) {
			    break;
			}
		    } else if (ifp->int_state & IFS_POINTOPOINT) {
			/* For a POINTOPOINT interface we have to verify */
			/* that this destination address is unique amoung */
			/* all other POINTOPOINT destination addresses and */
			/* non-POINTOPOINT local addresses */
			if (equal(&ifp->int_dstaddr, IF_ADDR(ifp2))) {
			    break;
			}
		    } else if (!(ifp2->int_state & IFS_POINTOPOINT)) {
			/* For all other interfaces we must make sure that */
			/* no one else has our local address and no one */
			/* else has a route to our subnet/network */
			if (equal(&ifp->int_addr, &ifp2->int_addr)) {
			    break;
			} else if (equal(&ifp->int_subnet, &ifp2->int_subnet)) {
			    break;
			}
		    }
		}
	    } IF_LISTEND(ifp2) ;
	    if (ifp2) {
		tracef("if_dupcheck: address/destination conflicts between %s %A",
		       ifp->int_name,
		       &ifp->int_addr);
		if (ifp->int_state & IFS_POINTOPOINT) {
		    tracef(" -> %A",
			   &ifp->int_dstaddr);
		}
		tracef(" and %s %A",
		       ifp2->int_name,
		       &ifp2->int_addr);
		if (ifp2->int_state & IFS_POINTOPOINT) {
		    tracef(" -> %A",
			   &ifp->int_dstaddr);
		}
		trace(TR_ALL, LOG_ERR, NULL);
		if_display("if_dupcheck", ifp);
		if_display("if_dupcheck", ifp2);
	    }
	}
    }
}


/* init_if() determines addresses and names of internet interfaces configured.
 * Results returned in global linked list of interface tables.
 * The interface names are required for later ioctl calls re interface status.
 *
 * External variables:
 * ifnet - pointer to interface list
 * n_interfaces - number of direct internet interfaces (set here)
 */

/*ARGSUSED*/
static void
init_if(tp)
task *tp;
{
    int if_index = 0;
#ifndef	AF_LINK
    char *last_name = 0;
#endif	/* AF_LINK */
    caddr_t limit;
    struct ifconf *ifconf_req;
    if_entry *ifp;
    struct ifreq *ifrp;
    u_long a;

/*
 * Determine internet addresses of all internet interfaces
 */

    /*
     * get interface configuration.
     */
#define	IFCONF_BUFSIZE	(40 * sizeof(struct ifreq) + 1)		/* ioctl assumes > size ifreq */
    ifconf_req = (struct ifconf *) malloc(IFCONF_BUFSIZE + sizeof (struct ifconf));
    ifconf_req->ifc_buf = (caddr_t) (ifconf_req + 1);
    ifconf_req->ifc_len = IFCONF_BUFSIZE;

    if (task_ioctl(tp->task_socket, SIOCGIFCONF, (char *) ifconf_req, ifconf_req->ifc_len) < 0) {
	trace(TR_ALL, LOG_ERR, "init_if: ioctl SIOCGIFCONF: %m");
	quit(errno);
    }


#define	ifrpsize(x) ((socksize(&(x)->ifr_addr) > sizeof((x)->ifr_addr)) ? \
    	sizeof(*(x)) + socksize(&(x)->ifr_addr) - sizeof((x)->ifr_addr) : sizeof(*(x)))

    limit = (caddr_t) ifconf_req->ifc_req + ifconf_req->ifc_len;
    n_interfaces = 0;
    for (ifrp = ifconf_req->ifc_req;
	 (caddr_t) ifrp < limit;
	 ifrp = (struct ifreq *) ((caddr_t) ifrp + ifrpsize(ifrp))) {

	trace(TR_INT, 0, "init_if: interface name %s address %A",
	      ifrp->ifr_name,
	      &ifrp->ifr_addr);
	trace(TR_INT, 0, NULL);

#ifndef	AF_LINK
	/* If this is a new interface, bump the index */
	if (!last_name || strcmp(ifrp->ifr_name, last_name)) {
	    if_index++;
	    last_name = ifrp->ifr_name;
	}
#else				/* AF_LINK */
	if (ifrp->ifr_addr.sa_family == AF_LINK) {
	    if_index = ((struct sockaddr_dl *) & ifrp->ifr_addr)->sdl_index;
	}
#endif				/* AF_LINK */

	if (ifrp->ifr_addr.sa_family != AF_INET) {
	    continue;
	}
	ifp = (if_entry *) malloc(sizeof(if_entry));
	if (!ifp) {
	    trace(TR_ALL, LOG_ERR, "init_if: malloc: %m");
	    quit(errno);
	}
	ifp->int_preference = RTPREF_DIRECT;	/* Preference not set here */
	ifp->int_index = if_index;
	if (if_index > int_index_max) {
	    int_index_max = if_index;
	}

	/* Copy the interface address */
	sockcopy(&ifrp->ifr_addr, &ifp->int_addr);

	/* save name for future ioctl calls */
	(void) strncpy(ifp->int_name, ifrp->ifr_name, IFNAMSIZ);

	/* Get interface flags */
	if (task_ioctl(tp->task_socket, SIOCGIFFLAGS, (char *) ifrp, sizeof (*ifrp))) {
	    trace(TR_ALL, LOG_ERR, "init_if: %s: ioctl SIOCGIFFLAGS: %m",
		  ifp->int_name);
	    quit(errno);
	}
	/*
         * Mask off flags that we don't understand as some systems use
         * them differently than we do.
         */
	ifp->int_state = IFS_INTERFACE | if_flags(ifrp->ifr_flags);
	if ((ifp->int_state & IFS_LOOPBACK) || (ifp->int_addr.in.sin_addr.s_addr == htonl(INADDR_LOOPBACK))) {
	    /* Loopback net is never announced or aged and is not counted towards total interfaces */
	    ifp->int_state |= IFS_NOAGE | IFS_LOOPBACK;
	    n_interfaces--;
	}
	/*
         * if interface is marked down, we will include it and try again
         * later.
         */

	/* get interface metric */
#if	defined(SIOCGIFMETRIC) && defined(ifr_metric)
	if (task_ioctl(tp->task_socket, SIOCGIFMETRIC, (caddr_t) ifrp, sizeof (*ifrp)) < 0) {
	    trace(TR_ALL, LOG_ERR, "init_if: %s: ioctl SIOCGIFMETRIC: %m",
		  ifp->int_name);
	    quit(errno);
	}
	ifp->int_metric = (ifrp->ifr_metric >= 0) ? ifrp->ifr_metric : 0;
#else				/* defined(SIOCGIFMETRIC) && defined(ifr_metric) */
	ifp->int_metric = 0;
#endif				/* defined(SIOCGIFMETRIC) && defined(ifr_metric) */

	/* Get the destination address for point-to-point interfaces */
	if (ifp->int_state & IFS_POINTOPOINT) {
	    if (task_ioctl(tp->task_socket, SIOCGIFDSTADDR, (caddr_t) ifrp, sizeof (*ifrp)) < 0) {
		trace(TR_ALL, LOG_ERR, "init_if: %s: ioctl SIOCGIFDSTADDR: %m",
		      ifp->int_name);
		quit(errno);
	    }
	    sockcopy(&ifrp->ifr_dstaddr, &ifp->int_dstaddr);
	}

	/* Calculate the natural netmask */
	sockcopy(IF_ADDR(ifp), &ifp->int_netmask);
	sockcopy(&ifp->int_netmask, &ifp->int_net);
	a = ntohl(ifp->int_netmask.in.sin_addr.s_addr);
	if (IN_CLASSA(a)) {
	    ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSA_NET);
	} else if (IN_CLASSB(a)) {
	    ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSB_NET);
	} else {
	    ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSC_NET);
	}
	ifp->int_net.in.sin_addr.s_addr &= ifp->int_netmask.in.sin_addr.s_addr;

	/* Calculate or get the subnetmask */
	if (ifp->int_state & (IFS_LOOPBACK|IFS_POINTOPOINT)) {
	    sockcopy(IF_ADDR(ifp), &ifp->int_subnet);
	    sockcopy(&ifp->int_subnet, &ifp->int_subnetmask);
	    ifp->int_subnetmask.in.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	} else {
#ifdef	SIOCGIFNETMASK
	    if (task_ioctl(tp->task_socket, SIOCGIFNETMASK, (caddr_t) ifrp, sizeof (*ifrp)) < 0) {
		trace(TR_ALL, LOG_ERR, "init_if: %s: ioctl SIOGIFNETMASK: %m",
		      ifp->int_name);
		quit(errno);
	    }
	    sockcopy(&ifrp->ifr_addr, &ifp->int_subnetmask);
#ifdef	BSD4_4
	    /* Masks don't have an address family specified */
	    if (ifp->int_subnetmask.a.sa_family == AF_UNSPEC) {
		ifp->int_subnetmask.a.sa_family = ifp->int_addr.a.sa_family;
	    }
#endif	/* BSD4_4 */
#else	/* SIOCGIFNETMASK */
	    sockcopy(&ifp->int_netmask, &ifp->int_subnetmask);
	    sockcopy(&ifp->int_net, &ifp->int_subnet);
#endif	/* SIOCGIFNETMASK */

	    if (!ifp->int_subnetmask.in.sin_addr.s_addr) {
		sockcopy(&ifp->int_netmask, &ifp->int_subnetmask);
	    } else if (!equal(&ifp->int_subnetmask, &ifp->int_netmask)) {
		ifp->int_state |= IFS_SUBNET;
	    }
	    sockcopy(&ifp->int_addr, &ifp->int_subnet);
	    ifp->int_subnet.in.sin_addr.s_addr &= ifp->int_subnetmask.in.sin_addr.s_addr;
	}

	/* Get the broadcast address for broadcast interfaces */
	if (ifp->int_state & IFS_BROADCAST) {
#ifdef SIOCGIFBRDADDR
	    if (task_ioctl(tp->task_socket, SIOCGIFBRDADDR, (caddr_t) ifrp, sizeof (*ifrp)) < 0) {
		trace(TR_ALL, LOG_ERR, "init_if: %s: ioctl SIOGIFBRDADDR: %m",
		      ifp->int_name);
		quit(errno);
	    }
	    sockcopy(&ifrp->ifr_broadaddr, &ifp->int_broadaddr);
#else				/* !SIOCGIFBRDADDR */
	    /* Assume a 4.2 based system with a zeros broadcast */
	    sockcopy(&ifp->int_net, &ifp->int_broadaddr);
#endif				/* SIOCGIFBRDADDR */
	}
	/* Add new interface to the end of the interface list */
	if (ifnet) {
	    /* Entries on the list, search for the end */
	    if_entry *ifp2;

	    for (ifp2 = ifnet; ifp2->int_next; ifp2 = ifp2->int_next) ;
	    ifp->int_next = ifp2->int_next;
	    ifp2->int_next = ifp;
	} else {
	    /* Nothing on the list yet, put this first */
	    ifp->int_next = ifnet;
	    ifnet = ifp;
	}

	if_display("init_if", ifp);

	n_interfaces++;
    }

    if (!n_interfaces) {
	trace(TR_ALL, LOG_ERR, "init_if: Exit no interfaces");
	quit(ENOENT);
    }

    free((caddr_t) ifconf_req);

    if_dupcheck();
}


/*
 * if_check() checks the current status of all interfaces
 * If any interface has changed status, then the interface values
 * are re-read from the kernel and re-set.
 */

/*ARGSUSED*/
void
if_check(tip, interval)
timer *tip;
time_t interval;
{
    register if_entry *ifp;
    struct ifreq ifrequest;
    u_long a;
    task *tp = tip->timer_task;

    rt_open(if_task);
    IF_LIST(ifp) {
	(void) strcpy(ifrequest.ifr_name, ifp->int_name);

	/* get interface status flags */
	if (task_ioctl(tp->task_socket, SIOCGIFFLAGS, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
	    trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFFLAGS: %m",
		  ifp->int_name);
	    quit(errno);
	} else {
	    if ((ifrequest.ifr_flags & IFF_UP) != (ifp->int_state & IFS_UP)) {
		ifp->int_state = if_flags(ifrequest.ifr_flags) | (ifp->int_state & IFS_KEEPMASK);
		if (ifrequest.ifr_flags & IFF_UP) {

		    /* Get the interface address */
		    if (task_ioctl(tp->task_socket, SIOCGIFADDR, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
			trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFADDR: %m",
			      ifp->int_name);
			quit(errno);
		    }
		    sockcopy(&ifrequest.ifr_addr, &ifp->int_addr);

		    /* Get the destination address for point-to-point interfaces */
		    if (ifp->int_state & IFS_POINTOPOINT) {
			if (task_ioctl(tp->task_socket, SIOCGIFDSTADDR, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
			    trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFDSTADDR: %m",
				  ifp->int_name);
			    quit(errno);
			}
		    }
		    sockcopy(&ifrequest.ifr_dstaddr, &ifp->int_dstaddr);

		    /* Get the new interface metric */
		    if (!(ifp->int_state & IFS_METRICSET)) {
			/* If config file specified a metric, do not reset it */
#if	defined(SIOCGIFMETRIC) && defined(ifr_metric)
			(void) strcpy(ifrequest.ifr_name, ifp->int_name);
			if (task_ioctl(tp->task_socket, SIOCGIFMETRIC, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
			    trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFMETRIC: %m",
				  ifp->int_name);
			    quit(errno);
			}
			ifp->int_metric = (ifrequest.ifr_metric >= 0) ? ifrequest.ifr_metric : 0;
#else				/* defined(SIOCGIFMETRIC) && defined(ifr_metric) */
			ifp->int_metric = 0;
#endif				/* defined(SIOCGIFMETRIC) && defined(ifr_metric) */
		    }
		    /* Calculate the natural netmask */
		    sockcopy(IF_ADDR(ifp), &ifp->int_netmask);
		    sockcopy(&ifp->int_netmask, &ifp->int_net);
		    a = ntohl(ifp->int_netmask.in.sin_addr.s_addr);
		    if (IN_CLASSA(a)) {
			ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSA_NET);
		    } else if (IN_CLASSB(a)) {
			ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSB_NET);
		    } else {
			ifp->int_netmask.in.sin_addr.s_addr = htonl(IN_CLASSC_NET);
		    }
		    ifp->int_net.in.sin_addr.s_addr &= ifp->int_netmask.in.sin_addr.s_addr;

		    /* Get or calculate the subnetmask */

		    if (ifp->int_state & (IFS_LOOPBACK|IFS_POINTOPOINT)) {
			sockcopy(IF_ADDR(ifp), &ifp->int_subnet);
			sockcopy(&ifp->int_subnet, &ifp->int_subnetmask);
			ifp->int_subnetmask.in.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		    } else {
#ifdef	SIOCGIFNETMASK
			if (task_ioctl(tp->task_socket, SIOCGIFNETMASK, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
			    trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFNETMASK: %m",
				  ifp->int_name);
			    quit(errno);
			}
			sockcopy(&ifrequest.ifr_addr, &ifp->int_subnetmask);
#ifdef	BSD4_4
			/* Masks don't have an address family specified */
			if (ifp->int_subnetmask.a.sa_family == AF_UNSPEC) {
			    ifp->int_subnetmask.a.sa_family = ifp->int_addr.a.sa_family;
			}
#endif	/* BSD4_4 */
#else	/* SIOCGIFNETMASK */
			sockcopy(&ifp->int_netmask, &ifp->int_subnetmask);
			sockcopy(&ifp->int_net, &ifp->int_subnet);
#endif	/* SIOCGIFNETMASK */

			if (!ifp->int_subnetmask.in.sin_addr.s_addr) {
			    sockcopy(&ifp->int_netmask, &ifp->int_subnetmask);
			} else if (!equal(&ifp->int_subnetmask, &ifp->int_netmask)) {
			    ifp->int_state |= IFS_SUBNET;
			}
			sockcopy(&ifp->int_addr, &ifp->int_subnet);
			ifp->int_subnet.in.sin_addr.s_addr &= ifp->int_subnetmask.in.sin_addr.s_addr;
		    }

		    /* Get the broadcast address */
		    if (ifp->int_state & IFS_BROADCAST) {
#ifdef SIOCGIFBRDADDR
			(void) strcpy(ifrequest.ifr_name, ifp->int_name);
			if (task_ioctl(tp->task_socket, SIOCGIFBRDADDR, (char *) &ifrequest, sizeof (ifrequest)) < 0) {
			    trace(TR_ALL, LOG_ERR, "if_check: %s: ioctl SIOCGIFBRDADDR: %m",
				  ifp->int_name);
			    quit(errno);
			}
			sockcopy(&ifrequest.ifr_broadaddr, &ifp->int_broadaddr);
#else				/* !SIOCGIFBRDADDR */
			sockcopy(&ifp->int_net, &ifp->int_broadaddr);
#endif				/* SIOCGIFBRDADDR */
		    }
		    trace(TR_INT, LOG_NOTICE, "if_check: %s, address %A up",
			  ifp->int_name,
			  (ifp->int_state & IFS_POINTOPOINT) ? &ifp->int_dstaddr : &ifp->int_addr);
		    if_display("if_check", ifp);
		    if_rtup(ifp);
		} else {
		    trace(TR_INT, LOG_NOTICE, "if_check: %s, address %A down",
			  ifp->int_name,
			  (ifp->int_state & IFS_POINTOPOINT) ? &ifp->int_dstaddr : &ifp->int_addr);
		    if_rtdown(ifp);
		}
		task_ifchange(ifp);
	    }
	}
    } IF_LISTEND(ifp) ;

    if (rt_close(if_task, (gw_entry *) 0, 0)) {
	task_flash(if_task);
    }

    if_dupcheck();
}

/*
 *	Dump the interface list
 */
static void
if_dump(fd)
FILE *fd;
{
    if_entry *ifp;

    /*
     * Print out all of the interface stuff.
     */
    (void) fprintf(fd, "Interface Stats:\n\n\t\tInterfaces: %d (does not include loopback interface)\n\n", n_interfaces);
    IF_LIST(ifp) {
	(void) fprintf(fd, "\t%s\t%A\t\tIndex: %d\tPreference: %d\tMetric: %d\n",
		       ifp->int_name,
		       &ifp->int_addr,
		       ifp->int_index,
		       ifp->int_preference,
		       ifp->int_metric);
	(void) fprintf(fd, "\t\tUp-down transitions: %u", ifp->int_transitions);
	(void) fprintf(fd, "\t\tFlags: <%s>\n", trace_bits(if_flag_bits, ifp->int_state));
	if (ifp->int_state & IFS_BROADCAST) {
	    (void) fprintf(fd, "\t\tBroadcast Address:   %A\n",
			   &ifp->int_broadaddr);
	}
	if (ifp->int_state & IFS_POINTOPOINT) {
	    (void) fprintf(fd, "\t\tDestination Address: %A\n",
			   &ifp->int_dstaddr);
	}
	(void) fprintf(fd, "\t\tNet Number:    %A\t\tNet Mask:    %A\n",
		       &ifp->int_net,
		       &ifp->int_netmask);

	(void) fprintf(fd, "\t\tSubnet Number: %A\t\tSubnet Mask: %A\n",
		       &ifp->int_subnet,
		       &ifp->int_subnetmask);
	(void) fprintf(fd, "\n");
    } IF_LISTEND(ifp) ;
}


/*
 *	Get ready for a reconfig
 */
void
if_cleanup(tp)
task *tp;
{
    if_entry *ifp;

    if_rtactive = FALSE;
    
    IF_LIST(ifp) {
	ifp->int_state &= ~(IFS_NOICMPIN|IFS_NORIPIN|IFS_NORIPOUT|IFS_NOHELLOIN|IFS_NOHELLOOUT);
	if (!(ifp->int_state & IFS_LOOPBACK)) {
	    ifp->int_state &= ~(IFS_NOAGE);
	}
    } IF_LISTEND(ifp) ;
}


/*
 *	Initialize task for interface check
 */
void
if_init()
{
    int save_test_flag = test_flag;

    test_flag = FALSE;

    if_task = task_alloc("IF");
    if_task->task_rtproto = RTPROTO_DIRECT;
    if_task->task_socket = task_get_socket(AF_INET, SOCK_DGRAM, 0);
    if_task->task_dump = if_dump;
    if_task->task_cleanup = if_cleanup;
    /* XXX - need a task_reinit to reset interface options (NOAGE, NORIP*, NOHELLO*) */
    /* XXX - also need an IFS_DELETE flag and a task_cleanup */
    if (if_task->task_socket < 0) {
	quit(errno);
    }
    if (!task_create(if_task, 0)) {
	quit(EINVAL);
    }
    (void) timer_create(if_task,
			0,
			"Check",
			0,
			(time_t) IF_CHECK_INTERVAL,
			if_check);

    init_if(if_task);
    test_flag = save_test_flag;
}
