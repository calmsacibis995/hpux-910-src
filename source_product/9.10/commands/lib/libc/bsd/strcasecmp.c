/* @(#) $Revision: 66.2 $ */

#include <sys/types.h>
#include <ctype.h>

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strcasecmp strcasecmp
#define strcasecmp _strcasecmp
#endif

strcasecmp(s1, s2)
	char *s1, *s2;
{
	register u_char	*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while (_tolower(*us1) == _tolower(*us2++))
		if (*us1++ == '\0')
			return(0);
	return(_tolower(*us1) - _tolower(*--us2));
}
