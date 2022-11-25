/* @(#) $Revision: 64.1 $ */      
/*LINTLIBRARY*/
/*
 * Convert longs to and from 3-byte disk addresses
 */
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define ltol3 _ltol3
#define l3tol _l3tol
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ltol3
#pragma _HP_SECONDARY_DEF _ltol3 ltol3
#define ltol3 _ltol3
#endif

void
ltol3(cp, lp, n)
char	*cp;
long	*lp;
int	n;
{
	register i;
	register char *a, *b;

	a = cp;
	b = (char *)lp;
	for(i=0; i < n; ++i) {
		b++;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
	}
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef l3tol
#pragma _HP_SECONDARY_DEF _l3tol l3tol
#define l3tol _l3tol
#endif

void
l3tol(lp, cp, n)
long	*lp;
char	*cp;
int	n;
{
	register i;
	register char *a, *b;

	a = (char *)lp;
	b = cp;
	for(i=0; i < n; ++i) {
		*a++ = 0;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
	}
}
