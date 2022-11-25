/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/kudp_fsend.c,v $
 * $Revision: 1.7.83.4 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/13 17:10:47 $
 *  Modified for 4.3BSD Quick Port
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
*	06/06/88	arm		initialized nextip; made changes so tha
t
*					request for mbufs for both ip hdr and 
*					type 2 mbuf are made before shuffling 
*					pointers. 
*					( bug fix - nfsd hang DTS INDaa02735 )
*
*	05/01/92	kls		Changes are made for fragmentation trai
n*					change. 
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: kudp_fsend.c,v 1.1.109.2 92/03/26 15:58:49
 stellaw Exp $ (Hewlett-Packard)";
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
#include "../h/errno.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/file.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/udp.h"
#include "../h/trigdef.h"
#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
/*
 * This is a flag that can be changed with adb to force checksumming
 * when going through a gateway. It is a non-supported, non-documented
 * option.
 * If its value is 2, then checksumming will be done regardless of
 * whether you are going through a gateway or not.  This is because
 * you can set up a CISCO gateway without using routing.
 */
int kudp_checksumming = 0;
/*---------------------------------------------------------------------------
int kudp_checksumming = 2;
----------------------------------------------------------------------------*/

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
 *	0  		send ok
 *	EAGAIN  	send failed: mbuf chain intact 
 *      OTHER          	send failed: mbuf chain freed by driver, return driver
 *			error code.
 *
 * Globals Referenced
 * 	none
 *
 * Description
 *  	Take the mbuf chain at am, add ip/udp headers, and fragment it
 *  	for the interface output routine.  A fragment train is built and 
 *	sent to the interface. If the send fails, mbuf chain will be freed 
 *      by the interface. Error code will return to the caller. 
 *	If kudp_checksumming == 2 is defined, return EAGAIN to the caller.
 *
 * Algorithm
 *			
 * Concurrency
 * 	none
 *
 * To Do List
 * 	none
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
 *	05/05/92	kls	Due to fragment train change, major rewrite
 *				for this procedure. Also MGET uses wait option
 *				VASSERT is used.
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
	struct mbuf *am;	        /* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int datalen;		/* length of all data in packet */
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct ip *ip;		/* ip header */
	struct mbuf *m;	                /* ip header mbuf */
        register struct in_ifaddr *ia;
	struct ifnet *ifp;		/* interface */
	struct mbuf *lam;		/* last mbuf in chain to be sent */
	struct sockaddr	*dst;		/* packet destination */
	struct inpcb *inp;		/* inpcb for binding */
	struct ip *nextip = NULL;	/* ip header for next fragment */
	struct route route;		/* route to send packet */
	static struct route zero_route;	/* to initialize route */
	int err = 0;			/* error number */
        struct mbuf *m0;                /* first mbuf in train to be sent */ 
	struct mbuf **mactp;		/* addr to append next chain to train */
	int s;
 
#ifdef NTRIGGER
	int trig_temp1;			/* used for trigger work only	*/

	if (utry_trigger(T_NFS_FSEND_FAIL, T_NFS_PROC, &trig_temp1, NULL)) {
		if (trig_temp1 == 2) 
			return (EAGAIN);
		else {
			(void) m_freem(am);
			return (-1);
		}
	}
#endif NTRIGGER
	/* 
	 * Don't bother to check for routing if equal to 2
	 */
	if (kudp_checksumming == 2)
		return (EAGAIN);
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

		/* Make sure we got a route */
		if (ro->ro_rt == 0)
			return (EAGAIN);

		/* Make sure there is an interface for the route */
		if ((ifp = ro->ro_rt->rt_ifp) == 0) {
			RTFREE(route.ro_rt);
			return (EAGAIN);
		}
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			if (kudp_checksumming) {
				RTFREE(route.ro_rt);
				return (EAGAIN);
			}
			dst = &ro->ro_rt->rt_gateway;
		} else {
			dst = &ro->ro_dst;
		}
	}
	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_WAIT, MT_HEADER);

        VASSERT(m != NULL );

	m->m_off = MMINOFF + IN_MAXLINKHDR;
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

	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_ifp == ifp) {
			ip->ip_src = IA_SIN(ia)->sin_addr;
			break;
		}

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
	 * Fragment the data into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof (struct ip)) & ~7;
	curlen = sizeof (struct udphdr);
	fragoff = 0;
      
	mactp = &m0;
	for (;;) {
		struct mbuf *mm;

		*mactp = m;
		mactp = &m->m_act;
		m->m_next = am;
		lam = m;
		while (am->m_len + curlen <= maxlen) {
			curlen += am->m_len;
			lam = am;
			am = am->m_next;
			if (am == 0) {
				ip->ip_off = htons((u_short) (fragoff >> 3));
				goto sendorfree;
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
		MGET(mm,  M_WAIT,MT_HEADER);
                
                VASSERT(mm != NULL);

	        mm->m_off = MMINOFF + IN_MAXLINKHDR;
		mm->m_len = sizeof (struct ip);
		nextip = mtod(mm, struct ip *);
		*nextip = *ip;
		if (curlen != maxlen) {
			/*
			 * We can squeeze part of the next mbuf (am)
			 * into this packet.  Copy the data that 
			 * can be appended to this packet.
			 */
			MGET(mm, M_WAIT, MT_DATA);

                        VASSERT(mm != NULL);

			grablen = maxlen - curlen;
			curlen  = maxlen;
			mm->m_len = grablen;

			if (M_HASCL(am)) {
				mm->m_clsize = am->m_clsize;
				mm->m_cltype = am->m_cltype;
				mm->m_off = mtod(am, caddr_t) - (caddr_t)mm;
                        	mm->m_head = am->m_head;
				/* 
				 * Assumes there are no copies to the buffers 
				 * ref'd by parameter am at entry; otherwise 
				 * the increment of refcnt must be protected.
				 */

				s = splimp();
                        	am->m_head->m_refcnt++;
				splx(s);
			}
			else {
				(void)bcopy(mtod(am,caddr_t), mtod(mm,caddr_t),
						mm->m_len);
			}
			am->m_len -= grablen;
			am->m_off += grablen;
			lam->m_next = mm;
		}
		else {
			/*
			 * Incredible luck: last mbuf exactly
			 * filled out the packet.
			 */
			lam->m_next = 0;
		}

		/*
		 * m now points to the head of an mbuf chain which
		 * contains the max amount that can be sent in a packet.
		 */
		/*
		 * Set ip_len and calculate the ip header checksum.
		 */
		ip->ip_len = htons(sizeof (struct ip) + curlen);
                ip->ip_sum = 0;
#ifdef __hp9000s800
                /* this is a new routine in 9.0 for performance improvement */
		ip->ip_sum = ip_cksum(ip, ip->ip_hl << 2);
#else
                ip->ip_sum = in_cksum(dtom(ip), ip->ip_hl << 2);
#endif
	        ip = nextip;
		m = dtom(ip);  

		fragoff += curlen;
		curlen = 0;

        } /* for(;;) loop */
	/* NOT REACHED */

sendorfree:    /* the last fragment needs to be checksummed here */ 
               ip->ip_len = htons(sizeof (struct ip) + curlen);
               ip->ip_sum = 0;
#ifdef __hp9000s800  
               /* this is a new routine in 9.0 for performance improvement */
               ip->ip_sum = ip_cksum(ip, ip->ip_hl << 2);
#else
               ip->ip_sum = in_cksum(dtom(ip), ip->ip_hl << 2);
#endif
		/*
		 * At last, we send it off to the ethernet.
		 */
#ifdef NTRIGGER
		if (utry_trigger(T_NFS_IF_ERROR, T_NFS_PROC, NIL, NIL)) {
			m_freem_train(m0);
			err = 1;
		}
		else
#endif NTRIGGER
		err = (*ifp->if_output)(ifp, m0, dst);
		if (err) {
			NS_LOG_INFO5(LE_NFS_FSEND_FAIL,NS_LC_WARNING,NS_LS_NFS,
				0, 3, err, 1, 0, 0, 0);
		}
	RTFREE(route.ro_rt);
	return (err);
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
	eegister struct ip *ip;
	register int len;

	len = 28;
	printf("%s: ", p);
	if (m && m->m_len >= 20) {
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off %d ttl %d p %d
 sum %d src %x dst %x\n",
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
