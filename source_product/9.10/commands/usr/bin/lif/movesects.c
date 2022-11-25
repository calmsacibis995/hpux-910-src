/* @(#) $Revision: 27.1 $ */   

#include "lifdef.h"
#include "global.h"
#include <stdio.h>

extern char bigbuf[];

movesects (frsect, frdir, tosect, todir, numsects, fd_from, fd_to)
register int frsect;
register char *frdir;
register int *tosect;
register char *todir;
register int numsects, fd_from, fd_to;
{
	register int j, n, m;
	register int numread, maxnumread;

	bugout("fromsect %d tosect %d numsects %d", frsect, *tosect, numsects);
	if ((frsect != *tosect) || ((strcmp(todir, frdir)) != 0)) {
		while (numsects > 0) {	/* more sectors to write */
		
			maxnumread = min(numsects*256, K64);
			n = unitread(fd_from, bigbuf, maxnumread,
				frsect*256, "movesects");
			if (n == -1) 
				return(IOERROR);	
			numread = min(maxnumread, numsects * QRT_K); 
			m = unitwrite(fd_to, bigbuf,
				numread, *tosect * 256, "movesects");
			if (m != numread) {
				fprintf(stderr, "lifcp: movesects failed\n");
				return(IOERROR);
			}
			j = numread/QRT_K;
			frsect += j;
			*tosect += j;
			numsects -= j;
			bugout("numread %d j %d fromsect %d", numread, j, frsect);
			bugout("tosect %d numsect %d", *tosect, numsects);
		}
	}
	else *tosect = frsect + numsects;
	return(TRUE);
}
