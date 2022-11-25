#ifndef lint
static  char rcsid[] = "@(#)spray:	$Revision: 1.32.109.1 $	$Date: 91/11/19 14:12:18 $  ";
#endif
/* spray.c	2.1 86/04/16 NFSSRC */
/*static  char sccsid[] = "spray.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/
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
#include <ctype.h> 
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <rpcsvc/spray.h>
/*
**	include the TRACE macro definitions
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/spray.trace"

#define DEFBYTES 100000		/* default numbers of bytes to send */
#define MAXPACKETLEN 1514

char *adrtostr();

/* define a pointer to original hostname. If this is an
 * ip addr, we do not address cycle even if using bind. */

char *orig_name = NULL;

char hostname[255], *host = NULL;

int adr;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
int	argc;
char	**argv;
{
	int err, cnt, i, rcved, lnth, atoi();
	int psec, bsec;
	int buf[SPRAYMAX/4];
	struct hostent *hp;
	struct sprayarr arr;	
	struct spraycumul cumul;
	
	STARTTRACE(TRACEFILE);
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("spray",0);
#endif NLS
	if (argc < 2)
		usage();

	cnt = -1;
	lnth = SPRAYOVERHEAD;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			switch(argv[1][1]) {
				case 'c':
					cnt = atoi(argv[2]);
					argc--;
					argv++;
					break;
				case 'l':
					lnth = atoi(argv[2]);
					argc--;
					argv++;
					break;
				default:
					usage();
			}
		}
		else {
			if (host)
				usage();
			else
				host = argv[1];
		}
		argc--;
		argv++;
	}
	TRACE2("main option host = %s", host);

	TRACE("main done parsing arguments");
	if (host == NULL)
		usage();
	orig_name = host; /* copy of hostname/addr string */
	if (isdigit(host[0])) {
		adr = inet_addr(host);
		/** HPNFS
		 **
		 ** exit if inet_addr returns a -1 ("malformed address")
		 ** with an appropriate message.              
		 **
		 **/
		if (adr == -1){
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,17, "spray: malformed address %s\n")), host);
			exit(1);
		}
		host = adrtostr(adr);
		strcpy(hostname,adrtostr(adr));
		host = hostname;
		TRACE3("main translated adr = 0x%x, host = %s", adr, host);
	}

	/* check if there is a host with this name/address */
	if ((hp = gethostbyname(host)) == NULL) {
	  if (isdigit(orig_name[0]))
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "%s is unknown host name\n")), orig_name);
	  else
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "%s is unknown host name\n")), host);
		exit(1);
	} else	{/* The host is in the hosts database. Copy the adrress
		  * from hp->h_addr only if orig_name is not an ip-addr.
		  * Otherwise use the ip-address provided. */

		if (! isdigit(orig_name[0]))
		      adr = *((int *)hp->h_addr);
	}
	TRACE3("main translated adr = 0x%x, host = %s", adr, host);

	if (lnth < SPRAYOVERHEAD)
		lnth = SPRAYOVERHEAD;
	else if (lnth >= SPRAYMAX)
		lnth = SPRAYMAX;
	if (lnth <= MAXPACKETLEN && lnth % 4 != 2)
		lnth = ((lnth+5)/4)*4 - 2;
	if (cnt <= 0)
		cnt = DEFBYTES/lnth;

	arr.lnth = lnth - SPRAYOVERHEAD;
	arr.data = buf;

	TRACE2("main cnt = %d", cnt);
	TRACE2("main lnth = %d", lnth);
	TRACE2("main arr.lnth = %d", arr.lnth);
	/*nl_printf((catgets(nlmsg_fd,NL_SETN,2, "sending %1$d packets of lnth %2$d to %3$s ...")), cnt, lnth, host); */
	nl_printf((catgets(nlmsg_fd,NL_SETN,2, "sending %d packets of lnth %d to %s ...")), cnt, lnth, host);

	fflush(stdout);

next_ipaddr:	
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_CLEAR,
	    xdr_void, NULL, xdr_void, NULL)) {
		TRACE2("main mycallrpc(CLEAR) returned err = %d", err);
            	if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
		    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,15, "\nspray: can't reach %1$s at address %2$s\n")), host, inet_ntoa((struct in_addr *)adr));
		  if (! isdigit(orig_name[0])) {
		    if (hp && hp->h_addr_list[1]) {
		       hp->h_addr_list++;
		       adr = *((int *)hp->h_addr);
		       TRACE3("main translated adr = 0x%x, host = %s",adr,host);
		       fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "spray: now trying %s\n")), inet_ntoa(* (struct in_addr *)hp->h_addr));
		       goto next_ipaddr;
		    }
		  }
		}
		fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,13,"\n")));
		fflush(stdout);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "SPRAYPROC_CLEAR ")));
		clnt_perrno(err);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "\n")));
		exit(1);
	}

	for (i = 0; i < cnt; i++)
		callrpcnowait(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_SPRAY,
		    xdr_sprayarr, &arr, xdr_void, NULL);

	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_GET,
	    xdr_void, NULL, xdr_spraycumul, &cumul)) {
		TRACE2("main mycallrpc(GET) returned err = %d", err);
		fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,14,"\n")));
		fflush(stdout);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "SPRAYPROC_GET ")));
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "%s ")), host);
		clnt_perrno(err);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "\n")));
		exit(1);
	}
	TRACE3("main cnt = %d, cumul.counter = %d", cnt, cumul.counter);
	if (cumul.counter < cnt)
		/*nl_printf((catgets(nlmsg_fd,NL_SETN,8, "\n\t%1$d packets (%2$.3f%%) dropped by %3$s\n")), */
		nl_printf((catgets(nlmsg_fd,NL_SETN,8, "\n\t%d packets (%.3f%%) dropped by %s\n")),
		    cnt - cumul.counter,
		    100.0*(cnt - cumul.counter)/cnt, host);
	else
		printf((catgets(nlmsg_fd,NL_SETN,9, "\n\tno packets dropped by %s\n")), host);
	psec = (1000000.0 * cumul.counter)
	    / (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);
	bsec = (lnth * 1000000.0 * cumul.counter)/
	    (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);
	TRACE3("main psec = %d, bsec = %d", psec, bsec);
	/*nl_printf((catgets(nlmsg_fd,NL_SETN,10, "\t%1$d packets/sec, %2$d bytes/sec\n")), psec, bsec); */
	nl_printf((catgets(nlmsg_fd,NL_SETN,10, "\t%d packets/sec, %d bytes/sec\n")), psec, bsec);
	exit(0);
}

/* 
 * like callrpc, but with addr instead of host name
 */
mycallrpc(addr, prognum, versnum, procnum, inproc, in, outproc, out)
	u_long addr;
	int prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;
	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	TRACE("mycallrpc SOP");
	if (valid && oldprognum == prognum && oldversnum == versnum
		&& adr == oldadr) {
		/* reuse old client */		
		TRACE("mycallrpc reusing old client");
	}
	else {
		TRACE("mycallrpc getting new client");
		(void) close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			TRACE("mycallrpc destroying old client");
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 10;
		memcpy(&server_addr.sin_addr, &addr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		TRACE("mycallrpc about to call clntudp_create");
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		TRACE2("mycallrpc clntudp_create returned 0x%x", client);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
		TRACE4("mycallrpc prog=%d, vers=%d, adr=0x%x",prognum,versnum,adr);
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	TRACE("mycallrpc about to clnt_call");
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	TRACE3("mycallrpc clnt_call returned %d, valid = %d", clnt_stat, valid);
	return ((int) clnt_stat);
}

callrpcnowait(adr, prognum, versnum, procnum, inproc, in, outproc, out)
	u_long adr;
	int prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;
	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	TRACE("callrpcnowait SOP");
	if (valid && oldprognum == prognum && oldversnum == versnum
		&& oldadr == adr) {
		/* reuse old client */		
		TRACE("callrpcnowait reusing old client");
	}
	else {
		TRACE("callrpcnowait getting new client");
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			TRACE("callrpcnowait destroying old client");
			clnt_destroy(client);
			client = NULL;
		}
		/* timeout set to zero to implement nowait ? PRABHA  */
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
		memcpy(&server_addr.sin_addr, &adr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		TRACE("callrpcnowait about to clntudp_create");
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		TRACE2("callrpcnowait clntudp_create returned 0x%x", client);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
		TRACE4("callrpcnowait prog=%d, vers=%d, adr=0x%x",prognum,versnum,adr);
	}
	tottimeout.tv_sec = 0;
	tottimeout.tv_usec = 0;
	TRACE("callrpcnowait about to clnt_call");
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 * since timeout is zero, normal return value is RPC_TIMEDOUT
	 */
	if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT)
		valid = 0;
	TRACE3("callrpcnowait clnt_call returned %d, valid = %d", clnt_stat, valid);
	return ((int) clnt_stat);
}

usage()
{
	TRACE("usage SOP");
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "Usage: spray host [-c cnt] [-l lnth]\n")));
	exit(1);
}

char *
adrtostr(adr)
int adr;
{
	struct hostent *hp;
	static char buf[100];		/* hope this is long enough */

	TRACE("adrtostr SOP");
	hp = gethostbyaddr(&adr, sizeof(adr), AF_INET);
	if (hp == NULL) {
		TRACE("adrtostr gethostbyname NULL");
	    	sprintf(buf, (catgets(nlmsg_fd,NL_SETN,12, "0x%x")), adr);
		TRACE2("adrtostr returns %s", buf);
		return buf;
	} else {
		TRACE2("adrtostr returns %s", hp->h_name);
		return hp->h_name;
	}
}
