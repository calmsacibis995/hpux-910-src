/*
 * $Header: raw_ip.c,v 1.13.83.4 93/09/17 19:05:07 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/raw_ip.c,v $
 * $Revision: 1.13.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:05:07 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) raw_ip.c $Revision: 1.13.83.4 $";
#endif

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
 *	@(#)raw_ip.c	7.4 (Berkeley) 6/29/88 plus MULTICAST 1.2
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#ifdef MULTICAST
#include "../netinet/in_var.h"
#endif MULTICAST

/*
 * Raw interface to IP protocol.
 */

struct	sockaddr_in ripdst = { AF_INET };
struct	sockaddr_in ripsrc = { AF_INET };
struct	sockproto ripproto = { PF_INET };
/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */
rip_input(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);

	ripproto.sp_protocol = ip->ip_p;
	ripdst.sin_addr = ip->ip_dst;
	ripsrc.sin_addr = ip->ip_src;
	raw_input(m, &ripproto, (struct sockaddr *)&ripsrc,
	  (struct sockaddr *)&ripdst);
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */
rip_output(m0, so)
	struct mbuf *m0;
	struct socket *so;
{
	register struct mbuf *m;
	register struct ip *ip;
	int len = 0, error;
	struct rawcb *rp = sotorawcb(so);
	struct sockaddr_in *sin;

#ifdef MULTICAST
	short proto = rp->rcb_proto.sp_protocol;

	/*
	 * If the protocol is IPPROTO_RAW or IPPROTO_IGMP, the user
	 * handed us a complete IP packet.  Otherwise, allocate an
	 * mbuf for a header and fill it in as needed.  This is a
	 * temporary hack, pending support for the IP_HDRINCL sockopt
	 * in 4.4BSD.
	 */
	if (proto != IPPROTO_RAW && proto != IPPROTO_IGMP) {
#endif MULTICAST
	/*
	 * Calculate data length and get an mbuf
	 * for IP header.
	 */
	for (m = m0; m; m = m->m_next)
		len += m->m_len;
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		m = m0;
		error = ENOBUFS;
		goto bad;
	}
	
	/*
	 * Fill in IP header as needed.
	 */
	m->m_off = MMAXOFF - sizeof(struct ip);
	m->m_len = sizeof(struct ip);
	m->m_next = m0;
	ip = mtod(m, struct ip *);
	ip->ip_tos = 0;
	ip->ip_off = 0;
	ip->ip_p = rp->rcb_proto.sp_protocol;
	ip->ip_len = sizeof(struct ip) + len;
	if (rp->rcb_flags & RAW_LADDR) {
		sin = (struct sockaddr_in *)&rp->rcb_laddr;
		if (sin->sin_family != AF_INET) {
			error = EAFNOSUPPORT;
			goto bad;
		}
		ip->ip_src.s_addr = sin->sin_addr.s_addr;
	} else
		ip->ip_src.s_addr = 0;
	ip->ip_dst = ((struct sockaddr_in *)&rp->rcb_faddr)->sin_addr;
	ip->ip_ttl = ((struct ripcb *) rp->rcb_pcb)->rcb_ttl;
#ifdef MULTICAST
	} else {
		m = m0;
		ip = mtod(m, struct ip *);
		if (ip->ip_src.s_addr != 0) {
			/*
			 * Verify that the source address is one of ours.
			 */
			struct ifnet *ifp;
			INADDR_TO_IFP(ip->ip_src, ifp);
			if (ifp == NULL) {
				error = EADDRNOTAVAIL;
				goto bad;
			}
		}
		ip->ip_dst = ((struct sockaddr_in *)&rp->rcb_faddr)->sin_addr;
	}

	return (ip_output(m, rp->rcb_options, &rp->rcb_route, 
	   (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST
	   | IP_MULTICASTOPTS, rp->rcb_moptions));
#else
	return (ip_output(m, rp->rcb_options, &rp->rcb_route, 
	   (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST));
#endif MULTICAST
bad:
	m_freem(m);
	return (error);
}

/*
 * Raw IP socket option processing.
 */
rip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;

{
	int error = 0;
	register struct rawcb *rp = sotorawcb(so);
	struct ripcb *rcbp = (struct ripcb *)rp->rcb_pcb;
	struct mbuf *m0;

        switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_OPTIONS:
			return (ip_pcbopts(&rp->rcb_options, *m));

		case IP_TTL:
			m0 = *m;
			if (m0 == NULL || m0->m_len < sizeof (int))
				error = EINVAL;
			else 
				rcbp->rcb_ttl = (u_char) (*mtod(m0, int *));
			break;

#ifdef MULTICAST
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_setmoptions(optname, &rp->rcb_moptions, *m);

		default:
			error = ip_mrouter_cmd(optname, so, *m);
			break;
#else
		default:
			error = EINVAL;
			break;
#endif MULTICAST
		}
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_OPTIONS:
			*m = m_get(M_WAIT, MT_SOOPTS);
			if (rp->rcb_options) {
				(*m)->m_off = rp->rcb_options->m_off;
				(*m)->m_len = rp->rcb_options->m_len;
				bcopy(mtod(rp->rcb_options, caddr_t),
				    mtod(*m, caddr_t), (unsigned)(*m)->m_len);
			} else
				(*m)->m_len = 0;
			break;

		case IP_TTL:
	                *m = m0 = m_get(M_WAIT, MT_SOOPTS);
			m0->m_len = sizeof(int);
                        *mtod(m0, int *) = (int) rcbp->rcb_ttl;
			break;

#ifdef MULTICAST
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_getmoptions(optname, rp->rcb_moptions, m);
			break;
#endif MULTICAST

		default:
			error = EINVAL;
			break;
		}
		break;

         case PRCO_SHRINKBUFFER:
                  break;

         default: 
                  error = EINVAL;
	          break;
	}

	if (op == PRCO_SETOPT && *m)
		(void)m_free(*m);
	return (error);
}

struct ripcb *
rip_newripcb()
{
	struct ripcb *rcb;
	MALLOC(rcb, struct ripcb *, sizeof (struct ripcb), M_PCB, M_NOWAIT);
	if (rcb == NULL)
		return(0);
	rcb->rcb_ttl = ipDefaultTTL;
	return(rcb);
}
