/* @(#) $Revision: 66.10 $ */
/*LINTLIBRARY*/

/*
 * findiop.c --
 *    This file contains the function _findiop, which is used to
 *    allocate a FILE or _FILEX struct.
 */

#ifdef _NAMESPACE_CLEAN
#define _iob     __iob
#  ifdef _ANSIC_CLEAN
#    define malloc   _malloc
#  endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

/*
 * iob_chunks is a pointer to a linked list of our dynamically
 * allocated iob chunks.  Each chunk contains a pointer to the next
 * chunk of IOBX_CHUNKSZ _FILEX structs.
 */
extern FILEX_CHUNK *iob_chunks;

/*
 * The following two variables keep track of how many FILE or _FILEX
 * structs that have been allocated and the number of currently
 * open streams.  These are used to avoid searching the chunks
 * for a new stream descriptor when there are none available.
 *
 * Each time we allocate a new chunk, iob_allocated is adjusted
 * by the number of new free streams.
 * Each time a stream is successfully fopen()ed or fclose()ed,
 * fopen and fclose adjust stream_count.
 */
extern int iob_allocated;  	 /* No. of static ones */
extern int stream_count;	 /* stdin, stdout, stderr */

/*
 * _findiop() --
 *    find and return a free FILE or _FILEX structure.
 *    The structures are distinguished by the _flag field having the
 *    _IOEXT bit set (indicating a _FILEX struct).
 */
FILE *
_findiop()
{
    register int i;

    /*
     * Search for a free element, only if we know there is one.
     */
    if (stream_count < iob_allocated)
    {
	/*
	 * First search the first three pre-allocated chunks (stdin, 
	 * stdout, stderr).  This is for compatibility with those programs 
	 * which may have done something like:
	 *     fclose(stdout);
	 *     (void) fopen("file", "w");
	 *     fprintf(stdout, ...);  * Expects to write to the new file *
	 */
	for (i = 0; i < 3; i++)
	    if ((_iob[i]._flag & (_IOREAD | _IOWRT | _IORW)) == 0)
		return &(_iob[i]);

	/*
	 * Next search the dynamically allocated chunks.  We prefer to
	 * allocate a dynamic one, since the _bufend() macro is slightly
	 * more efficient on these.  Also, the dynamically allocated
	 * chunks are in a linked list in which new items are added at
	 * the head, so the head of the list would be more likely to
	 * have free elements [assuming a LIFO use model].
	 */
	{
	    register FILEX_CHUNK *chunk;

	    for (chunk = iob_chunks; chunk; chunk = chunk->next)
	    {
		register _FILEX *iobx = chunk->iobx;

		for (i = 0; i < IOBX_CHUNKSZ; i++)
		    if ((iobx[i]._flag & (_IOREAD|_IOWRT|_IORW)) == 0)
		    {
			/* found a free one */
			iobx[i]._flag = _IOEXT;
			return (FILE *)&(iobx[i]);
		    }
	    }
	}

	/*
	 * No free ones from the dynamic pool, or in the first 3 _iob[]
	 * elements (stdin, stdout, stderr), check for other ones in the
	 * pre-allocated chunk.
	 */
	for (i = 3; i < IOB_CHUNKSZ; i++)
	    if ((_iob[i]._flag & (_IOREAD | _IOWRT | _IORW)) == 0)
		return &(_iob[i]);

    }

    /*
     * Didn't find a free one, so we have to allocate a new chunk.
     */
    {
	extern char *malloc();
	register FILEX_CHUNK *chunk;
	register _FILEX *iobx;

	chunk = (FILEX_CHUNK *)malloc(sizeof(FILEX_CHUNK));
	if (chunk == (FILEX_CHUNK *)0)
	    return (FILE *)0;

	/*
	 * Mark the new elements as "free", indicating that they
	 * are _FILEX structs too (not FILE structs).
	 */
	for (i = 0, iobx = chunk->iobx; i < IOBX_CHUNKSZ; i++)
	    iobx[i]._flag = _IOEXT;

	/*
	 * Insert the new chunk in the head of our list of chunks,
	 * adjust iob_allocated and return a free element from the
	 * new chunk.
	 */
	chunk->next = iob_chunks;
	iob_chunks = chunk;
	iob_allocated += IOBX_CHUNKSZ;
	return (FILE *)&(chunk->iobx[0]);
    }
}
