/* @(#) $Revision: 66.1 $ */   

#include "lifdef.h"
#include "global.h"

extern char bigbuf[];
extern int kflag, kay;

int mostavail;			/* sectors in largest opening */
struct dpointer mostentry;	/* cat entry for largest opening */
int lastused;			/* last data sector used */
int tempsize;
int sfixed;			/* TRUE= size is fixed, FALSE= size is max */

/* determine if there is room for a new file */

lifspace(lfibp, implem, volnum, lastvol)
register struct lfib *lfibp;
register int implem;
register short volnum;
register char lastvol;
{
	register int n, nread, frsect, dirent, left_to_read;
	register struct dentry *dentryp;
	struct dpointer openentry;	/* available cat entry */

	n = FALSE;	/* initialize the return value */
	mostavail = 0;
	lastused = (lfibp->dstart) + (lfibp->dsize) - 1;
	openentry.sector = 0;
	mostentry = openentry;
	frsect = lfibp->dstart;
	left_to_read = lfibp->dsize*256;
	dirent = 0;
	sfixed = (lfibp->lfile.size > 0);

	do {
		nread = unitread(lfibp->filedis, bigbuf, min(left_to_read, K64),
			frsect*256, "lifspace");
		if (nread == -1)
			return(IOERROR);
		left_to_read -= nread;
		dentryp = (struct dentry *) bigbuf;
		while(dentryp < (struct dentry *) (bigbuf + nread)) {
			if (((frsect - lfibp->dstart) * 8 + dirent) >= (lfibp->dsize * 8)) {
				/* physical end of directory */
				if ((openentry.sector ==0) && sfixed)
					return(FALSE);
				else {
					if ((openentry.sector != 0)) {
						tempsize = lfibp->lastsector - lastused;
						if ((tempsize > mostavail)) {
							/* new max size */
							mostavail = tempsize;
							mostentry = openentry;
							mostentry.start = lastused;
						}
					}
					if ((mostavail == 0) || (sfixed &&
					    (mostavail < lfibp->lfile.size))){
						return(FALSE);
					}
					n = allocate(lfibp, implem, volnum, lastvol);
					return(n);
				}
			}
			if (dentryp->ftype == EOD) {
				/* logical end of directory */
				if (openentry.sector == 0) {
					openentry.sector = frsect;
					openentry.index = dirent;
				}
				tempsize = lfibp->lastsector - lastused;
				if (tempsize > mostavail) {
					/* new max size */
					mostavail = tempsize;
					mostentry = openentry;
					mostentry.start = lastused;
				}
				/* last try to allocate */
				if ((mostavail == 0) ||
			    	(sfixed && (mostavail < lfibp->lfile.size))){
					return(n);
				}
				n = allocate(lfibp, implem, volnum, lastvol);
				return(n);
			}
			else if (dentryp->ftype == 0) {
				/* purged entry */
				if (openentry.sector == 0) {
					openentry.sector = frsect;
					openentry.index = dirent;
				}
			}
			else { /* normal file entry */
				if (openentry.sector != 0) {
					/* may bave end of usable opening */
					tempsize = dentryp->start - lastused -1;
					if (tempsize > mostavail) {
						mostavail = tempsize;
						mostentry = openentry;
						mostentry.start = lastused;
						if (sfixed && (mostavail >= lfibp->lfile.size)) {
							n = allocate(lfibp, implem, volnum, lastvol);
							return(n);
						}
					}
				}
				/* did not allocate so */
				/* clear entry and update end value */
				openentry.sector = 0;
				lastused = dentryp->start + dentryp->size - 1;
				/* normal file type */
			}
			dentryp++;
			if (++dirent >= 8) {
				frsect++;
				dirent = 0;
			}
		}
	} while(frsect < lfibp->dstart + lfibp->dsize);
	return(FALSE);
}


allocate(lfibp, implem, volnum, lastvol)
register struct lfib *lfibp;
register int implem; 
register short volnum;
register char lastvol;
{
	register int eod;
	register int nread, nwrite, frsect, dirent;
	register struct dentry *dentryp;

	gettime(lfibp->lfile.date); 
	if (!sfixed)
		lfibp->lfile.size = mostavail;
	if (kflag) {		/* enforce multiple 1K restriction */
		int fkay = 4 * kay;
		mostentry.start = ((mostentry.start+1 > fkay) ? mostentry.start +1: fkay);
		if((mostentry.start % fkay) != 0)
		       mostentry.start += ((fkay) - (mostentry.start % (fkay)));
		lfibp->lfile.start = mostentry.start;
		mostentry.start--;
		}
	else lfibp->lfile.start = mostentry.start +1; 
	frsect = mostentry.sector;
	nread = unitread (lfibp->filedis, bigbuf, TWO_K, frsect*256, "allocate");
	if (nread == -1)
		return(IOERROR);
	dentryp = (struct dentry *) bigbuf;
	dentryp += mostentry.index;
	dirent = mostentry.index;
	eod = (dentryp->ftype == EOD);
	lfibp->lfile.lastvolnumber = volnum;
	if (lastvol > 0)
		lfibp->lfile.lastvolnumber |= 0100000;
	else
		lfibp->lfile.lastvolnumber &= 0077777;
	lfibp->lfile.extension = implem;
	*dentryp = lfibp->lfile;
	bugout("allocate: eod %d frsect %d", eod, frsect);
	if (eod) {
		if (++dirent >= 8) frsect++;
		if (frsect  < (lfibp->dstart + lfibp->dsize)) {
			bugout("allocate: dstart %d dsize %d", lfibp->dstart, lfibp->dsize);
			(++dentryp)->ftype = EOD;
		}
		if (dirent >= 8) frsect--;
	};
	nwrite = unitwrite (lfibp->filedis, bigbuf, TWO_K, frsect*256, "allocate");
	if (nwrite == -1)
		return(IOERROR);
	/* save entry index */
	lfibp->dindex = mostentry.index + ((frsect - lfibp->dstart) * 8); 
	bugout("allocat: absindex %d frsect %d", lfibp->dindex, frsect);
	return(TRUE);
}
