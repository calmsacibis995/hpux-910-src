/* @(#) $Revision: 29.2 $ */     
#

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Define extra stuff not found in signal.h
 */

#ifndef SIGRETRO

#define	SIGRETRO				/* Can use this for cond code */

#ifndef SIG_HOLD

#define	SIG_HOLD	(int (*)()) 3		/* Phony action to hold sig */

#ifndef BADSIG
#define	BADSIG		(int (*)()) -1		/* Return value on error */
#endif

#endif SIG_HOLD

#endif SIGRETRO
