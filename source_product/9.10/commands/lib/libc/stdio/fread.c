/* @(#) $Revision: 64.9 $ */   
/*LINTLIBRARY*/
/*
 * This version reads directly from the buffer rather than looping on getc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */

#ifdef _NAMESPACE_CLEAN
#define _filbuf __filbuf
#define fread _fread
#define memcpy _memcpy
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

#define MIN(x, y)	(x < y ? x : y)

extern int _filbuf();
extern _bufsync();
extern void *memcpy();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fread
#pragma _HP_SECONDARY_DEF _fread fread
#define fread _fread
#endif

size_t
fread(ptr, size, count, iop)
void *ptr;
size_t size, count;
register FILE *iop;
{
	register unsigned int nleft;
	register int n;

	if (size <= 0 || count <= 0) return 0;
	nleft = count * size;

	/* Put characters in the buffer */
	/* note that the meaning of n when just starting this loop is
	   irrelevant.  It is defined in the loop */
	for ( ; ; ) {
		if (iop->_cnt <= 0) { /* empty buffer */
			if (_filbuf(iop) == EOF)
				return (count - (nleft + size - 1)/size);
			iop->_ptr--;
			iop->_cnt++;
		}
		n = MIN(nleft, iop->_cnt);
		ptr = (char *)memcpy(ptr, (char *) iop->_ptr, n) + n;
		iop->_cnt -= n;
		iop->_ptr += n;
		_BUFSYNC(iop);
		if ((nleft -= n) <= 0)
			return (count);
	}
}
