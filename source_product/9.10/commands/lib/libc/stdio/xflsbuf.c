/* @(#)$Revision: 66.3 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#  define write 	_write
#  ifdef __lint
#  define fileno 	_fileno
#  endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

/*
 * The function _xflsbuf writes out the current contents of the output
 * buffer delimited by iop->_base and iop->_ptr.
 * iop->_cnt is reset appropriately, but its value on entry to _xflsbuf
 * is ignored.
 *
 * The following code is not strictly correct.  If a signal is raised,
 * invoking a signal-handler which generates output into the same buffer
 * being flushed, a peculiar output sequence may result (for example,
 * the output generated by the signal-handler may appear twice).  At
 * present no means has been found to guarantee correct behavior without
 * resorting to the disabling of signals, a means considered too
 * expensive.
 *
 * For now the code has been written with the intent of reducing the
 * probability of strange effects and, when they do occur, of confining
 * the damage.  Except under extremely pathological circumstances, this
 * code should be expected to respect buffer boundaries even in the face
 * of interrupts and other signals.
 */

int
_xflsbuf(iop)
register FILE *iop;
{
    register unsigned char *base;
    register int n;
    register int size;
    int fno = fileno(iop);

    n = iop->_ptr - (base = iop->_base);
    iop->_ptr = base;
    iop->_cnt = (iop->_flag & (_IONBF|_IOLBF)) ? 0 : _bufsiz(iop);
    _BUFSYNC(iop);
    size = 0;

    while (n > 0)
    {
	size = write(fno, (char *)(base), (unsigned)n);
	if (size <= 0)
	{
	    iop->_flag |= _IOERR;
	    return EOF;
	}
	base += size;
	n -= size;
    }
    return 0;
}