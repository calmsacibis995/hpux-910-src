/*	@(#) $Revision: 51.3 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.gwd.c
	gwd.c	1.3	7/29/85 16:33:09
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include "uucp.h"

/*
 *	gwd - get working directory
 *	Uid and Euid are global
 *	return
 *		0 - ok
 *	 	FAIL - failed
 */

gwd(wkdir)
char *wkdir;
{
	FILE *fp;
	char cmd[BUFSIZ];

	*wkdir = '\0';
	(void) sprintf(cmd, "%s pwd 2>&-", PATH);

#ifndef V7
	(void) setuid(Uid);
	fp = popen(cmd, "r");
	(void) setuid(Euid);
#else
	fp = popen(cmd, "r");
#endif

	if (fp == NULL || fgets(wkdir, MAXFULLNAME, fp) == NULL) {
		(void) pclose(fp);
		return(FAIL);
	}
	if (wkdir[strlen(wkdir)-1] == '\n')
		wkdir[strlen(wkdir)-1] = '\0';
	(void) pclose(fp);
	return(0);
}


/*
 * uidstat(file, &statbuf)
 * This is a stat call with the uid set from Euid to Uid.
 * Used from uucp.c and uux.c to permit file copies
 * from directories that may not be searchable by other.
 * return:
 *	same as stat()
 */

int
uidstat(file, buf)
char *file;
struct stat *buf;
{
#ifndef V7
	register ret;

	(void) setuid(Uid);
	ret = stat(file, buf);
	(void) setuid(Euid);
	return(ret);
#else V7
	int	ret;

	if (vfork() == 0) {
		(void) setuid(Uid);
		_exit(stat(file, buf));
	}
	wait(&ret);
	return(ret);
#endif V7
}
