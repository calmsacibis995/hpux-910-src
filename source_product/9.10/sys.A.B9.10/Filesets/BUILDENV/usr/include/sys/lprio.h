/* @(#) $Revision: 1.11.83.4 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/lprio.h,v $
 * $Revision: 1.11.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:28:55 $
 */
#ifndef _SYS_LPRIO_INCLUDED /* allows multiple inclusion */
#define _SYS_LPRIO_INCLUDED
/*
 * Line Printer Type Devices I/O Control
 */

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

struct lprio {
	short	ind;		/* indent */
	short	col;		/* columns per page */
	short	line;		/* lines per page */
	short	bksp;		/* backspace handling flag */ 
	short	open_ej;	/* pages to eject on open */
	short	close_ej;	/* pages to eject on close */
        short   raw_mode;       /* used to indicate raw or cooked data */

};

/* ioctl commands */
#ifdef __hp9000s300
#define	LPR	('l'<<8)
#endif /* __hp9000s300 */
#define	LPRGET	_IOR('l', 1, struct lprio)
#define	LPRSET	_IOW('l', 2, struct lprio)

#ifdef __hp9000s800
struct lprstat {
	int	status;		/* generic printer status */
};

/* #define LPRSTATUS	_IOR('1', 3, struct lprstat) */
#endif /* __hp9000s800 */

/* flags for bksp */
#define PASSTHRU	0
#define OVERSTRIKE	1

/* flags for raw mode */
#define COOKED_MODE     0
#define RAW_MODE        1

#ifdef __hp9000s800
/* generic status word bit definitions from lsb to msb
	not_on_line
	paper_out
	paper_jam
	platen_open
	ribbon_fail
	self_test_fail
 */
#endif /* __hp9000s800 */

#endif /* _SYS_LPRIO_INCLUDED */
