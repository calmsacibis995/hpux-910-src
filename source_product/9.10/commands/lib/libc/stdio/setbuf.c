/* @(#) $Revision: 66.3 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define isatty _isatty
#define setbuf _setbuf
#define _iob   __iob
#ifdef __lint
# define fileno _fileno
#endif /* __lint */
#       ifdef   _ANSIC_CLEAN
#define free _free
#       endif  /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

extern void free();
extern int isatty();
extern unsigned char __smbuf[][_SBFSIZ];

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setbuf
#pragma _HP_SECONDARY_DEF _setbuf setbuf
#define setbuf _setbuf
#endif

void
setbuf(iop, buf)
register FILE *iop;
char	*buf;
{
    register int fno = fileno(iop); /* file number */

    if (iop->_base != NULL && iop->_flag & _IOMYBUF)
	free((char *)iop->_base);
    iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);

    if ((iop->_base = (unsigned char *)buf) == NULL)
    {
	/*
	 * file unbuffered
	 * use small block reserved for unbuffered files
	 */
	if (iop->_flag & _IOEXT)
	    iop->_base = ((_FILEX *)iop)->__smbuf;
	else
	    iop->_base = __smbuf[iop - _iob];

	_bufend(iop) = iop->_base + _SBFSIZ;
	iop->_flag |= _IONBF;
    }
    else
    {
	extern int errno;

        /*
	 * regular buffered I/O, standard buffer size
	 */
	int save_errno = errno;	/* save becauase of isatty() */

	_bufend(iop) = iop->_base + BUFSIZ;
	if (isatty(fno))
	    iop->_flag |= _IOLBF;
	else
	    errno = save_errno;
    }
    iop->_ptr = iop->_base;
    iop->_cnt = 0;
}
