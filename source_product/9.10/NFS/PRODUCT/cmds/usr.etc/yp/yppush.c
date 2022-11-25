#ifndef lint
static  char rcsid[] = "@(#)yppush:	$Revision: 1.39.109.1 $	$Date: 91/11/19 14:21:20 $  ";
#endif
/* yppush.c	2.1 86/04/16 NFSSRC */ 
/*static char sccsid[] = "yppush.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <sys/utsname.h>
#include <sys/types.h>

#define INTER_TRY 12			/* Seconds between tries */
#define TIMEOUT INTER_TRY*4		/* Total time for timeout */

/*  HPNFS
 *  The total time we will wait for ypxfrs to complete was changed
 *  from 40 to 80 as per a pre-3.2 change. Dave Erickson, 3-18-87.
 *  HPNFS
 */

#define GRACE_PERIOD 80

/* use a back off strategy to accomodate busy networks - To do this, we
 * define a max time we'll wait for acks - determined based on the worst
 * case network we know of. Then on each timeout, we hike the timeout
 * intervel by one min_timeout unit. - prabha */

#define MAX_TIMEOUT  480

/* define the min function to calculate the timeout intervel. */
#define min(a,b)	((a) < (b)) ? (a) : (b)

bool callback_timeout = FALSE;	/* set when a callback times out */

/*  the following variables added for parallel transfers - prabha */
int max_parallel	= 1;
int servers_called	= 0;
int acks_recvd		= 0;
int min_timeout 	= GRACE_PERIOD;
int timeout_intervel;
int countof_timeouts	= 0;

#ifdef DEBUG
time_t timenow = 0;	/* to register start and finish times. */
#endif DEBUG


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
bool verbose = FALSE;

struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
};
SVCXPRT *transport;
struct server {
	struct server *pnext;
/*  HPNFS
 *
 *  The array should be big enough to hold the terminating null, too.
 *  Dave Erickson, 3-17-87.
 *
 *  HPNFS
 */
	char name[YPMAXPEER+1];
	struct dom_binding domb;
	unsigned long xactid;
	unsigned short state;
	unsigned long status;
#ifdef DEBUG
	time_t start_time;
	time_t finish_time;
#endif DEBUG
	bool oldvers;
};
struct server *server_list = (struct server *) NULL;

/*  State values for server.state field */

#define SSTAT_INIT		0
#define SSTAT_CALLED		1
#define SSTAT_RESPONDED		2
#define SSTAT_PROGNOTREG	3
#define SSTAT_RPC		4
#define SSTAT_RSCRC		5
#define SSTAT_SYSTEM		6

/*
 * State_duple table.  All messages should take 1 arg - the node name.
 */
struct state_duple {
	int state;
	char *state_msg;
	int nlsmsg;
};
struct state_duple state_duples[] = {
	{SSTAT_INIT, "Internal error trying to talk to %s.", 1},
	{SSTAT_CALLED, "%s has been called.", 2},
	{SSTAT_RESPONDED, "%s (v1 ypserv) sent an old-style request.", 3},
	{SSTAT_PROGNOTREG, "nis server not registered at %s.", 4},
	{SSTAT_RPC, "RPC error to %s:  ", 5},
	{SSTAT_RSCRC,"Local resource allocation failure - can't talk to %s.",6},
	{SSTAT_SYSTEM, "System error talking to %s:  ", 7},
	{0, (char *) NULL, 0}
};
/*
 * Status_duple table.  No messages should require any args.
 */
struct status_duple {
	long status;
	char *status_msg;
	int nlsmsg;
};
struct status_duple status_duples[] = {
	{YPPUSH_SUCC, "Map successfully transferred.", 8},
	{YPPUSH_AGE,
	    "Transfer not done:  master's version isn't newer.", 9},
	{YPPUSH_NOMAP, "Failed - ypxfr can't find the map in the server's NIS domain.", 10},
	{YPPUSH_NODOM, "Failed - domain isn't supported.", 11},
	{YPPUSH_RSRC, "Failed - local resource allocation failure.", 12},
	{YPPUSH_RPC, "Failed - ypxfr had an RPC failure", 13},
	{YPPUSH_MADDR, "Failed - ypxfr couldn't get the map master's address.",
		 14},
	{YPPUSH_YPERR, "Failed - nis server or map format error.", 15},
	{YPPUSH_BADARGS, "Failed - args to ypxfr were bad.", 16},
	{YPPUSH_DBM, "Failed - dbm operation on map failed.", 17},
	{YPPUSH_FILE, "Failed - file I/O operation on map failed", 18},
	{YPPUSH_SKEW, "Failed - map version skew during transfer.", 19},
	{YPPUSH_CLEAR,
"Map successfully transferred, but ypxfr couldn't send \"Clear map\" to ypserv ", 20},
	{YPPUSH_FORCE,
	   "Failed - no local order number in map - use -f flag to ypxfr.", 21},
	{YPPUSH_XFRERR, "Failed - ypxfr internal error.", 22},
	{YPPUSH_REFUSED, "Failed - Transfer request refused.", 23},
	{0, (char *) NULL, 0}
};
/*
 * rpcerr_duple table
 */
struct rpcerr_duple {
	enum clnt_stat rpc_stat;
	char *rpc_msg;
	int nlsmsg;
};
struct rpcerr_duple rpcerr_duples[] = {
	{RPC_SUCCESS, "RPC success", 24},
	{RPC_CANTENCODEARGS, "RPC Can't encode args", 25},
	{RPC_CANTDECODERES, "RPC Can't decode results", 26},
	{RPC_CANTSEND, "RPC Can't send", 27},
	{RPC_CANTRECV, "RPC Can't recv", 28},
	{RPC_TIMEDOUT, "NIS server registered, but does not respond", 29},
	{RPC_VERSMISMATCH, "RPC version mismatch", 30},
	{RPC_AUTHERROR, "RPC auth error", 31},
	{RPC_PROGUNAVAIL, "RPC remote program unavailable", 32},
	{RPC_PROGVERSMISMATCH, "RPC program mismatch", 33},
	{RPC_PROCUNAVAIL, "RPC unknown procedure", 34},
	{RPC_CANTDECODEARGS, "RPC Can't decode args", 35},
	{RPC_UNKNOWNHOST, "unknown host", 36},
	{RPC_PMAPFAILURE, "portmap failure (host is down?)", 37},
	{RPC_PROGNOTREGISTERED, "RPC prog not registered", 38},
	{RPC_SYSTEMERROR, "RPC system error", 39},
	{RPC_SUCCESS, (char *) NULL, 0}		/* Duplicate rpc_stat unused
					         *  in list-end entry */
};

void get_default_domain_name();
void get_command_line_args();
unsigned short send_message();
void make_server_list();
void add_server();
void generate_callback();
void xactid_seed();
void main_loop();
void listener_exit();
void listener_dispatch();
bool read_server_state();
void print_state_msg();
void print_callback_msg();
void rpcerr_msg();
void get_xfr_response();
void set_time_up();
void usage();

extern unsigned long inet_addr();
extern int errno;
extern struct rpc_createerr rpc_createerr;
extern unsigned sys_nerr;
extern char *sys_errlist[];
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

void
main (argc, argv)
	int argc;
	char **argv;
	
{
	unsigned long program;
	unsigned short port;
	
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("yppush",0);
#endif NLS

	get_command_line_args(argc, argv);

	if (!domain) {
		get_default_domain_name();
	}
	
	make_server_list();
	
	/*
	 * All process exits after the call to generate_callback should be
	 * through listener_exit(program, status), not exit(status), so the
	 * transient server can get unregistered with the portmapper.
	 */

	generate_callback(&program, &port, &transport);
	
	main_loop(program, port);
	
	listener_exit(program, 0);
}

/*
 * This does the command line parsing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
	argv++;

	if (argc < 2) {
		usage();
		exit(1);
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'v':
				verbose = TRUE;
				argv++;
				break;
				
			case 'd': {

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						(void) fprintf(stderr,
							(catgets(nlmsg_fd,NL_SETN,70, "yppush:  NIS domain name exceeds %d characters\n")),
							YPMAXDOMAIN);
						exit(1);
					}
					
				} else {
					usage();
					exit(1);
				}
				
				break;
			}
				
			case 't': {
				if (argc > 1) {
					argv++;
					argc--;

					min_timeout = atoi(*argv);
					if ( min_timeout < GRACE_PERIOD)
					     min_timeout = GRACE_PERIOD;

					argv++;
				} else {
					usage();
					exit(1);
				}
				
				break;
			}
				
			case 'm': {
				if (argc > 1) {
					argv++;
					argc--;

					max_parallel = atoi(*argv);
					if (max_parallel <= 0)
						max_parallel = 1;

					argv++;
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

			if (!map) {
				map = *argv;
			} else {
				usage();
				exit(1);
			}
			
			argv++;
			
		}
	}

	if (!map) {
		usage();
		exit(1);
	}
}

/*
 *  This gets the local kernel domainname, and sets the global domain to it.
 */
void
get_default_domain_name()
{
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
	      (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,41, "Can't get domainname from system call.\n")));
		exit(1);
	}

	if (strlen(domain) == 0) {
		(void) fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,42, "The domainname hasn't been set on this machine.\n")));
		exit(1);
	}
}

/*
 * This uses nis operations to retrieve each server name in the map
 *  "ypservers". add_server is called for each one to add it to the list of
 *  servers.
 */
void
make_server_list()
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	char *ypservers = "ypservers";
	int count = 0;

	if (verbose) {
	    printf(catgets(nlmsg_fd,NL_SETN,43, "Finding NIS servers:"));
	    fflush(stdout);
	    count = 4;
	}
	if (err = yp_bind(domain) ) {
		(void) nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,44, "Can't find a nis server for domain %1$s.  Reason:  %2$s.\n")), 
				domain, yperr_string(err) );
		exit(1);
	}
	
	if (err = yp_first(domain, ypservers, &outkey, &outkeylen,
	    &val, &vallen) ) {
		(void) fprintf(stderr, 
	(catgets(nlmsg_fd,NL_SETN,45, "Can't build server list from map \"ypservers\".  Reason:  %s.\n")),
		     yperr_string(err) );
		exit(1);
	}

	while (TRUE) {
		add_server(outkey, outkeylen);
		if (verbose) {
		    printf((catgets(nlmsg_fd,NL_SETN,46, " %s")), outkey);
		    fflush(stdout);
		    if (count++ == 8) {
		    	printf((catgets(nlmsg_fd,NL_SETN,47, "\n")));
		        count = 0;
		    }
		}
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next(domain, ypservers, key, keylen,
		    &outkey, &outkeylen, &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				(void) fprintf(stderr,
	(catgets(nlmsg_fd,NL_SETN,48, "Can't build server list from map \"ypservers\".  Reason:  %s.\n")),
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
	if (count != 0) {
	    printf((catgets(nlmsg_fd,NL_SETN,49, "\n")));
	}
}

/*
 *  This adds a single server to the server list.  The servers name is
 *  translated to an IP address by calling gethostbyname(3n), which will
 *  probably make use of nis services.
 */
void
add_server(name, namelen)
	char *name;
	int namelen;
{
	struct server *ps;
	struct hostent *h;
	static unsigned long seq = 0;
	static unsigned long xactid = 0;

	if (xactid == 0) {
		xactid_seed(&xactid);
	}

#ifdef NEW_UPDATE
	if (uflag) {
            seq	= MAX_TRANSID;
	}
#endif /* NEW_UPDATE */
	
	ps = (struct server *) malloc( (unsigned) sizeof (struct server));

	if (ps == (struct server *) NULL) {
		perror((catgets(nlmsg_fd,NL_SETN,50, "yppush malloc failure")));
		exit(1);
	}

	name[namelen] = '\0';
	(void) strcpy(ps->name, name);
	ps->state = SSTAT_INIT;
	ps->status = 0;
	ps->oldvers = FALSE;
	
	if (h = (struct hostent *) gethostbyname(name) ) {
		ps->domb.dom_server_addr.sin_addr =
		    *((struct in_addr *) h->h_addr);
		ps->domb.dom_server_addr.sin_family = AF_INET;
		ps->domb.dom_server_addr.sin_port = 0;
		ps->domb.dom_server_port = 0;
		ps->domb.dom_socket = RPC_ANYSOCK;
		ps->xactid = xactid + seq++;
		ps->pnext = server_list;
		server_list = ps;
	} else {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,51, "Can't get an address for server %s.\n")),
		    name);
		free(ps);
	}
}

/*
 * This sets the base range for the transaction ids used in speaking the the
 *  server ypxfr processes.
 */
void
xactid_seed(xactid)
unsigned long *xactid;
{
	struct timeval t;

	if (gettimeofday(&t, (struct timezone *) NULL) == -1) {
		perror((catgets(nlmsg_fd,NL_SETN,52, "yppush gettimeofday failure")));
		*xactid = 1234567;
	} else {
		*xactid = t.tv_sec;
	}
}

/*
 *  This generates the udp channel which will be used as the listener process'
 *  service rendezvous point, and comes up with a transient program number
 *  for the use of the RPC messages from the ypxfr processes.
 */
void
generate_callback(program, port, transport)
	unsigned long *program;
	unsigned short *port;
	SVCXPRT **transport;
{
	struct sockaddr_in a;
	long unsigned prognum;
	SVCXPRT *xport;

	if ((xport = svcudp_create(RPC_ANYSOCK) ) == (SVCXPRT *) NULL) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,53, "Can't set up as a udp server.\n")));
		exit(1);
	}
	
	*port = xport->xp_port;
	*transport = xport;
	prognum = 0x40000000;

	while (!pmap_set(prognum++, YPPUSHVERS, IPPROTO_UDP, xport->xp_port) ) {
		;
	}

	*program = --prognum;
}


/*
 * This is the main loop. Send messages to each server,
 * and then wait for a response.
 */
void
main_loop(program, port)
	unsigned long program;
	unsigned short port;
{
	register struct server *ps;
	long error;

	if (!svc_register(transport, program, YPPUSHVERS,
	    listener_dispatch, 0) ) {
		(void) fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,54, "Can't set up transient callback server.\n")));
	}
	signal(SIGALRM, set_time_up);
	callback_timeout = FALSE;
	
	servers_called = 0;
	acks_recvd = 0;
	ps = server_list;

#ifdef DEBUG
		fprintf(stderr,"\nyppush: %d streams in parallel.\n",max_parallel);
#endif DEBUG
	/* start up to max_parallel servers */
	while ((ps)  && ( ++servers_called <= max_parallel)) {

	    /* if send_message is ok, set ps->state to SSTATE_CALLED */
		ps->state = send_message(ps, program, port, &error);
		print_state_msg(ps, error);
#ifdef DEBUG
	if (ps->state == SSTAT_CALLED) {
		time(&timenow);
		ps->start_time = timenow;
		fprintf(stderr,"debug: calling %s at %d\n", ps->name, timenow);
	}
#endif DEBUG
		ps = ps->pnext;
	}
	/* if ps <> NULL then servers_called is one too many */
	if (ps) servers_called--;
		
	/* calculate the timeout period */
	timeout_intervel = min((countof_timeouts + 1)*min_timeout, MAX_TIMEOUT);
#ifdef DEBUG
	fprintf(stderr,"debug: setting alarm to %d sec.\n",timeout_intervel);
#endif DEBUG

	/* wait for an ack from anyone we called so far */
	(void) wait_for_callback (timeout_intervel);

	/* now call each of the others upon an ack or a timeout */
	for (;ps; ps = ps->pnext) {

		ps->state = send_message(ps, program, port, &error);
		print_state_msg(ps, error);

		/* ps->state == SSTATE_CALLED if send_message
		 * worked. if call failed, skip this slave. */
		if (ps->state != SSTAT_CALLED)
			continue;
		servers_called++;
		callback_timeout = FALSE;

		/* calculate the timeout period */
		timeout_intervel = min((countof_timeouts + 1)*min_timeout, MAX_TIMEOUT);
#ifdef DEBUG
		time(&timenow);
		ps->start_time = timenow;
		fprintf(stderr,"debug: calling %s at %d\n", ps->name, timenow);
		fprintf(stderr,"debug: setting alarm to %d sec.\n",timeout_intervel);
#endif DEBUG

		/* wait for an ack from anyone  called so far */
		wait_for_callback (timeout_intervel);

	} /* for each server */


	/* wait for the last few acks or until timeout */
	callback_timeout = FALSE;
	while	((callback_timeout == FALSE) && 
		 (acks_recvd < servers_called)) {
			timeout_intervel = min((servers_called-acks_recvd)*min_timeout,MAX_TIMEOUT);
#ifdef DEBUG
		        fprintf(stderr,"debug: waiting for the last %d ack(s).\n", (servers_called-acks_recvd));
		        fprintf(stderr,"debug: setting alarm to %d sec.\n",timeout_intervel);
#endif DEBUG
			wait_for_callback (timeout_intervel);
	}
	/* cancel the alarm, if still running */
	(void) alarm(0);

	/* if the state is still SSTAT_CALLED, log error */
	for (ps = server_list; ps; ps = ps->pnext) {
	     if (ps->state == SSTAT_CALLED)
	    	(void)  fprintf( stderr, (catgets(nlmsg_fd,NL_SETN,57,
			"No response from ypxfr on %s\n")), ps->name);
#ifdef DEBUG
	     else
		if (ps->state == SSTAT_RESPONDED) {
			fprintf(stderr,"ypxfr to %s:\n\tcalled   : %s", ps->name, ctime(&(ps->start_time)));
			fprintf(stderr,"\tfinished : %s\ttotal xfer time  =  %d sec.\n\n", ctime(&(ps->finish_time)), (ps->finish_time-ps->start_time));
		}
#endif DEBUG
	}
}

/*
 * This call waits for a timeout or for activity on the socket.
 * The timeout waiting period - is incremented each time a timeout
 * happens upto a maximum of MAX_TIMEOUT. This is to accomodate
 * busy as well as slow networks. - prabha
 */
int wait_for_callback (timeout)
int timeout;
{    
int n;
	fd_set readfds;
		
	/* set timeout alarm */
	(void) alarm (timeout);

	n = 0;
	while ((callback_timeout == FALSE ) && ( n == 0)) {
		  memcpy(&readfds, &svc_fdset, sizeof(fd_set));
		  errno = 0;

		  n =   (int) select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		switch (n) {
		    case -1:		
			if (errno != EINTR) {
				(void) perror((catgets(nlmsg_fd,NL_SETN,55, "main loop select")));
				callback_timeout = TRUE;
			}
			break;

		    case 0:
			(void) fprintf (stderr,
			    (catgets(nlmsg_fd,NL_SETN,56, "Invalid timeout in main loop select.\n")));
			break;

		    default: 
			svc_getreqset(&readfds);
			break;
		} /* switch */
	}

}

/*
 * This does the listener process cleanup and process exit.
 */
void
listener_exit(program, stat)
	unsigned long program;
	int stat;
{
	(void) pmap_unset(program, YPPUSHVERS);
	exit(stat);
}

/*
 * This is the listener process' RPC service dispatcher.
 */
void
listener_dispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {

	case YPPUSHPROC_NULL:
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,58, "Can't reply to rpc call.\n")));
		}
		break;

	case YPPUSHPROC_XFRRESP:
		get_xfr_response(rqstp, transp);
		break;
		
	default:
		svcerr_noproc(transp);
		break;
	}
}


/*
 *  This dumps a server state message to stdout.  It is called in cases where
 *  we have no expectation of receiving a callback from the remote ypxfr.
 *  Modified to write to stderr.
 */
void
print_state_msg(s, e)
	struct server *s;
	long e;
{
	struct state_duple *sd;

	if (s->state == SSTAT_SYSTEM)
		return;			/* already printed */
	if (!verbose && ( s->state == SSTAT_RESPONDED ||
			  s->state == SSTAT_CALLED) )
		return;
	
	for (sd = state_duples; sd->state_msg; sd++) {
		if (sd->state == s->state) {
			(void) fprintf(stderr,catgets(nlmsg_fd,NL_SETN,sd->nlsmsg, sd->state_msg), s->name);

			if (s->state == SSTAT_RPC) {
				rpcerr_msg((enum clnt_stat) e);
			}
			
			(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,59, "\n")));
			return;
		}
	}

	(void) fprintf(stderr,
	  (catgets(nlmsg_fd,NL_SETN,60, "yppush: Bad server state value %d.\n")), s->state);
}

/*
 *  This dumps a transfer status message to stdout.  It is called in 
 *  response to a received RPC message from the called ypxfr.
 *  HPNFS Modified to write to stderr.
 */
void
print_callback_msg(s)
	struct server *s;
{
	register struct status_duple *sd;

	if (!verbose && (s->status==YPPUSH_AGE) || (s->status==YPPUSH_SUCC))
		return;
	for (sd = status_duples; sd->status_msg; sd++) {

		if (sd->status == s->status) {
			(void) nl_fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,61, "Status received from ypxfr on %1$s:\n\t%2$s\n")),
			    s->name, (catgets(nlmsg_fd,NL_SETN,sd->nlsmsg, sd->status_msg)));
			return;
		}
	}

	(void) fprintf(stderr,
	(catgets(nlmsg_fd,NL_SETN,62, "yppush listener: Garbage transaction status value from ypxfr on %s.\n")),
	    s->name);
}

/*
 *  This dumps an RPC error message to stdout.  This is basically a rewrite
 *  of clnt_perrno, but writes to stdout instead of stderr.
 *  HPNFS This routine has been modified to write to stderr since it 
 *	  doesn't seem to make sense for it to write to stdout.
 */
void
rpcerr_msg(e)
	enum clnt_stat e;
{
	struct rpcerr_duple *rd;

	for (rd = rpcerr_duples; rd->rpc_msg; rd++) {

		if (rd->rpc_stat == e) {
			(void) fprintf(stderr,(catgets(nlmsg_fd, NL_SETN, rd->nlsmsg, 
					rd->rpc_msg)));
			return;
		}
	}

	(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,63, "Bad error code passed to rpcerr_msg: %d.\n")),e);
}

/*
 * This picks up the response from the ypxfr process which has been started
 * up on the remote node.  The response status must be non-zero, otherwise
 * the status will be set to "ypxfr error".
 */
void
get_xfr_response(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yppushresp_xfr resp;
	register struct server *s;
	
	if (!svc_getargs(transp, xdr_yppushresp_xfr, &resp) ) {
		svcerr_decode(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,64, "Can't reply to rpc call.\n")));
	}

	for (s = server_list; s; s = s->pnext) {
		
		if (s->xactid == resp.transid) {
			s->status  = resp.status ? resp.status: YPPUSH_XFRERR;
			print_callback_msg(s);
			s->state = SSTAT_RESPONDED;

			callback_timeout = FALSE;
#ifdef DEBUG
		time(&timenow);
		s->finish_time = timenow;
		fprintf(stderr,"debug: %s called back at %d\n", s->name, timenow);
#endif DEBUG
			acks_recvd++;

			return;
		}
	}
}

/*
 * This is a UNIX signal handler which is called when the
 * timer expires waiting for a callback.
 */
void
set_time_up()
{
	signal(SIGALRM, set_time_up);
	callback_timeout = TRUE;
	countof_timeouts++;
#ifdef DEBUG
	fprintf(stderr, "debug: callback timedout\n");
#endif DEBUG
}


/*
 * This sends a message to a single ypserv process.  The return value is
 * a state value.  If the RPC call fails because of a version mismatch,
 * we'll assume that we're talking to a version 1 ypserv process, and
 * will send him an old "YPPROC_GET" request, as was defined in the
 * earlier version of yp_prot.h
 */
unsigned short
send_message(ps, program, port, err)
	struct server *ps;
	unsigned long program;
	unsigned short port;
	long *err;
{
	struct ypreq_xfr req;
	struct yprequest oldreq;
	enum clnt_stat s;
	char my_name[UTSLEN];
	struct rpc_err rpcerr;

	if ((ps->domb.dom_client = clntudp_create(&(ps->domb.dom_server_addr),
	    YPPROG, YPVERS, udp_intertry, &(ps->domb.dom_socket)))  == NULL) {

		if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED) {
			return(SSTAT_PROGNOTREG);
		} else {
			(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,65, "Error talking to %s: ")),ps->name);
			rpcerr_msg(rpc_createerr.cf_stat);
			(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,66, "\n")));
			return(SSTAT_SYSTEM);
		}
	}
	if (gethostname(my_name, sizeof (my_name) ) == -1) {
		return(SSTAT_RSCRC);
	}

	req.ypxfr_domain = domain;
	req.ypxfr_map = map;
	req.ypxfr_ordernum = 0;
	req.ypxfr_owner = my_name;
	req.transid = ps->xactid;
	req.proto = program;
	req.port = port;
	s = (enum clnt_stat) clnt_call(ps->domb.dom_client, YPPROC_XFR,
	    xdr_ypreq_xfr, &req, xdr_void, 0, udp_timeout);
	clnt_geterr(ps->domb.dom_client, &rpcerr);
	clnt_destroy(ps->domb.dom_client);
	close(ps->domb.dom_socket);
	
	if (s == RPC_SUCCESS) {
		return (SSTAT_CALLED);
	} else {

		if (s == RPC_PROGVERSMISMATCH) {
			ps->domb.dom_server_addr.sin_family = AF_INET;
			ps->domb.dom_server_addr.sin_port = 0;
			ps->domb.dom_server_port = 0;
			ps->domb.dom_socket = RPC_ANYSOCK;
			
			if ((ps->domb.dom_client =
			    clntudp_create(&(ps->domb.dom_server_addr),
	    		    YPPROG, (YPVERS - 1), udp_intertry,
			    &(ps->domb.dom_socket)))  == NULL) {

				if (rpc_createerr.cf_stat ==
				    RPC_PROGNOTREGISTERED) {
					return(SSTAT_PROGNOTREG);
				} else {
			(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,67, "V1 Error talking to %s: ")),
				ps->name);
					rpcerr_msg(rpc_createerr.cf_stat);
					(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,68, "\n")));
					return(SSTAT_SYSTEM);
				}
			}
			
			oldreq.yp_reqtype = YPGET_REQTYPE;
			oldreq.ypget_req_domain = domain;
			oldreq.ypget_req_map = map;
			oldreq.ypget_req_ordernum = 0;
			oldreq.ypget_req_owner = my_name;
		
			s = (enum clnt_stat) clnt_call(
			    ps->domb.dom_client, YPOLDPROC_GET,
			    _xdr_yprequest, &oldreq, xdr_void, 0, udp_timeout);
			clnt_geterr(ps->domb.dom_client, &rpcerr);
			clnt_destroy(ps->domb.dom_client);
			close(ps->domb.dom_socket);
		}

		if (s == RPC_SUCCESS) {
			return (SSTAT_RESPONDED);
		} else {
			*err = (long) rpcerr.re_status;
			return (SSTAT_RPC);
		}
	}
}

void
usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,69, "Usage:  yppush [-d domain] [-m num_of_parallel_streams] [-t minimum_timeout] [-v] mapname\n")));
}
