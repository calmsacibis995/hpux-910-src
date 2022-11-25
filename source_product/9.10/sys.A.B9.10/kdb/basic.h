/* @(#) $Revision: 66.2 $ */    

/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This file defines basic data types and macros not directly tied to the
 * debugger (but used by it).
 */
#include <sys/types.h>			/* brings in things like uint */

#define void int			/* don't actually use "void" */
#define bits unsigned			/* makes bit fields shorter  */

typedef char		FLAGT, *pFLAGT;

/*
 * This is a kludge that supports name "pointers" being either indices into
 * the string table or actual char ptrs (name already in memory).
 * See bitHigh in sym.h.
 */
typedef long	SBT;

typedef unsigned char	uchar;
typedef unsigned long	ulong;

/*
 * This is used for various kinds of ADDRESSES.	 Since it is not really a
 * pointer type, it can be used pretty vaguely.
 */

typedef int		REGT;

#define adrNil	   ((ADRT) -1)
#define adrUnknown ((ADRT) -2)		/* for HPSYMTAB only; see AdrFIsym() */

#define lengthen(a)	((long) ((uint) (a)))

/*
 * These are used FOR BUILDING TAGS and ext.h:
 */

#define exportdefine #define
#define export
#define local

/*
 * OTHER RANDOM TYPES:
 */

#define AND &&
#define OR  ||

#define true  1
#define false 0

#define nil	((char *) 0)
#define sbNil	((char *) 0)
