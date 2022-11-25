/*
 * Copyright (c) 1983 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifdef PATCH_STRING
static char *PHNE_2916_patch =
    "@(#) PATCH_9.X: rcp.c $Revision: 1.13.109.5 $ 95/02/27 PHNE_5287";
#endif

#ifndef lint
static char sccsid[] = "@(#)rcp.c	5.20 (Berkeley) 5/23/89";
static char rcsid[] = "@(#)$Header: rcp.c,v 1.13.109.5 95/02/22 15:50:52 randyc Exp $";
#endif /* not lint */

/*
 * rcp
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pwd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "pathnames.h"

#ifdef SecureWare
#include <sys/security.h>
#endif /* SecureWare */

#ifdef KERBEROS
#include <kerberos/krb.h>

char krb_realm[REALM_SZ];
int use_kerberos = 1, encrypt = 0;
CREDENTIALS cred;
Key_schedule schedule;
#endif

#ifdef SecureWare
#include <audnetd.h>
char *rhost = (char *)0, *rusr = (char *)0, *lusr = (char *)0;
uid_t	ruid = (uid_t) -1, luid = (uid_t) -1;
u_long	raddr = (u_long) -1, laddr = (u_long) -1;

/*
**    Auditing disabled because audit_daemon routine not yet available.
**    To re-enable auditing, remove the #ifdef notdef and the code
**    from the #else to the #endif.
**
**    To make auditing work, either give this program the potential
**    privileges enabled immediately below, or else make it part of
**    a protected subsystem, and remove the enablepriv() calls.
*/

#ifdef notdef
#ifdef B1
#define enable_priv_for_audit()	\
		if (ISB1) { \
			enablepriv(SEC_ALLOWDACACCESS); \
			enablepriv(SEC_ALLOWMACACCESS); \
			enablepriv(SEC_WRITE_AUDIT); \
		}
#define disable_priv_for_audit()	\
		if (ISB1) { \
			disablepriv(SEC_ALLOWDACACCESS); \
			disablepriv(SEC_ALLOWMACACCESS); \
			disablepriv(SEC_WRITE_AUDIT); \
		}
#else /* not B1 */
#define enable_priv_for_audit()	
#define disable_priv_for_audit()	
#endif /* not B1 */

#define AUDIT_SUCCESS(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_SUCCESS, NA_VALI_RUSEROK, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_IDENT, \
			NA_MASK_RADDR|NA_MASK_RUID|NA_MASK_LUID, (s)); \
		disable_priv_for_audit(); \
	}

#else /* not notdef */
#define AUDIT_SUCCESS(s)
#endif /* not notdef */

#else /* not SecureWare */
#define AUDIT_SUCCESS(s)
#endif /* not SecureWare */

extern int errno;
extern char *sys_errlist[], **environ;
struct passwd *pwd;
int errs, pflag, port, rem, userid, errfd = NULL;
int iamremote, iamrecursive, targetshouldbedirectory;
char *index(), *rindex();

#define	CMDNEEDS	20
char cmd[CMDNEEDS];		/* must hold "rcp -r -p -d\0" */

typedef struct _buf {
	int	cnt;
	char	*buf;
} BUF;

/*
 * Size of socket send/receive buffers and read/write buffers.
 * May be defined at compile time with -D"BIGBUFSIZ=??".
 */
#ifndef BIGBUFSIZ
#define	BIGBUFSIZ	32768
#endif BIGBUFSIZ

#ifndef SENDBUFSIZ
#define SENDBUFSIZ	BIGBUFSIZ
#endif SENDBUFSIZ

#ifndef RECVBUFSIZ
#define RECVBUFSIZ	BIGBUFSIZ
#endif RECVBUFSIZ

int bigbufsize, recvbufsize= RECVBUFSIZ;
#define S_RWXU          00700            
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
        /*  extern char **optarg;   causing compiling error in 9.0 */
	extern char *optarg;
	struct servent *sp;
	int ch, fflag, tflag;
	char *targ, *colon();
	struct passwd *getpwuid();
	int lostconn();
#ifdef SecureWare
	int mask;

	/*
	**      set_auth_parameters() sets the umask,
	**      which is inappropriate in the client sink case.
	*/
	if (ISSECURE) {
		mask = umask(0);
		set_auth_parameters(argc, argv);
		umask(mask);
	}
#ifdef B1
	if (ISB1)
		initprivs();
#endif /* B1 */
#endif /* SecureWare */

	/*
	**	clear timers, close open files, and wipe out environment
	*/
	cleanenv(&environ,
		"LANG", "LANGOPTS", "NLSPATH",
		"LOCALDOMAIN", "HOSTALIASES", 0);

#ifdef KERBEROS
	sp = getservbyname("kshell", "tcp");
	if (sp == NULL) {
		use_kerberos = 0;
		old_warning("kshell service unknown");
		sp = getservbyname("kshell", "tcp");
	}
#else
	sp = getservbyname("shell", "tcp");
#endif
	if (!sp) {
		(void)fprintf(stderr, "rcp: shell/tcp: unknown service\n");
		exit(1);
	}
	port = sp->s_port;

	if (!(pwd = getpwuid(userid = getuid()))) {
		(void)fprintf(stderr, "rcp: unknown user %d.\n", userid);
		exit(1);
	}
#ifdef SecureWare
	lusr = pwd->pw_name;
#endif /* SecureWare */

	fflag = tflag = 0;
	while ((ch = getopt(argc, argv, "dfkprtxR:")) != EOF)
		switch(ch) {
		case 'd':
			targetshouldbedirectory = 1;
			break;
		case 'f':			/* "from" */
			fflag = 1;
			break;
#ifdef KERBEROS
		case 'k':
			strncpy(krb_realm, ++argv, REALM_SZ);
			break;
#endif
		case 'p':			/* preserve access/mod times */
			++pflag;
			break;
		case 'r':
			++iamrecursive;
			break;
		case 't':			/* "to" */
			tflag = 1;
			break;
#ifdef KERBEROS
		case 'x':
			encrypt = 1;
			des_set_key(cred.session, schedule);
			break;
#endif
		case 'R':
			recvbufsize = atol(optarg);
			if ((recvbufsize <= 0) || (recvbufsize > 32768)) {
				fprintf(stderr, "rcp: invalid recv buffer size %s, ignored\n", *argv + 2);
				recvbufsize = RECVBUFSIZ;
			}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (fflag) {
		int on = 1;
		iamremote = 1;
		(void)response();
		(void)setuid(userid);
		bigbufsize = SENDBUFSIZ;
		if (setsockopt(rem ,SOL_SOCKET, SO_SNDBUF, &bigbufsize, sizeof(int)) < 0) {
			error("setsockopt(SO_SNDBUF): %s\n", sys_errlist[errno]);
			exit(1);
		}
		if (setsockopt(rem, getprotobyname("tcp")->p_proto,
			TCP_NODELAY, &on, sizeof(on)) < 0)
		{
			/* 
			   Failure to set the NODELAY option 
			   is not fatal; however, the excution
			   time will be very long if there are
			   many files involved.
			*/

			error("setsockopt(TCP_NODELAY): %s\n", strerror(errno));
		}
		source(argc, argv);
		exit(errs);
	}

	if (tflag) {
		int on = 1;
		iamremote = 1;
		(void)setuid(userid);
		bigbufsize = recvbufsize;
		if (setsockopt(rem, SOL_SOCKET, SO_RCVBUF, &bigbufsize, sizeof(int)) < 0) {
			error("setsockopt(SO_RCVBUF): %s\n", sys_errlist[errno]);
			exit(1);
		}
		if (setsockopt(rem, getprotobyname("tcp")->p_proto,
			TCP_NODELAY, &on, sizeof(on)) < 0)
		{
			/* 
			   Failure to set the NODELAY option 
			   is not fatal; however, the excution
			   time will be very long if there are
			   many files involved.
			*/

			error("setsockopt(TCP_NODELAY): %s\n", strerror(errno));
		}
		sink(argc, argv);
		exit(errs);
	}

	if (argc < 2)
		usage();
	if (argc > 2)
		targetshouldbedirectory = 1;

	rem = -1;
	(void)sprintf(cmd, "rcp%s%s%s", iamrecursive ? " -r" : "",
	    pflag ? " -p" : "", targetshouldbedirectory ? " -d" : "");

	(void)signal(SIGPIPE, lostconn);

	if (targ = colon(argv[argc - 1]))
		toremote(targ, argc, argv);
	else {
		if (targetshouldbedirectory)
			verifydir(argv[argc - 1]);
		tolocal(argc, argv);
	}
	exit(errs);
}

toremote(targ, argc, argv)
	char *targ;
	int argc;
	char **argv;
{
	int i, on = 1;
	char *bp, *host, *src, *suser, *thost, *tuser;
	char *colon(), *malloc();

	*targ++ = 0;
	if (*targ == 0)
		targ = ".";

	if (thost = index(argv[argc - 1], '@')) {
		*thost++ = 0;
		tuser = argv[argc - 1];
		if (*tuser == '\0')
			tuser = NULL;
		else if (!okname(tuser))
			exit(1);
	} else {
		thost = argv[argc - 1];
		tuser = NULL;
	}

	for (i = 0; i < argc - 1; i++) {
		src = colon(argv[i]);
		if (src) {			/* remote to remote */
			*src++ = 0;
			if (*src == 0)
				src = ".";
			host = index(argv[i], '@');
			if (!(bp = malloc((u_int)(strlen(_PATH_RSH) +
				    strlen(argv[i]) + strlen(src) +
				    strlen(tuser) + strlen(thost) +
				    strlen(targ)) + CMDNEEDS + 20)))
					nospace();
			if (host) {
				*host++ = 0;
				suser = argv[i];
				if (*suser == '\0')
					suser = pwd->pw_name;
				else if (!okname(suser))
					continue;
				(void)sprintf(bp,
				    "%s %s -l %s -n %s '%s' '%s%s%s:%s'",
				    _PATH_RSH, host, suser, cmd, src,
				    tuser ? tuser : "", tuser ? "@" : "",
				    thost, targ);
			} else
				(void)sprintf(bp, "%s %s -n %s '%s' '%s%s%s:%s'",
				    _PATH_RSH, argv[i], cmd, src,
				    tuser ? tuser : "", tuser ? "@" : "",
				    thost, targ);
			(void)susystem(bp);
			(void)free(bp);
		} else {			/* local to remote */
			if (rem == -1) {
				if (!(bp = malloc((u_int)strlen(targ) +
				    CMDNEEDS + 20)))
					nospace();
				(void)sprintf(bp, "%s -t %s", cmd, targ);
				host = thost;
#ifdef KERBEROS
				if (use_kerberos)
					kerberos(bp,
					    tuser ? tuser : pwd->pw_name);
				else
#endif
				{
#if defined(SecureWare) && defined(B1)
					/*
					**  rcmd binds to a reserved port,
					**  thus requires REMOTE.
					*/
					if (ISB1) {
						forcepriv(SEC_REMOTE);
					}
#endif /* SecureWare && B1 */
					rem = rcmd(&host, port, pwd->pw_name,
					    tuser ? tuser : pwd->pw_name,
					    bp, 0);
#if defined(SecureWare) && defined(B1)
					if (ISB1) {
						disablepriv(SEC_REMOTE);
					}
#endif /* SecureWare && B1 */
				}
				if (rem < 0)
					exit(1);

				if (setsockopt(rem, 
				      getprotobyname("tcp")->p_proto,
				      TCP_NODELAY, &on, sizeof(on)) < 0)
				{
					/* 
					   Failure to set the NODELAY option 
					   is not fatal; however, the excution
					   time will be very long if there are
					   many files involved.
					*/

					error("setsockopt(TCP_NODELAY): %s\n",
						strerror(errno));
				}

#ifdef SecureWare
				rhost = host;
				rusr = tuser ? tuser : pwd->pw_name;
				AUDIT_SUCCESS("Certified identity of local user");
#endif /* SecureWare */
				if (response() < 0)
					exit(1);
				(void)free(bp);
				(void)setuid(userid);
			}
			bigbufsize = SENDBUFSIZ;
			if (setsockopt(rem ,SOL_SOCKET, SO_SNDBUF, &bigbufsize, sizeof(int)) < 0) {
				error("setsockopt(SO_SNDBUF): %s\n", sys_errlist[errno]);
				exit(1);
			}
			source(1, argv+i);
		}
	}
}

tolocal(argc, argv)
	int argc;
	char **argv;
{
	int i, on = 1;
	char *bp, *host, *src, *suser;
	char *colon(), *malloc();

	for (i = 0; i < argc - 1; i++) {
		if (!(src = colon(argv[i]))) {	/* local to local */
			if (!(bp = malloc((u_int)(strlen(_PATH_CP) +
			    strlen(argv[i]) + strlen(argv[argc - 1])) + 20)))
				nospace();
			(void)sprintf(bp, "%s%s%s %s %s", _PATH_CP,
			    iamrecursive ? " -r" : "", pflag ? " -p" : "",
			    argv[i], argv[argc - 1]);
			(void)susystem(bp);
			(void)free(bp);
			continue;
		}
		*src++ = 0;
		if (*src == 0)
			src = ".";
		host = index(argv[i], '@');
		if (host) {
			*host++ = 0;
			suser = argv[i];
			if (*suser == '\0')
				suser = pwd->pw_name;
			else if (!okname(suser))
				continue;
		} else {
			host = argv[i];
			suser = pwd->pw_name;
		}
		if (!(bp = malloc((u_int)(strlen(src)) + CMDNEEDS + 20)))
			nospace();
		(void)sprintf(bp, "%s -f %s", cmd, src);
#ifdef KERBEROS
		if (use_kerberos)
			kerberos(bp, suser);
		else
#endif
		{
#if defined(SecureWare) && defined(B1)
			if (ISB1) {
				/*
				**  rcmd binds to a reserved port,
				**  thus requires REMOTE.
				*/
				forcepriv(SEC_REMOTE);
			}
#endif /* SecureWare && B1 */
			rem = rcmd(&host, port, pwd->pw_name, suser, bp, 0);
#if defined(SecureWare) && defined(B1)
			if (ISB1) {
				disablepriv(SEC_REMOTE);
			}
#endif /* SecureWare && B1 */
		}
		(void)free(bp);
		if (rem < 0)
		{
			++errs;
			continue;
		}

		if (setsockopt(rem, getprotobyname("tcp")->p_proto,
			TCP_NODELAY, &on, sizeof(on)) < 0)
		{
			/* 
			   Failure to set the NODELAY option 
			   is not fatal; however, the excution
			   time will be very long if there are
			   many files involved.
			*/

			error("setsockopt(TCP_NODELAY): %s\n",strerror(errno));
		}

#ifdef SecureWare
		rhost = host;
		rusr = suser;
		AUDIT_SUCCESS("Certified identity of local user");
#endif /* SecureWare */
		bigbufsize = recvbufsize;
		if (setsockopt(rem, SOL_SOCKET, SO_RCVBUF, &bigbufsize, sizeof(int)) < 0) {
		    error("setsockopt(SO_RCVBUF): %s\n", sys_errlist[errno]);
		    exit(1);
		}
		(void)setresuid(0, userid, -1);
		sink(1, argv + argc - 1);
		(void)setresuid(userid, 0, -1);
		(void)close(rem);
		rem = -1;
	}
}

#ifdef KERBEROS
kerberos(bp, user)
	char *bp, *user;
{
	struct servent *sp;
	char *host;

again:	rem = KSUCCESS;
	if (krb_realm[0] == '\0')
		rem = krb_get_lrealm(krb_realm, 1);
	if (rem == KSUCCESS) {
		if (encrypt)
			rem = krcmd_mutual(&host, port, user, bp, 0,
			    krb_realm, &cred, schedule);
		else
			rem = krcmd(&host, port, user, bp, 0, krb_realm);
	} else {
		(void)fprintf(stderr,
		    "rcp: error getting local realm %s\n", krb_err_txt[rem]);
		exit(1);
	}
	if (rem < 0 && errno == ECONNREFUSED) {
		use_kerberos = 0;
		old_warning("remote host doesn't support Kerberos");
		sp = getservbyname("shell", "tcp");
		if (sp == NULL) {
			(void)fprintf(stderr,
			    "rcp: unknown service shell/tcp\n");
			exit(1);
		}
		port = sp->s_port;
		goto again;
	}
}
#endif /* KERBEROS */

verifydir(cp)
	char *cp;
{
	struct stat stb;

	if (stat(cp, &stb) >= 0) {
		if ((stb.st_mode & S_IFMT) == S_IFDIR)
			return;
		errno = ENOTDIR;
	}
	error("rcp: %s: %s.\n", cp, sys_errlist[errno]);
	exit(1);
}

char *
colon(cp)
	register char *cp;
{
	for (; *cp; ++cp) {
		if (*cp == ':')
			return(cp);
		if (*cp == '/')
			return(0);
	}
	return(0);
}

okname(cp0)
	char *cp0;
{
	register char *cp = cp0;
	register int c;

	do {
		c = *cp;
		if (c & 0200)
			goto bad;
		if (!isalpha(c) && !isdigit(c) && c != '_' && c != '-')
			goto bad;
	} while (*++cp);
	return(1);
bad:
	(void)fprintf(stderr, "rcp: invalid user name %s\n", cp0);
	return(0);
}

susystem(s)
	char *s;
{
	int status, pid, w;
	register void (*istat)(), (*qstat)();

	if ((pid = vfork()) == 0) {
		(void)setuid(userid);
		execl(_PATH_BSHELL, "sh", "-c", s, (char *)0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		status = -1;
	(void)signal(SIGINT, istat);
	(void)signal(SIGQUIT, qstat);
	return(status);
}

source(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	static BUF buffer;
	BUF *bp;
	off_t i;
	int x, readerr, f, amt;
	char *last, *name, buf[BIGBUFSIZ];
	BUF *allocbuf();

	for (x = 0; x < argc; x++) {
		name = argv[x];
		if ((f = open(name, O_RDONLY | O_NDELAY, 0)) < 0) {
			error("rcp: %s: %s\n", name, sys_errlist[errno]);
			continue;
		}
		if (fstat(f, &stb) < 0)
			goto notreg;
		switch (stb.st_mode&S_IFMT) {

		case S_IFREG:
			break;

		case S_IFDIR:
			if (iamrecursive) {
				(void)close(f);
				rsource(name, &stb);
				continue;
			}
			/* FALLTHROUGH */
		default:
notreg:			(void)close(f);
			error("rcp: %s: not a plain file\n", name);
			continue;
		}
		last = rindex(name, '/');
		if (last == 0)
			last = name;
		else
			last++;
		if (pflag) {
			/*
			 * Make it compatible with possible future
			 * versions expecting microseconds.
			 */
			(void)sprintf(buf, "T%ld 0 %ld 0\n", stb.st_mtime,
			    stb.st_atime);
			(void)write(rem, buf, strlen(buf));
			if (response() < 0) {
				(void)close(f);
				continue;
			}
		}
		(void)sprintf(buf, "C%04o %ld %s\n", stb.st_mode&07777,
		    stb.st_size, last);
		(void)write(rem, buf, strlen(buf));
		if (response() < 0) {
			(void)close(f);
			continue;
		}
		if ((bp = allocbuf(&buffer, f, BIGBUFSIZ)) == 0) {
			(void)close(f);
			continue;
		}
		readerr = 0;
		for (i = 0; i < stb.st_size; i += bp->cnt) {
			amt = bp->cnt;
			if (i + amt > stb.st_size)
				amt = stb.st_size - i;
			if (readerr == 0 && read(f, bp->buf, amt) != amt)
				readerr = errno;
			(void)write(rem, bp->buf, amt);
		}
		(void)close(f);
		if (readerr == 0)
			(void)write(rem, "", 1);
		else
			error("rcp: %s: %s\n", name, sys_errlist[readerr]);
		(void)response();
	}
}

rsource(name, statp)
	char *name;
	struct stat *statp;
{
	DIR *d;
	struct direct *dp;
	char *last, *vect[1], path[MAXPATHLEN];

	if (!(d = opendir(name))) {
		error("rcp: %s: %s\n", name, sys_errlist[errno]);
		return;
	}
	last = rindex(name, '/');
	if (last == 0)
		last = name;
	else
		last++;
	if (pflag) {
		(void)sprintf(path, "T%ld 0 %ld 0\n", statp->st_mtime,
		    statp->st_atime);
		(void)write(rem, path, strlen(path));
		if (response() < 0) {
			closedir(d);
			return;
		}
	}
	(void)sprintf(path, "D%04o %d %s\n", statp->st_mode&07777, 0, last);
	(void)write(rem, path, strlen(path));
	if (response() < 0) {
		closedir(d);
		return;
	}
	while (dp = readdir(d)) {
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(name) + 1 + strlen(dp->d_name) >= MAXPATHLEN - 1) {
			error("%s/%s: name too long.\n", name, dp->d_name);
			continue;
		}
		(void)sprintf(path, "%s/%s", name, dp->d_name);
		vect[0] = path;
		source(1, vect);
	}
	closedir(d);
	(void)write(rem, "E\n", 2);
	(void)response();
}

response()
{
	register char *cp;
	char ch, resp, rbuf[BUFSIZ];

	if (read(rem, &resp, sizeof(resp)) != sizeof(resp))
		lostconn();

	cp = rbuf;
	switch(resp) {
	case 0:				/* ok */
		return(0);
	default:
		*cp++ = resp;
		/* FALLTHROUGH */
	case 1:				/* error, followed by err msg */
	case 2:				/* fatal error, "" */
		do {
			if (read(rem, &ch, sizeof(ch)) != sizeof(ch))
				lostconn();
			*cp++ = ch;
		} while (cp < &rbuf[BUFSIZ] && ch != '\n');

		if (!iamremote)
			(void)write(2, rbuf, cp - rbuf);
		++errs;
		if (resp == 1)
			return(-1);
		exit(1);
	}
	/*NOTREACHED*/
}

lostconn()
{
	if (!iamremote)
		(void)fprintf(stderr, "rcp: lost connection\n");
	exit(1);
}

/*
 * This routine is called if rcp is interrupted while it is
 * writing a file.  It changes the file mode to 0, to be consistent
 * with cp.
 */

intr()
{
	/* Ignore further interrupts */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	/*
	 * The integer errfd is set to the file 
	 * to which we are writing.
	 */
	if (errfd != NULL) {
		fchmod(errfd, 0);
		close(errfd);
	}

	error("rcp: interrupted\n");
	exit(3);
}

sink(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	static BUF buffer;
	struct stat stb;
	struct timeval tv[2];
	BUF *bp, *allocbuf();
	off_t i, j;
	char ch, *targ, *why;
	int amt, count, exists, first, mask, mode;
	int ofd, setimes, size, targisdir, wrerr;
	char *np, *vect[1], buf[BIGBUFSIZ], *malloc();
	long oldsigmask;
	void (*oldint)(), (*oldhup)(), (*oldterm)(), (*oldquit)();

#define	atime	tv[0]
#define	mtime	tv[1]
#define	SCREWUP(str)	{ why = str; goto screwup; }

	setimes = targisdir = 0;
	mask = umask(0);
	if (!pflag)
		(void)umask(mask);
	if (argc != 1) {
		error("rcp: ambiguous target\n");
		exit(1);
	}
	targ = *argv;
	if (targetshouldbedirectory)
		verifydir(targ);
	(void)write(rem, "", 1);
	if (stat(targ, &stb) == 0 && (stb.st_mode & S_IFMT) == S_IFDIR)
		targisdir = 1;
	for (first = 1;; first = 0) {
		cp = buf;
		if (read(rem, cp, 1) <= 0)
			return;
		if (*cp++ == '\n')
			SCREWUP("unexpected <newline>");
		do {
			if (read(rem, &ch, sizeof(ch)) != sizeof(ch))
				SCREWUP("lost connection");
			*cp++ = ch;
		} while (cp < &buf[BIGBUFSIZ - 1] && ch != '\n');
		*cp = 0;

		if (buf[0] == '\01' || buf[0] == '\02') {
			if (iamremote == 0)
				(void)write(2, buf + 1, strlen(buf + 1));
			if (buf[0] == '\02')
				exit(1);
			errs++;
			continue;
		}
		if (buf[0] == 'E') {
			(void)write(rem, "", 1);
			return;
		}

		if (ch == '\n')
			*--cp = 0;

#define getnum(t) (t) = 0; while (isdigit(*cp)) (t) = (t) * 10 + (*cp++ - '0');
		cp = buf;
		if (*cp == 'T') {
			setimes++;
			cp++;
			getnum(mtime.tv_sec);
			if (*cp++ != ' ')
				SCREWUP("mtime.sec not delimited");
			getnum(mtime.tv_usec);
			if (*cp++ != ' ')
				SCREWUP("mtime.usec not delimited");
			getnum(atime.tv_sec);
			if (*cp++ != ' ')
				SCREWUP("atime.sec not delimited");
			getnum(atime.tv_usec);
			if (*cp++ != '\0')
				SCREWUP("atime.usec not delimited");
			(void)write(rem, "", 1);
			continue;
		}
		if (*cp != 'C' && *cp != 'D') {
			/*
			 * Check for the case "rcp remote:foo\* local:bar".
			 * In this case, the line "No match." can be returned
			 * by the shell before the rcp command on the remote is
			 * executed so the ^Aerror_message convention isn't
			 * followed.
			 */
			if (first) {
				error("%s\n", cp);
				exit(1);
			}
			SCREWUP("expected control record");
		}
		mode = 0;
		for (++cp; cp < buf + 5; cp++) {
			if (*cp < '0' || *cp > '7')
				SCREWUP("bad mode");
			mode = (mode << 3) | (*cp - '0');
		}
		if (*cp++ != ' ')
			SCREWUP("mode not delimited");
		size = 0;
		while (isdigit(*cp))
			size = size * 10 + (*cp++ - '0');
		if (*cp++ != ' ')
			SCREWUP("size not delimited");
		if (targisdir) {
			static char *namebuf;
			static int cursize;
			int need;

			need = strlen(targ) + strlen(cp) + 250;
			if (need > cursize) {
				if (!(namebuf = malloc((u_int)need)))
					error("out of memory\n");
			}
			(void)sprintf(namebuf, "%s%s%s", targ,
			    *targ ? "/" : "", cp);
			np = namebuf;
		}
		else
			np = targ;
		exists = stat(np, &stb) == 0;
		if (buf[0] == 'D') {
			if (exists) {
				if ((stb.st_mode&S_IFMT) != S_IFDIR) {
					errno = ENOTDIR;
					goto bad;
				}
				if (pflag)
					(void)chmod(np, mode);

			}
           		/* Create the destination directory with S_RWXU mode                               to allow files be written to.  The original mode 
			   will be restored back after the copying is done. */
			else if (mkdir(np, S_RWXU) < 0)
				goto bad;
			vect[0] = np;
			sink(1, vect);
			/* After all the files have been copied into the
			   directory, need to change the mode of the directory
			   back to original. */
			(void)chmod(np,mode);	
			if (setimes) {
				setimes = 0;
				if (utimes(np, tv) < 0)
				    error("rcp: can't set times on %s: %s\n",
					np, sys_errlist[errno]);
			}
			continue;
		}
		if ((ofd = open(np, O_WRONLY|O_CREAT, 0)) < 0) {
bad:			error("rcp: %s: %s\n", np, sys_errlist[errno]);
			continue;
		}
		/*
		 * Handle interruptions while we are writing the file.
		 */
		if (!exists || (stb.st_mode&S_IFMT) == S_IFREG)
			errfd = ofd;
		oldint = signal(SIGINT, intr);
		oldhup = signal(SIGHUP, intr);
		oldterm = signal(SIGTERM, intr);
		oldquit = signal(SIGQUIT, intr);
		(void)write(rem, "", 1);
		if ((bp = allocbuf(&buffer, ofd, BIGBUFSIZ)) == 0) {
			(void)close(ofd);
			continue;
		}
		cp = bp->buf;
		count = 0;
		wrerr = 0;
		for (i = 0; i < size; i += BIGBUFSIZ) {
			amt = BIGBUFSIZ;
			if (i + amt > size)
				amt = size - i;
			count += amt;
			do {
				j = read(rem, cp, amt);
				if (j <= 0) {
					error("rcp: %s\n",
					    j ? sys_errlist[errno] :
					    "dropped connection");
					exit(1);
				}
				amt -= j;
				cp += j;
			} while (amt > 0);
			if (count == bp->cnt) {
				if (wrerr == 0 &&
				    write(ofd, bp->buf, count) != count)
					wrerr++;
				count = 0;
				cp = bp->buf;
			}
		}
		if (count != 0 && wrerr == 0 &&
		    write(ofd, bp->buf, count) != count)
			wrerr++;
		if (!exists || (pflag && (stb.st_mode&S_IFMT) == S_IFREG))
			fchmod(ofd, 0600);
		if (ftruncate(ofd, size))
			error("rcp: can't truncate %s: %s\n", np,
			    sys_errlist[errno]);
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			/*
			**  if this process is appropriately privileged,
			**  the enablepriv will succeed and sticky bit
			**  will be preserved, else not.
			*/
			enablepriv(SEC_LOCK);
#endif /* SecureWare && B1 */
		if (!exists || (pflag && (stb.st_mode&S_IFMT) == S_IFREG))
			(void)fchmod(ofd, pflag ? mode : mode & ~mask);
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			disablepriv(SEC_LOCK);
#endif /* SecureWare && B1 */
		/*
		 * Restore signal handlers.
		 */
		(void) signal(SIGINT, oldint);
		(void) signal(SIGHUP, oldhup);
		(void) signal(SIGTERM, oldterm);
		(void) signal(SIGQUIT, oldquit);
		errfd = NULL;
		(void)close(ofd);
		(void)response();
		if (setimes) {
			setimes = 0;
			if (utimes(np, tv) < 0)
				error("rcp: can't set times on %s: %s\n",
				    np, sys_errlist[errno]);
		}				   
		if (wrerr)
			error("rcp: %s: %s\n", np, sys_errlist[errno]);
		else
			(void)write(rem, "", 1);
	}
screwup:
	error("rcp: protocol screwup: %s\n", why);
	exit(1);
}

BUF *
allocbuf(bp, fd, blksize)
	BUF *bp;
	int fd, blksize;
{
	struct stat stb;
	int size;
	char *malloc();

	if (fstat(fd, &stb) < 0) {
		error("rcp: fstat: %s\n", sys_errlist[errno]);
		return(0);
	}
	size = roundup(stb.st_blksize, blksize);
	if (size == 0)
		size = blksize;
	if (bp->cnt < size) {
		if (bp->buf != 0)
			free(bp->buf);
		bp->buf = (char *)malloc((u_int)size);
		if (!bp->buf) {
			error("rcp: malloc: out of memory\n");
			return(0);
		}
	}
	bp->cnt = size;
	return(bp);
}

/* VARARGS1 */
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	static FILE *fp;

	++errs;
	if (fp || (fp = fdopen(rem, "w"))) {
		(void)fprintf(fp, "%c", 0x01);
		(void)fprintf(fp, fmt, a1, a2, a3);
		(void)fflush(fp);
	}
	if (!iamremote)
		(void)fprintf(stderr, fmt, a1, a2, a3);
}

nospace()
{
	(void)fprintf(stderr, "rcp: out of memory.\n");
	exit(1);
}

#ifdef KERBEROS
old_warning(str)
	char *str;
{
	(void)fprintf(stderr, "rcp: warning: %s, using standard rcp\n", str);
}
#endif

usage()
{
#ifdef KERBEROS
	(void)fprintf(stderr, "%s\n\t%s\n",
	    "usage: rcp [-k realm] [-px] f1 f2",
	    "or: rcp [-k realm] [-rpx] f1 ... fn directory");
#else
	(void)fprintf(stderr,
	    "usage: rcp [-p] f1 f2; or: rcp [-rp] f1 ... fn directory\n");
#endif
	exit(1);
}
