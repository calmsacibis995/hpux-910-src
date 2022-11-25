/* @(#) $Revision: 66.8 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fflush _fflush
#define lseek _lseek
#  ifdef __lint
#  define fileno _fileno
#  define ferror _ferror
#  endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

extern int scan_iops();
extern int _xflsbuf();
extern off_t lseek();

/*
 * fflush() --
 *    The fflush() routine must take care because of the
 *    possibility for recursion. The calling program might
 *    do IO in an interupt catching routine that is likely
 *    to interupt the write() call within fflush()
 */

#ifdef _NAMESPACE_CLEAN
#undef fflush
#pragma _HP_SECONDARY_DEF _fflush fflush
#define fflush _fflush
#endif /* _NAMESPACE_CLEAN */

int
fflush(iop)
register FILE *iop;
{
    int __fflush();

    /*
     * when iop is a null pointer, fflush will flush all currently
     * open streams.
     */
    if (iop == NULL)
	return scan_iops(__fflush, _IOREAD | _IOWRT | _IORW, ANY_BITS_IN_MASK);

    return __fflush(iop);
}

int
__fflush(iop)
register FILE *iop;
{
    /* 
     * If we are not writing to the file and the file is buffered,
     * we must adjust the kernel's idea of what the current
     * position in the file is with our idea of it.
     * We do this by seeking back in the file the number of bytes
     * that we have read ahead and setting the iop->_ptr to
     * the base of the buffer.
     */
    if (!(iop->_flag & (_IOWRT|_IONBF)))
    {
	if (iop->_cnt > 0 && !feof(iop))
	{
	    int err = errno;
	    (void) lseek((int) fileno(iop), (off_t) -(iop->_cnt), SEEK_CUR);
	    if (errno == ESPIPE)
		errno = err;

	}
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
	return 0;
    }

    while (!(iop->_flag & _IONBF) && (iop->_flag & _IOWRT) &&
	    (iop->_base != NULL) && (iop->_ptr > iop->_base))
	(void)_xflsbuf(iop);
    /* stream is currently writing and is open for reading and writing */
    if ((iop->_flag & (_IOWRT | _IORW)) == (_IOWRT | _IORW))
    {
        iop->_flag &= ~_IOWRT;
	iop->_cnt = 0; /* Invalidate the write buffer to force setting of
			  _IOREAD or _IOWRT on next i/o operation */
    }
    return ferror(iop) ? EOF : 0;
}
