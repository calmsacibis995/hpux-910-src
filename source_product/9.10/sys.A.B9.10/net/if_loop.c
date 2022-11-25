/*
 * $Header: if_loop.c,v 1.7.83.6 93/10/28 11:15:45 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/if_loop.c,v $
 * $Revision: 1.7.83.6 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/28 11:15:45 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) if_loop.c $Revision: 1.7.83.6 $ $Date: 93/10/28 11:15:45 $";
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
 *	@(#)if_loop.c	7.4 (Berkeley) 6/27/88 plus MULTICAST 1.1
 */

/*
 * Loopback interface driver for protocol testing and timing.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/subsys_id.h"
#include "../h/net_diag.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../net/cko.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../nipc/nipc_hpdsn.h"

/*   include of atalk.h replace with following declaration for ddpintq */
struct  ifqueue ddpintq;                /* ddp packet input queue */

#include "../h/mib.h"
#include "../netinet/mib_kern.h"

#define	LOMTU	(MCLBYTES+512)

struct	ifnet loif;
int	looutput(), loioctl(), locontrol();

mib_ifEntry     lo_mib;                 /* Mib stats */

#define LOM_SPEED	10000000	/* hmmm.... */

loattach()
{
	register struct ifnet *ifp = &loif;

	ifp->if_name = "lo";
	ifp->if_mtu = LOMTU;
	ifp->if_flags = IFF_LOOPBACK | IFF_UP | IFF_RUNNING;
#ifdef __hp9000s800
	ifp->if_flags |= IFF_CKO;
#endif
#ifdef MULTICAST
	ifp->if_flags |= IFF_MULTICAST;
#endif /* MULTICAST */
	ifp->if_ioctl = loioctl;
	ifp->if_output = looutput;
	ifp->if_control = locontrol;
	if_attach(ifp);
	lo_mibinit(ifp, &lo_mib);
}

static char *lo_desc = "lo0 Hewlett-Packard Software Loopback";
lo_mibinit(ifp, mif)
struct ifnet *ifp;
mib_ifEntry *mif;
{

	bzero(mif, sizeof(mib_ifEntry));
	mif->ifIndex	= ifp->if_index;
	bcopy(lo_desc, mif->ifDescr, 64);
	mif->ifType	= NM_LOOPBACK;
	mif->ifMtu 	= ifp->if_mtu;
	mif->ifSpeed	= LOM_SPEED;
	/* ifPhysaddress is zeroed */
	mif->ifAdmin	= LINK_UP;
	mif->ifOper	= LINK_DOWN;
	/* all counters and gauges are zeroed */
}

locontrol(ifp, cmd, s1, s2, s3)
struct ifnet *ifp;
int cmd;
caddr_t s1, s2, s3;
{
	switch(cmd) {
	    case IFC_IFPATHREPORT:
	    {
		/*
		 * For IFC_IFPATHREPORT, s1 points to the buffer where we
		 * build the path report. The first two bytes of the path
		 * report contains the length of the path report.
		 */
		u_short *sptr, *length;
		struct in_ifaddr *ia;
		u_long ip_addr;

		length = sptr = (u_short *)s1;
		*length = 0;
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
		    if (ia->ia_ifp == ifp)
			break;
		if (ia == (struct in_ifaddr *) 0)	/* not configured */
		    return(EINVAL);
		if ((ip_addr = IA_SIN(ia)->sin_addr.s_addr) == 0)
		    return(EINVAL);

		sptr++;					/* skip length field */
		*sptr++ = ((HPDSN_VER << 8) + (HPDSN_DOM));
		*sptr++ = ip_addr >> 16;
		*sptr++ = ip_addr & 0xFFFF;
		*sptr++ = 8;
		*sptr++ = (NSP_SERVICES << 8) + 2;
		*sptr++ = (NSB_NFT | NSB_IPCSR | NSB_LOOPBACK);
		*sptr++ = (NSP_TRANSPORT << 8) + 2;
		*sptr++ = (NSB_TCPCKSUM | NSB_TCP | NSB_HPPXP);
		*length = 8 * sizeof(u_short);
		return(0);
	    }
	    case IFC_IFNMGET: {
		lo_mib.ifOutQlen = (gauge) ifp->if_snd.ifq_len;
		bcopy((caddr_t)&lo_mib, (caddr_t) s1, sizeof(mib_ifEntry));
		return(0);
	    }
	    case IFC_IFNMEVENT: {
		if_mibevent(ifp, (int) s1, &lo_mib);
		return(0);
	    }
	    case IFC_IFNMSET: 
	    default:
		return(EOPNOTSUPP);
	}
}

looutput(ifp, m0, dst)
	struct ifnet *ifp;
	register struct mbuf *m0;
	struct sockaddr *dst;
{
	int s;
	register struct ifqueue *ifq;
	struct mbuf *m = m0;
	int npkts;
	struct mbuf *last;


	while (m) {
               if (M_HASCL(m)  &&  m->m_cltype == MCL_NFS)
                  break;
               m = m->m_next ? m->m_next : m->m_act;
             }
	s = splimp();
           if (m) {
              /* at least one NFS cluster.  copy entire mbuf chain */
              m = m_copy_nfs_loopback(m0);  /* copy a cluster */
              m_freem_train(m0);
              m0 = m;
              if (!m0) {
                 lo_mib.ifOutDiscards++;
                 lo_mib.ifInDiscards++;
                 splx(s);
                 return (ENOBUFS);
                 }
              }

	/* HP : Support multi-packet trains */
	for (npkts=1, last=m0; last->m_act; last=last->m_act, npkts++);


	ifp->if_opackets	+= npkts;
	lo_mib.ifOutUcastPkts	+= npkts;
	lo_mib.ifInUcastPkts	+= npkts;
	TRACE_PKT(ifp, NS_LS_LOOPBACK, TR_LINK_LOOP, m0, dst->sa_family);
	switch (dst->sa_family) {

	case AF_INET:
		ifq = &ipintrq;
		if (IF_QFULL_TRAIN(ifq, npkts)) {
			lo_mib.ifOutDiscards	+= npkts;
			lo_mib.ifInDiscards	+= npkts;
			IF_DROP_TRAIN(ifq, npkts);
			m_freem_train(m0);
			splx(s);
			return (ENOBUFS);
		}

#ifdef __hp9000s800

		/* Note:  The following condition is for testing purposes.   */
		/* The IFF_NOACC flag is turned on by scaffold so that copy  */
		/* avoidance can be tested in loopback mode.		     */

		if (ifp->if_flags & IFF_NOACC) {
			if (loopback_copy(m0)) {
				lo_mib.ifOutDiscards	+= npkts;
				lo_mib.ifInDiscards	+= npkts;
				IF_DROP_TRAIN(ifq, npkts);
				m_freem_train(m0);
				splx(s);
				return(ENOBUFS);
			}
		}

		/* HP : checksum assist for loopback driver */
		CKO_LOOPBACK(m0);
#endif

		IF_ENQUEUEIF_TRAIN(ifq, m0, ifp, last, npkts);
		schednetisr(NETISR_IP);
		break;

#ifdef APPLETALK
	case AF_APPLETALK:
		ifq = &ddpintq;
		if (IF_QFULL_TRAIN(ifq, npkts)) {
			IF_DROP_TRAIN(ifq, npkts);
			m_freem_train(m0);
			splx(s);
			return (ENOBUFS);
		}
		IF_ENQUEUEIF_TRAIN(ifq, m0, ifp, last, npkts);
		schednetisr(NETISR_DDP);
		break;
#endif /* APPLETALK */

	default:
		splx(s);
		printf("lo%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		m_freem_train(m0);
		return (EAFNOSUPPORT);
	}
	ifp->if_ipackets += npkts;
	splx(s);
	return (0);
}

/*
 * Process an ioctl request.
 */
/* ARGSUSED */
loioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
#ifdef MULTICAST
	register struct ifreq *ifr = (struct ifreq *)data;
#endif MULTICAST
	int error = 0;

	switch (cmd) {
	/*
	 * We always get asked, but in these cases all the work 
	 * is done above us; OK by us...
	 */
	case SIOCSIFFLAGS:
	case SIOCSIFMETRIC:
	case SIOCSIFNETMASK:
		break;


	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		lo_mib.ifOper	= LINK_UP;
		/*
		 * Everything else is done at a higher level.
		 */
		break;

#ifdef MULTICAST
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
			break;
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
#endif MULTICAST

	default:
		error = EINVAL;
	}
	return (error);
}


#ifdef __hp9000s800

loopback_copy(m0)

struct  mbuf *m0;

{
	struct mbuf *m, *m1, *n;

	for (m1 = m0; m1; m1 = m1->m_act) {
		for (m = m1; m; m = m->m_next) {
			caddr_t *p;
			if (!M_HASCL(m) || (m->m_cltype != MCL_OPS))
				continue;

			if (m->m_refcnt == 1) {
				MALLOC(p, caddr_t, MCLBYTES, M_MBUF, M_NOWAIT);
				if (p == 0) {
					printf( "looutput: drop MCL_OPS: malloc1\n" );
					return (ENOBUFS);
				}
				MCLCOPYFROM(m,0,m->m_len,p);
				MCLFREEBUF(m,0);
			} else {
				n = m->m_head;
				if (n == m) {
					printf( "looutput: drop MCL_OPS: head\n" );
					return (ENOBUFS);
				}
				MALLOC(p, caddr_t, MCLBYTES, M_MBUF, M_NOWAIT);
				if (p == 0) {
					printf( "looutput: drop MCL_OPS: malloc2\n" );
					return (ENOBUFS);
				}
				MCLCOPYFROM(m,0,m->m_len,p);
				n->m_refcnt--;
				m->m_head = m;
				m->m_refcnt = 1;
			}
			m->m_off = (int)(p) - (int)(m);
			m->m_cltype = MCL_NORMAL;
			m->m_flags |= MF_NOACC;
			fdcache(ldsid(p), p, m->m_len);
		}
	}
	return(0);
}
#endif
