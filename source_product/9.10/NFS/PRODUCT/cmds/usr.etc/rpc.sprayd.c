#ifndef lint
static  char rcsid[] = "@(#)rpc.sprayd:	$Revision: 1.29.109.1 $	$Date: 91/11/19 14:11:45 $  ";
#endif
/* rpc.sprayd.c	2.1 86/04/16 NFSSRC */
/*static  char sccsid[] = "rpc.sprayd.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

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
#include <signal.h>
#include <rpc/rpc.h>
#include <time.h>
#include <rpcsvc/spray.h>
/*
**	include the TRACE macros
*/
int traceon = 0;
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/sprayd.trace"

unsigned cnt;
struct timeval tv;
struct spraycumul cumul;
int spray();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
extern int errno;

/*
**	Declare Globals: (Note: they must have these same names)
*/
char	LogFile[64];		/* log file: use it instead of console	*/
long	Exitopt=3600l;		/* # of seconds to wait between checks	*/
int	My_prog=SPRAYPROG,		/* the program number	*/
	My_vers=SPRAYVERS,		/* the version number	*/
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
int  argc;
char	**argv;
{
	SVCXPRT *transp;
	int len = sizeof(struct sockaddr_in), num_fds;
	struct sockaddr_in addr;
	
	STARTTRACE(TRACEFILE);
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rpc.sprayd",0);
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

	if (getsockname(0, &addr, &len) != 0) {
		TRACE("main getsockname failed, abort!");
		log_perror(catgets(nlmsg_fd,NL_SETN,1, "sprayd: getsockname"));
		perror((catgets(nlmsg_fd,NL_SETN,1, "sprayd: getsockname")));
		exit(1);
	} else {
		My_port = addr.sin_port;
		TRACE2("main found My_port = %d", My_port);
	}
	transp = svcudp_create(0);
	if (transp == NULL) {
		TRACE("main svcudp_create failed, abort!");
		logmsg(catgets(nlmsg_fd,NL_SETN,2, "%s: couldn't create RPC server\n"), argv[0]);
		exit(1);
	}
	if (!svc_register(transp, SPRAYPROG, SPRAYVERS, spray, 0)) {
		TRACE("main svc_register failed, abort!");
		logmsg(catgets(nlmsg_fd,NL_SETN,3, "%s: couldn't register SPRAYPROG\n"), argv[0]);
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
	/************** debug wait loop - adopt while here **
	{
		int xdb = 1, x =0;
		while ( xdb == 1) {
			x = x + 1;
			x = x - 1;
		}
	}
	******************************/
#ifdef NEW_SVC_RUN

	/* nfds = 32 because sprayd uses only udp. In the case of TCP,
	 * more sockets could get opened upon accept, so the nfds would
	 * have to be larger */

	num_fds = 32;

	TRACE2("main calling svc_run_ms, num_fds = %d",num_fds);
	svc_run_ms(num_fds);

#else /* NEW_SVC_RUN */
	TRACE("main calling svc_run");
	svc_run();
#endif /* NEW_SVC_RUN */

	TRACE("main svc_run should not have exited!");
	logmsg(catgets(nlmsg_fd,NL_SETN,4, "%s shouldn't reach this point\n"), argv[0]);
	exit(1);
}

spray(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	TRACE("spray SOP");
	switch (rqstp->rq_proc) {
		case NULLPROC:
			TRACE("spray case NULLPROC");
			if (!svc_sendreply(transp, xdr_void, 0)) {
				TRACE("spray NULL couldn't send reply, exit");
				logmsg(catgets(nlmsg_fd,NL_SETN,5, "couldn't reply to RPC call\n"));
				exit(1);
			}
			return;
		case SPRAYPROC_SPRAY:
			TRACE2("spray case SPRAY, counter = %d", cumul.counter);
			cumul.counter++;
			return;
		case SPRAYPROC_GET:
			TRACE2("spray case GET, counter = %d", cumul.counter);
			gettimeofday(&cumul.clock, 0);
			if (cumul.clock.tv_usec < tv.tv_usec) {
				cumul.clock.tv_usec += 1000000;
				cumul.clock.tv_sec -= 1;
			}
			cumul.clock.tv_sec -= tv.tv_sec;
			cumul.clock.tv_usec -= tv.tv_usec;
			TRACE3("spray sec = %ld, u_sec = %ld",
				cumul.clock.tv_sec, cumul.clock.tv_usec);
			if (!svc_sendreply(transp, xdr_spraycumul, &cumul)) {
				TRACE("spray GET couldn't send reply, exit");
				logmsg(catgets(nlmsg_fd,NL_SETN,6, "couldn't reply to RPC call\n"));
				exit(1);
			}
			break;
		case SPRAYPROC_CLEAR:
			TRACE2("spray case CLEAR, counter = %d", cumul.counter);
			cumul.counter = 0;
			gettimeofday(&tv, 0);
			if (!svc_sendreply(transp, xdr_void, 0)) {
				TRACE("spray CLEAR couldn't send reply, return");
				logmsg(catgets(nlmsg_fd,NL_SETN,7, "couldn't reply to RPC call\n"));
			}
			return;
		default:
			TRACE("spray case default");
			svcerr_noproc(transp);
			return;
	}
	/*
	**	see if I should exit now, or hang around until remapped
	*/
	check_exit();
	TRACE("spray check_exit returned");
}
