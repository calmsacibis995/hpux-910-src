/* @(#) $Revision: 66.1 $ */
/*LINTLIBRARY*/
/*
 * Return file offset.
 * Coordinates with buffering.
 */

#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#define fileno _fileno
#endif /* __lint */
#define lseek _lseek
#define ftell _ftell
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
#include <sys/types.h>          /* for off_t */
#include <errno.h>

extern off_t lseek();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ftell
#pragma _HP_SECONDARY_DEF _ftell ftell
#define ftell _ftell
#endif

long
ftell(iop)
FILE	*iop;
{
	register long tres;
	register int adjust;

	if (iop == NULL)  {
		errno = EBADF;
		return -1;		/* leave if NULL file pointer */
	}

	if (iop->_cnt < 0)
		iop->_cnt = 0;
	if (iop->_flag & _IOREAD)
		adjust = - iop->_cnt;
	else if (iop->_flag & (_IOWRT | _IORW)) {
		adjust = 0;
		if ((iop->_flag & _IOWRT) && iop->_base &&
					(iop->_flag & _IONBF) == 0)
			adjust = iop->_ptr - iop->_base;
	} else {
		errno = EBADF;
		return -1;
	}
	if ((tres = (long)lseek(fileno(iop), (off_t)0, SEEK_CUR)) == -1)
		iop->_flag |= _IOERR;
	else
		tres += (long)adjust;
	return tres;
}
