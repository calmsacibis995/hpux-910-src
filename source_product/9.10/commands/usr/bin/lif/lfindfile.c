/* @(#) $Revision: 66.1 $ */   
#include "lifdef.h"
#include "global.h"

extern char bigbuf[];

/*
 * Look for a file.
 *
 * If the filetype given is EOD, just look for that file name.
 * Otherwise, the types must match.
 */
lfindfile (lfibp)
register struct lfib *lfibp;
{
	register int n, readsize, current, nread;
	register struct dentry *ptr;

	lfibp->dindex = 0;
		
	for (current=0; current<lfibp->dsize*256; current+=nread) {
		readsize = min(lfibp->dsize * 256, K64);
		nread = unitread(lfibp->filedis, bigbuf, readsize, 
			(lfibp->dstart*256)+current, "lfindfile");
		if (nread == -1)
			return IOERROR;
		ptr = (struct dentry *) bigbuf;
		for ( ;  (char *)ptr < bigbuf+nread; ptr++) {
			if (ptr->ftype == EOD)
				return FALSE;
			n = mystrcmp(ptr->fname,lfibp->lfile.fname,MAXFILENAME);
			if (n==0) {
			    if (ptr->ftype == lfibp->lfile.ftype
			    || ptr->ftype!=0 && lfibp->lfile.ftype==EOD) {
				lfibp->lfile = *ptr;
				return TRUE;
			   }
			}
			(lfibp->dindex)++;
		}
	}

	return FALSE;
}


/*
 * Compare two strings for <size> bytes.
 * Neither string is terminated by a '\0'.
 */
mystrcmp(s1, s2, size)
register char *s1, *s2;
register int size;
{
	while (size-- > 0)
		if (*s1++ != *s2++)
			return -1;		/* unequal */
	
	return 0;				/* equal */
}
