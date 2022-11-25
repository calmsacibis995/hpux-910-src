/* $Header: uinttoa.c,v 1.2.109.2 94/10/31 10:58:03 mike Exp $
 * uinttoa - return an asciized unsigned integer
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_stdlib.h"

char   *
uinttoa (uval)
U_LONG  uval;
{
    register char  *buf;

    LIB_GETBUF (buf);

    (void)sprintf (buf, "%lu", uval);
    return buf;
}
