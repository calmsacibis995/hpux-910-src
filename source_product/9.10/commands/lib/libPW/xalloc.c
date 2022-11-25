/* @(#) $Revision: 64.1 $ */      
/* The routines in this file have been modified to (at least temporarily)
   use the standard malloc and free.  "xfreeall" is null.  This means that
   SCCS cmds should only be used with one file arg at a time.
   KAH  4/27/82
*/


#include <stdlib.h>

xalloc(asize)
unsigned asize;
{
	char *p;
	if ( p = malloc(asize) )  return( (int) p);
	else  return( fatal("out of space (ut9)") );
}


xfree(aptr)
char *aptr;
{ 
	free(aptr);
}


xfreeall()
{
}


