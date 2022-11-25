/*
 * @(#)basic.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:22:16 $
 * $Locker:  $
 */

/*
 * Original version based on:
 * Revision 63.1  88/05/19  10:20:55  10:20:55  sjl (Steve Lilker)
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * This file defines basic data types and macros not directly tied to the
 * debugger (but used by it).
 */

#define void int			/* don't actually use "void" */
#define bits unsigned			/* makes bit fields shorter  */

#ifdef FOCUS				/* int is far more efficient for us */
typedef int		FLAGT, *pFLAGT;		/* also known as a boolean */
#else
typedef char		FLAGT, *pFLAGT;
#endif

/*
 * This is a kludge that supports name "pointers" being either indices into
 * the string table or actual char ptrs (name already in memory).
 * See bitHigh in sym.h.
 */
typedef long	SBT;

/*
 * If you get "illegal type combination" errors on the next line, #define
 * SYS_UINT 1 before you include basic.h.
 */
#ifndef SYS_UINT
typedef unsigned int	uint;
#endif
/*
 * If you get "illegal type combination" errors on the next line, #define
 * SYS_USHORT 1 before you include basic.h.
 */
#ifndef SYS_USHORT
typedef unsigned short	ushort;
#endif

typedef unsigned char	uchar;
typedef unsigned long	ulong;


/*
 * This is used for various kinds of ADDRESSES.	 Since it is not really a
 * pointer type, it can be used pretty vaguely.
 */

#ifdef OLDSYMTAB
typedef uint		ADRT, *pADRT;
#endif
typedef int		REGT;

#define adrNil	   ((ADRT) -1)
#define adrUnknown ((ADRT) -2)		/* for HPSYMTAB only; see AdrFIsym() */

#define lengthen(a)	((long) ((uint) (a)))
#define shorten(a)	((short) (a))
#define FOdd(x)		(((int) (x) & 1) == 1)
#define FEven(x)	(((int) (x) & 1) == 0)
#define FQuad(x)	(((int) (x) & 3) == 0)

/*
 * These are used FOR BUILDING TAGS and ext.h:
 */

#define exportdefine #define

/*
 * OTHER RANDOM TYPES:
 */

#define AND &&
#define OR  ||

#define true  1
#define false 0

#define nil	((char *) 0)
#define sbNil	((char *) 0)

#define emptyset 0
#define SeFInt(x) (1 << (x))			/* set element from integer */
#define In(x, y)  ((SeFInt (x) & (y)) != 0)	/* test for word inclusion  */

#ifdef NLS
#include <stdio.h>
#include <msgbuf.h>
#ifdef HPE
int	nl_fn;
char	nl_msg_buf[NL_TEXTMAX];
#endif
#else
#define nl_msg(i,s) (s)
#endif

/*
typedef	int	VOID, BOOL, WORD, ERROR;
typedef	char	*TEXT ;

#define		FOREVER		for(;;)
#define		FAST		register

#define		EXPORT
#define		IMPORT		extern
*/

#ifdef HPE
#undef		NULL
#define		NULL		0
#endif

#define		NO		0
#define         OFF             0
#define	        false		0
#define         FALSE           0

#define		YES		1
#define		ON		1
#define		true		1
#define		TRUE		1
