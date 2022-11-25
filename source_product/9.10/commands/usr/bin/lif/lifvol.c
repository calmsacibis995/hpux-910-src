/* @(#) $Revision: 70.1 $ */     

#include "lifdef.h"
#include "global.h"
#include <ctype.h>

int unused;		/* force long alignment on bigbuf */
char bigbuf[K256];	/* bigbuf and Fatal are used in several modules */

char *Fatal[] = {
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "", "", "", "", "", 
	"not a LIF volume", 
	"improperly formatted LIF name", 
	"invalid LIF file type", 
	"file already exists on the LIF volume", 
	"no space on the LIF volume", 
	"already opened", 
	"file not found", 
	"file not opened", 
	"end of file or end of volume reached", 
	"directory size too big", 
	"io error", 
	"wrong volume size"
};

lifvol (lfibp)
struct lfib *lfibp;  
{
	register int flag, n;
	struct lvol *lvolp;

	flag = NOTLIF;
	n = unitread(lfibp->filedis, bigbuf, QRT_K, 0, "lifvol");
	lvolp = (struct lvol *)bigbuf;
	if ( ! (n == -1 ||
	    lvolp->discid != LIFID ||
	    lvolp->dummy1 != 4096  ||
	    lvolp->dummy2 != 0     ||
	    lvolp->dummy3 != 0     ||
	    lvolp->dstart < 1      ||	/* accept PAWS/BASIC in entry 1 */
	    lvolp->dsize < 1       )) {
		lfibp->dstart = lvolp->dstart;
		lfibp->dsize = lvolp->dsize;
		if(lvolp->version == 0) {
			n=findvsize(lfibp->filedis, &(lfibp->lastsector));
			if (n != TRUE)
				flag = n;
		}
		else {
			lfibp->lastsector = (lvolp->tps* lvolp->spm*
			lvolp->spt) - 1;
			lfibp->dstart = lvolp->dstart;
		}
		flag = TRUE;
	}
	return(flag);
}


addb(str, size)
register char *str;
register int size;
{
	register int i = 0;

	while (*str) {
		str++;
		i++;
	}
	while (i++ < size)
		*str++ = ' ';
	*str = '\0';
}


goodlifname(filename)
register char *filename;
{
	register int i, len;
	register char c;

	/* first character must be upper case */
	if (!isupper(filename[0]))
		return(BADLIFNAME);
	
	/* find length of string */
	for (len=MAXFILENAME, i=0; i<MAXFILENAME; i++) {
		if (filename[i]=='\0') {
			len=i;
			break;
		}
	}

	/* strip blanks from our length */
	while (len>0 && filename[len-1]==' ')
		len--;

	/* Make sure all characters are valid */
	for (i=0; i<len; i++) {
		c = filename[i];

		if (isupper(c) || isdigit(c) || c=='_')
			continue;

		return(BADLIFNAME);
	}
	
	return(TRUE);
}



/* 
 * Translate HP-UX name to acceptable LIF format. 
 */
trnsname(fname, length)  
register char *fname;
register int length;
{
	register char *q, c, d;
	register int i;

	/* find length of string */
	for (i=0; i<length; i++)
		if (fname[i]=='\0')
			break;
	length=i;

	/* Ignore trailing blanks */
	while (length>0 && fname[length-1]==' ')
		length--;

	for (i=0; i<length; i++)
		fname[i] = convert_char(fname[i]);

	/* take care of the first letter */
	if (!isupper(fname[0])) {
		q = fname;
		c = *q;
		*q++ = 'X';
		i = 1;
		while (*q!='\0' && i<length) {
			d = *q; 
			*q++ = c;
			c = d;
			i++;
		}
		if (i < length) {
			*q++ = c;
			*q = '\0';
		}
	}
}

/* 
 * Convert charcter c to upper case.
 * Convert non alphanumeric chars to underscore.
 */
convert_char(c)	
register int c;
{
	if (islower(c))
		return toupper(c);
	else if (isdigit(c) || isupper(c))
		return(c);
	else return('_');
}

min(a, b)
register int a, b;
{
	return((a < b) ? a : b);
}

mystrcpy(s1, s2, length)
register char *s1, *s2;
register int length;
{
	while (length-- > 0)
		*s1++ = *s2++;
}
