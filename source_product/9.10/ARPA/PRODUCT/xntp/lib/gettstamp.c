/* $Header: gettstamp.c,v 1.2.109.2 94/10/28 17:29:01 mike Exp $
 * gettstamp - return the system time in timestamp format
 */
# include <stdio.h>
# include <sys/time.h>

# include "ntp_fp.h"
# include "ntp_unixtime.h"

void
gettstamp (ts)
l_fp   *ts;
{
    struct timeval  tv;

    /* 
     * Quickly get the time of day and convert it
     */
    (void)gettimeofday (&tv, (struct timezone  *)NULL);
    if (tv.tv_usec >= 1000000)
        {			/* bum solaris */
	tv.tv_usec -= 1000000;
	tv.tv_sec++;
        }
    TVTOTS (&tv, ts);
    ts -> l_uf += TS_ROUNDBIT;	/* guaranteed not to
				   overflow */
    ts -> l_ui += JAN_1970;
    ts -> l_uf &= TS_MASK;
}
