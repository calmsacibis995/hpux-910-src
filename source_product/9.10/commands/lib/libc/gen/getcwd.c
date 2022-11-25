/* @(#) $Revision: 72.2 $ */
/*LINTLIBRARY*/
/*
 * char *getcwd(char *buf, int buflen);
 * char *gethcwd(char *buf, int buflen);
 *
 * Library routine to get the Current Working Directory.
 *   buf --    a pointer to a character buffer into which the path name
 *             of the current directory is placed by the subroutine.
 *             This parameter may be (char *)0, in which case getcwd()
 *	       will call malloc to get the required space.
 *   buflen -- The parameter buflen is the length of the buffer space
 *             for the path-name.
 *
 *
 * Return values --
 *   the cwd   -- if no error encoungered
 *   (char *)0 -- if an error ocurrs.  The variable "errno" is set to:
 *      EACCES       -- Read or search permission is denied for a
 *                      component of path-name.
 *      EINVAL       -- The size argument is zero or negative.
 *      ENOMEM       -- The malloc routine failed to provide buflen
 *                      bytes of memory.
 *      ERANGE       -- The buflen argument is greater than 0, but
 *                      is smaller than the length of the path-name.
 *      ENAMETOOLONG -- The path to the current working directory is
 *                      longer than MAXPATHLEN.
 *      EFAULT       -- buf points outsize of the allocated address
 *                      space of the process.
 *                      This is not always reliably detected.
 */

#ifdef _NAMESPACE_CLEAN
#   define getcwd	_getcwd
#   define gethcwd	_gethcwd
#   define open		_open
#   define close	_close
# ifdef _THREAD_SAFE
#   define readdir_r	_readdir_r
#   define strtok	_strtok_r
# else
#   define readdir	_readdir
# endif
#   define closedir	_closedir
#   define getcontext	_getcontext
#   define fchdir	_fchdir
#   define chdir	_chdir
#   define fstat	_fstat
#   define lstat	_lstat
#   define stat		_stat
#   define strcmp	_strcmp
#   define strcpy	_strcpy
#   define strlen	_strlen
#   define strncpy	_strncpy
#   define strspn       _strspn
#   define strpbrk      _strpbrk
#   ifdef _ANSIC_CLEAN
#       define malloc	_malloc
#       define free	_free
#   endif
#endif

#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <ndir.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _cwd_rmutex;
#endif

#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#endif

#if defined(DUX) || defined(DISKLESS)
#   define CDF(st) (((st).st_mode & (S_IFMT|S_CDF)) == (S_IFDIR|S_CDF))
#endif

#ifdef SYMLINKS
#   define STAT lstat
#else
#   define STAT stat
#endif

/*
 * Tiny stat structure of just the information we care about.
 */
struct mystat
{
    ino_t ino;        /* file's inode number */
    dev_t dev;        /* device on which file resides */
#if defined(DUX) || defined(DISKLESS)
    int   cdf;        /* TRUE if file is a CDF */
#endif
};

/*
 * Variables to hold cached value of cwd
 */
static char cwd[MAXPATHLEN];
static struct mystat cached_mst;

/*
 * Variable used by search() to store result
 */
static char *cwdpos;

#if defined(DUX) || defined(DISKLESS)

#define MAXCXTS 32
#define NOMATCH ~0
#define BESTMATCH(x, y) ((x) < (y) ? (x) : (y))

#ifndef _THREAD_SAFE
static char *mystrtok();
#endif
static char *context[MAXCXTS];

#define CXT_NAME(prio)  context[prio]
/*
 * cxt() -- return the relative priority of "name" as it refers
 *          to the current context.  Returns NOMATCH (a very low
 *          priority) if name doesn't match the current context.
 *          Highest priority is 0, lowest is NOMATCH.
 *
 * Initializes local data structures when called with (char *)0.  In
 * this case, -1 indicates a malloc error, otherwise returns the number
 * of items in the context.
 */
static unsigned int
cxt(name)
    char *name;
{
    static unsigned int cxtmax = 0;
    int i;
#ifdef _THREAD_SAFE
    char *last;
#endif

    /*
     * Get the context if we were told to.  Assume that the context
     * can never change -- true for HP-UX 7.0 and earlier.
     */
    if (name == (char *)0)
    {
	char *bp;
	int len;

	if (cxtmax == 0)
	{
	    len = getcontext((char *)0, 0);	/* size of context */
	    if ((bp = malloc(len)) == (char *)0)
		return -1;
	    (void)getcontext(bp, len);

	    for (; cxtmax < MAXCXTS &&
#ifdef _THREAD_SAFE
		    (context[cxtmax] = strtok_r(bp, " ", &last)) != (char *)0;
#else
		    (context[cxtmax] = mystrtok(bp, " ")) != (char *)0;
#endif
		    cxtmax++)
		bp = (char *)0;
	}
	return cxtmax;
    }

    for (i = 0; i < cxtmax; i++)
	if (*context[i] == *name && strcmp(context[i], name) == 0)
	    return i;
    return NOMATCH;
}

#ifndef _THREAD_SAFE
/*
 * THIS ROUTINE IS NO LONGER NEEDED SINCE WE CAN MAKE USE OF strtok_r,
 * THE REENTRANT VERSION OF strtok
 */
/*
  mystrtok -- perform exactly the same as strtok but we cannot use it
  because if would affect the users "savept" in strtok
*/
static char * mystrtok(string, sepset)
char	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/*first or subsequent call*/
	p = (string == NULL)? savept: string;

	if(p == 0)		/* return if no tokens remaining */
		return(NULL);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept = ++r;
	}
	return(q);
}
#endif /* _THREAD_SAFE */
#endif /* DUX/DISKLESS */

/*
 * setup() -- Do up front checks and malloc a buffer if needed.  Also
 *            calls cxt() to initialize the context stuff if hide is
 *            TRUE.
 */
static char *
setup(buf, buflen, pdidmalloc)
    char *buf;
    int buflen;
    int *pdidmalloc;
{
    *pdidmalloc = (buf == (char *)0);

    if (buflen <= 0)
    {
	errno = EINVAL;
	return (char *)0;
    }

    if (buflen < 2)
    {
	errno = ERANGE;
	return (char *)0;
    }

#if defined(DUX) || defined(DISKLESS)
    if ((*pdidmalloc && (buf = malloc(buflen)) == (char *)0) ||
	    cxt((char *)0) == -1)
#else
    if (*pdidmalloc && (buf = malloc(buflen)) == (char *)0)
#endif
    {
	if (buf != (char *)0)
	    free(buf);
	errno = ENOMEM;
	return (char *)0;
    }

    errno = 0;
    return buf;
}

/*
 * search() --
 *   iterative routine to search up the directory hierarchy filling in
 *   cwd tail first with the current pathname.  It is passed a tiny
 *   stat struct (mystat) that refers to ".".
 *
 *  Returns:
 *     0 -- all okay
 *    -1 -- EACCES error
 *
 *  NOTE:  We call __opendir() to open the directory "..".  __opendir()
 *	   is an undocumented version of opendir(3c) that takes the
 *	   address of a stat structure as an extra argument.  opendir()
 *	   needs to do a fstat(dir->dd_fd), but so do we.  By passing
 *	   __opendir() a stat structure, we can eliminate an fstat()
 *	   system call, which greatly improves performance.
 */
#if defined(DUX) || defined(DISKLESS)
static int
search(mst, hide)
    struct mystat *mst;
    int hide;
#else
static int
search(mst)
    struct mystat *mst;
#endif /* DUX/DISKLESS */
{
    extern DIR *__opendir();
    struct stat st;
    char name[MAXNAMLEN+1];
    struct mystat dot;
    struct mystat dotdot;
    DIR *dir;
#ifdef _THREAD_SAFE
    struct direct dirbuf;
    struct direct *dp = &dirbuf;
#else
    struct direct *dp;
#endif
    int must_stat;
    int namlen;
    int matched_prev = FALSE;
    int first_time = TRUE;

    dot = *mst;
    for (;; dot = dotdot)
    {
	int found = FALSE;
#if defined(DUX) || defined(DISKLESS)
	unsigned int bestcxt = NOMATCH;
	unsigned int dotcxt = NOMATCH;

	if ((dir = __opendir("..+", &st)) != (DIR *)0 && CDF(st))
	{
	    dotdot.cdf = TRUE;
	}
	else
	{
	    if (dir != (DIR *)0)
		closedir(dir);

	    dir = __opendir("..", &st);
	    if (dir == (DIR *)0)
		return errno;

	    dotdot.cdf = FALSE;
	}
#else
	dir = __opendir("..", &st);
	if (dir == (DIR *)0)
	    return errno;
#endif /* DUX/DISKLESS */

	/*
	 * If "." and ".." are the same, we are finally at "/"
	 */
	dotdot.dev  = st.st_dev;
	dotdot.ino  = st.st_ino;
	if (dotdot.ino == dot.ino && dotdot.dev == dot.dev)
	{
	    closedir(dir);
	    if (*cwdpos == '\0')
		*(--cwdpos) = '/';
	    return 0;
	}
	must_stat = dotdot.dev != dot.dev;

	/*
	 * Move up one level in the hierarchy.
	 */
	if (fchdir(dir->dd_fd) != 0)
	{
	    int save_errno = errno;

	    closedir(dir);
	    return save_errno;
	}

#ifdef _THREAD_SAFE
	while (readdir_r(dir, &dirbuf) != -1)
#else
	while ((dp = readdir(dir)) != (struct direct *)0)
#endif
	{
#if defined(DUX) || defined(DISKLESS)
	    unsigned int thiscxt;
#endif
	    int same;

	    /*
	     * Ignore "." and "..", the name we are looking or can't be
	     * one of these.
	     */
	    if (dp->d_name[0] == '.' &&
		(dp->d_name[1] == '\0' || (dp->d_name[1] == '.' &&
					  dp->d_name[2] == '\0')))
		continue;

#if defined(DUX) || defined(DISKLESS)
	    if (dotdot.cdf && hide)
	    {
		thiscxt = cxt(dp->d_name);
		bestcxt = BESTMATCH(thiscxt, bestcxt);
	    }
#endif

	    if (must_stat)
	    {
		/*
		 * Drat! must stat this entry to see if it matches what
		 * we are looking for.
		 */
		strcpy(name, dp->d_name);
#if defined(DUX) || defined(DISKLESS)
		if (dot.cdf) /* "." is a cdf, add trailing '+' */
		{
		    name[dp->d_namlen]   = '+';
		    name[dp->d_namlen+1] = '\0';
		}
#endif
		if (STAT(name, &st) == -1)
		{
		    same = FALSE;
		}
		else
		    same = st.st_ino == dot.ino &&
			   st.st_dev == dot.dev;
	    }
	    else
		same = (dp->d_ino == dot.ino);

	    if (same)
	    {
		found = TRUE;
#if defined(DUX) || defined(DISKLESS)
		/*
		 * If dotdot is a cdf and we want to hide cdfs and the
		 * entry for "." matches our context, we must continue
		 * searching this directory to see if there is a better
		 * match (in which case we don't hide this entry). There
		 * can never be a better match than 0, so we can stop
		 * then too.
		 */
		dotcxt = thiscxt;
		if (dotdot.cdf && hide &&
		    thiscxt != NOMATCH && thiscxt != 0)
		    continue;
		else
#endif /* DUX/DISKLESS */
		    break;
	    }
	}

	if (!found)
	{
	    closedir(dir);
	    return ENOENT;
	}

#if defined(DUX) || defined(DISKLESS)
	/*
	 * If this was a CDF and we want to hide matching elements,
	 * copy the path name to name only if "." isn't the best
	 * context match.
	 *
	 * If "." is the best match for our context, we just continue.
	 */
	if (dotdot.cdf && hide && dotcxt != NOMATCH)
	{
	    /*
	     * If we are hiding CDF elements and this is the first
	     * iteration of the loop and dot is a CDF, we don't want
	     * to hide this element.
	     *
	     * This is to cover the following case:
	     *
	     *    wrong answer:          /foo
	     *    expanded wrong answer: /foo+/remoteroot+/default
	     *    correct answer:        /foo+/remoteroot+
	     *
	     * Also, if we are hiding CDF elements and dot is a CDF
	     * and dot matches our context but the element below dot
	     * doesn't, we don't want to hide this element.
	     *
	     * This is to cover the following case:
	     *
	     *    wrong answer:          /foo/no_match
	     *    expaned wrong answer:  ENOENT
	     *    correct answer:        /foo+/default+/no_match
	     *
	     */
	    if (dotcxt == bestcxt &&
		!((first_time || !matched_prev) && dot.cdf))
	    {
		closedir(dir);
		matched_prev = TRUE;
		first_time = FALSE;
		continue;
	    }
	    strcpy(name, CXT_NAME(dotcxt));
	    namlen = strlen(name);
	}
	else
#endif /* DUX/DISKLESS */
	{
	    namlen = dp->d_namlen;
	    if (must_stat)
		name[namlen] = '\0'; /* delete trailing '+' (if any) */
	    else
		strcpy(name, dp->d_name);
	}

	closedir(dir);

#if defined(DUX) || defined(DISKLESS)
	if (dot.cdf && (!hide || !matched_prev))
	{
	    name[namlen]    = '+';
	    name[++namlen] = '\0';
	}
#endif

	cwdpos -= (namlen+1);
	if (cwdpos < cwd)
	    return ENAMETOOLONG;

	*cwdpos = '/';
	strncpy(cwdpos+1, name, namlen);
	matched_prev = FALSE;
	first_time = FALSE;
    }
}

#if defined(DUX) || defined(DISKLESS)
static char *
docwd(buf, buflen, hide)
    char *buf;
    int buflen;
    int hide;
#else
static char *
docwd(buf, buflen)
    char *buf;
    int buflen;
#endif /* DUX/DISKLESS */
{
    struct mystat mst;
    struct stat st;
    int didmalloc;
    int status;
    int cwdlen;

    if ((buf = setup(buf, buflen, &didmalloc)) == (char *)0)
	return (char *)0;

/* Check if the current getcwd request is for the */
/* same information as the last getcwd() call.    */
/* This is accomplished by comparing the inode and */
/* device values of the directory of the current   */
/* request against the values that were cached     */
/* (saved) by the last request.			   */
/* If the answer is yes, then we simply return 	   */
/* the cached value, if no, then we go ahead and   */
/* process the getcwd request.			   */

/* Check if the current directory is searchable     */
/* with a (l)stat call and get the stat information */
/* for the current directory.			    */

    if ( STAT(".",&st) == -1 ) {
	mst.dev = -1;
	return (char *)0; /* If the stat failed, then return the error  */
			  /* code (value 0).  The errno has already     */
			  /* been set by the stat() call.		*/ 
    }

/* If the stat() call passed, then we need to */
/* save the inode, device, and cdf values.    */

    mst.ino = st.st_ino;
    mst.dev = st.st_dev;
#if defined(DUX) || defined (DISKLESS)
    mst.cdf=CDF(st);
#endif

/* Now to check if the current entry is the same as the one */
/* that has been cached.  If this is the first time that    */
/* getcwd() is called, the cache values will all be zeros.  */

    if (
#if defined(DUX) || defined(DISKLESS)
	hide == cached_mst.cdf &&
#endif
	cached_mst.dev != 0 &&
	cached_mst.ino == mst.ino &&
	cached_mst.dev == mst.dev &&
	STAT(cwdpos, &st) != -1 &&
	st.st_ino == mst.ino && st.st_dev == mst.dev)
    {
        if ( strlen(cwdpos) < buflen )  /* verify that the buffer is large */
        {   				/* enough for the string.	   */
	   errno=0; /* reset errno (stat may have set it) */
           strcpy(buf, cwdpos);
	   return buf; 
	} 
	else /* user's buffer is too small.  Return error */
	{
	   if ( didmalloc ) free(buf); 
	   errno=ERANGE;	/* Set errno to ERANGE */
	   return 0;		/* Return 0 as return code */
	}
    }	
    else
    {
           errno = 0;  /* clear errno possibly set by stat */
    } /* End of if ... else ... for checking cached mst */

    /*
     * At this point, invalidate our cached pwd
     * This can be done because we know that the 
     * current request is for a different directory and
     * that the cached pwd needs to be reset.
     */
    cached_mst.dev = 0;

    /*
     * search() will put the results starting at cwdpos. The global
     * variable cwdpos will point to the end of cwd, so we use it
     * as the length.
     */
    cwdpos = cwd + MAXPATHLEN-1;
    *cwdpos = '\0';

    /*
     * If the search failed, we don't write the leading '/'
     * character to indicate that pwd needs to be reversed
     */
#if defined(DUX) || defined(DISKLESS)
    if ((status = search(&mst, hide)) == 0)
#else
    if ((status = search(&mst)) == 0)
#endif
    {
       errno=0; /* make sure that errno is  0 */
       chdir(cwdpos);       
       if ( strlen(cwdpos) < buflen ) 
       {   
          strcpy(buf, cwdpos);

          /*
           * Indicate that the cache is valid.
           */
          cached_mst = mst;
#if defined(DUX) || defined(DISKLESS)
          cached_mst.cdf = hide;
#endif
          errno = 0;
          return buf;
       }
       else
	  status=ERANGE;
    }
/* fall through condition for failures. */
/* The following section of code is  	*/
/* executed if (1) a failure occurs in  */
/* the search sequence, or (2) the users */
/* buffer length was too small.		*/
Fail:
    if (didmalloc)
	free(buf);

    /* Assume that the 'status' variable   */
    /* contains the error code.		   */
    /* thus, we must set errno accordingly */

    errno = status;
    return (char *)0;
}

#ifdef _NAMESPACE_CLEAN
#undef getcwd
#pragma _HP_SECONDARY_DEF _getcwd getcwd
#define getcwd _getcwd
#endif /* _NAMESPACE_CLEAN */

char *
getcwd(buf, buflen)
char *buf;
int buflen;
{
#if defined(DUX) || defined(DISKLESS)
# ifdef _THREAD_SAFE
    char *retval;

    _rec_mutex_lock(&_cwd_rmutex);
    retval = docwd(buf, buflen, TRUE);
    _rec_mutex_unlock(&_cwd_rmutex);
    return retval;
# else
    return docwd(buf, buflen, TRUE);
# endif
#else
    return docwd(buf, buflen);
#endif
}

#if defined DUX || defined DISKLESS

#ifdef _NAMESPACE_CLEAN
#undef gethcwd
#pragma _HP_SECONDARY_DEF _gethcwd gethcwd
#define gethcwd _gethcwd
#endif /* _NAMESPACE_CLEAN */

char *
gethcwd(buf, buflen)
char *buf;
int buflen;
{
#ifdef _THREAD_SAFE
    char *retval;

    _rec_mutex_lock(&_cwd_rmutex);
    retval = docwd(buf, buflen, FALSE);
    _rec_mutex_unlock(&_cwd_rmutex);
    return retval;
#else
    return docwd(buf, buflen, FALSE);
#endif
}
#endif /* defined(DUX) || defined(DISKLESS) */
