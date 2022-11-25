/* @(#) $Revision: 66.10 $ */    

/************************************************************
 * C shell
 ************************************************************/

#include "sh.h"
#include "sh.proc.h"
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>

#if defined (DEBUG_CHILD) || defined (TRACE_DEBUG) || defined (ENV_DEBUG) || defined (DEBUG_DONEINP)  || defined (SIGNAL_DEBUG) || defined (VFORK_DEBUG) || defined (HASH_DEBUG)
/*  We need a file to print debug statements to from a child process; defined
    in sh.proc.c.
*/
#include <stdio.h>
extern FILE *childDebugStream;
#endif

#ifdef TRACE_DEBUG
#include <sys/pstat.h>
#endif

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 14	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

void sigH ();

/*VARARGS 1*/
/*  Called by:
	reexecute ()
	process ()
	evalav ()
	backeval ()
	execute ()
*/
/**********************************************************************/
execute(t, wanttty, pipein, pipeout)
	register struct command *t;
	int wanttty, *pipein, *pipeout;
/**********************************************************************/
{
	bool forked = 0;
	struct biltins *bifunc;
	int pid = 0;
	int pv[2];
	CHAR *tmp[3];
        long procGrp;


#ifdef TRACE_DEBUG
  printf ("execute (1): pid: %d, wanttty: %d, t->t_dtyp: %o\n", getpid (), 
	  wanttty, t->t_dtyp);
  printf ("\t\tt->t_dflg: %o\n", t->t_dflg);
  printf ("\t\tTCOM: %o, TPAR: %o, TFIL: %o, TLST: %o, TOR: %o, TAND: %o\n",
	  TCOM, TPAR, TFIL, TLST, TOR, TAND);
  printf ("\t\tFAND: %o, FNICE: %o, FNOHUP: %o, FTIME: %o, FPAR: %o\n",
	  FAND, FNICE, FNOHUP, FTIME, FPAR);
  printf ("\t\tFPOU: %o, FREDO: %o, FPIN: %o, FINT: %o, FDIAG: %o\n", FPOU, 
	  FREDO, FPIN, FINT, FDIAG);
#endif

#ifdef DEBUG_TREE
  printCmd (t);
#endif

	if (t == 0)
		return;
	if ((t->t_dflg & FAND) && wanttty > 0)
		wanttty = 0;
	switch (t->t_dtyp) {


	case TCOM:
#ifdef TRACE_DEBUG
  printf ("execute (2): pid: %d, Switch was a TCOM\n", getpid ());
#endif

		if ((t->t_dcom[0][0] & (QUOTE|TRIM)) == QUOTE)
			Strcpy(t->t_dcom[0], t->t_dcom[0] + 1);
		if ((t->t_dflg & FREDO) == 0)
			Dfix(t);		/* $ " ' \ */
		if (t->t_dcom[0] == 0)
			return;
		/* fall into... */

	case TPAR:
#ifdef TRACE_DEBUG
  printf ("execute (3): pid: %d, Switch was a TPAR or TCOM fell into TPAR\n",
	   getpid ());
#endif

		if (t->t_dflg & FPOU)
			mypipe(pipeout);
		/*
		 * Must do << early so parent will know
		 * where input pointer should be.
		 * If noexec then this is all we do.
		 */
		if (t->t_dflg & FHERE) {
			close(0);
			heredoc(t->t_dlef);
			if (noexec)
				close(0);
		}
		if (noexec)
			break;

		/* Don't reset status if we are executing the exit */
		/* command, so that exit with no args will return  */
		/* the last value of the status variable.  jsm.    */

		/* Don't reset status if we are executing the '@'  */
		/* command and the 'set' command either! so that   */
		/* both commands will return a correct value for   */
		/* the status variable.  jh.                       */

		if (t->t_dtyp != TCOM || (!eq(t->t_dcom[0], "exit") 
		    && !eq(t->t_dcom[0], "@") && !eq(t->t_dcom[0], "set")))
			set(CH_status, CH_zero);

		/*
		 * This mess is the necessary kludge to handle the prefix
		 * builtins: nice, nohup, time.  These commands can also
		 * be used by themselves, and this is not handled here.
		 * This will also work when loops are parsed.
		 */
		while (t->t_dtyp == TCOM)
			if (eq(t->t_dcom[0], "nice"))
				if (t->t_dcom[1])
					if (any(t->t_dcom[1][0], "+-"))
						if (t->t_dcom[2]) {
							setname("nice");
							t->t_nice = getn(t->t_dcom[1]);
							lshift(t->t_dcom, 2);
							t->t_dflg |= FNICE;
						} else
							break;
					else {
						t->t_nice = 4;
						lshift(t->t_dcom, 1);
						t->t_dflg |= FNICE;
					}
				else
					break;
			else if (eq(t->t_dcom[0], "nohup"))
				if (t->t_dcom[1]) {
					t->t_dflg |= FNOHUP;
					lshift(t->t_dcom, 1);
				} else
					break;
			else if (eq(t->t_dcom[0], "time"))
				if (t->t_dcom[1]) {
					t->t_dflg |= FTIME;
					lshift(t->t_dcom, 1);
				} else
					break;
			else
				break;
		/*
		 * Check if we have a builtin function and remember which one.
		 */
		bifunc = t->t_dtyp == TCOM ? isbfunc(t) : (struct biltins *) 0;
		tmp[0] = t->L.T_dlef; tmp[1] = t->R.T_drit; tmp[2] = 0;
#ifdef TRACE_DEBUG
  printf ("execute (4): pid: %d, bifunc: %X\n", getpid (), (long) bifunc);
#endif

		/*
		 * We fork only if we are timed, or are not the end of
		 * a parenthesized list and not a simple builtin function.
		 * Simple meaning one that is not pipedout, niced, nohupped,
		 * or &'d.
		 * It would be nice(?) to not fork in some of these cases.
		 */

/*  If VFORK is turned off, all forks go through pfork () which does a regular
    fork.  If VFORK is turned on, and it is a regular command, a vfork ()
    occurs.
*/
		if (((t->t_dflg & FTIME) || (t->t_dflg & FPAR) == 0 &&
		     (!bifunc || t->t_dflg & (FPOU|FAND|FNICE|FNOHUP))))
#ifdef VFORK
		    if (t->t_dtyp == TPAR || t->t_dflg&(FREDO|FAND)
		       || bifunc || (t->t_dtyp == TCOM && hasback(t->t_dcom))
		       || hasback(tmp) || hasback(&tmp[1]))
#endif
			{ 
/*  This routine will figure out if the child command is in the hash table or 
    not.  It will adjust the globals 'hits' and 'misses' accordingly.  However,
    the check will be on whether or not the command search was successful, not 
    on whether or not the actual 'exec' would work or not.  This is different 
    from previous versions of csh, where this check went on in the child process
    and the globals in the parent data space were updated, finally based on 
    whether or not the 'exec' of the command was successful.  Since the initial
    loading of the hash table is built on whether or not a command name can be 
    read in a directory, the new action seems reasonable.
*/
			  updateHashStats (t);

			  forked++; pid = pfork(t, wanttty);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream, 
	    "execute (5): pid: %d, Process group id just after pfork (): %d\n", 
	    getpid (), getpgrp ());
      fflush (childDebugStream);
    }
#endif

/*  Next figure out if the child would have been created using vfork if it
    was available.  If so, set the global childVfork.  This is used to figure
    out if certain global variables set in the child should have an impact on
    the parent after the child terminates. Always set the parent version to 0.
*/
			  if (pid != 0)
			    childVfork = 0;

		          else
			    {
			      if (t->t_dtyp == TPAR || t->t_dflg&(FREDO|FAND)
			         || bifunc 
			         || (t->t_dtyp == TCOM && hasback(t->t_dcom))
			         || hasback(tmp) || hasback(&tmp[1]))
			        childVfork = 0;
			  
			      else
			        childVfork = 1;
			    }
#ifdef DEBUG_DONEINP
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "execute (6): pid: %d, childVfork: %d\n", getpid (), childVfork);
      fflush (childDebugStream);
      fprintf (childDebugStream,
	       "\t\ttmp: %s\n", to_char (tmp));
      fflush (childDebugStream);
      fprintf (childDebugStream,
	       "\t\ttmp[1]: %s\n", to_char (tmp[1]));
      fflush (childDebugStream);
      fprintf (childDebugStream,
	       "\t\tt->t_dcom: %s\n", to_char (*t->t_dcom));
      fflush (childDebugStream);
    }
#endif
			}
#ifdef VFORK
		    else {
			void vffree();
			int ochild, osetintr, ohaderr, odidfds, odidcch;
			int oSHIN, oSHOUT, oSHDIAG, oOLDSTD, otpgrp;

			sighold(SIGCLD);
			ochild = child; osetintr = setintr;
			ohaderr = haderr; odidfds = didfds; odidcch = didcch;
			oSHIN = SHIN; oSHOUT = SHOUT;
			oSHDIAG = SHDIAG; oOLDSTD = OLDSTD; otpgrp = tpgrp;
			Vsav = Vdp = 0; Vav = 0;
			Vcmd = 0; Varg = 0;
			int saveErr;
			char forkErr [40];

			pid = vfork();
			if (pid < 0) {
				saveErr = errno;
				sigrelse(SIGCLD);

				if (saveErr == EAGAIN)
				  error((catgets(nlmsg_fd,NL_SETN,1, 
					   "vfork failed: No more processes")));
				
				else if (saveErr == ENOMEM)
				  error((catgets(nlmsg_fd,NL_SETN,2,
					      "vfork failed: No more memory")));

                                else
                                  {
                                    (void) sprintf (forkErr,
                                            (catgets (nlmsg_fd, NL_SETN, 3,
                                            "vfork failed: Unknown error: %d")),
                                            saveErr);
                                    error (forkErr);
                                  }
			}
			forked++;
			if (pid) {	/* parent */
				child = ochild; setintr = osetintr;
				haderr = ohaderr; didfds = odidfds;
				didcch = odidcch; SHIN = oSHIN;
				SHOUT = oSHOUT; SHDIAG = oSHDIAG;
				OLDSTD = oOLDSTD; tpgrp = otpgrp;
				xfree(Vsav); Vsav = 0;
				xfree(Vdp); Vdp = 0;
				xfree(Vav); Vav = 0;
				xfree(Vcmd); Vcmd = 0;
				if (Varg) {
					blkfree(Varg); 
					Varg = 0;
				}
				/* this is from pfork() */
				/* palloc(pid, t); */
				sigrelse(SIGINT);
				sigrelse(SIGCLD);
			} else {	/* child */
				/* this is from pfork() */
				int pgrp;
				bool ignint = 0;

				if (setintr)
					ignint =
					    (tpgrp == -1 && (t->t_dflg&FINT))
					    || gointr && eq(gointr, "-");
				child++;
				if (setintr) {
					setintr = 0;
					sigsys(SIGCLD, SIG_DFL);
					sigsys(SIGINT, ignint ? SIG_IGN : vffree);
					sigsys(SIGQUIT, ignint ? SIG_IGN : SIG_DFL);

#ifdef SIGTSTP
					if (wanttty >= 0) {
						sigsys(SIGTSTP, SIG_DFL);
						sigsys(SIGTTIN, SIG_DFL);
						sigsys(SIGTTOU, SIG_DFL);
					}
#endif SIGTSTP
					sigsys(SIGTERM, parterm);
				} else if (tpgrp == -1 && (t->t_dflg&FINT)) {
					sigsys(SIGINT, SIG_IGN);
					sigsys(SIGQUIT, SIG_IGN);

				}

/*  If currjob is not 0, then pgrp is its process group.  Otherwise, the
    process group is the new process id.  This is how most new jobs get to
    be in their own process group.
*/
				pgrp = pcurrjob ? pcurrjob->p_jobid : getpid();
				if (wanttty >= 0 && tpgrp >= 0) {
                                        procGrp = setpgrp (0, pgrp);
					if (procGrp == -1) {
						pgrp = getpid();
						setpgrp(0, pgrp);
						pcurrjob->p_jobid = pgrp;
						if (!(t->t_dflg & FAND))
							wanttty = 1;
					}
					/* setpgrp(0, pgrp); */
				}
#ifdef SIGTSTP
				if (wanttty > 0)
					ioctl(FSHTTY, TIOCSPGRP, &pgrp);
#endif SIGTSTP
				if (tpgrp > 0)
					tpgrp = 0;
				if (t->t_dflg & FNOHUP)
					sigsys(SIGHUP, SIG_IGN);
				if (t->t_dflg & FNICE)
					nice(t->t_nice);

				palloc(getpid(), t);
			}
		}
#endif  /* VFORK */

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	    "execute (7): real pid: %d, variable 'pid': %d\n", getpid (), pid);
      fflush (childDebugStream);
    }
#endif

/*  If the shell didn't fork, pid is still 0 from its initialization at the
    beginning of this routine and this code does not get executed.
*/
		if (pid != 0) {		/* parent */
			/*
			 * It would be better if we could wait for the
			 * whole job when we knew the last process
			 * had been started.  Pwait, in fact, does
			 * wait for the whole job anyway, but this test
			 * doesn't really express our intentions.
			 */
			if (didfds==0 && t->t_dflg&FPIN)
				close(pipein[0]), close(pipein[1]);
			if ((t->t_dflg & (FPOU|FAND)) == 0)
			  {
#ifdef TRACE_DEBUG
      printf ("execute (8): pid: %d, calling pwait()\n", getpid ());
      fflush (stdout);
#endif
				pwait();
			  }

/*  Break out of the switch statement.  So all the rest of this code for this
    case is only executed by the child if a for occurred, or by the parent
    if no fork occurred.
*/
			break;
		}

		if (setintr)
			sigrelse(SIGINT);
		doio(t, pipein, pipeout);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "execute (9): pid: %d, Returned from doio\n", getpid ());
      fflush (childDebugStream);
    }
#endif
		if (setintr)
			sighold(SIGINT);
		if (t->t_dflg & FPOU)
                  {
			close(pipeout[0]), close(pipeout[1]);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	    "execute (10): pid: %d, Closed pipeout [0-1]: %d, %d\n", getpid (),
            pipeout [0], pipeout [1]);
      fflush (childDebugStream);
    }
#endif
                  }

		/*
		 * Perform a builtin function.
		 * If we are not forked, arrange for possible stopping
		 */
		if (bifunc) {
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "execute (11): pid: %d, Calling func: %s\n", getpid (),
	       bifunc -> bname);
      fflush (childDebugStream);
    }
#endif

			func(t, bifunc);
			if (forked)
				exitstat();
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "execute (12): pid: %d, Breaking case statement\n", getpid ());
      fflush (childDebugStream);
    }
#endif

/*  Break out of the switch statement if this was a built-in.
*/
			break;
		}
		if (t->t_dtyp != TPAR) {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "execute (13): pid: %d, Calling doexec\n", getpid ());
      fflush (childDebugStream);
    }
#endif
			doexec(t);
			/*NOTREACHED*/
		}
		/*
		 * For () commands must put new 0,1,2 in FSH* and recurse
		 */
/*  If it was a TPAR, it didn't go through VFORK anyway, it went through pfork.
*/
		OLDSTD = dcopy(0, FOLDSTD);
		SHOUT = dcopy(1, FSHOUT);
		SHDIAG = dcopy(2, FSHDIAG);

#ifdef TRACE_DEBUG
  printf ("execute (14): pid: %d, Changed OLDSTD: %d, SHOUT: %d, SHDIAG: %d\n",
          getpid (), OLDSTD, SHOUT, SHDIAG);
#endif

		close(SHIN), SHIN = -1;
		didcch = 0, didfds = 0;
		wanttty = -1;
		t->t_dspr->t_dflg |= t->t_dflg & FINT;

#ifdef TRACE_DEBUG
  printf ("execute (15): pid: %d, Calling execute again\n", getpid ());
#endif

		execute(t->t_dspr, wanttty);
		exitstat();

	case TFIL:

#ifdef TRACE_DEBUG
  printf ("execute (16): pid: %d, Switch was a TFIL\n", getpid ());
#endif

		t->t_dcar->t_dflg |= FPOU |
		    (t->t_dflg & (FPIN|FAND|FDIAG|FINT));
		execute(t->t_dcar, wanttty, pipein, pv);
		t->t_dcdr->t_dflg |= FPIN |
		    (t->t_dflg & (FPOU|FAND|FPAR|FINT));
		if (wanttty > 0)
			wanttty = 0;		/* got tty already */
		execute(t->t_dcdr, wanttty, pv, pipeout);
		break;

	case TLST:

#ifdef TRACE_DEBUG
  printf ("execute (17): pid: %d, Switch was a TLST\n", getpid ());
#endif

		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			/*
			 * In strange case of A&B make a new job after A
			 */
			if (t->t_dcar->t_dflg&FAND && t->t_dcdr &&
			    (t->t_dcdr->t_dflg&FAND) == 0)
				pendjob();
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;

	case TOR:
	case TAND:

#ifdef TRACE_DEBUG
  printf ("execute (18): pid: %d, Switch was a TOR or TAND\n", getpid ());
#endif

		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			if ((getn(value(CH_status)) == 0) != (t->t_dtyp == TAND))
				return;
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;
	}
	/*
	 * Fall through for all breaks from switch
	 *
	 * If there will be no more executions of this
	 * command, flush all file descriptors.
	 * Places that turn on the FREDO bit are responsible
	 * for doing donefds after the last re-execution
	 */
	if (didfds && !(t->t_dflg & FREDO))
		donefds();
}

#ifdef VFORK
/**********************************************************************/
void vffree()
/**********************************************************************/
{
	register CHAR **v;

#ifdef TRACE_DEBUG
  printf ("vffree (1): pid: %d, gargv: %X, pargv: %X\n", getpid (), 
	  (long) gargv, (long) pargv);
#endif

/*  The routine xfree checks arguments to be sure they are in range, then calls
    cfree which does a free on each character.  

    It seems like the call to xfree should be done before the address gets
    reset to 0.
*/
	if (v = gargv)
		gargv = 0, xfree(gargv);
	if (v = pargv)
		pargv = 0, xfree(pargv);
	_exit(1);
}
#endif

/*
 * Perform io redirection.
 * We may or maynot be forked here.
 */
/**********************************************************************/
doio(t, pipein, pipeout)
	register struct command *t;
	int *pipein, *pipeout;
/**********************************************************************/
{
	register CHAR *cp;
	register int flags = t->t_dflg;

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doio (1): pid: %d, In doio()\n", getpid ());
      fflush (childDebugStream);
      fprintf (childDebugStream,
	       "\tdidfds: %d, flags: %o, FREDO: %o, FHERE: %o\n", didfds, 
	       flags, FREDO, FHERE);
      fflush (childDebugStream);
      fprintf (childDebugStream,
	      "\tFPIN: %o, FINT: %o, FPAR: %o, FANY: %o, FCAT: %o, FDIAG: %o\n",
	      FPIN, FINT, FPAR, FANY, FCAT, FDIAG);
      fflush (childDebugStream);
    }
#endif

	if (didfds || (flags & FREDO))
		return;
	if ((flags & FHERE) == 0) {	/* FHERE already done */
		close(0);
		if (cp = t->t_dlef) {

/*  The pointer cp needs to be free eventually since globone calloc's space.
    However, so does Dfix1, and a routine that it calls, Dfix2.  But Dfix1
    copies part of the space calloc'd by Dfix2 and then frees the space
    calloc'd by Dfix2.  Similarly globone copies the space returned by Dfix1
    and then frees the original Dfix1 space.  So all that needs to be freed
    is cp.

    Note that cp is freed before it is used.  This seems dangerous.
*/
			cp = globone(Dfix1(cp));
			xfree(cp);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	 "doio (2): pid: %d, File being opened: %s\n", getpid (), to_char (cp));
      fflush (childDebugStream);
    }
#endif

			if (open(to_char(cp), 0) < 0)
				Perror(to_char(cp));
		} else if (flags & FPIN)
			dup(pipein[0]), close(pipein[0]), close(pipein[1]);
		else if ((flags & FINT) && !(flags & FPAR) && tpgrp == -1)
			close(0), open("/dev/null", 0);
		else
                  {
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	      "doio (3): pid: %d, Dup OLDSTD: %x into 0.\n", getpid (), OLDSTD);
      fflush (childDebugStream);
    }
#endif

			dup(OLDSTD);
                  }
	}
	close(1);

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream, "doio (4): pid: %d, Closed 1\n", getpid ());
      fflush (childDebugStream);
    }
#endif

	if (cp = t->t_drit) {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
               "doio (5): pid: %d, cp = t -> t_drit\n", getpid ());
      fflush (childDebugStream);
    }
#endif

/*  The pointer cp needs to be free eventually since globone calloc's space.
    However, so does Dfix1, and a routine that it calls, Dfix2.  But Dfix1
    copies part of the space calloc'd by Dfix2 and then frees the space
    calloc'd by Dfix2.  Similarly globone copies the space returned by Dfix1
    and then frees the original Dfix1 space.  So all that needs to be freed
    is cp.

    Note that cp is freed before it is used.  This seems dangerous.
*/
		cp = globone(Dfix1(cp));

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doio (6): pid: %d, New cp: %s\n", getpid (), to_char (cp));
      fflush (childDebugStream);
    }
#endif

		xfree(cp);
		if ((flags & FCAT) && open(to_char(cp), 1) >= 0)
                  {
			lseek(1, 0l, 2);
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doio (7): pid: %d, Opened new stdout: %s\n", getpid (), 
	       to_char (cp));
      fflush (childDebugStream);
    }
#endif
                  }
		else {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	  "doio (8): pid: %d, flags not FCAT or file not opened\n", getpid ());
      fflush (childDebugStream);
    }
#endif

			if (!(flags & FANY) && adrof(CH_noclobber)) {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
        "doio (9): pid: %d, FANY not in flags && adrof(noclobber)\n",getpid ());
      fflush (childDebugStream);
    }
#endif

				if (flags & FCAT)
					Perror(to_char(cp));

/*  This routine does a stat and checks for the existance of a file.
*/
				chkclob(to_char(cp));
			}
			if (creat(to_char(cp), 0666) < 0)
                          {
#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
        "doio (10): pid: %d, Creat stdout fail: %s\n", getpid (), to_char (cp));
      fflush (childDebugStream);
    }
#endif
				Perror(to_char(cp));
                          }
		}
	} 
        else if (flags & FPOU)
          {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	   "doio (11): pid: %d, Dup pipeout [1]: %d\n", getpid (), pipeout [1]);
      fflush (childDebugStream);
    }
#endif

		dup(pipeout[1]);
          }

	else
          {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
               "doio (12): pid: %d, Dup SHOUT: %d\n", getpid (), SHOUT);
      fflush (childDebugStream);
    }
#endif

		dup(SHOUT);
          }

	close(2);
	dup((flags & FDIAG) ? 1 : SHDIAG);

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
         "doio (13): pid: %d, Dup 1 or SHDIAG into 2: %d\n", getpid (), SHDIAG);
      fflush (childDebugStream);
    }
#endif

	didfds = 1;
}

/**********************************************************************/
mypipe(pv)
	register int *pv;
/**********************************************************************/
{

	if (pipe(pv) < 0)
		goto oops;
	pv[0] = dmove(pv[0], -1);
	pv[1] = dmove(pv[1], -1);
	if (pv[0] >= 0 && pv[1] >= 0)
		return;
oops:
	error((catgets(nlmsg_fd,NL_SETN,4, "Can't make pipe")));
}

/**********************************************************************/
chkclob(cp)
	register char *cp;
/**********************************************************************/
{
	struct stat stb;

	if (stat(cp, &stb) < 0)
		return;
	if (((stb.st_mode & S_IFMT) == S_IFCHR) ||
	    ((stb.st_mode & S_IFMT) == S_IFIFO))
		return;
	error((catgets(nlmsg_fd,NL_SETN,5, "%s: File exists")), cp);
}

/* HPUX doesn't allow forks from a vforked child. Therefore we must */
/* check the command arguments for any backquotes. hasback returns  */
/* 1 (TRUE) if there is a non backslashed backquote in any of the   */
/* arguments. otherwise it returns 0 (FALSE).                       */

/**********************************************************************/
hasback(av)
	register CHAR **av;
/**********************************************************************/
{
	register int i;
	CHAR *c;
	CHAR backslash;

	backslash = 0;
	for (i = 0; av[i] != (CHAR *)0; i++) {
		c = av[i];
		while (*c != '\0' ) {
			if (*c == '`' && !backslash)
			  {
#ifdef DEBUG_BACKQUOTE
  printf ("hasback (1): pid: %d, Saw a backquote\n", getpid ());
#endif
				return(1);
			  }
			if (*c++ == '\\' && !backslash)
				backslash++;
			else
				backslash = 0;
		}
	}

#ifdef DEBUG_BACKQUOTE
  printf ("hasback (2): pid: %d, No backquotes\n", getpid ());
#endif
	return(0);
}

void sigH ()
{
  printf ("Got INT signal in csh\n");
}

#ifdef DEBUG_TREE

/**********************************************************************/
void printCmd (t)
  struct command *t;
/**********************************************************************/
{
  CHAR **commandPtr;

  commandPtr = t -> t_dcom;

  printf ("printCmd (1): %d\n", getpid());

  while (*commandPtr != NULL)
    {
      printf ("\tcommand: %s\n", to_char (*commandPtr));
      commandPtr ++;
    }

  printf ("\tleft: %lo, right: %lo, sub: %lo\n", t -> t_dcar, t -> t_dcdr,
	   t -> t_dspr);
  printf ("\tt_dtyp: %ho, t_dflg: %ho, t_nice: %ho\n", t -> t_dtyp,
	   t -> t_dflg, t -> t_nice);
}
#endif
