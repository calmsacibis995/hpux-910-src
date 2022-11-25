


#include <stdio.h>
#include <time.h>
#include <nlinfo.h>

nlfmtclock(time, outstr, langid, err)
unsigned int	time;
unsigned char	*outstr;
short		langid;
unsigned short	err[];
{
	unsigned char format_str[8];
	short	hours, minutes;

	err[0] = err[1] = 0;

	nlinfo(L_CLKSPEC, (int *) format_str, &langid, err);
	if (err[0])
		return;

	hours = (unsigned short)(time >> 24);
	minutes = (unsigned int)(time >> 16) & 0xff;

	if (hours > 24  ||  minutes > 59  ||  (hours == 24  &&  minutes > 0)){
		err[0] = E_INVALIDTIMEFORMAT;
		return;
	}


	if (strncmp(format_str, "12", 2) == 0){
		if (hours > 12)
			hours -= 12;
		else if (hours == 0)
			hours = 12;
	}

	if (format_str[7] == '0')
		sprintf(outstr, "%02d%c%02d ", hours, format_str[2], minutes);
	else
		sprintf(outstr, "%2d%c%02d ", hours, format_str[2], minutes);

	if (strncmp(format_str, "12", 2) == 0){
		if ((unsigned int)(time >> 24) >= 12  &&
				(unsigned int)(time >> 24) != 24){
			outstr[6] = format_str[5];
			outstr[7] = format_str[6];
		}
		else{
			outstr[6] = format_str[3];
			outstr[7] = format_str[4];
		}
	}
	else
		outstr[6] = outstr[7] = ' ';
	
	if (isaltdig(NULL))
		convalt(outstr, outstr, outstr + 8, ASCIITOALT, NULL);

}
