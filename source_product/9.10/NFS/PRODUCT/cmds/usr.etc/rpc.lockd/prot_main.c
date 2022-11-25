#ifndef lint
static	char rcsid[] = "@(#)rpc.lockd: lock.slow $Revision: 1.35.109.4 $	$Date: 94/10/21 10:36:37 $";
#endif
/* (#)prot_main.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_main.c	1.3 87/06/18 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)prot_main.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

/* ************************************************************** */
/* NOTE: prot_alloc.c, prot_libr.c, prot_lock.c, prot_main.c,     */
/* prot_msg.c, prot_pklm.c, prot_pnlm.c, prot_priv.c, prot_proc.c,*/
/* sm_monitor.c, svc_udp.c, tcp.c, udp.c, AND pmap.c share        */
/* a single message catalog (lockd.cat).  The last three files    */
/* pmap.c, tcp.c, udp.c have messages BOTH in lockd.cat AND       */
/* statd.cat.  For that reason we have allocated message ranges   */
/* for each one of the files.  If you need more than 10 messages  */
/* in this file check the  message numbers used by the other files*/
/* listed above in the NLS catalogs.                              */
/* ************************************************************** */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include "prot_time.h"

#include "prot_sec.h"

#define DEBUG_ON	17
#define DEBUG_OFF	18
#define KILL_IT		19
#define LOCKUDPMSGSIZE 8192

int HASH_SIZE;
int debug;
#ifdef SOFT_BREAK
int soft_timeout, soft_break;
#endif SOFT_BREAK
int klm, nlm;
int report_sharing_conflicts;
int old_debug;
XDR x;
FILE *fp;
SVCXPRT *klm_transp;			/* export klm transport handle */
SVCXPRT *nlm_transp;			/* export nlm transport handle */
char *progname;
char LogFile[64];

int klm_transp_is_udp;			/* TRUE if klm_transp is a udp, not
					 * tcp transport.  Set in klm_prog.
					 */

extern int grace_period;
extern msg_entry *klm_msg;
extern int lock_len, res_len;
extern remote_result res_nolock;
extern remote_result res_working;
extern remote_result res_grace;

extern reclock *get_le();
extern msg_entry *queue();
extern remote_result *get_res();
extern int xtimer();
extern int run_queue();
extern void priv_prog();

extern reclock *copy_le();
extern struct fs_rlck *copy_fe();

/*	HPNFS	jwe	88.10.12
**	handle	--	added to get BFA coverage after killing server;
**		the explicit exit() will be modified by BFA to write the
**		BFA data and close the BFA database file.
**              Also used in normal case to catch SIGINT, SIGHUP, SIGTERM so
**              that a message can be logged before lockd exits.
*/
handle(sig)
int sig;
{
    logmsg((catgets(nlmsg_fd,NL_SETN,94, "signal handler: signal = %d\n")),sig);
    semclose();
    exit(sig);
}


static void
nlm_prog(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	bool_t (*xdr_Argument)(), (*xdr_Result)();
	char *(*Local)();
	extern nlm_testres *proc_nlm_test();
	extern nlm_res *proc_nlm_lock();
	extern nlm_res *proc_nlm_cancel();
	extern nlm_res *proc_nlm_unlock();
	extern nlm_res *proc_nlm_granted();
	extern *proc_nlm_test_msg();
	extern *proc_nlm_lock_msg();
	extern *proc_nlm_cancel_msg();
	extern *proc_nlm_unlock_msg();
	extern *proc_nlm_granted_msg();
	extern *proc_nlm_test_res();
	extern *proc_nlm_lock_res();
	extern *proc_nlm_cancel_res();
	extern *proc_nlm_unlock_res();
	extern *proc_nlm_granted_res();
        extern void *proc_nlm_share();
        extern void *proc_nlm_freeall();
        int     monitor_this_lock = 1;

	reclock *req;
	msg_entry *msgp;
	remote_result *reply;
	int oldmask;

        if (debug)
	    logmsg((catgets(nlmsg_fd,NL_SETN,96,"NLM_PROG+++ version %1$d proc %2$d\n")),
                        Rqstp->rq_vers, Rqstp->rq_proc);

	oldmask = sigblock (1 << (SIGALRM -1));
	nlm_transp = Transp;		/* export the transport handle */
	switch (Rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case DEBUG_ON:
		debug = 2;
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case DEBUG_OFF:
		debug = 0;
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case NLM_TEST:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_nlm_testres;
		Local = (char *(*)()) proc_nlm_test;
		break;

	case NLM_LOCK:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_lock;
		break;

	case NLM_CANCEL:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_cancel;
		break;

	case NLM_UNLOCK:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_unlock;
		break;

	case NLM_GRANTED:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_granted;
		break;

	case NLM_TEST_MSG:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_msg;
		break;

	case NLM_LOCK_MSG:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_msg;
		break;

	case NLM_CANCEL_MSG:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_msg;
		break;

	case NLM_UNLOCK_MSG:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_msg;
		break;
	case NLM_GRANTED_MSG:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_granted_msg;
		break;
	case NLM_TEST_RES:
		xdr_Argument = xdr_nlm_testres;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_res;
		break;

	case NLM_LOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_res;
		break;

	case NLM_CANCEL_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_res;
		break;

	case NLM_UNLOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_res;
		break;

	case NLM_GRANTED_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_granted_res;
		break;

        case NLM_SHARE:
        case NLM_UNSHARE:
                if (Rqstp->rq_vers != NLM_VERSX)  {
                        svcerr_noproc(Transp);
                        (void) sigsetmask(oldmask);
                        return;
                }
                proc_nlm_share(Rqstp, Transp);
                (void) sigsetmask(oldmask);
                return;

        case NLM_NM_LOCK:
                if (Rqstp->rq_vers != NLM_VERSX)  {
                        svcerr_noproc(Transp);
                        (void) sigsetmask(oldmask);
                        return;
                }
                Rqstp->rq_proc = NLM_LOCK; /* fake it */
                monitor_this_lock = 0;
                xdr_Argument = xdr_nlm_lockargs;
                xdr_Result = xdr_nlm_res;
                Local = (char *(*)()) proc_nlm_lock;
		break;
  
        case NLM_FREE_ALL:
                if (Rqstp->rq_vers != NLM_VERSX)  {
                        svcerr_noproc(Transp);
                        (void) sigsetmask(oldmask);
                        return;
                }
                proc_nlm_freeall(Rqstp, Transp);
                (void) sigsetmask(oldmask);
                return;

	default:
		svcerr_noproc(Transp);
		sigsetmask(oldmask);
		return;
	}

	if( Rqstp->rq_proc != NLM_LOCK_RES && Rqstp->rq_proc != NLM_CANCEL_RES
	&& Rqstp->rq_proc != NLM_UNLOCK_RES && Rqstp->rq_proc != NLM_TEST_RES
	&& Rqstp->rq_proc != NLM_GRANTED_RES) {
		/* lock request */
		if((req = get_le()) != NULL) {
			if (! svc_getargs(Transp, xdr_Argument, req)) {
				svcerr_decode(Transp);
				sigsetmask(oldmask);
				return;
			}
			if(debug == 3){
/*
			if(fwrite(&nlm, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,61, "%s: fwrite nlm error\n"), progname);
			if(fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,62, "%s: fwrite nlm_proc error\n"), progname);
			(*xdr_Argument)(&x, req);
*/
			logmsg(catgets(nlmsg_fd,NL_SETN,63, "%1$s: range[%2$d, %3$d] \n"), progname, req->lck.l_offset, req->lck.l_len);
			/*
			fflush(fp);
			*/
			}

                        /*  RB 8/92  DTS Submiss #11082INDaa
                         * parameter conversion if NLM_VERSX is used
                         */
                        if (Rqstp->rq_vers == NLM_VERSX)
                                nlm_versx_conversion(Rqstp->rq_proc, req);


			if((map_klm_nlm(req, (int) Rqstp->rq_proc)) != -1 ) {
			/*
			 * only lock, unlockd need to preassign le;
			 * only lock needs to preassign fe;
			 */
				if(Rqstp->rq_proc == NLM_LOCK || Rqstp->rq_proc == NLM_LOCK_MSG 
				|| Rqstp->rq_proc == NLM_UNLOCK || Rqstp->rq_proc == NLM_UNLOCK_MSG) {  
				
					if((req->pre_le = copy_le(req)) != NULL) {
						if (Rqstp->rq_proc == NLM_LOCK || Rqstp->rq_proc == NLM_LOCK_MSG) {
							if((req->pre_fe = (char *) copy_fe(req)) == NULL)
								goto abnormal;
						}
					}
					else
						goto abnormal;
				}
			}
			else 
				goto abnormal;
			
			if(grace_period >0 && !(req->reclaim) ) {
				if(debug)
					logmsg(catgets(nlmsg_fd,NL_SETN,64, "%s: during grace period, please retry later\n"),
					    progname);
				nlm_reply(Rqstp->rq_proc, &res_grace, req); 
				req->rel = 1;
				release_le(req);
				sigsetmask(oldmask);
				return;
			}
			if( grace_period >0 && debug)
				logmsg(catgets(nlmsg_fd,NL_SETN,65, "%1$s: accept reclaim request(%2$x)\n"), progname, req);
			if( monitor_this_lock &&
			        (Rqstp->rq_proc == NLM_LOCK ||
				 Rqstp->rq_proc == NLM_LOCK_MSG))
				/* only monitor lock req */
				if(add_mon(req, 1) == -1) {
					req->rel = 1;
					release_le(req);
					logmsg(catgets(nlmsg_fd,NL_SETN,66, "%s: req discard due status monitor problem\n"), progname);
					sigsetmask(oldmask);
					return;
				}

			(*Local)(req);
			release_le(req);
			call_back();		/* check if req cause nlm calling klm back */
			release_me();
			release_fe();
			if(debug)
				pr_all();
		}
		else { /* malloc err, return nolock */
			nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
		}
	}
	else {
		/* msg reply */
		if((reply = get_res()) != NULL) {
			if (! svc_getargs(Transp, xdr_Argument, reply)) {
				svcerr_decode(Transp);
				sigsetmask(oldmask);
				return;
			}
			if(debug == 3){
/*
			if(fwrite(&nlm, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,67, "%s: fwrite nlm_reply error\n"), progname);
			if(fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,68, "%s: fwrite nlm_reply_proc error \n"),progname);
			(*xdr_Argument)(&x, reply);
			fflush(fp);
*/

			}
			if(debug)
				logmsg(catgets(nlmsg_fd,NL_SETN,69, "%1$s: msg reply(%2$d) to procedure(%3$d)\n"), progname, reply->lockstat, Rqstp->rq_proc);
			(*Local)(reply);
			release_me();
			release_fe();
			if(debug && reply->lockstat != blocking);
				pr_all();
		}
		else {/* malloc failure, do nothing */
		}
	}
	sigsetmask(oldmask);
	return;

abnormal:
	/* malloc error, release allocated space and error return*/
	nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
	req->rel = 1;
	release_le(req);
	sigsetmask(oldmask);
	return;
}

static void
klm_prog(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	bool_t (*xdr_Argument)(), (*xdr_Result)();
	char *(*Local)();
	extern klm_testrply *proc_klm_test();
	extern klm_stat *proc_klm_lock();
	extern klm_stat *proc_klm_cancel();
	extern klm_stat *proc_klm_unlock();

	reclock *req;
	msg_entry *msgp;
	int fd;
	int oldmask;
	int sock;
	int sock_type, optlen;

	oldmask = sigblock (1 << (SIGALRM -1));
		
	klm_transp = Transp;
	klm_msg = NULL;

        if (debug)
            logmsg("KLM_PROG+++ version %1$d proc %2$d\n",
                        Rqstp->rq_vers, Rqstp->rq_proc);

	switch (Rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case DEBUG_ON:
		debug = 2;
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case DEBUG_OFF:
		debug = 0;
		svc_sendreply(Transp, xdr_void, NULL);
		sigsetmask(oldmask);
		return;

	case KILL_IT:
		svc_sendreply(Transp, xdr_void, NULL);
		logmsg(catgets(nlmsg_fd,NL_SETN,70, "%s: rpc.lockd killed upon request\n"), progname);
		semclose();
		exit(2);

	case KLM_TEST:
		xdr_Argument = xdr_klm_testargs;
		xdr_Result = xdr_klm_testrply;
		Local = (char *(*)()) proc_klm_test;
		break;

	case KLM_LOCK:
		xdr_Argument = xdr_klm_lockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_lock;
		break;

	case KLM_CANCEL:
		xdr_Argument = xdr_klm_lockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_cancel;
		break;

	case KLM_UNLOCK:
		xdr_Argument = xdr_klm_unlockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_unlock;
		break;

	default:
		svcerr_noproc(Transp);
		sigsetmask(oldmask);
		return;
	}

	/* Determine if we are using UDP.  We almost always will be, but
	 * we do have a tcp transport registered for klm_prog.  We have to
	 * know this to know if we can save the transport info.  We only
	 * know how to do it for UDP.
	 */
	sock = klm_transp->xp_sock;
	optlen = sizeof(sock_type);
	klm_transp_is_udp = FALSE;
	if (getsockopt(sock, SOL_SOCKET, SO_TYPE, &sock_type, &optlen) == 0) {
		if (sock_type == SOCK_DGRAM) {
			klm_transp_is_udp = TRUE;
		}
	}

	if((req = get_le()) != NULL) {
		if (! svc_getargs(Transp, xdr_Argument, req)) {
			svcerr_decode(Transp);
			sigsetmask(oldmask);
			return;
		}
		if(debug == 3){
/*
			if(fwrite(&klm, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,71, "%s: fwrite klm error\n"), progname);
			if(fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,72, "%s: fwrite klm_proc error\n"), progname);
			(*xdr_Argument)(&x, req);
*/
			logmsg(catgets(nlmsg_fd,NL_SETN,73, "%1$s: range[%2$d, %3$d)\n"), progname, req->lck.l_offset, req->lck.l_len);
			/*
			fflush(fp);
			*/
		}

		if(map_kernel_klm(req) != -1) {
			if(Rqstp->rq_proc == KLM_LOCK ||
			 Rqstp->rq_proc == KLM_UNLOCK) {
		 		if ((req->pre_le = copy_le(req)) != NULL) {
					if(Rqstp->rq_proc == KLM_LOCK) {
						if((req->pre_fe =  (char *)copy_fe(req)) == NULL)
							goto abnormal;
					}
				}
				else
					goto abnormal;
			}
		}
		else
			goto abnormal;
		if(grace_period > 0 && !(req->reclaim)) {
			/* put msg in queue and delay reply, unless there is no queue space */
			if(debug)
				logmsg(catgets(nlmsg_fd,NL_SETN,74, "%s: during grace period, please retry later\n"), progname);
			if((msgp = queue(req, (int) Rqstp->rq_proc)) == NULL)
			{
				klm_reply(Rqstp->rq_proc, &res_working);
				req->rel = 1;
				release_le(req);
				sigsetmask(oldmask);
				return;
			}
			req->rel = 1;
			klm_msg = msgp;
			sigsetmask(oldmask);
			return;
		}
		if(grace_period >0 && debug)
			logmsg(catgets(nlmsg_fd,NL_SETN,75, "%s: accept reclaim request\n"), progname);
		if(Rqstp->rq_proc == KLM_LOCK) /* only monitor lock req */
			if(add_mon(req, 1) == -1) {
				req->rel = 1;
				release_le(req);
				logmsg(catgets(nlmsg_fd,NL_SETN,76, "%s: req discard due status monitor problem\n"), progname);
			sigsetmask(oldmask);
				return;
				}

		/* If we are using UDP for the transport, save the information
		 * we need to be able to reply later.
		 */

		if (klm_transp_is_udp)
			svcudp_save_connection(klm_transp,&(req->connect_info));

		/* local routine replies individually */
		(*Local)(req);
		release_le(req);
		call_back();
		release_me();
		release_fe();
	}
	else { /* malloc failure */
		klm_reply((int) Rqstp->rq_proc, &res_nolock);
	}
	if(debug)
		pr_all();
	sigsetmask(oldmask);
	return;

abnormal:

	klm_reply((int) Rqstp->rq_proc, &res_nolock);
	req->rel = 1;
	release_le(req);
	sigsetmask(oldmask);
	return;
}


main(argc, argv)
int argc;
char ** argv;
{
	SVCXPRT *Transp;
	int c, max_fds;
	int t;
	int ppid;
	FILE *fopen();
	extern int optind, opterr;
	extern char *optarg;
	int	sem_val;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("lockd",0);
#endif NLS
        progname = argv[0];

#if defined(SecureWare) && defined(B1)
	/*
	 * Initialize the current effective privilege mask
	 */
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
			logmsg(catgets(nlmsg_fd,NL_SETN,62, "%s: Not authorized for network subsystem:  Permission denied\n", progname));
			semclose();
			exit(1);
		}
		nfs_initpriv();
	}
	/*
 	* This program assumes it will be started by the "epa" command and that 
 	* it will have the default privileges plus SEC_ALLOWDACACCESS,
 	* SEC_REMOTE, SEC_ALLOWMACACCESS and SEC_KILL.
 	* An ENABLEPRIV will be done on the privileges as there is no sense
 	* in proceeding if they are not available.  They will then be disabled
 	* until they are needed later where they will be raised and lowered.
 	*/

	/*
	 * Since the program should be started with the
	 * SEC_ALLOWDACACCESS, SEC_REMOTE, SEC_ALLOWMACACCESS 
	 * and SEC_KILL privileges,
	 * disable them until they are needed
	 */
	ENABLEPRIV(SEC_ALLOWDACACCESS);	
	ENABLEPRIV(SEC_ALLOWMACACCESS);	
	ENABLEPRIV(SEC_REMOTE);	
	ENABLEPRIV(SEC_KILL);	

	DISABLEPRIV(SEC_ALLOWDACACCESS);	
	DISABLEPRIV(SEC_ALLOWMACACCESS);	
	DISABLEPRIV(SEC_REMOTE);	
	DISABLEPRIV(SEC_KILL);	
#endif /* SecureWare && B1 */

        /* catch signals so that we can log a message before exiting */
	(void) signal(SIGHUP, handle);
	(void) signal(SIGINT, handle);
	(void) signal(SIGTERM, handle);

#ifdef	BFA
	/*	HPNFS	jwe     88.10.1
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	*/
	(void) signal(SIGQUIT, handle);
#endif	BFA

	/* check for running lockd */
	if  ((sem_val=seminit()) != 0) {
	    logmsg((catgets(nlmsg_fd,NL_SETN,60, 
			"%s: already started\n")),progname);
	    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,60, 
			"%s: already started\n")), progname);
	    exit(1);	
	}

	LogFile[0] = '\0';

	LM_GRACE = LM_GRACE_DEFAULT;
	LM_TIMEOUT = LM_TIMEOUT_DEFAULT;
	HASH_SIZE = 29;
        report_sharing_conflicts = 0;

#ifdef SOFT_BREAK
	soft_break = FALSE;
	soft_timeout = MAX_LM_TIMEOUT_COUNT;
#endif SOFT_BREAK

        opterr = 0;
#ifdef SOFT_BREAK
	while((c = getopt(argc, argv, "s:t:d:g:h:l:S:")) != EOF)
#else
	while((c = getopt(argc, argv, "s:t:d:g:h:l:")) != EOF)
#endif SOFT_BREAK
		switch(c) {
		case 't':
			sscanf(optarg, "%d", &LM_TIMEOUT);
			break;
		case 'd':
			sscanf(optarg, "%d", &debug);
			break;
		case 'g':
			sscanf(optarg, "%d", &t);
			LM_GRACE = 1 + t/LM_TIMEOUT;
			break;
		case 'h':
			sscanf(optarg, "%d", &HASH_SIZE);
			break;
#ifdef SOFT_BREAK
 		case 'S':
 			sscanf(optarg, "%d", &t);
			/*
                         * subtract 1 because counter initialized to zero
			 *  and checked in xtimer() before incremented.
			 */
 			soft_timeout = (t/LM_TIMEOUT) - 1;
			if ( soft_timeout < 0 )
			   soft_timeout = 0;
 			soft_break = TRUE;
 			break;
#endif SOFT_BREAK
		case 'l':
			sscanf(optarg, "%s", LogFile);
			break;
                case 's':
                        report_sharing_conflicts++;
                        break;
		default :
#ifdef SOFT_BREAK
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,95, "Usage: %s [ -t timeout] [ -g grace_period] [ -S soft_timeout] [ -l logfile]\n")), progname);
#else SOFT_BREAK
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,95, "Usage: %s [ -t timeout] [ -g grace_period] [ -l logfile]\n")), progname);
#endif SOFT_BREAK
			semclose();
			exit(1);
		}

	(void)startlog( *argv, LogFile);

	if(debug) {
		(void) signal(SIGHUP, SIG_IGN);
		logmsg((catgets(nlmsg_fd,NL_SETN,78, "%1$s: lm_timeout = %2$d secs, grace_period = %3$d secs, hashsize = %4$d\n")), progname, LM_TIMEOUT,  LM_GRACE * LM_TIMEOUT, HASH_SIZE);
	}

	if(!debug) {
		ppid = fork();
		if(ppid == -1) {
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,80, "%s: in.lockd: fork failure : ABORT in main\n"), progname);
			semclose();
			abort();
		}
		if(ppid != 0) {
			exit(0);
		}
                max_fds = getnumfds();
                for (t = 0; t < max_fds; t++) {
                        (void) close(t);
		}

		ENABLEPRIV(SEC_ALLOWDACACCESS);
		ENABLEPRIV(SEC_ALLOWMACACCESS);
		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		DISABLEPRIV(SEC_ALLOWMACACCESS);

		setpgrp(0, 0);
	}
	else {
/* JWE - may want to clean this up instead of dropping it
		setlinebuf(stderr);
		setlinebuf(stdout);
*/
	}
	/* Store the pid of lockd in the semaphore structure.  */
	semsetpid();

	signal(SIGALRM, xtimer);
	/* NLM declaration */
	pmap_unset(NLM_PROG, NLM_VERS);

	Transp = svctcp_create(RPC_ANYSOCK, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (Transp == NULL) {
		logmsg(catgets(nlmsg_fd,NL_SETN,81, "%s: cannot create tcp service.\n"), progname);
		semclose();
		exit(1);
	}
	if (! svc_register(Transp, NLM_PROG, NLM_VERS, nlm_prog, IPPROTO_TCP)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,82, "%s: unable to register (NLM_PROG, NLM_VERS, tcp).\n"), progname);
		semclose();
		exit(1);
	}

	Transp = svcudp_bufcreate(RPC_ANYSOCK, LOCKUDPMSGSIZE,LOCKUDPMSGSIZE);
	if (Transp == NULL) {
		logmsg(catgets(nlmsg_fd,NL_SETN,83, "%s: cannot create udp service.\n"), progname);
		semclose();
		exit(1);
	}
	if (! svc_register(Transp, NLM_PROG, NLM_VERS, nlm_prog, IPPROTO_UDP)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,84, "%s: unable to register (NLM_PROG, NLM_VERS, udp).\n"), progname);
		semclose();
		exit(1);
	}
	svcudp_enablecache(Transp, 15);

        /* NLM V3 declaration */
        pmap_unset(NLM_PROG, NLM_VERSX);
 
        Transp = svctcp_create(RPC_ANYSOCK, 0, 0);
        if (Transp == NULL) {
		logmsg((catgets(nlmsg_fd,NL_SETN,97,"%s: cannot create tcp service.\n")), progname);
		semclose();
                exit(1);
        }
        if (!svc_register(Transp, NLM_PROG, NLM_VERSX, nlm_prog, IPPROTO_TCP)) {
		logmsg((catgets(nlmsg_fd,NL_SETN,98,"%s: unable to register (NLM_PROG, NLM_VERSX, tcp).\n")),progname);
		semclose();
                exit(1);
        }
 
        Transp = svcudp_bufcreate(RPC_ANYSOCK, LOCKUDPMSGSIZE, LOCKUDPMSGSIZE);
        if (Transp == NULL) {
		logmsg((catgets(nlmsg_fd,NL_SETN,99,"%s: cannot create udp service.\n")), progname);
		semclose();
                exit(1);
        }
        if (!svc_register(Transp, NLM_PROG, NLM_VERSX, nlm_prog, IPPROTO_UDP)) {
		logmsg((catgets(nlmsg_fd,NL_SETN,100,"%s: unable to register (NLM_PROG, NLM_VERSX, udp).\n")),progname);
		semclose();
                exit(1);
        }
        (void) svcudp_enablecache(Transp, 15);

	/* KLM declaration */
	pmap_unset(KLM_PROG, KLM_VERS);

	Transp = svcudp_bufcreate(RPC_ANYSOCK, LOCKUDPMSGSIZE,LOCKUDPMSGSIZE);
	if (Transp == NULL) {
		logmsg(catgets(nlmsg_fd,NL_SETN,85, "%s: cannot create udp service.\n"), progname);
		semclose();
		exit(1);
	}
	if (! svc_register(Transp, KLM_PROG, KLM_VERS, klm_prog, IPPROTO_UDP)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,86, "%s: unable to register (KLM_PROG, KLM_VERS, udp).\n"), progname);
		semclose();
		exit(1);
	}
	svcudp_enablecache(Transp, 15);

	Transp = svctcp_create(RPC_ANYSOCK, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (Transp == NULL) {
		logmsg(catgets(nlmsg_fd,NL_SETN,87, "%s: cannot create tcp service.\n"), progname);
		semclose();
		exit(1);
	}
	if (! svc_register(Transp, KLM_PROG, KLM_VERS, klm_prog, IPPROTO_TCP)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,88, "%s: unable to register (KLM_PROG, KLM_VERS, tcp).\n"), progname);
		semclose();
		exit(1);
	}

	/* PRIV declaration */
	pmap_unset(PRIV_PROG, PRIV_VERS);

	Transp = svctcp_create(RPC_ANYSOCK, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (Transp == NULL) {
		logmsg(catgets(nlmsg_fd,NL_SETN,89, "%s: cannot create tcp service.\n"), progname);
		semclose();
		exit(1);
	}
	if (! svc_register(Transp, PRIV_PROG, PRIV_VERS, priv_prog, IPPROTO_TCP)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,90, "%s: unable to register (KLM_PROG, KLM_VERS, tcp).\n"), progname);
		semclose();
		exit(1);
	}

	init();
	init_nlm_share();

	if(debug == 3) {
		klm = KLM_PROG;
		nlm = NLM_PROG;
		/*
		xdrstdio_create(&x, fp, XDR_ENCODE);
		*/
	}
	alarm(LM_TIMEOUT);
	pmap_svc_run(run_queue);
	logmsg(catgets(nlmsg_fd,NL_SETN,93, "%s: svc_run returned\n"),progname);
	semclose();
	exit(1);
}
