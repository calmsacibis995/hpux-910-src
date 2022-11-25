/* $Header: refnumtoa.c,v 1.2.109.2 94/10/31 10:51:11 mike Exp $
 * refnumtoa - return asciized refclock addresses stored in local array space
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_fp.h"

char   *
refnumtoa (num)
U_LONG  num;
{
    register U_LONG     netnum;
    register char  *buf;
    register const char    *rclock;

    netnum = ntohl (num);

    LIB_GETBUF (buf);

    rclock = clockname ((int)((netnum >> 8) & 0xff));

    if (rclock != NULL)
        {
	(void)sprintf (buf, "%s(%d)", clockname ((int)((netnum >> 8) & 0xff)), netnum & 0xff);
        }
    else
        {
	(void)sprintf (buf, "REFCLK(%d,%d)", (netnum >> 8) & 0xff, netnum & 0xff);
        }

    return buf;
}
