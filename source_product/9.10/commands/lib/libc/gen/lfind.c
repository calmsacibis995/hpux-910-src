/* @(#) $Revision: 64.1 $ */   
/*LINTLIBRARY*/
/*
 * Linear search algorithm, generalized from Knuth (6.1) Algorithm Q.
 *
 * This version no longer has anything to do with Knuth's Algorithm Q,
 * which first copies the new element into the table, then looks for it.
 * The assumption there was that the cost of checking for the end of the
 * table before each comparison outweighed the cost of the comparison, which
 * isn't true when an arbitrary comparison function must be called and when the
 * copy itself takes a significant number of cycles.
 * Actually, it has now reverted to Algorithm S, which is "simpler."
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define lfind _lfind
#endif

typedef char *POINTER;
extern POINTER memcpy();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef lfind
#pragma _HP_SECONDARY_DEF _lfind lfind
#define lfind _lfind
#endif

POINTER
lfind(key, base, nelp, width, compar)
register POINTER key;		/* Key to be located */
register POINTER base;		/* Beginning of table */
unsigned *nelp;			/* Pointer to current table size */
register unsigned width;	/* Width of an element (bytes) */
int (*compar)();		/* Comparison function */
{
	register POINTER next = base + *nelp * width;	/* End of table */

	for ( ; base < next; base += width)
		if ((*compar)(key, base) == 0)
			return (base);	/* Key found */
	return (POINTER)(0);
}
