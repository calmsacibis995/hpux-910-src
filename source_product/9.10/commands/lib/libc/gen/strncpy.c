/* @(#) $Revision: 66.1 $ */      
/*LINTLIBRARY*/
/*
 * Copy s2 to s1, truncating or null-padding to always copy n bytes
 * return s1
 */

#define NULL    (char *)0
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strncpy strncpy
#define strncpy _strncpy
#endif

#include <string.h>

char *
strncpy(s1, s2, n)
char	*s1;
register char	*s2;
register size_t	n;
{
	register char *s1p = s1;

	while (n > 0) {
		n--;
		if ((*s1p++ = *s2++) == '\0')
			break;
	}
	for ( ; n > 0; n-- )
		*s1p++ = '\0';

	return (s1);
}
