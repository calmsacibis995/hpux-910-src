/* $Header: if_ether.h,v 1.16.83.4 93/09/17 19:02:13 kcs Exp $ */

#ifndef	_SYS_IF_ETHER_INCLUDED
#define	_SYS_IF_ETHER_INCLUDED
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
 *	@(#)if_ether.h	7.3 (Berkeley) 6/29/88
 */

/*
 * header field size constants; needed because of non-uniform padding of
 * data structures by various C compilers (we need exactly 14 bytes for
 * the size of the ether_hdr and 18 for etherxt_hdr; sizeof() can
 * return different results.
 */
#define ETHER_HLEN       14    /* header length for ethernet packets */
#define ETHERXT_HLEN     18    /* header length for extended ethernet packets */

struct ether_hdr {                  /* Ethernet header */
        u_char  destaddr[6];        /* Ethernet Destination Address(48 bits) */
        u_char  sourceaddr[6];      /* Ethernet Source Address (48 bits) */
        u_short type;               /* Type value */
};

/*
 * We have these definitions so that users will be able to compile 
 * berkeley programs using the ether_header structure.
 */
#define ether_header		ether_hdr
#define ether_dhost		destaddr
#define ether_shost 		sourceaddr
#define ether_type 		type

struct etherxt_hdr {                /* Ethernet extended SAP header */
        u_char  destaddr[6];        /* Ethernet Destination Address(48 bits) */
        u_char  sourceaddr[6];      /* Ethernet Source Address (48 bits) */
        u_short type;               /* Value is always 0x8005 (ETHERTYPE_HP) */
        u_short dcanon_addr;        /* destination canonical address */
        u_short scanon_addr;        /* source canonical address */
};


/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000	/* Trailer packet */
#define	ETHERTYPE_NTRAILER	0x0010
#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#define ETHERTYPE_ARP		0x0806	/* Addr. resolution protocol */
#define ETHERTYPE_HP		0x8005	/* HP reserved address expansion type */
#define	ETHERTYPE_APPLETALK	0x809B		/* AppleTalk */
#define	ETHERTYPE_AARP		0x80F3		/* AppleTalk ARP */

#define	ETHERMTU		1500
#define ETHERMIN		(60-14)

/*
 * Ethernet Address Resolution Protocol.
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to 
 * RFC 826.
 */
struct	ether_arp {
	struct	arphdr ea_hdr;	/* fixed-size header */
	u_char	arp_sha[6];	/* sender hardware address */
	u_char	arp_spa[4];	/* sender protocol address */
	u_char	arp_tha[6];	/* target hardware address */
	u_char	arp_tpa[4];	/* target protocol address */
};
#define	arp_hrd	ea_hdr.ar_hrd
#define	arp_pro	ea_hdr.ar_pro
#define	arp_hln	ea_hdr.ar_hln
#define	arp_pln	ea_hdr.ar_pln
#define	arp_op	ea_hdr.ar_op

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.
 */
struct	arpcom {
	struct ifnet ac_if;		/* network-visible interface */
	u_char ac_enaddr[6]; 		/* ethernet hardware address */
	struct in_addr ac_ipaddr;	/* copy of ip address- XXX */
	u_short ac_type;		/* type of interface 802.3/802.4 */
	u_short ac_flags;		/* arpcom flags. Set by lanconfig. */
	int (*ac_build_hdr)();          /* routine provided by lan driver  */
					/* to build the link level header  */
	int (*ac_output)();             /* output routine for pkts with    */
					/* resolved network address        */
};

/* arpcom types set by the driver routines on initlialization  */
#define ACT_8023	0x0001		/* 802.3/ethernet interface */
#define ACT_8024	0x0002		/* 802.4 interface */
#define ACT_FDDI        0x0003          /* FDDI interface */
#define ACT_8025        0x0004          /* 802.5 interface */
#define ACT_ATR         0x0005          /* ATR interface */

/*
 * arpcom flags. Changing the ordering of these flags will affect the
 * way the whohas8023[][] and whohas8024[][] arrays (of function pointers)
 * is called in lanc_resolve() and arptimer(). Do not change this without 
 * understanding the implications. The low order four bits are used to
 * index into the 802.3 array; the next 4 bits are used to index into
 * 802.4 array; these are mutually exclusive, and flag values must not
 * exceed 4 bits for either link type without changing code!
 */
#define ACF_ETHER	0x0001	/* ieee802.3 interface ethernet enabled */
#define ACF_IEEE8023	0x0002	/* ieee802.3 interface ieee8023 enabled */
#define ACF_SNAP8023	0x0004	/* ieee802.3 interface snap enabled */
#define ACF_SNAP8024	0x0010	/* ieee802.4 interface snap enabled */
#define ACF_IEEE8024	0x0020	/* ieee802.4 interface ieee8024 enabled */
#define ACF_SNAPFDDI    0x0100  /* FDDI SNAP interface */
#define ACF_SNAP8025	0x1000	/* ieee802.5 interface snap enabled */
#define ACF_IEEE8025	0x2000	/* ieee802.5 interface ieee8025 enabled */

/* 
 * Internet to ethernet address resolution table. Some explanation is
 * necessary for the three timers (at_timer, at_unitimer and at_incomptimer)
 * used by the arptable. 
 *  at_timer is used to determine whether to flush an arpcache entry or not.
 *  The amount of time it is allowed to stay in the arp cache depends upon
 *  whether the entry is complete or not. This is valid in all states and
 *  the timer is reset to 0 each time someone tries to send a packet.
 *
 *  at_unitimer is used to determine whether it is time to send out a unicast
 *  packet to at_iaddr. This is valid only in the ATS_COMP (complete) state.
 *  The variable that determines the limit is unicast_time (which is
 *  configurable via adb). Setting unicast_time to a low value could increase
 *  the number of packets on the lan significantly.
 *
 *  at_incomptimer is used to determine whether it is time to retry resolving
 *  the address at_iaddr. This is valid only in the ATS_HOLD (hold down) 
 *  state. The variable that determines the limit is rebroadcast_time 
 *  (which is configurable via adb). It is currently set to 60 seconds and 
 *  should not changed.
 */
struct	arptab {
	struct arptab *at_next;		/* pointers for linking into hash'ed */
	struct arptab *at_prev;		/* arp table */
	struct in_addr at_iaddr;	/* internet address */
        struct mbuf *at_hold;           /* last packet until resolved/timeout */
        struct arpcom *at_ac;           /* for retransmission */
        u_short at_flags;               /* flags */
	u_short at_state;		/* FSM */
	u_short at_type;		/* last method used for resolution */
        u_short at_timer;               /* seconds !? since last reference */
	u_int at_unitimer;		/* used to send unicasts; state COMP */
	u_int at_incomptimer;		/* used to re-broadcast; state HOLD */
	u_short at_bitcnt;		/* replies from unicasts */
        u_short at_rtcnt;               /* retransmission count */
        u_short at_seqno;               /* Probe Sequence number */
        u_char at_enaddr[6];		/* ethernet address */
        u_char at_rif[18];		/* source routing info for 8025 */
};

/*
 * arp table head, an array of which constitues the head of the arp
 * table's doubly linked hash chains.
 */
struct arphd {
	struct arptab *at_next;		/* pointers for linking into hash'ed */
	struct arptab *at_prev;		/* arp table */
};
	

#define AT_RETRANS	2	/* Max number of retransmissions */
#define AT_SLOTS	4	/* slots in whohas array -- Must be ^2 */
#define AT_MAXWAIT	2	/* how long for transistion from COMP to HOLD */

/* arptab flags	- more flags in ../net/if_arp.h */
#define ATF_ETHERXT     0x0020	/* completed entry using Probe on ether */
#define ATF_IEEE8023    0x0040	/* completed entry using Probe on ieee */
#define ATF_SNAP8023	0x0080	/* completed entry using arp on SNAP - 8023 */
#define ATF_SNAP8024	0x0100	/* completed entry using arp on SNAP - 8024 */
#define ATF_SNAPFDDI    0x0200  /* FDDI SNAP */
#define ATF_SNAP8025	0x0400	/* completed entry using arp on SNAP - 8025 */
#define ATF_IEEE8025	0x0800	/* completed entry using arp on IEEE - 8025 */
#define ATF_ATR		0x1000	/* completed entry using arp on ATR */
#define ATF_DOUBTFUL	0x2000	/* haven't got to this dest recently */

/* arptab states */
#define ATS_NULL	0		/* null state */
#define ATS_INCOMP	1		/* incomplete state */
#define ATS_HOLD	2		/* hold down state */
#define ATS_COMP	3		/* completed state */

#define ETHER_PKT		0
#define IEEE8023XSAP_PKT	1
#define SNAP8023_PKT		2
#define ETHERXT_PKT		3
#define SNAP8024_PKT		4
#define IEEE8023_PKT		5
#define SNAP8023XT_PKT		6
#define SNAP8024XT_PKT		7
#define SNAPFDDI_PKT            8
#define SNAP8025_PKT		9
#define IEEE8025_PKT	       10
#define ATR_PKT		       11

/* address resolution method/type */
#define ATT_ARP		ETHER_PKT		/* arp on ether */
#define ATT_ETHERXT	ETHERXT_PKT		/* probe on etherxt */
#define ATT_IEEE8023	IEEE8023XSAP_PKT	/* probe on ieee - 8023 */
#define ATT_SNAP8023	SNAP8023_PKT		/* arp on snap - 8023 */
#define ATT_SNAP8024	SNAP8024_PKT		/* arp on snap - 8024 */
#define ATT_INVALID	1024			/* some large number */
#define ATT_SNAPFDDI    SNAPFDDI_PKT		/* arp on snap - FDDI */
#define ATT_SNAP8025	SNAP8025_PKT		/* arp on snap - 8025 */
#define ATT_ATR		ATR_PKT			/* arp on ATR */

/* decleration for testing arp/probe code */
struct arptest {
    caddr_t ats_packet;		/* address of packet data */
    int ats_clen;		/* number of bytes to copy from ats_packet */
    int ats_mlen;		/* value for mbuf m_len */
    int ats_type;		/* type of packet coming in (ether/ieee) */
    int ats_flags;		/* flags to indicate arp/probe packet */
};
#endif	/* _SYS_IF_ETHER_INCLUDED */
