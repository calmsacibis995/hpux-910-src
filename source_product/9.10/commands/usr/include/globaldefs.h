/* @(#) $Revision: 64.1 $ */       
/* file globaldefs.h - a rational place for common defines for masks, etc. */
#ifndef _GLOBALDEFS_INCLUDED /* allows multiple inclusions */
#define _GLOBALDEFS_INCLUDED

#ifdef __hp9000s300
#define	LOBYTE	0377	/* mask to retrieve low byte from a word */
#define LO5BITS 037	/* mask to retrieve low 5 bits from a word */
#define BLOCKSIZE 512	/* basic file system block size (a la BSIZE) */

#ifndef DIRSIZ
#define DIRSIZ 14
#endif /* not DIRSIZ */
#endif /* __hp9000s300 */

#endif /* _GLOBALDEFS_INCLUDED */
