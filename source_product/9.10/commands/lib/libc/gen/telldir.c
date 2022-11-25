/* @(#) $Revision: 72.1 $ */      

#ifdef _NAMESPACE_CLEAN
#define telldir  _telldir
#define lseek    _lseek
#endif

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <dirent.h>
#include "dirchk.h"
#ifdef _THREAD_SAFE
#include "rec_mutex.h"
#endif

/*
 * return a pointer into a directory
 */
#ifdef _NAMESPACE_CLEAN
#undef telldir
#pragma _HP_SECONDARY_DEF _telldir telldir
#define telldir _telldir
#endif
long
telldir(dirp)
DIR *dirp;
{
    extern long lseek();
    long int mfactor;
#ifdef _THREAD_SAFE
    long retval;
    REC_MUTEX *lock;

    lock = dirp->dd_lock;
#endif

    DIRCHK(dirp);

#ifdef _THREAD_SAFE
    _rec_mutex_lock(lock);

    /* Need to check again in case another thread called closedir */
    if (!GOOD_DIR(dirp)) {
	errno = EBADF;
	_rec_mutex_unlock(lock);
	return-1;
    }
#endif
#ifdef HP_NFS
	/*
 	 * Different multiplication factors are used during
	 * the encoding depending upon the file system block sizes.
	 * The multiplication factors were arrived in the 
	 * following manner.
	 *
	 * DIRSIZ(dp) macro (see ndir.h) finds out the space required
	 * for a directory entry given the name length. The computations
	 * were done assuming the worst case (i.e. name length is 1 byte).
	 * With this assumption we can compute the maximum number of
	 * directory entries per file system block. For example,
	 * an 8192 byte block can accommodate a maximum of 682 directory
	 * entries (since using the DIRSIZ macro the minimum space 
	 * required for a directory entry is 12 bytes - if the name length
	 * is 1 byte). Hence, 10 bits are sufficient to represent the
	 * the maximum number of entries in a 8192 byte block. The
	 * multiplication factor for a 8192 byte block is 1024 (2**10).
	 * Other multiplication factors were also obtained in the
	 * same fashion based on the block size.
	 * Note:
	 *  seekdir will use these multiplication factor for decoding.
	 * Fix for the dts UCSqm00596.
	 *
	 * Modified default case selector to handle non standard block
	 * sizes (e.g. in non-HP file systems.)  Fixes DSDe410357.
	 */
    switch (dirp->dd_bsize)
    {
	case 4096:
		mfactor=512;     /* 2**9  */
		break;
	case 8192:
		mfactor=1024;    /* 2**10 */
		break;
	case 16384:
		mfactor=2048;    /* 2**11 */
		break;
	case 32768:
		mfactor=4096;    /* 2**12 */
		break;
	case 65536:
		mfactor=8192;    /* 2**13 */
		break;
	default:
		mfactor=dirp->dd_bsize;		/* Non standard block size */
		break;
    }
# ifdef _THREAD_SAFE
    retval = (dirp->dd_bbase * mfactor) + dirp->dd_entno;
    _rec_mutex_unlock(lock);
    return retval;
# else
    return (dirp->dd_bbase * mfactor) + dirp->dd_entno;
# endif
#else
    return lseek(dirp->dd_fd, 0L, 1) - dirp->dd_size + dirp->dd_loc;
#endif
}
