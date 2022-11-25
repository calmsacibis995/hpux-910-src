/* @(#) $Revision: 66.2 $ */

#ifdef _NAMESPACE_CLEAN
#   define closedir _closedir
#   define close _close
#   ifdef   _ANSIC_CLEAN
#       define free _free
#   endif
#endif

#include <sys/errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "dirchk.h"

/*
 * close a directory.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   undef closedir
#   pragma _HP_SECONDARY_DEF _closedir closedir
#   define closedir _closedir
#endif

int
closedir(dirp)
register DIR *dirp;
{
    extern int errno;

    DIRCHK(dirp);

    /*
     * If this dirctory wasn't open, it wasn't a valid open directory
     * after all, so we return immediately (if we were to free dirp
     * we might corrupt malloc() space.
     */
    if (close(dirp->dd_fd) < 0)
	return -1;

    /*
     * dirp points to ALL of the dynamically allocated space, which
     * includes the dirp->dd_buf and the "struct dirent" -- which
     * no field points to, but starts at:
     *   (dirp->dd_buf - ((sizeof(struct dirent) + 3) & (~0x04)))
     *
     * So, we just need to free dirp to free all of this space.
     * See the code for readdir() for the reasoning behind all of
     * this.
     *
     * We also set the dd_fd, dd_loc and dd_buf fields to bogus values
     * so that closedir() can detect a bogus dirp in case someone
     * passes us a dirp that has been closed.
     */
    dirp->dd_fd = -1;
    dirp->dd_loc = 0;
    dirp->dd_buf = (char *)0;
    free(dirp);

    return 0;
}
