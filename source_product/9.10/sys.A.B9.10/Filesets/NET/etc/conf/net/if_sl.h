/* $Header: if_sl.h,v 1.4.83.4 93/09/17 19:00:29 kcs Exp $ */

#ifndef _SYS_IF_SL_INCLUDED
#define _SYS_IF_SL_INCLUDED

#undef	sc_flags		/* XXX */

struct sl_softc {
	struct ifnet	sc_if;
	u_short		sc_flags;
	struct tty *	sc_ttyp;
	struct mbuf *	sc_head;
	struct mbuf *	sc_tail;
	u_short		sc_tlen;
	u_short		sc_mlen;
	char *		sc_mdat;
	struct mbuf *	sc_mbuf;
};

#define	SL_MTU			1006

#define	SC_ESCAPED		0x0001
#define	SC_RESTART		0x0002
#define	SC_END_OF_FRAME		0x0004

#define FRAME_END		0300
#define FRAME_ESCAPE		0333
#define TRANS_FRAME_END		0334
#define TRANS_FRAME_ESCAPE	0335

#define	SLIPDISC		1

#ifdef	_KERNEL
extern	int		num_sl;
extern	struct sl_softc	sl_softc[];
#endif	/* _KERNEL */
#endif  /* _SYS_IF_SL_INCLUDED */
