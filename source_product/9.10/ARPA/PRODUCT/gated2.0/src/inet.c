/*
 *  $Header: inet.c,v 1.1.109.5 92/02/28 15:56:25 ash Exp $
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


#include "include.h"

/*
 * Formulate an Internet address from network + host.
 */

struct in_addr
gd_inet_makeaddr(net, host, subnetsAllowed)
u_long net;
int host, subnetsAllowed;
{
    register u_long mask;
    struct in_addr addr;

    addr.s_addr = net;
    if (subnetsAllowed && (mask = if_subnetmask(addr))) {
	mask = ~mask;
    } else if (mask = gd_inet_netmask(net)) {
	mask = ~mask;
    } else {
	addr.s_addr = INADDR_ANY;
	return (addr);
    }

    addr.s_addr = net | (host & mask);
    addr.s_addr = htonl(addr.s_addr);
    return (addr);
}


/*
 * Return the network number from an internet address.
 */

u_long
gd_inet_netof(in)
struct in_addr in;
{
    register u_long net;
    register u_long mask;

    if (mask = if_subnetmask(in)) {
	net = ntohl(in.s_addr) &mask;
    } else {
	net = gd_inet_wholenetof(in);
    }

    return (net);
}


/*
 * Return the network number from an internet address.
 * unsubnetted version.
 */

u_long
gd_inet_wholenetof(in)
struct in_addr in;
{
    register u_long i = ntohl(in.s_addr);

    return (i & gd_inet_netmask(i));
}

/*
 * Return the host portion of an internet address.
 */

u_long
gd_inet_lnaof(in)
struct in_addr in;
{
    register u_long host = ntohl(in.s_addr);
    register u_long mask;

    if (mask = if_subnetmask(in)) {
	host &= ~mask;
    } else {
	host ^= gd_inet_wholenetof(in);
    }

    return (host);
}

/*
 *	Return the class of the network or zero in not valid
 */

int
gd_inet_class(net)
u_char *net;
{
    if (in_isa(*net)) {
	return CLAA;
    } else if (in_isb(*net)) {
	return CLAB;
    } else if (in_isc(*net)) {
	return CLAC;
    } else {
	return 0;
    }
}


/*
 * Return 1 if the address is believed
 * for an Internet host -- THIS IS A KLUDGE.
 */

int
gd_inet_checkhost(sin)
struct sockaddr_in *sin;
{
    u_long i = ntohl(sin->sin_addr.s_addr);

    if (!gd_inet_class((u_char *) & sin->sin_addr.s_addr) || sin->sin_port != 0) {
	return (0);
    }
    if (i != 0 && (i & 0xff000000) == 0) {
	return (0);
    }
    for (i = 0; i < sizeof(sin->sin_zero) / sizeof(sin->sin_zero[0]); i++) {
	if (sin->sin_zero[i]) {
	    return (0);
	}
    }
    return (1);
}


/*
 * hash routine for the route table.
 */

u_long
gd_inet_hash(sin)
register sockaddr_un *sin;
{
    register u_long n;

    n = gd_inet_netof(sin->in.sin_addr);
    if (n) {
	while (!(n & 0xff)) {
	    n >>= 8;
	}
    }
    return (n);
}


/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */

#ifndef	vax11c
char *
inet_ntoa(in_addr)
struct in_addr in_addr;
{
    static char buf[16];
    struct sockaddr_in addr;

    sockclear_in(&addr);
    addr.sin_addr = in_addr;		/* struct copy */

    (void) sprintf(buf, "%A",
		   &addr);

    return (buf);
}
#endif	/* vax11c */


/*
 * Checksum routine for Internet Protocol - Modified from 4.3+ networking in_chksum.c
 *
 */

#define ADDCARRY(x)  (x > 65535 ? x -= 65535 : x)
#define REDUCE {l_util.l = sum; sum = l_util.s[0] + l_util.s[1]; ADDCARRY(sum);}

u_short
gd_inet_cksum(v, nv, len)
register struct iovec v[];		/* List of iovecs */
register int nv;			/* Number of iovecs */
register int len;			/* Length of data */
{
    register u_short *w;
    register int sum = 0;
    register int vlen = 0;
    register struct iovec *vp;
    int byte_swapped = 0;

    union {
	char c[2];
	u_short s;
    } s_util;
    union {
	u_short s[2];
	long l;
    } l_util;

    for (vp = v; nv && len; nv--, vp++) {
	if (vp->iov_len == 0) {
	    continue;
	}
	w = (u_short *) vp->iov_base;
	if (vlen == -1) {
	    /*
             * The first byte of this mbuf is the continuation
             * of a word spanning between this mbuf and the
             * last mbuf.
             *
             * s_util.c[0] is already saved when scanning previous
             * mbuf.
             */
	    s_util.c[1] = *(char *) w;
	    sum += s_util.s;
	    w = (u_short *) ((char *) w + 1);
	    vlen = vp->iov_len - 1;
	    len--;
	} else {
	    vlen = vp->iov_len;
	}
	if (len < vlen) {
	    vlen = len;
	}
	len -= vlen;
	/*
         * Force to even boundary.
         */
	if ((1 & (int) w) && (vlen > 0)) {
	    REDUCE;
	    sum <<= 8;
	    s_util.c[0] = *(u_char *) w;
	    w = (u_short *) ((char *) w + 1);
	    vlen--;
	    byte_swapped = 1;
	}
	/*
         * Unroll the loop to make overhead from
         * branches &c small.
         */
	while ((vlen -= 32) >= 0) {
	    sum += w[0];
	    sum += w[1];
	    sum += w[2];
	    sum += w[3];
	    sum += w[4];
	    sum += w[5];
	    sum += w[6];
	    sum += w[7];
	    sum += w[8];
	    sum += w[9];
	    sum += w[10];
	    sum += w[11];
	    sum += w[12];
	    sum += w[13];
	    sum += w[14];
	    sum += w[15];
	    w += 16;
	}
	vlen += 32;
	while ((vlen -= 8) >= 0) {
	    sum += w[0];
	    sum += w[1];
	    sum += w[2];
	    sum += w[3];
	    w += 4;
	}
	vlen += 8;
	if (vlen == 0 && byte_swapped == 0) {
	    continue;
	}
	REDUCE;
	while ((vlen -= 2) >= 0) {
	    sum += *w++;
	}
	if (byte_swapped) {
	    REDUCE;
	    sum <<= 8;
	    byte_swapped = 0;
	    if (vlen == -1) {
		s_util.c[1] = *(char *) w;
		sum += s_util.s;
		vlen = 0;
	    } else {
		vlen = -1;
	    }
	} else if (vlen == -1) {
	    s_util.c[0] = *(char *) w;
	}
    }
    if (len) {
	trace(TR_ALL, LOG_ERR, "inet_cksum: out of data");
    }
    if (vlen == -1) {
	/* The last buffer has odd # of bytes. Follow the
           standard (the odd byte may be shifted left by 8 bits
           or not as determined by endian-ness of the machine) */
	s_util.c[1] = 0;
	sum += s_util.s;
    }
    REDUCE;
    return (~sum & 0xffff);
}
