/* $Header: in.h,v 1.21.83.4 93/09/17 19:02:42 kcs Exp $ */

#ifndef	_SYS_IN_INCLUDED
#define	_SYS_IN_INCLUDED
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
 *	@(#)in.h	7.6 (Berkeley) 6/29/88 plus MULTICAST 1.1
 */

/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981.
 */

/*
 * Protocols
 */
#define	IPPROTO_IP		0		/* dummy for IP */
#define	IPPROTO_ICMP		1		/* control message protocol */
#define	IPPROTO_IGMP		2		/* group mgmt protocol */
#define	IPPROTO_GGP		3		/* gateway^2 (deprecated) */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_EGP		8		/* exterior gateway protocol */
#define	IPPROTO_IGP		9		/* interior gateway protocol */
#define	IPPROTO_PUP		12		/* pup */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_IDP		22		/* xns idp */
#define	IPPROTO_OSPF		89		/* open SPF routing protocol */
#define	IPPROTO_PXP		241		/* Netipc PXP protocol */

#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256


/*
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#define	IPPORT_RESERVED		1024
#define	IPPORT_USERRESERVED	5000

/*
 * Link numbers
 */
#define	IMPLINK_IP		155
#define	IMPLINK_LOWEXPER	156
#define	IMPLINK_HIGHEXPER	158

/*
 * Internet address (a structure for historical reasons)
 */
struct in_addr {
	u_long s_addr;
};

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define	IN_CLASSA(i)		(((u_long)(i) & ((u_long)0x80000000)) == 0)
#define	IN_CLASSA_NET		((u_long)0xff000000)
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		((u_long)0x00ffffff)
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((u_long)(i) & ((u_long)0xc0000000)) == \
					((u_long)0x80000000))
#define	IN_CLASSB_NET		((u_long)0xffff0000)
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		((u_long)0x0000ffff)
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((u_long)(i) & ((u_long)0xe0000000)) == \
				  ((u_long)0xc0000000))
#define	IN_CLASSC_NET		((u_long)0xffffff00)
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		((u_long)0x000000ff)

#define	IN_CLASSD(i)		(((u_long)(i) & ((u_long)0xf0000000)) == \
				  ((u_long)0xe0000000))
#define	IN_CLASSD_NET		((u_long)0xf0000000)/* These aren't really    */
#define	IN_CLASSD_NSHIFT	28		    /* net and host fields,but*/
#define	IN_CLASSD_HOST		((u_long)0x0fffffff)/* routing needn't know.  */
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((u_long)(i) & ((u_long)0xe0000000)) == \
				  ((u_long)0xe0000000))
#define	IN_BADCLASS(i)		(((u_long)(i) & ((u_long)0xf0000000)) == \
				  ((u_long)0xf0000000))

#define	INADDR_ANY		((u_long)0x00000000)
#define	INADDR_BROADCAST	((u_long)0xffffffff)	/* must be masked */
#define	INADDR_LOOPBACK		((u_long)0x7f000001)

#define	INADDR_UNSPEC_GROUP	((u_long)0xe0000000)	/* 224.0.0.0   */
#define	INADDR_ALLHOSTS_GROUP	((u_long)0xe0000001)	/* 224.0.0.1   */
#define	INADDR_MAX_LOCAL_GROUP 	((u_long)0xe00000ff)	/* 224.0.0.255 */

#ifndef _KERNEL
#define	INADDR_NONE		((u_long)0xffffffff)	/* -1 return */
#endif

#define	IN_LOOPBACKNET		127			/* official! */

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/*
 * Macros for number representation conversion.
 */
#ifndef ntohl
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#endif

/*
 * Options for use with [gs]etsockopt at the IP level.
 */
#define	IP_OPTIONS		1	/* set/get IP per-packet options   */
#define	IP_MULTICAST_IF		2	/* set/get IP multicast interface  */
#define	IP_MULTICAST_TTL	3	/* set/get IP multicast timetolive */
#define	IP_MULTICAST_LOOP	4	/* set/get IP multicast loopback   */
#define	IP_ADD_MEMBERSHIP	5	/* add  an IP group membership     */
#define	IP_DROP_MEMBERSHIP	6	/* drop an IP group membership     */
#define	IP_TTL			16      /* set/get socket's ttl */

#define	IP_DEFAULT_MULTICAST_TTL   1	/* normally limit m'casts to 1 hop  */
#define	IP_DEFAULT_MULTICAST_LOOP  1	/* normally hear sends if a member  */
#define	IP_MAX_MEMBERSHIPS         20	/* per socket; must fit in one mbuf */

/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
struct ip_mreq {
	struct in_addr	imr_multiaddr;	/* IP multicast address of group */
	struct in_addr	imr_interface;	/* local IP address of interface */
};

#ifdef _KERNEL
extern	struct domain inetdomain;
extern	struct protosw inetsw[];
struct	in_addr in_makeaddr();
u_long	in_netof(), in_lnaof();

#define IN_MAXLINKHDR	32
#endif	/* _KERNEL */

#endif	/* _SYS_IN_INCLUDED */
