/* $Header: lib_strbuf.h,v 1.2.109.2 94/10/28 17:37:03 mike Exp $
 * lib_strbuf.h - definitions for routines which use the common string buffers
 */

/*
 * Sizes of things
 */
# define	LIB_NUMBUFS	20
# define	LIB_BUFLENGTH	80

/*
 * Macro to get a pointer to the next buffer
 */
# define	LIB_GETBUF(buf) \
do\
{\
    buf = &lib_stringbuf[lib_nextbuf][0];\
    if (++lib_nextbuf >= LIB_NUMBUFS)\
	lib_nextbuf = 0;\
} while (0)

extern char     lib_stringbuf[LIB_NUMBUFS][LIB_BUFLENGTH];
extern int  lib_nextbuf;
