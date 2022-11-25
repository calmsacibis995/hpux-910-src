/* $Header: netisr.h,v 1.24.83.5 93/09/17 19:00:54 kcs Exp $ */

#ifndef _SYS_NETISR_INCLUDED
#define _SYS_NETISR_INCLUDED
/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)netisr.h	7.3 (Berkeley) 12/30/87
 */

/*
 * The networking code runs off software interrupts.
 */

/*
 * Each ``pup-level-1'' input queue has a bit in a ``netisr'' status
 * word which is used to de-multiplex a single software
 * interrupt used for scheduling the network code to calls
 * on the lowest level routine of each protocol.
 */
#define	NETISR_RAW		0		/* same as AF_UNSPEC */
#define	NETISR_IP		2		/* same as AF_INET */
#define	NETISR_IMP		3		/* same as AF_IMPLINK */
#define	NETISR_NS		6		/* same as AF_NS */
#define	NETISR_DDP		16		/* same as AF_APPLETALK */
#define	NETISR_NET_TIMEOUT	17		/* same as AF_MAX */
#define NETISR_ARP		18		/* arp */
#define NETISR_MAP		19		/* for MAP input events */
#define NETISR_PROBE		20		/* probe */
#define NETISR_X25		21		/* for X.25 events */
#define NETISR_DUX		22		/* DUX protocol */
#define NETISR_NIT		23		/* NIT */
#define NETISR_ISDN		24		/* ISDN */

/*
 * netisr real-time process priority (see netisr_daemon() in netisr.c)
 */

#define NETISR_RTPRIO           100

struct netisr_stats {
	int events_sched;		/* # of events scheduled	   */
	int netisr_called;		/* # of times netintr called	   */
        int net_timeouts;               /* # of calls to net_timeout()     */
        int net_untimeouts;             /* # of calls to net_untimeout()   */
        int net_timo_expired;           /* # of calls to net_ready_ntimo() */
        int net_callouts;               /* # of calls to net_callout()     */
};

#ifndef LOCORE
#ifdef _KERNEL
extern int netisr();
#endif
#endif
#endif  /* _SYS_NETISR_INCLUDED */
