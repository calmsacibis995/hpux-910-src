/* @(#) $Revision: 70.1 $ */     

/*LINTLIBRARY*/
/*
 * gcvt  - Floating output conversion to
 *
 * pleasant-looking string.
 */

#ifdef _NAMESPACE_CLEAN
#define strlen _strlen
#define ecvt _ecvt
#define gcvt _gcvt
#endif /* _NAMESPACE_CLEAN */

#ifdef NLS16	
#include <nl_ctype.h>
#else /* NLS16 */
#define ADVANCE(p)	(p++)
#define FIRSTof2(c)	(0)
#define CHARADV(p)	(*p++)
#define CHARAT(p)	(*p)
#define PCHAR(c, p)	(*p = c)
#define PCHARADV(c, p)	(*p++ = c)
#define SECof2(c)	(0)
#endif /* NLS16 */

#ifdef NLS

extern int _nl_radix;

/*   nl_rdxcvt() returns the string position which was converted, or
*    NULL if no position changed.
*/

static char *nl_rdxcvt(str, old, new)
register char *str;
register int old;
register int new;
{
    register char *halt   = str + strlen(str);
						
    while (str < halt) {
	if (CHARAT(str) == old) {
	    PCHAR(new, str);
	    return(str); 
	}
	ADVANCE(str);
    }
    return((char *) 0);
}
#endif /* NLS */


extern char *ecvt();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef gcvt
#pragma _HP_SECONDARY_DEF _gcvt gcvt
#define gcvt _gcvt
#endif /* _NAMESPACE_CLEAN */

#ifdef NLS

char *gcvt(number, ndigit, buf) double number; int ndigit; char *buf;
{
    	char *str, *xgcvt();

	str = xgcvt(number, ndigit, buf);
	if (_nl_radix != '.')
	    (void) nl_rdxcvt(str, '.', _nl_radix);
	return(str);
}

#endif /* NLS */

#ifdef NLS
static char *xgcvt(number,ndigit,buf) double number; int ndigit; char *buf;
#else /* NLS */
char *gcvt(number,ndigit,buf) double number; int ndigit; char *buf;
#endif /* NLS */

{
	int sign, decpt;
	register char *p1, *p2 = buf;
	register int i;

	p1 = ecvt(number, ndigit, &decpt, &sign);
	if (sign)
		*p2++ = '-';
	if (decpt > ndigit || decpt <= -4) {	/* E-style */
		decpt--;
		*p2++ = *p1++;
		*p2++ = '.';
		for (i = 1; i < ndigit; i++)
			*p2++ = *p1++;
		while (p2[-1] == '0')
			p2--;
		if (p2[-1] == '.')
			p2--;
		*p2++ = 'e';
		if (decpt < 0) {
			decpt = -decpt;
			*p2++ = '-';
		} else
			*p2++ = '+';
		for (i = 1000; i != 0; i /= 10) /* 3B or CRAY, for example */
			if (i <= decpt || i <= 10) /* force 2 digits */
				*p2++ = (decpt / i) % 10 + '0';
	} else {
		if (decpt <= 0) {
			*p2++ = '0';
			*p2++ = '.';
			while (decpt < 0) {
				decpt++;
				*p2++ = '0';
			}
		}
		for (i = 1; i <= ndigit; i++) {
			*p2++ = *p1++;
			if (i == decpt)
				*p2++ = '.';
		}
		if (ndigit < decpt) {
			while (ndigit++ < decpt)
				*p2++ = '0';
			*p2++ = '.';
		}
		while (*--p2 == '0' && p2 > buf)
			;
		if (*p2 != '.')
			p2++;
	}
	*p2 = '\0';
	return (buf);
}

