/* @(#) $Revision: 27.1 $ */      
#include <stdio.h>

int DEBUG=0;

/* LINTLIBRARY */
/* VARARGS1 */

/*
 * Print a debugging message on stderr if DEBUG is set.
 */
bugout(s, a,b,c,d,e,f)
char *s;
{
	if (DEBUG) {
		fflush(stdout);
		fprintf(stderr, s, a,b,c,d,e,f);
		fprintf(stderr, "\n");
	}
}
