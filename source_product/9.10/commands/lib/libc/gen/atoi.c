/* @(#) $Revision: 70.1 $ */    
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define atoi _atoi
#endif

#include <ctype.h>

#define ATOI

#ifdef	ATOI
typedef int TYPE;
#define NAME	atoi
#else
typedef long TYPE;
#define NAME	atol
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef ATOI
#ifdef _NAMESPACE_CLEAN
#undef atoi
#pragma _HP_SECONDARY_DEF _atoi atoi
#define atoi _atoi
#endif  /* _NAMESPACE_CLEAN */
#endif  /* ATOI */

#define MBASE		('z' - 'a' + 1 + 10)
int
__ASCII_to_digit(x)
int x;
{
	if( (x >= '0') && (x <= '9'))
		return (x - '0');
	if ( (x >= 'a') && (x <= 'z'))
		return (10 + x - 'a');
	if ( (x >= 'A') && (x <= 'Z'))
		return (10 + x - 'A');
	return (MBASE+1);
}

TYPE
NAME(p)
register char *p;
{
	register TYPE n;
	register int c, neg = 0;

	if (!isdigit(c = *p)) {
		while (isspace(c))
			c = *++p;
		switch (c) {
		case '-':
			neg++;
		case '+': /* fall-through */
			c = *++p;
		}
		if (!isdigit(c))
			return (0);
	}
	for (n = '0' - c; isdigit(c = *++p); ) {
		n *= 10; /* two steps to avoid unnecessary overflow */
		n += '0' - c; /* accum neg to avoid surprises at MAX */
	}
	return (neg ? n : -n);
}
