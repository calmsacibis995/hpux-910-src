/* @(#) $Revision: 66.1 $ */
/* C library -- exit	*/
/* exit(code) */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define exit ___exit
#define atexit _atexit
#endif

#include <limits.h>
#include <stdlib.h>

/*
 * aefunc[] -- an array that holds the address of the atexit functions
 *             to call when exit() is called.  The functions are called
 *             in reverse order.
 * nf       -- The number of functions in aefunc[].
 * ep	    -- Flag that says that we are in exit() [you can't call
 *             atexit() inside an atexit function.
 */
static void (*aefunc[ATEXIT_MAX])();
static int  nf = 0;
static int  ep = 0;

/*
 * Notes: Special cases during exit processing.
 *	1) call to atexit: ignore and return unsuccessful (-1)
 *	2) call to exit: do not return to caller, continue processing
 *	   atexit functions.  Notice that the status that _exit is
 *         finally called with is the exit status of the *last*
 *         call to exit().  ANSI-C currently does not define the
 *         behavior of exit() when it is called more than once.
 */

#ifdef _NAMESPACE_CLEAN
#undef exit
#pragma _HP_SECONDARY_DEF ___exit exit
#define exit ___exit
#endif

void
exit(status)
int status;
{
    ep = 1;		/* indicate exit processing */
    while (nf>0)	/* call atexit functions in reverse order */
	(*aefunc[--nf])();
    _exitcu();		/* exit cleanup routine (stdio stuff) */
    _exit(status);
}

#ifdef _NAMESPACE_CLEAN
#undef atexit
#pragma _HP_SECONDARY_DEF _atexit atexit
#define atexit _atexit
#endif

int
atexit(func)
void (*func)();
{
    if (ep)			/* ignore request, return error */
	return -1;

    if (nf < ATEXIT_MAX)	/* register function */
    {
	aefunc[nf++] = func;
	return 0;
    }

    return ATEXIT_MAX;		/* return error status */
}
