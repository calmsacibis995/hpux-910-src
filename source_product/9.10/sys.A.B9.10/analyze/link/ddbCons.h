/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/link/RCS/ddbCons.h,v $
 * $Revision: 1.9.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:27:05 $
 *
 */
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


/*
 *  Wakeup conditions for the driver
 *  These are returned by the target's rdb code to indicate
 *  why it `interrupted' us.  Since the 300 version does not
 *  interrupt, these will be returned when the while loop
 *  in the "wait_for" routines terminates.
 *
 *  SPECDSKIO should never be returned
 *  SPECTERMIO should be returned only when the target is doing
 *   console I/O during boot up or a kernel panic.
 */

#define NOWAIT		0		/* Don't wait at all */
#define SPECBREAK	1		/* Spectrum entered break handler */
#define SPECDSKIO	2		/* Spectrum request disc I/O */
#define SPECTERMIO	3		/* Spectrum request terminal I/O */

/*
 *  Command to driver
 *  Some of these commands are no longer supported.  This list
 *  needs to cleaned up
 */

#define AWRITE		1		/* Atomic write */
#define AREAD		2		/* Atomic read */
#define NOT_CLR_SEMA	0xe0000000	/* Don't clear semaphore mask for */
					/* AWRITE and AREAD commands */
#define CLRINC		3		/* Clear and increment pointer */
#define MEMINIT		4		/* Initialize Spectrum memory */
#define RREAD		5		/* Ring read */
#define RWRITE		6		/* Ring write */
#define MEMCONF		7		/* Configurate memory size */
/* Command interpreted by Indigo driver only. */
#define RESET		8		/* Reset processor (INDIGO only) */
#define BOOT		9		/* Boot processor (INDIGO only) */
#define INTERRUPT	10		/* Interrupt processor (INDIGO only) */


/*
 * Command to Spectrum Interrupt Handler
 */

#define STOPSPEC	1		/* Stop Spectrum */

/*
 *  File access modes for open
 */
#define O_RDWR		2		/* Open a file for read and write */

/*
 *  Return codes
 */
#define SUCCESS		1		/* All seem well */
#define FAIL		-1		/* Something is wrong */
#define SEMALOCK	-2		/* Semaphore is locked for ioctl */
#define SPECRUN		1		/* Spectrum is running for senseSpec */
#define SPECSTOP	2		/* Spectrum is stoped for senseSpec */
#define BADLESS 	-2		/* rmWrite failed fro vmInit and */
					/* vmDownload */
/*
 *  These constants are used to tell DDB informatioon it needs to know
 *  about symbol table entries.
 *      KERNELRDB = this symbol is a kernel RDB procedure and 
 *                  break points are not allowed in this procedure.
 *      OTHER     = these symbol have no special meaning (i.e all others)
 *    
 */
#define OTHER           0  
#define KERNELRDB      -1

/*
 *  Common constants
 */
#define NULL		0
#define TRUE		1
#define FALSE		0

/*
 * This section contains TARGET specific constants
 */

/*
 *   Memory initialize access type and id
 */
#define DACCTYP 0x1f
#define DACCID  0

/*
 * Number of arguments in a GpioIoctl call
 */
#define SPEC_IOCTL_ARGS 6

/*
 * Physical memory size related constants
 *
 *   Change LAST_PAGE and HTBL_SZ when physical memory size of
 *   LESS changes.  Make 2**(HTBL_SZ+10) >= LAST_PAGE + PAGESIZE.
 *
 * The Makefile redefine LAST_PAGE and HTBL_SZ to make a system
 * with different memory size.
 *
 */

#ifdef	PA89
#define NBPG		4*1024
#else	/* PA89 */
#define NBPG		2048
#endif	/* PA89 */
#define FIRST_PAGE_840	0x3800		/* First physical page of
					   memory not reserved by
					   PDC and IODC */
					/* The communications area is 
					   in the first page. */
#  define FIRST_PAGE_FF	0x8000		/* Firefox first page */

#ifndef LAST_PAGE
#ifdef INDIGO
#define LAST_PAGE	0x1ff800	/* Last physical page */
#endif INDIGO

#endif LAST_PAGE

#ifndef HTBL_SZ
#if (LAST_PAGE == 0x3ff800)
#define HTBL_SZ         12              /* No. of bits in hash tabel index */
					/* MUST BE CONSISTANT WITH LAST_PAGE*/
#endif LAST_PAGE
#if (LAST_PAGE == 0x1ff800)
#define HTBL_SZ         11		/* No. of bits in hash table index */
					/* MUST BE CONSISTANT WITH LAST_PAGE*/
#endif LAST_PAGE
#endif HTBL_SZ
#define MEMSIZE LAST_PAGE
#define PDIRSIZE (((MEMSIZE+PAGESIZE)/PAGESIZE)*sizeof(struct PDE))
#define HASHSIZE ((1 << HTBL_SZ)*sizeof(struct HTE))
#define NOIOENTRIES 0

