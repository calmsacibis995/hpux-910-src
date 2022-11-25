/* @(#) $Revision: 70.1 $ */      

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

#ifdef _NAMESPACE_CLEAN
#define scandir _scandir
#define opendir   _opendir
#define fstat     _fstat
#define readdir   _readdir
#define closedir  _closedir
#define strcoll    _strcoll
#define qsort     _qsort
#define alphasort _alphasort
#       ifdef   _ANSIC_CLEAN
#define malloc    _malloc
#define realloc   _realloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <sys/types.h>

	/* inclue <sys/stat.h> when kernel is ready */
#include "/usr/include/sys/stat.h"

#include <dirent.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef scandir
#pragma _HP_SECONDARY_DEF _scandir scandir
#define scandir _scandir
#endif

scandir(dirname, namelist, select, dcomp)
	char *dirname;
	struct dirent ***namelist;
	int (*select)(), (*dcomp)();
{
	register struct dirent *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	struct stat stb;
	long arraysz;
	DIR *dirp;
	int i;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stb) < 0)
		return(-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
#ifndef hpux
	arraysz = (stb.st_size / 24);
	if (!arraysz) 
		names = (struct direct **)malloc(1 * sizeof(struct direct *));
	else
		names = (struct direct **)malloc(arraysz * sizeof(struct direct *));
#else
	arraysz = (stb.st_size / sizeof(struct dirent));

	if (!arraysz) 
		names = (struct dirent **)malloc(1 * sizeof(struct dirent *));
	else
		names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));

#endif
	if (names == NULL)
		return(-1);

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
#ifndef hpux
		p = (struct direct *)malloc(DIRSIZ(d));
#else
		p = (struct dirent *)malloc(sizeof (struct dirent));
#endif
		if (p == NULL)
			return(-1);
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_namlen = d->d_namlen;
		for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++; );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
#ifdef hpux
			names = (struct dirent **)realloc((char *)names,
				stb.st_size) ;
#else
			names = (struct direct **)realloc((char *)names,
				(stb.st_size/12) * sizeof(struct direct *));
#endif
			if (names == NULL)
				return(-1);
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), dcomp);
	*namelist = (struct dirent *) names;
	return(nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */

#ifdef _NAMESPACE_CLEAN
#undef alphasort
#pragma _HP_SECONDARY_DEF _alphasort alphasort
#define alphasort _alphasort
#endif /* _NAMESPACE_CLEAN */

alphasort(d1, d2)
	struct dirent **d1, **d2;
{
	return(strcoll((*d1)->d_name, (*d2)->d_name));
}
