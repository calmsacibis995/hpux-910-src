/* $Header: clocktime.c,v 1.2.109.2 94/10/28 17:17:57 mike Exp $
 * clocktime - compute the NTP date from a day of year, hour, minute
 *	       and second.
 */
# include "ntp_fp.h"
# include "ntp_unixtime.h"


/*
 * Hacks to avoid excercising the multiplier.  I have no pride.
 */
# define	MULBY10(x)	(((x)<<3) + ((x)<<1))
# define	MULBY60(x)	(((x)<<6) - ((x)<<2))
/* watch overflow */
# define	MULBY24(x)	(((x)<<4) + ((x)<<3))

/*
 * Two days, in seconds.
 */
# define	TWODAYS		(2*24*60*60)

/*
 * We demand that the time be within CLOSETIME seconds of the receive
 * time stamp.  This is about 4 hours, which hopefully should be
 * wide enough to collect most data, while close enough to keep things
 * from getting confused.
 */
# define	CLOSETIME	(4*60*60)


int
clocktime (yday, hour, minute, second, tzoff, rec_ui, yearstart, ts_ui)
int     yday;
int     hour;
int     minute;
int     second;
int     tzoff;
U_LONG  rec_ui;
U_LONG *yearstart;
U_LONG *ts_ui;
{
    register LONG   tmp;
    register U_LONG     date;
    register U_LONG     yst;

    /* 
     * Compute the offset into the year in seconds.  Note that
     * this could come out to be a negative number.
     */
    tmp = (LONG)(MULBY24 ((yday - 1)) + hour + tzoff);
    tmp = MULBY60 (tmp) + (LONG)minute;
    tmp = MULBY60 (tmp) + (LONG)second;

    /* 
     * Initialize yearstart, if necessary.
     */
    yst = *yearstart;
    if (yst == 0)
        {
	yst = calyearstart (rec_ui);
	*yearstart = yst;
        }

    /* 
     * Now the fun begins.  We demand that the received clock time
     * be within CLOSETIME of the receive timestamp, but
     * there is uncertainty about the year the timestamp is in.
     * Use the current year start for the first check, this should
     * work most of the time.
     */
    date = (U_LONG)(tmp + (LONG)yst);
    if (date < (rec_ui + CLOSETIME) &&
	    date > (rec_ui - CLOSETIME))
        {
	*ts_ui = date;
	return 1;
        }

    /* 
     * Trouble.  Next check is to see if the year rolled over and, if
     * so, try again with the new year's start.
     */
    yst = calyearstart (rec_ui);
    if (yst != *yearstart)
        {
	date = (U_LONG)((LONG)yst + tmp);
	*ts_ui = date;
	if (date < (rec_ui + CLOSETIME) &&
		date > (rec_ui - CLOSETIME))
	    {
	    *yearstart = yst;
	    return 1;
	    }
        }

    /* 
     * Here we know the year start matches the current system
     * time.  One remaining possibility is that the time code
     * is in the year previous to that of the system time.  This
     * is only worth checking if the receive timestamp is less
     * than a couple of days into the new year.
     */
    if ((rec_ui - yst) < TWODAYS)
        {
	yst = calyearstart (yst - TWODAYS);
	if (yst != *yearstart)
	    {
	    date = (U_LONG)(tmp + (LONG)yst);
	    if (date < (rec_ui + CLOSETIME) &&
		    date > (rec_ui - CLOSETIME))
	        {
		*yearstart = yst;
		*ts_ui = date;
		return 1;
	        }
	    }
        }

    /* 
     * One last possibility is that the time stamp is in the year
     * following the year the system is in.  Try this one before
     * giving up.
     */
    yst = calyearstart (rec_ui + TWODAYS);
    if (yst != *yearstart)
        {
	date = (U_LONG)((LONG)yst + tmp);
	if (date < (rec_ui + CLOSETIME) &&
		date > (rec_ui - CLOSETIME))
	    {
	    *yearstart = yst;
	    *ts_ui = date;
	    return 1;
	    }
        }

    /* 
     * Give it up.
     */
    return 0;
}
