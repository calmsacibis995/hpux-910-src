/* @(#) $Revision: 66.4 $ */      
/*LINTLIBRARY*/
/*
 * Unix routine to do an "fopen" on file descriptor
 * The mode has to be repeated because you can't query its
 * status
 */

#ifdef _NAMESPACE_CLEAN
#define lseek  _lseek
#define fdopen _fdopen
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

extern long lseek();
extern FILE *_findiop();

#ifdef _NAMESPACE_CLEAN
#undef fdopen
#pragma _HP_SECONDARY_DEF _fdopen fdopen
#define fdopen _fdopen
#endif /* _NAMESPACE_CLEAN */

FILE *
fdopen(fd, mode)
int	fd;
const register char *mode;
{
    extern int stream_count;
    register FILE *iop;

    if (mode == NULL)
    {
	errno = EINVAL;
	return (FILE *)NULL;
    }

    if ((iop = _findiop()) == NULL)
    {
	errno = EMFILE;
	return (FILE *)NULL;
    }

    iop->_cnt = 0;
    iop->_flag &= _IOEXT; /* clear all but the _IOEXT bit */
    setfileno(iop, fd);
    _bufend(iop) = iop->_base = iop->_ptr = NULL;

    switch (*mode)
    {
    case 'r': 
	iop->_flag |= _IOREAD;
	break;
    case 'a': 
	(void)lseek(fd, 0L, SEEK_END);
	/* No break */
    case 'w': 
	iop->_flag |= _IOWRT;
	break;
    default: 
	errno = EINVAL;
	return (FILE *)NULL;
    }

    /* support for the binary mode */

    if ((mode[1] == '+') || ((mode[2] == '+') && (mode[1] == 'b')))
	iop->_flag = _IORW | (iop->_flag & _IOEXT);

    /*
     * Adjust the count of open streams, since we have just
     * successfully opened a new one.
     */
    stream_count++;
    return iop;
}
