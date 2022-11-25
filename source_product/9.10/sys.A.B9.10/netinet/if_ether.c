/*
 * $Header: if_ether.c,v 1.20.83.5 93/09/17 19:02:05 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/if_ether.c,v $
 * $Revision: 1.20.83.5 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:02:05 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) if_ether.c $Revision: 1.20.83.5 $";
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
 *	@(#)if_ether.c	7.7 (Berkeley) 6/29/88
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	link entries onto hash chains, keep free list
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/netfunc.h"
#include "../h/mib.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"
#include "../netinet/if_probe.h"
#include "../netinet/if_atr.h"
#include "../nipc/nipc_hpdsn.h"
#include "../netinet/mib_kern.h"

#include "../h/subsys_id.h"
#include "../h/net_diag.h"

#undef	insque
#undef	remque

#define RIF_SIZE        18      /* token ring - source routing */
#define	ARPTAB_NB	32	/* number of buckets - Must be ^2 */
#define	ARPT_KILLC	20*60*2	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	10*60*2	/* kill incomplete entry in 10 minutes */
#define BROADCAST	0	/* broadcast address resolution request */
#define UNICAST		1	/* unicast address resolution request */
#define ATF_FLAGS	(ATF_INUSE | ATF_PERM | ATF_PUBL | ATF_USETRAILERS | ATF_DOUBTFUL)
#define MIN_FDDI_HDR_LEN 32	/* minimum space for FDDI MAC header */

int arpt_killc = ARPT_KILLC;
int arpt_killi = ARPT_KILLI;
int prb_nameinit = 0;		/* probe name initiailized?? */
int unicast_time = 5*60*2;	/* unicast every 5 minutes */
static int min_time = 3*60*2;	/* min value to prevent too many unicasts */
int rebroadcast_time = 1*60*2;	/* broadcast INCOMP,HOLD entries every minute */
int arptab_nb = ARPTAB_NB;	/* for arp command */
u_short prb_sequence = 0;	/* sequence numbers */
struct arphd arphd[ARPTAB_NB];
struct probestat probestat;
struct arpstat arpstat;
struct ifqueue arpintrq;
struct ifqueue probeintrq;

char prb_mcast[PRB_NUMMCASTS][6] = {
	{ 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },	/* probe vna multicast addr */
	{ 0x09, 0x00, 0x09, 0x00, 0x00, 0x02 }, /* probe proxy multicast addr */
};
u_char etherbroadcastaddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static struct sockaddr_in gateway = { AF_INET };

/*
** Forward Declarations
*/
int whohas_null(), whohas_ether(), whohas_snap8023(), whohas_snap8024();
int whohas_etherxt(), whohas_ieee8023(), whohas_ether8023();
int whohas_snapfddi();
int whohas_IEEE8025();
int whohas_snap8025();
int whohas_atr();
void arptreset();
struct arptab *arptnew();
char *ether_sprintf();
struct ifnet *ifunit();

struct callfunc {
    int (*func)();
}; 

/*
 * The whohas arrays are indexed vertically (first index) by encapsulation 
 * method (i.e. the flags currently enabled in the arpcom), and horizontally 
 * (second index) by the current value of our 1/2 second timer modulo
 * the number of slots.
 * 
 * Note that for 802.3 interfaces we (potentially) support arbitrary
 * combinations of ethernet, ieee, and snap.  For 802.4 interfaces, we
 * support only snap.
 */
struct callfunc whohas8023[][AT_SLOTS] = {
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 0 */
{ {whohas_ether}, {whohas_null}, {whohas_etherxt}, {whohas_null} },
				/* 1 ACF_ETHER */
{ {whohas_ieee8023}, {whohas_null}, {whohas_ieee8023}, {whohas_null} },
				/* 2 ACF_IEEE8023 */
{ {whohas_ether}, {whohas_etherxt}, {whohas_ieee8023}, {whohas_null} },
				/* 3 ACF_ETHER & ACF_IEEE8023 */
{ {whohas_snap8023}, {whohas_null}, {whohas_snap8023}, {whohas_null} },
				/* 4 ACF_SNAP8023 */
{ {whohas_snap8023}, {whohas_ether}, {whohas_etherxt}, {whohas_null} },
				/* 5 ACF_ETHER & ACF_SNAP8023 */
{ {whohas_snap8023}, {whohas_null}, {whohas_ieee8023}, {whohas_null} }, 
				/* 6 ACF_IEEE8023 & ACF_SNAP8023 */
{ {whohas_snap8023}, {whohas_ether}, {whohas_etherxt}, {whohas_ieee8023} }, 
				/* 7 ACF_ETHER & ACF_IEEE8023 & ACF_SNAP8023 */
};
struct callfunc whohas8024[][AT_SLOTS] = {
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 0 */
{ {whohas_snap8024}, {whohas_null}, {whohas_snap8024}, {whohas_null} },
				/* 1 ACF_SNAP8024 */
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 2 ACF_IEEE8024 */
{ {whohas_snap8024}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 3 ACF_SNAP8024 & ACF_IEEE8024 */
};
struct callfunc whohasfddi[][AT_SLOTS] = {
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 0 */
{ {whohas_snapfddi}, {whohas_null}, {whohas_snapfddi}, {whohas_null} },
				/* 1 ACF_SNAPFDDI */
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 2 */
{ {whohas_snapfddi}, {whohas_null}, {whohas_snapfddi}, {whohas_null} },
				/* 3 ACF_SNAPFDDI */
};

struct callfunc whohas8025[][AT_SLOTS] = {
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 0 */
{ {whohas_snap8025}, {whohas_null}, {whohas_snap8025}, {whohas_null} },
				/* 1 ACF_SNAP8025 */
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 2 ACF_8025 */
{ {whohas_snap8025}, {whohas_null}, {whohas_snap8025}, {whohas_null} },
				/* 3 ACF_SNAP8025  &  ACF_IEEE8025 */
};

struct callfunc whohasatr[][AT_SLOTS] = {
{ {whohas_null}, {whohas_null}, {whohas_null}, {whohas_null} },
				/* 0 */
{ {whohas_atr}, {whohas_null}, {whohas_atr}, {whohas_null} },
				/* 1 */
};

extern struct ifnet loif;

#define	ARPTAB_HASH(a) \
	((u_long)(a) & (ARPTAB_NB - 1))

/* lookup entry with IP address addr */
#define	ARPTAB_LOOK(at, addr) { 					\
	int hash = ARPTAB_HASH(addr); 					\
	at = arphd[hash].at_next; 					\
	for (; at != (struct arptab *) &arphd[hash]; at = at->at_next)	\
		if (at->at_iaddr.s_addr == addr) 			\
			break; 						\
	if (at == (struct arptab *) &arphd[hash]) 			\
		at = 0; 						\
}

/* lookup entry with sequence number seq -- for probe */
#define ARPTAB_SEQ(at,seq) { 						\
	int i; 								\
	for (i = 0; i < ARPTAB_NB; i++)					\
	    for (at = arphd[i].at_next; at != (struct arptab *) &arphd[i]; \
		 at = at->at_next)					\
		    if ((at->at_seqno == seq) && (at->at_iaddr.s_addr != 0)) \
			goto arptab_seq_done;				\
	if (at == (struct arptab *) &arphd[ARPTAB_NB-1])		\
		at = 0; 						\
arptab_seq_done:							\
	;								\
}

/* complete the state of an arptable entry */
#define ARPTAB_COMPLETE(at, new_type) {					\
	(at)->at_state = ATS_COMP;					\
	(at)->at_type = (new_type);					\
	(at)->at_bitcnt = AT_MAXWAIT;					\
	(at)->at_unitimer = unicast_time;				\
}

/* 
 * Set flags depending upon type when entry if completed by arp or probe.
 * Validation of the packet type is done in the respective routines,
 * in_arpinput() and probeinput().
 */
#define ARPTAB_FLAGSSET(at, pkt_type) {					\
	if ((pkt_type) == ETHER_PKT)					\
	    (at)->at_flags =  (at->at_flags & (ATF_FLAGS)) | ATF_COM;	\
	else if ((pkt_type) == SNAP8023_PKT)				\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_SNAP8023; \
	else if ((pkt_type) == SNAP8024_PKT)				\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_SNAP8024; \
	else if ((pkt_type) == ETHERXT_PKT)				\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_ETHERXT; \
	else if ((pkt_type) == IEEE8023XSAP_PKT)			\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_IEEE8023; \
	else if ((pkt_type) == SNAPFDDI_PKT)				\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_SNAPFDDI; \
	else if ((pkt_type) == SNAP8025_PKT)				\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_SNAP8025; \
	else if ((pkt_type) == ATR_PKT)					\
	    (at)->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_ATR;	\
}
	

/* 
 * dummy routine for the array whohas8023[][] and whohas8024[][] 
 * i.e. for this encapsulation and pass, no operation is supported
 */
static int
whohas_null(ac, addr, seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
}

/*
 * send out an arp request on ether, asking who has addr on interface ac 
 */
static struct ether_hdr ether_req = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct ether_hdr ether_unireq = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};

static int
whohas_ether(ac, addr, seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;				/* not used */
	int mode;
{
	struct mbuf *m = 0;
	int error = 0;

	if (ac->ac_ipaddr.s_addr == 0) 		/* not initialized */
		return(error);

	if (mode == BROADCAST) 
		error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
				(caddr_t)&ether_req, &m);
	else 		
		error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
				(caddr_t)&ether_unireq, &m);

	if (!error) {
		struct ether_arp *ea;  
		struct mbuf *m1 = m; 
		/* skip leading mbufs */ 
		while (m1->m_next) 
			m1 = m1->m_next; 
		/* check if there is enough space to append the arp packet */ 
		if ((m1->m_len + sizeof(struct ether_arp)) > MLEN) { 
			struct mbuf *m2; 
			if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) { 
				m_freem(m); 
				return (ENOBUFS); 
			} 
			m1->m_next = m2; 
			m1 = m2; 
		} 
		/* fill in the data portion for the ARP request packet */ 
		ea = (struct ether_arp *)(mtod(m1, caddr_t) + m1->m_len); 
		ea->arp_hrd = htons(ARPHRD_ETHER); 
		ea->arp_pro = htons(ETHERTYPE_IP); 
		/* hardware address length */ 
		ea->arp_hln = sizeof(ea->arp_sha); 
		/* protocol address length */ 
		ea->arp_pln = sizeof(ea->arp_spa); 
		ea->arp_op = htons(ARPOP_REQUEST); 
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha, 
	   		sizeof(ea->arp_sha)); 
		bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa, 
	   		sizeof(ea->arp_spa)); 
		bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa,  
			sizeof(ea->arp_tpa)); 
		m1->m_len += sizeof(struct ether_arp); 

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	} 
	else 
		if (error != ENOBUFS)
			m_freem(m);
	return (error);
} /* whohas_ether */

/*
 * send out an arp request on snap8023, asking who has addr on interface ac 
 */
static struct snap8023_hdr snap8023_req = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8023_hdr snap8023_unireq = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};

static int
whohas_snap8023(ac,addr, seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m = 0;
	int error = 0;

	if (ac->ac_ipaddr.s_addr == 0)          /* not initialized */
		return(error);
	if (mode == BROADCAST) {
	    snap8023_req.length = sizeof(struct ether_arp) + 8;
	    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			(caddr_t)&snap8023_req, &m);
	} else {		
	    snap8023_unireq.length = sizeof(struct ether_arp) + 8;
	    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			(caddr_t)&snap8023_unireq, &m);
	}

	if (!error) {
		struct ether_arp *ea;  
		struct mbuf *m1 = m; 
		/* skip leading mbufs */ 
		while (m1->m_next) 
			m1 = m1->m_next; 
		/* check if there is enough space to append the arp packet */ 
		if ((m1->m_len + sizeof(struct ether_arp)) > MLEN) { 
			struct mbuf *m2; 
			if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) { 
				m_freem(m); 
				return (ENOBUFS); 
			} 
			m1->m_next = m2; 
			m1 = m2; 
		} 
		/* fill in the data portion for the ARP request packet */ 
		ea = (struct ether_arp *)(mtod(m1, caddr_t) + m1->m_len); 
		ea->arp_hrd = htons(ARPHRD_ETHER); 
		ea->arp_pro = htons(ETHERTYPE_IP); 
		/* hardware address length */ 
		ea->arp_hln = sizeof(ea->arp_sha); 
		/* protocol address length */ 
		ea->arp_pln = sizeof(ea->arp_spa); 
		ea->arp_op = htons(ARPOP_REQUEST); 
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha, 
	   		sizeof(ea->arp_sha)); 
		bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa, 
	   		sizeof(ea->arp_spa)); 
		bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa,  
			sizeof(ea->arp_tpa)); 
		m1->m_len += sizeof(struct ether_arp); 

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	} 
	else 
		if (error != ENOBUFS)
			m_freem(m);
	return(error);
} /* whohas_snap8023 */

/* send out an arp request on snapfddi, asking who has addr */
static struct snapfddi_hdr_info snapfddi_req = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    ETHERTYPE_ARP
};

static struct snapfddi_hdr_info snapfddi_unireq = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    ETHERTYPE_ARP
};

static int
whohas_snapfddi(ac,addr,seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m = 0;
	int error = 0;

	if (ac->ac_ipaddr.s_addr == 0)		/* not initliazed */
		return(error);
        if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
                return(ENOBUFS);
	m->m_off = MIN_FDDI_HDR_LEN;
	m->m_len = 0;
	if (mode == BROADCAST) 
            error = (*ac->ac_build_hdr)(ac, SNAPFDDI_PKT,
                        (caddr_t)&snapfddi_req, &m);
	else 
            error = (*ac->ac_build_hdr)(ac, SNAPFDDI_PKT,
                        (caddr_t)&snapfddi_unireq, &m);

	if (!error) {
		struct ether_arp *ea;  
		struct mbuf *m1 = m; 
		/* skip leading mbufs */ 
		while (m1->m_next) 
			m1 = m1->m_next; 
		/* check if there is enough space to append the arp packet */ 
		if ((m1->m_len + sizeof(struct ether_arp)) > MLEN) { 
			struct mbuf *m2; 
			if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) { 
				m_freem(m); 
				return (ENOBUFS); 
			} 
			m1->m_next = m2; 
			m1 = m2; 
		} 
		/* fill in the data portion for the ARP request packet */ 
		ea = (struct ether_arp *)(mtod(m1, caddr_t) + m1->m_len); 
		ea->arp_hrd = htons(ARPHRD_ETHER); 
		ea->arp_pro = htons(ETHERTYPE_IP); 
		/* hardware address length */ 
		ea->arp_hln = sizeof(ea->arp_sha); 
		/* protocol address length */ 
		ea->arp_pln = sizeof(ea->arp_spa); 
		ea->arp_op = htons(ARPOP_REQUEST); 
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha, 
	   		sizeof(ea->arp_sha)); 
		bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa, 
	   		sizeof(ea->arp_spa)); 
		bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa,  
			sizeof(ea->arp_tpa)); 
		m1->m_len += sizeof(struct ether_arp); 

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	} 
	else 
		m_freem(m);
	return(error);
} /* whohas_snapfddi */


/* 
 * send out an arp request on snap8024, asking who has addr on interface ac 
 */
static struct snap8024_hdr snap8024_req = {
    IEEE8024_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8024_hdr snap8024_unireq = {
    IEEE8024_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};

static int
whohas_snap8024(ac,addr, seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m;
	int error = 0;

	if (ac->ac_ipaddr.s_addr == 0)		/* not initliazed */
		return(error);
        if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
                return(ENOBUFS);

	if (mode == BROADCAST) 
            error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT,
                        (caddr_t)&snap8024_req, m);
	else 
            error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT,
                        (caddr_t)&snap8024_unireq, m);

	if (!error) {
		struct ether_arp *ea;  
		struct mbuf *m1 = m; 
		/* skip leading mbufs */ 
		while (m1->m_next) 
			m1 = m1->m_next; 
		/* check if there is enough space to append the arp packet */ 
		if ((m1->m_len + sizeof(struct ether_arp)) > MLEN) { 
			struct mbuf *m2; 
			if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) { 
				m_freem(m); 
				return (ENOBUFS); 
			} 
			m1->m_next = m2; 
			m1 = m2; 
		} 
		/* fill in the data portion for the ARP request packet */ 
		ea = (struct ether_arp *)(mtod(m1, caddr_t) + m1->m_len); 
		ea->arp_hrd = htons(ARPHRD_ETHER); 
		ea->arp_pro = htons(ETHERTYPE_IP); 
		/* hardware address length */ 
		ea->arp_hln = sizeof(ea->arp_sha); 
		/* protocol address length */ 
		ea->arp_pln = sizeof(ea->arp_spa); 
		ea->arp_op = htons(ARPOP_REQUEST); 
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha, 
	   		sizeof(ea->arp_sha)); 
		bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa, 
	   		sizeof(ea->arp_spa)); 
		bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa,  
			sizeof(ea->arp_tpa)); 
		m1->m_len += sizeof(struct ether_arp); 

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	} 
	else 
		m_freem(m);
	return(error);
} /* whohas_snap8024 */


/* 
 * send out an arp request on snap8025 
 */
static struct snap8025_sr_hdr snap8025_req = {
    IEEE8025_AC, IEEE8025_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 0xE2 ->> single route, nonbroad ret, 2 byte rif.
       0x70 ->> dir default, longest frm = init val. */
    { 0xE2, 0x70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8025_sr_hdr snap8025_unireq = {
    IEEE8025_AC, IEEE8025_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};


static int
whohas_snap8025(ac, addr, seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m = 0;
	int error = 0;

	if (ac->ac_ipaddr.s_addr == 0)		/* not initliazed */
		return(error);

	if (mode == BROADCAST) 
            error = (*ac->ac_build_hdr)(ac, SNAP8025_PKT,
                        (caddr_t)&snap8025_req, &m);
	else 
            error = (*ac->ac_build_hdr)(ac, SNAP8025_PKT,
                        (caddr_t)&snap8025_unireq, &m);

	if (!error) {
		struct ether_arp *ea;  
		struct mbuf *m1 = m; 
		/* check if there is enough space to append the arp packet */ 
		if ((m->m_len + sizeof(struct ether_arp)) > MLEN) { 
			if ((m1 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) { 
				m_freem(m); 
				return (ENOBUFS); 
			} 
			m->m_next = m1; 
		} 
		/* fill in the data portion for the ARP request packet */ 
		ea = (struct ether_arp *)(mtod(m1, caddr_t) + m->m_len); 
		ea->arp_hrd = htons(ARPHRD_IEEE); 
		ea->arp_pro = htons(ETHERTYPE_IP); 
		/* hardware address length */ 
		ea->arp_hln = sizeof(ea->arp_sha); 
		/* protocol address length */ 
		ea->arp_pln = sizeof(ea->arp_spa); 
		ea->arp_op = htons(ARPOP_REQUEST); 
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha, 
	   		sizeof(ea->arp_sha)); 
		bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa, 
	   		sizeof(ea->arp_spa)); 
		bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa,  
			sizeof(ea->arp_tpa)); 
		m1->m_len += sizeof(struct ether_arp); 

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	} 
	else 
		if (error != ENOBUFS)
			m_freem(m);
	return(error);
} /* whohas_snap8025 */


/* 
 * send out a probe request on ieee8023, asking who has addr on interface ac 
 */
static struct ieee8023xsap_hdr ieee8023xsap_req = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023xsap_unireq = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};

static int
whohas_ieee8023(ac,addr,seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m = 0;
	struct prb_hdr *ph;
	struct prb_vnareq *prq;
	int error=0;

	if (ac->ac_ipaddr.s_addr == 0)		/* not initialiazed */
		return(error);
	if (mode == BROADCAST) {
	    ieee8023xsap_req.length = PRB_VNAREQSIZE + PRB_PHDRSIZE +
					IEEE8023XSAP_LEN;
	    error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
			(caddr_t)&ieee8023xsap_req, &m);
	} else {		
	    ieee8023xsap_unireq.length = PRB_VNAREQSIZE + PRB_PHDRSIZE +
					IEEE8023XSAP_LEN;
	    error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
			(caddr_t)&ieee8023xsap_unireq, &m);
	}
	if (!error) {
		struct mbuf *m1 = m;
		while (m1->m_next)
			m1 = m1->m_next;
	
		if ((m1->m_len + PRB_VNAREQSIZE + PRB_PHDRSIZE) > MLEN) {
			struct mbuf *m2;
        		if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) {
				m_freem(m);
				return (ENOBUFS);
			}
			m1->m_next = m2;
			m1 = m2;
		}
		ph = (struct prb_hdr *)(mtod(m1,caddr_t) + m1->m_len);
		ph->ph_version = PHV_VERSION;
		ph->ph_type = PHT_VNAREQ;
		ph->ph_len = PRB_VNAREQSIZE + PRB_PHDRSIZE;
		ph->ph_seq = seqno;
		prq = (struct prb_vnareq *)(ph + 1);
		prq->prq_replen = PRQR_REPLEN;
		prq->prq_dreplen = PRQD_DREPLEN;
		prq->prq_version = PRQV_VNAVERSION;
		prq->prq_domain = HPDSN_DOM;
		bcopy((caddr_t)addr, (caddr_t)prq->prq_iaddr, 4);
		m1->m_len += PRB_VNAREQSIZE + PRB_PHDRSIZE;

		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	}
	else
		if (error != ENOBUFS)
			m_freem(m);
	return(error);
} /* whohas_ieee8023 */

/* send out an arp request on ether, asking who has addr on interface ac */
static struct etherxt_hdr etherxt_req = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct etherxt_hdr etherxt_unireq = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static int
whohas_etherxt(ac,addr,seqno, mode)
	struct arpcom *ac;
	struct in_addr *addr;
	int seqno;
	int mode;
{
        struct mbuf *m = 0;
	struct prb_hdr *ph;
	struct prb_vnareq *prq;
	int error = 0;
	/*
	 * If the no ifconfig has been done on the interface (for AF_INET) or
	 * if netipc is not configured in the kernel, we do nothing. No point
	 * in sending out probe packets on ethernet, if netipc is not 
	 * configured in the system. Kludgy, but less clutter on the net.
	 */
        if ((ac->ac_ipaddr.s_addr == 0) || !(prb_nameinit))
                return(error);
	if (mode == BROADCAST)
	    error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT,
			(caddr_t)&etherxt_req, &m);
	else 			
	    error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT,
			(caddr_t)&etherxt_unireq, &m);
	if (!error) {
		struct mbuf *m1 = m;
		while (m1->m_next)
			m1 = m1->m_next;
	
		if ((m1->m_len + PRB_VNAREQSIZE + PRB_PHDRSIZE) > MLEN) {
			struct mbuf *m2;
        		if ((m2 = m_get(M_DONTWAIT, MT_HEADER)) == NULL) {
				m_freem(m);
				return (ENOBUFS);
			}
			m1->m_next = m2;
			m1 = m2;
		}
        	ph = (struct prb_hdr *)(mtod(m1,caddr_t) + m1->m_len);
		ph->ph_version = PHV_VERSION;
		ph->ph_type = PHT_VNAREQ;
		ph->ph_len = PRB_VNAREQSIZE + PRB_PHDRSIZE;
		ph->ph_seq = seqno;
		prq = (struct prb_vnareq *)(ph + 1);
		prq->prq_replen = PRQR_REPLEN;
		prq->prq_dreplen = PRQD_DREPLEN;
		prq->prq_version = PRQV_VNAVERSION;
		prq->prq_domain = HPDSN_DOM;
		bcopy((caddr_t)addr, (caddr_t)prq->prq_iaddr, 4);
		m1->m_len += PRB_VNAREQSIZE + PRB_PHDRSIZE;
	
		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, m);
	}
	else 
		if (error != ENOBUFS)
			m_freem(m);
	return(error);
} /* whohas_etherxt */

/*
 * This is called from the if_output() routine provided by the LAN driver
 * interface.  The if_output() routine, when getting an outbound packet
 * of address family AF_INET, will call this routine to resolve
 * the network address.
 *
 * Philosophy : ip knows better than arp/probe
 * if ifp does not match cached (struct ifnet *)at_ac, re-arp on ifp
 * for input routines : if unsolicited reply comes in, make a new entry
 *			discard reply/request if ifp and at_ac don't match
 */
arp_resolve(ifp,m,sa,desten)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *sa;
	u_long *desten;
{
	struct sockaddr_in *sin;
	struct arptab *at;
	int error = 0;
	u_long bcastaddr = 0;


	switch (sa->sa_family) {
	case AF_UNSPEC:
	    m_freem_train(m);
	    error = EAFNOSUPPORT;
	    break;
#ifdef	APPLETALK
	case AF_APPLETALK:
	    m_freem_train(m);
	    error = EAFNOSUPPORT;
	    break;
#endif
	case AF_INET:
	    if (!(ifp->if_flags & IFF_UP)) {
		m_freem_train(m);
		error = ENETUNREACH;
		break;
	    }
	    sin = (struct sockaddr_in *)sa;
	    ARPTAB_LOOK(at, ((u_long)sin->sin_addr.s_addr));
	    if (at) {
		at->at_timer = 0;
		if ((at->at_state == ATS_COMP) || 
		    ((at->at_state == ATS_HOLD) && (at->at_type != ATT_INVALID))) {
		    /*
		     * entry is in complete state or in hold state,
		     * but was complete some time in the past 
		     */
		    if (ifp != (struct ifnet *)(at->at_ac)) {
			/* 
			 * Some one above has decided to try a different
			 * interface than the one we have a cache entry for.
			 * By reseting the cache entry, arptimer will pick
			 * up on the need to arp again.  We do this instead
			 * of arping directly to implement a 1/2 second
			 * hold down (and there by prevent arp floods)
			 */
			arptreset(at);
			at->at_hold = m;
			at->at_ac = (struct arpcom *)ifp;
		    } else {
			/*
			 * Completed or Hold Down -- have to give the
			 * best effort even if doubtful
			 */
			if (at->at_ac->ac_type == ACT_ATR) {
			    bcopy(at->at_enaddr, desten, 4);
			    return(-1);
			} else
			    error = unicast_ippkt(at, m);
		    }
		} else {
		    /* 
		     * Entry is either incomplete or in hold down state.
		     * If it is incomplete let arptimer() take care of it.
		     * If it is in hold down state, then we might never
		     * resolve it. So free up the mbuf chain now rather
		     * than wait a minute.
		     */
		    if (at->at_hold) {
			m_freem_train(at->at_hold);
			at->at_hold = NULL;
		    }
		    if (at->at_state == ATS_HOLD)
			/* should we return an error -XXX */
			m_freem_train(m);
		    else
			at->at_hold = m;
		    /*
		     * If no encapsulation methods are enabled, we return
		     * an error.
		     */
		    if ((at->at_ac->ac_type == ACT_8023) &&
			!(at->at_ac->ac_flags &
			  (ACF_ETHER | ACF_IEEE8023 | ACF_SNAP8023)))
			error = ENETUNREACH;
		}
	    } else if ((sin->sin_addr.s_addr == 
		       (((struct arpcom *)ifp)->ac_ipaddr.s_addr)) &&
		       (((struct arpcom *)ifp)->ac_ipaddr.s_addr != 0)) {
		error = looutput(&loif, m, sin);
	    } else if (in_broadcast(sin->sin_addr)) {
		if (!bcmp(ifp->if_name, "atr", 3)) {
		    *desten = ATRBROADCAST;
		    return(-1);
		} else
		    error = broadcast_ippkt(ifp, m);
	    } else {
		/*
		 * There is no cache entry, it is not a loopback packet, and it
		 * is not a broadcast packet. Make a new entry.
		 */
		if ((at = arptnew(&sin->sin_addr)) == NULL) {
		    m_freem_train(m);
		    error = ENOBUFS;
		} else {
		    at->at_hold = m;
		    at->at_ac = (struct arpcom *)ifp;
		    if (++prb_sequence == 0)
			++prb_sequence;
		    at->at_seqno = prb_sequence;
		    /* call appropriate whohas routine */
		    switch (at->at_ac->ac_type) {
		    case ACT_8023:
			    (*whohas8023[at->at_ac->ac_flags &
			    (ACF_ETHER | ACF_IEEE8023 | ACF_SNAP8023)][0].func)
			    ((struct arpcom *)ifp, &sin->sin_addr,
			     at->at_seqno, BROADCAST);
			    break;
		    case ACT_8024:
			    (*whohas8024[(at->at_ac->ac_flags &
			    (ACF_IEEE8024 | ACF_SNAP8024)) >> 4][0].func)
			    ((struct arpcom *)ifp, &sin->sin_addr,
			     at->at_seqno, BROADCAST);
			    break;
		    case ACT_8025:
			    (*whohas8025[(at->at_ac->ac_flags & 
                            ACF_SNAP8025) >> 12][0].func)
			    ((struct arpcom *)ifp, &sin->sin_addr,
			     at->at_seqno, BROADCAST);
			    break;
		    case ACT_FDDI:
			    (*whohasfddi[(at->at_ac->ac_flags &
			    ACF_SNAPFDDI) >> 8][0].func)
			    ((struct arpcom *)ifp, &sin->sin_addr,
			     at->at_seqno, BROADCAST);
			    break;
		    case ACT_ATR:
			    (*whohasatr[1][0].func)
			    ((struct arpcom *)ifp, &sin->sin_addr, &bcastaddr);
			    break;
		    default:
			    panic("arp_resolve : type field");
		    }
		    at->at_rtcnt++;
		}
	    }
	    break;
	default:
	    m_freem_train(m);
	    error = EAFNOSUPPORT;
	    break;
	}
	return(error);
} /* arp_resolve */

/*
 * Called from 10 Mb/s Ethernet interrupt handlers when ether packet type 
 * ETHERTYPE_ARP is received.  Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
arpintr(ac, m, loginfo, type)
	struct arpcom *ac;
	struct mbuf *m;
	int loginfo, type;
{
	struct ifqueue *ifq = &arpintrq;
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
		return(ENOBUFS);
	}
	m->m_quad[1] = type; 
	IF_ENQUEUEIF(ifq, m, (struct ifnet *) ac);
	schednetisr(NETISR_ARP);
}

more_packets(ifq, ifp, m, type)
struct ifqueue *ifq;
struct ifnet **ifp;
struct mbuf **m;
int *type;
{
	int s = splimp();
	IF_DEQUEUEIF(ifq, *m, *ifp);
	splx(s);
	if (*m) {
		*type = (*m)->m_quad[1];
		return(1);
	} else
		return(0);
}

arpinput()
{
        struct arpcom *ac;
        struct mbuf *m;
	int type;
        struct arphdr *ar;

	while (more_packets(&arpintrq, (struct ifnet **) &ac, &m, &type)) {
	    if (m->m_len < sizeof(struct arphdr)) {
		    arpstat.as_badlen++;
		    goto out;
	    }
	    /* 
	     * If we ever support snap8024 we need to check for alignment,
	     * maybe do a pullup.  But for the time being everything in the
	     * struct is two byte aligned. -XXX
	     */
	    ar = mtod(m, struct arphdr *);
	    /* 
	     * drop the arp packets if the hardware type is not ethernet 
	     * or IEEE802.2 (the hardware type is 6 for IEEE802.2 and is
	     * required by SNAPFDDI; see RFC 1188 IP and ARP over FDDI)
	     */
	    if ((ntohs(ar->ar_hrd) != ARPHRD_ETHER) &&
	        (ntohs(ar->ar_hrd) != ARPHRD_IEEE) &&
		(ntohs(ar->ar_hrd) != ARPHRD_ATR) &&
		(ntohs(ar->ar_hrd) != 6)) {
		    arpstat.as_badhrd++;
		    goto out;
	    }
	    if (m->m_len<sizeof(struct arphdr)+(2*ar->ar_hln)+(2*ar->ar_pln)) {
		    arpstat.as_badlen++;
		    goto out;
	    }

	    /* Is this an ATR packet? */
	    if (type == ATR_PKT) {
		/* Yes */
		if ((ar->ar_hln != 4) || (ar->ar_pln != 4))
		    goto out;
		atr_in_arpinput(ac, m, type);
		continue;
	    } /* end if */

	    switch (ntohs(ar->ar_pro)) {
	    case ETHERTYPE_IP:
		    if ((ar->ar_hln != 6) || (ar->ar_pln != 4))
			break;
		    in_arpinput(ac, m, type);
		    continue;
	    case ETHERTYPE_TRAIL:
	    default:
		    arpstat.as_badproto++;
		    break;
	    }
out:
            m_freem(m);
	}
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet. Algorithm is that given 
 * in RFC 826. In addition, a sanity check is performed on the sender 
 * protocol * address, to catch impersonators. We do not handle negotiations 
 * for use of trailer protocol.
 */
static struct ether_hdr ether_repl = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8023_hdr snap8023_repl = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8025_sr_hdr snap8025_repl = {
    IEEE8025_AC, IEEE8025_FC,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snap8024_hdr snap8024_repl = {
    IEEE8024_FC,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_ARP
};
static struct snapfddi_hdr_info snapfddi_repl = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    ETHERTYPE_ARP
};

in_arpinput(ac, m, type)
        struct arpcom *ac;
        struct mbuf *m;
	int type;
{
        struct ether_arp *ea;
	struct mbuf *mhdr = 0;
        struct arptab *at;
        struct in_addr isaddr, itaddr, myaddr;
	char *ptr;
        int  op;
	int flags = 0;
        caddr_t rif_ptr = (caddr_t)m->m_quad[2];

	/*
	 * Drop packet if interface does not have an inet address.
	 */
	if (ac->ac_ipaddr.s_addr == 0)
	    goto out;
        myaddr = ac->ac_ipaddr;
        ea = mtod(m, struct ether_arp *);
        op = ntohs(ea->arp_op);
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)ac->ac_enaddr,
          sizeof (ea->arp_sha)))
                goto out;       /* it's from me, ignore it. */
        bcopy((caddr_t)ea->arp_spa, (caddr_t)&isaddr, sizeof (isaddr));
        bcopy((caddr_t)ea->arp_tpa, (caddr_t)&itaddr, sizeof (itaddr));
        if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)etherbroadcastaddr,
            sizeof (ea->arp_sha))) {
                printf("arp: ether address is broadcast for IP address %x!\n",
                    ntohl(isaddr.s_addr));
                goto out;
        }
        if (isaddr.s_addr == myaddr.s_addr) {
                printf("%s: %s\n",
                        "duplicate IP address!! sent from ethernet address",
                        (char *)ether_sprintf(ea->arp_sha));

                /* send a netwrok management event up for the agent */
                arp_mibevent(ac,ea,NMV_DUPLINKADDRS);

	 	NS_LOG_INFO5( LE_PR_ARP_DUP_IP, NS_LC_ERROR, NS_LS_PROBE, 0,
			4, isaddr.s_addr,
			(ea->arp_sha[0] << 8) | ea->arp_sha[1],
			(ea->arp_sha[2] << 8) | ea->arp_sha[3],
			(ea->arp_sha[4] << 8) | ea->arp_sha[5], 0);
		/*
		 * By setting the target address to my address we'll 
		 * respond to the duplicator, so both console's get the
		 * error.  Maybe we should broadcast a reply to overwrite 
		 * other machine's arp cache?
		 */
                itaddr = myaddr;
                if (op == ARPOP_REQUEST)
                        goto reply;
                goto out;
        }
        ARPTAB_LOOK(at, isaddr.s_addr);
	/*
 	 * Because ARP is symmetric, an incoming request or
	 * reply will refresh state information about existing
	 * cache entries.  Ignore replies from interfaces which are 
	 * different from the one we first arp'ed on.  Reply to requests
	 * of this sort by falling down to 'reply:' below.
	 */
        if ((at) && (ac == at->at_ac)) {
                bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
                    sizeof(ea->arp_sha));
		/* save rif if any */
                if (type == SNAP8025_PKT){
		   if (rif_ptr) {
                      bcopy(rif_ptr, (caddr_t)at->at_rif, 
                            (rif_ptr[0] & 0x1f));
		      at->at_rif[0] = rif_ptr[0] & 0x1f;
		      at->at_rif[1] = rif_ptr[1] ^ 0x80;
                      }
		   else at->at_rif[0] = 0;
		   }
		ARPTAB_FLAGSSET(at, type);
		if (at->at_type != ATT_INVALID)
			flags = RTN_NUKEREDIRS;
		ARPTAB_COMPLETE(at, type);
                if (at->at_hold) {
		    struct mbuf *mb = at->at_hold;
		    at->at_hold = 0;
		    unicast_ippkt(at, mb);
                }
		/*
		 * We implement active/passive response times so that
		 * between any pair of nodes we send out just one set
		 * of request/reply packets and the request initiating
		 * alternates between the two nodes.
		 */
		at->at_unitimer = (op == ARPOP_REQUEST) ? unicast_time :
				(unicast_time << 1);
		/*
		 * If this entry was ever doubtful, we mark it not so now.
		 * we could do this before assigning the value at_flags,
		 * but we may get an infinitely small amount of parallelism
		 * by sending the ip packet before walking through the routing
		 * tables.
		 */
		if (at->at_flags & ATF_DOUBTFUL) {
			gateway.sin_addr = at->at_iaddr;
			rtnotify(&gateway, flags);
			at->at_flags &= ~ATF_DOUBTFUL;
		}
        }
	/*
	 * if this is the first time someone is arping for me, create
	 * a cache entry
	 */
        if ((at == 0) && (itaddr.s_addr == myaddr.s_addr)) {
                if ((at = arptnew(&isaddr)) == NULL) {
			goto out;
		} else {
                        bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
                            sizeof(ea->arp_sha));
			/* save rif if any */
                	if (type == SNAP8025_PKT){
		    	   if (rif_ptr) {
                           bcopy(rif_ptr, (caddr_t)at->at_rif, 
                                (rif_ptr[0] & 0x1f));
		           at->at_rif[0] = rif_ptr[0] & 0x1f;
		           at->at_rif[1] = rif_ptr[1] ^ 0x80;
			   }
		   	   else at->at_rif[0] = 0;
		   	}
			ARPTAB_FLAGSSET(at, type);
			at->at_ac = ac;
			ARPTAB_COMPLETE(at, type);
                }
        }
	if (op != ARPOP_REQUEST)
		goto out;
reply:
     { int error = 0;
        if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
                bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
                    sizeof(ea->arp_sha));
                bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
                    sizeof(ea->arp_sha));
        } else {
		/*
		 * Implement a form of proxy: if a cache entry has
		 * the publish bit set, respond on that machine's
		 * behalf, presumably because it doesn't implement arp.
		 */
                ARPTAB_LOOK(at, itaddr.s_addr);
                if ((at == NULL) || ((at->at_flags & ATF_PUBL) == 0))
                        goto out;
                bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
                    sizeof(ea->arp_sha));
                bcopy((caddr_t)at->at_enaddr, (caddr_t)ea->arp_sha,
                    sizeof(ea->arp_sha));
        }
        bcopy((caddr_t)ea->arp_spa, (caddr_t)ea->arp_tpa,
            sizeof(ea->arp_spa));
        bcopy((caddr_t)&itaddr, (caddr_t)ea->arp_spa,
            sizeof(ea->arp_spa));
        ea->arp_op = htons(ARPOP_REPLY);

	switch (type) {		/* do not broadcast replies */
	case ETHER_PKT:
		bcopy((caddr_t)ea->arp_tha, (caddr_t)ether_repl.destaddr, 6);
		error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
			(caddr_t)&ether_repl, &mhdr);
		switch (error) {
		case NULL:
			break;
		case ENOBUFS:
			goto out;
		default:
			m_freem(mhdr);
			goto out;
		} /* end switch */
		break;
	case SNAP8023_PKT:
		bcopy((caddr_t)ea->arp_tha, (caddr_t)snap8023_repl.destaddr, 6);
		snap8023_repl.length = sizeof(struct ether_arp) + 8;
		error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			(caddr_t)&snap8023_repl, &mhdr);
		switch (error) {
		case NULL:
			break;
		case ENOBUFS:
			goto out;
		default:
			m_freem(mhdr);
			goto out;
		} /* end switch */
		break;
	case SNAP8024_PKT:
		if ((mhdr = m_get(M_DONTWAIT, MT_HEADER)) == NULL) 
			goto out;
		bcopy((caddr_t)ea->arp_tha, (caddr_t)snap8024_repl.destaddr, 6);
		error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT, 
			(caddr_t)&snap8024_repl, mhdr);
		break;
	case SNAP8025_PKT:
		bcopy((caddr_t)ea->arp_tha, (caddr_t)snap8025_repl.destaddr, 6);
		if (rif_ptr) {
                   bcopy(rif_ptr, (caddr_t)snap8025_repl.rif, 
                        (rif_ptr[0] & 0x1f));
		   snap8025_repl.rif[0] = rif_ptr[0] & 0x1f;
		   snap8025_repl.rif[1] = rif_ptr[1] ^ 0x80;
		   }
		else snap8025_repl.rif[0] = 0;
		error = (*ac->ac_build_hdr)(ac, SNAP8025_PKT, 
			(caddr_t)&snap8025_repl, &mhdr);
		switch (error) {
		case NULL:
			break;
		case ENOBUFS:
			goto out;
		default:
			m_freem(mhdr);
			goto out;
		} /* end switch */
		break;
	case SNAPFDDI_PKT:
		if ((mhdr = m_get(M_DONTWAIT, MT_HEADER)) == NULL) 
			goto out;
		ea->arp_hrd = htons(ARPHRD_ETHER); /* required by RFC1188 */
		bcopy((caddr_t)ea->arp_tha, (caddr_t)snapfddi_repl.destaddr, 6);
		mhdr->m_off = MIN_FDDI_HDR_LEN;
		mhdr->m_len = 0;
		error = (*ac->ac_build_hdr)(ac, SNAPFDDI_PKT, 
			(caddr_t)&snapfddi_repl, &mhdr);
		break;
	default:
		panic("in_arpinput: packet type unknown");
		break;
	}
	if (!error) {
		struct mbuf *m1 = mhdr;
		while (m1->m_next)
			m1 = m1->m_next;
		m1->m_next = m;
		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, mhdr);
	}
	else
		m_freem(m);
        return;
    } /* reply */ 
out:
        m_freem(m);
} /* in_arpinput */

/*
 * Called from 10 Mb/s Ethernet interrupt handlers when ether packet type 
 * ETHERTYPE_HP with probe extended type is received or when an ieee 802.3
 * packet with sap IEEESAP_HP and xsap IEEEXSAP_PROBE is received. 
 * If the packet is for probe name then this code queues the packet on the
 * interface and schedules a netisr interrupt for probe name.
 */
static struct etherxt_hdr etherxt_repl = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023xsap_repl = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
probeintr(ac, m, loginfo, type)
	struct arpcom *ac;
	struct mbuf *m;
	int loginfo, type;
{
	struct ifqueue *ifq = &probeintrq;
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
		return(ENOBUFS);
	}
	m->m_quad[1] = type;
	IF_ENQUEUEIF(ifq, m, (struct ifnet *) ac);
	schednetisr(NETISR_PROBE);
}

probeinput()
{
	struct arpcom *ac;
	struct mbuf *m;
	int type;
	struct mbuf *mhdr = 0;
	char * ptr;
        struct prb_hdr *ph;
        struct prb_vnareq *prq;
	struct ether_hdr *lan;
	struct arptab *at = NULL;
	struct etherxt_hdr *eptr;
	struct ieee8023xsap_hdr *iptr;
	u_long addr;
	struct ifqueue *ifq;
	int flags = 0;

	while (more_packets(&probeintrq, (struct ifnet **) &ac, &m, &type)) {
	    int error = 0;
	    lan = mtod(m, struct ether_hdr *);
	    switch(type) {
	    case IEEE8023XSAP_PKT:
		    if (m->m_len < IEEE8023XSAP_HLEN + PRB_PHDRSIZE)
			goto out;
		    m->m_len -= IEEE8023XSAP_HLEN;
		    m->m_off += IEEE8023XSAP_HLEN;
		    iptr = (struct ieee8023xsap_hdr *)lan;
		    ph = (struct prb_hdr *)(iptr + 1);
		    break;
	    case ETHERXT_PKT:
		    if (m->m_len < ETHERXT_HLEN + PRB_PHDRSIZE) 
			goto out;
		    m->m_len -= ETHERXT_HLEN;
		    m->m_off += ETHERXT_HLEN;
		    eptr = (struct etherxt_hdr *)lan;
		    ph = (struct prb_hdr *)(eptr + 1);
		    break;
	    default:	/* we do not handle any other type of probe pkts */
		    goto out;
	    }
	    if (ph->ph_version != PHV_VERSION)
		goto out;
	    switch (ph->ph_type) {
	    case PHT_VNAREQ:			/* VNA request */
		    goto request;
	    case PHT_VNAREP:			/* VNA reply */
		    break;
	    case PHT_USNAMEREP:			/* unsolicited */
	    case PHT_NAMEREQ:			/* name request */
	    case PHT_NAMEREP:			/* name reply */
	    case PHT_PROXYREQ:			/* proxy request */
	    case PHT_PROXYREP:			/* proxy reply */
		    /* 
		     * Pass the packet to probe name.  If nipc isn't
		     * configured into this kernel, prb_ninput is a
		     * stub which simply m_freem's the packet and
		     * returns.
		     */
		    prb_ninput(ac, m, type, (caddr_t)lan->sourceaddr);
		    continue;
	    case PHT_GTWYREQ:		/* where-is-gateway request - unsupp*/
	    case PHT_GTWYREP:		/* where-is-gateway reply - unsupp*/
	    case PHT_NODEDOWN:		/* unsolicited node-down msg - unsupp*/
	    default:
		    probestat.prb_badfield++;
		    goto out;
	    }

	    /* 
	     * Process VNA replies  
	     */
	    if (ph->ph_seq == 0) {
		    probestat.prb_badseq++;
		    goto out;
	    }
	    ARPTAB_SEQ(at, ph->ph_seq);
	    if ((!at) || (ac != at->at_ac)) {
		    probestat.prb_badseq++;
		    goto out;
	    }
	    bcopy((caddr_t)lan->sourceaddr, (caddr_t)at->at_enaddr, 6);
	    if (at->at_type != ATT_INVALID)
		flags = RTN_NUKEREDIRS;
	    ARPTAB_COMPLETE(at, type);
	    ARPTAB_FLAGSSET(at, type);
	    if (at->at_hold) {
		struct mbuf *mb = at->at_hold;
		at->at_hold = 0;
		unicast_ippkt(at, mb);
	    }
	    /*
	     * If this entry was ever doubtful, we mark it not so now.
	     * We could do this before assigning the value at_flags,
	     * but we may get an infinitely small amount of parallelism
	     * by sending the ip packet before walking through the routing
	     * tables.
	     */
	    if (at->at_flags & ATF_DOUBTFUL) {
		    gateway.sin_addr = at->at_iaddr;
		    rtnotify(&gateway, flags);
		    at->at_flags &= ~ATF_DOUBTFUL;
	    }
	    goto out;
request:
	    /* assume everything is in one mbuf */
	    if (m->m_len < (PRB_PHDRSIZE + PRB_VNAREQSIZE)) {
		probestat.prb_lesshdr++;
		goto out;
	    }
	    prq = (struct prb_vnareq *)(ph + 1);
	    if ((ph->ph_len != PRB_PHDRSIZE + PRB_VNAREQSIZE ) ||
		(prq->prq_replen != PRQR_REPLEN) || 
		(prq->prq_dreplen != PRQD_DREPLEN) ||
		(prq->prq_version != PRQV_VNAVERSION) ||
		(prq->prq_domain != HPDSN_DOM) ) {
		probestat.prb_badfield++;
		goto out;
	    }
	    /* 
	     * prq->prq_iaddr is actually a char pointer 
	     * Drop the packet if the interface does not have an inet
	     * address or if it is not for our interface.
	     */
	    bcopy((caddr_t)prq->prq_iaddr, (caddr_t)&addr, 4);
	    if ((ac->ac_ipaddr.s_addr == 0) ||
		(addr != (u_long)ac->ac_ipaddr.s_addr))
		goto out;
	    /*
	     * This &*(%($# protocol assumes that the station address will be 
	     * stripped off the reply packet unlike ARP which has a separate
	     * field for sender hardware address. The reply to a request is
	     * thus returning just the header part of the request packet.
	     */
	    ph->ph_len = PRB_PHDRSIZE;
	    ph->ph_type = PHT_VNAREP;
	    m->m_len = PRB_PHDRSIZE;

	    switch (type) {
	    case ETHERXT_PKT:
		bcopy((caddr_t)lan->sourceaddr, (caddr_t)etherxt_repl.destaddr, 6);
		mhdr = 0;
		error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT, 
			(caddr_t)&etherxt_repl, &mhdr);
		switch (error) {
		case NULL:
			break;
		case ENOBUFS:
			goto out;
		default:
			m_freem(mhdr);
			goto out;
		} /* end switch */
		break;
	    case IEEE8023XSAP_PKT:
		bcopy((caddr_t)lan->sourceaddr,
		      (caddr_t)ieee8023xsap_repl.destaddr, 6);
		ieee8023xsap_repl.length = PRB_PHDRSIZE + 10;
		mhdr = 0;
		error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
			(caddr_t)&ieee8023xsap_repl, &mhdr);
		switch (error) {
		case NULL:
			break;
		case ENOBUFS:
			goto out;
		default:
			m_freem(mhdr);
			goto out;
		} /* end switch */
		break;
	    default:
		goto out;
	    }
	    if (!error) {
		struct mbuf *m1 = mhdr;
		while (m1->m_next) 
			m1 = m1->m_next;
		m1->m_next = m;
		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
		 */
		error = (*ac->ac_output)(&ac->ac_if, mhdr);
	    }
	    else 
		goto out;
	    continue;
out:
	    m_freem(m);
      } /* while more packet */

} /* probeinput */

/*
 * Free an arptab entry.
 */
arptfree(at)
        struct arptab *at;
{

	gateway.sin_addr = at->at_iaddr;
	rtnotify(&gateway, RTN_NUKEREDIRS);
	remque(at);
	FREE(at, M_ATABLE);
}

/*
 * Enter a new address in arptab.
 */
struct arptab *
arptnew(addr)
        struct in_addr *addr;
{
        int n;
        int oldest = -1;
        struct arptab **at_head, *at;
	struct ifaddr *ifa, *ifa_ifwithaddr();
	struct sockaddr sa;
	struct sockaddr_in *sin = (struct sockaddr_in *) &sa; 

	/* 
	 * Sanity check unicast_time. A value too low could be disastrous.
	 */
	unicast_time = (unicast_time < min_time) ? min_time : unicast_time;
	/* 
	 * Do not allow broadcast entries in the arp cache.
	 */
	if (in_broadcast(addr->s_addr))
	    return(NULL);
	/*
	 * Do not allow any arptab entries to be created for one of our
	 * interface addresss.
	 */
	bzero((caddr_t)sin, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr->s_addr;
	if ((ifa = ifa_ifwithaddr(sin)) != NULL)
	    return(NULL);

	MALLOC(at, struct arptab *, sizeof (struct arptab), M_ATABLE, M_NOWAIT);
	if (at == NULL)
		return(NULL);
        at->at_iaddr = *addr;
	at->at_hold = NULL;
	at->at_ac = NULL;
        at->at_flags = ATF_INUSE;
	at->at_state = ATS_INCOMP;
	at->at_type = ATT_INVALID;
	at->at_timer = 0;
	at->at_unitimer = unicast_time;
	at->at_incomptimer = rebroadcast_time;
	at->at_bitcnt = AT_MAXWAIT;
	at->at_rtcnt = 0;
	at->at_seqno = 0;

        insque(at, &arphd[ARPTAB_HASH(addr->s_addr)]);
        return (at);
}

/*
 * Resets an arp/probe vna cache entry so that the resolution of the IP
 * address occurs again. For example this would be called when we have
 * resolved an address using arp on ethernet and the administrator 
 * lan_configured the ACF_ETHER flag down.
 */
void
arptreset(at)
	struct arptab *at;
{
	if (at->at_flags & ATF_DOUBTFUL) {
		gateway.sin_addr = at->at_iaddr;
		rtnotify(&gateway, 0);
	}
	at->at_flags = ATF_INUSE | (at->at_flags & (ATF_PERM | ATF_PUBL |
				ATF_USETRAILERS));
	at->at_state = ATS_INCOMP;
	at->at_type = ATT_INVALID;
	at->at_timer = 0;
	at->at_bitcnt = AT_MAXWAIT;
	at->at_rtcnt = 0;
	if (++prb_sequence == 0)
	    ++prb_sequence;
	at->at_seqno = prb_sequence;
}

/*
 * This routine handles the requests for the arp command to add, delete and
 * show arp/probe vna cache entries.
 */
arpioctl(cmd, data)
        int cmd;
        caddr_t data;
{
        struct arpreq *ar = (struct arpreq *)data;
        struct arptab *at;
        struct sockaddr_in *sin;
	struct ifaddr *ifa = NULL;
        struct arpcom *ac;
	int flags = 0;
        u_short pkt_type;
	int encap = 0, type;

	if (cmd == SIOCTARP)
		return(arptest(cmd, data));
        if (ar->arp_pa.sa_family != AF_INET ||
            ar->arp_ha.sa_family != AF_UNSPEC)
                return (EAFNOSUPPORT);
        sin = (struct sockaddr_in *)&ar->arp_pa;
        ARPTAB_LOOK(at, sin->sin_addr.s_addr);
        if (at == NULL) {               /* not found */
                if (cmd != SIOCSARP) {
                        return (ENXIO);
                }
                if ((ifa = ifa_ifwithnet(&ar->arp_pa)) == NULL) {
                        return (ENETUNREACH);
                }
        }
        switch (cmd) {
        case SIOCSARP:          /* set entry */

                if (at == NULL) {
                        if ((at = arptnew(&sin->sin_addr)) == NULL) {
                                return (EINVAL);
                        }
                        ac = (struct arpcom*)ifa->ifa_ifp;
                        pkt_type = ac->ac_type;
                } else {
                        pkt_type = at->at_ac->ac_type;
                  }

                bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_enaddr,
                    sizeof(at->at_enaddr));
                at->at_flags =  ATF_INUSE | (ar->arp_flags & (ATF_FLAGS));
		if (at->at_type != ATT_INVALID)
		    flags = RTN_NUKEREDIRS;

                switch (pkt_type) { 
		/*
		 * Undocumented feature of the arp command::
		 * We allow the arp command to specify the type of 
		 * encapsulation to be used. If none is specified, then
		 * we default to ether. For non-permanent entries, we
		 * will re-arp/probe if necessary. For permanent entries,
		 * there is a kludge to permit us to try other encapsulation
		 * methods at the bottom of unicast_ippkt().
		 * In essence, it makes sense to use the arp command to set
		 * the type of encapsulation for an entry if and only if more
		 * that one encapsulation method is enabled for the interface.
		 */
 
                case ACT_8023:
		       encap = (ar->arp_flags & 
			       (ATF_COM | ATF_IEEE8023 | ATF_SNAP8023));
		       encap = (encap == 0) ? ATF_COM : encap;
		       if (encap & ATF_COM)
		           type = ATT_ARP;
		       else if (encap & ATF_IEEE8023)
		           type = ATT_IEEE8023;
		       else if (encap & ATF_SNAP8023)
		           type = ATT_SNAP8023;

                       break;

                case ACT_8024:
                         break;
  
                case ACT_FDDI:
                          type = ATT_SNAPFDDI;
                          break;

                case ACT_8025:
                          type = ATT_SNAP8025;
                          /* copyin rif */
                          bcopy((caddr_t)ar->rif, (caddr_t)at->at_rif, RIF_SIZE);
                          break;
 
                 default:
                      break;
              }

              ARPTAB_FLAGSSET(at, type);
	      ARPTAB_COMPLETE(at, type);

		/*
		 * For new entries, we have to provide the interface pointer.
		 */
		if ((at->at_ac == NULL) && (ifa != NULL))
		    at->at_ac = (struct arpcom *)ifa->ifa_ifp;
                at->at_timer = 0;
		if (at->at_flags & ATF_DOUBTFUL) {
			gateway.sin_addr = at->at_iaddr;
			rtnotify(&gateway, flags);
			at->at_flags &= ~ATF_DOUBTFUL;
		}
		if (at->at_hold) {
		    /*
		     * unicast_ippkt() may manipulate at->at_hold
		     */
		    struct mbuf *mb = at->at_hold;
		    at->at_hold = 0;
		    unicast_ippkt(at, mb);
                }
                break;
        case SIOCDARP:          /* delete entry */
                arptfree(at);
                break;
        case SIOCGARP:          /* get entry */
                bcopy((caddr_t)at->at_enaddr, (caddr_t)ar->arp_ha.sa_data,
                    sizeof(at->at_enaddr));
                bcopy((caddr_t)at->at_rif, (caddr_t)ar->rif, RIF_SIZE);
                ar->arp_flags = at->at_flags;
                break;
        }
        return (0);
}

/*
 * Convert Ethernet address to printable (loggable) representation.
 */
char *
ether_sprintf(ap)
        u_char *ap;
{
        int i;
        static char etherbuf[18];
        char *cp = etherbuf;
        static char digits[] = "0123456789abcdef";

        for (i = 0; i < 6; i++) {
                *cp++ = digits[*ap >> 4];
                *cp++ = digits[*ap++ & 0xf];
                *cp++ = ':';
        }
        *--cp = 0;
        return (etherbuf);
}

/*
 * This timer routine is first called by arpinit() and it reschedules itself
 * to be called one every half second via net_timeout().
 */
arptimer()
{
	struct arptab *at;
	struct arptab at_tmp;
	int i;
	u_long bcastaddr = 0;
	u_long atrhwaddr = 0;

	for (i = 0; i < ARPTAB_NB; i++) {
	    for (at = arphd[i].at_next; at != (struct arptab *) &arphd[i];
	      at = at->at_next) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
		    continue;		/* XXX - flags ever == 0 ?? */
		if (++(at->at_timer) >= ((at->at_state == ATS_COMP) ? 
			    arpt_killc : arpt_killi)) {
		    /*
		     * Gross attack: because arptab entries are MALLOC'd,
		     * the call to FREE in arptfree may allow this
		     * memory to be allocated before we can dereference
		     * the at_next pointer.  We cache it first here to
		     * avoid ending up in panic-land.
		     */
		    at_tmp.at_next = at->at_next;
		    arptfree(at);
		    at = &at_tmp;
		    continue;
		}
		switch (at->at_state) {
		case ATS_INCOMP:
		    if (at->at_rtcnt >= (AT_SLOTS * AT_RETRANS)) {
			printf("arptimer: arpentry %d\n",at);
			panic("arp: retransmissions\n");
		    }
		    switch(at->at_ac->ac_type) {
		    case ACT_8023:
			    (*whohas8023[at->at_ac->ac_flags &
			    (ACF_ETHER | ACF_IEEE8023 | ACF_SNAP8023)]
			    [at->at_rtcnt & (AT_SLOTS - 1)].func)
			    (at->at_ac, &at->at_iaddr, at->at_seqno, BROADCAST);
			    break;
		    case ACT_8024:
			    (*whohas8024[(at->at_ac->ac_flags &
			    (ACF_IEEE8024 | ACF_SNAP8024)) >> 4]
			    [at->at_rtcnt & (AT_SLOTS - 1)].func)
			    (at->at_ac, &at->at_iaddr, at->at_seqno, BROADCAST);
			    break;
		    case ACT_8025:
			    (*whohas8025[(at->at_ac->ac_flags &
			    ACF_SNAP8025) >> 12]
			    [at->at_rtcnt & (AT_SLOTS - 1)].func)
			    (at->at_ac, &at->at_iaddr, at->at_seqno, BROADCAST);
			    break;
		    case ACT_FDDI:
			    (*whohasfddi[(at->at_ac->ac_flags & 
			    ACF_SNAPFDDI) >>8]
			    [at->at_rtcnt & (AT_SLOTS - 1)].func)
			    (at->at_ac, &at->at_iaddr, at->at_seqno, BROADCAST);
			    break;
		    case ACT_ATR:
			    (*whohasatr[1]
			    [at->at_rtcnt & (AT_SLOTS - 1)].func)
			    (at->at_ac, &at->at_iaddr, &bcastaddr);
			    break;
		    default:
			    panic("arptimer : type field");
		    }
		    at->at_rtcnt++;
		    if (at->at_rtcnt >= (AT_SLOTS * AT_RETRANS)) {
			at->at_state = ATS_HOLD;
			at->at_incomptimer = rebroadcast_time;
			/*
			 * This entry may never be resolved. Free the mbufs,
			 * or we might hang on to them for arpt_killi secs.
			 */
			if (at->at_hold) {
			    m_freem_train(at->at_hold);
			    at->at_hold = 0;
			}
		    }
		    break;
		case ATS_HOLD:
		    at->at_incomptimer--;
		    if (at->at_incomptimer == 0) {
			/*
			 * No response to our last broadcasts (after hold down
			 * period).  We drop into the incompleted state, but 
			 * initiailize at_rtcnt to AT_SLOTS * AT_RETRANS-1: the
			 * side effect is that we loop through the whohas array 
			 * once broadcasting for the host.  We fall back into 
			 * this state after this pass.
			 */
			at->at_state = ATS_INCOMP;
			at->at_rtcnt = AT_SLOTS * (AT_RETRANS - 1);
			if (++prb_sequence == 0)
			    ++prb_sequence;
			at->at_seqno = prb_sequence;
		    }
		    break;
		case ATS_COMP:
		    at->at_unitimer--;
		    if (at->at_unitimer == 0) {
			at->at_unitimer = unicast_time;
			at->at_bitcnt--;
			if (at->at_bitcnt == 0) {
			    /*
			     * No response to our last unicasts in last 
			     * AT_MAXWAIT minutes.  We drop into the incompleted
			     * state, but initiailize at_rtcnt to AT_SLOTS * 
			     * AT_RETRANS-1: the side effect is that we loop 
			     * through the whohas array 
			     _ hold down state if no response after this pass,
			     * else back into this state if a response comes in
			     */
			    at->at_state = ATS_INCOMP;
			    at->at_rtcnt = AT_SLOTS * (AT_RETRANS - 1);
			    at->at_flags |= ATF_DOUBTFUL;
			    gateway.sin_addr = at->at_iaddr;
			    rtnotify(&gateway, RTN_NUKEREDIRS|RTN_DOUBTFUL);
			} else {
			    /*
			     * Send unicast using last successful resolution 
			     * method
			     */
			    switch (at->at_type) {
			    case ATT_ARP:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)ether_unireq.destaddr,6);
				whohas_ether(at->at_ac, &at->at_iaddr, 0, 
					    UNICAST);
				break;
			    case ATT_ETHERXT:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)etherxt_unireq.destaddr,6);
				if (++prb_sequence == 0)
				    ++prb_sequence;
				at->at_seqno = prb_sequence;
				whohas_etherxt(at->at_ac, &at->at_iaddr,
					    at->at_seqno, UNICAST);
				break;
			    case ATT_IEEE8023:
				bcopy((caddr_t)at->at_enaddr,
				    (caddr_t)ieee8023xsap_unireq.destaddr,6);
				if (++prb_sequence == 0)
				    ++prb_sequence;
				at->at_seqno = prb_sequence;
				whohas_ieee8023(at->at_ac, &at->at_iaddr,
					    at->at_seqno, UNICAST);
				break;
			    case ATT_SNAP8023:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)snap8023_unireq.destaddr,
					    6);
				whohas_snap8023(at->at_ac, &at->at_iaddr, 0,
					    UNICAST);
				break;
			    case ATT_SNAP8024:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)snap8024_unireq.destaddr,
					    6);
				whohas_snap8024(at->at_ac, &at->at_iaddr, 0,
					    UNICAST);
				break;
			    case ATT_SNAP8025:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)snap8025_unireq.destaddr,
					    6);
				bcopy((caddr_t)at->at_rif,
					    (caddr_t)snap8025_unireq.rif, 
					    (at->at_rif[0] & 0x1f));
				whohas_snap8025(at->at_ac, &at->at_iaddr, 0,
					    UNICAST);
				break;
		            case ATT_SNAPFDDI:
				bcopy((caddr_t)at->at_enaddr,
					    (caddr_t)snapfddi_unireq.destaddr,
					    6);
			        whohas_snapfddi(at->at_ac, &at->at_iaddr, 0,
					    UNICAST);
				break;
			    case ATT_ATR:
				bcopy(at->at_enaddr, (u_char *)&atrhwaddr, 4);
				whohas_atr(at->at_ac, &at->at_iaddr, &atrhwaddr);
				break;
			    default:
				break;
			    }
			}
		    }
		    break;
		case ATS_NULL:
		    break;
		}
	    }
	}
	net_timeout(arptimer, (caddr_t)0, hz/2);
}

/*
 * Sends out an IP packet pointed to by m, on to the interface pointed 
 * to by at->at_ac using the address translation available in the arp/probe
 * cache entry at.
 */
static struct ether_hdr ether_ippkt = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct ieee8023_hdr ieee8023_ippkt = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_IP, IEEESAP_IP, IEEECTRL_DEF,
};
static struct snap8023_hdr snap8023_ippkt = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct snap8024_hdr snap8024_ippkt = {
    IEEE8024_FC,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct snap8025_sr_hdr snap8025_ippkt = {
    IEEE8025_AC, IEEE8025_FC,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct snapfddi_hdr_info snapfddi_ippkt = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    ETHERTYPE_IP
};

static int
unicast_ippkt(at, m)
	struct arptab *at;
	struct mbuf *m;
{
	struct arpcom *ac = at->at_ac;
	struct mbuf *mhdr = 0;
	struct mbuf *next_pkt;
	char *ptr;
	int error = 0;

	switch(at->at_type) {
	case ATT_ARP:
	case ATT_ETHERXT:
		if (!(ac->ac_flags & ACF_ETHER))
		    goto err_exit;
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ether_ippkt.destaddr, 6);
	        next_pkt = m->m_act;	
		m->m_act = NULL;
		error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
				(caddr_t)&ether_ippkt, &m);
		m->m_act = next_pkt;
		if (error) {
			m_freem_train(m);
			return(error);
		}

	        error = (*ac->ac_output)((struct ifnet *)ac, m, ETHER_PKT,
		                         (caddr_t)&ether_ippkt);
		break;
	case ATT_IEEE8023:
		if (!(ac->ac_flags & ACF_IEEE8023))
		    goto err_exit;
		bcopy((caddr_t)at->at_enaddr,
		      (caddr_t)ieee8023_ippkt.destaddr, 6);
		ieee8023_ippkt.length = m_len(m) + 3;
	        next_pkt = m->m_act;	
		m->m_act = NULL;
		error = (*ac->ac_build_hdr)(ac, IEEE8023_PKT, 
				(caddr_t)&ieee8023_ippkt, &m);
		m->m_act = next_pkt;
		if (error) {
			m_freem_train(m);
			return(error);
		}

	        error = (*ac->ac_output)((struct ifnet *)ac, m, IEEE8023_PKT,
					 (caddr_t)&ieee8023_ippkt);
		break;
	case ATT_SNAP8023:
		if (!(ac->ac_flags & ACF_SNAP8023))
		    goto err_exit;
		bcopy((caddr_t)at->at_enaddr, 
		      (caddr_t)snap8023_ippkt.destaddr, 6);
		snap8023_ippkt.length = m_len(m) + 8; 
	        next_pkt = m->m_act;	
		m->m_act = NULL;
		error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
				(caddr_t)&snap8023_ippkt, &m);
		m->m_act = next_pkt;
		if (error) {
			m_freem_train(m);
			return(error);
		}

	        error = (*ac->ac_output)((struct ifnet *)ac, m, SNAP8023_PKT,
		                         (caddr_t)&snap8023_ippkt);
		break;
	case ATT_SNAP8024:
		if (!(ac->ac_flags & ACF_SNAP8024))
		    goto err_exit;
		if ((mhdr = m_get(M_DONTWAIT, MT_HEADER)) == NULL) {
		    m_freem_train(m);
		    return(ENOBUFS);
		}
		bcopy((caddr_t)at->at_enaddr, 
			(caddr_t)snap8024_ippkt.destaddr, 6);
		error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT, 
				(caddr_t)&snap8024_ippkt, mhdr);
		if (!error) {
			struct mbuf *mlast = mhdr;
			while (mlast->m_next)
				mlast = mlast->m_next;
			mlast->m_next = m;
			mhdr->m_act = m->m_act;
			m->m_act = NULL;
		}
		else {
			m_freem(mhdr);
			m_freem_train(m);
			return(error);
		}

	        error = (*ac->ac_output)((struct ifnet *)ac, mhdr, SNAP8024_PKT,
		                         (caddr_t)&snap8024_ippkt);
		break;
	case ATT_SNAP8025:
		bcopy((caddr_t)at->at_enaddr,
		      (caddr_t)snap8025_ippkt.destaddr, 6);
		bcopy((caddr_t)at->at_rif,
		      (caddr_t)snap8025_ippkt.rif, (at->at_rif[0] & 0x1f));
	        next_pkt = m->m_act;	
		m->m_act = NULL;
		error = (*ac->ac_build_hdr)(ac, SNAP8025_PKT, 
				(caddr_t)&snap8025_ippkt, &m);
		m->m_act = next_pkt;
		if (error) {
			m_freem_train(m);
			return(error);
		}

	        error = (*ac->ac_output)((struct ifnet *)ac, m, SNAP8025_PKT,
					 (caddr_t)&snap8025_ippkt);
		break;
	case ATT_SNAPFDDI:
		if (!(ac->ac_flags & ACF_SNAPFDDI))
		    goto err_exit;
		bcopy((caddr_t)at->at_enaddr, 
			(caddr_t)snapfddi_ippkt.destaddr, 6);
	        next_pkt = m->m_act;	
		m->m_act = NULL;
		error = (*ac->ac_build_hdr)(ac, SNAPFDDI_PKT, 
				(caddr_t)&snapfddi_ippkt, &m);
		m->m_act = next_pkt;
		if (error) {
                    m_freem_train(m);
                    return(error);
                }
	        error = (*ac->ac_output)((struct ifnet *)ac, m, SNAPFDDI_PKT,
					 (caddr_t)&snapfddi_ippkt);
		break;
	default:
		at->at_hold = m;
		panic("unicast_ippkt: illegal ip pkt");
		break;
	}
			
	return(error);
err_exit:
	if (!(at->at_flags & ATF_PERM))
	    arptreset(at);
	else {
	    /*
	     * If the entry is permanent, this is the only way we will try
	     * another encapsulation method, since the arp command does not
	     * have any interface to specify the encapsulation type. We choose
	     * an encapsulation method that is enabled.
	     */
	    switch(at->at_type) {
	    case ATT_ARP:
	    case ATT_ETHERXT:
	    case ATT_IEEE8023:
	    case ATT_SNAP8023:
		if (ac->ac_flags & ACF_ETHER) {
		    at->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_COM;
		    at->at_type = ATT_ARP;
		} else if (ac->ac_flags & ACF_IEEE8023) {
		    at->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_IEEE8023;
		    at->at_type = ATT_IEEE8023;
		} else if (ac->ac_flags & ACF_SNAP8023) {
		    at->at_flags = (at->at_flags & (ATF_FLAGS)) | ATF_SNAP8023;
		    at->at_type = ATT_SNAP8023;
		} else
		    error = ENETUNREACH;
		break;
	    case ATT_SNAP8024:
		break;
	    case ATT_SNAP8025:
		break;
	    case ATT_SNAPFDDI:
		break;
	    }
	}
	if (at->at_hold)		/* unnecessary check !? -XXX */
	    m_freem_train(at->at_hold);
	at->at_hold = m;
	return(error);
} /* unicast_ippkt */

/*
 * Sends out an IP packet pointed to by m, on to the interface pointed 
 * to by at->at_ac. The IP packet is broadcast and depending up the state
 * of at->ac_flags, we might have to make copies of the packet.
 * This routine is quite gross.
 */
static struct ether_hdr ether_ipbcast = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct ieee8023_hdr ieee8023_ipbcast = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_IP, IEEESAP_IP, IEEECTRL_DEF,
};
static struct snap8023_hdr snap8023_ipbcast = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
};
static struct snap8024_hdr snap8024_ipbcast = {
    IEEE8024_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
/* alignment for ETHERTYPE_IP - should be u_char [2] -XXX */
};
static struct snap8025_sr_hdr snap8025_ipbcast = {
    IEEE8025_AC, IEEE8025_FC,
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* 0xE2 ->> single route, nonbroad ret, 2 byte rif.
       0x70 ->> dir default, longest frm = init val. */
    { 0xE2, 0x70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 
    IEEESAP_SNAP, IEEESAP_SNAP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, ETHERTYPE_IP
/* alignment for ETHERTYPE_IP - should be u_char [2] -XXX */
};
static struct snapfddi_hdr_info snapfddi_ipbcast = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    ETHERTYPE_IP
};


static int
broadcast_ippkt(ac, m)
	struct arpcom *ac;
	struct mbuf *m;
{
	struct mbuf *mhdr=NULL, *mhdr1=NULL, *m1=NULL, *mhdr2=NULL, *m2 = NULL;
	int error = 0;
        /* 
         * NOTE: The following are tuples
         *       (mhdr, m) (mhdr1, m1) and (mhdr2, m2)
         */

        /*
         * broadcast_ippkt is not capable of handling and does not expect
         * to receive a fragment train.
         */
        VASSERT(m->m_act == 0);

	switch (ac->ac_type) {
	case ACT_8023:
	    switch (ac->ac_flags & (ACF_ETHER | ACF_IEEE8023 | ACF_SNAP8023)) {
	    case ACF_ETHER:
		    error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
			    (caddr_t)&ether_ipbcast, &mhdr);
                    break;
	    case ACF_IEEE8023:
		    ieee8023_ipbcast.length = m_len(m) + 3;
		    error = (*ac->ac_build_hdr)(ac, IEEE8023_PKT, 
			    (caddr_t)&ieee8023_ipbcast, &mhdr);
                    break;
	    case (ACF_ETHER | ACF_IEEE8023):
		    ieee8023_ipbcast.length = m_len(m) + 3;
		    error = (*ac->ac_build_hdr)(ac, IEEE8023_PKT, 
			    (caddr_t)&ieee8023_ipbcast, &mhdr);
                    if (error)
                       break;
		    m1 = m_copy(m, 0, M_COPYALL);
		    if (m1) {
		       error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
			       (caddr_t)&ether_ipbcast, &mhdr1);
                    }
		    break;
	    case (ACF_SNAP8023):
		    snap8023_ipbcast.length = m_len(m) + 8;
		    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			    (caddr_t)&snap8023_ipbcast, &mhdr);
                    break;
	    case (ACF_ETHER | ACF_SNAP8023):
		    snap8023_ipbcast.length = m_len(m) + 8;
		    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			    (caddr_t)&snap8023_ipbcast, &mhdr);
                    if (error)
                       break;
		    m1 = m_copy(m, 0, M_COPYALL);
		    if (m1) {
		       error = (*ac->ac_build_hdr)(ac, ETHER_PKT,
		               (caddr_t)&ether_ipbcast, &mhdr1);
                    }
		    break;
	    case (ACF_IEEE8023 | ACF_SNAP8023):
		    snap8023_ipbcast.length = m_len(m) + 8;
		    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			(caddr_t)&snap8023_ipbcast, &mhdr);
                    if (error)
                       break;
		    m1 = m_copy(m, 0, M_COPYALL);
		    if (m1) {
		       ieee8023_ipbcast.length = m_len(m1) + 3;
		       error = (*ac->ac_build_hdr)(ac, IEEE8023_PKT,
			       (caddr_t)&ieee8023_ipbcast, &mhdr1);
                    }
		    break;
	    case (ACF_ETHER | ACF_IEEE8023 | ACF_SNAP8023):
		    snap8023_ipbcast.length = m_len(m) + 8;
		    error = (*ac->ac_build_hdr)(ac, SNAP8023_PKT, 
			(caddr_t)&snap8023_ipbcast, &mhdr);
                    if (error)
                       break;
		    m1 = m_copy(m, 0, M_COPYALL);
		    if (m1) {
		       error = (*ac->ac_build_hdr)(ac, ETHER_PKT, 
			    (caddr_t)&ether_ipbcast, &mhdr1);
                    }
                    if (error)
                       break;
    		    m2 = m_copy(m, 0, M_COPYALL);
		    if (m2) {
		      ieee8023_ipbcast.length = m_len(m2) + 3;
		      error = (*ac->ac_build_hdr)(ac, IEEE8023_PKT, 
			      (caddr_t)&ieee8023_ipbcast, &mhdr2);
                    }
		    break;
	    default:
		    error = ENETUNREACH;
		    break;
	    }
	    break;

	case ACT_8024:
            /*
             * we should try to get rid of this piece of code
             * in a future cycle.
             */
	    if ((mhdr = m_get(M_DONTWAIT, MT_HEADER)) == NULL) {
		break;
	    }
	    switch (ac->ac_flags & (ACF_SNAP8024 | ACF_IEEE8024)) {
	    case ACF_SNAP8024:
		    error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT,
				(caddr_t)&snap8024_ipbcast, mhdr);
		    break;
	    case ACF_IEEE8024:
		    /* should we send ip packets on ieee802.4 -XXX */
		    error = ENETUNREACH;
		    break;
	    case (ACF_SNAP8024 | ACF_IEEE8024):
		    error = (*ac->ac_build_hdr)(ac, SNAP8024_PKT,
				(caddr_t)&snap8024_ipbcast, mhdr);
		    break;
	    default:
		    error = ENETUNREACH;
		    break;
	    }
	    break;
	case ACT_8025:
	    switch (ac->ac_flags & ACF_SNAP8025) {
	    case ACF_SNAP8025:
		    error = (*ac->ac_build_hdr)(ac, SNAP8025_PKT,
			    (caddr_t)&snap8025_ipbcast, &mhdr);
                    break;
	    default:
		    error = ENETUNREACH;
		    break;
	    }
	    break;
	case ACT_FDDI:
	    switch (ac->ac_flags & ACF_SNAPFDDI) {
	    case ACF_SNAPFDDI:
		    error = (*ac->ac_build_hdr)(ac, SNAPFDDI_PKT,
				(caddr_t)&snapfddi_ipbcast, &mhdr);
		    break;
	    default:
		    error = ENETUNREACH;
		    break;
	    }
	    break;
	default:
	    panic("broadcast_ippkt: unsupported interface");
	}

        /*
         * Either we failed in m_copy or in ac_build_hdr()
         * Clean up and exit.  m_freem() returns gracefully if 
         * mbuf ptr passed to it is null
         */
        if (error) {
           m_freem(mhdr);
           m_freem(m);
           m_freem(mhdr1);
           m_freem(m1);
           m_freem(mhdr2);
           m_freem(m2);
           return(error);
        }

	if (!error) {
		struct mbuf *mlast=mhdr;
		while (mlast->m_next)
			mlast = mlast->m_next;
		mlast->m_next = m;
		/* 
		 * Since m_act field is 0, the last two parameters need
		 * not be passed (they are needed only for fragment chains)
                 *
                 * Note: (*ac->ac_output) is reponsible for freeing the mbuf 
                 * chain passed to it
		 */
		error = (*ac->ac_output)((struct ifnet *)ac, mhdr);

                /*
                 * Free mbufs chains if error != 0
                 * Not necessary to optimize on error paths.
                 */
                if (error) {
                   m_freem(mhdr1);
                   m_freem(m1);
                   m_freem(mhdr2);
                   m_freem(m2);
                   return(error);
                }

		if (mhdr1) {
			struct mbuf *mlast1 = mhdr1;
			while (mlast1->m_next)
				mlast1 = mlast1->m_next;
			mlast1->m_next = m1;
                        /*
                         * Note: (*ac->ac_output) is reponsible for freeing 
                         * the mbuf chain passed to it
		         */
			error = (*ac->ac_output)((struct ifnet *)ac, mhdr1);
		}	

                if (error) {
                   m_freem(mhdr2);
                   m_freem(m2);
                   return(error);
                }
 
		if (mhdr2) {
			struct mbuf *mlast2 = mhdr2;
			while (mlast2->m_next)
				mlast2 = mlast2->m_next;
			mlast2->m_next = m2;
                        /*
                         * Note: (*ac->ac_output) is reponsible for freeing 
                         * the mbuf chain passed to it
		         */
			error = (*ac->ac_output)((struct ifnet *)ac, mhdr2);
		}	
	}
	return(error);
} /* broadcast_ippkt */


/*
 * Arp/Probe vna initialization routine called from lanc_setup_stc_logging()
 * in sio/lanc.c which logs the IP and ARP type and SAP with the lan driver.
 * The lanc_setup_stc_logging() routine is the if_control() routine for 
 * csma/cd card and is called indirectly by ip_init(). This routine copies
 * multicast addresses into pre-formatted headers and logs the probe vna
 * multicast address with the card.
 */
arpinit()
{
	struct ifnet *ifp;
	static int arp_init = 0;
	int i;
	int nmget_arp(), nmset_arp();

	if (!arp_init) {
	    /* 
	     * copy multicast address into ieee8023xsap_hdr and etherxt_hdr
	     */
	    bcopy((caddr_t)prb_mcast[PRB_VNA], 
		  (caddr_t)ieee8023xsap_req.destaddr, 6);
	    bcopy((caddr_t)prb_mcast[PRB_VNA], 
		  (caddr_t)etherxt_req.destaddr, 6);

	    /* 
	     * copy ethernet broadcast address into packet headers 
	     */
	    bcopy((caddr_t)etherbroadcastaddr,
		  (caddr_t)ether_req.destaddr, 6);
	    bcopy((caddr_t)etherbroadcastaddr,
		  (caddr_t)snap8023_req.destaddr, 6);
	    bcopy((caddr_t)etherbroadcastaddr,
		  (caddr_t)ether_ipbcast.destaddr, 6);
	    bcopy((caddr_t)etherbroadcastaddr,
		  (caddr_t)ieee8023_ipbcast.destaddr, 6);
	    bcopy((caddr_t)etherbroadcastaddr,
		  (caddr_t)snap8023_ipbcast.destaddr, 6);

	    netproc_assign(NET_ARPIOCTL, arpioctl);
	    netproc_assign(NET_ARP, arpinput);
	    netproc_assign(NET_PROBE, probeinput);
	    arpintrq.ifq_maxlen = IFQ_MAXLEN;
	    probeintrq.ifq_maxlen = IFQ_MAXLEN;
	    ifcontrol(IFC_PRBVNABIND, 0, 0, probeintr);

	    for (i = 0; i < ARPTAB_NB; i++) {
		arphd[i].at_next = (struct arptab *) &arphd[i];
		arphd[i].at_prev = (struct arptab *) &arphd[i];
	    }

	    NMREG(GP_at, nmget_arp, nmset_arp,  mib_unsupp,  mib_unsupp);

	    arp_init++;
	    arptimer();
	}
}

/*
 * This routine copies a formatted packet from user space and queues it
 * on a protocol queue (currently supports only arp and probe) and schedules
 * a netisr event. This enables us to deliver a packet to any protocol input
 * routine which runs off netisr.
 */
arptest(cmd, data)
int cmd;
caddr_t data;
{
        struct arptest *atptr = (struct arptest *)data;
	struct mbuf *m;
        int s;
	struct ifqueue *ifq;
	struct ifnet *ifp;

	if (atptr->ats_flags == ETHERTYPE_ARP)
	    ifq = &arpintrq;
	else if (atptr->ats_flags == IEEEXSAP_PROBE)
	    ifq = &probeintrq;
	else
	    return(EINVAL);
	/* 
	 * Get an mbuf and copyin the packet
	 */
	if (atptr->ats_clen > MLEN)
	    return(EINVAL);
        if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
            return(ENOBUFS);
        copyin((caddr_t)atptr->ats_packet, mtod(m, caddr_t), atptr->ats_clen);
        m->m_len = atptr->ats_mlen;
	m->m_quad[1] = atptr->ats_type;

	s = splimp();
	ifp = ifunit("lan0");
	if (IF_QFULL(ifq) || (ifp == NULL)) {
	    IF_DROP(ifq);
	    m_freem(m);
	    splx(s);
	    return(ENOBUFS);
        }
        IF_ENQUEUEIF(ifq, m, (struct ifnet *)ifp);
	if (atptr->ats_flags == ETHERTYPE_ARP)
	    schednetisr(NETISR_ARP);
	else
	    schednetisr(NETISR_PROBE);
	splx(s);

	return(0);
}

nmget_arp(objid, ubuf, len)
	int 	objid;
	char 	*ubuf;
	int 	*len;
{
    	int 	max_num, count;
    	int 	error = 0;
    	struct arptab 	*at;
    	mib_AtEntry nment;
	int i;

    	switch (objid) {

    	case ID_atNumEnt :	/* return num of completed entries */

	    	if ( *len < sizeof(int) ) 
			return (EMSGSIZE);

		count = 0;
		for (i = 0; i < ARPTAB_NB; i++)
		    for (at = arphd[i].at_next;
			 at != (struct arptab *) &arphd[i]; at = at->at_next)
				if ((at->at_flags & ATF_INUSE) &&
				    (at->at_state == ATS_COMP))
					count++;
		*len = sizeof(int);
		copyout((caddr_t)&count, (caddr_t)ubuf, sizeof(int));
	    	break;

    	case ID_atTable:	/* return all completed entries */

	    	if ((max_num = *len/sizeof(mib_AtEntry)) < 1)
			return (EMSGSIZE);

		count = 0;
		for (i = 0; i < ARPTAB_NB; i++)
		    for (at = arphd[i].at_next;
			 at != (struct arptab *) &arphd[i]; at = at->at_next)
		    	    if ((at->at_flags & ATF_INUSE) && (at->at_ac) &&
				(at->at_state == ATS_COMP)) {
				    nment.IfIndex =
					((struct ifnet *)at->at_ac)->if_index;
				    nment.NetAddr = at->at_iaddr.s_addr;
				    bcopy((caddr_t)at->at_enaddr, 
						    (caddr_t)nment.PhysAddr, 6);
				    copyout((caddr_t)&nment, (caddr_t)ubuf, 
						    sizeof(mib_AtEntry));
				    ubuf += sizeof(mib_AtEntry);
				    count++;
				    max_num--;
		    	    }
		*len = count * sizeof(mib_AtEntry);
	    	break;

    	case ID_atEntry:		/* return specific entry */

	    	if (*len < sizeof(mib_AtEntry))
			return (EMSGSIZE);

		copyin((caddr_t)ubuf, (caddr_t)&nment, 
				sizeof(mib_AtEntry));
		ARPTAB_LOOK(at, nment.NetAddr);
		if ((at) && (at->at_flags & ATF_INUSE) &&
			(at->at_state == ATS_COMP) && (at->at_ac) && 
			(((struct ifnet *)at->at_ac)->if_index == 
					nment.IfIndex)) {
			bcopy((caddr_t)at->at_enaddr, 
					(caddr_t)nment.PhysAddr, 6);
			copyout((caddr_t)&nment, (caddr_t)ubuf,
				sizeof(mib_AtEntry));
			*len = sizeof(mib_AtEntry);
		} else
		    	error = ENOENT;
	    	break;

    	default:
	    	error = EINVAL;
	    	break;
    	}
    	return(error);
}

nmset_arp(objid, ubuf, len)
	int 	objid;
	char 	*ubuf;
	int 	*len;
{
	int error = 0;
	struct arptab *at;
	mib_AtEntry nment;
	int flags = 0;

    	switch (objid) {

	case	ID_atEntry :

		if (*len < sizeof(mib_AtEntry))
	    		return(EMSGSIZE);

		copyin((caddr_t)ubuf, (caddr_t)&nment, 
				sizeof(mib_AtEntry));
		ARPTAB_LOOK(at, nment.NetAddr);
		if ((at) && (at->at_flags & ATF_INUSE) && (at->at_ac) &&
			(((struct ifnet *)at->at_ac)->if_index == 
					nment.IfIndex)) {
			bcopy((caddr_t)nment.PhysAddr, 
				(caddr_t)at->at_enaddr, 6);
			at->at_flags =  (at->at_flags & (ATF_FLAGS)) | ATF_COM;
			if (at->at_type != ATT_INVALID)
			    flags = RTN_NUKEREDIRS;
			ARPTAB_COMPLETE(at, ATT_ARP);
			at->at_timer 	= 0;
			if (at->at_flags & ATF_DOUBTFUL) {
				gateway.sin_addr = at->at_iaddr;
				rtnotify(&gateway, flags);
				at->at_flags &= ~ATF_DOUBTFUL;
			}
			if (at->at_hold) {
			    struct mbuf *mb = at->at_hold;
			    at->at_hold = 0;
			    unicast_ippkt(at, mb);
			}
		} else
			error = ENOENT;
		break;

	default :
		error = EINVAL;
		break;
	}
	return(error);
}

nmget_ipNetToMediaNum()         /* return numer of ARP entries */
{
        int     count=0;
        struct arptab   *at;
        int i;
        int s;

        count = 0;
        s=splnet();
        for (i = 0; i < ARPTAB_NB; i++)
                for (at = arphd[i].at_next;
                        at != (struct arptab *) &arphd[i]; at = at->at_next)
                        if ((at->at_flags & ATF_INUSE) &&
                                (at->at_state == ATS_COMP))
                                count++;

        splx(s);
        return(count);
}

nmget_ipNetToMediaTable(kbuf, nument, klen)
        mib_ipNetToMediaEnt  *kbuf;  /* kernel buffer */
        int     nument;         /* #entries that can fit in kbuf */
        int     *klen;          /* size of user buffer */
{
        int     count;
        struct arptab   *at;
        int i;
	int s;

	extern struct arphd arphd[];

        count = 0;
	s=splnet();
	for (i = 0; ( i < ARPTAB_NB ) && (count < nument); i++)
              for (at = arphd[i].at_next;
                   at != (struct arptab *) &arphd[i]; at = at->at_next)
                         if ((at->at_flags & ATF_INUSE) && (at->at_ac) &&
                              (at->at_state == ATS_COMP)) {
                               kbuf->IfIndex =
                                   ((struct ifnet *)at->at_ac)->if_index;
                               kbuf->NetAddr = at->at_iaddr.s_addr;
                               bcopy((caddr_t)at->at_enaddr, 
				(caddr_t)kbuf->PhysAddr, sizeof(at->at_enaddr));
		   	       /* all of the arp entries are dynamics */
			       kbuf->Type = INTM_DYNAMIC;
                               kbuf ++;
		               count++;
                         }
         *klen = count * sizeof(mib_ipNetToMediaEnt);
	 splx(s);
         return(NULL);
}

nmget_ipNetToMediaEntry(kbuf, klen)
        mib_ipNetToMediaEnt  *kbuf;  /* kernel buffer */
        int     *klen;          /* size of user buffer */
{
        struct arptab   *at;
        int i;
	int s;

	s=splnet();
        ARPTAB_LOOK(at, kbuf->NetAddr);
        if ((at) && (at->at_flags & ATF_INUSE) &&
              (at->at_state == ATS_COMP) && (at->at_ac) &&
              (((struct ifnet *)at->at_ac)->if_index ==
                                  kbuf->IfIndex)) {
               bcopy((caddr_t)at->at_enaddr, (caddr_t)kbuf->PhysAddr,
			sizeof(at->at_enaddr));
               *klen = sizeof(mib_ipNetToMediaEnt);
	       kbuf->Type = INTM_DYNAMIC;
	       splx(s);
	       return(NULL);
         } else {
		splx(s);
        	return(ENOENT);
	 }
}

arp_mibevent(ac, ea, event_id)
        struct arpcom *ac;
        struct ether_arp *ea;
        int event_id;
{

        struct arp_mevinfo {
                u_char  destaddr[6];        /* Ethernet Destination Address) */
                u_char  sourceaddr[6];      /* Ethernet Source Address ) */
                struct in_addr ipaddr;      /* internet address */
        } evinfo;

        struct evrec    *ev;

        MALLOC(ev, struct evrec *, sizeof (struct evrec), M_TEMP, M_NOWAIT);

        if (!ev)
                return(ENOMEM);

        ev->ev.code = event_id;

        bcopy ((caddr_t)ea->arp_sha, (caddr_t)evinfo.destaddr,
                sizeof (evinfo.destaddr));
        bcopy ((caddr_t)ea->arp_tha, (caddr_t)evinfo.sourceaddr,
                sizeof (evinfo.sourceaddr));

        evinfo.ipaddr = ac->ac_ipaddr;


        bcopy ((caddr_t)&evinfo ,(caddr_t)ev->ev.info, sizeof(struct arp_mevinfo
));
        ev->ev.len = sizeof(struct arp_mevinfo);
        nmevenq(ev);
        return;

}

/**************************************************************************
****
**** whohas_atr()
****
***************************************************************************
* Send an ARP packet, asking who has addr on interface ac.
*
* Input parameters:
*
* Return Value:
*
***************************************************************************/
int
whohas_atr(ac, addr, dest)
register struct arpcom *ac;
struct in_addr	*addr;
u_char		*dest;		/* Dest hwaddr - bcast/etc */
{
register struct mbuf *m;
register struct atr_arp *aa;
int		hln, pln;

    hln = 4;
    pln = sizeof (*addr);
    if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
	return (0); /* Return value may be used in the future */
    m->m_len = sizeof(*aa);
    m->m_quad[3] = 0;
    aa = mtod(m, struct atr_arp *);
    aa->arp_hrd = ARPHRD_ATR;
    aa->arp_pro = htons(DR_IPTYPE);
    aa->arp_hln = hln;	/* hardware address length */
    aa->arp_pln = pln;	/* protocol address length */
    aa->arp_op  = htons(ARPOP_REQUEST);
    aa->arp_sha        = htonl(*((dr_addr_t *)ac->ac_enaddr));
    aa->arp_spa.s_addr = ac->ac_ipaddr.s_addr; /* Don't use address from incoming packet */
    aa->arp_tha        = (dr_addr_t)0L; /* Clear address field */
    aa->arp_tpa.s_addr = addr->s_addr;
    (void) atr_arpoutput(ac, m, dest, ARPHRD_ATR);
    return (0); /* Return value may be used in the future */
} /* end whohas_atr() */

/**************************************************************************
****
**** atr_arpoutput()
****
***************************************************************************
* ARP output.
*
* Input parameters:
*
* Return Value:
*
***************************************************************************/
int
atr_arpoutput(ac, m, dest, arphrd)
struct arpcom	*ac;
struct mbuf	*m;
u_char		*dest;
u_short		arphrd;
{
sockaddr_atr_t	sa;

    switch (arphrd) {
    case ARPHRD_ATR:
	sa.sa_family = AF_UNSPEC;
	sa.sa_len = sizeof(sa);
	/*
	 * atr_output() will swap the values in these fields.
	 */
	sa.sa_align            = 0;
	sa.sa_header.dr_length = m->m_len; /* Called routine will add dr_header_t size */
	sa.sa_header.dr_type   = DR_ARPTYPE;
	sa.sa_header.dr_dest   = *((dr_addr_t *)dest);
	sa.sa_header.dr_src    = 0L; /* Clear address field */
	break;
    default:
	printf("atr_arpoutput: can't handle type %d\n", (int)arphrd);
	m_freem(m);
	return (0); /* Return value may be used in the future */
    } /* end switch */
    mtod(m, struct arphdr *)->ar_hrd = htons(arphrd);
    (*ac->ac_output)((struct ifnet *)ac, m, &sa);
    return (0); /* Return value may be used in the future */
} /* end atr_arpoutput() */

/**************************************************************************
****
**** atr_in_arpinput()
****
***************************************************************************
*
* Input parameters:
*
* Return Value:
*
***************************************************************************/
atr_in_arpinput(ac, m, type)
struct arpcom	*ac;
struct mbuf	*m;
int		type;
{
struct atr_arp	*aa;
struct arptab	*at;
struct in_addr	isaddr, itaddr, myaddr;
struct sockaddr_in sin;
char		*ptr;
int		op;
int		flags = 0;
caddr_t		rif_ptr = (caddr_t)m->m_quad[3];

	/*
	 * Drop packet if interface does not have an inet address.
	 */
	if (ac->ac_ipaddr.s_addr == 0) {
	    m_freem(m);
	    return(0);
	}

        myaddr = ac->ac_ipaddr;
        aa = mtod(m, struct atr_arp *);
        op = ntohs(aa->arp_op);
	if (!bcmp((caddr_t)&aa->arp_sha, (caddr_t)ac->ac_enaddr,
          sizeof (aa->arp_sha))) {
                m_freem(m);       /* it's from me, ignore it. */
	    	return(0);
	}
        bcopy((caddr_t)&aa->arp_spa, (caddr_t)&isaddr, sizeof (isaddr));
        bcopy((caddr_t)&aa->arp_tpa, (caddr_t)&itaddr, sizeof (itaddr));
        if (!bcmp((caddr_t)&aa->arp_sha, (caddr_t)etherbroadcastaddr,
            sizeof (aa->arp_sha))) {
                printf("arp: ether address is broadcast for IP address %x!\n",
                    ntohl(isaddr.s_addr));
                m_freem(m);
	    	return(0);
        }
        if (isaddr.s_addr == myaddr.s_addr) {
                printf("%s: %s\n",
                        "duplicate IP address!! sent from atr address",
                        (char *)ether_sprintf(aa->arp_sha));

#ifdef NOCODE
	 	NS_LOG_INFO5( LE_PR_ARP_DUP_IP, NS_LC_ERROR, NS_LS_PROBE, 0,
			4, isaddr.s_addr,
			(aa->arp_sha[0] << 8) | aa->arp_sha[1],
			(aa->arp_sha[2] << 8) | aa->arp_sha[3], 0);
#endif

		/*
		 * By setting the target address to my address we'll 
		 * respond to the duplicator, so both console's get the
		 * error.  Maybe we should broadcast a reply to overwrite 
		 * other machine's arp cache?
		 */
                itaddr = myaddr;
                if (op == ARPOP_REQUEST)
                        goto reply;
                m_freem(m);
		return(0);
        }
        ARPTAB_LOOK(at, isaddr.s_addr);
	/*
 	 * Because ARP is symmetric, an incoming request or
	 * reply will refresh state information about existing
	 * cache entries.  Ignore replies from interfaces which are 
	 * different from the one we first arp'ed on.  Reply to requests
	 * of this sort by falling down to 'reply:' below.
	 */
        if ((at) && (ac == at->at_ac)) {
                bcopy((caddr_t)&aa->arp_sha, (caddr_t)at->at_enaddr,
                    sizeof(aa->arp_sha));
		ARPTAB_FLAGSSET(at, type);
		if (at->at_type != ATT_INVALID)
			flags = RTN_NUKEREDIRS;
		ARPTAB_COMPLETE(at, type);
                if (at->at_hold) {
		    struct mbuf *mb = at->at_hold;
		    at->at_hold = 0;
		    sin.sin_family = AF_INET;
		    sin.sin_addr = isaddr;
		    (*ac->ac_output)((struct ifnet *)ac, mb, &sin);
                }
		/*
		 * We implement active/passive response times so that
		 * between any pair of nodes we send out just one set
		 * of request/reply packets and the request initiating
		 * alternates between the two nodes.
		 */
		at->at_unitimer = (op == ARPOP_REQUEST) ? unicast_time :
				(unicast_time << 1);
		/*
		 * If this entry was ever doubtful, we mark it not so now.
		 * we could do this before assigning the value at_flags,
		 * but we may get an infinitely small amount of parallelism
		 * by sending the ip packet before walking through the routing
		 * tables.
		 */
		if (at->at_flags & ATF_DOUBTFUL) {
			gateway.sin_addr = at->at_iaddr;
			rtnotify(&gateway, flags);
			at->at_flags &= ~ATF_DOUBTFUL;
		}
        }
	/*
	 * if this is the first time someone is arping for me, create
	 * a cache entry
	 */
        if ((at == 0) && (itaddr.s_addr == myaddr.s_addr)) {
                if ((at = arptnew(&isaddr)) == NULL) {
			m_freem(m);
			return(0);
		} else {
                        bcopy((caddr_t)&aa->arp_sha, (caddr_t)at->at_enaddr,
                            sizeof(aa->arp_sha));
			ARPTAB_FLAGSSET(at, type);
			at->at_ac = ac;
			ARPTAB_COMPLETE(at, type);
                }
        }
	if (op != ARPOP_REQUEST) {
		m_freem(m);
		return(0);
	}
reply:
     { int error = 0;
        if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
                bcopy((caddr_t)&aa->arp_sha, (caddr_t)&aa->arp_tha,
                    sizeof(aa->arp_sha));
                bcopy((caddr_t)&ac->ac_enaddr, (caddr_t)&aa->arp_sha,
                    sizeof(aa->arp_sha));
        } else {
		/*
		 * Implement a form of proxy: if a cache entry has
		 * the publish bit set, respond on that machine's
		 * behalf, presumably because it doesn't implement arp.
		 */
                ARPTAB_LOOK(at, itaddr.s_addr);
                if ((at == NULL) || ((at->at_flags & ATF_PUBL) == 0)) {
			m_freem(m);
			return(0);
		}
                bcopy((caddr_t)&aa->arp_sha, (caddr_t)&aa->arp_tha,
                    sizeof(aa->arp_sha));
                bcopy((caddr_t)at->at_enaddr, (caddr_t)&aa->arp_sha,
                    sizeof(aa->arp_sha));
        }
        bcopy((caddr_t)&aa->arp_spa, (caddr_t)&aa->arp_tpa,
            sizeof(aa->arp_spa));
        bcopy((caddr_t)&itaddr, (caddr_t)&aa->arp_spa,
            sizeof(aa->arp_spa));
        aa->arp_op = htons(ARPOP_REPLY);

	atr_arpoutput(ac, m, &aa->arp_tha, ntohs(aa->arp_hrd));
    } /* end reply */
} /* end atr_in_arpinput() */
