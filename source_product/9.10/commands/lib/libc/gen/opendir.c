/* @(#) $Revision: 70.1 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   define opendir _opendir
#   define open _open
#   define fstat _fstat
#   define close _close
#   define fcntl _fcntl
#   ifdef   _ANSIC_CLEAN
#       define malloc _malloc
#       define free _free
#   endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include "dirchk.h"

/*
 * This version of opendir() is called directly by getcwd(3c) to avoid
 * an extra fstat() system call.  Since this is the most expensive
 * operation of getcwd(), eliminating the need for an extra fstat()
 * call greatly improves getcwd(3c) performance.
 */
DIR *
__opendir(name, pst)
char *name;
struct stat *pst;
{
    extern char *malloc();
    register DIR *dirp;
    register int fd;
    int size;

    if ((fd = open(name, 0)) == -1)
	return (DIR *)NULL;

    if (fstat(fd, pst) == -1)
    {
	close(fd);
	return (DIR *)NULL;
    }

    if ((pst->st_mode & S_IFMT) != S_IFDIR)
    {
	errno = ENOTDIR;
	close(fd);
	return (DIR *)NULL;
    }

    /* POSIX 1003.1: set close-on-exec flag */
    {
	union
	{
	    int val;
	    struct flock *lockdes;
	} arg;

	arg.val = 1;
	if (fcntl(fd, F_SETFD, arg) == -1)
	{
	    close(fd);
	    return NULL;
	}
    }

#ifdef HP_NFS
    /*
     * We malloc the space for the following 3 elements as one chunk
     * which are placed, in order, in the buffer:
     *
     *   o) The DIR structure
     *   o) A buffer for the directory data read using getdirentries(2)
     *
     * Each space is aligned to a word boundry.
     */
    size = WORD_PAD(sizeof (DIR)) + pst->st_blksize;
#else
    /*
     * We malloc the space for the following 2 elements as one chunk
     * which are placed, in order, in the buffer:
     *
     *   o) The DIR structure
     *   o) One (struct dirent) structure [readdir() puts the
     *      directory entry in this space and returns a pointer to it]
     *
     * Each space is aligned to a word boundry.
     */
    size = WORD_PAD(sizeof (DIR));
#endif

    if ((dirp = (DIR *)malloc(size)) == (DIR *)NULL)
    {
	close(fd);
	return (DIR *)NULL;
    }

#ifdef HP_NFS
    /*
     * dirp->dd_buf is set to the end of the malloc-ed region, just
     * after the DIR and the space reserved for one (struct dirent).
     */
    dirp->dd_buf = (char *)dirp + WORD_PAD(sizeof (DIR));
    dirp->dd_bsize = pst->st_blksize;
    dirp->dd_bbase = 0;
    dirp->dd_entno = 0;
#endif
    dirp->dd_fd = fd;
    dirp->dd_loc = 0;
    dirp->dd_size = 0;

    return dirp;
}

/*
 * open a directory.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   undef opendir
#   pragma _HP_SECONDARY_DEF _opendir opendir
#   define opendir _opendir
#endif

DIR *
opendir(name)
char *name;
{
    struct stat st;

    return __opendir(name, &st);
}
