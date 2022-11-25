/* @(#) $Revision: 66.6 $ */

#ifndef _DIRCHK_INCLUDED
#define _DIRCHK_INCLUDED

#ifdef _NAMESPACE_CLEAN
#  ifndef sysconf
#    define sysconf _sysconf
#  endif
#endif /* _NAMESPACE_CLEAN */

#include <unistd.h> /* For the sysconf call */
#include <errno.h>  /* For setting errno to EBADF in DIRCHK */

/*
 * WORD_PAD    -- round a number of bytes up to a even word boundry
 *                i.e. WORD_PAD(1..3) == sizeof(long)
 * GOOD_DIR    -- TRUE if its argument seems to be a valid directory
 *                pointer
 *
 * DIRCHK      -- generates a code fragment that returns the proper
 *		  error condition if dirp is invalid
 *
 */

#define WORD_PAD(x) (((x) + sizeof (long) - 1) & (~(sizeof (long) - 1)))

#define GOOD_DIR(d) \
    ((d) != (DIR *)0 && \
     (d)->dd_fd >= 0  && \
     (d)->dd_buf == ((char *)(d) + WORD_PAD(sizeof (DIR))))

#define DIRCHK(dirp) \
    if (!GOOD_DIR(dirp))  \
    {			  \
	errno = EBADF;    \
	return -1;        \
    }
#endif /* _DIRCHK_INCLUDED */
