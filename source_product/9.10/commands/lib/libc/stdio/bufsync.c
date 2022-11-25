/* @(#)$Revision: 66.1 $ */
/*LINTLIBRARY*/

#include <stdio.h>
#include "stdiom.h"

/*
 * _bufsync() --
 *    _bufsync is called because interrupts and other signals which
 *    occur in between the decrementing of iop->_cnt and the 
 *    incrementing of iop->_ptr, or in other contexts as well, may
 *    upset the synchronization of iop->_cnt and iop->ptr.  If this
 *    happens, calling _bufsync should resynchronize the two quantities
 *    (this is not always possible).
 *    Resyn-chronization guarantees that putc invocations will not
 *    write beyond the end of the buffer.  Note that signals during
 *    _bufsync can cause _bufsync to do the wrong thing, but usually
 *    with benign effects.
 */
_bufsync(iop)
register FILE *iop;
{
    register int spaceleft;
    unsigned char *bufendp = _bufend(iop);

    if ((spaceleft = bufendp - iop->_ptr) < 0)
	iop->_ptr = bufendp;
    else if (spaceleft < iop->_cnt)
	iop->_cnt = spaceleft;
}
