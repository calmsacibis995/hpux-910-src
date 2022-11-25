/* @(#) $Revision: 66.2 $ */

#include <sys/types.h>
#include <ctype.h>

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strncasecmp strncasecmp
#define strncasecmp _strncasecmp
#endif

strncasecmp(s1, s2, n)
	char *s1, *s2;
	register int n;
{
	register u_char	*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while (--n >= 0 && _tolower(*us1) == _tolower(*us2++))
		if (*us1++ == '\0')
			return(0);
	return(n < 0 ? 0 : _tolower(*us1) - _tolower(*--us2));
}
