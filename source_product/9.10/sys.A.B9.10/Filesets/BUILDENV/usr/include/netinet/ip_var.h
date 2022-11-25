/* $Header: ip_var.h,v 1.14.83.4 93/09/17 19:04:20 kcs Exp $ */

#ifndef	_SYS_IP_VAR_INCLUDED
#define	_SYS_IP_VAR_INCLUDED
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
 *	@(#)ip_var.h	7.5 (Berkeley) 6/29/88 plus MULTICAST 1.0
 */

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	caddr_t	ih_next, ih_prev;	/* for protocol sequence q's */
	u_char	ih_x1;			/* (unused) */
	u_char	ih_pr;			/* protocol */
	short	ih_len;			/* protocol length */
	struct	in_addr ih_src;		/* source internet address */
	struct	in_addr ih_dst;		/* destination internet address */
};

/*
 * Ip reassembly queue structure.  Each fragment
 * being reassembled is attached to one of these structures.
 * They are timed out after ipq_ttl drops to 0, and may also
 * be reclaimed if memory becomes tight.
 */
struct ipq {
	struct	ipq *next,*prev;	/* to other reass headers */
	u_char	ipq_ttl;		/* time for reass q to live */
	u_char	ipq_p;			/* protocol of this fragment */
	u_short	ipq_id;			/* sequence id for reassembly */
	struct	ipasfrag *ipq_next,*ipq_prev;
					/* to ip headers of fragments */
	struct	in_addr ipq_src,ipq_dst;
	int	ipq_sum;		/* accumulate sum for fragment */
};

/*
 * Ip header, when holding a fragment.
 *
 * Note: ipf_next must be at same offset as ipq_next above
 *
 *   HP: ip_len was a short, but this is a problem with UDP messages
 *       larger than 32K
 */
struct	ipasfrag {
	u_char	ip_v:4,
		ip_hl:4;
	u_char	ipf_mff;		/* copied from (ip_off&IP_MF) */
	u_short	ip_len;
	u_short	ip_id;
	short	ip_off;
	u_char	ip_ttl;
	u_char	ip_p;
	u_short	ip_sum;
	struct	ipasfrag *ipf_next;	/* next fragment */
	struct	ipasfrag *ipf_prev;	/* previous fragment */
};

/*
 * Structure stored in mbuf in inpcb.ip_options
 * and passed to ip_output when ip options are in use.
 * The actual length of the options (including ipopt_dst)
 * is in m_len.
 */
#define MAX_IPOPTLEN	40

struct ipoption {
	struct	in_addr ipopt_dst;	/* first-hop dst if source routed */
	char	ipopt_list[MAX_IPOPTLEN];	/* options proper */
};

/*
 * Structure stored in an mbuf attached to inpcb.ip_moptions and
 * passed to ip_output when IP multicast options are in use.
 */
struct ip_moptions {
	struct	ifnet   *imo_multicast_ifp;  /* ifp for outgoing multicasts */
	u_char	         imo_multicast_ttl;  /* TTL for outgoing multicasts */
	u_char		 imo_multicast_loop; /* 1 => hear sends if a member */
	u_short		 imo_num_memberships;/* no. memberships this socket */
	struct in_multi *imo_membership[IP_MAX_MEMBERSHIPS];
};

struct	ipstat {
	long	ips_total;		/* total packets received */
	long	ips_badsum;		/* checksum bad */
	long	ips_tooshort;		/* packet too short */
	long	ips_toosmall;		/* not enough data */
	long	ips_badhlen;		/* ip header length < data size */
	long	ips_badlen;		/* ip length < ip header length */
	long	ips_fragments;		/* fragments received */
	long	ips_fragdropped;	/* frags dropped (dups, out of space)*/
	long	ips_fragtimeout;	/* fragments timed out */
	long	ips_forward;		/* packets forwarded */
	long	ips_cantforward;	/* packets rcvd for unreachable dest */
	long	ips_redirectsent;	/* packets forwarded on same net */
};

/*
 * Definition of raw (ip specific) control block.
 */

struct ripcb {
	u_char rcb_ttl;
};

#ifdef _KERNEL
/* flags passed to ip_output as last parameter */
#define	IP_FORWARDING		0x1		/* most of ip header exists */
#define IP_MULTICASTOPTS	0x2		/* multicast opts present */
#define	IP_ROUTETOIF		SO_DONTROUTE	/* bypass routing tables */
#define	IP_ALLOWBROADCAST	SO_BROADCAST	/* can send broadcast packets */

struct	ipstat	ipstat;
struct	ipq	ipq;			/* ip reass. queue */
u_short	ip_id;				/* ip packet ctr, for ids */

struct	mbuf *ip_srcroute();
#endif	/* _KERNEL */
#endif	/* _SYS_IP_VAR_INCLUDED */
