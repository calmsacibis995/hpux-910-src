/*	@(#) $Revision: 1.1.109.1 $	*/

#include <sys/param.h>
#include <errno.h>
char *getcwd();
extern int errno, sys_nerr;
extern char *sys_errlist[];

char *
getwd(pathname)
char *pathname;
{
	char *p = getcwd(pathname, MAXPATHLEN);

	if (p == NULL)
        	if ((errno < sys_nerr) && (errno >= 0))
			sprintf(pathname, "getwd: %s", sys_errlist[errno]);
		else
			sprintf(pathname, "getwd: Unknown error %d", errno);
	return p;
}
