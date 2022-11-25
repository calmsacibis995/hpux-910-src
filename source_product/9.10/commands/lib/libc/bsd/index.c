/* @(#) $Revision: 66.2 $  */

/*
 * index()
 *
 * find the first position of a character in a string
 *
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _index index
#   define index _index
#   define strchr _strchr
#endif

char *
index(s, c)
char *s;
char c;
{
    extern char *strchr();

    /* just call strchr */
    return strchr(s, c);
}
