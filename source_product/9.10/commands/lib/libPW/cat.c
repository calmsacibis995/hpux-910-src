/* @(#) $Revision: 37.1 $ */     
/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
 
	returns destination.
*/

#include	<varargs.h>

char *cat(dest,va_alist)
char *dest;
va_dcl
{
	register char *d, *s;
	va_list ap;

	d = dest;
	for (va_start(ap); s = va_arg(ap, char*); ) {
		while (*d++ = *s++) ;
		d--;
	}
	return(dest);
}
