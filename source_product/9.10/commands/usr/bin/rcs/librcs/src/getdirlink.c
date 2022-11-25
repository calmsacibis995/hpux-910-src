/*
 *   Read Directory link function
 *
 *  	getdirlink()
 *
 * $Header: getdirlink.c,v 1.4 86/05/21 10:35:51 bob Exp $
 * Copyright Hewlett Packard Co. 1986
 */

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define MAXDIRLINKS 10

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];

char
*getdirlink(path)
char *path;
/*
 * getdirlink: follow a chain of directory links until a directory
 * is located or MAXDIRLINKS is exceeded.  A directory link is a
 * regular file containing the name of a directory or another
 * directory link.  In a limited way these links serve the same
 * purpose as BSD symbolic links.
 */
{
	struct stat statbuf;
	static char buf[MAXPATHLEN+1];
	char *endofline;
	int i, len, fd;

	if ((i = strlen(strcpy(buf, path))) == 0)
		return(NULL);	/* return NULL on all errors */

	if (buf[--i] == '/') buf[i] = '\0';	/* drop trailing slash */
	if (stat(buf, &statbuf) < 0)
		return(NULL);	/* no warning if first file doesn't exist */

	errno = 0;		/* clear system error code */

	for (i = 1; i <= MAXDIRLINKS; i++) {
		if (stat(buf, &statbuf) < 0)
			break;		/* ERROR */
		if (statbuf.st_mode & S_IFDIR)
			return(buf);
		if ((fd = open(buf, O_RDONLY)) == -1)
			break;		/* ERROR */
		len = read(fd, buf, MAXPATHLEN);
		close(fd);
		if (len <= 0)
			break;		/* ERROR */
		buf[len] = '\0';
	/* if more than 1 newline, truncate at first 1 */
		if ((endofline = strchr(buf, '\n')) != NULL)
			*endofline = '\0';
		if (strlen(buf) == 0)
			break;		/* ERROR */
	}

	if (errno != 0)
		warn2s("directory link %s: %s", buf,
			(errno <= sys_nerr) ? sys_errlist[errno] : "\0");
	return(NULL);	/* return NULL on all errors */
}
