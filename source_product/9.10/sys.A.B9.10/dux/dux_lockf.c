/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_lockf.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:41:22 $
 */


#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/fcntl.h"
#include "../h/unistd.h"
#include "../h/mount.h"
#include "../h/conf.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/nsp.h"
#include "../dux/dux_dev.h"

struct lockf_req
  { dev_t  		lr_idev;
    ino_t  		lr_ino;
    int    		lr_flag;
    off_t  		lr_LB;
    off_t  		lr_UB;
    enum lockf_type 	lr_function;
    struct flock    	lr_flock;
    int             	lr_fpflags;
  };

struct unlockf_req
  { dev_t  		lr_idev;
    ino_t  		lr_ino;
  };

struct procstat_req
  { short  psr_pid;	/* Process ID or process ID NSP is serving	*/
    site_t psr_psite;	/* 0 if process is local, nonzero if NSP	*/
  };


struct locklist *dd_procstat(), *dd_nspstat();

/*  Handle a remote lockf request					*/

dux_lockf(reqp)
  dm_message reqp;
  { register struct lockf_req *lrp = DM_CONVERT(reqp, struct lockf_req);
    register struct inode *ip = ifind(localdev(lrp->lr_idev), lrp->lr_ino);
    register int error;
    
    if(ip == NULL) {
        error = EBADF;			/*  Shouldn't EVER happen	*/
	goto bad;
    }

    if(lrp->lr_flag == F_ULOCK) {
        delete_lock(ip, lrp->lr_LB, lrp->lr_UB, lrp->lr_function);
	error = u.u_error;
    }
    else {
        if(locked(lrp->lr_flag,ip,lrp->lr_LB,lrp->lr_UB,lrp->lr_function,
	    &lrp->lr_flock,lrp->lr_flock.l_type,lrp->lr_fpflags) == NULL) {
		if ((lrp->lr_flag == F_GETLK) || (lrp->lr_flag == F_TEST))
			lrp->lr_flock.l_type = F_UNLCK;
		else 
	   		insert_lock(ip, lrp->lr_LB, lrp->lr_UB, 
				    lrp->lr_function, lrp->lr_flock.l_type);
	}
	error = u.u_error;
    }

  bad:
    if (lrp->lr_function == L_LOCKF)
	dm_quick_reply(error);
    else
      	dm_reply(reqp, 0, error, NULL, NULL, NULL);
  }


/*  Handle a remote unlockf request					*/

dux_unlockf(reqp)
  dm_message reqp;
  { register struct unlockf_req *lrp = DM_CONVERT(reqp, struct unlockf_req);
    register struct inode *ip = ifind(localdev(lrp->lr_idev), lrp->lr_ino);
    register int error;
    
    if(ip == NULL)
      { error = EBADF;			/*  Shouldn't EVER happen	*/
	goto bad;
      }

    unlock(ip);
    error = u.u_error;

  bad:
      dm_quick_reply(error);
  }

/*  Routines for dealing with distributed deadlock checking		*/

/*  Find out the status of an NSP WRT locks.  Differs from dd_procstat in
 *  that 1) the NSP is looked up 2) NSPs cannot be forwarded, so the return
 *  value is either a lock or NULL 3) if the return value is NULL & u.u_error
 *  is set to ESRCH, the NSP was not found.
 */

struct locklist *
dd_nspstat(s_pid, s_site)
   short  s_pid;		/*  Process ID of process using NSP	*/
   site_t s_site;		/*  Site where using process lives	*/
{ 
   register struct nsp *nspp;
   register struct inode *ip;
   register struct locklist *cl, *nl;

   for(nspp = nsp; nspp < nspNCSP; nspp++) {
      
      /* do a match against the processes with NSPs */
      if((nspp->nsp_flags & NSP_VALID) &&
          nspp->nsp_pid == s_pid && nspp->nsp_site == s_site) {

	     /* bug fix for a race condition  -
	        run through the inode list & be sure the lock is still there
	      */
             for (ip=inode; ip<inodeNINODE; ip++) {

   		cl = (struct locklist *)&ip->i_locklist;
                while((nl = cl->ll_link) != NULL) {
                   if (nl == (struct locklist *)nspp->nsp_proc->p_dlchan)
                            return(nl);
                   else cl = nl;
                } 	/* end of the locklist search for that inode */

             }	/* end of the inode table search */

      }	 /* found a valid nsp */

   }	/* nsp for loop */

   /* No match found, return NULL (no deadlock) */
   return(NULL);
}


/*  Find out the status of a remote process WRT locks.  If the return value
 *  is zero, there is no deadlock.  Otherwise, either the forwarding address
 *  will be set or the pid/site pair wil be modified.
 */

int
Rdd_procstat(pidp, sitep, faddr)
  short  *pidp;		/*  Process ID of remote process		*/
  site_t *sitep;	/*  Site where the process is running		*/
  site_t *faddr;	/*  Forwarding address of process		*/
  { register dm_message msg = dm_alloc(sizeof(struct procstat_req), WAIT);
    register struct procstat_req *psp = DM_CONVERT(msg, struct procstat_req);
    register int rval = 0;

    psp->psr_pid   = *pidp;
    psp->psr_psite = 0;		/*  To indicate a regular process	*/
  
    msg = dm_send(msg, DM_RELEASE_REQUEST|DM_REPEATABLE|DM_SLEEP, DM_PROCLOCKF, 
		  *sitep, sizeof(struct procstat_req), NULL, NULL,
		  NULL, NULL, NULL, NULL, NULL);

    psp = DM_CONVERT(msg, struct procstat_req);

    if(psp->psr_pid != 0)		/*  Continue search w/ new proc	*/
      { *pidp  = psp->psr_pid;
	*sitep = psp->psr_psite;
	*faddr = 0;
	rval = 1;
      }
    else
      if(psp->psr_psite != 0)		/*  Continue search w/ NSP	*/
	{ *faddr = psp->psr_psite;
	  rval = 1;
	}

    dm_release(msg, 0);
    return(rval);
  }

int
Rdd_nspstat(pidp, sitep, faddr)
  short  *pidp;
  site_t *sitep;
  site_t faddr;
  { register dm_message msg = dm_alloc(sizeof(struct procstat_req), WAIT);
    register struct procstat_req *psp = DM_CONVERT(msg, struct procstat_req);
    register int rval = 0;

    psp->psr_pid   = *pidp;
    psp->psr_psite = *sitep;	/*  To indicate an NSP			*/
  
    msg = dm_send(msg, DM_RELEASE_REQUEST|DM_REPEATABLE|DM_SLEEP, DM_PROCLOCKF, 
		  faddr, sizeof(struct procstat_req), NULL, NULL, NULL,
		  NULL, NULL, NULL, NULL);

    psp = DM_CONVERT(msg, struct procstat_req);

    if(psp->psr_pid != 0)
      { *pidp  = psp->psr_pid;
	*sitep = psp->psr_psite;
	rval = 1;
      }
	
    dm_release(msg, 0);
    return(rval);
  }

dux_procstat(reqp)
  dm_message reqp;
  { register struct procstat_req *psp = DM_CONVERT(reqp, struct procstat_req);
    register struct locklist *lp;
    site_t faddr;

    if(psp->psr_psite == 0)		/*  Local process		*/
      { struct proc *pfind();
	register struct proc *procp = pfind(psp->psr_pid);

	if(procp == NULL)		/*  Shouldn't happen, but ends	*/
	  { lp    = NULL;		/*  the deadlock search		*/
	    faddr = 0;
	  }
	else
	  lp = dd_procstat(procp, &faddr);
      }
    else
      { lp    = dd_nspstat(psp->psr_pid, psp->psr_psite);
	faddr = 0;
      }

    if(lp == NULL)
      { psp->psr_pid = 0;
	psp->psr_psite = faddr;
      }
    else
      if(lp->ll_flags & L_REMOTE)
	{ psp->psr_pid   = lp->ll_pid;
	  psp->psr_psite = lp->ll_psite;
	}
      else
	{ psp->psr_pid   = lp->ll_proc->p_pid;
	  psp->psr_psite = my_site;
	}

    dm_reply(reqp, 0, 0, NULL, NULL, NULL);
  }



#ifdef SYSCALLTRACE
extern int  lockfdbg;	/* defined in ufs_lockf.c */
#endif SYSCALLTRACE

/*
 *  dux_lockctl() -- provide the locking functions that would be called
 *  from fcntl().  This functions assumes that parameter error checking
 *  has already occured, and that both LB and UB are valid.  The
 *  parameter *flock will be modified if the request if F_GETLK, and we
 *  assume that the calling routine (fcntl()) will take responsibility
 *  for copying the changed structure back to user space.  Other than
 *  the previous sentence, this code is essentially what was in fcntl()
 *  for the diskless case, modified for the fact that some things are params.
 */

/*ARGSUSED*/
int
dux_lockctl( vp, flock, cmd, cred, fp, LB, UB )
    struct vnode *vp;
    struct flock *flock;
    int cmd;
    struct ucred *cred;
    struct file *fp;
    off_t LB, UB;
{

	struct inode *ip = VTOI(vp);
	register struct lockf_req *lreq, *lresp;
	register dm_message msg;
	register site_t dest = devsite(ip->i_dev);
	register int ret_code;
	int req_flag;

	/*  Do the request locally first. */

	/*  If the operation is F_GETLK, struct flock 
	 *  may be updated as a side effect in locked() 
	 *  if a lock was found
	 */
	if ( cmd == F_GETLK ) {
	       ret_code = locked(F_GETLK, ip, LB, UB, L_FCNTL,
			flock, flock->l_type,fp->f_flag);

		/* if a lock was found on client, no need to check the server */
		if (ret_code)
			return (u.u_error);
	}

	/* will be set if delete_lock fails */
	u.u_prelock = NULL;
	if(flock->l_type == F_UNLCK) 
		delete_lock(ip, LB, UB, L_FCNTL);
	else 
	   if ( cmd != F_GETLK ) {
		if (locked(cmd, ip, LB, UB, L_FCNTL, NULL,
			flock->l_type, fp->f_flag)) 
		    return(u.u_error);
		else
		    insert_lock(ip, LB, UB, L_FCNTL, flock->l_type);
	   }

	if ((u.u_error == 0) && (u.u_prelock == NULL)) {
		/* send the request to the server */
	}
	else
		return(u.u_error);

	msg  = dm_alloc(sizeof(struct lockf_req), WAIT);
	lreq = DM_CONVERT(msg, struct lockf_req);

	lreq->lr_idev    = ip->i_dev;
	lreq->lr_ino     = ip->i_number;
	if (flock->l_type == F_UNLCK) 
		/* map to lockf(2) call */
		lreq->lr_flag    = F_ULOCK; 
	else
		lreq->lr_flag    = cmd;

	lreq->lr_LB      = LB;
	lreq->lr_UB      = UB;
	lreq->lr_function = L_FCNTL;
	bcopy(flock, &lreq->lr_flock, sizeof(struct flock)); 
	lreq->lr_fpflags = 0;

	if(flock->l_type != F_UNLCK && cmd != F_GETLK)
		u.u_procp->p_faddr = dest;

	req_flag = lreq->lr_flag;

	msg = dm_send(msg, DM_RELEASE_REQUEST|DM_SLEEP|DM_INTERRUPTABLE, 
		DM_LOCKF, dest, sizeof(struct lockf_req), 
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	/*  Reset the forwarding address - doesn't matter
	 *  if it was set	
	 */
	u.u_procp->p_faddr = 0;

	u.u_error = DM_RETURN_CODE(msg);
	lresp = DM_CONVERT(msg, struct lockf_req);

	/* if the request was insert_lock and there was
	 * an error, delete the successful lock on 
	 * the client side
	 */
	if ((u.u_error != 0) && 
	    ((req_flag!=F_ULOCK)&&(req_flag!=F_GETLK))) {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			("fcntl (remoteip): error on the server\n");
#endif SYSCALLTRACE
		delete_lock(ip, LB, UB, L_FCNTL);
	}

	/* the command states that the pid of the locker 
	   will be returned in the flock structure.  This
	   information is correct but is the server pid,
	   and not useful for a client */
	/*
	 * This used to be a direct copyout to user space.  Instead assign
	 * it to the flock structure, and let the calling routine handle it.
         */
	if(req_flag == F_GETLK) {
		*flock = lresp->lr_flock;
	}
	dm_release(msg, 0);
	
	return (u.u_error);	/* u.u_error could be set in locked.. */
}

/*
 *  dux_clockf() -- provide the locking functions that would be called
 *  from lockf() on a DUX CLIENT.  This functions assumes that parameter error
 *  checking has already occured, and that both LB and UB are valid.
 *  This code is essentially what was in lockf() for the diskless case,
 *  modified for the fact that some things are params.
 */

/*ARGSUSED*/
int
dux_clockf( vp, flag, size, cred, fp, LB, UB )
    struct vnode *vp;
    int flag;
    off_t size;
    struct ucred *cred;
    struct file *fp;
    off_t LB, UB;
{
	struct inode *ip = VTOI(vp);
	register struct lockf_req *lreq;
	register dm_message msg;
	register site_t dest = devsite(ip->i_dev);

	/*  Do the request locally first  */

	u.u_prelock = NULL;  /* will be set if delete_lock fails */
	if(flag == F_ULOCK) 
		delete_lock(ip, LB, UB, L_LOCKF);
	
	else {
		if ( locked( flag, ip, LB, UB, L_LOCKF, NULL, F_WRLCK,
			fp->f_flag)) 
		       return (u.u_error);	/* error found */
		if ( flag != F_TEST ) 
		    insert_lock(ip, LB, UB, L_LOCKF, F_WRLCK);
	}

	if ((u.u_error == 0) && (u.u_prelock == NULL)) {
		/* send the request to the server */
	}
	else
		return (u.u_error);

	msg  = dm_alloc(sizeof(struct lockf_req), WAIT);
	lreq = DM_CONVERT(msg, struct lockf_req);

	lreq->lr_idev    = ip->i_dev;
	lreq->lr_ino     = ip->i_number;
	lreq->lr_flag    = flag;
	lreq->lr_LB      = LB;
	lreq->lr_UB      = UB;
	lreq->lr_function = L_LOCKF;
	bzero(&lreq->lr_flock, sizeof(struct flock));
	lreq->lr_flock.l_type = F_WRLCK;
	lreq->lr_fpflags = 0;

	if(flag != F_ULOCK && flag != F_TEST)
		u.u_procp->p_faddr = dest;

	msg = dm_send(msg, DM_RELEASE_REQUEST|DM_SLEEP|DM_INTERRUPTABLE,
		      DM_LOCKF,
		      dest, DM_EMPTY, NULL, NULL, NULL, NULL,
		      NULL, NULL, NULL);

	/*  Reset the forwarding address - doesn't matter if it was set	*/
	u.u_procp->p_faddr = 0;

	u.u_error = DM_RETURN_CODE(msg);
	dm_release(msg, 0);

	/* if there was an error and the request was to insert_lock, 
	 * delete the successful lock on the client side
	 */
	if ( (u.u_error != 0) && ((flag == F_LOCK) || (flag == F_TLOCK)) ) { 
#ifdef SYSCALLTRACE
		if (lockfdbg)
			("lockf (remoteip): delete_lock, server error\n");
#endif SYSCALLTRACE
		delete_lock(ip, LB, UB, L_LOCKF);
	}

	return (u.u_error);
}
