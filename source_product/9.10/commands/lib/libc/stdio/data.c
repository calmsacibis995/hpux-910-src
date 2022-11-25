/* @(#) $Revision: 66.6 $ */    
/*LINTLIBRARY*/
#include <stdio.h>
#include "stdiom.h"

/*
 * some slop is allowed at the end of the buffers in case an upset in
 * the synchronization of _cnt and _ptr (caused by an interrupt or other
 * signal) is not immediately detected.
 */
unsigned char _sibuf[_DBUFSIZ+8];   /* pre-allocated stdin buffer */
unsigned char _sobuf[_DBUFSIZ+8];   /* pre-allocated stdout buffer */

/*
 * Ptrs to start of preallocated buffers for stdin, stdout.
 */
unsigned char *_stdbuf[] = { _sibuf, _sobuf };

/*
 * __smbuf has pre-allocated buffers for multi-character output to
 * unbuffered files.
 */
unsigned char __smbuf[IOB_CHUNKSZ][_SBFSIZ];

#ifdef _NAMESPACE_CLEAN
#undef _iob
#pragma _HP_SECONDARY_DEF __iob _iob
#define _iob __iob
#endif

FILE _iob[IOB_CHUNKSZ] =
{
    { 0, NULL, NULL, _IOREAD, 0},                   /* stdin  */
    { 0, NULL, NULL, _IOWRT, 1},                    /* stdout */
    { 0, __smbuf[2], __smbuf[2], _IOWRT|_IONBF, 2}, /* stderr (unbuffered) */
};

/*
 * Ptrs to end of read/write buffers for each open file.  These
 * pointers are only used for FILEs.  _FILEXs have the buffer end
 * pointer directly in the struct.
 */
unsigned char *__bufendtab[IOB_CHUNKSZ] =
    { NULL, NULL, __smbuf[2]+_SBFSIZ, };

/*
 * iob_chunks is a pointer to a linked list of our dynamically
 * allocated iob chunks.  Each chunk contains a pointer to the next
 * chunk of IOBX_CHUNKSZ _FILEX structs.
 */
FILEX_CHUNK *iob_chunks;

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
int iob_allocated = IOB_CHUNKSZ;         /* No. of static ones */
int stream_count = 3;                    /* stdin, stdout, stderr */
