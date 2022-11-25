#ifndef lint
static  char rcsid[] = "@(#)yppoll:	$Revision: 1.31.109.1 $	$Date: 91/11/19 14:21:14 $  ";
#endif
/* yppoll.c	2.1 86/04/16 NFSSRC */ 
/*static char sccsid[] = "yppoll.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * This is a user command which asks a particular ypserv which version of a
 * map it is using.  Usage is:
 * 
 * yppoll [-h <host>] [-d <domainname>] mapname
 * 
 * where host may be either a name or an internet address of form ww.xx.yy.zz
 * 
 * If the host is ommitted, the local host will be used.  If host is specified
 * as an internet address, no nis services need to be locally available.
 *  
 */
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <sys/utsname.h>

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */

int status = 0;				/* exit status */
char *domain = NULL;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
char default_domain_name[YPMAXDOMAIN+1];
char *map = NULL;
char *host = NULL;
struct hostent *hp; /* moved here to make hp global - PRABHA */

char default_host_name[UTSLEN];
struct in_addr host_addr;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

void get_command_line_args();
void getdomain();
void getlochost();
void getrmthost();
void getmapparms();
void newresults();
void oldresults();
void usage();

extern u_long inet_addr();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/*
 * This is the mainline for the yppoll process.
 */

void
main (argc, argv)
	int argc;
	char **argv;
	
{
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("yppoll",0);
#endif NLS
	get_command_line_args(argc, argv);

	if (!domain) {
		getdomain();
	}
	
	if (!host) {
		getypserv();
	} else {
		getrmthost(host);
	}
	
	getmapparms();
	exit(status);
}

/*
 * This does the command line argument processing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
	argv++;
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'h': 

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if (strlen(host) > YPMAXPEER) {
						(void) fprintf(stderr,
						    (catgets(nlmsg_fd,NL_SETN,1, "yppoll:  hostname exceeds %d characters\n")),
							YPMAXPEER);
						exit(1);
					}
					
				} else {
					usage();
					exit(1);
				}
				
				break;
				
			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						(void) fprintf(stderr,
							(catgets(nlmsg_fd,NL_SETN,2, "yppoll:  NIS domain name exceeds %d characters\n")),
							YPMAXDOMAIN);
						exit(1);
					}
					
				} else {
					usage();
					exit(1);
				}
				
				break;
				
			default: 
				usage();
				exit(1);
			
			}
			
		} else {
			if (!map) {
				map = *argv;

				if (strlen(map) > YPMAXMAP) {
					(void) fprintf(stderr,
						(catgets(nlmsg_fd,NL_SETN,3, "yppoll:  mapname exceeds %d characters\n")),
						YPMAXMAP);
					exit(1);
				}

			} else {
				usage();
				exit(1);
			}
		}
	}

	if (!map) {
		usage();
		exit(1);
	}
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
void
getdomain()		
{
	if (!getdomainname(default_domain_name, sizeof(default_domain_name)) ) {
		domain = default_domain_name;
	} else {
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,4, "yppoll:  getdomainname system call failed\n")));
		exit(1);
	}

	if (strlen(domain) == 0) {
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,5, "yppoll:  the NIS domain name hasn't been set on this machine\n")));
		exit(1);
	}
}

/*
 * This gets the local hostname back from the kernel, and comes up with an
 * address for the local node without using the yp.  host_addr is set here.
 * This routine is never used so we will unifdef it out.
 */
#ifdef NOTDEF
void
getlochost()
{
/*
 *  HPNFS
 *
 *  The gethostname kernel call does not return a zero when it succeeds
 *  on the s300.
 *  Dave Erickson, 3-4-87.
 *
 *  HPNFS
 */
#ifdef hp9000s300
	if (gethostname(default_host_name, sizeof(default_host_name)) >= 0)
#else  hp9000s300
	if (! gethostname(default_host_name, sizeof(default_host_name)))
#endif hp9000s300
	{
		host = default_host_name;
	} else {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "yppoll:  gethostname system call failed\n")));
		exit(1);
	}

	get_myaddress(&myaddr); /* change this in 8.0 as follows? testing reqd*/
	host_addr = myaddr.sin_addr;
/*
 *	host_addr.sin_family = AF_INET;
 *	host_addr.sin_addr.s_addr = INADDR_LOOPBACK;
 */
}
#endif NOTDEF

/*
 * This gets an address for some named node by calling the standard library
 * routine gethostbyname.  host_addr is set here.
 */
void
getrmthost(hostname)
char	*hostname;
{		
	struct in_addr tempaddr;
	
	if (isdigit(*hostname) ) {
		tempaddr.s_addr = inet_addr(hostname);

		if ((int) tempaddr.s_addr != -1) {
			host_addr = tempaddr;
			return;
		}
	}
	
	/* this is called iff hostname != NULL and hostname ! in dot format */
	hp = gethostbyname(hostname);
		
	if (hp == NULL) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "yppoll:  %s was not found in /etc/hosts or the NIS hosts map\n")), hostname);
		exit(1);
	}
	
	host_addr.s_addr = *(u_long *)hp->h_addr;
}

void
getmapparms()
{
	struct dom_binding domb;
	struct ypreq_nokey req;
	struct ypresp_master mresp;
	struct ypresp_order oresp;
	struct ypresp_master *mresults = (struct ypresp_master *) NULL;
	struct ypresp_order *oresults = (struct ypresp_order *) NULL;
	struct yprequest oldreq;
	struct ypresponse oldresp;
	enum clnt_stat s;

next_ipaddr:

	domb.dom_server_addr.sin_addr = host_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = 0;
	domb.dom_server_port = 0;
	domb.dom_socket = RPC_ANYSOCK;
	req.domain = domain;
	req.map = map;
	mresp.master = NULL;

	if ((domb.dom_client = clntudp_create(&(domb.dom_server_addr),
	    YPPROG, YPVERS, udp_intertry, &(domb.dom_socket)))  == NULL) {
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
		 (void) fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,42, "yppoll: Can't reach master at %s\n")), inet_ntoa(*(u_long *)hp->h_addr));

		/* if hp != NULL, host != NULL and not in dot format; recycle */

		    if (hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			host_addr.s_addr = *(u_long *)hp->h_addr_list;
			(void) fprintf(stderr,
		    		(catgets(nlmsg_fd,NL_SETN,43, "yppoll: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
			goto next_ipaddr;
		    }
		}

		(void) fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,8, "Can't create UDP connection to %s\n")), host);
		clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,9, "Reason")));
		exit(1);
	}

	s = (enum clnt_stat) clnt_call(domb.dom_client, YPPROC_MASTER,
	    xdr_ypreq_nokey, &req, xdr_ypresp_master, &mresp, udp_timeout);
	
	if(s == RPC_SUCCESS) {
		mresults = &mresp;
		s = (enum clnt_stat) clnt_call(domb.dom_client, YPPROC_ORDER,
	    	    xdr_ypreq_nokey, &req, xdr_ypresp_order, &oresp,
		    udp_timeout);

		if(s == RPC_SUCCESS) {
			oresults = &oresp;
			newresults(mresults, oresults);
		} else {
			(void) fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,10, "Can't make YPPROC_ORDER call to ypserv at %s.\n	")),
			        host);
			clnt_perror(domb.dom_client, (catgets(nlmsg_fd,NL_SETN,11, "Reason")));
			exit(1);
		}
		
	} else {

		if (s == RPC_PROGVERSMISMATCH) {
			clnt_destroy(domb.dom_client);
			close(domb.dom_socket);
			domb.dom_server_addr.sin_port = 0;
			domb.dom_server_port = 0;
			domb.dom_socket = RPC_ANYSOCK;

			if ((domb.dom_client = clntudp_create(
			    &(domb.dom_server_addr), YPPROG, YPOLDVERS,
			    udp_intertry, &(domb.dom_socket)))  == NULL) {
				(void) fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,12, "Can't create V1 UDP connection to %s.\n	")), host);
				clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,13, "Reason")));
				exit(1);
			}
			
			oldreq.yp_reqtype = YPPOLL_REQTYPE;
			oldreq.yppoll_req_domain = domain;
			oldreq.yppoll_req_map = map;
			oldresp.yppoll_resp_domain = NULL;
			oldresp.yppoll_resp_map = NULL;
			oldresp.yppoll_resp_ordernum = 0;
			oldresp.yppoll_resp_owner = NULL;

			s = (enum clnt_stat) clnt_call(domb.dom_client,
			    YPOLDPROC_POLL, _xdr_yprequest, &oldreq,
			    _xdr_ypresponse, &oldresp, udp_timeout);
			
			if(s == RPC_SUCCESS) {
				oldresults(&oldresp);
			} else {
				(void) fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,14, "Can't make YPPROC_POLL call to ypserv at %s.\n	")),
			        host);
				clnt_perror(domb.dom_client, (catgets(nlmsg_fd,NL_SETN,15, "yppoll")));
				exit(1);
			}

		} else {
			(void) fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,16, "Can't make YPPROC_MASTER call to ypserv at %s.\n	")),
			        host);
			clnt_perror(domb.dom_client, (catgets(nlmsg_fd,NL_SETN,17, "Reason")));
			exit(1);
		}
	}
}

void
newresults(m, o)
	struct ypresp_master *m;
	struct ypresp_order *o;
{
	if (m->status == YP_TRUE	&& o->status == YP_TRUE) {
		(void) printf((catgets(nlmsg_fd,NL_SETN,18, "Domain %s is supported.\n")), domain);
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,19, "Map %1$s has order number %2$d.\n")),map, o->ordernum);
		(void) printf((catgets(nlmsg_fd,NL_SETN,20, "The master server is %s.\n")), m->master);
	} else if (o->status == YP_TRUE)  {
		(void) printf((catgets(nlmsg_fd,NL_SETN,21, "Domain %s is supported.\n")), domain);
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,22, "Map %1$s has order number %2$d.\n")),map, o->ordernum);
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,23, "Can't get master for map %1$s.\n	Reason:  %2$s\n")),
			       map, yperr_string(ypprot_err(m->status)) );
		status = 1;
	} else if (m->status == YP_TRUE)  {
		(void) printf((catgets(nlmsg_fd,NL_SETN,24, "Domain %s is supported.\n")), domain);
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,25, "Can't get order number for map %1$s.\n	Reason:  %2$s\n")), map,
		    yperr_string(ypprot_err(o->status)) );
		(void) printf((catgets(nlmsg_fd,NL_SETN,26, "The master server is %s.\n")), m->master);
		status = 1;
	} else {
		(void) printf((catgets(nlmsg_fd,NL_SETN,27, "Can't get any map parameter information.\n")));
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,28, "Can't get order number for map %1$s.\n	Reason:  %2$s\n")), map,
		    yperr_string(ypprot_err(o->status)) );
		(void) nl_printf((catgets(nlmsg_fd,NL_SETN,29, "Can't get master for map %1$s.\n	Reason:  %2$s\n")),
			       map, yperr_string(ypprot_err(m->status)) );
		status = 1;
	}
}

void
oldresults(resp)
	struct ypresponse *resp;
{
	if (resp->yp_resptype != YPPOLL_RESPTYPE) {
		(void) fprintf(stderr, 
	(catgets(nlmsg_fd,NL_SETN,30, "Ill-formed response returned from ypserv on host %s.\n")), host);
		status = 1;
	}

	if (!strcmp(resp->yppoll_resp_domain, domain) ) {
		(void) printf((catgets(nlmsg_fd,NL_SETN,31, "Domain %s is supported.\n")), domain);

		if (!strcmp(resp->yppoll_resp_map, map) ) {

			if (resp->yppoll_resp_ordernum != 0) {
				(void) nl_printf((catgets(nlmsg_fd,NL_SETN,32, "Map %1$s has order number %2$d.\n")),
				    map, resp->yppoll_resp_ordernum);

				if (strcmp(resp->yppoll_resp_owner, "") ) {
					(void) printf(
					    (catgets(nlmsg_fd,NL_SETN,33, "The master server is %s.\n")),
					    resp->yppoll_resp_owner);
				} else {
					(void) printf(
					    (catgets(nlmsg_fd,NL_SETN,34, "Unknown master server.\n")));
					status = 1;
				}
				
			} else {
				(void) printf((catgets(nlmsg_fd,NL_SETN,35, "Map %s is not supported.\n")),
				    map);
				status = 1;
			}
			
		} else {
			(void) nl_printf((catgets(nlmsg_fd,NL_SETN,36, "Map %1$s does not exist at %2$s.\n")),
			    map, host);
			status = 1;
		}
		
	} else {
		(void) printf((catgets(nlmsg_fd,NL_SETN,37, "Domain %s is not supported.\n")), domain);
		status = 1;
	}
}

getypserv()
{
	int x;
	char host[256];
	struct in_addr addr;
	struct ypbind_resp response;

/*
 *  HPNFS
 *
 *  The gethostname kernel call does not return a zero when it succeeds
 *  on the s300.
 *  Dave Erickson, 3-4-87.
 *
 *  HPNFS
 */
#ifdef hp9000s300
	if (gethostname(host, sizeof(host)) < 0)
#else  hp9000s300
	if (gethostname(host, sizeof(host)) != 0)
#endif hp9000s300
	{
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,38, "Can't get hostname\n")));
		exit(1);
	}
	x = callrpc(host, YPBINDPROG, YPBINDVERS, YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp, &response);

	/* on failure try old protocol */
	if (x)	{
		x = callrpc(host, YPBINDPROG, YPBINDOLDVERS, YPBINDPROC_DOMAIN,
		    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp,
		    &response);

		if (x) {
			clnt_perrno(x);
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,39, "\n")));
			exit(1);
		}
	}
	if (response.ypbind_status != YPBIND_SUCC_VAL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,40, "Couldn't get NIS server %d\n")),
		    response.ypbind_status);
		exit(1);
	}
	host_addr=response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
}

void
usage()
{
	fprintf(stderr,
(catgets(nlmsg_fd,NL_SETN,41, "Usage:  yppoll [-h host] [-d domain] mapname\n\n where   host is either a name or an internet address of the form ww.xx.yy.zz\n")));
}
