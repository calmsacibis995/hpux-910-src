/* @(#) $Revision: 66.2 $ */

/*
 * ffs() --
 *     Find first bits set in argument and return index of bit. Index
 *     begins with one.
 *     Return zero if argument is zero.
 */

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _ffs ffs
#   define ffs _ffs
#endif /* _NAMESPACE_CLEAN */

int
ffs(i)
register int i;
{
	register int res = 1;

	/* If zero argument, return zero */
	if (i == 0)
		return 0;

	/* Look for first bit set beginning with LSB.  */
	while (! (i & 1)) {
		i >>= 1;
		res++;
	}
	return res;
}
