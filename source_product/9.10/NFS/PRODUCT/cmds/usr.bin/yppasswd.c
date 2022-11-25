#ifndef lint
static  char rcsid[] = "@(#)yppasswd:	$Revision: 1.26.109.2 $	$Date: 92/03/10 13:16:30 $  ";
#endif
/* NFSSRC yppasswd.c	2.1 86/04/17 */
/*"yppasswd.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpcsvc/yppasswd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <errno.h>
#include <stdlib.h>
#define WEEK (24L * 7 * 60 * 60)

char *index();
struct yppasswd *getyppw();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)	
	char **argv;
{
	int ans, port, ok;
	char domain[256];
	char *master;
	struct yppasswd *yppasswd;
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("yppasswd",0);
#endif NLS

	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "yppasswd:  getdomainname system call failed\n")));
		exit(1);
	}
	if (yp_master(domain, "passwd.byname", &master) != 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "yppasswd:  can't get the master server from the NIS passwd map\n")));
		exit(1);
	}
	port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
		IPPROTO_UDP);
	if (port == 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "yppasswd:  the NIS passwd map master server, %s, is not\n           running the yppasswd daemon.  Cannot change the NIS passwd.\n")), master);
		exit(1);
	}
	if (port >= IPPORT_RESERVED) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "yppasswd:  the yppasswd daemon is not running on a privileged port\n")));
		exit(1);
	}
	yppasswd = getyppw(argc, argv);
	ans = callrpc(master, YPPASSWDPROG, YPPASSWDVERS,
	    YPPASSWDPROC_UPDATE, xdr_yppasswd, yppasswd, xdr_int, &ok);
	if (ans != 0) {
		clnt_perrno(ans);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "\nCouldn't change the NIS passwd.\n")));
	}
	else if (ok != 0)
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "\nCouldn't change the NIS passwd.\n")));
	else
		printf((catgets(nlmsg_fd,NL_SETN,7, "The NIS passwd has been changed on %s, the master NIS passwd server.\n")), master);
	free(master);
        exit(0);
}

struct	passwd *pwd;
struct	passwd *getpwnam();
char	*strcpy();
char	*crypt();
char	*getpass();
char	*getlogin();
char	*pw;
char	pwbuf[10];
char	pwbuf1[10];
extern	int errno;
time_t  when;
time_t  now;
time_t  maxweeks;
time_t  minweeks;
long    a64l();
char    *l64a();
long    time();

struct yppasswd *
getyppw(argc, argv)
	char *argv[];
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c, pwlen;
	char *uname;
	static struct yppasswd yppasswd;
	struct  passwd *pwd;

	insist = 0;
	uname = NULL;
	if (argc > 1)
		uname = argv[1];
	if (uname == NULL) {
		/* change the caller's passwd */
		if ((uname = getlogin()) == NULL) { /* this is strange */
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "yppasswd:  you don't have a login name in /etc/utmp.\n           Try entering \"yppasswd user_name\".\n")));
			exit(1);
		}
		printf((catgets(nlmsg_fd,NL_SETN,9, "Changing NIS password for %s...\n")), uname);
	}

	/* the following sequential search is slow. replaced with
	 * hashed search - prabha
	 */
	/*
	   while (((pwd = getpwent()) != NULL) && strcmp(pwd->pw_name, uname))
		;
	*/
	pwd = getpwnam(uname);	/* get the matching pw entry */
		
	u = getuid();
	if (pwd == NULL) {
		printf((catgets(nlmsg_fd,NL_SETN,10, "Not in passwd file.\n")));
		exit(1);
	}
	if (u != 0 && u != pwd->pw_uid) {
		printf((catgets(nlmsg_fd,NL_SETN,11, "Permission denied.\n")));
		exit(1);
	}
/*	The password aging checks for the special cases starts here */
/*    These checks are here since the deamon runs as root and will override */
/*    them if placed in rpc.yppasswdd                                       */
if (pwd->pw_age != NULL)
	   {
	   when = (long) a64l(pwd->pw_age);
	   maxweeks = when & 077;
	   minweeks = (when >> 6) & 077;
	   when >>= 12;
	   now = time((long *) 0)/WEEK;
if (when <= now) {
     if(u != 0 && (now < when + minweeks)){
	  fprintf(stderr,"sorry: < %ld weeks since last change\n",minweeks);
	  exit(1);}
     if (minweeks > maxweeks && u != 0){
	 fprintf(stderr,"you may not change this password\n");
	  exit(1);}
	  }
	}
	endpwent();
	/* getpass actually returns up to 50 chars */
	strncpy(pwbuf1, getpass((catgets(nlmsg_fd,NL_SETN,12, "Old NIS password:"))), sizeof(pwbuf1));
	pwbuf1[sizeof(pwbuf1)-1] = 0;
tryagain:
	strcpy(pwbuf, getpass((catgets(nlmsg_fd,NL_SETN,13, "New password:"))));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		printf((catgets(nlmsg_fd,NL_SETN,14, "Password unchanged.\n")));
		exit(1);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		if (flags == 1)
		   printf((catgets(nlmsg_fd,NL_SETN,15, "Please use at least one non-numeric character.\n")));
		else
		   printf((catgets(nlmsg_fd,NL_SETN,16, "Please use a longer password.\n")));
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass((catgets(nlmsg_fd,NL_SETN,17, "Retype new password:")))) != 0) {
		printf((catgets(nlmsg_fd,NL_SETN,18, "Mismatch - password unchanged.\n")));
		exit(1);
	}
	time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	pw = crypt(pwbuf, saltc);
	yppasswd.oldpass = pwbuf1;
	pwd->pw_passwd = pw;
	yppasswd.newpw = *pwd;
	return (&yppasswd);
}
