/* $Header: strsav.c,v 1.4 86/04/17 13:47:09 bob Exp $ */

/*
 * Author: Peter J. Nicklin
 */

/*
 * strsav() saves a string somewhere and returns a pointer to the somewhere.
 * Aborts with message "out of memory" on any error.
 */
#include "rcsbase.h"

char *
strsav(s)
	char *s;
{
	char *sptr;			/* somewhere string pointer */
	char *malloc();			/* memory allocator */
	char *strcpy();			/* string copy */
	int strlen();			/* string length */

	if ((sptr = malloc((unsigned)(strlen(s)+1))) == nil)
		faterror("out of memory");
	return(strcpy(sptr, s));
}
