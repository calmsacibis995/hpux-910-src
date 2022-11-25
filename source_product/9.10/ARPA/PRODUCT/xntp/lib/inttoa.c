/* $Header: inttoa.c,v 1.2.109.2 94/10/28 17:34:14 mike Exp $
 * inttoa - return an asciized signed integer
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_stdlib.h"

char   *
inttoa (ival)
LONG    ival;
{
    register char  *buf;

    LIB_GETBUF (buf);

    (void)sprintf (buf, "%ld", ival);
    return buf;
}
