/*
 * $Header: udp_usrreq.c,v 1.44.83.7 93/10/27 14:31:26 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/udp_usrreq.c,v $
 * $Revision: 1.44.83.7 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 14:31:26 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) udp_usrreq.c $Revision: 1.44.83.7 $ $Date: 93/10/27 14:31:26 $";
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
 *	@(#)udp_usrreq.c	7.7 (Berkeley) 6/29/88 plus MULTICAST 1.2
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/stat.h"

#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/udp.h"
#include "../netinet/udp_var.h"

#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../net/netmp.h"
#include "../net/cko.h"

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
udp_init()
{
	udb.inp_next = udb.inp_prev = &udb;

	/*  Register network mgmt access routines	*/
	NMREG (GP_udp, nmget_udp, mib_unsupp, mib_unsupp, mib_unsupp);
}

#ifndef	COMPAT_42
int	udpcksum = 1;
#else
int	udpcksum = 0;		/* XXX */
#endif
u_char	udpDefaultTTL = UDP_TTL;

struct	sockaddr_in udp_in = { AF_INET };

udp_input(m0, ifp)
	struct mbuf *m0;
	struct ifnet *ifp;
{
	register struct udpiphdr *ui;
	register struct inpcb *inp;
	register struct mbuf *m;
	struct mbuf *m1;
	int len, status;
	struct ip ip;
	unsigned int	comp_sum;

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	m = m0;
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct udpiphdr)) &&
	    (m = m_pullup(m, sizeof (struct udpiphdr))) == 0) {
		udpstat.udps_hdrops++;
		MIB_udpIncrCounter(ID_udpInErrors);
		return;
	}
	ui = mtod(m, struct udpiphdr *);
	if (((struct ip *)ui)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ui, (struct mbuf *)0);

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	len = ntohs((u_short)ui->ui_ulen);
	if (((struct ip *)ui)->ip_len != len) {
		if (len > ((struct ip *)ui)->ip_len) {
			udpstat.udps_badlen++;
			MIB_udpIncrCounter(ID_udpInErrors);
			goto bad;
		}
		m_adj(m, len - ((struct ip *)ui)->ip_len);
		/* ((struct ip *)ui)->ip_len = len; */
	}
	/*
	 * Save a copy of the IP header in case we want restore it for ICMP.
	 */
	ip = *(struct ip*)ui;

	/*
	 * Checksum extended UDP header and data.
	 */
	if (udpcksum && ui->ui_sum) {
		ui->ui_next = ui->ui_prev = 0;
		ui->ui_x1 = 0;
		ui->ui_len = ui->ui_ulen;
		/* HP : checksum assist */
		if (m->m_flags & MF_CKO_IN) {
			CKO_PSEUDO(comp_sum, m, ui);
		} else {
		    	comp_sum = in_cksum(m, len + sizeof(struct ip));
			for (m1 = m; m1; m1 = m1->m_next)
				m1->m_flags &= ~MF_NOACC;
		}

		if (comp_sum) {
			udpstat.udps_badsum++;
			MIB_udpIncrCounter(ID_udpInErrors);
			m_freem(m);
			return;
		}
	}

#ifdef MULTICAST
	if (IN_MULTICAST(ntohl(ui->ui_dst.s_addr)) ||
	    in_broadcast(ui->ui_dst)) {
		struct socket *last;
		/*
		 * Deliver a multicast or broadcast datagram to *all* sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multi/broadcasts on the same port.
		 * (This really ought to be done for unicast datagrams as
		 * well, but that would cause problems with existing
		 * applications that open both address-specific sockets and
		 * a wildcard socket listening to the same port -- they would
		 * end up receiving duplicates of every unicast datagram.
		 * Those applications open the multiple sockets to overcome an
		 * inadequacy of the UDP socket interface, but for backwards
		 * compatibility we avoid the problem here rather than
		 * fixing the interface.  Maybe 4.4BSD will remedy this?)
		 */

		/*
		 * Construct sockaddr format source address.
		 */
		udp_in.sin_port = ui->ui_sport;
		udp_in.sin_addr = ui->ui_src;
		m->m_len -= sizeof (struct udpiphdr);
		m->m_off += sizeof (struct udpiphdr);
		/*
		 * Locate pcb(s) for datagram.
		 * (Algorithm copied from raw_intr().)
		 */
		last = NULL;
		for (inp = udb.inp_next; inp != &udb; inp = inp->inp_next) {
			if (inp->inp_lport != ui->ui_dport) {
				continue;
			}
			if (inp->inp_laddr.s_addr != INADDR_ANY) {
				if (inp->inp_laddr.s_addr !=
					ui->ui_dst.s_addr)
					continue;
			}
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				if (inp->inp_faddr.s_addr !=
					ui->ui_src.s_addr ||
				    inp->inp_fport != ui->ui_sport)
					continue;
			}

			if (last != NULL) {
				struct mbuf *n;

				if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
					if (sbappendaddr(&last->so_rcv,
						(struct sockaddr *)&udp_in,
						n, (struct mbuf *)0) <= 0)
						m_freem(n);
					else
						sorwakeup(last);
				}
			}
			last = inp->inp_socket;
			/*
			 * Don't look for additional matches if this one
			 * does not have the SO_REUSEADDR socket option set.
			 * This heuristic avoids searching through all pcbs
			 * in the common case of a non-shared port.  It
			 * assumes that an application will never clear
			 * the SO_REUSEADDR option after setting it.
			 */
			if ((last->so_options & SO_REUSEADDR) == 0)
				break;
		}

		if (last == NULL) {
			/*
			 * No matching pcb found; discard datagram.
			 * (No need to send an ICMP Port Unreachable
			 * for a broadcast or multicast datgram.)
			 */
			goto bad;
		}
		if (sbappendaddr(&last->so_rcv, (struct sockaddr *)&udp_in,
		     m, (struct mbuf *)0) <= 0)
			goto bad;
		sorwakeup(last);
		return;
	}
#endif MULTICAST
	/*
	 * Locate pcb for datagram.
	 */
	inp = in_pcblookup(&udb,
	    ui->ui_src, ui->ui_sport, ui->ui_dst, ui->ui_dport,
		INPLOOKUP_WILDCARD);
	if (inp == 0) {
#ifndef MULTICAST
		MIB_udpIncrCounter(ID_udpNoPorts);
		/* don't send ICMP response for broadcast packet */
		if (in_broadcast(ui->ui_dst))
			goto bad;
#endif MULTICAST
		*(struct ip *)ui = ip;
		icmp_error((struct ip *)ui, ICMP_UNREACH, ICMP_UNREACH_PORT,
		    ifp);
		return;
	}

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	udp_in.sin_port = ui->ui_sport;
	udp_in.sin_addr = ui->ui_src;
	m->m_len -= sizeof (struct udpiphdr);
	m->m_off += sizeof (struct udpiphdr);
	if ((status = sbappendaddr(&inp->inp_socket->so_rcv, (struct sockaddr *)&udp_in,
	    m, (struct mbuf *)0)) <= 0) {
                if (status == 0) 
                     udpstat.udps_fullsock++;
                else 
                     udpstat.udps_discard++;

		MIB_udpIncrCounter(ID_udpInErrors);
		goto bad;
	}
	MIB_udpIncrCounter(ID_udpInDatagrams);
	sorwakeup(inp->inp_socket);
	return;
bad:
	m_freem(m);
}

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
udp_notify(inp)
	register struct inpcb *inp;
{

	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}

udp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int in_rtchange();

	if ((unsigned)cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;

	switch (cmd) {

	case PRC_QUENCH:
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		in_pcbnotify(&udb, sin, 0, in_rtchange);
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
		in_pcbnotify(&udb, sin, (int)inetctlerrmap[cmd],
			udp_notify);
	}
}

udp_output(inp, m0)
	register struct inpcb *inp;
	struct mbuf *m0;
{
	register struct mbuf *m;
	register struct udpiphdr *ui;
	register int len = 0;

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */
	for (m = m0; m; m = m->m_next)
		len += m->m_len;
	MGET(m, M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		m_freem(m0);
		return (ENOBUFS);
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	m->m_off = MMINOFF + IN_MAXLINKHDR;
	m->m_len = sizeof (struct udpiphdr);
	m->m_next = m0;
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)len + sizeof (struct udphdr));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = ui->ui_len;

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum) {
	/* HP : checksum assist
	*
	*	The casting (struct udpiphdr *)0 will generate the
	*	proper offset for ui_sum.  This is an HP-PA feature
	*	used in genassym.c.
	*/
	    cko_cksum(m, sizeof (struct udpiphdr) + len, inp, 
		&((struct udpiphdr *)0)->ui_sum, CKO_ALGO_UDP|CKO_INSERT);
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = inp->inp_ttl;
	MIB_udpIncrCounter(ID_udpOutDatagrams);
#ifdef MULTICAST
	return (ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST)
	    | IP_MULTICASTOPTS, inp->inp_moptions));
#else
	return (ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST)));
#endif MULTICAST
}

#ifdef 	__hp9000s800
/*ARGSUSED*/
udp_usrsend(so, nam, uio, flags, rights, kdatap)
	struct socket *so;
	struct mbuf *nam;
	struct uio *uio;
	int flags;
	struct mbuf *rights;
	struct mbuf *kdatap;  /* not used */
{
	struct in_addr laddr;
	struct inpcb *inp;
	struct mbuf *top;
	int dontroute;
	sv_sema_t savestate;			  /*MPNET: MP prot state */
	int savelevel;
	int savespl;
	int error;

	if (rights && rights->m_len) {
		error = EINVAL;
		goto exit;
	}
	if ((inp = sotoinpcb(so)) == NULL) {
		error = EINVAL;
		goto exit;
	}
	if (uio->uio_resid > so->so_snd.sb_hiwat) {
		error = EMSGSIZE;
		goto exit;
	}
	if (flags & MSG_OOB) {
		error = EOPNOTSUPP;
		goto exit;
	}
	dontroute = (flags & MSG_DONTROUTE) && 
		((so->so_options & SO_DONTROUTE) == 0);

#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgsnd++;	/* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgsnd++;
#endif /* Series 800 */

	NETMP_GO_EXCLUSIVE(savelevel, savestate); /*MPNET: MP prot on */
	sblock(&so->so_snd);

	if (dontroute)
		so->so_options |= SO_DONTROUTE;   /*set before in_pcbconnect*/

	if (so->so_state & SS_CANTSENDMORE) {
		error = EPIPE;
		goto unlock;
	}
	if (error = so->so_error) {
		so->so_error = 0;			/* ??? */
		goto unlock;
	}
	if (nam) {
		laddr = inp->inp_laddr;
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			goto unlock;
		}
		/*
		 * Must block input while temporarily connected.
		 */
		NET_SPLNET(savespl); /* ifdef MP noop */
		error = in_pcbconnect(inp, nam);
		if (error) {
			NET_SPLX(savespl); /* ifdef MP noop */
			goto unlock;
		}
	} 
	else {
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
                        /* udp_usrreq( PRU_SEND ) returned ENOTCONN here;
                         * however, sosend tested SS_ISCONNECTED and
                         * returned EDESTADDRREQ prior to the udp_usrreq
                         * test.
                         */
			error = EDESTADDRREQ;
			goto unlock;
		}
	}

	error = sobuildsnd(&top, uio, 0x7fffffff, so->so_snd.sb_flags);
	if (error == 0) 
		error = udp_output(inp, top);
	else
		m_freem(top);

	if (nam) {
		in_pcbdisconnect(inp);
		inp->inp_laddr = laddr;
		NET_SPLX(savespl); /* ifdef MP noop */
	}

unlock: if (dontroute)
		so->so_options &= ~SO_DONTROUTE;
	sbunlock(&so->so_snd);
	NETMP_GO_UNEXCLUSIVE(savelevel, savestate);/*MPNET: prot off */
exit:	if (error == EPIPE)
		psignal(u.u_procp, SIGPIPE);
	return(error);
}
#endif	/* __hp9000s800 */

u_long	udp_sendspace = 9*1024;		/* really max datagram size */
u_long	udp_recvspace = 9 * (1024+sizeof(struct sockaddr_in)); /* 4 1K dgrams */

/*ARGSUSED*/
udp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;

	if (req == PRU_CONTROL)
		return (in_control(so, (int)m, (caddr_t)nam,
			(struct ifnet *)rights));
	if (rights && rights->m_len) {
		error = EINVAL;
		goto release;
	}
	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:
		if (inp != NULL) {
			error = EINVAL;
			break;
		}
		error = in_pcballoc(so, &udb);
		if (error)
			break;
		inp = sotoinpcb(so);
		inp->inp_ttl = udpDefaultTTL;
		error = soreserve(so, udp_sendspace, udp_recvspace);
		if (error)
			break;
		MIB_udpIncrLsnNumEnt;
		break;

	case PRU_DETACH:
		in_pcbdetach(inp);
		MIB_udpDecrLsnNumEnt;
		break;

	case PRU_BIND:
		error = in_pcbbind(inp, nam);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		error = in_pcbconnect(inp, nam);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		in_pcbdisconnect(inp);
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND: {
		struct in_addr laddr;
		int s;

		if (nam) {
			laddr = inp->inp_laddr;
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				error = EISCONN;
				break;
			}
			/*
			 * Must block input while temporarily connected.
			 */
			NET_SPLNET(s);
			error = in_pcbconnect(inp, nam);
			if (error) {
				NET_SPLX(s);
				break;
			}
		} else {
			if (inp->inp_faddr.s_addr == INADDR_ANY) {
				error = ENOTCONN;
				break;
			}
		}
		error = udp_output(inp, m);
		m = NULL;
		if (nam) {
			in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;
			NET_SPLX(s);
		}
		}
		break;

	case PRU_ABORT:
		soisdisconnected(so);
		in_pcbdetach(inp);
		MIB_udpDecrLsnNumEnt;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error =  EOPNOTSUPP;
		break;

	case PRU_RCVD:
	case PRU_RCVOOB:
		return (EOPNOTSUPP);	/* do not free mbuf's */

	/*
	 * NIKE: return zero if the protocol/interface support copy on write 
	 */
	case PRU_COPYAVOID:
		if (inp->inp_route.ro_rt == 0) 
			return (EINVAL);
		if (!udpcksum)
			inp->inp_route.ro_flags |= RF_PRNOCKSUM;
		if (COW_SUPP(&inp->inp_route))
			return(0);
		else
			return(EOPNOTSUPP);	

	default:
		panic("udp_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}
