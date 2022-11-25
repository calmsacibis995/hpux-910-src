/* @(#) $Revision: 72.1 $ */    
/**********************************************************************
   Print error string s with optional argument arg.
   This routine always resets or exits.  The flag haderr
   is set so the routine who catches the unwind can propogate
   it if they want.
  
   Note that any open files at the point of error will eventually
   be closed in the routine process in sh.c which is the only
   place error unwinds are ever caught.
 **********************************************************************/

#include "sh.h"
#include <sys/ioctl.h>

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 17	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

bool	errspl;			/* Argument to error was spliced by seterr2 */
CHAR	one[2] = { '1', 0 };
CHAR	*onev[2] = { one, NOSTR };
/*  Called by just about every routine.
*/
/*  Purpose:  Set some global error indicators then either call exit or
	      unwind via a longjmp.
*/
/*  This routine can set globals that may be used by the parent.  This used
    to work when vfork had Berkeley semantics, but no longer works for 8.0.
    The upshot is that some of this information needs to be passed back in
    the exit value, and some is lost.

    Globals of interest:  haderr
			  doneinp
			  err     (actually set from parent - seterr, seterr2)
			  errspl  (actually set from parent - seterr, seterr2)
			  child   (not set from error)
    
    Interestingly, although haderr might be set here in the 7.0 version, for
    some reason it was always 0 when it was printed in pchild.  So this global
    won't be encoded in the exit value.

    The global pointer err and the variable errspl are both still valid in 
    the parent.  So these don't need to be encoded.  Rather, the child signal
    handler needs to free err if it is a valid pointer and errspl is set.  It
    should then clear errspl.

    The other globals need to be passed back in the exit value.
*/
/**********************************************************************/
error(s, arg)
	char *s;
/**********************************************************************/
{
	register CHAR **v;
	register char *ep;

	/*
	 * Must flush before we print as we wish output before the error
	 * to go on (some form of) standard output, while output after
	 * goes on (some form of) diagnostic output.
	 * If didfds then output will go to 1/2 else to FSHOUT/FSHDIAG.
	 * See flush in sh.print.c.
	 */
	flush();

/*  This will cause flush () (called from putchar () via printf ()) to print
    to 2 or SHDIAG instead of 1 or SHOUT.
*/
	haderr = 1;		/* Now to diagnostic output */

/*  Doesn't appear to be used anywhere!
*/
	timflg = 0;		/* This isn't otherwise reset */

/*  Free pargv and gargv if they are set.
*/
	if (v = pargv)
		pargv = 0, blkfree(v);
	if (v = gargv)
		gargv = 0, blkfree(v);

	/*
	 * A zero arguments causes no printing, else print
	 * an error diagnostic here.
	 */
	if (s)
		printf(s, arg), printf(catgets(nlmsg_fd,NL_SETN,1,".\n"));

/*  This doesn't need to be passed back since the only place it matters is
    in Perror which calls this routine again.  If this routine is called then
    didfds is reset before the longjmp.  If we were just going to exit this
    wouldn't matter at all.
*/
        didfds = 0;		/* Forget about 0,1,2 */

/*  This needs to be done in the parent as well as the child, since it was
    allocated in the parent.  So in the parent, err is still around as a
    valid pointer.  For that matter, errspl is still a valid global.  So all
    that needs to happen is for the child signal handler to check these and
    free the memory and clear errspl.
*/
	if ((ep = err) && errspl) {
		errspl = 0;
		xfree((CHAR *)ep);
	}
	errspl = 0;

	/*
	 * Reset the state of the input.
	 * This buffered seek to end of file will also
	 * clear the while/foreach stack.
	 */
	/* Fix for FSDlj08532 and for DSDe409077. if condition added */
	if (!(exiterr || child))
		btoeof();

	/*
	 * Go away if -e or we are a child shell
	 */

/*  If the command was exec or the program was invoked with a -e option then
    exiterr will be set.  If this is the result of a fork then child will
    be set.  If this is a shell script then intty is not set.  One upshot of
    this is that shell scripts exit on an error.  This only occurs if doneinp
    is set, so this value needs to be passed back to the parent via a
    signal provided that this child would have been the result of a vfork if
    it was being used.

    The fact that this is a child that would have been the result of a vfork
    and that it is done with input is related back to the parent by
    sending it a SIGUSR2 signal.
*/

#ifdef DEBUG_EXIT
  printf ("error (1): %d, exiterr: %d, child: %d\n", getpid (), exiterr,
	   child);
#endif

	if (exiterr || child) {
		if (!intty)
		  {
			doneinp = 1;	/* don't want to read any more input */

/*  Signal the parent that doneinp needs to be set provided
    that this child is the result of a vfork (if vfork was available).

    The signal is only set if the environment variable EXITONERR
    is defined and set to 1
*/
		        if (childVfork) {
		          char *envp;
		          if (((envp = getenv("EXITONERR")) != NULL) &&
			   	strcmp(envp, "1") == 0)
		      	    kill(getppid(), SIGUSR2);
		        }
		  }
/*  Instead of just 'exit (1)', add the encodeing of doneinp/vfork child to the
    exit 1.
*/

#ifdef DEBUG_EXIT
  printf ("error (2): pid: %d\n", getpid ());
#endif
		exit(1);
	}

/*  Modify the shell variable list to set the value of CH_status to '1'.  This
    would occur if no fork had happened and -e was not an option to the shell.
*/
	setq(CH_status, onev, &shvhed);

#ifdef SIGTSTP
	if (tpgrp > 0)
          {

#ifdef DEBUG_EXIT
  printf ("error (3): pid: %d, getting back the tty\n", getpid ());
#endif
		ioctl(FSHTTY, TIOCSPGRP, &tpgrp);
          }
#endif

/*  Perform a longjmp.  This restores a previous stack, but keeps all the
    globals with their present values.
*/

#ifdef DEBUG_EXIT
  printf ("error (4): pid: %d, doing a longjmp\n", getpid ());
#endif
	reset();		/* Unwind */
}

/*
 * Perror is the shells version of perror which should otherwise
 * never be called.
 */
/*  Called by several routines.
*/
/**********************************************************************/
Perror(s)
	char *s;
/**********************************************************************/
{

	/*
	 * Perror uses unit 2, thus if we didn't set up the fd's
	 * we must set up unit 2 now else the diagnostic will disappear
	 */
	if (!didfds) {
		register int oerrno = errno;

/*  This routine ends up doing a dup after going through other routines.
*/
		(void) dcopy(SHDIAG, 2);
		errno = oerrno;
	}
	perror(s);
	error((char *) 0);		/* To exit or unwind */
}

/*  Called by lot of routines.
*/
/**********************************************************************/
bferr(cp)
	char *cp;
/**********************************************************************/
{

#ifdef DEBUG_EXIT
  printf ("bferr (1): pid: %d, cp: %s, child: %hd, exiterr: %hd\n", 
	  getpid (), cp, (child & 0377), (exiterr & 0377));
#endif

/*  Flushes stdout or stderr depending on the current value of haderr.
    Then prints the command name stored in the global bname.  Since printf
    calls putchar that calls flush, this gets printed to stderr.  Then 
    error is called with the error message.
*/
	flush();
	haderr = 1;
	printf("%s: ", bname);
	error(cp);
}

/*
 * The parser and scanner set up errors for later by calling seterr,
 * which sets the variable err as a side effect; later to be tested,
 * e.g. in process.
 */
/*  Called by several routines.
*/
/**********************************************************************/
seterr(s)
	char *s;
/**********************************************************************/
{

	if (err == 0)
#ifdef NLS
		/* message need to be saved */
		err = savebyte(s), errspl = 1;
#else
		err = s, errspl = 0;
#endif
}

/* Set err to a splice of cp and dp, to be freed later in error() */
/*  Called by:
	seterrc ()
	noev ()
*/
/**********************************************************************/
seterr2(cp, dp)
	char *cp, *dp;
/**********************************************************************/
{

	if (err)
		return;
	err = strspl(cp, dp);
	errspl++;
}

/* Set err to a splice of cp with a string form of character d */
/*  Called by:
	work ()
	getsub ()
*/
/**********************************************************************/
seterrc(cp, d)
	char *cp, d;
/**********************************************************************/
{
	char chbuf[2];

	chbuf[0] = d;
	chbuf[1] = 0;
	seterr2(cp, chbuf);
}
