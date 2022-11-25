/* @(#) $Revision: 64.4 $ */    
/*LINTLIBRARY*/
/*
 * convert base 64 ascii to long int
 * char set is [./0-9A-Za-z]
 *
 */
#ifdef _NAMESPACE_CLEAN
#define a64l _a64l
#endif

#define BITSPERCHAR	6 /* to hold entire character set */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef a64l
#pragma _HP_SECONDARY_DEF _a64l a64l
#define a64l _a64l
#endif

long
a64l(s)
register char *s;
{
	register int i, c;
	long lg = 0;

	for (i = 0; (c = *s++) != '\0'; i += BITSPERCHAR) {
		if (c > 'Z')
			c -= 'a' - 'Z' - 1;
		if (c > '9')
			c -= 'A' - '9' - 1;
		lg |= (long)(c - ('0' - 2)) << i;
	}
	return (lg);
}

