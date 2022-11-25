/* @(#) $Revision: 64.1 $ */    
/*LINTLIBRARY*/
/*
 * convert long int to base 64 ascii
 * char set is [./0-9A-Za-z]
 * two's complement negatives are assumed,
 * but no assumptions are made about sign propagation on right shift
 *
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define l64a _l64a
#endif

#include <values.h>
#define BITSPERCHAR	6 /* to hold entire character set */
#define BITSPERLONG	(BITSPERBYTE * sizeof(long))
#define NMAX	((BITSPERLONG + BITSPERCHAR - 1)/BITSPERCHAR)
#define SIGN	(-(1L << (BITSPERLONG - BITSPERCHAR - 1)))
#define CHARMASK	((1 << BITSPERCHAR) - 1)
#define WORDMASK	((1L << ((NMAX - 1) * BITSPERCHAR)) - 1)

static char buf[NMAX + 1];

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef l64a
#pragma _HP_SECONDARY_DEF _l64a l64a
#define l64a _l64a
#endif

char *
l64a(lg)
register long lg;
{
	register char *s = buf;

	while (lg != 0) {

		register int c = ((int)lg & CHARMASK) + ('0' - 2);

		if (c > '9')
			c += 'A' - '9' - 1;
		if (c > 'Z')
			c += 'a' - 'Z' - 1;
		*s++ = c;
		/* fill high-order CHAR if negative */
		/* but suppress sign propagation */
		lg = ((lg < 0) ? (lg >> BITSPERCHAR) | SIGN :
			lg >> BITSPERCHAR) & WORDMASK;
	}
	/* return '.' (zero) on null parameter string */
	if (s == buf)
		*s++ = '.';
	*s = '\0';
	return (buf);
}
