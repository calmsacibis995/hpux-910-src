/* static char *HPUX_ID = "@(#) wcstoul.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <wchar.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <wpi.h>

#ifdef _NAMESPACE_CLEAN
#    undef  wcstoul
#    pragma _HP_SECONDARY_DEF _wcstoul wcstoul
#    define wcstoul _wcstoul
#endif

#define DIGIT(x)	(iswdigit(x) ? (x) - L'0' : ((x) | 32) + 10 - L'a')
#define MBASE		('z' - 'a' + 1 + 10)

/* Note: This code is derived from strtoul.c */

/*****************************************************************************
 *                                                                           *
 *  CONVERT WIDE STRING INTO UNSIGNED LONG INT                               *
 *                                                                           *
 *  INPUTS:      ws            - the string to be converted                  *
 *                                                                           *
 *               base          - base for conversion                         *
 *                                                                           *
 *  OUTPUTS:     the unsigned long int value                                 *
 *               ULONG_MAX on error                                          *
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

/* Note: the overflow test used here assumes unsigned arithmetic is
	done modulo the word size and hence the result of an addition
	that results in overflow will be less than the larger of the
	two operands.  This test is valid for the 300 and 800.
*/

unsigned long
wcstoul(const wchar_t *ws, wchar_t **endptr, int base)
{
	unsigned long val, valprv;
	wchar_t wc;
	int ovf = 0;
	int dig, neg = 0;

#ifdef hp9000s500
	WARNING! following code was copied from strtol.c.
	If the hp9000s500 macro is to be used, it probably
	requires modification.
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
	if (base == 16 && wc == L'0' && isxdigit(ws[2]) &&
	    (ws[1] == L'x' || ws[1] == L'X'))
		wc = *(ws += 2); /* skip over leading "0x" or "0X" */
	val = valprv = DIGIT(wc);
	while ( iswalnum(wc = *++ws) && (dig = DIGIT(wc)) < base ) {
		if (! ovf) {
			val = base * val + dig;
			ovf = ( val < valprv );
			valprv = val;
			}
		}

	if (endptr != (wchar_t **)0)
		*endptr = (wchar_t *)ws;

	if (ovf) {
		errno = ERANGE;
		return(ULONG_MAX);
		}

	return (neg ? -val : val);
}
