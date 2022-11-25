/* $Revision: 70.1 $ */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 18	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS
/*
 * rcancel - remove the current user's spool entry
 *
 * rcancel [-a] [-dprinter] [-uuser] [ids]
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


struct	pstat	p;
struct	outq	o;

extern char **environ;	/* for cleanenv() */

main(argc, argv)
int	argc;
char	*argv[];
{
	char	*arg;

	char	dest[DESTMAX + 1];
	char	destbuf[DESTMAX + 1];
	char	RM_buf[SP_MAXHOSTNAMELEN];
	char	RP_buf[DESTMAX+1];

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);


#ifdef NLS
	nlmsg_fd = catopen("lp");
#endif
	name = argv[0];
	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,1, "spool directory non-existent")), 1);
	gethostname(host, sizeof(host));
	from = host;
	
	person	= getname();
	if (strlen(person) >= sizeof(luser))
		fatal((catgets(nlmsg_fd,NL_SETN,2, "Your name is too long")),1);
	strcpy(luser, person);
	person = luser;
 	if (!strcmp(person, "-all")) {
		fatal((catgets(nlmsg_fd,NL_SETN,3, "The login name \"-all\" is reserved")),1);
 	}
	while (--argc) {
		if ((arg = *++argv)[0] == '-')
			switch (arg[1]) {
			case 'a':
				if (users >= 0){
					if (users >= MAXUSERS)
						fatal((catgets(nlmsg_fd,NL_SETN,4, "Too many users")),1);
					user[users++] = person;
				}
				break;
			case 'e':
				users = -1;
				break;
			case 'u':
				if (users >= 0){
					if (users >= MAXUSERS)
						fatal((catgets(nlmsg_fd,NL_SETN,4, "Too many users")),1);
					user[users++] = arg;
				}
				break;
			default:
				usage();
			}
		else {
			if (isrequest(arg,dest,&seqno)){
				if (requests >= MAXREQUESTS)
					fatal((catgets(nlmsg_fd,NL_SETN,6, "Too many requests")),1);
				requ[requests++] = seqno;
				if (printer == NULL){
					strncpy(destbuf, dest, DESTMAX+1);
					destbuf[DESTMAX] = '\0';
					printer = destbuf;
				}
			} else {
				if (isdest(arg)){
					strncpy(destbuf, arg, DESTMAX+1);
					destbuf[DESTMAX] = '\0';
					printer = destbuf;
				}else{
					usage();
				}
			}
		}
	}

	if ((printer == NULL) || (getpdest(&p,printer) == EOF)){
		fatal((catgets(nlmsg_fd,NL_SETN,7, "rcancel: No printer was specified")),1);
	}
	endpent();
	RM = RM_buf;
	RP = RP_buf;
	strncpy (RM, p.p_remotedest, SP_MAXHOSTNAMELEN);
	strncpy (RP, p.p_remoteprinter, DESTMAX + 1);

	if (users < 0) {
		if (((int) getuid()) == 0)
			all = 1;	/* all files in local queue */
		else {
			user[0] = person;
			users = 1;
		}
	}

	chkremote(TRUE);
/* NOTREACHED */
}

static
usage()
{
	printf((catgets(nlmsg_fd,NL_SETN,8, "usage: rcancel [-a] [printer] [-uuser] [ids]\n")));
	exit(2);
}
cleanup()
{}
