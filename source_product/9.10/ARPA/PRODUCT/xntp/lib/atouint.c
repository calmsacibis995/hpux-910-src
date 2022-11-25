/* $Header: atouint.c,v 1.2.109.2 94/10/28 16:51:17 mike Exp $
 * atouint - convert an ascii string to an unsigned long, with error checking
 */
# include <ctype.h>
# include "ntp_stdlib.h"

int
atouint (str, uval)
const char *str;
U_LONG *uval;
{
    register U_LONG     u;
    register const char    *cp;

    cp = str;
    if (*cp == '\0')
	return 0;

    u = 0;
    while (*cp != '\0')
        {
	if (!isdigit (*cp))
	    return 0;
	if (u > 429496729 || (u == 429496729 && *cp >= '6'))
	    return 0;		/* overflow */
	u = (u << 3) + (u << 1);
	u += *cp++ - '0';	/* ascii dependent */
        }

    *uval = u;
    return 1;
}
