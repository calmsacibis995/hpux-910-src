/* @(#) $Revision: 70.4 $ */      
#if !defined(DUX) && !defined(DISKLESS)
/*
 * This text is put here to cause an error if you attempt
 * to compile ftwh() without DUX or DISKLESS defined.
 */
Error: you should not be compiling this routine for non-DUX
       systems.
#else
/*LINTLIBRARY*/
/***************************************************************
 *	ftwh - file tree walk
 *
 *	int ftwh (path, fn, depth)  char *path; int (*fn)(); int depth;
 *
 *	Given a path name, ftwh starts from the file given by that path
 *	name and visits each file and directory in the tree beneath
 *	that file.  If a single file has multiple links within the
 *	structure, it will be visited once for each such link.
 *	For each object visited, fn is called with three arguments.
 *	The first contains the path name of the object, the second
 *	contains a pointer to a stat buffer which will usually hold
 *	appropriate information for the object and the third will
 *	contain an integer value giving additional information about
 *
 *		FTW_F	The object is a file for which stat was
 *			successful.  It does not guarantee that the
 *			file can actually be read.
 *
 *		FTW_D	The object is a directory for which stat and
 *			open for read were both successful.
 *
 *		FTW_DNR	The object is a directory for which stat
 *			succeeded, but which cannot be read.  Because
 *			the directory cannot be read, fn will not be
 *			called for any descendants of this directory.
 *
 *		FTW_NS	Stat failed on the object because of lack of
 *			appropriate permission.  This indication will
 *			be given, for example, for each file in a
 *			directory with read but no execute permission.
 *			Because stat failed, it is not possible to
 *			determine whether this object is a file or a
 *			directory.  The stat buffer passed to fn will
 *			contain garbage.  Stat failure for any reason
 *			other than lack of permission will be
 *			considered an error and will cause ftwh to stop 
 *			and return -1 to its caller.
 *
 *	If fn returns nonzero, ftwh stops and returns the same value
 *	to its caller.  If ftwh gets into other trouble along the way,
 *	it returns -1 and leaves an indication of the cause in errno.
 *
 *	The third argument to ftwh does not limit the depth to which
 *	ftwh will go.  Rather, it limits the depth to which ftw will
 *	go before it starts recycling file descriptors.  In general,
 *	it is necessary to use a file descriptor for each level of the
 *	tree, but they can be recycled for deep trees by saving the
 *	position, closing, re-opening, and seeking.  It is possible
 *	to start recycling file descriptors by sensing when we have
 *	run out, but in general this will not be terribly useful if
 *	fn expects to be able to open files.  We could also figure out
 *	how many file descriptors are available and guarantee a certain
 *	number to fn, but we would not know how many to guarantee,
 *	and we do not want to impose the extra overhead on a caller who
 *	knows how many are available without having to figure it out.
 *	Instead, we always make a check to see if the opening of a directory
 *  	failed due to errno == EMFILE, which means that we are out of file
 *	descriptors.  If this is the case, then we save our current directory
 * 	position and free up a file descriptor.
 *
 *	It is possible for ftwh to die with a memory fault in the event
 *	of a file system so deeply nested that the stack overflows.
 **************************************************************/

#ifdef _NAMESPACE_CLEAN
#define stat _stat
#define opendir _opendir 
#define close _close
#define ftwh _ftwh
#define strlen _strlen
#define closedir _closedir
#define strcpy _strcpy
#define readdir _readdir
#define strcmp _strcmp
#define strcat _strcat
#define telldir _telldir
#define seekdir _seekdir
#define sysconf _sysconf
#       ifdef   _ANSIC_CLEAN
#define free _free
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>
#include <errno.h>
#include <ftw.h>
#include <sys/param.h>
#include <unistd.h>

#ifndef NULL
#  define NULL 0
#endif

extern char *malloc(), *strcpy();
extern int errno;
extern DIR *opendir();
extern struct direct *readdir();
extern long telldir();
extern void seekdir();

#define OK_ERROR(x)     ((x) == EACCES || (x) == ENOENT)

#ifdef _NAMESPACE_CLEAN
#undef ftwh
#pragma _HP_SECONDARY_DEF _ftwh ftwh
#define ftwh _ftwh
#endif /* _NAMESPACE_CLEAN */

int
ftwh(path, fn, depth)
char *path;
int (*fn)();
int depth;
{
	DIR *dirp;
	struct stat sb;
	char p_plus[MAXPATHLEN];

	/* Check to make sure depth is in the valid range */
	if (depth > sysconf(_SC_OPEN_MAX)) {
		errno = EINVAL;
		return(-1);
	}

	/* If depth is < 1, set it to 1 */
	if (depth < 1)
		depth = 1;

	/*
	 * Try to get file status.
	 * See if the given path is a CDF, if so we use that.
	 * If not, we use the path as is.
	 */
	strcpy(p_plus, path);
	strcat(p_plus, "+");
	if (stat(p_plus, &sb) < 0 || !S_ISCDF(sb.st_mode)) {
		if (stat(path, &sb) < 0)
			return (errno == EACCES? (*fn)(path, &sb, FTW_NS): -1);
	} else
		path = p_plus;
	
	/*
	 * The stat succeeded, so we know the object exists.
	 * If not a directory, call the user function and return.
	 */
	if (!S_ISDIR(sb.st_mode))
		return((*fn)(path, &sb, FTW_F));

	/*
	 * The object was a directory.
	 *
	 * Open a file to read the directory
	 */
	dirp = opendir(path);
	if (dirp == NULL)
		return(errno == EACCES? (*fn)(path, &sb, FTW_DNR): -1);

	/*
	 * Call the REAL ftwh routine 
	 */
	return real_ftwh(path, fn, depth, dirp, &sb);
}

static int
real_ftwh(path, fn, depth, dirp, psb)
char *path;
int (*fn)();
int depth;
DIR *dirp;
struct stat *psb;
{
	int rc, n;
	char *subpath, *component;
	DIR *new_dirp;
	struct direct *dp;

	/*
	 * We know we can read the directory pointed to by dirp;
	 * call the user function
	 */
	rc = (*fn)(path, psb, FTW_D);
	if (rc != 0) {
		(void) close (fn);
		return(rc);
	}

	/* Allocate a buffer to hold generated pathnames */
	n = strlen(path);
	subpath = malloc((unsigned) (n + MAXNAMLEN + 3));
	if (subpath == NULL) {
		(void) closedir(dirp);
		errno = ENOMEM;
		return(-1);
	}

	/* Create a prefix to which we will append component names */
	(void) strcpy(subpath, path);
	if (subpath[0] != '\0' && subpath[n-1] != '/')
		subpath[n++] = '/';
	component = &subpath[n];

	/*
	 * Read the directory one component at a time.
	 * We must ignore "." and "..", but other than that, just 
	 * create a path name and call self if a directory.
	 */
	while (dp = readdir(dirp)) {
		if (dp->d_ino != 0 && strcmp(dp->d_name, ".") != 0 && 
				strcmp(dp->d_name, "..") != 0) {
			int i;
			char *p, *q;
			long here;

			/* Append component name to the working path */
			p = component;
			q = dp->d_name;
			for (i=0; i < MAXNAMLEN && *q != '\0'; i++)
				*p++ = *q++;

			/*
			 * Check if this is a CDF.  If so, we use the
			 * CDF path name, if not, we use as is.
			 */
			*p = '+';
			*(p + 1) = '\0';
			if (stat(subpath, psb) < 0 || !S_ISCDF(psb->st_mode)) {
			    *p = '\0'; /* remove the '+' */
			    if (stat(subpath, psb) < 0) {
				 if (!OK_ERROR(errno)) {
					free(subpath);
					(void) closedir(dirp);
					return(-1);
				}
				else {
					rc = (*fn)(subpath, psb, FTW_NS);
					if (rc != 0) {
						free(subpath);
						(void) closedir(dirp);
						return(rc);
					}
					continue;
				}
			    }
			}

			/*
			 * The stat succeeded, so we know the object exists.
			 * If not a directory, call the user function and 
			 * return.
			 */
			if (!S_ISDIR(psb->st_mode)) {
				rc = (*fn)(subpath, psb, FTW_F);
				if (rc != 0) {
					free(subpath);
					(void) closedir(dirp);
					return(rc);
				}
				continue;
			}

			/* 
			 * The object was a directory.
			 * Open a file to read the directory.
			 */

			/* If depth <= 1, then close a directory pointer */
			if (depth <= 1) {
				here = telldir(dirp);
				(void) closedir(dirp);
			}

			/* Open a new directory pointer */
			new_dirp = opendir(subpath);
			if (new_dirp == NULL && errno == EMFILE) {
				/* We have run out of file descriptors */
				here = telldir(dirp);
				(void) closedir(dirp);
				/* Artificially change depth so that we know
				   to reopen the directory.
				 */
				depth = 1;
				new_dirp = opendir(subpath);
			}
			/*
			 * Call the user function, telling it whether
			 * the directory can be read.  If it can't be read
			 * call the user function or indicate an error,
			 * depending on the reason it couldn't be read.
			 */
			if (new_dirp == NULL)
				if (errno == EACCES) {
					rc = (*fn)(subpath, psb, FTW_DNR);
					if (rc != 0) {
						free(subpath);
						return(rc);
					}
					else {
						if (depth <= 1) {
							dirp = opendir(path);
							if (dirp == NULL) {
								free(subpath);
								return(-1);
							}
							/*
							 * There is no way to
							 * determine if the 
							 * lseek() within
							 * seekdir() succeeded.
							 * A NULL will be 
							 * returned on the 
							 * next readdir(), but
							 * that's it.
							 */
							seekdir(dirp, here);
						}
						continue;
					}
				}
				else {
					free(subpath);
					return(-1);
				}

			/* Make a recursive call to real_ftwh to check subpaths*/
			rc = real_ftwh(subpath, fn, depth-1, new_dirp, psb);
			if (rc != 0) {
				free(subpath);
				if (depth > 1)
					(void) closedir(dirp);
				return(rc);
			}

			/*
			 * If we closed the file, try to reopen it.
			 */
			if (depth <= 1) {
				dirp = opendir(path);
				if(dirp == NULL) {
					free(subpath);
					return(-1);
				}
				/*
				 * There is no way (currently) to
				 * determine if the lseek() within
				 * seekdir() succeeded.  A NULL will
				 * be returned on the next readdir(),
				 * but that's it.
				 */
				seekdir(dirp, here);
			}
		}
	} /* end while */
	free(subpath);
	(void) closedir(dirp);
	return(0);
} /* end real_ftwh() */
#endif /* DUX || DISKLESS */
