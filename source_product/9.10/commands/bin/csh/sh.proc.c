/* @(#) $Revision: 72.3 $ */   
/************************************************************************
 * This file holds definitions relevent to the wait system call.
 * Some of the options here are available only through the ``wait3''
 * entry point; the old entry point with one argument has more fixed
 * semantics, never returning status of unstopped children, hanging until
 * a process terminates if any are outstanding, and never returns
 * detailed information about process resource utilization (<vtimes.h>).
 ************************************************************************/

#ifdef DEBUG_CHILD
#include <sys/pstat.h>
#endif

#if defined (DEBUG_CHILD) || defined (TRACE_DEBUG) || defined (ENV_DEBUG) || defined (NITTY_TRACE_DEBUG) || defined (DEBUG_DONEINP) || defined (SIGNAL_DEBUG) || defined (VFORK_DEBUG) || defined (HASH_DEBUG)
/*  We need a file to print debug statements to from a child process.
*/
#include <stdio.h>
FILE *childDebugStream;
#endif

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 13	/* set number */
	/* This file shares the same set number with sh.init.c. */
	/* The message number 1-32 are used in sh.init.c. */
	/* So the message number in this file begins with 41. */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include "sh.h"
#include "sh.dir.h"
#include "sh.proc.h"

extern void (*sigset())();
extern int (*sigignore())();

#ifdef SIGTSTP
#include <sys/wait.h>
#else

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */
union wait      {
        int     w_status;               /* used in syscall */
        /*
         * Terminated process status.
         */
        struct {
		unsigned short  dummy1:16;
                unsigned short  w_Retcode:8;    /* exit code if w_termsig==0 */
                unsigned short  w_Coredump:1;   /* core dump indicator */
                unsigned short  w_Termsig:7;    /* termination signal */
        } w_T;
        /*
         * Stopped process status.  Returned
         * only for traced children unless requested
         * with the WUNTRACED option bit.
         */
        struct {
		unsigned short  dummy2:16;
                unsigned short  w_Stopval:8;    /* == W_STOPPED if stopped */
                unsigned short  w_Stopsig:8;    /* signal that stopped us */
        } w_S;
};
#define w_termsig       w_T.w_Termsig
#define w_coredump      w_T.w_Coredump
#define w_retcode       w_T.w_Retcode
#define w_stopval       w_S.w_Stopval
#define w_stopsig       w_S.w_Stopsig


#define WSTOPPED        0177    /* value of s.stopval if process is stopped */


#define WEXITSTATUS(x)  (((int)(x)>>8)&0377)
#define WIFSTOPPED(x)   ((x).w_stopval == WSTOPPED)
#define WIFSIGNALED(x)  ((x).w_stopval == 0 && (x).w_stopsig != 0)
#define WIFEXITED(x)    ((x).w_stopval != WSTOPPED && (x).w_stopsig == 0)
#endif
#include <sys/ioctl.h>

/*
 * C Shell - functions that manage processes, handling hanging, termination
 */

#define BIGINDEX	9	/* largest desirable job index */

/*
 * pchild - called at interrupt level by the SIGCLD signal
 *	indicating that at least one child has terminated or stopped
 *	thus at least one wait system call will definitely return a
 *	childs status.  Top level routines (like pwait) must be sure
 *	to mask interrupts when playing with the proclist data structures!
 *
 *      Since HPUX does not have wait3(), we can only do one wait(). We
 *      must reinstall the SIGCLD signal handler (even though it is not
 *      reset) to take advantage of the Bell SIGCLD semantics that will cause
 *      the proc table to be re-scanned for zombies.
 */
/*  CHLD SIGNAL HANDLER
*/
/**********************************************************************/
void
pchild()
/**********************************************************************/
{
	register struct process *pp;
	register struct process	*fp;
	register int pid;
	union wait w;

/*  Variable to hold the exit status value if the child exited.
*/
int exitStatus;

	register struct varent *v;
	int jobflags;
#ifdef VMUNIX
	struct vtimes vt;
#endif

#ifdef DEBUG_SIGNAL
/*  Debug:  Used to determine what signal handler is set for SIGINT.
*/
  {
    struct sigaction oact;
    char *traceDebugIgn = "SIG_IGN";
    char *traceDebugDfl = "SIG_DFL";
    char *traceDebugOth = "other";
    char *traceDebugSig;

    sigaction (SIGINT, 0, &oact);

    if (oact.sa_handler == SIG_IGN)
      printf ("pchild (1): pid: %d, SIGINT handler: SIG_IGN\n", getpid ());

    else if (oact.sa_handler == SIG_DFL)
      printf ("pchild (1): pid: %d, SIGINT handler: SIG_DFL\n", getpid ());

    else
      printf ("pchild (1): pid: %d, SIGINT handler: %lo\n", getpid (), 
              oact.sa_handler);
  }
#endif

	if (!timesdone)
	{
		timesdone++; 
		times(&shtimes);
	}
loop:
#ifdef SIGTSTP
	pid = wait3(&w.w_status, (setintr ? WNOHANG|WUNTRACED:WNOHANG), (int *) 0);
#else
	pid = wait(&w.w_status); /* Berkeley uses a wait3() here */
#endif

#ifdef DEBUG_CHILD
  printf ("pchild (2): pid: %d, child pid: %d, wait status: %o\n", getpid (), 
	  pid, w.w_status);

  fflush (stdout);
  if (w.w_stopval == WSTOPPED)
    {
      printf ("\tStopped: signal = %hu\n", w.w_stopsig);
      fflush (stdout);
    }
    
  else
    {
      printf ("\tRetcode: %ho, Termsig: %ho\n", w.w_retcode, w.w_termsig);
      fflush (stdout);
    }
#endif

	if (pid <= 0) {
		if (errno == EINTR) {
			errno = 0;
			goto loop;
		}
		pnoprocesses = pid == -1;

#ifdef DEBUG_PROC
/*  Print the process table contents.
*/
  printf ("pchild (3): pid: %d\n", getpid ());
  fflush (stdout);
  dumpProcTable ();
  fflush (stdout);
#endif
		return;
	}

#ifndef SIGTSTP
	/* Now that we have successfully done a wait, we reset the SIGCLD */
	/* handler (even though it was never unset). This is to take      */
	/* advantage of the Bell SIGCLD semantics that causes the proc    */
	/* table to be re-scanned for zombies and generate another SIGCLD */
	/* if any are found. In this way we won't lose any children if    */
	/* two died at the same time, or at some time SIGCLD was being    */
	/* blocked.                                                       */

	/* This is not needed if we have wait3() 			  */
	sigset(SIGCLD,pchild);

#endif
	for (pp = proclist.p_next; pp != PNULL; pp = pp->p_next)
		if (pid == pp->p_pid)
			goto found;
#ifdef SIGTSTP
	goto loop;
#else

#ifdef DEBUG_PROC
/*  Print the process table contents.
*/
  printf ("pchild (4): pid: %d\n", getpid ());
  fflush (stdout);
  dumpProcTable ();
  fflush (stdout);
#endif
	return; /* Used to be goto loop:. We can not do that since we do   */
		/* not have wait3(). Instead we must exit handler and rely */
		/* on the Bell SIGCLD semantics.                           */
#endif

found:
	if (pid == Atoi(value(CH_child)))
		unsetv(CH_child);
	pp->p_flags &= ~(PRUNNING|PSTOPPED|PREPORTED);

#ifdef SIGTSTP
	if (WIFSTOPPED(w.w_status)) {
		pp->p_flags |= PSTOPPED;
		pp->p_reason = w.w_stopsig;
		} else {
#endif
		if (pp->p_flags & (PTIME|PPTIME) || adrof(CH_time)) {
			time_t oldcutimes, oldcstimes;
			oldcutimes = shtimes.tms_cutime;
			oldcstimes = shtimes.tms_cstime;
			time(&pp->p_etime);
			times(&shtimes);
			pp->p_utime = shtimes.tms_cutime - oldcutimes;
			pp->p_stime = shtimes.tms_cstime - oldcstimes;
		} else
			times(&shtimes);
#ifdef VMUNIX
		pp->p_vtimes = vt;
#endif
		if (WIFSIGNALED(w.w_status)) {
			if (w.w_termsig == SIGINT)
				pp->p_flags |= PINTERRUPTED;
			else
				pp->p_flags |= PSIGNALED;
			if (w.w_coredump)
				pp->p_flags |= PDUMPED;
			pp->p_reason = w.w_termsig;
		} else
		{
			pp->p_reason = w.w_retcode;
#ifdef IIASA
			if (pp->p_reason >= 3)
#else
			if (pp->p_reason != 0)
#endif
				pp->p_flags |= PAEXITED;
			else
				pp->p_flags |= PNEXITED;
		}

#ifdef SIGTSTP
	}
#endif
	jobflags = 0;
	fp = pp;
	do {
		if ((fp->p_flags & (PPTIME|PRUNNING|PSTOPPED)) == 0 &&
		    !child && adrof(CH_time) &&
		    (fp->p_utime + fp->p_stime) / HZ >=
		     Atoi(value(CH_time)))
			fp->p_flags |= PTIME;
		jobflags |= fp->p_flags;
	} while ((fp = fp->p_friends) != pp);
	pp->p_flags &= ~PFOREGND;
	if (pp == pp->p_friends && (pp->p_flags & PPTIME)) {
		pp->p_flags &= ~PPTIME;
		pp->p_flags |= PTIME;
	}
	if ((jobflags & (PRUNNING|PREPORTED)) == 0) {
		fp = pp;
		do {
			if (fp->p_flags&PSTOPPED)
				fp->p_flags |= PREPORTED;
		} while((fp = fp->p_friends) != pp);
		while(fp->p_leader != 1)
			fp = fp->p_friends;
		if (jobflags&PSTOPPED) {

#ifdef DEBUG_PROC
  printf ("pchild (7): pid: %d, job was stopped; changing pcurrent to: %d\n",
	   getpid (), fp);
  fflush (stdout);
#endif
			if (pcurrent && pcurrent != fp)
				pprevious = pcurrent;
			pcurrent = fp;
		} else
		    {

#ifdef DEBUG_PROC
  printf ("pchild (8): pid: %d, job NOT stopped; calling pclrcurr on: %d\n",
	   getpid (), fp);
  fflush (stdout);
#endif
			pclrcurr(fp);
		    }

		if (jobflags&PFOREGND) {
			if (jobflags & (PSIGNALED|PSTOPPED|PPTIME) ||
#ifdef IIASA
			    jobflags & PAEXITED ||
#endif
			    Strcmp(dcwd->di_name, fp->p_cwd->di_name)) {
				;	/* print in pjwait */
			}
/*
		else if ((jobflags & (PTIME|PSTOPPED)) == PTIME)
				ptprint(fp);
*/
		} else {
			v = adrof(CH_notify);
			if (jobflags&PNOTIFY || v) {
				if (v == 0 ||
				    (blklen(v->vec) == 1 && Strlen(v->vec[0]) == 0))
					printf("\015\n");	/* NLS: used to be \0215 */
				else
				{
					register int unit;

/*  For some reason, in 7.0 (with VFORK) even if hadderr was 1 just before the
    _exit call from the child, it was 0 on entry into this routine.  Therefore
    using fork instead of vfork in 8.0 shouldn't matter as far as haderr goes.
*/
					if (haderr)
						unit = didfds ? 2 : SHDIAG;
					else
						unit = didfds ? 1 : SHOUT;
					flush();
					write(unit,to_char(v->vec[0]),Strlen(v->vec[0]));
				}
				(void) pprint(pp, NUMBER|NAME|REASON);
				if (v != 0 && blklen(v->vec) > 1)
				{
					register int unit;

/*  See comment above on haderr.
*/
					if (haderr)
						unit = didfds ? 2 : SHDIAG;
					else
						unit = didfds ? 1 : SHOUT;
					flush();
					write(unit,to_char(v->vec[1]),Strlen(v->vec[1]));
				}
				if ((jobflags&PSTOPPED) == 0)
				  {
#ifdef DEBUG_PFLUSH
  printf ("pchild (9): pid: %d, Calling pflush()\n", getpid ());
#endif
					pflush(pp);
				  }
			} else {
				fp->p_flags |= PNEEDNOTE;
				neednote++;
			}
		}
	}
#ifdef SIGTSTP
	goto loop;
#else

#ifdef DEBUG_PROC
/*  Print the process table contents.
*/
  printf ("pchild (10): pid: %d\n", getpid ());
  fflush (stdout);
  dumpProcTable ();
  fflush (stdout);
#endif
	return; /* used to be goto loop:. See note on other return statement */
#endif
}

/**********************************************************************/
pnote()
/**********************************************************************/
{
	register struct process *pp;
	int flags;

	neednote = 0;
	for (pp = proclist.p_next; pp != PNULL; pp = pp->p_next) {
		if (pp->p_flags & PNEEDNOTE) {
			sighold(SIGCLD);
			pp->p_flags &= ~PNEEDNOTE;
			flags = pprint(pp, NUMBER|NAME|REASON);
			if ((flags&(PRUNNING|PSTOPPED)) == 0)
			  {
#ifdef DEBUG_PFLUSH
  printf ("pnote (1): pid: %d, Calling pflush()\n", getpid ());
#endif
				pflush(pp);
			  }
			sigrelse(SIGCLD);
		}
	}
}

/*
 * pwait - wait for current job to terminate, maintaining integrity
 *	of current and previous job indicators.
 */
/**********************************************************************/
pwait()
/**********************************************************************/
{
	register struct process *fp, *pp;

	/*
	 * Here's where dead procs get flushed.
	 */
	sighold(SIGCLD);
	for (pp = (fp = &proclist)->p_next; pp != PNULL; pp = (fp = pp)->p_next)

/*  If p_pid is 0 then the entire process structure is freed.  The p_pid gets
    set to 0 in pflush ().
*/
		if (pp->p_pid == 0) {
			fp->p_next = pp->p_next;
			xfree(pp->p_command);
			if (pp->p_cwd && --pp->p_cwd->di_count == 0)
				if (pp->p_cwd->di_next == 0)
					dfree(pp->p_cwd);
			xfree((CHAR *)pp);
			pp = fp;
		}
	sigrelse(SIGCLD);
	if (setintr)
		sigignore(SIGINT);
	pjwait(pcurrjob);
}

/*
 * pjwait - wait for a job to finish or become stopped
 *	It is assumed to be in the foreground state (PFOREGND)
 */
/**********************************************************************/
pjwait(pp)
	register struct process *pp;
/**********************************************************************/
{
	register struct process *fp;
	int jobflags, reason;
	sigset_t pending_set;

#ifdef DEBUG_SIGNAL
/*  Debug:  Used to determine what signal handler is set for SIGINT.
*/
  {
    struct sigaction oact;
    char *traceDebugIgn = "SIG_IGN";
    char *traceDebugDfl = "SIG_DFL";
    char *traceDebugOth = "other";
    char *traceDebugSig;

    sigaction (SIGINT, 0, &oact);

    if (oact.sa_handler == SIG_IGN)
      printf ("pjwait (1): pid: %d, SIGINT handler: SIG_IGN\n", getpid ());

    else if (oact.sa_handler == SIG_DFL)
      printf ("pjwait (1): pid: %d, SIGINT handler: SIG_DFL\n", getpid ());

    else
      printf ("pjwait (1): pid: %d, SIGINT handler: %lo\n", getpid (), 
              oact.sa_handler);
  }
#endif

#ifdef SIGTSTP
	while (pp->p_leader != 1)
		pp = pp->p_friends;
#endif
	fp = pp;
	do {
		if ((fp->p_flags&(PFOREGND|PRUNNING)) == PRUNNING)
			printf((catgets(nlmsg_fd,NL_SETN,41, "BUG: waiting for background job!\n")));
	} while ((fp = fp->p_friends) != pp);
	/*
	 * Now keep pausing as long as we are not interrupted (SIGINT),
	 * and the target process, or any of its friends, are running
	 *
	 * DSDe412996: The SIGINT is blocked, and will not arrive here!
	 */
	fp = pp;
	for (;;) {
		sighold(SIGCLD);
		jobflags = 0;
		do
			jobflags |= fp->p_flags;
		while((fp = (fp->p_friends)) != pp);
		if ((jobflags & PRUNNING) == 0)
			break;
		sigpause(sigblock(0) & ~((1L << (SIGCLD - 1)) | 
					 (1L << (SIGUSR2 - 1))));
	}

	/*
	 * Just in case the child sent a SIGUSR2 but we haven't
	 * seen it yet....
	 */
	if (!intty) {
		sigpending(&pending_set);
		if (sigismember(&pending_set, SIGUSR2))
			sigpause(sigblock(0) & ~((1L << (SIGUSR2 - 1))));
	}

#ifdef DEBUG_CHILD
  printf ("pjwait (2): %d, releasing sigchild; child is done\n", getpid ());
  fflush (stdout);
#endif

	sigrelse(SIGCLD);

	/* DSDe412996: Now release SIGINT. */
	sigrelse(SIGINT);

#ifdef SIGTSTP
	if (tpgrp > 0)
          {

#ifdef DEBUG_CHILD
  printf ("pjwait (3): %d, setting ttypgrp to %d\n", getpid (), tpgrp);
  fflush (stdout);
#endif

	    ioctl(FSHTTY, TIOCSPGRP, &tpgrp); /*get tty back */
          }
#endif
	if ((jobflags&(PSIGNALED|PSTOPPED|PTIME)) ||
	     Strcmp(dcwd->di_name, fp->p_cwd->di_name)) {
		if (jobflags&PSTOPPED)
			printf("\n");
		(void) pprint(pp, AREASON|SHELLDIR);
	}
	if ((jobflags&(PINTERRUPTED|PSTOPPED)) && setintr &&
#ifdef SIGTSTP
	    (!gointr && !eq(gointr, "-"))) {
#else
	    (gointr && !eq(gointr, "-"))) {
#endif

#ifdef DEBUG_CHILD
  printf ("pjwait (4): %d, job was stopped or interrupted; calling pintr1\n",
	   getpid ());
#endif

		if ((jobflags & PSTOPPED) == 0)
		  {
#ifdef DEBUG_PFLUSH
  printf ("pjwait (5): pid: %d, Calling pflush()\n", getpid ());
#endif
			pflush(pp);
		  }
		pintr1(0);
		/*NOTREACHED*/
	}
	reason = 0;
	fp = pp;
	do {
		if (fp->p_reason)
			reason = fp->p_flags & (PSIGNALED|PINTERRUPTED) ?
				fp->p_reason | 0200 : fp->p_reason;
	} while ((fp = fp->p_friends) != pp);

#if defined (DEBUG_CHILD) || defined (DEBUG_STATUS)
  printf ("pjwait (6): pid: %d, reason: %lo\n", getpid (), reason);
  fflush (stdout);
#endif
	set(CH_status, putn(reason));
	if (reason && exiterr)
	  {

#ifdef DEBUG_STATUS
  printf ("pjwait (7): pid: %d, reason and exiterr; exiting via exitstat\n",
	   getpid ());
#endif

		exitstat();
	  }

#ifdef DEBUG_PFLUSH
  printf ("pjwait (8): pid: %d, Calling pflush()\n", getpid ());
#endif
	pflush(pp);
}

/*
 * dowait - wait for all processes to finish
 */
/**********************************************************************/
dowait()
/**********************************************************************/
{
	register struct process *pp;

	pjobs++;
	if (setintr)
	{
		sigrelse(SIGINT);
#ifndef SIGTSTP
		sigset(SIGINT, pintr);
#endif
	}
loop:
	sighold(SIGCLD);
	for (pp = proclist.p_next; pp; pp = pp->p_next)
		if (pp->p_pid && pp->p_leader == 1 &&
		    pp->p_flags&PRUNNING) {
			sigpause(sigblock(0) & ~(1L << (SIGCLD - 1)));
			goto loop;
		}
	sigrelse(SIGCLD);
	pjobs = 0;
}

/*
 * pflushall - flush all jobs from list (e.g. at fork())
 */
/**********************************************************************/
pflushall()
/**********************************************************************/
{
	register struct process	*pp;

	for (pp = proclist.p_next; pp != PNULL; pp = pp->p_next)
		if (pp->p_pid)
		  {
#ifdef DEBUG_PFLUSH
  printf ("pflushall (1): pid: %d, Calling pflush()\n", getpid ());
#endif
			pflush(pp);
		  }
}

/*
 * pflush - flag all process structures in the same job as the
 *	the argument process for deletion.  The actual free of the
 *	space is not done here since pflush is called at interrupt level.
 */
/**********************************************************************/
pflush(pp)
	register struct process	*pp;
/**********************************************************************/
{
	register struct process *np;
	register int index;

#ifdef DEBUG_PFLUSH
  printf ("pflush (1): pid: %d\n", getpid ());
#endif
	if (pp->p_pid == 0) {
		printf((catgets(nlmsg_fd,NL_SETN,42, "BUG: process flushed twice")));
		return;
	}
	while (pp->p_leader != 1)
		pp = pp->p_friends;
	pclrcurr(pp);
	if (pp == pcurrjob)
		pcurrjob = 0;
	index = pp->p_index;
	np = pp;
	do {

/*  This sets the process id in the process structure to 0.  It will be
    freed in pwait ().
*/
		np->p_index = np->p_pid = 0;
		np->p_flags &= ~PNEEDNOTE;
	} while ((np = np->p_friends) != pp);
	if (index == pmaxindex) {
		for (np = proclist.p_next, index = 0; np; np = np->p_next)
			if (np->p_index > index)
				index = np->p_index;
		pmaxindex = index;
	}
}

/*
 * pclrcurr - make sure the given job is not the current or previous job;
 *	pp MUST be the job leader
 */
/**********************************************************************/
pclrcurr(pp)
	register struct process *pp;
/**********************************************************************/
{

#ifdef DEBUG_PROC
  printf ("pclrcurr (1): pid: %d, Original pcurrent: %d, pprevious: %d\n", 
	   getpid (), pcurrent, pprevious);
#endif

	if (pp == pcurrent)
	  {
		if (pprevious != PNULL) 
		  {
			pcurrent = pprevious;
			pprevious = pgetcurr(pp);
		  } 

	        else 
		  {
			pcurrent = pgetcurr(pp);
			pprevious = pgetcurr(pp);
		  }
	  }

	else if (pp == pprevious)
	  {
		pprevious = pgetcurr(pp);
	  }

#ifdef DEBUG_PROC
  printf ("pclrcurr (2): pid: %d, New pcurrent: %d, pprevious: %d\n", 
	   getpid (), pcurrent, pprevious);
#endif
}

/*
 * pclrcurr1 - almost the same as pclrcurr (differ by one line),
 *	when pp is the only process, i.e. pp == pcurrent,
 *	pprevious == PNULL, and pgetcurr(pp) == PNULL,
 *	pcurrent stays the same.
 *	this function is used only by pstart() which is called only by
 *	dofg(), dofg1, dobg(), dobg1().   -jh
 */
/**********************************************************************/
pclrcurr1(pp)
	register struct process *pp;
/**********************************************************************/
{

	if (pp == pcurrent)
		if (pprevious != PNULL) {
			pcurrent = pprevious;
			pprevious = pgetcurr(pp);
		} else {
			pcurrent = (pgetcurr(pp) == PNULL) ? pp : pgetcurr(pp);
			pprevious = pgetcurr(pp);
		}
	else if (pp == pprevious)
		pprevious = pgetcurr(pp);
}

/* +4 here is 1 for '\0', 1 ea for << >& >> */
CHAR	command[PMAXLEN+4];
int	cmdlen;
CHAR	*cmdp;
/*
 * palloc - allocate a process structure and fill it up.
 *	an important assumption is made that the process is running.
 */
/*  Called by:
	pfork ()
*/
/**********************************************************************/
palloc(pid, t)
	int pid;
	register struct command *t;
/**********************************************************************/
{
	register struct process	*pp;
	int i;

	pp = (struct process *)calloc(1, sizeof(struct process));
	pp->p_pid = pid;
	pp->p_flags = t->t_dflg & FAND ? PRUNNING : PRUNNING|PFOREGND;
	if (t->t_dflg & FTIME)
		pp->p_flags |= PPTIME;
	cmdp = command;
	cmdlen = 0;
	padd(t);
	*cmdp++ = 0;
	if (t->t_dflg & FPOU) {
		pp->p_flags |= PPOU;
		if (t->t_dflg & FDIAG)
			pp->p_flags |= PDIAG;
	}
	pp->p_command = savestr(command);
	if (pcurrjob) {
		struct process *fp;
		/* careful here with interrupt level */
		pp->p_cwd = 0;
		pp->p_index = pcurrjob->p_index;
		pp->p_friends = pcurrjob;
		pp->p_jobid = pcurrjob->p_jobid;
		/* pp->p_jobid = pcurrjob->p_pid; */
		pp->p_leader = 0;
		for (fp = pcurrjob; fp->p_friends != pcurrjob; fp = fp->p_friends)
			if (fp->p_jobid != pcurrjob->p_jobid)
				fp->p_jobid = pcurrjob->p_jobid;
		fp->p_friends = pp;
	} else {
		pcurrjob = pp;
		pp->p_jobid = pid;
		pp->p_leader = 1;
		pp->p_friends = pp;
		pp->p_cwd = dcwd;
		dcwd->di_count++;
		if (pmaxindex < BIGINDEX)
			pp->p_index = ++pmaxindex;
		else {
			struct process *np;

			for (i = 1; ; i++) {
				for (np = proclist.p_next; np; np = np->p_next)
					if (np->p_index == i)
						goto tryagain;
				pp->p_index = i;
				if (i > pmaxindex)
					pmaxindex = i;
				break;			
			tryagain:;
			}
		}
		if (pcurrent == PNULL)
			pcurrent = pp;
		else if (pprevious == PNULL)
			pprevious = pp;
	}
	pp->p_next = proclist.p_next;
	proclist.p_next = pp;
	time(&pp->p_btime);

#ifdef TRACE_DEBUG
  printf ("palloc (1): pid: %d\n", getpid ());
  fflush (stdout);
#endif

#ifdef DEBUG_PROC
/*  Print the process table contents.
*/
  printf ("palloc (2): pid: %d\n", getpid ());
  fflush (stdout);
  dumpProcTable ();
  fflush (stdout);
#endif
}

#ifndef NONLS
CHAR CH_right_paren[] = {'(',' ',0};
CHAR CH_left_paren[] = {' ',')',0};
CHAR CH_blank[] = {' ',0};
CHAR CH_pipe[] = {' ','|',' ',0};
CHAR CH_semicolon[] = {';',' ',0};
CHAR CH_append_inp[] = {' ','<','<',' ',0};
CHAR CH_inp[] = {' ','<',' ',0};
CHAR CH_append_out[] = {' ','>','>',0};
CHAR CH_out[] = {' ','>',0};
CHAR CH_and[] = {'&',0};
#else
#define CH_right_paren 		"( "
#define CH_left_paren   	" )"
#define CH_blank   		" "
#define CH_pipe   		" | "
#define CH_semicolon   		"; "
#define CH_append_inp   	" << "
#define CH_inp   		" < "
#define CH_append_out   	" >>"
#define CH_out   		" >"
#define CH_and   		"&"
#endif

/**********************************************************************/
padd(t)
	register struct command *t;
/**********************************************************************/
{
	CHAR **argp;

	if (t == 0)
		return;
	switch (t->t_dtyp) {

	case TPAR:
		pads(CH_right_paren);
		padd(t->t_dspr);
		pads(CH_left_paren);
		break;

	case TCOM:
		for (argp = t->t_dcom; *argp; argp++) {
			pads(*argp);
			if (argp[1])
				pads(CH_blank);
		}
		break;

	case TFIL:
		padd(t->t_dcar);
		pads(CH_pipe);
		padd(t->t_dcdr);
		return;

	case TLST:
		padd(t->t_dcar);
		pads(CH_semicolon);
		padd(t->t_dcdr);
		return;
	}
	if ((t->t_dflg & FPIN) == 0 && t->t_dlef) {
		pads((t->t_dflg & FHERE) ? CH_append_inp : CH_inp);
		pads(t->t_dlef);
	}
	if ((t->t_dflg & FPOU) == 0 && t->t_drit) {
		pads((t->t_dflg & FCAT) ? CH_append_out : CH_out);
		if (t->t_dflg & FDIAG)
			pads(CH_and);
		pads(CH_blank);
		pads(t->t_drit);
	}
}

#ifndef NONLS
CHAR CH_dotdotdot[] = {' ','.','.','.',0};
#else
#define CH_dotdotdot	" ..."
#endif

/**********************************************************************/
pads(cp)
	CHAR *cp;
/**********************************************************************/
{
	register int i = Strlen(cp);

	if (cmdlen >= PMAXLEN)
		return;
	if (cmdlen + i >= PMAXLEN) {
		Strcpy(cmdp, CH_dotdotdot);
		cmdlen = PMAXLEN;
		cmdp += 4;
		return;
	}
	Strcpy(cmdp, cp);
	cmdp += i;
	cmdlen += i;
}

/*
 * psavejob - temporarily save the current job on a one level stack
 *	so another job can be created.  Used for { } in exp6
 *	and `` in globbing.
 */
/**********************************************************************/
psavejob()
/**********************************************************************/
{

	pholdjob = pcurrjob;
	pcurrjob = PNULL;
}

/*
 * prestjob - opposite of psavejob.  This may be missed if we are interrupted
 *	somewhere, but pendjob cleans up anyway.
 */
/**********************************************************************/
prestjob()
/**********************************************************************/
{

	pcurrjob = pholdjob;
	pholdjob = PNULL;
}

/*
 * pendjob - indicate that a job (set of commands) has been completed
 *	or is about to begin.
 */
/**********************************************************************/
pendjob()
/**********************************************************************/
{
	register struct process *pp, *tp;

	if (pcurrjob && (pcurrjob->p_flags&(PFOREGND|PSTOPPED)) == 0) {
		pp = pcurrjob;
		while (pp->p_leader != 1)
			pp = pp->p_friends;
		printf("[%d]", pp->p_index);
		tp = pp;
		do {
			printf(" %d", pp->p_pid);
			pp = pp->p_friends;
		} while (pp != tp);
		printf("\n");
	}
	pholdjob = pcurrjob = 0;
}

/*
 * pprint - print a job
 */
/**********************************************************************/
pprint(pp, flag)
	register struct process	*pp;
/**********************************************************************/
{
	register status, reason;
	struct process *tp;
	extern char *linp, linbuf[];
	int jobflags, pstatus;
	char *format,*str1;
	char flg;

	while (pp->p_leader != 1)
		pp = pp->p_friends;
	if (pp == pp->p_friends && (pp->p_flags & PPTIME)) {
		pp->p_flags &= ~PPTIME;
		pp->p_flags |= PTIME;
	}
	tp = pp;
	status = reason = -1; 
	jobflags = 0;
	do {
		jobflags |= pp->p_flags;
		pstatus = pp->p_flags & PALLSTATES;
		if (tp != pp && linp != linbuf && !(flag&FANCY) &&
		    (pstatus == status && pp->p_reason == reason ||
		     !(flag&REASON)))
			printf(" ");
		else {
			if (tp != pp && linp != linbuf)
				printf("\n");
			if(flag&NUMBER)
				if (pp == tp)
				{
					printf("[%d]",pp->p_index);
					if(pp==pcurrent)
					    flg='+';
					else if(pp == pprevious)
					    flg='-';
					else 
					    flg=' ';
					if(pp->p_index < 10)
					    str1= " ";
					else
					    str1= "";
					printf("%s",str1);
					printf(" %c",flg);
				}
				else
					printf("       ");
			if (flag&FANCY)
				printf("%5d ", pp->p_pid);
			if (flag&(REASON|AREASON)) {
				if (flag&NAME)
					format = "%-21s";
				else
					format = "%s";
				if (pstatus == status)
					if (pp->p_reason == reason) {
						printf(format, "");
						goto prcomd;
					} else
						reason = pp->p_reason;
				else {
					status = pstatus;
					reason = pp->p_reason;
				}
				switch (status) {

				case PRUNNING:
					printf(format, (catgets(nlmsg_fd,NL_SETN,43, "Running ")));
					break;

				case PINTERRUPTED:
				case PSTOPPED:
				case PSIGNALED:
					/* DSDe407880 Broken pipe fix */
					if (flag&(REASON|AREASON)
					&& reason != SIGINT
					&& reason != SIGPIPE)
						printf(format, (catgets(nlmsg_fd,NL_SETN,pp->p_reason,mesg[pp->p_reason].pname)));
					break;

				case PNEXITED:
				case PAEXITED:
					if (flag & REASON)
						if (pp->p_reason)
							printf((catgets(nlmsg_fd,NL_SETN,44, "Exit %-16d")), pp->p_reason);
						else
							printf(format, (catgets(nlmsg_fd,NL_SETN,45, "Done")));
					break;

				default:
					printf((catgets(nlmsg_fd,NL_SETN,46, "BUG: status=%-9o")), status);
				}
			}
		}
prcomd:
		if (flag&NAME) {
			printf("%-.50s", to_char(pp->p_command));
			if (pp->p_flags & PPOU)
				printf(" |");
			if (pp->p_flags & PDIAG)
				printf("&");
		}
		if (flag&(REASON|AREASON) && pp->p_flags&PDUMPED)
			printf((catgets(nlmsg_fd,NL_SETN,47, " (core dumped)")));
		if (tp == pp->p_friends) {
			if (flag&AMPERSAND)
				printf(" &");
			if (flag&JOBDIR &&
			    Strcmp(tp->p_cwd->di_name, dcwd->di_name)) {
				printf((catgets(nlmsg_fd,NL_SETN,48, " (wd: ")));
				dtildepr(value(CH_home), tp->p_cwd->di_name);
				printf(")");
			}
		}
		if (pp->p_flags&PPTIME && !(status&(PSTOPPED|PRUNNING))) {
			if (linp != linbuf)
				printf("\n\t");
#ifndef VMUNIX
			ptimes(pp->p_utime, pp->p_stime, pp->p_etime-pp->p_btime);
#else
			pvtimes(&zvms, &pp->p_vtimes, pp->p_etime - pp->p_btime);
#endif
		}
		if (tp == pp->p_friends) {
			if (linp != linbuf)
				printf("\n");
			if (flag&SHELLDIR && Strcmp(tp->p_cwd->di_name, dcwd->di_name)) {
				printf((catgets(nlmsg_fd,NL_SETN,49, "(wd now: ")));
				dtildepr(value(CH_home), dcwd->di_name);
				printf(")\n");
			}
		}
	} while ((pp = pp->p_friends) != tp);
	if (jobflags&PTIME && (jobflags&(PSTOPPED|PRUNNING)) == 0) {
		if (jobflags & NUMBER)
			printf("       ");
		ptprint(tp);
	}
	return (jobflags);
}

/**********************************************************************/
ptprint(tp)
	register struct process *tp;
/**********************************************************************/
{
	time_t tetime = 0;
#ifdef VMUNIX
	struct vtimes vmt;
#else
	time_t tutime = 0, tstime = 0;
#endif
	register struct process *pp = tp;

#ifdef VMUNIX
	vmt = zvms;
#endif
	do {
#ifdef VMUNIX
		vmsadd(&vmt, &pp->p_vtimes);
#else
		tutime += pp->p_utime;
		tstime += pp->p_stime;
#endif
		if (pp->p_etime - pp->p_btime > tetime)
			tetime = pp->p_etime - pp->p_btime;
	} while ((pp = pp->p_friends) != tp);
#ifdef VMUNIX
	pvtimes(&zvms, &vmt, tetime);
#else
	ptimes(tutime, tstime, tetime);
#endif
}

/*
 * dojobs - print all jobs
 */
/**********************************************************************/
dojobs(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;
	register int flag = NUMBER|NAME|REASON;
	int i;

	if (chkstop)
		chkstop = 2;
	if (*++v) {
		if (v[1] || !eq(*v, "-l"))
			error((catgets(nlmsg_fd,NL_SETN,50, "Usage: jobs [ -l ]")));
		flag |= FANCY|JOBDIR;
	}
	for (i = 1; i <= pmaxindex; i++)
		for (pp = proclist.p_next; pp; pp = pp->p_next)
			if (pp->p_index == i && pp->p_leader == 1) {
				pp->p_flags &= ~PNEEDNOTE;
				if (!(pprint(pp, flag) & (PRUNNING|PSTOPPED)))
				  {
#ifdef DEBUG_PFLUSH
  printf ("dojobs (1): pid: %d, Calling pflush()\n", getpid ());
#endif
					pflush(pp);
				  }
				break;
			}
}

#ifdef SIGTSTP
/*
 * dofg - builtin - put the job into the foreground
 */
/**********************************************************************/
dofg(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;

	okpcntl();
	++v;
	do {

#ifdef DEBUG_PFIND
  printf ("dofg (1): pid: %d, Calling pfind()\n", getpid ());
#endif
		pp = pfind(*v);
		pstart(pp, 1);
		if (setintr)
			sigignore(SIGINT);
		pjwait(pp);
	} while (*v && *++v);
}
#endif

/*
 * %... - builtin - put the job into the foreground
 */
/**********************************************************************/
dofg1(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;

	okpcntl();

#ifdef DEBUG_PFIND
  printf ("dofg1 (1): pid: %d, Calling pfind()\n", getpid ());
#endif
	pp = pfind(v[0]);
	pstart(pp, 1);
	if (setintr)
		sigignore(SIGINT);
	pjwait(pp);
}

#ifdef SIGTSTP
/*
 * dobg - builtin - put the job into the background
 */
/**********************************************************************/
dobg(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;

	okpcntl();
	++v;
	do {

#ifdef DEBUG_PFIND
  printf ("dobg (1): pid: %d, Calling pfind()\n", getpid ());
#endif
		pp = pfind(*v);
		pstart(pp, 0);
	} while (*v && *++v);
}
#endif

/*
 * %... & - builtin - put the job into the background
 */
/**********************************************************************/
dobg1(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;

#ifdef DEBUG_PFIND
  printf ("dobg1 (1): pid: %d, Calling pfind()\n", getpid ());
#endif
	pp = pfind(v[0]);
	pstart(pp, 0);
}

/*
 * dostop - builtin - stop the job
 */
/**********************************************************************/
dostop(v)
	CHAR **v;
/**********************************************************************/
{
#ifdef SIGTSTP

	pkill(++v, SIGSTOP);
#endif
}

/*
 * dokill - builtin - superset of kill (1)
 */
/**********************************************************************/
dokill(v)
	CHAR **v;
/**********************************************************************/
{
	register int signum;
	register char *name;

	v++;
	if (v[0] && v[0][0] == '-') {
		if (v[0][1] == 'l') {
			/* skip mesg[0] since it is a null string anyway.hn */
			for (signum = 1; signum < NSIG; signum++) {
				if (name = mesg[signum].iname)
					printf("%s ", name);
				if (signum == 16)
					printf("\n");
			}
			printf("\n");
			return;
		}
		if (digit(v[0][1])) {
			signum = Atoi(v[0]+1);
			if (signum < 1 || signum >= NSIG)
				bferr((catgets(nlmsg_fd,NL_SETN,51, "Bad signal number")));
		} else {
			name = to_char(&v[0][1]);
			for (signum = 1; signum < NSIG; signum++)
			if (mesg[signum].iname &&
			    strcmp(name, mesg[signum].iname)==0)
				goto gotsig;
			setname(name);
			bferr((catgets(nlmsg_fd,NL_SETN,52, "Unknown signal; kill -l lists signals")));
		}
gotsig:
#ifndef NONLS
		xfree(name);
#endif
		v++;
	} else
		signum = SIGTERM;
	pkill(v, signum);
}

/**********************************************************************/
pkill(v, signum)
	CHAR **v;
	int signum;
/**********************************************************************/
{
	register struct process *pp, *np;
#ifdef SIGTSTP
	register int jobflags = 0;
#endif
	int pid;
	CHAR *cp;
	int err = 0;

	if (setintr)
		sighold(SIGINT);
	sighold(SIGCLD);
	while (*v) {
		cp = globone(*v);
		if (*cp == '%') {

#ifdef DEBUG_PFIND
  printf ("pkill (1): pid: %d, Calling pfind()\n", getpid ());
#endif
			np = pp = pfind(cp);
#ifdef SIGTSTP
			do
				jobflags |= np->p_flags;
			while ((np = np->p_friends) != pp);

			switch (signum) {

			case SIGSTOP:
			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				if ((jobflags & PRUNNING) == 0) {
					printf((catgets(nlmsg_fd,NL_SETN,53, "%s: Already stopped\n")), to_char(cp));
					err++;
					goto cont;
				}
			}
			if (killpg(pp->p_jobid, signum) && errno == ESRCH)
				kill(pp->p_jobid, signum);
			if (signum == SIGTERM || signum == SIGHUP)
				killpg(pp->p_jobid, SIGCONT);
#else
			/* loop through and kill each pid individually */
			/* since background jobs are not in their own  */
			/* process group.                              */

			do
			if (kill(np->p_pid, signum) < 0) {
				printf("%d: ", np->p_pid);
#ifndef NLS
				printf("%s\n", sys_errlist[errno]);
#else
				printf("%s\n", strerror(errno));
#endif
				err++;
				goto cont;
			}
			while ((np = np->p_friends) != pp);
#endif
		} else if (!digit(*cp) && (*cp) != '-')
			bferr((catgets(nlmsg_fd,NL_SETN,54, "Arguments should be jobs or process id's")));
		else {
			pid = Atoi(cp);
			if (kill(pid, signum) < 0) {
				printf("%d: ", pid);
#ifndef NLS
				printf("%s\n", sys_errlist[errno]);
#else
				printf("%s\n", strerror(errno));
#endif
				err++;
				goto cont;
			}
#ifdef SIGTSTP
			if (signum == SIGTERM || signum == SIGHUP)
				kill(pid, SIGCONT);
#endif
		}
cont:
		xfree(cp);
		v++;
	}
	sigrelse(SIGCLD);
	if (setintr)
	{
		sigrelse(SIGINT);
#ifndef SIGTSTP
		sigset(SIGINT, pintr);
#endif
	}
	if (err)
		error((char *) 0);
}

/*
 * pstart - start the job in foreground/background
 */
/**********************************************************************/
pstart(pp, foregnd)
	register struct process *pp;
	int foregnd;
/**********************************************************************/
{
	register struct process *np;
#ifdef SIGTSTP
	int jobflags = 0;
#endif

	sighold(SIGCLD);
	np = pp;
	do {
#ifdef SIGTSTP
		jobflags |= np->p_flags;
#endif
		if (np->p_flags&(PRUNNING|PSTOPPED)) {
			np->p_flags |= PRUNNING;
			np->p_flags &= ~PSTOPPED;
			if (foregnd)
				np->p_flags |= PFOREGND;
			else
				np->p_flags &= ~PFOREGND;
		}
	} while((np = np->p_friends) != pp);
	if (!foregnd)
		pclrcurr1(pp);
	(void) pprint(pp, foregnd ? NAME|JOBDIR : NUMBER|NAME|AMPERSAND);
#ifdef SIGTSTP
	if (foregnd)
		ioctl(FSHTTY, TIOCSPGRP, &pp->p_jobid);
	if (jobflags&PSTOPPED)
		killpg(pp->p_jobid, SIGCONT);
#endif
	sigrelse(SIGCLD);
}

/**********************************************************************/
panystop(neednl)
/**********************************************************************/
{
	register struct process *pp;

	chkstop = 2;
	for (pp = proclist.p_next; pp; pp = pp->p_next)
		if (pp->p_flags & PSTOPPED)
			error((catgets(nlmsg_fd,NL_SETN,55, "\nThere are stopped jobs")) + 1 - neednl);
}

/*  Called by:
	dofg ();
	dofg1 ();
	dobg ();
	dobg1 ();
	pkill ();
	donotify ();
*/
/**********************************************************************/
struct process *
pfind(cp)
	CHAR *cp;
/**********************************************************************/
{
	register struct process *pp, *np;

#ifdef DEBUG_PFIND
  printf ("pfind (1): pid: %d, In pfind(), cp: %s\n", getpid (), to_char (cp));
#endif
	if (cp == 0 || cp[1] == 0 || eq(cp, "%%") || eq(cp, "%+")) {
		if (pcurrent == PNULL)
			bferr((catgets(nlmsg_fd,NL_SETN,56, "No current job")));
		return (pcurrent);
	}
	if (eq(cp, "%-") || eq(cp, "%#")) {
		if (pprevious == PNULL)
			bferr((catgets(nlmsg_fd,NL_SETN,57, "No previous job")));
		return (pprevious);
	}
	if (digit(cp[1])) {
		int index = Atoi(cp+1);

#ifdef DEBUG_PFIND
  printf ("pfind (2): pid: %d, index: %d\n", getpid (), index);
  dumpProcTable ();
#endif

/*  If the index is 0, then we requested something like '%0'.  The problem 
    with this is that if the shell has put anything in the process table, when
    it is done the job index number is reset to 0.  Later the space is freed.
    So if the job is done, but the index is 0, then we would get a match and
    return the process information for the job that has already completed.
    This causes the calling routines to do a wait for the child process, which
    has already terminated.  They then attempt to flush the child from the
    process table, and this causes the job to have been flushed twice, resulting
    in an error message in pflush().

    This was reported as DTS #FSDlj07349.  The fix is an explicit check for 
    a job number of 0, which is now illegal.
*/
		if (index == 0)
		  {
#ifdef DEBUG_PFIND
  printf ("pfind (3): pid: %d, cp[1] digit, no such job.\n", getpid ());
#endif
		    bferr((catgets(nlmsg_fd,NL_SETN,58, "No such job")));
		  }

		for (pp = proclist.p_next; pp; pp = pp->p_next)
			if (pp->p_index == index && pp->p_leader == 1)
				return (pp);

#ifdef DEBUG_PFIND
  printf ("pfind (4): pid: %d, cp[1] digit, no such job.\n", getpid ());
#endif
		bferr((catgets(nlmsg_fd,NL_SETN,58, "No such job")));
	}
	np = PNULL;
	for (pp = proclist.p_next; pp; pp = pp->p_next)
		if (pp->p_leader == 1 && pp->p_index != 0) {
			if (cp[1] == '?') {
				register CHAR *dp;
				for (dp = pp->p_command; *dp; dp++) {
					if (*dp != cp[2])
						continue;
					if (prefix(cp+2, dp))
						goto match;
				}
			} else if (prefix(cp+1, pp->p_command)) {
match:
				if (np)
					bferr((catgets(nlmsg_fd,NL_SETN,59, 
					       "Ambiguous")));
				np = pp;
			}
		}
	if (np)
		return (np);

	if (cp[1] == '?')
		bferr((catgets(nlmsg_fd,NL_SETN,60, "No job matches pattern")));

	else
	  {

#ifdef DEBUG_PFIND
  printf ("pfind (5): pid: %d, cp[1] not ? or digit, no such job.\n",getpid ());
#endif
		bferr((catgets(nlmsg_fd,NL_SETN,61, "No such job")));
	  }
	/* NOTREACHED */
}

/*
 * pgetcurr - find most recent job that is not pp, preferably stopped
 */
/**********************************************************************/
struct process *
pgetcurr(pp)
	register struct process *pp;
/**********************************************************************/
{
	register struct process *np;
	register struct process *xp = PNULL;

	for (np = proclist.p_next; np; np = np->p_next)
		if (np != pcurrent && np != pp && np->p_pid &&
		    np->p_leader == 1) {
			if (np->p_flags & PSTOPPED)
				return (np);
			if (xp == PNULL)
				xp = np;
		}
	return (xp);
}

/*
 * donotify - flag the job so as to report termination asynchronously
 */
/**********************************************************************/
donotify(v)
	CHAR **v;
/**********************************************************************/
{
	register struct process *pp;

#ifdef DEBUG_PFIND
  printf ("donotify (1): pid: %d, Calling pfind()\n",getpid ());
#endif
	pp = pfind(*++v);
	pp->p_flags |= PNOTIFY;
}

/*
 * psigusr1 - this is the signal handler for SIGUSR1 and simply
 *	      returns
 *	    - it's needed by the child of the fork so that the
 *	      child can sigpause on the SIGUSR1 signal sent by the parent
 */
/**********************************************************************/
void
psigusr1()
/**********************************************************************/
{
}

/*
 * psigusr2 - this is the signal handler for SIGUSR2
 *	    - SIGUSR2 is sent by the child to its parent to indicate
 *	      that doneinp should be set (ie. a shell script cannot
 *	      find the command to execute or globbing has failed)
 *	    - the indication to set doneinp used to be encoded in
 *	      the exit value from the child, but it caused problems
 *	      for real programs which used the same return value
 */
/**********************************************************************/
void
psigusr2()
/**********************************************************************/
{
	doneinp = 1;
}

/*
 * Do the fork and whatever should be done in the child side that
 * should not be done if we are not forking at all (like for simple builtin's)
 * Also do everything that needs any signals fiddled with in the parent side
 *
 * Wanttty tells whether process and/or tty pgrps are to be manipulated:
 *	-1:	leave tty alone; inherit pgrp from parent
 *	 0:	already have tty; manipulate process pgrps only
 *	 1:	want to claim tty; manipulate process and tty pgrps
 * It is usually just the value of tpgrp.
 */
/*  Note that this routine is only called to do a fork.  So any globals touched
    in this routine after the fork will not affect the parent, and never did.
*/
/**********************************************************************/
pfork(t, wanttty)
	struct command *t;	/* command we are forking for */
	int wanttty;
/**********************************************************************/
{
	register int pid;
	bool ignint = 0;
	int pgrp;
	int   forkcnt = 0;
	int saveErr;
	char forkErr [40];

#if defined (DEBUG_CHILD) || defined (TRACE_DEBUG) || defined (ENV_DEBUG) || defined (NITTY_TRACE_DEBUG) || defined (DEBUG_DONEINP) || defined (SIGNAL_DEBUG) || defined (VFORK_DEBUG) || defined (HASH_DEBUG)
/*  Open the file to print debug statements to from a child process.
*/
  fclose (childDebugStream);
  childDebugStream = fopen ("debugChild", "a");
#endif
	/*
	 * A child will be uninterruptible only under very special
	 * conditions. Remember that the semantics of '&' is
	 * implemented by disconnecting the process from the tty so
	 * signals do not need to ignored just for '&'.
	 * Thus signals are set to default action for children unless:
	 *	we have had an "onintr -" (then specifically ignored)
	 *	we are not playing with signals (inherit action)
	 */

	if (setintr)
		ignint = (tpgrp == -1 && (t->t_dflg&FINT))
		    || (gointr && eq(gointr, "-"));
	flush();
	/*
	 * Hold SIGCLD until we have the process installed in our table.
	 */
	sighold(SIGCLD);

	/* DSDe412996: The parent dies leaving zombie child if interrupt is
	 * received. Don't ignore interrupt here, instead block it until
	 * after wait.
 	 */
	sighold(SIGINT);

#ifdef DEBUG_SIGNAL
/*  Debug:  Used to determine what signal handler is set for SIGINT.
*/
  {
    struct sigaction oact;
    char *traceDebugIgn = "SIG_IGN";
    char *traceDebugDfl = "SIG_DFL";
    char *traceDebugOth = "other";
    char *traceDebugSig;

    sigaction (SIGINT, 0, &oact);

    if (oact.sa_handler == SIG_IGN)
      printf ("pfork (1): pid: %d, SIGINT handler: SIG_IGN\n", getpid ());

    else if (oact.sa_handler == SIG_DFL)
      printf ("pfork (1): pid: %d, SIGINT handler: SIG_DFL\n", getpid ());

    else
      printf ("pfork (1): pid: %d, SIGINT handler: %lo\n", getpid (), 
              oact.sa_handler);
  }
#endif

	/* DSDe410511: when SIGUSR1 is ignored and csh exec'ed, csh hangs 
	 * Re-init SIGUSR1 here! 
	 */
	signal(SIGUSR1, psigusr1);

	/* block SIGUSR1 in both parent and child */
	sighold(SIGUSR1);

	while ((pid = fork()) < 0)
		if (setintr == 0)
			sleep(FORKSLEEP);
		else 
		{
			if (++forkcnt <5) /* try a few times */
			    sleep(1);
			else
			{
			    saveErr = errno;
			    sigrelse(SIGUSR1);
			    sigrelse(SIGINT);
			    sigrelse(SIGCLD);

			    /* we used to be sure that if fork failed then the
			     * errno was always set to "No more processes". -jh 
			     *
			     *  This is no longer the case.  There are 
			     *  currently 2 possible errors back from fork.
			     *  We saved errno just after the fork so it is
			     *  usable now when we need it.  If the errno
			     *  isn't one we recognize, then print a default
			     *  message along with the errno number.
			     */

			    if (saveErr == EAGAIN)
			      error((catgets(nlmsg_fd,NL_SETN,62, 
				     "fork failed: No more processes")));

			    else if (saveErr == ENOMEM)
			      error((catgets(nlmsg_fd,NL_SETN,63, 
				     "fork failed: No more memory")));

			    else
			      {
				(void) sprintf (forkErr, 
					(catgets (nlmsg_fd, NL_SETN, 64, 
					"fork failed: Unknown error: %d")), 
					saveErr);
				error (forkErr);
			      }
			}
		}
	if (pid == 0) {		/* child */

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "pfork (1):  pid: %d, Process group id just after fork: %d\n",
	       getpid (), getpgrp ());
      fflush (childDebugStream);
      fprintf (childDebugStream,
	       "pfork (2): pid: %d, tpgrp: %d, t->t_dflg: %o, FINT: %o\n", 
	       getpid (), tpgrp, t->t_dflg, FINT);
      fflush (childDebugStream);
    }
#endif

#ifdef SPIN_CHILD_DEBUG
  {
    int dodah = 0;
    while (dodah == 0)
      ;
  }
#endif

#ifdef ENV_DEBUG
  {
    extern char **environ;
    char **debugEnviron;

    debugEnviron = environ;

    if (childDebugStream != 0)
      {
        fprintf (childDebugStream,
		 "pfork (3): pid: %d, Environment just after pfork\n");
        fflush (childDebugStream);

        while (*debugEnviron != (char *) 0)
          {
	    fprintf (childDebugStream, "\t%s\n", *debugEnviron);
	    fflush (childDebugStream);
	    debugEnviron ++;
          }
      }
  }
#endif
		settimes();

		/* pflushall(); */	/* Fix the "jobs | cat" problem.  -jh */
		pcurrjob = PNULL;

		timesdone = 0;

		child++;

		if (setintr) {
			setintr = 0;		/* until I think otherwise */

			sigrelse(SIGCLD);

			/*
			 * Children just get blown away on SIGINT, SIGQUIT
			 * unless "onintr -" seen.
			 */
			sigset(SIGINT, ignint ? SIG_IGN : SIG_DFL);
			sigset(SIGQUIT, ignint ? SIG_IGN : SIG_DFL);

			if (wanttty >= 0) {
#ifdef SIGTSTP
				/* make stoppable */
				sigset(SIGTSTP, SIG_DFL);
				sigset(SIGTTIN, SIG_DFL);
				sigset(SIGTTOU, SIG_DFL);

#endif
			}
			sigset(SIGTERM, parterm);

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	  "pfork (4): pid: %d, Potentially changing signals:\n", getpid ());
      fflush (childDebugStream);
      fprintf (childDebugStream,
        "\t\tsetintr: %d, wanttty: %d, ignint: %d\n", setintr, wanttty, ignint);
      fflush (childDebugStream);
      fprintf (childDebugStream,
	   "\t\tignint = 0 =>SIG_DFL, otherwise SIG_IGN for SIGINT, SIGQUIT\n");
      fflush (childDebugStream);
    }
#endif

		} else if (tpgrp == -1 && (t->t_dflg&FINT)) {
			sigset(SIGINT, SIG_IGN);
			sigset(SIGQUIT, SIG_IGN);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "pfork (5): pid: %d, tpgrp: %d\n", getpid (), tpgrp);
      fflush (childDebugStream);
      fprintf (childDebugStream,
	      "\t\tINT flag: %d: -1/1 => ignore INT, QUIT\n", (t->t_dflg&FINT));
      fflush (childDebugStream);
    }
#endif

		}

		/* 
		   We need to restore the child's signal handler for SIGINT.  
		   Since it's currently set to SIG_IGN, if we don't reset
		   it, our children will also ignore SIGINT.
		 */
		else sigrelse(SIGINT);

		/* wait for parent to give us OK to proceed */
		/* signal(SIGUSR1, psigusr1); -- moved this before fork */
		sigpause(sigblock(0) & (~(1L << (SIGUSR1 - 1))));
		sigrelse(SIGUSR1);

		if (tpgrp > 0)
			tpgrp = 0;		/* gave tty away */

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "pfork (7): pid: %d, tpgrp: %d\n", getpid (), tpgrp);
      fflush (childDebugStream);
    }
#endif
		/*
		 * Nohup and nice apply only to TCOM's but it would be
		 * nice (?!?) if you could say "nohup (foo;bar)"
		 * Then the parser would have to know about nice/nohup/time
		 */

#ifdef SIGTSTP
		/* With job control, background jobs are in a different
		 * process goup and don't get SOGHUP from the terminating 
		 * shell. This is how 4.2 does it.
		 */
		 if (t->t_dflg & FNOHUP)
#else
		/* Without job control, we simulate 4.2 behavior by
		 * ignoring SIGHUP on backgound processes.
		 */
		if (t->t_dflg & (FNOHUP|FAND)) /* cfi */
#endif
			sigset(SIGHUP, SIG_IGN);
		if (t->t_dflg & FNICE) {
/* sigh...
			nice(20);
			nice(-10);
*/
			nice(t->t_nice);
		}

#ifdef TRACE_DEBUG
/*  Debug:  Figure out what the terminal process group and process group are.
*/
  {
    int traceDebugPid;

    if (childDebugStream != 0)
      {
        fprintf (childDebugStream,
		 "pfork (8): pid: %d, from tcgetpgrp: tty group = %d\n", 
		 getpid (), tcgetpgrp (0));
	fflush (childDebugStream);
      }
  }
#endif

	} else {		/* parent */
		sigrelse(SIGUSR1);
		pgrp = pcurrjob ? pcurrjob->p_jobid : pid;

#ifdef TRACE_DEBUG
  printf ("pfork (8.2): pid: %d new pgrp is %d, wanttty: %d, tpgrp:%d\n", 
	  getpid (), pgrp, wanttty, tpgrp);
  fflush (stdout);
#endif
		if (wanttty >= 0 && tpgrp >= 0) {
			if (setpgrp(pid, pgrp) == -1) {

				int Ctpgrp;
				int oldPgrp = pgrp;
		                
				pgrp = pid;

#ifdef TRACE_DEBUG
  printf ("pfork (8.4): pid: %d setpgrp () failed; setting %d's pgrp to %d\n", 
	  getpid (), pid, pgrp);
  fflush (stdout);
#endif
				setpgrp(pid, pgrp);
				pcurrjob->p_jobid = pgrp;

				if ((ioctl (FSHTTY, TIOCGPGRP, &Ctpgrp)) != -1)
				  {
				    if (Ctpgrp == oldPgrp)
				      {
#ifdef TRACE_DEBUG
  printf ("pfork (8.5): pid: %d ttypgrp old currjob (%d); resetting to %d\n", 
	  getpid (), oldPgrp, pgrp);
  fflush (stdout);
#endif
			                ioctl (FSHTTY, TIOCSPGRP, &pgrp);
				      }
				  }
			        }
			/* setpgrp(0, pgrp); */
		}
#ifdef SIGTSTP
		if (wanttty > 0)
		  {
#ifdef TRACE_DEBUG
  printf ("pfork (8.6): pid: %d setting ttygrp to %d\n", getpid (), pgrp);
  fflush (stdout);
#endif
			ioctl(FSHTTY, TIOCSPGRP, &pgrp);
		  }
                
#ifdef TRACE_DEBUG
		else {
  printf ("pfork (8.7): pid: %d, didn't want a tty\n", getpid ());
  fflush (stdout);
  }
#endif

#endif

	/* inform child that is now safe to proceed */
		kill(pid, SIGUSR1);

	/* DSDe412996: To prevent the parent from dying and leaving a zombie
	 * child if an interrupt is received we don't release interrupts here.
	 * the call to "sigrelse(SIGINT)" is done in pjwait().
	 */
		palloc(pid, t);
		sigrelse(SIGCLD);

	}

	return (pid);
}

/**********************************************************************/
okpcntl()
/**********************************************************************/
{

	if (tpgrp == -1)
		error((catgets(nlmsg_fd,NL_SETN,65, 
					     "No job control in this shell")));
	if (tpgrp == 0)
		error((catgets(nlmsg_fd,NL_SETN,66, 
					      "No job control in subshells")));
}

#ifdef DEBUG_PROC
/*  Called by:
	pchild ()
	palloc ()
*/
/*  Purpose:  Print the contents of the process table.  For each entry
	      in the table, all processes in the 'job' are printed
	      together.  They will get printed as many times as there are
	      processes in the job since the main loop is through the
	      process table.
*/
/**********************************************************************/
dumpProcTable ()
/**********************************************************************/
{
  struct process *debugProc, *debugJob;

  debugProc = proclist.p_next;

  while (debugProc != NULL)
    {
      printf ("\nTable: Addr: %lu, pid: %hd, leader: %hu, flags: %hu, ",
	       (unsigned) debugProc, debugProc -> p_pid, debugProc -> p_leader,
	       debugProc -> p_flags);
      printf ("reason: %hu, jobid: %d\n\tjob index: %hd\n", 
	       debugProc -> p_reason, debugProc -> p_jobid, 
	       debugProc -> p_index);

      debugJob = debugProc -> p_friends;

      if (debugJob == debugProc)
	printf ("No friends in the job.\n");

      else
	{
          while (debugJob != debugProc)
	    {
              printf ("\tJob: Addr: %lu, pid: %hd, leader: %hu, flags: %hu, ",
	               (unsigned) debugJob, debugJob -> p_pid, 
		       debugJob -> p_leader, debugJob -> p_flags);
              printf ("reason: %hu, jobid: %d\n\tjob index: %hd\n", 
	               debugProc -> p_reason, debugProc -> p_jobid, 
	               debugProc -> p_index);
	      
	      debugJob = debugJob -> p_friends;
	    }
	}
      
      debugProc = debugProc -> p_next;
    }
}
#endif
