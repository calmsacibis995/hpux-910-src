/* @(#)rpc.lockd:	$Revision: 1.16.109.3 $	$Date: 95/03/21 13:30:48 $
*/
/* (#)prot_pnlm.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_pnlm.c	1.3 87/06/18 NFSSRC */
#ifndef lint
static char sccsid[] = "@(#)prot_pnlm.c 1.rb5 86/09/24 Copyr 1986 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_5380="@(#) PATCH_9.X: prot_pnlm.o $Revision: 1.16.109.3 $ 95/03/21 PHNE_5380";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_pnlm.c
	 * consists of all procedures called bu nlm_prog
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

extern int debug;
extern reclock *call_q;
extern SVCXPRT *nlm_transp;
extern char *progname;

extern msg_entry *search_msg(), *retransmitted(), *reply_queue();
extern remote_result *local_lock(), *local_unlock(), *local_test(), *local_cancel(), *local_granted(), *local_granted_msg();
extern remote_result *cont_test(), *cont_lock(), *cont_unlock(), *cont_cancel(), *cont_reclaim();

proc_nlm_test(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,121, "proc_nlm_test(%x) \n")), a);
	result = local_test(a);
	nlm_reply(NLM_TEST, result);
}

proc_nlm_lock(a)
reclock *a;
{
	remote_result *result;


	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,122, "enter proc_nlm_lock(%x) \n")), a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK, result);
}

proc_nlm_cancel(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,123, "enter proc_nlm_cancel(%x) \n")), a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL, result);
}

proc_nlm_unlock(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,124, "enter proc_nlm_unlock(%x) \n")), a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK, result);
}

proc_nlm_granted(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,125, "enter proc_nlm_granted(%x)\n")), a);
	result = local_granted(a, RPC);
	if(result != NULL) {
		nlm_reply(NLM_GRANTED, result, a);
	}
}

proc_nlm_test_msg(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,126, "enter proc_nlm_test_msg(%x)\n")), a);
	result = local_test(a);
	nlm_reply(NLM_TEST_MSG, result, a);
}

proc_nlm_lock_msg(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,127, "enter proc_nlm_lock_msg(%x)\n")), a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK_MSG, result, a);
}

proc_nlm_cancel_msg(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,128, "enter proc_nlm_cancel_msg(%x)\n")), a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL_MSG, result, a);
}

proc_nlm_unlock_msg(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,129, "enter proc_nlm_unlock_msg(%x)\n")), a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK_MSG, result, a);
}

proc_nlm_granted_msg(a)
reclock *a;
{
	remote_result *result;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,130, "enter proc_nlm_granted_msg(%x)\n")), a);
	result = local_granted(a, MSG);
	if(result != NULL) 
		nlm_reply(NLM_GRANTED_MSG, result, a);
}

/* 
 * return rpc calls;
 * if rpc calls, directly reply to the request;
 * if msg passing calls, initiates one way rpc call to reply!
 */
nlm_reply(proc, reply, a)
int proc;
remote_result *reply;
reclock *a;
{
	bool_t (*xdr_reply)();
	int act;
	int nlmreply = 1;
	int newcall = 2;
	int rpc_err;
	char *name;
	int valid;

	switch(proc) {
	case NLM_TEST:
		xdr_reply = xdr_nlm_testres;
		act = nlmreply;
		break;
	case NLM_LOCK:
	case NLM_CANCEL:
	case NLM_UNLOCK:
	case NLM_GRANTED:
		xdr_reply = xdr_nlm_res;
		act = nlmreply;
		break;
	case NLM_TEST_MSG:
		xdr_reply = xdr_nlm_testres;
		act = newcall;
		proc = NLM_TEST_RES;
		name = a->lck.clnt;
		break;
	case NLM_LOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_LOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_CANCEL_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_CANCEL_RES;
		name = a->lck.clnt;
		break;
	case NLM_UNLOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_UNLOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_GRANTED_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_GRANTED_RES;
		name = a->lck.svr;
		break;
	default:
		logmsg(catgets(nlmsg_fd,NL_SETN,131, "%1$s: unknown nlm_reply proc vaule: %2$d\n"), progname, proc);
		return;
	}
	if(act == nlmreply) { /* reply to nlm_transp */
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,132, "rpc nlm_reply %1$d: %2$d\n")), proc, reply->lockstat);
		 if(!svc_sendreply(nlm_transp, xdr_reply, reply)) 
			svcerr_systemerr(nlm_transp);
		return;
	}
	else { /* issue a one way rpc call to reply */
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,133, "nlm_reply: (%1$s, %2$d), result = %3$d\n")), name, proc, reply->lockstat);
		reply->cookie_len = a->cookie_len;
		reply->cookie_bytes = a->cookie_bytes;
		valid = 1;
		if((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, proc,
		 xdr_reply, reply, xdr_void, NULL, valid, 0))
		 != (int) RPC_TIMEDOUT && rpc_err != (int) RPC_SUCCESS) {
			/* in case of error, print out error msg */
			logclnt_perrno(rpc_err);
		}
		else if ( rpc_err == RPC_TIMEDOUT ) {
			/*
			 * Need to add to a new queue until we get response
			 * from portmap on remote system.
			 */
			if (reply_queue(proc, a, reply) == NULL) {
			    logmsg("Cannot reply to client %s due to malloc problem");
			}
		}
	}
	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,135, "exit nlm_reply\n")));

}

proc_nlm_test_res(reply)
remote_result *reply;
{
	nlm_res_routine(reply,  cont_test);
}

proc_nlm_lock_res(reply)
remote_result *reply;
{
	nlm_res_routine(reply,  cont_lock);
}

proc_nlm_cancel_res(reply)
remote_result *reply;
{
	nlm_res_routine(reply,  cont_cancel);
}

proc_nlm_unlock_res(reply)
remote_result *reply;
{
	nlm_res_routine(reply,  cont_unlock);
}

/*
 * common routine shared by all nlm routines that expects replies from svr nlm: 
 * nlm_lock_res, nlm_test_res, nlm_unlock_res, nlm_cancel_res
 * private routine "cont" is called to continue local operation;
 * reply is matched with msg in msg_queue according to cookie
 * and then attached to msg_queue;
 */
nlm_res_routine(reply, cont)
remote_result *reply;
remote_result *(*cont)();
{
	msg_entry *msgp;
	remote_result *resp;

	if((msgp = search_msg(reply)) != NULL) {	/* found */
		if(msgp->reply != NULL) { /* reply already exists */
			if(msgp->reply->lockstat != reply->lockstat) {
				logmsg(catgets(nlmsg_fd,NL_SETN,136, "%s: inconsistent  lock reply exists, ignored \n"), progname);
				if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,137, "inconsistent reply (%1$d, %2$d) exists for lock(%3$x)\n")), msgp->reply->lockstat, reply->lockstat, msgp->req);
			}
			release_res(reply);
			return;
		}
		/* continue process req according to remote reply */
		if(debug) {
			logmsg((catgets(nlmsg_fd,NL_SETN,138, "nlm_res_routine(%x)\n")), msgp->req);
			fflush(stdout);
		}
		if(msgp->proc == NLM_LOCK_RECLAIM) 
			/* reclaim response */
			resp = cont_reclaim(msgp->req, reply);
		else 
			/* normal response */
			resp = cont(msgp-> req , reply);	
		add_reply(msgp, resp);
	}
	else
		release_res(reply);	/* discard this resply */
}

proc_nlm_granted_res(reply)
remote_result *reply;
{
	msg_entry *msgp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,139, "enter nlm_granted_res\n")));
	if((msgp = search_msg(reply)) != NULL)
		dequeue(msgp);
}

/*
 * rpc msg passing calls to nlm msg procedure;
 * used by local_lock, local_test, local_cancel and local_unloc;
 * proc specifis the name of nlm procedures;
 * retransmit indicate whether this is retransmission;
 * rpc_call return -1 if rpc call is not successful, clnt_perrno is printed out;
 * rpc_call return 0 otherwise
 */
nlm_call(proc, a, retransmit)
int proc;
reclock *a;
int retransmit;
{
	int rpc_err;
	bool_t (*xdr_arg)();
	char *name;
	int func;
	int valid;
	remote_result bad_result;
	remote_result *bad_resp = &bad_result;
        msg_entry *msgp;

	func = proc;		/* this is necc for NLM_LOCK_RECLAIM */
	if(retransmit == 0)
		valid = 1;	/* use cache value for first time calls */
	else
		valid = 0;		/* invalidate cache */
	switch(proc) {
	case NLM_TEST_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.svr;
		break;
	case NLM_LOCK_MSG:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		break;
	case NLM_LOCK_RECLAIM:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		func = NLM_LOCK_MSG;
		valid = 0; 	/* turn off udp cache */
		break;
	case NLM_CANCEL_MSG:
		xdr_arg = xdr_nlm_cancargs;
		name = a->lck.svr;
		break;
	case NLM_UNLOCK_MSG:
		xdr_arg = xdr_nlm_unlockargs;
		name = a->lck.svr;
		break;
	case NLM_GRANTED_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.clnt;
		/* modify caller name */
		a->lock.caller_name = a->lock.server_name;
		break;
	default:
		logmsg(catgets(nlmsg_fd,NL_SETN,140, "%1$s: %2$d not supported in nlm_call\n"), progname, proc);
		return(-1);
	}

	if(debug)
	   logmsg( (catgets(nlmsg_fd,NL_SETN,141, "nlm_call to (%1$s, %2$d) op=%3$d, (%4$d, %5$d); retran = %6$d, valid = %7$d\n")), name, proc, a->lck.op, a->lck.l_offset, a->lck.l_len, retransmit, valid);
	/* 
 	 * call is a one way rpc call to simulate msg passing
 	 * no timeout nor reply is specified;
 	 */
	if(((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, func, xdr_arg, a,
			    xdr_void, NULL, valid, 0)) == (int) RPC_SUCCESS ) 
			|| (rpc_err == RPC_TIMEDOUT) ) {
		/* if rpc call is successful, add msg to msg_queue */
		if(retransmit == 0)	/* first time calls */
			if(queue(a, proc) == NULL) {
				return(-1);
			}
		return(0);
	}
	else {
		logclnt_perrno(rpc_err);
		/*
		 * If we get PROGNOTREGISTERED that means that lockd is not
 		 * up on the remote system, so we return ENOLCK to indicate
		 * that we have a problem, and delete the request.  
                 * However, if it's an unlock 
		 * request, or a GRANTED request, we have to be sure that
		 * those get through or we could be left in an inconsistent
		 * state that would be nasty.
		 */
		if ((rpc_err == RPC_PROGNOTREGISTERED) &&
		    (proc != NLM_UNLOCK_MSG) && (proc != NLM_GRANTED_MSG)) {
		   if ( retransmit && ((msgp = retransmitted(a, proc)) != NULL))
		   {
			msgp->req->rel = 1;
			dequeue(msgp);
		   }
		   bad_resp->lockstat = nolocks;
		   klm_reply(proc, bad_resp);
		}
		return(-1);
	}
} 

call_back()
{
	reclock *nl;

	if(call_q == NULL)		/* no need to call back */
		return;
	nl = call_q;
	while( nl!= NULL) {
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,143, "enter call_back(%1$d, %2$d), op =%3$d\n")), nl->lck.l_offset, nl->lck.l_len, nl->lck.op);
		if(nlm_call(NLM_GRANTED_MSG, nl, 0) == -1)
		{
			logmsg((catgets(nlmsg_fd,NL_SETN,144, "nlm_call error: ABORT in call_back\n")));
			/*abort(); */
                }
		nl = nl->nxt;
	}
	call_q = NULL;
}




/*
 * This procedure is ostensibly for mapping from VERS to VERSX.
 * In practice it _probably_ only applies to PC-NFS clients. However
 * the protocol specification specifically allows the length of
 * a lock to be an unsigned 32-bit integer, so it is possible to
 * imagine other cases (especially non-Unix).
 *
 * Even this mapping is inadequate. A better version would simply
 * test the sign bit, and treat ALL "negative" values as 0. We
 * may still get there in this bugfix.
 *
 * Geoff - RB DTS INDaa12755
 */
void
nlm_versx_conversion(procedure, req)
	int	procedure;
	reclock   *req;
{
  if(debug) logmsg(("enter nlm_versx_conversion"));

	switch (procedure) {
	case NLM_LOCK:
	case NLM_NM_LOCK:
	case NLM_UNLOCK:
	case NLM_CANCEL:
	case NLM_TEST:
                /*HPNFS Patch # PHNE_4056 
                  INDaa17497 fix l_offset is negative or the sum 
                  of l_offset and l_len is exceeded MAXLEN - hv
                 */

		if(req->lock.l_offset > (u_int) MAXLEN) 
                {
                  if (debug) 
                      logmsg(
                       "Reset invalid lock offset to MAXLEN and the len to 0");
	          req->lock.l_offset= MAXLEN;
	          req->lock.l_len= 0;
                }
                else
                {
                  if ((req->lock.l_offset)+(req->lock.l_len) > (u_int)MAXLEN)
                  {
                    if (debug) 
                      logmsg(
                       "The sum of offset and record len exceed MAXLEN");
                      logmsg(
                       "  Reset record len to %d",MAXLEN-(req->lock.l_offset));
	            req->lock.l_len= MAXLEN - req->lock.l_offset;
                  }
                }

		return;
	default:
		return;
	}

}
