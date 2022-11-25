# ifdef	ELIMINATE
/* $Header: tstotod.c,v 1.2.109.2 94/10/31 10:54:07 mike Exp $
 * tstotod - compute calendar time given an NTP timestamp
 */
# 	include <stdio.h>

# 	include "ntp_fp.h"
# 	include "ntp_calendar.h"

void
tstotod (ts, tod)
l_fp   *ts;
struct calendar    *tod;
{
    register U_LONG     cyclesecs;

    cyclesecs = ts.l_ui - MAR_1900;
    /* bump forward to March 1900 */

}
# endif				/* ELIMINATE */
