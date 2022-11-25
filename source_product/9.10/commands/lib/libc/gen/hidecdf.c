/* @(#) $Revision: 70.1 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define hidecdf _hidecdf
#define lstat _lstat
#    ifdef _ANSIC_CLEAN
#        define malloc _malloc
#        define free _free
#    endif /* _ANSIC_CLEAN */
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#define TRUE   1
#define FALSE  0

extern int errno;

static int
hide_cdf_path(path, dest, endp)
char *path;
char *dest;
char *endp;
{
    char *src_path = path;

    for (;;)
    {
	/*
	 * Copy everything up to a '+' character
	 */
	while (*path && *path != '+' && dest < endp)
	    *dest++ = *path++;
	
	/*
	 * If end of source string, we are done.
	 */
	if (*path == '\0')
	{
	    *dest = '\0';
	    return 0;
	}

	/*
	 * Source is too long for destination buffer.
	 */
	if (dest >= endp)
	{
	    errno = ENAMETOOLONG;
	    return -1;
	}

	/*
	 * This is potentially a CDF, see if the context matches.
	 * This is done by lstat-ing the path name with and without
	 * the "+/foo".  If the two stats are the same, then we can
	 * omit the "+/foo" part.
	 */
	if (*path == '+' && *(path+1) == '/')
	{
	    struct stat sbuf1;
	    struct stat sbuf2;
	    char *s = path+2;
	    char c;
	    int r1;
	    int r2;

	    /*
	     * First skip any multiple '/' characters and/or any
	     * "./" sequences.
	     */
	    while (*s)
	    {
		if (*s == '/')
		    s++;
		else if (*s == '.' && *(s+1) == '/')
		    s += 2;
		else
		    break;
	    }
	    
	    /*
	     * Now skip up to the next '/'
	     */
	    while (*s && *s != '/')
		s++;
	    
	    /*
	     * Stat the path with the "+/foo".  If this stat fails,
	     * we return an error, since the path must be valid for
	     * our algorithm to work.
	     */
	    c = *s;
	    *s = '\0';
	    if (lstat(src_path, &sbuf1) != 0)
		return -1;
	    *s = c;

	    /*
	     * Stat the path without the "+/foo"
	     */
	    *path = '\0';
	    r2 = lstat(src_path, &sbuf2);
	    *path = '+';

	    /*
	     * If the result of the two lstats are the same, then we
	     * have the same path with either path name.  Set 'path'
	     * to the point after the '+/foo'.
	     *
	     * Otherwise, we copy the '+' to the destination and keep
	     * going.
	     */
	    if (r2 == 0 &&
		sbuf1.st_ino == sbuf2.st_ino &&
		sbuf1.st_dev == sbuf2.st_dev)
		path = s;
	    else
		*dest++ = *path++;
	}
	else
	    *dest++ = *path++;
    }
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef hidecdf
#pragma _HP_SECONDARY_DEF _hidecdf hidecdf
#define hidecdf _hidecdf
#endif

char *
hidecdf(path, buf, size)
char *path;
char *buf;
int size;
{
    char *storage;		/* points to what we return */
    int dynamic;

    if (buf == NULL)
    {
	if ((storage = (char *)malloc(size)) == NULL)
	{
	    errno = ENOMEM;
	    return NULL;
	}
	dynamic = TRUE;
    }
    else
    {
	dynamic = FALSE;
	storage = buf;
    }

    if (size > MAXPATHLEN)
	size = MAXPATHLEN;

    if (hide_cdf_path(path, storage, &storage[size-1]) == -1)
    {
	if (dynamic)
	    free(storage);
	return NULL;
    }
    return storage;
}
