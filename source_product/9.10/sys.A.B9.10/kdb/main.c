/* @(#) $Revision: 66.2 $ */    
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This is the top of the HPUX symbolic debugger.  This file contains the
 * entry routine (main()), the top of the command handling tree, various
 * error catchers, and miscellaneous utility routines.
 */

#define stderr -1
#define BUFSIZ 200
#include "cdb.h"

extern	char	*malloc();		/* libraries */
extern	void	free();
extern	char	*_sbrk();

/***********************************************************************
 * ERROR MESSAGE TEMPLATES:
 *
 * These are used mostly for Panic() calls, and some UError() calls, to
 * save space and make the messages consistent.  In each case except Adr
 * and Access, each uppercase letter after "vsbErr" represents a string
 * (S) or decimal (D) argument.
 */

export	char	*vsbErrSinSD	= "%s in %s() (%d)";
export	char	*vsbErrSinSDD	= "%s in %s() (%d, %d)";
export	char	*vsbErrSfailSD	= "%s() failed in %s() (%d)";
export	char	*vsbErrNoChild	= "No child process";

export	char	*vsbCmdsIgnored = "Command line or portion ignored:  \"%s\"\n";




/***********************************************************************
 * D E B U G   I T
 *
 * This is the top of the command line tree.  It reads lines from input
 * (keyboard or playback file) and records and executes them.  It also
 * handles command repetition.  Loops, reading new command lines, until
 * FDoCommand() tells it to quit.
 */

export void DebugIt ()
{
    register int	count, i;	/* command repeat count */
    char	sbCmd [BUFSIZ];		/* cmd line read here	*/

    vsbCmd = 0;
    while (1)			/* until hell freezes over */
    {
	count = 1;
#ifdef KDBKDB
	NextCmd (sbCmd, BUFSIZ, "#>");	/* read from tty or file */
#else
#ifdef CDBKDB
	NextCmd (sbCmd, BUFSIZ, "kdb>");	/* read from tty or file */
#else /* not CDBKDB */
	NextCmd (sbCmd, BUFSIZ, ">");	/* read from tty or file */
#endif /* CDBKDB */
#endif

	if (*sbCmd == chNull)		/* hit ^D or played back blank line */
	{
	    printf ("\010");			/* get rid of prompt */
	    count    = 10;
	    sbCmd[0] = '~';			/* the "repeat" command */
	    sbCmd[1] = chNull;
	}
	for (i = 0; i < count; i++)	/* here's how we repeat things (ecch) */
	{
	    FDoCommand (sbCmd);
	}
    }
} /* DebugIt */




/***********************************************************************
 * P A N I C
 *
 * General purpose handler for debugger (not user) errors.
 */

/* VARARGS1 */
export void Panic (msg, arg1, arg2, arg3, arg4)
    char	*msg;			/* message (printf() string) */
    int		arg1, arg2, arg3, arg4; /* arguments that go with it */
{
    printf ("kdb panic: ");		/* start with who we are */

    if (msg)
	printf (msg, arg1, arg2, arg3, arg4);
    printf ("\n");			/* always finish a line */

    vsbCmd = sbNil;
#ifdef KASSERT
    vfRunAssert = true;
#endif
    vcNest = 0;
#ifndef CDBKDB
    Kjmp();
#endif /* CDBKDB */

} /* Panic */


/***********************************************************************
 * U   E R R O R
 *
 * General purpose handler for user (not debugger) errors.
 */

/* VARARGS1 */
export void UError (msg, arg1, arg2, arg3, arg4)
    char	*msg;			/* message (printf() string) */
    int		arg1, arg2, arg3, arg4; /* arguments that go with it */
{
    if (msg)
	printf (msg, arg1, arg2, arg3, arg4);
    else
	printf ("error");			/* generic ouch */

    printf  ("\n");				/* always finish a line */
    vsbCmd = sbNil;
#ifdef KASSERT
    vfRunAssert = true;
#endif
    vcNest = 0;
#ifndef CDBKDB
    Kjmp();
#endif /* CDBKDB */

} /* UError */




/***********************************************************************
 * M A L L O C
 *
 * This front-end to malloc(3) optionally does error checking (if the
 * calling proc tells its name) and keeps the totals variable updated.
 * Rather than adding and subtracting cb here and in Free(), we give
 * a more precise estimate using sbrk(2).
 */

export char * Malloc (cb, sbCaller)
    uint	cb;		/* bytes to malloc	   */
    char	*sbCaller;	/* name of caller or sbNil */
{
    char	*sb;		/* ptr to memory gotten	   */

    if (((sb = malloc (cb)) == sbNil)		/* malloc failed */
    AND (sbCaller != sbNil))			/* caller known  */
	Panic (vsbErrSfailSD, "Malloc", sbCaller, cb);
    return (sb);

} /* Malloc */


/***********************************************************************
 * F R E E
 *
 * This front-end to free(3) avoids using a nil pointer, and keeps the
 * totals variable updated.
 */

export void Free (sb)
    char	*sb;		/* memory to free */
{
    if (sb == sbNil)
	Panic (vsbErrSinSD, "Invalid pointer", "Free", sb);

    free (sb);
} /* Free */




