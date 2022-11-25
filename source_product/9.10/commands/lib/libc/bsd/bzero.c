/* @(#) $Revision: 66.1 $  */

/*
 * bzero - zero count bytes at address addr
 *
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _bzero bzero
#   define bzero _bzero
#   define memset _memset
#endif

bzero(addr, count)
char *addr;
int count;
{
    /* just call memset */
    return memset(addr, (char)0, count);
}
