/* @(#) $Revision: 27.1 $ */   

#include "lifdef.h"
#include "global.h"

char *strcpy();

lifcreate(lfibp,filename,ftype,filesize,implem,volnum,lastvol)
register struct lfib *lfibp;
register char *filename;
register int ftype, filesize, implem;
register char lastvol;
register short volnum;
{

	if (goodlifname(filename) != TRUE)
		return(BADLIFNAME);

	strcpy(lfibp->lfile.fname, filename);

	if (ftype==0 || ftype==EOD || ftype>LIFID || ftype<-LIFID)
		return(BADLIFTYPE);
	
	lfibp->lfile.ftype = EOD;
	if (lfindfile(lfibp) == TRUE)
		return(LIFEXISTS);
	
	lfibp->lfile.ftype = ftype;
	lfibp->lfile.size = filesize;
	if (lifspace(lfibp,implem,volnum,lastvol) != TRUE)
		return(NOSPACE);
	
	return(TRUE);
}
