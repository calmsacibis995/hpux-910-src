/*
 * $Header: maketime.c,v 66.4 90/11/14 14:46:24 ssa Exp $
 *
 * MAKETIME		derive 32-bit time value from TM structure.
 *
 * Usage:
 *	long t,maketime();
 *	struct tm *tp;	Pointer to TM structure from <time.h>
 *			NOTE: this must be extended version!!!
 *	t = maketime(tp);
 *
 * Returns:
 *	0 if failure; parameter out of range or nonsensical.
 *	else long time-value.
 * Notes:
 *	This code is quasi-public; it may be used freely in like software.
 *	It is not to be sold, nor used in licensed software without
 *	permission of the author.
 *	For everyone's benefit, please report bugs and improvements!
 * 	Copyright 1981 by Ken Harrenstien, SRI International.
 *	(ARPANET: KLH @ SRI)
 */

#include <sys/types.h>
#include "system.h"
#include "time.h"

int daytb[] = {   /* # days in year thus far, indexed by month (0-12!!) */
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

struct tm *localtime();

long maketime(atm)
struct tm *atm;
{	register struct tm *tp;
	register int i;
	int year, yday, mon, day, hour, min, sec, zone, dst, leap;
	long tres, curtim;
	long time();

	time(&curtim);
	tp = localtime(&curtim);        /* Get breakdowns of current time */
	year = tp->tm_year;		/* Use to set up defaults */
	mon = tp->tm_mon;
	day = tp->tm_mday;


#ifdef DEBUG
printf("first YMD: %d %d %d, T=%ld\n",year,mon,day,tres);
#endif DEBUG
	tp = atm;

	/* First must find date, using specified year, month, day.
	 * If one of these is unspecified, it defaults either to the
	 * current date (if no more global spec was given) or to the
	 * zero-value for that spec (i.e. a more global spec was seen).
	 * Start with year... note 32 bits can only handle 135 years.
	 */
	if(tp->tm_year != TMNULL)
	  {	if((year = tp->tm_year) >= 1900)	/* Allow full yr # */
	  		year -= 1900;			/* by making kosher */
		mon = 0;		/* Since year was given, default */
		day = 1;		/* for remaining specs is zero */
	  }
	if(year < 70 || 70+134 < year )	/* Check range */
		return(0);		/* ERR: year out of range */
	leap = year&03 ? 0 : 1;		/* See if leap year */
	year -= 70;			/* UNIX time starts at 1970 */

	/*
	 * Find day of year.
	 * YDAY is used only if it exists and either the month or day-of-month
	 * is missing.
	 */
	if (tp->tm_yday != TMNULL
	 && (tp->tm_mon == TMNULL || tp->tm_mday == TMNULL))
		yday = tp->tm_yday;
	else
	  {	if(tp->tm_mon  != TMNULL)
		  {	mon = tp->tm_mon;	/* Month was specified */
			day = 1;		/* so set remaining default */
		  }
		if(mon < 0 || 11 < mon) return(0);	/* ERR: bad month */
		if(tp->tm_mday != TMNULL) day = tp->tm_mday;
		if(day < 1
		 || (((daytb[mon+1]-daytb[mon]) < day)
			&& (day!=29 || mon!=1 || !leap) ))
				return(0);		/* ERR: bad day */
		yday = daytb[mon]	/* Add # of days in months so far */
		  + ((leap		/* Leap year, and past Feb?  If */
		      && mon>1)? 1:0)	/* so, add leap day for this year */
		  + day-1;		/* And finally add # days this mon */

                if (tp->tm_yday != TMNULL       /* Confirm that YDAY correct */
                 && tp->tm_yday != yday) return(0);     /* ERR: conflict */
	  }
	if(yday < 0 || (leap?366:365) <= yday)
		return(0);		/* ERR: bad YDAY or maketime bug */

	tres = year*365			/* Get # days of years so far */
		+ ((year+1)>>2)		/* plus # of leap days since 1970 */
		+ yday;			/* and finally add # days this year */

        if((i = tp->tm_wday) != TMNULL) /* Check WDAY if present */
                if(i < 0 || 6 < i       /* Ensure within range */
                  || i != (tres+4)%7)   /* Matches? Jan 1,1970 was Thu = 4 */
                        return(0);      /* ERR: bad WDAY */

#ifdef DEBUG
printf("YMD: %d %d %d, T=%ld\n",year,mon,day,tres);
#endif DEBUG
	/*
	 * Now determine time.  If not given, default to zeros
	 * (since time is always the least global spec)
	 */
	tres *= 86400L;			/* Get # seconds (24*60*60) */
	hour = min = sec = 0;
	if(tp->tm_hour != TMNULL) hour = tp->tm_hour;
	if(tp->tm_min  != TMNULL) min  = tp->tm_min;
	if(tp->tm_sec  != TMNULL) sec  = tp->tm_sec;
	if( min < 0 || 60 <= min
	 || sec < 0 || 60 <= sec) return(0);	/* ERR: MS out of range */
	if(hour < 0 || 24 <= hour)
		if(hour != 24 || (min+sec) !=0)	/* Allow 24:00 */
			return(0);		/* ERR: H out of range */

	/* confirm AM/PM if there */
	switch(tp->tm_ampm)
	  {	case 0: case TMNULL:	/* Ignore these values */
			break;
		case 1:			/* AM */
		case 2:			/* PM */
			if(hour > 12) return(0);  /* ERR: hrs 13-23 bad */
			if(hour ==12) hour = 0;	/* Modulo 12 */
			if(tp->tm_ampm == 2)	/* If PM, then */
				hour += 12;	/*   get 24-hour time */
			break;
		default: return(0);	/* ERR: illegal TM_AMPM value */
	  }

	tres += sec + 60L*(min + 60L*hour);	/* Add in # secs of time */

#ifdef DEBUG
printf("HMS: %d %d %d T=%ld\n",hour,min,sec,tres);
#endif DEBUG

	return(tres);
}
