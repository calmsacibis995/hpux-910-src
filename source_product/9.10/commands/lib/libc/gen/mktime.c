/* @(#) $Revision: 70.1 $ */     
/* LINTLIBRARY */
/*
 *	mktime()
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define localtime _localtime
#define tzset _tzset
#define timezone __timezone
#define mktime _mktime
#endif

#include <time.h>
#include <values.h>
#include <errno.h>

/*
 * mktime() converts a tm structure into a time_t value.  The original
 * values of tm_wday and tm_yday are ignored and the other components of
 * the tm structure are not restricted to their normal ranges.  A positive
 * or zero value for tm_isdst causes mktime initially to presume that
 * Daylight Saving Time, respectively, is or is not in effect for the
 * specified time.  A negative value for tm_isdst causes mktime to attempt
 * to determine whether Daylight Saving Time is in effect.  On successful
 * completion, all the components of the tm structure are set to represent
 * the specified calendar time but with their values forced to their normal
 * ranges.  If the time cannot be represented, mktime sets errno to ERANGE
 * and returns (time_t)-1.
 */

static short mon_sum[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define MIN_TIME_T      ((time_t)1 << BITS(time_t) - 1)			/* min time_t representation	*/
#define MAX_TIME_T      (~MIN_TIME_T)					/* max time_t representation	*/

time_t	timet();							/* defined below		*/
static int error;							/* error flag set by timet()	*/
extern	time_t __sdt_dst;						/* courtesy of localtime()	*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mktime
#pragma _HP_SECONDARY_DEF _mktime mktime
#define mktime _mktime
#endif

time_t mktime(tp)
struct tm *tp;
{
	struct	tm *lp;							/* ptr to localtime() return	*/
	struct	tm work_tm;						/* working copy of *tp		*/

	time_t	tim1, dst;
	int	hour = tp->tm_hour;
	int	min = tp->tm_min;
	int	sec = tp->tm_sec;
	int	year = tp->tm_year;					/* algorithm OK /wo adding 1900	*/
	int	mon = tp->tm_mon;
	int	yday = tp->tm_mday - 1;					/* -1 because 1st day == 0	*/

	work_tm = *tp;							/* just in case tp points to	*/
									/* the same struct returned by	*/
									/* a call to localtime()	*/

	(void)tzset();							/* initialize timezone		*/

	year += mon / 12;						/* normalize months		*/
	mon %= 12;
	if (mon < 0) {
		year--;
		mon += 12;
	}
	work_tm.tm_mon = mon;

	/* convert remaining months to days of year */
	yday += mon_sum[mon] + (mon > 1 && (year % 4 == 0));		/* only works year=[1,199]	*/
									/* (actual years [1901,2099])	*/

	min += sec / 60;						/* normalize seconds		*/
	sec %= 60;
	if (sec < 0) {
		min--;
		sec += 60;
	}
	work_tm.tm_sec = sec;

	hour += min / 60;						/* normalize minutes		*/
	min %= 60;
	if (min < 0) {
		hour--;
		min += 60;
	}
	work_tm.tm_min = min;

	yday += hour / 24;						/* normalize hours		*/
	hour %= 24;
	if (hour < 0) {
		yday--;
		hour += 24;
	}
	work_tm.tm_hour = hour;

	/*
	 * quit if the date is more than 2 years outside of the representable
	 * range (2 years allows for inaccurate leap year counting and for
	 * ignoring the month, hour, min and sec values)
	 */
	if (year + yday/365.25 + 2 < 70+MIN_TIME_T/(86400*365.25) ||	/* min 1901/12/13 20:45:52 CUT0	*/
	    year + yday/365.25 - 2 > 70+MAX_TIME_T/(86400*365.25)) {	/* max 2038/01/19 03:14:07 CUT0	*/
		errno = ERANGE;
		return (time_t)-1;
	}

	/*
	 * normalize day of year by first converting year + yday to days
	 * since Jan 1, 1900 and then converting that back to the proper
	 * year and day of year
	 */
	year += 1899;
	yday += (146097 * (year/100))/4 + (1461 * (year%100))/4 - 693595;	/* valid [0001,9999]	*/
	work_tm.tm_year = year = (yday + 1) / 365.25;			/* only works for [1901,2099]	*/
	work_tm.tm_yday = yday - ((1461 * (year-1))/4 + 365);		/* only works for [1901,2099]	*/

	error = 0;
	tim1 = timet(&work_tm);						/* convert to a time_t value	*/
	if (error)
		return (time_t)-1;
	lp = localtime(&tim1);						/* convert new time_t value	*/
									/* back to a tm struct		*/

	if (work_tm.tm_isdst < 0)					/* not sure about DST status?	*/
		work_tm.tm_isdst = !(work_tm.tm_hour == lp->tm_hour &&	/*	if tp==lp, assume SDT	*/
				work_tm.tm_min == lp->tm_min &&		/*	otherwise assume DST	*/
				work_tm.tm_sec == lp->tm_sec);

	if (!work_tm.tm_isdst) {					/* if not in a DST period then	*/
		*tp = *lp;						/*	we have what we want	*/

		  return tim1;
	}

	/*
	 * In a DST period - tim1 contains the time we want plus the DST adjustment
	 * factor which must be subtracted out.  If lp is also in the DST period,
	 * the difference between timet(localtime(tim1)) and tim1 is the amount of
	 * the DST factor.  This algorithm works even for (hypothetical) time zone
	 * rule sets in which the amount of DST varies from year to year (all
	 * countries currently supported use a constant DST factor of one hour).
	 * If lp is not in the DST period, then we use a DST factor calculated for
	 * us by localtime() from a comparison of two arbitrary SDT and DST periods
	 * for the current time zone.  This factor will not necessarily be correct
	 * if the time zone has varying DST factors.  But if they vary and we're
	 * not currently in one of them, this is the best we can do since we have
	 * no basis upon which to decide which is the "correct" one.
	 */
	if (lp->tm_isdst)
		dst = timet(lp) - tim1;
	else
		dst = __sdt_dst;

	if ((dst > 0 && tim1 < 0 && tim1 - MIN_TIME_T < dst) ||		/* going to exceed time_t	*/
	    (dst < 0 && tim1 > 0 && tim1 - MAX_TIME_T > dst)) {		/* range?			*/
		errno = ERANGE;
		return (time_t)-1;
	}

	tim1 -= dst;
	*tp = *localtime(&tim1);
return tim1;
}


/*
 * Convert normalized tm structure values to a time_t value.
 */
static time_t timet(tmptr)
struct tm *tmptr;
{
	int day;
	time_t time1, time2;

	day = tmptr->tm_yday;

	day += (1461 * (tmptr->tm_year - 1))/4 - 25202;			/* convert years to days since	*/
									/* Jan 1, 1970			*/
									/* (works in range [1,199])	*/

	/* error if too many days to fit in a time_t representation */
	if ((day > 0 && MAX_TIME_T/86400 + 1 < day) || (day < 0 && MIN_TIME_T/86400 - 1 > day)) {
		error = errno = ERANGE;
		return (time_t)0;
	}

	/* these two time pieces will individually fit into a time_t representation */
	if (day == MIN_TIME_T/86400 - 1) {		/* have to shift a day if right on the boundary	*/
		time1 = (day + 1) * 86400;
		time2 = (tmptr->tm_hour * 60 + tmptr->tm_min) * 60 + tmptr->tm_sec + timezone - 86400;
	} else if (day == MAX_TIME_T/86400 + 1) {	/* opposite shift for other end of range	*/
		time1 = (day - 1) * 86400;
		time2 = (tmptr->tm_hour * 60 + tmptr->tm_min) * 60 + tmptr->tm_sec + timezone + 86400;
	} else {					/* normal situation				*/
		time1 = day * 86400;
		time2 = (tmptr->tm_hour * 60 + tmptr->tm_min) * 60 + tmptr->tm_sec + timezone;
	}

	/* ensure the two parts will fit into a time_t when added together */
	if ((time1 > 0 && MAX_TIME_T - time1 < time2) || (time1 < 0 && MIN_TIME_T - time1 > time2)) {
		error = errno = ERANGE;
		return (time_t)0;
	}

	return time1 + time2;
}


/************************************************************************************************************

Some information that may help in understanding the above code:

Given a tm structure that contains only tm_hour and tm_isdst, that the
relationship between a time_t value and tm_hour (exclusive of DST aberrations)
is:
	time_t = 10 * tm_hour

and that a particular day appears as (i.e., DST offset is 2 hours):

time_t:		110	120	130	140	150	160	170	180	190	200	210	220

tm_hour:	 11	 12	 13	 16	 17	 18	 19	 20	 21	 20	 21	 22
tm_isdst:	  0	  0	  0	  1	  1	  1	  1	  1	  1	  0	  0	  0

The input cases that need to be handled and their outputs are:

	------INPUT-------	------------OUTPUT------------
	tm_hour : tm_isdst	tm_hour : tm_isdst	time_t

 1)	     12 : 0		     12 : 0		120		in SDT period
 2)	     12 : 1		     10 : 0		100
 3)	     12 : -1		     12 : 0		120

 4)	     14 : 0		     16 : 1		140		SDT -> DST transition
 5)	     14 : 1		     12 : 0		120
 6)	     14 : -1		     12 : 0		120

 7)	     18 : 0		     20 : 1		180		in DST period
 8)	     18 : 1		     18 : 1		160
 9)	     18 : -1		     18 : 1		160

10)	     20 : 0		     20 : 0		200		DST -> SDT transition
11)	     20 : 1		     20 : 1		180
12)	     20 : -1		     20 : 0		200

 1)  To ensure: mktime(localtime(t)) == t
 2)  Caller specified DST value for SDT period--convert value to SDT.
 3)  Arbitrary choice between 1) & 2).  Picked 1) assuming caller is using correct DST/SDT period.
 4)  Probably originated from caller wanting to know what is 1 hour after "13 : 0".
 5)  Probably originated from caller wanting to know what is 2 hours before "16 : 1".
 6)  Arbitrary choice between 4) & 5).  Picked 5) because it resulted in simpler code.
 7)  Caller specified SDT value for DST period--convert value to DST.
 8)  To ensure: mktime(localtime(t)) == t
 9)  Arbitrary choice between 7) & 8).  Picked 8) assuming caller is using correct DST/SDT period.
10)  To ensure: mktime(localtime(t)) == t
11)  To ensure: mktime(localtime(t)) == t
12)  Arbitrary choice between 10) & 11).  Picked 10) because it resulted in simpler code.

/***********************************************************************************************************/
