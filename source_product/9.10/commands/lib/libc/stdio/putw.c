/* @(#) $Revision: 64.3 $ */    
/*LINTLIBRARY*/
/*
 * The intent here is to provide a means to make the order of
 * bytes in an io-stream correspond to the order of the bytes
 * in the memory while doing the io a `word' at a time.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define putw _putw
#ifdef __lint
#   define putc   _putc
#endif  /* __lint */
#endif

#include <stdio.h>

#ifdef _NAMESPACE_CLEAN
#undef putw
#pragma _HP_SECONDARY_DEF _putw putw
#define putw _putw
#endif
int
putw(w, stream)
int w;
register FILE *stream;
{
	register char *s = (char *)&w;
	register int i = sizeof(int);

	while (--i >= 0) {
		(void) putc(*s++, stream);
		if (stream->_flag & _IOERR)
			return(EOF);
	}
	return (0);
}
