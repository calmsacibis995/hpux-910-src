/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ram.h,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:17:05 $
 */
/* HPUX_ID: @(#)ram.h	52.3     88/08/02  */
#ifndef _MACHINE_RAM_INCLUDED /* allow multiple inclusions */
#define _MACHINE_RAM_INCLUDED

#ifdef	_KERNEL_BUILD
#include "../h/ioctl.h"
#else	/* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* max ram volumes cannot exceed 16 */
#define RAM_MAXVOLS 16

/* ioctl to deallocate ram volume */
#define RAM_DEALLOCATE	_IOW('R', 1, int)

/* ioctl to reset the access counter to ram volume */
#define RAM_RESETCOUNTS	_IOW('R', 2, int)


/* defines for 'new' semantics (rdg) */
#define RAM_NEW_SEMANTICS 0x1
#define RAM_NOCACHE_BIT 0x2
#define RAM_VIRTUAL_BIT 0x4
#define RAM_RESERVED 0x8

#define RAM_SIZE(x) \
    (((x) & RAM_NEW_SEMANTICS) ? RAM_SIZE_NEW(x) : RAM_SIZE_OLD(x))

/* new size semantics: */
/* up to 65535 chunks of 64k each */
/* upper 16 bits of minor number give number of chunks */
#define RAM_SIZE_NEW(x) ((((x) >> 4) & 0xffff) << 8)   

/* old size semantics: */
/* io mapping minor number macros */
/* up to 1048575 - 256 byte sectors */
#define	RAM_SIZE_OLD(x)	((x) & 0xfffff) 	/* XXX */

/* up 16 disc allowed */
#define	RAM_DISC(x)	(((x) >> 20) & 0xf) 	/* XXX */
#define	RAM_MINOR(x)	((x) & 0xffffff) 	/* XXX */
#define RAM_NOCACHE(x)     ((x) & RAM_NOCACHE_BIT)
#define RAM_VIRTUAL(x)     ((x) & RAM_VIRTUAL_BIT)

#define LOG2SECSIZE 8	/* (256 bytes) "sector" size (log2) of the ram discs */

#define RAM_RETURN 1
#define RAM_BOOT   2

struct ram_descriptor {
	char	*addr;
	int	size;
	short	opencount;
	short	flag;
	int	rd1k;
	int	rd2k;
	int	rd3k;
	int	rd4k;
	int	rd5k;
	int	rd6k;
	int	rd7k;
	int	rd8k;
	int	rdother;
	int	wt1k;
	int	wt2k;
	int	wt3k;
	int	wt4k;
	int	wt5k;
	int	wt6k;
	int	wt7k;
	int	wt8k;
	int	wtother;
} ram_device[RAM_MAXVOLS];
#endif /* _MACHINE_RAM_INCLUDED */
