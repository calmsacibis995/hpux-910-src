/* @(#) $Revision: 32.3 $ */    
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	general purpose string handling ======== */

char *
movstr(a, b)
register char	*a, *b;
{
	if (a == NIL || b == NIL)
		return(b);
	while (*b++ = *a++);
	return(--b);
}


tchar *			
tmovstr(a, b)
register tchar	*a, *b;	
{
if (a == TNIL || b == TNIL)
		return(b);
	while (*b++ = *a++);
	return(--b);
}

any(c, s)
register tchar	c;
tchar	*s;
{
	register tchar d;

	while (d = *s++)
	{
		if (d == c)
			return(TRUE);
	}		
	return(FALSE);
}		

cf(s1, s2)
register tchar *s1, *s2;
{
	if (s1 == TNIL)
		return(s2 == TNIL ? 0 : -(*s2));
	if (s2 == TNIL)
		return(*s1);
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return(0);
	return(*--s1 - *s2);
}

length(as)
char	*as;		
{
	register char	*s;

	if (s = as)
		while (*s++);
	return(s - as);		/* returns number of elements, inc. null */
}

tlength(as)
tchar	*as;
{
	register tchar	*s;

	if (s = as)
		while (*s++);
	return(s - as);		/* returns number of elements, inc. null */
}

tchar *
movstrn(a, b, n)
	register tchar *a, *b;
	register int n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}

char *
cmovstrn(a, b, n)
	register char *a, *b;
	register int n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}
