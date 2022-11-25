/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/nsp.c,v $
 * $Revision: 1.11.83.3 $        $Author: root $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 16:44:44 $
 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


/*
 *This file contains the code that manages the network server processes (NSPS).
 *These are processes that remain completely in the kernel and perform
 *operations on behalf of remote sites.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../dux/cct.h"
#include "../dux/dm.h"
#include "../dux/nsp.h"
#include "../dux/unsp.h"
#include "../dux/selftest.h"
#include "../dux/protocol.h"
#include "../h/kern_sem.h"
#include "../h/ki_calls.h"

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

#if defined(__hp9000s700) || defined(__hp9000s800)
#define CRIT()			spl6()
#define UNCRIT(x)		splx(x)
#endif /* s700 || s800 */

extern struct dux_context dux_context;

/*
** Pointer to first and last message in regular queue
*/
dm_message nsp_head, nsp_tail;

/*
** Pointer to first and last message in limited queue
*/
dm_message limited_head, limited_tail;

/*
** The number of network server processes currently alive.
** Some of these may have timed out.
** This includes only general (not limited) NSPs.
*/
int num_nsps;

/*The number of non busy NSPs. If this is zero,
** there is no sense searching for an available NSP.
*/
volatile int free_nsps;

/*
** This number of NSPs that should be forked off.
** Since it is modified by a timeout routine, it is often read and
** always modified at spl6.
*/
volatile int nsps_to_invoke;

/*
** This is the total number of NSP's started by the user,
** and available long-term.  It includes the counted in num_nsps which
** have not timed out, and those counted in nsps_to_invoke.
*/
int nsps_started;


volatile int nsp_first_try=1;	/*first nsp to try*/
volatile int max_nsp=0;		/*maximum NSP slot in use*/
int forking_nsp = 0;		/*for synchronization when replacing NSPs*/
char lnsp_kill_sync;		/*for synchronization when killing limited NSP*/


/*	MP notes - globals in this file are protected as follows
**		by spl's - nsp_head, nsp_tail, limited_head,
**			limited_tail, free_nsps, nsps_to_invoke
**		by the FS empire semaphore - num_nsps, nsps_started,
**			max_nsp, forking_nsp
**		no race - nsp_first_try
**	I have also moved a few things around to get them inside
**	spl's for top-half synchronization even where there is
**	no contention from the bottom half.
*/

/*
** Wakeup the limited nsp to fork more nsps
*/
void
wakeup_nsp_forker()
{
	if (num_nsps < ngcsp)
		wakeup((caddr_t)&nsp[0]);
}

/*
** System call to invoke and kill NSPs. Called by /etc/csp.
**
*/
nsp_init()
{
	register struct a {
		int command;
		int how_many;
		int length;	/* for NSP_INFO_READ */
	} *uap = (struct a *) u.u_ap;
	register int delta_nsps;
	int s;

	/*
	** NSP_INFO_READ --
	**     read nsp statistics into a csp_info structure.
	**     Anyone can read these, so this code is before the
	**     suser() check.  We also don't need exclusive access,
	**     thus we don't do the P() and V() operations for this
	**     request.
	*/
	if (uap->command == NSP_INFO_READ) {
		struct csp_info *p = (struct csp_info *)uap->how_many;
		struct csp_info info;
		int len = uap->length;

		/*
		** First, gather all of the statistics into the
		** our own copy of an 'info' structure.
		*/
		bcopy(&csp_stats, &info, sizeof info);
		info.active_nsps = num_nsps;
		info.free_nsps = free_nsps;
		info.ngcsp = ngcsp;
		info.started = nsps_started;

		/*
		** Now copy the data to the user's space.  We only
		** copy as many bytes as he asks for (or as many bytes
		** as we have, whichever is less).  This allows for
		** the 'info' structure to be expanded in the future
		** while maintaining compatibility.  We return the
		** number of bytes that we copied.
		*/
		if (len > sizeof info)
			len = sizeof info;
		u.u_rval1 = len;
		u.u_error = copyout(&info, p, len);
		return;
	}

	if (!suser())		/*only the superuser can do this*/
		return;

	if (! (nsp[0].nsp_flags & NSP_VALID)) {
		u.u_error = EACCES;	/* not clustered, or limited NSP dead */
		return;
	}

	PSEMA(&filesys_sema);
	switch (uap->command) {
	case NSP_CMD_DELTA:
		delta_nsps = uap->how_many;
		break;

	case NSP_CMD_ABS:
		delta_nsps = uap->how_many - nsps_started;
		break;

	case NSP_INFO_RESET:
		init_csp_stats();	/* reset statistics */
		VSEMA(&filesys_sema);
		return;

	default:
		u.u_error = EINVAL;
		VSEMA(&filesys_sema);
		return;
	}

	if (delta_nsps < -nsps_started ||
	    (delta_nsps + nsps_started) > ngcsp) {
		/* trying to start or kill too many */
		u.u_error = EINVAL;
		VSEMA(&filesys_sema);
		return;
	}

	/*
	 * If we are trying to set the number of nsps to 0, make sure
	 * that there are no clients in the cluster.  We must have at
	 * least one gcsp running on all machines in a cluster to avoid
	 * deadlock situations.
	 */
	if ((delta_nsps + nsps_started) == 0) {
	    if (my_site_status & CCT_ROOT) {
		extern struct cct clustab[];
		site_t site;

		for (site = 0; site < MAXSITE; site++) {
		    if (site != my_site &&
			(clustab[site].status & CL_IS_MEMBER)) {
			/*
			 * We are the rootserver and there is at least
			 * one client, adjust delta_nsps so that we have
			 * one gcsp left.
			 */
			delta_nsps++;
			break;
		    }
		}
	    }
	    else {
		/*
		 * Clients always need at least 1 nsp, adjust the
		 * delta value by one so that we have one left.
		 */
		delta_nsps++;
	    }
	}

	/* set up return value */
	u.u_rval1 = nsps_started;
	nsps_started += delta_nsps;

	if (delta_nsps > 0) {
		s=CRIT();
		nsps_to_invoke += delta_nsps;
		UNCRIT(s);
		wakeup_nsp_forker();
	} else if (delta_nsps < 0) {
		kill_nsps(-delta_nsps);
	}

	VSEMA(&filesys_sema);
}


/*
** This function continues to invoke nsps as long as nsps_to_invoke is
** greater than zero.  When it goes to zero, or if it can't invoke an NSP,
** it returns.  The return value is the number of NSPs it tried to invoke
** but couldn't (note that nsps_to_invoke can be incremented by a timeout
** routine called under interrupt).
*/
fork_nsps()
{
	int error = 0;

	while (nsps_to_invoke > 0 && num_nsps < ngcsp && error == 0)
	{
		forking_nsp = 1;
		error = fork_nsp(0);
		forking_nsp = 0;
	}
	return (nsps_to_invoke);
}


/*
** This function invokes an NSP.  The parameter is 1 if and only if the NSP
** should be a limited one.  It returns 0 if it successfully invoked an NSP,
** and an errno value if an error.  It is called from only three places:
**	- by the cluster system call to create the limited LNSP on a server
**	- by main() to create the limited LNSP on a client
**	- by fork_nsps() above (the only case where limited == 0)
*/
fork_nsp(limited)
int limited;
{
	register struct proc *p;
	register struct nsp *nspp;
	register int nsp_num;
	int s, max_nsp_save;
	short nextactive, prevactive;
	int newpid;
	sv_sema_t ss;

	if (limited)
	    newpid = PID_LCSP;
	else {
	    /*
	     * Since getnewpid() can sleep, we must call it before
	     * we search the proc table (by calling allocproc()).
	     * Note that any errors after this will cause us to waste
	     * the new pid.
	     */
	    newpid=getnewpid();
	    if (newpid < 0)	/*no pids*/
	        return(EAGAIN);

	    if (u.u_nsp != &nsp[0])
	        panic ("fork nsp");
	}

	/*
	 * Now acquire a slot in the proc table.  If the proc table is
	 * full, return and try again later
	 */
	if ((p = allocproc(S_DONTCARE)) == NULL)
		return(EAGAIN);

	/*
	 * Now we can acquire resources.  They may have to be backed out
	 * if newproc() fails, however.
	 */
	if (!limited)
	{
		/*
		 * We need to mark an nsp structure as ours and decrement
		 * nsps_to_invoke without being pre-empted, since kill_nsps()
		 * depends on these being consistent.
		 */

		/*first find an NSP*/
		for (nsp_num = 1; nsp_num <= max_nsp; nsp_num++)
		{
			if (!(nsp[nsp_num].nsp_flags & NSP_VALID))
				break;
		}

		/* At this time we have found a free NSP.  Note that since
		*  we were called by fork_nsps, which ensured that there
		*  is at least one free nsp slot, if there were no empty
		*  slots below max_nsp, the value of nsp_num is equal to
	 	*  (max_nsp+1), which indicates an available slot.
		*/
		max_nsp_save = max_nsp;
		if (nsp_num > max_nsp)
		{
			max_nsp = nsp_num;
		}
		num_nsps++;

		/* Set up the first of the various fields in the NSP array
		 * Some can't be set up until the child exists, but these
		 * must be set at the same time as nsps_to_invoke is
		 * decremented
		 */
		nspp = &nsp[nsp_num];
		nspp->nsp_site = 0;	/*not working on behalf of a site*/
		nspp->nsp_pid = 0;	/*not working on behalf of a process*/
		nspp->nsp_rid = 0;	/*not processing a request*/
		nspp->nsp_flags = NSP_BUSY|NSP_VALID;

		/* we succeeded.  Decrement the count*/
		s=CRIT();
		nsps_to_invoke--;
		UNCRIT(s);
	}

	/*
	** Before we fork, make sure that our context is the "standard"
	** context.  We do not want newproc() duplicating a non-standard
	** context on any NSP processes, since these are not allocated
	** in the standard way.
	*/
	u.u_cntxp = (char **)&dux_context;

	/*
	** Now fork.
	** Set up child swap maps for procdup(), but don't allocate any
	** swap space.
	*/
	switch (newproc(FORK_DAEMON, p))
	{
	case FORKRTN_ERROR:
		/*
		 * Unable to fork nsp due to lack of memory, back out
		 * changes and return, try again later.
		 */
		if (!limited) {
			/*
			 * back out what we have already acquired
			 */
			max_nsp = max_nsp_save;
			num_nsps--;
			nspp->nsp_flags = 0;
			s = CRIT();
			nsps_to_invoke++;
			UNCRIT(s);
		}
		/*
		 * remove off of active chain.
		 */
		SPINLOCK(activeproc_lock);
		nextactive = p->p_fandx;
		prevactive = p->p_pandx;
		(proc + nextactive)->p_pandx = prevactive;
		(proc + prevactive)->p_fandx = nextactive;
		SPINUNLOCK(activeproc_lock);
		freeproc(p);
		u.u_error = ENOMEM;
		return(ENOMEM);
		break;

	case FORKRTN_PARENT:
		/* Parent process continues here */
		return (0);
		break;

	case FORKRTN_CHILD:
		/*child process*/
		p = u.u_procp;
		proc_hash(p, 0, newpid, PGID_NOT_SET, SID_NOT_SET);

		/*
		 * This is a no-op for the s300.  However for the
		 * s700 and s800, we want to ensure that we are
		 * never preempted.  Thus, we force our spl level
		 * to SPLNOPREEMPT (SPL0).
		 */
		spl0();		/* always run non-preemptable */

		if (limited)
		{
			/*
			 * Give a limited csp its name for pstat.
			 */
			pstat_cmd(u.u_procp, "lcsp", 1, "lcsp");
			u.u_syscall = KI_LCSP;

			PXSEMA(&filesys_sema, &ss);
			limited_nsp();
			/*we should never return*/
		}
		else	/*normal NSP*/
		{
			void catch_signal();

			/* Make gcsp's the child of the lcsp */
			abandonchild(u.u_procp);
			makechild(nsp[0].nsp_proc, u.u_procp);

			/*
			 * Give a csp its name for pstat.
			 */
			pstat_cmd(u.u_procp, "gcsp", 1, "gcsp");
			u.u_syscall = KI_GCSP;

			/* Set up so we are interruptable by SIGUSR1 only.
			** Our parent (the limited NSP) has already set
			** all signals to be ignored.
			** We use a real routine and not SIG_DFL because
			** issig() ignores SIG_DFL for system processes.
			** Currently that is defined as the parent pid is 0,
			** and our's isn't, but we are marked as a system
			** process so we will make double sure that we
			** SIGUSR1 is not ignored.  Notice that we will
			** never actually get SIGUSR1, we will just get woken
			** up out of sleep() when it is recieved. cwb
			*/
			p->p_sigignore = ~(1<<(SIGUSR1-1));
			/*rest ignored by parent*/
			u.u_signal[SIGUSR1-1] = catch_signal;

			/* set up the remaining fields in the NSP array */
			nspp->nsp_proc = u.u_procp;
			nspp->nsp_cred = u.u_cred;
			u.u_nsp = nspp;

			PXSEMA(&filesys_sema, &ss);
			/*Call the NSP service routine*/
			nsp_serve(nspp,nsp_num);

			/*If we return, the nsp has died*/
			num_nsps--;
			s = splimp();
			if (!(nspp->nsp_flags & NSP_BUSY))
				if (--free_nsps < csp_stats.min_gen_free)
					csp_stats.min_gen_free = free_nsps;
			splx(s);
			nspp->nsp_flags = 0;
			if (nsp_num == max_nsp)
			{
				while (--nsp_num > 0)
				{
					if (nsp[nsp_num].nsp_flags & NSP_VALID)
						break;
				}
				max_nsp = nsp_num;
			}
			/*if we return we do an exit.  If there were other
			 *nsps to invoke, but they weren't invoked due to
			 *lack of space, wake the parent so he can invoke
			 *them.  A potential optimization is to just invoke
			 * another NSP on this process - but there are
			 * synchronization issues.
			 */
			if (nsps_to_invoke)
				wakeup_nsp_forker();
			VXSEMA(&filesys_sema, &ss);

			/*
			 * Make sure that our context to the "standard"
			 * one.  We do not want exit() thinking that we
			 * have malloced a non-standard context, which
			 * would cause it to be free-ed.  If our cntxp
			 * is pointing at something non-standard, then
			 * it is pointing at part of a DUX message,
			 * which was already freed.
			 */
			u.u_cntxp = (char **)&dux_context;
			exit(0);	/*if we return, exit*/
			/*NOTREACHED*/
		}
	}
	/*NOTREACHED*/
}



/*
** Be a limited NSP.  This NSP is special in that it is created at cluster
** time.  It will only perform those actions which are necessary to be a good
** citizen, as indicated in the funcentry table.  It should not perform any
** action which may:
**	1) sleep at an interuptable priority
**	2) take a long time
**		or
**	3) be unnecessary for continued operation of the cluster.
**
** NOTE:  These are guidelines only.  Failure to adhere to them will not
** necessarily cause problems.  In particular, it is not necessary to do
** a detailed check of the code to guarantee condition 1, since interrupts
** will be ignored.  However, if it is sleeping at interruptable priority,
** interrupts should not be ignored, and we are doing something wrong.
**
** There should only be one limited NSP.
**
** NOTE: On a root server, the limited NSP is created at cluster time
** and will therefore NOT be process id 3. However, on the client the
** limited NSP will always be pid=3.
*/

limited_nsp()
{
	register int s;
	register dm_message qp;
	register int couldnt_fork;
	register int start_ticks;

	/*the limited NSP always uses slot 0 in the NSP table*/
	nsp[0].nsp_site = 0;	/*not working on behalf of a site*/
	nsp[0].nsp_pid = 0;	/*not working on behalf of a process*/
	nsp[0].nsp_rid = 0;	/*not processing a request*/
	nsp[0].nsp_flags = NSP_BUSY|NSP_VALID|NSP_LIMITED;
	nsp[0].nsp_proc = u.u_procp;
	nsp[0].nsp_cred = u.u_cred;
	u.u_nsp = &nsp[0];

	/* save my u area.  It is sufficiently representative
	** to satisfy all NSPS
	*/
	backup_ulookup();

	/* Ignore all signals.
	** This includes SIGCLD, so we clean up zombies.
	*/
	u.u_procp->p_sigignore = ~0;
	for (s = 0; s < NSIG; s++)
		u.u_signal[s] = SIG_IGN;
	/* start up selftest code - now that all components are running */
	schedule_selftest();

	/* since we've ignored signals, we should never see one */
	if (setjmp(&u.u_qsave))
		panic ("limited nsp signaled");

	/*loop forever waiting for things to do*/
	for(;;)
	{
		couldnt_fork = 0;
		s = spl6(); /* must be >= splimp as well */
		while ((limited_head == NULL) &&
		       !(nsp[0].nsp_flags & NSP_TIMED_OUT))
		{
			nsp[0].nsp_flags &= ~NSP_BUSY;
			if (nsps_to_invoke > couldnt_fork)
			{
				splx(s);
				/* u_site must be correct so any PIDs    */
				/* allocated are given to the right site */
				u.u_site = my_site;
				couldnt_fork = fork_nsps();
				(void) spl6();
			}
			else
			{
				/* sleep at a signalable priority */
				/* to clean up zombies on SIGCLD  */
				sleep(&nsp[0],PZERO+1);
				couldnt_fork = 0;
			}
		}
		if (nsp[0].nsp_flags & NSP_TIMED_OUT)
		{
			splx(s);
			nsp[0].nsp_flags = 0;
			u.u_site = my_site;	/* restore so exit works ok */
			wakeup((caddr_t)&lnsp_kill_sync);
			exit(0);
		}
		/*there is something on the queue; take it off*/
		qp = limited_head;
		limited_head = (dm_message)(((struct mbuf *)qp)->m_act);
		--csp_stats.limitedq_curlen;
		++csp_stats.requests[CSPSTAT_LIMITED];
		start_ticks = lbolt;
		nsp[0].nsp_flags |= NSP_BUSY;
		splx(s);
		/*process the request*/
		nsp_exec(qp,&nsp[0],1);
		if (lbolt - start_ticks > csp_stats.max_lim_time)
			csp_stats.max_lim_time = lbolt - start_ticks;
	}
	/*NOTREACHED*/
}


/*
 *In order to prevent all NSPs from being tied up with slow operations,
 *we set up a timeout when a general NSP begins its operation.  If
 *the timeout takes place, we signal the forker to create a new NSP, and we
 *set a flag in this NSP so that it will die after the operation is complete.
 *This function processes such a timeout.
 */
nsptimeout(nspp)
register struct nsp *nspp;	/*pointer to nsp structure*/
{
	int s;

	if ((nspp->nsp_flags & NSP_TIMED_OUT) == 0)
	{
		nspp->nsp_flags |= NSP_TIMED_OUT;	/*mark for suicide*/
		s = CRIT();		/* MP */
		nsps_to_invoke++;			/*we need a new NSP*/
		(void) UNCRIT(s);
		wakeup_nsp_forker();
	}
	++csp_stats.timeouts[nspp->nsp_timeout_type];
}


/*
 *service NSPs.
 *The basic algorithm is as follows:
 *	1) Sleep as long as there is nothing on the queue
 *
 *	2) Once there is something on the queue, process it.
 *
 *	3) Goto 1
 */
nsp_serve(nspp,nsp_num)
register struct nsp *nspp;	/*pointer to nsp structure*/
register int nsp_num;		/*this nsp number*/
{
	register int s;
	register dm_message qp;

	while (1)
	{
		/*
		 * Check the queue.  We actually check both the general
		 * queue and the limited queue.	 If the general queue
		 * empties but there are requests pending on the limited
		 * queue, we might as well start servicing the limited
		 * requests instead of going to sleep.	This helps to
		 * maintain the order of requests, and generally improves
		 * system throughput and robustness by reducing the
		 * likelyhood of deadlock situations.
		 */
		s = splimp();
		while (nsp_head == NULL && limited_head == NULL &&
		       !(nspp->nsp_flags & NSP_TIMED_OUT))
		{
			/* both queues are empty */
			if (nspp->nsp_flags & NSP_BUSY)
			/*If I was marked as busy*/
			{
				/*mark myself free*/
				nspp->nsp_flags &= ~NSP_BUSY;
				free_nsps++;
			}
			nsp_first_try = nsp_num;/*have them wake me up first*/
			sleep(nspp,PZERO);	/*ignore signals*/
		}

		/*
		 * If we need to commit suicide (because of a timeout
		 * or because the number of nsps in the system has been
		 * decreased), do it.
		 */
		if (nspp->nsp_flags & NSP_TIMED_OUT)
		{
			/*
			 * Only exit if we have already been replaced or
			 * if we are currently being replaced.  We must be
			 * at level 6 to lockout timeouts while modifying
			 * nsps_to_invoke; don't overwrite s (set above
			 * by s=splimp()).
			 */
			(void)CRIT();
			if (nsps_to_invoke <= forking_nsp)
			{
				/*
				 * Before we exit, restore u.u_site.  That will
				 * permit exit to correctly perform such actions
				 * as closing open file descriptors
				 */
				splx(s);
				u.u_site = my_site;
				return;
			}
			--nsps_to_invoke;
			nspp->nsp_flags &= ~NSP_TIMED_OUT;
			splx(s);
			continue;
		}

		/*
		 * There is something on one of the queues, service it.
		 */
		if (nsp_head != NULL)	/* general queue */
		{
			qp = nsp_head;
			nsp_head = (dm_message)(((struct mbuf *)qp)->m_act);
			((struct mbuf *)qp)->m_act=(struct mbuf *)nspp->nsp_pid;
			--csp_stats.generalq_curlen;
			splx(s);
			/* process the request */
			nsp_exec(qp,nspp,0);
		}
		else			/* limited queue */
		{
			qp = limited_head;
			limited_head = (dm_message)(((struct mbuf *)qp)->m_act);
			--csp_stats.limitedq_curlen;
			((struct mbuf *)qp)->m_act=(struct mbuf *)nspp->nsp_pid;
			splx(s);
			/* process the request */
			nsp_exec(qp,nspp,0);
		}
	}
}

/*
 *Execute the function requested by reqp.  The current nsp is indicated
 *by nspp.  Limited is 1 if the nsp is a limited one.
 */
nsp_exec(reqp,nspp,limited)
register dm_message reqp;
register struct nsp *nspp;
int limited;
{
	register struct dm_header *hp;

	/*record that we have invoked an NSP for selftest purposes*/
	TEST_PASSED(ST_NSP);
	u.u_error = 0;			/*initial value*/
	u.u_eosys = EOSYS_NORMAL;	/*initial value*/
	u.u_acflag = 0;			/*initial value*/
	SPINLOCK(sched_lock);
	u.u_procp->p_flag |= SPRIV;	/*recompute privgrps if need to use*/
	SPINUNLOCK(sched_lock);
	hp = DM_HEADER(reqp);
	nspp->nsp_site = u.u_site = hp->dm_srcsite;
	nspp->nsp_pid = hp->dm_pid;
	nspp->nsp_rid = hp->dm_ph.p_rid;
	u.u_request = reqp;
	bordend();

	if (!limited)
	{
		/*
		** Schedule a timeout for the request. The length of
		** the timeout is a function of the number of currently
		** available free nsps.
		*/
		if( free_nsps > 10 ) {
			nspp->nsp_timeout_type = CSPSTAT_LONG;
			++csp_stats.requests[CSPSTAT_LONG];
			timeout (nsptimeout, nspp, NSPTIMEOUT_LONG);
		} else if( free_nsps < 5 ) {
			nspp->nsp_timeout_type = CSPSTAT_SHRT;
			++csp_stats.requests[CSPSTAT_SHRT];
			timeout (nsptimeout, nspp, NSPTIMEOUT_SHRT);
		} else {
			nspp->nsp_timeout_type = CSPSTAT_MED;
			++csp_stats.requests[CSPSTAT_MED];
			timeout (nsptimeout, nspp, NSPTIMEOUT_MED);
		}

		/*set up apropriate signal handling*/
		SPINLOCK(sched_lock);
		u.u_procp->p_flag &= ~SOUSIG;
		if (hp->dm_tflags & DM_SOUSIG)
			u.u_procp->p_flag |= SOUSIG;
		SPINUNLOCK(sched_lock);

		/*clear any pending signals*/
		u.u_procp->p_sig = 0;

		/*if I need to do so, send my self a signal*/
		if (hp->dm_flags & DM_SIGPENDING)
			psignal(u.u_procp, SIGUSR1);
	}
	/*prepare to catch signals if necessary*/
	if (!limited && setjmp(&u.u_qsave))
	{
		PSEMA(&filesys_sema);
		/* If we got here we caught a signal with the default action
		 * Unless this NSP was operating on behalf of a local operation,
		 * send back a reply stating that an interrupt occurred.
		 */
		if (nspp->nsp_site == my_site || nspp->nsp_site == 0)
			printf("unexpected longjmp in local CSP request");
			/* and apparently re-try - should panic ???? */
		else
			dm_reply(NULL, DM_LONGJMP, EINTR, NULL, NULL, NULL);
	}
	else
	{
		/*process the request*/
		(*hp->dm_func)(reqp);
	}
	/*clear the timeout*/
	if (!limited)
		untimeout(nsptimeout, nspp);

	/* Return the NSP to an idle state, specifying that
	 * it is:  */
	nspp->nsp_site = 0;	/*not working on behalf of a site, and*/
	nspp->nsp_pid = 0;	/*not working on behalf of a process,*/
	nspp->nsp_rid = 0;	/*and not processing a request*/
}

/*
 *If there exists a free NSP, wake it up (but only wake up one).
 *Also, mark it busy.
 *If there are no free NSPs, that's OK too, since as soon as one frees
 *up it will do what needs to be done.
 */
schedule_nsp()
{
	static int nsp_count;
	register int nsp_start;
	register struct nsp *nspp;
	int s;

	if (free_nsps)	/*if any are free*/
	{
		/*circulate through all the existing NSPs to see if any
		 *are free.  Wake up the first one found.  We start where we
		 *left off searching the last time under the assumption that
		 *we are more likely to find a free one if it was started
		 *less recently.  Also, whenever an NSP completes processing,
		 *it resets the first try to its address.  The reason for
		 *this is there has not yet been a context switch since the
		 *NSP completed, it will be cheaper to wake up that NSP than
		 *to wake up any other.
		 *
		 *Theoretically, it is impossible to actually complete the
		 *loop and terminate at the while statement.  This is because
		 *we shouldn't execute the loop unless there are any free
		 *NSPs, and we should terminate the loop as soon as we find
		 *one.  However, I would rather be safe than in an infinite
		 *loop.
		 *
		 *Note that if there are no free NSPs, there is no problem.
		 *As soon as one of the NSPs finishes what it is doing
		 *it will check the queue and process the next request.
		 */
		nsp_count = nsp_start = nsp_first_try;
		s = splimp();
		do
		{
			nspp = &nsp[nsp_count];
			if (nspp->nsp_flags & NSP_VALID &&
				! (nspp->nsp_flags & NSP_BUSY))
			{
				/*mark it busy*/
				nspp->nsp_flags |= NSP_BUSY;
				if (--free_nsps < csp_stats.min_gen_free)
					csp_stats.min_gen_free = free_nsps;
				splx(s);
				/*
				** We found a free NSP.  Wake it up.
				** Now go to nsp_serve()
				*/
				wakeup((caddr_t)nspp);
				return;	/*we found it.  Thats enough.*/
			}
			if (++nsp_count > max_nsp)
				nsp_count = 1;
		} while (nsp_count != nsp_start);
		splx(s);
	}
	else if (nsps_to_invoke > 0)
		wakeup_nsp_forker();/* get the limited NSP to try to fork 'em */
}

/*
 *Invoke an NSP.
 *
 *The algorithm is as follows:
 *
 *	1) verify that there exists at least one NSP.  If there is none,
 *	and the limited flag is not set, return an error.  If the limited
 *	flag is set, the request will go on a special queue.
 *
 *	2) enqueue the request.
 *
 *	3) Schedule an NSP if any are available by calling schedule_nsp.
 *
 *
 *This routine returns 0, unless no NSPs exist, in which case it sets and
 *returns u.u_error.
 */
int
invoke_nsp(message,func,limited)
dm_message message;
int (*func)();
int limited;
{
	int s;

	(DM_HEADER(message))->dm_func = func;
	if (limited)
		(DM_HEADER(message))->dm_flags |= DM_LIMITED_OK;
	((struct mbuf *)message)->m_act = NULL;
	if (limited && (free_nsps <= 0))
	{
		/*if there are no free nsps, and we permit use of the
		 *limited queue, put it on that queue*/
		s = splimp();
		if (limited_head == NULL)
		{
			limited_head = limited_tail = message;
		}
		else
		{
			(((struct mbuf *)
				limited_tail)->m_act) = (struct mbuf *) message;
			limited_tail = message;
		}
		if (++csp_stats.limitedq_curlen > csp_stats.limitedq_maxlen)
			csp_stats.limitedq_maxlen = csp_stats.limitedq_curlen;
		splx(s);
		wakeup((caddr_t)&nsp[0]);
		return (0);
	}

	if ((nsps_started <= 0) && !limited)
		return (EACCES);	/*No NSPs--an error*/

	/* We are scheduling a regular NSP.  First put the request on the
	 * queue
	 */
	s = splimp();
	if (nsp_head == NULL)
	{
		nsp_head = nsp_tail = message;
	}
	else
	{
		(((struct mbuf *)nsp_tail)->m_act) = (struct mbuf *) message;
		nsp_tail = message;
	}
	/* MP - stay at spl a bit longer to serialize top half
	** access to stats.
	*/
	if (++csp_stats.generalq_curlen > csp_stats.generalq_maxlen)
		csp_stats.generalq_maxlen = csp_stats.generalq_curlen;
	splx(s);
	/*now wake up a process if any are available.*/
	schedule_nsp();
	return (0);
}

/*
 *Send a signal to an NSP.  This function is called under interrupt whenever
 *a process which has a pending remote request gets a signal.  It takes the
 *following steps:
 *	1)  If an NSP is working on the specified request, send it a signal.
 *	2)  Otherwise, if the request is in the queue, set DM_SIGPENDING.
 *	3)  Otherwise, if the request is being processed by a UNSP, send
 *		it a signal
 *	4)  Otherwise, send back an error response
 *NOTE:  This function is called under interrupt.
 */
signalnsp(message)
dm_message *message;
{
	register long rid = *(DM_CONVERT(message, long));
	register struct nsp *nspp;
	register struct unsp *unspp;
	int found=0;

	/*first search the active nsps*/
	for (nspp = nsp; nspp < nspNCSP; nspp++)
	{
		if ((nspp->nsp_flags & (NSP_VALID|NSP_BUSY)) ==
			(NSP_VALID|NSP_BUSY) &&
			nspp->nsp_rid == rid)
		{
			/* We found the NSP--Signal it */
			found = 1;
			psignal(nspp->nsp_proc,SIGUSR1);
			break;	/*from for*/
		}
	}
	/*traverse the queue*/
	if (!found)
	{
		register dm_message qp;
		register struct dm_header *hp;
		int s;

		s = CRIT();	/* MP */
		qp = nsp_head;
		while (qp != NULL)
		{
			hp = DM_HEADER(qp);
			if (hp->dm_ph.p_rid == rid)
			{
				/* We found the message on the NSP queue.
				 * Mark the message so that the NSP signals
				 * itself when it actually executes
				 */
				hp->dm_flags |= DM_SIGPENDING;
				found = 1;
				break;	/*from while*/
			}
			qp = (dm_message)(((struct mbuf *)qp)->m_act);
		}
		(void) UNCRIT(s);
	}
	/*check for a unsp*/

	/* !!!! This code should check for unsp structures that are !!! */
	/* !!!! flagged UNSP_IN_USE but not UNSP_HAS_PROCESS, and   !!! */
	/* !!!! somehow co-ordinate treatment of them with unsp.c.  !!! */
	/* !!!! This is not currently a problem, since the only     !!! */
	/* !!!! current unsp (/etc/read_cct) can never be signaled. !!! */

	if (!found)
	{
		for (unspp = unsps; unspp < unsps+MAX_UNSP; unspp++)
		{
			if ((unspp->un_flags&UNSP_HAS_PROCESS) &&
				unspp->un_request &&
				(DM_HEADER(unspp->un_request))->dm_ph.p_rid ==
					rid)
			{
				/* We found the UNSP.  Signal it.*/
				found = 1;
				psignal(unspp->un_proc,SIGUSR1);
				break;	/*from for*/
			}
		}

	}
	/*send back the reply.  It will contain a 1 if we recognized the
	 *ID, a 0 otherwise.
	 */
	DM_SHRINK(message,sizeof(int));
	*(DM_CONVERT(message, int)) = found;
	dm_reply(message, 0, 0, NULL, NULL, NULL);
}

/*
 *Scan the nsp table to determine if there are nsp still running for the
 *failed site.  If yes, send a signal to kill those processes.
 *Also, remove all pending requests from the NSP queue.
 *Before returning, check for unsps also by calling no_unsps.  If we
 *found an active NSP or UNSP, return 0.  Otherwise, return 1.
 */

no_nsp (crashed_site)
site_t crashed_site;
{
	register no_process;
	register struct nsp *nspp;

	no_process = 1;

	/* First check the active NSPs */
	for (nspp = nsp; nspp < nspNCSP; nspp++)
		if ( (nspp->nsp_flags & NSP_BUSY) &&
		     (nspp->nsp_site == crashed_site) ) {
			/*
			** Don't have to worry about cleanup nsp or
			** serve_clusterreq nsp since they are both
			** run on behalf of site 0
			*/
			psignal (nspp->nsp_proc, SIGUSR1);
			no_process = 0;
		}

	flush_nsp_queue (&nsp_head, &nsp_tail, crashed_site,
			 &csp_stats.generalq_curlen);
	flush_nsp_queue (&limited_head, &limited_tail, crashed_site,
			 &csp_stats.limitedq_curlen);
	return (no_unsp(crashed_site) && no_process);
}

/*
 * Flush all entries for a crashed site from one of the two NSP queues
 */
flush_nsp_queue (head_ptr, tail_ptr, crashed_site, qlength_ptr)
dm_message *head_ptr, *tail_ptr;
register site_t crashed_site;
register int *qlength_ptr;
{
	register dm_message *p_cur;
	register dm_message cur;
	register dm_message last;
	int s;
	extern struct serving_entry serving_array[];

	/* Now check the queue of pending requests.  If there are any
	 * requests from the specified site, remove them from the queue.
	 */
	last = NULL;
	s = splimp();
	p_cur = head_ptr;
	while ((cur = *p_cur) != NULL)
	{
		if (DM_SOURCE(cur) == crashed_site)
		{
			/* Change state of serving entry so it gets cleaned
			 * up by protocol code.
			 */
      			serving_array[DM_HEADER(cur)->dm_mid].state
				= RECVING_REQUEST;
			*p_cur=((dm_message)(((struct mbuf *)cur)->m_act));
			--(*qlength_ptr);
		}
		else
		{
			last = cur;
			p_cur =
			    (dm_message *) (&(((struct mbuf *)last)->m_act));
		}
	}
	*tail_ptr = last;	/* update tail, whether or not it changed */
	splx(s);
}


/* Kill off NSPs.  If this leaves no NSPs in the system, clean up the
 * queues as well.  Note that this routine reads and modifies nsps_to_invoke
 * and the nsp[].nsp_flags, which are also manipulated in nsp_timeout().
 * It would be simplest to do all the work (the first for loop) at spl6 to
 * lock out timeouts, but that could lock out interrupts for too long
 * Using splimp and triggering nsp_timeout() to that level would be
 * a minor improvement.  Instead the loop keeps toggling the processor
 * priority, and there is code following the loop to allow for the
 * effects of timeouts during the loop.
 */
kill_nsps (kill_count)
register int kill_count;
{
	register struct nsp *nspp;
	register dm_message mp, nmp;
	register struct dm_header *hp;
	register error;
	register s;

	s=CRIT();
	if (nsps_to_invoke >= kill_count)
	{
		nsps_to_invoke -= kill_count;
		kill_count = 0;
	}
	else
	{
		kill_count -= nsps_to_invoke;
		nsps_to_invoke = 0;
	}
	UNCRIT(s);

	/* Walk through the NSPs marking the last <kill_count> for suicide and
	 * waking them up.
	 */
	for (nspp = nspNCSP-1; kill_count > 0 && nspp > nsp; nspp--)
	{

		s = CRIT();	/* while checking/setting NSP_TIMED_OUT */
		if ((nspp->nsp_flags & (NSP_VALID|NSP_TIMED_OUT)) == NSP_VALID)
		{
			nspp->nsp_flags |= NSP_TIMED_OUT;
			UNCRIT(s);
			if (!(nspp->nsp_flags & NSP_BUSY))
				wakeup((caddr_t)nspp);
			--kill_count;
		}
		else
			UNCRIT(s);
	}
	if (kill_count > 0)
	{
		/* It's possible that more NSPs timed out during our scan,
		 * incrementing nsps_to_invoke.  If not, we should have
		 * found enough to kill.  Note that nsps_to_invoke is never
		 * decreased under interrupt.
	 	 */
		s = CRIT();
		if (nsps_to_invoke < kill_count)
			panic ("kill_nsps - not enough to kill");
		nsps_to_invoke -= kill_count;
		UNCRIT(s);
	}
	if (nsps_started > 0)
		return;

	/*We have killed off all the NSPs.
	 *Take all requests on the queue and try to reexecute them.
	 *If they can be processed by the limited NSP, it will do
	 *it, otherwise, we can send back errors.
	 */
	s = CRIT();
	mp = nsp_head;
	nsp_head = NULL;
	csp_stats.generalq_curlen = 0;
	UNCRIT(s);
	while (mp != NULL)
	{
		nmp = (dm_message)(((struct mbuf *)mp)->m_act);
		hp = DM_HEADER(mp);
		if ((error = invoke_nsp(mp,hp->dm_func,
			hp->dm_flags&DM_LIMITED_OK)) != 0)
		{
			/* Set up u_request so we can reply to the
			 * correct message.
			 */
			u.u_request = mp;
			dm_quick_reply(error);
		}
		mp = nmp;
	}
}

/* Kill the limited NSPs.  For now this routine is only called by the
 * reboot system call on the rootserver, so we don't worry too much
 * about cleaning up things the LNSP might have to do (eg. the limited
 * queue, the regular NSPs, or nsps_to_invoke).  Generally this routine
 * should be called after the other nodes have been shut down, so the
 * LNSP should be idle.  This call waits for the LNSP to actually
 * die before returning, with a hard-wired timeout of 5 seconds.
 * The return value is 0 for success, 1 for timeout.
 */

kill_limited_nsp()
{
	int timeout_kill_lnsp();

	lnsp_kill_sync = 0;
	/* wait for it to really die */
	while ((nsp[0].nsp_flags & NSP_VALID) && (lnsp_kill_sync == 0))
	{
		nsp[0].nsp_flags |= NSP_TIMED_OUT;
		wakeup((caddr_t)&nsp[0]);
		timeout(timeout_kill_lnsp, (caddr_t)0, 5*HZ);
		sleep (&lnsp_kill_sync, PZERO);
		untimeout(timeout_kill_lnsp, (caddr_t)0);
	}
	return(lnsp_kill_sync);
}

timeout_kill_lnsp()
{
	lnsp_kill_sync = 1;
	wakeup ((caddr_t)&lnsp_kill_sync);
}

init_csp_stats()
{
	int s;
	s = splimp();
	/* re-initialize statistics - except current Q lengths */
	bzero (&csp_stats.CSPSTAT_FIRST_CLEAR, CSPSTAT_CLEAR_SIZE);
	csp_stats.min_gen_free = nsps_started;
	splx(s);
}

/* Dummy signal handler for SIGUSR1 for gcsp's */
void catch_signal()
{
#ifdef OSDEBUG
	/* This should never happen, but let's find out if it does. */
	printf("Catch Signal Called");
#endif
}
