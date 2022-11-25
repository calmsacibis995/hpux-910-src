/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */

#ifdef _NAMESPACE_CLEAN
#define fputs _fputs
#define memccpy _memccpy
#define strlen _strlen
#define write _write
#ifdef __lint
#  define fileno _fileno
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"
extern char *memccpy();

#ifdef _NAMESPACE_CLEAN
#undef fputs
#pragma _HP_SECONDARY_DEF _fputs fputs
#define fputs _fputs
#endif /* _NAMESPACE_CLEAN */

int
fputs(ptr, iop)
char *ptr;
register FILE *iop;
{
	register int ndone = 0, n, size;
	register unsigned char *cptr, *bufend;
	char *p;

	if (_WRTCHK(iop))
		return (EOF);
	bufend = _bufend(iop);

	if ((iop->_flag & _IONBF) == 0)  {
		for ( ; ; ptr += n) {
			while ((n = bufend - (cptr = iop->_ptr)) <= 0)  
				/* full buf */
				if (_xflsbuf(iop) == EOF)
					return(EOF);
			if ((p = memccpy((char *) cptr, ptr, '\0', n)) != NULL)
				n = (p - (char *) cptr) - 1;
			iop->_cnt -= n;
			iop->_ptr += n;
			_BUFSYNC(iop);
			ndone += n;
			if (p != NULL)  { 
				/* done; flush buffer if line-buffered */
	       			if (iop->_flag & _IOLBF)
	       				if (_xflsbuf(iop) == EOF)
	       					return(EOF);
	       			return(ndone);
	       		}
		}
	}  else  {
		/* write out to an unbuffered file */
		register unsigned int cnt = strlen(ptr);
		register int fno = fileno(iop);

		while (cnt > 0) {
			size = write(fno, ptr, cnt);
			if (size <= 0) {
		    		iop->_flag |= _IOERR;
		    		return(EOF);
			}
			ptr += size;
			cnt -= size;
		}
		return(cnt);
	}
}
