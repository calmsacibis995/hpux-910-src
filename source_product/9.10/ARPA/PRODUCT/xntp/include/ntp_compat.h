/*
 * @(#)ntp_compat.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:02 $
 * $Locker:  $
 */

/* ntp_compat.h,v 3.1 1993/07/06 01:06:49 jbj Exp
 * Collect all machine dependent idiosyncrasies in one place.
 */

#if defined(ULT_2_0_SUCKS)
#ifndef sigmask
#define	sigmask(m)	(1<<(m))
#endif
#endif

#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

/* What sort of TTY driver do we have? */
#if defined(HPUX) || defined(_AUX_SOURCE) || defined(sgi)
#define SYSV_TTYS
#endif

/* XXX temporary compatibility with Config.* */
#ifdef	STUPID_SIGNAL
#undef	HAVE_RESTARTABLE_SYSCALLS
#else
#define	HAVE_RESTARTABLE_SYSCALLS	1
#endif	/* STUPID_SIGNAL */

#ifndef	RETSIGTYPE
#define	RETSIGTYPE	void
#endif
