


#include <stdio.h>
#include <time.h>
#include <nlinfo.h>

nlfmtdate(date, time, outstr, langid, err)
unsigned short	date, err[];
unsigned int	time;
unsigned char	*outstr;
short		langid;
{
	unsigned short	nlcrtm();
	unsigned char	*wrk, format_str[LENDATELINE];
	struct tm xtime;

	err[0] = err[1] = 0;

	xtime.tm_year = (unsigned int)(date >> 9);
	xtime.tm_yday = (unsigned int)(date & 0x1ff);

	if ( (err[0] = nlcrtm(&xtime)) == 0){
		fillbuff(outstr, 0, LENDATELINE);
		nlinfo(L_DATELINE, (int *) format_str, &langid, err);
		if (err[0] != 0)  return;
		format_nldt(langid, format_str, LENDATELINE, outstr, 
				LENDATELINE, NULL, &xtime);
		for (wrk = outstr + 28; wrk != outstr && *(wrk-1) == ' '; wrk--)
			;
		nlfmtclock(time, wrk + 1, langid, err);
		if (err[0] != 0)  {
	            err[0] = 4;
		    return;
                }
	} 
	if (err [0] != 0)  err[0] = 3;
}

