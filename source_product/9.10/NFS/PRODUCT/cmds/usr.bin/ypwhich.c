#ifndef lint
static  char rcsid[] = "@(#)ypwhich:	$Revision: 1.36.109.1 $	$Date: 91/11/19 14:07:48 $  ";
#endif
/* NFSSRC ypwhich.c	2.1 86/04/17 */
/*"ypwhich.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

/*
 * This is a user command which tells which nis server is being used by a
 * given machine, or which nis server is the master for a named map.
 * 
 * Usage is:
 * ypwhich [-d domain] [-m [mname] [-t] | [-V1 | -V2] host]
 * or
 * ypwhich -x
 * 
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  -m tells the master of that map.  mname may be either a
 * mapname, or a nickname which will be translated into a mapname according
 * to the translation table at transtable.  The  -t switch inhibits this
 * translation.  If the -m option is used, ypwhich will act like a vanilla
 * nis client, and will not attempt to choose a particular yp server.  On the
 * other hand, if no -m switch is used, ypwhich will talk directly to the yp
 * bind process on the named host, or to the local ypbind process if no host
 * name is specified.  The -x switch can be used to dump the nickname
 * translation table.
 *  
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
#include <netdb.h>
#include <time.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>
#include <sys/utsname.h>

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */
#define MAXHOSTNAMELEN 256		/* max hostname+1 */

bool translate = TRUE;
bool dodump = FALSE;
bool newvers = FALSE;
bool oldvers = FALSE;
bool ask_specific = FALSE;
char *domain = NULL;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
char default_domain_name[YPMAXDOMAIN+1];
char *host = NULL;
struct hostent *hp = NULL; /* if NULL, no address recycling */
char default_host_name[MAXHOSTNAMELEN];
struct in_addr host_addr;
bool get_master = FALSE;
bool get_server = FALSE;
char *map = NULL;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byaddr",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases","mail.aliases",
	"ethers", "ethers.byname",
	NULL
};

void get_command_line_args();
void getdomain();
void getlochost();
void getrmthost();
void get_server_name();
bool call_binder();
void print_server();
void get_map_master();
void dumptable();
void dump_ypmaps();
void v1dumpmaps();
void v2dumpmaps();
void usage();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS


/*
 * This is the main line for the ypwhich process.
 */
main(argc, argv)
	char **argv;
{
	int addr;
	int err;
	struct ypbind_resp response;
	int i;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypwhich",0);
#endif NLS
	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {
		getdomain();
	}
	
	if (get_server) {
		
		if (!host) {
			getlochost();
		} else {
			getrmthost(host);
		}

		get_server_name();
	} else {
		
		if (map) {
	
			if (translate) {
						
				for (i = 0; transtable[i]; i+=2) {
					    
					if (strcmp(map, transtable[i]) == 0) {
						map = transtable[i+1];
					}
				}
			}
			
			get_map_master();
		} else {
			dump_ypmaps();
		}
	}

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

	if (argc == 1) {
		get_server = TRUE;
		return;
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 't': 
				translate = FALSE;
				argv++;
				break;

			case 'x': 
				dodump = TRUE;
				argv++;
				break;

			case 'V':

				if ((*argv)[2] == '1') {
					oldvers = TRUE;
					argv++;
					break;
				} else if ((*argv)[2] == '2') {
					newvers = TRUE;
					argv++;
					break;
				} else {
					usage();
					exit(1);
				}

			case 'm': 
				get_master = TRUE;
				argv++;
				
				if (argc > 1) {
					
					if ( (*(argv))[0] == '-') {
						break;
					}
					
					argc--;
					map = *argv;
					argv++;

					if (strlen(map) > YPMAXMAP) {
						(void) fprintf(stderr,
							(catgets(nlmsg_fd,NL_SETN,1, "ypwhich:  mapname exceeds %d characters\n")),
							YPMAXMAP);
						exit(1);
					}
					
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
							(catgets(nlmsg_fd,NL_SETN,2, "ypwhich:  NIS domain name exceeds %d characters\n")),
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
			
			if (get_server) {
				usage();
				exit(1);
			}
		
			get_server = TRUE;
			host = *argv;
			argv++;
			
			if (strlen(host) > YPMAXPEER) {
				(void) fprintf(stderr,
					(catgets(nlmsg_fd,NL_SETN,3, "ypwhich:  hostname exceeds %d characters\n")),
					YPMAXPEER);
				exit(1);
			}
		}
	}

	if (newvers && oldvers) {
		usage();
		exit(1);
	}
	
	if (newvers || oldvers) {
		ask_specific = TRUE;
	}
	
	if (get_master && get_server) {
		usage();
		exit(1);
	}
	
	if (!get_master && !get_server) {
		get_server = TRUE;
	} 
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
void
getdomain()		
{
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
	if (!getdomainname(default_domain_name, sizeof(default_domain_name)) ) {
		domain = default_domain_name;
	} else {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "ypwhich:  getdomainname system call failed\n")));
		exit(1);
	}

	if (strlen(domain) == 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "ypwhich:  the NIS domain name hasn't been set on this machine\n")));
		exit(1);
	}
}

/*
 * This gets the local hostname back from the kernel, and comes up with an
 * address for the local node without using the yp.  host_addr is set here.
 */
void
getlochost()
{
	struct sockaddr_in myaddr;

/*
 *  HPNFS
 *
 *  The gethostname kernel call does not return a zero when it succeeds
 *  on the s300.
 *  Dave Erickson
 *
 *  HPNFS
 */
#ifdef hp9000s300
	if (gethostname(default_host_name, sizeof(default_host_name)) >= 0) {
#else  hp9000s300
	if (! gethostname(default_host_name, sizeof(default_host_name))) {
#endif hp9000s300
		host = default_host_name;
	} else {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "ypwhich:  gethostname system call failed\n")));
		exit(1);
	}

	get_myaddress(&myaddr);
	host_addr = myaddr.sin_addr;
/*
 *	host_addr.sin_family = AF_INET;
 *	host_addr.s_addr = INADDR_LOOPBACK;
 */
}

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
	
	hp = gethostbyname(hostname);
		
	if (hp == NULL) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "ypwhich:  %s was not found in /etc/hosts or the NIS hosts map\n")), hostname);
		exit(1);
	}
	
	host_addr.s_addr = *(u_long *)hp->h_addr;
}

/*
 * This tries to find the name of the server to which the binder in question
 * is bound.  If one of the -Vx flags was specified, it will try only for
 * that protocol version, otherwise, it will start with the current version,
 * then drop back to the previous version.
 */
void
get_server_name()
{
	int vers;
	struct in_addr server;
	
	if (ask_specific) {
		vers = oldvers ? YPBINDOLDVERS : YPBINDVERS;
		
		if (call_binder(vers, &server)) {
			print_server(&server);
		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "NIS domain %s not bound.\n")), domain);
		}
		
	} else {
		vers = YPBINDVERS;
		
		if (call_binder(vers, &server)) {
			print_server(&server);
		} else {

			vers = YPBINDOLDVERS;
		
			if (call_binder(vers, &server)) {
				print_server(&server);
			} else {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "NIS domain %s not bound.\n")), domain);
			}
		}
	}
}

/*
 * This sends a message to the ypbind process on the node with address held
 * in host_addr.
 */
bool
call_binder(vers, server)
	int vers;
	struct in_addr *server;
{
	struct sockaddr_in query;
	CLIENT *client;
	int sock = RPC_ANYSOCK;
	enum clnt_stat rpc_stat;
	struct ypbind_resp response;
	char errstring[256];

next_ipaddr:

	query.sin_family = AF_INET;
	query.sin_port = 0;
	query.sin_addr = host_addr;
	memset(query.sin_zero, 0, 8);
	
	if ((client = clntudp_create(&query, YPBINDPROG, vers, udp_intertry,
	    &sock)) == NULL) {
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
		    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,29, "ypwhich: can't reach host %1$s at address %2$s\n")),host, inet_ntoa(*(u_long *)hp->h_addr));

		/* hp != NULL iff host name is != NULL and != dot format */

		    if (hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			host_addr.s_addr = *(u_long *)hp->h_addr;
		        fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,30, "ypwhich: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
			goto next_ipaddr;
		    }
		}
		(void) clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,10, "ypwhich:  clntudp_create error")));
		exit(1);
	}

	rpc_stat = clnt_call(client, YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp, &response,
	    udp_timeout);
	    
	if ((rpc_stat != RPC_SUCCESS) &&
	    (rpc_stat != RPC_PROGVERSMISMATCH) ) {
		(void) nl_sprintf(errstring,
			(catgets(nlmsg_fd,NL_SETN,11, "ypwhich:  can't talk to ypbind on %1$s or to an NIS server for domain %2$s")), host, domain);
		(void) clnt_perror(client, errstring);
		exit(1);
	}

	*server = response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
	clnt_destroy(client);
	close(sock);
	
	if ((rpc_stat != RPC_SUCCESS) ||
	    (response.ypbind_status != YPBIND_SUCC_VAL) ) {
		return (FALSE);
	}
	
	return (TRUE);
}

/*
 * This translates a server address to a name and prints it.  If the address
 * is the same as the local address as returned by get_myaddress, the name
 * is that retrieved from the kernel.  If it's any other address (including
 * another ip address for the local machine), we'll get a name by using the
 * standard library routine (which calls the yp).  
 */
void
print_server(server)
	struct in_addr *server;
{
	struct sockaddr_in myaddr;
	char myname[MAXHOSTNAMELEN];
	struct hostent *hp;
	
	get_myaddress(&myaddr);
	
	if (server->s_addr == myaddr.sin_addr.s_addr) {
		
/*
 *  HPNFS
 *
 *  The gethostname kernel call does not return a zero when it succeeds
 *  on the s300.
 *  Dave Erickson
 *
 *  HPNFS
 */
#ifdef hp9000s300
		if (gethostname(myname, sizeof(myname)) < 0) {
#else  hp9000s300
		if (gethostname(myname, sizeof(myname))) {
#endif hp9000s300
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "ypwhich:  gethostname system call failed\n")));
			exit(1);
		}

		printf(myname);
		printf((catgets(nlmsg_fd,NL_SETN,13, "\n")));
	} else {
		hp = gethostbyaddr(server, sizeof(server), AF_INET);
	
		if (hp == NULL) {
			printf((catgets(nlmsg_fd,NL_SETN,14, "0x%x\n")), *server);
		} else {
			printf((catgets(nlmsg_fd,NL_SETN,15, "%s\n")), hp->h_name);
		}
	}
}

/*
 * This asks any nis server for the map's master.  
 */
void
get_map_master()
{
	int err;
	char *master;
     
	err = yp_master(domain, map, &master);
	
	if (err) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "ypwhich:  can't find the master of %1$s.\n          Reason:  %2$s\n")),
		    map, yperr_string(err) );
	} else {
		printf((catgets(nlmsg_fd,NL_SETN,17, "%s\n")), master);
	}
}

/*
 * This will print out the map nickname translation table.
 */
void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		nl_printf((catgets(nlmsg_fd,NL_SETN,18, "Use \"%1$s\" for map \"%2$s\"\n")), transtable[i],
		    transtable[i + 1]);
	}
}

/*
 * This enumerates the entries within map "ypmaps" in the domain at global 
 * "domain", and prints them out key and value per single line.  dump_ypmaps
 * just decides whether we are (probably) able to speak the new NIS protocol,
 * and dispatches to the appropriate function.
 */
void
dump_ypmaps()
{
	int err;
	struct dom_binding *binding;
	
	if (err = _yp_dobind(domain, &binding)) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "ypwhich:  can't bind to an NIS server for domain %1$s.\n          Reason:  %2$s\n")),
		    domain, yperr_string(ypprot_err(err)));
		return;
	}

	if (binding->dom_vers == YPVERS) {
		v2dumpmaps(binding);
	} else {
		v1dumpmaps();
	}
}

void
v1dumpmaps()
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	char *scan;
	
	if (err = yp_first(domain, "ypmaps", &outkey, &outkeylen, &val,
	    &vallen) ) {

		if (err = YPERR_NOMORE) {
			exit(0);
		} else {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "ypwhich:  can't get the first record from the NIS map.\n          Reason:  %s\n")),
			    yperr_string(err) );
		}

		exit(1);
	}

	while (TRUE) {

		for (scan = outkey; *scan != NULL; scan++) {

			if (*scan == '\n') {
				*scan = ' ';  
			}
		}

		if (strlen(outkey) < 3 || strncmp(outkey, "YP_", 3) ) {
			printf(outkey);
			printf(val);
		}
		
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next(domain, "ypmaps", key, keylen, &outkey,
		    &outkeylen, &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "ypwhich:  can't get the next record from the NIS map.\n          Reason:  %s\n")),
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
}

void
v2dumpmaps(binding)
	struct dom_binding *binding;
{
	enum clnt_stat rpc_stat;
	int err;
	char *master;
	struct ypmaplist *pmpl;
	struct ypresp_maplist maplist;

	maplist.list = (struct ypmaplist *) NULL;
	
	rpc_stat = clnt_call(binding->dom_client, YPPROC_MAPLIST,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypresp_maplist, &maplist,
	    udp_timeout);

	if (rpc_stat != RPC_SUCCESS) {
		(void) clnt_perror(binding->dom_client,
		    (catgets(nlmsg_fd,NL_SETN,22, "ypwhich:  can't get the list of maps")));
		exit(1);
	}

	if (maplist.status != YP_TRUE) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,23, "ypwhich:  can't get the list of maps.\n          Reason:  %s\n")),
		    yperr_string(ypprot_err(maplist.status)) );
		exit(1);
	}

	for (pmpl = maplist.list; pmpl; pmpl = pmpl->ypml_next) {
		printf((catgets(nlmsg_fd,NL_SETN,24, "%s ")), pmpl->ypml_name);
		
		err = yp_master(domain, pmpl->ypml_name, &master);
	
		if (err) {
			printf((catgets(nlmsg_fd,NL_SETN,25, "????????\n")));
			nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,26, "ypwhich:  can't find the master of %1$s.\n          Reason:  %2$s\n")),
		   	    pmpl->ypml_name, yperr_string(err) );
		} else {
			printf((catgets(nlmsg_fd,NL_SETN,27, "%s\n")), master);
		}
	}
}

void
usage()
{
fprintf(stderr,
(catgets(nlmsg_fd,NL_SETN,28, "Usage:  ypwhich [-d domain] [-V1 | -V2] [hostname]\n        ypwhich [-d domain] [-t] [-m [mname]]\n        ypwhich -x\n        ypwhich\n\nwhere   mname may be either a mapname or a map nickname\n        -t inhibits map nickname translation\n        -m lists the master server's name for one or all maps\n        -x prints a list of map nicknames and their corresponding maps\n")));
}
