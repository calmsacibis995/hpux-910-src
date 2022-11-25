

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <nlinfo.h>


almanac(date, err, pYear, pMonth, pDay, pWeekday)
unsigned short	date, err[];
short		*pYear, *pMonth, *pDay, *pWeekday;
{
	struct tm 	xtime;
	unsigned short	nlcrtm();

	err[0] = err[1] = 0;

	xtime.tm_year = (unsigned int)(date >> 9);
	xtime.tm_yday = (unsigned int)(date & 0x1ff);

	if ( (err[0] = nlcrtm(&xtime)) == 0){
	    if (pYear)    *pYear =    (unsigned short)xtime.tm_year;
	    if (pMonth)   *pMonth =   (unsigned short)xtime.tm_mon + 1;
	    if (pDay)     *pDay =     (unsigned short)xtime.tm_mday;
	    if (pWeekday) {
	        if ((xtime.tm_wday + 1) == 0)
	             *pWeekday = 7;
	        else *pWeekday = xtime.tm_wday + 1;
	    }
            if ((!pYear) && (!pMonth) && (!pDay) && (!pWeekday)) err [0] = 1;
	}
}
