/*
 * $Header: if_ni.c,v 1.7.83.6 93/10/28 13:37:19 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/if_ni.c,v $
 * $Revision: 1.7.83.6 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/28 13:37:19 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) if_ni.c $Revision: 1.7.83.6 $ $Date: 93/10/28 13:37:19 $";
#endif

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/proc.h"
#include "../h/mib.h"
#include "../h/subsys_id.h"
#include "../h/net_diag.h"

#include "../netinet/mib_kern.h"
#include "../net/af.h"
#include "../net/if.h"
#include "../net/if_ni.h"
#include "../net/netmp.h"
#include "../net/cko.h"

extern int ni_max;
extern struct ni_cb ni_cb[];
struct ni_loginfo ni_loginfo[AF_MAX];

/*
 * Attach all network interfaces, called at system initialization.
 * Also take the opportunity to initialize our address family info
 * which will be updated to reflect reality as protocols bind 
 * (see ni_ifcontrol) with us.
 */

ni_link()   /* for s300 and s700 machine */
{
     ni_init();
}


ni_init()  /* for  s800 machine */
{
        int unit;
        struct ifnet *ifp;
        extern int ni_ifoutput(), ni_ifioctl(), ni_ifcontrol();

        for (unit = 0; unit < ni_max; unit++) {
                ifp = &ni_cb[unit].ni_if;
                ifp->if_name = "ni";
                ifp->if_unit = unit;
                ifp->if_control = ni_ifcontrol;
                ifp->if_ioctl = ni_ifioctl;
                ifp->if_output = ni_ifoutput;
                if_attach(ifp);
		ni_mibinit(ifp, &ni_cb[unit].ni_mib);
        }

	for (unit = 0; unit < AF_MAX; unit++) 
		ni_loginfo[unit].nli_queue = (struct ifqueue *) 0;
}

static char *ni_desc = "ni%d Hewlett Packard Network Interface Pseudo Driver";
ni_mibinit(ifp, mif)
	struct ifnet *ifp;
	mib_ifEntry *mif;
{
	mif->ifIndex = ifp->if_index;
	sprintf ((caddr_t)mif->ifDescr, 64, (caddr_t)ni_desc, ifp->if_unit);
	mif->ifType = NIM_TYPE;
	mif->ifSpeed = NIM_SPEED;
	mif->ifAdmin = LINK_UP;
	mif->ifOper = LINK_DOWN;
}

/*
 * Interface Input:  Demux sockaddr, enqueue packet, and schednetisr.
 * One sanity check: must be able to support the address family, which
 * implies that the protocol must bind with us.
 */
ni_ifinput(m, sa)
	struct mbuf *m;
	struct sockaddr *sa;
{
        int s, npkts;
	struct mbuf *last;

	/* HP : Support multi-packet trains */
	for (npkts=1, last=m; last->m_act; last=last->m_act, npkts ++);

        s = splimp();
	if (IF_QFULL_TRAIN(ni_loginfo[sa->sa_family].nli_queue, npkts)) {
		IF_DROP_TRAIN(ni_loginfo[sa->sa_family].nli_queue, npkts);
#if 0
		np->ni_mib.ifInDiscards += npkts;
#endif
		splx(s);
		m_freem_train(m);
		return (ENOBUFS);
	}
	IF_ENQUEUE_TRAIN(ni_loginfo[sa->sa_family].nli_queue, m, last, npkts);
	schednetisr(ni_loginfo[sa->sa_family].nli_event);
        splx(s);
	return(0);
}

/*
 * Interface Output:  We get passed an ifp which conviently enough is also
 * a ni_cb.  Do one sanity check first, that being our send queue isn't full
 * yet.  The protocol for giving packets to the user states that we give
 * them everything protocols give us, namely the packet and the destination
 * socket address.  They already know the interface by virtue of the device
 * file open.  We stuff the sockaddr into the mbuf chain before the
 * packet if there's space; otherwise we allocate a new mbuf.
 *
 * Once done we can enqueue the packet and deal with the other side:
 * Anyone who is blocked wake's up; any one selecting wake's up, and
 * anyone who wants a signal gets one.
 */
ni_ifoutput(np, m, sa)
	struct ni_cb *np;
	struct mbuf *m;
	struct sockaddr *sa;
{
	struct mbuf *n, *m1, *m0;
	struct ifaddr *ifa;
        int s;
	int npkts;
	struct mbuf *last;

	/* HP : Support multi-packet trains */
	for (npkts=1, last=m; last->m_act; last=last->m_act, npkts ++);

	np->ni_mib.ifOutUcastPkts += npkts;	/* XXX */

	if ((np->ni_if.if_flags & IFF_UP) == 0) {
		m_freem_train(m);
		np->ni_mib.ifOutDiscards += npkts;
		return (ENETUNREACH);
	}

	if (np->ni_bound[sa->sa_family] == 0) {
		m_freem_train(m);
		np->ni_mib.ifOutDiscards += npkts;
		return (EAFNOSUPPORT);
	}

	np->ni_if.if_opackets += npkts;

	/*
	 * We look to see if this is to ourselves. If so, we just
	 * send the packet back where it came from.
	 * - Subbu 5/16/90
	 */
	if (ifa_ifwithaddr(sa)) {
		np->ni_if.if_ipackets += npkts;
		TRACE_PKT(&np->ni_if, NS_LS_NI, TR_LINK_LOOP, m, sa->sa_family);
		return(ni_ifinput(m, sa));
	}

        m0 = m;
	m1 = 0;
        while (m) {
		if (m->m_off <= MMAXOFF &&
	    	    m->m_off >= MMINOFF + sizeof(struct sockaddr)) {
		   m->m_off -= sizeof(struct sockaddr);
		   m->m_len += sizeof(struct sockaddr);
		} else {
		  if ((n = m_get(M_DONTWAIT, MT_SONAME)) == NULL) {
		    s = splimp();
		    IF_DROP_TRAIN(&np->ni_if.if_snd, npkts);
		    np->ni_mib.ifOutDiscards += npkts;
		    splx(s);
		    m_freem_train(m0);
		    return(ENOBUFS);
		  }
		  n->m_len = sizeof(struct sockaddr);
		  n->m_next = m;
		  /* HP : Support multi-packet train */
		  n->m_act = m->m_act;
		  m->m_act = 0;
		  if (last == m)
		    last = n;
                  if (m0 == m)  		/* the head of the mbuf train */
                    m0 = n;
		  else
		    m1->m_act = n;
		  m = n;
		}
		bcopy((caddr_t) sa, mtod(m, caddr_t), sizeof(struct sockaddr));
		m1 = m;
                m = m->m_act;
	}

	s = splimp();
	if (IF_QFULL_TRAIN(&np->ni_if.if_snd, npkts)) {
		IF_DROP_TRAIN(&np->ni_if.if_snd, npkts);
		np->ni_mib.ifOutDiscards += npkts;
		splx(s);
		m_freem_train(m0);
		return (ENOBUFS);
	}
	IF_ENQUEUE_TRAIN(&np->ni_if.if_snd, m0, last, npkts);
	splx(s);

	ni_wakeup(np);

	return (0);
}

/*
 * Interface Ioctl's:  We really don't support any this low, but
 * setting address's and flags isn't an error either.  By convention we
 * always get asked if its OK; for the most part, it is.
 */
ni_ifioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	struct ifreq *data;
{
	int error = 0, flags;
	struct ni_cb *np = (struct ni_cb *)ifp;

	if (!(np->ni_flags & NIF_INUSE)) 
		error = EINVAL;
	else switch (cmd) {
		case SIOCSIFFLAGS:
			if (data->ifr_flags & IFF_UP && ifp->if_mtu == 0)
				return (EINVAL);
			if ((data->ifr_flags ^ ifp->if_flags) & IFF_UP) {
				ni_wakeup((struct ni_cb *)ifp);
				if_mibevent(ifp, NMV_LINKUP, &np->ni_mib);
			}
			break;

		case SIOCSIFMETRIC:
		case SIOCSIFDSTADDR:
		case SIOCSIFBRDADDR:
		case SIOCSIFNETMASK:
		case SIOCSIFADDR:
			break;

		default:
			error = EINVAL;

	}
	return (error);
}

/*
 * Interface Control:  The only supported command is for a protocol to
 * bind with us.  We accomplish this end by stashing away the given
 * info in our loginfo array.  Any protocol which doesn't bind will
 * not recieve packets from us!
 */
ni_ifcontrol(ifp, cmd, p1, p2, p3)
	struct ifnet *ifp;
	int cmd;
	char *p1, *p2, *p3; /* af, netisr_event, input_queue; */
{
	struct ni_cb *np = (struct ni_cb *)ifp;
	switch (cmd) {
		case IFC_IFBIND:
			ni_loginfo[(int) p1].nli_event = (int) p2;
			ni_loginfo[(int) p1].nli_queue = (struct ifqueue *) p3;
			break;

		case IFC_IFNMGET: {
			mib_ifEntry *mif = &np->ni_mib;

   			mif->ifOutQlen = ifp->if_snd.ifq_len;

   			bcopy((caddr_t)mif, (caddr_t)p1, 
			    sizeof (mib_ifEntry));

   			return (0);
		}

		case IFC_IFNMSET: {
			int if_status = ((mib_ifEntry *)p1)->ifAdmin;

			if (!(np->ni_flags & NIF_INUSE)) 
				return(EINVAL);
			else switch (if_status) {
				case LINK_DOWN:
					np->ni_mib.ifAdmin = if_status;
					if (ifp->if_flags & IFF_UP) {
						if_mibevent(ifp, NMV_LINKDOWN, 
						    &np->ni_mib);
						ifp->if_flags &= ~IFF_UP;
					}
					return (0);

				case LINK_UP:
					if (ifp->if_mtu == 0)
						return (EINVAL);
					np->ni_mib.ifAdmin = if_status;
					if (!(ifp->if_flags & IFF_UP)) {
						ni_wakeup(np);
						if_mibevent(ifp, NMV_LINKDOWN, 
						    &np->ni_mib);
						ifp->if_flags |= IFF_UP;
					}
					return (0);
				
				default:
					return (EINVAL);
			}
		}

		case IFC_IFNMEVENT:
			if_mibevent(ifp, (int) p1, &np->ni_mib);
			break;

		default:
			break;
	}
	return(0);
}

/*
 * Open a network interface.  Loop through the array of network interface
 * control blocks, searching for an interface that is not in use.  Upon
 * finding an unused interface, initialize millions of ifnet structure
 * fields, mark the interface as "in use" and return success.  If all
 * network interfaces are in use (we're SOL), so return an error.
 */
ni_open(dev, flag, minnum)
	dev_t dev;
	int flag, *minnum;
{
	int s, af;
	struct ni_cb *np;
	struct ifnet *ifp;

	if (!suser())
		return (u.u_error);

 	NET_SPLNET(s);
	for (np = ni_cb; np < &ni_cb[ni_max]; np++) {	/* XXX */
		if (np->ni_flags & NIF_INUSE)
			continue;
		*minnum = np->ni_if.if_unit;
		ifp = &np->ni_if;
		ifp->if_mtu = 0;
		ifp->if_flags = IFF_RUNNING;
		ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;
		ifp->if_snd.ifq_drops = 0;
		ifp->if_ierrors = 0;
		ifp->if_ipackets = 0;
		ifp->if_oerrors = 0;
		ifp->if_opackets = 0;
		ifp->if_collisions = 0;
		np->ni_flags = NIF_INUSE;
		for (af = 0; af < AF_MAX; af++)
			np->ni_bound[af] = 0;
 		NET_SPLX(s);
		return (0);
	}
 	NET_SPLX(s);

	return (ENXIO);
}

/*
 * Close a network interface.  It's time to go home.  Any packets that remain
 * on the output queue at this point are flushed.  Mark the interface as
 * not up or running, and mark the network interface control block as no
 * longer in use.
 */
ni_close(dev, flag)
	dev_t dev;
	int flag;
{
	int s;
	struct ni_cb *np;
	struct mbuf *m;

	np = &ni_cb[minor(dev)];

 	NET_SPLNET(s);
	if_qflush(&np->ni_if.if_snd);
	np->ni_if.if_flags &= ~(IFF_UP|IFF_RUNNING);
	np->ni_flags &= ~NIF_INUSE;
	if_mibevent((struct ifnet *) np, NMV_LINKDOWN, &np->ni_mib);
	ni_mibinit(&np->ni_if, &np->ni_mib);
 	NET_SPLX(s);

	return (0);
}

/*
 * Read a packet from the interface's output queue.  If no packet is currently
 * enqueued, then we need to decide what to do about it.  If non-blocking mode
 * is enabled, then it's real easy; just leave.  Otherwise, we go to sleep
 * until ni_ifoutput() wakes us up.  Eventually a packet will show up.  At this
 * point we need to do some checking.  The must read one entire packet at a
 * time.  If the read count is less than the length of the packet (plus the
 * sockaddr header we added in ni_output()), then we return EMSGSIZE,
 * indicating that the user screwed up.  There is really no excuse for this
 * type of error, since the maximum-sized read that can ever occur is the
 * interface's MTU plus the sizeof(struct sockaddr).  At any rate, after
 * we're sure that the read size is <= the packet length + header, then all
 * we do is uiomove the packet into user space and return.
 */
ni_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int len, error, s, s2;
	struct ni_cb *np;
	struct mbuf *m, *n;
	short af;

	np = &ni_cb[minor(dev)];

	NET_SPLNET(s);
	for (;;) {
		s2 = splimp();
		IF_DEQUEUE(&np->ni_if.if_snd, m);
		splx(s2);
		if (m != NULL)
			break;
		if (uio->uio_fpflags & FNDELAY) {
			NET_SPLX(s);
			return (0);
		}
		if (uio->uio_fpflags & FNBLOCK) {
			NET_SPLX(s);
			return (EAGAIN);
		}
		if (np->ni_flags & NIF_NBIO) {
			NET_SPLX(s);
			return (EWOULDBLOCK);
		}
		np->ni_flags |= NIF_WAIT;
		if (sleep(&np->ni_if, PCATCH|PZERO+1)) {
			NET_SPLX(s);
			return (EINTR);
		}
	}
	NET_SPLX(s);
	len = m_len(m);
	if (uio->uio_resid < len) {
		NET_SPLNET(s);
		np->ni_if.if_oerrors++;
		np->ni_mib.ifOutErrors++;
		NET_SPLX(s);
		m_freem(m);
		return (EMSGSIZE);
	}
	for (n = m; n != NULL; n = n->m_next) {
		if (n->m_len == 0)
			continue;
		error = uiomove(mtod(n, caddr_t), n->m_len, UIO_READ, uio);
		if (error != 0) {
			np->ni_mib.ifOutErrors++;
			m_freem(m);
			return (error);
		}
	}
	np->ni_mib.ifOutOctets += len;
	af = (mtod(m, struct sockaddr *))->sa_family;
	m_adj(m, sizeof(struct sockaddr));
	TRACE_PKT(&np->ni_if, NS_LS_NI, TR_LINK_OUTBOUND, m, af);
	m_freem(m);
	return (0);
}

/*
 * Write a packet to the network interface.  The write count must be large
 * enough to contain a sockaddr header plus data, yet small enough to honor
 * the interface's MTU.  If it doesn't fall within these limits, then we
 * return EMSGSIZE, indicating a bad message length.  Next, read the sockaddr
 * header into a local structure.  Finally, uiomove the packet into mbufs and
 * call ni_ifinput() to deliver the packet to the protocols.
 */
ni_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int resid, len, error, s;
	struct ni_cb *np;
	struct mbuf *m, *n;
	struct sockaddr sa;

	np = &ni_cb[minor(dev)];

	resid = uio->uio_resid - sizeof(struct sockaddr);
	if (resid <= 0 || resid > np->ni_if.if_mtu) {
		np->ni_if.if_ierrors++;
		np->ni_mib.ifInErrors++;
		return (EMSGSIZE);
	}

	error = uiomove(&sa, sizeof(struct sockaddr), UIO_WRITE, uio);
	if (error != 0) {
		np->ni_mib.ifOutErrors++;
		return (error);
	}

	if (sa.sa_family < 0 || sa.sa_family >= AF_MAX) {
		np->ni_mib.ifInErrors++;
		return (EINVAL);
	}

	if (np->ni_bound[sa.sa_family] == 0) {
		np->ni_mib.ifInUnknownProtos++;
		return (EAFNOSUPPORT);
	}

	np->ni_mib.ifInOctets += resid;

	m = n = m_get(M_WAIT, MT_DATA);
	n->m_len = 0;
	while ((resid = uio->uio_resid) != 0) {
		len = (M_HASCL(n) ? MCLBYTES : MLEN) - n->m_len;
		if (len == 0) {
			n = n->m_next = m_get(M_WAIT, MT_DATA);
			MCLGET(n);
			len = n->m_len;
			n->m_len = 0;
		}
		len = (len < resid) ? len : resid;
		error = uiomove(mtod(n, caddr_t) + n->m_len, len,UIO_WRITE,uio);
		if (error != 0) {
			np->ni_mib.ifOutErrors++;
			m_freem(m);
			return (error);
		}
		n->m_len += len;
	}

	NET_SPLNET(s);
	if ((np->ni_if.if_flags & IFF_UP) == 0) {
		m_freem(m);
		np->ni_mib.ifInDiscards++;
		NET_SPLX(s);
		return (EACCES);
	}
	np->ni_if.if_ipackets++;
	TRACE_PKT(&np->ni_if, NS_LS_NI, TR_LINK_INBOUND, m, sa.sa_family);
	ni_ifinput(m, &sa);
	NET_SPLX(s);

	np->ni_mib.ifInUcastPkts++;	/* XXX */

	return (error);
}

/*
 * Network interface ioctl handler.  Various and sundry control functions...
 */
ni_ioctl(dev, command, data)
	dev_t dev;
	int *data;
{
	int error = 0, flags, s;
	struct mbuf *m;
	struct ni_cb *np = &ni_cb[minor(dev)];
	struct ifnet *ifp = &np->ni_if;

	NET_SPLNET(s);
	switch (command) {
		case FIOASYNC:
			if (*data)
				np->ni_flags |= NIF_ASYNC;
			else
				np->ni_flags &= ~NIF_ASYNC;
			break;

#ifdef hp9000s800
                case FIOSNBIO:
#endif
		case FIONBIO:
			if (*data)
				np->ni_flags |= NIF_NBIO;
			else
				np->ni_flags &= ~NIF_NBIO;
			break;

		case FIONREAD:
			if (m = np->ni_if.if_snd.ifq_head)
				*data = m_len(m);
			else
				*data = 0;
			break;

		case NIOCBIND:
			if (*data < 0 || *data >= AF_MAX)
				error = EINVAL;
			else
				if (ni_loginfo[*data].nli_queue == NULL)
					error = EAFNOSUPPORT;
				else
					np->ni_bound[*data] = 1;
			break;

		case NIOCBOUND:
			if (*data < 0 || *data >= AF_MAX)
				error = EINVAL;
			else
				if (np->ni_bound[*data] == 0)
					error = EAFNOSUPPORT;
			break;

		case NIOCGFLAGS:
			*data = ifp->if_flags;
			break;

		case NIOCSFLAGS:
			if (*data & IFF_UP && ifp->if_mtu == 0)
				return (EINVAL);
			if ((*data ^ ifp->if_flags) & IFF_UP) {
				ni_wakeup((struct ni_cb *)ifp);
				if_mibevent(ifp, NMV_LINKUP, &np->ni_mib);
			}
			ifp->if_flags = *data | IFF_RUNNING;
			break;

		case NIOCGMTU:
			*data = ifp->if_mtu;
			break;

		case NIOCSMTU:
			if (ifp->if_flags & IFF_UP || *data <= 0)
				error = EINVAL;
			else
				np->ni_mib.ifMtu = ifp->if_mtu = *data;
			break;

		case NIOCGPGRP:
			*data = np->ni_pgrp;
			break;

		case NIOCSPGRP:
			np->ni_pgrp = *data;
			break;

		case NIOCGQLEN:
			*data = ifp->if_snd.ifq_maxlen;
			break;

		case NIOCUNBIND:
			if (*data < 0 || *data >= AF_MAX)
				error = EINVAL;
			else
				np->ni_bound[*data] = 0;
			break;

		case NIOCSQLEN:
			if (ifp->if_flags & IFF_UP || *data <= 0)
				error = EINVAL;
			else
				ifp->if_snd.ifq_maxlen = *data;
			break;

		case NIOCGUNIT:
			*data = ifp->if_unit;
			break;
		
		case NIOCSMDESCR:
			bcopy((caddr_t) data, np->ni_mib.ifDescr, 64);/* XXX */
			break;

		case NIOCSMTYPE:
			np->ni_mib.ifType = *data;	/* XXX */
			break;

		case NIOCSMSPEED:
			if (*data < 0)
				return(EINVAL);
			else
				np->ni_mib.ifSpeed = *data;	/* XXX */
			break;

		default:
			error = EINVAL;
	}
	NET_SPLX(s);

	return (error);
}

/*
 * Select on a network interface.  If we're selecting for read, then we
 * return true if there is a packet on the output queue.  If we're selecting
 * for write, then we return true only if the interface is up.  If we're
 * selecting for exceptional conditions, then we return true if the network
 * interface is down (i.e. not IFF_UP).  Writable and exceptional conditionals
 * are conversely related.
 */
ni_select(dev, which)
	dev_t dev;
{
	struct ni_cb *np;
	int s;

	np = &ni_cb[minor(dev)];

	NET_SPLNET(s);
	switch (which) {
		case FREAD:
			if (np->ni_if.if_snd.ifq_len != 0) {
				NET_SPLX(s);
				return (1);
			}
			break;

		case FWRITE:
			if (np->ni_if.if_flags & IFF_UP) {
				NET_SPLX(s);
				return (1);
			}
			break;

		case 0:
			if ((np->ni_if.if_flags & IFF_UP) == 0) {
				NET_SPLX(s);
				return (1);
			}
			break;
	}

	if (np->ni_sel != u.u_procp)
		if (np->ni_sel == NULL)
			np->ni_sel = u.u_procp;
		else
			np->ni_flags |= NIF_COLL;

	NET_SPLX(s);
	return (0);
}

/*
 * Network Interface driver wakeup: This routine is called when something
 * interesting has happened, and we want to alert the upper part of the
 * driver (and possibly the application).  If the upper half of the driver
 * is blocked waiting for input, then wake him up.  If someone is selecting
 * on the network interface, then do a selwakeup().  If asynchronous I/O
 * is enabled, then this important enough to send a signal...
 */
ni_wakeup(np)
	struct ni_cb *np;
{
	struct proc *p;

	if (np->ni_flags & NIF_WAIT) {
		wakeup((caddr_t) &np->ni_if);
		np->ni_flags &= ~NIF_WAIT;
	}
	if (np->ni_sel) {
		selwakeup(np->ni_sel, np->ni_flags & NIF_COLL);
		np->ni_sel = 0;
		np->ni_flags &= ~NIF_COLL;
	}
	if (np->ni_flags & NIF_ASYNC) {		
		if (np->ni_pgrp < 0)
			gsignal(-np->ni_pgrp, SIGIO);
		else if (np->ni_pgrp > 0 && (p = pfind(np->ni_pgrp)) != 0)
			psignal(p, SIGIO);
	}
}
