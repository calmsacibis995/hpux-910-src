/* @(#) $Revision: 66.1 $ */     
/*
 *	convert device to linename (as in /dev/linename)
 *	return ptr to LSZ-byte string, "?" if not found
 *	device must be character device
 *	maintains small list in tlist for speed
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include <ndir.h>
#include <sys/stat.h>


#define TSIZE1	50	/* # distinct names, for speed only */
#define DEVLEN	5	/* length of "/dev" + 1 */

static	tsize1;
static struct tlist {
	char	tname[LSZ];	/* linename */
	dev_t	tdev;		/* device */
} tl[TSIZE1];

static struct direct *d;

static char *tosearch[] = {
	"/dev/",
	"/dev/pty/",
	"\0"
};

dev_t	lintodev();

char *
devtolin(device)
dev_t device;
{
    register struct tlist *tp;
    struct stat sb;
    char newname[LSZ+DEVLEN], *result;
    int count;
    DIR *fdev;
    
    for (tp = tl; tp < &tl[tsize1]; tp++)
    	if (device == tp->tdev)
    		return(tp->tname);

    result="?";
    for (count=0; *tosearch[count] != '\0'; count++) {
	if ((fdev = opendir(tosearch[count])) == (DIR *)NULL)
		continue;
	while ((d = readdir(fdev)) != (struct direct *)NULL) {
		strcpy(newname, tosearch[count]);
		strncat(newname, d->d_name, d->d_namlen);
		if (d->d_ino != 0 && 
		    stat(newname, &sb) != -1 &&
		    (sb.st_mode & S_IFMT) == S_IFCHR &&
		    sb.st_rdev == device) {
			result = d->d_name;
		        closedir(fdev);
			goto end_search;
		} /* end of if */
	} /* end of while */
    	closedir(fdev);
    }  /* end of for */

end_search:

    /* Whatever we found (even failure), remember that in the cache */
    if (tsize1 < TSIZE1) {
	tp->tdev = device;
	CPYN(tp->tname, result);
	tsize1++;
    }
    return result;
}
