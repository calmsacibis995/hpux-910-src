/*
 *		RCS command executor with stdout redirection
 *
 *  $Header: execstdout.c,v 66.1 90/11/14 14:47:10 ssa Exp $
 */
/***************************************************************************
 *                       execstdout()
 ***************************************************************************
 */

#include <stdio.h>
#include "system.h"
#include "rcsbase.h"

extern void rcsallowsigs();
extern void rcsholdsigs();

int
execstdout(path, argv, dest_file)
char *path,
     *argv[],
     *dest_file;
/* Function: forks a child process, redirects standard output to
 * "dest_file" and executes the program specified by "path" with
 * "argv" arguments.  This function made necessary by the fact that
 * the system command will return a misleading error status if the
 * program to be executed does not exist or cannot be accessed.
 */
{
	int wait_stat, child, w;

	FILE *freopen();

	if ((child = fork()) == 0)
		{
		if (freopen(dest_file, "w", stdout) == NULL)
			{
			error1s("cannot open file %s", dest_file);
			exit(127);
			}
		execv(path, argv);
		error1s("cannot execute %s", path);
		exit(127);
		}

	rcsholdsigs();
	while ((w = wait(&wait_stat)) != child && w != -1)
		;
	rcsallowsigs();
	if (w == -1)
		return(1);
	else if ((wait_stat & 0377) != 0)
		return(-1);
	else return(((wait_stat >> BYTESIZ) & 0377));
}
