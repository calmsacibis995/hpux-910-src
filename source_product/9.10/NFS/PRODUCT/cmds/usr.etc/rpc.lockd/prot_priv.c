/* @(#)rpc.lockd:	$Revision: 1.13.109.1 $	$Date: 91/11/19 14:17:44 $
*/
/* (#)prot_priv.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_priv.c	1.3 87/06/18 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)prot_priv.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * consists of all private protocols for comm with
	 * status monitor to handle crash and recovery
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
extern nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include <rpcsvc/sm_inter.h>

extern int debug;
extern int pid;
extern char hostname[255];
extern int local_state;
extern struct msg_entry *retransmitted();
extern struct fs_rlck *find_fe();
extern char *progname;

void proc_priv_crash(), proc_priv_recovery();

void
priv_prog(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	char *(*Local)();
	struct status stat;
	extern bool_t xdr_status();

	switch(rqstp->rq_proc) {
	case PRIV_CRASH:
		Local = (char *(*)()) proc_priv_crash;
		break;
	case PRIV_RECOVERY:
		Local = (char *(*)()) proc_priv_recovery;
		break;
	default:
		svcerr_noproc(transp);
		return;
	}

	memset(&stat, 0, sizeof(struct status));
	if(! svc_getargs(transp, xdr_status, &stat)) {
		svcerr_decode(transp);
		return;
	}
	(*Local)(&stat);
	if(! svc_sendreply(transp, xdr_void, NULL)) {
		svcerr_systemerr(transp);
	}
	if (! svc_freeargs(transp, xdr_status, &stat)) {
		logmsg(catgets(nlmsg_fd,NL_SETN,151, "%s: unable to free arguments\n"), progname);
		semclose();
		exit(1);
	}
}

void
proc_priv_crash(statp)
struct status *statp;
{
	struct fs_rlck *mp, *fp;
	reclock *next, *nl, *insrtp;
	struct priv_struct *privp;

	privp = (struct priv_struct *) statp->priv;
	if(privp->pid != pid) {
		if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,152, "this is not for me(%1$d): %2$d\n")), privp->pid, pid); 
		return;
	}
	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,153, "enter proc_lm_crash due to %s failure\n")),
		statp->mon_name);

        destroy_client_shares(statp->mon_name);

	mp = (struct fs_rlck *) privp->priv_ptr; 
	if(strcmp(statp->mon_name, mp->svr) != 0) {
		if(debug)
		     logmsg((catgets(nlmsg_fd,NL_SETN,154, "crashed site is not my concern(%s)\n")), mp->svr);
		return;
	}
	delete_hash(statp->mon_name);
	next = mp->rlckp;
	while((nl = next) != NULL) {
		next = next->mnt_nxt;
		if(nl->state >= statp->state)	/* notice obsolete */
			continue;
		if(nl->w_flag == 1) {	/* lock blocked */
			if(debug)
				logmsg((catgets(nlmsg_fd,NL_SETN,155, "remove blocked lock (%x)\n")), nl);
			remove_wait(nl);
		}
		else {
			insrtp = NULL;
			fp = find_fe(nl);	/* return is not checked! */
			delete_le(fp, nl);
			wakeup(nl);
		}
		nl->rel = 1;
		release_le(nl);
	}
	release_fe();	/* should I move it into while loop ? */
	release_me();
}

void
proc_priv_recovery(statp)
struct status *statp;
{
	struct fs_rlck *mp;
	reclock *next, *nl;
	struct msg_entry *msgp;
	struct priv_struct *privp;

	privp = (struct priv_struct *) statp->priv;
	if(privp->pid != pid) {
		if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,156, "this is not for me(%1$d): %2$d\n")), privp->pid, pid); 
		return;
	}

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,157, "enter proc_lm_recovery due to %1$s state(%2$d)\n")), statp->mon_name, statp->state);

        destroy_client_shares(statp->mon_name);

	delete_hash(statp->mon_name);
	if(!up(statp->state)) 
		return;
	if(strcmp(statp->mon_name, hostname) == 0) {
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,158, "I have been declared as failed!!!\n")));
		/* update local status monitor number */
		local_state = statp->state;
	}

	mp = (struct fs_rlck *) privp->priv_ptr;
	if(strcmp(statp->mon_name, mp->svr) != 0) {
		if(debug)
		     logmsg((catgets(nlmsg_fd,NL_SETN,159, "recovered site is not my concern(%s)\n")), mp->svr);
		return;
	}
	next = mp->rlckp;
	while((nl = next) != NULL) {
		next = next->mnt_nxt;
		if(search_lock(nl) != NULL) {
		/* make sure the lock is not in the middle of being processed */
			if(nl-> w_flag == 0) {
				nl->reclaim = 1;
			}
			else {
			     if((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
					dequeue(msgp);
			     }
			     else 
			        logmsg(catgets(nlmsg_fd,NL_SETN,160, "%1$s: blocked req (%2$x) cannot be found in msg queue\n"), progname, nl);
			}
			if(nlm_call(NLM_LOCK_RECLAIM, nl, 0) == -1)
			/*rpc error */
				if(queue(nl, NLM_LOCK_RECLAIM) == NULL)
				   logmsg(catgets(nlmsg_fd,NL_SETN,161, "%1$s: reclaim requet (%2$x) cannot be sent and cannot be queued either!\n"), progname, nl);
		}
	}
}
