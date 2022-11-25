/*
 * $Header: if_sl.c,v 1.3.83.4 93/09/17 19:00:35 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/if_sl.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:00:35 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) if_sl.c $Revision: 1.3.83.4 $";
#endif

#ifdef	SLIP

#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/proc.h"
#include "../h/errno.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/ns_diag.h"
#include "../h/tty.h"
#include "../h/ld_dvr.h"

#include "../netinet/in.h"

#include "../net/if.h"
#include "../net/if_sl.h"
#include "../net/netisr.h"
#include "../net/route.h"

extern	struct macct *	inbound_macct;

#define	spl_tty	splimp

slip_init()
{
}

sl_attach()
{
	int unit;
	struct sl_softc *sc;
	int ipintr(), sl_ioctl(), sl_output();

	if (num_sl > 10)
		num_sl = 10;

	for (unit = 0, sc = sl_softc; unit < num_sl; unit++, sc++) {
		sc->sc_if.if_name = "sl";
		sc->sc_if.if_unit = unit;
		sc->sc_if.if_mtu = SL_MTU;
		sc->sc_if.if_flags = IFF_POINTOPOINT;
		sc->sc_if.if_ioctl = sl_ioctl;
		sc->sc_if.if_input = ipintr;
		sc->sc_if.if_output = sl_output;
		if_qinit(&sc->sc_if.if_snd, IFQ_MAXLEN);
		if_qinit(&sc->sc_if.if_rcv, IFQ_MAXLEN);
		if_attach(&sc->sc_if);
	}

	if (num_sl > 0)
		printf("Attached %d Serial Line IP Interface(s)\n", num_sl);
}

sl_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	caddr_t data;
{
	int s, error = 0;
	struct ifreq *ifr;
	struct sockaddr_in *sin;

	ifr = (struct ifreq *) data;

	s = spl_tty();
	switch (cmd) {
		case SIOCSIFDSTADDR:
			if ((ifp->if_flags & IFF_POINTOPOINT) == 0) {
				error = EINVAL;
				break;
			}
			if (ifr->ifr_dstaddr.sa_family != AF_INET) {
				error = EAFNOSUPPORT;
				break;
			}
			rtinit(&ifp->if_dstaddr, &ifp->if_addr, -1);
			ifp->if_dstaddr = ifr->ifr_dstaddr;
			rtinit(&ifp->if_dstaddr, &ifp->if_addr,
				RTF_HOST|RTF_UP);
			break;

		case SIOCSIFADDR:
			if (ifp->if_flags & IFF_RUNNING)
				if_rtinit(ifp, -1);
			ifp->if_addr = ifr->ifr_addr;
			sin = (struct sockaddr_in *) &ifp->if_addr;
			ifp->if_netmask = IN_MASK(sin->sin_addr.s_addr);
			if (ifp->if_subnetmask == 0)
				ifp->if_subnetmask = ifp->if_netmask;
			ifp->if_net = sin->sin_addr.s_addr & ifp->if_netmask;
			ifp->if_subnet = sin->sin_addr.s_addr & ifp->if_subnetmask;
			ifp->if_host[0] = in_lnaof(sin->sin_addr);

			if_rtinit(ifp, RTF_UP);
			break;

		case SIOCGIFNETMASK:
			sin = (struct sockaddr_in *) &ifr->ifr_addr;
			sin->sin_addr.s_addr = ifp->if_subnetmask;
			break;

		case SIOCSIFNETMASK:
			sin = (struct sockaddr_in *) &ifr->ifr_addr;
			if (ifp->if_flags & IFF_RUNNING)
				if_rtinit(ifp, -1);
			ifp->if_subnetmask = sin->sin_addr.s_addr;
			break;

		default:
			error = EINVAL;
			break;
	}
	(void) splx(s);

	return(error);
}

struct iii {
	unsigned char d[6];
	unsigned char s[6];
	unsigned short t;
};

sl_input(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	int s = spl_tty();
	static struct mbuf *m0 = 0;
	struct iii *iiip;

	if (m0 == 0) {
		m0 = m_getclr(M_DONTWAIT, MA_NULL, MT_HEADER);
		if (m0) {
			m0->m_len = 14;
			iiip = mtod(m0, struct iii *);
			iiip->t = 0x800;
		}
	}

	if (IF_QFULL(&ifp->if_rcv)) {
		/* NS_LOG_INFO */
		IF_DROP(&ifp->if_rcv);
		ifp->if_ierrors++;
		m_freem(m);
		(void) splx(s);
		return;
	}

	if (m0) {
		m0->m_next = m;
		ns_trace_link(ifp, TR_LINK_INBOUND, m0);
	}

	/* NS_LOG_INFO */
	schednetisr(NETISR_INPUT, ifp, m);
	ifp->if_ipackets++;

	(void) splx(s);
}

sl_output(ifp, m, dst)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
{
	int s, error = 0;
	struct sl_softc *sc;
	static struct mbuf *m0 = 0;
	struct iii *iiip;

	if (m0 == 0) {
		m0 = m_getclr(M_DONTWAIT, MA_NULL, MT_HEADER);
		if (m0) {
			m0->m_len = 14;
			iiip = mtod(m0, struct iii *);
			iiip->t = 0x800;
		}
	}

	if (dst->sa_family != AF_INET) {
		/* NS_LOG_INFO */
		m_freem(m);
		return(EAFNOSUPPORT);
	}

	s = spl_tty();

	sc = &sl_softc[ifp->if_unit];

	if (sc->sc_ttyp == (struct tty *) 0) {
		/* NS_LOG_INFO */
		error = ENETDOWN;
		goto exit;
	}

	if ((sc->sc_ttyp->t_state & CARR_ON) == 0) {
		/* NS_LOG_INFO */
		error = EHOSTUNREACH;
		goto exit;
	}

	if (IF_QFULL(&ifp->if_snd)) {
		/* NS_LOG_INFO */
		IF_DROP(&ifp->if_snd);
		sc->sc_if.if_oerrors++;
		error = ENOBUFS;
		goto exit;
	}

	/* NS_LOG_INFO */
	IF_ENQUEUE(&ifp->if_snd, m);
	sc->sc_if.if_opackets++;
	sc->sc_ttyp->t_tdstate |= TTOUT;

	if (m0) {
		m0->m_next = m;
		ns_trace_link(ifp, TR_LINK_OUTBOUND, m0);
	}

	(void) splx(s);

	return(0);

exit:
	(void) splx(s);
	m_freem(m);
	return(error);
}

sl_alloc_cblock(ccp)
	struct ccblock *ccp;
{
	struct cblock *cp;

	if (ccp->c_size != 0)
		return(0);

	if ((cp = getcf()) == (struct cblock *) 0)
		return(ENOSPC);

	ccp->c_ptr = cp->c_data;
	ccp->c_count = 0;
	ccp->c_size = cp->c_last;

	return(0);
}

sl_ld_open(tp)
	struct tty *tp;
{
	int s, unit, error;
	struct sl_softc *sc;

	if (!suser())
		return(EPERM);

	error = ENXIO;

	s = spl_tty();

	for (unit = 0, sc = sl_softc; unit < num_sl; unit++, sc++)
		if (sc->sc_ttyp == (struct tty *) 0) {
			if ((error = sl_alloc_cblock(&tp->t_tbuf)) != 0)
				break;
			sl_init(sc);
			tp->t_linep = (char *) sc;
			sc->sc_ttyp = tp;
			error = 0;
			break;
		}

	(void) splx(s);

	return(error);
}

sl_ld_close(tp)
	struct tty *tp;
{
	int s;
	struct sl_softc *sc;

	s = spl_tty();

	sc = (struct sl_softc *) tp->t_linep;

	if (sc != (struct sl_softc *) 0) {
		if_down(&sc->sc_if);
		sc->sc_ttyp = (struct tty *) 0;
		tp->t_line = 0;
		tp->t_linep = (char *) 0;
		m_freem(sc->sc_head);
		m_freem(sc->sc_mbuf);
	}

	splx(s);

	return(0);
}

sl_ld_ioctl()
{
	return(0);
}

sl_init(sc)
	struct sl_softc *sc;
{
	sc->sc_flags = 0;
	sc->sc_head = (struct mbuf *) 0;
	sc->sc_tail = (struct mbuf *) 0;
	sc->sc_mbuf = (struct mbuf *) 0;
	sc->sc_tlen = 0;
	sc->sc_mlen = 0;
	sc->sc_mdat = (char *) 0;
}

sl_restart(sc)
	struct sl_softc *sc;
{
	if (sc->sc_head)
		m_freem(sc->sc_head);
	sc->sc_head = (struct mbuf *) 0;
	sc->sc_tail = (struct mbuf *) 0;
	sc->sc_tlen = 0;
	sc->sc_mlen = 0;
	sc->sc_mdat = (char *) 0;
	sc->sc_flags &= ~SC_ESCAPED;
	sc->sc_flags |= SC_RESTART;
	sc->sc_if.if_ierrors++;
}

sl_ld_rint(tp, code)
	struct tty *tp;
{
	u_char c, *cp;
	int s, len;
	struct mbuf *m;
	struct sl_softc *sc;

	s = spl_tty();

	if ((sc = (struct sl_softc *) tp->t_linep) == (struct sl_softc *) 0) {
		(void) splx(s);
		return;
	}

	len = tp->t_rbuf.c_size - tp->t_rbuf.c_count;
	cp = tp->t_rbuf.c_ptr;

	while (len != 0) {
		c = *cp++, --len;
		if (sc->sc_flags & SC_RESTART) {
			if (c == FRAME_END)
				sc->sc_flags &= ~SC_RESTART;
			continue;
		}
		if (sc->sc_mlen == 0) {
			m = m_get(M_DONTWAIT, inbound_macct, MT_DATA);
			if (m == (struct mbuf *) 0) {
				sl_restart(sc);
				break;
			}
			if (sc->sc_tail == (struct mbuf *) 0) {
				sc->sc_head = m;
				sc->sc_tail = m;
			} else {
				sc->sc_tail->m_next = m;
				sc->sc_tail = m;
			}
			sc->sc_tail->m_len = 0;
			sc->sc_mlen = MLEN;
			sc->sc_mdat = mtod(m, u_char *);
		}
		if (sc->sc_flags & SC_ESCAPED) {
			sc->sc_flags &= ~SC_ESCAPED;
			switch (c) {
				case TRANS_FRAME_END:
					c = FRAME_END;
					break;

				case TRANS_FRAME_ESCAPE:
					c = FRAME_ESCAPE;
					break;

				default:
					sl_restart(sc);
					continue;
			}
		} else {
			switch (c) {
				case FRAME_END:
					if (sc->sc_tlen != 0)
						sl_input(&sc->sc_if, sc->sc_head);
					else
						m_freem(sc->sc_head);

					sc->sc_head = (struct mbuf *) 0;
					sc->sc_tail = (struct mbuf *) 0;
					sc->sc_tlen = 0;
					sc->sc_mlen = 0;
					sc->sc_mdat = (char *) 0;
					continue;

				case FRAME_ESCAPE:
					sc->sc_flags |= SC_ESCAPED;
					continue;
			}
		}
		if (sc->sc_tlen >= SL_MTU) {
			sl_restart(sc);
			continue;
		}
		sc->sc_tlen++;
		sc->sc_mlen--;
		sc->sc_tail->m_len++;
		*(sc->sc_mdat++) = c;
	}

	tp->t_rbuf.c_count = tp->t_rbuf.c_size;

	(void) splx(s);

	return(0);
}

sl_ld_xint(tp)
	struct tty *tp;
{
	u_char c, *cp;
	int s, size, len;
	struct sl_softc *sc;

	s = spl_tty();

	if ((sc = (struct sl_softc *) tp->t_linep) == (struct sl_softc *) 0) {
		(void) splx(s);
		return(0);
	}

	len = size = CBSIZE;
	cp = tp->t_tbuf.c_ptr;

	while (len != 0) {
		if (sc->sc_mbuf == (struct mbuf *) 0) {
			IF_DEQUEUE(&sc->sc_if.if_snd, sc->sc_mbuf);
			if (sc->sc_mbuf == (struct mbuf *) 0) {
				if (sc->sc_flags & SC_END_OF_FRAME) {
					*cp++ = FRAME_END, --len;
					sc->sc_flags &= ~SC_END_OF_FRAME;
				}
				tp->t_tdstate &= ~TTOUT;
				break;
			}
			*cp++ = FRAME_END, --len;
			sc->sc_flags |= SC_END_OF_FRAME;
			continue;
		}
		if (sc->sc_mbuf->m_len == 0) {
			sc->sc_mbuf = m_free(sc->sc_mbuf);
			continue;
		}
		c = *mtod(sc->sc_mbuf, u_char *);
		if (c == FRAME_END || c == FRAME_ESCAPE) {
			if (len < 2)
				break;
			*cp++ = FRAME_ESCAPE, --len;
			if (c == FRAME_END)
				c = TRANS_FRAME_END;
			else
				c = TRANS_FRAME_ESCAPE;
		}
		*cp++ = c, --len;
		sc->sc_mbuf->m_off++;
		sc->sc_mbuf->m_len--;
	}

	tp->t_tbuf.c_size = tp->t_tbuf.c_count = len = (size - len);

	(void) splx(s);

	return((len > 0) ? LDX_TBUF : 0);
}

sl_ld_control(tp, code, arg)
	struct tty *tp;
	enum ldc_func code;
	caddr_t arg;
{
	switch (code) {
		case LDC_LOPEN:
			return(sl_ld_open(tp));

		case LDC_LCLOSE:
			return(sl_ld_close(tp));

		default:
			return(0);
	}
}

#endif	SLIP
