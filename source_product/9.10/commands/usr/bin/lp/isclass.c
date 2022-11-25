/* @(#) $Revision: 66.3 $ */     
/* isclass -- predicate which returns TRUE if the specified name is
	     a legal lp class, FALSE if not.		*/

#include	"lp.h"

isclass(name)
char *name;
{
	char class[FILEMAX];

	if(*name == '\0' || strlen(name) > DESTMAX)
		return(FALSE);

	/* Check class directory */

	sprintf(class, "%s/%s/%s", SPOOL, CLASS, name);
#if defined(SecureWare) && defined(B1)
	/* GET_ACCESS==eaccess is a system call on the secure system */
{
	if(ISB1) {
	    struct stat sb;
	
	    if (stat(class, &sb) < 0)
		return 0;

	    return(GET_ACCESS(class, ACC_R | ACC_W) != -1);
	}
	else
	    return(GET_ACCESS(class, ACC_R | ACC_W) != -1);
}
#else
	return(GET_ACCESS(class, ACC_R | ACC_W) != -1);
#endif
}
