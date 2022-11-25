/* @(#) $Revision: 66.2 $ */      
/* isdest -- predicate which returns TRUE if the specified name is
	     a legal lp destination, FALSE if not.		*/

#include	"lp.h"

isdest(name)
char *name;
{
	char dest[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check request directory */

	sprintf(dest, "%s/%s/%s", SPOOL, REQUEST, name);
#if defined(SecureWare) && defined(B1)
	/* GET_ACCESS==eaccess is a system call on the secure system */
{
	if(ISB1) {
	    struct stat sb;
	
	    if (stat(dest, &sb) < 0)
		return 0;
	    if((sb.st_mode & S_IFMT) != S_IFDIR)
		return 0;

	    return(GET_ACCESS(dest, ACC_R | ACC_W | ACC_X) != -1);
	}
	else
	    return(GET_ACCESS(dest, ACC_R | ACC_W | ACC_X | ACC_DIR) != -1);
}
#else
	return(GET_ACCESS(dest, ACC_R | ACC_W | ACC_X | ACC_DIR) != -1);
#endif
}

