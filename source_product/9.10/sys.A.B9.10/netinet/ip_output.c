/*
 * $Header: ip_output.c,v 1.34.83.5 93/12/09 10:35:51 marshall Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/ip_output.c,v $
 * $Revision: 1.34.83.5 $		$Author: marshall $
 * $State: Exp $		$Locker:  $
 * $Date: 93/12/09 10:35:51 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) ip_output.c $Revision: 1.34.83.5 $ $Date: 93/12/09 10:35:51 $";
#endif

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)ip_output.c	7.13 (Berkeley) 6/29/88 plus MULTICAST 1.2
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/cko.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"

extern copy_fixup();
struct mbuf *ip_insertoptions();
u_char ipDefaultTTL = MAXTTL;

/* flags added for nike testing */

/*
 * IP output.  The packet in mbuf chain m contains a skeletal IP
 * header (with len, off, ttl, proto, tos, src, dst).
 * The mbuf chain containing the packet will be freed.
 * The mbuf opt, if present, will not be freed.
 */
#ifdef MULTICAST
ip_output(m0, opt, ro, flags, mopts)
#else
ip_output(m0, opt, ro, flags)
#endif MULTICAST
	struct mbuf *m0;
	struct mbuf *opt;
	struct route *ro;
	int flags;
#ifdef MULTICAST
	struct mbuf *mopts;
#endif MULTICAST
{
	register struct ip *ip;
	register struct ifnet *ifp;
	register struct mbuf *m = m0;
	register int hlen = sizeof (struct ip);
	int len, error = 0;
	struct route iproute;
	struct sockaddr_in *dst;
	u_long roflags;
	struct socket *so;

	MIB_ipIncrCounter(ID_ipOutRequests);
	if (opt) {
		m = ip_insertoptions(m, opt, &len);
		hlen = len;
	}
	ip = mtod(m, struct ip *);
	/*
	 * Fill in IP header.
	 */
	if ((flags & IP_FORWARDING) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
		ip->ip_id = htons(ip_id++);
		ip->ip_hl = hlen >> 2;
	} else
		hlen = ip->ip_hl << 2;

	/*
	 * Route packet.
	 */
	if (ro == 0) {
		ro = &iproute;
		bzero((caddr_t)ro, sizeof (*ro));
	}
	dst = (struct sockaddr_in *)&ro->ro_dst;
	/*
	 * If there is a cached route,
	 * check that it is to the same destination
	 * and is still up.  If not, free it and try again.
	 */
	if (ro->ro_rt && ((ro->ro_rt->rt_flags & RTF_UP) == 0 ||
	   dst->sin_addr.s_addr != ip->ip_dst.s_addr)) {
		RTFREE(ro->ro_rt);
		ro->ro_rt = (struct rtentry *)0;
	}
	if (ro->ro_rt == 0) {
		dst->sin_family = AF_INET;
		dst->sin_addr = ip->ip_dst;
	}
	/*
	 * If routing to interface only,
	 * short circuit routing lookup.
	 */
	if (flags & IP_ROUTETOIF) {
		struct in_ifaddr *ia;

		ia = (struct in_ifaddr *)ifa_ifwithdstaddr((struct sockaddr *)dst);
		if (ia == 0)
			ia = in_iaonnetof(in_netof(ip->ip_dst));
		if (ia == 0) {
			error = ENETUNREACH;
			MIB_ipIncrCounter(ID_ipOutNoRoutes);
			goto bad;
		}
		ifp = ia->ia_ifp;
	} else {
		if (ro->ro_rt == 0) {
		    /*
		     * NIKE 
		     * We need to get a new route, so we must re-determine if
		     * we can do copy avoidance based on the new interface.
		     * If we were previously doing copy on write or checksum
		     * assist and now we're not, we must fixup the data.
		     */	

		    rtalloc(ro);
		    if ((ro->ro_rt) && (ro->ro_rt->rt_ifp)) {
			if (so = ro->ro_socket) {
			    if ((so->so_proto->pr_flags & PR_COPYAVOID) &&
			        ((*so->so_proto->pr_usrreq)(so, PRU_COPYAVOID, 0, 0, 0) == 0))
				so->so_snd.sb_flags |= SB_COPYAVOID_SUPP;
			    else if (so->so_snd.sb_flags & SB_COPYAVOID_SUPP) {
				if (error = copy_fixup(m, so->so_snd)) {
					/* if the copy failed we nuke the   */
					/* route we just got so as to force */
					/* us to try the copy_fixup() again */
					/* on the next send.		    */
					RTFREE(ro->ro_rt);
					ro->ro_rt = 0;
					ro->ro_flags = 0;
					goto bad;
				}						
				so->so_snd.sb_flags &= ~SB_COPYAVOID_SUPP;
			    }
			}
			if ((m->m_flags & MF_CKO_OUT) &&
			    ((ro->ro_flags & RF_IFCKO) == 0) && 
			    (error = cko_fixup(m)))
				goto bad;
		    }
		}

		if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
			MIB_ipIncrCounter(ID_ipOutNoRoutes);
			if (in_localaddr(ip->ip_dst))
				error = EHOSTUNREACH;
			else
				error = ENETUNREACH;
			goto bad;
		}
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = (struct sockaddr_in *)&ro->ro_rt->rt_gateway;
	}

#ifdef MULTICAST
	if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr))) {
		struct ip_moptions *imo;
		struct in_multi *inm;
		extern struct ifnet loif;
		extern struct socket *ip_mrouter;
		/*
		 * IP destination address is multicast.  Make sure "dst"
		 * still points to the address in "ro".  (It may have been
		 * changed to point to a gateway address, above.)
		 */
		dst = (struct sockaddr_in *)&ro->ro_dst;
		/*
		 * See if the caller provided any multicast options
		 */
		if ((flags & IP_MULTICASTOPTS) && mopts != NULL) {
			imo = mtod(mopts, struct ip_moptions *);
			ip->ip_ttl = imo->imo_multicast_ttl;
			if (imo->imo_multicast_ifp != NULL)
				ifp = imo->imo_multicast_ifp;
		}
		else {
			imo = NULL;
			ip->ip_ttl = IP_DEFAULT_MULTICAST_TTL;
		}
		/*
		 * Confirm that the outgoing interface supports multicast.
		 */
		if ((ifp->if_flags & IFF_MULTICAST) == 0) {
			error = ENETUNREACH;
			goto bad;
		}
		/*
		 * If source address not specified yet, use address
		 * of outgoing interface.
		 */
		if (ip->ip_src.s_addr == INADDR_ANY) {
			register struct in_ifaddr *ia;

			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp) {
					ip->ip_src = IA_SIN(ia)->sin_addr;
					break;
				}
		}

		IN_LOOKUP_MULTI(ip->ip_dst, ifp, inm);
		if (inm != NULL &&
		   (imo == NULL || imo->imo_multicast_loop)) {
			/*
			 * If we belong to the destination multicast group
			 * on the outgoing interface, and the caller did not
			 * forbid loopback, loop back a copy.
			 */
			ip_mloopback(ifp, m, dst);
		}
		else if (ip_mrouter && (flags & IP_FORWARDING) == 0) {
			/*
			 * If we are acting as a multicast router, perform
			 * multicast forwarding as if the packet had just
			 * arrived on the interface to which we are about
			 * to send.  The multicast forwarding function
			 * recursively calls this function, using the
			 * IP_FORWARDING flag to prevent infinite recursion.
			 *
			 * Multicasts that are looped back by ip_mloopback(),
			 * above, will be forwarded by the ip_input() routine,
			 * if necessary.
			 */
			if (ip_mforward(ip, ifp) != 0) {
				m_freem(m);
				goto done;
			}
		}
		/*
		 * Multicasts with a time-to-live of zero may be looped-
		 * back, above, but must not be transmitted on a network.
		 * Also, multicasts addressed to the loopback interface
		 * are not sent -- the above call to ip_mloopback() will
		 * loop back a copy if this host actually belongs to the
		 * destination group on the loopback interface.
		 */
		if (ip->ip_ttl == 0 || ifp == &loif) {
			m_freem(m);
			goto done;
		}

		goto sendit;
	}
#endif MULTICAST

#ifndef notdef
	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (ip->ip_src.s_addr == INADDR_ANY) {
		register struct in_ifaddr *ia;

		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp) {
				ip->ip_src = IA_SIN(ia)->sin_addr;
				break;
			}
	}
#endif
	/*
	 * Look for broadcast address and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(dst->sin_addr)) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			MIB_ipIncrCounter(ID_ipOutDiscards);
			goto bad;
		}
		if ((flags & IP_ALLOWBROADCAST) == 0) {
			error = EACCES;
			MIB_ipIncrCounter(ID_ipOutDiscards);
			goto bad;
		}
		/* don't allow broadcast messages to be fragmented */
		if (ip->ip_len > ifp->if_mtu) {
			error = EMSGSIZE;
			MIB_ipIncrCounter(ID_ipOutDiscards);
			goto bad;
		}
	}

#ifdef MULTICAST
sendit:
#endif MULTICAST
	/*
	 *  If small enough for interface, can just send directly.
	 */
	m0 = m;	/* HP : m0 refs first frag (does not change) */
	if (ip->ip_len <= ifp->if_mtu)
		goto output;
	/*
	 *  Too large for interface; is it okay to fragment?
	 *  Must be able to put at least 8 bytes per fragment.
	 */
	if (ip->ip_off & IP_DF)
		goto msgsiz;
	len = (ifp->if_mtu - hlen) & ~7;
	if (len <= 0)
		goto msgsiz;

    {	/*  Fragmentation code
 	 * 
	 *     ip_output() attempts to minimize the times it needs to split
	 *  an mbuf or cluser when fragmenting a large datagram.  Given the 
	 *  assumption that network interface hardware can generally dma from 
	 *  data that begins at the top of a mbuf or cluster, but might have 
	 *  problems with data that begins at some arbitrary alignment within 
	 *  a mbuf or cluster, the flexible fragmentation policy codified below
	 *  should greatly reduce the amount and frequency of data movement by 
	 *  network interface drivers to meet their hardware dma alignment 
	 *  requirements.
	 *     ip_output() determines a minimum and maximum fragment size.  
	 *  For MTUs <= MCLBYTES, the minimum and maximum equal the MTU; but 
	 *  for MTUs > MCLBYTES, the minimum equals the greatest multiple of 
	 *  MCLBYTES < the MTU (just like the TCP MSS calculation).  The
	 *  minimum is represented by a field called maxresid, which equals 
	 *  the maximum - the mimimum fragment size.  
	 *     the new fragmentation code always attempts to fragment to the 
	 *  maximum size; however, if a maximum fragment requires an mbuf or 
	 *  cluster to be split, ip tests to see if not including the needs-
	 *  to-be-split mbuf or cluster reduces the fragment's size below the 
	 *  minimum.  If not then the needs-to-be-split mbuf or cluster is not 
	 *  included in the fragment.  A new mbuf utility, m_frag(), performs 
	 *  this flexible fragmentation.
	 */
	struct mbuf **mactp; /* addr of previous frag's m_act field */
	struct mbuf *mh; /* fragment header mbuf */
	struct ip *mhip; /* fragment header */
	int mhoff; /* offset - ie, mhip->ip_off */
	int mhlen; /* header length - ie, mhip->ip_hl and mh->m_len */
	int mdlen; /* data length - ie, mhip->ip_len = mhlen + mdlen */
	int ipoff; /* frag offset in bytes */
	int resid; /* m_frag() parm (see m_frag source) */
	int maxresid; /* m_frag() parm (see m_frag src) */
	int cko_cntl; /* cksum offload control */
	/*
	 *  Setup - initialize mactp, cko_cntl, maxresid.
	 */
	mactp = &m0->m_act;
	if (m0->m_flags & MF_CKO_OUT)
		cko_cntl =  /* HP : checksum offload control */
		    ((struct cko_info *)&(m->m_quad[MQ_CKO_OUT0]))->cko_type &
			~(CKO_INSERT);
	else
		cko_cntl = 0;
	if (len > MCLBYTES) /* HP : allow fragments less than if_mtu */
#if (MCLBYTES & (MCLBYTES - 1)) == 0
		maxresid = len & (MCLBYTES-1);
#else
		maxresid = len % MCLBYTES;
#endif
	else /* HP : when len < MCLBYTES all fragments equal if_mtu */
		maxresid = 0;
	/*
	 *  Cut off first fragment: adjust its ip length accordingly;
	 */
	resid = hlen + len;
	m = m_frag( m0, &resid, maxresid, 7 );
	if (m == (struct mbuf *)-1)
		goto nobufs;
	*mactp = m;  /* *mactp will be overwritten on first pass of loop */
	mdlen = len - resid;
	ipoff = mdlen;
	if (cko_cntl) {	 /* HP : checksum offload assist */
		((struct cko_info *)(&(m0->m_quad[MQ_CKO_OUT0])))->cko_stop -=
			(ip->ip_len - hlen - mdlen);
	}
	ip->ip_len = hlen + mdlen;
	/*
	 *  Loop through remaining portion of ip datagram: each pass cuts 
	 *  off a fragment until datagram is consumed (ie, m_frag returns 0).
	 */
	while (m) {
		/* 
		 *  Get mbuf for header of next fragment and attach next
		 *  fragment to its m_next field.  Attach the remaining
		 *  portion of datagram to its m_act field.
		 */
		MGET(mh, M_DONTWAIT, MT_HEADER);
		if (mh == 0)
			goto nobufs;
		mh->m_next = m;
		*mactp = mh;
		mactp = &mh->m_act;
		resid = len;
		m = m_frag( m, &resid, maxresid, 7 );
		if (m == (struct mbuf *)-1)
			goto nobufs;
		*mactp = m;  /* *mactp will be overwritten on next pass */
		mdlen = len - resid;
		/*  
		 *  Create ip fragment template
		 */
		mh->m_off = MMINOFF + IN_MAXLINKHDR;
		mhip = mtod(mh, struct ip*);
		*mhip = *ip;
		/*  
		 *  Set offset
		 */
		mhoff = (ipoff >> 3) + (ip->ip_off & ~IP_MF);
		if (m || (ip->ip_off & IP_MF))
			mhoff |= IP_MF;
		mhip->ip_off = htons((u_short)mhoff);
		ipoff += mdlen;
		
		mhlen = sizeof(struct ip);
		/* set kco 
		 */
		if (cko_cntl) {	 /* HP : checksum offload assist */
			CKO_SET((struct cko_info *)&(mh->m_quad[MQ_CKO_OUT0]), 
				mhlen, mhlen+mdlen-1, 0, cko_cntl);
			mh->m_flags |= MF_CKO_OUT;
		}
		/*  
		 *  Set length
		 */
		mhip->ip_len = htons((u_short)(mhlen + mdlen));
		if (hlen > mhlen) {
			int oplen = ip_optcopy(ip, mhip);
			int iplen = ntohs((u_short)mhip->ip_len);
			mhip->ip_len = htons((u_short)(iplen + oplen));
			mhlen += oplen;
			mhip->ip_hl = mhlen >> 2;
			/* adjust kco 
		 	*/
			if (cko_cntl) {	 /* HP : checksum offload assist */
				CKO_ADJ((struct cko_info *)&(mh->m_quad[MQ_CKO_OUT0]), 
					oplen);
			}
			
		}
		mh->m_len = mhlen;
		/*  
		 *  Set checksum
		 */
		mhip->ip_sum = 0;
#ifdef __hp9000s800
		mhip->ip_sum = ip_cksum(mhip, mhlen);
#else
		mhip->ip_sum = in_cksum(mh, mhlen);
#endif /* __hp9000s800 */
		MIB_ipIncrCounter(ID_ipFragCreates);
	}
	ip->ip_off |= IP_MF;
	MIB_ipIncrCounter(ID_ipFragCreates);	/* Count 1st fragment */
	MIB_ipIncrCounter(ID_ipFragOKs);
    }	/* End of Fragmentation code */
	
output:	ip->ip_len = htons((u_short)ip->ip_len);
	ip->ip_off = htons((u_short)ip->ip_off);
	ip->ip_sum = 0;
	/*
	 *  HP : ip_cksum computes checksum for ip header 
	 *	 and is in in_cksum.s
	 */
	if(hlen <= m0->m_len)
#ifdef __hp9000s800
		ip->ip_sum = ip_cksum(ip, hlen);
#else
		ip->ip_sum = in_cksum(m0, hlen);
#endif /* __hp9000s800 */
	else
		ip->ip_sum = in_cksum(m0, hlen);
	error = (*ifp->if_output)(ifp, m0, (struct sockaddr *)dst);
done:	if (ro == &iproute && (flags & IP_ROUTETOIF) == 0 && ro->ro_rt)
		RTFREE(ro->ro_rt);
	return (error);

nobufs:	error = ENOBUFS;
	MIB_ipIncrCounter(ID_ipOutDiscards);
	m_freem_train(m0);	/* Free multi-fragment train */
	goto done;
msgsiz:	error = EMSGSIZE;
	MIB_ipIncrCounter(ID_ipFragFails);
bad:	m_freem(m0);
	goto done;
}

/*
 * Insert IP options into preformed packet.
 * Adjust IP destination as required for IP source routing,
 * as indicated by a non-zero in_addr at the start of the options.
 */
struct mbuf *
ip_insertoptions(m, opt, phlen)
	register struct mbuf *m;
	struct mbuf *opt;
	int *phlen;
{
	register struct ipoption *p = mtod(opt, struct ipoption *);
	struct mbuf *n;
	register struct ip *ip = mtod(m, struct ip *);
	unsigned optlen;

	optlen = opt->m_len - sizeof(p->ipopt_dst);
	if (p->ipopt_dst.s_addr)
		ip->ip_dst = p->ipopt_dst;
	if (m->m_off >= MMAXOFF || MMINOFF + optlen > m->m_off) {
		MGET(n, M_DONTWAIT, MT_HEADER);
		if (n == 0)
			return (m);
		m->m_len -= sizeof(struct ip);
		m->m_off += sizeof(struct ip);
		n->m_next = m;
		/* HP : Move checksum assist info into 1st mbuf */
		if (m->m_flags & MF_CKO_OUT) {
			CKO_MOVE(m,n);
		}
		m = n;
		m->m_off = MMAXOFF - sizeof(struct ip) - optlen;
		m->m_len = optlen + sizeof(struct ip);
		bcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	} else {
		m->m_off -= optlen;
		m->m_len += optlen;
		ovbcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	}
	ip = mtod(m, struct ip *);
	bcopy((caddr_t)p->ipopt_list, (caddr_t)(ip + 1), (unsigned)optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_len += optlen;
	/* 
 	 * HP : Checksum Assist info in m_quad was set up assuming no
	 *	IP options.  Adjust the checksum assist info due to IP options.
	 */
	if (m->m_flags & MF_CKO_OUT)
		CKO_ADJ((struct hw_assist *) &(m->m_quad[MQ_CKO_OUT0]),optlen);
	return (m);
}

/*
 * Copy options from ip to jp,
 * omitting those not copied during fragmentation.
 */
ip_optcopy(ip, jp)
	struct ip *ip, *jp;
{
	register u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else
			optlen = cp[IPOPT_OLEN];
		/* bogus lengths should have been caught by ip_dooptions */
		if (optlen > cnt)
			optlen = cnt;
		if (IPOPT_COPIED(opt)) {
			bcopy((caddr_t)cp, (caddr_t)dp, (unsigned)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (u_char *)(jp+1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}

/*
 * IP socket option processing.
 */
ip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);
	register struct mbuf *m0;

	/* udp is for SHRINKBUFFER from svcudp_create() NFS code */ 

	if (inp == NULL)
	     error = EINVAL;

	else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_OPTIONS:
			return (ip_pcbopts(&inp->inp_options, *m));

		case IP_TTL:
			m0 = *m;
			if (m0 == NULL || m0->m_len < sizeof (int))
				error = EINVAL;
			else 
				inp->inp_ttl = (u_char) (*mtod(m0, int *));
			break;

#ifdef MULTICAST
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_setmoptions(optname, &inp->inp_moptions, *m);
			break;
#endif MULTICAST
		default:
			error = EINVAL;
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_OPTIONS:
			*m = m_get((so->so_state & SS_NOWAIT)
			           ? M_DONTWAIT : M_WAIT, MT_SOOPTS);
			if (!*m) {
			   error = ENOBUFS;
			   break;
			}
			if (inp->inp_options) {
				(*m)->m_off = inp->inp_options->m_off;
				(*m)->m_len = inp->inp_options->m_len;
				bcopy(mtod(inp->inp_options, caddr_t),
				    mtod(*m, caddr_t), (unsigned)(*m)->m_len);
			} else
				(*m)->m_len = 0;
			break;

		case IP_TTL:
			*m = m_get((so->so_state & SS_NOWAIT)
			           ? M_DONTWAIT : M_WAIT, MT_SOOPTS);
			if (!*m) {
			   error = ENOBUFS;
			   break;
			}
			(*m)->m_len = sizeof(int);
			*mtod((*m), int *) = (int) inp->inp_ttl;
			break;

#ifdef MULTICAST
		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_getmoptions(optname, inp->inp_moptions, m);
			break;
#endif MULTICAST
		default:
			error = EINVAL;
			break;
		}
		break;

	case PRCO_SHRINKBUFFER:
                /* no checking needs to be done, datagram allows changing
                   socket buffer size at any time. */ 
                break;

        default:
                error = EINVAL;
                break;
	}
	if (op == PRCO_SETOPT && *m)
		(void)m_free(*m);
	return (error);
}

/*
 * Set up IP options in pcb for insertion in output packets.
 * Store in mbuf with pointer in pcbopt, adding pseudo-option
 * with destination address if source routed.
 */
ip_pcbopts(pcbopt, m)
	struct mbuf **pcbopt;
	register struct mbuf *m;
{
	register cnt, optlen;
	register u_char *cp;
	u_char opt;

	/* turn off any old options */
	if (*pcbopt)
		(void)m_free(*pcbopt);
	*pcbopt = 0;
	if (m == (struct mbuf *)0 || m->m_len == 0) {
		/*
		 * Only turning off any previous options.
		 */
		if (m)
			(void)m_free(m);
		return (0);
	}

	if (m->m_len % sizeof(long))
		goto bad;
	/*
	 * IP first-hop destination address will be stored before
	 * actual options; move other options back
	 * and clear it when none present.
	 */
#if	MAX_IPOPTLEN >= MMAXOFF - MMINOFF
	if (m->m_off + m->m_len + sizeof(struct in_addr) > MAX_IPOPTLEN)
		goto bad;
#else
	if (m->m_off + m->m_len + sizeof(struct in_addr) > MMAXOFF)
		goto bad;
#endif
	cnt = m->m_len;
	m->m_len += sizeof(struct in_addr);
	cp = mtod(m, u_char *) + sizeof(struct in_addr);
	ovbcopy(mtod(m, caddr_t), (caddr_t)cp, (unsigned)cnt);
	bzero(mtod(m, caddr_t), sizeof(struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= IPOPT_OLEN || optlen > cnt)
				goto bad;
		}
		switch (opt) {

		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			/*
			 * user process specifies route as:
			 *	->A->B->C->D
			 * D must be our final destination (but we can't
			 * check that since we may not have connected yet).
			 * A is first hop destination, which doesn't appear in
			 * actual IP option, but is stored before the options.
			 */
			if (optlen < IPOPT_MINOFF - 1 + sizeof(struct in_addr))
				goto bad;
			m->m_len -= sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			optlen -= sizeof(struct in_addr);
			cp[IPOPT_OLEN] = optlen;
			/*
			 * Move first hop before start of options.
			 */
			bcopy((caddr_t)&cp[IPOPT_OFFSET+1], mtod(m, caddr_t),
			    sizeof(struct in_addr));
			/*
			 * Then copy rest of options back
			 * to close up the deleted entry.
			 */
			ovbcopy((caddr_t)(&cp[IPOPT_OFFSET+1] +
			    sizeof(struct in_addr)),
			    (caddr_t)&cp[IPOPT_OFFSET+1],
			    (unsigned)cnt + sizeof(struct in_addr));
			break;
		}
	}
	*pcbopt = m;
	return (0);

bad:
	(void)m_free(m);
	return (EINVAL);
}

#ifdef MULTICAST
/*
 * Set the IP multicast options in response to user setsockopt().
 */
ip_setmoptions(optname, mopts, m)
	int optname;
	struct mbuf **mopts;
	struct mbuf *m;
{
	int error = 0;
	struct ip_moptions *imo;
	u_char loop;
	int i;
	struct in_addr addr;
	struct ip_mreq *mreq;
	struct ifnet *ifp;
	struct route ro;
	struct sockaddr_in *dst;

	if (*mopts == NULL) {
		/*
		 * No multicast option buffer attached to the pcb;
		 * allocate one and initialize to default values.
		 */
		MGET(*mopts, M_DONTWAIT, MT_IPMOPTS);
		if (*mopts == NULL)
			return (ENOBUFS);
		imo = mtod(*mopts, struct ip_moptions *);
		imo->imo_multicast_ifp   = NULL;
		imo->imo_multicast_ttl   = IP_DEFAULT_MULTICAST_TTL;
		imo->imo_multicast_loop  = IP_DEFAULT_MULTICAST_LOOP;
		imo->imo_num_memberships = 0;
	}

	imo = mtod(*mopts, struct ip_moptions *);

	switch (optname) {

	case IP_MULTICAST_IF:
		/*
		 * Select the interface for outgoing multicast packets.
		 */
		if (m == NULL || m->m_len != sizeof(struct in_addr)) {
			error = EINVAL;
			break;
		}
		addr = *(mtod(m, struct in_addr *));
		/*
		 * INADDR_ANY is used to remove a previous selection.
		 * When no interface is selected, a default one is
		 * chosen every time a multicast packet is sent.
		 */
		if (addr.s_addr == INADDR_ANY) {
			imo->imo_multicast_ifp = NULL;
			break;
		}
		/*
		 * The selected interface is identified by its local
		 * IP address.  Find the interface and confirm that
		 * it supports multicasting.
		 */
		INADDR_TO_IFP(addr, ifp);
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		imo->imo_multicast_ifp = ifp;
		break;

	case IP_MULTICAST_TTL:
		/*
		 * Set the IP time-to-live for outgoing multicast packets.
		 */
		if (m == NULL || m->m_len != 1) {
			error = EINVAL;
			break;
		}
		imo->imo_multicast_ttl = *(mtod(m, u_char *));
		break;

	case IP_MULTICAST_LOOP:
		/*
		 * Set the loopback flag for outgoing multicast packets.
		 * Must be zero or one.
		 */
		if (m == NULL || m->m_len != 1 ||
		   (loop = *(mtod(m, u_char *))) > 1) {
			error = EINVAL;
			break;
		}
		imo->imo_multicast_loop = loop;
		break;

	case IP_ADD_MEMBERSHIP:
		/*
		 * Add a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		if (m == NULL || m->m_len != sizeof(struct ip_mreq)) {
			error = EINVAL;
			break;
		}
		mreq = mtod(m, struct ip_mreq *);
		if (!IN_MULTICAST(ntohl(mreq->imr_multiaddr.s_addr))) {
			error = EINVAL;
			break;
		}
		/*
		 * If no interface address was provided, use the interface of
		 * the route to the given multicast address.
		 */
		if (mreq->imr_interface.s_addr == INADDR_ANY) {
			ro.ro_rt = NULL;
			dst = (struct sockaddr_in *)&ro.ro_dst;
			dst->sin_family = AF_INET;
			dst->sin_addr   = mreq->imr_multiaddr;
			rtalloc(&ro);
			if (ro.ro_rt == NULL) {
				error = EADDRNOTAVAIL;
				break;
			}
			ifp = ro.ro_rt->rt_ifp;
			rtfree(ro.ro_rt);
		}
		else {
			INADDR_TO_IFP(mreq->imr_interface, ifp);
		}
		/*
		 * See if we found an interface, and confirm that it
		 * supports multicast.
		 */
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		/*
		 * See if the membership already exists or if all the
		 * membership slots are full.
		 */
		for (i = 0; i < imo->imo_num_memberships; ++i) {
			if (imo->imo_membership[i]->inm_ifp == ifp &&
			    imo->imo_membership[i]->inm_addr.s_addr
						== mreq->imr_multiaddr.s_addr)
				break;
		}
		if (i < imo->imo_num_memberships) {
			error = EADDRINUSE;
			break;
		}
		if (i == IP_MAX_MEMBERSHIPS) {
			error = ETOOMANYREFS;
			break;
		}
		/*
		 * Everything looks good; add a new record to the multicast
		 * address list for the given interface.
		 */
		if ((imo->imo_membership[i] =
		    in_addmulti(mreq->imr_multiaddr, ifp)) == NULL) {
			error = ENOBUFS;
			break;
		}
		++imo->imo_num_memberships;
		break;

	case IP_DROP_MEMBERSHIP:
		/*
		 * Drop a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		if (m == NULL || m->m_len != sizeof(struct ip_mreq)) {
			error = EINVAL;
			break;
		}
		mreq = mtod(m, struct ip_mreq *);
		if (!IN_MULTICAST(ntohl(mreq->imr_multiaddr.s_addr))) {
			error = EINVAL;
			break;
		}
		/*
		 * If an interface address was specified, get a pointer
		 * to its ifnet structure.
		 */
		if (mreq->imr_interface.s_addr == INADDR_ANY)
			ifp = NULL;
		else {
			INADDR_TO_IFP(mreq->imr_interface, ifp);
			if (ifp == NULL) {
				error = EADDRNOTAVAIL;
				break;
			}
		}
		/*
		 * Find the membership in the membership array.
		 */
		for (i = 0; i < imo->imo_num_memberships; ++i) {
			if ((ifp == NULL ||
			     imo->imo_membership[i]->inm_ifp == ifp) &&
			     imo->imo_membership[i]->inm_addr.s_addr
					    == mreq->imr_multiaddr.s_addr)
				break;
		}
		if (i == imo->imo_num_memberships) {
			error = EADDRNOTAVAIL;
			break;
		}
		/*
		 * Give up the multicast address record to which the
		 * membership points.
		 */
		in_delmulti(imo->imo_membership[i]);
		/*
		 * Remove the gap in the membership array.
		 */
		for (++i; i < imo->imo_num_memberships; ++i)
			imo->imo_membership[i-1] = imo->imo_membership[i];
		--imo->imo_num_memberships;
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	/*
	 * If all options have default values, no need to keep the mbuf.
	 */
	if (imo->imo_multicast_ifp   == NULL &&
	    imo->imo_multicast_ttl   == IP_DEFAULT_MULTICAST_TTL &&
	    imo->imo_multicast_loop  == IP_DEFAULT_MULTICAST_LOOP &&
	    imo->imo_num_memberships == 0) {
		m_free(*mopts);
		*mopts = NULL;
	}

	return(error);
}

/*
 * Return the IP multicast options in response to user getsockopt().
 */
ip_getmoptions(optname, mopts, m)
	int optname;
	struct mbuf *mopts;
	struct mbuf **m;
{
	u_char *ttl;
	u_char *loop;
	struct in_addr *addr;
	struct ip_moptions *imo;
	struct in_ifaddr *ia;

	*m = m_get(M_WAIT, MT_IPMOPTS);

	imo = (mopts == NULL) ? NULL : mtod(mopts, struct ip_moptions *);

	switch (optname) {

	case IP_MULTICAST_IF:
		addr = mtod(*m, struct in_addr *);
		(*m)->m_len = sizeof(struct in_addr);
		if (imo == NULL || imo->imo_multicast_ifp == NULL)
			addr->s_addr = INADDR_ANY;
		else {
			IFP_TO_IA(imo->imo_multicast_ifp, ia);
			addr->s_addr = (ia == NULL) ? INADDR_ANY
					: IA_SIN(ia)->sin_addr.s_addr;
		}
		return(0);

	case IP_MULTICAST_TTL:
		ttl = mtod(*m, u_char *);
		(*m)->m_len = 1;
		*ttl = (imo == NULL) ? IP_DEFAULT_MULTICAST_TTL
				     : imo->imo_multicast_ttl;
		return(0);

	case IP_MULTICAST_LOOP:
		loop = mtod(*m, u_char *);
		(*m)->m_len = 1;
		*loop = (imo == NULL) ? IP_DEFAULT_MULTICAST_LOOP
				      : imo->imo_multicast_loop;
		return(0);

	default:
		return(EOPNOTSUPP);
	}
}

/*
 * Discard the IP multicast options.
 */
ip_freemoptions(mopts)
	struct mbuf *mopts;
{
	struct ip_moptions *imo;
	int i;

	if (mopts != NULL) {
		imo = mtod(mopts, struct ip_moptions *);
		for (i = 0; i < imo->imo_num_memberships; ++i)
			in_delmulti(imo->imo_membership[i]);
		m_free(mopts);
	}
}

/*
 * Routine called from ip_output() to loop back a copy of an IP multicast
 * packet to the input queue of a specified interface.  Note that this
 * calls the output routine of the loopback "driver", but with an interface
 * pointer that might NOT be &loif -- easier than replicating that code here.
 */
ip_mloopback(ifp, m, dst)
	struct ifnet *ifp;
	register struct mbuf *m;
	register struct sockaddr_in *dst;
{
	register struct ip *ip;
	struct mbuf *copym;

	copym = m_copy(m, 0, M_COPYALL);
	if (copym != NULL) {
		/*
		 * We don't bother to fragment if the IP length is greater
		 * than the interface's MTU.  Can this possibly matter?
		 */
		ip = mtod(copym, struct ip *);
		ip->ip_len = htons((u_short)ip->ip_len);
		ip->ip_off = htons((u_short)ip->ip_off);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(copym, ip->ip_hl << 2);
		(void) looutput(ifp, copym, (struct sockaddr *)dst);
	}
}

#endif MULTICAST
