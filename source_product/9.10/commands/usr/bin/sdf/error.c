/* @(#) $Revision: 37.1 $ */    
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                error.c                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <stdio.h>

#define	SDF_LOCK	"/tmp/SDF..LCK"

#ifdef	DEBUG
extern int debug;
#endif	DEBUG

/* --------------------------------------------------- */
/* fatal - print error message and die (variable args) */
/* --------------------------------------------------- */

/*VARARGS*/
fatal(s, a, b, c, d, e, f, g, h, i)
register char *s;
{
	error(s, a, b, c, d, e, f, g, h, i);
	unlink(SDF_LOCK);
	exit(1);
}

/* ---------------------------------------------- */
/* error - print an error message (variable args) */
/* ---------------------------------------------- */

/*VARARGS*/
error(s, a, b, c, d, e, f, g, h, i)
register char *s;
{
	fprintf(stderr, s, a, b, c, d, e, f, g, h, i);
	fprintf(stderr, "\n");
}

/* -------------------------------------------------- */
/* nomemory - report that the heap has been exahusted */
/* -------------------------------------------------- */

nomemory()
{
	fatal("out of memory; malloc() failed");
}

#ifdef	DEBUG
/* ----------------------------------- */
/* bugout - debugging/logging function */
/* ----------------------------------- */

bugout()
{
	return(0);
}
#endif	DEBUG
