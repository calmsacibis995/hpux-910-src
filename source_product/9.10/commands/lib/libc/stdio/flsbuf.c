/* @(#)$Revision: 66.7 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define _iob __iob
#define _flsbuf __flsbuf
#define _findbuf __findbuf
#define write _write
#define close _close
#define isatty _isatty
#define fclose _fclose
#  ifdef __lint
#  define fileno _fileno
#  define ferror _ferror
#  define putc _putc
#  endif /* __lint */
#       ifdef   _ANSIC_CLEAN
#define free _free
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
#include <errno.h>

extern void free();
extern void *malloc();

extern int write(), close(), isatty();
extern FILE *_lastbuf;
extern int _xflsbuf();
extern int __fflush();
extern unsigned char __smbuf[][_SBFSIZ];

/*
 * fclose() --
 *    fclose() will flush (output) buffers for a buffered open
 *    FILE and then issue a system close on the fileno.  The
 *    _base field will be reset to NULL for any but stdin and
 *    stdout, the _ptr field will be set the same as the _base
 *    field. The _flags and the _cnt field will be zeroed.
 *    If buffers had been obtained via malloc(), the space will
 *    be free()'d.  In case the FILE was not open, or __fflush()
 *    or close() failed, an EOF will be returned, otherwise the
 *    return value is 0.
 */

#ifdef _NAMESPACE_CLEAN
#undef fclose
#pragma _HP_SECONDARY_DEF _fclose fclose
#define fclose _fclose
#endif /* _NAMESPACE_CLEAN */

int
fclose(iop)
register FILE *iop;
{
    extern int stream_count;
    register int rtn = EOF;
    register int err;

    if (iop == NULL)
	return EOF;

    if (iop->_flag & (_IOREAD | _IOWRT | _IORW))
    {
	rtn = (iop->_flag & _IONBF) ? 0 : __fflush(iop);
	if (close(fileno(iop)) < 0)
	    rtn = EOF;
    }

    /*
     * Free the buffer if we allocated it and mark this iop as
     * unused.  Adjust stream_count, since there is now a new
     * free iop element.
     */
    if (iop->_flag & _IOMYBUF)
    {
	free((char *)iop->_base);
	iop->_base = NULL;
    }
    iop->_flag &= _IOEXT;	/* clear all but the _IOEXT bit */
    iop->_cnt = 0;
    iop->_ptr = iop->_base;
    stream_count--;
    return rtn;
}

/*
 * _flsbuf() --
 *    The routine _flsbuf may or may not actually flush the output
 *    buffer. If the file is line-buffered, the fact that iop->_cnt
 *    has run below zero is meaningless: it is always kept below zero
 *    so that invocations of putc will consistently give control to
 *    _flsbuf, even if the buffer is far from full.
 *    _flsbuf, on seeing the "line-buffered" flag, determines whether
 *    the buffer is actually full by comparing iop->_ptr to the
 *    end-of-buffer pointer _bufend(iop).  If it is full, or if an
 *    output line is completed (with a newline), the buffer is flushed.
 *
 * Note:
 *    the character argument to _flsbuf is not flushed with the current
 *    buffer if the buffer is actually full-- it goes into the buffer
 *    after flushing.
 */

#ifdef _NAMESPACE_CLEAN
#undef _flsbuf
#pragma _HP_SECONDARY_DEF __flsbuf _flsbuf
#define _flsbuf __flsbuf
#endif /* _NAMESPACE_CLEAN */

int
_flsbuf(c, iop)
unsigned char c;
register FILE *iop;
{
    unsigned char c1;
    unsigned char *bufendp = _bufend(iop);

    do
    {
	/* 
	 * check for linebuffered with write perm, but no EOF
	 */
	if ((iop->_flag & (_IOLBF|_IOWRT|_IOEOF)) == (_IOLBF|_IOWRT))
	{
	    if (iop->_ptr >= bufendp)	/* if buffer full, */
		break;		/* exit do-while, and flush buf. */
	    if ((*iop->_ptr++ = c) != '\n')
		return c;
	    return _xflsbuf(iop) == EOF ? EOF : c;
	}

	/* 
	 * write out an unbuffered file, if have write perm, but no EOF
	 */
	if ((iop->_flag & (_IONBF|_IOWRT|_IOEOF)) == (_IONBF|_IOWRT))
	{
	    c1 = c;
	    iop->_cnt = 0;
	    if (write(fileno(iop), (char *)&c1, 1) == 1)
		return c;
	iop->_flag |= _IOERR;
	    return EOF;
	}

	/* 
	 * The _wrtchk call is here rather than at the top of _flsbuf
	 * to reduce overhead for line-buffered I/O under normal
	 * circumstances.
	 */
	if (_WRTCHK(iop))	/* is writing legitimate? */
	    return EOF;
    } while (iop->_flag & (_IONBF|_IOLBF));

    (void)_xflsbuf(iop);	/* full buffer:  flush buffer */
    (void)putc((char)c, iop);	/* then put "c" in newly emptied buf */
    /* (which, because of signals, may NOT be empty) */
    return ferror(iop) ? EOF : c;
}

/*
 * _wrtchk()
 *    The function _wrtchk checks to see whether it is legitimate to
 *    write to the specified device.  If it is, _wrtchk sets flags in
 *    iop->_flag for writing, assures presence of a buffer, and returns
 *    0.  If writing is not legitimate, EOF is returned.
 */
int
_wrtchk(iop)
register FILE *iop;
{
    if ((iop->_flag & (_IOWRT|_IOEOF)) != _IOWRT)
    {
	if (!(iop->_flag & (_IOWRT|_IORW)))
	{
	    errno = EBADF;
	    return EOF;		/* bogus call--read-only file */
	}
	iop->_flag = (iop->_flag & ~_IOEOF) | _IOWRT; /* fix flags */
    }

    /*
     * If this is the first I/O to this file, get a buffer
     */
    if (iop->_base == NULL)
	_findbuf(iop);

    /*
     * If this is the first write since a sync, we must reset the
     * value of cnt.
     */
    if (iop->_ptr == iop->_base && !(iop->_flag & (_IONBF|_IOLBF)))
    {
	iop->_cnt = _bufsiz(iop);
	_BUFSYNC(iop);
    }
    return 0;
}

/*
 * _findbuf() --
 *    _findbuf, called only when iop->_base == NULL, locates a
 *    predefined buffer or allocates a buffer using malloc.  If a
 *    buffer is obtained from malloc, the _IOMYBUF flag is set in
 *    iop->_flag.
 *
 *    If an attempt to get a buffer with malloc() fails, we use a
 *    small buffer which we always have on hand (it is preallocated).
 */

#ifdef _NAMESPACE_CLEAN
#undef _findbuf
#pragma _HP_SECONDARY_DEF __findbuf _findbuf
#define _findbuf __findbuf
#endif /* _NAMESPACE_CLEAN */

_findbuf(iop)
register FILE *iop;
{
    /* 
     * allocate a small block for unbuffered, large for buffered.
     * Small buffers come from the pre-allocated __smbuf[] array or,
     * in the case of _FILEX structs, from the iop structure itself.
     */
    if (iop->_flag & _IONBF)
    {
	if (iop->_flag & _IOEXT)
	    iop->_base = ((_FILEX *)iop)->__smbuf;
	else
	    iop->_base = __smbuf[iop - _iob];

	_bufend(iop) = iop->_base + _SBFSIZ;
    }
    else
    {
	int fno = fileno(iop);

	/* 
	 * use existing bufs for files 0 and 1 (normally stdin, stdout)
	 */
	if (fno < 2)
	{
	    extern unsigned char _sibuf[];
	    extern unsigned char _sobuf[];

	    iop->_base = fno == 0 ? _sibuf : _sobuf;
	    _bufend(iop) = iop->_base + _DBUFSIZ;
	}
	else
	{
	    iop->_base = (unsigned char *)malloc(_DBUFSIZ + 8);
	    if (iop->_base != NULL)
	    {
		/* we got a buffer */
		iop->_flag |= _IOMYBUF;
		_bufend(iop) = iop->_base + _DBUFSIZ;
	    }
	    else
	    {
		/*
		 * No room for buffer, use small buffer
		 */
		if (iop->_flag & _IOEXT)
		    iop->_base = ((_FILEX *)iop)->__smbuf;
		else
		    iop->_base = __smbuf[iop - _iob];

		_bufend(iop) = iop->_base + _SBFSIZ;
	    }
	}
    }

    iop->_ptr = iop->_base;
    if (!(iop->_flag & _IONBF))
    {
	int save_errno = errno;	/* save because of isatty() */

	if (isatty(fileno(iop)))
	    iop->_flag |= _IOLBF;
	else
	    errno = save_errno;
    }
}
