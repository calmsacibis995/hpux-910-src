#ifndef lint
static char	rcsid[] = "@(#)ypserv:	$Revision: 1.42.109.5 $	$Date: 94/12/16 09:07:04 $  ";
#endif

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: ypserv.o $Revision: 1.42.109.5 $ 94/06/01 PHNE_5081";
#endif

/* ypserv.c	2.1 86/04/16 NFSSRC */
/* static  char sccsid[] = "ypserv.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/
/*
 * This contains the mainline code for the Network Information Service server.  Data
 * structures which are process-global are also in this module.  
 */
#ifndef YP_CACHE
#undef YP_UPDATE
#endif

#include <sys/types.h>
#include "ypsym.h"
#include <sys/ioctl.h>
#include <sys/file.h>

#ifdef YP_CACHE
#include "yp_cache.h"
#endif /* YP_CACHE */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#define nl_cxtime(a, b)	ctime(a)
#else /* NLS */
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif /* NLS */
int traceon = 0;
extern int errno;
#include <arpa/trace.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define ACCFILE "/etc/securenets"
#define MAXLINE 128
char	ypdbpath[] = __YP_PATH_PREFIX;
char	order_key[] = ORDER_KEY;
char	master_key[] = MASTER_KEY;

#ifdef DBINTERDOMAIN
int debuginterdomain=0;
int dbinterdomain=0;
#endif /* DBINTERDOMAIN */

struct timeval ypintertry = {		/* udp secs betw tries in peer comm */
	YPINTERTRY_TIME, 		/* Seconds */
	0				/* uSecs */
};
struct timeval yptimeout = {		/* udp total timeout for peer comm */
	YPTOTAL_TIME, 			/* Seconds */
	0				/* uSecs */
};
char	myhostname[MAX_MASTER_NAME + 1];
SVCXPRT *udphandle;
SVCXPRT *tcphandle;
bool silent = TRUE;
void ypinit();
void ypdispatch();
void ypolddispatch();
void ypget_command_line_args();
void get_secure_nets();

#ifdef YP_CACHE
int	cache_size;

#ifdef YP_UPDATE
/******************************************************************************
 *	The YPPROC_CLEARMAP and YPPROC_UPDATE_MAPENTRY	are not supported by
 *	SUN's NIS PROTOCOL. The code has been added here, but these procedure
 *	numbers have not been registered with SUN. So if/when we decide to
 *	sell/support this, we must register these with SUN and perhaps move
 *	these calls into a different dispatch. The version obtained from SUN
 *	for these procs must be used to register them (using svc_register).
 ******************************************************************************/
bool     uflag = FALSE;
bool    mustpull = FALSE;
#endif /* YP_UPDATE */

#endif /* YP_CACHE */

/*
 * External refs to functions named only by the dispatchers.
 */
extern void ypdomain();
extern void ypmatch();
extern void ypfirst();
extern void ypnext();
extern void ypxfr();
extern void ypall();
extern void ypmaster();
extern void yporder();
extern void ypoldmatch();
extern void ypoldfirst();
extern void ypoldnext();
extern void ypoldpoll();
extern void yppush();
extern void yppull();
extern void ypget();
extern void ypmaplist();

#ifdef NLS
static nl_catd nlmsg_fd;
char	*nl_cxtime();
#endif /* NLS */

/*
 *  If the user does not use the -l option,
 *  write error msgs to this file if it exists.
 */
char	default_log_file[] = "/usr/etc/yp/ypserv.log";
bool logging = FALSE;			/*  TRUE if messages should be
logged to a file  */
extern int	startlog();		/*  The logging routines  */
extern int	logmsg();

#ifdef hpux
bool longfiles = FALSE;		/* Does ypdbpath support long filenames */
#endif /* hpux */

int	max_length = DIRSIZ_CONSTANT;/* the max_length of a filename that is */
				     /* supported on the ypdbpath directory  */
void unreg_it();		/*  This function is used to handle signals,
				    unregistering ypserv from portmap's list  */
# define	TRACEFILE	"/tmp/ypserv.trace"

#ifdef YPSERV_DEBUG
#define	NUM_DEBUG_ENTRIES	16
struct debug_struct debug_info[NUM_DEBUG_ENTRIES];
struct debug_struct *debug_infoptr = debug_info;
fd_set svc_fdset_save, svc_fdset_copy;
int	debug_entry	 = -1;	/* points to the entry in use */
struct sockaddr_in myaddr;	/* save my_address at startup. */
#define CALL_COUNT 10000000
int	call_count1 = 0, call_count2 = 0;
void print_debug_entry();
void print_debug_info();
#endif /* YPSERV_DEBUG */

char	*invo_name;	/*  parsing of the arguments to  */

/*
 * This is the main line code for the nis server.
 */

#ifdef DBINTERDOMAIN

/*
 * This is the main line code for the NIS server.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	int readfds;

	if (geteuid() != 0) {
		(void)fprintf(stderr, "must be root to run %s\n", argv[0]);
		exit(1);
	}
	STARTTRACE(TRACEFILE);
 	ypinit(argc, argv); 			/* Set up shop */
	svc_run_as(); /*run asynchronous services*/
	fprintf(stderr,"svc_run_as returned\n");
	pmap_unset(YPPROG, YPVERS);
	pmap_unset(YPPROG, YPOLDVERS);
	abort();
}

#else

main(argc, argv)
int	argc;
char	**argv;
{
	long	nfds;
	fd_set	readfds;
	register fd_set_len, fdsetlen_save, fd_size, or_mask, *mask_ptr;
	struct timer_action *palarm_action;

/* The following defined to terminate ypserv if too many select failures */
#define MAX_SELECT_ERR	1000
#define MAX_TIMEOUT_ERR	10000

	int	select_err_count	 = 0;
	int	timeout_err_count	 = 0;

	STARTTRACE(TRACEFILE);
	ypinit(argc, argv); 			/* Set up shop */

#ifdef YPSERV_DEBUG
	/* save a copy of the original svc_fdset for our info */
	memcpy(&svc_fdset_save, &svc_fdset, sizeof(fd_set));
#endif /* YPSERV_DEBUG */
#ifdef NEW_SVC_RUN
		/* we will use 128 for better performance. With the tcp
		 * socket registered, each accept will temporarily open
		 * a new socket which you have to select on. 128 should
		 * be ok because 60 worked fine in 6.5 & 7.0 and is much
		 * smaller than the 8.0 value of 2024.
		 */
		nfds = 128;
#else
		nfds = FD_SETSIZE;
#endif /* NEW_SVC_RUN */

		fdsetlen_save = howmany(nfds, NFDBITS);
		fd_size = (fdsetlen_save << 2);

	for (; ; ) {
		int	t;

#ifdef YPSERV_DEBUG
		/* save the value right before the select */
		memcpy(&svc_fdset_copy, &svc_fdset, fd_size);
#endif /* YPSERV_DEBUG */
		/*
		 * now copy enough bytes of svc_fdset
		 * to readfds so we can do the select
		 */
		memcpy(&readfds, &svc_fdset, fd_size);

		TRACE("Before the readfds switch in main");
		errno = 0;
		switch ( t = (int) select(nfds, &readfds, (int *) NULL,
		    (int *) NULL, (struct timeval *) NULL) ) {
		case -1: {
				TRACE2("Switch case = -1, errno = %d", errno);
				if (errno != EINTR) {
					if (logging) {

#ifdef YPSERV_DEBUG
						/* NOTE: no NLSing of DEBUG messages */
						(void) logmsg("Bad bits in mask, errno = %d, nfds = %d", errno, _NFILE);
						print_select_mask("svc_fdset (original)",svc_fdset_save);
						print_select_mask("svc_fdset (after read)",svc_fdset);
						print_select_mask("read_fds (before read)",svc_fdset_copy);
						print_select_mask("readfds (after read)",read_fds);
						print_debug_entry(debug_infoptr, debug_entry);
#else
						(void) logmsg(catgets(nlmsg_fd, NL_SETN, 1, "Bad fds bits in main loop select mask"));
#endif /* YPSERV_DEBUG */

					}
					if (++select_err_count  > MAX_SELECT_ERR) {
						(void) logmsg(catgets(nlmsg_fd, NL_SETN, 18, "Too many select_errors, aborting.\n"));
						abort();
					}
				}
				break;
			}
		case 0: {
				TRACE("Switch case = 0");
				if (logging)
					(void) logmsg(catgets(nlmsg_fd, NL_SETN, 2, "Invalid timeout in main loop select"));

#ifdef YPSERV_DEBUG
				if (++timeout_err_count > MAX_TIMEOUT_ERR)
					abort();
#endif /* YPSERV_DEBUG */

				break;
			}
		default: {
				TRACE2("Switch case = default( % d); calling svc_getreq", t);
				mask_ptr = svc_fdset.fds_bits;
				or_mask = 0;
				fd_set_len = fdsetlen_save;
				do {
					or_mask |= *(mask_ptr++);
				} while (--fd_set_len);
				if (or_mask == 0) {
					TRACE("readfds == 0. No readfds bits set ?!");
#ifdef YPSERV_DEBUG
					(void) logmsg("select returned >0, but reafds == 0 !");
					if (++select_err_count  > MAX_SELECT_ERR) {
						(void) logmsg(catgets(nlmsg_fd, NL_SETN, 18, "Too many select_errors, aborting.\n"));
						abort();
					}
#endif /* YPSERV_DEBUG */
				} else {
#ifdef NEW_SVC_RUN
					svc_getreqset_ms (&readfds, nfds);
#else
					svc_getreqset (&readfds);
#endif /* NEW_SVC_RUN */
				}
				break;
			}
		}
	}
}
#endif  /* DBINTERDOMAIN */
/*
 * Does startup processing for the nis server.
 */
void
ypinit(argc, argv)
int	argc;
char	**argv;
{
	int	pid;
	int	max_fds;

#ifdef YPSERV_DEBUG
	struct debug_struct *db;
	int	i;
#endif /* YPSERV_DEBUG */

	TRACE("ypinit SOP");
	ypget_command_line_args(argc, argv);
	/* HPNFS A filesystem can support long or short filenames.
	 *	When ypserv is first brought up we check to see if
	 *	longfilenames are supported and set a flag accordingly.
	 *	We are disallowing sub-directories under ypdbpath to
	 *	have a different long/short filename setup than
	 *	ypdbpath. -	chm 09/25/87
	 */

#ifdef hpux
	TRACE("ypinit : about to call max_file_length");
	max_length = max_file_length(ypdbpath);
	switch (max_length) {
	case MAXNAMLEN:
		longfiles = TRUE;
		break;

	case DIRSIZ_CONSTANT:
		longfiles = FALSE;
		break;

	default:
		fprintf(stderr, "ypxfr: Unable to determine filename length allowed in directory % s\n", ypdbpath);
		exit(-1);
	}
	TRACE3("ypinit: longiles is % d, max_length is % d", longfiles, max_length);
#endif /* hpux */

	get_secure_nets(argv);
	pmap_unset(YPPROG, YPVERS);
	pmap_unset(YPPROG, YPOLDVERS);
	if (silent) {
		pid = fork();
		if (pid == -1) {
			if (logging) {
				/* open catalog for NLS, log msg and die */
#ifdef NLS
				nl_init(getenv("LANG"));
				nlmsg_fd = catopen("ypserv", 0);
#endif /* NLS */

				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 3, "ypinit() fork failure"));
			}
			abort();
		}
		if (pid != 0) {
			exit(0);
		}
	/**
	* Close any open file descriptors and reopen stdin, stdout and \		* stderr, so they are not used, without expressly being setup,\			* by any children. nlmsg_fd also gets closed in this process.
	* logfile is opened for each write anyway. Finally, break the
	* child's terminal affiliation.
	**/
		 {
			int	t;
			max_fds = getnumfds();
			for (t = 3; t < max_fds; t++)
				close(t);
			freopen(" / ", "r", stdin);
			freopen(" / dev / null", "w", stdout);
			freopen(" / dev / null", "w", stderr);
			/* now open nlmsg_fd to be used by NLS. */

#ifdef NLS
			nl_init(getenv("LANG"));
			nlmsg_fd = catopen("ypserv", 0);
#endif /* NLS */

			STARTTRACE(TRACEFILE);
			TRACE("ypinit: fork, close and dups complete");
       /*
	*  HPNFS
	*
	*  Break the terminal affiliation.
	*  Dave Erickson.
	*
	*  HPNFS
	*/
			setpgrp();
		}
	}
	(void) gethostname(myhostname, MAX_MASTER_NAME);

#ifdef YPSERV_DEBUG

#ifdef DEBUG
	fprintf(stderr, "myname = % s, start of debug_info = % x\n", myhostname, debug_info);
#endif /* DEBUG */

	db = debug_info;
	for (i = 0; i < NUM_DEBUG_ENTRIES; i++, db++)
		memset(db, 0, sizeof(struct debug_struct ));
	(void) get_myaddress(&myaddr);
#endif /* YPSERV_DEBUG */

	/*  Use unreg_it to catch these signals.  We want to unregister
	 *  ypserv from portmap's list of registered programs if ypserv
	 *  is killed in any way.
	 */
	TRACE("ypinit: preparing to catch signals");
	(void) signal(SIGHUP,  unreg_it);
	(void) signal(SIGINT,  unreg_it);
	(void) signal(SIGIOT,  unreg_it);	/*  Sent by abort()  */
	(void) signal(SIGQUIT, unreg_it);
	(void) signal(SIGTERM, unreg_it);
	/*  HPNFS
	 *
	 *  Set the function to SIG_IGN to prevent creation of zombies.
	 *  Dave Erickson.
	 *
	 *  HPNFS
	 */
	if ((int) signal(SIGCLD, SIG_IGN) == -1) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 4, "Can't catch process exit signal"));
		abort();
	}
	TRACE("ypinit:  calling svcudp_bufcreate");
	if ((udphandle = svcudp_bufcreate(RPC_ANYSOCK, YPMSGSZ, YPMSGSZ))
	     == (SVCXPRT * ) NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 5, "Can't create udp server"));
		abort();
	}
	TRACE("ypinit: calling svctcp_create");
	if ((tcphandle = svctcp_create(RPC_ANYSOCK, YPMSGSZ, YPMSGSZ))
	     == (SVCXPRT * ) NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 6, "Can't create tcp server"));
		abort();
	}
	TRACE2("ypinit:  calling svc_register to register udp, version %d",
	    YPVERS);
	if (!svc_register(udphandle, YPPROG, YPVERS, ypdispatch,
	    IPPROTO_UDP) ) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 7, "Can't register V % d udp service"), YPVERS);
		abort();
	}
	TRACE2("ypinit: calling svc_register to register tcp, version % d",
	    YPVERS);
	if (!svc_register(tcphandle, YPPROG, YPVERS, ypdispatch,
	    IPPROTO_TCP) ) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 8, "Can't register V%d tcp service"), YPVERS);
		abort();
	}
	TRACE2("ypinit:  calling svc_register to register udp, version %d",
	    YPOLDVERS);
	if (!svc_register(udphandle, YPPROG, YPOLDVERS, ypolddispatch,
	    IPPROTO_UDP) ) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 9, "Can't register V % d udp service"), YPOLDVERS);
		abort();
	}
	TRACE2("ypinit: calling svc_register to register tcp, version % d",
	    YPOLDVERS);
	if (!svc_register(tcphandle, YPPROG, YPOLDVERS, ypolddispatch,
	    IPPROTO_TCP) ) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 10, "Can't register V%d tcp service"), YPOLDVERS);
		abort();
	}
	TRACE("ypinit:  EOP");
}

/*
 * This picks up any command line args passed from the process invocation.
 */
void
ypget_command_line_args(argc, argv)
int	argc;
char	**argv;
{
	char	*log_file_name = NULL;	/*  The file for logging error msgs  */
	int	c, error = 0;		/*  These variables are used in the  */

	invo_name = *argv;		/*  parsing of the arguments to  */

	/*  Parse the arguments passed in to ypserv  */
	while (--argc) {
		++argv;
		if ((*argv)[0] == '-') {
			switch ((*argv)[1]) {
			case 'l':
				if ((*argv)[2] != '\0') {
					log_file_name = &((*argv)[2]);
				} else {
					--argc;
					++argv;
					if (argc > 0)
						log_file_name = *argv;
					else {
						usage();
						exit (1);
					}
				}
				break;

			case 'v':
				if ((*argv)[2] != '\0') {
					fprintf(stderr, "Option Specification Error: %s\n",(*argv));
					error++;
					break;
				}
				silent = FALSE;
				break;
#ifdef YP_CACHE
			case 'c':
				if ((*argv)[2] != '\0') {
					cache_size = atoi(&((*argv)[2]));
					if (cache_size < 0) {
						fprintf(stderr, "Option Specification Error: %s\n",(*argv));
						exit (1);
					}
				} else {
					--argc;
					++argv;
					if (argc > 0) {
						cache_size = atoi(*argv);
						if (cache_size < 0) {
							fprintf(stderr, "Option Specification Error: -c %s\n",(*argv));
							exit (1);
						}
					} else {
						fprintf(stderr, "Option Specification Error: -c\n",(*argv));
						exit(1);
					}
				}
				if (cache_size < 1)
					cache_size = 0;/*no cache.*/

				if (cache_size > MAXENTRIES)
					cache_size = MAXENTRIES;

				(void) set_max_cachesize(cache_size);
				break;
#ifdef YP_UPDATE
			case 'p':
				if ((*argv)[2] != '\0') {
					fprintf(stderr, "Option Specification Error: %s\n",(*argv));
					error++;
					break;
				}
				mustpull = TRUE;
				break;

			case 'u':
				if ((*argv)[2] != '\0') {
					fprintf(stderr, "Option Specification Error: %s\n",(*argv));
					error++;
					break;
				}
				uflag = TRUE;
				break;

#endif /* YP_UPDATE */
#endif /* YP_CACHE */

#ifdef DBINTERDOMAIN
			case 'i':
			   	if ((*argv)[2] != '\0') {
				   fprintf(stderr, "Option Specification Error %s\n",(*argv));
				   error++;
				   break;
				}
				debuginterdomain = TRUE;
				break;
			case 'd':
			   	if ((*argv)[2] != '\0') {
				   fprintf(stderr, "Option Specification Error %s\n",(*argv));
				   error++;
				   break;
				}
				dbinterdomain = TRUE;
				break;
#endif /* DBINTERDOMAIN */
			default:
				fprintf (stderr,"Unknown Option: %s\n",(*argv));
				error++;
				break;
			}
		}
	}
	if (error) {
		usage();
		exit(EINVAL);
	}

	if (!log_file_name  &&  !access(default_log_file, W_OK))
		log_file_name = default_log_file;
	if (log_file_name) {
		/*
		 * initialise the static filename, invocation name etc.,
		 * for logmsg to use later. the file is opened each time
		 * by logmsg for logging and then closed.
		 */
		if (startlog(invo_name, log_file_name)) {
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 15, "The log file \"%s\" cannot be opened.\n%s aborted.\n")),
			log_file_name, invo_name);
			exit(EINVAL);
		}
		logging = TRUE;
	}
}

/*
 * This dispatches to server action routines based on the input procedure
 * number.  ypdispatch is called from the RPC function svc_getreq.
 *
 * NOTE: The YPPROC_CLEARMAP and YPPROC_UPDATE_MAPENTRY	are not supported by
 *	SUN's NIS PROTOCOL. The code has been added here, but these procedure
 *	numbers have not been registered with SUN. So if/when we decide to
 *	sell/support this, we must register these with SUN and perhaps move
 *	these calls into a different routine. The version obtained from SUN
 *	for these procs must be registered with the svc_register call too.
 */
void
ypdispatch(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	TRACE("ypdispatch:  SOP");

#ifdef YPSERV_DEBUG
	if (++call_count1 > CALL_COUNT) {
		++call_count2;
		call_count1 = 0;
	}
	debug_entry = (++debug_entry) % NUM_DEBUG_ENTRIES;
	if (debug_entry == 0)
		debug_infoptr = debug_info;
	else
		debug_infoptr++;
	memset(debug_infoptr, 0, sizeof(struct debug_struct ));
	debug_infoptr->raddr = transp->xp_raddr;
#endif /* YPSERV_DEBUG */

	switch (rqstp->rq_proc) {
	case YPPROC_NULL:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_NULL;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = YPPROC_NULL");
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			TRACE("ypdispatch:  YPPROC_NULL svc_sendreply failed");
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 11, "YPPROC_NULL can't reply to rpc call"));
		}
		break;

	case YPPROC_DOMAIN:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_DOMAIN;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_DOMAIN");
		ypdomain(rqstp, transp, TRUE);
		break;

	case YPPROC_DOMAIN_NONACK:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_DOMAIN_NONACK;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_DOMAIN_NONACK");
		ypdomain(rqstp, transp, FALSE);
		break;

	case YPPROC_MATCH:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_MATCH;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_MATCH");
		ypmatch(rqstp, transp);
		break;

	case YPPROC_FIRST:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_FIRST;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_FIRST");
		ypfirst(rqstp, transp);
		break;

	case YPPROC_NEXT:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_NEXT;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_NEXT");
		ypnext(rqstp, transp);
		break;

	case YPPROC_XFR:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_XFR;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_XFR");
		ypxfr(rqstp, transp);
		break;

	case YPPROC_CLEAR:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_CLEAR;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case  = YPPROC_CLEAR");
		ypclr_current_map();
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			TRACE("ypdispatch: YPPROC_CLEAR svc_sendreply failed");
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 12, "YPPROC_CLEAR can't reply to rpc call"));
		}
		break;

	case YPPROC_ALL:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_ALL;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = YPPROC_ALL");
		ypall(rqstp, transp);
		break;

	case YPPROC_MASTER:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_MASTER;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = YPPROC_MASTER");
		ypmaster(rqstp, transp);
		break;

	case YPPROC_ORDER:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_ORDER;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = YPPROC_ORDER");
		yporder(rqstp, transp);
		break;

	case YPPROC_MAPLIST:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_MAPLIST;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = YPPROC_MAPLIST");
		ypmaplist(rqstp, transp);
		break;

#ifdef YP_CACHE
	case YPPROC_CLEARMAP:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_CLEARMAP;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch: case: YPPROC_CLEARMAP");
		ypclear_map(rqstp, transp);
		break;

#endif /* YP_CACHE */

#ifdef YP_UPDATE
	case YPPROC_UPDATE_MAPENTRY:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V2_PROC_OFF + YPPROC_UPDATE_MAPENTRY;
#endif /* YPSERV_DEBUG */

		/* PRABHA: This routine for receiving exactly one passwd entry. */
		yp_update_mapentry (rqstp, transp);
		break;
#endif /* YP_UPDATE */

	default:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = 0;
#endif /* YPSERV_DEBUG */

		TRACE("ypdispatch:  case = default");
		svcerr_noproc(transp);
		break;
	}
	TRACE("ypdispatch:  EOP");
	return;
}

/*
 * This is the dispatcher for the old nis protocol.  The case symbols are
 * defined in ypv1_prot.h, and are copied (with an added "OLD") from version
 * 1 of yp_prot.h.
 */
void
ypolddispatch(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	TRACE("ypolddispatch:  SOP");

#ifdef YPSERV_DEBUG
	if (logging)	/* NOTE: no NLSing of DEBUG messages */
		(void) logmsg("OLD_VERS call received: req = %d\n", rqstp->rq_proc);
	if (++call_count1 > CALL_COUNT) {
		++call_count2;
		call_count1 = 0;
	}
	debug_entry = (++debug_entry) % NUM_DEBUG_ENTRIES;
	if (debug_entry == 0)
		debug_infoptr = debug_info;
	else
		debug_infoptr++;
	memset(debug_infoptr, 0, sizeof(struct debug_struct ));
	debug_infoptr->raddr = transp->xp_raddr;
#endif /* YPSERV_DEBUG */

	switch (rqstp->rq_proc) {
	case YPOLDPROC_NULL:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_NULL;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch:  case = YPOLDPROC_NULL");
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			TRACE("ypolddispatch:  YPOLDPROC_NULL svc_sendreply failed");
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 13, "YPOLDPROC_NULL can't reply to rpc call"));
		}
		break;

	case YPOLDPROC_DOMAIN:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_DOMAIN;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_DOMAIN");
		ypdomain(rqstp, transp, TRUE);
		break;

	case YPOLDPROC_DOMAIN_NONACK:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_DOMAIN_NONACK;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_DOMAIN_NONACK");
		ypdomain(rqstp, transp, FALSE);
		break;

	case YPOLDPROC_MATCH:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_MATCH;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_MATCH");
		ypoldmatch(rqstp, transp);
		break;

	case YPOLDPROC_FIRST:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_FIRST;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_FIRST");
		ypoldfirst(rqstp, transp);
		break;

	case YPOLDPROC_NEXT:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_NEXT;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_NEXT");
		ypoldnext(rqstp, transp);
		break;

	case YPOLDPROC_POLL:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_POLL;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_POLL");
		ypoldpoll(rqstp, transp);
		break;

	case YPOLDPROC_PUSH:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_PUSH;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_PUSH");
		yppush(rqstp, transp);
		break;

	case YPOLDPROC_PULL:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_PULL;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_PULL");
		yppull(rqstp, transp);
		break;

	case YPOLDPROC_GET:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = V1_PROC_OFF + YPOLDPROC_GET;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = YPOLDPROC_GET");
		ypget(rqstp, transp);
		break;

	default:

#ifdef YPSERV_DEBUG
		debug_infoptr->rq_proc = 0;
#endif /* YPSERV_DEBUG */

		TRACE("ypolddispatch: case  = default");
		svcerr_noproc(transp);
		break;

	}
	TRACE("ypolddispatch: EOP");
	return;
}

void
unreg_it(sig)
int	sig;
{
	int	sig_save = sig, i;
	TRACE2("unreg_it: SOP - signal = % d", sig_save);
	if (logging)
		(void) logmsg(catgets (nlmsg_fd, NL_SETN, 19, "ypserv: Caught signal % d.  Unregistering from / etc / portmap."),
		    sig_save);
	(void) pmap_unset(YPPROG, YPVERS);
	(void) pmap_unset(YPPROG, YPOLDVERS);

#ifdef YPSERV_DEBUG
	print_debug_info();
#endif /* YPSERV_DEBUG */


#ifdef YP_CACHE
	print_all_maps();
#endif 	/* YP_CACHE */

	TRACE("unreg_it: pmap_unsets complete.  Exiting");
	if (sig_save == SIGIOT)	/* if abort call caused this */
		abort (); /* so we core dump */
	else
		exit(sig_save);
}

usage()
{
#ifndef YP_CACHE
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 14, "Usage:  %s [-d] [-l log_file]\n")), invo_name);
#else	/* ifdef YP_CACHE */
#ifndef YP_UPDATE
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 16, "Usage:  %s [-d] [-l log_file] [-c num]\n")), invo_name);
#else	/* ifdef YP_CACHE && YP_UPDATE */
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 17, "Usage:  %s [-d] [-l log_file] [-c num] [-u] [-p]\n")), invo_name);
#endif /* YP_UPDATE */
#endif /* YP_CACHE */
}

#ifdef YPSERV_DEBUG
void
print_selectmask(msg, mask)
char	*msg;
fd_set	mask;
{
int num_fds, fd_set_len, i = -1;

	num_fds = getnumfds();
	fd_set_len = howmany(num_fds, NFDBITS);
	logmsg("%s: num_fds = %d, bits (in groups of 32): ",msg, num_fds);
	for (i=0; i < fd_set_len; i++)
		logmsg("%d: %x",i, mask.fds_bits[i]);
}

char	*
rqproc(n)
int	n;
{
	static char	proc_name[64];
	memset(proc_name, 0, 64);
	switch (n) {
	case (V2_PROC_OFF + YPPROC_NULL) :
			strcpy(proc_name, " YPPROC_NULL");
		break;

	case (V2_PROC_OFF + YPPROC_CLEAR) :
			strcpy(proc_name, " YPPROC_CLEAR");
		break;

	case (V2_PROC_OFF + YPPROC_DOMAIN) :
			strcpy(proc_name, " YPPROC_DOMAIN");
		break;

	case (V2_PROC_OFF + YPPROC_DOMAIN_NONACK) :
			strcpy(proc_name, " YPPROC_DOMAIN_NONACK");
		break;

	case (V2_PROC_OFF + YPPROC_MAPLIST) :
			strcpy(proc_name, " YPPROC_MAPLIST");
		break;

	case (V2_PROC_OFF + YPPROC_FIRST) :
			strcpy(proc_name, " YPPROC_FIRST");
		break;

	case (V2_PROC_OFF + YPPROC_ALL) :
			strcpy(proc_name, " YPPROC_ALL");
		break;

	case (V2_PROC_OFF + YPPROC_MASTER) :
			strcpy(proc_name, " YPPROC_MASTER");
		break;

	case (V2_PROC_OFF + YPPROC_ORDER) :
			strcpy(proc_name, " YPPROC_ORDER");
		break;

	case (V2_PROC_OFF + YPPROC_MATCH) :
			strcpy(proc_name, " YPPROC_MATCH");
		break;

	case (V2_PROC_OFF + YPPROC_NEXT) :
			strcpy(proc_name, " YPPROC_NEXT");
		break;

	case (V2_PROC_OFF + YPPROC_XFR) :
			strcpy(proc_name, " YPPROC_XFR");
		break;

	case (V1_PROC_OFF + YPOLDPROC_NULL) :
			strcpy(proc_name, " YPOLDPROC_NULL");
		break;

	case (V1_PROC_OFF + YPOLDPROC_DOMAIN) :
			strcpy(proc_name, " YPOLDPROC_DOMAIN");
		break;

	case (V1_PROC_OFF + YPOLDPROC_DOMAIN_NONACK) :
			strcpy(proc_name, " YPOLDPROC_DOMAIN_NONACK");
		break;

	case (V1_PROC_OFF + YPOLDPROC_MATCH) :
			strcpy(proc_name, " YPOLDPROC_MATCH");
		break;

	case (V1_PROC_OFF + YPOLDPROC_FIRST) :
			strcpy(proc_name, " YPOLDPROC_FIRST");
		break;

	case (V1_PROC_OFF + YPOLDPROC_NEXT) :
			strcpy(proc_name, " YPOLDPROC_NEXT");
		break;

	case (V1_PROC_OFF + YPOLDPROC_POLL) :
			strcpy(proc_name, " YPOLDPROC_POLL");
		break;

	case (V1_PROC_OFF + YPOLDPROC_PUSH) :
			strcpy(proc_name, " YPOLDPROC_PUSH");
		break;

	case (V1_PROC_OFF + YPOLDPROC_PULL) :
			strcpy(proc_name, " YPOLDPROC_PULL");
		break;

	case (V1_PROC_OFF + YPOLDPROC_GET) :
			strcpy(proc_name, " YPOLDPROC_GET");
		break;

	default:
		strcpy(proc_name, "UNKNOWN PROC");
		break;
	}

#ifdef DEBUG
	fprintf(stderr, "rq_proc = % d, proc_name = % s\n", n, proc_name);
#endif /* DEBUG */

	return(proc_name);
}

void
print_debug_entry(db, i)
register struct debug_struct *db;
int	i;
{
	char	msg[1024];
	char	*pname;
	pname = rqproc(db->rq_proc);
	sprintf(msg, "entry % d: \n\trq_proc = % s\n\tdomainname = % s\n\tmapname = % s\n\tkeylen = % d\n\tkey = % s\n\tresp_status = % d\n\tremote_host_addr = % s\n\tvallen = % d\n\tval = % s\n\tresult = % d\n\n",
	    i, pname, db->domainname, db->mapname, db->keylen, db->key, db->resp_status, inet_ntoa(db->raddr.sin_addr),
	    db->vallen, db->val, db->result);
	logmsg(msg);
	exit(sig);
}

/* print last NUM_DEBUG_ENTRIES calls in the order received */
void
print_debug_info()
{
	register struct debug_struct *db;
	int	i;
	(void) logmsg("debug_info: \n\tmyhostname = % s\n\tmy_host_addr = % s\n\tdebug_entry = % d\n\tselect_err_count = % d\n\tcall_count1 = % d\n\tcall_count2 = % d\n\n",
	    myhostname, inet_ntoa(myaddr.sin_addr), debug_entry, select_err_count, call_count1, call_count2);
	db = debug_info;
	for (i = 0; i < NUM_DEBUG_ENTRIES ; i++, db++)
		print_debug_entry(db, i);
}

void
debug_getkey(keydatptr)
register datum	*keydatptr;
{
	int	len;
	char	*db_key;
	len = (((keydatptr->dsize) < YPMAXKEY) ? (keydatptr->dsize) : (YPMAXKEY - 1));
	db_key = (debug_infoptr->key);
	memcpy (db_key, keydatptr->dptr, len);
	*(db_key + len) = '\0';
	debug_infoptr->keylen = keydatptr->dsize;
}

void
debug_getval(valptr)
register struct ypresp_val *valptr;
{
	int	len;
	char	*db_val;
	len = ((((valptr->valdat).dsize) < YPMAXVAL) ? ((valptr->valdat).dsize) : (YPMAXVAL - 1));
	db_val	 = (debug_infoptr->val);
	memcpy (db_val, (valptr->valdat).dptr, len);
	*(db_val + len) = '\0';
	debug_infoptr->vallen = (valptr->valdat).dsize;
	debug_infoptr->resp_status = valptr->status;
}

void
debug_getkeyval(keyvalptr)
register struct ypresp_key_val *keyvalptr;
{
	int	len;
	char	*db_val;
	len = ((((keyvalptr->valdat).dsize) < YPMAXVAL) ? ((keyvalptr->valdat).dsize) : (YPMAXVAL - 1));
	db_val	 = (debug_infoptr->val);
	memcpy (db_val, (keyvalptr->valdat).dptr, len);
	*(db_val + len) = '\0';
	debug_infoptr->vallen = (keyvalptr->valdat).dsize;
	debug_infoptr->resp_status = keyvalptr->status;
}

void
debug_getmap(map)
char	*map;
{
	int	len;
	char	*db_map;
	len = (strlen (map));
	len = (len < YPMAXMAP) ? len : (YPMAXMAP - 1);
	db_map = (debug_infoptr->mapname);
	memcpy (db_map, map, len);
	*(db_map + len) = '\0';
}

void
debug_getdomain(domain)
char	*domain;
{
	int	len;
	char	*db_domain;
	len = (strlen (domain));
	len = (len < YPMAXDOMAIN) ? len : (YPMAXDOMAIN - 1);
	db_domain = (debug_infoptr->domainname);
	memcpy (db_domain, domain, len);
	*(db_domain + len) = '\0';
}

void
debug_get_ypreq_nokey(req_ptr)
register struct ypreq_nokey *req_ptr;
{
	(void) debug_getdomain(req_ptr->domain);
	(void) debug_getmap(req_ptr->map);
}

void
debug_get_ypreq_key(req_ptr)
register struct ypreq_key *req_ptr;
{
	(void)	debug_get_ypreq_nokey((struct ypreq_nokey *)req_ptr);
	(void)	debug_getkey(&(req_ptr->keydat));
}

void
debug_get_v1_ypreq(req_ptr)
register struct yprequest *req_ptr;
{
	int	len;
	enum ypreqtype rt;
	rt = req_ptr->yp_reqtype;
	debug_infoptr->rq_proc = rt;
	switch (rt) {
		/* ALL 5 BELOW HAVE THE VALUE YPREQ_NOKEY = 2 */
		/* YPFIRST_REQTYPE, YPPUSH_REQTYPE,
		 * YPPULL_REQTYPE,  YPPOLL_REQTYPE, YPGET_REQTYPE.
		 */
	case YPREQ_NOKEY:
		debug_get_ypreq_nokey((struct ypreq_nokey *) & (req_ptr->yp_reqbody.yp_req_nokeytype));
		break;

		/* ALL 2 CASES BELOW HAVE THE VALUE YPREQ_KEY = 1 */
		/* YPMATCH_REQTYPE,
		 * YPNEXT_REQTYPE.
		 */
	case YPREQ_KEY:
		debug_get_ypreq_key((struct ypreq_key *) & (req_ptr->yp_reqbody.yp_req_keytype));
		break;

	default:
		break;

	}
}

void
debug_get_req_args(req_ptr)
register char	*req_ptr;
{
	switch (debug_infoptr->rq_proc) {
	case (V2_PROC_OFF + YPPROC_NULL) :
	case (V2_PROC_OFF + YPPROC_CLEAR) :
	/* the following are OLD NIS cases with no args */
	case (V1_PROC_OFF + YPOLDPROC_NULL) :
					break;

	case (V2_PROC_OFF + YPPROC_DOMAIN) :
	case (V2_PROC_OFF + YPPROC_DOMAIN_NONACK) :
	case (V2_PROC_OFF + YPPROC_MAPLIST) :
		/* req_ptr is pointing to a domain_name */
		(void) debug_getdomain(req_ptr);
		break;

	case (V2_PROC_OFF + YPPROC_FIRST) :
	case (V2_PROC_OFF + YPPROC_ALL) :
	case (V2_PROC_OFF + YPPROC_MASTER) :
	case (V2_PROC_OFF + YPPROC_ORDER) :
	case (V2_PROC_OFF + YPPROC_XFR) :
		(void) debug_get_ypreq_nokey((struct ypreq_nokey *)req_ptr);
		break;

	case (V2_PROC_OFF + YPPROC_MATCH) :
	case (V2_PROC_OFF + YPPROC_NEXT) :
		(void) debug_get_ypreq_key((struct ypreq_key *)req_ptr);
		break;

	case (V1_PROC_OFF + YPOLDPROC_DOMAIN) :
	case (V1_PROC_OFF + YPOLDPROC_DOMAIN_NONACK) :
	case (V1_PROC_OFF + YPOLDPROC_MATCH) :
	case (V1_PROC_OFF + YPOLDPROC_FIRST) :
	case (V1_PROC_OFF + YPOLDPROC_NEXT) :
	case (V1_PROC_OFF + YPOLDPROC_POLL) :
	case (V1_PROC_OFF + YPOLDPROC_PUSH) :
	case (V1_PROC_OFF + YPOLDPROC_PULL) :
	case (V1_PROC_OFF + YPOLDPROC_GET) :
		(void) debug_get_v1_ypreq((struct yprequest *)req_ptr);
		break;

	default:
		break;
	}
}

void
call_abort()
{
	(void) pmap_unset(YPPROG, YPVERS);
	(void) pmap_unset(YPPROG, YPOLDVERS);
	abort ();
}
#endif /* YPSERV_DEBUG */

struct seclist {
     u_long mask;
     u_long net;
     struct seclist *next;
};

static struct seclist *slist=NULL;
static int nofile = 0;

void get_secure_nets(ypname)
    char **ypname;
{
    FILE *fp;
    
    char strung[MAXLINE],nmask[MAXLINE],net[MAXLINE];
    unsigned long maskin, netin;
    struct seclist *tmp1, *stail;
    int line = 0;

    if ((fp = fopen(ACCFILE,"r")) == NULL) {
	syslog(LOG_WARNING|LOG_DAEMON,"%s: no %s file\n",*ypname,ACCFILE);
        nofile = 1 ;
    }
    else {

	while (fgets(strung,MAXLINE,fp)) {
	    line++;

	    if (strung[strlen(strung) - 1] != '\n'){
	       syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: too long\n",*ypname,
			ACCFILE,line);
                exit(1);
            }

            if ((strung[0] == '#') || (strung[0] == '\n'))
                continue;

            if (sscanf(strung,"%16s%16s",nmask,net) < 2) {
	        syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: missing fields\n",
			*ypname,ACCFILE,line);
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
				*ypname,ACCFILE,line);
	        exit(1);
	    }

            netin = inet_addr(net);
            if (netin == -1) {
	        syslog(LOG_ERR|LOG_DAEMON, "%s: %s line %d: error in address\n",
				*ypname,ACCFILE,line);
                exit(1);
            }

            if ((maskin & netin) != netin) {
                 syslog(LOG_ERR|LOG_DAEMON,
	     		"%s: %s line %d: netmask does not match network\n",
	     		*ypname,ACCFILE,line);
	         exit(1);
	    }

	    /* Allocate and fill it a structure for this information */
	    tmp1 = (struct seclist *) malloc(sizeof (struct seclist));
	    if (tmp1 == NULL) {
                 syslog(LOG_ERR|LOG_DAEMON,
	     		"%s: Could not allocate memory\n", *ypname);
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
