/* @(#)  $Revision: 64.1 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#define DIRSIZ_MACRO
#include <ndir.h>


/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	char *malloc();

	if ((fd = open(name, 0)) == -1)
		return NULL;
	if ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		close (fd);
		return NULL;
	}
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	dirp->dd_size = 0;
	dirp->dd_bbase = 0;
	dirp->dd_entno = 0;
	dirp->dd_bsize = 8192;
	if ((dirp->dd_buf = malloc(dirp->dd_bsize)) == NULL) {
		close (fd);
		return NULL;
	}
	return dirp;
}
