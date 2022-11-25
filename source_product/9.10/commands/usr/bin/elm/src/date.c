/**			date.c				**/

/*
 *  @(#) $Revision: 64.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Return the current date and time in a readable format! 
 *  also returns an ARPA RFC-822 format date...           
 */


#include <time.h>
#include <ctype.h>

#include "headers.h"


#define MONTHS_IN_YEAR	11	/* 0-11 equals 12 months! */
#define FEB		 1	/* 0 = January 		  */
#define DAYS_IN_LEAP_FEB 29	/* leap year only 	  */

#define ampm(n)		(n > 12? n - 12 : n)
#define am_or_pm(n)	(n > 11? (n > 23? "am" : "pm") : "am")
#define leapyear(year)	((year % 4 == 0) && (year % 100 != 0))


char           *arpa_dayname[] = {"Sun", "Mon", "Tue", "Wed", "Thu",
				  "Fri", "Sat", ""};

char           *arpa_monname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""};

int             days_in_month[] = {31, 28, 31, 30, 31, 30,
				   31, 31, 30, 31, 30, 31, -1};

extern char     *tzname[];

char		*shift_lower(), 
		*strcpy(), 
		*strncpy();


char 
*get_arpa_date()

{
	/*
	 *  returns an ARPA standard date.  The format for the date
	 *  according to DARPA document RFC-822 is exemplified by;
 	 * 
	 *     	      Mon, 12 Aug 85 6:29:08 MST
 	 * 
	 */


	struct tm      *the_time,	/* Time structure, see CTIME(3C) */
	               *localtime();
	long            junk,	 	/* time in seconds....		 */
	                time();

	junk = time( (long *) 0 ); 	/* this must be here for it to work! */

	the_time = localtime( &junk );

	sprintf( arpa_date, "%s, %d %s %d %d:%02d:%02d %s",
		 arpa_dayname[the_time->tm_wday],
		 the_time->tm_mday % 32,
		 arpa_monname[the_time->tm_mon],
		 the_time->tm_year % 100,
		 the_time->tm_hour % 24,
		 the_time->tm_min % 61,
		 the_time->tm_sec % 61,
		 tzname[the_time->tm_isdst] );

	return ( (char *) arpa_date );
}


int 
days_ahead( days, buffer )

	int             days;
	char           *buffer;

{
	/*
	 *  return in buffer the date (Day, Mon Day, Year) of the date
	 *  'days' days after today.  If we've defined X.400, then
	 *  return in the buffer DD MM YY Day-of-week instead, since
	 *  we have the 4x_submit program to rewrite it in the appropriate
	 *  format for either of the two types of mail (e.g. Unix or X400)
	 */


	struct tm      *the_time,	/* Time structure, see CTIME(3C) */
	               *localtime();
	long            junk,	 	/* time in seconds....		 */
	                time();


	junk = time((long *) 0);	/* this must be here for it to work! */
	the_time = localtime(&junk);

	/*
	 * increment the day of the week 
	 */

	the_time->tm_wday = (the_time->tm_wday + days) % 7;

	/*
	 * the day of the month... 
	 */

	the_time->tm_mday += days;

	if ( the_time->tm_mday > days_in_month[the_time->tm_mon] ) {
		if ( the_time->tm_mon == FEB && leapyear(the_time->tm_year) ) {
			if ( the_time->tm_mday > DAYS_IN_LEAP_FEB ) {
				the_time->tm_mday -= days_in_month[the_time->tm_mon];
				the_time->tm_mon += 1;
			}

		} else {
			the_time->tm_mday -= days_in_month[the_time->tm_mon];
			the_time->tm_mon += 1;
		}
	}
		
	/*
	 * check the month of the year 
	 */

	if ( the_time->tm_mon > MONTHS_IN_YEAR ) {
		the_time->tm_mon = the_time->tm_mon%12;
		the_time->tm_year += 1;
	}

	/*
	 * now, finally, build the actual date string 
	 */

	sprintf( buffer, "%s, %d %s %d",
		 arpa_dayname[the_time->tm_wday],
		 the_time->tm_mday % 32,
		 arpa_monname[the_time->tm_mon],
		 the_time->tm_year % 100);
}


int
valid_date( day, /* mon, */ year )

	char            *day, 
			/* *mon, */ 
			*year;
{
	/*
	 *  Validate the given date - returns TRUE iff the date
	 *  handed is reasonable and valid.  Ignore month param, okay? 
	 */


	register int    daynum, 
			yearnum;


	daynum = atoi( day );
	yearnum = atoi( year );

	if ( daynum < 1 || daynum > 31 ) {
		dprint( 3, (debugfile,
			"Error: day %d is obviously wrong! (valid_date)\n",
			daynum) );
		return ( 0 );
	}

	if ( yearnum < 1 || (yearnum > 100 && yearnum < 1900) 
			 || yearnum > 2000 ) {
		dprint( 3, (debugfile,
			"Error: year %d is obviously wrong! (valid_date)\n",
		        yearnum) );
		return ( 0 );
	}
	
	return ( 1 );
}


int 
fix_date( entry )

	struct header_rec *entry;

{
	/*
	 *  This routine will 'fix' the date entry for the specified
	 *  message.  This consists of 1) adjusting the year to 0-99
	 *  and 2) altering time from HH:MM:SS to HH:MM am|pm 
	 */


	if ( atoi(entry->year) > 99 )
		sprintf( entry->year, "%d", atoi(entry->year) - 1900 );

	fix_time( entry->time );
}


int 
fix_time( timestring )

	char           *timestring;

{
	/*
	 *  Timestring in format HH:MM:SS (24 hour time).  This routine
	 *  will fix it to display as: HH:MM [am|pm] 
	 */


	int             hour, 
			minute;


	sscanf( timestring, "%d:%d", &hour, &minute );

	if ( hour < 1 || hour == 24 )
		sprintf( timestring, "12:%02d (midnight)", minute );
	else if ( hour < 12 )
		sprintf( timestring, "%d:%02d am", hour, minute );
	else if ( hour == 12 )
		sprintf( timestring, "%d:%02d (noon)", hour, minute );
	else if ( hour < 24 )
		sprintf( timestring, "%d:%02d pm", hour - 12, minute );
}


int
compare_dates( rec1, rec2 )

	struct header_rec 	*rec1, 
				*rec2;

{
	/*
	 *  This function works similarly to the "strcmp" function, but
	 *  has lots of knowledge about the internal date format...
	 *  Apologies to those who "know a better way"...
	 */


	int             month1, day1, year1, hour1, minute1, 
			month2, day2, year2, hour2, minute2;


	year1 = atoi( rec1->year );
	year2 = atoi( rec2->year );

	if ( year1 > 100 )
		year1 -= 1900;

	if ( year2 > 100 )
		year2 -= 1900;

	if ( year1 != year2 )
		return ( year1 - year2 );

	/*
	 * And HERE's where the performance of this sort dies... 
	 */

	month1 = month_number( rec1->month );	
	month2 = month_number( rec2->month );

	if ( month1 == -1 )
		dprint( 2, (debugfile,
		        "month_number failed on month '%s'\n", rec1->month) );

	if ( month2 == -1 )
		dprint( 2, (debugfile,
		        "month_number failed on month '%s'\n", rec2->month) );

	if ( month1 != month2 )
		return ( month1 - month2 );

	/*
	 * back and cruisin' now, though... 
	 */

	day1 = atoi( rec1->day );	/* unfortunately, 2 is greater than 19  */
	day2 = atoi( rec2->day );	/* on a dump string-only compare...     */

	if ( day1 != day2 )
		return ( day1 - day2 );

	/*
	 * we're really slowing down now... 
	 */

	minute1 = minute2 = -1;

	sscanf( rec1->time, "%d:%d", &hour1, &minute1 );
	sscanf( rec2->time, "%d:%d", &hour2, &minute2 );

	/*
	 * did we get the time?  If not, try again 
	 */

	if ( minute1 < 0 )
		sscanf( rec1->time, "%2d%2d", &hour1, &minute1 );

	if ( minute2 < 0 )
		sscanf( rec2->time, "%2d%2d", &hour2, &minute2 );

	/*
	 *  deal with am/pm, if present... 
	 */

	if ( strlen(rec1->time) > 3 )
		if ( rec1->time[strlen(rec1->time) - 2] == 'p' )
			hour1 += 12;

	if ( strlen(rec2->time) > 3 )
		if ( rec2->time[strlen(rec2->time) - 2] == 'p' )
			hour2 += 12;

	if ( hour1 != hour2 )
		return ( hour1 - hour2 );

	return ( minute1 - minute2 );		/* ignore seconds... */
}


int
compare_parsed_dates( rec1, rec2 )

	struct date_rec rec1, 
			rec2;

{
	/*
	 *  This function is very similar to the compare_dates
	 *  function but assumes that the two record structures
	 *  are already parsed and stored in "date_rec" format.
	 */


	if ( rec1.year != rec2.year )
		return ( rec1.year - rec2.year );

	if ( rec1.month != rec2.month )
		return ( rec1.month - rec2.month );

	if ( rec1.day != rec2.day )
		return ( rec1.day - rec2.day );

	if ( rec1.hour != rec2.hour )
		return ( rec1.hour - rec2.hour );

	return ( rec1.minute - rec2.minute );	/* ignore seconds... */
}


int
month_number(name)

	char           *name;

{
	/*
	 *  return the month number given the month name... 
	 */


	char            ch;


	switch ( (char)tolower((int)name[0]) ) {
	case 'a':
		if ( (ch = (char)tolower((int)name[1])) == 'p' )
			return ( APRIL );
		else if ( ch == 'u' )
			return ( AUGUST );
		else
			return ( -1 );		/* error! */

	case 'd':
		return ( DECEMBER );
	case 'f':
		return ( FEBRUARY );
	case 'j':
		if ( (ch = (char)tolower((int)name[1])) == 'a' )
			return ( JANUARY );
		else if ( ch == 'u' ) {
			if ( (ch = (char)tolower((int)name[2])) == 'n' )
				return ( JUNE );
			else if ( ch == 'l' )
				return ( JULY );
			else
				return ( -1 );	/* error! */
		} else
			return ( -1 );		/* error */
	case 'm':
		if ( (ch = (char)tolower((int)name[2])) == 'r' )
			return ( MARCH );
		else if ( ch == 'y' )
			return ( MAY );
		else
			return ( -1 );		/* error! */
	case 'n':
		return ( NOVEMBER );
	case 'o':
		return ( OCTOBER );
	case 's':
		return ( SEPTEMBER );
	default:
		return ( -1 );
	}
}


#ifdef SITE_HIDING

char 
*get_ctime_date()

{
	/*
	 *  returns a ctime() format date, but a few minutes in the 
	 *  past...(more cunningness to implement hidden sites) 
	 */


	struct tm      *the_time,	/* Time structure, see CTIME(3C) */
	               *localtime();
	long            junk,		/* time in seconds....		 */
	                time();


	junk = time( (long *) 0 );	/* this must be here for it to work! */
	the_time = localtime( &junk );

	sprintf( ctime_date, "%s %s %d %02d:%02d:%02d %d",
		 arpa_dayname[the_time->tm_wday],
		 arpa_monname[the_time->tm_mon],
		 the_time->tm_mday % 32,
		 min(the_time->tm_hour % 24, (rand() % 24)),
	     	 min(abs(the_time->tm_min % 61 - (rand() % 60)), (rand() % 60)),
	     	 min(abs(the_time->tm_sec % 61 - (rand() % 60)), (rand() % 60)),
		 the_time->tm_year % 100 + 1900 );

	return ( (char *) ctime_date );
}

#endif
