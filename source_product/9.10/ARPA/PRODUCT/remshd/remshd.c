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

/*
 * Merge of remshd and rexecd.
 * To compile as rexecd, define REXECD.
 * To compile as remshd, define REMSHD.
 */

#if defined(REMSHD) && defined(REXECD)
ERROR: "Both REMSHD and REXECD defined";
#endif /* both REMSHD and REXECD */

#if !defined(REMSHD) && !defined(REXECD)
ERROR: "Neither REMSHD nor REXECD defined";
#endif /* neither REMSHD nor REXECD */

#if defined(SecureWare) && defined(AUDIT)
ERROR: "AUDIT and SecureWare #defines are incompatible";
#endif /* SecureWare && AUDIT */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifdef REMSHD
#ifndef lint
static char rcsid[] = "@(#)remshd.c $Revision: 1.32.109.2 $";
static char sccsid[] = "@(#)rshd.c	5.17.1.2 (Berkeley) 2/7/89";
#endif /* not lint */
#endif REMSHD

#ifdef REXECD
#ifndef lint
static char rcsid[] = "@(#)rexecd.c $Revision: 1.32.109.2 $";
static char sccsid[] = "@(#)rexecd.c	5.7 (Berkeley) 1/4/89";
#endif /* not lint */
#endif REXECD

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>

#include <arpa/nameser.h>
#include <resolv.h>

#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#endif /* SecureWare */

#if defined(AUDIT) || defined(SecureWare)
#include <audnetd.h>
char *rhost = (char *)0, *rusr = (char *)0, *lusr = (char *)0;
uid_t	ruid = (uid_t) -1, luid = (uid_t) -1;
u_long	raddr = (u_long) -1, laddr = (u_long) -1;
int	daemonaudid;

#ifdef REXECD
#define SERVICE NA_SERV_REXECD
u_short validation = NA_VALI_PASSWD;
#endif REXECD
#ifdef REMSHD
#define SERVICE NA_SERV_REMSH
u_short validation = NA_VALI_RUSEROK;
#endif REMSHD

/*
**    Auditing disabled because audit_daemon routine not yet available.
**    To re-enable auditing, remove the #ifdef notdef and the code
**    from the #else to the #endif.
*/

#ifdef SecureWare
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
		audit_daemon(NA_RSLT_SUCCESS, validation, \
			raddr, rhost, ruid, rusr, luid, lusr, event, \
			NA_MASK_RUID|NA_MASK_LUID|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0), (s)); \
		disable_priv_for_audit(); \
	}
#define AUDIT_FAILURE(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_FAILURE, validation, \
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

#ifdef AUDIT
#define AUDIT_SUCCESS(event,s)	audit_daemon(SERVICE, NA_RSLT_SUCCESS, \
		validation, raddr, ruid, laddr, luid, \
		event, NA_MASK_RUSR|(luid < 0 ? NA_MASK_LUSR : 0), (s))
#define AUDIT_FAILURE(s)        audit_daemon(SERVICE, NA_RSLT_FAILURE, \
		validation, raddr, ruid, laddr, luid, \
		0, NA_MASK_RUSR|NA_MASK_EVNT|(luid < 0 ? NA_MASK_LUSR : 0), (s))
#endif /* AUDIT */

#else /* !defined(AUDIT) && !defined(SecureWare) */
#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
#endif /* defined(AUDIT) || defined(SecureWare) */

int	errno;
int	keepalive = 1;
struct	passwd *getpwnam();
char	*crypt();
/*VARARGS1*/
int	error(), sperror();

extern char *optarg;
extern int optind, opterr;

static char *servname;	/* Runtime name of server */

/*
 * remote shell server:
 *	[port]\0
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 *
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */

main(argc, argv)
	int argc;
	char **argv;

{
	extern int opterr, optind, _check_rhosts_file;
	struct linger linger;
	int ch, on = 1, fromlen;
	struct sockaddr_in from;
#ifdef AUDIT
	struct sockaddr_in myaddr;
	int myaddrlen;
#endif AUDIT

#ifdef SecureWare
	if (ISSECURE) {
		set_auth_parameters(argc, argv);
#ifdef B1
		if (ISB1)
			initprivs();
#endif /* B1 */
	}
#endif /* SecureWare */

	/*
	 * Determine the name of the server at runtime, even
	 * though our functionality is determined at compile
         * time (this is for error messages).
	 */
	servname = (char *)strrchr (argv[0], '/');
	if (servname == (char *)NULL)
		servname = argv[0];
	else
		servname++;
	
	openlog(servname, LOG_PID | LOG_ODELAY, LOG_DAEMON);

	opterr = 0;
	while ((ch = getopt(argc, argv, "ln")) != EOF)
		switch((char)ch) {
		case 'l':
#ifdef REMSHD
			_check_rhosts_file = 0;
#endif /* REMSHD */
			break;
		case 'n':
			keepalive = 0;
			break;
		case '?':
		default:
#ifdef REMSHD
			syslog(LOG_ERR, "usage: %s [-ln]", servname);
#endif /* REMSHD */
#ifdef REXECD
			syslog(LOG_ERR, "usage: %s [-n]", servname);
#endif /* REXECD */
			break;
		}

	argc -= optind;
	argv += optind;

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0)
	{
		if (errno == ENOTSOCK)
			fprintf(stderr, "%s: Must be executed by inetd(1M).\n",
				servname);
		else
			sperror("getpeername");
		_exit(1);
	}
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
		       sizeof(on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;			/* XXX */
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
		       sizeof (linger)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
#if defined(AUDIT) || defined(SecureWare)
	/*
	 *	Establish remote host address for auditing.
	 */
	raddr = from.sin_addr.s_addr;
#endif /* AUDIT or SecureWare */
#ifdef AUDIT
	myaddrlen = sizeof(myaddr);
	if (getsockname(0, &myaddr, &myaddrlen) < 0) {
		sperror("getsockname");
		exit(1);
	}
	laddr = myaddr.sin_addr.s_addr;
#endif AUDIT
	doit(&from);
}

char	username[20] = "LOGNAME=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
struct	sockaddr_in asin = { AF_INET };

doit(fromp)
	struct sockaddr_in *fromp;
{
	char cmdbuf[NCARGS+1], *cp, *namep;
	char locuser[16], remuser[16], pass[16];
	struct passwd *pwd;
	int s;
	struct hostent *hp;
	char *hostname;
	short port;
	int pv[2], pid, cc;
	int nfd;
	fd_set ready, readfrom;
	char buf[BUFSIZ], sig;
	int one = 1;
	char remotehost[2 * MAXHOSTNAMELEN + 1];
	extern struct state _res;
	int save_errno;

#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
#ifdef TIOCNOTTY
		ioctl(t, TIOCNOTTY, (char *)0);
#else
		setpgrp();
#endif /* TIOCNOTTY */
		(void) close(t);
	  }
	}
#endif

	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	if (fromp->sin_family != AF_INET) {
		syslog(LOG_ERR, "malformed from address\n");
		exit(1);
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
#endif

#ifdef REMSHD
	/*
	 * check that the client's source port is a reserved port.
	 */
	if (fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2) {
		AUDIT_FAILURE("Client source port not reserved.");
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
			inet_ntoa(fromp->sin_addr));
		exit(1);
	}
#endif REMSHD

	/*
	**	only wait for 60 seconds for the secondary port number
	**	to come through from the remote ... if the remote is too
	**	slow then we will just give up
	*/
	(void) alarm(60);
	port = 0;
	/*
	**	read the secondary port number from the primary port;
	**	if non-null then we must establish the secondary connection
	*/
	for (;;) {
		char c;
		if ((cc = read(0, &c, 1)) != 1) {
			if (cc < 0) {
				syslog(LOG_NOTICE, "read: %m");
				sperror("read");
			}
			shutdown(0, 1+1);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
#ifdef REMSHD
		int lport = IPPORT_RESERVED - 1;	/* for remshd port */
		/*
		**	If we have a secondary port to connect back to,
		**	then try to establish the connection.
		**	Error messages go down the socket.
		*/
#if defined(SecureWare) && defined(B1)
		/*
		**      REMOTE privilege required to bind to a reserved port
		*/
		if (ISB1)
			enablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
		s = rresvport(&lport);
		save_errno = errno;
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
		if (s < 0) {
			errno = save_errno;
			syslog(LOG_ERR, "rresvport: %m");
			sperror("rresvport");
			exit(1);
		}
		if (port >= IPPORT_RESERVED) {
			AUDIT_FAILURE("Secondary port not reserved.");
			syslog(LOG_ERR, "Secondary port not reserved");
			exit(1);
		}
#endif REMSHD
#ifdef REXECD
		s = socket(AF_INET, SOCK_STREAM, 0, 0);
		if (s < 0)  {
			syslog(LOG_ERR, "socket: %m");
			sperror ("socket");
			exit(1);
		}
		if (bind(s, &asin, sizeof (asin), 0) < 0)  {
			syslog(LOG_ERR, "bind: %m");
			sperror("bind");
			exit(1);
		}
#endif REXECD
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0) {
			syslog(LOG_INFO, "connect second port: %m");
			sperror("connect");
			exit(1);
		}
	}

	errno = 0;
	hp = gethostbyaddr(&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp) {
		long oldoptions = _res.options;
		/*
		 * Attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		if (errno != ECONNREFUSED) {
			strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
			remotehost[sizeof(remotehost) - 1] = 0;
			if (strchr(remotehost, '.') != NULL)
				_res.options &= ~(RES_DEFNAMES | RES_DNSRCH);
			hp = gethostbyname(remotehost);
			_res.options = oldoptions;
			if (hp == NULL) {
				syslog(LOG_INFO,
				    "Couldn't look up address for %s",
				    remotehost);
				error("Couldn't look up address for your host");
				AUDIT_FAILURE("Couldn't look up client address.");
				exit(1);
			} else for (; ; hp->h_addr_list++) {
				if (!bcmp(hp->h_addr_list[0],
				    (caddr_t)&fromp->sin_addr,
				    sizeof(fromp->sin_addr)))
					break;
				if (hp->h_addr_list[0] == NULL) {
					syslog(LOG_NOTICE,
					  "Host addr %s not listed for host %s",
					    inet_ntoa(fromp->sin_addr),
					    hp->h_name);
					error("Host address mismatch");
					AUDIT_FAILURE("Host address mismatch.");
					exit(1);
				}
			}
		}
		hostname = hp->h_name;
#ifdef SecureWare
		rhost = hostname;
#endif /* SecureWare */
	} else
		hostname = inet_ntoa(fromp->sin_addr);

#ifdef REMSHD
	/*
	**	read the local user, remote user, and command information
	**	from the other side (remshd protocol).
	*/
	getstr(remuser, sizeof(remuser), "remuser");
#ifdef SecureWare
	rusr = remuser;
#endif /* SecureWare */
	getstr(locuser, sizeof(locuser), "locuser");
#endif REMSHD
#ifdef REXECD
	/*
	**	read the user name, password, and command information
	**	from the other side (rexecd protocol).
	*/
	getstr(locuser, sizeof(locuser), "username");
	getstr(pass, sizeof(pass), "password");
#endif REXECD
#ifdef SecureWare
	lusr = locuser;
#endif /* SecureWare */
	getstr(cmdbuf, sizeof(cmdbuf), "command");

	/*
	**	look up the local user in /etc/passwd
	*/
	setpwent();
	pwd = getpwnam(locuser);
	if (pwd == NULL) {
		error("Login incorrect");
		AUDIT_FAILURE("No passwd entry.");
		exit(1);
	}
	/* 
	  User/group ids should be in the range allowed by HP-UX,
	    otherwise a user like "nobody" on SUN systems
	    could gain root access (if system administrator
	    is careless).
	 */
	if (pwd->pw_uid > MAXUID || pwd->pw_uid < 0 ||
	    pwd->pw_gid > MAXUID || pwd->pw_gid < 0) {
		error("Invalid ID");
		AUDIT_FAILURE("Invalid ID.");
		exit(1);
	}
#if defined(AUDIT) || defined(SecureWare)
	luid = pwd->pw_uid;
#endif /* AUDIT  or SecureWare */
	endpwent();
#ifdef REXECD
	/*
	**	verify the password sent over
	*/
#ifdef SecureWare
	if (ISSECURE) {
#ifdef B1
		if (ISB1)
			enablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */
		if (remshd_check_prpw(pwd) < 0
#ifdef B1
		|| ((ISB1) && (remshd_setclrnce(pwd) < 0))
#endif /* B1 */
		) {
			exit(1);
		}
#ifdef B1
		if (ISB1)
			disablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */
		if (rexecd_check_passwd(pass) != 0) {
			error("Login incorrect");
			AUDIT_FAILURE("Failed password verification.");
			exit(1);
		}
	} else
#endif /* SecureWare */
	       {
		if (*pwd->pw_passwd != '\0') {
			namep = crypt(pass, pwd->pw_passwd);
			if (strcmp(namep, pwd->pw_passwd)) {
				error("Login incorrect");
				AUDIT_FAILURE("Failed password verification.");
				exit(1);
			}
		}
	}
#endif REXECD
	/*
	**	chdir to the user's $HOME directory, if possible.
	*/
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		enablepriv(SEC_ALLOWDACACCESS);
#endif /* SecureWare && B1 */
	if (chdir(pwd->pw_dir) < 0) {
		error("No remote directory");	
		AUDIT_FAILURE("Failed chdir to home directory.");
		exit(1);
	}
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		disablepriv(SEC_ALLOWDACACCESS);
#endif /* SecureWare && B1 */
#ifdef REMSHD
#ifdef SecureWare
	if (ISSECURE) {
		/*
		**  remote user is allowed in as local user if:
		**    local user has a password entry;
		**    local user has a password;
		**    local user's account is not administratively locked;
		**    local user's clearance dominates current sensitivity,
		**      set when connection was made;
		**    ruserok check passes.
		*/
		if (remshd_check_prpw(pwd) < 0
#ifdef B1
		    || ((ISB1) && (remshd_setclrnce(pwd) < 0))
#endif /* B1 */
		) {
			exit(1);
		}
#ifdef B1
		if (ISB1)
			enablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */
		if (ruserok(hostname, remshd_is_priv(pwd),
		    remuser, locuser) < 0) {
			error("Login incorrect");
			AUDIT_FAILURE("Access denied by ruserok.");
			exit(1);
		}
#ifdef B1
		if (ISB1)
			disablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */
	} else
#endif /* SecureWare */
	       {
		/*
		**  remote user is allowed in as local user if:
		**    local user has a password entry;
		**    local user has a password;
		**    ruserok check passes.
		*/
		if (pwd->pw_passwd != 0 && *pwd->pw_passwd != '\0' &&
		    ruserok(hostname, pwd->pw_uid == 0, remuser, locuser) < 0) {
			error("Login incorrect");
			AUDIT_FAILURE("Access denied by ruserok.");
			exit(1);
		}
	}
#endif REMSHD
#ifdef NOTDEF
	if (pwd->pw_uid && !access("/etc/nologin", F_OK)) {
		error("Logins currently disabled.");
		AUDIT_FAILURE("Access denied by /etc/nologin");
		exit(1);
	}
#endif NOTDEF
#ifdef AUDIT
	daemonaudid = getaudid();
	setaudid(pwd->pw_audid);	/* Should check return value. */
	AUDIT_SUCCESS(NA_EVNT_START, NULL);
#endif AUDIT
#ifdef SecureWare
	{
		char audbuf[(sizeof cmdbuf) + 20];

		sprintf(audbuf, "Command = \"%s\"", cmdbuf);
		AUDIT_SUCCESS(NA_EVNT_START, audbuf);
	}
#endif /* SecureWare */
	/*
	**	send a NULL byte, indicating that we have set everything up
	**	correctly, and that the flow of data may begin!
	*/
	(void) write(0, "\0", 1);

	if (port) {
		/*
		**	if we have a secondary socket, then we set up a pipe
		**	so we can send the stderr from the command down the
		**	secondary socket, and signal numbers from the socket
		**	to the child process.  If we do not have a secondary
		**	socket, then we have simply tied stderr to the socket,
		**	and we can exec() the child process -- when we do this
		**	the socket is used for stdin/stdout/stderr and there is
		**	no need for the remshd to hang around any longer.
		*/
		if (pipe(pv) < 0) {
			sperror("pipe");
			exit(1);
		}
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			enablepriv(SEC_LIMIT);
#endif /* SecureWare && B1 */
		pid = fork();
		if (pid == -1)  {
			sperror("fork");
			exit(1);
		}
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			disablepriv(SEC_LIMIT);
#endif /* SecureWare && B1 */
		if (pv[0] > s)
			nfd = pv[0];
		else
			nfd = s;
		nfd++;
		if (pid) {
#ifdef AUDIT
			/* reset the daemon's audit id */
			setaudid(daemonaudid);
#endif AUDIT
#ifdef SecureWare
			/* parent can drop all privileges here */
			if (ISSECURE)
				remshd_drop_privs();
#endif /* SecureWare */
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(pv[1]);
			FD_ZERO(&readfrom);
			FD_SET(s, &readfrom);
			FD_SET(pv[0], &readfrom);
			ioctl(pv[0], FIONBIO, (char *)&one);
			/* should set s nbio! */

			/*
			**	the parent now selects between the socket and
			**	the read end of the pipe, whose write end will
			**	be tied to the stderr of the child process.
			*/
			do {
				ready = readfrom;
				if (select(nfd, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0) < 0)
					break;
				if (FD_ISSET(s, &ready)) {
					if (read(s, &sig, 1) <= 0) {
						/*
						**	EOF or error on the
						**	socket means we don't
						**	want to consider it any
						**	longer for data xfer.
						*/
						FD_CLR(s, &readfrom);
					}
					else {
						/*
						**	send the signal to the
						**	child's process group
						*/
						kill(-pid, sig);
					}
				}
				if (FD_ISSET(pv[0], &ready)) {
					errno = 0;
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						/*
						**	EOF or error on the
						**	pipe means we don't
						**	want to consider it any
						**	longer for data xfer.
						*/
						shutdown(s, 1+1);
						FD_CLR(pv[0], &readfrom);
					} else
						(void) write(s, buf, cc);
				}
			} while (FD_ISSET(s, &readfrom) ||
			    FD_ISSET(pv[0], &readfrom));
			exit(0);
		}
		/*
		** Establish the child as a process group leader.
		*/
		setpgrp2(0, getpid());

		/*
		**	dup the write end of the pipe onto child's stderr
		**	and make sure that *all* of our open fd's have
		**	been properly closed down
		**	NOTE: secondary socket "s" only open if port != 0
		*/
		dup2(pv[1], 2);
		(void) close(pv[0]);
		(void) close(pv[1]);
		(void) close(s);
	}

	/*
	**	When we get here, either we have the remshd parent being a
	**	switching yard for stderr/signals (secondary socket), or we
	**	don't have a secondary socket, and thus have stdin/out/err
	**	all tied to the same socket.  in this case we can simply
	**	exec() the command and all our I/O is taken care of for us.
	*/

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	/*
	**	set up uid, gid, and group membership
	*/
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		enablepriv(SEC_IDENTITY);
#endif /* SecureWare && B1 */
	initgroups(pwd->pw_name, pwd->pw_gid);
	(void) setgid(pwd->pw_gid);
	(void) setuid(pwd->pw_uid);
#ifdef SecureWare
#ifdef B1
	if (ISB1)
		disablepriv(SEC_IDENTITY);
#endif /* B1 */
	if (ISSECURE)
		remshd_setup_privs();
#endif /* SecureWare */

	/*  Replace shell, username, homedir and path on the */
	/*  environment inherited from parent process.	     */
	/*  Keep the rest of the environment inherited.	     */

	putenv(homedir);
	putenv(shell);
	putenv("PATH=:/bin:/usr/bin:/usr/contrib/bin:/usr/local/bin");
	putenv(username);
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	strncat(username, pwd->pw_name, sizeof(username)-9);
	cp = (char *)strrchr(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	/*
	**	NOTE:	this assumes that all valid user shells take a command
	**	line argument "-c" which means "a command follows".  This is
	**	certainly true for [Bourne] sh, ksh, and csh.
	*/
	execl(pwd->pw_shell, cp, "-c", cmdbuf, 0);
	sperror(pwd->pw_shell);
	exit(1);
} /* end of doit */



/*
**	Error messages using the remshd/rexecd protocol.
*/

error(fmt)
	char *fmt;
{
	char buf[BUFSIZ];

	/*
	**	this is remshd/rexecd protocol -- error messages are
	**	prefixed by a ^A ('\001') to indicate to rcmd/rexec
	**	that it's an error message
	*/
	sprintf(buf, "\001%s: %s\n\000", servname, fmt);
	(void) write(2, buf, strlen(buf));
}



/* 
**	Do a perror(), only use the name of the server and 
**	follow the remshd/rexecd protocol.
*/
sperror(s)
        char *s;
{
	char buf[BUFSIZ];

        sprintf(buf, "\001%s: %s\000", servname, s);
        perror(buf);
}



/*
**	read a string from stdin (fd=0); similar to code in login.c
**	NOTE:	problems can occur and not be reported if read(0) fails;
**	some kind of error message should probably be printed in this case.
*/
getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (cnt-- == 0) {
			char fmt[32];
			sprintf(fmt,"%s too long",err);
			error(fmt);
			exit(1);
		}
	/*
	**	read until a NULL byte is encountered
	*/
	} while (c != 0);
}
