/* @(#) $Revision: 66.3 $  */

/*
 * bcopy - copy count bytes from "from" to "to".
 *
 * Works with overlapped areas.
 *
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _bcopy bcopy
#   define bcopy _bcopy
#   define memmove _memmove
#   define memcpy _memcpy
#endif

/*
NOTE: the s700/800 performance fix for MCF/8.0 is to call memcpy in
cases where overlap is not an issue.  For IF1, memmove will be re-written
in PA assembly.  At that time, this source should be changed to always
call memmove.
*/

bcopy(from, to, count)
char *from;
char *to;
int count;
{
#ifdef __hp9000s300
    /* just call memmove [with first two args reversed] */
    return memmove(to, from, count);
#else
    /* only call memmove if the end of <from> overlaps the start of <to> */
    if (from<to && from+count>to)
	/* just call memmove [with first two args reversed] */
    	return memmove(to, from, count);
    else /* this should handle 99.9% of the cases */
    	return memcpy(to, from, count);
#endif
}
