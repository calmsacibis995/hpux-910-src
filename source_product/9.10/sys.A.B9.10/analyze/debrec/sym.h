/*
 * @(#)sym.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:23:36 $
 * $Locker:  $
 */

/*
 * Original version based on: 
 * Revision 56.1  88/02/24  14:48:34  14:48:34  markm
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
 * This file defines symbol handling types, structures, etc.
 */

#define isymNil (-1L)		/* nil   symbol number */
#define isym0	  0L		/* first symbol number */

/*
 * This is a double-kludge, used to distinguish true string pointers (in cdb's
 * memory) from iss values saved in TYR.sbVar fields.  Non-FOCUS systems using
 * BSD41 or HPSYMTAB(II) set the high bit and SbInCore() merely clears it as 
 * needed.
 *
 * For FOCUS, bitHigh is zero, so or'ing it in or masking its inverse is a
 * no-op.  Meanwhile, bitsIssMax is the largest issMax can ever be (see
 * InitSymfile()).  If bits are set above bitsIssMax, we must have a string
 * pointer (SB-rel, DB-rel, or EDSP).  This assumes all cdb EDSPs have a code
 * segment number bigger than some value (see below).
 */

#ifdef FOCUS
#define bitHigh	   (0x00000000)
#define bitsIssMax (0x007fffff)		/* code seg > 31; max iss = 2^23-1 */
#else
#define bitHigh	   (0x80000000)
#endif


/***********************************************************************
 * MAGIC NUMBERS:
 */

#if ((FOCUS || S200 || SPECTRUM) && (! AOUTINC))
#ifdef HPE
#include <spectrum/som.h>
#else
#include <a.out.h>		/* get the magic numbers and more */
#endif
#else
#define magicImpure	0407	/* impure format	*/
#define magicReadOnly	0410	/* read-only text	*/
#define magicDemand	0413	/* demand load format	*/
#define magicLpd	0401	/* lpd (UNIX/RT)	*/
#define magicSplit	0411	/* separated I&D	*/
#define magicOverlay	0405	/* overlay		*/
#endif


/***********************************************************************
 * OBJECT FILE AND SYMBOL TABLE STUFF:
 */

/*
 * In invocation of this macro, argument must not be a function:
 */

#ifdef SPECTRUM
#ifdef HPE
#ifdef DEMAND_MAGIC
#define FBadMagic(x)  (	 (x.system_id != HP9000S800_ID)  || \
                        ((x.file_type != EXECLIBMAGIC) && \
			 (x.file_type != NAEXECMAGIC) && \
			 (x.file_type != EXEC_MAGIC) && \
                         (x.file_type != DEMAND_MAGIC) && \
			 (x.file_type != SHARE_MAGIC))	)
#else
#define FBadMagic(x)  (	 (x.system_id != HP9000S800_ID)  || \
                        ((x.file_type != EXECLIBMAGIC) && \
			 (x.file_type != NAEXECMAGIC) && \
			 (x.file_type != EXEC_MAGIC) && \
			 (x.file_type != SHARE_MAGIC))	)
#endif
#else
#ifdef DEMAND_MAGIC
#define FBadMagic(x)  (	 (x.system_id != HP9000S800_ID)  || \
			((x.a_magic != EXEC_MAGIC) && \
                         (x.a_magic != 0x102) && \
                         (x.a_magic != DEMAND_MAGIC) && \
			 (x.a_magic != SHARE_MAGIC))	)
#else
#define FBadMagic(x)  (	 (x.system_id != HP9000S800_ID)  || \
			((x.a_magic != EXEC_MAGIC) && \
                         (x.a_magic != 0x102) && \
			 (x.a_magic != SHARE_MAGIC))	)
#endif
#endif
#endif

#ifdef S200
#ifdef S200BSD
#define FBadMagic(x)  (	((x.a_magic.system_id != HP98x6_ID)  && \
			 (x.a_magic.system_id != HP9000S200_ID)) || \
			((x.a_magic.file_type != EXEC_MAGIC) && \
			 (x.a_magic.file_type != DEMAND_MAGIC) && \
			 (x.a_magic.file_type != SHARE_MAGIC))	)
#else
#define FBadMagic(x)  (	 (x.a_magic.system_id != HP98x6_ID)  || \
			((x.a_magic.file_type != EXEC_MAGIC) && \
			 (x.a_magic.file_type != SHARE_MAGIC))	)
#endif

#define CbTextOffset(x) (cbEXECR)
#define CbSymOffset(x)	(LESYM_OFFSET(x))
#endif

#ifdef FOCUS
#define FBadMagic(x)  (	 (x.a_magic.system_id != HP9000_ID)  || \
			((x.a_magic.file_type != EXEC_MAGIC) && \
			 (x.a_magic.file_type != SHARE_MAGIC))	)
#endif

#ifdef SPECTRUM
#include "symtab.h"
#else
#include <symtab.h>
#endif

#ifdef S200
/* missing defs for this case */
#endif

#ifdef FOCUS

typedef LINK_SYMBOL	LSTR;		/* linker symbol table */
typedef LSTR		*pLSTR;

#define cbLSTR		((long)(sizeof (LINK_SYMBOL)))

#endif /* FOCUS */

#ifdef SPECTRUM

typedef struct symbol_dictionary_record   LSTR;	/* linker symbol table */
typedef LSTR				*pLSTR;

#define cbLSTR		((long)(sizeof (LSTR)))

#endif /* SPECTRUM */

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

#define cbSLTR		SLTBLOCKSIZE

extern char * SbInCore();		/* we also use the cache */

