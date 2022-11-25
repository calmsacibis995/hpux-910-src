/* $Header: if_arp.h,v 1.4.83.4 93/09/17 19:00:12 kcs Exp $ */

#ifndef _SYS_IF_ARP_INCLUDED
#define _SYS_IF_ARP_INCLUDED
/*
 * Copyright (c) 1986 Regents of the University of California.
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
 *	@(#)if_arp.h	7.3 (Berkeley) 6/27/88
 */

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
 */
struct	arphdr {
	u_short	ar_hrd;		/* format of hardware address */
#define ARPHRD_ETHER 	1	/* ethernet hardware address */
#define ARPHRD_IEEE  	6	/* ieee hardware address */
	u_short	ar_pro;		/* format of protocol address */
	u_char	ar_hln;		/* length of hardware address */
	u_char	ar_pln;		/* length of protocol address */
	u_short	ar_op;		/* one of: */
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */


#define MAX_RIF_SIZE   18       /* for 802.5 */
/*
 * The remaining fields are variable in size,
 * according to the sizes above.
 */
/*	u_char	ar_sha[];	/* sender hardware address */
/*	u_char	ar_spa[];	/* sender protocol address */
/*	u_char	ar_tha[];	/* target hardware address */
/*	u_char	ar_tpa[];	/* target protocol address */
};

/*
 * ARP ioctl request
 */
struct arpreq {
	struct	sockaddr arp_pa;		/* protocol address */
	struct	sockaddr arp_ha;		/* hardware address */
        u_char  rif[MAX_RIF_SIZE];              /* 802.5 source routing */
	int	arp_flags;			/* flags */
};
/*  arp_flags and at_flags field values. More values in net/if_arpcom.h */
#define	ATF_INUSE	0x0001	/* entry in use */
#define	ATF_COM		0x0002	/* completed entry (enaddr valid) */
#define	ATF_PERM	0x0004	/* permanent entry */
#define	ATF_PUBL	0x0008	/* publish entry (respond for other host) */
#define	ATF_USETRAILERS	0x0010	/* has requested trailers */

struct arpstat {
	int as_badlen;		/* packet too short to be an arp */
	int as_badproto;	/* protocol not supported */
	int as_badhrd;		/* bad as_hrd field */
};
#endif  /* _SYS_IF_ARP_INCLUDED */
