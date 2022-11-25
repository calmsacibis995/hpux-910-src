/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/klm_lckmgr.c,v $
 * $Revision: 1.5.83.5 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/25 17:51:56 $
 */

/*
 * Copyright (c) 1988 by Hewlett Packard
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Kernel<->Network Lock-Manager Interface
 *
 * File- and Record-locking requests are forwarded (via RPC) to a
 * Network Lock-Manager running on the local machine.  The protocol
 * for these transactions is defined in /usr/src/protocols/klm_prot.x
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/stat.h"

/* files included by <rpc/rpc.h> */
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"

#include "../net/if.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../nfs/lockmgr.h"
#include "../nfs/klm_prot.h"
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */

#define NS_LOG(a,b,c,d)
#define NS_LOG_INFO5(a,b,c,d,e,f,g,h,i,j)
#define NS_LOG_INFO(a,b,c,d,e,f,g)
#define NS_LOG_STR(a,b,c,d,e)

#ifdef hpux
extern char *rpcstatnames[];
#endif hpux

struct sockaddr_in lm_sa;	/* talk to portmapper & lock-manager */

int talk_to_lockmgr();

extern int wakeup();

int klm_debug = 0;

/* Define static parameters for run-time tuning */
int backoff_timeout = 10;	/* time to wait on klm_denied_nolocks */
int first_retry = 0;		/* first attempt if klm port# known */
int first_timeout = 1;
int normal_retry = 1;		/* attempts after new port# obtained */
int normal_timeout = 5;
int working_retry = 0;		/* attempts after klm_working */
int working_timeout = 1;

/*
 * Structure for keeping various counts and statistics about LM calls.
 */

struct {
	u_int klm_calls;
	u_int klm_cancels;
	u_int klm_pmap_calls;
	u_int klm_bad_pmap_calls;
	u_int klm_lm_not_registered;
	u_int klm_lm_not_up;
	u_int klm_clnt_call;
	u_int klm_timeouts;	/* Got RPC_TIMEDOUT from clnt_call */
	u_int klm_intr;		/* Got interrupted */
	u_int klm_bad_rpc;	/* Got RPC error from clnt_call */
	u_int klm_return_vals[4]; /* working, granted, denied, denied_nolocks*/
	u_int klm_req_getlk[4]; /* F_RDLCK = 1, F_WRLCK =2, F_UNLCK  = 3*/
	u_int klm_req_setlk[4]; /* F_RDLCK = 1, F_WRLCK =2, F_UNLCK  = 3*/
	u_int klm_req_setlkw[4]; /* F_RDLCK = 1, F_WRLCK =2, F_UNLCK  = 3*/
} klmstats;

/*
 * klm_lockctl - process a lock/unlock/test-lock request
 *
 * Calls (via RPC) the local lock manager to register the request.
 * Lock requests are cancelled if interrupted by signals.
 */
klm_lockctl(lh, ld, cmd, cred)
	register lockhandle_t *lh;
	register struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	register int	error;
	klm_lockargs	args;
	klm_testrply	reply;
	u_long		xdrproc;
	xdrproc_t	xdrargs;
	xdrproc_t	xdrreply;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/* initialize sockaddr_in used to talk to local processes */
	if (lm_sa.sin_port == 0) {
		lm_sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		lm_sa.sin_family = AF_INET;
	}
	/*
	 * Count requests and type of requests
	 */
	klmstats.klm_calls++;

	args.block = FALSE;
	args.exclusive = FALSE;
	args.lock.fh.n_bytes = (char *)&lh->lh_id;
	args.lock.fh.n_len = sizeof (lh->lh_id);
	args.lock.server_name = lh->lh_servername;
	args.lock.pid = (int)u.u_procp->p_pid;
	args.lock.l_offset = ld->l_start;
	args.lock.l_len = ld->l_len;
	xdrproc = KLM_LOCK;
	xdrargs = (xdrproc_t)xdr_klm_lockargs;
	xdrreply = (xdrproc_t)xdr_klm_stat;

	/* now modify the lock argument structure for specific cases */
	switch (ld->l_type) {
	case F_WRLCK:
		args.exclusive = TRUE;
		break;
	case F_UNLCK:
		xdrproc = KLM_UNLOCK;
		xdrargs = (xdrproc_t)xdr_klm_unlockargs;
		break;
	}

	switch (cmd) {
	case F_SETLK:
		klmstats.klm_req_setlk[ld->l_type]++;
		klmstats.klm_req_setlk[0]++;
		break;
	case F_SETLKW:
		klmstats.klm_req_setlkw[ld->l_type]++;
		klmstats.klm_req_setlkw[0]++;
		args.block = TRUE;
		break;
	case F_GETLK:
		klmstats.klm_req_getlk[0]++;
		xdrproc = KLM_TEST;
		xdrargs = (xdrproc_t)xdr_klm_testargs;
		xdrreply = (xdrproc_t)xdr_klm_testrply;
		break;
	}


	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

requestloop:
	/* send the request out to the local lock-manager and wait for reply */
	error = talk_to_lockmgr(xdrproc,xdrargs, &args, xdrreply, &reply, cred);
	if (error == ENOLCK) {
		goto ereturn;	/* no way the request could have gotten out */
	}

	/*
	 * The only other possible return values are:
	 *   klm_granted  |  klm_denied  | klm_denied_nolocks |  EINTR
	 */
	switch (xdrproc) {
	case KLM_LOCK:
		switch (error) {
		case klm_granted:
			error = 0;		/* got the requested lock */
			goto ereturn;
		case klm_denied:
			if (args.block) {
				NS_LOG_STR( LE_NFS_LM_LOCKDENIED, NS_LC_ERROR,
				    NS_LS_NFS, 0, args.lock.server_name);
				/*
				 * Sun went into an infinite loop here, but
				 * that doesn't make sense because we'll
				 * probably get the same error again.  
				 * Therefore we blow outta here....
				 * SUN CODE: goto requestloop;
				 */
				error = EINVAL;
			}
			else
			    error = EACCES;		/* EAGAIN?? */
			goto ereturn;
		case klm_denied_nolocks:
			error = ENOLCK;		/* no resources available?! */
			goto ereturn;
		case EINTR:
			if (args.block)
				goto cancel;	/* cancel blocking locks */
			else
				goto requestloop;	/* loop forever */
		}

	case KLM_UNLOCK:
		switch (error) {
		case klm_granted:
			error = 0;
			goto ereturn;
		case klm_denied:
			NS_LOG_STR( LE_NFS_LM_UNLOCKDENIED, NS_LC_ERROR,
				    NS_LS_NFS, 0, args.lock.server_name);
			error = EINVAL;
			goto ereturn;
		case klm_denied_nolocks:
			/*
			 * Sun looped forever here with "goto nolocks_wait".
			 * But there ENOLCK is a legitimate return value
			 * for the unlock case where you are unlocking a
			 * segment in the middle of a lock, and the server
			 * has no free lock structures to use.
			 */
			error = ENOLCK;
			goto ereturn;
		case EINTR:
			goto requestloop;	/* loop forever */
		}

	case KLM_TEST:
		switch (error) {
		case klm_granted:
			ld->l_type = F_UNLCK;	/* mark lock available */
			klmstats.klm_req_getlk[ld->l_type]++;
			error = 0;
			goto ereturn;
		case klm_denied:
			ld->l_type = (reply.klm_testrply.holder.exclusive) ?
			    F_WRLCK : F_RDLCK;
			klmstats.klm_req_getlk[ld->l_type]++;
			ld->l_start = reply.klm_testrply.holder.l_offset;
			ld->l_len = reply.klm_testrply.holder.l_len;
			ld->l_pid = reply.klm_testrply.holder.svid;
			error = 0;
			goto ereturn;
		case klm_denied_nolocks:
			/*
			 * Sun looped forever here, with a "goto nolocks_wait".
			 * But if you get this error, it is most likely a
			 * condition that will not go away, so looping forever
			 * is a bad thing to do!
			 */
			error = ENOLCK;
			goto ereturn;
		case EINTR:
			/*
			 * Again, Sun looped forever here.  But there is
			 * no reason to!
			 */
			error = EINTR;
			goto ereturn;
		}
	}

/*NOTREACHED*/
#ifdef notdef
/*
 * KLM_TEST and KLM_UNLOCK used to come here when they got denied_nolocks, but
 * looping forever is bogus, since you are obscureing possible return values.
 * therefore this is no longer needed.
 */
nolocks_wait:
	timeout(wakeup, (caddr_t)&lm_sa, (backoff_timeout * hz));
	(void) sleep((caddr_t)&lm_sa, PZERO+1|PCATCH);
	untimeout(wakeup, (caddr_t)&lm_sa);
	goto requestloop;	/* go try again. */

#endif notdef

cancel:
	/*
	 * If we get here, a signal interrupted a rqst that must be cancelled.
	 * Change the procedure number to KLM_CANCEL and reissue the exact same
	 * request.  Use the results to decide what return value to give.
	 */
	xdrproc = KLM_CANCEL;
	klmstats.klm_cancels++;
	error = talk_to_lockmgr(xdrproc,xdrargs, &args, xdrreply, &reply, cred);
	switch (error) {
	case klm_granted:
		error = 0;		/* lock granted */
		goto ereturn;
	case klm_denied:
		/* may want to take a longjmp here */
		error = EINTR;
		goto ereturn;
	case EINTR:
		goto cancel;		/* ignore signals til cancel succeeds */

	case klm_denied_nolocks:
		error = ENOLCK;		/* no resources available?! */
		goto ereturn;
	case ENOLCK:
		NS_LOG_STR( LE_NFS_LM_CANCEL_NOLOCKS, NS_LC_ERROR,
			    NS_LS_NFS, 0, args.lock.server_name);
		goto ereturn;
	}
/*NOTREACHED*/
ereturn:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	return(error);
}


/*
 * Send the given request to the local lock-manager.
 * If timeout or error, go back to the portmapper to check the port number.
 * This routine loops forever until one of the following occurs:
 *	1) A legitimate (not 'klm_working') reply is returned (returns 'stat').
 *
 *	2) A signal occurs (returns EINTR).  In this case, at least one try
 *	   has been made to do the RPC; this protects against jamming the
 *	   CPU if a KLM_CANCEL request has yet to go out.
 *
 *	3) A drastic error occurs (e.g., the local lock-manager has never
 *	   been activated OR cannot create a client-handle) (returns ENOLCK).
 */

int
talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, reply, cred)
	u_long xdrproc;
	xdrproc_t xdrargs;
	klm_lockargs *args;
	xdrproc_t xdrreply;
	klm_testrply *reply;
	struct ucred *cred;
{
	register CLIENT *client;
	struct timeval tmo;
	register int error;
	bool_t lm_is_working = TRUE;

	/* set up a client handle to talk to the local lock manager */
	client = clntkudp_create(&lm_sa, (u_long)KLM_PROG, (u_long)KLM_VERS,
	    first_retry, cred);
	if (client == (CLIENT *) NULL) {
		return(ENOLCK);
	}
	tmo.tv_sec = first_timeout;
	tmo.tv_usec = 0;

	/*
	 * If cached port number, go right to CLNT_CALL().
	 * This works because timeouts go back to the portmapper to
	 * refresh the port number.
	 */
	if (lm_sa.sin_port != 0) {
		goto retryloop;		/* skip first portmapper query */
	}

	for (;;) {
remaploop:
		/* go get the port number from the portmapper...
		 * if return 1, signal was received before portmapper answered;
		 * if return -1, the lock-manager is not registered
		 * else, got a port number
		 */
		klmstats.klm_pmap_calls++;
		switch (getport_loop(&lm_sa,
		    (u_long)KLM_PROG, (u_long)KLM_VERS, (u_long)KLM_PROTO)) {
		case 2:
			klmstats.klm_intr++;
			error = EINTR;		/* signal interrupted things */
			goto out;
		case 1:
			klmstats.klm_bad_pmap_calls++;
			error = ENOLCK;		/* Can't talk to portmapper */
			lm_sa.sin_port = 0;
			goto out;

		case -1:
			/* LM isn't registered with portmap */
			klmstats.klm_lm_not_registered++;
			NS_LOG_INFO( LE_NFS_LM_NOT_REGISTERED, NS_LC_ERROR,
			    NS_LS_NFS, 0, 0, 0, 0);
			error = ENOLCK;
			goto out;
		}
		/*
		 * Verify that the port gotten from the portmapper is actually
		 * in use.  If it's not, then the lock manager probably died
		 * and didn't unregister with portmap (can you say core-dump!)
		 */
		if ( !check_port_bound(lm_sa.sin_port) ) {
			klmstats.klm_lm_not_up++;
			NS_LOG( LE_NFS_LM_NOT_UP, NS_LC_ERROR, NS_LS_NFS, 0);
			error = ENOLCK;
			lm_sa.sin_port = 0;
			goto out;
		}

		/* reset the lock-manager client handle */
		lm_is_working = FALSE;
		clntkudp_freecred(client);
		(void) clntkudp_init(client, &lm_sa, normal_retry, cred);
		tmo.tv_sec = normal_timeout;

retryloop:
		/* retry the request until completion, timeout, or error */
		for (;;) {
			/*
			 * Enable interrupts at the RPC level.  Ideally, this
			 * would happen only if the mount was interruptable.
			 * Unfortunately, this could also be called for the
			 * local file system, which doesn't know anything
			 * about this, so just always turn it on.
			 */
			clntkudp_setint(client);
			error = (int) CLNT_CALL(client, xdrproc, xdrargs,
			    (caddr_t)args, xdrreply, (caddr_t)reply, tmo);
			klmstats.klm_clnt_call++;

			if (klm_debug)
				printf(
				    "klm: pid:%d cmd:%d [%d,%d]  stat:%d/%d\n",
				    args->lock.pid,
				    (int) xdrproc,
				    args->lock.l_offset,
				    args->lock.l_len,
				    error, (int) reply->stat);

			switch (error) {
			case RPC_SUCCESS:
				error = (int) reply->stat;
				klmstats.klm_return_vals[error]++;
				if (error == (int) klm_working) {
				    /*
				     * clntkudp_init will cause the XID to be
				     * reset, so don't do it if we can assume
				     * the LM is up and working.
				     */
				    if ( ! lm_is_working ) {
					/* lock-mgr is up...can wait longer */
					clntkudp_freecred(client);
					(void) clntkudp_init(client, &lm_sa,
					    working_retry, cred);
					tmo.tv_sec = working_timeout;
					lm_is_working = TRUE;
				    }
				    continue;	/* retry */
				}
				goto out;	/* got a legitimate answer */

			case RPC_TIMEDOUT:
				klmstats.klm_timeouts++;
				goto remaploop;	/* ask for port# again */

			case RPC_INTR:
				klmstats.klm_intr++;
				error = EINTR;
				goto out;

			default:
				NS_LOG_STR( LE_NFS_LM_RPCERROR, NS_LC_ERROR,
				    NS_LS_NFS, 0, rpcstatnames[(int) error]);
				klmstats.klm_bad_rpc++;

				/* on RPC error, wait a bit and try again */
				timeout(wakeup, (caddr_t)&lm_sa,
				    (normal_timeout * hz));
				error = sleep((caddr_t)&lm_sa, PZERO+1|PCATCH);
				untimeout(wakeup, (caddr_t)&lm_sa);
				if (error) {
				    error = EINTR;
				    goto out;
				}
				goto remaploop;	/* ask for port# again */
	    
			} /*switch*/

		} /*for*/	/* loop until timeout, error, or completion */
	} /*for*/		/* loop until signal or completion */

out:
	AUTH_DESTROY(client->cl_auth);	/* drop the authenticator */
	/*
	 * Free up the credential structure grabbed by
	 * clntkudp_init() --- dds ---
	 */
	clntkudp_freecred(client);
	CLNT_DESTROY(client);		/* drop the client handle */
	return(error);
}
