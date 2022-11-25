/* @(#) $Revision: 70.1 $ */   
/*********************************************************************
 * System level search and execute of a command.
 * We look in each directory for the specified command name.
 * If the name contains a '/' then we execute only the full path name.
 * If there is no search path then we execute only full path names.
 **********************************************************************/

#include "sh.h"

#if defined (DEBUG_CHILD) || defined (TRACE_DEBUG) || defined (ENV_DEBUG) || defined (NITTY_TRACE_DEBUG) || defined (DEBUG_DONEINP)  || defined (SIGNAL_DEBUG) || defined (VFORK_DEBUG) || defined (HASH_DEBUG)
/*  We need a file to print debug statements to from a child process; defined
    in sh.proc.c.
*/
#include <stdio.h>
extern FILE *childDebugStream;
#endif

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 6	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

/* 
 * As we search for the command we note the first non-trivial error
 * message for presentation to the user.  This allows us often
 * to show that a file has the wrong mode/no access when the file
 * is not in the last component of the search path, so we must
 * go on after first detecting the error.
 */
char	*exerr;			/* Execution error message */
CHAR	*expath;		/* Path for exerr */

/*
 * Xhash is an array of HSHSIZ chars, which are used to hash execs.
 * If it is allocated, then to tell whether ``name'' is (possibly)
 * present in the i'th component of the variable path, you look at
 * the i'th bit of xhash[hash("name")].  This is setup automatically
 * after .login is executed, and recomputed whenever ``path'' is
 * changed.
 */
int	havhash;
#define	HSHSIZ	511
CHAR	xhash[HSHSIZ];
int	hits, misses;

/* Dummy search path for just absolute search when no path */
CHAR	*justabs[] =	{ nullstr, 0 };

/*  Called by:

	execash ()
	execute ()
*/
/**********************************************************************/
doexec(t)
	register struct command *t;
/**********************************************************************/
{
	CHAR *sav;
	register CHAR *dp, **pv, **av;  /* av used to be **av */
	register struct varent *v;
	bool slash;
	int hashval, i;
	CHAR *blk[2];

#ifdef SIGNAL_DEBUG
  {
    int traceDebugIndex;
    struct sigaction oact;
    char *traceDebugIgn = "SIG_IGN";
    char *traceDebugDfl = "SIG_DFL";
    char *traceDebugOth = "other";
    char *traceDebugSigs [31];

    if (childDebugStream != 0)
      {
        fprintf ("childDebugStream,
		 doexec (1): pid: %d, signal handlers:\n", getpid ());
	fflush (childDebugStream);

        for (traceDebugIndex = 1; traceDebugIndex < 30; traceDebugIndex ++)
          {
	    sigaction (traceDebugIndex, 0, &oact);

	    if (oact.sa_handler == SIG_IGN)
	      traceDebugSigs [traceDebugIndex] = traceDebugIgn;

	    else if (oact.sa_handler == SIG_DFL)
	      traceDebugSigs [traceDebugIndex] = traceDebugDfl;

	    else 
	      traceDebugSigs [traceDebugIndex] = traceDebugOth;
          }
    
        for (traceDebugIndex = 1; traceDebugIndex < 30; traceDebugIndex ++)
          {
            fprintf (childDebugStream,
		"%3d %-7s    %3d %-7s    %3d %-7s    %3d %-7s    %3d %-7s\n", 
	        (traceDebugIndex), traceDebugSigs [traceDebugIndex],
	        (traceDebugIndex + 1), traceDebugSigs [traceDebugIndex + 1],
	        (traceDebugIndex + 2), traceDebugSigs [traceDebugIndex + 2],
	        (traceDebugIndex + 3), traceDebugSigs [traceDebugIndex + 3],
	        (traceDebugIndex + 4), traceDebugSigs [traceDebugIndex + 4]);
	    fflush (childDebugStream);
	    traceDebugIndex += 5;
          }
      }
  }
#endif
	/*
	 * Glob the command name.  If this does anything, then we
	 * will execute the command only relative to ".".  One special
	 * case: if there is no PATH, then we execute only commands
	 * which start with '/'.
	 */

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (2): pid: %d, Command to execute: %s\n", getpid (),
               to_char (t->t_dcom[0]));
      fflush (childDebugStream);
      if (t->t_dcom [1] != 0)
	{
          fprintf (childDebugStream, "\targ1: %s\n", to_char (t->t_dcom[1]));
          fflush (childDebugStream);
	}
    }
#endif

/*  After this call, dp points to calloc'd space to a single command name.
    So dp eventually needs to be freed.
*/
	dp = globone(t->t_dcom[0]);

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (3): pid: %d, dp value: %s\n", getpid (), to_char (dp));
      fflush (childDebugStream);
    }
#endif

/*  Slash is 0 or 1.
*/
	slash = Any('/', dp);

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	   "doexec (4): pid: %d, dp value after call to Any(): %s\n", getpid (),
	  to_char (dp));
      fflush (childDebugStream);
    }
#endif

/*  The original command name pointer is saved.  Then the command name pointer
    is reset to point to the globbed command name and the string originally
    pointed to is freed.  
*/
	sav = t->t_dcom[0];
	exerr = 0; expath = t->t_dcom[0] = dp;

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (5): pid: %d, dp value after reset: %s\n", getpid (),
	  to_char (dp));
      fflush (childDebugStream);
      fprintf (childDebugStream, "\texpath value: %s\n", to_char (expath));
      fflush (childDebugStream);
      fprintf (childDebugStream, "\tsav value: %s\n", to_char (sav));
      fflush (childDebugStream);
    }
#endif

	xfree(sav);

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	      "doexec (6): pid: %d, sav value after call to xfree: %s\n", 
	       getpid (), to_char (sav));
      fflush (childDebugStream);
    }
#endif

/*  Get the PATH variable.  If there isn't one and the first character of the
    command name isn't '/' then exit with an error.
*/
	v = adrof(CH_path);
	if (v == 0 && expath[0] != '/')
		pexerr();

/*  OR in the gflag to the slash flag.  The gflag was set if any globbing did
    occur on the command name.  This saves the gflag as far as the command name
    globbing is concerned.
*/
	slash |= gflag;

	/*
	 * Glob the argument list, if necessary.
	 * Otherwise trim off the quote bits.
	 */

#ifdef TRACE_DEBUG
  {
    CHAR *traceArg;

    traceArg = t -> t_dcom [1];

  if (childDebugStream != 0)
    {
      fprintf (childDebugStream, "doexec (7): pid: %d\n", getpid ());
      fflush (childDebugStream);
      fprintf (childDebugStream, "\tArg: %s\n", to_char (traceArg));
      fflush (childDebugStream);
    }
  }
#endif

	gflag = 0; av = &t->t_dcom[1];

/*  The rscan and tglob routines set gflag if globbing needs to be done on the
    command arguments.
*/
	rscan(av, tglob);
	if (gflag) {

/*  If globbing needs to be done on the arguments, do it and set av to the
    new space.  Av will eventually need to be freed.

    NOTE:  This doesn't happen!  Since it's in the child process though it
    shouldn't eat memory.
*/
		av = glob(av);
		if (av == 0)
			error((catgets(nlmsg_fd,NL_SETN,1, "No match")));
	}
	blk[0] = t->t_dcom[0];

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (8): pid: %d, blk[0] value: %s\n", getpid (),
	  to_char (blk[0]));
      fflush (childDebugStream);
    }
#endif

	blk[1] = 0;

/*  This call calloc's enough space for both of its arguments, and stores them
    in the new space and returns a pointer to it.  It does not free space from
    either block.  So the space pointed to by av is now lost and hasn't been
    freed!  
    
    The new space pointed to by 'av' is freed at the end of this routine.
*/
	av = blkspl(blk, av);
#ifdef VFORK
	Vav = av;
#endif
	scan(av, trim);

#ifdef VFORK_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (9): pid: %d, av[0] value: %s\n", getpid (), 
	       to_char (av[0]));
      fflush (childDebugStream);
    }
#endif


	xechoit(av);		/* Echo command if -x */

/*  Actually closes file descriptors 3-60 and resets SHIN, SHOUT, SHDIAG, OLDSTD
*/
	closech();		/* Close random fd's */

	/*
	 * We must do this after any possible forking (like `foo`
	 * in glob) so that this shell can still do subprocesses.
	 */
	sigsys(SIGCLD, SIG_DFL);	/* sigsys for vforks sake */
	sigsetmask(0L);

	/*
	 * If no path, no words in path, or a / in the filename
	 * then restrict the command search.
	 */
/*  In fact, set pv to NULL.
*/
	if (v == 0 || v->vec[0] == 0 || slash)
	  {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	     "doexec (10): pid: %d, slash: %d, justabs: %s\n", getpid (), slash,
	     to_char (*justabs));
      fflush (childDebugStream);
    }
#endif

		pv = justabs;
	  }

/*  Otherwise, set pv to point to the PATH components.
*/
	else
		pv = v->vec;

/*  Concatenate the first component of av (the command name) onto a '/'.  By
    using Strspl, space is calloc'd for this new name and a pointer to it is
    returned.  So sav will eventually need to be freed.

    The space pointed to by 'sav' is freed at the end of this routine.
*/
	sav = Strspl(CH_slash, *av);	/* / command name for postpending */
#ifdef VFORK
	Vsav = sav;
#endif

/*  If there is a hash table, calculate the hash index from the command name
    (simple addition of all the characters and then mod the result by the hash
    table size), and retrieve the value stored there.
*/
	if (havhash)
	  {
		hashval = xhash[hash(*av)];

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (11): pid: %d, hashval: %d\n", getpid (), hashval);
      fflush (childDebugStream);
      fprintf (childDebugStream, "\thash(*av): %d\n", hash(*av));
      fflush (childDebugStream);
    }
#endif

	  }

/*  Initialize the PATH component index.
*/
	i = 0;

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	  "doexec (12): pid: %d, Incrementing hits from %d\n", getpid (), hits);
      fflush (childDebugStream);
    }
#endif

/*  Indescrimenantly increment the hits global.  Note that this is in the child
    process, so the parent global is uneffected.
*/
	hits++;

#ifdef TRACE_DEBUG
  {
    CHAR **tracePv;
    tracePv = pv;

  if (childDebugStream != 0)
    {
      fprintf (childDebugStream, "doexec (13): pid: %d\n", getpid ());
      fflush (childDebugStream);
      while (*tracePv)
        {
          fprintf (childDebugStream, "pv: %s\n", to_char (*tracePv));
          fflush (childDebugStream);
	  tracePv ++;
        }
    }
  }
#endif

	do {

/*  If there wasn't a slash in the command name and globbing did not take place,
    AND the PATH component starts with a /, AND there is a hash table, shift 1
    to the left by the PATH component index mod 8 and see if this bit is set in
    the hash table value.  If so, then the command could be located in this PATH
    component.  If all the cases are TRUE but the last (the appropriate bit 
    isn't set) then the command can't be located in this PATH component, so go
    on to the next PATH component.

    If there was a slash or globbing took place, OR the PATH component doesn't
    start with a slash OR there isn't a hash table, try to exec the command.

    Note that if the appropriate bit in the hash table value was set, there
    could still be a miss due to a collision.  For example, since the hash index
    calcuation is just a simple addition mod the table size, there are many 
    combinations of characters that could generate the same index.  Furthermore,
    if the command name has the same characters as one of the files, but in a
    different order, the same index will be generated and the appropriate bit
    will be set.
*/
		if (!slash && pv[0][0] == '/' && havhash 
		    && (hashval & (1 << (i % 8))) == 0)
		  {

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (14): pid: %d, pv: %s\n", getpid (), to_char (*pv));
      fflush (childDebugStream);
    }
#endif

			goto cont;
		  }

/*  If there isn't a PATH or the PATH component starts with a dot, just try
    to exec the command name as is, with no PATH component pre-pended.

    If globbing occurred or there was a slash in the command name or there
    wasn't a PATH, pv got set to NULL.
*/
		if (pv[0][0] == 0 || eq(pv[0], ".")) {	/* don't make ./xxx */

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (15): pid: %d, pv: %s\n", getpid (), to_char (*pv));
      fflush (childDebugStream);
    }
#endif

#ifdef NONLS
			texec(*av, av);
#else
			/*  NLS: this is where the command and arg list gets
			    changed from CHAR back to char.  We vforked
			    to get here, so let the parent process release
			    storage malloced here, since the child won't				    be able to after a successful exec, but will
			    need this data after a failed exec.
			*/
			
/*  Both savebyte and blk_to_char calloc space, so the pointers Vcmd and
    Varg will eventually need to be freed.  Previous to HP-UX release 8.0
    VFORK was turned on, so the freeing of these pointers was in the VFORK
    code.  Since VFORK is now turned off, but the pointers are still used,
    they are freed if the exec doesn't work.
*/
			Vcmd = savebyte(to_char(*av));
			Varg = blk_to_char(av);

#ifdef TRACE_DEBUG
  {
    char **traceArgv;

    traceArgv = Varg;

    if (childDebugStream != 0)
      {
        fprintf (childDebugStream, "doexec (16): pid: %d\n", getpid ());
        fflush (childDebugStream);
    
        while (*traceArgv)
	  {
            fprintf (childDebugStream, "\tArg to texec: %s\n", *traceArgv ++);
            fflush (childDebugStream);
	  }
    }
  }
#endif

			texec(Vcmd, Varg);

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (17): pid: %d, Returned from texec\n", getpid ());
      fflush (childDebugStream);
    }
#endif

/*  These two lines used to be inside a #ifdef VFORK.  However, Vcmd and
    Varg are used whether or not VFORK is defined.  Therefore, they need
    to be freed in any case.  So the #ifdef VORK/#endif were removed.
*/
			xfree(Vcmd);
			blkfree(Varg);
#endif
		}

/*  If there was a match on the bit in the hash table, or there was a slash
    in the command name or globbing took place on it and the PATH component
    doesn't start with a dot, then build the full path to the command using
    the PATH component and try to exec it.

    Build the full path using Strspl.  This callocs space and returns a pointer
    to it, so dp will eventually need to be freed.  Exec the command.  If the
    command doesn't exec correctly, dp will be freed at the end of this 
    routine if it was used.
*/
		else {
			dp = Strspl(*pv, sav);

#ifdef TRACE_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "doexec (18): pid: %d, pv: %s\n", getpid (), to_char (*pv));
      fflush (childDebugStream);
      fprintf (childDebugStream, "\tdp: %s\n", to_char (dp));
      fflush (childDebugStream);
    }
#endif

#ifdef VFORK
			Vdp = dp;
#endif
#ifdef NONLS
			texec(dp, av);
#else
			/* NLS: ditto */

/*  Both savebyte and blk_to_char calloc space, so the pointers Vcmd and
    Varg will eventually need to be freed.  Previous to HP-UX release 8.0
    VFORK was turned on, so the freeing of these pointers was in the VFORK
    code.  Since VFORK is now turned off, but the pointers are still used,
    they are freed if the exec doesn't work.
*/
			Vcmd = savebyte(to_char(dp));
			Varg = blk_to_char(av);
			texec(Vcmd, Varg);

/*  These two lines used to be inside a #ifdef VFORK.  However, Vcmd and
    Varg are used whether or not VFORK is defined.  Therefore, they need
    to be freed in any case.  So the #ifdef VORK/#endif were removed.
*/
			xfree(Vcmd);
			blkfree(Varg);
#endif
#ifdef VFORK
			xfree(Vdp);
			Vdp = 0;
#endif

/*  If this side of the 'if' is taken, dp was used to hold the full path
    to the command name, and space was calloc'd for this name.  This space
    needs to be freed.
*/
                        xfree (dp);
		}

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	      "doexec (19): pid: %d, Incrementing misses from %d\n", getpid (), 
	      misses);
      fflush (childDebugStream);
    }
#endif

/*  If we get to this point, then the exec failed, so the misses global is
    incremented.  Note that since this is in the child process it does not
    affect the parent global.
*/
		misses++;

/*  This is the point we jump to if the command was not in the PATH component
    according to the hash table value.  The next PATH component is chosen, and
    the PATH component index is incremented.
*/
cont:
		pv++;
		i++;
	} while (*pv);

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream, 
	  "doexec (20): pid: %d, Decrementing hits from %d\n", getpid (), hits);
      fflush (childDebugStream);
    }
#endif

/*  If we get to this point, then none of the exec's worked, or we didn't try
    any.  In this case the hits global is decremented.  Note that since this 
    occurs in the child process, it does not effect the global in the parent.
*/
	hits--;
#ifdef VFORK	/* no exec, so child of vfork does own clean-up */
	Vsav = 0;
	Vav = 0;
#endif
	xfree(sav);
	xfree((CHAR *) av);
	pexerr();
}

/*  Called by:

	doexec ()
*/
/**********************************************************************/
pexerr()
/**********************************************************************/
{
	/* Couldn't find the damn thing */

/*  The setname routine sets bname to the command name.
*/
	setname(to_char(expath));
	/* xfree(expath); */

/*  This variable is set by texec if there was an unusual error after the
    execv.  If we get here from doexec then exerr is 0.
*/
	if (exerr)
		bferr(exerr);
	bferr((catgets(nlmsg_fd,NL_SETN,2, "Command not found")));
}

/* Last resort shell */
char	*lastsh[] = { SHELLPATH, 0 }; 

/*  Called by:

	doexec ()
*/
/*
 * Execute command f, arg list t.
 * Record error message if not found.
 * Also do shell scripts here.
 */
/**********************************************************************/
texec(f, t)
	char *f;
	register char *t[];  /* used to be **t */
/**********************************************************************/
{
	register struct varent *v;
	register CHAR **vp; 
#ifndef NONLS
	CHAR **tmp_t;		/* to store t temporarily */
#endif

#ifdef DEBUG_EXEC
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "texec (1): pid: %d, Exec'ing: %s\n", getpid (), f);
      fflush (childDebugStream);
    }
#endif

	sigrelse(SIGUSR2);
	untty();
	execv(f, t);
	switch (errno) {

	case ENOEXEC:
		/*
		 * If there is an alias for shell, then
		 * put the words of the alias in front of the
		 * argument list replacing the command name.
		 * Note no interpretation of the words at this point.
		 */
		v = adrof1(CH_shell, &aliases);
		if (v == 0) {
#ifdef OTHERSH
			register int ff = open(f, 0);
			char ch;
#endif

/*  This is done in all cases.  This routine calloc's space, so vp eventually
    needs to be freed.
*/
			vp = blk_to_short(lastsh);
#ifdef NONLS
			vp[0] = adrof(CH_shell) ? value(CH_shell) : SHELLPATH;
#else

/*  The routine savestr does a calloc, so vp[0] eventually needs to be freed.
*/
			vp[0] = adrof(CH_shell) ? value(CH_shell) : savestr(to_short(SHELLPATH));
#endif
#ifdef OTHERSH
			if (ff != -1 && read(ff, &ch, 1) == 1 && ch != '#')
#ifdef NONLS
				vp[0] = OTHERSH;
#else

/*  The routine savestr does a calloc, so vp[0] eventually needs to be freed.
*/
				vp[0] = savestr(to_short(OTHERSH));
#endif
			close(ff);
#endif
		} else
			vp = v->vec;
		t[0] = f;
#ifdef NONLS

/*  The routine blkspl does a calloc, so t eventually needs to be freed.
    Note that the reuse of t looses the memory that it pointed to previously.
    However, that memory is being freed at the end of the calling routine,
    doexec.
*/
		t = blkspl(vp, t);		/* splice up the new arglist */
#else

/*  The routines blk_to_short, blkspl, and blk_to_char all calloc space.  So
    tmp_t eventually needs to be freed, as does t.  However, the space calloc'd
    by blkspl seems to be lost and never freed.
*/
		tmp_t = blk_to_short(t);
		t = blk_to_char(blkspl(vp, tmp_t));	/* Splice up the new arglst */
		blkfree(tmp_t);
#endif
		f = *t;
		execv(f, t);
		xfree((CHAR *)t);
#ifndef NONLS
		if (v == 0)
			xfree(vp[0]);
#endif
		/* The sky is falling, the sky is falling! */

	case ENOMEM:
		Perror(f);

	case ENOENT:
		break;

	case EINVAL:
		error((catgets(nlmsg_fd,NL_SETN,4, "%s: Executable file incompatible with hardware")), f);

	default:
		if (exerr == 0) {
#ifndef NLS
			exerr = sys_errlist[errno];
#else
			exerr = strerror(errno);
#endif
			expath = savestr(to_short(f));
		}
	}
	ununtty();
	sighold(SIGUSR2);
}

/*  Called by:

	texec ()
	donewgrp ()
*/
/**********************************************************************/
ununtty()
/**********************************************************************/
{
#ifdef SIGTSTP
	if (tpgrp > 0) {
		setpgrp(0, tpgrp);
		ioctl(FSHTTY, TIOCSPGRP, &tpgrp);
	}
#endif
}

/*  Not called by any other routine.
*/
/**********************************************************************/
execash(t, kp)
	char **t;
	register struct command *kp;
/**********************************************************************/
{
	didcch++;
	signal(SIGINT, parintr);
	signal(SIGQUIT, parintr);
	signal(SIGTERM, parterm);		/* if doexec loses, screw */
	lshift(kp->t_dcom, 1);
	exiterr++;
	doexec(kp);
	/* NOTREACHED */
}

/*  Called by:

	doexec ()
	func ()
*/
/**********************************************************************/
xechoit(t)
	CHAR **t;
/**********************************************************************/
{

	if (adrof(CH_echo)) {
		flush();

/*  This is pretty odd; haderr directs where output will go, to stderr or
    to stdout, but it isn't used directly by either blkpr or printf.  
    
    The blkpr routine calls to_char and then error.  Then error calls flush
    (which uses haderr), and sets haderr to 1.  The printf routine calls 
    putchar which calls flush.
    
    So the net result is that this stuff gets printed to the error stream.
*/
		haderr = 1;
		blkpr(t), printf("\n");
		haderr = 0;
	}
}

/*  Called by:

	main ()
	dosetenv ()
	doset ()
	dolet ()
	Indirectly by func when the rhash command is executed.
*/
/**********************************************************************/
dohash()
/**********************************************************************/
{
	DIR *dirp;
	register struct direct *dp;
	register int cnt;
	int i = 0;
	struct varent *v = adrof(CH_path);
	CHAR **pv;

	havhash = 1;

	for (cnt = 0; cnt < HSHSIZ; cnt++)
		xhash[cnt] = 0;

	if (v == 0)
		return;

/*  Loop through all the PATH components.  Each component has an associated
    index value that cycles from 0 to 7 (i).
*/
	for (pv = v->vec; *pv; pv++, i = (i + 1) % 8) {

#ifdef HASH_DEBUG_TABLE
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	    "\ndohash (1): pid: %d, Directory: %s\n", getpid (), to_char (*pv));
      fflush (childDebugStream);
    }
#endif

/*  Only modify the hash table for PATH components that start with a slash.
*/
		if (pv[0][0] != '/')
			continue;

		if ((dirp = opendir(to_char(*pv))) == (DIR *)0)
			continue;

/*  Loop through the files in the PATH component directory:

    For each file, calculate an index (simple addition of all the characters
    in the name mod the table size.  This could lead to LOTS of collisions not
    only among files in the same directory but also among different directories.

    Shift 1 by the directory index and OR this into the existing hash table
    value.  This should cut down on collisions between directories, but looses
    information about collisions in the same directory.  

    Note that collisions will definitely occur between file names that have
    the same characters but in different orders as well as in other, more
    random, cases.
*/
		while ((dp = readdir(dirp)) != (struct direct *)0)
		  {
		    int hashVal;
		    hashVal = hash (to_short (dp -> d_name));

			xhash[hashVal] |= 1 << i;

#ifdef HASH_DEBUG_TABLE
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	      "\tfile: %s, i: %d, xhash [%d]: %o\n", dp -> d_name, i, hashVal, 
	      xhash [hashVal]);
      fflush (childDebugStream);
    }
#endif

		  }

		(void) closedir(dirp);
	}
}

/*  Called indirectly from func when the unhash command is executed.
*/
/**********************************************************************/
dounhash()
/**********************************************************************/
{
	havhash = 0;
}

/*  Called indirectly from func when the hashstat command is executed.
*/
/**********************************************************************/
hashstat()
/**********************************************************************/
{
#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "hashstat (1): pid: %d, hits: %d, misses: %d\n", getpid (), hits,
	       misses);
      fflush (childDebugStream);
    }
#endif

	if (hits+misses)
	printf((catgets(nlmsg_fd,NL_SETN,3, "%d hits, %d misses, %2d%%\n")), hits, misses, 100 * hits / (hits + misses));
}

/*  Add up all the characters in the file name.  If the result is negative,
    make it positive.  Return this number mod the hash table size.
*/
/*  Called by:

	doexec ()
	dohash ()
*/
/**********************************************************************/
hash(cp)
/**********************************************************************/
	register CHAR *cp;
{
	register long hash = 0;
	int retval;

	while (*cp)
		hash += hash + *cp++;
	if (hash < 0)
		hash = -hash;
	retval = hash % HSHSIZ;
	return (retval);
}

/*  This code will figure out if the child command is in the hash table or not.
    It will adjust the globals 'hits' and 'misses' accordingly.  However, the
    check will be on whether or not the command search was successful, not on
    whether or not the actual 'exec' would work or not.  This is different from
    previous versions of csh, where this check went on in the child process and
    the globals in the parent data space were updated, finally based on whether
    or not the 'exec' of the command was successful.  Since the initial loading
    of the hash table is built on whether or not a command name can be read in
    a directory, this seems like a reasonable action.
*/
/*  Called by:

	execute ()
*/
/**********************************************************************/
updateHashStats (t)
/**********************************************************************/
  struct command *t;
{
  struct varent *hashV;
  CHAR *hashDp;
  CHAR *tempDp;
  int hashGflg;
  struct stat hashStatStruct;
  int hashValue;
  int hashSlash;
  CHAR **hashPv;
  int hashI;
  CHAR *hashPath;
  int hashIndex;
  CHAR *hashFullPath;
  int retValue;
  int hashAND;

/*  If there isn't a hash table, don't bother.
*/
  if (!havhash)
    {

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (1): pid: %d, No hash table.\n", getpid ());
      fflush (childDebugStream);
    }
#endif

      return;
    }

/*  If there isn't a PATH, don't bother.
*/
  hashV = adrof (CH_path);
  if ((hashV == 0) || (hashV -> vec [0] == 0))
    {

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (2): pid: %d, No path.\n", getpid ());
      fflush (childDebugStream);
    }
#endif

      return;
    }

/*  Copy the command name into another pointer.  Check for a '/' in the command
    name.  The Strspl routine does a calloc, so this pointer will eventually 
    have to be freed.  
*/
  hashDp = Strspl (t -> t_dcom [0], nullstr);
  hashSlash = Any ('/', hashDp);

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	   "updateHashStats (3): pid: %d, command: %s, slash: %d\n", getpid (), 
	   to_char (hashDp), hashSlash);
      fflush (childDebugStream);
    }
#endif

/*  Save the current gflag.  The routine globone calls rscan which calls tglob,
    which can set this flag via an OR operation.  The routine rscan can also
    set the flag via an OR.  Clear this global flag.
*/
  hashGflg = gflag;
  gflag = 0;

/*  This routine checks to see if glob characters are in the command name.  If
    so, it calls the routine glob.  This routine ends up allocating memory for
    any matches it finds.  If more than one match is found the routine generates
    an error.  If no globbing is done, the command name is still saved so tempDp
    will have to be freed.
*/
  tempDp = globone (hashDp);

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
            "updateHashStats (4): pid: %d, globbed: %s, gflag: %d\n", getpid (),
	   to_char (tempDp), gflag);
      fflush (childDebugStream);
    }
#endif

/*  Free tempDp.  Check gflag.  If set, then the hash table won't be used to 
    find the command.  So reset gflag from the saved value, free hashDp, and
    return.
*/
  xfree (tempDp);
  if (gflag)
    {
      gflag = hashGflg;
      xfree (hashDp);
      return;
    }

/*  Otherwise, the hash table will be used to find the command.  So calculate
    the hash index from the command name (the algorithm uses a simple addition
    of all the characters and does a 'mod' the table size), and retrieve the
    value from the hash table.  This value is an OR of all the possible 
    locations of a particular command name.
*/
  hashIndex = hash (hashDp);
  hashValue = xhash [hashIndex];

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (5): pid: %d, hashIndex: %d, hashValue: %d\n", 
	       getpid (), hashIndex, hashValue);
      fflush (childDebugStream);
    }
#endif

/*  Set up a pointer to the PATH components, and initialize an index variable.
*/
  hashPv = hashV -> vec;
  hashI = 0;

/*  Loop through all the PATH components until there aren't any more, or until
    a match is found.
*/
  while (*hashPv)
    {

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	     "updateHashStats (6): pid: %d, hashPv: %s, hashI: %d\n", getpid (),
	     to_char (*hashPv), hashI);
      fprintf (childDebugStream,
	     "\tshifted bit: %d, AND result: %d\n", (1 << (hashI % 8)), 
	    (hashValue & (1 << (hashI % 8))));
      fflush (childDebugStream);
    }
#endif

/*  The original hash table setup looped through all the PATH components, and 
    used an index variable that was incremented for each component.  For each
    command in the directory that could be opened, a hash index was calculated 
    from the file name (as described above).  Then a 1, left shifted by the
    index variable for the directory after a 'mod' 8 was OR'd into the hash
    table value at this location.  This check does the same sort of thing 
    backwards.  That is, the directory index value mod 8 is used to left shift
    a 1, which is then AND'd with the hash table value.  If the bit in the hash
    table value is not set, then there is no possibility that this directory
    has the command in it, so the next PATH component is checked.  
    
    Note that also there can be no '/' in the command name, and that the PATH 
    component must start with a '/'.  If any of these cases is not TRUE, then 
    csh will try to exec the command name (in the child code in doexec()).
*/

      hashAND = hashValue & (1 << (hashI % 8));

      if ((!hashSlash) && (hashPv [0][0] == '/') && (hashAND == 0))
	{

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	      "updateHashStats (7): pid: %d, Skipping: hashPv: %s, hashI: %d\n",
	      getpid (), to_char (*hashPv), hashI);
      fflush (childDebugStream);
    }
#endif

	  hashPv ++;
	  hashI ++;
	  continue;
	}
      
/*  If the csh child would try to exec the command with this PATH component,
    build the full command name.  Use Strspl to copy the PATH component and
    a '/' into hashPath.  (Strspl calloc's space, so hashPath will have to be
    freed.)

    Next concatenate the command name onto hashPath to create hashFullPath.
    Again, use Strspl to do this.  (Since Strspl calloc's space, hashFullPath
    will have to be freed.)

    Free hashPath.
*/
      hashPath = Strspl (*hashPv, CH_slash);
      hashFullPath = Strspl (hashPath, hashDp);
      xfree (hashPath);

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (8): pid: %d, hashFullPath: %s\n", getpid (), 
	       to_char (hashFullPath));
      fflush (childDebugStream);
    }
#endif

/*  Stat the full path to the command.  If this works, then the hash 
    table has been used to find the command, so increment the hits global
    variable.  Free hashFullPath, and break out of the while loop.
*/

      retValue = stat (to_char (hashFullPath), &hashStatStruct);
      if (retValue == 0)
	{

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (9): pid: %d, stat worked\n", getpid ());
      fflush (childDebugStream);
    }
#endif

	  hits ++;
          xfree (hashFullPath);
	  break;
	}
      
/*  If the stat fails, then the hash table didn't find a hit, so increment
    the misses global variable.  Increment to the next PATH component, and
    free hashFullPath.
*/
      else
	{

#ifdef HASH_DEBUG
  if (childDebugStream != 0)
    {
      fprintf (childDebugStream,
	       "updateHashStats (10): pid: %d, stat failed: %d, errno: %d\n", 
	       getpid (), retValue, errno);
      fflush (childDebugStream);
    }
#endif

	  misses ++;
	  hashPv ++;
	  hashI ++;
          xfree (hashFullPath);
	}
    }
  
/*  Once the while loop is finished, restore the gflag global and free hashDp.
*/
  gflag = hashGflg;
  xfree (hashDp);
}
