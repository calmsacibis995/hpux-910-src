/* @(#) $Revision: 66.2 $ */

/*
 * strrstr() --
 *    find the last occurance of a substring in a string.
 *    NULL and "" are treated both treated as the empty string.
 *
 *    Returns pointer to the beginning of the last occurance of
 *    substring in string if found.
 *    Returns s if t is the emtpy string.
 *    Returns NULL if substring is not found.
 */

#ifdef _NAMESPACE_CLEAN
#  define strrstr _strrstr
#  define strlen  _strlen
#endif /* _NAMESPACE_CLEAN */

#include <string.h>

#ifdef _NAMESPACE_CLEAN
#undef strrstr
#pragma _HP_SECONDARY_DEF _strrstr strrstr
#define strrstr _strrstr
#endif

char *
strrstr(s, t)
register char *s;
register const char *t;
{
    register char *se;
    register char *te;

    if (t == NULL || *t == '\0')
	return s;

    if (s == NULL)
	return NULL;

    /*
     * Find end of s and t
     */
    se = s + strlen(s) - 1;
    te = t + strlen(t) - 1;

    /*
     * Now search backwards
     */
    do
    {
	register char *tp = te;
	register char *sp = se;

	while (*sp == *tp)
	    if (tp > t)
		--sp, --tp;
	    else
		return sp;
	--se;
    } while (se >= s);

    return NULL;
}
