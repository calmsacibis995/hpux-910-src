/* @(#)rpc.lockd:	$Revision: 1.17.109.3 $	$Date: 94/10/21 10:36:38 $
*/
/* (#)prot_msg.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_msg.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "@(#)prot_msg.c 1.3.rb 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/* prot_msg.c
 	 * consists all routines handle msg passing
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

#ifndef NULL
#define NULL 0
#endif

#include "prot_lock.h"
#include "prot_time.h"
#include <signal.h>

extern int handle();
#ifdef SOFT_BREAK
extern int soft_break;
extern int soft_timeout;
#endif SOFT_BREAK
extern int debug;
extern int grace_period;
extern int msg_len;
extern remote_result res_working;
char *xmalloc();

extern remote_result *get_res();
extern reclock *copy_le();

msg_entry *klm_msg;	/* ptr last msg to klm in msg queue */
msg_entry *msg_q;	/* head of msg queue */
msg_entry *reply_q;	/* head of reply queue -- reply messages waiting for
				the port of clnt lockd */


/*
 * signal handler:
 * wake up periodically to check retransmiting status and reply to last req
 */
xtimer()
{
	msg_entry *msgp, *next;
#ifdef SOFT_BREAK
	remote_result *dead_resp;
#endif SOFT_BREAK

	signal(SIGALRM, handle);
	alarm(0);
	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,101, "\nalarm! enter xtimer:\n")));
	if(grace_period > 0) {
		/* reduce the remaining grace period */
		grace_period--;
		if(grace_period == 0) {
			if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,102, "**********end of grace period\n")));
			/* remove proc == klm_xxx in msg queue */
			next = msg_q;
			while((msgp = next) != NULL) {
				next = msgp->nxt;
				if(msgp->proc == KLM_LOCK ||
				   msgp->proc == KLM_UNLOCK ||
				   msgp->proc == KLM_TEST ||
				   msgp->proc == KLM_CANCEL) {
					if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,103, "remove grace period msg (%x) from msg queue\n")), msgp);
					dequeue(msgp);
				}
			}
		}
	}

	next = msg_q;
	while((msgp = next) != NULL) {
		next = msgp->nxt;
		if(msgp->reply == NULL) { /* check for retransimssion */
			/* KLM_LOCK is for local blocked locks */
			if(msgp->proc != KLM_LOCK) {
#ifdef SOFT_BREAK
			   if ((soft_break) && (msgp->t.curr >= soft_timeout)) {
			      if( (dead_resp = get_res()) != NULL ) {
				  dead_resp->lockstat = nolocks;
				  add_reply(msgp, dead_resp);
				  logmsg("Timing out request to %s\n",
					msgp->req->lck.svr);
			      }
                           }
			   else {
#endif SOFT_BREAK
				if(msgp->t.exp == (msgp->t.curr % 2)) {
				/* retransmit this time */
					if(debug)
						logmsg((catgets(nlmsg_fd,NL_SETN,104, "xtimer retransmit: ")));
					(void) nlm_call(msgp->proc,
							msgp->req, 1);
                                        if (msgp->t.exp == 0) {
					   msgp->t.exp = 1;
					   msgp->t.curr = 1;
                                        }
					else {
					   /* double timeout period */
					   msgp->t.exp = 2 * msgp->t.exp;
#ifdef SOFT_BREAK
                                           if (soft_break)
					      msgp->t.curr++;
					   else
#endif SOFT_BREAK
					      msgp->t.curr = 0;
                                        }
					if(msgp->t.exp > MAX_LM_TIMEOUT_COUNT) {
					     msgp->t.exp = MAX_LM_TIMEOUT_COUNT;
					}
				}
				else 	/* increment current count */
					msgp->t.curr++;
#ifdef SOFT_BREAK
			   }
#endif SOFT_BREAK
			}
		}
		else { /* check if reply is sitting there too long */
			if(msgp->reply->lockstat != blocking) {
		
				if(msgp->t.curr > OLDMSG) /* discard this msg */
					dequeue(msgp);
				else
					msgp->t.curr++;
			}
		}
	}

	/* send back working reply for last req received in klm */
	if(klm_msg != NULL) {	
		if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,105, "klm_msg = %x\n")), klm_msg->req);
		add_reply(klm_msg, NULL);
	}
	else if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,106, "klm_msg == NULL\n")));

	/*
	 * Code added to run the reply_q retransmitting responses
	 */
	run_reply_queue(NULL);

	/* added signal() call because signal gets cleared */
	signal(SIGALRM, xtimer);
	if(grace_period != 0 || msg_q != NULL || reply_q != NULL )
        {
		alarm(LM_TIMEOUT);
        }
}


/*
 * retransmitted searches through msg_q to determine if "a" is a
 * retransmission of a previously received msg;
 * it returns the addr of the msg entry if "a" is found
 * otherwise, it returns NULL
 */
msg_entry *
retransmitted(a, proc)
reclock *a;
int proc;
{
	msg_entry *msgp;

	msgp = msg_q;
	while(msgp != NULL) {
		if(same_lock(msgp->req, a) || simi_lock(msgp->req, a)) {
		  /* 5 is the constant diff between rpc calls and msg passing */
			if((msgp->proc == NLM_LOCK_RECLAIM &&
			(proc == KLM_LOCK || proc == NLM_LOCK_MSG))
		 || msgp->proc == proc + 5 || msgp->proc == proc) 
				return(msgp);
		}
		msgp = msgp->nxt;
	}
	return(NULL);
}


/*
 * search_msg2   searches through msg_q to determine if "a" 
 * matches the reclock pointed to any message in the message 
 * it returns the addr of the msg entry if "a" is found
 * otherwise, it returns NULL
 */
msg_entry *
search_msg2(a)
reclock *a;
{
	msg_entry *msgp;

	msgp = msg_q;
	while(msgp != NULL) {
		if(msgp->req == a ) {
				return(msgp);
		}
		msgp = msgp->nxt;
	}
	return(NULL);
}


/*
 * search_msg3 searches through msg_q to determine if  the cookie "a" 
 * matches the cookie in any message in the message_queue
 * it returns the addr of the msg entry if "a" is found
 * otherwise, it returns NULL
 */
msg_entry *
search_msg3(a)
reclock *a;
{
	msg_entry *msgp;
	reclock *req;

	msgp = msg_q;
	while(msgp != NULL) {
		req = msgp->req;
		if(obj_cmp(&req->cookie, &a->cookie))
				return(msgp);
		msgp = msgp->nxt;
	}
	return(NULL);
}

/*
 * match response's cookie with msg req
 * either return msgp or NULL if not found
 */
msg_entry *
search_msg(resp)
remote_result *resp;
{
	msg_entry *msgp;
	reclock *req;

	msgp = msg_q;
	while(msgp != NULL) {
		req = msgp->req;
		if(obj_cmp(&req->cookie, &resp->cookie))
			return(msgp);
		msgp = msgp->nxt;
	}
	return(NULL);
}


/*
 * add a to msg queue; called from nlm_call: when
 * rpc call is succ and reply is needed
 * proc is needed for sending back reply later
 * if case of error, NULL is returned;
 * otherwise, the msg entry is returned
 */
msg_entry *
queue(a, proc)
reclock *a;
int proc;
{
	msg_entry *msgp;

	if((msgp = (msg_entry *) xmalloc(msg_len)) == NULL)
		return(NULL);
	memset((char *) msgp, 0, msg_len);
	msgp->req = a;
	msgp->proc = proc;
/* - leave it 0
	msgp->t.exp = 1;
*/

	/* insert msg into msg queue */
	if(msg_q == NULL) {
		msgp->nxt = msgp->prev = NULL;
		msg_q = msgp;
		/* turn on alarm only when there are msgs in msg queue */
	        /* added signal() call because signal gets cleared */
		if ( reply_q == NULL ) {
		    signal(SIGALRM, xtimer);
		    if(grace_period == 0)
			    alarm(LM_TIMEOUT);
		}
	}
	else {
		msgp->nxt = msg_q;
		msgp->prev = NULL;
		msg_q->prev = msgp;
		msg_q = msgp;
	}

	if( proc != NLM_GRANTED_MSG && proc != NLM_LOCK_RECLAIM)
		klm_msg = msgp;			/* record last msg to klm */
	return(msgp);
}

/*
 * dequeue remove msg from msg_queue;
 * and deallocate space obtained  from malloc
 * lockreq is release only if a->rel == 1;
 */
dequeue(msgp)
msg_entry *msgp;
{
	/*
	 * First, delete msg from msg queue since dequeue(),
	 * release_le() and dequeue_reclock() are recursive.
	 */
	if (msgp->prev != NULL)
		msgp->prev->nxt = msgp->nxt;
	else
		msg_q = msgp->nxt;
	if (msgp->nxt != NULL)
		msgp->nxt->prev = msgp->prev;

        if (msg_q == NULL)
                klm_msg = NULL;

	if (msgp->req != NULL)
		release_le(msgp->req);
	if (msgp->reply != NULL)
		release_res(msgp->reply);


        xfree(&msgp);
}

/*
 * if resp is not NULL, add reply to msg_entry and reply if msg is last req;
 * otherwise, reply working
 */
add_reply(msgp, resp )
msg_entry *msgp;
remote_result *resp;
{

	if( resp != NULL) {
		extern int klm_transp_is_udp;

		msgp->t.curr = 0;  /* reset timer counter to record old msg */
		msgp->reply = resp; 
		if ((klm_msg == msgp) ||
			((msgp->req->connect_info.port != 0) &&
			 klm_transp_is_udp)) { /* reply immediately */ 
			int save[2];
			extern SVCXPRT *klm_transp;

			/* Make sure we are talking to the correct connection */
			if (klm_transp_is_udp &&
					(msgp->req->connect_info.port != 0)) {

				svcudp_save_connection(klm_transp, save);
				svcudp_reset_connection(klm_transp,
					      &(msgp->req->connect_info.port));
			}
			klm_reply(msgp->proc, resp); 

			/* Reset back to the old connection */
			if (msgp->req->connect_info.port != 0) {

				svcudp_reset_connection(klm_transp, save);
			}

			/* prevent timer routine reply "working" to */
			/* already replied req */
			klm_msg = NULL;
			if(resp->lockstat != blocking) 
				dequeue(msgp); 
		}        
	}
	else /* res == NULL, used by xtimer */ 
		if(klm_msg == msgp) {
			if(debug)
				logmsg((catgets(nlmsg_fd,NL_SETN,107, "xtimer reply to (%x): ")), msgp->req);
			klm_reply(msgp->proc, &res_working);
		}
}

/*
 * copy_reply() -- used by reply_queue() to save a copy of a reply.
 * Malloc's space for reply, and also (possibly) two object handles.
 */

remote_result *
copy_reply( reply , proc)
remote_result *reply;
{
	remote_result *new;

	if ( (new = get_res()) == NULL)
		return(NULL);
	*new = *reply;
	if ( obj_copy(&new->cookie, &reply->cookie) ) {
		xfree(&new);
		return(NULL);
	}
	if ( proc == NLM_TEST_RES && reply->stat.stat == denied )
		if ( obj_copy(&new->lholder.oh, &reply->lholder.oh) ) {
		    xfree(&new->cookie.n_bytes);
		    xfree(&new);
		    return(NULL);
		}
	return(new);
}

free_reply( reply, proc )
remote_result *reply;
int proc;
{
	if ( proc == NLM_TEST_RES && reply->stat.stat == denied )
		xfree(&reply->lholder.oh.n_bytes);
	release_res(reply);
}

/*
 * reply_queue() -- Add to the queue of reply's waiting to get a port #.
 * Called from nlm_reply() after sending out the initial request if
 * call_udp() needed to wait for the port # of the remote lockd.
 */

msg_entry *
reply_queue( proc, a, reply )
int proc;
reclock *a;
remote_result *reply;
{
	msg_entry *msgp;

	if ( debug > 2)
		logmsg("reply_queue(proc = %d, clnt = %s, svr = %s)",
			proc, a->lck.clnt, a->lck.svr);

	if((msgp = (msg_entry *) xmalloc(msg_len)) == NULL)
		return(NULL);
	msgp->proc = proc;
	if ( (msgp->req = copy_le(a)) == NULL ) {
		xfree(&msgp);
		return (NULL);
	}
	if ( (msgp->reply = copy_reply(reply,proc)) == NULL ) {
		free_le(msgp->req);
		xfree(&msgp);
		return (NULL);
	}

	/* insert msg into msg queue */
	if(reply_q == NULL) {
		msgp->nxt = msgp->prev = NULL;
		reply_q = msgp;
		/* turn on alarm only when there are msgs in msg queue */
	        /* added signal() call because signal gets cleared */
		if ( msg_q == NULL ) {
		    signal(SIGALRM, xtimer);
		    if(grace_period == 0)
			    alarm(LM_TIMEOUT);
		}
	}
	else {
		msgp->nxt = reply_q;
		msgp->prev = NULL;
		reply_q->prev = msgp;
		reply_q = msgp;
	}

	return(msgp);
}

/*
 * reply_dequeue() -- remove a reply from the reply queue and free those
 * things we malloc'd earlier.
 */

reply_dequeue(msgp)
msg_entry *msgp;
{
	/*
	 * First, delete msg from msg queue since dequeue(),
	 * release_le() and dequeue_reclock() are recursive.
	 */
	if (msgp->prev != NULL)
		msgp->prev->nxt = msgp->nxt;
	else
		reply_q = msgp->nxt;
	if (msgp->nxt != NULL)
		msgp->nxt->prev = msgp->prev;

	if (msgp->req != NULL)
		free_le(msgp->req);
	if (msgp->reply != NULL)
		free_reply(msgp->reply, msgp->proc);
        xfree(&msgp);
}

/*
 * run_queue() -- Added in support of asynchronous portmap requests.  This
 * function is called when the response from portmap comes in, and simply
 * searches the queue looking for requests that might have been waiting for
 * that response.  If found, then we call nlm_call().
 */

run_queue(host)
char *host;
{
	msg_entry *msgp, *next;
	int valid = 1;

	next = msg_q;
	while((msgp = next) != NULL) {
	    next = msgp->nxt;
	    if(msgp->reply == NULL) { /* check for retransimssion */
		    /* KLM_LOCK is for local blocked locks */
		    if(msgp->proc != KLM_LOCK) {
			    if ( !strcmp(host, msgp->req->lck.svr) ||
				( msgp->proc == NLM_GRANTED_MSG &&
				!strcmp(host, msgp->req->lck.clnt))) {
				    (void) nlm_call(msgp->proc,msgp->req,valid);
			    }
		    }
	    }
	}
	/*
	 * Check the reply queue for a possible match...
	 */
	run_reply_queue(host);

}

run_reply_queue(host)
char *host;
{
	bool_t (*xdr_reply)();
	char *name;
	enum clnt_stat rpc_err;
	int valid = 1;
	msg_entry *msgp, *next;

	/*
	 * Code added to run the reply_q retransmitting responses
	 */
	next = reply_q;
	while((msgp = next) != NULL) {
	    next = msgp->nxt;
	    xdr_reply = xdr_nlm_res;
	    name = msgp->req->lck.clnt;

	    /* Customize based on proc */
	    if ( msgp->proc == NLM_TEST_RES )
		    xdr_reply = xdr_nlm_testres;
	    else if (msgp->proc == NLM_GRANTED_RES)
		    name = msgp->req->lck.svr;
	    /*
	     * host == NULL then we are being called from xtimer(), retransmit
	     */

	    if ( host == NULL || !strcmp(host, name) ) {
		/*
                 * Try to call the remote system, if error is TIMEDOUT, still
		 * trying to contact remote, otherwise dequeue because we
		 * either succeeded in sending the response or we got a error
		 */
                if((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, msgp->proc,
		     xdr_reply, msgp->reply, xdr_void, NULL,
		     valid, 0)) != (int) RPC_TIMEDOUT) {
			if ( rpc_err != RPC_SUCCESS ) {
			    /* in case of error, print out error msg */
			    logclnt_perrno(rpc_err);
			}
			reply_dequeue(msgp);
                }
	    }
	}
}
