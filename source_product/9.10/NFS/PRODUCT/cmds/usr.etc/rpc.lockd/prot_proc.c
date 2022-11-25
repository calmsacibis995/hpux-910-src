/* @(#)rpc.lockd:	$Revision: 1.19.109.2 $	$Date: 91/11/27 15:28:16 $
*/
/* (#)prot_proc.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_proc.c	1.3 87/06/18 NFSSRC */
#ifndef lint
static char sccsid[] = "@(#)prot_proc.c 1.9.rb 86/09/24 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_proc.c 
	 * consists all local, remote, and continuation routines:
	 * local_xxx, remote_xxx, and cont_xxx.
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
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "prot_lock.h"
#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

#include "prot_sec.h"


remote_result nlm_result;		/* local nlm result */
remote_result *nlm_resp = &nlm_result;	/* ptr to klm result */

remote_result *remote_cancel(), *remote_lock(), *remote_test(), *remote_unlock();
remote_result *local_test(), *local_lock(), *local_cancel(), *local_unlock();

msg_entry *search_msg(), *search_msg2(), *search_msg3(), *retransmitted();
char *xmalloc();
struct fs_rlck *find_fe();
reclock *search_lock(), *search_block_lock();

extern int debug;
extern int res_len;
extern msg_entry *klm_msg;
extern char *progname;
extern char hostname[255];			/* for generating oh */
extern int host_len;				/* for generating oh */

remote_result *
local_lock(a)
reclock *a;
{
int f_cmd;
struct flock flock;
	struct fs_rlck *fp;
	reclock *insrtp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,171, "enter local_lock\n")));
	if(blocked(&fp, &insrtp, a) == 1) {  /*blocked*/
		if(a->lck.op & LOCK_NB) {
			nlm_resp->lockstat = denied;
			a->rel = 1;		/* to release a */
		}
		else { 				/* add_wait */
			add_wait(a);
			nlm_resp->lockstat = blocking;
		}
	}
	else { /* not blocked - by other remote locks */
        /* now try local kernel to check for local locks */
                if (a->block)
                   f_cmd = F_SETLKW;
	        else
                   f_cmd = F_SETLK;
                if (a->exclusive)
                   flock.l_type = F_WRLCK;
	        else
                   flock.l_type = F_RDLCK;
	        flock.l_whence = 0;
	        flock.l_start = a->lck.l_offset;
	        flock.l_len = a->lck.l_len;
		ENABLEPRIV(SEC_REMOTE);
	        if (nfs_fcntl(a->lck.fh.n_bytes, f_cmd, &flock) == -1) {
		   DISABLEPRIV(SEC_REMOTE);
		   if (errno == EACCES) {
		      if(a->lck.op & LOCK_NB) {
		 	 nlm_resp->lockstat = denied;
			 a->rel = 1;		/* to release a */
		      }
		      else { 				/* add_wait */
			 add_wait(a);
			 nlm_resp->lockstat = blocking;
		      }
                   }
		   else if (errno == ENOSPC) {
		      nlm_resp->lockstat = nolocks;
		      a->rel = 1;		/* to release a */
		   }
		   else { /* some other error */
		      if (errno != EINVAL)
		         logmsg(catgets(nlmsg_fd,NL_SETN,172, "%1$s: fcntl (local_lock) : errno = %2$d!\n"), progname, errno);
		      nlm_resp->lockstat = nolocks; /* can't return denied */
		      a->rel = 1;		/* to release a */
		   }
                }
		else { /* not blocked - period */
		   DISABLEPRIV(SEC_REMOTE);
		   if(add_reclock(fp, insrtp, a) == -1) {
		   	nlm_resp->lockstat= nolocks;
		   	a->rel = 1;
		   }
		   else 
			nlm_resp->lockstat = granted;
                }
	}
	return(nlm_resp);
}

/*
 * remote_lock and local_lock have great similarities!
 * choice == RPC; rpc calls to remote;
 * choice == MSG; msg passing calls to remote;
 */
remote_result *
remote_lock(a, choice)
reclock *a;
int choice;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,173, "enter remote_lock\n")));
	if(blocked(&fp, &insrtp, a) == 1) { /*blocked*/
		if(a->lck.op & LOCK_NB) {
			nlm_resp->lockstat = denied;
			a->rel = 1;
			return(nlm_resp);
		}
	}
	/* blocked == 0 or blocked == 1 and blocking */
	if( insrtp != NULL && same_proc(insrtp, a) && same_op(insrtp, a) &&
		     inside(a, insrtp)) {
		/* lock exists */
		a->rel = 1;		
		nlm_resp->lockstat = granted;
		return(nlm_resp);
	}
	else {		/* consult with svr lock manager */
		if(choice == MSG) { /* msg passing */
			if(nlm_call(NLM_LOCK_MSG, a, 0) == -1)
				a->rel = 1;		/* rpc error, discard */
			return(NULL);			/* no reply available */
		}
		else { /*rpc*/
			logmsg(catgets(nlmsg_fd,NL_SETN,174, "%s: rpc not supported\n"), progname);
			a->rel = 1;
			return(NULL);
		}

	}
}

remote_result *
local_unlock(a)
reclock *a;
{
struct flock flock;
	struct fs_rlck *fp;
	reclock *insrtp;
        msg_entry *msgp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,175, "enter local_unlock\n")));
	insrtp = NULL;
	if((fp = find_fe(a)) != NULL) {		  /*found*/
           if((msgp = search_msg3(fp->rlckp)) != NULL)
           { /* granted msg still in msg queue- deque it */
              if(debug)
	        logmsg((catgets(nlmsg_fd,NL_SETN,208, "local_unlock dequeue(%x)\n")), msgp->req);
                dequeue(msgp);
           }
              
           /* delete from kernel locks */
           flock.l_type = F_UNLCK;
	   flock.l_whence = 0;
	   flock.l_start = a->lck.l_offset;
	   flock.l_len = a->lck.l_len;
 	   ENABLEPRIV(SEC_REMOTE);
	   if (nfs_fcntl(a->lck.fh.n_bytes, F_SETLK, &flock) == -1) {
	      DISABLEPRIV(SEC_REMOTE);
	      logmsg(catgets(nlmsg_fd,NL_SETN,176, "%1$s: fcntl (local_unlock) : errno = %2$d!\n"), progname, errno);
	      if (errno == ENOLCK)
	         nlm_resp->lockstat = nolocks;
              else
	         nlm_resp->lockstat = denied;
	      a->rel = 1;
	      return(nlm_resp);
           }
	   DISABLEPRIV(SEC_REMOTE);
           /* delete from lockd locks */
	   delete_reclock(fp, &insrtp, a);
        }
	(void) wakeup(a);
	a->rel = 1;
	nlm_resp->lockstat = granted;
	return(nlm_resp);
}

remote_result *
remote_unlock(a, choice)
reclock *a;
int choice;
{
	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,177, "enter remote_unlock\n")));
	if( find_fe(a) != NULL) {  /*found*/
		if(choice == MSG) { 
			if(nlm_call(NLM_UNLOCK_MSG, a, 0) == -1)
				a->rel = 1;
			return(NULL);
			}

		/* rpc case */
		else {
			logmsg(catgets(nlmsg_fd,NL_SETN,178, "%s: rpc not supported\n"), progname);
			a->rel = 1;
			return(NULL);
		}
	}
	a->rel = 1;		/* not found */
	nlm_resp->lockstat = granted;
	return(nlm_resp);
}

	
remote_result *
local_test(a)
reclock *a;
{
struct flock flock;
	struct fs_rlck *fp;
	reclock *insrtp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,179, "enter local_test\n")));
	a->rel = 1;
	if(blocked(&fp, &insrtp, a) == 1){ /* blocked by another remote lock */
		nlm_resp->lockstat = denied;
		nlm_resp->lholder.svid = insrtp->lck.svid;
		nlm_resp->lholder.l_offset = insrtp->lck.l_offset;
		nlm_resp->lholder.l_len = insrtp->lck.l_len;
		nlm_resp->lholder.exclusive = insrtp->exclusive;
		obj_copy( &nlm_resp->lholder.oh, &insrtp->lck.oh);
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,180, "lock blocked by %1$d, (%2$d, %3$d)\n")), nlm_resp->lholder.svid, nlm_resp->lholder.l_offset, nlm_resp->lholder.l_len );
		return(nlm_resp);
	}
	else {
             /* now try local kernel to check for local locks */
                if (a->exclusive)
                   flock.l_type = F_WRLCK;
	        else
                   flock.l_type = F_RDLCK;
	        flock.l_whence = 0;
	        flock.l_start = a->lck.l_offset;
	        flock.l_len = a->lck.l_len;
		ENABLEPRIV(SEC_REMOTE);
	        if (nfs_fcntl(a->lck.fh.n_bytes, F_GETLK, &flock) == -1) {
		    DISABLEPRIV(SEC_REMOTE);
		     logmsg(catgets(nlmsg_fd,NL_SETN,181, "%1$s: fcntl (local_test) : errno = %2$d!\n"), progname, errno);
		     nlm_resp->lockstat =  granted; /* What else ???  We don't
						    send nolocks cause Sun will
						    loop forever */
	     	     return(nlm_resp);
                }
	        DISABLEPRIV(SEC_REMOTE);
		if (flock.l_type != F_UNLCK) { /* blocked by local lock */
		     nlm_resp->lockstat = denied;
		     nlm_resp->lholder.svid = flock.l_pid;
		     nlm_resp->lholder.l_offset = flock.l_start;
		     nlm_resp->lholder.l_len = flock.l_len;
		     if (flock.l_type == F_WRLCK)
		        nlm_resp->lholder.exclusive = TRUE;
                     else
		        nlm_resp->lholder.exclusive = FALSE;
	             nlm_resp->lholder.oh.n_len = host_len + sizeof(int);
	             if((nlm_resp->lholder.oh.n_bytes =
				xmalloc(nlm_resp->lholder.oh.n_len) ) == NULL)
		        return(nlm_resp);       /* -1 ??? */
	             strcpy(nlm_resp->lholder.oh.n_bytes, hostname);
	             memcpy(&nlm_resp->lholder.oh.n_bytes[host_len],
			    (char *) &flock.l_pid, sizeof(int));
		     if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,182, "lock blocked by %1$d, (%2$d, %3$d)\n")), nlm_resp->lholder.svid, nlm_resp->lholder.l_offset, nlm_resp->lholder.l_len );
		     return(nlm_resp);
		}
	        else { /* Not blocked - period! */
	     	     nlm_resp->lockstat = granted;
	     	     return(nlm_resp);
                }
	}
}

remote_result *
remote_test(a, choice)
reclock *a;
int choice;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,183, "enter remote_test\n")));
/* fix for SUN bug 
	if(blocked(&fp, &insrtp, a) == 0) { /* nonblocking */
	if(blocked(&fp, &insrtp, a) == 1) { /* blocked locally */
		a->rel = 1;
		nlm_resp->lockstat = denied;
		nlm_resp->lholder.svid = insrtp->lck.svid;
		nlm_resp->lholder.l_offset = insrtp->lck.l_offset;
		nlm_resp->lholder.l_len = insrtp->lck.l_len;
		nlm_resp->lholder.exclusive = insrtp->exclusive;
/*
		obj_copy( &nlm_resp->lholder.oh, &insrtp->lck.oh);
*/
		return(nlm_resp);
	}
	else {
		if(choice == MSG) {
			if(nlm_call(NLM_TEST_MSG, a, 0) == -1)
				a->rel = 1;
			return(NULL);
		}

		/* rpc case */
		else {
			logmsg(catgets(nlmsg_fd,NL_SETN,184, "%s: rpc not supported\n"), progname);
			a->rel = 1;
			return(NULL);
		}
	}
}

remote_result *
local_cancel(a)
reclock *a;
{
	reclock *nl;
	msg_entry *msgp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,185, "enter local_cancel(%x)\n")), a);
	a->rel = 1;
	if((nl = search_lock(a)) == NULL)
		nlm_resp->lockstat = denied;
	else {
		if(nl->w_flag == 0)
			nlm_resp->lockstat = granted;	
		else {
			remove_wait(nl);
			nl->rel = 1;
			if(!remote_clnt(nl)) {
			   if((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
		              if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,186, " local_cancel: dequeue(%x)\n")), msgp->req);
		              dequeue(msgp);
			   }
			   else {
			      release_le(nl);
			   }
			}
			else
			   release_le(nl);
			nlm_resp->lockstat = denied;
		}
	}
	return(nlm_resp);
}

remote_result *
remote_cancel(a, choice)
reclock *a;
int choice;
{
	reclock *nl;
	msg_entry *msgp;

	if(debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,187, "enter remote_cancel(%x)\n")), a);
	if((nl = search_lock(a)) == NULL) {
		if((msgp = retransmitted(a, KLM_LOCK)) == NULL) {
			/* msg was never received */
			a->rel = 1;
			nlm_resp->lockstat = denied;
			return(nlm_resp);
		}
		else { /* msg is being processed */
			if(debug)
			         logmsg((catgets(nlmsg_fd,NL_SETN,188, "remove msg(%x) due to remote cancel\n")), msgp->req);
			msgp->req->rel = 1;
			if(a->pre_le != NULL || a->pre_fe != NULL) {
				logmsg(catgets(nlmsg_fd,NL_SETN,189, "%1$s: cancel request pre_le=%2$x pre_fe = %3$x\n"), progname, a->pre_le, a->pre_fe);
			}
			else { /* take over the pre_fe and pre_le */
				a->pre_le = msgp->req->pre_le;
				a->pre_fe = msgp->req->pre_fe;
				msgp->req->pre_le = NULL;
				msgp->req->pre_fe = NULL;
			}
			dequeue(msgp);
		}
	}
	else {
		if(nl->w_flag == 0) { /* not in wait_q implies it is granted */

                        if((msgp = retransmitted(a, KLM_LOCK)) != NULL) 
                        {   /* granted message in mesg queue  - dequeue it */
                         if(debug)
                                 logmsg((catgets(nlmsg_fd,NL_SETN,188, "Remove msg(%x) due to remote cancel\n")), msgp->req);

                            dequeue(msgp);
                        }

			a->rel = 1;
			nlm_resp->lockstat = granted;	
			return(nlm_resp);
		}
	}				

	if(choice == MSG){
		if(nlm_call(NLM_CANCEL_MSG, a, 0) == -1)
			a->rel = 1;
		return(NULL);
	}
	else { /* rpc case */
		a->rel = 1;
		logmsg(catgets(nlmsg_fd,NL_SETN,190, "%s: rpc not supported\n"), progname);
		return(NULL);
	}

}


/*
 * local_granted reply to kernel if a is still in wait_queue;
 * return NULL if msg is discarded
 */
remote_result *
local_granted(a, choice)
reclock *a;
int choice;
{
	reclock *nl;
	struct fs_rlck *fp;
	reclock *insrtp;
	msg_entry *msgp;
	remote_result *resp;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,191, "enter local_granted\n")));
	a->rel = 1;
	nl = search_block_lock(a);
	if(nl != NULL ) { /* nl still in wait_queue, reply to kernel*/
		if( choice == MSG) { /* create reply */
			if(blocked(&fp, &insrtp, nl) != NULL) {
				logmsg(catgets(nlmsg_fd,NL_SETN,192, "%s: lock_granted msg discarded due to lock tbl inconsistent (unlock reply may be lost!)\n"), progname);
				return(NULL);
			}
			if((msgp = retransmitted(a, KLM_LOCK)) != NULL) {
			/*find */

					if((resp = (remote_result *)
						    xmalloc(res_len)) != NULL) {
						memset((char *) resp,0,res_len);
						resp->lockstat = granted;
						add_reply(msgp, resp);
					}
					else { /* malloc error! */
						nlm_resp->lockstat = denied;
						logmsg(catgets(nlmsg_fd,NL_SETN,193, "%1$s: msg(%2$x) is deleted from msg queue because reply cannot be created due to malloc prob\n"), progname, msgp->req);
						dequeue(msgp);
					}
			}
			else { /* msg no longer in msg_queue */
				nlm_resp->lockstat = denied;
				logmsg(catgets(nlmsg_fd,NL_SETN,194, "%s: msg no longer in msg_queue\n"), progname);
				}
		}
		else { /*rpc */
			logmsg(catgets(nlmsg_fd,NL_SETN,195, "%s: local granted: rpc not supported\n"), progname);
			return(NULL);
		}
		add_reclock(fp, insrtp, nl);
		remove_wait(nl);
		nlm_resp->lockstat = granted;	/* what else can it be */
	}
	else { 	/* nl no longer in wait_queue */
		nlm_resp->lockstat = denied;
		logmsg(catgets(nlmsg_fd,NL_SETN,196, "%s: msg no longer in wait_queue, this may be a retransmitted msg\n"), progname);
	}
	return(nlm_resp);
}


remote_result *
cont_lock(a, resp)
reclock *a;
remote_result *resp;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,197, "enter cont_lock\n")));
	switch(resp->lockstat) {
	case granted:
		if((blocked(&fp, &insrtp, a)) == 1) { 
		/* redundant, no way to avoid */
			logmsg(catgets(nlmsg_fd,NL_SETN,198, "%s: cont_lock: remote and local lock tbl inconsistent\n"), progname);
			release_res(resp);	/* discard this response */
			return(NULL);
		}
		else {
			if(add_reclock(fp, insrtp, a) == -1) {
					resp->lockstat = nolocks;
					a->rel = 1;
					return(resp);
				}
				else
					return(resp);
		}

		case denied:
			a->rel = 1;
			return(resp);
		case nolocks:
			a->rel = 1;
			return(resp);
		case blocking:
			add_wait(a);
			return(resp);
		case grace:
			release_res(resp);
			return(NULL);
		default:
			release_res(resp);
			logmsg(catgets(nlmsg_fd,NL_SETN,199, "%1$s: unknown lock return: %2$d\n"), progname, resp->lockstat);
			return(NULL);
			}
}
			

remote_result *
cont_unlock(a, resp)
reclock *a;
remote_result *resp;
{
	struct fs_rlck *fp;
	reclock *insrtp;
	msg_entry *msgp;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,200, "enter cont_unlock\n")));
	insrtp = NULL;
	a->rel = 1;
	switch(resp->lockstat) {
		case granted:

			if((fp = find_fe(a)) != NULL) 
                        {
                            if((msgp = search_msg2(fp->rlckp)) != NULL)
                            { /* granted msg still in msg queue- deque it */
                               if(debug)
                                     logmsg((catgets(nlmsg_fd,NL_SETN,208,                                         "cont_unlock dequeue(%x)\n")), msgp->req);
                               dequeue(msgp);
                            }
			    delete_reclock(fp, &insrtp, a);
                        }
			return(resp);

		case denied:		/* impossible */
		case nolocks:
		case blocking:		/* impossible */
			return(resp);
		case grace:
			a->rel = 0;		/* keep this lock*/
			release_res(resp);
			return(NULL);
		default:
			a->rel = 0;		/* discard the response */
			release_res(resp);
			logmsg(catgets(nlmsg_fd,NL_SETN,201, "%1$s: unkown rpc_unlock return: %2$d\n"), progname, resp->lockstat);
			return(NULL);
		}
}

remote_result *
cont_test(a, resp)
reclock *a;
remote_result *resp;
{
	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,202, "enter cont_test\n")));

	a->rel = 1;
	switch (resp->lockstat) {
	case grace:
		a->rel = 0;	/* keep this msg */
		release_res(resp);
		return(NULL);
	case denied:
		if(debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,203, "lock blocked by %1$d, (%2$d, %3$d)\n")), resp->lholder.svid, resp->lholder.l_offset, resp->lholder.l_len );
		return(resp);
	case granted:
	case nolocks:
	case blocking:
		return(resp);
	default:
		logmsg(catgets(nlmsg_fd,NL_SETN,204, "%1$s: cont_test: unknown return: %2$d\n"), progname, resp->lockstat);
		release_res(resp);
		return(NULL);
	}
}

remote_result *
cont_cancel(a, resp)
reclock *a;
remote_result *resp;
{
	reclock *nl;
	reclock *insrtp;
	struct fs_rlck *fp;
	msg_entry *msgp;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,205, "enter cont_cancel\n")));

	a->rel = 1;
	switch(resp->lockstat) {
	case granted:			
		if(search_lock(a) == NULL) { /* lock is not found */
			if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,206, "cont_cancel: msg must be removed from msg queue due to remote_cancel, now has to be put back\n")));

			if((blocked(&fp, &insrtp, a)) == 1) {
			/* redundant, no way to avoid */
				logmsg(catgets(nlmsg_fd,NL_SETN,207, "%s: cont_cancel: remote and local lock tbl inconsistent\n"), progname);
				release_res(resp);   /* discard this response */
				return(NULL);
			}
			else {
				add_reclock(fp, insrtp, a);
				a->rel = 0;
				return(resp);
			}
		}
                else
                {    /* see if it is in wait_q */
                  nl = search_block_lock(a);
                  if(nl != NULL ) { /* nl still in wait_queue, GRANT the lock */
                        if(blocked(&fp, &insrtp, nl) != NULL) {
				logmsg(catgets(nlmsg_fd,NL_SETN,207, "%s: cont_cancel: remote and local lock tbl inconsistent\n"), progname);
				release_res(resp);   /* discard this response */
                                return(NULL);
                        }

			if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,206, "cont_cancel: grant_reply - add to granted list \n")));

                        add_reclock(fp, insrtp, nl);
			a->rel = 0;
                        remove_wait(nl);

                        if((msgp = retransmitted(a, KLM_LOCK)) == NULL) 
                        { /* msg no longer in msg_queue */
                                logmsg(catgets(nlmsg_fd,NL_SETN,194, "%s: msg no longer in msg_queue\n"), progname);
                        }
                        else
                        {
                          dequeue(msgp); /*dequeue- cancel reply will suffice */
                        } 
                   }
                }
		return(resp);
	case blocking:		/* should not happen */
	case nolocks:
		return(resp);
	case grace:
		a->rel = 0;
		release_res(resp);
		return(NULL);
	case denied:
		if((nl = search_lock(a)) != NULL && nl->w_flag == 1) {
			(void) remove_wait(nl);
			nl->rel = 1;
			if((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
				if(debug)
				     logmsg((catgets(nlmsg_fd,NL_SETN,208, "cont_cancel: dequeue(%x)\n")), msgp->req);
				dequeue(msgp);
			}
			else {
				logmsg(catgets(nlmsg_fd,NL_SETN,209, "%s: cont_cancel cannot find blocked lock request in msg queue! \n"), progname);
				release_le(nl);
			}
			return(resp);
		}
		else if( nl!= NULL && nl->w_flag == 0) {
		/* remote and local lock tbl inconsistent */
			logmsg(catgets(nlmsg_fd,NL_SETN,210, "%s: remote and local lock tbl inconsistent\n"), progname);
			release_res(resp);		/* discard this msg*/
			return(NULL);
		}
		else {
			return(resp);
		}
	default:
		logmsg(catgets(nlmsg_fd,NL_SETN,211, "%1$s: unexpected remote_cancel %2$d\n"), progname, resp->lockstat);
		release_res(resp);
		return(NULL);
	}		/* end of switch */
}

remote_result *
cont_reclaim(a, resp)
reclock *a;
remote_result *resp;
{
	remote_result *local;
	struct msg_entry *msgp;
	struct fs_rlck *fp;
	reclock *insrtp, *nl;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,212, "enter cont_reclaim\n")));
	switch(resp->lockstat) {
	case granted:
		if(a->reclaim) {
			if(debug)
				logmsg((catgets(nlmsg_fd,NL_SETN,213, "reclaim request(%x) is granted\n")), a);
			if((msgp = retransmitted(a, NLM_LOCK_RECLAIM)) != NULL)
				dequeue(msgp);
			local = NULL;
		}
		else { /* reclaim block request is granted */
			if(debug)
		 	    logmsg((catgets(nlmsg_fd,NL_SETN,214, "reclaim block request (%x) is granted!!!\n")), a);
			/* use the same steps as in local_granted */
			/* instead of using cont_lock */
			if(a->w_flag == 1) {
				blocked(&fp, &insrtp, a);
				add_reclock(fp, insrtp, a);
				remove_wait(a);
			}
			else {
				logmsg(catgets(nlmsg_fd,NL_SETN,215, "%s: reclaim blocked request already granted, impossible\n"), progname);
			}
			local =  resp;
		}
		break;
	case denied:
	case nolocks:
		if(a->reclaim) {
			logmsg(catgets(nlmsg_fd,NL_SETN,216, "%1$s: reclaim lock(%2$x) fail!\n"), progname);
			kill_process(a);
			local = NULL;
		}
		else {
			if(debug)
				logmsg((catgets(nlmsg_fd,NL_SETN,217, "reclaim block lock fail due to(%x)\n")), resp->lockstat);
			if(a->w_flag == 1) {
				remove_wait(a);
				a->rel = 1;
			}
			else {
				logmsg(catgets(nlmsg_fd,NL_SETN,218, "%s: reclaim block lock has been granted with reclaim rejected as nolocks\n"), progname);
			}
			local = resp;
		}
		break;
	case blocking:
		if(a->reclaim) {
			logmsg(catgets(nlmsg_fd,NL_SETN,219, "%1$s: reclaim lock(%2$x) fail!\n"), progname);
			kill_process(a);
			/* should cancel remote_add_wait! */
			local = NULL;
		}
		else {
			if(a->w_flag == 0)
				logmsg(catgets(nlmsg_fd,NL_SETN,220, "%s: blocked reclaim request already granted locally, impossible\n"), progname);
			local = resp;
		}
		break;
	case grace:
		if(a->reclaim)
			logmsg(catgets(nlmsg_fd,NL_SETN,221, "%1$s: reclaim lock req(%2$x) is returned due to grace period, impossible\n"), progname, a);
		local = NULL;
		break;
	default:            
		logmsg(catgets(nlmsg_fd,NL_SETN,222, "%1$s: unknown cont_reclaim return: %2$d\n"), progname, resp->lockstat);
		local = NULL;
		break;
	}

	if(local == NULL)
		release_res(resp);
	return(local);
}
