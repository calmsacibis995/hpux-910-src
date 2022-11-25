/* $Header: atoint.c,v 1.2.109.4 94/10/28 16:46:42 mike Exp $
 * atoint - convert an ascii string to a signed long, with error checking
 */
# include "ntp_stdlib.h"
# include <ctype.h>

int
atoint (str, ival)
const char *str;
LONG   *ival;
{
    register U_LONG     u;
    register const char    *cp;
    register int    isneg;
    register int    oflow_digit;

    cp = str;

    if (*cp == '-')
        {
	cp++;
	isneg = 1;
	oflow_digit = '8';
        }
    else
        {
	isneg = 0;
	oflow_digit = '7';
        }

    if (*cp == '\0')
	return 0;

    u = 0;
    while (*cp != '\0')
        {
	if (!isdigit (*cp))
	    return 0;
	if (u > 214748364 || (u == 214748364 && *cp > oflow_digit))
	    return 0;		/* overflow */
	u = (u << 3) + (u << 1);
	u += *cp++ - '0';	/* ascii dependent */
        }

    if (isneg)
	*ival = -((LONG)u);
    else
	*ival = (LONG)u;
    return 1;
}
