/* @(#) $Revision: 70.3 $ */      
#ifdef _NAMESPACE_CLEAN
#  ifdef __lint
#  define isalnum _isalnum
#  define isspace _isspace
#  define isxdigit _isxdigit
#  endif /* __lint */
#define strtoul _strtoul
#define toupper __toupper
#endif

#include <ctype.h>
#include <limits.h>
#include <errno.h>
#define MBASE		('z' - 'a' + 1 + 10)
extern	int	errno;

/* Note: the overflow test used here assumes unsigned arithmetic is
	done modulo the word size and hence the result of an addition
	that results in overflow will be less than the larger of the
	two operands.  This test is valid for the 300 and 800.
*/

#ifdef _NAMESPACE_CLEAN
#undef strtoul
#pragma _HP_SECONDARY_DEF _strtoul strtoul
#define strtoul _strtoul
#endif

unsigned long
strtoul(str, ptr, base)
register char *str;
char **ptr;
register int base;
{
	register unsigned long val, valprv;
	register int c;
	register int ovf = 0;
	int xx, neg = 0;

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
	if (base == 16 && c == '0' && isxdigit(str[2]) && toupper(str[1]) == 'X')
	    /* (str[1] == 'x' || str[1] == 'X')) */
		c = *(str += 2); /* skip over leading "0x" or "0X" */
	val = valprv = __ASCII_to_digit(c);
	while ( isalnum(c = *++str) && (xx = __ASCII_to_digit(c)) < base ) {
		if (! ovf) {
			val = base * val + xx;
			ovf = ((val < valprv ) || (((val-xx)/base) != valprv ));
			valprv = val;
			}
		}

	if (ptr != (char **)0)
		*ptr = str;

	if (ovf) {
		errno = ERANGE;
		return(ULONG_MAX);
		}

	return (neg ? -val : val);
}
