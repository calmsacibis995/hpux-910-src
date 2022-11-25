/*
 * $Header: ntp_stdlib.h,v 1.2.109.2 94/10/28 16:36:40 mike Exp $
 * ntp_stdlib.h - Prototypes for XNTP lib.
 */
#include <sys/types.h>

#include "ntp_types.h"
#include "ntp_compat.h"

#ifndef	P
#if defined(__STDC__) || defined(USE_PROTOTYPES)
#define P(x)	x
#else
#define	P(x)	()
#if	!defined(const)
#define	const
#endif
#endif
#endif

#ifdef	SOLARIS
extern	int	isascii		P((int));	/* XXX authusekey() */
#include <memory.h>
#define bcopy(s1,s2,n) memcpy(s2, s1, n)
#define bzero(s,n)     memset(s, 0, n)
#define bcmp(s1,s2,n)  memcmp(s1, s2, n)
#else
#ifdef	__bsdi__
extern	int	bcmp		P((const void *, const void *, size_t));
extern	void	bcopy		P((const void *, void *, size_t));
extern	void	bzero		P((void *, size_t));
#else
extern	int	bcmp		P((char *, char *, int));
extern	void	bcopy		P((char *, char *, int));
extern	void	bzero		P((char *, int));
#endif	/* __bsdi__ */
#endif	/* SOLARIS */

extern	void	exit		P((int));
extern	void *	malloc		P((int));
extern	int	atoi		P((char *str));

#if	defined(FILE) || defined(_STDIO_H) || defined(_STDIO_H_)
#ifdef	mips
extern int	_flsbuf		P((unsigned char, FILE *));
#else
extern	int	_flsbuf		P((int, FILE *));
#endif	/* mips */
extern	int	fputs		P((const char *, FILE *));
#endif

#if	defined(_sys_time_h) || defined(_SYS_TIME_H) || defined(_TIME_H_)
extern	int	gettimeofday	P((struct timeval *, struct timezone *));
#endif	/* _sys_time_h */

#ifdef hpux
extern	int	closelog	P((void));
#else
extern	void	closelog	P((void));
#endif
#if defined(mips) && (!defined(sgi))
extern	void	openlog		P((const char *, int));
#else
#if	!defined(__alpha)
#ifdef hpux
extern	int	openlog		P((const char *, int, int));
#else
extern	void	openlog		P((const char *, int, int));
#endif
#endif	/* __ alpha */
#endif	/* mips */
extern	int	setlogmask	P((int));
#if	!defined(__alpha)
#ifdef hpux
extern	int	syslog		P((int, const char *, ...));
#else
extern	void	syslog		P((int, const char *, ...));
#endif
#endif	/* __ alpha */
extern	void	msyslog		P((int, char *, ...));

extern	void	auth_des	P((U_LONG *, u_char *));
extern	void	auth_delkeys	P((void));
extern	int	auth_havekey	P((U_LONG));
extern	int	auth_parity	P((U_LONG *));
extern	void	auth_setkey	P((U_LONG, U_LONG *));
extern	void	auth_subkeys	P((U_LONG *, u_char *, u_char *));
extern	int	authistrusted	P((U_LONG));
extern	int	authusekey	P((U_LONG, int, const char *));

extern	void	auth_delkeys	P((void));

extern	void	auth1crypt	P((U_LONG, U_LONG *, int));
extern	int	auth2crypt	P((U_LONG, U_LONG *, int));
extern	int	authdecrypt	P((U_LONG, U_LONG *, int));
extern	int	authencrypt	P((U_LONG, U_LONG *, int));
extern	int	authhavekey	P((U_LONG));
extern	int	authreadkeys	P((const char *));
extern	void	authtrust	P((U_LONG, int));
extern	void	calleapwhen	P((U_LONG, U_LONG *, U_LONG *));
extern	U_LONG	calyearstart	P((U_LONG));
extern	const char *clockname	P((int));
extern	int	clocktime	P((int, int, int, int, int, U_LONG, U_LONG *, U_LONG *));
extern	char *	emalloc		P((u_int));
extern	int	getopt		P((int, char **, char *));
extern	void	init_auth	P((void));
extern	void	init_lib	P((void));
extern	void	init_random	P((void));

#ifdef	DES
extern	void	DESauth1crypt	P((U_LONG, U_LONG *, int));
extern	int	DESauth2crypt	P((U_LONG, U_LONG *, int));
extern	int	DESauthdecrypt	P((U_LONG, const U_LONG *, int));
extern	int	DESauthencrypt	P((U_LONG, U_LONG *, int));
extern	void	DESauth_setkey	P((U_LONG, const U_LONG *));
extern	void	DESauth_subkeys	P((const U_LONG *, u_char *, u_char *));
extern	void	DESauth_des	P((U_LONG *, const u_char *));
extern	int	DESauth_parity	P((U_LONG *));
#endif	/* DES */

#ifdef	MD5
extern	void	MD5auth1crypt	P((U_LONG, U_LONG *, int));
extern	int	MD5auth2crypt	P((U_LONG, U_LONG *, int));
extern	int	MD5authdecrypt	P((U_LONG, const U_LONG *, int));
extern	int	MD5authencrypt	P((U_LONG, U_LONG *, int));
extern	void	MD5auth_setkey	P((U_LONG, const U_LONG *));
#endif	/* MD5 */

extern	int	atoint		P((const char *, LONG *));
extern	int	atouint		P((const char *, U_LONG *));
extern	int	hextoint	P((const char *, U_LONG *));
extern	char *	humandate	P((U_LONG));
extern	char *	inttoa		P((LONG));
extern	char *	mfptoa		P((U_LONG, U_LONG, int));
extern	char *	mfptoms		P((U_LONG, U_LONG, int));
extern	char *	modetoa		P((int));
extern	char *	numtoa		P((U_LONG));
extern	char *	numtohost	P((U_LONG));
extern	int	octtoint	P((const char *, U_LONG *));
extern	U_LONG	ranp2		P((int));
extern	char *	refnumtoa	P((U_LONG));
extern	int	tsftomsu	P((U_LONG, int));
extern	char *	uinttoa		P((U_LONG));

extern	int	decodenetnum	P((const char *, U_LONG *));
