/*
 * $Header: ip_input.c,v 1.51.83.4 93/09/17 19:03:53 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/ip_input.c,v $
 * $Revision: 1.51.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:03:53 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) ip_input.c $Revision: 1.51.83.4 $ $Date: 93/09/17 19:03:53 $";
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
 *	@(#)ip_input.c	7.10 (Berkeley) 6/29/88 plus MULTICAST 1.1
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../net/cko.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/tcp.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../net/netmp.h"

#undef insque
#undef remque

u_char	ip_protox[IPPROTO_MAX];
int	ipqmaxlen = IFQ_MAXLEN;
struct	in_ifaddr *in_ifaddr;			/* first inet address */

#ifndef	IPFORWARDING
#define	IPFORWARDING	1
#endif
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif
int	ipprintfs = 0;
int	ipforwarding = IPFORWARDING;
int     ipq_msize = 0; /* netmem used by IP Reassembly queues */
extern	int in_interfaces;
extern  int netmemmax;
int	ipsendredirects = IPSENDREDIRECTS;

u_char inetctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		0,		EHOSTDOWN,	EHOSTUNREACH,
	ENETUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */
int	ip_nhops = 0;
static	struct ip_srcrt {
	char	nop;				/* one NOP to align */
	char	srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and OFFSET */
	struct	in_addr route[MAX_IPOPTLEN];
} ip_srcrt;

/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */
ip_init()
{
	register struct protosw *pr;
	register int i;

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);
	if (pr == 0)
		panic("ip_init");
	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;
	for (pr = inetdomain.dom_protosw;
	    pr < inetdomain.dom_protoswNPROTOSW; pr++)
		if (pr->pr_domain->dom_family == PF_INET &&
		    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
			ip_protox[pr->pr_protocol] = pr - inetsw;
	ipq.next = ipq.prev = &ipq;
	ip_id = time.tv_sec & 0xffff;
	ipintrq.ifq_maxlen = ipqmaxlen;
	ifcontrol(IFC_IFBIND, AF_INET, NETISR_IP, &ipintrq);

	MIB_ipcounter [ID_ipReasmTimeout & INDX_MASK]=(counter) (IPFRAGTTL>>1);
	MIB_ipcounter [ID_ipInDiscards & INDX_MASK] = 0;

	/* Register get/set routines with Network Mgmt */
	NMREG(GP_ip, nmget_ip, nmset_ip,  nmcreate_ip,  nmdelete_ip);
	NMREG(GP_icmp, nmget_icmp, mib_unsupp, mib_unsupp, mib_unsupp);
}

u_char	ipcksum = 1;
struct	ip *ip_reass();
struct	sockaddr_in ipaddrs = { AF_INET };
struct	route ipforward_rt;

/*
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassamble.  If complete and fragment queue exists, discard.
 * Process options.  Pass to next level.
 */
ipintr()
{
	register struct ip *ip;
	register struct mbuf *m;
	struct mbuf *m0;
	register int i;
	register struct ipq *fp;
	register struct in_ifaddr *ia;
	struct ifnet *ifp;
	int hlen, s;
	int savesum;		/* HP : checksum assist */

next:
	/*
	 * Get next datagram off input queue and get IP header
	 * in first mbuf.
	 */
	s = splimp();
	IF_DEQUEUEIF(&ipintrq, m, ifp);
	splx(s);
	if (m == 0)
		return;
	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
	 */
	if (in_ifaddr == NULL)
		goto bad;
	ipstat.ips_total++;
	MIB_ipIncrCounter(ID_ipInReceives);
	/* 
	 * HP : If interface provided checksum, save the checksum
	 * 	in case we call m_pullup().
	 */
	CKO_SAVE(m,savesum);
	if ((m->m_off > MMAXOFF || 
	     m->m_len < sizeof (struct ip) ||
	     M_UNALIGNED(m)) &&
	    (m = m_pullup(m, sizeof (struct ip))) == 0) {
		ipstat.ips_toosmall++;
		MIB_ipIncrCounter(ID_ipInHdrErrors);
		goto next;
	}
	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
		ipstat.ips_badhlen++;
		MIB_ipIncrCounter(ID_ipInHdrErrors);
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {
			ipstat.ips_badhlen++;
			MIB_ipIncrCounter(ID_ipInHdrErrors);
			goto next;
		}
		ip = mtod(m, struct ip *);
	}
	/* HP : restore interface provided checksum */
	CKO_RESTORE(m, savesum);
	if (ipcksum)
#ifdef __hp9000s800
		if (ip->ip_sum = ip_cksum(ip, hlen))
#else
		if (ip->ip_sum = in_cksum(m, hlen))
#endif
		{
			ipstat.ips_badsum++;
			MIB_ipIncrCounter(ID_ipInHdrErrors);
			goto bad;
		}

	/*
	 * Convert fields to host representation.
	 */
	ip->ip_len = ntohs((u_short)ip->ip_len);
	if (ip->ip_len < hlen) {
		ipstat.ips_badlen++;
		MIB_ipIncrCounter(ID_ipInHdrErrors);
		goto bad;
	}
	ip->ip_id = ntohs(ip->ip_id);
	ip->ip_off = ntohs((u_short)ip->ip_off);

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	i = -(u_short)ip->ip_len;
	m0 = m;
	for (;;) {
		i += m->m_len;
		if (m->m_next == 0)
			break;
		m = m->m_next;
	}
	if (i != 0) {
		if (i < 0) {
			ipstat.ips_tooshort++;
			m = m0;
			MIB_ipIncrCounter(ID_ipInHdrErrors);
			goto bad;
		}
		if (i <= m->m_len)
			m->m_len -= i;
		else
			m_adj(m0, -i);
	}
	m = m0;

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */
	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(ip, ifp)) {
		MIB_ipIncrCounter(ID_ipInHdrErrors);
		goto next;
	}

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
#define	satosin(sa)	((struct sockaddr_in *)(sa))

		if (IA_SIN(ia)->sin_addr.s_addr == ip->ip_dst.s_addr)
			goto ours;
		if (
#ifdef	DIRECTED_BROADCAST
		    ia->ia_ifp == ifp &&
#endif
		    (ia->ia_ifp->if_flags & IFF_BROADCAST)) {
			u_long t;

			if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr ==
			    ip->ip_dst.s_addr)
				goto ours;
			if (ip->ip_dst.s_addr == ia->ia_netbroadcast.s_addr)
				goto ours;
			/*
			 * Look for all-0's host part (old broadcast addr),
			 * either for subnet or net.
			 */
			t = ntohl(ip->ip_dst.s_addr);
			if (t == ia->ia_subnet)
				goto ours;
			if (t == ia->ia_net)
				goto ours;
		}
	}
#ifdef MULTICAST
	if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr))) {
		struct in_multi *inm;
		extern struct socket *ip_mrouter;

		if (ip_mrouter) {
			/*
			 * If we are acting as a multicast router, all
			 * incoming multicast packets are passed to the
			 * kernel-level multicast forwarding function.
			 * The packet is returned (relatively) intact; if
			 * ip_mforward() returns a non-zero value, the packet
			 * must be discarded, else it may be accepted below.
			 *
			 * (The IP ident field is put in the same byte order
			 * as expected when ip_mforward() is called from
			 * ip_output().)
			 */
			ip->ip_id = htons(ip->ip_id);
			if (ip_mforward(ip, ifp) != 0) {
				m_freem(dtom(ip));
				goto next;
			}
			ip->ip_id = ntohs(ip->ip_id);

			/*
			 * The process-level routing demon needs to receive
			 * all multicast IGMP packets, whether or not this
			 * host belongs to their destination groups.
			 */
			if (ip->ip_p == IPPROTO_IGMP)
				goto ours;
		}
		/*
		 * See if we belong to the destination multicast group on the
		 * arrival interface.
		 */
		IN_LOOKUP_MULTI(ip->ip_dst, ifp, inm);
		if (inm == NULL) {
			m_freem(dtom(ip));
			goto next;
		}
		goto ours;
	}
#endif MULTICAST
	if (ip->ip_dst.s_addr == (u_long)INADDR_BROADCAST)
		goto ours;
	if (ip->ip_dst.s_addr == INADDR_ANY)
		goto ours;

	/*
	 * Not for us; forward if possible and desirable.
	 */
	ip_forward(ip, ifp);
	goto next;

ours:
	/*
	 * If offset or IP_MF are set, must reassemble.
	 * Otherwise, nothing need be done.
	 * (We could look in the reassembly queue to see
	 * if the packet was previously fragmented,
	 * but it's not worth the time; just let them time out.)
	 */
	if (ip->ip_off &~ IP_DF) {
		/*
		 * Look for queue of fragments
		 * of this datagram.
		 */
		for (fp = ipq.next; fp != &ipq; fp = fp->next)
			if (ip->ip_id == fp->ipq_id &&
			    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
			    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
			    ip->ip_p == fp->ipq_p)
				goto found;
		fp = 0;
found:

		/*
		 * Adjust ip_len to not reflect header,
		 * set ip_mff if more fragments are expected,
		 * convert offset of this to bytes.
		 */
		ip->ip_len -= hlen;
		((struct ipasfrag *)ip)->ipf_mff = 0;
		if (ip->ip_off & IP_MF)
			((struct ipasfrag *)ip)->ipf_mff = 1;
		ip->ip_off <<= 3;

		/*
		 * If datagram marked as having more fragments
		 * or if this is not the first fragment,
		 * attempt reassembly; if it succeeds, proceed.
		 */
		if (((struct ipasfrag *)ip)->ipf_mff || ip->ip_off) {
			ipstat.ips_fragments++;
			ip = ip_reass((struct ipasfrag *)ip, fp);
			if (ip == 0)
				goto next;
			m = dtom(ip);
		} else
			if (fp)
				ip_freef(fp);
	} else
		ip->ip_len -= hlen;

	/*
	 * Switch out to protocol's input routine.
	 */
	(*inetsw[ip_protox[ip->ip_p]].pr_input)(m, ifp);
	MIB_ipIncrCounter(ID_ipInDelivers);
	goto next;
bad:
	m_freem(m);
	goto next;
}

int
ip_count_mem(m)
struct mbuf *m;
{
   int count = 0;

   while(m) {
      count += MSIZE;
      if (M_HASCL(m))
         count += m->m_clsize;
      m = m->m_next;
   }
   return(count);
}

/*
 * Take incoming datagram fragment and try to
 * reassemble it into whole datagram.  If a chain for
 * reassembly of this datagram already exists, then it
 * is given as fp; otherwise have to make a chain.
 */
struct ip *
ip_reass(ip, fp)
	register struct ipasfrag *ip;
	register struct ipq *fp;
{
	register struct mbuf *m = dtom(ip);
	register struct ipasfrag *q, *tt;
	struct mbuf *t;
	int hlen = ip->ip_hl << 2;
	int i, next;

        /* variables for ipq_msize computations (ip frag memory size)
         * to ensure that ipq_msize <= netmemmax
         */
        int newcnt = 0;  /* mbuf memory added to ip frag memory */
        int oldcnt = 0;  /* mbuf memory removed from ip frag memory */

	/*
	 * Presence of header sizes in mbufs
	 * would confuse code below.
	 */
	m->m_off += hlen;
	m->m_len -= hlen;
	MIB_ipIncrCounter(ID_ipReasmReqds);

	/*
	 * If first fragment to arrive, create a reassembly queue.
	 */
	if (fp == 0) {
		if ((t = m_get(M_DONTWAIT, MT_FTABLE)) == NULL) {
			MIB_ipIncrCounter(ID_ipReasmFails);
			goto dropfrag;
		}
                /*
                 * HP : IP Reassembly : Will store mem used in m_quad[1]
		 *
		 * Note:  We set m_quad[1] even if netmemmax < 0 now to
		 * ensure correctness in case netmemmax is changed with
		 * adb later.
                 */
                newcnt = MSIZE;
		t->m_quad[1] = 0;
		fp = mtod(t, struct ipq *);
		insque(fp, &ipq);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = fp->ipq_prev = (struct ipasfrag *)fp;
		fp->ipq_src = ((struct ip *)ip)->ip_src;
		fp->ipq_dst = ((struct ip *)ip)->ip_dst;
		/* HP : checksum assist */
		if (m->m_flags & MF_CKO_IN)
			fp->ipq_sum = CKO_IPQSUM_ON;
		else
			fp->ipq_sum = CKO_IPQSUM_OFF;
		q = (struct ipasfrag *)fp;
		goto insert;
	}

	/*
	 * Find a segment which begins after this one does.
	 */
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if (q->ipf_prev != (struct ipasfrag *)fp) {
		i = q->ipf_prev->ip_off + q->ipf_prev->ip_len - ip->ip_off;
		if (i > 0) {
			if (i >= ip->ip_len)
				goto dropfrag;
			m_adj(dtom(ip), i);
			ip->ip_off += i;
			ip->ip_len -= i;
			/* HP : checksum assist */
			fp->ipq_sum = CKO_IPQSUM_OFF;
		}
	}

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while( q!=(struct ipasfrag *)fp && ip->ip_off + ip->ip_len > q->ip_off){
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			q->ip_len -= i;
			q->ip_off += i;
			m_adj(dtom(q), i);
			/* HP : checksum assist */
			fp->ipq_sum = CKO_IPQSUM_OFF;
			break;
		}
		tt = q;
		q = q->ipf_next;
		ip_deq(tt);
                /* 
                 * HP : IP Reassembly : Update old count since we dropped 
                 *      from something from the original list
                 */
                oldcnt += (dtom(tt))->m_quad[1];
		m_freem(dtom(tt));
	}

insert:
	/*
	 * Stick new segment in its place;
	 * check for complete reassembly.
	 */
	ip_enq(ip, q->ipf_prev);

	/* HP : checksum assist.  Accumulate partial sum for fragment */
	CKO_PSUM(m, fp->ipq_sum);
	next = 0;
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next) {
		if (q->ip_off != next)
		   goto checkmem;
		next += q->ip_len;
	}
	if (q->ipf_prev->ipf_mff)
		goto checkmem;

	/*
	 * Reassembly is complete; concatenate fragments.
	 */
	q = fp->ipq_next;
	m = dtom(q);
	t = m->m_next;
	m->m_next = 0;
	m_cat(m, t);
	q = q->ipf_next;
	while (q != (struct ipasfrag *)fp) {
		t = dtom(q);
		q = q->ipf_next;
		m_cat(m, t);
	}

	/*
	 * Create header for new ip packet by
	 * modifying header of first packet;
	 * dequeue and discard fragment reassembly header.
	 * Make header visible.
	 */
	ip = fp->ipq_next;
	ip->ip_len = next;
	((struct ip *)ip)->ip_src = fp->ipq_src;
	((struct ip *)ip)->ip_dst = fp->ipq_dst;
	m = dtom(ip);
	m->m_len += (ip->ip_hl << 2);
	m->m_off -= (ip->ip_hl << 2);

        /*
         * HP : IP Reassembly : decrement gloabal mem count
         */
        ipq_msize -= (dtom(fp))->m_quad[1];

	/* 
	 * HP : Checksum Offload.  
	 *	Fold carry bits back into sum 
	 */
	CKO_CARRY(m, fp->ipq_sum);
	remque(fp);
	(void) m_free(dtom(fp));
	MIB_ipIncrCounter(ID_ipReasmOKs);
	return ((struct ip *)ip);

dropfrag:
	ipstat.ips_fragdropped++;
	m_freem(m);
	return (0);

checkmem:
	/*
	 * HP : IP Reassembly : Account for new frag memory usage.
	 *
	 * At this point, "ip" and "fp" are still the same values as input
	 * to ip_reass, and "m" is still dtom(ip).
	 */
	if (netmemmax <= 0) {
		/*
		 * Not accounting for IP frag memory usage now.  We must
		 * set m_quad[1] to zero to ensure correctness in case
		 * netmemmax is changed with adb later (!).
		 *
		 * Note:  This is an optimization to avoid overhead in this
		 * case.  However, if netmemmax is changed later, our
		 * accounting of memory and the LRU order will be "off".
		 * This is not critical as long as m_quad[1] is valid.  We
		 * don't worry about this accounting "error" detail because
		 * we don't expect netmemmax to be changed dynamically.
		 */
		m->m_quad[1] = 0;
	}
	else {
	        /*
		 * Move this reassembly queue to the head of list of reass
		 * queues.  Thus, least-recently updated queues migrate to
		 * to the end of list.  (See ip_freeq condition below.)
	         */
		remque(fp);
		insque(fp, &ipq);
	
	        /*
	         * Increment local and global memory count.
		 * newcnt = 0 or MSIZE, depending on if this is first frag.
		 * oldcnt > 0 if frags dropped due to overlapping segments.
		 *
		 * Save ip_count_mem for new frag in m_quad[1] so that we
		 * can compute reduction efficiently later if the frag is
		 * dropped due to overlapping segments.
	         */
		m->m_quad[1] = ip_count_mem(m);  /* frag memory */
                newcnt += m->m_quad[1];

	        (dtom(fp))->m_quad[1] += newcnt - oldcnt;  /* queue memory */
	        ipq_msize             += newcnt - oldcnt;
	
	        /*
	         * Check if fragments have to be dropped from reassembly queue
	         * when mem locked in reassembly queue exceeds our threshold.
		 * If so, we drop the least-recently updated frag, which has
		 * migrated to the end of the ipq.
		 *
		 * Dropping the LRU frag tends to randomize the hit among
		 * slow and fast links, although queues for fast links will
		 * tend to migrate to the front of the queue.
	         */
	        if (ipq_msize > netmemmax)
	           ip_freef(ipq.prev);
        }

        return(0);
}
/*
 * Free a fragment reassembly header and all
 * associated datagrams.
 */
ip_freef(fp)
	struct ipq *fp;
{
	register struct ipasfrag *q, *p;
        /*
         * HP : IP Reassembly : Decrement global mem count
         */
        ipq_msize -= (dtom(fp))->m_quad[1];
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = p) {
		p = q->ipf_next;
		ip_deq(q);
		m_freem(dtom(q));
	}
	remque(fp);
	(void) m_free(dtom(fp));
}

/*
 * Put an ip fragment on a reassembly chain.
 * Like insque, but pointers in middle of structure.
 */
ip_enq(p, prev)
	register struct ipasfrag *p, *prev;
{

	p->ipf_prev = prev;
	p->ipf_next = prev->ipf_next;
	prev->ipf_next->ipf_prev = p;
	prev->ipf_next = p;
}

/*
 * To ip_enq as remque is to insque.
 */
ip_deq(p)
	register struct ipasfrag *p;
{

	p->ipf_prev->ipf_next = p->ipf_next;
	p->ipf_next->ipf_prev = p->ipf_prev;
}

/*
 * IP timer processing;
 * if a timer expires on a reassembly
 * queue, discard it.
 */
ip_slowtimo()
{
	register struct ipq *fp;
	int s;

	NET_SPLNET(s);

	fp = ipq.next;
	if (fp == 0) {
		NET_SPLX(s);
		return;
	}
	while (fp != &ipq) {
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
			ipstat.ips_fragtimeout++;
			ip_freef(fp->prev);
		}
	}
	NET_SPLX(s);
}

/*
 * Drain off all datagram fragments.
 */
ip_drain()
{

	while (ipq.next != &ipq) {
		ipstat.ips_fragdropped++;
		ip_freef(ipq.next);
	}
}

extern struct in_ifaddr *ifptoia();
struct in_ifaddr *ip_rtaddr();

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options
 * are encountered.
 */
ip_dooptions(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register u_char *cp;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB;
	register struct ip_timestamp *ipt;
	register struct in_ifaddr *ia;
	struct in_addr *sin;
	n_time ntime;

	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			ipaddrs.sin_addr = ip->ip_dst;
			ia = (struct in_ifaddr *)
				ifa_ifwithaddr((struct sockaddr *)&ipaddrs);
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
 			if (off > optlen - sizeof(struct in_addr)) { 
				/*
				 * End of source route.  Should be for us.
				 */
				save_rte(cp, ip->ip_src);
				break;
			} 
			/*
			 * locate outgoing interface
			 */
			bcopy((caddr_t)(cp + off), (caddr_t)&ipaddrs.sin_addr,
			    sizeof(ipaddrs.sin_addr));
			if ((opt == IPOPT_SSRR &&
			    in_iaonnetof(in_netof(ipaddrs.sin_addr)) == 0) ||
			    (ia = ip_rtaddr(ipaddrs.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = ipaddrs.sin_addr;
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr))
				break;
			bcopy((caddr_t)(&ip->ip_dst), (caddr_t)&ipaddrs.sin_addr,
			    sizeof(ipaddrs.sin_addr));
			/*
			 * locate outgoing interface
			 */
			if ((ia = ip_rtaddr(ipaddrs.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if ((ipt->ipt_len < 5) || (ipt->ipt_ptr < 5))
				goto bad;
			if (ipt->ipt_ptr - 1 > ipt->ipt_len - sizeof (long)) {
				if (++ipt->ipt_oflw == 0)
					goto bad;
				break;
			}
			sin = (struct in_addr *)(cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				ia = ifptoia(ifp);
				bcopy((caddr_t)&IA_SIN(ia)->sin_addr,
				    (caddr_t)sin, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				bcopy((caddr_t)sin, (caddr_t)&ipaddrs.sin_addr,
				    sizeof(struct in_addr));
				if (ifa_ifwithaddr((struct sockaddr *)&ipaddrs) == 0)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = iptime();
			bcopy((caddr_t)&ntime, (caddr_t)cp + ipt->ipt_ptr - 1,
			    sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	return (0);
bad:
	/* icmp_error() expects that ip_len does not include header */
	ip->ip_len -= (short)(ip->ip_hl <<2);
	icmp_error(ip, type, code, ifp);
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(dst)
	 struct in_addr dst;
{
	register struct sockaddr_in *sin;
	register struct in_ifaddr *ia;

	sin = (struct sockaddr_in *) &ipforward_rt.ro_dst;

	if (ipforward_rt.ro_rt == 0 || dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = dst;

		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == 0)
		return ((struct in_ifaddr *)0);
	/*
	 * Find address associated with outgoing interface.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_ifp == ipforward_rt.ro_rt->rt_ifp)
			break;
	return (ia);
}

/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
save_rte(option, dst)
	u_char *option;
	struct in_addr dst;
{
	unsigned olen;
	extern ipprintfs;

	olen = option[IPOPT_OLEN];
	if (olen > sizeof(ip_srcrt) - 1) {
		if (ipprintfs)
			printf("save_rte: olen %d\n", olen);
		return;
	}
	bcopy((caddr_t)option, (caddr_t)ip_srcrt.srcopt, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.route[ip_nhops++] = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute()
{
	register struct in_addr *p, *q;
	register struct mbuf *m;

	if (ip_nhops == 0)
		return ((struct mbuf *)0);
	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == 0)
		return ((struct mbuf *)0);
	m->m_len = ip_nhops * sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1;

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	bcopy((caddr_t)&ip_srcrt, mtod(m, caddr_t) + sizeof(struct in_addr),
	    IPOPT_OFFSET + 1 + 1);
	q = (struct in_addr *)(mtod(m, caddr_t) +
	    sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1);
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route)
		*q++ = *p--;
	return (m);
}

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 */
ip_stripoptions(ip, mopt)
	struct ip *ip;
	struct mbuf *mopt;
{
	register int i;
	register struct mbuf *m;
	register caddr_t opts;
	int olen, optsoff = 0;

	olen = (ip->ip_hl<<2) - sizeof (struct ip);
	m = dtom(ip);
	opts = (caddr_t)(ip + 1);
	if (mopt) {
		/*
		 * If m_len is 0, we're dealing with an option set which 
		 * ip_srcroute found no source routing in. So, we've got 
		 * an empty mbuf, into the beginning of which we have to 
		 * coerce a "first hop" address.  In a packet with no source 
		 * routing, this would be the destination address.  Otherwise, 
		 * m_len is real, and we're just appending to the mbuf coming 
		 * out of ip_srcroute.
		 */
		if (!mopt->m_len) {
			mopt->m_len = sizeof(struct in_addr);
			bcopy(&ip->ip_dst, mtod(mopt, caddr_t), mopt->m_len);
		}
		/*
		 * Push the rest of the options in. We don't have to worry 
		 * about the other IP level options like we do the source 
		 * routing, so just search for them and insert them into the 
		 * mbuf.  Notice that anything dealing with source routing is 
		 * ignored, since you would want to do that in ip_srcroute 
		 * instead.
		 */
		while (optsoff + 1 <= olen)
			switch(opts[optsoff]) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			case IPOPT_EOL:
				mopt->m_dat[mopt->m_len++] = opts[optsoff++];
				/*
				 * HP PATCH 920121
				 * copy padding up to "olen" bytes.  otherwise, 
				 * ip_insertoptions can misalign IP packet
				 * later, causing problems when ip_dst is 
				 * referenced in ip_output.  also, this is
				 * consistent with preconditions enforced by
				 * setopt(IP_OPTIONS), namely that options
				 * (olen) should be a multiple of 4.  thus,
				 * copying padding should be less risky.
				 *
				 * (we ass-u-me that olen is a multiple of 4
				 * per RFC.)
				 *
				 * NOTE: if we are concerned about non-zero
				 * octets after EOL, we could pad with zeros.
				 * however, copying padding preserves all post
				 * conditions, namely that optsoff is properly
				 * set to last+1 byte.  good habit, altho that
				 * seems unnecessary in the current coding.
				 */
				while (optsoff < olen)
				   mopt->m_dat[mopt->m_len++] = opts[optsoff++];
				goto endopt;
			case IPOPT_NOP:
				mopt->m_dat[mopt->m_len++] = opts[optsoff++];
				break;
			default:
				bcopy(&opts[optsoff], &mopt->m_dat[mopt->m_len],
				    opts[optsoff + IPOPT_OLEN]);
				mopt->m_len += opts[optsoff + IPOPT_OLEN];
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			}
endopt:
		mopt->m_off = MMINOFF;
	}
	i = m->m_len - (sizeof (struct ip) + olen);
	bcopy(opts  + olen, opts, (unsigned)i);
	m->m_len -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding (possibly because we have only a single external
 * network), just drop the packet.  This could be confusing if ipforwarding
 * was zero but some routing protocol was advancing us as a gateway
 * to somewhere.  However, we must let the routing protocol deal with that.
 */
ip_forward(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register int error, type = 0, code;
	register struct sockaddr_in *sin;
	struct mbuf *mcopy;
	struct in_addr dest;

	dest.s_addr = 0;
	if (ipprintfs)
		printf("forward: src %x dst %x ttl %x\n", ip->ip_src,
			ip->ip_dst, ip->ip_ttl);
	ip->ip_id = htons(ip->ip_id);
	if (ipforwarding == 0 || in_interfaces <= 1) {
		ipstat.ips_cantforward++;
		MIB_ipIncrCounter(ID_ipInAddrErrors);
#ifdef GATEWAY
		type = ICMP_UNREACH, code = ICMP_UNREACH_NET;
		goto sendicmp;
#else
		m_freem(dtom(ip));
		return;
#endif
	}
	if (in_canforward(ip->ip_dst) == 0) {
		MIB_ipIncrCounter(ID_ipInAddrErrors);
		m_freem(dtom(ip));
		return;
	}
	if (ip->ip_ttl <= IPTTLDEC) {
		MIB_ipIncrCounter(ID_ipInAddrErrors);
		type = ICMP_TIMXCEED, code = ICMP_TIMXCEED_INTRANS;
		goto sendicmp;
	}
	ip->ip_ttl -= IPTTLDEC;

	/*
	 * Save at most 64 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 */
	mcopy = m_copy(dtom(ip), 0, imin((int)ip->ip_len, 64));

	sin = (struct sockaddr_in *)&ipforward_rt.ro_dst;
	if (ipforward_rt.ro_rt == 0 ||
	    ip->ip_dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = ip->ip_dst;

		rtalloc(&ipforward_rt);
	}
	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modfied by a redirect.
	 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
	if (ipforward_rt.ro_rt && ipforward_rt.ro_rt->rt_ifp == ifp &&
	    (ipforward_rt.ro_rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0 &&
	    satosin(&ipforward_rt.ro_rt->rt_dst)->sin_addr.s_addr != 0 &&
	    ipsendredirects && ip->ip_hl == (sizeof(struct ip) >> 2)) {
		struct in_ifaddr *ia;
		u_long src = ntohl(ip->ip_src.s_addr);
		u_long dst = ntohl(ip->ip_dst.s_addr);

		if ((ia = ifptoia(ifp)) &&
		   (src & ia->ia_subnetmask) == ia->ia_subnet) {
		    if (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY)
			dest = satosin(&ipforward_rt.ro_rt->rt_gateway)->sin_addr;
		    else
			dest = ip->ip_dst;
		    /*
		     * If the destination is reached by a route to host,
		     * is on a subnet of a local net, or is directly
		     * on the attached net (!), use host redirect.
		     * (We may be the correct first hop for other subnets.)
		     */
		    type = ICMP_REDIRECT;
		    code = ICMP_REDIRECT_NET;
		    if ((ipforward_rt.ro_rt->rt_flags & RTF_HOST) ||
		       (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY) == 0)
			code = ICMP_REDIRECT_HOST;
		    else for (ia = in_ifaddr; ia = ia->ia_next; )
			if ((dst & ia->ia_netmask) == ia->ia_net) {
			    if (ia->ia_subnetmask != ia->ia_netmask)
				    code = ICMP_REDIRECT_HOST;
			    break;
			}
		    if (ipprintfs)
		        printf("redirect (%d) to %x\n", code, dest);
		}
	}

	error = ip_output(dtom(ip), (struct mbuf *)0, &ipforward_rt,
		IP_FORWARDING);
	if (error)
		ipstat.ips_cantforward++;
	else if (type)
		ipstat.ips_redirectsent++;
	else {
		if (mcopy)
			m_freem(mcopy);
		ipstat.ips_forward++;
		MIB_ipIncrCounter(ID_ipForwDatagrams);
		return;
	}
	if (mcopy == NULL)
		return;
	ip = mtod(mcopy, struct ip *);
	type = ICMP_UNREACH;
	switch (error) {

	case 0:				/* forwarded, but need redirect */
		type = ICMP_REDIRECT;
		/* code set above */
		break;

	case ENETUNREACH:
	case ENETDOWN:
		if (in_localaddr(ip->ip_dst))
			code = ICMP_UNREACH_HOST;
		else
			code = ICMP_UNREACH_NET;
		break;

	case EMSGSIZE:
		code = ICMP_UNREACH_NEEDFRAG;
		break;

	case EPERM:
		code = ICMP_UNREACH_PORT;
		break;

	case ENOBUFS:
		type = ICMP_SOURCEQUENCH;
		break;

	case EHOSTDOWN:
	case EHOSTUNREACH:
		code = ICMP_UNREACH_HOST;
		break;
	}
sendicmp:
	/* icmp_error() expects that ip_len does not include header */
	ip->ip_len -= (short)(ip->ip_hl <<2);
	icmp_error(ip, type, code, ifp, dest);
}
