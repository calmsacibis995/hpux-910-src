/* @(#) $Revision: 27.1 $ */    

#include "lifdef.h"
#include "global.h"

extern char bigbuf[];

lifpurge(dirpath, name, filedis)
register char *dirpath;
register char *name;
register int filedis;
{
	struct lfib frec;
	struct dentry dir;
	register int n, frbyte;

	strcpy(frec.dirpath, dirpath);	
	frec.filedis = filedis;

	n = lifvol(&frec);			/* is it a lif volume? */
	if (n!=TRUE)				/* if not */
		return n;			/* exit with error */

	strcpy(frec.lfile.fname, name); 	/* name of file */
	frec.lfile.ftype = EOD;			/* look for any type */
	n = lfindfile(&frec);			/* find the file */
	if (n!=TRUE)				/* if it wasn't there */
		return(FILENOTFOUND);		/* exit with error */
		
	frbyte = frec.dstart*QRT_K + frec.dindex*DESIZE; /* where dir is */
	n = unitread(frec.filedis, &dir, sizeof(dir), frbyte, "lifclose");
	if (n == -1)
		return(IOERROR);
	dir.ftype = 0;					/* now purged */
	n = unitwrite(frec.filedis, &dir, sizeof(dir), frbyte, "lifclose");
	if (n == -1)
		return(IOERROR);
	return(TRUE);
}
