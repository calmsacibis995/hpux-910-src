/*
 * $Header: ip_igmp.c,v 1.3.83.4 93/09/17 19:03:41 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/ip_igmp.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:03:41 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) ip_igmp.c $Revision: 1.3.83.4 $";
#endif

/*
 * Internet Group Management Protocol (IGMP) routines.
 *
 * Written by Steve Deering, Stanford, May 1988.
 *
 * MULTICAST 1.1
 */

#ifdef MULTICAST

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/protosw.h"

#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_igmp.h"
#include "../netinet/igmp_var.h"

extern struct ifnet loif;

static struct sockproto   igmpproto = { AF_INET, IPPROTO_IGMP };
static struct sockaddr_in igmpsrc   = { AF_INET };
static struct sockaddr_in igmpdst   = { AF_INET };

static int                igmp_timers_are_running = 0;
static u_long             igmp_all_hosts_group;

igmp_init()
{
	/*
	 * To avoid byte-swapping the same value over and over again.
	 */
	igmp_all_hosts_group = htonl(INADDR_ALLHOSTS_GROUP);
}

igmp_input(m, ifp)
	register struct mbuf *m;
	register struct ifnet *ifp;
{
	register struct igmp *igmp;
	register struct ip *ip;
	register int igmplen;
	register int iphlen;
	register int minlen;
	struct in_multi *inm;
	struct in_multistep step;
	struct in_ifaddr *ia;

	++igmpstat.igps_rcv_total;

	ip = mtod(m, struct ip *);
	iphlen = ip->ip_hl << 2;
	igmplen = ip->ip_len;

	/*
	 * Validate lengths
	 */
	if (igmplen < IGMP_MINLEN) {
		++igmpstat.igps_rcv_tooshort;
		m_freem(m);
		return;
	}
	minlen = iphlen + IGMP_MINLEN;
	if ((m->m_off > MMAXOFF || m->m_len < minlen) &&
	    (m = m_pullup(m, minlen)) == 0) {
		++igmpstat.igps_rcv_tooshort;
		return;
	}

	/*
	 * Validate checksum
	 */
	m->m_off += iphlen;
	m->m_len -= iphlen;
	igmp = mtod(m, struct igmp *);
	if (in_cksum(m, igmplen)) {
		++igmpstat.igps_rcv_badsum;
		m_freem(m);
		return;
	}
	m->m_off -= iphlen;
	m->m_len += iphlen;
	ip = mtod(m, struct ip *);

	switch (igmp->igmp_type) {

	case IGMP_HOST_MEMBERSHIP_QUERY:
		++igmpstat.igps_rcv_queries;

		if (ifp == &loif)
			break;

		if (ip->ip_dst.s_addr != igmp_all_hosts_group) {
			++igmpstat.igps_rcv_badqueries;
			m_freem(m);
			return;
		}

		/*
		 * Start the timers in all of our membership records for
		 * the interface on which the query arrived, except those
		 * that are already running and those that belong to the
		 * "all-hosts" group.
		 */
		IN_FIRST_MULTI(step, inm);
		while (inm != NULL) {
			if (inm->inm_ifp == ifp && inm->inm_timer == 0 &&
			    inm->inm_addr.s_addr != igmp_all_hosts_group) {
				inm->inm_timer =
					IGMP_RANDOM_DELAY(inm->inm_addr);
				igmp_timers_are_running = 1;
			}
			IN_NEXT_MULTI(step, inm);
		}

		break;

	case IGMP_HOST_MEMBERSHIP_REPORT:
		++igmpstat.igps_rcv_reports;

		if (ifp == &loif)
			break;

		if (!IN_MULTICAST(ntohl(igmp->igmp_group.s_addr)) ||
		    igmp->igmp_group.s_addr != ip->ip_dst.s_addr) {
			++igmpstat.igps_rcv_badreports;
			m_freem(m);
			return;
		}

		/*
		 * KLUDGE: if the IP source address of the report has an
		 * unspecified (i.e., zero) subnet number, as is allowed for
		 * a booting host, replace it with the correct subnet number
		 * so that a process-level multicast routing demon can
		 * determine which subnet it arrived from.  This is necessary
		 * to compensate for the lack of any way for a process to
		 * determine the arrival interface of an incoming packet.
		 */
		if ((ntohl(ip->ip_src.s_addr) & IN_CLASSA_NET) == 0) {
			IFP_TO_IA(ifp, ia);
			if (ia) ip->ip_src.s_addr = htonl(ia->ia_subnet);
		}

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		IN_LOOKUP_MULTI(igmp->igmp_group, ifp, inm);
		if (inm != NULL) {
			inm->inm_timer = 0;
			++igmpstat.igps_rcv_ourreports;
		}

		break;
	}

	/*
	 * Pass all valid IGMP packets up to any process(es) listening
	 * on a raw IGMP socket.
	 */
	igmpsrc.sin_addr = ip->ip_src;
	igmpdst.sin_addr = ip->ip_dst;
	raw_input(m, &igmpproto,
		    (struct sockaddr *)&igmpsrc, (struct sockaddr *)&igmpdst);
}

igmp_joingroup(inm)
	struct in_multi *inm;
{
	int s;

	NET_SPLNET(s);

	if (inm->inm_addr.s_addr == igmp_all_hosts_group ||
	    inm->inm_ifp == &loif)
		inm->inm_timer = 0;
	else {
		igmp_sendreport(inm);
		inm->inm_timer = IGMP_RANDOM_DELAY(inm->inm_addr);
		igmp_timers_are_running = 1;
	}
	NET_SPLX(s);
}

igmp_leavegroup(inm)
	struct in_multi *inm;
{
	/*
	 * No action required on leaving a group.
	 */
}

igmp_fasttimo()
{
	register struct in_multi *inm;
	struct in_multistep step;
	int s;

	/*
	 * Quick check to see if any work needs to be done, in order
	 * to minimize the overhead of fasttimo processing.
	 */
	if (!igmp_timers_are_running)
		return;

	NET_SPLNET(s);
	igmp_timers_are_running = 0;
	IN_FIRST_MULTI(step, inm);
	while (inm != NULL) {

		if (inm->inm_timer == 0) {
			/* do nothing */
		}
		else if (--inm->inm_timer == 0) {
			igmp_sendreport(inm);
		}
		else {
			igmp_timers_are_running = 1;
		}
		IN_NEXT_MULTI(step, inm);
	}
	NET_SPLX(s);
}

igmp_sendreport(inm)
	struct in_multi *inm;
{
	struct mbuf *m;
	struct igmp *igmp;
	struct ip *ip;
	struct mbuf *mopts;
	struct ip_moptions *imo;
	extern struct socket *ip_mrouter;

	MGET(m, M_DONTWAIT, MT_HEADER);
	if (m == NULL)
		return;
	MGET(mopts, M_DONTWAIT, MT_IPMOPTS);
	if (mopts == NULL) {
		m_free(m);
		return;
	}
	m->m_off = MMAXOFF - IGMP_MINLEN;
	m->m_len = IGMP_MINLEN;
	igmp = mtod(m, struct igmp *);
	igmp->igmp_type   = IGMP_HOST_MEMBERSHIP_REPORT;
	igmp->igmp_code   = 0;
	igmp->igmp_group  = inm->inm_addr;
	igmp->igmp_cksum  = 0;
	igmp->igmp_cksum  = in_cksum(m, IGMP_MINLEN);

	m->m_off -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);
	ip = mtod(m, struct ip *);
	ip->ip_tos        = 0;
	ip->ip_len        = sizeof(struct ip) + IGMP_MINLEN;
	ip->ip_off        = 0;
	ip->ip_p          = IPPROTO_IGMP;
	ip->ip_src.s_addr = INADDR_ANY;
	ip->ip_dst        = igmp->igmp_group;

	imo = mtod(mopts, struct ip_moptions *);
	imo->imo_multicast_ifp  = inm->inm_ifp;
	imo->imo_multicast_ttl  = 1;
	/*
	 * Request loopback of the report if we are acting as a multicast
	 * router, so that the process-level routing demon can hear it.
	 */
	imo->imo_multicast_loop = (ip_mrouter != NULL);

	ip_output(m, (struct mbuf *)0, (struct route *)0,
			IP_MULTICASTOPTS, mopts);

	m_free(mopts);
	++igmpstat.igps_snd_reports;
}

#endif MULTICAST
