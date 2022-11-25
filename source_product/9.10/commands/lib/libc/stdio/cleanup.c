/* @(#) $Revision: 66.3 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fclose _fclose
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

extern int scan_iops();

static void
do_cleanup(iop)
FILE *iop;
{
    /*
     * Only close buffered files (pretty wierd, but that is the way
     * people expect it to work).  I actually know of nobody who
     * calls _cleanup() anyway -- rsh 12/6/89.
     * Note:  This function is only called on files open _IOWRT or
     *        _IORW, scan_iops() filters out the rest [see _cleanup()
     *        below].
     */
    if ((iop->_flag & _IONBF) == 0)
	fclose(iop);
}

/*
 * Flush buffers on exit.  Nobody really calls this anymore.
 * Only fflush() streams open for writing!
 * Do not fflush any streams that are open for reading only.
 * It can cause serious problems in applications (like sh and ksh).
 * Note:  This routine has been split out of flsbuf.c. -- raf 1/5/90
 */
void
_cleanup()
{
    (void)scan_iops(do_cleanup, _IOWRT | _IORW, ANY_BITS_IN_MASK);
}
