/*
 * Copyright (c) 1983, 1986 Regents of the University of California.
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
"@(#) Copyright (c) 1983, 1986 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)telnetd.c	5.31 (Berkeley) 2/23/89";
static char rcsid[] = "@(#)$Header: telnetd.c,v 1.21.109.8 94/11/16 11:26:45 mike Exp $";
#endif /* not lint */

#define SEND_OPER 	1
#define	RECV_OPER	2
/*
 * Telnet server.
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef SecureWare
#ifdef OLD_LOGIN
ERROR: "OLD_LOGIN and SecureWare #defines are incompatible";
#endif /* OLD_LOGIN */
#ifdef AUDIT
ERROR: "AUDIT and SecureWare #defines are incompatible";
#endif /* AUDIT */
#endif /* SecureWare */

#ifdef SecureWare
#include <sys/security.h>
#include <protcmd.h>
#include <prot.h>
#ifdef B1
#include <mandatory.h>
#endif /* B1 */
#endif /* SecureWare */

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#include <ctype.h>

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <sys/ptyio.h>
#include <utmp.h>
#include <pwd.h>
#include <grp.h>
#include <termios.h>

#if defined(AUDIT) || defined(SecureWare)
#include <audnetd.h>

uid_t	ruid = (uid_t) -1, luid = (uid_t) -1;
u_long	raddr = (u_long) -1, laddr = (u_long) -1;

#ifdef AUDIT
#define event NA_EVNT_OTHER

#define AUDIT_SUCCESS(event,s)	audit_daemon(NA_SERV_TELNET, NA_RSLT_SUCCESS, NA_VALI_PASSWD, \
		raddr, ruid, laddr, luid, \
		event, NA_MASK_RUSR|NA_MASK_LUSR|NA_MASK_VALI, (s))
#define AUDIT_FAILURE(s)	audit_daemon(NA_SERV_TELNET, NA_RSLT_FAILURE, NA_VALI_PASSWD, \
		raddr, ruid, laddr, luid, \
		0, NA_MASK_RUSR|NA_MASK_EVNT|NA_MASK_LUSRL|NA_MASK_VALI, (s))
#endif /* AUDIT */

#ifdef SecureWare

#define event NA_EVNT_CONN

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
		 audit_daemon(NA_RSLT_SUCCESS, NA_VALI_PASSWD, \
			raddr, rhost, ruid, rusr, luid, lusr, event, \
			NA_MASK_VALI|NA_MASK_RUID|NA_MASK_RUSRNAME|\
			NA_MASK_LUID|NA_MASK_LUSRNAME, (s)); \
		 disable_priv_for_audit(); \
	}
#define AUDIT_FAILURE(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_FAILURE, NA_VALI_PASSWD, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_CONN, \
			NA_MASK_VALI|\
			(raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
			(rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
			NA_MASK_RUID|NA_MASK_LUID|NA_MASK_LUSRNAME, (s)); \
		 disable_priv_for_audit(); \
	}
#else /* not notdef*/
#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
#endif /* not notdef */
#endif /* SecureWare */

#else /* neither AUDIT nor SecureWare */
#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
#endif /* neither AUDIT nor SecureWare */

#ifdef AUDIT
int	daemonaudid;
extern	int setaudid(), getaudid();
#endif /* AUDIT */

/* redefine BUFSIZ for use with NVS ioctl */
#undef BUFSIZ
#define BUFSIZ 512

#define	OPT_NO			0		/* won't do this option */
#define	OPT_YES			1		/* will do this option */
#define	OPT_YES_BUT_ALWAYS_LOOK	2
#define	OPT_NO_BUT_ALWAYS_LOOK	3
char	hisopts[256];
char	myopts[256];

char	doopt[] = { IAC, DO, '%', 'c', 0 };
char	dont[] = { IAC, DONT, '%', 'c', 0 };
char	will[] = { IAC, WILL, '%', 'c', 0 };
char	wont[] = { IAC, WONT, '%', 'c', 0 };

/*
 * I/O data buffers, pointers, and counters.
 */
char	ptyibuf[BUFSIZ], *ptyip = ptyibuf;

char	ptyobuf[BUFSIZ], *pfrontp = ptyobuf, *pbackp = ptyobuf;

char	netibuf[BUFSIZ], *netip = netibuf;

char	stobuf[BUFSIZ];		/* Buffer used for port ID */

#define	NIACCUM(c)	{   *netip++ = c; \
			    ncc++; \
			}

char	netobuf[BUFSIZ], *nfrontp = netobuf, *nbackp = netobuf;
char	*neturg = 0;		/* one past last bye of urgent data */
	/* the remote system seems to NOT be an old 4.2 */
int	not42 = 1;

		/* buffer for sub-options */
char	subbuffer[100], *subpointer= subbuffer, *subend= subbuffer;
#define	SB_CLEAR()	subpointer = subbuffer;
#define	SB_TERM()	{ subend = subpointer; SB_CLEAR(); }
#define	SB_ACCUM(c)	if (subpointer < (subbuffer+sizeof subbuffer)) { \
				*subpointer++ = (c); \
			}
#define	SB_GET()	((*subpointer++)&0xff)
#define	SB_EOF()	(subpointer >= subend)

int	pcc, ncc,scc;

int	pty, net,fd;
int	inter;
extern	char **environ;
extern	int errno;
int	SYNCHing = 0;		/* we are in TELNET SYNCH mode */
int	telnet_pid;		/* pid of telnet child */
#ifdef SIOCJNVS
int	kernel = 1;		/* use NVS ioctl */
#else
int	kernel = 0;
#endif
int 	userbanner = 0;		/* use user banner instead of the banner */
int	eightbit = 0;		/* start sessions in 8-bit mode */
char 	bfname[MAXPATHLEN+1];	/* file for user specified banner */

/*
 * The following are some clocks used to decide how to interpret
 * the relationship between various variables.
 */

struct {
    int
	system,			/* what the current time is */
	echotoggle,		/* last time user entered echo character */
	modenegotiated,		/* last time operating mode negotiated */
	didnetreceive,		/* last time we read data from network */
	ttypeopt,		/* ttype will/won't received */
	ttypesubopt,		/* ttype subopt is received */
	getterminal,		/* time started to get terminal information */
	gotDM;			/* when did we last see a data mark */
} clocks;

#define	settimer(x)	(clocks.x = ++clocks.system)
#define	sequenceIs(x,y)	(clocks.x < clocks.y)

main(argc, argv)
	char *argv[];
{
	struct sockaddr_in from;
	int on = 1, fromlen, c;
	char *cp;
    int tcp_nodelay = 1;
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

	/*
	 * Process any options.
	 */
	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		cp = &argv[0][1];
		switch (*cp) {

		case 'u':
#if defined(SIOCJNVS) && defined(USERSPACE)
			kernel = 0;
#endif /* defined(SIOCJNVS) && defined(USERSPACE) */
			break;

		case '8':
			eightbit++;
			break;

		case 'b':
			userbanner = 1;
			memset(bfname, 0, sizeof(bfname));
			if (*++cp)
			    strcpy(bfname, cp);
			else if (argc > 1 && argv[1][0] != '-') {
			    strcpy(bfname, argv[1]);
			    argc--, argv++;
			}
			break;

         case 'T':  
			if (!strcmp(cp,"TCP_DELAY")) {  /* if strcmp fails then */
				tcp_nodelay = 0;              /* case if fall into default */
				break;						  /* action */
			}

		default:
			sprintf(nfrontp,
				"telnetd: Unknown option '%s' ignored\r\n",
				cp);
			nfrontp += strlen(nfrontp);
			break;
		}
		argc--, argv++;
	}

	/*
	 * syslog at last!
	 */
	openlog("telnetd", LOG_PID | LOG_ODELAY, LOG_DAEMON);

	/*
	 *  get the identity of our peer.
	 */
	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		if (errno == ENOTSOCK)
			fprintf(stderr, 
				"telnetd: Must be executed by inetd(1M).\n");
		else
			fatalperror(0, "telnetd: getpeername", errno);
		exit(1);
	}

	/*
	 * set up KEEPALIVE on our socket
	 */
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0) {
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	}

#ifdef TCP_NODELAY
	/* 
	 * set up TCP_NODELAY for this socket, if available so that
	 * small outbound packets will not wait for delayed ACKs
	 * before being sent
	 */
	if (tcp_nodelay) {
		int optval;
		struct protoent *pp;

		optval = 1;
		pp = getprotobyname("tcp");
		if (!pp) {
			syslog(LOG_WARNING, "getprotobyname: %m");
		}
		else if (setsockopt(0, pp->p_proto, TCP_NODELAY, &optval,
				    sizeof(optval)) < 0) {
			syslog(LOG_WARNING, "setsockopt (TCP_NODELAY): %m");
		}
	}
#endif TCP_NODELAY


#if defined(AUDIT) || defined(SecureWare)
	/*
	 *	Establish remote host address for auditing.
	 */
	raddr = from.sin_addr.s_addr;
#endif /* AUDIT or SecureWare */
#ifdef AUDIT
	/* 
	 * Establish local host addresses for HP-C2 auditing.
	 */
	myaddrlen = sizeof(myaddr);
	if (getsockname(0, &myaddr, &myaddrlen) < 0) {
		fatalperror(0,"telnetd: getsockname",errno);
		exit(1);
	}
	laddr = myaddr.sin_addr.s_addr;
#endif AUDIT
#ifdef	DEBUG
	fd = open("/tmp/tellog", O_RDWR | O_CREAT | O_TRUNC );
#endif	DEBUG
	doit(0, &from);
}

char	*terminaltype = 0;
char	*envinit[2];

/*
 * ttloop
 *
 *	A small subroutine to flush the network output buffer, get some data
 * from the network, and pass it through the telnet state machine.  We
 * also flush the pty input buffer (by dropping its data) if it becomes
 * too full.
 */

void
ttloop()
{
    if (nfrontp-nbackp) {
	netflush();
    }

	/* Changed for Port ID by Rajesh Srivastava */
    	if( scc )
	{
		bcopy( stobuf, netibuf, scc );
		ncc = scc;
		scc = 0;
	}
	else	ncc += read(net, netibuf, sizeof netibuf);
	/* end of changes */

    if (ncc < 0) {
	syslog(LOG_INFO, "ttloop:  read: %m\n");
	exit(1);
    } else if (ncc == 0) {
	syslog(LOG_INFO, "ttloop:  peer died: %m\n");
	exit(1);
    }
    netip = netibuf;
    telrcv();			/* state machine */
    if (ncc > 0) {
	pfrontp = pbackp = ptyobuf;
	telrcv();
    }
}

/*
 * getterminaltype
 *
 *	Ask the other end to send along its terminal type.
 * Output is the variable terminaltype filled in.
 */

void
getterminaltype()
{
    static char sbuf[] = { IAC, DO, TELOPT_TTYPE };

    settimer(getterminal);
    bcopy(sbuf, nfrontp, sizeof sbuf);
    nfrontp += sizeof sbuf;
    hisopts[TELOPT_TTYPE] = OPT_YES_BUT_ALWAYS_LOOK;
    while (sequenceIs(ttypeopt, getterminal)) {
	ttloop();
    }
    if (hisopts[TELOPT_TTYPE] == OPT_YES) {
	static char sbbuf[] = { IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE };

	bcopy(sbbuf, nfrontp, sizeof sbbuf);
	nfrontp += sizeof sbbuf;
	while (sequenceIs(ttypesubopt, getterminal)) {
	    ttloop();
	}
    }
}

char master[MAXPATHLEN], slave[MAXPATHLEN], tname[MAXPATHLEN];
char O_device_name[MAXPATHLEN];		/* device name for Port ID */

/*
 * Get a pty, scan input lines.
 */
doit(f, who)
	int f;
	struct sockaddr_in *who;
{
	char *host, *tn, *inet_ntoa(), *ttyname();
	int i, p, t, t_;
	struct termio b;
	struct hostent *hp;
	int vhangup_err;
	int c;
#if defined(SecureWare) || defined(AUDIT)
	char buf[BUFSIZ];
#endif /* SecureWare or AUDIT */

	/*
	 * get a pty pair for our child (login/shell) to talk to.
	 */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		enablepriv(SEC_OWNER);
		enablepriv(SEC_ALLOWDACACCESS);
	}
#endif /* SecureWare && B1 */

	/* changed by Rajesh Srivastava for Port ID via Telnet */

	if (get_portid_param(&p, master, slave) != 0) {

	/* End of changes */

	    syslog(LOG_ERR, "Cannot allocate pty");
	    fatal(f, "Unable to allocate pty on remote host");
        }
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		/* keep SEC_ALLOWDACACCESS for the open call */
		disablepriv(SEC_OWNER);
	}
#endif /* SecureWare && B1 */

	/*
	** Slave becomes controlling terminal for this session.
	*/
	
	/* changed for Port ID */

	if ((t_=open(O_device_name, O_RDWR)) < 0)
		fatalperror(f, slave);

	vhangup_err = vhangup(); 

	if ((vhangup_err == EPERM) || (vhangup_err == ENOTTY))
		fatalperror(f, slave);

	if ((t=open(O_device_name, O_RDWR)) < 0)
		fatalperror(f, slave);

	close(t_);

	/* end of changes */

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
	 * Initialize the pty modes.
	 */
	(void) ioctl(t, TCGETA, &b);
	b.c_oflag = OPOST|ONLCR|TAB3;
	b.c_iflag = BRKINT|IGNPAR|ISTRIP|ICRNL|IXON;
	b.c_lflag = ISIG|ICANON;
	b.c_cflag = HUPCL|B9600;
	/*
	** Let the administrator violate the telnet RFC and
	** initialize 8-bit settings.
	*/
	if (eightbit) {
		b.c_iflag &= ~ISTRIP;
		b.c_cflag &= ~PARENB;
		b.c_cflag |= CS8;

	} else {
		b.c_iflag |= ISTRIP;
		b.c_cflag |= CS7 | PARENB;
	}
	(void) ioctl(t, TCSETAW, &b);

	/*
	**  Get the name of our peer.
	*/
	hp = gethostbyaddr(&who->sin_addr, sizeof (struct in_addr),
		who->sin_family);
	if (hp) {
		host = hp->h_name;
#ifdef SecureWare
		rhost = host;
#endif /* SecureWare */
	}
	else
		host = inet_ntoa(who->sin_addr);

	net = f;
	pty = p;
	
	/*
	 *  Initialize the pty window size
	 */
#ifdef TIOCSWINSZ
    {
	   struct winsize win;
	  
	   win.ws_row = win.ws_col = win.ws_xpixel = win.ws_ypixel = 0;
	   ioctl(p,TIOCSWINSZ,&win);
	}
#endif /* TIOCSWINSZ */

	/*
	 * get terminal type.
	 */
	getterminaltype();

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

	if ((telnet_pid = fork()) < 0)
		fatalperror(f, "fork", errno);
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		disablepriv(SEC_LIMIT);
#endif /* SecureWare && B1 */
	if (telnet_pid) {
		/*
		** Set the child up as a process group leader.
		** It must not be part of the parent's process group,
		** because signals issued in the child's process group
		** must not affect the parent.
		*/
		setpgid(telnet_pid, telnet_pid);

		/*
		** Make the pty slave the controlling terminal for the
		** child's process group.
		*/
		tcsetpgrp(t, telnet_pid);

		/*
		** Close our controlling terminal.
		*/
		close(t);

		/*
		** Let the child know that it is a process group
		** leader and has a controlling tty.
		*/
		lockf(p, F_ULOCK, 0);

		telnet(f, p);
	}

	/*
	** Wait for the parent to setup our process group and
	** controlling tty.
	*/
	lockf(p, F_LOCK, 0);
	lockf(p, F_ULOCK, 0);

	close(f);
	close(p);
	dup2(t, 0);
	dup2(t, 1);
	dup2(t, 2);
	close(t);
	envinit[0] = terminaltype;
	envinit[1] = 0;
	environ = envinit;
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		enablepriv(SEC_ALLOWDACACCESS);
		enablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
	account(LOGIN_PROCESS,getpid(),"LOGIN",slave,host,who);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_ALLOWDACACCESS);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
	/*
	 * -h : pass on name of host.
	 *	WARNING:  -h is accepted by login only if getuid() == 0
#ifdef SecureWare
	 *      or if getluid() < 0 and errno == EPERM.
#endif
	 * -p : don't clobber the environment (so terminal type stays set).
	 */
#ifdef OLD_LOGIN
#ifdef AUDIT
	sprintf(buf, "exec \"login\" (%s)", tname);
	AUDIT_SUCCESS(event, buf);
#endif /* AUDIT */

	execl("/bin/login", "login", 0);
#else /* not OLD_LOGIN */
#if defined(AUDIT) || defined(SecureWare)
	sprintf(buf, "exec \"login -h %s%s\" (%s)",
					host, terminaltype ? " -p" : "", tname);
	AUDIT_SUCCESS(event, buf);
#endif /* AUDIT or SecureWare */

#ifdef SecureWare
	if (ISSECURE) {
#ifdef B1
		if (ISB1) {
			enablepriv(SEC_ALLOWDACACCESS);
		}
#endif /* B1 */
		execl("/tcb/lib/login", "login", "-h", host,
					terminaltype ? "-p" : 0, 0);
		openlog("telnetd", LOG_PID | LOG_ODELAY, LOG_DAEMON);
		syslog(LOG_ERR, "/tcb/lib/login: %m");
		fatalperror(2, "/tcb/lib/login");
	} else
#endif /* SecureWare */
	{
		execl("/bin/login", "login", "-h", host,
					terminaltype ? "-p" : 0, 0);
		openlog("telnetd", LOG_PID | LOG_ODELAY, LOG_DAEMON);
		syslog(LOG_ERR, "/bin/login: %m");
		fatalperror(2, "/bin/login");
	}
#endif /* not OLD_LOGIN */
	/*NOTREACHED*/
}

fatal(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, "telnetd: %s.\r\n", msg);
	(void) write(f, buf, strlen(buf));
	exit(1);
}

fatalperror(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];
	extern char *sys_errlist[];

	(void) sprintf(buf, "%s: %s\r\n", msg, sys_errlist[errno]);
	fatal(f, buf);
}


/*
 * Check a descriptor to see if out of band data exists on it.
 */


stilloob(s)
int	s;		/* socket number */
{
    static struct timeval timeout = { 0 };
    fd_set	excepts;
    int value;

    do {
	FD_ZERO(&excepts);
	FD_SET(s, &excepts);
	value = select(s+1, (fd_set *)0, (fd_set *)0, &excepts, &timeout);
    } while ((value == -1) && (errno == EINTR));

    if (value < 0) {
	syslog(LOG_ERR, "select: %m");
	fatalperror(pty, "select");
    }
    if (FD_ISSET(s, &excepts)) {
	return 1;
    } else {
	return 0;
    }
}

/*
 * Main loop.  Select from pty and network, and
 * hand data to telnet receiver finite state machine.
 */
telnet(f, p)
{
	int on = 1;
	FILE *bfile;
	void child_died(), cleanup();
#ifdef AUDIT
	char buf[255];

	sprintf(buf, "Executing login pid = %d", telnet_pid);
	AUDIT_SUCCESS(event, buf);
#endif AUDIT

	ioctl(f, FIONBIO, &on);
	ioctl(p, FIONBIO, &on);
#ifdef SIOCJNVS
	if (!kernel)
#endif /* SIOCJNVS */
		ioctl(p, TIOCPKT, &on);
#if	defined(SO_OOBINLINE)
	setsockopt(net, SOL_SOCKET, SO_OOBINLINE, &on, sizeof on);
#endif	/* defined(SO_OOBINLINE) */
	signal(SIGTSTP, SIG_IGN);
	/*
	 * Ignoring SIGTTOU keeps the kernel from blocking us
	 * in ttioctl() in /sys/tty.c.
	 */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCLD, child_died,
	       sigmask(SIGCLD) | sigmask(SIGTERM) | sigmask(SIGPIPE));
	signal(SIGTERM, cleanup,
	       sigmask(SIGCLD) | sigmask(SIGTERM) | sigmask(SIGPIPE));
	signal(SIGPIPE, cleanup,
	       sigmask(SIGCLD) | sigmask(SIGTERM) | sigmask(SIGPIPE));

	/*
	 * Request to do remote echo and to suppress go ahead.
	 * Request to do window size negotiation.
	 */
	if (!myopts[TELOPT_ECHO]) {
	    dooption(TELOPT_ECHO);
	}
	if (!myopts[TELOPT_SGA]) {
	    dooption(TELOPT_SGA);
	}
	if (!hisopts[TELOPT_NAWS]) {
	    willoption(TELOPT_NAWS);
	}
	/*
	 * Is the client side a 4.2 (NOT 4.3) system?  We need to know this
	 * because 4.2 clients are unable to deal with TCP urgent data.
	 *
	 * To find out, we send out a "DO ECHO".  If the remote system
	 * answers "WILL ECHO" it is probably a 4.2 client, and we note
	 * that fact ("WILL ECHO" ==> that the client will echo what
	 * WE, the server, sends it; it does NOT mean that the client will
	 * echo the terminal input).
	 */
	(void) sprintf(nfrontp, doopt, TELOPT_ECHO);
	nfrontp += sizeof doopt-2;
	hisopts[TELOPT_ECHO] = OPT_YES_BUT_ALWAYS_LOOK;

	/*
	 * Show banner that getty never gave.
	 */
	if (userbanner) {
	    if (bfname[0] && (bfile = fopen(bfname, "r")) != NULL) {
		register int c;
		while ((c = fgetc(bfile)) != EOF) {
		    if (c == IAC)
			*nfrontp++ = IAC;
		    if ((c == '\n') || (c == '\r')) {
			*nfrontp++ = '\r';
			c = '\n';
		    }
		    *nfrontp++ = c;
		    if ((&netobuf[BUFSIZ] - nfrontp) <= 2)
			netflush();
		}
		close(bfile);
		*nfrontp++ = '\r';
		*nfrontp++ = '\n';
		netflush();
	    }
	} else {
	    char *getbanner(), *bannerp = getbanner();
	    strcpy(nfrontp, bannerp);
	    nfrontp += strlen(bannerp);
	}


	ptyip = ptyibuf+1;		/* Prime the pump */
	pcc = strlen(ptyip);		/* ditto */

	/* Clear ptybuf[0] - where the packet information is received */
	ptyibuf[0] = 0;

	/*
	 * Call telrcv() once to pick up anything received during
	 * terminal type negotiation.
	 */
	telrcv();

#ifdef SIOCJNVS
	if (kernel) {
		nvs_telnet(f, p);
	}
#endif /* SIOCJNVS */

#ifdef USERSPACE
	for (;;) {
		fd_set ibits, obits, xbits;
		register int c;

		if (ncc < 0 && pcc < 0)
			break;

		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		FD_ZERO(&xbits);
		/*
		 * Never look for input if there's still
		 * stuff in the corresponding output buffer
		 */
		if (nfrontp - nbackp || pcc > 0) {
			FD_SET(f, &obits);
			FD_SET(p, &xbits);
		} else {
			FD_SET(p, &ibits);
		}
		if (pfrontp - pbackp || ncc > 0) {
			FD_SET(p, &obits);
		} else {
			FD_SET(f, &ibits);
		}
		if (!SYNCHing) {
			FD_SET(f, &xbits);
		}
		if ((c = select(16, &ibits, &obits, &xbits,
						(struct timeval *)0)) < 1) {
			if (c == -1) {
				if (errno == EINTR) {
					continue;
				}
			}
			sleep(5);
			continue;
		}

		/*
		 * Any urgent data?
		 */
		if (FD_ISSET(net, &xbits)) {
		    SYNCHing = 1;
		}

		/*
		 * Something to read from the network...
		 */
		if (FD_ISSET(net, &ibits)) {
#if	!defined(SO_OOBINLINE)
			/*
			 * In 4.2 (and 4.3 beta) systems, the
			 * OOB indication and data handling in the kernel
			 * is such that if two separate TCP Urgent requests
			 * come in, one byte of TCP data will be overlaid.
			 * This is fatal for Telnet, but we try to live
			 * with it.
			 *
			 * In addition, in 4.2 (and...), a special protocol
			 * is needed to pick up the TCP Urgent data in
			 * the correct sequence.
			 *
			 * What we do is:  if we think we are in urgent
			 * mode, we look to see if we are "at the mark".
			 * If we are, we do an OOB receive.  If we run
			 * this twice, we will do the OOB receive twice,
			 * but the second will fail, since the second
			 * time we were "at the mark", but there wasn't
			 * any data there (the kernel doesn't reset
			 * "at the mark" until we do a normal read).
			 * Once we've read the OOB data, we go ahead
			 * and do normal reads.
			 *
			 * There is also another problem, which is that
			 * since the OOB byte we read doesn't put us
			 * out of OOB state, and since that byte is most
			 * likely the TELNET DM (data mark), we would
			 * stay in the TELNET SYNCH (SYNCHing) state.
			 * So, clocks to the rescue.  If we've "just"
			 * received a DM, then we test for the
			 * presence of OOB data when the receive OOB
			 * fails (and AFTER we did the normal mode read
			 * to clear "at the mark").
			 */
		    if (SYNCHing) {
			int atmark;

			ioctl(net, SIOCATMARK, (char *)&atmark);
			if (atmark) {
			    ncc = recv(net, netibuf, sizeof (netibuf), MSG_OOB);
#ifndef hpux
			    if ((ncc == -1) && (errno == EINVAL)) {
				ncc = read(net, netibuf, sizeof (netibuf));
				if (sequenceIs(didnetreceive, gotDM)) {
				    SYNCHing = stilloob(net);
				}
			    }
#endif /* ~hpux */
			} else {
			    ncc = read(net, netibuf, sizeof (netibuf));
			}
		    } else {
			ncc = read(net, netibuf, sizeof (netibuf));
		    }
		    settimer(didnetreceive);
#else	/* !defined(SO_OOBINLINE)) */
		    ncc = read(net, netibuf, sizeof (netibuf));
#endif	/* !defined(SO_OOBINLINE)) */
		    if (ncc < 0 && errno == EWOULDBLOCK)
			ncc = 0;
		    else {
			if (ncc <= 0)
			    break;
			netip = netibuf;
		    }
		}

		/*
		 * Something to read from the pty...
		 */
		if (FD_ISSET(p, &xbits)) {
			if (read(p, ptyibuf, 1) != 1)
				break;
		}
		if (FD_ISSET(p, &ibits)) {
			pcc = read(p, ptyibuf, BUFSIZ);
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else {
				if (pcc <= 0)
					break;
				/* Skip past "packet" */
				pcc--;
				ptyip = ptyibuf+1;
			}
		}
		if (ptyibuf[0] & TIOCPKT_FLUSHWRITE) {
			netclear();	/* clear buffer back */
			*nfrontp++ = IAC;
			*nfrontp++ = DM;
			neturg = nfrontp-1;  /* off by one XXX */
			ptyibuf[0] = 0;
		}

		while (pcc > 0) {
			if ((&netobuf[BUFSIZ] - nfrontp) < 2)
				break;
			c = *ptyip++ & 0377, pcc--;
			if (c == IAC)
				*nfrontp++ = c;
			*nfrontp++ = c;
			/* Don't do CR-NUL if we are in binary mode */
			if ((c == '\r') && (myopts[TELOPT_BINARY] == OPT_NO)) {
				if (pcc > 0 && ((*ptyip & 0377) == '\n')) {
					*nfrontp++ = *ptyip++ & 0377;
					pcc--;
				} else
					*nfrontp++ = '\0';
			}
		}
		if (FD_ISSET(f, &obits) && (nfrontp - nbackp) > 0)
			netflush();
		if (ncc > 0)
			telrcv();
		if (FD_ISSET(p, &obits) && (pfrontp - pbackp) > 0)
			ptyflush();
	}
	cleanup();
#endif /* USERSPACE */
}
	
/*
 * State for recv fsm
 */
#define	TS_DATA		0	/* base state */
#define	TS_IAC		1	/* look for double IAC's */
#define	TS_CR		2	/* CR-LF ->'s CR */
#define	TS_SB		3	/* throw away begin's... */
#define	TS_SE		4	/* ...end's (suboption negotiation) */
#define	TS_WILL		5	/* will option negotiation */
#define	TS_WONT		6	/* wont " */
#define	TS_DO		7	/* do " */
#define	TS_DONT		8	/* dont " */

static int state = TS_DATA;

telrcv()
{
	register int c;

	while (ncc > 0) {
		if ((&ptyobuf[BUFSIZ] - pfrontp) < 2)
			return;
		c = *netip++ & 0377, ncc--;
		switch (state) {

		case TS_CR:
			state = TS_DATA;
			/* Strip off \n or \0 after a \r */
			if ((c == 0) || (c == '\n')) {
				break;
			}
			/* FALL THROUGH */

		case TS_DATA:
			if (c == IAC) {
				state = TS_IAC;
				break;
			}
			if (inter > 0)
				break;
			/*
			 * We now map \r\n ==> \r for pragmatic reasons.
			 * Many client implementations send \r\n when
			 * the user hits the CarriageReturn key.
			 *
			 * We USED to map \r\n ==> \n, since \r\n says
			 * that we want to be in column 1 of the next
			 * printable line, and \n is the standard
			 * unix way of saying that (\r is only good
			 * if CRMOD is set, which it normally is).
			 */
			if ((c == '\r') && (hisopts[TELOPT_BINARY] == OPT_NO)) {
				state = TS_CR;
			}
			*pfrontp++ = c;
			break;

		case TS_IAC:
			switch (c) {

			/*
			 * Send the process on the pty side an
			 * interrupt.  Do this with a NULL or
			 * interrupt char; depending on the tty mode.
			 */
			case IP:
				interrupt();
				break;

			case BREAK:
				sendbrk();
				break;

			/*
			 * Are You There?
			 */
			case AYT:
				strcpy(nfrontp, "\r\n[Yes]\r\n");
				nfrontp += 9;
				break;

			/*
			 * Abort Output
			 */
			case AO: {
#ifndef hpux
					struct ltchars tmpltc;
#endif
					ptyflush();	/* half-hearted */
#ifndef hpux
					ioctl(pty, TIOCGLTC, &tmpltc);
					if (tmpltc.t_flushc != '\377') {
						*pfrontp++ = tmpltc.t_flushc;
					}
#endif
					netclear();	/* clear buffer back */
					*nfrontp++ = IAC;
					*nfrontp++ = DM;
					neturg = nfrontp-1; /* off by one XXX */
					break;
				}

			/*
			 * Erase Character and
			 * Erase Line
			 */
			case EC:
			case EL: {
					struct termio b;
					int ch;

					ptyflush();	/* half-hearted */

					ioctl(pty, TCGETA, &b);
					ch = (c == EC) ?
						b.c_cc[VERASE] : b.c_cc[VKILL];
					if (ch != -1) {
						*pfrontp++ = ch;
					}
					break;
				}

			/*
			 * Check for urgent data...
			 */
			case DM:
				SYNCHing = stilloob(net);
				settimer(gotDM);
				break;


			/*
			 * Begin option subnegotiation...
			 */
			case SB:
				state = TS_SB;
				SB_CLEAR();
				continue;

			case WILL:
				state = TS_WILL;
				continue;

			case WONT:
				state = TS_WONT;
				continue;

			case DO:
				state = TS_DO;
				continue;

			case DONT:
				state = TS_DONT;
				continue;

			case IAC:
				*pfrontp++ = c;
				break;
			}
			state = TS_DATA;
			break;

		case TS_SB:
			if (c == IAC) {
				state = TS_SE;
			} else {
				SB_ACCUM(c);
			}
			break;

		case TS_SE:
			if (c != SE) {
				if (c != IAC) {
					SB_ACCUM(IAC);
				}
				SB_ACCUM(c);
				state = TS_SB;
			} else {
				SB_TERM();
				suboption();	/* handle sub-option */
				state = TS_DATA;
			}
			break;

		case TS_WILL:
			if (hisopts[c] != OPT_YES)
				willoption(c);
			state = TS_DATA;
			continue;

		case TS_WONT:
			if (hisopts[c] != OPT_NO)
				wontoption(c);
			state = TS_DATA;
			continue;

		case TS_DO:
			if (myopts[c] != OPT_YES)
				dooption(c);
			state = TS_DATA;
			continue;

		case TS_DONT:
			if (myopts[c] != OPT_NO) {
				dontoption(c);
			}
			state = TS_DATA;
			continue;

		default:
			syslog(LOG_ERR, "telnetd: panic state=%d\n", state);
			printf("telnetd: panic state=%d\n", state);
			exit(1);
		}
	}
}

#ifdef SIOCJNVS
/*
**  This routine performs the NVS ioctl for kernel telnetd.
*/

nvs_telnet(f, p)
{
	void cleanup();
	int rc, arg;
	struct timeval timeout;

	for (;;) {
		fd_set ibits, obits, xbits;
		int dotimeout;

		while ((arg = nfrontp - nbackp) > 0) {
			rc = send(f, nbackp, arg, 0);
			if (rc < 0) {
				syslog(LOG_ERR, "send: %m");
				cleanup();
			}
			nbackp += rc;
			if (nbackp == nfrontp)
				nbackp = nfrontp = netobuf;
		}

		dotimeout = 0;
		while (pfrontp - pbackp > 0) {
			if (dotimeout) {
				/*
				 * do the select with a timeout
				 * we don't want to include
				 * pty read select here, because
				 * we don't want to hog the CPU.
				 */
				FD_ZERO(&ibits);
				FD_ZERO(&obits);
				FD_ZERO(&xbits);
				FD_SET(f, &xbits);
				FD_SET(p, &obits);
				rc = select(FD_SETSIZE, &ibits, &obits, 
					    &xbits, &timeout);
				if (rc)
					/*
					 * if we didn't timeout,
					 * skip the next select
					 */
					goto selectdone;
			}

			/* do the select */
			FD_ZERO(&ibits);
			FD_ZERO(&obits);
			FD_ZERO(&xbits);
			FD_SET(f, &xbits);
			FD_SET(p, &ibits);
			FD_SET(p, &obits);
			rc = select(FD_SETSIZE, &ibits, &obits, &xbits, 0);
selectdone:		
			if (rc < 0) {
				syslog(LOG_ERR, "select: %m");
				cleanup();
			}

			if (FD_ISSET(f, &xbits)) {
				SYNCHing = 1;
			}
			if (FD_ISSET(p, &obits)) {
				/*
				 * The pty is writable!  Let's get
				 * as much of our data written as
				 * possible.
				 */
				arg = pfrontp - pbackp;
				rc = write(p, pbackp, arg);
				if (rc < 0) {
					syslog(LOG_ERR, "write: %m");
					cleanup();
				}
				pbackp += rc;
				if (pbackp == pfrontp)
					pbackp = pfrontp = ptyobuf;
				dotimeout = 0;
				continue;
			}

			/*
			 * We get here if the pty is unwritable
			 * but it is readable.  In order to avoid
			 * possible deadlock, we need to call the
			 * kernel to try to flush some of the data
			 * from the pty to the socket.  Hopefully,
			 * after we do this enough times, we will
			 * have flushed enough outbound data that the
			 * application can read the pty and free up
			 * space to do some more writing.
			 */
			arg = 8 << 24;
			if (hisopts[TELOPT_BINARY])
				arg |= 1 << 24;
			if (myopts[TELOPT_BINARY])
				arg |= 8 << 16;
			arg |= 2 << 16;	/* temporary join */
			arg |= p;
#if defined(SecureWare) && defined(B1)
			/*
			**  remove this privilege bracketing if
			**  SIOCJNVS doesn't require privilege.
			*/
			if (ISB1)
				enablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
			if (ioctl(f,SIOCJNVS,(char *)&arg) == -1) {
				syslog(LOG_ERR, "ioctl(SIOCJNVS): %m");
#if defined(SecureWare) && defined(B1)
				if (ISB1)
					disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
				cleanup();
			}
#if defined(SecureWare) && defined(B1)
			if (ISB1)
				disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */

			dotimeout = 1;
		}

		/*
		 * If the state is now TS_DATA, we have finished
		 * handling the option negotiation and should
		 * now go into the kernel for normal processing.
		 */
		if (state == TS_DATA) {
			arg = 8 << 24;
			if (hisopts[TELOPT_BINARY])
				arg |= 1 << 24;
			if (myopts[TELOPT_BINARY])
				arg |= 8 << 16;
			arg |= 1 << 16;
			arg |= p;
#if defined(SecureWare) && defined(B1)
			/*
			**  remove this privilege bracketing if
			**  SIOCJNVS doesn't require privilege.
			*/
			if (ISB1)
				enablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
			if (ioctl(f,SIOCJNVS,(char *)&arg) == -1) {
				syslog(LOG_ERR, "ioctl(SIOCJNVS): %m");
#if defined(SecureWare) && defined(B1)
				if (ISB1)
					disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
				cleanup();
			}
#if defined(SecureWare) && defined(B1)
			if (ISB1)
				disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
			SYNCHing = stilloob(net);
		}

		ncc = recv(f, netibuf, 1, 0);
		if (ncc <= 0) {
		    if (ncc && errno == EWOULDBLOCK)
			continue;
		    if (ncc)
			syslog(LOG_ERR, "recv: %m");
		    break;
		}
		netip = netibuf;
		telrcv();

	}
	cleanup();
}
#endif /* SIOCJNVS */

willoption(option)
	int option;
{
	char *fmt;

	switch (option) {

	case TELOPT_BINARY:
		mode(0, 0, ISTRIP);
		fmt = doopt;
		break;

	case TELOPT_ECHO:
		not42 = 0;		/* looks like a 4.2 system */
		/*
		 * Now, in a 4.2 system, to break them out of ECHOing
		 * (to the terminal) mode, we need to send a "WILL ECHO".
		 * Kludge upon kludge!
		 */
		if (myopts[TELOPT_ECHO] == OPT_YES) {
		    dooption(TELOPT_ECHO);
		}
		fmt = dont;
		break;

	case TELOPT_TTYPE:
		settimer(ttypeopt);
		if (hisopts[TELOPT_TTYPE] == OPT_YES_BUT_ALWAYS_LOOK) {
		    hisopts[TELOPT_TTYPE] = OPT_YES;
		    return;
		}
		fmt = doopt;
		break;

	case TELOPT_SGA:
	case TELOPT_NAWS:
		fmt = doopt;
		break;

	case TELOPT_TM:
		fmt = dont;
		break;

	default:
		fmt = dont;
		break;
	}
	if (fmt == doopt) {
		hisopts[option] = OPT_YES;
	} else {
		hisopts[option] = OPT_NO;
	}
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (dont) - 2;
}

wontoption(option)
	int option;
{
	char *fmt;

	switch (option) {
	case TELOPT_ECHO:
		not42 = 1;		/* doesn't seem to be a 4.2 system */
		break;

	case TELOPT_BINARY:
		mode(0, ISTRIP, 0);
		break;

	case TELOPT_TTYPE:
	    settimer(ttypeopt);
	    break;
	}

	fmt = dont;
	hisopts[option] = OPT_NO;
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (doopt) - 2;
}

dooption(option)
	int option;
{
	char *fmt;

	switch (option) {

	case TELOPT_TM:
		fmt = wont;
		break;

	case TELOPT_ECHO:
		mode(3, ECHO, 0);
		fmt = will;
		break;

	case TELOPT_BINARY:
		mode(1, 0, OPOST);		/* no output processing */
		mode(2, CS8, CSIZE|PARENB);	/* 8bit, disable parity */
		fmt = will;
		break;

	case TELOPT_SGA:
		fmt = will;
		break;

	default:
		fmt = wont;
		break;
	}
	if (fmt == will) {
	    myopts[option] = OPT_YES;
	} else {
	    myopts[option] = OPT_NO;
	}
	(void) sprintf(nfrontp, fmt, option);
	nfrontp += sizeof (doopt) - 2;
}


dontoption(option)
int option;
{
    char *fmt;

    switch (option) {
    case TELOPT_ECHO:		/* we should stop echoing */
	mode(3, 0, ECHO);
	fmt = wont;
	break;

    case TELOPT_BINARY:
	mode(1, OPOST, 0);
	mode(2, CS7|PARENB, CSIZE);

    default:
	fmt = wont;
	break;
    }

    if (fmt = wont) {
	myopts[option] = OPT_NO;
    } else {
	myopts[option] = OPT_YES;
    }
    (void) sprintf(nfrontp, fmt, option);
    nfrontp += sizeof (wont) - 2;
}

/*
 * suboption()
 *
 *	Look at the sub-option buffer, and try to be helpful to the other
 * side.
 *
 *	Currently we recognize:
 *
 *	Terminal type is
 */

suboption()
{
    switch (SB_GET()) {
    case TELOPT_TTYPE: {		/* Yaaaay! */
	static char terminalname[5+41] = "TERM=";

	settimer(ttypesubopt);

	if (SB_GET() != TELQUAL_IS) {
	    return;		/* ??? XXX but, this is the most robust */
	}

	terminaltype = terminalname+strlen(terminalname);

	while ((terminaltype < (terminalname + sizeof terminalname-1)) &&
								    !SB_EOF()) {
	    register int c;

	    c = SB_GET();
	    if (isupper(c)) {
		c = tolower(c);
	    }
	    *terminaltype++ = c;    /* accumulate name */
	}
	*terminaltype = 0;
	terminaltype = terminalname;
	break;
    } /* case TELOPT_TTYPE */

    case TELOPT_NAWS: {
    register int xwinsize, ywinsize;

    if (!hisopts[TELOPT_NAWS]) /* Ignore if option disabled */
        break;

    if (SB_EOF())
        return;
    xwinsize = SB_GET() << 8;
    if (SB_EOF())
        return;
    xwinsize |= SB_GET();
    if (SB_EOF())
        return;
    ywinsize = SB_GET() << 8;
    if (SB_EOF())
        return;
    ywinsize |= SB_GET();
#ifdef  TIOCSWINSZ
    {
       struct winsize ws;
        /*
         * Change window size as requested by client.
         */

        ws.ws_col = xwinsize;
        ws.ws_row = ywinsize;
        (void) ioctl(pty, TIOCSWINSZ, (char *)&ws);
    }
#endif  /* TIOCSWINSZ */


    break;

    }  /* end of case TELOPT_NAWS */

    default:
	;
    }
}


mode(flag, on, off)
int flag, on, off;
{
	struct termio b;

	if (ioctl(pty, TCGETA, &b) < 0)
		syslog(LOG_WARNING, "ioctl(TCGETA): %m");

	switch (flag)
	{
	case 0: b.c_iflag &= ~off;
		b.c_iflag |= on;
		break;
	case 1: b.c_oflag &= ~off;
		b.c_oflag |= on;
		break;
	case 2: b.c_cflag &= ~off;
		b.c_cflag |= on;
		break;
	case 3: b.c_lflag &= ~off;
		b.c_lflag |= on;
		break;
	default: b.c_cc[on] = off;
	}

	if (ioctl(pty, TCSETA, &b) < 0)
		syslog(LOG_WARNING, "ioctl(TCSETA): %m");
}

/*
 * Send interrupt to process on other side of pty.
 */
interrupt()
{
	if (ioctl(pty, TIOCSIGSEND, SIGINT) < 0)
		syslog(LOG_WARNING, "ioctl(TIOCSIGSEND): %m");
}

/*
 * Send break to process on other side of pty.
 */
sendbrk()
{
	if (ioctl(pty, TIOCBREAK, 0) < 0)
		syslog(LOG_WARNING, "ioctl(TIOCBREAK): %m");
}

ptyflush()
{
	int n;

	if ((n = pfrontp - pbackp) > 0)
		n = write(pty, pbackp, n);
	if (n < 0)
		return;
	pbackp += n;
	if (pbackp == pfrontp)
		pbackp = pfrontp = ptyobuf;
}

/*
 * nextitem()
 *
 *	Return the address of the next "item" in the TELNET data
 * stream.  This will be the address of the next character if
 * the current address is a user data character, or it will
 * be the address of the character following the TELNET command
 * if the current address is a TELNET IAC ("I Am a Command")
 * character.
 */

char *
nextitem(current)
char	*current;
{
    if ((*current&0xff) != IAC) {
	return current+1;
    }
    switch (*(current+1)&0xff) {
    case DO:
    case DONT:
    case WILL:
    case WONT:
	return current+3;
    case SB:		/* loop forever looking for the SE */
	{
	    register char *look = current+2;

	    for (;;) {
		if ((*look++&0xff) == IAC) {
		    if ((*look++&0xff) == SE) {
			return look;
		    }
		}
	    }
	}
    default:
	return current+2;
    }
}


/*
 * netclear()
 *
 *	We are about to do a TELNET SYNCH operation.  Clear
 * the path to the network.
 *
 *	Things are a bit tricky since we may have sent the first
 * byte or so of a previous TELNET command into the network.
 * So, we have to scan the network buffer from the beginning
 * until we are up to where we want to be.
 *
 *	A side effect of what we do, just to keep things
 * simple, is to clear the urgent data pointer.  The principal
 * caller should be setting the urgent data pointer AFTER calling
 * us in any case.
 */

netclear()
{
    register char *thisitem, *next;
    char *good;
#define	wewant(p)	((nfrontp > p) && ((*p&0xff) == IAC) && \
				((*(p+1)&0xff) != EC) && ((*(p+1)&0xff) != EL))

    thisitem = netobuf;

    while ((next = nextitem(thisitem)) <= nbackp) {
	thisitem = next;
    }

    /* Now, thisitem is first before/at boundary. */

    good = netobuf;	/* where the good bytes go */

    while (nfrontp > thisitem) {
	if (wewant(thisitem)) {
	    int length;

	    next = thisitem;
	    do {
		next = nextitem(next);
	    } while (wewant(next) && (nfrontp > next));
	    length = next-thisitem;
	    bcopy(thisitem, good, length);
	    good += length;
	    thisitem = next;
	} else {
	    thisitem = nextitem(thisitem);
	}
    }

    nbackp = netobuf;
    nfrontp = good;		/* next byte to be sent */
    neturg = 0;
}

/*
 *  netflush
 *		Send as much data as possible to the network,
 *	handling requests for urgent data.
 */


netflush()
{
    int n;

    if ((n = nfrontp - nbackp) > 0) {
	/*
	 * if no urgent data, or if the other side appears to be an
	 * old 4.2 client (and thus unable to survive TCP urgent data),
	 * write the entire buffer in non-OOB mode.
	 */
	if ((neturg == 0) || (not42 == 0)) {
	    n = write(net, nbackp, n);	/* normal write */
	} else {
	    n = neturg - nbackp;
	    /*
	     * In 4.2 (and 4.3) systems, there is some question about
	     * what byte in a sendOOB operation is the "OOB" data.
	     * To make ourselves compatible, we only send ONE byte
	     * out of band, the one WE THINK should be OOB (though
	     * we really have more the TCP philosophy of urgent data
	     * rather than the Unix philosophy of OOB data).
	     */
	    if (n > 1) {
		n = send(net, nbackp, n-1, 0);	/* send URGENT all by itself */
	    } else {
		n = send(net, nbackp, n, MSG_OOB);	/* URGENT data */
	    }
	}
    }
    if (n < 0) {
	if (errno == EWOULDBLOCK)
	    return 0;
	/* should blow this guy away... */
	return n;
    }
    nbackp += n;
    if (nbackp >= neturg) {
	neturg = 0;
    }
    if (nbackp == nfrontp) {
	nbackp = nfrontp = netobuf;
    }
    return 0;
}

/*
 * 
 * When the child died, we could get a SIGCLD before the last of the
 * data in the pty had been read and written to the socket.  Thus,
 * what we try and do is as follows:
 *
 * 	1) If there is already data to write to the socket, try and
 *         write it all out.
 *
 *	2) Do temporary join to flush any pty data to socket.
 *
 * If at any time, a write returns zero or an error, go ahead and exit
 */

void
child_died()
{
	int wc;
	void cleanup();

	/*
	** Require non-blocking I/O on the pty master.
	** O_NDELAY is used because it applies to the master and not the slave.
	** FIONBIO doesn't work on ptys.  (Is this a bug?)
	** FIOSNBIO works on ptys, but sets both the master and the slave.
	**   (Is this a bug?)
	*/
	fcntl(pty, F_SETFL, O_NDELAY);

	/*
	 * flush anything currently pending to socket 
	 */
#ifdef USERSPACE
#ifdef SIOCJNVS
	if (!kernel) 
#endif /* SIOCJNVS */
	    if (netflush() < 0)
		cleanup();
#endif /* USERSPACE */

#ifdef SIOCJNVS
	if (kernel) {
		/* do temporary join to drain data */
		wc = 8 << 24;
		if (hisopts[TELOPT_BINARY])
			wc |= 1 << 24;
		if (myopts[TELOPT_BINARY])
			wc |= 8 << 16;
		wc |= 2 << 16;	/* temporary join */
		wc |= pty;
#if defined(SecureWare) && defined(B1)
		/*
		**  remove this privilege bracketing if
		**  SIOCJNVS doesn't require privilege.
		*/
		if (ISB1)
			enablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
		if (ioctl(net,SIOCJNVS,(char *)&wc) == -1) {
			syslog(LOG_WARNING, "ioctl(SIOCJNVS): %m");
		}
#if defined(SecureWare) && defined(B1)
		if (ISB1)
			disablepriv(SEC_REMOTE);
#endif /* SecureWare && B1 */
	}
#endif /* SIOCJNVS */

	/*
	 * now we can exit knowing we flushed as much as we could.
	 */
	cleanup();

}

void
cleanup()
{
	/*
	 * clean up utmp entry
	 */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		enablepriv(SEC_ALLOWDACACCESS);
		enablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
	account(DEAD_PROCESS,telnet_pid,"",slave,NULL,NULL);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_ALLOWDACACCESS);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* SecureWare && B1 */
#ifdef SecureWare
	/*
	** restore pty device files to their unused state
	*/
	if (ISSECURE) {
		(void) telnetd_condition_line(master);
		(void) telnetd_condition_line(slave);
	}
	else
#endif /* SecureWare */
	{
		chmod(master,0666);
		chown(master, 0, 0);
		chmod(slave,0666);
		chown(slave, 0, 0);
	
		/* added for Port Id Changes */

		if( strcmp( slave, O_device_name ) != 0 )
			unlink( O_device_name );
		/* end of changes */
	}

	shutdown(net, 2);
	exit(1);

}


/* 
** routine to set up the banner to be printed first.
** simply get the information from uname, unless it fails
** in which case we simply print a default banner.
*/

#define DEFAULT "\r\nHP-UX (%s)\r\n\r\n"
#define BANNER "\r\n%s %s %s %s %s (%s)\r\n\r\n"

#include <sys/utsname.h>

char *getbanner()
{
	char hostname[32];
	struct utsname name;
	static char buffer[200];
	char *tty;

	tty = (char *)strrchr(tname, '/');
	if (tty++ == NULL)
		tty = "unknown";
	
	gethostname(hostname, sizeof (hostname));
	if ( uname (&name) < 0 ) {
		sprintf(buffer,DEFAULT,hostname);
	}
	else
		sprintf(buffer, BANNER, name.sysname, hostname,
			name.release, name.version, name.machine,
			tty);
	return(buffer);
}


#ifdef SecureWare
/*
** restore pty device files to their unused state
*/

telnetd_condition_line(line)
	register char *line;
{
	register int uid_to_set, gid_to_set;
	register struct group *g;
	register struct passwd *p;

	p = getpwnam("bin");
	if (p != (struct passwd *) 0)
		uid_to_set = p->pw_uid;
	else
		uid_to_set = 0;

	g = getgrnam("terminal");
	if (g != (struct group *) 0)
		gid_to_set = g->gr_gid;
	else
		gid_to_set = GID_NO_CHANGE;

	endgrent();

#ifdef B1
	if (ISB1) {
		enablepriv(SEC_OWNER);
	}
#endif /* B1 */
	if (chmod(line, 0600) < 0)
		syslog(LOG_ERR, "chmod(%s, 0600): %m", line);
	if (chown(line, uid_to_set, gid_to_set) < 0)
		syslog(LOG_ERR, "chown(%s, %d, %d): %m", line,
			uid_to_set, gid_to_set);
#ifdef B1
	if (ISB1) {
		/*
	 	 * Reset the sensitivity label of the pty to WILDCARD.
	 	 */
		enablepriv(SEC_ALLOWMACACCESS);
		if (chslabel(line, mand_er_to_ir(AUTH_F_WILD)) < 0)
			syslog(LOG_ERR, "chslabel(%s, %s): %m",
				line, AUTH_F_WILD);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* B1 */

	/*
	 * Disable I/O on line.
	 * All future attempts at I/O on file descriptors referring
	 * to line will cause SIGSYS (?) to be sent to the process 
	 * attempting the I/O.
	 */
	if (stopio(line) < 0)
		syslog(LOG_ERR, "stopio(%s): %m", line);
#ifdef B1
	if (ISB1) {
		disablepriv(SEC_OWNER);
	}
#endif /* B1 */
}
#endif /* SecureWare */


/*--------------------------------------------------------------------------

Changes by Rajesh Srivastava for Port Id. 2/21/92

---------------------------------------------------------------------------*/

struct seq {
		char seq[15];
		int  size;
	   };

struct seq pseq[8] = {
			{ IAC, DO, TELOPT_ENV }, { 3 },
			{ IAC, WILL, TELOPT_ENV }, { 3 },
			{ IAC, WONT, TELOPT_ENV }, { 3 },
			{ IAC, DONT, TELOPT_ENV }, { 3 },
			{ IAC, SB, TELOPT_ENV, TELQUAL_SEND, TELQUAL_USERVAR, 
			  IAC, SE }, { 7 },
			{ IAC, SB, TELOPT_ENV, TELQUAL_IS, TELQUAL_USERVAR }, { 5 }, 
			{ 'b', 'o', 'a', 'r', 'd', TELQUAL_VALUE }, { 6 },
			{ TELQUAL_USERVAR,'p', 'o', 'r', 't', TELQUAL_VALUE }, { 6 }
		     };

/*------------------------------------------------------------------------

	This routine handles the Environment Option Negotiation.

		Author	RAJESH SRIVASTAVA

--------------------------------------------------------------------------*/

get_portid_param(f, master, slave)
int  *f;
char *master,*slave;
{

	unsigned char pidbuf[BUFSIZ];
	int  pidcc = 0, pidcc_hold;
	int	send_next, remotelen;
	unsigned char *p, *p_hold;
	char b_val[5], p_val[5];
	int	board, port;
	u_long	IP_address;
	struct sockaddr_in remote;
	struct stat buf;
	int	loop = 1;

#ifdef 	DEBUG
	char tmp[80];
	long	a1,b1,c1,d1,a,b,c,d;	
#endif 	DEBUG
	
	board = port = 0;

	if( write( net, pseq[0].seq, pseq[0].size ) != pseq[0].size )
	{
		syslog( LOG_INFO, "getpid : peer died: %m\n");
		exit(1);
	}

#ifdef	DEBUG
	tellog(fd, SEND_OPER, pseq[0].seq, pseq[0].size );
#endif	DEBUG

	/*	
	 Read the reply from the socket.
	*/	

	scc = 0;
	while(loop)
	{
		pidcc =  read( net, pidbuf, sizeof(pidbuf));

#ifdef 	DEBUG 
		tellog(fd, RECV_OPER, pidbuf, pidcc );
#endif	DEBUG

		if( pidcc < 0 )
		{
			syslog( LOG_INFO, "getpid : read: %m\n");
			exit(1);
		}

		if( pidcc == 0 )
		{
			syslog( LOG_INFO, "getpid : peer died: %m\n");
			exit(1);
		}

	/*	
		 process the reply
	*/	

		p = pidbuf;

		while( pidcc )
		{
			if( msgcmp( pseq[1].seq, p, pseq[1].size ) )
			{
				/* WILL option received
				*/
				send_next = 1;
				pidcc -= pseq[1].size;
				p += pseq[1].size;
				loop = 0;
				continue;
			}

			if( msgcmp( pseq[2].seq, p, pseq[2].size ) )
			{
				/* WONT option received
				*/
				send_next = 0;
				pidcc -= pseq[2].size;
				p += pseq[2].size;
				loop = 0;
				continue;
			}

			if( msgcmp( pseq[3].seq, p, pseq[3].size ))
			{
				/* DONT option received
				*/
				
				send_next = 0;
				pidcc -= pseq[3].size;
				p += pseq[3].size;
				loop =0;
				continue;
			}

			if( scc < sizeof( stobuf) )
			{
				stobuf[scc++] = *p++;
				pidcc--;
			}
			else	pidcc = 0;
		}
	
#ifdef DEBUG
		tellog( fd, RECV_OPER, stobuf, scc );
#endif DEBUG
		
	}
	
	if( send_next )
	{
		if( write( net, pseq[4].seq, pseq[4].size) != pseq[4].size )
		{
			syslog( LOG_INFO, "getpid : peer died: %m\n");
			exit(1);
		}

#ifdef DEBUG
		tellog( fd, SEND_OPER, pseq[4].seq, pseq[4].size);
#endif	DEBUG

	/*	
		 Read the reply from the socket.
	
	*/
		loop = 1;
		while( loop )
		{
			pidcc =  read( net, pidbuf, sizeof(pidbuf));
	
#ifdef	DEBUG
			tellog(fd, RECV_OPER, pidbuf, pidcc );
#endif	DEBUG

			if( pidcc < 0 )
			{
				syslog( LOG_INFO, "getpid : read: %m\n");
				exit(1);
			}

			if( pidcc == 0 )
			{
				syslog( LOG_INFO, "getpid : peer died: %m\n");
				exit(1);
			}

			p = pidbuf;
			while( pidcc )
			{
				if( msgcmp( pseq[2].seq, p, pseq[5].size ) )
				{
					p += pseq[2].size;
					pidcc -= pseq[2].size;
					loop = 0;
					send_next = 0;
				}

				if( msgcmp( pseq[5].seq, p, pseq[5].size ) )
				{
					p += pseq[5].size;
					pidcc -= pseq[5].size;

					if( msgcmp( pseq[6].seq, p, pseq[6].size ))
					{	
						p += pseq[6].size;
						pidcc -= pseq[6].size;
						*b_val = *p++;
						*(b_val+1) = *p++;
						*(b_val+2) = '\0';
						board = (int)atol( b_val );
						pidcc -= 2;	
						if( msgcmp( pseq[7].seq, p, pseq[7].size ) )
						{
							p += pseq[7].size;
							pidcc -= pseq[7].size;
							*p_val = *p++;
							*(p_val+1) = *p++;
							*(p_val+2) = *p++;
							*(p_val+3) = '\0';
							pidcc -= 3;
							port = (int)atol( p_val );
							if( (*p == IAC) && (*(p+1) == SE))
							{
								p += 2;
								pidcc -= 2;
#ifdef	DEBUG
								sprintf( tmp, "\nBoard = %d Port = %d\n", board, port );
								write( fd, tmp, strlen( tmp ));
#endif	DEBUG
								loop = 0;
								continue;
							}
							else	{
									send_next = 0;
							}
						}
						else	 send_next = 0;
					}
				}

                                /* We are about to handle a situation where
    				   we got a reply to our request for USERVAR
				   but it doesn't contain any useful info.
				   This case will occur if we are trying to
				   telnet from a PC running SUN PC NFS.  If
				   we receive a reply we're not expecting
				   but it's terminated by IAC SE, we'll
				   ignore it and continue the telnet
				   connection without Port ID func.   */

                                pidcc_hold = pidcc;
				p_hold = p;

				while ( pidcc )
				{
					if( ( *p == IAC ) && ( *(p+1) == SE ))
					{
						loop = 0;
						pidcc = 0;
						send_next = 0;
						continue;
					}
					p++;
					pidcc--;
				}

                                pidcc = pidcc_hold;
				p = p_hold;

				if( scc < sizeof( stobuf) )
				{	
					stobuf[scc++] = *p++;
					pidcc--;
				}
				else	pidcc = 0;
			}
		} 
			
	}
#ifdef	DEBUG 
	tellog( fd, RECV_OPER, stobuf, scc );
#endif	DEBUG
	if( getpty( f, master, slave ) != 0 )
	{
		syslog( LOG_ERR, "Cannot allocate pty");
		fatal( f, "Unable to allocate pty on remote host");
		return(1);
	}
	if( send_next)	
	{
		remotelen = sizeof (remote);
		getpeername(0, &remote, &remotelen);
		IP_address = remote.sin_addr.s_addr;
#ifdef DEBUG
		a = IP_address/ ( 256*256*256);
		a1 = a*256*256*256;
		b = (IP_address - a1 ) / (256*256);
		b1 = b*256*256;
		c = (IP_address -a1 -b1 )/ 256;
		c1 = c*256;
		d = (IP_address -a1 -b1 -c1 );
		sprintf(tmp, "\nIP address = %d.%d.%d.%d\n", a,b,c,d );
		write( fd, tmp, strlen(tmp));
#endif DEBUG
		if( stat( slave, &buf ) == 0 )
		{
			if( get_f_device_name( IP_address, port, board,
			    O_device_name ) == 0 )
			{
				if( mknod( O_device_name, S_IFCHR, 
				           buf.st_rdev) != 0 )
				strcpy( O_device_name, slave );
			}
			else	{
#ifdef	DEBUG
					strcpy(tmp, "\n get_f_device Failed\n");
					write(fd, tmp, strlen(tmp));
#endif	DEBUG
					strcpy( O_device_name, slave );
				}
		}
	}
	else	strcpy( O_device_name, slave );
	return(0);
}

msgcmp( p, q, size )
char *p,*q;
int size;
{
	while( size)
	{
		if( *p++ == *q++)
			size--;
		else	return(0);
	}
	return(1);
}

tellog( fd, oper, buf, size )
int	fd, oper, size;
char *buf;
{
	char tmp[80];
	if(( size == 0 ) && (oper != 0 ))
		return;
	switch( oper )
	{
		case SEND_OPER:
			write( fd, "\nSEND    ",9);
			break;
		case RECV_OPER:
			write( fd, "\nRECEIVE ",9);
			break;
		default:
			strcpy( tmp, buf );
			write(fd, tmp, strlen(tmp));
			return;
		}
	write( fd, buf, size );
}



