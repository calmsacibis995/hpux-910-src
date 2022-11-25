/* @(#) $Revision: 66.1 $ */

#include <sys/param.h>
#include <errno.h>
#include <string.h>

char *
getwd(pathname)
char *pathname;
{
    extern char *getcwd();
    char *cwd = getcwd(pathname, MAXPATHLEN);

    /*
     * FSDlj06761 -- BSD getwd() prints error message into pathname
     *		     buffer if an error occurs.   I don't understand
     *		     why the application just doesn't use errno, but
     *		     BSD code expects this behavior, so we have to
     *		     do it this way.
     */
    if (cwd == NULL)
    {
	extern char *sys_errlist[];
	extern int sys_nerr;
	extern char *ltoa();

	strcpy(pathname, "getwd: ");
	if (errno > sys_nerr)
	{
	    char *s = ltoa(errno);

	    strcat(pathname, "Unknown error (");
	    strcat(pathname, ltoa(errno));
	    strcat(pathname, ")");
	}
	else
	    strcat(pathname, sys_errlist[errno]);
    }
    return cwd;
}
