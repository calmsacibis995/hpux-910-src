/*
 * @(#)stdlib.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:28 $
 * $Locker:  $
 */

/* stdlib.h,v 3.1 1993/07/06 01:07:04 jbj Exp
 * stdlib.h - Prototypes for libc.
 */
#ifndef	_ustdlib_h
#define	_ustdlib_h

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

#ifdef NeXT
FILE    *fdopen(int filedes, const char *mode);
FILE    *popen(const char *command, const char *mode);
#if !defined(pid_t)
#define pid_t int
#endif
#if !defined(sigset_t)
#define sigset_t int
#endif
#endif

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#ifdef _POSIX_SOURCE
#include <fcntl.h>
#include <signal.h>
#endif
#ifdef __STDC__
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#endif

extern	int	atoi		P((char *));
extern	void	exit		P((int));
#if	!defined(__alpha)
extern	int	free		P((void *));
#endif	/* __alpha */
#ifdef	__bsdi__
extern	char *	getpass		P((const char *));
#else
extern	char *	getpass		P((char *));
#endif	/* __bsdi__ */
extern	int	getopt		P((int, char **, char *));
extern	char *	mktemp		P((char *));
extern	void	qsort		P((char *, int, int, int(*)()));
extern	int	rand		P((void));
extern	char *	realloc		P((void *, size_t));
extern	void	srand		P((int));

#ifdef	XNTP_RETROFIT_STDLIB
/* ==== (2) ==== */
extern	int	brk		P((void *));
extern	int	close		P((int));
extern	int	getdtablesize	P((void));
#ifdef sgi
extern	long	gethostid	P((void));
#else
extern	int	gethostid	P((void));
#endif
#if defined(SOLARIS) || defined(sgi)
extern	pid_t	getpgrp		P((void));
#else
extern	pid_t	getpgrp		P((int));
#endif	/* SOLARIS */
extern	int	getpriority	P((int, int));
extern	int	ioctl		P((int, int, ...));
#if defined(SOLARIS) || defined(_POSIX_SOURCE) || defined(sgi)
extern	int	mkdir		P((const char *, mode_t));
#else
extern	int	mkdir		P((const char *, int));
#endif	/* SOLARIS */
extern	int	mount		P((char *, char *, int, char *));

#if defined(SOLARIS) || defined(_POSIX_SOURCE) || defined(sgi)
extern	int	open		P((const char *, int, ...));
#else
extern	int	open		P(());	/* XXX FIXME! <sys/fcntlcom.h> conflict */
#endif	/* SOLARIS / _POSIX_SOURCE */

extern	int	rename		P((const char *, const char *));
extern	int	rmdir		P((const char *));
#ifdef IRIX5
extern	int	setreuid	P((uid_t, uid_t));
#else
extern	int	setreuid	P((int, int));
#endif
extern	void *	sbrk		P((int));

#ifdef  _POSIX_SOURCE
extern  int	setpgid		P((pid_t, pid_t));
#define	setpgrp(pid,pgrp)	setpgid(pid,pgrp)
#else
#if defined(SOLARIS) || defined(sgi)
extern	pid_t	setpgrp		P((void));
#else
extern	pid_t	setpgrp		P((int, int));
#endif	/* SOLARIS */
#endif	/* _POSIX_SOURCE */

extern	int	setpriority	P((int, int, int));
extern	time_t	time		P((time_t *));
extern	int	unmount		P((char *));

#if	defined(_sys_resource_h) || defined(_RESOURCE_H_)
extern	int	getrusage	P((int, struct rusage *));
#endif

/* ==== (3) ==== */
extern	int	bcmp		P((char *, char *, int));
extern	void	bcopy		P((char *, char *, int));
extern	void	bzero		P((char *, int));
extern	int	ffs		P((int));
#ifdef IRIX4
extern	char *	getenv		P((const char *));
#else
extern	char *	getenv		P((char *));
#endif
extern	int	mkstemp		P((char *));
extern	int	nice		P((int));
extern	void	perror		P((const char *));
extern	int	plock		P((int));
#ifdef sgi
extern	int	system		P((const char *));
#else
extern	int	system		P((char *));
#endif

#ifndef	isalpha
extern	int	isalpha		P((int));
extern	int	isupper		P((int));
extern	int	islower		P((int));
extern	int	isdigit		P((int));
extern	int	isxdigit	P((int));
extern	int	isalnum		P((int));
extern	int	isspace		P((int));
extern	int	ispunct		P((int));
extern	int	isprint		P((int));
extern	int	iscntrl		P((int));
extern	int	isascii		P((int));
extern	int	isgraph		P((int));
extern	int	toascii		P((int));
#endif	/* ndef isalpha */

extern	int	tolower		P((int));
extern	int	toupper		P((int));

extern	double	aint		P((double));
extern	double	anint		P((double));
extern	double	ceil		P((double));
extern	double	floor		P((double));
extern	double	rint		P((double));
extern	int	irint		P((double));
extern	int	nint		P((double));

#ifdef	NOTYET
extern	char *	calloc		P((unsigned, unsigned));
extern	char *	malloc		P((unsigned));
#endif	/* NOTYET ndef(__GNUC__) */

extern	int	cfree		P((char *));
#if defined(SOLARIS) || defined(sgi)
extern	int	fcntl		P((int, int, ...));
#else
extern	int	fcntl		P(());	/* XXX FIXME! */
#endif	/* SOLARIS */
extern	void	mallocmap	P((void));
extern	int	mallopt		P((int, int));
extern	char *	memalign	P((unsigned, unsigned));
extern	char *	valloc		P((unsigned));

#ifdef	HPUX
extern	long	sigblock	P((long));
extern	long	sigsetmask	P((long));
#else
extern	int	sigblock	P((int));
extern	int	sigsetmask	P((int));
#endif	/* HPUX */

extern	long	strtol		P((char *, char **, int));
extern	long	atol		P((char *));

#ifdef	NOTNOW
extern	char *	strcat		P((char *, char *));
extern	char *	strncat		P((char *, char *, int));
extern	char *	strdup		P((char *));
/* extern	int	strcmp		P((char *, char *)); */
extern	int	strncmp		P((char *, char *, int));
extern	int	strcasecmp	P((char *, char *));
extern	int	strncasecmp	P((char *, char *, int));
/* extern	char *	strcpy		P((char *, char *)); */
extern	char *	strncpy		P((char *, char *, int));
extern	char *	strchr		P((char *, int));
extern	char *	strpbrk		P((char *, char *));
extern	char *	strrchr		P((char *, int));
extern	char *	strstr		P((char *, char *));
extern	char *	strtok		P((char *, char *));
extern	long	strtol		P((char *, char **, int));
extern	char *	index		P((char *, int));
extern	char *	rindex		P((char *, int));

#ifdef	mips
#define	SLEN	size_t
#define	MPTR	void
#else
#define	SLEN	int
#define	MPTR	char
#endif

extern	SLEN	strspn		P((char *, char *));
extern	SLEN	strcspn		P((char *, char *));
extern	MPTR *	memccpy		P((char *, char *, int, int));
extern	MPTR *	memchr		P((char *, int, int));
extern	MPTR *	memset		P((char *, int, int));

#ifndef	__GNUC__
extern	SLEN	strlen		P((char *));
extern	MPTR *	memcpy		P((char *, char *, int));
extern	int	memcmp		P((char *, char *, int));
#endif	/* ndef __GNUC__ */
#endif	/* NOTNOW */

extern	int	tgetent		P((char *, char *));
extern	int	tgetnum		P((char *));
extern	int	tgetflag	P((char *));
extern	char *	tgetstr		P((char *, char **));
extern	char *	tgoto		P((char *, int, int));
extern	int	tputs		P((char *, int, int (*)()));

extern	int	printf		P((const char *, ...));
extern	int	puts		P((const char *));
/* extern	char *	sprintf		P((char *, char *, ...));	*/
extern	int	scanf		P((const char *, ...));
extern	int	sscanf		P((const char *, const char *, ...));

#if	defined(_nlist_h) || (defined(mips) && defined(N_UNDF))
extern	int	nlist		P((const char *, struct nlist *));
#endif

#if	defined(__sys_signal_h) || defined(_SYS_SIGNAL_H) || defined(_SIGNAL_H) || defined(_SIGNAL_H_) || defined(__SIGNAL_H__)
#ifdef	SOLARIS
typedef struct {	/* XXX HACK! */
	unsigned long __sigbits[4];
} sigset_t;
extern	int	gsignal		P((int));
extern	int	sigaddset	P((sigset_t *, int));
extern	int	sigdelset	P((sigset_t *, int));
extern	int	sigemptyset	P((sigset_t *));
extern	int	sigfillset	P((sigset_t *));
extern	int	sighold		P((int));
extern	int	sigignore	P((int));
extern	int	sigismember	P((sigset_t *, int));
extern	int	sigprocmask	P((int, const sigset_t *, sigset_t *));
extern	int	sigrelse	P((int));
#else	/* SOLARIS */
extern	int	sigvec		P((int, struct sigvec *, struct sigvec *));
#endif	/* SOLARIS */
extern	int	sigpause	P((int));
#ifdef IRIX4
extern	int	sigsuspend	P((sigset_t *));
#else
extern	int	sigsuspend	P((const sigset_t *));
#endif

typedef	RETSIGTYPE	(*sig_type) P((int));
extern	sig_type signal		P((int, sig_type));
#endif

#if	defined(_sys_socket_h) || defined(XTI)
extern	int	socket		P((int, int, int));
extern	int	bind		P((int, struct sockaddr *, int));
extern	int	connect		P((int, struct sockaddr *, int));
extern	int	accept		P((int, struct sockaddr *, int *));
extern	int	getsockopt	P((int, int, int, char *, int *));
extern	int	setsockopt	P((int, int, int, char *, int));
extern	int	listen		P((int, int));
extern	int	shutdown	P((int, int));

extern	int	recv		P((int, char *, int, int));
extern	int	recvfrom	P((int, char *, int, int, struct sockaddr *, int *));
extern	int	recvmsg		P((int, struct msghdr *, int));
extern	int	send		P((int, char *, int, int));
extern	int	sendto		P((int, char *, int, int, struct sockaddr *, int));
extern	int	sendmsg		P((int, struct msghdr *, int));
#endif

extern	void	closelog	P((void));
#if defined(mips) && !defined(sgi)
extern	void	openlog		P((const char *, int));
#else
extern	void	openlog		P((const char *, int, int));
#endif	/* mips */
extern	int	setlogmask	P((int));
extern	void	syslog		P((int, const char *, ...));

#if	defined(_sys_time_h) || defined(_SYS_TIME_H) || defined(_TIME_H_)
extern	int	adjtime		P((struct timeval *, struct timeval *));
extern	int	gettimeofday	P((struct timeval *, struct timezone *));
extern	int	settimeofday	P((struct timeval *, struct timezone *));
extern	int	getitimer	P((int, struct itimerval *));
extern	int	setitimer	P((int, struct itimerval *, struct itimerval *));
extern	char *	ctime		P((const time_t *));
extern	char *	asctime		P((const struct tm *));
extern	struct tm *localtime	P((const time_t *));
extern	int	select		P((int, fd_set *, fd_set *, fd_set *, struct timeval *));
extern	size_t	strftime	P((char *, size_t, const char *, const struct tm *));
extern	char *	strptime	P((char *, int, char *, struct tm *));
extern	time_t	timegm		P((struct tm *));
extern	time_t	timelocal	P((struct tm *));
#endif

#ifdef	_sys_timeb_h
extern	int	ftime		P((struct timeb *));
#endif

#ifdef	_sys_uio_h
extern	int	readv		P((int, struct iovec *, int));
extern	int	writev		P((int, struct iovec *, int));
#endif

#if	defined(FILE) || defined(_STDIO_H) || defined(_STDIO_H_)
#ifdef	mips
extern int	_flsbuf		P((unsigned char, FILE *));
#else
extern	int	_flsbuf		P((int, FILE *));
#endif	/* mips */
extern	int	_filbuf		P((FILE *));
extern	char *	ctermid		P((char *));
extern	char *	cuserid		P((char *));
extern	int	fclose		P((FILE *));
extern	int	fflush		P((FILE *));
extern	char *	fgets		P((char *, int, FILE *));
#ifdef	__STDC__
extern	int	fileno		P((FILE *));
#endif	/* __GNUC__ */
extern	FILE *	fopen		P((const char *, const char *));
extern	FILE *	freopen		P((const char *, const char *, FILE *));
extern	int	fprintf		P((FILE *, const char *, ...));
extern	int	fputc		P((int, FILE *));
extern	int	fputs		P((const char *, FILE *));

#ifdef	SOLARIS
extern	FILE *	fdopen		P((int, const char *));
#else
#ifdef NeXT
extern FILE    *fdopen(int filedes, const char *mode);
#else
extern	FILE *	fdopen		P((int, char *));
#endif /* NeXT */
#endif	/* SOLARIS */

#if	defined(SOLARIS) || defined(mips) || defined(NeXT)
extern	size_t	fread		P((void *, size_t, size_t, FILE *));
extern	size_t	fwrite		P((const void *, size_t, size_t, FILE *));
#else
#ifndef __STDC__
extern	int	fread		P((char *, int, int, FILE *));
extern	int	fwrite		P((const char *, int, int, FILE *));
#endif
#endif	/* SOLARIS */

extern	int	fscanf		P((FILE *, const char *, ...));
extern	int	fseek		P((FILE *, long, int));
extern	long	ftell		P((FILE *));
extern	FILE *	popen		P((const char *, const char *));
extern	int	pclose		P((FILE *));
extern	void	rewind		P((FILE *));
extern	void	setbuf		P((FILE *, char *));
extern	void	setbuffer	P((FILE *, char *, int));
#ifndef ultrix
extern	int	setlinebuf	P((FILE *));
#endif
extern	int	setvbuf		P((FILE *, char *, int, size_t));

extern	FILE *	tmpfile		P((void));
extern	char *	tmpnam		P((char *));
extern	char *	tempnam		P((const char *, const char *));
#if	defined(_sys_varargs_h) || defined(_STDARG_H)
#if	defined(mips) && defined(__GNUC__)
extern	int	vprintf		P((const char *, __gnuc_va_list));
extern	int	vfprintf	P((FILE *, const char *, __gnuc_va_list));
#else
extern	int	vprintf		P((const char *, va_list));
extern	int	vfprintf	P((FILE *, const char *, va_list));
#endif	/* mips && __GNUC__ */
#endif	/* va_list defined */
#endif	/* FILE */

#ifndef	__STDC__
	/* vsprintf() comes from stdio.h in ANSI C */
#if	defined(_sys_varargs_h) || defined(_STDARG_H)
#if	defined(mips) && defined(__GNUC__)
extern	int	vsprintf	P((char *, const char *, __gnuc_va_list));
#else
#ifdef	SOLARIS
extern	int	vsprintf	P((char *, const char *, va_list));
#else
extern	char *	vsprintf	P((char *, const char *, va_list));
#endif	/* SOLARIS */
#endif	/* mips && __GNUC__ */
#endif	/* va_list defined */
#endif  /* __STDC__ not defined */

#endif	/* XNTP_RETROFIT_STDLIB */

#endif	/* _ustdlib_h */
