/* @(#) $Revision: 37.2 $ */    

#include "lifdef.h"
#include "global.h"
#include <time.h>


gettime(param)
char *param;
{
	long time();
	struct tm *localtime();
	long now;
	register struct tm *date;
	register struct date_fmt *bcd_date;

	bcd_date = (struct date_fmt *) param;

	now = time((char *) 0);		/* get GMT time */
	date = localtime(&now);		/* convert to local time */
	date->tm_mon++;			/* unix gives 0-11, we want 1-12 */

	bcd_date->year1 = date->tm_year / 10;
	bcd_date->year2 = date->tm_year % 10;
	bcd_date->mon1  = date->tm_mon  / 10;
	bcd_date->mon2  = date->tm_mon  % 10;
	bcd_date->day1  = date->tm_mday / 10;
	bcd_date->day2  = date->tm_mday % 10;
	bcd_date->hour1 = date->tm_hour / 10;
	bcd_date->hour2 = date->tm_hour % 10;
	bcd_date->min1  = date->tm_min  / 10;
	bcd_date->min2  = date->tm_min  % 10;
	bcd_date->sec1  = date->tm_sec  / 10;
	bcd_date->sec2  = date->tm_sec  % 10;
}
