/* @(#) $Revision: 66.1 $  */

/*
 * bcmp
 *
 * compare count bytes at s1 and s2; return 0 iff equal
 *
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _bcmp bcmp
#   define bcmp _bcmp
#   define memcmp _memcmp
#endif

bcmp(s1, s2, count)
char *s1;
char *s2;
int count;
{
    /* just call memcmp */
    return memcmp(s1, s2, count);
}
