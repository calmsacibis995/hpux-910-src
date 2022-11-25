


#include <stdio.h>
#include <time.h>
#include <nlinfo.h>

void nlfmtcalendar(date, outstr, langid, err)
unsigned short	date, err[];
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
		fillbuff(outstr, 0, 18);
		nlinfo(L_DATELINE, (int *) format_str, &langid, err);
		if (err[0] == 0){
			format_nldt(langid, format_str, LENDATELINE, outstr, 
					18, NULL, &xtime);
			for (wrk = outstr + 17; 
					wrk != outstr && *wrk == ' '; wrk--)
				;
			if (*wrk == ',')
				*wrk = ' ';
		} else return;
	} 
	if (err[0])  err[0] = E_INVALIDDATEFORMAT;
}

