/* static char *HPUX_ID = "@(#) wcstol.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <wpi.h>

#ifdef _NAMESPACE_CLEAN
#    undef  wcstol
#    pragma _HP_SECONDARY_DEF _wcstol wcstol
#    define wcstol _wcstol
#endif

#define DIGIT(x)	(iswdigit(x) ? (x) - L'0' : ((x) | 32) + 10 - L'a')
#define MBASE		('z' - 'a' + 1 + 10)

/* Note: This code is derived from strtol.c */


/* Note:
	The basic computational statement for this routine is:

		val = base * val - dig;

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

/*****************************************************************************
 *                                                                           *
 *  CONVERT WIDE STRING INTO LONG INT                                        *
 *                                                                           *
 *  INPUTS:      ws            - the string to be converted                  *
 *                                                                           *
 *               base          - base for conversion                         *
 *                                                                           *
 *  OUTPUTS:     the long int value                                          *
 *               LONG_MIN/LONG_MAX on error                                  *
 *                                                                           *
 *               endptr        - pointer to first character not in value     *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

long
wcstol(const wchar_t *ws, wchar_t **endptr, int base)
{
	long val, lmb;
	wchar_t wc;
	int ovf = 0;
	int dig, neg = 0;

#ifdef hp9000s500
	asm("
		pshr	4
		lni	0x800001
		and
		setr	4
	");
#endif /* hp9000s500 */

	if (endptr != (wchar_t **)0)
		*endptr = (wchar_t *)ws; /* in case no number is formed */
	if (!iswalnum(wc = *ws)) {
		while (iswspace(wc))
			wc = *++ws;
		switch (wc) {
		case L'-':
			neg++;
		case L'+': /* fall-through */
			wc = *++ws;
		}
	}
	if (base == 0)
		if (wc != L'0')
			base = 10;
		else if (ws[1] == L'x' || ws[1] == L'X')
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
	if (!iswalnum(wc) || (dig = DIGIT(wc)) >= base) {
		errno = EINVAL;
		return (0); /* no number formed */
		}
	if (base == 16 && wc == L'0' && iswxdigit(ws[2]) &&
	    (ws[1] == L'x' || ws[1] == L'X'))
		wc = *(ws += 2); /* skip over leading "0x" or "0X" */
	while ( wc == L'0' )
		wc = *++ws;	/* skip leading zeros */
	lmb = LONG_MIN / base;
	for (val = 0; iswalnum(wc) && (dig = DIGIT(wc)) < base; wc = *++ws ) {
		/* accumulate neg avoids surprises near LONG_MAX */
		if (! ovf) {
			if ( val < lmb )	/*  overflow on multiply? */
				ovf = 1;
			else {
				val = base * val - dig;
				if ( val >= 0 ) /* overflow on addition? */
					ovf = 1;
				}
			}
		}

	if (endptr != (wchar_t **)0)
		*endptr = (wchar_t *)ws;

	/* check overflow, special case LONG_MIN */
	if (ovf || (val == LONG_MIN && !neg)) {
		errno = ERANGE;
		return(neg ? LONG_MIN : LONG_MAX);
	}

	return (neg ? val : -val);
}
