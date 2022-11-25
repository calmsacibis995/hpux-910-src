/* $Header: hextoint.c,v 1.2.109.2 94/10/28 17:29:48 mike Exp $
 * hextoint - convert an ascii string in hex to an unsigned
 *	      long, with error checking
 */
# include "ntp_stdlib.h"
# include <ctype.h>

int
hextoint (str, ival)
const char *str;
U_LONG *ival;
{
    register U_LONG     u;
    register const char    *cp;

    cp = str;

    if (*cp == '\0')
	return 0;

    u = 0;
    while (*cp != '\0')
        {
	if (!isxdigit (*cp))
	    return 0;
	if (u >= 0x10000000)
	    return 0;		/* overflow */
	u <<= 4;
	if (*cp <= '9')		/* very ascii dependent */
	    u += *cp++ - '0';
	else if (*cp >= 'a')
	    u += *cp++ - 'a' + 10;
	else
	    u += *cp++ - 'A' + 10;
        }
    *ival = u;
    return 1;
}
