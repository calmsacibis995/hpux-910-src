/* @(#) $Revision: 10.2 $ */     
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This file defines symbol handling types, structures, etc.
 */

#define isymNil (-1L)		/* nil   symbol number */
#define isym0	  0L		/* first symbol number */

/*
 * This is a double-kludge, used to distinguish true string pointers (in cdb's
 * memory) from iss values saved in TYR.sbVar fields.  Non-FOCUS systems using
 * BSD41 or HPSYMTAB set the high bit and SbInCore() merely clears it as needed.
 */

#define bitHigh	   (0x00000000)

#include "symtab.h"

/*
 * This is a little wierd.  SYMR is a full symbol record (more than one block)
 * but cbSYMR is just the size of one block because that's the level at which
 * they are cached.  Note that the cache is an array of SYMBLOCKs, not SYMRs.
 */

typedef union dnttentry SYMR;		/* debug name and type table */
typedef SYMR		*pSYMR;
typedef struct DNTT_BLOCK SYMBLOCK;

#define cbSYMR		DNTTBLOCKSIZE

#define islt0		0L

typedef union sltentry	SLTR;		/* source line table */
typedef SLTR		*pSLTR;

#define psltNil		((pSLTR)-1)	/* for empty table pointers */
#define psymNil		((pSYMR)-1)
#define pfdrNil		((pFDR)-1)
#define ppdrNil		((pPDR)-1)

#define cbSLTR		SLTBLOCKSIZE

extern char * SbInCore();		/* we also use the cache */

