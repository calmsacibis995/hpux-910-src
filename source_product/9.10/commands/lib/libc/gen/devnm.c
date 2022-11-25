/* @(#) $Revision: 70.2 $ */

/*
 * devnm(3) LIBRARY CALL
 *
 * Compile with -DDEBUG to include a main() procedure for interactive testing.
 * See below for its usage.
 *
 * IDEAS CONSIDERED BUT NOT IMPLEMENTED:
 *
 * These are from various sources.  Note that the current design permits any or
 * all of these to be added later with full backward compatibility (so long as
 * cache is currently passed as 0 or 1, as recommended in the manual entry).
 *
 * - Provide a way to tell devnm() to free() its cached memory, perhaps a
 *   negative value of the cache parameter (not backward compatible), or a new
 *   "free_cache" flag.  Note that FreeCache() already exists for internal use
 *   of devnm() when malloc() fails in mid-caching.
 *
 * - Embellish the cache flag to allow finer control, perhaps:
 *
 *	DEVNM_NOCACHE	0   /* do not use an existing cache		* /
 *	DEVNM_CACHE	1   /* use existing cache, create if none	* /
 *	DEVNM_NEWCACHE	2   /* force creation of new cache		* /
 *	DEVNM_FREECACHE	3   /* just free malloc'd memory and return 0	* /
 *
 * - Specify the starting directory to be other than /dev, such as /dev/ptym if
 *   one is looking for a particular master pty based on its device ID.  Would
 *   require a new "use_path" flag to indicate the path passed in is valid and
 *   should be used as a starting point.
 *
 * - For efficiency at the cost of robustness, skip a subdirectory upon
 *   encountering a pty master or slave file if the major number of the
 *   supplied dev_t is not a pty master or pty slave major number.  However,
 *   must always process the pty subdirs if the cache parameter is true, in
 *   case of another call with cache true, for a char special file that is a
 *   pty.  This is probably not worthwhile.
 *
 *   (Perhaps there ought to be a header file (or perhaps even a data file)
 *   that contains macros for these numbers.  This file could also be used by
 *   SAM, lsdev, and other utilities that care about devices and their
 *   corresponding numbers.)
 *
 * - Control the previous (make it optional) with a new "skip_pty_dir" flag.
 *
 * - Combine the cache, free_cache, use_path, and skip_pty_dir flags in a
 *   single parameter using macro-ized flag values, for example (and possibly
 *   include the other cache controls listed above):
 *
 *	DEVNM_CACHE	   0x01
 *	DEVNM_USE_PATH	   0x02
 *	DEVNM_SKIP_PTY_DIR 0x04
 *
 * - Improve the caching method, which is currently merely a singly linked
 *   list.  Perhaps a simple hash algorithm with linked list overflow would
 *   work very well.  Use dev_t as the key and keep the array of buckets fairly
 *   small (say 113 or so).  The amount of extra memory used would be minimal.
 *   To maintain ordering, would also need to keep tail pointers for each hash
 *   bucket (or scan to the end of the chain when inserting a new element).
 *
 *   Testing with nearly 3000 special files under /dev on an S370 showed the
 *   linked list was not bad.  Even when it took nearly 500 clock ticks (at
 *   1/50 second each) to set up the cache, the entire cache was then re-read
 *   in only 1/3 clock tick on average on the next call to devnm().
 */

/* LINTLIBRARY -- for lint */

/*
 * Call real names of lib routines:
 *
 * Only need to include those that are part of devnm(), not the DEBUG code.
 * These were found using nm on devnm.o.
 */

#ifdef _NAMESPACE_CLEAN
#define	ftw	_ftw
#define	strcpy	_strcpy
#define	strlen	_strlen
#define	strncpy	_strncpy
#define devnm	_devnm
#ifdef _ANSIC_CLEAN
#define	malloc	_malloc
#define	free	_free
#endif /* _ANSIC_CLEAN	   */
#endif /* _NAMESPACE_CLEAN */

#include <sys/types.h>		/* for various types	*/
#include <sys/stat.h>		/* for stat() values	*/
#include <stdlib.h>		/* for malloc()		*/
#include <string.h>		/* for str*()		*/
#include <ftw.h>		/* for FTW_* values	*/


/****************************************************************************
 * GLOBAL VALUES:
 */

#define	CHNULL	'\0'

#define	FALSE	0
#define	TRUE	1

#define	RETURN_FOUND	   0	/* return values from devnm() */
#define	RETURN_FTWERR	 (-1)
#define	RETURN_NOTFOUND	 (-2)
#define	RETURN_TRUNCATED (-3)

static	char	g_startpath[] = "/dev";		/* under which to search */

static	mode_t	g_devtype;	/* globals of parameters for subroutines */
static	dev_t	g_devid;
static	char *	g_path;
static	size_t	g_pathlen;

static	int	g_nomalloc;	/* set by OneFileCache() */
static	int	g_notfound;	/* set by SavePath()	 */
static	int	g_truncated;


/****************************************************************************
 * CACHE ENTRIES:
 *
 * These definitions are used for a linked list of cache entry nodes, one for
 * each file found by ftw(), in malloc'd memory.  The struct elements are
 * ordered for maximal packing => minimal space, assuming mode_t is 2 bytes.
 * Using an "immediate array" for path saves memory, and making it 2 bytes
 * minimizes memory used, assuming element sizes and padding, with no risk.
 */

typedef	struct cache * cachep_t;

struct cache {			/* holds info about one file		 */
	cachep_t nextp;		/* next in chain, or CACHEPNULL	 4 bytes */
	dev_t	 devid;		/* s_rdev value for this file	 4 bytes */
	mode_t	 devtype;	/* S_IFBLK or S_IFCHR		 2 bytes */
	char	 path [2];	/* path name in malloc'd memory	 2 bytes */
};

#define	CACHEPNULL ((cachep_t) 0)
#define	CACHESIZE  (sizeof (struct cache))

static	cachep_t g_cacheheadp = CACHEPNULL;	/* head of chain */
static	cachep_t g_cachelastp;			/* last in chain */


/****************************************************************************
 * FUNCTION TYPES:
 */

	int	devnm();		/* the entry point		*/
static	int	OneFile();		/* called by ftw()		*/
static	int	OneFileCache();		/* called by ftw()		*/
static	void	SearchCache();		/* called by devnm()		*/
static	void	FreeCache();		/* in case of malloc() failure	*/
static	void	SavePath();		/* called by subs to save path	*/


/****************************************************************************
 * D E V N M
 *
 * See the manual entry, devnm(3), for an explanation.
 */

#ifdef _NAMESPACE_CLEAN
#undef devnm
#pragma _HP_SECONDARY_DEF _devnm devnm
#define devnm _devnm
#endif

int devnm (devtype, devid, path, pathlen, cache)
	mode_t	devtype;	/* stat() S_* value to select	 */
	dev_t	devid;		/* from stat() st_dev or st_rdev */
	char *	path;		/* to return result		 */
	size_t	pathlen;	/* maximum chars allowed in path */
	int	cache;		/* flag to control caching	 */
{
static	int	have_cache = FALSE;	/* flag:  built a cache	 */
static	int	cant_cache = FALSE;	/* flag:  build failed	 */

#define	FTW_DEPTH 10  /* assume 10 file descriptors available and sufficient */

/*
 * SET GLOBALS FOR SUBROUTINES:
 *
 * The OneFile*() routines need globals because ftw() can't pass the devnm()
 * parameter values to them.
 *
 * All subroutines communicate back through globals as well because that's
 * simpler than (necessarily) mapping a return value passed through ftw().  If
 * they find a file path they set g_notfound FALSE and put a path name in
 * g_path, and set g_truncated as appropriate.
 */

	g_devtype = devtype & S_IFMT;	/* note, pre-masked */
	g_devid	  = devid;
	g_path	  = path;
	g_pathlen = pathlen;

	g_notfound = TRUE;

/*
 * SEARCH WITHOUT CACHING:
 */

	if ((! cache) || cant_cache)
	{
	    if (ftw (g_startpath, OneFile, FTW_DEPTH) < 0)  /* ftw() failed */
		return (RETURN_FTWERR);	      /* leave errno set for caller */
	}

/*
 * SEARCH AND CREATE CACHE:
 *
 * If ftw() fails, free any cache memory just allocated.  If malloc() fails,
 * set cant_cache for future calls and retry without caching.
 */

	else if (! have_cache)
	{
	    if (ftw (g_startpath, OneFileCache, FTW_DEPTH) < 0)
	    {
		FreeCache();
		return (RETURN_FTWERR);	  /* leave errno set for caller */
	    }

	    if (g_nomalloc)		/* malloc() failed in OneFileCache() */
	    {
		cant_cache = TRUE;
		return (devnm (devtype, devid, path, pathlen, FALSE));
	    }

	    have_cache = TRUE;
	}

/*
 * SEARCH EXISTING CACHE:
 */

	else
	{
	    SearchCache();
	}

/*
 * CHECK RESULTS:
 */

	return (g_notfound ?  RETURN_NOTFOUND :
		g_truncated ? RETURN_TRUNCATED :
			      RETURN_FOUND);

} /* devnm */


/****************************************************************************
 * O N E   F I L E
 *
 * Call from ftw() to handle one file when caching is not in effect, given
 * global g_* variables.  Ignore the file if any of the following are true:
 *
 * - file is an unreadable directory
 * - stat() failed for the file
 * - file is not of the right type, using global g_devtype
 * - file st_rdev does not match the desired g_devid
 *
 * Upon finding a match, set g_notfound FALSE, copy path to g_path, set
 * g_truncated as appropriate, and return 1 to stop ftw(); otherwise return 0
 * to keep searching.
 *
 * This is a separate routine from OneFileCache() for speed and simplicity at
 * the cost of some replicated code.
 */

static int OneFile (path, statp, status)
	char *	path;		/* this file's path */
	struct stat * statp;	/* info about file  */
	int	status;		/* FTW_* value	    */
{
	if ((status == FTW_DNR) || (status == FTW_NS) || (status == FTW_D)
	 || (((statp -> st_mode) & S_IFMT) != g_devtype)
	 || ( (statp -> st_rdev) != g_devid))
	{
	    return (0);
	}

	SavePath (path);		/* sets g_* values */
	return (1);

} /* OneFile */


/****************************************************************************
 * O N E   F I L E   C A C H E
 *
 * This is the equivalent of OneFile() (see comments above) with the big
 * exception that it creates a cache on the fly for future reuse.  Call it from
 * ftw() when no cache exists and one is to be created while searching.
 *
 * For any block or char special file, save the file's info in a node appended
 * to the g_cacheheadp chain.  The first time (only) that a matching file is
 * found, set g_notfound FALSE, copy path to g_path, and set g_truncated as
 * appropriate.
 *
 * Return 0 to continue ftw() through the whole tree unless malloc() fails; in
 * that case, free any existing cache entries, set g_nomalloc, and return 1 to
 * stop ftw().
 */

static int OneFileCache (path, statp, status)
	char *	path;		/* this file's path */
	struct stat * statp;	/* info about file  */
	int	status;		/* FTW_* value	    */
{
	cachep_t cachep;	/* new cache entry  */

/*
 * IGNORE IRRELEVANT FILE:
 *
 * This includes unreadable directories, non-stat-able files, other
 * directories, and files other than block or char special.
 */

	if ((status == FTW_DNR) || (status == FTW_NS) || (status == FTW_D)
	 || (! (S_ISBLK (statp -> st_mode) || S_ISCHR (statp -> st_mode))))
	{
	    return (0);
	}

/*
 * CACHE THIS FILE'S INFO:
 *
 * If cannot allocate space, bail out.  Note that struct cache already contains
 * two bytes usable for path.
 */

	if ((cachep = malloc (CACHESIZE + strlen (path) - 1)) == CACHEPNULL)
	{
	    FreeCache();			/* any previous nodes */
	    g_nomalloc = TRUE;
	    return (1);
	}

	(void) strcpy (cachep -> path, path);

	(cachep -> devtype) = (statp -> st_mode) & S_IFMT;
	(cachep -> devid)   = (statp -> st_rdev);
	(cachep -> nextp)   = CACHEPNULL;

/*
 * APPEND NEW CACHE ENTRY TO LIST:
 *
 * The new entry could be pushed at the head of the chain with less effort,
 * thereby reversing the order of the entries, but this would lead to possibly
 * different results for a given device type and ID the first time while
 * building a cache and later while using the cache, if two or more special
 * files have the same type and ID.  Never mind that ftw() searches the tree in
 * essentially random order anyway!  Be internally consistent.
 */

	if (g_cacheheadp == CACHEPNULL)		/* no current list */
	    g_cachelastp = (g_cacheheadp = cachep);
	else
	    g_cachelastp = ((g_cachelastp -> nextp) = cachep);

/*
 * HANDLE MATCHING FILE:
 *
 * Only save the first one encountered.  Return 0 even if a file is found, to
 * continue cache building.
 */

	if (g_notfound
	 && (((statp -> st_mode) & S_IFMT) == g_devtype)
	 && ( (statp -> st_rdev) == g_devid))
	{
	    SavePath (path);		/* sets g_* values */
	}

	return (0);

} /* OneFileCache */


/****************************************************************************
 * S E A R C H   C A C H E
 *
 * Given the cache list starting with g_cacheheadp, and other global g_*
 * values, search the cache for a file matching g_devtype and g_devid.  Note
 * that g_devtype and cachep -> devtype are both already masked to S_IFMT bits
 * only.  If found, set g_notfound FALSE, copy path to g_path, and set
 * g_truncated as appropriate.  Otherwise leave g_path unaltered.
 *
 * This is a simple linear search, but it seems fast enough for typical /dev
 * trees.
 */

static void SearchCache()
{
	cachep_t cachep;	/* current entry */

	for (cachep  = g_cacheheadp;
	     cachep != CACHEPNULL;
	     cachep  = cachep -> nextp)
	{
	    if ((g_devtype == (cachep -> devtype))
	     && (g_devid   == (cachep -> devid)))
	    {
		SavePath (cachep -> path);	/* sets g_* values */
		return;
	    }
	}

} /* SearchCache */


/****************************************************************************
 * F R E E   C A C H E
 *
 * Given global g_cacheheadp, free all nodes in the list, if any, and leave
 * g_cacheheadp set to CACHEPNULL.
 */

static void FreeCache()
{
	cachep_t cachep;	/* current entry */

	while (g_cacheheadp != CACHEPNULL)
	{
	    cachep	 = g_cacheheadp;
	    g_cacheheadp = g_cacheheadp -> nextp;

#ifdef DEBUG				/* dump cache list for checking */
	    (void) printf ("0%o 0x%x %s\n",
			   cachep -> devtype,
			   cachep -> devid,
			   cachep -> path);
#endif

	    free ((void *) cachep);
	}

} /* FreeCache */


/****************************************************************************
 * S A V E   P A T H
 *
 * Given a path string for a file found during searching, and global g_*
 * values, set g_notfound FALSE, copy the string to g_path with truncation if
 * necessary, and set g_truncated accordingly.  Note that g_pathlen includes a
 * trailing CHNULL.
 */

static void SavePath (path)
	char * path;
{
	g_notfound = FALSE;

	if (! (g_truncated = (strlen (path) >= g_pathlen)))	/* it fits */
	{
	    (void) strcpy (g_path, path);
	    return;
	}

	(void) strncpy (g_path, path, g_pathlen);	/* truncate string */

	if (g_pathlen > 0)
	    g_path [g_pathlen - 1] = CHNULL;		/* terminate string */

} /* SavePath */


#ifdef DEBUG

/****************************************************************************
 * DEBUGGING SUPPORT:
 */

#include <sys/times.h>		/* for calling times() */
#include <errno.h>
#include <stdio.h>

#define	CPNULL	((char *) 0)

	int	main();
	int	TimeCall();


/****************************************************************************
 * M A I N
 *
 * Take each argument to be a special file name and use it to get a device type
 * and ID.  Then call devnm() several times with performance numbers dumped.
 */

int main (argc, argv)
	int	argc;
	char **	argv;
{
	char *	myname = *argv;
	struct stat statbuf;
	time_t	basetime;		/* base time for comparisons */
	char	path [BUFSIZ];		/* for return from devnm()   */

/*
 * CHECK USAGE:
 */

	if (argc < 2)
	{
	    (void) fprintf (stderr, "usage:  %s device_file ...\n", myname);
	    exit (1);
	}

/*
 * STAT EACH ARG:
 */

	while (*(++argv) != CPNULL)
	{
	    if (stat (*argv, & statbuf) < 0)
	    {
		(void) printf ("\ncannot stat file \"%s\" to get device type and ID; will use zero\n",
				*argv);

		statbuf.st_mode = 0;
		statbuf.st_rdev = 0;
	    }

	    (void) printf ("\nFile path:  %s\n",   *argv);
	    (void) printf (  "File mode:  0%o\n",  statbuf.st_mode);
	    (void) printf (  "Device ID:  0x%x\n", statbuf.st_rdev);

/*
 * CALL devnm DIFFERENT WAYS:
 */

	    basetime = TimeCall (statbuf.st_mode, statbuf.st_rdev, path,
				 BUFSIZ, FALSE, 0);

	    (void) TimeCall (statbuf.st_mode, statbuf.st_rdev, path,
			     BUFSIZ, TRUE, basetime);

	    (void) TimeCall (statbuf.st_mode, statbuf.st_rdev, path,
			     BUFSIZ, TRUE, basetime);

	} /* while */

	exit (0);

	/* NOTREACHED -- for lint */

} /* main */


/****************************************************************************
 * T I M E   C A L L
 *
 * Given devnm() parameters plus a base time for comparison (0 for first call),
 * call devnm(), report results, and also report the time taken.  Return the
 * time taken for this run.
 *
 * After the first call with cache true, call devnm() many times and report the
 * average, since it tends to be less than one clock tick.
 */

int TimeCall (devtype, devid, path, pathlen, cache, basetime)
	mode_t	devtype;
	dev_t	devid;
	char *	path;
	size_t	pathlen;
	int	cache;
	time_t	basetime;
{
static	int	cache_done = FALSE;	/* flag: already have a cache */
#define	LOOPS	1000			/* times to repeat call	      */
	int	loop;			/* loop index		      */

	struct tms times_before;	/* from times(2)	*/
	struct tms times_after;
	time_t	thistime;		/* based on times(2)	*/
	int	retval;			/* from devnm()		*/

/*
 * CALL devnm() ONCE OR MANY TIMES:
 */

	if (! (cache && cache_done))		/* call once */
	{
	    (void) times (& times_before);
	    retval = devnm (devtype, devid, path, pathlen, cache);
	    (void) times (& times_after);
	}
	else					/* will use cache, call many */
	{
	    (void) times (& times_before);

	    for (loop = 1; loop < LOOPS; ++loop)
		(void) devnm (devtype, devid, path, pathlen, cache);

	    retval = devnm (devtype, devid, path, pathlen, cache);

	    (void) times (& times_after);
	}

/*
 * REPORT RESULTS:
 */

	(void) printf ("\n%s devnm() returned %d, errno %d\n",
		       cache ? "cached" : "uncached", retval, errno);

	if ((retval == 0) || (retval == RETURN_TRUNCATED))
	    (void) printf ("path = \"%s\"\n", path);

	thistime = (times_after.tms_utime  + times_after.tms_stime)
		 - (times_before.tms_utime + times_before.tms_stime);

	if (! basetime)
	{
	    (void) printf ("elapsed user + sys clock ticks:  %d\n", thistime);
	}
	else if (cache && cache_done)
	{
	    (void) printf ("elapsed user + sys clock ticks:  %.3f  (avg; %.0f%% of base)\n",
			   ((float) thistime) / LOOPS,
			   100.0 * thistime / LOOPS / basetime);
	}
	else
	{
	    (void) printf ("elapsed user + sys clock ticks:  %d  (%.0f%% of base)\n",
			   thistime, 100.0 * thistime / basetime);
	}

	if (cache)
	    cache_done = TRUE;

	return ((int) (thistime + 0.5));

} /* TimeCall */

#endif /* DEBUG */
