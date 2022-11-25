/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/kudp_fsend.c,v $
 * $Revision: 12.0 $	$Author: nfsmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/09/25 16:08:40 $
 */

/* BEGIN_IMS_MOD  
******************************************************************************
****								
****		kudp_fsend - kernel UDP/IP fastsend path used by NFS
****								
******************************************************************************
* Description
*
* Externally Callable Routine
*	ku_fastsend	- send a UDP/IP packet the "fast" way
*
* Internal Routines
*	buffree		- noop to call when special mbuf is freed
*	pr_mbuf		- print mbuf chain, including IP header
*
* Test Module
*	$SCAFFOLD/nfs/*
*
* To Do List
*
* Notes
*	The fastsend path bypasses the protocol stack and calls the lan driver
*	directly. It would be desirable from the point of view of software
*	maintenance and extensibility to eliminate the fastpath altogether.
*	This would eliminate the need to maintain two functionally equivalent 
*	paths. This module is sensitive to changes in the routing and IP code.
*	The one rub is performance: the fastpath should not be removed until
*	improvements in the IP code close the current ~10% performance gap.
*
* Modification History
*	11/24/87	lmb		added global kudp_checksumming
*	02/26/88	mds		modified global kudp_checksumming
*					so it optionally be done regardless
*					of whether routing is used
*	06/06/88	arm		initialized nextip; made changes so that
*					request for mbufs for both ip hdr and 
*					type 2 mbuf are made before shuffling 
*					pointers. 
*					( bug fix - nfsd hang DTS INDaa02735 )
*
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: kudp_fsend.c,v 12.0 89/09/25 16:08:40 nfsmgr Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY.
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/file.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/udp.h"
#include "../h/ns_diag.h"
#include "../h/trigdef.h"

/*
 * This is a flag that can be changed with adb to force checksumming
 * when going through a gateway. It is a non-supported, non-documented
 * option.
 * If its value is 2, then checksumming will be done regardless of
 * whether you are going through a gateway or not.  This is because
 * you can set up a CISCO gateway without using routing.
 */
int kudp_checksumming = 0;

/* BEGIN_IMS buffree *
 ********************************************************************
 ****
 ****			buffree()
 ****
 ********************************************************************
 *
 * Description
 *	This is a null procedure used for a special MF_LARGEBUF mbuf.
 *	It is necessary to have some routine to call when this kind
 *	of mbuf (obtained by mclgetx() is freed.
 *
 ********************************************************************
 * END_IMS buffree */

static
buffree()
{
}


/* BEGIN_IMS ku_fastsend *
 ********************************************************************
 ****
 ****			ku_fastsend(so, am, to)
 ****
 ********************************************************************
 * Input Parameters
 *	so	socket to use in sending packet
 *	am	mbuf chain containing data to send
 *	to	destination address 
 *
 * Output Parameters
 * 	none
 *
 * Return Value
 *	 0  	send ok
 *	-1	send failed: mbuf chain freed
 *	-2  	send failed: mbuf chain intact 
 *
 * Globals Referenced
 * 	none
 *
 * Description
 *  	Take the mbuf chain at am, add ip/udp headers, and fragment it
 *  	for the interface output routine.  Once a chain is sent to the
 *  	interface, it is freed upon return.  It is possible for one
 *  	fragment to succeed, and another to fail.  If this happens
 *  	we must free the remaining mbuf chain (at am).  The caller
 *  	must assume that the entire send failed if one fragment failed.
 *  	If we get an error before a fregment is sent, then the original
 *  	chain is intact and the caller may take other action.
 *
 * Algorithm
 *	get the total length of the data to send
 *	get routing information (rtalloc)
 *	assign the destination address based on route
 *	get mbuf for IP, UDP headers
 *	if (mbuf == NULL)
 *		return (-2)
 *	create and initialize IP header
 *	if (socket not bound to address)
 *		bind socket with unused port number
 *	create and initialize UDP header
 *	get the maximum transmission unit for interface
 *	for (each IP fragment) {
 *		while (current length <= maximum length) {
 *			get data from next mbuf
 *			if ( no more mbufs )
 *				goto send;
 *		}
 *		get a new mbuf to make a copy of the ip hdr
 *		if (mbuf == NULL) {
 *			if ( a fragment was sent )
 *				free mbuf chain
 *				return(-1)
 *			else {
 *				free ip hdr mbuf
 *				return (-2)
 *			     }
 *		}
 *		if (some data in the next mbuf can fit in packet) {
 *			get a new type 2 mbuf
 *			if (mbuf == NULL) {
 *				if (a fragment was sent)
 *					return (-1)
 *				else
 *					return (-2)
 *			}
 *			point mbuf at remaining data 
 *		}
 * send:
 *		calculate the fragment offset
 *		get mbuf to hold a copy of IP header for next fragment
 *		calculate IP length and checksum
 *		call the interface routine to send the mbuf chain
 *		if (error in sending fragment) {
 *			free any remaining mbufs
 *			return (-1)
 *		}
 *		if (no more data to send)
 *			return (0)
 *		record the fact that one fragment has been sent
 *		update the IP fragment offset
 *	}
 *			
 * Concurrency
 * 	none
 *
 * To Do List
 *	This fastpath routine does not do UDP checksumming. There are
 *	some case (particularly internetworking) where this feature
 *	would be desirable. Add a check comparing the interface network
 *	(or subnet?) mask with network of the destination address.
 *	If they are not the same, we could either return -2 so that
 *	udp_output, which does checksumming, is called, or call the 
 *	udp checksum routine directly. An alternative test for doing
 *	checksumming would be whether we are using the gateway address 
 *	in the route structure.
 *
 * Notes
 *	The test for kudp_checksumming is a non-supported, non-documented
 *	option for enabling UDP checksumming for gateway traffic. The 
 *	value of kudp_checksumming (initially 0) can be changed with adb.
 *
 * Modification History
 *	11/2/87		lmb	added triggers, converted to NS logging
 *	11/6/87		lmb	added comments
 *	11/24/87	lmb     added test for kudp_checksumming 
 *	02/26/88	mds	modified global kudp_checksumming
 *				so it optionally be done regardless
 *				of whether routing is used
 *	06/06/88	arm	initialized nextip; made changes so that
 *				request for mbufs for both ip hdr and type
 *				2 mbuf are made before shuffling pointers. 
 *				( bug fix - nfsd hang DTS INDaa02735 )
 *
 * External Calls
 *	rtalloc		get a route to the destination
 *	MGET		get mbufs for headers
 *	m_freem		free an mbuf chain
 *	m_free		free a single mbuf
 *	sotoinpcb	macro converting socket to internet pcb
 *	in_pcbbind	bind an unused port to socket
 *	in_cksum	get an internet checksum for IP
 *	*ifp->if_output	interface output routine (lan0_output)
 *	
 * Called By
 *	ku_sento_mbuf	NFS kernel UDP send routine
 *
 ********************************************************************
 * END_IMS ku_fastsend */

ku_fastsend(so, am, to)
	struct socket *so;		/* socket data is sent from */
	register struct mbuf *am;	/* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int datalen;		/* length of all data in packet */
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct ip *ip;		/* ip header */
	register struct mbuf *m;	/* ip header mbuf */
	struct ifnet *ifp;		/* interface */
	struct mbuf *lam;		/* last mbuf in chain to be sent */
	struct sockaddr	*dst;		/* packet destination */
	struct inpcb *inp;		/* inpcb for binding */
	struct ip *nextip = NULL;	/* ip header for next fragment */
	struct route route;		/* route to send packet */
	static struct route zero_route;	/* to initialize route */
	int err;			/* error number */
	int sentpck = 0;		/* set if we try to send a pck */

#ifdef NTRIGGER
	int trig_temp1;			/* used for trigger work only	*/

	if (utry_trigger(T_NFS_FSEND_FAIL, T_NFS_PROC, &trig_temp1, NULL)) {
		if (trig_temp1 == 2) 
			return(-2);
		else {
			(void) m_freem(am);
			return(-1);
		}
	}
#endif NTRIGGER
	/* 
	 * Don't bother to check for routing if equal to 2
	 */
	if (kudp_checksumming == 2)
		return (-2);
	/*
	 * Determine length of data.
	 * This should be passed in as a parameter.
	 */
	datalen = 0;
	for (m = am; m; m = m->m_next) {
		datalen += m->m_len;
	}
	/*
	 * Routing.
	 * We worry about routing early so we get the right ifp.
	 */
	{
		register struct route *ro;

		route = zero_route;
		ro = &route;
		ro->ro_dst.sa_family = AF_INET;
		((struct sockaddr_in *)&ro->ro_dst)->sin_addr = to->sin_addr;
		rtalloc(ro);
		if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
			return(-2);
		}
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			if (kudp_checksumming)
				return (-2);
			dst = &ro->ro_rt->rt_gateway;
		} else {
			dst = &ro->ro_dst;
		}
	}
	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_WAIT, 0, MT_HEADER);
#ifdef NTRIGGER
	if (utry_trigger(T_NFS_MGET_3, T_NFS_PROC, NULL, NULL)) {
		(void) m_freem(m);
		m = NULL;
	}
#endif NTRIGGER
	if (m == NULL) {
		NS_LOG(LE_NFS_FSEND_MGET, NS_LC_RESOURCELIM, NS_LS_NFS, 0);
		return(-2);
	}
	m->m_off = MMINOFF;
	m->m_len = sizeof (struct ip) + sizeof (struct udphdr);
	/*
	 * Create IP header.
	 */
	ip = mtod(m, struct ip *);
	ip->ip_hl = sizeof (struct ip) >> 2;	/* header length */
	ip->ip_v = IPVERSION;			/* version */
	ip->ip_tos = 0;				/* type of service */
	ip->ip_id = ip_id++;			/* identification */
	ip->ip_off = 0;				/* fragment offset field */
	ip->ip_ttl = MAXTTL;			/* time to live */
	ip->ip_p = IPPROTO_UDP;			/* protocol */
	ip->ip_sum = 0;				/* checksum */
	ip->ip_src = ((struct sockaddr_in *)&ifp->if_addr)->sin_addr;
	ip->ip_dst = to->sin_addr;
	/*
	 * Bind port, if necessary.
	 * Is this really necessary?
	 */
	inp = sotoinpcb(so);
	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport==0) {
		(void) in_pcbbind(inp, (struct mbuf *)0);
	}
	/*
	 * Create UDP header.
	 */
	{
		register struct udphdr *udp;

		udp = (struct udphdr *)(ip + 1);
		udp->uh_sport = inp->inp_lport;		/* source port */
		udp->uh_dport = to->sin_port;		/* destination port */
		udp->uh_ulen = htons(sizeof (struct udphdr) + datalen);
		udp->uh_sum = 0;		/* 0 means "no checksumming" */
	}
	/*
	 * Fragnemt the data into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof (struct ip)) & ~7;
	curlen = sizeof (struct udphdr);
	fragoff = 0;
	for (;;) {
		register struct mbuf *mm;

		m->m_next = am;
		lam = m;
		while (am->m_len + curlen <= maxlen) {
			curlen += am->m_len;
			lam = am;
			am = am->m_next;
			if (am == 0) {
				ip->ip_off = htons((u_short) (fragoff >> 3));
				goto send;
			}
		}
		/* We get mbufs for both the remainder of the packet
		 * and a copy of the ip hdr before we start shuffling
		 * pointers. This is for a bug fix which was causing
		 * the nfsd to hang as there was a stage when the mbuf
		 * pointing to LARGEBUF was not linked to the mbuf
		 * chain. We were returning all mbufs except the
		 * one which would cause rrokwakeup to be called, when
		 * the m_get request for the ip hdr was returning null.
		 * ( bug fix - nfsd hang DTS INDaa02735 )
		 *		-- arm 6/6/88
		 */
		ip->ip_off = htons((u_short) ((fragoff >> 3) | IP_MF));
		/*
		 * There are more frags, so we save
		 * a copy of the ip hdr for the next
		 * frag.
		 */
		MGET(mm, M_WAIT, 0, MT_HEADER);
#ifdef NTRIGGER
		if (utry_trigger(T_NFS_MGET_5, T_NFS_PROC, NULL, NULL)) {
			(void) m_freem(mm);
			mm = NULL;
		}
#endif NTRIGGER
		if (mm == 0) {
			NS_LOG(LE_NFS_DUP_MGET, NS_LC_RESOURCELIM, NS_LS_NFS, 0);
			/* free entire chain if sent any frag */
			if (sentpck) {
				m_freem(m);	/* includes ip hdr */
				return (-1);
			} else {
				(void) m_free(dtom(ip));  /* just the hdr */
				return (-2);
			}
		}
	        mm->m_off = MMINOFF;
		mm->m_len = sizeof (struct ip);
		nextip = mtod(mm, struct ip *);
		*nextip = *ip;
		if (curlen == maxlen) {
			/*
			 * Incredible luck: last mbuf exactly
			 * filled out the packet.
			 */
			lam->m_next = 0;
		} else {
			/*
			 * We can squeeze part of the next
			 * mbuf into this packet, so we
			 * get a type 2 mbuf and point it at
			 * this data fragment.
			 */
			MGET(mm, M_WAIT, 0, MT_DATA);
#ifdef NTRIGGER
			if (utry_trigger(T_NFS_MGET_4, T_NFS_PROC, 0, 0)) {
				(void) m_freem(mm);
				mm = NULL;
			}
#endif NTRIGGER
			if (mm == NULL) {
				/*
				 *  if already sent a pck, free entire
				 *  chain; else just free the ip/udp hdr.
				 */
				if (sentpck)
					m_freem(m);
				else
					(void) m_free(m);
				/* free nextip mbuf (cannot be null) */
				(void) m_free(dtom(nextip));
				mm = dtom(ip);
				if (mm != m) {
					(void) m_free(mm);
				}
				NS_LOG(LE_NFS_FRAG_MGET, NS_LC_RESOURCELIM,
					NS_LS_NFS, 0);
				return (sentpck? -1 : -2);
			}
			grablen = maxlen - curlen;
			mm->m_off = mtod(am, int) - (int) mm;
			mm->m_len = grablen;
			mm->m_flags |= MF_LARGEBUF;
			mm->m_cltype = 2;
			mm->m_clfun = buffree;
			mm->m_clswp = NULL;
			lam->m_next = mm;
			am->m_len -= grablen;
			am->m_off += grablen;
			curlen = maxlen;
		}
		/*
		 * m now points to the head of an mbuf chain which
		 * contains the max amount that can be sent in a packet.
		 */
send:
		/*
		 * Set ip_len and calculate the ip header checksum.
		 */
		ip->ip_len = htons(sizeof (struct ip) + curlen);
                ip->ip_sum = 0;
                ip->ip_sum = in_cksum(dtom(ip), ip->ip_hl << 2);
		/*
		 * At last, we send it off to the ethernet.
		 */
#ifdef NTRIGGER
		if (utry_trigger(T_NFS_IF_ERROR, T_NFS_PROC, NIL, NIL)) {
			m_freem(m);
			err = 1;
		}
		else
			err = (*ifp->if_output)(ifp, m, dst);
		if (err) {
#else NTRIGGER
		if (err = (*ifp->if_output)(ifp, m, dst)) {
#endif NTRIGGER
			/*
			 * mbuf chain m has been freed at this point.
			 * am and nextip (if nonnull) must be freed here
			 */
			NS_LOG_INFO5(LE_NFS_FSEND_FAIL, NS_LC_WARNING,NS_LS_NFS,
				0, 3, err, sentpck, am, 0, 0);
			if (am) {
				m_free(dtom(nextip));
				m_freem(am);
			}
			return (-1);
		}
		if (am == 0) {
			return (0);
		}
		sentpck = 1;
		ip = nextip;
		m = dtom(ip);
		fragoff += curlen;
		curlen = 0;
	}
}


/* BEGIN_IMS pr_mbuf *
 ********************************************************************
 ****
 **** 			pr_mbuf(p, m)
 ****
 ********************************************************************
 * Input Parameters
 *	p	print string
 *	m	mbuf containing ip header to print
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This is a debug routine for the fastsend module. It prints out
 *	a chain of mbufs, including the fields of the IP header.
 *	
 *
 * Algorithm
 *	ifdef DEBUG
 *		if (m is large enough to hold an ip header)
 *			print the fields of the ip header
 *		else
 *			print the length of the mbuf
 *		while (still mbufs on chain) {
 *			print mbuf address, length, data address
 *			print each byte of data
 *		}
 *	endif DEBUG
 *
 * Notes
 *	As it stands, the routine will not print out all the bytes of data
 *	in each packet. If this is desired, the line resetting len must
 *	uncommented before the loop to print out data.
 *
 * Modiication History
 *	11/6/87		lmb	added (commented) line allow printing of data
 *
 * External Calls
 *	printf, mtod
 *
 ********************************************************************
 * END_IMS pr_mbuf */

#ifdef DEBUG
pr_mbuf(p, m)
	char *p;
	struct mbuf *m;
{
	register char *cp, *cp2;
	register struct ip *ip;
	register int len;

	len = 28;
	printf("%s: ", p);
	if (m && m->m_len >= 20) {
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off %d ttl %d p %d sum %d src %x dst %x\n",
			ip->ip_hl, ip->ip_v, ip->ip_tos, ip->ip_len,
			ip->ip_id, ip->ip_off >> 13, ip->ip_off & 0x1fff,
			ip->ip_ttl, ip->ip_p, ip->ip_sum, ip->ip_src.s_addr,
			ip->ip_dst.s_addr);
		len = 0;
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		m = m->m_next;
	} else if (m) {
		printf("pr_mbuf: m_len %d\n", m->m_len);
	} else {
		printf("pr_mbuf: zero m\n");
	}
	while (m) {
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		cp = mtod(m, caddr_t);
		cp2 = cp + m->m_len;
		/* len = m->m_len; */
		while (cp < cp2) {
			if (len-- < 0) {
				break;
			}
			printf("%x ", *cp & 0xFF);
			cp++;
		}
		m = m->m_next;
		printf("\n");
	}
}
#endif DEBUG
