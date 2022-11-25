/* @(#) $Revision: 66.5 $ */      

/*
 * This is the "old" fileno() macro which used to be in <stdio.h>.  
 * We had to nix the macro from supported code because this version has
 * a problem with evaluating its arguments twice.  This means that it 
 * should not be used on functions or operations with side effects.
 *    EX:
 *
 *    fileno(fopen("file", "w"))
 *	       or
 *    fileno(stream++)
 *             .
 *	       .
 *             .
 */
#ifndef __lint
#undef fileno
#define fileno(p) (((p)->__fileH << 8) | (p)->__fileL)
#endif /* __lint */

/*
 * This macro is used to set the file number of a file to "n".
 * We have to swap the bytes so that people using the old
 * _file get the right value (i.e. the ".o" compatability issue).
 */
#define setfileno(iop, n)				\
    {							\
	unsigned short x = (n); 			\
	(iop)->__fileH = ((unsigned char *)(&x))[0];	\
	(iop)->__fileL = ((unsigned char *)(&x))[1];	\
    }

/*
 * The following macros improve performance of the stdio by reducing
 * the number of calls to _bufsync and _wrtchk.  _BUFSYNC has the same
 * effect as _bufsync, and _WRTCHK has the same effect as _wrtchk,
 * but often these functions have no effect, and in those cases the
 * macros avoid the expense of calling the functions.
 */
#define _BUFSYNC(iop)					\
    if (_bufend(iop) - (iop)->_ptr <			\
		((iop)->_cnt < 0 ? 0 : (iop)->_cnt ))	\
	_bufsync(iop)

#define _WRTCHK(iop)						\
    ( ((((iop)->_flag & (_IOWRT | _IOEOF)) != _IOWRT) ||	\
	 ((iop)->_base == NULL) || 				\
	 ((iop)->_ptr == (iop)->_base &&			\
	  (iop)->_cnt == 0 &&					\
	  !((iop)->_flag & (_IONBF | _IOLBF)))) ? _wrtchk(iop) : 0)

/*
 * Space for FILE structs is in a pre-allocated chunk with
 * IOB_CHUNKSZ elements.
 * Space for _FILEX structs are dynamically allocated in chunks of
 * IOBX_CHUNKSZ.
 * These two values don't have to be the same (although they are
 * at this time).
 */
#define IOB_CHUNKSZ	8
#define IOBX_CHUNKSZ	8

/*
 * Dynamically allocated _FILEX structures.  We allocate these in
 * chunks of IOBX_CHUNKSZ, with a pointer at the beginning to the
 * next chunk (if any).
 */
typedef struct filex_chunk {
    struct filex_chunk *next;
    _FILEX	iobx[IOBX_CHUNKSZ];
} FILEX_CHUNK;

#define _IOBUFDIRTY	0004000 /* used by fseek(), ungetc() */

/*
 * One of the following macros should be passed as the third argument to
 * the scan_iops() routine.  This argument tells scan_iops() whether it
 * should call the user specified function when any bit in the mask is
 * turned 'on' or all bits in the mask are turned 'on'.
 */

#define ANY_BITS_IN_MASK	0
#define ALL_BITS_IN_MASK	1

/*
 * The following macros are used exclusively in stdio to redefine global
 * identifiers and routine names to something more cryptic so that 
 * customers do not try to use them.
 */

#define scan_iops	___stdio_unsup_1
#define iob_chunks	___stdio_unsup_2
#define iob_allocated	___stdio_unsup_3
#define stream_count	___stdio_unsup_4
