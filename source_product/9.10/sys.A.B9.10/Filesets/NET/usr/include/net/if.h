/* $Header: if.h,v 1.30.83.4 93/09/17 18:59:59 kcs Exp $ */

#ifndef _SYS_IF_INCLUDED
#define _SYS_IF_INCLUDED
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
 *	@(#)if.h	7.3 (Berkeley) 6/27/88 plus MULTICAST 1.1
 */

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */
struct ifnet {
	char	*if_name;		/* name, e.g. ``en'' or ``lo'' */
	short	if_unit;		/* sub-unit for lower level driver */
	short	if_mtu;			/* maximum transmission unit */
	int	if_index;	
	u_short	if_flags;		/* up/down, broadcast, etc. */
	short	if_timer;		/* time 'til if_watchdog called */
	int	if_metric;		/* routing metric (external only) */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
	} if_snd;			/* output queue */
/* procedure handles */
	int	(*if_init)();		/* init routine */
	int	(*if_output)();		/* output routine */
	int	(*if_ioctl)();		/* ioctl routine */
	int	(*if_control)();	/* kernel control routine */
	int	(*if_watchdog)();	/* timer routine */
/* generic interface statistics */
	int	if_ipackets;		/* packets received on interface */
	int	if_ierrors;		/* input errors on interface */
	int	if_opackets;		/* packets sent on interface */
	int	if_oerrors;		/* output errors on interface */
	int	if_collisions;		/* collisions on csma interfaces */
/* end statistics */
	struct	ifnet *if_next;
};

#define	IFF_UP		0x1		/* interface is up */
#define	IFF_BROADCAST	0x2		/* broadcast address valid */
#define	IFF_DEBUG	0x4		/* turn on debugging */
#define	IFF_LOOPBACK	0x8		/* is a loopback net */
#define	IFF_POINTOPOINT	0x10		/* interface is point-to-point link */
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers */
#define	IFF_RUNNING	0x40		/* resources allocated */
#define	IFF_NOARP	0x80		/* no address resolution protocol */
/* next two not supported now, but reserved: */
#define	IFF_PROMISC	0x100		/* receive all packets */
#define	IFF_ALLMULTI	0x200		/* receive all multicast packets */
/* for host requirements: */
#define IFF_LOCALSUBNETS 0x400		/* subnets off this net are local */
#define	IFF_MULTICAST	0x800		/* supports multicast */
/*
 * The IFF_MULTICAST flag indicates that the network can support the
 * transmission and reception of higher-level (e.g., IP) multicast packets.
 * It is independent of hardware support for multicasting; for example,
 * point-to-point links or pure broadcast networks may well support
 * higher-level multicasts.
 */
/* next two used for hardware checksum and copy avoidance */
#define	IFF_CKO		0x1000		/* interface supports checksum offload*/
#define	IFF_NOACC	0x2000		/* no data access on outbound */
/* Required for ATR driver */
#define IFF_OACTIVE	0x4000		/* transmission in progress */

/* flags set internally only: */
#define	IFF_CANTCHANGE	(IFF_BROADCAST | IFF_POINTOPOINT |   \
				IFF_RUNNING | IFF_MULTICAST | IFF_OACTIVE)

#define IFC_IFBIND		0x0001	/* bind interface to driver */
#define IFC_IFATTACH		0x0002	/* attach connection to driver */
#define IFC_IFDETACH		0x0004	/* detach connection from driver */
#define IFC_PRBPROXYBIND	0x0008	/* bind probe PROXY, add mutlicast */
#define IFC_PRBPROXYUNBIND	0x0010	/* unbind probe PROXY, del multicast */
#define IFC_OSIBIND		0x0020	/* bind OSI and add multicast */
#define IFC_OSIUNBIND		0x0040	/* unbind OSI and delete multicast */
#define IFC_PRBVNABIND		0x0080	/* bind probe VNA and add multicast */
#define IFC_IFPATHREPORT	0x0100	/* get path report element */
#define IFC_IFNMGET		0x1000	/* netwk mgmt get operation */
#define IFC_IFNMSET		0x2000	/* netwk mgmt set operation */
#define IFC_IFNMEVENT		0x4000  /* netwk mgmt evnt generation */


/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { \
	int s; \
	s = splimp(); \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_act = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
	splx(s); \
}
#define	IF_ENQUEUEIF(ifq, m0, ifp) { 					\
	int s;								\
	s = splimp();							\
	*((struct ifnet **) (m0)->m_quad) = ifp;			\
	(m0)->m_act = 0; 						\
	if ((ifq)->ifq_tail == 0) 					\
		(ifq)->ifq_head = m0; 					\
	else 								\
		(ifq)->ifq_tail->m_act = m0; 				\
	(ifq)->ifq_tail = m0; 						\
	(ifq)->ifq_len++; 						\
	splx(s);							\
}
#define	IF_PREPEND(ifq, m) { \
	(m)->m_act = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}
#define	IF_QFULL_TRAIN(ifq, n)	(((ifq)->ifq_len + (n)) > (ifq)->ifq_maxlen)
#define	IF_DROP_TRAIN(ifq, n)	((ifq)->ifq_drops += (n))
#define	IF_ENQUEUE_TRAIN(ifq, m, last, pkts) { 			\
	if ((ifq)->ifq_tail == 0) 					\
		(ifq)->ifq_head = m; 					\
	else 								\
		(ifq)->ifq_tail->m_act = m; 				\
	(ifq)->ifq_tail = last; 					\
	(ifq)->ifq_len += pkts; 					\
}
#define	IF_ENQUEUEIF_TRAIN(ifq, m0, ifp, last, pkts) { 			\
	*((struct ifnet **) (m0)->m_quad) = ifp;			\
	if ((ifq)->ifq_tail == 0) 					\
		(ifq)->ifq_head = m0; 					\
	else 								\
		(ifq)->ifq_tail->m_act = m0; 				\
	(ifq)->ifq_tail = last;						\
	(ifq)->ifq_len += pkts;						\
}
/*
 * Packets destined for level-1 protocol input routines
 * have a pointer to the receiving interface prepended to the data.
 * IF_DEQUEUEIF extracts and returns this pointer when dequeueing the packet.
 * IF_ADJ should be used otherwise to adjust for its presence.
 */
#define	IF_ADJ(m) { \
	(m)->m_off += sizeof(struct ifnet *); \
	(m)->m_len -= sizeof(struct ifnet *); \
	if ((m)->m_len == 0) { \
		struct mbuf *n; \
		MFREE((m), n); \
		(m) = n; \
	} \
}
#define	IF_DEQUEUEIF(ifq, m0, ifp) { 					\
	int s;								\
	s = splimp();							\
	(m0) = (ifq)->ifq_head; 					\
	if (m0) { 							\
		if (((ifq)->ifq_head = (m0)->m_act) == 0) 		\
			(ifq)->ifq_tail = 0; 				\
		(m0)->m_act = 0; 					\
		(ifq)->ifq_len--; 					\
		(ifp) = *((struct ifnet **) (m0)->m_quad);		\
	} 								\
	splx(s);							\
}
#define	IF_DEQUEUE(ifq, m) { \
	int s; \
	s = splimp(); \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
	} \
	splx(s); \
}

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct	sockaddr ifa_addr;	/* address of interface */
	union {
		struct	sockaddr ifu_broadaddr;
		struct	sockaddr ifu_dstaddr;
	} ifa_ifu;
#define	ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address */
#define	ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of p-to-p link */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifaddr *ifa_next;	/* next address for interface */
};

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct	ifreq {
#define	IFNAMSIZ	16
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		caddr_t	ifru_data;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
};

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

#define TRACE_PKT(ifp, ss, event, m, af) {				\
	if (KTRC_SUBSYS_ON(ss)) {					\
		extern struct mbuf *trace_m;				\
		trace_m->m_next = (m);					\
		*(mtod(trace_m, short *)) = (short) (af);		\
		ns_trace_link((ifp), (event), trace_m, (ss));		\
	}								\
}

#ifdef _KERNEL_BUILD
#include "../net/if_arp.h"
#else /* ! _KERNEL_BUILD  */
#include <net/if_arp.h>
#endif /* _KERNEL_BUILD  */

#ifdef _KERNEL
extern	struct	ifqueue rawintrq;		/* raw packet input queue */
extern	struct	ifnet *ifnet;
struct	ifaddr *ifa_ifwithaddr(), *ifa_ifwithnet();
struct	ifaddr *ifa_ifwithdstaddr();
#endif /* KERNEL */

#endif  /* _SYS_IF_INCLUDED */
