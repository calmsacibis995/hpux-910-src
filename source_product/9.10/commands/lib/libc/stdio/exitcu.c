/* @(#) $Revision: 66.4 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#ifdef _ANSIC_CLEAN
#define free _free
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

extern int scan_iops();
extern int __fflush();

static void
do_exitcu(iop)
FILE *iop;
{
    extern int stream_count;

    /*
     * Only flush buffered files, since flushing a non-buffered
     * file is like beating a dead horse.
     * Note:  This function is only called on files open _IOWRT or
     *        _IORW, scan_iops() filters out the rest [see below].
     */
    if ((iop->_flag & _IONBF) == 0)
	(void)__fflush(iop);

    /*
     * Note: we don't free the memory, even if it is one of our
     *       buffers.  There is no need, since we are just going to
     *       exit anyway.  In fact, we don't reset any of the other
     *	     fields of the struct, since if flags is 0 or _IOEXT
     *       the struct is considered to be unused anyway.
     */
    iop->_flag &= _IOEXT;	/* mark free, preserve the _IOEXT bit */
    stream_count--;		/* another free iop element */
}

/*
 * This routine, _exitcu, is used exclusively by the exit() call.
 * It is identical to the _cleanup routine but it will not
 * cause the file descriptors to be closed.  It will only do
 * the flush as specified on the man page for exit(2).
 *
 * Note that any dynamically allocated buffers (_IOMYBUF) are *NOT*
 * free()-ed.  Since we are only called just before process exit,
 * it doesn't make any sense to spend time freeing memory that will
 * just go away anyway when we exit.
 */
void
_exitcu()
{
    (void)scan_iops(do_exitcu, _IOWRT | _IORW, ANY_BITS_IN_MASK);
}
