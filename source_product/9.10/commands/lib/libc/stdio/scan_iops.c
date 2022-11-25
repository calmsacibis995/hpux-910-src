/* @(#) $Revision: 66.4 $ */
/*LINTLIBRARY*/

/*
 * scan_iops.c --
 *    This file contains the function scan_iops, which is the
 *    only place that is cognizant of how to loop through the various
 *    places where a FILE or _FILEX struct may be found.
 */

#ifdef _NAMESPACE_CLEAN
#define _iob     __iob
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
 * fopen and fclose adjust __stream_count.
 */
extern int iob_allocated;      		 /* No. of static ones */
extern int stream_count;		 /* stdin, stdout, stderr */

/*
 * scan_iops() --
 *
 *    Loop through all allocated FILE and _FILEX structs, calling
 *    the specifed function with a pointer to the struct.  A mask
 *    is passed which specifies which iop's that you are interested
 *    in (the mask is ANDed with the _flag field. If the any_or_all 
 *    flag is 0, then the function is only called with that iop if 
 *    the result of the AND is non-zero.  If the any_or_all flag is 1,
 *    then the function is only called with that iop if the result of the
 *    AND is equal to the mask ==> all bits in the mask are set.)
 *
 *    This function returns 0 if all of the invocations of the
 *    function 'fn' return 0.  Otherwise it returns the last non-zero
 *    value returned by 'fn'.  [This is so that fflush() can return
 *    the right value when a "fflush((FILE *)NULL)" is done.
 */
int
scan_iops(fn, mask, any_or_all)
int (*fn)();
unsigned short mask;
int any_or_all;
{
    register int i;
    register FILEX_CHUNK *chunk;
    register int retval = 0;
    register int tmp;

    if (any_or_all == ALL_BITS_IN_MASK)
    {
        /*
         * First scan in the pre-allocated chunk (order isn't actually
         * important here).
         */
        for (i = 0; i < IOB_CHUNKSZ; i++)
	    if ((_iob[i]._flag & mask) == mask)
	        if ((tmp = (*fn)(&(_iob[i]))) != 0)
		    retval = tmp;

        /*
         * Now scan the dynamically allocated chunks.
         */
        for (chunk = iob_chunks; chunk; chunk = chunk->next)
        {
	    register _FILEX *iobx = chunk->iobx;

	    for (i = 0; i < IOBX_CHUNKSZ; i++)
	        if ((iobx[i]._flag & mask) == mask)
		    if ((tmp = (*fn)(&(iobx[i]))) != 0)
		        retval = tmp;
        }
    }
    else
    {
        /*
         * First scan in the pre-allocated chunk (order isn't actually
         * important here).
         */
        for (i = 0; i < IOB_CHUNKSZ; i++)
	    if ((_iob[i]._flag & mask) != 0)
	        if ((tmp = (*fn)(&(_iob[i]))) != 0)
		    retval = tmp;

        /*
         * Now scan the dynamically allocated chunks.
         */
        for (chunk = iob_chunks; chunk; chunk = chunk->next)
        {
	    register _FILEX *iobx = chunk->iobx;

	    for (i = 0; i < IOBX_CHUNKSZ; i++)
	        if ((iobx[i]._flag & mask) != 0)
		    if ((tmp = (*fn)(&(iobx[i]))) != 0)
		        retval = tmp;
        }
    }
    return retval;
}
