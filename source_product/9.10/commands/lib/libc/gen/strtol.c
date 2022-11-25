/* @(#) $Revision: 70.2 $ */      
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#define MBASE		('z' - 'a' + 1 + 10)
extern	int	errno;

/* Note:
	The basic computational statement for this routine is:

		val = base * val - xx;

	in which the multiplication or the subtraction can cause overflow.
	Different overflow tests are used for these two cases.

	The subtraction overflow test used assumes that overflow will cause
	a carry into the sign bit and hence a change in the expected sign
	of the result.  Example: LONG_MIN - 1 = LONG_MAX.  This test is
	valid for the 300 and 800.

	A non-negative string that converts to LONG_MIN must be handled
	as a special case since it will be converted without overflow
	detection but it cannot be represented as a (positive) long.

*/
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strtol strtol
#define strtol _strtol
#endif

long
strtol(str, ptr, base)
register char *str;
char **ptr;
register int base;
{
	register long val, lmb;
	register int c, xx;
	register int ovf = 0;
	int neg = 0;

#ifdef hp9000s500
	asm("
		pshr	4
		lni	0x800001
		and
		setr	4
	");
#endif hp9000s500

	if (ptr != (char **)0)
		*ptr = str; /* in case no number is formed */
	if (!isalnum(c = *str)) {
		while (isspace(c))
			c = *++str;
		switch (c) {
		case '-':
			neg++;
		case '+': /* fall-through */
			c = *++str;
		}
	}
	if (base == 0)
		if (c != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	if (base < 2 || base > MBASE) {
		errno = EINVAL;
		return (0); /* base is invalid */
		}
	/*
	 * for any base > 10, the digits incrementally following
	 *	9 are assumed to be "abc...z" or "ABC...Z"
	 */
	if (!isalnum(c) || (xx = __ASCII_to_digit(c)) >= base)
		return (0); /* no number formed */
	if (base == 16 && c == '0' && isxdigit(str[2]) &&
	    (str[1] == 'x' || str[1] == 'X'))
		c = *(str += 2); /* skip over leading "0x" or "0X" */
	while ( c == '0' )
		c = *++str;	/* skip leading zeros */
	lmb = LONG_MIN / base;
	for (val = 0; isalnum(c) && (xx = __ASCII_to_digit(c)) < base; c = *++str ) {
		/* accumulate neg avoids surprises near LONG_MAX */
		if (! ovf) {
			if ( val < lmb )	/*  overflow on multiply? */
				ovf = 1;
			else {
				val = base * val - xx;
				if ( val >= 0 ) /* overflow on addition? */
					ovf = 1;
				}
			}
		}

	if (ptr != (char **)0)
		*ptr = str;

	/* check overflow, special case LONG_MIN */
	if (ovf || (val == LONG_MIN && !neg)) {
		errno = ERANGE;
		return(neg ? LONG_MIN : LONG_MAX);
	}

	return (neg ? val : -val);
}
