


#include <stdio.h>
#include <time.h>



/*
 *	return the system julian date in a short.
 *
 *	Bits	Contents
 *	----------------
 *	0 - 6	Year of Century (1900 only!!!)
 *	7 - 15	Day of Year
 */
unsigned short calendar()
{
	long	sys_clock, time();
	struct tm *localtime(), *ltim;

	sys_clock = time((long *)0);

	ltim = localtime(&sys_clock);

	return ((unsigned short)((ltim->tm_year << 9) | ltim->tm_yday + 1));
}




/*
 *	return the system time in the following format:
 *
 *	Bits	Contents
 *	----------------
 *	 0 - 7	Hour of Day
 *	 8 - 15	Minute of Hour
 *	16 - 23 Seconds
 *	24 - 32 Tenths of Seconds
 */
unsigned long clock()
{
	long	sys_clock, time();
	struct tm *localtime(), *ltim;
	unsigned short hhmm;		/* hours/minutes */
	unsigned short sstt;		/* seconds/tics  */

	sys_clock = time((long *)0);

	ltim = localtime(&sys_clock);

	hhmm = (ltim->tm_hour << 8) | ltim->tm_min;
	sstt = (ltim->tm_sec << 8) | 0;

	return ( (unsigned long)((hhmm << 16) | sstt ));
}
