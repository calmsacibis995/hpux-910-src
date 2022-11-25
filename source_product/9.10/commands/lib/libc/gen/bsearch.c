/* HPUX_ID: @(#) $Revision: 64.2 $  */
/* @(#) $Revision: 64.2 $ */      

/*LINTLIBRARY*/
/*
 * Binary search algorithm, generalized from Knuth (6.2.1) Algorithm B.
 *
 */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _bsearch bsearch
#define bsearch _bsearch
#endif

typedef char *POINTER;

POINTER
bsearch(key, base, nel, width, compar)
register POINTER	key;		/* Key to be located */
register POINTER	base;		/* Beginning of table */
register unsigned 	nel;		/* Number of elements in the table */
unsigned 		width;		/* Width of an element (bytes) */
register int		(*compar)();	/* Comparison function */
{
	register POINTER 	p;
	register baseelt = 0;		/* first element to search */
	register lastelt = nel - 1;	/* last element to search */
	unsigned register 	elt;			/* current element */
	register int 		res;

	while (lastelt >= baseelt) {

		/* high performance divide by 2 */
		elt = ((lastelt-baseelt) >> 1) + baseelt;

		p = base + (elt * width );

		/* call compare routine and return if matched */
		if ((res = (*compar)(key, p)) == 0) return(p);

		/* no match, adjust base or last element */
		if (res < 0)	lastelt = elt - 1;
		else		baseelt = elt + 1;
	}
	return((POINTER) 0);			/* key not found */
}
