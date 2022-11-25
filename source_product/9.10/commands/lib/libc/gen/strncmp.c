/* @(#) $Revision: 64.4 $ */      
/*LINTLIBRARY*/
/*
 * Compare strings (at most n bytes)
 *	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

#define NULL    (unsigned char *)0
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strncmp strncmp
#define strncmp _strncmp
#endif

#include <string.h>

int
strncmp(s1, s2, n)
register unsigned char *s1, *s2;
register size_t n;
{
	if(s1 == s2)
		return(0);
	if (s1 == NULL)
	{
		if (s2 == NULL)
			return(0);
		else
			return(-((int)*s2));
	}
	else if (s2 == NULL)
		return(*s1);

	for ( ; n > 0 && *s1 == *s2++; n-- )
		if(*s1++ == '\0')
			return(0);
	return((n == 0)? 0: (*s1 - *--s2));
}
