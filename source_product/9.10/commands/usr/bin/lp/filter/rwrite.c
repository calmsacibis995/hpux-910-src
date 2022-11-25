/* $Revision: 70.1 $ */
/* rwrite -- write to remote user's tty */

#include "../lp.h"
#include <limits.h>

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 22			/* set number */
#include <msgbuf.h>
#endif NLS

#define bzero(a,b)	memset(a,0,b)
#define bcopy(a,b,c)	memcpy(b,a,c)

#define MAXCOMLENGTH	BUFSIZ

#define	UTMP	"/etc/utmp"
#define	BEL	'\07'
#define CMD_WRITE	'\7'

char	work[BUFSIZ];

char	*f_name = NULL;
int	(*f_clean)() = NULL;

extern char **environ;			/* for cleanenv() */

void
main(argc, argv)
int	argc;
char	*argv[];
{
	char	local[SP_MAXHOSTNAMELEN];

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH",0);

#ifdef NLS
	nl_catopen("lp");
#endif NLS

	if(argc != 4){
		fprintf(stderr,
		    "usage : %s user host message\n", argv[0]);
		fflush(stderr);
		exit(1);
	}

	gethostname(local, sizeof(local));

	if(!strcmp(local, argv[2])){
		exit(!wrtmsg(argv[1], "local", argv[3]));
	}else{
		exit(!wrtrem(argv[1], argv[2], argv[3]));
	}
/* NOTREACHED */
}


/* fatal -- prints error message */

void
fatal(message, code)
char	*message;
int	code;
{
	fprintf(stderr, "%s\n", message);
	fflush(stderr);

	if(code != 0)
		exit(code);
}


/*
 * findtty(user) -- find first tty that user is logged in to.
 *	returns: /dev/tty?? if user is logged in
 *		 NULL, if not
 */

char *
findtty(user)
char *user;
{
	struct utmp utmp;
	static char tty[NAMEMAX + 5];
	FILE *u;

	if((u = fopen(UTMP, "r")) == NULL)
		return(NULL);
	while(fread((char *) &utmp, sizeof(struct utmp), 1, u) == 1){
		if(utmp.ut_type == USER_PROCESS){
			if(strncmp(utmp.ut_name, user,PASS_MAX) == 0) {
				sprintf(tty, "/dev/%s", utmp.ut_line);
				fclose(u);
				return(tty);
			}
		}
	}
	fclose(u);
	return(NULL);
}


/*
 * Create a connection to the remote printer server.
 * Most of this code comes from rcmd.c.
 */

getport(rhost)
	char *rhost;
{
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s, lport = IPPORT_RESERVED - 1;
	long	timo = 1;
	int err;

	/*
	 * Get the host address and port number to connect to.
	 */
	hp = gethostbyname(rhost);
	if (hp == NULL){
		sprintf(work,"unknown host %s", rhost);
		fatal(work,1);
	}
	sp = getservbyname("printer", "tcp");
	if (sp == NULL)
		fatal("printer/tcp: unknown service",1);
	bzero((char *)&sin, sizeof(sin));
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = sp->s_port;

	/*
	 * Try connecting to the server.
	 */
retry:
	s = rresvport(&lport);
	if (s < 0)
		return(-1);
	if (connect(s, (caddr_t)&sin, sizeof(sin), 0) < 0) {
		err = errno;
		(void) close(s);
		errno = err;
		if (errno == EADDRINUSE) {
			lport--;
			goto retry;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		return(-1);
	}
	return(s);
}


/*
 *  not_response -- check the remote rlpdaemon supports this function
 *                  and check if the remote user logged in.
 *		    resp:     '\0'  --> logged in
 *                           others --> not logged in
 */   

int
not_response(fd)
int	fd;
{
	char	resp;

	if( read(fd, &resp, 1) != 1 )
		fatal("Lost connection");

	if (resp == '\0')
		return(0);
	else
		return(1);
}


/* sigalrm() -- catch SIGALRM */

static int
sigalrm()
{
}


/*
 *  rresvport
 */

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return(-1);
	for (; *alport > IPPORT_RESERVED/2; (*alport)--) {
		sin.sin_port = htons((u_short) *alport);
		if (bind(s, (caddr_t)&sin, sizeof(sin), 0) >= 0)
			return(s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL)
			break;
	}
	(void) close(s);
	return(-1);
}


/* wrtmsg(user, msg) -- write message to user's tty if logged in.
 *	return codes: TRUE ==> success,
 *		      FALSE ==> failure
 */

wrtmsg(user, from, msg)
char *user;
char *from;
char *msg;
{
	char *tty, *findtty();
	FILE *f;
	int sigalrm();

	if((tty = findtty(user)) == NULL || (f = fopen(tty, "w")) == NULL)
		return(FALSE);
	signal(SIGALRM, sigalrm);
	alarm(10);
	fputc(BEL, f);
	fflush(f);
	sleep(2);
	fputc(BEL, f);
	fprintf(f, (nl_msg(1, "\nlp@%s: %s\n")), from, msg);
	alarm(0);
	fclose(f);
	return(TRUE);
}


/*
 *   wrtrem -- write message to remote spool daemon
 *		return codes:	 TRUE ==> success,
 *				FALSE ==> failure
 */

int
wrtrem(user, host, message)
char	*user;
char	*host;
char	*message;
{
	int	s, status;
	char	command[MAXCOMLENGTH];
	int	sigalrm();

	if ( (s = getport(host)) < 0 )
		fatal("can't get port", 1);

	sprintf(command, "%c%s %s\n", CMD_WRITE, user, message);

	if(write(s, command, MAXCOMLENGTH) != MAXCOMLENGTH){
		if(errno == EINTR){
			fprintf(stderr, "wrtremote : interupted\n");
			fflush(stderr);
			return(FALSE);
		}else{
			fprintf(stderr,
			    "wrtremote : Connection aborted on error\n");
			fflush(stderr);
			return(FALSE);
		}
	}

	if(not_response(s))
	    return(FALSE);

	shutdown(s, 1);
	close(s);
	return(TRUE);
}
