/* @(#) $Revision: 64.3 $ */      
/*LINTLIBRARY*/
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters from charset.
 */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strspn strspn
#define strspn _strspn
#endif

#include <string.h>

size_t
strspn(string, charset)
char	*string;
register char	*charset;
{
	register char *p, *q;

	for(q=string; *q != '\0'; ++q) {
		for(p=charset; *p != '\0' && *p != *q; ++p)
			;
		if(*p == '\0')
			break;
	}
	return(q-string);
}
