/* @(#) $Revision: 37.1 $ */      
#include <sys/types.h>
#include <sys/stat.h>
#include "lifdef.h"
#include "global.h"
#include "model.h"


/*
 * Findvsize calls volsize(3) which returns the volume size in bytes. If 
 * volume is a regular HP-UX file volsize returns 0.
 * Findvsize returns the size of the volume in 256 byte sectors.
 * In case of regular HP-UX files findvsize returns 527*2-1 sectors.
 */

findvsize(fd,sector)
register int fd;
register int *sector;
{
	
	struct stat statbuf;
	long volsize();
	register int n;

	/* check if volume is a regular HP-UX file */
	fstat(fd,&statbuf);
	statbuf.st_mode &= S_IFMT;
	if (statbuf.st_mode == S_IFREG) {
		*sector = 527 * 2 -1;
	    	return (TRUE);
	}
	n = volsize(fd);
	if (n == -1)
		return(VOLSIZERR);
	*sector = n / QRT_K;

	return(TRUE);
}
