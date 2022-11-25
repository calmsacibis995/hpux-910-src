/*
 *		RCS command executor
 *
 * $Header: execonly.c,v 66.1 90/11/14 14:46:11 ssa Exp $
 * Copyright Hewlett Packard Co. 1986
 */
/*****************************************************************************
 *                       execonly()
 *****************************************************************************
 */

#include "system.h"
#include "rcsbase.h"

extern void rcsallowsigs();
extern void rcsholdsigs();

int
execonly(path, argv)
char *path,
     *argv[];
/* Function: forks a child process and executes the program specified
 * by "path" with "argv" arguments.  This function made necessary by
 * the fact that the system command will return a misleading error
 * status if the program to be executed does not exist or cannot be
 * accessed.
 */
{
	int wait_stat, child, w;

	if ((child = fork()) == 0)
		{
		execv(path, argv);
		perror(argv[0]);
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
