#ifndef lint
static  char rcsid[] = "@(#)rwall:	$Revision: 1.27.109.1 $	$Date: 91/11/19 14:12:08 $  ";
#endif
/* rwall.c	2.1 86/04/16 NFSSRC */
/*static  char sccsid[] = "rwall.c 1.1 86/02/05 Copyr 1984 Sun Micro";*/
/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <rpcsvc/rwall.h>
/*	HPNFS
**	add TRACE() macros
*/
#include <arpa/trace.h>

# define	TRACEFILE	"/tmp/rwall.trace"
/*
**	the name of the UTMP file, instead of hardcoding twice ...
*/
# define	UTMP	"/etc/utmp"

#define	USERS	128
char who[9] = "???";
char *path;
/* flag to indicate if 1 or more hostnames or netgroups is invalid */
int  err_found;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
	char **argv;
{
	int msize;
	char buf[BUFSIZ];
	register i;
	struct	utmp utmp[USERS];
	FILE *f;
	int sline;
	char	hostname[256];
	int hflag;
	char *machine, *user, *domain;
	
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rwall",0);
#endif NLS
	STARTTRACE(TRACEFILE);
	if (argc < 2)
		usage();
	gethostname(hostname, sizeof (hostname));
	TRACE2("main hostname = %s", hostname);

	if((f = fopen(UTMP, "r")) == NULL) {
		TRACE3("main cannot open %s, errno = %d", UTMP, errno);
		perror(UTMP);
		exit(1);
	}
	sline = ttyslot(2); /* 'utmp' slot no. of sender */
	TRACE2("main sline =%d, about to read UTMP", sline);
	(void) fread((char *)utmp, sizeof(struct utmp), USERS, f);
	TRACE("maindone with entire UTMP file read");
	(void) fclose(f);
	if (sline)
		strncpy(who, utmp[sline].ut_name, sizeof(utmp[sline].ut_name));

	TRACE2("main who = %s", who);
	sprintf(buf, (catgets(nlmsg_fd,NL_SETN,1, "broadcast message from %s!%s:  ")), hostname, who);
	TRACE2("main buf = %s", buf);
	msize = strlen(buf);
	while((i = getchar()) != EOF) {
		if (msize >= sizeof buf) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "Message too long\n")));
			exit(1);
		}
		buf[msize++] = i;
	}
	path = buf;
	TRACE2("main message = %s\n", path);
        
        /* initialize hostname/netgroupname error flag */
        err_found = FALSE;

	hflag = 1;
	while(argc > 1) {
		if (argv[1][0] == '-') {
			switch (argv[1][1]) {
				case 'h':
					hflag = 1;
					break;
				case 'n':
					hflag = 0;
					break;
				default:
					usage();
					break;
			}
		}
		else if (hflag)
			doit(argv[1]);
		else {
			setnetgrent(argv[1]);
                        /* check to see if group exists */
                        if (! innetgr(argv[1],NULL,NULL,NULL)) {
                          err_found = TRUE;
                	  fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "%s is unknown netgroup\n")), argv[1]);
			}
                        else {
       			  while (getnetgrent(&machine, &user, &domain)) {
			       if (machine) {
                                 if (legal_host(machine))
            				doit(machine);
			        }
        	           } /* while */
		         }  /* else */
			endnetgrent();
		 } 
	  argc--;
	  argv++;
         }
    if (err_found) 
      exit(1);

    exit(0);
}
/*
* check to see that the first character of the hostname returned by 
* getnetgrent is a valid starting character for a host name. If not,
* the host field matches no hosts and hence no message is sent 
* Bug fix 1/4/87 mjk 
*/
legal_host(machine)
     char *machine;
{
  if ( machine[0] >= 'a' && machine[0] <= 'z' || machine[0] >= 'A' && 
       machine[0] <= 'Z' ) 
            return(TRUE);
  else 
   if ( machine[0] >= '0' && machine[0] <= '9' ) 
             return(TRUE);
    else 
     if ( machine[0] == '_' )
              return(TRUE);
      else 
        return(FALSE);
}


/*
 * Clnt_call to a host that is down has a very long timeout
 * waiting for the portmapper, so we use rmtcall instead.   Since pertry
 * value of rmtcall is 6 secs, make timeout here 14 secs so that
 * you get 2 tries (HPNFS, this used to have a timeout of 8 sec because the
 * rmtcall timeout used to be 3 seconds.  This was changed to fix DTS 
 * CNOdm00724).
 */
doit(host)
	char *host;
{
	struct sockaddr_in server_addr;
	struct hostent *hp;
	int socket, port;
	struct timeval timeout;
	enum clnt_stat stat;
	CLIENT *client;

	TRACE2("doit SOP, host = %s", host);
	socket = RPC_ANYSOCK;
	if ((hp = gethostbyname(host)) == NULL) {
		TRACE("doit unknown host name");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "%s is unknown host\n")), host);
                err_found = TRUE;
		return;
	}
next_ipaddr:
	timeout.tv_usec = 0;
	timeout.tv_sec = 14;
	memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	TRACE("doit about to pmap_rmtcall(WALLPROG)");
	stat = pmap_rmtcall(&server_addr, WALLPROG, WALLVERS, WALLPROC_WALL,
	    xdr_wrapstring, &path,  xdr_void, NULL, timeout, &port);

	if (stat != RPC_SUCCESS) {
	   if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
	       fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "rwall: can't reach %1$s at address %2$s\n")), host, inet_ntoa(*(u_long *)hp->h_addr));
	       if (hp && hp->h_addr_list[1]) {
		   hp->h_addr_list++;
		   fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "rwall: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
		   goto next_ipaddr;
		}
	   }
		TRACE("doit pmap_rmtcall failed!");
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "Couldn't contact %s: ")), host);
		clnt_perrno(stat);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "\n")));
	} else {
		TRACE("doit pmap_rmtcall returned RPC_SUCCESS");
	}
}

usage()
{
	fprintf(stderr,
	    (catgets(nlmsg_fd,NL_SETN,6, "Usage: rwall host .... [-n netgroup ....] [-h host ...]\n")));
	exit(1);
}
