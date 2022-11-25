/* $Revision: 70.1 $ */

extern	int	errno;

/*
 * rlpdaemon -- remote line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 */

#include "lp.h"
#include <unistd.h>
#ifdef TRUX
#include <sys/security.h>
#endif

extern	char *strchr();
extern	char *strncpy();

char	*RM;		/* remote machine name */
char	*RP;		/* remote printer name */
char	*name;
char	*printer;
char	*from;

int	all = 0;		/* eliminate all files (root only) */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

char	fromb[SP_MAXHOSTNAMELEN]; /* buffer for client's machine name */
char	cbuf[BUFSIZ];	/* command line buffer */

char	host[SP_MAXHOSTNAMELEN]; /* name of host machine */
char	work[BUFSIZ];	/* working buffer */

struct	outq	o;
struct	pstat	p;
struct	qstat	q;

int	lflag;		/* log requests flag */
int	ls;		/* listen socket descriptor */
int	inetd_flag;	/* started from inetd */
int	s;		/* connected socket descriptor */

char	*logfile = DEFLOGF;

struct	hostent	*hp;	/* pointer to host info for remote system */
struct	servent	*sp;	/* pointer to service information */

struct	sockaddr_in	myaddr_in;	/* for local socket address */
struct	sockaddr_in	peeraddr_in;	/* for peer socket address */

static char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob",
	"request alt",
	"get message"
};

int	cleanup();

extern char **environ;	/* for cleanenv() */

main(argc, argv)
int	argc;
char	*argv[];
{
	int options=0;

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);

	name		= argv[0];
	lflag		= 0;
	inetd_flag	= 0;

	while (--argc > 0) {
		argv++;
		if (argv[0][0] == '-')
		switch (argv[0][1]) {
		        case 'd':
			        options |= SO_DEBUG;
				break;
			case 'l':
				lflag++;
				break;
			case 'L':
				argc--;
				logfile = *++argv;
				break;
			case 'i':
				inetd_flag++;
				break;
			default:
				log("Unknown option:  %s",argv[0]);
				exit(1);
			}
	}

	gethostname(host, sizeof(host));

	if(chdir(SPOOL) == -1)
		fatal("spool directory non-existent", 1);

	if (inetd_flag == 0){	/* if inetd_flag is zero, rlpdaemon */
		no_inetd();	/* was not started from inetd. */
	}else{
		doit();
	}
/* NOTREACHED */
}

no_inetd()
{
	int	addrlen;
	int	dummy;
	int	f;
	int	index1;
	int	lfd;
	char	line[50];

/*	Allow parent shell to continue.
	Ensure the process is not a process group leader.
*/
	if (fork() != 0){
		exit(0);	/* parent exits */
	}

/*				   child is left */

/*	Disassociate from the controlling terminal and process group */

	setpgrp();	/* lose controlling terminal and */
			/* change process group */

	signal(SIGHUP, SIG_IGN);	/* immune from pgrp */
					/* leader death */
	if (fork() != 0){
		exit(0);	/* first child exits */
	}

/*				   second child is left
				   from this point, do not do a
				   setpgrp() or you will become a
				   process group leader.  This might
				   cause you to aquire a controlling
				   tty sometime in the future */
/*
	Change stdin to point  to /dev/null.
	Change stdout to point to /dev/null,
	Change stderr to point to the log file.
*/

	for (index1 = 0; index1 < 3; index1++){
		(void) close(index1);
	}

	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	if (lflag == 0){
		(void) open("/dev/null", O_WRONLY);
	}else{
		(void) open(logfile, O_WRONLY|O_APPEND|O_CREAT);
	}
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL, logfile, 0644, ADMIN,
			"open of rlpdaemon logfile");
	else
		chmod(logfile,0644);
#else
	chmod(logfile,0644);
#endif

	(void) umask(0);	/* clear any inherited file mode */
				/* creation mask */

	lfd = open(MASTERLOCK, O_WRONLY|O_CREAT, 0644);

	if (lfd < 0) {
		log("cannot create %s", MASTERLOCK);
		exit(1);
	}
	if (lockf(lfd, F_TLOCK,0) < 0) {
		if (errno == EACCES){	/* active deamon present */
			log("active daemon present");
			exit(1);
		}
		sprintf(work,"cannot lock %s", MASTERLOCK);
		log(work);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		log("cannot write rlpdaemon pid");
		exit(1);
	}

/*	Clear out address structures */

	memset ((char *) &myaddr_in,   0 ,sizeof(struct sockaddr_in));
	memset ((char *) &peeraddr_in, 0 ,sizeof(struct sockaddr_in));

/*	Set up address structure for the listen socket */

	myaddr_in.sin_family = AF_INET;

/*	The server will listen on the wildcard address, rather than
	on its own internet address. */

	myaddr_in.sin_addr.s_addr = INADDR_ANY;

/*	Find the information for the server in order to get the
	needed port number */

	sp = getservbyname("printer", "tcp");
	if (sp == NULL) {
		log("(getservbyname) printer/tcp: is a unknown service");
		exit(1);
	}

	myaddr_in.sin_port = sp->s_port;

/*	Create the listen socket */

	ls = socket (AF_INET, SOCK_STREAM, 0);
	if (ls == -1){
		log("(socket) unable to create listen socket");
		exit(1);
	}

/*	Bind the listen address to the socket */

	if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1){
		perror("unable to bind address (internet domain bind)");
		cleanup();
	}

/*
	Initiate the listen on the socket so remote users
	can connect.  The listen backlog is set to 5.
*/

	if (listen(ls, 5) == -1){
		log("unable to listen on socket");
		exit(1);
	}
		
/*
	Set SIGCLD to SIG_IGN, in order to prevent the accumulation
	of zombies as each child terminates.  This means the daemon
	does not have to make wait calls to clean them up.
*/
	signal (SIGCLD, SIG_IGN);

	for(;;){
		addrlen = sizeof(struct sockaddr_in);
		s = accept(ls, &peeraddr_in, &addrlen);
		if (s == -1)
			exit(1);

		switch (fork()){

			case -1:
				close(s);
				exit(1);

			case 0:
				close(0);

				if ((dummy = fcntl(s, F_DUPFD, 0)) != 0){
					log("Could not change stdin to a socket");
					log("The file descriptor returned was %d",dummy);
					exit(1);
				}

				close(1);
				if ((dummy = fcntl(s, F_DUPFD, 1)) != 1){
					log("Could not change stdout to a socket");
					log("The file descriptor returned was %d",dummy);
					exit(1);
				}

				close(s);
				close(ls);
				chkhost(&peeraddr_in);
				doit();
				exit(0);

			default:
				close(s);
		}
	}
}

cleanup()
{
	if (lflag)
		log("cleanup()");
	endpent();
	endoent();
	exit(0);
}

/*
 * Stuff for handling job specifications
 */

int
doit()
{
	char	*cp;
	int	n;
	struct	sockaddr_in addrbuf;
	struct	sockaddr_in *addr;
	int	*addrlen;
	int	addrlenbuf;
	extern char *inet_ntoa();
	char	*strncpy();
	char *requestor;
	char *message;

	addr = &addrbuf;
	addrlen = &addrlenbuf;
	if (inetd_flag != 0){	/* if inetd_flag is zero, rlpdaemon */

		(void) close(2);
		(void) open(logfile, O_WRONLY|O_APPEND|O_CREAT);
#ifdef SecureWare
		if(ISSECURE)
		    lp_change_mode(SPOOL, logfile, 0644, ADMIN,
			"open of rlpdaemon logfile");
		else
		    chmod(logfile,0644);
#else
		chmod(logfile,0644);
#endif
		*addrlen = sizeof (struct sockaddr_in);
		if (getpeername(0,addr,addrlen) == -1){
			fatal("Lost connection",1);
		}
		hp = gethostbyaddr(&addr->sin_addr, sizeof (struct in_addr), addr->sin_family);
		if (hp == 0){
			sprintf(work,"Host name for your address (%s) unknown",
				inet_ntoa(addr->sin_addr));
			fatal(work,1);
		}

		strncpy(fromb, hp->h_name, SP_MAXHOSTNAMELEN);

		/* Make sure you've not got a domain name */
		if (cp=strchr(fromb, '.'))
		    *cp = '\0';
		from = fromb;
	}


/*	Get the command line from the remote machine.
	Replace the '\n' with a '\0'. */

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long",1);
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection",1);
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = cbuf;

/*	Log the request if required */

		if (lflag && *cp >= '\1' && *cp <= '\7') {
			printer = NULL;
			log("%s requests %s %s", from, cmdnames[*cp], cp+1);
		}
		
/*	Determine the type of request and process it. */

		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs */
				/* there.  This is left in, incase */
				/* BSD ever sends a printit command. */
				/* This does nothing. */
			break;
		case '\2':	/* receive files to be queued */
			printer = cp;		/* get the printer */
			recvjob();		/* go get the job */
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;		/* get the printer */
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace((int)(*cp)))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit((int)(*cp))) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests",1);
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users",1);
					user[users++] = cp;
				}
			}

/*				0 is a short request */
/*				1 is a long  request */

			displayq(cbuf[0] - '\3',FALSE);
			exit(0);
		case '\5':	/* remove a job from the queue */
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace((int)(*cp)))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit((int)(*cp))) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests",1);
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users",1);
					user[users++] = cp;
				}
			}
			rmjob();
			break;

		case '\6':
			printer = cp;
			log("%s : get alt request", printer);
			alter();
			break;

		case '\7' :
			/* remote message return */
                        requestor = cp;
			while(*cp && *cp != ' ')
				cp++;
			if(!*cp)
				break;
			*cp++ = '\0';
			message = cp;
			
			if( ! wrtmsg(requestor, from, message) )
			    write(1, "\1", 1);	/* failure */
			else
			    write(1, "", 1);	/* success */

			break;
		default:
			fatal("Illegal service request",1);
		}
	}
}

/*
 * Check to see if the from host has access to the line printer.
 */
static
chkhost(f)
struct sockaddr_in *f;
{
	extern	char	*inet_ntoa();

	f->sin_port = ntohs(f->sin_port);
	if (f->sin_family != AF_INET || f->sin_port >= IPPORT_RESERVED)
		fatal("Malformed from address",1);
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0){
		sprintf(work,"Host name for your address (%s) unknown",
			inet_ntoa(f->sin_addr));
		fatal(work,1);
	}

	strncpy(fromb, hp->h_name, SP_MAXHOSTNAMELEN);
	from = fromb;

	/* ensure access rights are in keeping with hosts.equiv(4) */
	if (ruserok(from, 0, ADMIN, ADMIN) == 0)
	    return;

	fatal("HP-UX.  Your host does not have line printer access",1);

} /* chkhost */

/*
	send the message to the log file.  Note that stderr
	has been opened as the log file.
*/
int
/*VARARGS1*/
log(msg, a1, a2, a3)
char *msg;
{
	short console = isatty(fileno(stderr));

	fprintf(stderr, console ? "\r\n%s: " : "%s: ", name);
	if (printer)
		fprintf(stderr, "%s: ", printer);
	fprintf(stderr, msg, a1, a2, a3);
	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}

