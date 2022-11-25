/*
 * $Header: ip_mroute.c,v 1.3.83.4 93/09/17 19:04:00 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/ip_mroute.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:04:00 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) ip_mroute.c $Revision: 1.3.83.4 $";
#endif

/*
 * Procedures for the kernel part of DVMRP,
 * a Distance-Vector Multicast Routing Protocol.
 * (See RFC-1075.)
 *
 * Written by David Waitzman, BBN Labs, August 1988.
 * Modified by Steve Deering, Stanford, February 1989.
 *
 * MROUTING 1.1
 */

#ifdef MULTICAST

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "socketvar.h"
#include "protosw.h"
#include "errno.h"
#include "time.h"
#include "ioctl.h"
#include "syslog.h"
#include "../net/af.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "in.h"
#include "in_pcb.h"
#include "in_var.h"
#include "in_systm.h"
#include "ip.h"
#include "ip_var.h"
#include "igmp.h"
#include "igmp_var.h"
#include "ip_mroute.h"


#ifndef MROUTING
/*
 * Dummy routines and globals used when multicast routing is not compiled in.
 */

struct socket  *ip_mrouter  = NULL;
u_int		ip_mrtproto = 0;

int
ip_mrouter_cmd(cmd, so, m)
	int cmd;
	struct socket *so;
	struct mbuf *m;
{
	return(EOPNOTSUPP);
}

int
ip_mrouter_done()
{
	return(0);
}

int
ip_mforward(ip, ifp)
	struct ip *ip;
	struct ifnet *ifp;
{
	return(0);
}
#else MROUTING


#define INSIZ		sizeof(struct in_addr)
#define	same(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), INSIZ) == 0)

/*
 * Globals.  All but ip_mrouter and ip_mrtproto could be static,
 * except for netstat or debugging purposes.
 */
struct socket  *ip_mrouter  = NULL;
int		ip_mrtproto = IGMP_DVMRP;    /* for netstat only */

struct mbuf    *mrttable[MRTHASHSIZ];
struct vif	viftable[MAXVIFS];
struct mrtstat	mrtstat;
u_int		mrtdebug = 0;	  /* debug level */

/*
 * Private variables.
 */
static vifi_t	   numvifs = 0;
static struct mrt *cached_mrt = NULL;
static u_long	   cached_origin;
static u_long	   cached_originmask;

/*
 * Handle DVMRP setsockopt commands to modify the multicast routing tables.
 */
int
ip_mrouter_cmd(cmd, so, m)
    int cmd;
    struct socket *so;
    struct mbuf *m;
{
   if (cmd != DVMRP_INIT && so != ip_mrouter) return EACCES;

    switch (cmd) {
	case DVMRP_INIT:     return ip_mrouter_init(so);
	case DVMRP_DONE:     return ip_mrouter_done();
	case DVMRP_ADD_VIF:  return add_vif (mtod(m, struct vifctl *));
	case DVMRP_DEL_VIF:  return del_vif (mtod(m, short *));
	case DVMRP_ADD_LGRP: return add_lgrp(mtod(m, struct lgrplctl *));
	case DVMRP_DEL_LGRP: return del_lgrp(mtod(m, struct lgrplctl *));
	case DVMRP_ADD_MRT:  return add_mrt (mtod(m, struct mrtctl *));
	case DVMRP_DEL_MRT:  return del_mrt (mtod(m, struct mrtctl *));
	default:             return EOPNOTSUPP;
    }
}

/*
 * Enable multicast routing
 */
static int
ip_mrouter_init(so)
    struct socket *so;
{
    if (so->so_type != SOCK_RAW ||
	so->so_proto->pr_protocol != IPPROTO_IGMP) return EOPNOTSUPP;

    if (ip_mrouter != NULL) return EADDRINUSE;

    ip_mrouter = so;

    if (mrtdebug)
	log(LOG_DEBUG, "ip_mrouter_init");

    return 0;
}

/*
 * Disable multicast routing
 */
int
ip_mrouter_done()
{
    vifi_t vifi;
    int i;
    struct ifnet *ifp;
    struct ifreq ifr;
    int s;

    NET_SPLNET(s);

    /*
     * For each phyint in use, free its local group list and
     * disable promiscuous reception of all IP multicasts.
     */
    for (vifi = 0; vifi < numvifs; vifi++) {
	if (viftable[vifi].v_lcl_addr.s_addr != 0 &&
	    !(viftable[vifi].v_flags & VIFF_TUNNEL)) {
	    m_freem(viftable[vifi].v_lcl_groups);
	    ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
	    ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr
								= INADDR_ANY;
	    ifp = viftable[vifi].v_ifp;
	    (*ifp->if_ioctl)(ifp, SIOCDELMULTI, (caddr_t)&ifr);
	}
    }
    bzero((caddr_t)viftable, sizeof(viftable));
    numvifs = 0;

    /*
     * Free any multicast route entries.
     */
    for (i = 0; i < MRTHASHSIZ; i++)
        m_freem(mrttable[i]);
    bzero((caddr_t)mrttable, sizeof(mrttable));
    cached_mrt = NULL;

    ip_mrouter = NULL;

    NET_SPLX(s);

    if (mrtdebug)
	log(LOG_DEBUG, "ip_mrouter_done");

    return 0;
}

/*
 * Add a vif to the vif table
 */
static int
add_vif(vifcp)
    register struct vifctl *vifcp;
{
    register struct vif *vifp = viftable + vifcp->vifc_vifi;
    static struct sockaddr_in sin = {AF_INET};
    struct ifaddr *ifa;
    struct ifnet *ifp;
    struct ifreq ifr;
    int error, s;

    if (vifcp->vifc_vifi >= MAXVIFS)  return EINVAL;
    if (vifp->v_lcl_addr.s_addr != 0) return EADDRINUSE;

    /* Find the interface with an address in AF_INET family */
    sin.sin_addr = vifcp->vifc_lcl_addr;
    ifa = ifa_ifwithaddr(&sin);
    if (ifa == 0) return EADDRNOTAVAIL;

    NET_SPLNET(s);

    if (vifcp->vifc_flags & VIFF_TUNNEL) {
	vifp->v_rmt_addr  = vifcp->vifc_rmt_addr;
    }
    else {
	/* Make sure the interface supports multicast */
	ifp = ifa->ifa_ifp;
	if ((ifp->if_flags & IFF_MULTICAST) == 0) {
	    NET_SPLX(s);
	    return EOPNOTSUPP;
	}
	/* Enable promiscuous reception of all IP multicasts from the if */
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
	error = (*ifp->if_ioctl)(ifp, SIOCADDMULTI, (caddr_t)&ifr);
	if (error) {
	    NET_SPLX(s);
	    return error;
	}
    }

    vifp->v_flags     = vifcp->vifc_flags;
    vifp->v_threshold = vifcp->vifc_threshold;
    vifp->v_lcl_addr  = vifcp->vifc_lcl_addr;
    vifp->v_ifp       = ifa->ifa_ifp;

    /* Adjust numvifs up if the vifi is higher than numvifs */
    if (numvifs <= vifcp->vifc_vifi) numvifs = vifcp->vifc_vifi + 1;

    NET_SPLX(s);

    if (mrtdebug)
	log(LOG_DEBUG, "add_vif #%d, lcladdr %x, %s %x, thresh %x",
	    vifcp->vifc_vifi, 
	    ntohl(vifcp->vifc_lcl_addr.s_addr),
	    (vifcp->vifc_flags & VIFF_TUNNEL) ? "rmtaddr" : "mask",
	    ntohl(vifcp->vifc_rmt_addr.s_addr),
	    vifcp->vifc_threshold);
    
    return 0;
}

/*
 * Delete a vif from the vif table
 */
static int
del_vif(vifip)
    vifi_t *vifip;
{
    register struct vif *vifp = viftable + *vifip;
    register vifi_t vifi;
    struct ifnet *ifp;
    struct ifreq ifr;
    int s;

    if (*vifip >= numvifs) return EINVAL;
    if (vifp->v_lcl_addr.s_addr == 0) return EADDRNOTAVAIL;

    NET_SPLNET(s);

    if (!(vifp->v_flags & VIFF_TUNNEL)) {
	m_freem(vifp->v_lcl_groups);
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
	ifp = vifp->v_ifp;
	(*ifp->if_ioctl)(ifp, SIOCDELMULTI, (caddr_t)&ifr);
    }

    bzero((caddr_t)vifp, sizeof (*vifp));

    /* Adjust numvifs down */
    for (vifi = numvifs - 1; vifi >= 0; vifi--)
      if (viftable[vifi].v_lcl_addr.s_addr != 0) break;
    numvifs = vifi + 1;

    NET_SPLX(s);

    if (mrtdebug)
      log(LOG_DEBUG, "del_vif %d, numvifs %d", *vifip, numvifs);

    return 0;
}

/*
 * Add the multicast group in the lgrpctl to the list of local multicast
 * group memberships associated with the vif indexed by gcp->lgc_vifi.
 */
static int
add_lgrp(gcp)
    register struct lgrplctl *gcp;
{
    register struct vif *vifp = viftable + gcp->lgc_vifi;
    register struct mbuf *lp, *prev_lp;
    int s;

    if (mrtdebug)
      log(LOG_DEBUG,"add_lgrp %x on %d",
	  ntohl(gcp->lgc_gaddr.s_addr), gcp->lgc_vifi);

    if (gcp->lgc_vifi >= numvifs) return EINVAL;
    if (vifp->v_lcl_addr.s_addr == 0 ||
       (vifp->v_flags & VIFF_TUNNEL)) return EADDRNOTAVAIL;

    /* try to find a group list mbuf that has free space left */
    for (lp = vifp->v_lcl_groups, prev_lp = NULL
	 ; lp && (lp->m_len / INSIZ) == GRPLSTLEN
         ; prev_lp = lp, lp = lp->m_next)
      ;

    NET_SPLNET(s);

    if (lp == NULL) {	    /* no group list mbuf with free space was found */
	MGET(lp, M_DONTWAIT, MT_MRTABLE);
	if (lp == NULL) {
	  NET_SPLX(s);
	  return ENOBUFS;
	}
	if (prev_lp == NULL)
	  vifp->v_lcl_groups = lp;
	else
	  prev_lp->m_next = lp;
	lp->m_len = 0;
    }

    (mtod(lp,struct grplst *)->gl_gaddr)[lp->m_len / INSIZ].s_addr
      = gcp->lgc_gaddr.s_addr;
    lp->m_len += INSIZ;

    if (gcp->lgc_gaddr.s_addr == vifp->v_cached_group)
	vifp->v_cached_result = 1;

    NET_SPLX(s);

    return 0;
}

/*
 * Delete the the local multicast group associated with the vif
 * indexed by gcp->lgc_vifi.
 * Does not pullup the values from other mbufs in the list unless the
 * current mbuf is totally empty.  This is a ~bug.
 */

static int
del_lgrp(gcp)
    register struct lgrplctl *gcp;
{
    register struct vif *vifp = viftable + gcp->lgc_vifi;
    register u_long i;
    register struct mbuf *lp, *next_lp, *prev_lp = NULL;
    register struct grplst *glp;
    int cnt, s;

    if (mrtdebug)
      log(LOG_DEBUG,"del_lgrp %x on %d",
	  ntohl(gcp->lgc_gaddr.s_addr), gcp->lgc_vifi);

    if (gcp->lgc_vifi >= numvifs) return EINVAL;
    if (vifp->v_lcl_addr.s_addr == 0 ||
       (vifp->v_flags & VIFF_TUNNEL)) return EADDRNOTAVAIL;

    NET_SPLNET(s);

    if (gcp->lgc_gaddr.s_addr == vifp->v_cached_group)
	vifp->v_cached_result = 0;

    /* for all group list mbufs */
    for (lp = vifp->v_lcl_groups; lp; lp = next_lp) {
	/* for all group addrs in an mbuf */
	for (cnt = lp->m_len / INSIZ, i = 0; i < cnt; i++)
	  /* if this is the addr to delete */
	  if (same(&gcp->lgc_gaddr,
		   &mtod(lp,struct grplst *)->gl_gaddr[i])) {
	      lp->m_len -= INSIZ;
	      cnt--;       
	      if (lp->m_len == 0)  {
		  /* the mbuf is now empty */
		  if (prev_lp) {
		      MFREE(lp, prev_lp->m_next);
		  } else {
		      MFREE(lp, vifp->v_lcl_groups);
		  }
	      } else
		/* move all other group addresses down one address */
		/* djw- could use ovbcopy? */
		for (glp = mtod(lp, struct grplst *); i < cnt; i++)
		  glp->gl_gaddr[i] = glp->gl_gaddr[i + 1];
	      NET_SPLX(s);
	      return 0;
	  }
	prev_lp = lp;
	next_lp = lp->m_next;
    }
    NET_SPLX(s);
    return EADDRNOTAVAIL;		/* not found */
}

/*
 * Return 1 if gaddr is a member of the local group list for vifp.
 */
static int
grplst_member(vifp, gaddr)
    struct vif *vifp;
    struct in_addr gaddr;
{
    register int i;
    register u_long addr;
    register struct in_addr *gl;
    register struct mbuf *mb_gl;
    int s;

    mrtstat.mrts_grp_lookups++;

    addr = gaddr.s_addr;
    if (addr == vifp->v_cached_group)
	return (vifp->v_cached_result);

    mrtstat.mrts_grp_misses++;

    for (mb_gl = vifp->v_lcl_groups; mb_gl; mb_gl = mb_gl->m_next) {
      for (gl = mtod(mb_gl, struct in_addr *), i = mb_gl->m_len / INSIZ;
	   i; gl++, i--) {
	if (addr == gl->s_addr) {
	  NET_SPLNET(s);
	  vifp->v_cached_group  = addr;
	  vifp->v_cached_result = 1;
	  NET_SPLX(s);
	  return 1;
	}
      }
    }
    NET_SPLNET(s);
    vifp->v_cached_group  = addr;
    vifp->v_cached_result = 0;
	NET_SPLX(s);
    return 0;
}

/*
 * A simple hash function: returns MRTHASHMOD of the low-order octet of
 * the argument's network or subnet number.
 */ 
static u_long
nethash(in)
    struct in_addr in;
{
    register u_long n;

    n = in_netof(in);
    while ((n & 0xff) == 0) n >>= 8;
    return (MRTHASHMOD(n));
}

/*
 * Add an mrt entry
 */
static int
add_mrt(mrtcp)
    struct mrtctl *mrtcp;
{
    struct mrt *rt, *mrtfind();
    struct mbuf *mb_rt;
    u_long hash;
    int s;

    if (rt = mrtfind(mrtcp->mrtc_origin)) {
	if (mrtdebug)
	  log(LOG_DEBUG,"add_mrt update o %x m %x p %x c %x l %x",
	      ntohl(mrtcp->mrtc_origin.s_addr),
	      ntohl(mrtcp->mrtc_originmask.s_addr),
	      mrtcp->mrtc_parent, mrtcp->mrtc_children, mrtcp->mrtc_leaves);

	/* Just update the route */
	NET_SPLNET(s);
	rt->mrt_parent = mrtcp->mrtc_parent;
	VIFM_COPY(mrtcp->mrtc_children, rt->mrt_children);
	VIFM_COPY(mrtcp->mrtc_leaves,   rt->mrt_leaves);
	NET_SPLX(s);
	return 0;
    }

    if (mrtdebug)
      log(LOG_DEBUG,"add_mrt o %x m %x p %x c %x l %x",
	  ntohl(mrtcp->mrtc_origin.s_addr),
	  ntohl(mrtcp->mrtc_originmask.s_addr),
	  mrtcp->mrtc_parent, mrtcp->mrtc_children, mrtcp->mrtc_leaves);

    NET_SPLNET(s);
    MGET(mb_rt, M_DONTWAIT, MT_MRTABLE);
    if (mb_rt == 0) {
	NET_SPLX(s);
	return ENOBUFS;
    }
    rt = mtod(mb_rt, struct mrt *);

    /*
     * insert new entry at head of hash chain
     */
    rt->mrt_origin     = mrtcp->mrtc_origin;
    rt->mrt_originmask = mrtcp->mrtc_originmask;
    rt->mrt_parent     = mrtcp->mrtc_parent;
    VIFM_COPY(mrtcp->mrtc_children, rt->mrt_children); 
    VIFM_COPY(mrtcp->mrtc_leaves,   rt->mrt_leaves);     
    /* link into table */
    hash = nethash(mrtcp->mrtc_origin);
    mb_rt->m_next  = mrttable[hash];
    mrttable[hash] = mb_rt;

    NET_SPLX(s);

    return 0;
}

/*
 * Delete an mrt entry
 */
static int
del_mrt(origin)
    struct in_addr *origin;
{
    register struct mrt *rt;
    struct mbuf *mb_rt, *prev_mb_rt;
    register u_long hash = nethash(*origin);
    int s;

    if (mrtdebug)
      log(LOG_DEBUG,"del_mrt orig %x",
	  ntohl(origin->s_addr));

    for (prev_mb_rt = mb_rt = mrttable[hash]
	 ; mb_rt
	 ; prev_mb_rt = mb_rt, mb_rt = mb_rt->m_next) {
        rt = mtod(mb_rt, struct mrt *);
	if (origin->s_addr == rt->mrt_origin.s_addr)
	    break;
    }
    if (!mb_rt) {
	return ESRCH;
    }

    NET_SPLNET(s);

    if (rt == cached_mrt)
        cached_mrt = NULL;

    if (prev_mb_rt != mb_rt) {	/* if moved past head of list */
	MFREE(mb_rt, prev_mb_rt->m_next);
    } else			/* delete head of list, it is in the table */
        mrttable[hash] = m_free(mb_rt);

    NET_SPLX(s);

    return 0;
}

/*
 * Find a route for a given origin IP address.
 */
static struct mrt *
mrtfind(origin)
    struct in_addr origin;
{
    register struct mbuf *mb_rt;
    register struct mrt *rt;
    register u_int hash;
    int s;

    mrtstat.mrts_mrt_lookups++;

    if (cached_mrt != NULL &&
	(origin.s_addr & cached_originmask) == cached_origin)
	return (cached_mrt);

    mrtstat.mrts_mrt_misses++;

    hash = nethash(origin);
    for (mb_rt = mrttable[hash]; mb_rt; mb_rt = mb_rt->m_next) {
	rt = mtod(mb_rt, struct mrt *);
	if ((origin.s_addr & rt->mrt_originmask.s_addr) ==
					rt->mrt_origin.s_addr) {
	    NET_SPLNET(s);
	    cached_mrt        = rt;
	    cached_origin     = rt->mrt_origin.s_addr;
	    cached_originmask = rt->mrt_originmask.s_addr;
	    NET_SPLX(s);
	    return (rt);
	}
    }
    return NULL;
}

/*
 * IP multicast forwarding function. This function assumes that the packet
 * pointed to by "ip" has arrived on (or is about to be sent to) the interface
 * pointed to by "ifp", and the packet is to be relayed to other networks
 * that have members of the packet's destination IP multicast group.
 *
 * The packet is returned unscathed to the caller, unless it is tunneled
 * or erroneous, in which case a non-zero return value tells the caller to
 * discard it.
 */

#define IP_HDR_LEN  20	/* # bytes of fixed IP header (excluding options) */
#define TUNNEL_LEN  12  /* # bytes of IP option for tunnel encapsulation  */

int
ip_mforward(ip, ifp)
    register struct ip *ip;
    struct ifnet *ifp;
{
    register struct mrt *rt;
    register struct vif *vifp;
    register int vifi;
    register u_char *ipoptions;
    u_long tunnel_src;

    if (mrtdebug > 1)
      log(LOG_DEBUG, "ip_mforward: src %x, dst %x, ifp %x",
	  ntohl(ip->ip_src.s_addr), ntohl(ip->ip_dst.s_addr), ifp);

    if (ip->ip_hl < (IP_HDR_LEN + TUNNEL_LEN) >> 2 ||
       (ipoptions = (u_char *)(ip + 1))[1] != IPOPT_LSRR ) {
	/*
	 * Packet arrived via a physical interface.
	 */
	tunnel_src = 0;
    }
    else {
	/*
	 * Packet arrived through a tunnel.
	 *
	 * A tunneled packet has a single NOP option and a two-element
	 * loose-source-and-record-route (LSRR) option immediately following
	 * the fixed-size part of the IP header.  At this point in processing,
	 * the IP header should contain the following IP addresses:
	 *
	 *	original source          - in the source address field
	 *	destination group        - in the destination address field
	 *	remote tunnel end-point  - in the first  element of LSRR
	 *	one of this host's addrs - in the second element of LSRR
	 *
	 * NOTE: RFC-1075 would have the original source and remote tunnel
	 *	 end-point addresses swapped.  However, that could cause
	 *	 delivery of ICMP error messages to innocent applications
	 *	 on intermediate routing hosts!  Therefore, we hereby
	 *	 change the spec.
	 */

	/*
	 * Verify that the tunnel options are well-formed.
	 */
	if (ipoptions[0] != IPOPT_NOP ||
	    ipoptions[2] != 11 ||	/* LSRR option length   */
	    ipoptions[3] != 12 ||	/* LSRR address pointer */
	    (tunnel_src = *(u_long *)(&ipoptions[4])) == 0) {
	    mrtstat.mrts_bad_tunnel++;
	    if (mrtdebug)
		log(LOG_DEBUG,
		"ip_mforward: bad tunnel from %u (%x %x %x %x %x %x)",
		ntohl(ip->ip_src.s_addr),
		ipoptions[0], ipoptions[1], ipoptions[2], ipoptions[3],
		*(u_long *)(&ipoptions[4]), *(u_long *)(&ipoptions[8]));
	    return 1;
	}

	/*
	 * Delete the tunnel options from the packet.
	 */
  	ovbcopy((caddr_t)(ipoptions + TUNNEL_LEN), (caddr_t)ipoptions,
	      (unsigned)(dtom(ip)->m_len - (IP_HDR_LEN + TUNNEL_LEN)));
	dtom(ip)->m_len -= TUNNEL_LEN;
	ip->ip_len      -= TUNNEL_LEN;
	ip->ip_hl       -= TUNNEL_LEN >> 2;
    }

    /*
     * Don't forward a packet with time-to-live of zero or one,
     * or a packet destined to a local-only group.
     */
    if (ip->ip_ttl <= 1 ||
	ntohl(ip->ip_dst.s_addr) <= INADDR_MAX_LOCAL_GROUP)
	return (int)tunnel_src;

    /*
     * Don't forward if we don't have a route for the packet's origin.
     */
    if (!(rt = mrtfind(ip->ip_src))) {
	mrtstat.mrts_no_route++;
	if (mrtdebug)
	    log(LOG_DEBUG, "ip_mforward: no route for %u",
				ntohl(ip->ip_src.s_addr));
	return (int)tunnel_src;
    }

    /*
     * Don't forward if it didn't arrive from the parent vif for its origin.
     */
    vifi = rt->mrt_parent;
    if (tunnel_src == 0 ) {
	if ((viftable[vifi].v_flags & VIFF_TUNNEL) ||
	    viftable[vifi].v_ifp != ifp )
	    return (int)tunnel_src;
    }
    else {
	if (!(viftable[vifi].v_flags & VIFF_TUNNEL) ||
	    viftable[vifi].v_rmt_addr.s_addr != tunnel_src )
	    return (int)tunnel_src;
    }

    /*
     * For each vif, decide if a copy of the packet should be forwarded.
     * Forward if:
     *		- the ttl exceeds the vif's threshold AND
     *		- the vif is a child in the origin's route AND
     *		- ( the vif is not a leaf in the origin's route OR
     *		    the destination group has members on the vif )
     *
     * (This might be speeded up with some sort of cache -- someday.)
     */
    for (vifp = viftable, vifi = 0; vifi < numvifs; vifp++, vifi++) {
	if (ip->ip_ttl > vifp->v_threshold &&
	    VIFM_ISSET(vifi, rt->mrt_children) &&
	    (!VIFM_ISSET(vifi, rt->mrt_leaves) ||
	     grplst_member(vifp, ip->ip_dst))) {
		if (vifp->v_flags & VIFF_TUNNEL) tunnel_send(ip, vifp);
		else				 phyint_send(ip, vifp);
	}
    }

    return (int)tunnel_src;
}

static
phyint_send(ip, vifp)
    struct ip *ip;
    struct vif *vifp;
{
    register struct mbuf *mb_copy;
    register struct mbuf *mopts;
    register struct ip_moptions *imo;
    int error;

    mb_copy = m_copy(dtom(ip), 0, M_COPYALL);
    if (mb_copy == NULL)
	return;

    MGET(mopts, M_DONTWAIT, MT_IPMOPTS);
    if (mopts == NULL) {
	m_freem(mb_copy);
	return;
    }

    imo = mtod(mopts, struct ip_moptions *);
    imo->imo_multicast_ifp  = vifp->v_ifp;
    imo->imo_multicast_ttl  = ip->ip_ttl - 1;
    imo->imo_multicast_loop = 1;

    error = ip_output(mb_copy, (struct mbuf *)0, (struct route *)0,
				IP_FORWARDING|IP_MULTICASTOPTS, mopts);
    m_free(mopts);
    if (mrtdebug > 1)
	log(LOG_DEBUG, "phyint_send on vif %d err %d", vifp-viftable, error);
}

static
tunnel_send(ip, vifp)
    struct ip *ip;
    struct vif *vifp;
{
    struct mbuf *mb_copy, *mb_opts;
    register struct ip *ip_copy;
    int error;
    u_char *cp;

    /*
     * Make sure that adding the tunnel options won't exceed the
     * maximum allowed number of option bytes.
     */
    if (ip->ip_hl > (60 - TUNNEL_LEN) >> 2) {
	mrtstat.mrts_cant_tunnel++;
	if (mrtdebug)
	    log(LOG_DEBUG, "tunnel_send: no room for tunnel options, from %u",
					ntohl(ip->ip_src.s_addr));
	return;
    }

    mb_copy = m_copy(dtom(ip), 0, M_COPYALL);
    if (mb_copy == NULL)
      return;
    ip_copy = mtod(mb_copy, struct ip *);
    ip_copy->ip_ttl--;
    ip_copy->ip_dst = vifp->v_rmt_addr;	  /* remote tunnel end-point */
    /*
     * Adjust the ip header length to account for the tunnel options.
     */
    ip_copy->ip_hl  += TUNNEL_LEN >> 2;
    ip_copy->ip_len += TUNNEL_LEN;
    MGET(mb_opts, M_DONTWAIT, MT_HEADER);
    if (mb_opts == NULL) {
	m_freem(mb_copy);
	return;
    }
    /*
     * 'Delete' the base ip header from the mb_copy chain
     */
    mb_copy->m_len -= IP_HDR_LEN;
    mb_copy->m_off += IP_HDR_LEN;
    /*
     * Make mb_opts be the new head of the packet chain.
     * Any options of the packet were left in the old packet chain head
     */
    mb_opts->m_next = mb_copy;
    mb_opts->m_off = MMAXOFF - IP_HDR_LEN - TUNNEL_LEN;
    mb_opts->m_len = IP_HDR_LEN + TUNNEL_LEN;
    /*
     * Copy the base ip header from the mb_copy chain to the new head mbuf
     */
    bcopy((caddr_t)ip_copy, mtod(mb_opts, caddr_t), IP_HDR_LEN);
    /*
     * Add the NOP and LSRR after the base ip header
     */
    cp = mtod(mb_opts, u_char *) + IP_HDR_LEN;
    *cp++ = IPOPT_NOP;
    *cp++ = IPOPT_LSRR;
    *cp++ = 11; /* LSRR option length */
    *cp++ = 8;  /* LSSR pointer to second element */
    *(u_long*)cp = vifp->v_lcl_addr.s_addr;	/* local tunnel end-point */
    cp += 4;
    *(u_long*)cp = ip->ip_dst.s_addr;		/* destination group */

    error = ip_output(mb_opts, (struct mbuf *)0, (struct route *)0,
				IP_FORWARDING, (struct mbuf *)0);
    if (mrtdebug > 1)
	log(LOG_DEBUG, "tunnel_send on vif %d err %d", vifp-viftable, error);
}

#endif MROUTING
#endif MULTICAST
