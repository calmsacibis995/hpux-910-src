

#include <nlinfo.h>

extern char	*idtolang();
extern int      _nl_errno;

nlappend(filename, langid, err)
char	*filename;
unsigned short	err[];
short		langid;
{
	char	*wrk, buff[4];

	_nl_errno = err[0] = err[1] = 0;
        idtolang (langid);
	if (_nl_errno) err [0] = E_LNOTCONFIG;

	for (wrk = filename; wrk < filename + 6; wrk++){
		if (*wrk == ' ' && strncmp(wrk, "   ", 3) == 0){
			sprintf(buff, "%03d", langid);
			memcpy(wrk, buff, 3);
			return;
		}
	}
	err[0] = E_APPENDNOTHREEBLANKS;
}
