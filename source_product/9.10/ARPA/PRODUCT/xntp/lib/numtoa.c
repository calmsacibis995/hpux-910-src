/* $Header: numtoa.c,v 1.2.109.2 94/10/31 10:30:00 mike Exp $
 * numtoa - return asciized network numbers store in local array space
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_fp.h"

char   *
numtoa (num)
U_LONG  num;
{
    register U_LONG     netnum;
    register char  *buf;

    netnum = ntohl (num);
    LIB_GETBUF (buf);

    (void)sprintf (buf, "%d.%d.%d.%d", (netnum >> 24) & 0xff,
	    (netnum >> 16) & 0xff, (netnum >> 8) & 0xff, netnum & 0xff);

    return buf;
}
