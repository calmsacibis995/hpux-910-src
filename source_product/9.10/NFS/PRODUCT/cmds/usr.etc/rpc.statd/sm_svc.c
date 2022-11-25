#ifndef lint
static	char rcsid[] = "@(#)rpc.statd:	$Revision: 1.18.109.1 $	$Date: 91/11/19 14:19:35 $";
#endif
#ifndef lint
/* (#)sm_svc.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)sm_svc.c	1.2 86/12/30 NFSSRC */
static char sccsid[] = "(#)sm_svc.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

/* NOTE: sm_svc.c, sm_proc.c, sm_stad.c, pmap.c, tcp.c, and udp.c       */
/* share a single message catalog (statd.cat).  Buyer Beware! pmap.c,   */
/* tcp.c, and udp.c have messages in BOTH statd.cat and lockd.cat.      */
/* For that reason we have allocated messages in ranges per file: 	*/
/* 1 through 20 for sm_svc.c, 21 through 40 for sm_proc.c and from 41 	*/
/* on for sm_statd.c.  If we need more than 20 messages in this file  	*/
/* we will need to take into account the message numbers that are 	*/
/* already used by the other files.					*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/ioctl.h>
#include <rpcsvc/sm_inter.h>
#include "sm_statd.h"
#include "sm_sec.h"

#define current0	"/etc/sm"
#define backup0		"/etc/sm.bak"
#define state0		"/etc/state"

#define current1	"sm"
#define backup1		"sm.bak"
#define state1		"state"

char STATE[20], CURRENT[20], BACKUP[20];

char LogFile[64];
int debug;
char *progname;
extern crash_notice(), recovery_notice(), sm_try(), sm_run_queue();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

#ifdef	BFA
/*	HPNFS	jwe     88.10.12
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


static void
sm_prog_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		struct sm_name sm_stat_1_arg;
		struct mon sm_mon_1_arg;
		struct mon_id sm_unmon_1_arg;
		struct my_id sm_unmon_all_1_arg;
		struct stat_chge ntf_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();
	extern struct sm_stat_res *sm_stat_1();
	extern struct sm_stat_res *sm_mon_1();
	extern struct sm_stat *sm_unmon_1();
	extern struct sm_stat *sm_unmon_all_1();
	extern *sm_simu_crash_1();
	extern *sm_notify();
	extern bool_t xdr_notify();
        int oldmask;

        oldmask = sigblock (1 << (SIGALRM -1));

	switch (rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(transp, xdr_void, NULL);
                sigsetmask(oldmask);
		return;

	case SM_STAT:
		xdr_argument = xdr_sm_name;
		xdr_result = xdr_sm_stat_res;
		local = (char *(*)()) sm_stat_1;
		break;

	case SM_MON:
		xdr_argument = xdr_mon;
		xdr_result = xdr_sm_stat_res;
		local = (char *(*)()) sm_mon_1;
		break;

	case SM_UNMON:
		xdr_argument = xdr_mon_id;
		xdr_result = xdr_sm_stat;
		local = (char *(*)()) sm_unmon_1;
		break;

	case SM_UNMON_ALL:
		xdr_argument = xdr_my_id;
		xdr_result = xdr_sm_stat;
		local = (char *(*)()) sm_unmon_all_1;
		break;

	case SM_SIMU_CRASH:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) sm_simu_crash_1;
		break;

	case SM_NOTIFY:
		xdr_argument = xdr_notify;
		xdr_result = xdr_void;
		local = (char *(*)()) sm_notify;
		break;

	default:
		svcerr_noproc(transp);
                sigsetmask(oldmask);
		return;
	}
	memset(&argument, 0, sizeof(argument));
	if (! svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
                sigsetmask(oldmask);
		return;
	}
	result = (*local)(&argument);
	if (! svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if(rqstp->rq_proc != SM_MON)
	if (! svc_freeargs(transp, xdr_argument, &argument)) {
		logmsg((catgets(nlmsg_fd,NL_SETN,1, "unable to free arguments\n")));
		exit(1);
	}
	sigsetmask(oldmask);
}

main(argc, argv)
int argc;
char **argv;
{
	SVCXPRT *transp;
	int t;
	int c;
	int ppid;
	extern int optind, opterr;
	extern char *optarg;
	int choice = 0;
	int max_fds;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("statd",0);
#endif NLS
	progname = argv[0];

#ifdef	BFA
	/*	HPNFS	jwe     88.10.12
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	*/
	(void) signal(SIGHUP, handle);
	(void) signal(SIGINT, handle);
	(void) signal(SIGQUIT, handle);
	(void) signal(SIGTERM, handle);

        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif	BFA
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		/*
		 * Only allow a user who is a member of the "network"
		 * protected subsystem to execute and utilize the potential
		 * privileges associated with this program.
                 *
                 * set_auth_parameters() will change the umask, so keep
                 * a copy and restore it.
	         */
		
                mode_t cmask = umask(0);
		set_auth_parameters(argc, argv);
                umask(cmask);
		if (authorized_user("") == 0) {
			logmsg(catgets(nlmsg_fd,NL_SETN,10, "statd: Not authorized for network subsystem:  Permission denied\n"));
			exit(1);
		}
		nfs_initpriv();
	}
#endif /* SecureWare && B1 */

	LogFile[0] = '\0';
	signal(SIGALRM, sm_try);
	opterr = 0;
	while((c = getopt(argc, argv, "Dd:l:")) != EOF)
		switch(c) {
		case 'd':
			sscanf(optarg, "%d", &debug);
			break;
		case 'D':
			choice = 1;
			break;
		case 'l':
			sscanf(optarg, "%s", LogFile);
			break;
		default:
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "Usage: rpc.statd [ -l logfile]\n")));
			return;
		}

	(void)startlog( *argv, LogFile);

	if(choice == 0) {
		strcpy(CURRENT, current0);
		strcpy(BACKUP, backup0);
		strcpy(STATE, state0);
	}
	else {
		strcpy(CURRENT, current1);
		strcpy(BACKUP, backup1);
		strcpy(STATE, state1);
	}
	if(debug)
		(void) signal(SIGHUP, SIG_IGN);
		logmsg((catgets(nlmsg_fd,NL_SETN,3, "debug is on, create entry: %1$s, %2$s, %3$s\n")), CURRENT, BACKUP, STATE);

	if(!debug) {
		ppid = fork();
		if(ppid == -1) {
			(void) logmsg((catgets(nlmsg_fd,NL_SETN,4, "in.lockd: fork failure\n")));
                        /* removed during change to logging 
			(void) fflush(stderr);
			*/
			abort();
		}
		if(ppid != 0) {
			exit(0);
		}

		max_fds = getnumfds();
        	for (t = 0; t < max_fds; t++)
			(void) close(t);

		ENABLEPRIV(SEC_ALLOWDACACCESS);
		ENABLEPRIV(SEC_ALLOWMACACCESS);
		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		DISABLEPRIV(SEC_ALLOWMACACCESS);
		setpgrp(0, 0);
	}
	pmap_unset(SM_PROG, SM_VERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		logmsg((catgets(nlmsg_fd,NL_SETN,5, "cannot create udp service.\n")));
		exit(1);
	}
	if (! svc_register(transp, SM_PROG, SM_VERS, sm_prog_1, IPPROTO_UDP)) {
		logmsg((catgets(nlmsg_fd,NL_SETN,6, "unable to register (SM_PROG, SM_VERS, udp).\n")));
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		logmsg((catgets(nlmsg_fd,NL_SETN,7, "cannot create tcp service.\n")));
		exit(1);
	}
	if (! svc_register(transp, SM_PROG, SM_VERS, sm_prog_1, IPPROTO_TCP)) {
		logmsg((catgets(nlmsg_fd,NL_SETN,8, "unable to register (SM_PROG, SM_VERS, tcp).\n")));
		exit(1);
	}
	statd_init();
	pmap_svc_run(sm_run_queue );
	logmsg((catgets(nlmsg_fd,NL_SETN,9, "svc_run returned\n")));
	exit(1);
}
