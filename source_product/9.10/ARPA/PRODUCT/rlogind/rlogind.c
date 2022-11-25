/*
 * Copyright (c) 1983, 1988 The Regents of the University of California.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rlogind.c	5.22.1.6 (Berkeley) 2/7/89";
static char rcsid[] = "@(#)$Header: rlogind.c,v 1.13.109.6 93/10/26 17:31:57 donnad Exp $";
#endif /* not lint */

/*
 * remote login server:
 *	\0
 *	remuser\0
 *	locuser\0
 *	terminal_type/speed\0
 *	data
 *
 * Automatic login protocol is done here, using login -f upon success,
 * unless OLD_LOGIN is defined (then done in login, ala 4.2/4.3BSD).
 */

#ifdef SecureWare
#ifdef OLD_LOGIN
ERROR: "OLD_LOGIN and SecureWare #defines are incompatible";
#endif /* OLD_LOGIN */
#ifdef AUDIT
ERROR: "AUDIT and SecureWare #defines are incompatible";
#endif /* AUDIT */
#endif /* SecureWare */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>

#include <netinet/in.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <sys/ptyio.h>
#include <sys/ioctl.h>
#include <utmp.h>
#include <syslog.h>
#include <unistd.h>

#ifdef SecureWare
#include <sys/security.h>
#endif /* SecureWare */

#if defined(AUDIT) || defined(SecureWare)
#include <audnetd.h>

uid_t	ruid = (uid_t) -1, luid = (uid_t) -1;
u_long	raddr = (u_long) -1, laddr = (u_long) -1;
extern	void audit_daemon();

#ifdef AUDIT
#define SERVICE NA_SERV_RLOGIN

int	daemonaudid;
extern	int setaudid(), getaudid();

#ifndef NA_EVNT_IDENT
#define NA_EVNT_IDENT NA_EVNT_START
#endif NA_EVNT_IDENT

#define AUDIT_SUCCESS(event,s)	audit_daemon(NA_SERV_RLOGIN, NA_RSLT_SUCCESS, NA_VALI_RUSEROK, \
					     raddr, ruid, laddr, luid, \
					     event, NA_MASK_RUSR|NA_MASK_LUSR|NA_MASK_VALI, (s))
#define AUDIT_FAILURE(s)	audit_daemon(NA_SERV_RLOGIN, NA_RSLT_FAILURE, NA_VALI_RUSEROK, \
					     raddr, ruid, laddr, luid, \
					     0, NA_MASK_RUSR|NA_MASK_LUSR|NA_MASK_EVNT|NA_MASK_VALI, (s))
#endif /* AUDIT */

#ifdef SecureWare

char *rhost = (char *)0, *rusr = (char *)0, *lusr = (char *)0;

/*
**    Auditing disabled because audit_daemon routine not yet available.
**    To re-enable auditing, remove the #ifdef notdef and the code
**    from the #else to the #endif.
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

#define AUDIT_SUCCESS(event,s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_SUCCESS, NA_VALI_RUSEROK, \
			raddr, rhost, ruid, rusr, luid, lusr, event, \
			NA_MASK_RUID|NA_MASK_LUID|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0), (s)); \
		disable_priv_for_audit(); \
	}
#define AUDIT_FAILURE(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_FAILURE, NA_VALI_RUSEROK, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_START, \
			NA_MASK_RUID|NA_MASK_LUID|\
			(raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
			(rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0)|\
			(lusr == (char *)0 ? NA_MASK_LUSR : 0), (s)); \
		disable_priv_for_audit(); \
	}
#else /* not notdef */
#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
#endif /* not notdef */
#endif /* SecureWare */

#else /* neither AUDIT nor SecureWare */
#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
#endif /* neither AUDIT nor SecureWare */

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif /* TIOCPKT_WINDOW */

char	*env[2];
#define	NMAX 30
char	lusername[NMAX+1], rusername[NMAX+1];
static	char term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */
int	keepalive = 1;

#define	SUPERUSER(pwd)	((pwd)->pw_uid == 0)
#define SECUREPASS		"/.secure/etc/passwd"

extern	int errno;
int	reapchild();
struct	passwd *getpwnam(), *pwd;
struct  s_passwd  *s_pwd, *getspwnam();
char	*malloc(), *index(), *rindex();
struct stat s_pfile;

main(argc, argv)
	int argc;
	char **argv;
{
	extern int opterr, optind, _check_rhosts_file;
	int ch;
	int on = 1, fromlen;
	struct sockaddr_in from;
#ifdef AUDIT
	struct sockaddr_in myaddr;
	int myaddrlen;
#endif AUDIT

#ifdef SecureWare
	if (ISSECURE)
		set_auth_parameters(argc, argv);
#ifdef B1
	if (ISB1)
		initprivs();
#endif /* B1 */
#endif /* SecureWare */

	openlog("rlogind", LOG_PID | LOG_CONS, LOG_AUTH);

	opterr = 0;
	while ((ch = getopt(argc, argv, "ln")) != EOF)
		switch (ch) {
		case 'l':
			_check_rhosts_file = 0;
			break;
		case 'n':
			keepalive = 0;
			break;
		case '?':
		default:
			syslog(LOG_ERR, "usage: rlogind [-l] [-n]");
			break;
		}
	argc -= optind;
	argv += optind;

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		syslog(LOG_ERR, "Couldn't get peer name of remote host: %m");
		fatalperror("Can't get peer name of host");
	}
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
#if defined(AUDIT) || defined(SecureWare)
	/*
	 *	Establish remote and local host addresses for auditing.
	 */
	raddr = from.sin_addr.s_addr;
#endif /* AUDIT or SecureWare */
#ifdef AUDIT
	myaddrlen = sizeof(myaddr);
	if (getsockname(0, &myaddr, &myaddrlen) < 0) {
		fatalperror(0,"rlogind: getsockname",errno);
		exit(1);
	}
	laddr = myaddr.sin_addr.s_addr;
#endif AUDIT
	doit(0, &from);
}

int	child;
int	cleanup();
int	netf, netp;
char	master[MAXPATHLEN];
char 	slave[MAXPATHLEN];
char	tname[MAXPATHLEN];
int	pid;
extern	char	*inet_ntoa();

#ifdef TIOCSWINSZ
struct winsize win = { 0, 0, 0, 0 };
#else /* ~TIOCWINSIZ */
/*
 * Window/terminal size structure.
 * This information is stored by the kernel
 * in order to provide a consistent interface,
 * but is not used by the kernel.
 *
 * Type must be "unsigned short" so that types.h not required.
 */
struct winsize {
        unsigned short  ws_row;                 /* rows, in characters */
        unsigned short  ws_col;                 /* columns, in characters */
        unsigned short  ws_xpixel;              /* horizontal size, pixels */
        unsigned short  ws_ypixel;              /* vertical size, pixels */
};
#endif /* ~TIOCWINSIZ */


doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	int i, p, t, t_, on = 1;
	int hostok = 0;
#ifndef OLD_LOGIN
	int authenticated = 0;
	char remotehost[2 * MAXHOSTNAMELEN + 1];
#endif /* ~OLD_LOGIN */
	register struct hostent *hp;
	struct hostent hostent;
	char c, *tn, *ttyname();
	char *saved_name;
#ifdef AUDIT
	char buf[255];
#endif /* AUDIT */

	alarm(60);
	read(f, &c, 1);
	if (c != 0)
		exit(1);

	alarm(0);
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	errno = 0;
	hp = gethostbyaddr(&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp == 0) {
		/*
		 * Only the name is used below.
		 */
		hp = &hostent;
		hp->h_name = inet_ntoa(fromp->sin_addr);
#ifndef OLD_LOGIN
		hostok++;
#endif /* ~OLD_LOGIN */
	}
	else if (errno != ECONNREFUSED) {
		/*
		 * If name returned by gethostbyaddr is from the nameserver,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
 		long oldoptions = _res.options;

		strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
		remotehost[sizeof(remotehost) - 1] = 0;
 		if (strchr(remotehost, '.') != NULL)
 			_res.options &= ~(RES_DEFNAMES | RES_DNSRCH);
		hp = gethostbyname(remotehost);
		_res.options = oldoptions;
		if (hp)
		    for (; hp->h_addr_list[0]; hp->h_addr_list++)
			if (!bcmp(hp->h_addr_list[0], (caddr_t)&fromp->sin_addr,
			    sizeof(fromp->sin_addr))) {
				hostok++;
				break;
			}
		if (!hostok)
#ifdef OLD_LOGIN
		{
			hp = &hostent;
			hp->h_name = inet_ntoa(fromp->sin_addr);
		}
#else  /* ~OLD_LOGIN */
		{
			/*
			**  Audit here because we would like to know
			**  about this case even if ruserok() fails
			**  to authenticate.
			*/
			AUDIT_FAILURE("Host address mismatch.");
		}
#endif /* OLD_LOGIN */
	}
#ifndef OLD_LOGIN
	else
		hostok++;
#endif /* OLD_LOGIN */

	{
		/* Save remote host name for utmp entry */
		int len = strlen(hp->h_name);
		if ((saved_name = malloc(len + 1)) == NULL)
			fatal(f, "malloc() failure");
		strcpy(saved_name, hp->h_name);
	}

	if (fromp->sin_family != AF_INET ||
	    fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
			inet_ntoa(fromp->sin_addr));
		AUDIT_FAILURE("Source port not reserved.");
		fatal(f, "Permission denied");
	}
#ifdef IP_OPTIONS
      {
	u_char optbuf[BUFSIZ/3], *cp;
	char lbuf[BUFSIZ], *lp;
	int optsize = sizeof(optbuf), ipproto;
	struct protoent *ip;

	if ((ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;
	if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf, &optsize) == 0 &&
	    optsize != 0) {
		lp = lbuf;
		for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
			sprintf(lp, " %2.2x", *cp);
		syslog(LOG_NOTICE,
		    "Connection received using IP options (ignored):%s", lbuf);
		if (setsockopt(0, ipproto, IP_OPTIONS,
		    (char *)NULL, &optsize) != 0) {
			syslog(LOG_ERR, "setsockopt IP_OPTIONS NULL: %m");
			exit(1);
		}
	}
      }
#endif /* IP_OPTIONS */
	write(f, "", 1);
#ifndef OLD_LOGIN
	if (do_rlogin(hp->h_name) == 0) {
		if (hostok)
			authenticated++;
		else {
			error(f, "Host address mismatch.");
	        }
	}
#endif /* ~OLD_LOGIN */

#if defined(SecureWare) && defined(B1)
	/*
	**    OWNER required to chmod/chown the pty;
	**    ALLOWDACACCESS required to open the pty master.
	*/
	if (ISB1) {
		enablepriv(SEC_OWNER);
		enablepriv(SEC_ALLOWDACACCESS);
	}
#endif /* SecureWare && B1 */
	if (getpty(&p, master, slave) != 0)
	    fatal(f, "Unable to allocate pty on remote host");
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_OWNER);
		disablepriv(SEC_ALLOWDACACCESS);
	}
#endif /* SecureWare && B1 */

	/*
	** Global copies for use by SIGCHLD handler
	*/
	netf = f;
	netp = p;

	/*
	** Slave becomes controlling terminal for this session.
	*/

#if defined(SecureWare) && defined(B1)
	if (ISB1)
		enablepriv(SEC_ALLOWDACACCESS);
#endif /* SecureWare && B1 */

	t_ = open(slave, O_RDWR);
	if (t_ < 0)
		fatalperror(f, slave);
	/* 
	  Remove any reference to the 
	  current control terminal 
	 */
	if (vhangup() < 0)
		fatalperror(f, "rlogind: vhangup()");

	/* reopen the slave pseudo-terminal */
	t = open(slave, O_RDWR);
	if (t < 0)
		fatalperror(f, slave);
	close(t_); 

#if defined(SecureWare) && defined(B1)
	if (ISB1)
		disablepriv(SEC_ALLOWDACACCESS);
#endif /* SecureWare && B1 */

	/*
	 * Get the line name for utmp accounting.
	 */
	tn = ttyname(t);
	if (tn != NULL)
		strcpy(tname, tn);
	else
		strcpy(tname, slave);

	/*
	 * Initialize the pty.
	 */
#ifdef TIOCSWINSZ
	(void) ioctl(p, TIOCSWINSZ, &win);
#endif /* TIOCSWINSZ */
#ifndef OLD_LOGIN
	setup_term(t);
#endif /* ~OLD_LOGIN */


#if defined(SecureWare) && defined(B1)
	if (ISB1)
		enablepriv(SEC_LIMIT);
#endif /* SecureWare && B1 */
	/*
	** Lock the pty so that the parent can then synchronize setting up
	** the child's process group and controlling terminal with the
	** login process.  The login process won't be able to read from
	** the pty until the parent unlocks it.
	*/
	lockf(p, F_LOCK, 0);

	pid = fork();
	if (pid < 0)
		fatalperror(f, "");

#if defined(SecureWare) && defined(B1)
	if (ISB1)
		disablepriv(SEC_LIMIT);
#endif /* SecureWare && B1 */
	if (pid == 0) {
		/*
		**	CHILD
		*/
		char audbuf[64];
	        
		/*
		** Wait for the parent to setup our process group and
		** controlling tty.
		*/
		lockf(p, F_LOCK, 0);
		lockf(p, F_ULOCK, 0);
		close(f), close(p);
		dup2(t, 0), dup2(t, 1), dup2(t, 2);
		close(t);
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
			enablepriv(SEC_ALLOWDACACCESS);
			enablepriv(SEC_ALLOWMACACCESS);
		}
#endif /* SecureWare && B1 */
		account(LOGIN_PROCESS,getpid(),"LOGIN",tname,saved_name, fromp);
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
			disablepriv(SEC_ALLOWMACACCESS);
			disablepriv(SEC_ALLOWDACACCESS);
		}
#endif /* SecureWare && B1 */
#ifdef OLD_LOGIN
		AUDIT_SUCCESS(NA_EVNT_START, NULL);
		execl("/bin/login", "login", "-r", hp->h_name, 0);
#else /* ~OLD_LOGIN */
		if (authenticated) {
			sprintf(audbuf, "exec \"login -p -h %s -f %s\" (%s)",
				hp->h_name, lusername, tname);
			AUDIT_SUCCESS(NA_EVNT_START, audbuf);
#ifdef SecureWare
			if (ISSECURE) {
#ifdef B1
				if (ISB1) {
					enablepriv(SEC_ALLOWDACACCESS);
				}
#endif /* B1 */
				execl("/tcb/lib/login", "login", "-p", "-h",
				    hp->h_name, "-f", lusername, 0);
			} else
#endif /* SecureWare */
			{
				execl("/bin/login", "login", "-p", "-h",
				    hp->h_name, "-f", lusername, 0);
			}
		} else {
			sprintf(audbuf, "exec \"login -p -h %s %s\" (%s)",
				hp->h_name, lusername, tname);
			AUDIT_SUCCESS(NA_EVNT_IDENT, audbuf);
#ifdef SecureWare
			if (ISSECURE) {
#ifdef B1
				if (ISB1) {
					enablepriv(SEC_ALLOWDACACCESS);
				}
#endif /* B1 */
				execl("/tcb/lib/login", "login", "-p", "-h",
				    hp->h_name, lusername, 0);
			} else
#endif /* SecureWare */
			{
				execl("/bin/login", "login", "-p", "-h",
				    hp->h_name, lusername, 0);
			}
#endif /* ~OLD_LOGIN */
		}
		fatalperror(2, "/bin/login");
		/*NOTREACHED*/
	}

	/*
	**	PARENT
	*/

	/*
	** Set the child up as a process group leader.
	** It must not be part of the parent's process group,
	** because signals issued in the child's process group
	** must not affect the parent.
	*/
	setpgid(pid, pid);

	/*
	** Make the pty slave the controlling terminal for the
	** child's process group.
	*/
	tcsetpgrp(t, pid);

	/*
	** Close our controlling terminal.
	*/
	close(t);

	/*
	** Let the child know that it is a process group
	** leader and has a controlling tty.
	*/
	lockf(p, F_ULOCK, 0);

	ioctl(f, FIONBIO, &on);
	ioctl(p, FIONBIO, &on);
	ioctl(p, TIOCPKT, &on);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, cleanup);
	signal(SIGTERM, cleanup,
           sigmask(SIGCLD) | sigmask(SIGTERM) | sigmask(SIGPIPE));
    signal(SIGPIPE, cleanup,
           sigmask(SIGCLD) | sigmask(SIGTERM) | sigmask(SIGPIPE));
	setpgid(0, 0);
	protocol(f, p);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	cleanup();
}

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
control(pty, cp, n)
	int pty;
	char *cp;
	int n;
{
	struct winsize w;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */
	bcopy(cp+4, (char *)&w, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
#ifdef TIOCSWINSZ
	(void)ioctl(pty, TIOCSWINSZ, &w);
#endif /* TIOCWINSZ */
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
protocol(f, p)
	int f, p;
{
	char pibuf[1024], fibuf[1024], *pbp, *fbp;
	register pcc = 0, fcc = 0;
	int cc, nfd, pmask, fmask;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	if (f > p)
		nfd = f + 1;
	else
		nfd = p + 1;
	fmask = 1 << f;
	pmask = 1 << p;
	for (;;) {
		int ibits, obits, ebits;

		ibits = 0;
		obits = 0;
		if (fcc)
			obits |= pmask;
		else
			ibits |= fmask;
		if (pcc >= 0)
			if (pcc)
				obits |= fmask;
			else
				ibits |= pmask;
		ebits = pmask;
		if (select(nfd, &ibits, obits ? &obits : (int *)NULL,
		    &ebits, 0) < 0) {
			if (errno == EINTR)
				continue;
			fatalperror(f, "select");
		}
		if (ibits == 0 && obits == 0 && ebits == 0) {
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (ebits & pmask) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					ibits &= ~pmask;
				}
			}
		}
		if (ibits & fmask) {
			fcc = read(f, fibuf, sizeof(fibuf));
			if (fcc < 0 && errno == EWOULDBLOCK)
				fcc = 0;
			else {
				register char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								bcopy(cp+n, cp, left);
							fcc -= n;
							goto top; /* n^2 */
						}
					}
				obits |= pmask;		/* try write */
			}
		}

		if ((obits & pmask) && fcc > 0) {
			cc = write(p, fbp, fcc);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (ibits & pmask) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else if (pcc <= 0)
				break;
			else if (pibuf[0] == 0) {
				pbp++, pcc--;
				obits |= fmask;	/* try a write */
			} else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
				}
				pcc = 0;
			}
		}
		if ((obits & fmask) && pcc > 0) {
			cc = write(f, pbp, pcc);
			if (cc < 0 && errno == EWOULDBLOCK) {
				continue;
			}
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

cleanup()
{
	char buf[BUFSIZ];
	int cc;

	/*
	** require non-blocking I/O on the pty.
	*/
	fcntl(netp, F_SETFL, O_NDELAY);

	/*
	** half-hearted attempt to flush remaining data from pty.
	*/
	if ((cc = read(netp, buf, sizeof buf)) > 0) {
		write(netf, buf, cc);
	}

	/*
	** clean up the child's utmp entry.
	*/
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		enablepriv(SEC_ALLOWDACACCESS);
		enablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
	account(DEAD_PROCESS,pid,"",tname,NULL,NULL);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_ALLOWDACACCESS);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
	/*
	** restore pty device files to their unused state
	*/
#ifdef SecureWare
	if (ISSECURE) {
		rlogind_condition_line(master);
		rlogind_condition_line(slave);
	}
	else
#endif /* SecureWare */
	{
		chmod(master,0666);
		chown(master, 0, 0);
		chmod(slave,0666);
		chown(slave, 0, 0);
	}

	/*
	** shut down the socket for all further reads/writes and
	** then close it (implicit in exit()) and go home ...
	*/
	shutdown(netf, 2);
	exit(1);
}


error(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	buf[0] = '\01';		/* error indicator */
	(void) sprintf(buf + 1, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, strlen(buf));
}

fatal(f, msg)
	int f;
	char *msg;
{
	error(f, msg);
	exit(1);
}

fatalperror(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];
	extern int sys_nerr;
	extern char *sys_errlist[];

	if ((unsigned)errno < sys_nerr)
		(void) sprintf(buf, "%s: %s", msg, sys_errlist[errno]);
	else
		(void) sprintf(buf, "%s: Error %d", msg, errno);
	fatal(f, buf);
}


#ifndef OLD_LOGIN
do_rlogin(host)
	char *host;
{
	int ru, isp;

	getstr(rusername, sizeof(rusername), "remuser");
	getstr(lusername, sizeof(lusername), "locuser");
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type");

#ifdef SecureWare
	if (ISSECURE) {
		/* fail if not primordial */
		if (!(getluid() < 0 && errno == EPERM)) {
			AUDIT_FAILURE("Not primordial.");
			return(-1);
		}
	} else
#endif /* SecureWare */
	{
		if (getuid())
			return(-1);
	}

/* If /.secure/etc/passwd file exists then make sure that user has
 * a valid entry in both password files.
 * Initialize s_pwd to point to pwd so check won't fail if the
 * secure password file doesn't exists.
 */
	pwd = getpwnam(lusername);
	s_pwd = (struct s_passwd *) pwd;
	if (stat(SECUREPASS, &s_pfile) == 0)
		s_pwd = getspwnam(lusername);
	if (pwd == NULL || s_pwd == NULL) {
		AUDIT_FAILURE("No valid password entry.");
		return(-1);
	}

#if defined(AUDIT) || defined(SecureWare)
	luid = pwd->pw_uid;
#endif /* AUDIT  or SecureWare */
#ifdef SecureWare
	if (ISSECURE)
		if (rlogind_check_prpw(pwd) < 0)
			return(-1);
#ifdef B1
	if (ISB1) {
		isp = rlogind_is_priv(pwd);
		enablepriv(SEC_ALLOWDACACCESS);
		ru = ruserok(host, isp, rusername, lusername);
		disablepriv(SEC_ALLOWDACACCESS);
		if (ru < 0)
			AUDIT_FAILURE("Access denied by ruserok.");
		return(ru);
	}
	else
#endif /* B1 */
#endif /* SecureWare */
	{
		ru = ruserok(host, SUPERUSER(pwd), rusername, lusername);
	}
	if (ru < 0)
		AUDIT_FAILURE("Access denied by ruserok.");
	return(ru);
}


getstr(buf, cnt, errmsg)
	char *buf;
	int cnt;
	char *errmsg;
{
	char c;
	char msgbuf[255];

	do {
		if (read(0, &c, 1) != 1) {
#ifdef AUDIT
			sprintf(msgbuf, "Cannot read %s", errmsg);
			AUDIT_FAILURE(msgbuf);
#endif /* AUDIT */
			exit(1);
		}
		if (--cnt < 0) {
			sprintf(msgbuf, "%s too long", errmsg);
			fatal(1, msgbuf);
		}
		*buf++ = c;
	} while (c != 0);
}

extern	char **environ;

char *speeds[] = {
    "0", "50", "75", "110", "134", "150", "200", "300",
    "600", "900", "1200", "1800", "2400", "3600", "4800", "7200",
    "9600", "19200", "38400", "EXTA","EXTB"
};
#define	NSPEEDS	(sizeof (speeds) / sizeof (speeds[0]))

setup_term(fd)
    int fd;
{
	register char *cp = index(term, '/'), **cpp;
	char *speed;
        struct termios tp;

	tcgetattr(fd, &tp);
	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = index(speed, '/');
		if (cp)
			*cp++ = '\0';
		for (cpp = speeds; cpp < &speeds[NSPEEDS]; cpp++)
		    if (strcmp(*cpp, speed) == 0) {
			    tp.c_cflag &= ~CBAUD;
			    tp.c_cflag |= cpp - speeds;
			    break;
		    }
	}
	tp.c_iflag &= ~INPCK;
	tp.c_iflag |= ICRNL|IXON;
	tp.c_oflag |= OPOST|ONLCR|TAB3;
	tp.c_oflag &= ~ONLRET;
	tp.c_lflag |= (ECHO|ECHOE|ECHOK|ISIG|ICANON);
	tp.c_cflag &= ~PARENB;
	tp.c_cflag |= CS8;
	tp.c_cc[VMIN] = 1;
	tp.c_cc[VTIME] = 0;
	tp.c_cc[VEOF] = CEOF;

	tcsetattr(fd, TCSAFLUSH, &tp);

	env[0] = term;
	env[1] = 0;
	environ = env;
}
#endif /* ~OLD_LOGIN */

