#ifndef lint
static	char rcsid[] = "@(#)ypbind:	$Revision: 1.43.109.9 $	$Date: 94/12/16 09:23:08 $";
#endif
/* ypbind.c	2.1 86/04/17 NFSSRC */ 
/*"ypbind.c 1.1 85/12/18 Copyr 1985 Sun Micro";*/

/*
 * This constructs a list of servers by domains, and keeps more-or-less up to
 * date track of those server's reachability.
 */

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: ypbind.o $Revision: 1.43.109.9 $ 94/06/02 PHNE_5081";
#endif


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
#include <string.h>
#include <rpc/rpc.h>
#include <sys/dir.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <syslog.h>
/*
**	include the TRACE macros
*/
int traceon = 0;
extern int errno;
#include <arpa/trace.h>
#define 	TRACEFILE	"/tmp/ypbind.trace"

#ifdef DEBUG
	struct timeval tp;
	struct timezone tzp;
	char date[26];
	int ID;
#include <string.h>
	FILE *dbf;
	char debug_file[] = "/tmp/ypbind_debug";
#endif DEBUG

/*
 * The domain struct is the data structure used by the nis binder to remember
 * mappings of domain to a server.  The list of domains is pointed to by
 * known_domains.  Domains are added when the nis binder gets binding requests
 * for domains which are not currently on the list.  Once on the list, an
 * entry stays on the list forever.  Bindings are initially made by means of
 * a broadcast method, using functions ypbind_broadcast_bind and
 * ypbind_broadcast_ack.  This means of binding is re-done any time the domain
 * becomes unbound, which happens when a server doesn't respond to a ping.
 * current_domain is used to communicate among the various functions in this
 * module; it is set by ypbind_get_binding.
 *  
 */
struct domain {
	struct domain *dom_pnext;
	char dom_name[MAXNAMLEN + 1];
	unsigned short dom_vers;	/* YPVERS or YPOLDVERS */
	bool dom_boundp;
	CLIENT *ping_clnt;
	struct in_addr dom_serv_addr;
	unsigned short int dom_serv_port;
	int dom_report_success;		/* Controls msg to /dev/console*/
	int dom_broadcaster_pid;
};
static int ping_sock = RPC_ANYSOCK;
struct domain *known_domains = (struct domain *) NULL;
struct domain *current_domain;		/* Used by ypbind_broadcast_ack, set
					 *   by all callers of clnt_broadcast */
struct domain *broadcast_domain;	/* Set by ypbind_get_binding, used
					 *   by the mainline. */
SVCXPRT *tcphandle;
SVCXPRT *udphandle;

#define BINDING_TRIES 1			/* Number of times we'll broadcast to
					 *   try to bind default domain.  */
#define PINGTOTTIM 20			/* Total seconds for ping timeout */
#define PINGINTERTRY 10
#define SETDOMINTERTRY 20
#define SETDOMTOTTIM 60
#define ACCFILE "/etc/securenets"
#define MAXLINE 128
int silent = TRUE;

#define YPSETLOCAL 	3
static int setok = YPSETLOCAL ;              

/* HPNFS With the introduction of NFS3_2 svc_fds is defined in the header */
/* 	 file rpc/svc.h and therefore, does not need to be defined inside */
/*	 this program as an extern.  svc.h also declares svc_fdset, which */
/*	 is used later on in this program, as an extern.	 	  */

void dispatch();
void ypbind_dispatch();
void ypbind_olddispatch();
void ypbind_get_binding();
void ypbind_set_binding();
void ypbind_send_setdom();
struct domain *ypbind_point_to_domain();
bool ypbind_broadcast_ack();
void ypbind_ping();
/* void ypbind_init_default(); */
void broadcast_proc_exit();
void get_secure_nets();
check_secure_net();

extern bool xdr_ypdomain_wrap_string();
extern bool xdr_ypbind_resp();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
char *log_file_name = NULL;			/*  The file for logging error
						    messages  */
char default_log_file[] = "/dev/console";	/*  If the user does not use the
						    -l option, this is the
						    default file to write error
						    messages to  */
extern int startlog();				/*  The logging routines  */
extern int logmsg();
void unreg_it();		/*  This function is used to handle signals,
				    unregistering ypbind from portmap's list  */

#ifdef BFA
write_BFAdbase()
{
        _UpdateBFA();
}
#endif /* BFA */

main(argc, argv)
	int argc;
	char **argv;
{
	int pid;
	int t, num_fds;
	fd_set readfds;
	char *pname;
	bool true;
	int c, error = 0;		/*  These variables are used in the  */
	char *invo_name = *argv;	/*  parsing of the arguments to  */
	extern char *optarg;		/*  ypbind  */
	extern int optind, opterr;
	int max_fds;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("ypbind",0);
#endif NLS

/*
 *  Added this to ensure the user is root.  Broadcasting, as ypbind does
 *  to find a server, is illegal unless done by root.  Dave Erickson
 */
	if (getuid()) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "You must be super-user to run %s\n")), argv[0]);
		exit(EPERM);
	}


	opterr = 0;
	get_secure_nets("ypbind");

	/*
	 * Check to see if we are running "secure" which means that we should
	 * require that the server be using a reserved port.  We are running
	 * secure if the -s option is specified
	 */
	argc--;
	argv++;
	while (argc > 0) {
		if (strcmp(*argv, "-v") == 0) {
			silent = FALSE;
		}
                else if (strncmp(*argv, "-l",2) == 0) {
                  if (argv[0][2] != '\0') {
                        log_file_name = *argv+2;
                  }
                  else {
                     if ((argc > 1) && argv[1][0] != '-'){
			argc--;
			argv++;
			log_file_name = *argv;
                     }
                     else {
                         goto usage;
                     }
                  }
		}
		else if (strcmp(*argv, "-ypset") == 0) {
			setok = TRUE;
		}
		else if (strcmp(*argv, "-ypsetme") == 0) {
                   /*   setok is defaulted to YPSETLOCAL
			setok = YPSETLOCAL;
                   */
		}
		else {
usage:
                        fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "Usage:  %s [-l log_file] [-v] [-ypset] \n")), invo_name);
                        exit(EINVAL);
		}
		argc--,
		argv++;
	}

		
	if (!log_file_name)
		log_file_name = default_log_file;
	if (startlog(invo_name, log_file_name)) {
		fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,23, "The log file \"%s\" cannot be opened.\n%s aborted.\n")),
			log_file_name, invo_name);
		exit(EINVAL);
	}

	(void) pmap_unset(YPBINDPROG, YPBINDVERS);
	(void) pmap_unset(YPBINDPROG, YPBINDOLDVERS);
	/*
	 * Scrap the initial binding processing for now, and see if we like
	 * fast startup better than initial bindings.  [Sun comment]
	 *
	 * ypbind_init_default();
	 */

	if (silent) {

		pid = fork();

		if (pid == -1) {
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,1, "Fork failure"));
			abort();
		}

		/*  Simply sleep for a short period of time to allow the child
		    to register itself with portmap.  If the child fails to do
		    so, the parent exits anyway.  Previously, the parent would
		    loop forever, waiting for the child to register.

		    SUN just exits here, but by putting the sleep() in, we can
		    be fairly certain that no subsequent processes will fail
		    because ypbind has not registered with portmap.  */

		if (pid != 0) {
			sleep(3);
			exit(0);
		}

/**
* Close any open file descriptors and reopen stdin, stdout and stderr
* as required.  Finally, break the child's terminal affiliation.
**/ 

		max_fds = getnumfds();
		for(t=3; t < max_fds; t++)
			close(t);
		freopen("/", "r", stdin);
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
		setpgrp ();
	}

	STARTTRACE(TRACEFILE);
#ifdef DEBUG
	gettimeofday (&tp, &tzp);
	strcpy (date, ctime (&tp.tv_sec));
	dbf = fopen (debug_file, "a");
	dup2 (fileno(dbf), fileno(stderr));
	fprintf (dbf, "%d	===========================================\n",ID);
	fprintf (dbf, "%d	The date is %s",ID, date);
	fprintf (dbf, "%d	===========================================\n",ID);
	fflush (dbf);
#endif DEBUG

	/*  Use unreg_it to catch these signals.  We want to unregister ypbind
	    from portmap's list of registered programs if ypbind is killed
	    in any way.
	*/

	(void) signal(SIGHUP,  unreg_it);
	(void) signal(SIGINT,  unreg_it);
	(void) signal(SIGIOT,  unreg_it);		/*  Sent by abort()  */
	(void) signal(SIGQUIT, unreg_it);
	(void) signal(SIGTERM, unreg_it);

#ifdef BFA
        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif /* BFA */

	if ((int) signal(SIGCLD, broadcast_proc_exit) == -1) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,2, "Can't catch broadcast process exit signal"));
		abort();
	}


        /* Open a socket for pinging everyone can use */
	/* HPNFS If a socket is not created by ypbind, clntudp_bufcreate */
	/* 	 will have to create a socket.  With the RPC 3.9 changes */
	/*	 if the library creates a socket it also closes it in 	 */
	/* 	 clnt_destroy.  So, the problem is that if the socket is */
	/*	 destroyed, ping_sock is not reset to -1 and because of	 */
	/* 	 that another socket will not be created later on.	 */

        ping_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (ping_sock < 0) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,24, "Cannot create socket for pinging.\n"));
                abort();
        }

	if ((tcphandle = svctcp_create(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,3, "Can't create TCP service"));
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_TCP) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,4, "Can't register TCP service"));
		abort();
	}

	if ((udphandle = svcudp_bufcreate(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (SVCXPRT *) NULL) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,5, "Can't create UDP service"));
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_UDP) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,6, "Can't register UDP service"));
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_TCP) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,7, "Can't register TCP service"));
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_UDP) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,8, "Can't register UDP service"));
		abort();
	}
#ifdef NEW_SVC_RUN
		/* we will use a nfds of 128 for better performance. With a
		 * tcp socket registered, each accept() call will generate a
		 * new socket. 128 is chosen because it is much greater than
		 * the 7.0 value of 60 and much smaller than the 8.0
		 * FD_SETSIZE of 2024.
		 */

		TRACE("Calling svc_run_ms with nfds = 128\n");
		max_fds = 128;
		svc_run_ms(max_fds);
#else	/* not NEW_SVC_RUN */
	for (;;) {
#ifdef DEBUG
	fprintf (dbf, "%d	Just inside the infinite for loop...\n",ID);
	fflush (dbf);
#endif DEBUG
		memcpy(&readfds, &svc_fdset, sizeof(fd_set));
		errno = 0;

		switch ((int) select(FD_SETSIZE, &readfds, NULL, NULL, NULL)){

		case -1:  {
		
#ifdef DEBUG
	fprintf (dbf, "%d	Case -1.\n",ID);
	fflush (dbf);
#endif DEBUG
			if (errno != EINTR) {
			    (void) logmsg(catgets(nlmsg_fd,NL_SETN,9, "Bad fds bits in main loop select mask"));
			}

			break;
		}

		case 0:  {
#ifdef DEBUG
	fprintf (dbf, "%d	Case 0.\n",ID);
	fflush (dbf);
#endif DEBUG
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,10, "Invalid timeout in main loop select"));
			break;
		}

		default:  {
#ifdef DEBUG
	fprintf (dbf, "%d	Case default; readfds = %d.\n",ID, readfds);
	fflush (dbf);
#endif DEBUG
			svc_getreqset(&readfds);
			break;
		}
		
		}
	}
#endif /* NEW_SVC_RUN */
}

/*
 * ypbind_dispatch and ypbind_olddispatch are wrappers for dispatch which
 * remember which protocol the requestor is looking for.  The theory is,
 * that since YPVERS and YPBINDVERS are defined in the same header file, if
 * a request comes in on the old binder protocol, the requestor is looking
 * for the old nis server.
 */
void
ypbind_dispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPVERS);
}

void
ypbind_olddispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPOLDVERS);
}

/*
 * This dispatches to binder action routines.
 */
void
dispatch(rqstp, transp, vers)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	unsigned short vers;
{
#ifdef DEBUG
	fprintf (dbf, "%d	Entering dispatch.\n",ID);
	fflush (dbf);
	fprintf (dbf, "%d	rpstp->rq_proc = %d\n",ID,rqstp->rq_proc);
	fflush (dbf);
#endif DEBUG

	switch (rqstp->rq_proc) {

	case YPBINDPROC_NULL:
#ifdef DEBUG
	fprintf (dbf, "%d	In dispatch: case YPBINDPROC_NULL.\n",ID);
	fflush (dbf);
#endif DEBUG

		if (!svc_sendreply(transp, xdr_void, 0) ) {
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,11, "Can't reply to RPC call"));
		}

		break;

	case YPBINDPROC_DOMAIN:
#ifdef DEBUG
	fprintf (dbf, "%d	In dispatch: case YPBINDPROC_DOMAIN.\n",ID);
	fflush (dbf);
#endif DEBUG
		ypbind_get_binding(rqstp, transp, vers);
		break;

	case YPBINDPROC_SETDOM:
#ifdef DEBUG
	fprintf (dbf, "%d	In dispatch: case YPBINDPROC_SETDOM.\n",ID);
	fflush (dbf);
#endif DEBUG
		ypbind_set_binding(rqstp, transp, vers);
		break;

	default:
#ifdef DEBUG
	fprintf (dbf, "%d	In dispatch: case default.\n",ID);
	fflush (dbf);
#endif DEBUG
		svcerr_noproc(transp);
		break;

	}
}

/*
 * This is a Unix SIGCHILD handler which notices when a broadcaster child
 * process has exited, and retrieves the exit status.  The broadcaster pid
 * is set to 0.  If the broadcaster succeeded, dom_report_success will be
 * be set to -1.
 */

void
broadcast_proc_exit()
{
	int pid;
	int stat_loc;
	register struct domain *pdom;

#ifdef DEBUG
	fprintf (dbf, "%d	Before the wait in broadcast_proc_exit.\n",ID);
	fflush (dbf);
#endif DEBUG

	pid = wait (&stat_loc);

#ifdef DEBUG
	fprintf (dbf, "%d	In broadcast_proc_exit, pid = %d\n", ID, pid);
	fflush (dbf);
#endif DEBUG

	if (pid > 0)
		for (pdom = known_domains; pdom != (struct domain *)NULL;
				pdom = pdom->dom_pnext) {
			if (pdom->dom_broadcaster_pid == pid) {
				pdom->dom_broadcaster_pid = 0;
#ifdef DEBUG
	fprintf (dbf, "%d	Found a match in the list: pid = %d\n",ID, pid);
	fflush (dbf);
#endif DEBUG
				if (stat_loc == 0)
					pdom->dom_report_success = -1;
			}
		}
	signal(SIGCLD, broadcast_proc_exit);
	return;
}

/*
 * This returns the current binding for a passed domain.
 */
void
ypbind_get_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	char domain_name[YPMAXDOMAIN + 1];
	char *pdomain_name = domain_name;
	char *pname;
	struct ypbind_resp response;
	bool true;
	char outstring[YPMAXDOMAIN + 256];
	int broadcaster_pid;
	struct domain *v1binding;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_get_binding.\n",ID);
	fflush (dbf);
#endif DEBUG

	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		svcerr_decode(transp);
		return;
	}

	if ( (current_domain = ypbind_point_to_domain(pdomain_name, vers) ) !=
	    (struct domain *) NULL) {

		/*
		 * Ping the server to make sure it is up.
		 */
		 
		if (current_domain->dom_boundp) {
			ypbind_ping(current_domain);
		}

		/*
		 * Bound or not, return the current state of the binding.
		 */

		if (current_domain->dom_boundp) {
			response.ypbind_status = YPBIND_SUCC_VAL;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr =
			    current_domain->dom_serv_addr;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port = 
			    current_domain->dom_serv_port;
		} else {
			response.ypbind_status = YPBIND_FAIL_VAL;
			response.ypbind_respbody.ypbind_error =
			    YPBIND_ERR_NOSERV;
		}
		    
	} else {
		response.ypbind_status = YPBIND_FAIL_VAL;
		response.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
	}
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_get_binding: calling svc_sendreply.\n",ID);
	fflush (dbf);
#endif DEBUG

	if (!svc_sendreply(transp, xdr_ypbind_resp, &response) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,12, "Can't respond to RPC request"));
	}

/*  The following is a kludge because of FREE in xdr_string (). */
	pdomain_name = NULL;
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_get_binding: calling svc_freeargs.\n",ID);
	fflush (dbf);
#endif DEBUG
	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		(void) logmsg(catgets(nlmsg_fd,NL_SETN,13, "ypbind_get_binding can't free args"));
	}

	if ((current_domain) && (!current_domain->dom_boundp) &&
	    (!current_domain->dom_broadcaster_pid)) {
		/*
		 * The current domain is unbound, and there is no broadcaster 
		 * process active now.  Fork off a child who will yell out on 
		 * the net.  Because of the flavor of request we're making of 
		 * the server, we only expect positive ("I do serve this
		 * domain") responses.
		 */
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_get_binding: Entering big if 1.\n",ID);
	fflush (dbf);
#endif DEBUG
		broadcast_domain = current_domain;
		broadcast_domain->dom_report_success++;
		pname = current_domain->dom_name;
				
		if ( (broadcaster_pid = fork() ) == 0) {
#ifdef DEBUG
	ID=getpid();
	/*fclose (dbf);*/
#endif DEBUG

			/*  We do not want the child, if killed, to unregister
			 *  ypbind from portmap, so each of the signals is
			 *  handled by the default method.
			 */

			(void) signal(SIGHUP,  SIG_DFL);
			(void) signal(SIGINT,  SIG_DFL);
			(void) signal(SIGIOT,  SIG_DFL);
			(void) signal(SIGQUIT, SIG_DFL);
			(void) signal(SIGTERM, SIG_DFL);

			(void) clnt_broadcast(YPPROG, vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			    
			if (current_domain->dom_boundp) {
				
				/*
				 * Send out a set domain request to our parent
				 */
				ypbind_send_setdom(pname,
				    current_domain->dom_serv_addr,
				    current_domain->dom_serv_port, vers);
				    
				if (current_domain->dom_report_success > 0) {
					(void) sprintf(outstring,
					 (catgets(nlmsg_fd,NL_SETN,14, "Server for NIS domain \"%s\" OK")),
					    pname);
					(void) logmsg(outstring);
				}
				exit(0);
			} else {
				/*
				 * Hack time.  If we're looking for a current-
				 * version server and can't find one, but we
				 * do have a previous-version server bound, then
				 * suppress the console message.
				 */
				if (vers == YPVERS && ((v1binding =
				   ypbind_point_to_domain(pname, YPOLDVERS) ) !=
				    (struct domain *) NULL) &&
				    v1binding->dom_boundp) {
					    exit(1);
				}
				
				(void) sprintf(outstring,
	      (catgets(nlmsg_fd,NL_SETN,15, "Server not responding for NIS domain \"%s\"; still trying")),
				    pname);
				(void) logmsg(outstring);
				exit(1);
			}

		} else if (broadcaster_pid == -1) {
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,16, "Broadcaster fork failure"));
		} else {
			current_domain->dom_broadcaster_pid = broadcaster_pid;
		}
	}
}

/*
 * This sends a (current version) ypbind "Set domain" message back to our
 * parent.  The version embedded in the protocol message is that which is passed
 * to us as a parameter.
 */
void
ypbind_send_setdom(dom, addr, port, vers)
	char *dom;
	struct in_addr addr;
	unsigned short int port;
	unsigned short int vers;
{
	struct ypbind_setdom req;
	struct sockaddr_in myaddr;
	int socket;
	struct timeval timeout;
	struct timeval intertry;
	CLIENT *client;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_send_setdom:version = %d, addr=%x, port=%d, domain=%s.\n",ID,vers,addr,port,dom);
	fflush (dbf);
#endif DEBUG

	strcpy(req.ypsetdom_domain, dom);
	req.ypsetdom_addr = addr;
	req.ypsetdom_port = port;
	req.ypsetdom_vers = vers;

        /* The following call to get my address may now be replaced with the
           loo back address defined in 8.0 netinet/in.h */

	get_myaddress(&myaddr);

	 /* myaddr.sin_family = AF_INET; */
	 /* myaddr.sin_addr.s_addr = INADDR_LOOPBACK; */
	myaddr.sin_port = htons(udphandle->xp_port);

	socket = RPC_ANYSOCK;
	timeout.tv_sec = SETDOMTOTTIM;
	intertry.tv_sec = SETDOMINTERTRY;
	timeout.tv_usec = intertry.tv_usec = 0;

#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_send_setdom: calling clntudp_bufcreate.\n",ID);
	fflush (dbf);
#endif DEBUG
	if ((client = clntudp_bufcreate (&myaddr, YPBINDPROG, YPBINDVERS,
	    intertry, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE) ) != NULL) {

#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_send_setdom: calling clnt_call.\n",ID);
	fflush (dbf);
#endif DEBUG
		clnt_call(client, YPBINDPROC_SETDOM, xdr_ypbind_setdom,
		    &req, xdr_void, 0, timeout);
		clnt_destroy(client);
		close(socket);
	} else {
		clnt_pcreateerror(
		    (catgets(nlmsg_fd,NL_SETN,18, "ypbind:  ypbind_send_setdom clntudp_create error")));
	}
}

/*
 * This sets the internet address and port for the passed domain to the
 * passed values, and marks the domain as supported.  This accepts both the
 * old style message (in which case the version is assumed to be that of the
 * requestor) or the new one, in which case the protocol version is included
 * in the protocol message.  This allows our child process (which speaks the
 * current protocol) to look for nis servers on behalf old-style clients.
 */
void
ypbind_set_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	struct ypbind_setdom req;
	struct ypbind_oldsetdom oldreq;
	unsigned short version;
	struct in_addr addr;
	struct sockaddr_in *who;
	unsigned short int port;
	char *domain;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_set_binding.\n",ID);
	fflush (dbf);
#endif DEBUG

	if (vers == YPVERS) {
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_set_binding: calling svc_getargs.\n",ID);
	fflush (dbf);
#endif DEBUG

		if (!svc_getargs(transp, xdr_ypbind_setdom, &req) ) {
			svcerr_decode(transp);
			return;
		}

		version = req.ypsetdom_vers;
		addr = req.ypsetdom_addr;
		port = req.ypsetdom_port;
		domain = req.ypsetdom_domain;
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_set_bind:version = %d, addr=%x, port=%d, domain=%s.\n",ID,version,addr,port,domain);
	fflush (dbf);
#endif DEBUG
	} else {
#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_set_binding: calling svc_getargs(2).\n",ID);
	fflush (dbf);
#endif DEBUG

		if (!svc_getargs(transp, _xdr_ypbind_oldsetdom, &oldreq) ) {
			svcerr_decode(transp);
			return;
		}

		version = vers;
		addr = oldreq.ypoldsetdom_addr;
		port = oldreq.ypoldsetdom_port;
		domain = oldreq.ypoldsetdom_domain;
	}


	/* find out who originated the request */
	who = svc_getcaller(transp);

	if (setok == FALSE) {
		fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,25, "ypbind:  Set domain request to host %s,from host %s, failed (ypset not allowed)!\n")) ,
		    inet_ntoa(addr), inet_ntoa(who->sin_addr) );
	svcerr_systemerr(transp);
	return;
	}
		
	/* This code implements some restrictions on who can set the	*
 	 * yp server for this host 					*/

	/* This policy is that root can set the yp server to anything, 	*
     	 * everyone else can't. This should also check for a valid yp 	*
	 * server but time is short, 4.1 for sure			*/

   	if (ntohs(who->sin_port) > IPPORT_RESERVED) {
		fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,26, "ypbind: Set domain request to host %s, from host %s, failed (bad port).\n")),
			inet_ntoa(addr), inet_ntoa(who->sin_addr) );

		svcerr_systemerr(transp);
		return;
	}

        
	if (setok == YPSETLOCAL) {
		if (!chklocal(who->sin_addr)) {

		fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,27, "ypbind: Set domain request to host %s, from host %s, failed (not local).\n")),
			inet_ntoa(addr), inet_ntoa(who->sin_addr) );

		svcerr_systemerr(transp);
		return;
		}
	}
        

	/* Now check the credentials */
	if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		if (((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid != 0) {
	
		fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,28, "ypbind: Set domain request to host %s, from host %s, failed (not root).\n")),
			inet_ntoa(addr), inet_ntoa(who->sin_addr) );

			svcerr_systemerr(transp);
			return;
		}
	}
        /*The following section is comment out for HP-UX 9.0,
	 * hence, previous release ypset may work.
	 */
        /*
	else {
		fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,29, "ypbind: Set domain request to host %s, from host %s, failed (credentials).\n")),
			inet_ntoa(addr), inet_ntoa(who->sin_addr) );

		svcerr_weakauth(transp);
		return;
	}
        */





#ifdef DEBUG
	fprintf (dbf, "%d	ypbind_set_binding: calling svc_sendreply.\n",ID);
	fflush (dbf);
#endif DEBUG
	
	if (!svc_sendreply(transp, xdr_void, 0) ) {
		logmsg(catgets(nlmsg_fd,NL_SETN,19, "Can't reply to RPC call"));
	}

	if ( (current_domain = ypbind_point_to_domain(domain,
	    version) ) != (struct domain *) NULL) {
		current_domain->dom_serv_addr = addr;
		current_domain->dom_serv_port = port;
		current_domain->dom_boundp = TRUE;

		/*  Get rid of the old client structure, if one exists.
		 *  This prevents the old structure from being used when doing
		 *  a "ypbind_ping" to the newly-set server.  This could be a
		 *  problem if the binding is set by ypset, rather than by a
		 *  broadcasting child forked by ypbind.
		 */

		if (current_domain->ping_clnt != (CLIENT *)NULL) {
			clnt_destroy(current_domain->ping_clnt);
			current_domain->ping_clnt = (CLIENT *)NULL;
		}
	}
#ifdef DEBUG
	fprintf (dbf, "%d	Exiting ypbind_set_binding.\n",ID);
	fflush (dbf);
#endif DEBUG
}
/*
 * This returns a pointer to a domain entry.  If no such domain existed on
 * the list previously, an entry will be allocated, initialized, and linked
 * to the list.  Note:  If no memory can be malloc-ed for the domain structure,
 * the functional value will be (struct domain *) NULL.
 */
static struct domain *
ypbind_point_to_domain(pname, vers)
	register char *pname;
	unsigned short vers;
{
	register struct domain *pdom;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_point_to_domain:\n",ID);
	fprintf (dbf, "%d		the domain name is %s; version %d.\n",ID, pname, vers);
	fflush (dbf);
#endif DEBUG
	
	for (pdom = known_domains; pdom != (struct domain *)NULL;
	    pdom = pdom->dom_pnext) {
		if (!strcmp(pname, pdom->dom_name) && vers == pdom->dom_vers) {
#ifdef DEBUG
	fprintf (dbf, "%d	Exiting ypbind_point_to_domain after finding an existing entry:\n",ID);
	fprintf (dbf, "%d		the domain name is %s; version %d.\n",ID, pname, vers);
	fflush (dbf);
#endif DEBUG
			return (pdom);}
	}
	
	/* Not found.  Add it to the list */
	
	if (pdom = (struct domain *)malloc(sizeof (struct domain))) {
		pdom->dom_pnext = known_domains;
		known_domains = pdom;
		strcpy(pdom->dom_name, pname);
		pdom->dom_vers = vers;
		pdom->dom_boundp = FALSE;
		pdom->ping_clnt = (CLIENT *)NULL;
		pdom->dom_report_success = -1;
		pdom->dom_broadcaster_pid = 0;
	}
	
#ifdef DEBUG
	fprintf (dbf, "%d	Exiting ypbind_point_to_domain after creating a new entry: ",ID);
	fprintf (dbf, "%d	the domain name is %s; version %d.\n",ID, pname, vers);
	fflush (dbf);
#endif DEBUG
	return (pdom);
}

/*
 * This is called by the broadcast rpc routines to process the responses 
 * coming back from the broadcast request. Since the form of the request 
 * which is used in ypbind_broadcast_bind is "respond only in the positive  
 * case", the internet address of the responding server will be picked up 
 * from the saddr parameter, and stuffed into the domain.  The domain's
 * boundp field will be set TRUE.  Because this function returns TRUE, 
 * the first responding server will be the bound server for the domain.
 */
bool
ypbind_broadcast_ack(ptrue, saddr)
	bool *ptrue;
	struct sockaddr_in *saddr;
{
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_broadcast_ack.\n",ID);
	fflush (dbf);
#endif DEBUG
        /* this is the check for secure_net stuff */

        if (!(check_secure_net(saddr, "ypbind"))) {
			return (FALSE);
			}

	current_domain->dom_boundp = TRUE;
	current_domain->dom_serv_addr = saddr->sin_addr;
	current_domain->dom_serv_port = saddr->sin_port;
	return(TRUE);
}

/*
 * This checks to see if a server bound to a named domain is still alive and
 * well.  If he's not, boundp in the domain structure is set to FALSE.
 */
void
ypbind_ping(pdom)
	struct domain *pdom;

{
	struct sockaddr_in addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout;
	struct timeval intertry;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_ping.\n",ID);
	fflush (dbf);
#endif DEBUG
	
	timeout.tv_sec = PINGTOTTIM;
	timeout.tv_usec = intertry.tv_usec = 0;
	if (pdom->ping_clnt == (CLIENT *)NULL) {
		TRACE("ypbind_ping	pdom->ping_clnt == NULL");
		intertry.tv_sec = PINGINTERTRY;
		addr.sin_addr = pdom->dom_serv_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = pdom->dom_serv_port;
		memset (addr.sin_zero, 0, 8);
		if ((pdom->ping_clnt = clntudp_bufcreate(&addr, YPPROG,
		    pdom->dom_vers, intertry, &ping_sock,
		    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (CLIENT *)NULL) {
			TRACE("ypbind_ping	clntudp_bufcreate failed");
			clnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,20, "ypbind:  ypbind_ping clntudp_create error")));
			pdom->dom_boundp = FALSE;
			return;
		} else {
			/*  Reset the port.  If pdom->dom_serv_port is ever
			 *  zero (as it is when ypset is used to set the
			 *  binding) before calling clntudp_bufcreate, this MUST
			 *  be done.
			 */
			TRACE("ypbind_ping	clntudp_bufcreate succeeded");
			pdom->dom_serv_port = addr.sin_port;
			TRACE2("ypbind_ping	pdom->dom_serv_port = %d", pdom->dom_serv_port);	
		}
	}
	if ((clnt_stat = (enum clnt_stat) clnt_call(pdom->ping_clnt,
	    YPPROC_NULL, xdr_void, 0, xdr_void, 0, timeout)) != RPC_SUCCESS) {
		pdom->dom_boundp = FALSE;
		clnt_destroy(pdom->ping_clnt);	
		pdom->ping_clnt = (CLIENT *)NULL;
#ifndef DEBUG
	}
#else   DEBUG
		fprintf (dbf, "%d	Leaving ypbind_ping; server dead. clnt_stat: %d\n",ID,clnt_stat);
		fflush (dbf);
	} else {
		fprintf (dbf, "%d	Leaving ypbind_ping; server alive.\n",ID);
		fflush (dbf);
	}
#endif DEBUG
}

/*
 * Preloads the default domain's domain binding. Domain binding for the
 * local node's default domain for both the current version, and the
 * previous version will be set up.  Bindings to servers which serve the
 * domain for both versions may additionally be made.  
 */
/*******************************************************************
   ypbind_init_default is not used - commented out.  Dave Erickson
 *******************************************************************
static void
ypbind_init_default()
{
	char domain[256];
	char *pname = domain;
	int true;
	int binding_tries;
#ifdef DEBUG
	fprintf (dbf, "%d	Entering ypbind_init_default.\n",ID);
	fflush (dbf);
#endif DEBUG

	if (getdomainname(domain, 256) == 0) {
		current_domain = ypbind_point_to_domain(domain, YPVERS);

		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
		
		current_domain = ypbind_point_to_domain(domain, YPOLDVERS);

		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
	}
}
 *******************************************************************
   ypbind_init_default is not used - commented out.  Dave Erickson
 *******************************************************************/

/*  The unreg_it function is used to handle signals, unregistering ypbind from
    portmap's list of registered programs  */
void
unreg_it(sig)
int sig;
{
	(void) pmap_unset(YPBINDPROG, YPBINDVERS);
	(void) pmap_unset(YPBINDPROG, YPBINDOLDVERS);
	exit(sig);
}

int
chklocal(taddr)
	struct in_addr taddr;
{
	struct in_addr addrs;
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	struct sockaddr_in *sin;
	int n, i,j, sock;
	char buf[UDPMSGSIZE];

	ifc.ifc_len = UDPMSGSIZE;
	ifc.ifc_buf = buf;
	sock=socket(PF_INET,SOCK_DGRAM,0);
	if (sock<0) return(FALSE);
	if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
		perror("SIOCGIFCONF");
		close(sock);
		return (FALSE);
	}
	ifr = ifc.ifc_req;
	j=0;
	for (i = 0, n = ifc.ifc_len/sizeof (struct ifreq); n > 0; n--, ifr++) {
		ifreq = *ifr;
		if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
			perror("SIOCGIFFLAGS");
			continue;
		}
		if ((ifreq.ifr_flags & IFF_UP) && ifr->ifr_addr.sa_family == AF_INET) {
			sin = (struct sockaddr_in *)&ifr->ifr_addr;
			if (ioctl(sock, SIOCGIFADDR, (char *)&ifreq) < 0) {
			perror("SIOCGIFADDR");
			}
			else {

				addrs = ((struct sockaddr_in*)
				&ifreq.ifr_addr)->sin_addr;
				if (bcmp((char *)&taddr,(char *)&addrs,sizeof(addrs))==0) {
				close(sock);
				return(TRUE);
				}
			}
		}
	}
	close(sock);
	return (FALSE);
}

struct seclist {
     u_long mask;
     u_long net;
     struct seclist *next;
};

static struct seclist *slist=NULL;
static int nofile = 0;

void get_secure_nets(ypname)
    char *ypname;
{
    FILE *fp;
    
    char strung[MAXLINE],nmask[MAXLINE],net[MAXLINE];
    unsigned long maskin, netin;
    struct seclist *tmp1, *stail;
    int line = 0;

    if ((fp = fopen(ACCFILE,"r")) == NULL) {
	syslog(LOG_WARNING|LOG_DAEMON,"%s: no %s file\n",ypname,ACCFILE);
        nofile = 1 ;
    }
    else {

	while (fgets(strung,MAXLINE,fp)) {
	    line++;

	    if (strung[strlen(strung) - 1] != '\n'){
	       syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: too long\n",ypname,
			ACCFILE,line);
                exit(1);
            }

            if ((strung[0] == '#') || (strung[0] == '\n'))
                continue;

            if (sscanf(strung,"%16s%16s",nmask,net) < 2) {
	        syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: missing fields\n",
			ypname,ACCFILE,line);
		exit(1);
	    }
	    maskin = inet_addr(nmask);

	    /* Do some validation of the file so we can warn sys admins about
	     * problems now rather than when it doesn't work.
	     */
	    if (maskin == -1
	 	    && strcmp(nmask, "255.255.255.255") != 0
	 	    && strcmp(nmask, "0xffffffff") != 0
	 	    && strcmp(nmask, "host") != 0) {

	        syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: error in netmask\n",
				ypname,ACCFILE,line);
	        exit(1);
	    }

            netin = inet_addr(net);
            if (netin == -1) {
	        syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: error in address\n",
				ypname,ACCFILE,line);
                exit(1);
            }

            if ((maskin & netin) != netin) {
                 syslog(LOG_ERR|LOG_DAEMON,
	     		"%s: %s line %d: netmask does not match network\n",
	     		ypname,ACCFILE,line);
	         exit(1);
	    }

	    /* Allocate and fill it a structure for this information */
	    tmp1 = (struct seclist *) malloc(sizeof (struct seclist));
	    if (tmp1 == NULL) {
                 syslog(LOG_ERR|LOG_DAEMON,
	     		"%s: Could not allocate memory\n", ypname);
	         exit(1);
	    }
            tmp1->mask = maskin;
	    tmp1->net = netin;
            tmp1->next = NULL;

	    /* Link it onto the list at the end. */
	    if (slist == NULL)
	    	slist = tmp1;
	    else {
		stail->next = tmp1;
	    }
	    stail = tmp1;
	}
	fclose(fp);
    }
}

check_secure_net(caller, ypname)
    struct sockaddr_in *caller;
    char *ypname;
{
    struct seclist *tmp;

    if (nofile)
        return(1);

    for (tmp = slist; tmp != NULL; tmp = tmp->next) {
        if ((caller->sin_addr.s_addr & tmp->mask) == tmp->net){
	    return(1);
	}
    }

    syslog(LOG_ERR|LOG_DAEMON,"%s: access denied for %s\n", ypname,
		inet_ntoa(caller->sin_addr));
    return(0);
}
