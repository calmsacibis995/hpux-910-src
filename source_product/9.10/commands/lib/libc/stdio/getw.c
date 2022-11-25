/* @(#) $Revision: 66.2 $ */
/*LINTLIBRARY*/
/*
 * The intent here is to provide a means to make the order of
 * bytes in an io-stream correspond to the order of the bytes
 * in the memory while doing the io a `word' at a time.
 */

#ifdef _NAMESPACE_CLEAN
#define getw _getw
#define memcpy _memcpy
#ifdef __lint
#   define getc _getc
#endif __lint
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getw
#pragma _HP_SECONDARY_DEF _getw getw
#define getw _getw
#endif

int
getw(stream)
register FILE *stream;
{
    int w;

    /*
     * If there is a whole word in the buffer, just copy it directly
     * into w and return;
     * We must use memcpy since ptr is not necessarily word aligned.
     */
    if (stream->_cnt >= sizeof (int))
    {
	memcpy((char *)&w, stream->_ptr, sizeof (int));
	stream->_cnt -= sizeof (int);
	stream->_ptr += sizeof (int);

	return (stream->_flag & (_IOEOF|_IOERR)) ? EOF : w;
    }

    /*
     * Wasn't a whole word buffered, just read in a byte at a time.
     */
    {
	register char *s = (char *)&w;
	register int i = sizeof (int);

	while (--i >= 0)
	{
	    *s++ = getc(stream);
	    if (stream->_flag & (_IOEOF|_IOERR))
		return EOF;
	}
    }
    return w;
}
