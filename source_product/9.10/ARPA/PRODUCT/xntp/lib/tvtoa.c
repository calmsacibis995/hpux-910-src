/* $Header: tvtoa.c,v 1.2.109.2 94/10/31 10:56:00 mike Exp $
 * tvtoa - return an asciized representation of a struct timeval
 */
# include <stdio.h>

# include "lib_strbuf.h"
# include "ntp_stdlib.h"
# include <sys/time.h>

char   *
tvtoa (tv)
struct timeval *tv;
{
    register char  *buf;
    register U_LONG     sec;
    register U_LONG     usec;
    register int    isneg;

    if (tv -> tv_sec < 0 || tv -> tv_usec < 0)
        {
	sec = -tv -> tv_sec;
	usec = -tv -> tv_usec;
	isneg = 1;
        }
    else
        {
	sec = tv -> tv_sec;
	usec = tv -> tv_usec;
	isneg = 0;
        }

    LIB_GETBUF (buf);

    (void)sprintf (buf, "%s%lu.%06lu", (isneg ? "-" : ""), sec, usec);
    return buf;
}
