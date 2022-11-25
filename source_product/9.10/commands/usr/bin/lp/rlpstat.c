/* $Revision: 70.1 $ */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 19	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS
/*
 * rlpstat - Remote spool queue examination program
 *
 * rlpstat [-dprinter] [-uuser] [ids]
 *
 */

#include "lp.h"

/*
 * Stuff for handling job specifications
 */

char	host[SP_MAXHOSTNAMELEN];
char	luser[16];		/* buffer for person */
char	work[BUFSIZ];

char	*RM;
char	*RP;
char	*from;
char	*name;			/* name of program */
char	*person;		/* name of person doing rcancel */
char	*printer;
char	*user[MAXUSERS];	/* users to process */

int	all = 0;		/* eliminate all files (root only) */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests=0;		/* # of spool requests */
int	seqno;
int	users=0;		/* # of users in user array */

struct	outq	o;
struct	pstat	p;
struct	qstat	q;

extern char **environ;	/* for cleanenv() */

main(argc, argv)
int	argc;
char	*argv[];
{
	char	*arg;
	char	dest[DESTMAX];
	char	destbuf[DESTMAX];

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);

#ifdef NLS
	nlmsg_fd = catopen("lp");
#endif

	name = argv[0];
	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,1, "spool directory non-existent")), 1);
	gethostname(host, sizeof(host));
	from = host;
	
/*
	person	= getname();
	if (strlen(person) >= sizeof(luser))
		fatal("Your name is too long",1);
	strcpy(luser, person);
	person = luser;
*/
	while (--argc) {
		if ((arg = *++argv)[0] == '-')
			switch (arg[1]) {
			case 'd':
				if (arg[2])
					printer = &arg[2];
				else if (argc > 1) {
					argc--;
					printer = *++argv;
				}
				break;
			case 'u':
				if (users >= 0){
					if (users >= MAXUSERS)
						fatal((catgets(nlmsg_fd,NL_SETN,2, "Too many users")),1);
					user[users++] = &arg[2];
				}
				break;
			default:
				usage();
			}
		else {
			if (isrequest(arg,dest,&seqno)){
				if (requests >= MAXREQUESTS)
					fatal((catgets(nlmsg_fd,NL_SETN,3, "Too many requests")),1);
				requ[requests++] = seqno;
				if (printer == NULL){
					strcpy(destbuf,dest);
					printer = destbuf;
				}
			} else {
				usage();
			}
		}
	}
	if (printer == NULL){
		fatal((catgets(nlmsg_fd,NL_SETN,4, "rlpstat: No printer was specified")),1);
	}

	displayq(1,TRUE);
/* NOTREACHED */
}

static
usage()
{
	printf((catgets(nlmsg_fd,NL_SETN,5, "usage: rlpstat [-dprinter] [-uuser] [ids]\n")));
	exit(2);
}

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
