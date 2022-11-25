#ifndef lint
static  char rcsid[] = "@(#)ypxfr:	$Revision: 1.45.109.3 $	$Date: 94/12/16 09:19:38 $  ";
#endif
/* ypxfr.c	2.1 86/04/16 NFSSRC */ 
/* ypxfr.c 1.1 86/02/05 Copyr 1984 Sun Micro */

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: ypxfr.o $Revision: 1.45.109.3 $ 94/06/01 PHNE_5081";
#endif

/*
 * This is a user command which gets a nis data base from some running
 * server, and gets it to the local site by using the normal nis client
 * enumeration functions.  The map is copied to a temp name, then the real
 * map is removed and the temp map is moved to the real name.  ypxfr then
 * sends a "YPPROC_CLEAR" message to the local server to insure that he will
 * not hold a removed map open, so serving an obsolete version.  
 * 
 * <ypxfr [-h <host>] [-d <domainname>] [-f] [-c] [-C tid prot ipadd port] map>
 * *****  The above comment line by Sun is incorrect.  It should read:  *****
 * ypxfr [-h <host>] [-d <domainname>] [-f] [-c] [-C tid prog ipadd port] map
 *							 ^^^^
 * 
 * where host may be either a name or an internet address of form ww.xx.yy.zz
 * 
 * If the host is ommitted, ypxfr will attempt to discover the master by 
 * using normal nis services.  If it can't get the record, it will use
 * the address of the callback, if specified. If the host is specified 
 * as an internet address, no nis services need to be locally available.  
 * 
 * If the domain is not specified, the default domain of the local machine
 * is used.
 * 
 * If the -f flag is used, the transfer will be done even if the master's
 * copy is not newer than the local copy.
 *
 * The -c flag suppresses the YPPROC_CLEAR request to the local ypserv.  It
 * may be used if ypserv isn't currently running to suppress the error message.
 * 
 * The -C flag is used to pass callback information to ypxfr when it is
 * activated by ypserv.  The callback information is used to send a
 * yppushresp_xfr message with transaction id "tid" to a yppush process
 * <speaking a transient protocol number "prot".  The yppush program is>
 * *****  The above comment line by Sun is incorrect.  It should read:  *****
 * running a transient program number "program".  The yppush program is
 * running on the node with IP address "ipadd", and is listening (UDP) on
 * "port".  
 *  
 */
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#define nl_cxtime(a, b)	ctime(a)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <dbm.h>
#undef NULL
#define DATUM

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/param.h>
/* HPNFS Use ndir.h instead of sys/dir.h to get the right definition */
/* HPNFS for DIRSIZ */
#include <ndir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <fcntl.h>

int traceon = 0;
#include <arpa/trace.h>

# define PARANOID 1		/* make sure maps have the right # entries */

#define UDPINTER_TRY 10			/* Seconds between tries for udp */
#define UDPTIMEOUT UDPINTER_TRY*4	/* Total timeout for udp */
#define CALLINTER_TRY 10		/* Seconds between callback tries */
#define CALLTIMEOUT CALLINTER_TRY*6	/* Total timeout for callback */


struct timeval udp_intertry = {
	UDPINTER_TRY,
	0
};
struct timeval udp_timeout = {
	UDPTIMEOUT,
	0
};
struct timeval tcp_timeout = {
	180,	/* timeout for map enumeration (could be long) */
	0
};


char *domain = NULL;
char *map = NULL;
char *master = NULL;		/* The name of the xfer peer as specified as a
				 *   -h option, or from querying the nis */
struct in_addr master_addr;	/* Addr of above */
struct dom_binding master_server;/* To talk to above */
unsigned int master_prog_vers;	/* YPVERS or YPOLDVERS */
char *master_name = NULL;	/* Map's master as contained in the map */
unsigned *master_version = NULL; /* Order number as contained in the map */
char *master_ascii_version;	/* ASCII order number as contained in the map */
bool fake_master_version = FALSE; /* TRUE only if there's no order number in
				  *  the map, and the user specified -f */
char yp_last_modified[] = "YP_LAST_MODIFIED";
char yp_master_name[] = "YP_MASTER_NAME";
#ifdef DBINTERDOMAIN
char yp_interdomain[] = "YP_INTERDOMAIN";
char yp_interdomain_sz = sizeof (yp_interdomain) - 1;
#endif /* DBINTERDOMAIN */
bool force = FALSE;		/* TRUE iff user specified -f flag */
bool logging = FALSE;		/* TRUE iff no tty, but log file exists */
bool send_clear = TRUE;		/* FALSE iff user specified -c flag */
bool callback = FALSE;		/* TRUE iff -C flag set.  tid, proto, ipadd,
			     **** Above line should read: tid, program, ipadd,
				 * and port will be set to point to the
				 * command line args. */
#ifdef DBINTERDOMAIN
bool interdomain_map = FALSE;   /* True if there is yp_interdomain in ether
                                   the local or the master version of the map */
int interdomain_sz = 0;         /* Size of the interdomain value */
char *interdomain_value;        /* place to store the interdomain value */
#endif /* DBINTERDOMAIN */

char *tid;
/*  HPNFS
 *
 *  The name of the variable "proto" was changed to "program", because the
 *  variable is used to point to a transient program number registered with
 *  portmap on the machine invoking a yppush (if such a one exists).  It is
 *  not a protocol!  The name then matches the corresponding variable in the
 *  yppush code.  (See also the flagged erroneous Sun comments, above.)
 *  Dave Erickson, 3-18-87.
 *
 *  HPNFS
 */
/*  char *proto;  */
char *program;
char *ipadd;
char *port;
int entry_count;		/* counts entries in the map */
/*  HPNFS
 *
 *  Sun has a symbolic link from /etc/yp to /usr/etc/yp.  HP does not.
 *  Dave Erickson, 3-18-87.
 *
 *  HPNFS
 */
char logfile[] = "/usr/etc/yp/ypxfr.log";

/*  HPNFS
 *
 *  Sun has a symbolic link from /etc/yp to /usr/etc/yp.  HP does not.
 *  Dave Erickson, 3-19-87.
 *
 *  HPNFS
 */
char ypdbpath[] = "/usr/etc/yp";

void get_command_line_args();
bool get_master_addr();
bool bind_to_server();
bool ping_server();
bool get_private_recs();
bool get_order();
bool get_v1order();
bool get_v2order();
bool get_misc_recs();
bool get_master_name();
bool get_v1master_name();
bool get_v2master_name();
void find_map_master();
bool move_map();
unsigned get_local_version();
/*  HPNFS
 *
 *  The mkfilename function now returns one of these defined integer values.
 *  Dave Erickson, 3-20-87.
 *
 *  HPNFS
 */
int mkfilename();
#define MAPNAME_OK		0
#define PATH_TOO_LONG		1
#define MAPNAME_TOO_LONG	2
void mk_tmpname();
bool rename_map();
bool check_map_existence();
bool get_map();
bool add_private_entries();
bool new_mapfiles();
void del_mapfiles();
void set_output();
void logprintf();
bool send_ypclear();
void xfr_exit();
void send_callback();
int ypall_callback();
int map_yperr_to_pusherr();

extern u_long inet_addr();
extern struct hostent *gethostbyname();
extern int errno;
void usage();
/*  HPNFS
 *
 *  The translate_mapname function, shared by ypxfr and ypserv, is needed.
 *  Dave Erickson, 3-20-87.
 *
 *  HPNFS
 */
#include "../ypserv/trans.h"
#include <sys/utsname.h>
extern char *translate_mapname();

int max_length = DIRSIZ_CONSTANT; /* What is the maximum filename in the  */
				  /* directory ypdbpath.		  */

#ifdef NLS
nl_catd nlmsg_fd;
char *nl_cxtime();
#endif NLS

/*
 * This is the mainline for the ypxfr process.
 */

void
main(argc, argv)
	int argc;
	char **argv;
	
{
	int i;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
	static char default_domain_name[YPMAXDOMAIN+1];
	static unsigned big = 0xffffffff;
	int status;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypxfr",0);
#endif NLS
	
	STARTTRACE("/tmp/ypxfr.trace");
	set_output();

	TRACE("Before get_command_line_args");
	get_command_line_args(argc, argv);
	TRACE("After get_command_line_args");

	if (!domain) {
		
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
		TRACE("In !domain, before getdomainname");
		if (!getdomainname(default_domain_name, YPMAXDOMAIN+1) ) {
			domain = default_domain_name;
			TRACE2("domain is %s", domain);
		} else {
			logprintf( (catgets(nlmsg_fd,NL_SETN,1, "ypxfr:  getdomainname system call failed\n")));
			xfr_exit(YPPUSH_RSRC);
		}

		if (strlen(domain) == 0) {
			logprintf( (catgets(nlmsg_fd,NL_SETN,2, "ypxfr:  NIS domain name not set on this machine\n")));
			xfr_exit(YPPUSH_RSRC);
		}
	}
	
	if (!master) {
		TRACE("In !master");
		find_map_master();
		TRACE("After find_map_master");
	}
	
	if (!get_master_addr() ) {
		TRACE("In !get_master_addr");
		xfr_exit(YPPUSH_MADDR);
	}
		
	if (!bind_to_server(master, master_addr, &master_server,
	    &master_prog_vers) ) {
		TRACE("In !bind_to_server");
		xfr_exit(YPPUSH_RPC);
	}

	if (!get_private_recs(&status) ) {
		TRACE("In !get_private_recs");
		xfr_exit(status);
	}
	
	if (!master_version) {

		TRACE("In !master_version");
		if (force) {
			TRACE("In force");
			master_version = &big;
			fake_master_version = TRUE;
		} else {
			logprintf(
    (catgets(nlmsg_fd,NL_SETN,3, "ypxfr:  can't get order number for map %s\n        from server at %s:  use the -f flag\n")),
			  map, master);
			xfr_exit(YPPUSH_FORCE);
		}
	}
	
	if (!move_map(&status) ) {
		TRACE("In !move_map");
		xfr_exit(status);
	}

	if (send_clear && !send_ypclear(&status) ) {
		TRACE("In send_clear");
		xfr_exit(status);
	}

	if (logging) {
		logprintf((catgets(nlmsg_fd,NL_SETN,4, "ypxfr:  transferred map %1$s from %2$s (%3$d entries)\n")),
		    map, master, entry_count);
	}

	TRACE("Before xfr_exit successfull");
	xfr_exit(YPPUSH_SUCC);
}

/*
 * This decides whether we're being run interactively or not, and, if not,
 * whether we're supposed to be logging or not.  If we are logging, it sets
 * up stderr to point to the log file, and sets the "logging"
 * variable.  If there's no logging, the output goes in the bit bucket.
 * Logging output differs from interactive output in the presence of a
 * timestamp, present only in the log file.  stderr is reset, too, because it
 * it's used by various library functions, including clnt_perror.
 */
void
set_output()
{
	if (!isatty(1)) {
		if (access(logfile, W_OK)) {
			(void) freopen("/dev/null", "w", stderr);
			(void) freopen("/dev/null", "w", stdout);
		} else {
			(void) freopen(logfile, "a", stderr);
			(void) freopen(logfile, "a", stdout);
			logging = TRUE;
		}
	}
}

/*
 * This constructs a logging record.
 */
void
logprintf(arg1,arg2,arg3,arg4,arg5,arg6,arg7)
{
	struct timeval t;

	fseek(stderr,0,2);
	if (logging) {
		(void) gettimeofday(&t, NULL);
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "%19.19s: ")), nl_cxtime(&t.tv_sec, ""));
	}
	(void) nl_fprintf(stderr,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	fflush(stderr);
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

	if (argc < 2) {
		usage();
		xfr_exit(YPPUSH_BADARGS);
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'f': {
				force = TRUE;
				argv++;
				break;
			}

			case 'c': {
				send_clear = FALSE;
				argv++;
				break;
			}

			case 'h': {

				if (argc > 1) {
 					argv++;
					argc--;
					master = *argv;
					argv++;

					if (strlen(master) > YPMAXPEER) {
						logprintf(
							(catgets(nlmsg_fd,NL_SETN,57, "ypxfr:  hostname exceeds %d characters\n")),
							YPMAXPEER);
						xfr_exit(YPPUSH_BADARGS);
					}
					
				} else {
					usage();
					xfr_exit(YPPUSH_BADARGS);
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
						logprintf(
							(catgets(nlmsg_fd,NL_SETN,58, "ypxfr:  NIS domain name exceeds %d characters\n")),
							YPMAXDOMAIN);
						xfr_exit(YPPUSH_BADARGS);
					}
					
				} else {
					usage();
					xfr_exit(YPPUSH_BADARGS);
				}
				
				break;
			}

			case 'C': {

				if (argc > 5) {
					callback = TRUE;
					argv++;
					tid = *argv++;
					program = *argv++;
					ipadd = *argv++;
					port = *argv++;
					argc -= 4;
				} else {
					usage();
					xfr_exit(YPPUSH_BADARGS);
				}
				
				break;
			}

			default: {
				usage();
				xfr_exit(YPPUSH_BADARGS);
			}
			
			}
			
		} else {

			if (!map) {
				map = *argv;
				argv++;
			
				if (strlen(map) > YPMAXMAP) {
					logprintf( 
						(catgets(nlmsg_fd,NL_SETN,59, "ypxfr:  mapname exceeds %d characters\n")),
						YPMAXMAP);
					xfr_exit(YPPUSH_BADARGS);
				}
				
			} else {
				usage();
				xfr_exit(YPPUSH_BADARGS);
			}
		}
	}

	if (!map) {
		usage();
		xfr_exit(YPPUSH_BADARGS);
	}
}

/*
 * This checks to see if the master name is an ASCII internet address, in
 * which case it's translated to an internet address here, or is a host
 * name.  In the second case, the standard library routine gethostbyname(3n)
 * (which uses the nis services) is called to do the translation.  
 */
bool
get_master_addr()
{
	struct in_addr tempaddr;
	struct hostent *h;
	bool error = FALSE;

	TRACE("get_master_addr SOP");
	if (master==NULL) {
		/*
		 * if we were unable to get the master name, use the
		 * address of the person who called us.
		 */
	    if (callback) {
	       master_addr.s_addr = inet_addr(ipadd);
	       master = ipadd;
	       return (TRUE);
	    }
	    return (FALSE);
	}

	if (isdigit(*master) ) {
		tempaddr.s_addr = inet_addr(master);

		if ((int) tempaddr.s_addr != -1) {
			master_addr = tempaddr;
		} else {
			error = TRUE;
		}

	} else {

		if (h = gethostbyname(master) ) {
			(void) memcpy((char *) &master_addr, h->h_addr,
			    h->h_length);
		} else {
			error = TRUE;
		}
	}
	
	if (error) {
		logprintf(
		   (catgets(nlmsg_fd,NL_SETN,6, "ypxfr:  can't translate master name %s to an address\n")), master);
		if (callback) {
	 		master_addr.s_addr = inet_addr(ipadd);
	 		master = ipadd;
	       		return (TRUE);
		}
	}
	return (!error);
}

/*
 * This sets up a udp connection to speak the correct program and version
 * to a nis server.  vers is set to one of YPVERS or YPOLDVERS to reflect which
 * language the server speaks.
 */
bool
bind_to_server(host, host_addr, pdomb, vers)
	char *host;
	struct in_addr host_addr;
	struct dom_binding *pdomb;
	unsigned int *vers;
{
	TRACE("bind_to_server SOP");
	if (ping_server(host, host_addr, pdomb, YPVERS)) {
		TRACE("In ping_server");
		*vers = YPVERS;
		TRACE("vers YPVERS");
		return (TRUE);
	} else {
		TRACE("In NOT ping_server");
		if (ping_server(host, host_addr, pdomb, YPOLDVERS)) {
			TRACE("In second ping_server");
			*vers = YPOLDVERS;
			TRACE("vers is YPOLDVERS");
			return (TRUE);
		} else {
			TRACE("In NOT second ping_server");
			return (FALSE);
		}
	}
}

/*
 * This sets up a UDP channel to a server which is assumed to speak an input
 * version of YPPROG.  The channel is tested by pinging the server.  In all
 * error cases except "Program Version Number Mismatch", the error is
 * reported, and in all error cases, the client handle is destroyed and the
 * socket associated with the channel is closed.
 */
bool
ping_server(host, host_addr, pdomb, vers)
	char *host;
	struct in_addr host_addr;
	struct dom_binding *pdomb;
	unsigned int vers;
{
	enum clnt_stat rpc_stat;
	
	pdomb->dom_server_addr.sin_addr = host_addr;
	pdomb->dom_server_addr.sin_family = AF_INET;
	pdomb->dom_server_addr.sin_port = htons((u_short) 0);
	pdomb->dom_server_port = htons((u_short) 0);
	pdomb->dom_socket = RPC_ANYSOCK;

	TRACE("ping_server SOP");
	if (pdomb->dom_client = clntudp_create(&(pdomb->dom_server_addr),
	    YPPROG, vers, udp_intertry, &(pdomb->dom_socket )) ) {
		    
		TRACE("ping_server clntudp_create OK");
		rpc_stat = clnt_call(pdomb->dom_client, YPBINDPROC_NULL,
		    xdr_void, 0, xdr_void, 0, udp_timeout);

		if (rpc_stat == RPC_SUCCESS) {
			TRACE("ping_server rpc_stat SUCCESS");
			return (TRUE);
		} else {
			clnt_destroy(pdomb->dom_client);
			close(pdomb->dom_socket);
			
			if (rpc_stat != RPC_PROGVERSMISMATCH) {
				TRACE("ping_server rpc_stat != PROGVERSMISMATCH");
				(void) clnt_perror(pdomb->dom_client,
				     (catgets(nlmsg_fd,NL_SETN,7, "ypxfr:  bind_to_server clnt_call error")));
			}
			
			return (FALSE);
		}
	} else {
		TRACE("ping_server clntudp_create not OK");
		logprintf((catgets(nlmsg_fd,NL_SETN,8, "ypxfr:  bind_to_server clntudp_create error")));
		(void) clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,9, "")));
		fflush(stderr);
		return (FALSE);
	}
}

/*
 * This gets values for the YP_LAST_MODIFIED and YP_MASTER_NAME keys from the
 * master server's version of the map.  Values are held in static variables
 * here.  In the success cases, global pointer variables are set to point at
 * the local statics.  
 */
bool
get_private_recs(pushstat)
	int *pushstat;
{
	static char anumber[20];
	static unsigned number;
	static char name[YPMAXPEER + 1];
	int status;

	TRACE("get_private_recs SOP");
	status = 0;
	
	if (get_order(anumber, &number, &status) ) {
		TRACE("get_private_recs: get_order OK");
		master_version = &number;
		master_ascii_version = anumber;
	} else {
	TRACE2("get_private_recs: get_order BAD, status is: %d", status);
		if (status != 0) {
			TRACE2("get_private_recs: status is %d", status);
			*pushstat = status;
			TRACE2("get_private_recs: pushstat is %d", *pushstat);
			return (FALSE);
		}
	}

	if (get_master_name(name, &status) ) {
		TRACE("get_private_recs: get_master_name OK");
		master_name = name;
	} else {
	TRACE2("get_private_recs: get_master_name BAD, status is: %d", status);
		if (status != 0) {
		TRACE2("get_private_recs: (get_master) status is %d", status);
			*pushstat = status;
	TRACE2("get_private_recs: (get_master) pushstat is %d", pushstat);
			return (FALSE);
		}
		master_name = master;
	}

#ifdef DBINTERDOMAIN
	if (get_misc_recs(&status)) {
		TRACE2("get_private_recs: Masters map %s an interdomian map.\n",
		       (interdomian_map) ? "is" : "is not");
	} else {
		if (status != 0 ) {
			*pushstat = status;
			TRACE("get_private_recs: Couldn't get state of interdomain flag in map.\n");
			return(FALSE);
		}
	}

#endif /* DBINTERDOMAIN */

	return (TRUE);
}

/*
 * This gets the map's order number from the master server
 */
bool
get_order(an, n, pushstat)
	char *an;
	unsigned *n;
	int *pushstat;
{
	TRACE("get_order SOP");
	if (master_prog_vers == YPVERS) {
		TRACE("get_order YPVERS");
		return (get_v2order(an, n, pushstat) );
	} else {
		TRACE("get_order YPOLDVERS");
		return (get_v1order(an, n, pushstat) );
	}
}

bool
get_v1order(an, n, pushstat)
	char *an;
	unsigned *n;
	int *pushstat;
{
	struct yprequest req;
	struct ypresponse resp;
	bool retval;
	char errmsg[256];

	retval = FALSE;
	req.yp_reqtype = YPMATCH_REQTYPE;
	req.ypmatch_req_domain = domain;
	req.ypmatch_req_map = map;
	req.ypmatch_req_keyptr = yp_last_modified;
	req.ypmatch_req_keysize = (sizeof (yp_last_modified)) - 1;
	
	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

	if(clnt_call(master_server.dom_client, YPOLDPROC_MATCH, _xdr_yprequest,
	    &req, _xdr_ypresponse, &resp, udp_timeout) == RPC_SUCCESS) {

		if (resp.ypmatch_resp_status == YP_TRUE) {
			memcpy(an, resp.ypmatch_resp_valptr,
			    resp.ypmatch_resp_valsize);
			an[resp.ypmatch_resp_valsize] = '\0';
			*n = atoi(an);
			retval = TRUE;
		} else if (resp.ypmatch_resp_status != YP_NOKEY) {
			*pushstat = map_yperr_to_pusherr(
			    resp.ypmatch_resp_status);

				logprintf(
    (catgets(nlmsg_fd,NL_SETN,10, "ypxfr:  can't get order number for map %1$s from ypserv at %2$s\n        reason:  %3$s\n")),
				    map, master, yperr_string(ypprot_err(
				    (unsigned) resp.ypmatch_resp_status)) );
		}

		CLNT_FREERES(master_server.dom_client, _xdr_ypresponse, &resp);
	} else {
		*pushstat = YPPUSH_RPC;
		(void) sprintf(errmsg,
		    (catgets(nlmsg_fd,NL_SETN,11, "ypxfr(get_v1order) RPC call to %s failed")), master);
		clnt_perror(master_server.dom_client, errmsg);
	}

	return(retval);
}

bool
get_v2order(an, n, pushstat)
	char *an;
	unsigned *n;
	int *pushstat;
{
	struct ypreq_nokey req;
	struct ypresp_order resp;
	int retval;
	char errmsg[256];

	TRACE("get_v2order SOP");
	req.domain = domain;
	req.map = map;
	
	/*
	 * Get the map''s order number, null-terminate it and store it,
	 * and convert it to binary and store it again.
	 */
	retval = FALSE;
	
	if((enum clnt_stat) clnt_call(master_server.dom_client,
	    YPPROC_ORDER, xdr_ypreq_nokey, &req, xdr_ypresp_order, &resp,
	    udp_timeout) == RPC_SUCCESS) {

		if (resp.status == YP_TRUE) {
			TRACE("get_v2order: resp.status == YP_TRUE");
			sprintf(an, "%d", resp.ordernum);
			*n = resp.ordernum;
			retval = TRUE;
		} else if (resp.status != YP_BADDB) {
			TRACE2("get_v2order: resp.status = %d", resp.status);
			*pushstat = map_yperr_to_pusherr(resp.status);
			TRACE2("get_v2order: pushstat = %d", *pushstat);
			
				logprintf(
    (catgets(nlmsg_fd,NL_SETN,12, "ypxfr:  can't get order number for map %1$s from ypserv at %2$s\n        reason:  %3$s\n")),
				    map, master, yperr_string(
				    ypprot_err(resp.status)) );
		}

		CLNT_FREERES(master_server.dom_client, xdr_ypresp_order,
		    &resp);
	} else {
		*pushstat = YPPUSH_RPC;
		TRACE2("get_v2order: clnt_call not success, pushstat %d", *pushstat);
		logprintf((catgets(nlmsg_fd,NL_SETN,13, "ypxfr:  get_v2order RPC call to %s failed")), master);
		clnt_perror(master_server.dom_client, (catgets(nlmsg_fd,NL_SETN,14, "")));
	}

	return (retval);
}

#ifdef DBINTERDOMAIN

/*
 * Pick up the state of the YP_INTERDOMAIN records from the
 * master.  Only works on 4.0 V2 masters that will match a YP_ private key  HPNFS
 * when asked to explicitly.
 */
bool
get_misc_recs(pushstat)                                              /* HPNFS */
   	int *pushstat;                                               /* HPNFS */
{                                                                    /* HPNFS */
   	struct ypreq_key req;                                        /* HPNFS */
	struct ypresp_val resp;                                      /* HPNFS */
	int retval;                                                  /* HPNFS */
	char errmsg[256];                                            /* HPNFS */
	
	req.domain = domain;                                         /* HPNFS */
	req.map = map;                                               /* HPNFS */
	
	req.keydat.dptr = yp_interdomain;                            /* HPNFS */
	req.keydat.dsize = yp_interdomain_sz;                        /* HPNFS */
	
	resp.valdat.dptr = NULL;                                     /* HPNFS */
	resp.valdat.dsize = 0;                                       /* HPNFS */
	
	/*                                                              HPNFS
	 * Get the value of the INTERDOMAIN key in the map              HPNFS
	 *                                                              HPNFS */
	
	if ((enum clnt_stat) clnt_call(master_server.dom_client,     /* HPNFS */
	     YPPROC_MATCH, xdr_ypreq_key, &req, xdr_ypresp_val, &resp,  /* HPNFS */
	     udp_timeout) == RPC_SUCCESS) {                          /* HPNFS */
	   	if (resp.status == YP_TRUE) {                        /* HPNFS */
			interdomain_map = TRUE;                      /* HPNFS */
			interdomain_value = (char *)malloc(resp.valdat.dsize+1);  /* HPNFS */
			memcpy(interdomain_value, resp.valdat.dptr,  /* HPNFS */
			       resp.valdat.dsize);                   /* HPNFS */
			*(interdomain_value+resp.valdat.dsize) = '\0';  /* HPNFS */
			interdomain_sz = resp.valdat.dsize;          /* HPNFS */
			retval = TRUE;                               /* HPNFS */
		} else if ((resp.status != YP_NOKEY) &&              /* HPNFS */
			   (resp.status != YP_VERS)) {               /* HPNFS */
		   	*pushstat = ypprot_err(resp.status);         /* HPNFS */
			
			if (!logging) {                              /* HPNFS */
			   	logprintf(                           /* HPNFS */
	"(info) Can't get interdomain flag from ypserv at %s.  Reason: %s.\n",  /* HPNFS */
				   master, yperr_string(             /* HPNFS */
				   ypprot_err(resp.status)) );       /* HPNFS */
			}                                            /* HPNFS */
		}                                                    /* HPNFS */
		
		CLNT_FREERES(master_server.dom_client, xdr_ypresp_val,  /* HPNFS */
		    &resp);                                          /* HPNFS */
	} else {                                                     /* HPNFS */
	   	*pushstat = YPPUSH_RPC;                              /* HPNFS */
		logprintf("ypxfr(get_misc_recs) RPC call to %s failed", master);  /* HPNFS */
		clnt_perror(master_server.dom_client, "");           /* HPNFS */
	}                                                            /* HPNFS */
	
	
	return (retval);                                             /* HPNFS */
}                                                                    /* HPNFS */

#endif /* DBINTERDOMAIN */

/*
 * This gets the map's master name from the master server
 */
bool
get_master_name(name, pushstat)
	char *name;
	int *pushstat;
{
	if (master_prog_vers == YPVERS) {
		return (get_v2master_name(name, pushstat));
	} else {
		return (get_v1master_name(name, pushstat));
	}
}

bool
get_v1master_name(name, pushstat)
	char *name;
	int *pushstat;
{
	struct yprequest req;
	struct ypresponse resp;
	bool retval;
	char errmsg[256];

	retval = FALSE;
	req.yp_reqtype = YPMATCH_REQTYPE;
	req.ypmatch_req_domain = domain;
	req.ypmatch_req_map = map;
	req.ypmatch_req_keyptr = yp_master_name;
	req.ypmatch_req_keysize = (sizeof (yp_master_name)) -1;
	
	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

	if(clnt_call(master_server.dom_client, YPOLDPROC_MATCH, _xdr_yprequest,
	    &req, _xdr_ypresponse, &resp, udp_timeout) == RPC_SUCCESS) {

		if (resp.ypmatch_resp_status == YP_TRUE) {
			memcpy(name, resp.ypmatch_resp_valptr, 
			    resp.ypmatch_resp_valsize);
			name[resp.ypmatch_resp_valsize] = '\0';
			retval = TRUE;
		} else if (resp.ypmatch_resp_status != YP_NOKEY) {
			*pushstat = map_yperr_to_pusherr(
			    resp.ypmatch_resp_status);

			logprintf(
        (catgets(nlmsg_fd,NL_SETN,15, "ypxfr:  can't get master name for map %1$s from ypserv at %2$s\n        reason:  %3$s\n")),
				    map, master, 
	yperr_string(ypprot_err((unsigned) resp.ypmatch_resp_status)) );
		}
		
		CLNT_FREERES(master_server.dom_client, _xdr_ypresponse, &resp);
	} else {
		*pushstat = YPPUSH_RPC;
		(void) sprintf(errmsg,
		   (catgets(nlmsg_fd,NL_SETN,16, "ypxfr(get_v1master_name) RPC call to %s failed")), master);
		clnt_perror(master_server.dom_client, errmsg);
	}

	return(retval);
}

bool
get_v2master_name(name, pushstat)
	char *name;
	int *pushstat;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	int retval;
	char errmsg[256];

	req.domain = domain;
	req.map = map;
	resp.master = NULL;
	retval = FALSE;
	
	if((enum clnt_stat) clnt_call(master_server.dom_client,
	    YPPROC_MASTER, xdr_ypreq_nokey, &req, xdr_ypresp_master, &resp,
	    udp_timeout) == RPC_SUCCESS) {

		if (resp.status == YP_TRUE) {
			strcpy(name, resp.master);
			retval = TRUE;
		} else if (resp.status != YP_BADDB) {
			*pushstat = map_yperr_to_pusherr(resp.status);

				logprintf(
(catgets(nlmsg_fd,NL_SETN,17, "ypxfr:  can't get master name for map %1$s from ypserv at %2$s\n        reason:  %3$s\n")),
				    map, master, yperr_string(
				    ypprot_err(resp.status)) );
		}
	
		CLNT_FREERES(master_server.dom_client, xdr_ypresp_master,
		    &resp);
	} else {
		*pushstat = YPPUSH_RPC;
		logprintf(
		   (catgets(nlmsg_fd,NL_SETN,18, "ypxfr:  get_v2master_name RPC call to %s failed")), master);
		clnt_perror(master_server.dom_client, (catgets(nlmsg_fd,NL_SETN,19, "")));
	}

	return (retval);
}

/*
 * This tries to get the master name for the named map, from any
 * server's version, using the vanilla nis client interface.  If we get a
 * name back, the global "master" gets pointed to it.
 */
void
find_map_master()
{
	int err;
		
	if (err = yp_master(domain, map, &master)) {
		logprintf((catgets(nlmsg_fd,NL_SETN,20, "ypxfr:  can't get master of %1$s\n        reason:  %2$s\n")), map,
		    yperr_string(err));
	}
	
	yp_unbind(domain);
}

/*
 * This does the work of transferring the map.
 */
bool
move_map(pushstat)
	int *pushstat;
{
	unsigned local_version;
/*  HPNFS
 *
 *  MAXNAMLEN should be MAXPATHLEN of <sys/param.h>.
 *  Dave Erickson, 3-10-87.
 *
 *  HPNFS
 */
	char map_name[MAXPATHLEN + 1];
	char tmp_name[MAXPATHLEN + 1];
	char bkup_name[MAXPATHLEN + 1];
	char an[11];
	unsigned n;

/*  HPNFS
 *
 *  The mkfilename() function now returns an integer value.
 *  Dave Erickson, 3-20-87.
 *
 *  HPNFS
 */
	switch (mkfilename(map_name)) {
		case PATH_TOO_LONG:		/*  A rare (if ever) event!  */
			logprintf((catgets(nlmsg_fd,NL_SETN,21, "ypxfr:  the map pathname, \"%1$s...\",\n        exceeds %2$d characters\n")),
					ypdbpath, MAXPATHLEN);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		case MAPNAME_TOO_LONG:
			logprintf((catgets(nlmsg_fd,NL_SETN,22, "ypxfr:  the name of the non-standard map, \"%1$s\",\n        should not exceed %2$d characters, if this machine is to be an NIS server.\n")),
					map, max_length - 4);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		case MAPNAME_OK: ;		/*  "map_name" is OK.  */
	}

	if (!force) {
		local_version = get_local_version(map_name);

		if (local_version >= *master_version) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,23, "ypxfr:  map %1$s at %2$s is not more recent than local\n")),
			    map, master);
			*pushstat = YPPUSH_AGE;
			return (FALSE);
		}
	}

	mk_tmpname(tmp_name);

	if (!new_mapfiles(tmp_name) ) {
		logprintf(
		    (catgets(nlmsg_fd,NL_SETN,24, "ypxfr:  can't create temp map %s\n")), tmp_name);
		*pushstat = YPPUSH_FILE;
		return (FALSE);
	}

	if (dbminit(tmp_name) < 0) {
		logprintf(
		    (catgets(nlmsg_fd,NL_SETN,25, "ypxfr:  can't dbm init temp map %s\n")), tmp_name);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	if (!get_map(tmp_name, pushstat) ) {
		del_mapfiles(tmp_name);
		return (FALSE);
	}

	if (!add_private_entries(tmp_name) ) {
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	if (dbmclose(tmp_name) < 0) {
		logprintf(
		    (catgets(nlmsg_fd,NL_SETN,26, "ypxfr:  can't dbm close temp map %s\n")),
		    tmp_name);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	if (!get_order(an, &n, pushstat)) {
		return(FALSE);
	}
	if (n != *master_version) {
		logprintf(
		    (catgets(nlmsg_fd,NL_SETN,27, "ypxfr:  version skew at %1$s while transferring map %2$s\n")),
		    master, map);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_SKEW;
		return (FALSE);
	}

# ifdef PARANOID
	if (!count_mismatch(tmp_name,entry_count)) {
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return(FALSE);
	}
# endif PARANOID

	if (!check_map_existence(map_name) ) {

		if (!rename_map(tmp_name, map_name) ) {
			del_mapfiles(tmp_name);
			logprintf(
			  (catgets(nlmsg_fd,NL_SETN,28, "ypxfr:  rename error:  couldn't mv %1$s to %2$s\n")),
			    tmp_name, map_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}
		
	} else {
		mk_tmpname(bkup_name);
	
		if (!rename_map(map_name, bkup_name) ) {
			(void) rename_map(bkup_name, map_name);
			logprintf(
			  (catgets(nlmsg_fd,NL_SETN,29, "ypxfr:  rename error:  check that old %s is still intact\n")),
			    map_name);
			del_mapfiles(tmp_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}

		if (rename_map(tmp_name, map_name) ) {
			del_mapfiles(bkup_name);
		} else {
			del_mapfiles(tmp_name);
			(void) rename_map(bkup_name, map_name);
				logprintf(
			  (catgets(nlmsg_fd,NL_SETN,30, "ypxfr:  rename error:  check that old %s is still intact\n")),
			    map_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}
	}

	return (TRUE);
}

/*
 * This tries to get the order number out of the local version of the map.
 * If the attempt fails for any version, the function will return "0"
 */
unsigned
get_local_version(name)
	char *name;
{
	datum key;
	datum val;
	char number[11];
	
	if (!check_map_existence(name) ) {
		return (0);
	}

	if (dbminit(name) < 0) {
		return (0);
	}
		
	key.dptr = yp_last_modified;
	key.dsize = (sizeof (yp_last_modified) - 1);
	val = fetch(key);

	if (!val.dptr) {
		return (0);
	}

	if (val.dsize == 0 || val.dsize > 10) {
		return (0);
	}

	(void) memcpy(number, val.dptr, val.dsize);
	number[val.dsize] = '\0';

#ifdef DBINTERDOMAIN

	/*                                                              HPNFS
	 * Now check to see if interdomain requests are made of the     HPNFS
	 * local map.  Keep the value around if the are.                HPNFS
	 *                                                              HPNFS */
	
	if (!interdomain_map) {                                      /* HPNFS */
	   	key.dptr = yp_interdomain;                           /* HPNFS */
		key.dsize = yp_interdomain_sz;                       /* HPNFS */
		val = fetch(key);                                    /* HPNFS */
		if (interdomain_map = (val.dptr != NULL)) {          /* HPNFS */
		   	interdomain_value = (char *)malloc(val.dsize+1);  /* HPNFS */
			(void)memcpy(interdomain_value, val.dptr, val.dsize);  /* HPNFS */
			*(interdomain_value+val.dsize) = '\0';       /* HPNFS */
			interdomain_sz = val.dsize;                  /* HPNFS */
		}                                                    /* HPNFS */
	}                          
#endif /* DBINTERDOMAIN */
	(void) dbmclose(name);

	return ((unsigned) atoi(number) );

}


/*  HPNFS
 *
 *  The mkfilename() function now returns an integer value indicating
 *  either success (MAPNAME_OK), the full pathname is too long
 *  (PATH_TOO_LONG) or the requested map exceeds ten characters (which
 *  implies it is a non-standard mapname) - MAPNAME_TOO_LONG.
 *
 *  DIRSIZ_CONSTANT-4 is the length limit to mapnames in short filename
 *  filesystems, because the dbm routines add the four-character suffixes 
 *  and ".pag" to any mapname, and DIRSIZ_CONSTANT defines the filename 
 *  ".dir" length limit.  
 *  Filesystems can support long filenames and the limit is defined
 *  by a variable called max_length that contains the result of a read of the
 *  superblock. (chm)
 *  Dave Erickson, 3-20-87.
 *
 *  HPNFS
 */
int mkfilename(ppath)
	char *ppath;
{
	char *temp_mapname;

	TRACE("mkfilename SOP");
	if ((strlen(domain) + strlen(map) + strlen(ypdbpath) + 2) > MAXPATHLEN)
		return (PATH_TOO_LONG);

#ifdef hpux
	switch (max_length = max_file_length(ypdbpath))
	{
		case MAXNAMLEN: 
			temp_mapname = map; 
			break;
		case DIRSIZ_CONSTANT:
			temp_mapname = translate_mapname(map, HP_mapname);
			break;
		default:
			fprintf(stderr,"ypxfr: Unable to determine filename length allowed in directory %s\n", ypdbpath);
			exit(-1);
	}
	TRACE2("mkfilename max_length is %d", max_length);

#else not hpux

	temp_mapname = translate_mapname(map, HP_mapname);

#endif hpux

	if (strlen(temp_mapname) > max_length - 4)
	{
		TRACE("mkfilename: about to return MAPNAME_TOO_LONG");
		return (MAPNAME_TOO_LONG);
	}

	(void) sprintf(ppath, "%s/%s/%s", ypdbpath, domain, temp_mapname);
	return (MAPNAME_OK);
}

/*
 * This returns a temporary name for a map transfer minus its ".dir" or
 * ".pag" extensions.
 */
void
mk_tmpname(xfr_name)
	char *xfr_name;
{
	char ypdir[MAXPATHLEN + 1];

	sprintf(ypdir, "%s/%s", ypdbpath, domain);
	strcpy(xfr_name, tempnam(ypdir, ""));
}

/*
 * This deletes the .pag and .dir files which implement a map.
 *
 * Note:  No error checking is done here for a garbage input file name or for
 * failed unlink operations.
 */
void
del_mapfiles(basename)
	char *basename;
{
/*  HPNFS
 *
 *  MAXNAMLEN should be MAXPATHLEN of <sys/param.h>.
 *  Dave Erickson, 3-10-87.
 *
 *  HPNFS
 */
	char dbfilename[MAXPATHLEN + 1];

	if (!basename) {
		return;
	}
	
	strcpy(dbfilename, basename);
	strcat(dbfilename, ".pag");
	unlink(dbfilename);
	strcpy(dbfilename, basename);
	strcat(dbfilename, ".dir");
	unlink(dbfilename);
}

/*
 * This checks to see if the source map files exist, then renames them to the
 * target names.  This is a boolean function.  The file names from.pag and
 * from.dir will be changed to to.pag and to.dir in the success case.
 *
 * Note:  If the second of the two renames fails, yprename_map will try to
 * un-rename the first pair, and leave the world in the state it was on entry.
 * This might fail, too, though...
 */
bool
rename_map(from, to)
	char *from;
	char *to;
{
	char fromfile[MAXPATHLEN + 1];
	char tofile[MAXPATHLEN + 1];

	if (!from || !to) {
		return (FALSE);
	}
	
	if (!check_map_existence(from) ) {
		return (FALSE);
	}
	
	(void) strcpy(fromfile, from);
	(void) strcat(fromfile, ".pag");
	(void) strcpy(tofile, to);
	(void) strcat(tofile, ".pag");
	
	if (rename(fromfile, tofile) ) {
		logprintf( (catgets(nlmsg_fd,NL_SETN,31, "ypxfr:  can't mv %1$s to %2$s\n")), fromfile,
		    tofile);
		return (FALSE);
	}
	
	(void) strcpy(fromfile, from);
	(void) strcat(fromfile, ".dir");
	(void) strcpy(tofile, to);
	(void) strcat(tofile, ".dir");
	
	if (rename(fromfile, tofile) ) {
		logprintf( (catgets(nlmsg_fd,NL_SETN,32, "ypxfr:  can't mv %1$s to %2$s\n")), fromfile,
		    tofile);
		(void) strcpy(fromfile, from);
		(void) strcat(fromfile, ".pag");
		(void) strcpy(tofile, to);
		(void) strcat(tofile, ".pag");
		
		if (rename(tofile, fromfile) ) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,33, "ypxfr:  can't recover from rename failure\n")));
			return (FALSE);
		}
		
		return (FALSE);
	}
	
	return (TRUE);
}

/*
 * This performs an existence check on the dbm data base files <pname>.pag and
 * <pname>.dir.
 */
bool
check_map_existence(pname)
	char *pname;
{
/*  HPNFS
 *
 *  MAXNAMLEN should be MAXPATHLEN of <sys/param.h>.
 *  Dave Erickson, 3-10-87.
 *
 *  HPNFS
 */
	char dbfile[MAXPATHLEN + 1];
	struct stat filestat;
	int len;

	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + 5) > (MAXPATHLEN + 1) ) {
		return (FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, ".dir");

	if (stat(dbfile, &filestat) != -1) {
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, ".pag");

		if (stat(dbfile, &filestat) != -1) {
			return (TRUE);
		} else {

			if (errno != ENOENT) {
				logprintf(
				    (catgets(nlmsg_fd,NL_SETN,43, "ypxfr:  stat error on map file %s\n")),
				    dbfile);
			}

			return (FALSE);
		}

	} else {

		if (errno != ENOENT) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,35, "ypxfr:  stat error on map file %s\n")),
			    dbfile);
		}

		return (FALSE);
	}
}

/*
 * This creates <pname>.dir and <pname>.pag
 */
bool
new_mapfiles(pname)
	char *pname;
{
/*  HPNFS
 *
 *  MAXNAMLEN should be MAXPATHLEN of <sys/param.h>.
 *  Dave Erickson, 3-10-87.
 *
 *  HPNFS
 */
	char dbfile[MAXPATHLEN + 1];
	int f;
	int len;

	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + 5) > (MAXPATHLEN + 1) ) {
		return (FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, ".dir");

	if ((f = open(dbfile, (O_WRONLY | O_CREAT | O_TRUNC), 0644)) >= 0) {
		(void) close(f);
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, ".pag");

		if ((f = open(dbfile, (O_WRONLY | O_CREAT | O_TRUNC),
		    0644)) >= 0) {
			(void) close(f);
			return (TRUE);
		} else {
			return (FALSE);
		}

	} else {
		return (FALSE);
	}
}

/* HPNFS count_callback is only called within an ifdef REALLY_PARANOID in    */
/* HPNFS count_mismatch.  So to help with BFA coverage we put count_callback */
/* HPNFS in the same ifdef.						     */

# ifdef REALLY_PARANOID
count_callback(status) 
	int status;
{
	if (status != YP_TRUE) {
		
		if (status != YP_NOMORE) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,36, "ypxfr:  error from ypserv on %1$s\n        ypall_callback = %2$s\n")),
			    master, yperr_string(ypprot_err(status)));
		}
		
		return(TRUE);
	}

	entry_count++;
	return(FALSE);
}
# endif REALLY_PARANOID

/*
 * This counts the entries in the dbm file after the transfer to
 * make sure that the dbm file was built correctly.  
 * Returns TRUE if everything is OK, FALSE if they mismatch.
 */
count_mismatch(pname,oldcount)
	char *pname;
	int oldcount;
{
	datum key, value;
	struct dom_binding domb;
	enum clnt_stat s;
	struct ypreq_nokey allreq;
	struct ypall_callback cbinfo;

    entry_count = 0;
    dbminit(pname);
    for (key = firstkey(); key.dptr != NULL; key = nextkey(key))
	entry_count++;
    dbmclose(pname);

    if (oldcount != entry_count) {
	logprintf( 
		  (catgets(nlmsg_fd,NL_SETN,37, "ypxfr:  count mismatch in dbm file %1$s:\n        old=%2$d, new=%3$d\n")),
		    map, oldcount, entry_count);
	return(FALSE);
    }

# ifdef REALLY_PARANOID
	domb.dom_server_addr.sin_addr = master_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = htons((u_short) 0);
	domb.dom_server_port = htons((u_short) 0);
	domb.dom_socket = RPC_ANYSOCK;

	if ((domb.dom_client = clnttcp_create(&(domb.dom_server_addr),
	    YPPROG, master_prog_vers, &(domb.dom_socket), 0, 0)) ==
	    (CLIENT *) NULL) {
		clnt_pcreateerror(
		    (catgets(nlmsg_fd,NL_SETN,38, "ypxfr (mismatch) - TCP channel create failure")));
		return(FALSE);
	}

	if (master_prog_vers == YPVERS) {
		int tmpstat;

		allreq.domain = domain;
		allreq.map = map;
		cbinfo.foreach = count_callback;
		tmpstat = 0;
		cbinfo.data = (char *) &tmpstat;
	
		entry_count = 0;
		s = clnt_call(domb.dom_client, YPPROC_ALL, xdr_ypreq_nokey,
		    &allreq, xdr_ypall, &cbinfo, tcp_timeout);

		if (tmpstat == 0) {
			if (s == RPC_SUCCESS) {
			} else {
				clnt_perror(domb.dom_client,
			   (catgets(nlmsg_fd,NL_SETN,39, "ypxfr (get_map/all) - RPC clnt_call (TCP) failure")));
		    return(FALSE);
			}
			
		} else {
		    return(FALSE);
		}
		
	} else {
	    logprintf((catgets(nlmsg_fd,NL_SETN,40, "ypxfr:  wrong version number!\n")));
	    return(FALSE);
	}
    clnt_destroy(domb.dom_client);
    close(domb.dom_socket);
    entry_count += 2;			/* add in YP_entries */
    if (oldcount != entry_count) {
	logprintf(
	(catgets(nlmsg_fd,NL_SETN,41, "ypxfr:  count mismatch after enumerating %1$s:\n        old=%2$d, new=%3$d\n")),
		    map, oldcount, entry_count);
	return(FALSE);
    }
# endif REALLY_PARANOID

    return(TRUE);
}

/*
 * This sets up a TCP connection to the master server, and either gets
 * ypall_callback to do all the work of writing it to the local dbm file
 * (if the ypserv is current version), or does it itself for an old ypserv.  
 */
bool
get_map(pname, pushstat)
	char *pname;
	int *pushstat;
{
	struct dom_binding domb;
	enum clnt_stat s;
	struct ypreq_nokey allreq;
	struct ypall_callback cbinfo;
	struct yprequest oldreq;
	struct ypresponse resp;
	bool retval = FALSE;
	int tmpstat;
	
	domb.dom_server_addr.sin_addr = master_addr;
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = htons((u_short) 0);
	domb.dom_server_port = htons((u_short) 0);
	domb.dom_socket = RPC_ANYSOCK;

	if ((domb.dom_client = clnttcp_create(&(domb.dom_server_addr),
	    YPPROG, master_prog_vers, &(domb.dom_socket), 0, 0)) ==
	    (CLIENT *) NULL) {
		clnt_pcreateerror(
			(catgets(nlmsg_fd,NL_SETN,42, "ypxfr (get_map) - TCP channel create failure")));
		*pushstat = YPPUSH_RPC;
		return(FALSE);
	}

	entry_count = 0;
	if (master_prog_vers == YPVERS) {
		allreq.domain = domain;
		allreq.map = map;
		cbinfo.foreach = ypall_callback;
		tmpstat = 0;
		cbinfo.data = (char *) &tmpstat;
	
		s = clnt_call(domb.dom_client, YPPROC_ALL, xdr_ypreq_nokey,
		    &allreq, xdr_ypall, &cbinfo, tcp_timeout);

		if (tmpstat == 0) {
			
			if (s == RPC_SUCCESS) {
				retval = TRUE;
			} else {
				clnt_perror(domb.dom_client,
			   (catgets(nlmsg_fd,NL_SETN,43, "ypxfr (get_map/all) - RPC clnt_call (TCP) failure")));
				*pushstat = YPPUSH_RPC;
			}
			
		} else {
			*pushstat = tmpstat;
		}
		
	} else {
		datum inkey, inval;

		oldreq.yp_reqtype = YPFIRST_REQTYPE;
		oldreq.ypfirst_req_domain = domain;
		oldreq.ypfirst_req_map = map;
	
		resp.ypfirst_resp_keyptr = NULL;
		resp.ypfirst_resp_keysize = 0;
		resp.ypfirst_resp_valptr = NULL;
		resp.ypfirst_resp_valsize = 0;
	
		if((s = clnt_call(domb.dom_client, YPOLDPROC_FIRST,
		    _xdr_yprequest, &oldreq, _xdr_ypresponse,
		    &resp, tcp_timeout)) != RPC_SUCCESS) {
			clnt_perror(domb.dom_client,
			 (catgets(nlmsg_fd,NL_SETN,44, "ypxfr (get_map/first) - RPC clnt_call (TCP) failure")));
			*pushstat = YPPUSH_RPC;
			goto cleanup;
		}

		if (resp.ypfirst_resp_status != YP_TRUE) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,45, "ypxfr:  error from ypserv on %1$s\n        get first = %2$s\n")),
			    master, yperr_string(ypprot_err(
			    resp.ypfirst_resp_status)));
			*pushstat = YPPUSH_RPC;
			goto cleanup;
		}

		inkey = resp.ypfirst_resp_keydat;
		inval = resp.ypfirst_resp_valdat;
	
		/*
		 * Continue to get the next entries in the map as long as
		 * there are no errors, and there are entries remaining.  
		 */
		oldreq.yp_reqtype = YPNEXT_REQTYPE;
		oldreq.ypnext_req_domain = domain;
		oldreq.ypnext_req_map = map;

		while (TRUE) {
			if (strncmp(inkey.dptr,"YP_",3)) {
				if (store(inkey, inval) < 0) {
					logprintf(
				    (catgets(nlmsg_fd,NL_SETN,46, "ypxfr:  can't do dbm store into temp map %s\n")),
				    	    pname);
					*pushstat = YPPUSH_DBM;
					goto cleanup;
				}
				entry_count++;
			}
			CLNT_FREERES(domb.dom_client, _xdr_ypresponse, &resp);

			oldreq.ypnext_req_keydat = inkey;
			resp.ypnext_resp_keydat.dptr = NULL;
			resp.ypnext_resp_valdat.dptr = NULL;
			resp.ypnext_resp_keydat.dsize = 0;
			resp.ypnext_resp_valdat.dsize = 0;
	
			if ((s = clnt_call(domb.dom_client, YPOLDPROC_NEXT,
		    	    _xdr_yprequest, &oldreq, _xdr_ypresponse,
		    	    &resp, tcp_timeout)) != RPC_SUCCESS) {
				clnt_perror(domb.dom_client,
			  (catgets(nlmsg_fd,NL_SETN,47, "ypxfr (get_map/next) - RPC clnt_call (TCP) failure")));
				*pushstat = YPPUSH_RPC;
				break;
			}

			if (resp.ypnext_resp_status != YP_TRUE) {

				if (resp.ypnext_resp_status == YP_NOMORE) {
					retval = TRUE;
				} else {
					logprintf(
			    (catgets(nlmsg_fd,NL_SETN,48, "ypxfr:  error from ypserv on %1$s\n        get next = %2$d\n")),
			    		    master, yperr_string(ypprot_err(
			    resp.ypnext_resp_status)));
					*pushstat = YPPUSH_RPC;
				}
				
				break;
			}

			inkey = resp.ypnext_resp_keydat;
			inval = resp.ypnext_resp_valdat;
		}
	}
cleanup:
	clnt_destroy(domb.dom_client);
	close(domb.dom_socket);
	return (retval);
}

/*
 * This sticks each key-value pair into the current map.  It returns FALSE as
 * long as it wants to keep getting called back, and TRUE on error conditions
 * and "No more k-v pairs".
 */
int
ypall_callback(status, key, kl, val, vl, pushstat)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	int *pushstat;
{
	datum keydat;
	datum valdat;
	datum test;

	if (status != YP_TRUE) {
		
		if (status != YP_NOMORE) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,49, "ypxfr:  error from ypserv on %1$s\n        ypall_callback = %2$s\n")),
			    master, yperr_string(ypprot_err(status)));
			*pushstat = map_yperr_to_pusherr(status);
		}
		
		return(TRUE);
	}

	keydat.dptr = key;
	keydat.dsize = kl;
	valdat.dptr = val;
	valdat.dsize = vl;
	entry_count++;

# ifdef PARANOID
	test = fetch(keydat);
	if (test.dptr!=NULL) {
		logprintf((catgets(nlmsg_fd,NL_SETN,50, "ypxfr:  duplicate key %1$s in map %2$s\n")),key,map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# endif PARANOID
	if (store(keydat, valdat) < 0) {
		logprintf(
		    (catgets(nlmsg_fd,NL_SETN,51, "ypxfr:  can't do dbm store into temp map %s\n")),map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# ifdef PARANOID
	test = fetch(keydat);
	if (test.dptr==NULL) {
		logprintf((catgets(nlmsg_fd,NL_SETN,52, "ypxfr:  key %1$s was not inserted into dbm file %2$s\n")),
			key,map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# endif PARANOID
	return(FALSE);
}

/*
 * This maps a YP_xxxx error code into a YPPUSH_xxxx error code
 */
int
map_yperr_to_pusherr(yperr)
	int yperr;
{
	int reason;

	switch (yperr) {
	
 	case YP_NOMORE:
		reason = YPPUSH_SUCC;
		break;

 	case YP_NOMAP:
		reason = YPPUSH_NOMAP;
		break;

 	case YP_NODOM:
		reason = YPPUSH_NODOM;
		break;

 	case YP_NOKEY:
		reason = YPPUSH_YPERR;
		break;

 	case YP_BADARGS:
		reason = YPPUSH_BADARGS;
		break;

 	case YP_BADDB:
		reason = YPPUSH_YPERR;
		break;

	default:
		reason = YPPUSH_XFRERR;
		break;
	}
	
  	return(reason);
}

/*
 * This writes the last-modified and master entries into the new dbm file
 */
bool
add_private_entries(pname)
	char *pname;
{
	datum key;
	datum val;

	if (!fake_master_version) {
		key.dptr = yp_last_modified;
		key.dsize = sizeof (yp_last_modified) - 1;
		val.dptr = master_ascii_version;
		val.dsize = strlen(master_ascii_version);

		if (store(key, val) < 0) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,53, "ypxfr:  can't do dbm store into temp map %s\n")),
		    	    pname);
			return (FALSE);
		}
		entry_count++;
	}
	
	if (master_name) {
		key.dptr = yp_master_name;
		key.dsize = sizeof (yp_master_name) - 1;
		val.dptr = master_name;
		val.dsize = strlen(master_name);
		if (store(key, val) < 0) {
			logprintf(
			    (catgets(nlmsg_fd,NL_SETN,54, "ypxfr:  can't do dbm store into temp map %s\n")),
		    	    pname);
			return (FALSE);
		}
		entry_count++;
	}
	
#ifdef DBINTERDOMAIN

	if (interdomain_map) {                                       /* HPNFS */
	   	key.dptr = yp_interdomain;                           /* HPNFS */
		key.dsize = yp_interdomain_sz;                       /* HPNFS */
		val.dptr = interdomain_value;                        /* HPNFS */
		val.dsize = interdomain_sz;                          /* HPNFS */
		if (store(key, val) < 0) {                           /* HPNFS */
		   	logprintf(                                   /* HPNFS */
			    "Can't do dbm store into temp map %s.\n",/* HPNFS */
			    pname);                                  /* HPNFS */
			return(FALSE);                               /* HPNFS */
		}                                                    /* HPNFS */
		entry_count++;                                       /* HPNFS */
	}                                                            /* HPNFS */

#endif /* DBINTERDOMAIN */

	return (TRUE);
}
	
	
/*
 * This sends a YPPROC_CLEAR message to the local ypserv process.  If the
 * local ypserv is a v.1 protocol guy, we'll just say we succeeded.  Such a
 * situation is outlandish - why are they running the old ypserv at the same
 * place they are running ypxfr?  And who are they, anyway?
 */
bool
send_ypclear(pushstat)
	int *pushstat;
{
	struct sockaddr_in myaddr;
	struct dom_binding domb;
	char local_host_name[UTSLEN];
	unsigned int progvers;

	get_myaddress(&myaddr);

	/* 8.0 change - use loopbackaddr - prabha */
	/*
	 * myaddr.sin_family = AF_INET;
	 * myaddr.sin_addr.s_addr = INADDR_LOOPBACK;
	 */
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
	if (gethostname(local_host_name, sizeof(local_host_name)) < 0)
#else  hp9000s300
	if (gethostname(local_host_name, sizeof(local_host_name)))
#endif hp9000s300
	{
		logprintf((catgets(nlmsg_fd,NL_SETN,55, "ypxfr:  gethostname system call failed\n")));
		*pushstat = YPPUSH_RSRC;
		return (FALSE);
	}

	if (!bind_to_server(local_host_name, myaddr.sin_addr, &domb,
	    &progvers) ) {
		*pushstat = YPPUSH_CLEAR;
		return (FALSE);
	}

	if (progvers == YPOLDVERS)
		return (TRUE);

	if((enum clnt_stat) clnt_call(domb.dom_client,
	    YPPROC_CLEAR, xdr_void, 0, xdr_void, 0,
	    udp_timeout) != RPC_SUCCESS) {
		logprintf(
		(catgets(nlmsg_fd,NL_SETN,56, "ypxfr:  can't send ypclear message to ypserv on local machine\n")));
		xfr_exit(YPPUSH_CLEAR);
	}

	return (TRUE);
}

/*
 * This decides if send_callback has to get called, and does the process exit.
 */
void
xfr_exit(status)
	int status;
{
	TRACE("In xfr_exit");
	if (callback) {
		TRACE2("Before send_callback, status is %d", status);
		send_callback(&status);
	}

	if (status == YPPUSH_SUCC) {
		exit(0);
	} else {
		exit(1);
	}
}

/*
 * This sets up a UDP connection to the yppush process which contacted our
 * parent ypserv, and sends him a status on the requested transfer.
 */
void
send_callback(status)
	int *status;
{
	struct yppushresp_xfr resp;
	struct dom_binding domb;

	TRACE2("In send_callback, status is %d", *status);
	resp.transid = (unsigned long) htonl(atoi(tid));
	resp.status = (unsigned long) htonl(*status);
	TRACE2("send_callback: resp.status is %d", resp.status);

	domb.dom_server_addr.sin_addr.s_addr = inet_addr(ipadd);
	domb.dom_server_addr.sin_family = AF_INET;
	domb.dom_server_addr.sin_port = (unsigned short) htons(atoi(port));
	domb.dom_server_port = domb.dom_server_addr.sin_port;
	domb.dom_socket = RPC_ANYSOCK;

	udp_intertry.tv_sec = CALLINTER_TRY;
	udp_timeout.tv_sec = CALLTIMEOUT;

	if ((domb.dom_client = clntudp_create(&(domb.dom_server_addr),
	    (unsigned long) htons(atoi(program)), YPPUSHVERS, 
	    udp_intertry, &(domb.dom_socket) ) ) == NULL) {
		*status = YPPUSH_RPC;
		return;
	}	

	TRACE2("send_callback: resp.status is %d", resp.status);	
	if((enum clnt_stat) clnt_call(domb.dom_client,
	    YPPUSHPROC_XFRRESP, xdr_yppushresp_xfr, &resp, xdr_void, 0, 
	    udp_timeout) != RPC_SUCCESS) {
		*status = YPPUSH_RPC;
		return;
	} 
	TRACE2("send_callback: resp.status is %d", resp.status);
}

void
usage()
{
	logprintf((catgets(nlmsg_fd,NL_SETN,60, "Usage:  ypxfr [-h host] [-f] [-d domain] [-c] [-C tid prog ipaddr port] mapname\n\nwhere\thost is either a name or an internet address of the form ww.xx.yy.zz\n\t-f forces transfer even if the master's copy is not newer\n\t-c inhibits sending a \"Clear map\" message to the local ypserv\n\t-C is used by ypserv to pass callback information\n")));
}
