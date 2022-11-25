/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# include <errno.h>
# include "sendmail.h"

# ifndef lint
static char rcsid[]="$Header: daemon.c,v 1.29.109.9 95/02/21 16:07:38 mike Exp $";
# 	ifndef hpux
# 		ifdef DAEMON
static char sccsid[] = "@(#)daemon.c	5.36 (Berkeley) 6/1/90 (with daemon mode)";
# 		else	/* ! DAEMON */
static char sccsid[] = "@(#)daemon.c	5.36 (Berkeley) 6/1/90 (without daemon mode)";
# 		endif	/* DAEMON */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: daemon.o $Revision: 1.29.109.9 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# ifdef DAEMON

# 	include <netdb.h>
# 	include <sys/signal.h>
# 	include <sys/wait.h>
# 	include <sys/param.h>
# 	include <sys/time.h>
# 	include <sys/resource.h>
# 	include <errno.h>
# 	include <arpa/nameser.h>
# 	include <resolv.h>

/*
**  DAEMON.C -- routines to use when running as a daemon.
**
**	This entire file is highly dependent on the 4.2 BSD
**	interprocess communication primitives.  No attempt has
**	been made to make this file portable to Version 7,
**	Version 6, MPX files, etc.  If you should try such a
**	thing yourself, I recommend chucking the entire file
**	and starting from scratch.  Basic semantics are:
**
**	getrequests()
**		Opens a port and initiates a connection.
**		Returns in a child.  Must set InChannel and
**		OutChannel appropriately.
**	clrdaemon()
**		Close any open files associated with getting
**		the connection; this is used when running the queue,
**		etc., to avoid having extra file descriptors during
**		the queue run and to avoid confusing the network
**		code (if it cares).
**	makeconnection(host, port, outfile, infile)
**		Make a connection to the named host on the given
**		port.  Set *outfile and *infile to the files
**		appropriate for communication.  Returns zero on
**		success, else an exit status describing the
**		error.
**	maphostname(hbuf, hbufsize)
**		Convert the entry in hbuf into a canonical form.  It
**		may not be larger than hbufsize.
**	closeconnection(fd)
**		Marks the host which we connected to via file desc 'fd'
**		as closed, so we won't try to reuse the connection.
**		Note that this does not actually close the file
**		descriptor.  That's the caller's responsibility.
*/

static struct hostinfo *host_from_fd[NOFILE];	/* Host entry for each */
/*
**  GETREQUESTS -- open mail IPC port and get requests.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Waits until some interesting activity occurs.  When
**		it does, a child is created to process it, and the
**		parent waits for completion.  Return from this
**		routine is always in the child.  The file pointers
**		"InChannel" and "OutChannel" should be set to point
**		to the communication channel.
*/

struct sockaddr_in	SendmailAddress;/* internet address of sendmail */

int	DaemonSocket	= -1;		/* fd describing socket */
char	*NetName;			/* name of home (local?) network */

void getrequests()
{
	int t;
	register struct servent *sp;
	int on = 1;
	void daemonexit();

	/*
	**  Set up the address for the mailer.
	*/

	sp = getservbyname("smtp", "tcp");
	if (sp == NULL)
	{
		syserr("service \"smtp\" unknown");
		goto severe;
	}
	SendmailAddress.sin_family = AF_INET;
	SendmailAddress.sin_addr.s_addr = INADDR_ANY;
	SendmailAddress.sin_port = sp->s_port;

	/*
	**  Try to actually open the connection.
	*/

# 	ifdef DEBUG
	if (tTd(15, 1))
		printf("getrequests: port 0x%x\n", SendmailAddress.sin_port);
# 	 endif /* DEBUG */

	/* get a socket for the SMTP connection */
	DaemonSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (DaemonSocket < 0)
	{
		/* probably another daemon already */
		syserr("getrequests: can't create socket");
	  severe:
# 	ifdef LOG
		if (LogLevel > 0)
			syslog(LOG_ALERT, "problem creating SMTP socket");
# 	endif /* LOG */
		finis();
	}

# 	ifdef DEBUG
	/* turn on network debugging? */
	if (tTd(15, 101))
		(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof on);
# 	 endif /* DEBUG */

	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

	if (bind(DaemonSocket, &SendmailAddress, sizeof SendmailAddress) < 0)
	{
		syserr("getrequests: Cannot bind");
		(void) close(DaemonSocket);
		goto severe;
	}

	if (listen(DaemonSocket, 10) < 0)
	{
		syserr("getrequests: Cannot listen");
		(void) close(DaemonSocket);
		goto severe;
	}


	/* ignore SIGCLD, generate no zombies. */
	signal(SIGCLD, SIG_IGN);

# 	ifdef DEBUG
	if (tTd(15, 1))
		printf("getrequests: %d\n", DaemonSocket);
# 	 endif /* DEBUG */
# 	ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "accepting SMTP connections, pid=%d", getpid());
	else if (LogLevel > 8)
		syslog(LOG_INFO, "accepting SMTP connections");
# 	 endif /* LOG */

	for (;;)
	{
		register int pid;
		auto int lotherend;
		extern int RefuseLA;
# 	ifdef LOG
		int minutes, logrefuse;

		minutes = 0;
		logrefuse = 0;
# 	 endif /* LOG */

		/*
		**	die on SIGTERM while waiting for connections
		*/
		signal(SIGTERM, daemonexit);

		/* see if we are rejecting connections */
		while ((CurrentLA = getla()) > RefuseLA)
		{
# 	ifdef LOG
			char minutebuf[40];

			if (minutes == 0)
				minutebuf[0] = '\0';
			else if (minutes == 1)
				sprintf (minutebuf, " for 1 minute");
			else 
				sprintf (minutebuf, " for %d minutes", minutes);

			/*
			**	log that we are refusing, once a minute
			*/
			if (logrefuse == 0 && LogLevel > 11)
				syslog(LOG_DEBUG, 
				       "load average %.2f, refusing connections%s",
				       (double)CurrentLA, minutebuf);
			logrefuse++;
			minutes += logrefuse / 12;
			logrefuse %= 12;
# 	 endif /* LOG */
			setproctitle("rejecting connections: load average: %.2f", (double)CurrentLA);
			sleep(5);
		}

		/* wait for a connection */
		setproctitle("accepting connections");
		do
		{
			errno = 0;
			lotherend = sizeof RealHostAddr;
			t = accept(DaemonSocket, &RealHostAddr, &lotherend);
		} while (t < 0 && errno == EINTR);

		/*
		**	hold off SIGTERM until finished handling this connection
		*/
		signal(SIGTERM, SIG_IGN);

		if (t < 0)
		{
			syserr("getrequests: accept");
			sleep(5);
			continue;
		}

		/*
		**  Create a subprocess to process the mail.
		*/

# 	ifdef DEBUG
		if (tTd(15, 2))
			printf("getrequests: forking (fd = %d)\n", t);
# 	 endif /* DEBUG */

		pid = dofork();
		if (pid < 0)
		{
			syserr("daemon: cannot fork");
			sleep(10);
			(void) close(t);
			continue;
		}

		if (pid == 0)
		{
			extern struct hostent *gethostbyaddr();
			register struct hostent *hp;
			extern char *RealHostName;	/* srvrsmtp.c */
			char buf[MAXNAME];
			extern int h_errno;

			/*
			**  CHILD -- return to caller.
			**	Collect verified idea of sending host.
			**	Verify calling user id if possible here.
			*/
			signal(SIGCLD, SIG_DFL);

			/* determine host name */
			h_errno = 0;
			hp = gethostbyaddr((char *) &RealHostAddr.sin_addr,
			    sizeof(struct in_addr), AF_INET);
			if (hp != NULL)
				(void) strcpy(buf, hp->h_name);
			else
			{
				extern char *inet_ntoa();

				/* produce a dotted quad */
				(void) sprintf(buf, "[%s]",
					inet_ntoa(RealHostAddr.sin_addr));
			}

			/* should we check for illegal connection here? XXX */

			RealHostName = newstr(buf);

			(void) close(DaemonSocket);
			InChannel = fdopen(t, "r");
			OutChannel = fdopen(dup(t), "w");
# 	ifdef DEBUG
			if (tTd(15, 2))
				printf("getreq: returning\n");
# 	 endif /* DEBUG */
# 	ifdef LOG
			if (LogLevel > 11)
				syslog(LOG_DEBUG, "connected to %s, pid=%d",
				       RealHostName, getpid());
# 	 endif /* LOG */
			return;
		}

		/* close the port so that others will hang (for a while) */
		(void) close(t);
	}
	/*NOTREACHED*/
}
/*
**  DAEMONEXIT -- exit daemon,  and possibly log
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		logs exit, maybe, and exits.
*/

void
daemonexit()
{
# 	ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "daemon: exiting, pid=%d", getpid());
	else if (LogLevel > 8)
		syslog(LOG_INFO, "daemon: exiting");
# 	 endif /* LOG */
	exit(EX_OK);
}
/*
**  CLRDAEMON -- reset the daemon connection
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		releases any resources used by the passive daemon.
*/

void clrdaemon()
{
	if (DaemonSocket >= 0)
		(void) close(DaemonSocket);
	DaemonSocket = -1;
}
/*
**  MAKECONNECTION -- make a connection to an SMTP socket on another machine.
**
**	Parameters:
**		host -- the name of the host.
**		port -- the port number to connect to.
**		outfile -- a pointer to a place to put the outfile
**			descriptor.
**		infile -- ditto for infile.
**
**	Returns:
**		An exit code telling whether the connection could be
**			made and if not why not.
**
**	Side Effects:
**		none.
*/

makeconnection(host, port, outfile, infile)
	char *host;
	u_short port;
	FILE **outfile;
	FILE **infile;
{
	register int i, s;
	register struct hostent *hp = (struct hostent *)NULL;
	extern char *inet_ntoa();
	int sav_errno;
# 	ifdef NAMED_BIND
	extern int h_errno;
# 	endif	/* NAMED_BIND */
	static char servaddr[64];


	/*
	**  Set up the address for the mailer.
	**	Accept "[a.b.c.d]" syntax for host name.
	*/

# 	ifdef NAMED_BIND
	h_errno = 0;
# 	endif	/* NAMED_BIND */
	errno = 0;
	ServerAddress = NULL;
	CurMxHost = NULL;

	if (host[0] == '[')
	{
		long hid;
		register char *p = strchr(host, ']');

		if (p != NULL)
		{
			*p = '\0';
			hid = inet_addr(&host[1]);
			*p = ']';
		}
		if (p == NULL || hid == -1)
		{
			usrerr("553 Invalid numeric domain spec \"%s\"", host);
			return (EX_NOHOST);
		}
		SendmailAddress.sin_addr.s_addr = hid;
	}
	else
	{
		/*
		** Disable the usual resolver search scheme.
		** Disable appending of default domain names unless
		** the host has no domain.
		*/
		_res.options &= ~RES_DNSRCH;
		if (strchr(host, '.') != NULL)
			_res.options &= ~RES_DEFNAMES;

		h_errno = 0;
		hp = gethostbyname(host);

		/*
		** Restore the resolver options.
		*/
		_res.options |= (RES_DEFNAMES | RES_DNSRCH);

		if (hp == NULL) 
		{
# 	ifdef NAMED_BIND
			if ((errno != ECONNREFUSED) && 
			    (errno == ETIMEDOUT || h_errno == TRY_AGAIN))
				return (EX_TEMPFAIL);

			/* if name server is specified, assume temp fail */
			if (errno == ECONNREFUSED && UseNameServer)
				return (EX_TEMPFAIL);

			/* if name server was not used, punt errno */
			if (errno == ECONNREFUSED)
				errno = 0;
# 	endif	/* NAMED_BIND */

			/*
			**  XXX Should look for mail forwarder record here
			**  XXX if (h_errno == NO_ADDRESS).
			*/

			return (EX_NOHOST);
		}
		bcopy(hp->h_addr, (char *) &SendmailAddress.sin_addr, hp->h_length);
		i = 1;
	}

	/*
	**  Determine the port number.
	*/

	if (port != 0)
		SendmailAddress.sin_port = htons(port);
	else
	{
		register struct servent *sp = getservbyname("smtp", "tcp");

		if (sp == NULL)
		{
			syserr("makeconnection: service \"smtp\" unknown");
			return (EX_OSFILE);
		}
		SendmailAddress.sin_port = sp->s_port;
	}

	/*
	**  Actually try to open the connection.
	*/

	ServerAddress = servaddr;
	CurMxHost = host;
again:

	/*
	** For logging, save a copy of the address we are trying 
	** to connect to.
	*/
	strcpy(ServerAddress, inet_ntoa(SendmailAddress.sin_addr.s_addr));

# 	ifdef DEBUG
	if (tTd(16, 1))
		printf("makeconnection (%s [%s])\n", host,
                     inet_ntoa(SendmailAddress.sin_addr.s_addr));
# 	 endif /* DEBUG */

	message("  trying address %s...", inet_ntoa(SendmailAddress.sin_addr.s_addr));

	s = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (s < 0)
	{
		sav_errno = errno;
		syserr("makeconnection: socket");
		goto failure;
	}

# 	ifdef DEBUG
	if (tTd(16, 1))
		printf("makeconnection: %d\n", s);

	/* turn on network debugging? */
	if (tTd(16, 14))
	{
		int on = 1;
		(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof on);
	}
# 	 endif /* DEBUG */
	if (CurEnv->e_xfp != NULL)
		(void) fflush(CurEnv->e_xfp);		/* for debugging */
	errno = 0;					/* for debugging */
	h_errno = 0;
	SendmailAddress.sin_family = AF_INET;
	if (connect(s, &SendmailAddress, sizeof SendmailAddress) < 0)
	{
		sav_errno = errno;
		(void) close(s);
		if (hp && hp->h_addr_list[i])
		{
			bcopy(hp->h_addr_list[i++],
			    (char *)&SendmailAddress.sin_addr, hp->h_length);
			goto again;
		}

		/* failure, decide if temporary or not */
	failure:
		switch (sav_errno)
		{
		  case EISCONN:
		  case ENOSPC:
		  case ETIMEDOUT:
		  case EINPROGRESS:
		  case EALREADY:
		  case EADDRINUSE:
		  case EHOSTDOWN:
		  case ENETDOWN:
		  case ENETRESET:
		  case ENOBUFS:
		  case ECONNREFUSED:
		  case ECONNRESET:
		  case EHOSTUNREACH:
		  case ENETUNREACH:
			/* there are others, I'm sure..... */
			return (EX_TEMPFAIL);

		  case EPERM:
			/* why is this happening? */
			syserr("makeconnection: funny failure, addr=%lx, port=%x",
			       SendmailAddress.sin_addr.s_addr,
			       SendmailAddress.sin_port);
			return (EX_TEMPFAIL);
		  default:
			{
				extern char *errstring();

				message("%s", errstring(sav_errno));
				return (EX_UNAVAILABLE);
			}
		}
	}

	/* connection ok, put it into canonical form */
	*outfile = fdopen(s, "w");
	*infile = fdopen(dup(s), "r");

	{
	    int bigbufsiz;

	    if ((getsockopt(s,SOL_SOCKET,SO_SNDBUF,&bigbufsiz,sizeof(int)) == 0) &&
		bigbufsiz < BIGBUFSIZ)
	    {

		/* Enlarge socket send buffer */
		bigbufsiz = BIGBUFSIZ;
		if (setsockopt(s,SOL_SOCKET,SO_SNDBUF,&bigbufsiz,sizeof(int)) < 0)
		{
			syserr("makeconnection: setsockopt");
		}
	    }

	    /*
	     *	Enlarge write buffer; read buffering is handled
	     *	conditionally in smtp().
	     */
	    setvbuf(*outfile, NULL, _IOFBF, BIGBUFSIZ);
	}

	return (EX_OK);
}
/*
**  CLOSECONNECTION -- mark an open connection as closed.
**
**	Parameters:
**		fd -- the file descriptor of the connection.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Marks the host (which had a connection opened via
**		makeconnection()) as not having a current connection.
**		Note that this does not actually close the file
**		descriptor.  That's the caller's responsibility.
**		Also frees the memory allocated by RealHostName in the
**		"newstr()" call above; otherwise queue runners are swap hogs.
*/
void closeconnection(fd)
	int fd;
{
	register struct hostinfo *hp;

	if (fd < NOFILE) {
		hp = host_from_fd[fd];
# 	ifdef SUN
		if (hp != NULL && hp->h_open) {
			hp->h_open = 0;
		}
# 	 endif /* SUN */
		host_from_fd[fd] = NULL;
		if (RealHostName && RealHostName[0] != '\0') {
			free(RealHostName);
			RealHostName = "";
		}
	}
}
/*
**  MYHOSTNAME -- return the name of this host.
**
**	Parameters:
**		hostbuf -- if empty, a place to return the name of this host.
**			   otherwise, the name we want to be known by.
**		size -- the size of hostbuf.
**
**	Returns:
**		A list of aliases for this host.
**
**	Side Effects:
**		none.
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	extern struct hostent *gethostbyname();
	struct hostent *hp;
# 	if ((__RES >= 19931104) && defined (V4FS))
	struct __res_state save_res;
	extern struct __res_state _res;
# 	else	/* ! defined (V4FS)) */
	struct state save_res;
	extern struct state _res;
# 	endif	/* defined (V4FS)) */

	if (hostbuf[0] == '\0' && gethostname(hostbuf, size) < 0)
	{
		(void) strcpy(hostbuf, "localhost");
	}

	/*
	** Up the number of retries to the name server,
	** should gethostbyname use it.
	*/
	save_res.retry = _res.retry;
	save_res.retrans = _res.retrans;
	_res.retry = 6;
	_res.retrans = 2;

	hp = gethostbyname(hostbuf);

	/*
	** Restore the original resolver state.
	*/
	_res.retry = save_res.retry;
	_res.retrans = save_res.retrans;

	if (hp != NULL)
                (void) strncpy(hostbuf, hp->h_name, size);
	else {
		/* if name server was not used, punt errno */
		if (errno == ECONNREFUSED)
			errno = 0;
		syserr("Local hostname lookup failed");
	}

	if (hp != NULL)
		/*
		**	no aliases if nameserver is running.
		*/
		return(hp->h_aliases);
	else
		return(NULL);
}

/*
 *  MAPHOSTNAME -- turn a hostname into canonical form
 *
 *	Parameters:
 *		hbuf -- a buffer containing a hostname.
 *		hbsize -- the size of hbuf.
 *
 *	Returns:
 *		none.
 *
 *	Side Effects:
 *		Looks up the host specified in hbuf.  If it is not
 *		the canonical name for that host, replace it with
 *		the canonical name.  If the name is unknown, or it
 *		is already the canonical name, leave it unchanged.
 */
void maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	register struct hostent *hp;
	u_long in_addr;
	char ptr[256], *cp;
	struct hostent *gethostbyaddr();
	extern int h_errno;

	/*
	 * If first character is a bracket, then it is an address
	 * lookup.  Address is copied into a temporary buffer to
	 * strip the brackets and to preserve hbuf if address is
	 * unknown.
	 */
	if (*hbuf != '[') {
		getcanonname(hbuf, hbsize);
		return;
	}
	if ((cp = strchr(strcpy(ptr, hbuf), ']')) == NULL)
		return;
	*cp = '\0';
	in_addr = inet_addr(&ptr[1]);
	h_errno = 0;
	hp = gethostbyaddr((char *)&in_addr, sizeof(struct in_addr), AF_INET);
	if (hp == NULL)
		return;
	if (strlen(hp->h_name) >= hbsize)
		hp->h_name[hbsize - 1] = '\0';
	(void)strcpy(hbuf, hp->h_name);
}

# else /* DAEMON */
/* code for systems without sophisticated networking */

/*
**  MYHOSTNAME -- stub version for case of no daemon code.
**
**	Can't convert to upper case here because might be a UUCP name.
**
**	Mark, you can change this to be anything you want......
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	register FILE *f;

	hostbuf[0] = '\0';
	f = fopen("/usr/include/whoami", "r");
	if (f != NULL)
	{
		(void) fgets(hostbuf, size, f);
		fixcrlf(hostbuf, TRUE);
		(void) fclose(f);
	}
	return (NULL);
}
/*
**  MAPHOSTNAME -- turn a hostname into canonical form
**
**	Parameters:
**		hbuf -- a buffer containing a hostname.
**		hbsize -- the size of hbuf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Looks up the host specified in hbuf.  If it is not
**		the canonical name for that host, replace it with
**		the canonical name.  If the name is unknown, or it
**		is already the canonical name, leave it unchanged.
*/

/*ARGSUSED*/
void maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	return;
}

# endif /* DAEMON */
