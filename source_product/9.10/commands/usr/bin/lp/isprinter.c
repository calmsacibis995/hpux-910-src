/* @(#) $Revision: 66.3 $ */   
/* isprinter -- predicate which returns TRUE if the specified name is
	     a legal lp printer, FALSE if not.		*/

#include	"lp.h"

isprinter(name)
char *name;
{
	char printer[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check member directory */

	sprintf(printer, "%s/%s/%s", SPOOL, MEMBER, name);
#if defined(SecureWare) && defined(B1)
	/* GET_ACCESS==eaccess is a system call on the secure system */
{
	if(ISB1) {
	    struct stat sb;
	
	    if (stat(printer, &sb) < 0)
		return 0;

	    return(GET_ACCESS(printer, ACC_R | ACC_W) != -1);
	}
	else
	    return(GET_ACCESS(printer, ACC_R | ACC_W) != -1);
}
#else
	return(GET_ACCESS(printer, ACC_R | ACC_W) != -1);
#endif
}
