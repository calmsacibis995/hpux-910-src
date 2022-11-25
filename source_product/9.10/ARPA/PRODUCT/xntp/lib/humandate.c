/* $Header: humandate.c,v 1.2.109.2 94/10/28 17:32:50 mike Exp $
 * humandate - convert an NTP time to something readable
 */
# include <stdio.h>

# include "ntp_fp.h"
# include "ntp_unixtime.h"
# include "lib_strbuf.h"

# ifdef RS6000
# 	include <time.h>
# endif	/* RS6000 */
# ifdef	_AUX_SOURCE
# 	include <time.h>
# endif	/* _AUX_SOURCE */

char   *
humandate (ntptime)
U_LONG  ntptime;
{
    char   *bp;
    struct tm  *tm;
    U_LONG  sec;
    static char    *months[] =
        {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
    static char    *days[] =
        {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
        };

    LIB_GETBUF (bp);

    sec = ntptime - JAN_1970;
    tm = localtime ((LONG *)&sec);

    (void)sprintf (bp, "%s, %s %2d %4d %2d:%02d:%02d",
	    days[tm -> tm_wday], months[tm -> tm_mon], tm -> tm_mday,
	    1900 + tm -> tm_year, tm -> tm_hour, tm -> tm_min, tm -> tm_sec);

    return bp;
}
