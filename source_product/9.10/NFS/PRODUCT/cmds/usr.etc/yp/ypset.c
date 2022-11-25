#ifndef lint
static  char rcsid[] = "@(#)ypset:	$Revision: 1.28.109.2 $	$Date: 92/03/11 14:17:35 $  ";
#endif
/* ypset.c	2.1 86/04/16 NFSSRC */ 
/*static char sccsid[] = "ypset.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * This is a user command which issues a "Set domain binding" command to a 
 * network information service binder (ypbind) process
 *
 *	ypset [-V1] [-h <host>] [-d <domainname>] server_to_use
 *
 * where host and server_to_use may be either names or internet addresses.
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
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>	/* 8.0 addition - prabha */
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

bool oldvers = FALSE;
char *domain = NULL;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
char default_domain_name[YPMAXDOMAIN+1];
char default_host_name[UTSLEN];
char *host = NULL;
struct hostent *hp = NULL;
char *server_to_use;
struct in_addr host_addr, server_to_use_addr;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};


void get_command_line_args();
void get_host_addr();
void send_message();
void usage();

/*
 * Funny external reference to inet_addr to make the reference agree with the
 * code, not the documentation.  Should be:
 * extern struct in_addr inet_addr();
 * according to the documentation, but that's not what the code does.
 */
extern u_long inet_addr();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/*
 * This is the mainline for the ypset process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

void
main (argc, argv)
	int argc;
	char **argv;

{
	struct sockaddr_in myaddr;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypset",0);
#endif NLS
	get_command_line_args(argc, argv);

	if (!domain) {

/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
		if (!getdomainname(default_domain_name, YPMAXDOMAIN+1) ) {
			domain = default_domain_name;
		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "Sorry, can't get domainname back from system call.\n")));
			exit(1);
		}

		if (strlen(domain) == 0) {
			fprintf(stderr, 
		(catgets(nlmsg_fd,NL_SETN,2, "Sorry, the domainname hasn't been set on this machine.\n")));
			exit(1);
		}
	}

	if (!host) {
		
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
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "ypset:  gethostname system call failed\n")));
			exit(1);
		}


		get_myaddress(&myaddr);
		host_addr = myaddr.sin_addr;

		/* 8.0 change - use loopbackaddr ? testing required */
	/*
	 *	host_addr.sin_family = AF_INET;
	 *	host_addr.s_addr = INADDR_LOOPBACK;
	 */
	} else  {
		get_host_addr(host, &host_addr);
	}

	/* this call must be made after the get_host_addr call to get host addr
	 * because send_message expects hostent hp to have the server addr */

	get_host_addr(server_to_use, &server_to_use_addr);
	send_message();
	exit(0);
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
	
	while (--argc > 1) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'V':

				if ((*argv)[2] == '1') {
					oldvers = TRUE;
					argv++;
					break;
				} else {
					usage();
					exit(1);
				}

			case 'h': {

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if (strlen(host) > 256) {
						fprintf(stderr, 
				(catgets(nlmsg_fd,NL_SETN,4, "Sorry, the hostname argument is bad.\n")));
						exit(1);
					}
					
				} else {
					usage();
					exit(1);
				}
				
				break;
			}
				
			case 'd': {

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						(void) fprintf(stderr,
							(catgets(nlmsg_fd,NL_SETN,5, "ypset:  NIS domain name exceeds %d characters\n")),
							YPMAXDOMAIN);
						exit(1);
					}
					
				} else {
					usage();
					exit(1);
				}
				
				break;
			}
				
			default: {
				usage();
				exit(1);
			}
			
			}
			
		} else {
			usage();
			exit(1);
		}
	}

	if (argc == 1) {
		
		if ( (*argv)[0] == '-') {
			usage();
			exit(1);
		}
		
		server_to_use = *argv;

		if (strlen(server_to_use) > 256) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "Sorry, the server_to_use argument is bad.\n")));
			exit(1);
		}

	} else {
		usage();
		exit(1);
	}
}

/*
 * This gets an address for the host to which the request will be directed,
 * or the host to be used as the nis server.
 *
 * If the first character of the host string is a digit, it calls inet_addr(3)
 * to do the translation.  If that fails, it then tries to contact the NIS
 * server (any server) and tries to get a match on the host name.  It then calls
 * inet_addr to turn the returned string (equivalent to that which is in
 * /etc/hosts) to an internet address.
 */
 
void
get_host_addr(name, addr)
	char *name;
	struct in_addr *addr;
{
	char *ascii_addr;
	int addr_len;
	struct in_addr tempaddr;

	if (isdigit(*name) ) {
		tempaddr.s_addr = inet_addr(name);

		if ((int) tempaddr.s_addr != -1) {
			*addr = tempaddr;
			return;
		}
	}

/********* changed the following to use BIND before YP - prabha ****************

	if (!yp_bind(domain) ) {
		
		if (!yp_match (domain, "hosts.byname", name, strlen(name),
		    &ascii_addr, &addr_len) ) {
			tempaddr.s_addr = inet_addr(ascii_addr);

			if ((int) tempaddr.s_addr != -1) {
				*addr = tempaddr;
			} else {
				fprintf(stderr, 
     (catgets(nlmsg_fd,NL_SETN,7, "Sorry, I got a garbage address for host %s back from the network information service.\n")),
				    name);
				exit(1);
			}

		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "Sorry, I can't get an address for host %s from the network information service.\n")), name);
			exit(1);
		}
		
	} else {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "Sorry, I can't make use of the network information service.  I give  up.\n")));
		exit(1);
	}
********************** changed the above to use bind - prabha *****************/

        hp = gethostbyname(name);

        if (hp == NULL) {
                fprintf(stderr, (catgets(nlmsg_fd,NL_SETN, 14, "ypset: %s not found in hosts databases\n")), name);
                exit(1);
        }

	/* else */

        addr->s_addr = *(u_long *)hp->h_addr;/* assign the 1st addr for name */
}


/*
 * This takes the internet address of the network information service host of interest,
 * and fires off the "set domain binding" message to the ypbind process.
 */
 
void
send_message()
{
	struct dom_binding domb;
	struct ypbind_setdom req;
	struct ypbind_oldsetdom oldreq;
	enum clnt_stat clnt_stat;
	int vers;

next_ipaddr:

	vers = oldvers ? YPBINDOLDVERS : YPBINDVERS;
	domb.dom_server_addr.sin_addr = host_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = 0;
	domb.dom_server_port = 0;
	domb.dom_socket = RPC_ANYSOCK;

	/*
	 * Open up a udp path to the server
	 */

	if ((domb.dom_client = clntudp_create(&(domb.dom_server_addr),
	    YPBINDPROG, vers, udp_intertry, &(domb.dom_socket))) == NULL) {
                if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
                 (void) fprintf(stderr,
                    (catgets(nlmsg_fd,NL_SETN,15, "ypset: Can't reach %s at %s\n")),host, inet_ntoa(*(u_long *)hp->h_addr));

                 /* if hp != NULL, server_to_use != NULL & ! in dot format */

                    if (hp && hp->h_addr_list[1]) {
                        hp->h_addr_list++;
                        host_addr.s_addr = *(u_long *)hp->h_addr_list;
                        (void) fprintf(stderr,
                                (catgets(nlmsg_fd,NL_SETN,16, "ypset: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
                        goto next_ipaddr;
                    }
		}
		fprintf(stderr,
	(catgets(nlmsg_fd,NL_SETN,10, "Sorry, I can't set up a udp connection to ypbind on host %s.\n")),
			host);
		exit(1);
	}
	/*
	 * Load up the message structure and fire it off.
	 */
	if (oldvers) {
		strcpy(oldreq.ypoldsetdom_domain, domain);
		oldreq.ypoldsetdom_addr = server_to_use_addr;
		oldreq.ypoldsetdom_port = 0;
	
		clnt_stat = (enum clnt_stat) clnt_call(domb.dom_client,
		    YPBINDPROC_SETDOM, _xdr_ypbind_oldsetdom, &oldreq,
		    xdr_void, 0, udp_timeout); 
		
	} else {
		strcpy(req.ypsetdom_domain, domain);
		req.ypsetdom_addr = server_to_use_addr;
		req.ypsetdom_port = 0;
		req.ypsetdom_vers = YPVERS;
	
		clnt_stat = (enum clnt_stat) clnt_call(domb.dom_client,
		    YPBINDPROC_SETDOM, xdr_ypbind_setdom, &req, xdr_void, 0,
		    udp_timeout); 
	}
	 



	  /* sun ypbind returns RPC_PROGUNAVAIL to indicate
	   * -ypset or -ypsetme is not set 
	   * but hp ypbind return RPC_SYSTEMERROR 
	   */

	   if (clnt_stat == RPC_SUCCESS)
	       return;

	       else if (clnt_stat == RPC_SYSTEMERROR)

                    {

			fprintf(stderr, 
	(catgets(nlmsg_fd,NL_SETN,11, "Sorry, ypbind on host %s has rejected your request.\n")), host);

                     }
		     
                  else
		     {

			fprintf(stderr, 
	(catgets(nlmsg_fd,NL_SETN,12, "Sorry, I couldn't send my rpc message to ypbind on host %s.\n")), host);

                       }
			exit(1);
}

void
usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "Usage:  ypset [-V1] [-h host] [-d domain] server_to_use\n\nwhere   host and server_to_use are either names or internet addresses of the\nform ww.xx.yy.zz\n")));
}
