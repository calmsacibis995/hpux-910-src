/*
 * $Header: if.h,v 1.1.109.5 92/02/28 15:54:23 ash Exp $
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
 * Interface data definitions.
 *
 * Modified from Routing Table Management Daemon, routed/interface.h
 *
 * Structure interface stores information about a directly attached interface,
 * such as name, internet address, and bound sockets. The interface structures
 * are in a singly linked list pointed to by external variable "ifnet".
 */

typedef struct _if_entry {
    struct _if_entry *int_next;
    sockaddr_un int_addr;		/* Interface address */
    sockaddr_un int_dstaddr;		/* Destination address */
#define	int_broadaddr	int_dstaddr
    sockaddr_un int_net;		/* network # */
    sockaddr_un int_netmask;		/* net mask for addr */
    sockaddr_un int_subnet;		/* subnet # */
    sockaddr_un int_subnetmask;		/* subnet mask for addr */
    int int_metric;			/* init's routing entry */
    flag_t int_state;			/* see below */
    int int_ipackets;			/* input packets received */
    int int_opackets;			/* output packets sent */
    char int_name[IFNAMSIZ];		/* from kernel if structure */
    u_short int_transitions;		/* times gone up-down */
    int int_index;			/* Index into the kernel's interface table for SNMP */
    pref_t int_preference;		/* Preference for this interface */
} if_entry;

/* Interface flags.  Renamed to IFS_ to avoid conflicts with kernel interface flags */

#define	IFS_UP		0x01		/* interface is up */
#define	IFS_BROADCAST	0x02		/* broadcast address valid */
#define	IFS_POINTOPOINT 0x04		/* interface is point-to-point link */
#define	IFS_REMOTE	0x08		/* interface isn't on this machine */
#define	IFS_LOOPBACK	0x10		/* This is a loopback interface */
#define	IFS_INTERFACE	0x20		/* hardware interface */
#define	IFS_SUBNET	0x40		/* is this a subnet interface? */
#define	IFS_NOAGE	0x80		/* don't time out/age this interface */
#define	IFS_NORIPOUT	0x0100		/* Talk RIP on this interface? */
#define	IFS_NORIPIN	0x0200		/* Listen to RIP on this interface? */
#define	IFS_NOHELLOOUT	0x0400		/* Talk HELLO on this interface? */
#define	IFS_NOHELLOIN	0x0800		/* Listen to HELLO on this interface? */
#define	IFS_NOICMPIN	0x1000		/* Listen to ICMP on this interface */
#define	IFS_METRICSET	0x2000		/* Set if metric specified in config file */
#define	IFS_MULTICAST	0x4000		/* Multicast possible on this interface */
#define	IFS_SIMPLEX	0x8000		/* Can't hear my own packets */

/* Flags to maintain during an if_check() */
#define	IFS_KEEPMASK	(IFS_METRICSET | IFS_NOICMPIN |\
			IFS_NOHELLOIN | IFS_NOHELLOOUT |\
			IFS_NORIPIN | IFS_NORIPOUT |\
			IFS_NOAGE | IFS_LOOPBACK |\
			IFS_INTERFACE)

/*
 * When we find any interfaces marked down we rescan the
 * kernel every CHECK_INTERVAL seconds to see if they've
 * come up.
 */

#define	IF_CHECK_INTERVAL	(1*60)

#define	IF_LIST(ifp) for (ifp = ifnet; ifp; ifp = ifp->int_next)

#define	IF_LISTEND(ifp)

/* To get the correct address for an interface */
#define	IF_ADDR(ifp)	((ifp)->int_state & IFS_POINTOPOINT ? &(ifp)->int_dstaddr : &(ifp)->int_addr)

extern int n_interfaces;		/* # internet interfaces */
extern int int_index_max;		/* Maximum if_index seen */
extern if_entry *ifnet;			/* direct internet interface list */

#ifdef	USE_PROTOTYPES
extern void if_init(void);
extern if_entry *if_withdst(sockaddr_un * dstaddr);
extern if_entry *if_withaddr(sockaddr_un * dstaddr);
extern if_entry *if_withname(char *name);
extern u_long if_subnetmask(struct in_addr addr);	/* Subnet mask of an address if known */
extern void if_check(timer * tip, time_t interval);
extern void if_display(const char *name, if_entry * ifp);
extern void if_rtupdate(if_entry * ifp);
extern void if_rtup(if_entry * ifp);
extern void if_rtdown(if_entry * ifp);
extern void if_rtinit(void);

#else				/* USE_PROTOTYPES */
extern void if_init();
extern if_entry *if_withdst();
extern if_entry *if_withaddr();
extern if_entry *if_withname();
extern u_long if_subnetmask();		/* Subnet mask of an address if known */
extern void if_check();
extern void if_display();
extern void if_rtupdate();
extern void if_rtup();
extern void if_rtdown();
extern void if_rtinit();

#endif				/* USE_PROTOTYPES */

extern bits if_flag_bits[];		/* Interface flag bits */
extern int if_rtactive;			/* True if a broadcast interface is supplying */
extern task *if_task;			/* If check task pointer */
