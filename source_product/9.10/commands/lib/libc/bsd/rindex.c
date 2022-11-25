/* @(#) $Revision: 66.2 $  */

/*
 * rindex()
 *
 * find the latest position of a character in a string
 *
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _rindex rindex
#   define rindex _rindex
#   define strrchr _strrchr
#endif

char *
rindex(s, c)
char *s;
char c;
{
    extern char *strrchr();

    /* just call strrchr */
    return strrchr(s, c);
}
