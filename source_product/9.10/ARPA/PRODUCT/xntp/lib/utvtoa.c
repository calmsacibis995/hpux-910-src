/* $Header: utvtoa.c,v 1.2.109.2 94/10/31 11:01:55 mike Exp $
 * utvtoa - return an asciized representation of an unsigned struct timeval
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_stdlib.h"
# include <sys/time.h>

char   *
utvtoa (tv)
struct timeval *tv;
{
    register char  *buf;

    LIB_GETBUF (buf);

    (void)sprintf (buf, "%lu.%06lu", (U_LONG)tv -> tv_sec,
	    (U_LONG)tv -> tv_usec);
    return buf;
}
