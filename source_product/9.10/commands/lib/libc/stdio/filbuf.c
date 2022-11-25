/* @(#) $Revision: 66.4 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define read _read
#ifdef __lint
#define fileno _fileno
#endif /* __lint */
#define _filbuf __filbuf
#define _findbuf __findbuf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

extern _findbuf();
extern int read();
extern int __fflush();
extern FILE *_lastbuf;
extern int scan_iops();

#ifdef _NAMESPACE_CLEAN
#undef _filbuf
#pragma _HP_SECONDARY_DEF	__filbuf	_filbuf
#define _filbuf __filbuf
#endif /* _NAMESPACE_CLEAN */

int
_filbuf(iop)
register FILE *iop;
{
	register FILE *diop;

	/*
	 * special check for sscanf case.  We can't really guarantee
	 * that the user hasn't directly modified the _flag field (to turn
	 * on _IODUMMY), but this is the best that we can do.
	 */
	if ((iop->_flag & (_IOREAD|_IODUMMY)) == (_IOREAD|_IODUMMY))
		return(EOF);

	if (iop->_base == NULL)  /* get buffer if we don't have one */
		_findbuf(iop);

	if ( !(iop->_flag & _IOREAD) )
		if (iop->_flag & _IORW)
			iop->_flag |= _IOREAD;
		else {
			iop->_flag |= _IOERR;
			errno = EBADF;
			return(EOF);
		}

	/*
	 * if this device is a terminal (line-buffered) or unbuffered,
	 * then flush buffers of all line-buffered devices currently
	 * writing
	 *
	 * According to the description of _IOLBF in the setbuf(3S) man 
	 * page, _IOLBF "causes output to be line buffered; the buffer will
	 * be flushed when a newline is written, the buffer is full, *OR
	 * INPUT IS REQUESTED*".  Since we are requesting input on a line
	 * buffered stream and we don't know which output stream (supposidly
	 * associated with a tty) is associated with this input stream, we 
	 * go ahead and flush all streams that fit this description.
	 *
	 */
	if (iop->_flag & (_IOLBF | _IONBF))
	    (void)scan_iops(__fflush, _IOLBF | _IOWRT, ALL_BITS_IN_MASK);

	iop->_ptr = iop->_base;
	iop->_cnt = read(fileno(iop), (char *)iop->_base,
	    (unsigned)((iop->_flag & _IONBF) ? 1 : _bufsiz(iop) ));
	if (--iop->_cnt >= 0)		/* success */
		return (*iop->_ptr++);
	if (iop->_cnt != -1)		/* error */
		iop->_flag |= _IOERR;
	else {				/* end-of-file */
		iop->_flag |= _IOEOF;
		if (iop->_flag & _IORW)
			iop->_flag &= ~_IOREAD;
	}
	iop->_cnt = 0;
	return (EOF);
}
