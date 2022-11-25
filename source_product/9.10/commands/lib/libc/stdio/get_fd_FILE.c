/* @(#) $Revision: 66.1 $ */
/*LINTLIBRARY*/

/*
 * get_fd_FILE.c --
 *    This file contains __get_fd_FILE() which walks the allocated stream
 *    pointers looking for the first match to a file descriptor value.
 *
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
 * __get_fd_FILE() --
 *
 *    Loop through allocated FILE and _FILEX structs, looking for a match
 *    to the fd argument.  If a match is found, then return the FILE
 *    pointer, otherwise, return NULL.
 */
FILE *
__get_fd_FILE(fd)
int fd;
{
    register int i;
    register FILEX_CHUNK *chunk;

    /*
     * First scan in the pre-allocated chunk (order isn't actually
     * important here).
     */
    for (i = 0; i < IOB_CHUNKSZ; i++)
	if (fileno(&(_iob[i])) == fd)
	    return &(_iob[i]);

    /*
     * Now scan the dynamically allocated chunks.
     */
    for (chunk = iob_chunks; chunk; chunk = chunk->next)
    {
	register _FILEX *iobx = chunk->iobx;

	for (i = 0; i < IOBX_CHUNKSZ; i++)
	    if (fileno((FILE *) &(iobx[i])) == fd)
		return (FILE *) &(iobx[i]);
    }
    return (FILE *)NULL;
}
