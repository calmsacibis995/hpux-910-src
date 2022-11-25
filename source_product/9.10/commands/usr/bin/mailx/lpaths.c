/* @(#) $Revision: 64.2 $ */    

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */


/*
 *	libpath(file) - return the full path to the library file
 *	binpath(file) - return the full path to the library file
 *
 *	If MAILXTEST is defined in the environment, use that.
 */

#include "uparm.h"

char *strcat();

char *
libpath (file)
char *file;		/* the file name */
{
static char buf[100];	/* build name here */
char *envexlib;		/* the pointer returned by getenv */
extern char *getenv();

	if ( (envexlib = getenv ("MAILXTEST")) != 0) {
		strcpy (buf, envexlib);
		strcat (buf, "/lib/mailx");
	} else {
		strcpy (buf, LIBPATH);
	}
	strcat (buf, "/");
	strcat (buf, file);
	return (buf);
}

#ifdef NLS

#include <limits.h>

char *
nlspath (file)
char *file;		/* the file name */
{
	static char buf[100];	/* build name here */
	char *lang;		/* the pointer returned by getenv */
	extern char *getenv();

	strcpy(buf, NLSDIR);
	if ((lang = getenv("LANG")) == 0)
		return("");
	strcat(buf, lang);
	strcat(buf, "/mailx/");
	strcat(buf, file);
	return(buf);
}

#endif
