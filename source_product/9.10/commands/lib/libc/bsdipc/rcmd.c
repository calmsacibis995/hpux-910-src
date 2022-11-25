/* @(#)$Header: rcmd.c,v 70.1 93/11/02 11:51:31 ssa Exp $ */

/*
**	generic remote command facility -- implements rcmd and rexec
*/

#ifdef _NAMESPACE_CLEAN
#define fflush _fflush
#define fputs _fputs
#define fputc _fputc
#define close _close
#define fprintf _fprintf
#define sprintf _sprintf
#define accept _accept 
#define strcpy _strcpy
#define strlen _strlen
#define strchr _strchr
#define sleep _sleep
#define alarm _alarm
#define sigvector __sigvector
#define read _read
#define write _write
#define recv _recv
#define ioctl _ioctl
#define gethostbyname _gethostbyname
#define inet_addr _inet_addr
#define inet_ntoa _inet_ntoa
#define ruserpass _ruserpass
#define socket _socket
#define setsockopt _setsockopt
#define getsockname _getsockname
#define connect _connect
#define listen _listen
#define select _select
#define bind _bind
#define memset _memset
#define memcpy _memcpy
#define perror _perror
#define getlogin _getlogin
#define getlongpass _getlongpass
#define exit _exit
#ifdef _ANSIC_CLEAN
#define malloc _malloc
#define free _free
#endif
/* local */
#define rresvport _rresvport
#define rcmd _rcmd
#define rexec _rexec
/* NLS */
#ifdef NLS
#define catgets _catgets
#define catopen _catopen
#endif
#endif _NAMESPACE_CLEAN



#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS


#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <errno.h>

#include <string.h>
#include <signal.h>

extern	errno;
static int	doremote();

static	int			/* made global for signal handler */
	s=0,			/* rcmd/rexec primary data socket	*/
	s2=0,			/* rcmd/rexec listen()/accept() socket	*/
	s3=0;			/* secondary (stderr) socket */

#define RCMD_ENTRY	0
#define RCMD_NAME	"rcmd"
#define REXEC_ENTRY	1
#define REXEC_NAME	"rexec"

#define	MAX_NAME	64
#ifdef NLS
static nl_catd nlmsg_fd;
#endif NLS



#ifdef _NAMESPACE_CLEAN
#undef rcmd
#pragma _HP_SECONDARY_DEF _rcmd rcmd
#define rcmd _rcmd
#endif _NAMESPACE_CLEAN

int
rcmd(ahost, rport, luser, ruser, cmd, fd2p)
	char **ahost;
	int rport;
	char *luser, *ruser, *cmd;
	int *fd2p;
{
    return(doremote(RCMD_ENTRY, ahost, rport, luser, ruser, cmd, fd2p));
}



#ifdef _NAMESPACE_CLEAN
#undef rexec
#pragma _HP_SECONDARY_DEF _rexec rexec
#define rexec _rexec
#endif _NAMESPACE_CLEAN

int
rexec(ahost, rport, name, pass, cmd, fd2p)
	char **ahost;
	int rport;
	char *name, *pass, *cmd;
	int *fd2p;
{
    return(doremote(REXEC_ENTRY, ahost, rport, name, pass, cmd, fd2p));
}



static int
doremote(entry, ahost, rport, luser_ruser, ruser_pw, cmd, fd2p)
    int entry;
    char **ahost;
    int rport;
    char *luser_ruser, *ruser_pw, *cmd;
    int *fd2p;
{
    int lport = IPPORT_RESERVED - 1;
    int timo = 1, sel, selected = 0, on = 1;
    char c, *function_name, errbuf[BUFSIZ];
    static char hnamebuf[MAX_NAME];
    struct sockaddr_in sin, sin2, from;
    struct hostent *hp;
    fd_set mask;
    struct timeval timeout;
    struct sigvec vec, sig;
    void catch_pipe(), catch_alarm();

    function_name = entry == RCMD_ENTRY ? RCMD_NAME : REXEC_NAME;
#ifdef NLS
    nlmsg_fd = catopen("rcmd",0);
#endif NLS
    strcpy(hnamebuf, *ahost);
    errno = 0;
    hp = gethostbyname(*ahost);
    if (hp == 0) {
	/*
	**	ran out of file descriptors
	*/
	if (errno == EMFILE) {
	    perror(function_name);
	    return(-1);
	}
	/*
	**	use the host name as an internet address
	*/
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(hnamebuf);
	if (sin.sin_addr.s_addr == -1) {
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "%s: %s: Unknown host\n")),
		    function_name, hnamebuf);
	    return(-1);
	}
	*ahost = hnamebuf;
    }
    else {
	*ahost = hp->h_name;
	sin.sin_family = hp->h_addrtype;
	(void) memcpy((caddr_t)&sin.sin_addr, hp->h_addr, hp->h_length);
    }
    sin.sin_port = rport;
    if (entry == REXEC_ENTRY) {
	ruserpass(hnamebuf, &luser_ruser, &ruser_pw);
	if (luser_ruser == 0) {
		char *myname = (char *)getlogin();
		luser_ruser = (char *)malloc(16);
		fprintf(stdout, "Name (%s:%s): ", *ahost, myname);
		fflush(stdout);
		/*
		**      read from stderr, in case stdin redirected!
		**      (so why don't we write to stderr, in case
		**	 stdout is redirected?  dunno ...)
		*/
		if (read(2, luser_ruser, 16) <= 0)
			exit(1);
		if ((luser_ruser)[0] == '\n')
			luser_ruser = myname;
		else
			if (strchr(luser_ruser, '\n'))
			        *strchr(luser_ruser, '\n') = 0;
	}
	if (luser_ruser && ruser_pw == 0) {
		char prompt[BUFSIZ];
		sprintf(prompt, "Password (%s:%s): ", *ahost, luser_ruser);
		ruser_pw = (char *)getlongpass(prompt);
	}
    }

retry:
    switch (entry) {
	case RCMD_ENTRY:
	    /*
	    **	rcmd: get our (privileged) primary port 
	    */
	    s = rresvport(&lport);
	    if (s < 0)
		return(-1);
	    break;

	case REXEC_ENTRY:
	    s = socket(AF_INET, SOCK_STREAM, 0);
	    if (s < 0) {
		perror("rexec: socket");
		return(-1);
	    }
	    if (setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on)) < 0)
		perror("setsockopt SO_KEEPALIVE");
	    break;
    }
	
    /*
    **	attempt to connect to the listening socket on the remote host
    */
    if (connect(s, (caddr_t)&sin, sizeof (sin)) < 0) {
	/*
	**	local address is in use, try another one 
	*/
	if (entry == RCMD_ENTRY && errno == EADDRINUSE) {
	    (void) close(s);
	    lport--;
	    goto retry;
	}
	/*
	**	forcibly refused by the remote (may be due to some
	**	resource problems) -- do a Binary Exponential Backoff
	**	and try again later; will quit trying after 31 seconds
	*/
	if (errno == ECONNREFUSED && timo <= 16) {
	    (void) close(s);
	    sleep(timo);
	    timo *= 2;
	    goto retry;
	}
	/*
        **	If all attempts fail, try another address for this host.
        */
	if ((hp != (struct hostent *)NULL)
	    && (hp->h_addr_list[1] != (char *)NULL)) {
		int oerrno = errno;

		fprintf(stderr, "connect to address %s: ", 
			 inet_ntoa(sin.sin_addr));
		errno = oerrno;
		perror((char *)NULL);
		hp->h_addr_list++;
		(void) memcpy((caddr_t)&sin.sin_addr, hp->h_addr_list[0],
			      hp->h_length);
		fprintf(stderr, "Trying %s ...\n", inet_ntoa(sin.sin_addr));
		(void) close(s);
		timo = 1;
		lport = IPPORT_RESERVED - 1;
		goto retry;
	}
	sprintf(errbuf,(catgets(nlmsg_fd,NL_SETN,2, "%s: connect: %s")),
		 function_name, *ahost);
	perror(errbuf);
	return (-1);
    }
    /*
    **	lport is irrelevant to rexec
    */
    lport--;

    /*
    **	set up signal handler, saving old handler to reinstate
    **	REMEMBER not to return() without reinstating handler!!
    **	to return() after this point, use "goto bad" 
    */
    vec.sv_handler = catch_pipe;
    vec.sv_mask = vec.sv_onstack = 0;
    (void) sigvector(SIGPIPE, &vec, &sig);

    if (fd2p == 0) {
	/*
	**	we don't need a secondary port, so write a null byte
	**	to the remote process (remshd) to tell him to go on.
	*/
	(void) write(s, "", 1);
	lport = 0;
    }
    else {
	char num[8];
	int sin2len;

	switch (entry) {
	    case RCMD_ENTRY:
		/*
		**	establish a secondary socket;
		**	first, get our secondary (for rcmd, reserved) port
		*/
		s2 = rresvport(&lport);
		if (s2 < 0) {
		    goto bad;
		}
		break;
	    
	    case REXEC_ENTRY:
		s2 = socket(AF_INET, SOCK_STREAM, 0);
		if (s2 < 0) {
			perror("rexec: Secondary socket");
			goto bad;
		}
		break;
	}

	/*
	**	listen for the incoming connection from the remote
	*/
	listen(s2, 1);

	/*
	**	advertise the port we are now listening on
	**	to the remote process (remshd or rexecd);
	**	it will now try to connect()
	**	back to us to complete the socket pair 
	*/
	if (entry == REXEC_ENTRY) {
	    sin2len = sizeof (sin2);
	    if (getsockname(s2, (char *)&sin2, &sin2len) < 0 ||
		    sin2len != sizeof (sin2)) {
		perror("rexec: getsockname");
		(void) close(s2);
		goto bad;
	    }
	    lport = sin2.sin_port;
	}
	(void) sprintf(num, "%d", lport);
	if (write(s, num, strlen(num) + 1) != strlen(num) + 1) {
	    sprintf(errbuf, catgets(nlmsg_fd,NL_SETN,3, "%s: write: Setting up stderr"),
		    function_name);
	    perror(errbuf);
	    (void) close(s2);
	    goto bad;
	}

	/*
	**	CHANGES MADE TO INCREASE RELIABILITY OF rcmd()/rexec():
	**	increased timeout to a more reasonable figure (5 mins);
	**	added select on primary socket so we can tell if the
	**	remote host went down or could not establish the secondary;
	**	in those cases, the primary will be selected for reading,
	**	and we'll see the EOF or some kind of error
	*/
	while (!selected) {
	    /*
	    **	select on both primary and secondary sockets 
	    */
	    FD_ZERO(&mask);
	    FD_SET(s, &mask);
	    FD_SET(s2, &mask);
	    timeout.tv_sec =  (long)600;
	    timeout.tv_usec = (long) 0;
	    if ((sel = select(FD_SETSIZE, &mask, 0, 0, &timeout)) < 0) {
		if (errno == EINTR) {
		    continue;  
		}
		else {
		    sprintf(errbuf, catgets(nlmsg_fd,NL_SETN,5, "%s: select"),
			    function_name);
		    perror(errbuf);
		    (void) close(s2);
		    goto bad;
		}
	    }
	    else if (sel == 0) {
		fprintf(stderr, catgets(nlmsg_fd,NL_SETN,4, "%s: Connection timeout\n"),
			function_name);
		(void) close(s2);
		goto bad;
	    }
	    else if (FD_ISSET(s2, &mask)) {
		/*
		**	here we've gotten *something* 
		**
		**	listen() socket has activity!  the GOOD_THING
		*/
		int len = sizeof (from);
		u_long tUnslept, alarm();
		struct sigvec vec, ovec;
		/*
		**	accept can block forever, if there's not
		**	enough memory for the connection!  Use a
		**	timeout to get us out of this condition.
		*/
		vec.sv_handler = catch_alarm;
		vec.sv_mask = vec.sv_onstack = 0;
		/*
		**	replace the user's SIGALRM handler with
		**	one of our own; make sure to remember how
		**	much time was left on his timer, too!
		*/
		tUnslept = alarm((u_long) 30);
		(void) sigvector(SIGALRM, &vec, &ovec);
		(void) alarm(30);
		/*
		**	try to accept the connection
		*/
		s3 = accept(s2, &from, &len, 0);
		(void) close(s2);
		/*
		**	now reinstate the signal handler, and
		**	reset it to how long we had left to sleep
		*/
		(void) sigvector(SIGALRM, &ovec, &vec);
		(void) alarm(tUnslept);
		/*
		**	could use NBIO on socket instead, so
		**	accept() will return immediately 
		*/
		if (s3 < 0) {
		    /*
		    **	oh no!  the accept call failed.
		    **	inform user, clean up, and go home
		    */
		    sprintf(errbuf, catgets(nlmsg_fd,NL_SETN,6, "%s: accept"),
			    function_name);
		    perror(errbuf);
		    lport = 0;
		    goto bad;
		}
		/*
		**	break out of the while (!selected) loop 
		*/
		selected = 1;
	    }
	    else if (FD_ISSET(s, &mask)) {
		int data, cc, on=1;
		/*
		**	oh no! primary socket activity
		**	put socket into non-blocking mode
		**	so we don't block when we try to recv()
		*/
		(void) ioctl(s, FIOSNBIO, &on);
		/*
		**	now try a PEEKing recv;
		**	see what's ready to be read.
		*/
		if ((data=recv(s, &c, 1, MSG_PEEK)) == 0) {
		    /*	EOF!  we will go home now */
		    fprintf(stderr, catgets(nlmsg_fd,NL_SETN,7, "%s: primary connection shut down\n"),
			    function_name);
		}
		else if (data < 0) {
		    /* some recv error */
		    sprintf(errbuf, catgets(nlmsg_fd,NL_SETN,8, "%s: recv"), function_name);
		    perror(errbuf);
		}
		else {
		    /* (data > 0) : the peer could not set */
		    /* up the secondary socket, so it sent */
		    /* us an error message.                */
		    while (read(s, &c, 1) == 1)
			write(2, &c, 1);
		}
		/*
		**	If anything happened on the primary,
		**	the secondary will never be set up,
		**	so we should return an error.
		*/
		(void) close(s2);
		goto bad;
	    }
	    else {
		/*
		** select returned with neither descriptor selected
		*/
		sprintf(errbuf, catgets(nlmsg_fd,NL_SETN,9, "%s: select"), function_name);
		perror(errbuf);
		/*
		**	now all we can do is die 
		*/
		(void) close(s2);
		goto bad;
	    }
	}
	/*
	**	if we got here then we successfully accepted the new
	**	connection on socket s3; continue with the protocol
	*/
	*fd2p = s3;
	if (entry = RCMD_ENTRY) {
	    from.sin_port = ntohs((u_short)from.sin_port);
	    if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "rcmd: socket: Protocol failure in circuit setup\n")));
		goto bad;
	    }
	}
    }
    /*
    **	Common code again, whether or not we have set up the
    **	secondary connection.  If we are rcmd, the protocol requires
    **	that we send our local user name, remote user name, and
    **	"command" to the remote side.  If we are rexec, we are sending
    **	a remote user name and a password.
    */
    (void) write(s, luser_ruser, strlen(luser_ruser)+1);
    (void) write(s, ruser_pw, strlen(ruser_pw)+1);
    (void) write(s, cmd, strlen(cmd)+1);
    /*
    **	now we need to receive the ACKnowledgement from the remote
    **	that he has read the data OK -- do a blocking recv() for it.
    */
    if (read(s, &c, 1) != 1) {
	fprintf(stderr, catgets(nlmsg_fd,NL_SETN,11, "%s: Lost connection\n"), function_name);
	goto bad;
    }
    /*
    **	if the ACK is not a NULL byte, then there was some sort of
    **	error -- print the error message (everything up to EOF),
    **	and go home. 
    */
    if (c != 0) {
	while (read(s, &c, 1) == 1) {
	    (void) write(2, &c, 1);
	    if (c == '\n')
		break;
	}
	goto bad;
    }
    /*
    **	reset the SIGPIPE signal handler to what the user had before
    **	and return the primary socket; secondary socket is value-
    **	result parameter, if desired.
    */
    (void) sigvector(SIGPIPE, &sig, (struct sigvec *)0);
    return (s);

bad:	/* NOTREACHED */	/* except by goto's, of course */
    if (lport)
	    (void) close(*fd2p);
    /*
    **	close the primary socket, reset the SIGPIPE handler,
    **	and return -1, telling the user that he loses 
    */
    (void) close(s);
    (void) sigvector(SIGPIPE, &sig, (struct sigvec *)0);
    return (-1);
}



/*
**	get a reserved TCP socket (SOCK_STREAM) in the Internet domain (AF_INET)
**	and return it to the caller
*/

#ifdef _NAMESPACE_CLEAN
#undef rresvport
#pragma _HP_SECONDARY_DEF _rresvport rresvport
#define rresvport _rresvport
#endif _NAMESPACE_CLEAN

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s, on = 1;

#ifdef NLS
	nlmsg_fd = catopen("rcmd",0);
#endif NLS
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (s < 0)
	{
		perror((catgets(nlmsg_fd,NL_SETN,12, "rresvport: socket")));
		return (-1);
	}
	/*
	**	set up keepalives; this is not the default
	*/
	if (setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on)) < 0)
	    perror((catgets(nlmsg_fd,NL_SETN,13, "setsockopt SO_KEEPALIVE")));
#ifdef	DEBUG
	if (setsockopt(s,SOL_SOCKET,SO_DEBUG,&on,sizeof(on)) < 0)
	    perror("setsockopt SO_DEBUG");
#endif	DEBUG
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin), 0) >= 0)
			return (s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) 
		{
			/*
			**	NOTE:	we should close(s) before we return 
			**	this is chewing up an fd we should not be!
			*/
			perror((catgets(nlmsg_fd,NL_SETN,14, "rresvport: bind")));
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			/*
			**	NOTE:	we should close(s) before we return 
			**	this is chewing up an fd we should not be!
			*/
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,15, "rresvport: socket: All ports in use\n")));
			return (-1);
		}
	}
}




/*
**	catch_pipe()	--	handler for SIGPIPE signal 
**		NOTE:	This signal handler used by both rcmd and rexec
*/
static void
catch_pipe()
{
    char c;
    /*
    **	used to flush data on sockets after we get a SIGPIPE from
    **	trying to write to [primary] socket.
    */
    if (s3) {		/* stderr connection has been set up */
	while (read(s3, &c, 1) == 1) {
	    /*
	    **	transfer data from stderr socket
	    */
	    (void) write(2, &c, 1);
	    if (c == '\n')
		break;
	    }
	(void) close(s3);
    }
    while (read(s, &c, 1) == 1) {
	(void) write(2, &c, 1);
	if (c == '\n')
	    break;
	}
    (void) close(s);
}


/*
**	catch_alarm()	--	handler for the SIGALRM signal
*/
static void
catch_alarm()
{
    /*
    **	we don't need to do anything in here, really, just abort the
    **	syscall which is blocking (see accept() above)
    */
}

