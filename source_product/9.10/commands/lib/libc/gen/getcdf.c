/* @(#) $Revision: 66.2 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define getcdf _getcdf
#define closedir _closedir
#define getcontext _getcontext
#define lstat _lstat
#define stat _stat
#define strcmp _strcmp
#define strchr _strchr
#define strcpy _strcpy
#define strlen _strlen
#define strtok _strtok
#define opendir _opendir
#define readdir _readdir
#    ifdef _ANSIC_CLEAN
#        define malloc _malloc
#        define free _free
#    endif /* _ANSIC_CLEAN */
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ndir.h>
#include <errno.h>

#define TRUE   1
#define FALSE  0

extern int errno;

#define MAXCXTS 32
#define NOMATCH ~0
#define BESTMATCH(x, y) ((x) < (y) ? (x) : (y))

static char *context[MAXCXTS];

#define CXT_NAME(prio)  context[prio]
/*
 * cxt() -- return the relative priority of "name" as it refers
 *          to the current context.  Returns NOMATCH (a very low
 *          priority) if name doesn't match the current context.
 *          Highest priority is 0, lowest is NOMATCH.
 */
static unsigned int
cxt(name)
char *name;
{
    extern int getcontext();
    extern char *strtok();
    static unsigned int cxtmax = 0;
    int i;

    /*
     * Get the context, if we haven't already.  Assume that the context
     * can never change -- true for HP-UX 7.0 and earlier.
     */
    if (cxtmax == 0)
    {
	char *bp;
	int len;

	len = getcontext(NULL, 0); /* size of context */
	bp = (char *)malloc(len);
	(void)getcontext(bp, len);

	for (; cxtmax < MAXCXTS && 
		(context[cxtmax] = strtok(bp, " ")) != NULL; cxtmax++)
	    bp = NULL;
    }

    for (i = 0; i < cxtmax; i++)
	if (*context[i] == *name && strcmp(context[i], name) == 0)
	    return i;
    return NOMATCH;
}

/*
 * getmatch() --
 *   
 *    Given a path "dest" that specifies a starting CDF directory,
 *    expand the path to include the best matching cdf element.
 *    The resulting path is placed at "endp" as long as it will fit
 *    (endp < dest_max).  The new value of endp is returned.
 */
static char *
getmatch(dest, endp, dest_max)
char *dest;
char *endp;
char *dest_max;
{
    DIR *dirp;
    struct direct *dp;
    int len;
    unsigned int best_cxt = NOMATCH;

    if ((dirp = opendir(dest)) == NULL)
    {
	errno = ENOENT;
	return NULL;
    }

    /*
     * Search for the best match, we can quit searching if our best
     * match is zero because nothing matches better.
     */
    while ((dp = readdir(dirp)) != NULL && best_cxt != 0)
    {
	unsigned int this_cxt;

	/*
	 * Ignore "." and "..", the name we are looking or can't be
	 * one of these.
	 */
	if (dp->d_name[0] == '.' &&
	    (dp->d_name[1] == '\0' || (dp->d_name[1] == '.' &&
				      dp->d_name[2] == '\0')))
	    continue;

	/*
	 * See if this entry matches our context better than
	 * any entries weve seen so far.
	 */
	this_cxt = cxt(dp->d_name);
	best_cxt = BESTMATCH(this_cxt, best_cxt);
    }
    closedir(dirp);

    /*
     * Copy the best matching entry (if  any) to the end of dest
     * and search deeper.  If there was no entry that matched our
     * context, return with errno set to ENOENT.
     */
    if (best_cxt == NOMATCH)
    {
	*endp = '\0';
	errno = ENOENT;
	return NULL;
    }

    /*
     * We had a matching context.  Copy this to the end of
     * the current path and return.  If it won't fit return
     * an error.
     */
    len = strlen(CXT_NAME(best_cxt));
    if (endp + len >= dest_max)
    {
	*endp = '\0';
	errno = ENAMETOOLONG;
	return NULL;
    }

    strcpy(endp, CXT_NAME(best_cxt));
    return endp += len;
}
        
/*
 * get_cdf_path() --
 *   Expand the path "dest" in place.  Return 0 if successful, return
 *   -1 and set errno if an error occurs.
 */
static int
get_cdf_path(dest, dest_max)
char *dest;
char *dest_max;
{
    char orig[MAXPATHLEN];
    struct stat st1;
    struct stat st2;
    int last_element = FALSE;
    char *endp = dest;
    char *basename = orig;
    char *pos;
    int had_slash = FALSE;
    int first_time = TRUE;

    st1.st_mode = S_IFDIR;  /* in case dest is "//" (or worse) */

    strcpy(orig, dest);     /* make a copy of input */
    if (*endp == '/')
	endp++;
    *endp = '\0';

    for (;;)
    {
	int is_special;  /* is basename "." or ".." */

	/*
	 * Skip any extra '/' characters
	 */
	if (!last_element && *basename == '/')
	{
	    had_slash = TRUE;
	    while (*basename == '/')
		basename++;
	}

	/*
	 * If the path ended in a '/', make sure that the file
	 * we found is a directory, otherwise return ENOENT.
	 */
	if (last_element || *basename == '\0')
	{
	    *endp-- = '\0';
	    if (had_slash && (st1.st_mode & S_IFMT) != S_IFDIR)
	    {
		errno = ENOENT;
		return -1;
	    }

	    /* remove any trailing '/' characters */
	    while (*endp == '/' && endp > dest)
		*endp-- = '\0';

	    /*
	     * But add one if there was one in the original path
	     * and the original path wasn't "/".
	     */
	    if (had_slash && *endp != '/')
	    {
		*++endp = '/';
		*++endp = '\0';
	    }
	    return 0;
	}

	/*
	 * extract the next component of the original path
	 */
	if ((pos = strchr(basename, '/')) == NULL)
	{
	    last_element = TRUE;
	    pos = basename + strlen(basename);
	    had_slash = FALSE;
	}
	else
	{
	    had_slash = TRUE;
	    *pos = '\0';
	}
    
	/*
	 * Move the next component to the end of the expanded path
	 * unless it won't fit.
	 */
	if (endp + (pos - basename) > dest_max)
	{
	    *endp = '\0';
	    errno = ENAMETOOLONG;
	    return -1;
	}

	/*
	 * Add a slash to the end of the path (except the first time)
	 */
	if (first_time)
	    first_time = FALSE;
	else
	    *endp++ = '/';

	/*
	 * Remember if basename was "." or "..".  We don't add "+"
	 * to these names.
	 */
	is_special = (basename[0] == '.' &&
		     (basename[1] == '\0' ||
		      (basename[1] == '.' && basename[2] == '\0')));

	strcpy(endp, basename);
	endp += (pos - basename);
	basename = pos + 1;

	/*
	 * Now, stat the current path both with and without a '+' to
	 * see if it is a CDF.
	 */
	if (stat(dest, &st1) == -1)
	{
	    *endp = '\0';
	    return -1;
	}

	/*
	 * If the path was already a CDF, do nothing.
	 */
	if ((st1.st_mode & (S_IFMT|S_CDF)) == (S_IFDIR|S_CDF))
	{
	    *endp   = '\0';
	    continue;
	}

	/*
	 * Path wasn't an explicit CDF, see if there is one hiding.
	 */
	if (!is_special)
	{
	    *endp++ = '+';
	    *endp = '\0';
	}

	if (is_special || stat(dest, &st2) == -1 ||
	    (st2.st_mode & (S_IFMT|S_CDF)) != (S_IFDIR|S_CDF))
	{
	    /*
	     * Not a cdf, get rid of the '+' (only if we added it).
	     */
	    if (!is_special)
		*--endp = '\0';
	}
	else
	{
	    /*
	     * It is a cdf. Search the hidden directory for the best
	     * matching element.  If there is no matching element, we
	     * just return (errno == ENOENT).  If we find a matching
	     * element, we must see if it is a cdf too (cdf-s can nest),
	     * so we tack a '+' on the end and stat the path.  If we get
	     * a cdf, we repeat the whole process, if not we just
	     * continue.
	     */
	    do
	    {
		if (endp + 1 >= dest_max)
		{
		    *endp = '\0';
		    errno = ENAMETOOLONG;
		    return -1;
		}
		*endp++ = '/';
		*endp   = '\0';

		if ((endp = getmatch(dest, endp, dest_max)) == NULL)
		    return -1;

		if (endp + 1 >= dest_max)
		{
		    errno = ENAMETOOLONG;
		    return -1;
		}
		*endp++ = '+';
		*endp = '\0';
		if (stat(dest, &st2) == -1)
		    st2.st_mode = 0; /* not a cdf */
	    } while ((st2.st_mode & (S_IFMT|S_CDF)) == (S_IFDIR|S_CDF));
	    *--endp = '\0'; /* get rid of '+' */
	}

	/*
	 * Finally at a non-cdf element.  If this is the last element,
	 * continue (which will shortly return).
	 */
	if (last_element)
	    continue; /* returns at top of loop */

	/*
	 * Not the last element.  Make sure this is a directory,
	 * if it isn't, return with errno set to ENOENT.
	 */
	if ((st1.st_mode & S_IFMT) != S_IFDIR)
	{
	    errno = ENOENT;
	    return -1;
	}
    }
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getcdf
#pragma _HP_SECONDARY_DEF _getcdf getcdf
#define getcdf _getcdf
#endif

char *
getcdf(path, buf, size)
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

    /*
     * If the input path is longer than the destination, there is no
     * point in even trying, just return.
     */
    if (strlen(path) > size)
    {
	errno = ENAMETOOLONG;
	return NULL;
    }

    if (path != storage)
	strcpy(storage, path);

    if (get_cdf_path(storage, &storage[size-1]) == -1)
    {
	if (dynamic)
	    free(storage);
	return NULL;
    }
    return storage;
}
