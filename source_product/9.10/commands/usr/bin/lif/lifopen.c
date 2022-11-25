/* @(#) $Revision: 27.1 $ */     

#include "lifdef.h"
#include "global.h"

char *strcpy();

lifopen(lfibp, dirpath,rwmode)
register struct lfib *lfibp;
char *dirpath;
int rwmode;
{
	register int n;

	if ((lfibp->filedis = open(dirpath,rwmode)) < 0)
		return(IOERROR);

	strcpy(lfibp->dirpath, dirpath);
	if ((n = lifvol(lfibp)) != TRUE)
		return(n);

	return(TRUE);
}
