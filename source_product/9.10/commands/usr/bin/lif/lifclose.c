/* @(#) $Revision: 27.1 $ */    

#include "lifdef.h"
#include <stdio.h>
#include "global.h"

extern char bigbuf[];

lifclose (lfibp, cmode, size)
register struct lfib *lfibp;
register int cmode;
register int size;
{
	register int rc, addr;
	
	bugout("lifclose: cmode %d size %d dindex %d", 
		cmode, size, lfibp->dindex);

	if (cmode == LIFREMOVE) {
		rc=lifpurge(lfibp->dirpath, lfibp->lfile.fname, lfibp->filedis);
		if (rc != TRUE)
			return(rc);
	}

	else if (cmode == LIFMINSIZE) { /* keep or minimize */
		gettime(lfibp->lfile.date);  /* set create time */
		lfibp->lfile.size = (size % QRT_K)
					? (size / QRT_K) +1
					: (size / QRT_K);
		addr = (lfibp->dstart * QRT_K) + (lfibp->dindex * DESIZE);
		rc = unitwrite(lfibp->filedis, &lfibp->lfile, DESIZE,
			addr, "lifclose");
		if (rc == -1)
			return(IOERROR);
	}
	closeit(lfibp->filedis);
	return(TRUE);
}
