/* @(#) $Revision: 72.1 $ */     

#ifdef _NAMESPACE_CLEAN
#define seekdir _seekdir
#define lseek _lseek
#define read _read
#ifdef _THREAD_SAFE
#define readdir_r _readdir_r
#else
#define readdir _readdir
#endif
#endif

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "dirchk.h"
#ifdef _THREAD_SAFE
#include "rec_mutex.h"
#endif

/*
 * seek to an entry in a directory.
 * Only values returned by "telldir" should be passed to seekdir.
 */

#ifdef _NAMESPACE_CLEAN
#undef seekdir
#pragma _HP_SECONDARY_DEF _seekdir seekdir
#define seekdir _seekdir
#endif

void
seekdir(dirp, loc)
register DIR *dirp;
long loc;
{
    long base, offset;
#ifdef _THREAD_SAFE
    REC_MUTEX *lock;
    struct dirent dp;
#else
    struct dirent *dp;
#endif
    extern long lseek();
    long int dfactor;

#ifdef _THREAD_SAFE
    lock = dirp->dd_lock;
#endif

    /*
     * Note -- although seekdir() doesn't return a value, it should
     *         set errno if it is passed a bogus directory.
     */
    if (!GOOD_DIR(dirp))
    {
	errno = EBADF;
	return;
     }

#ifdef _THREAD_SAFE
    _rec_mutex_lock(lock);

    /* Need to check again in case another thread called closedir */
    if (!GOOD_DIR(dirp))
    {
	errno = EBADF;
	_rec_mutex_unlock(lock);
	return;
    }
#endif

#ifdef HP_NFS

	/*
	 * see telldir.c to understand how the 'dfactor's 
	 * were arrived.
	 * Fix for the dts UCSqm00596.
	 *
	 * Modified default case selector to handle non standard block
	 * sizes (e.g. in non-HP file systems.)  Fixes DSDe410357.
         */

    switch (dirp->dd_bsize)
    {
        case 4096:
                dfactor=512;     /* 2**9  */
                break;
        case 8192:
                dfactor=1024;    /* 2**10 */
                break;
        case 16384:
                dfactor=2048;    /* 2**11 */
                break;
        case 32768:
                dfactor=4096;    /* 2**12 */
                break;
        case 65536:
                dfactor=8192;    /* 2**13 */
                break;
        default:
                dfactor=dirp->dd_bsize;		/* Non standard block size */
                break;
    }
    base = loc / dfactor;
    offset = loc % dfactor;  /* really entry number */
    if (offset < 0)
    {
	(void)lseek(dirp->dd_fd, -1, 0);
	dirp->dd_loc = 0;
# ifdef _THREAD_SAFE
	_rec_mutex_unlock(lock);
# endif
	return;
    }
#else
    base = loc & ~(DIRBLKSIZ - 1);
    offset = loc & (DIRBLKSIZ - 1);
#endif
    (void)lseek(dirp->dd_fd, base, 0);
    dirp->dd_loc = 0;
    dirp->dd_size = 0;
#ifdef HP_NFS
    dirp->dd_entno = 0;
    dirp->dd_bbase = base;

    while (dirp->dd_entno < offset)
#else
    while (dirp->dd_loc < offset)
#endif
    {
#ifdef _THREAD_SAFE
	/* NOTE:  readdir() increments dirp->dd_entno */
	if (readdir_r(dirp, &dp) == -1) break;
#else
	dp = readdir(dirp);  /* NOTE:  readdir() increments dirp->dd_entno */
	if (dp == NULL)
	    return;
#endif
    }

#ifdef _THREAD_SAFE
    _rec_mutex_unlock(lock);
#endif
}
