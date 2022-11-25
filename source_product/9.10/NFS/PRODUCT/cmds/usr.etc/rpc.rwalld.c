#ifndef lint
static  char rcsid[] = "@(#)rpc.rwalld:	$Revision: 1.29.109.1 $	$Date: 91/11/19 14:11:40 $  ";
#endif
/* rpc.rwalld.c	2.1 86/04/17 NFSSRC */ 
/*static  char sccsid[] = "rpc.rwalld.c 1.1 86/02/05 Copyr 1984 Sun Micro";*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <rpcsvc/rwall.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/signal.h>
/*	HPNFS
**	add TRACE() macros
*/
#include <arpa/trace.h>

# define	TRACEFILE	"/tmp/rwalld.trace"

# define	WALL		"/etc/wall"

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
int splat();
extern int errno;

/*
**	Declare Globals; by default we exit if remapped ... different
**	from Sun (they exit as with "-e").
*/
char	LogFile[64];		/* log file: use it instead of console	*/
long	Exitopt=600l;		/* # of seconds to wait between checks	*/
int	My_prog=WALLPROG,		/* the program number	*/
	My_vers=WALLVERS,		/* the version number	*/
	My_prot=IPPROTO_UDP,		/* the protocol number	*/
	My_port;		/* the port number, filled in later	*/

#ifdef	BFA
/*	HPNFS	jad	87.07.02
**	handle	--	added to get BFA coverage after killing server;
**		the explicit exit() will be modified by BFA to write the
**		BFA data and close the BFA database file.
*/
handle(sig)
int sig;
{
	exit(sig);
}

write_BFAdbase()
{
        _UpdateBFA();
}
#endif	BFA

main(argc,argv)
int	argc;
char	**argv;
{
	register SVCXPRT *transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in), num_fds;

	STARTTRACE(TRACEFILE);
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rpc.rwalld",0);
#endif NLS
#ifdef	BFA
	/*	HPNFS	jad	87.07.02
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	*/
	(void) signal(SIGHUP, handle);
	(void) signal(SIGINT, handle);
	(void) signal(SIGQUIT, handle);
	(void) signal(SIGTERM, handle);
	TRACE("main signals set up for HUP, INT, QUIT, TERM");

        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif	BFA
	/*	HPNFS	jad	87.10.02
	**	ignore SIGCLD -- if we don't exit after serving one request,
	**	then our children queue up to be wait()ed for.  ignoring the
	**	signal causes them to silently go away ...
	*/
	(void) signal(SIGCLD, SIG_IGN);

	if (getsockname(0, &addr, &len) != 0) {
		TRACE2("main getsockname returns %d, exit", errno);
		log_perror(catgets(nlmsg_fd,NL_SETN,1, "rwalld: getsockname"));
		perror((catgets(nlmsg_fd,NL_SETN,1, "rwalld: getsockname")));
		exit(1);
	} else {
		My_port = addr.sin_port;
		TRACE2("main found My_port = %d", My_port);
	}
	if ((transp = svcudp_create(0)) == NULL) {
		TRACE("main svcudp_create returns error, exit");
		logmsg(catgets(nlmsg_fd,NL_SETN,2, "svc_rpc_udp_create: error\n"));
		exit(1);
	}
	if (!svc_register(transp, WALLPROG, WALLVERS, splat, 0)) {
		TRACE("main svcudp_create returns error, exit");
		logmsg(catgets(nlmsg_fd,NL_SETN,3, "svc_rpc_register: error\n"));
		exit(1);
	}
	/*
	**	call argparse to parse the options and set Exitopt
	*/
	LogFile[0] = '\0';
	TRACE("main calling argparse ...");
	argparse(argc,argv);
	TRACE2("main LogFile = %s", LogFile);
	startlog(*argv,LogFile);
	TRACE2("main Exitopt = %d", Exitopt);

	num_fds = getnumfds();

#ifdef NEW_SVC_RUN
	TRACE2("main about to call svc_run_ms, num_fds = %d",num_fds);
	svc_run_ms(num_fds);
#else /* NEW_SVC_RUN */
	TRACE("main about to call svc_run");
	svc_run();
#endif /* NEW_SVC_RUN */


	TRACE("main svc_run returned, error!");
	logmsg(catgets(nlmsg_fd,NL_SETN,4, "Error: svc_run shouldn't have returned\n"));
	exit(1);
}

splat(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	FILE *fp, *popen();
	char *msg = NULL; /* need to be initialised for xdr_string to allocate
			     memory to save the message; fixes dts#CNDdm02936 */
        struct sockaddr_in addr;	
	struct hostent *hp;
	char buf[256];

	TRACE("splat SOP");
	switch (rqstp->rq_proc) {
		case 0:
			TRACE("splat case 0, calling svc_sendreply");
			if (svc_sendreply(transp, xdr_void, 0)  == FALSE) {
				TRACE("splat case 0, svc_sendreply failed");
				logmsg(catgets(nlmsg_fd,NL_SETN,5, "rwalld: svc_sendreply failed"));
				exit(1);
			    }
			break;
		case WALLPROC_WALL:
			TRACE("splat case WALLPROC_WALL");
			if (!svc_getargs(transp, xdr_wrapstring, &msg)) {
				TRACE("splat svc_getargs failed");
			    	svcerr_decode(transp);
				exit(1);
			}
			TRACE("splat about to call svc_sendreply");
			if (svc_sendreply(transp, xdr_void, 0)  == FALSE) {
				TRACE("splat svc_sendreply failed");
				logmsg(catgets(nlmsg_fd,NL_SETN,6, "rwalld: svc_sendreply failed"));
				exit(1);
			}
			if (fork() == 0) {/* fork off child to do it */
				TRACE("in child, about to popen");
				if ((fp=popen(WALL, "w")) == NULL) {
				    TRACE2("splat could not popen, errno = %d", errno);
				    log_perror(WALL);
				    _exit(1);	/* child exiting, no flush */
				}
				TRACE2("splat got fp = 0x%x", fp);
				(void) fprintf(fp, (catgets(nlmsg_fd,NL_SETN,7, "%s")), msg);
				pclose(fp);
				TRACE("splat message sent, all done");
				/*	the child exits here	*/
				exit(0);
			}
			break;
		default: 
			TRACE("splat case default");
			svcerr_noproc(transp);
			break;
	}
	/*
	**	see if I should exit now, or hang around until remapped
	*/
	check_exit();
	TRACE("splat check_exit returned");
}
