/*
 *  $Header: walkfs.c,v 70.1 92/02/22 13:43:58 ssa Exp $
 *  walkfs.c -- file tree walk
 *
 * Authors: Byron T. Jenings Jr.
 *          R. Scott Holbrook
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ndir.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "walkfs.h"
#include <ftw.h>
#include <nl_ctype.h>

#define TRUE         1
#define FALSE        0
#define ENDOF(s) (&s[strlen(s)])

extern char *malloc();
extern int stat();
#ifdef SYMLINKS
extern int lstat();
#endif /* SYMLINKS */

#if defined(DUX) || defined(DISKLESS)
#define ISCDF(st) (((st).st_mode & (S_IFMT|S_CDF)) == (S_IFDIR|S_CDF))
#endif

static char Fftw;
static char Fquick;
static char Fpop;
static char Fdocdf;
static char Fstripdot;
static char Fshowcdf;
static int relpath;
static int (*statfn)();
static int (*fn)();

typedef struct
{
    struct walkfs_info i;
    int dot_removed;		/* TRUE if path ended in /.           */
    DIR *dirp;			/* DIR file descriptor                */
    long here;                  /* saved place in dirp                */
} state_t;

/*
 * maybe_mountpoint() -- Return TRUE if 'kid' might be a mount point
 */
static int
maybe_mountpoint(dad, kid)
struct stat *dad;
struct stat *kid;
{
    /* 
     * If this isn't a directory, or the st_dev fields of the
     * parent match the kid and the kid's inode is not 2 (HFS
     * mount point) it is not a mount point.  Since '/' is a
     * mount point but "." has the same dev field as "..", we
     * have to have this extra test for kid->st-ino != 2.
     */
    if ((kid->st_mode & S_IFMT) != S_IFDIR ||
	(kid->st_dev == dad->st_dev && kid->st_ino != 2))
	return FALSE;

#ifdef FSTYPE_KLUDGE  /* no st_fstype field returned by stat(2) */
    /*
     * Don't know what type of filesystem we are on so just return
     * TRUE if the inode is 2 (HFS mountpoint) or the dev fields are
     * different.
     */
    return kid->st_ino == 2 || kid->st_dev != dad->st_dev;
#else
    /* 
     * If the current directory is of type HFS, we just see if
     * the inode is 2 (the mount point is always inode 2 on HFS).
     * If this is not HFS, we can only see if the dev fields 
     * of this entry and its parent are different.
     */
    return (kid->st_fstype == MOUNT_UFS) ?
	    kid->st_ino == 2 : kid->st_dev != dad->st_dev;
#endif
}

/*
 * ismountpoint() -- Determine if this is a mount point.
 */
static int
ismountpoint(path, dad, kid)
char *path;
struct stat *dad;
struct stat *kid;
{
    int rc = maybe_mountpoint(dad, kid);

#ifndef SYMLINKS
    return rc;
#else
    /* 
     * If maybe_mountpoint() returned FALSE or we are using lstat()
     * to stat things, we just return what maybe_mountpoint()
     * returned.
     */
    if (!rc || statfn == lstat)
	return rc;

    /* 
     * This might be a mount point and we are not using lstat().
     * Lstat() the file and see if it is a symlink.  If it is, return
     * FALSE, otherwise return TRUE.
     */
    {
	struct stat st;

	return lstat(path, &st) != -1 &&
	    (st.st_mode & S_IFMT) != S_IFLNK;
    }
#endif /* SYMLINKS */
}

/*
 * checkmount() -- Determine if the starting point is a mount point.
 */
static int
checkmount(info)
struct walkfs_info *info;
{
    struct stat pst;
    int rc;

#if defined(DUX) || defined(DISKLESS)
    strcpy(info->endp, "/..+");
    if ((rc = (*statfn)(info->relpath, &pst)) == -1 || !ISCDF(pst))
#endif
    {
	strcpy(info->endp, "/..");
	rc = (*statfn)(info->relpath, &pst);
    }

    *(info->endp) = '\0';
    if (rc == -1)
	return FALSE;
    errno = 0;

    return ismountpoint(info->shortpath, &pst, &info->st);
}

/*
 * Requires info->endp and info->basep to be set properly when called.
 *
 * do_stat --
 *    Stat a file, returning one of the following
 *    codes:
 *       WALKFS_NOSTAT  -- No stat, Stat failed
 *       WALKFS_NONDIR  -- File
 *       WALKFS_DIR     -- Directory
 *
 *       If WALKFS_NOSTAT is returned, the global "errno" will indicate
 *       the error.
 *
 * As a side effect, the following info fields will be set:
 *	info->st
 *	info->shortpath
 *	info->ismountpoint
 */
static
int
do_stat(info, inode)
struct walkfs_info *info;
register ino_t inode;
{
    int rc = -1;

    /*
     * Build the name to use for the stat() call.
     * Use relpath if in slow mode or this is the start point,
     * otherwise use basep.
     */
    info->shortpath =
	       (Fquick && info->parent != NULL) ? info->basep : info->relpath;

#if defined(DUX) || defined(DISKLESS)
    if (Fdocdf)
    {
	/* 
	 * Optimization:  If we know what the inode of the
	 * file should be, use that to detect CDF's instead
	 * of stat-ing the file with a trailing +.  Since
	 * CDF's are rare, this first stat will usually
	 * succeed.  If the stat without the '+' succeeds but
	 * is not the file that we expected, we pretend that the
	 * stat failed (so that we will stat it later with a '+').
	 */
	if (inode != 0)
	{
	    rc = (*statfn)(info->shortpath, &info->st);
	    if (rc != -1 && info->st.st_ino != inode)
		rc = -1;
	}

	/* See if the file is a CDF:  */
	if (rc == -1)
	{
	    *info->endp++ = '+';
	    *info->endp = '\0';
	    if ((rc = (*statfn)(info->shortpath, &info->st)) == -1 ||
	        !ISCDF(info->st))
	    {
		*--info->endp = '\0';
		rc = -1;
	    }
	}
    }
#endif /* defined(DUX) || defined(DISKLESS) */

    /* Not a CDF, try stat-ing the file */
    if (rc == -1)
	rc = (*statfn)(info->shortpath, &info->st);

    if (rc == -1)
    {
	info->ismountpoint = FALSE;
	return WALKFS_NOSTAT;
    }

    errno = 0;
    info->ismountpoint = info->parent == NULL ?
			   checkmount(info) : 
			   ismountpoint(info->shortpath, &info->parent->st, &info->st);

    return (info->st.st_mode & S_IFMT) == S_IFDIR ? 
	WALKFS_DIR : WALKFS_NONDIR;
}

/*
 * See if this entry is executable (searchable).  Root (uid=0)
 * can search no matter what.  For non-root, we must
 * check the correct execute bits.
 */
static int
isexec(st)
struct stat *st;
{
    extern unsigned short geteuid(), getegid();
    unsigned short uid = geteuid();

    if (uid == 0)
	return TRUE;
    if (uid == st->st_uid)
	return (st->st_mode & S_IEXEC) != 0;
    if (getegid() == st->st_gid)
	return (st->st_mode & (S_IEXEC>>3)) != 0;
    return (st->st_mode & (S_IEXEC>>6)) != 0;
}

/*
 * getdir --
 *   Open a directory, saving our place in the parent directory if we
 *   have used all the file descriptors that we were told we could or
 *   we run out.  If here is not -1, we don't save and close the
 *   parent directory (we assume that it has already been done).
 */
static void
getdir(parentstate, state, depth)
state_t *parentstate;
state_t *state;
int depth;
{
    register char *path = Fquick ? state->i.shortpath : state->i.relpath;

    if (parentstate && parentstate->here == -1 && depth < 1)
    {
	parentstate->here = telldir(parentstate->dirp);
	closedir(parentstate->dirp);
    }

    if ((state->dirp = opendir(path)) == NULL &&
	errno == EMFILE && parentstate && parentstate->here == -1)
    {
	parentstate->here = telldir(parentstate->dirp);
	closedir(parentstate->dirp);
	state->dirp = opendir(path);
    }
}

/*
 * real_walkfs --
 *    Recursive function that processing directories.  The parameter
 *    'inode' indicates the inode # that we expect from a stat of
 *    the current entry.  If we don't know what we expect (initial
 *    condition), 'inode' will be 0 (no file ever has an st_ino == 0).
 */
/* VARARGS */
static
int
real_walkfs(parentstate, depth, inode, fpath, fpathbuflen)
state_t *parentstate;
int depth;
ino_t inode;
char *fpath;
int fpathbuflen;
{
    state_t state;		/* information about this call's file */
    struct direct *dp;		/* directory entry                    */
    int rc;			/* Return code from user function     */
    int retry = 0;		/* Retry bit to OR into flags         */
    int did_chdir = FALSE;	/* did we do a chdir successfully     */
    int can_srch;		/* can we search this directory       */
    char *new_buffer;		/* new buffer if used */

    if (parentstate == NULL)	/* is this the start point? */
    {
	state.i.parent = NULL;
	state.i.fullpath = fpath;	/* sent as a parameter */
	state.i.buflen = fpathbuflen;	/* length of buffer */
	state.i.relpath = fpath + relpath;
	state.i.shortpath = state.i.relpath;
	state.i.basep = strrchr(&fpath[relpath], '/');
	if (state.i.basep == NULL)
	    state.i.basep = &fpath[relpath];
	else
	    state.i.basep++;
    }
    else
    {
	state.i.parent = &parentstate->i;
	state.i.basep = parentstate->i.endp;
	state.i.fullpath = parentstate->i.fullpath;
	state.i.shortpath = parentstate->i.shortpath;
	state.i.relpath = parentstate->i.relpath;
	state.i.buflen = parentstate->i.buflen;
    }

    state.i.endp = ENDOF(state.i.basep);
    state.dirp = NULL;
    state.here = -1;

    /*
     * If there is not enough room for the next path component in our
     * path buffer, allocate a new buffer that is twice as large
     * as the old one. Duplicate the previous buffer, preserving 
     * the relative position of the various pointers in the buffer
     *
     * Set new_buffer to point to the newly allocated buffer. Before
     * we return, make sure to free the buffer if it is not null.
     */
     if ((state.i.buflen - (state.i.endp - state.i.fullpath)) <= MAXNAMLEN)
     {
	int off_relpath = state.i.relpath - state.i.fullpath;
	int off_shortpath = state.i.shortpath - state.i.fullpath;
	int off_basep = state.i.basep - state.i.fullpath;
	int off_endp = state.i.endp - state.i.fullpath;

	new_buffer = malloc(state.i.buflen * 2);
	strcpy(new_buffer, state.i.fullpath);
	state.i.fullpath = new_buffer;
	state.i.relpath = state.i.fullpath + off_relpath;
	state.i.shortpath = state.i.fullpath + off_shortpath;
	state.i.basep = state.i.fullpath + off_basep;
	state.i.endp = state.i.fullpath + off_endp;
	state.i.buflen *= 2;
     }
     else
	 new_buffer = (char *)0;

    /* 
     * Repeatedly stat this entry until the stat succeeds or the user
     * says to give up.  If it is just a file, call the user function.
     * If it is a directory, set up for a recursive call to ourselves.
     */
    for (;;)
    {
	switch (do_stat(&state.i, inode))
	{
	case WALKFS_NOSTAT: 
	    /* 
	     * We ignore ENOENT errors except at the start point. 
	     * These errors can occur in non-WALKFS_DOCDF mode when
	     * there is no match for our context or if the file
	     * disappears between the time we read the directory and
	     * we get around to stating it.
	     */
	    if (parentstate != NULL && errno == ENOENT)
	    {
		errno = 0;
		if (new_buffer != (char *)0)
		    free(new_buffer);
		return WALKFS_OK;
	    }

	    if (Fftw)
	    {
		rc = (*fn)(state.i.relpath, &state.i.st, FTW_NS);
		if (new_buffer != (char *)0)
		    free(new_buffer);
		return rc;
	    }

	    rc = (*fn)(&state.i, retry | WALKFS_NOSTAT);
	    if (rc == WALKFS_RETRY)
	    {
		retry = WALKFS_RETRY;
		continue;
	    }
	    if (new_buffer != (char *)0)
		free(new_buffer);
	    return rc == WALKFS_SKIP ? WALKFS_OK : rc;
	case WALKFS_NONDIR: 
	    if (Fftw)
		rc = (*fn)(state.i.relpath, &state.i.st, FTW_F);
	    else
	    	rc = (*fn)(&state.i, retry | WALKFS_NONDIR);
	    if (new_buffer != (char *)0)
		free(new_buffer);
	    return rc;
	case WALKFS_DIR:
	    {
		/*
		 * A regular directory.  We must first look up the
		 * parent chain to see if this directory is currently
		 * active.  If so, this directory creates a cycle, so
		 * we call the user function with WALKFS_CYCLE and
		 * treat this entry as a terminal node.
		 */
		struct walkfs_info *parent = state.i.parent;
		dev_t dev = state.i.st.st_dev;
		ino_t ino = state.i.st.st_ino;

		while (parent != (struct walkfs_info *)0)
		{
		    if (parent->st.st_ino == ino &&
			parent->st.st_dev == dev)
		    {
			if (Fftw)
			{
			    errno = ELOOP;
			    rc = (*fn)(state.i.relpath, &state.i.st, FTW_NS);
			    if (new_buffer != (char *)0)
				free(new_buffer);
			    return rc;
			}
			rc = (*fn)(&state.i, retry | WALKFS_CYCLE);
			if (new_buffer != (char *)0)
			    free(new_buffer);
			return rc;
		    }
		    parent = parent->parent;
		}
	    }
	    break;
	}

	/* 
	 * Directory processing.  Keep trying to open and chdir (Fquick)
	 * or search (!Fquick) this directory until we succeed or the
	 * user function tells us to give up.
	 */
	getdir(parentstate, &state, depth);

	if (Fftw)
	{
	    if (state.dirp == NULL)
	    {
		rc = (*fn)(state.i.relpath, &state.i.st, FTW_DNR);
		if (new_buffer != (char *)0)
		    free(new_buffer);
		return rc;
	    }
	    if ((rc = (*fn)(state.i.relpath, &state.i.st, FTW_D)) != 0)
	    {
		if (new_buffer != (char *)0)
		    free(new_buffer);
		return rc;
	    }
	    break;
	}

	if (Fquick && state.dirp != NULL)
	{
	    if (can_srch = did_chdir =
		    (chdir(state.i.shortpath) != -1))
		state.i.shortpath = ".";
	}
	else if (!(can_srch = isexec(&(state.i.st))))
	    errno = EACCES;

	if (state.dirp == NULL || !can_srch)
	{
	    int flags = retry | WALKFS_ERROR;

	    if (state.dirp == NULL)
		flags |= WALKFS_NOREAD;
	    if (!can_srch)
		flags |= WALKFS_NOSEARCH;

	    if ((rc = (*fn)(&state.i, flags)) == WALKFS_RETRY)
	    {
		retry = WALKFS_RETRY;
		if (state.dirp != NULL)
		{
		    closedir(state.dirp);
		    state.dirp = NULL;
		}
		continue;
	    }
	    rc = WALKFS_SKIP;
	    break;
	}

	fcntl(state.dirp->dd_fd, F_SETFD, 1); /* close dir on exec */

	/*
	 * Call the user function for the directory
	 */
	if ((rc = (*fn)(&state.i, retry | WALKFS_DIR)) == WALKFS_RETRY)
	    rc = WALKFS_OK;
	break;
    }

    /* 
     * Read the directory one entry at a time.  We ignore "." and
     * "..", stat'ing all other entries.  All entries get processed
     * by a recursive call.
     */
    if (rc == WALKFS_OK)
    {
	int slash_added = FALSE;

	state.dot_removed = FALSE;
	/* 
	 * This endp[-x] stuff is dependent on an absolute path
	 * name being in 'fullpath'.  Since Fstripdot is always
	 * FALSE in Fftw mode, this is not a problem.
	 */
	if (Fstripdot && state.i.endp[-1] == '.' &&
		state.i.endp[-2] == '/')
	{
	    /* remove redundant "." from end of path */
	    *--state.i.endp = '\0';
	    state.dot_removed = TRUE;
	}
	else if (state.i.endp[-1] != '/')
	{
	    *state.i.endp++ = '/';  /* Add a trailing slash */
	    slash_added = TRUE;
	}

	while ((rc == WALKFS_OK || (rc == WALKFS_RETRY && !Fftw)) &&
		(dp = readdir(state.dirp)) != NULL)
	{
	    register char *nm = dp->d_name;

	    /* skip entries for deleted links */
	    if (dp->d_ino == 0 || *nm == '\0')
		continue;

	    /* skip . and .. entries */
	    if (nm[0] == '.' &&
		    (nm[1] == '\0' || (nm[1] == '.' && nm[2] == '\0')))
		continue;

	    strcpy(state.i.endp, nm);
	    rc = real_walkfs(&state, depth - 1, dp->d_ino);
	}

	/* restore original directory name */
	if (slash_added)
	    state.i.endp--;
	if (state.dot_removed)
	    *state.i.endp++ = '.';
	*state.i.endp = '\0';
    }

    /* 
     * Call the user function to indicate that we are done with
     * this directory.  We do this before modifying any of the
     * fields of state.i so that it has the same stuff as when we
     * called with WALKFS_DIR.  Note that we just called the user
     * function with WALKFS_DIR if we are in depth first mode.
     * (i.e. WALKFS_TELLPOPDIR is silly with WALKFS_DEPTH, but
     * it is allowed).
     */
    if (Fpop)
	(*fn)(&state.i, WALKFS_POPDIR);

    /* 
     * Restore parent's absolute path.  We need the absolute path
     * for chdir() to avoid problems with RFA and SYMLINKS.
     */
    if (state.i.basep != state.i.fullpath)
    {
	if (parentstate && parentstate->dot_removed)
	{
	    state.i.basep[0] = '.';
	    state.i.basep[1] = '\0';
	}
	else
	    state.i.basep[0] = '\0';
    }

    /*
     * Chdir to our parent directory.
     *
     * No need to chdir back if this is the top level of the recursion.
     * The "walkfs" function that called us originally will do that
     * for us.
     *
     * If an absolute chdir fails, try a relative one, but make sure
     * that we are where we expect to be (in case of symlinks, etc.).
     * If we still aren't where we want to be, call the user function
     * with WALKFS_NOCHDIR.
     *
     * At this point, the user function may return WALKFS_RETRY.
     * We then see if he got us to where we wanted to be -- If he
     * did, we proceed as if nothing happened; if not we again
     * call him with WALKFS_RETRY (until he gets us where we want
     * or he returns something other than WALKFS_RETRY).
     *
     * If the user function returns a value other than WALKFS_RETRY,
     * we return WALKFS_NOCHDIR, terminating all the levels of
     * recursion.
     */
    if (did_chdir && parentstate != NULL && lchdir(state.i.fullpath) == -1)
    {
	struct stat dot;
	dev_t dev = parentstate->i.st.st_dev;
	ino_t ino = parentstate->i.st.st_ino;

	if (chdir("..") == -1 || stat(".", &dot) == -1 ||
	    dot.st_ino != ino || dot.st_dev != dev)
	{
	    do
	    {
		rc = (*fn)(&(parentstate->i), WALKFS_NOCHDIR);
	    } while (rc == WALKFS_RETRY && (stat(".", &dot) == -1 ||
		      dot.st_ino != ino || dot.st_dev != dev));

	    /*
	     * If rc is WALKFS_RETRY, we must be where we wanted
	     * since we are out of the loop.  Otherwise, we abort with
	     * WALKFS_NOCHDIR.
	     */
	    if (rc != WALKFS_RETRY)
	    {
		if (new_buffer != (char *)0)
		    free(new_buffer);
		return WALKFS_NOCHDIR;
	    }
	}
    }

    if (state.dirp != NULL)
	closedir(state.dirp);

    if (parentstate && parentstate->here != -1)
    {
	/*
	 * If we are in "Fquick" mode, use a relative path to re-open
	 * the directory, in case fullpath is no longer valid but
	 * ".." was.
	 */
	seekdir((parentstate->dirp = opendir(Fquick ? "." : state.i.fullpath)),
		parentstate->here);
	parentstate->here = -1;
	fcntl(parentstate->dirp->dd_fd, F_SETFD, 1);
    }

    if (new_buffer != (char *)0)
	free(new_buffer);
    if (Fftw)
	return rc;
    return (rc == WALKFS_SKIP || rc == WALKFS_RETRY) ? WALKFS_OK : rc;
}

int
walkfs(in_path, in_fn, depth, cntl)
char *in_path;
int (*in_fn)();
int depth;
int cntl;
{
    char cwd[MAXPATHLEN];
    char *pathbuf;
    int pathbuflen = MAXPATHLEN;
    int fd_cwd;

    fn = in_fn;

    Fftw   = !!(cntl & HIBITI); /* undocumented hook for ftw[h](LIBC) */
    Fdocdf = !!(cntl & WALKFS_DOCDF);

    if (Fftw)
	Fshowcdf = Fpop = Fstripdot = Fquick = FALSE;
    else
    {
	/*
	 * Save commonly used flag values in global variables for
	 * efficiency.
	 */
	Fshowcdf  = !!(cntl & WALKFS_SHOWCDF);
	Fpop      = !!(cntl & WALKFS_TELLPOPDIR);
	Fstripdot =  !(cntl & WALKFS_LEAVEDOT);
	Fquick    =  !(cntl & WALKFS_SLOW);
    }

#ifdef SYMLINKS
    if (!Fftw && (cntl & WALKFS_LSTAT))
	statfn = lstat;
    else
#endif /* SYMLINKS */
	statfn = stat;

    if (!Fftw && (Fquick || in_path[0] != '/'))
#if defined(DUX) || defined(DISKLESS)
	if (Fshowcdf)
	    gethcwd(cwd, sizeof cwd);
	else
#endif /* defined(DUX) || defined(DISKLESS) */
	    getcwd(cwd, sizeof cwd);

    if (Fquick)				/* guaranteed to be !Fftw */
	fd_cwd = open(".", O_RDONLY);

    /* 
     * Make an absolute pathname out of in_path so we can change
     * to it safely even if we have SYMLINKS.  We don't make it
     * an absolute path name in WALKFS_FTW mode since we don't
     * need the absolute path in this case.
     */
    pathbuf = cwd;
    if (!Fftw && in_path[0] != '/')
    {
	relpath = strlen(pathbuf);	/* offset to relative path */
	pathbuf[relpath++] = '/';
    }
    else
	relpath = 0;
    pathbuf[relpath] = '\0';

    /*
     * Check if inpath can be copied to relpath in the first place,
     * including expanded CDFs.
     */
     if (strlen(in_path) >= (sizeof cwd - strlen(pathbuf) - MAXNAMLEN))
     {
	pathbuflen = 2 * MAXPATHLEN;
	pathbuf = (char *)malloc(pathbuflen);
	strcpy(pathbuf, cwd);
     }

    /*
     * Copy in_path to pathbuf at relpath.  If Fshowcdf is set, copy
     * the expanded version of in_path to relpath.
     */
#if defined(DUX) || defined(DISKLESS)
    if (Fshowcdf)
    {
	if (!getcdf(in_path, &pathbuf[relpath], sizeof cwd - strlen(pathbuf)))
	    strcpy(&pathbuf[relpath], in_path);
    }
    else
#endif /* defined(DUX) || defined(DISKLESS) */
	strcpy(&pathbuf[relpath], in_path);

    /*
     * Now call real_walkfs to do the real work.  Note that in
     * WALKFS_FTW mode, cwd is garbage, but Fquick is always false
     * in this case, so we don't have to worry (be happy!).
     */
    {
	int rc = real_walkfs(NULL, depth, 0, pathbuf, pathbuflen);

	if (Fquick)
	{
	    fchdir(fd_cwd);
	    close(fd_cwd);
	}

	if (pathbuf != cwd)
	    free(pathbuf);

	return rc;
    }
}

/*
 * lchdir(path) --
 *	change the current working directory to the given path.
 *	Unlike chdir(), this function can handle path greater than
 *	MAXPATHLEN in length.
 *
 * Returns:
 *  0	Success. Current working directory is changed to path.
 * -1	Failure. Current working directory is guaranteed to be
 *	unchanged. The global errno indicates the cause of failure.
 *
 * NOTE:This function will provide undefined results if any single
 *	component of the path is longer than MAXPATHLEN. This should
 *	not happen since the maximum component length is MAXNAMLEN.
 *	Error checking could have been added but this situation does
 *	not arise as lchdir() is used by walkfs().
 */
static int
lchdir(path)
char *path;
{
    int fd_cwd;

    if (strlen(path) < MAXPATHLEN)
	return chdir(path);

    if ((fd_cwd = open(".", O_RDONLY)) < 0)
	return -1;

    for (;;)
    {
	char *path2;

	/*
	 * Find the last '/' in path before MAXPATHLEN.
	 */
#if defined(NLS) || defined(NLS16)
	if (__cs_SBYTE)
	{
	    path2 = path + MAXPATHLEN - 1;
	    while (*path2 != '/')
		path2--;
	}
	else
	{
	    register char *tmp = path;
	    register char *endp = path + MAXPATHLEN - 1;

	    while (tmp < endp)
	    {
		if (*tmp == '/')
		    path2 = tmp;
		else
		    CHARADV(tmp);
	    }
	}
#else
	path2 = path + MAXPATHLEN - 1;
	while (*path2 != '/')
		path2--;
#endif  /* NLS */

	*path2 = '\0';
	if (chdir(path) == -1)
	{
	    *path2 = '/';		/* restore path */
	    fchdir(fd_cwd);		/* restore original cwd */
	    close(fd_cwd);
	    return -1;
	}
	*path2 = '/';			/* restore slash */
	path = path2 + 1;		/* next path */

	if (strlen(path) < MAXPATHLEN)
	{
	    if (chdir(path) == -1)
	    {
		fchdir(fd_cwd);	/* restore original cwd */
		close(fd_cwd);
		return -1;
	    }
	    close(fd_cwd);
	    return 0;		/* success */
	}
    }
    /* NOTREACHED */
}
