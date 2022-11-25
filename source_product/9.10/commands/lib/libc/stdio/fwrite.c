/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 *
 * This version does buffered writes larger than _bufsiz(iop) directly, when
 * the buffer is empty.
 */

#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#define fileno _fileno
#endif /* __lint */
#define write _write
#define fwrite _fwrite
#define memchr _memchr
#define memcpy _memcpy
#define ulimit __ulimit
#define lseek  _lseek
#endif /* _NAMESPACE_CLEAN */

#include <sys/types.h>
#include <stdio.h>
#include <ulimit.h>
#include <string.h>
#include <errno.h>
#include <sys/unistd.h>
#include "stdiom.h"

#define MIN(x, y)       (x < y ? x : y)

#ifdef _NAMESPACE_CLEAN
#undef fwrite
#pragma _HP_SECONDARY_DEF _fwrite fwrite
#define fwrite _fwrite
#endif /* _NAMESPACE_CLEAN */

size_t
fwrite(ptr, size, count, iop)
void *ptr;
size_t size, count;
register FILE *iop;
{
	register unsigned long nleft;
	register int n;
	register unsigned char *cptr, *bufend;

	if (size <= 0 || count <= 0 || _WRTCHK(iop))
	        return 0;

	bufend = _bufend(iop);
	nleft = count*size;

	/* if the file is unbuffered, or if the iop->ptr = iop->base, and there
	   is > _bufsiz(iop) chars to write, we can do a direct write */
	if (iop->_base >= iop->_ptr)	/*this covers the unbuffered case, too*/
	{
		register int fno = fileno(iop);

		if (((iop->_flag & _IONBF) != 0) || (nleft >= _bufsiz(iop)))
		{
			if ((n=write(fno,ptr,nleft)) != nleft)
		        {
				long u_size, f_pos;
				int err;

				iop->_flag |= _IOERR;
				/* write failed to write all data, see
				   if the error was due to exceeding ulimit
				 */
				err = errno;
				u_size = ulimit(UL_GETFSIZE, 0) * 512;
				f_pos = lseek(fno, (off_t)0, SEEK_CUR);
				if (f_pos >= u_size)
					errno = EFBIG;
				else
					errno = err;
				n = (n >= 0) ? n : 0;
			}
			return n/size;
		}
	}
	/*
	 * Put characters in the buffer
	 * note that the meaning of n when just starting this loop is
	 * irrelevant.  It is defined in the loop
	 */
	for (; ; ptr = (char *)ptr + n)
	{
	        while ((n = bufend - (cptr = iop->_ptr)) <= 0)  /* full buf */
	                if (_xflsbuf(iop) == EOF)
	                        return (count - (nleft + size - 1)/size);
	        n = MIN(nleft, n);
	        (void) memcpy((char *) cptr, ptr, n);
	        iop->_cnt -= n;
	        iop->_ptr += n;
	        _BUFSYNC(iop);
		/* done; flush if unbuffered or linebuffered with a newline */
	        if ((nleft -= n) == 0)
		{ 
			if ( (iop->_flag & _IONBF) || ((iop->_flag & _IOLBF) &&
 			   (memchr(cptr, '\n',n) != NULL)) )
		        {
				     	(void) _xflsbuf(iop);
		        }
	                return count;
	        }
	}
}
