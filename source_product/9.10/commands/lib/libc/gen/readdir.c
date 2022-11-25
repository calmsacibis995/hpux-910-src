/* @(#) $Revision: 66.2 $ */

#ifdef _NAMESPACE_CLEAN
#   define readdir _readdir
#   define getdirentries _getdirentries
#endif

#include <sys/errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "dirchk.h"

/*
 * get next entry in a directory.
 */

#ifdef _NAMESPACE_CLEAN
#   undef readdir
#   pragma _HP_SECONDARY_DEF _readdir readdir
#   define readdir _readdir
#endif

struct dirent *
readdir(dirp)
register DIR *dirp;
{
    extern int errno;
    register struct dirent *dp;
    static struct dirent *dir;

    if (!GOOD_DIR(dirp))
    {
	errno = EBADF;
	return (struct dirent *)0;
    }

    for (;;)
    {
	if (dirp->dd_loc == 0)
	{
#ifdef HP_NFS
	    dirp->dd_size = getdirentries(dirp->dd_fd, dirp->dd_buf,
					  dirp->dd_bsize,
					  &dirp->dd_bbase);
#else
	    dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ);
#endif
	    if (dirp->dd_size <= 0)
		return (struct dirent *)0;
	}

	if (dirp->dd_loc >= dirp->dd_size)
	{
	    dirp->dd_loc = 0;
#ifdef HP_NFS
	    dirp->dd_entno = 0;
#endif
	    continue;
	}

	dp = (struct dirent *)(dirp->dd_buf + dirp->dd_loc);
#ifdef HP_NFS
	dirp->dd_loc += dp->d_reclen;
	dirp->dd_entno++;
#else
	dirp->dd_loc += sizeof (struct dirent);
#endif

	if (dp->d_ino == 0)
	    continue;

	dir = dp;
	return dir;
    }
}
