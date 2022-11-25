/* @(#) $Revision: 64.4 $ */      
/*LINTLIBRARY*/
/*
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * At most n characters are moved.
 * Return s1.
 */

#define NULL    (char *)0
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strncat strncat
#define strncat _strncat
#endif

#include <string.h>

char *
strncat(s1, s2, n)
register char *s1, *s2;
register size_t n;
{
	register char *os1;

	if ( (s2 == NULL) || (n == 0) )
		return(s1);

	os1 = s1;
	while(*s1++)
		;
	--s1;
	while(*s1++ = *s2++)
		if(--n == 0) {
			*s1 = '\0';
			break;
		}
	return(os1);
}
