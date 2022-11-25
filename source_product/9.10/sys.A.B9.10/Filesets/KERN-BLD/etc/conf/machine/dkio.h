/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/dkio.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:59 $
 */
/* HPUX_ID: @(#)dkio.h	52.1		88/04/19 */
/*
 * Structures and definitions for disk io control commands
 *
 * THIS WHOLE AREA NEEDS MORE THOUGHT.  FOR NOW JUST IMPLEMENT
 * ENOUGH TO READ AND WRITE HEADERS ON MASSBUS DISKS.  EVENTUALLY
 * SHOULD BE ABLE TO DETERMINE DRIVE TYPE AND DO OTHER GOOD STUFF.
 */

#ifndef _MACHINE_DKIO_INCLUDED
#define _MACHINE_DKIO_INCLUDED

/* disk io control commands */
#define DKIOCHDR	_IO('d', 1)	/* next I/O will read/write header */

#endif /* not _MACHINE_DKIO_INCLUDED */
