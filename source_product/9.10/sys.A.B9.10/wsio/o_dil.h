/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/o_dil.h,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:30:35 $
 */
/*@(#) $Header: o_dil.h,v 1.4.83.3 93/09/17 20:30:35 kcs Exp $ */
#ifndef O_DIL_INCLUDED /* allows multiple inclusion */
#define O_DIL_INCLUDED

/* HPUX_ID: @(#)o_dil.h	46.2     86/11/03  */
/*
**	o_dil.h	5.0 Device I/O Library header
*/

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */


struct o_ioctl_type {
	int type;
	int data[3];
};

#define	O_CMDSIZE	122

struct o_ioctl_cmd_type {
	int length;
	unsigned char data[O_CMDSIZE];
};

#ifdef __hp9000s300
/* 5.141 memory mapped structure */
struct o_fd_info {	/* this is 16 bytes for quick access */
	char state;
	char ba;
	char *cp;
	char pattern;
	char card_address;
	char reason;
	char addr;
	short temp;
	int  dev;
};

/* 5.15 memory mapped structure */
struct D515_fd_info {
	char state;
	char ba;
	char *cp;
	char pattern;
	char card_address;
	char reason;
	char addr;
	short temp;
	int  dev;
	struct sc_info *d_sc;
};
#endif /* _hp9000s300 */

/* ioctl's for all dil channels */

#define	O_IO_CONTROL	_IOWR('I', 1, struct o_ioctl_type)
#define	O_IO_STATUS	_IOWR('I', 2, struct o_ioctl_type)

#define	O_HPIB_CONTROL	_IOWR('H', 1, struct o_ioctl_type)
#define	O_HPIB_STATUS	_IOWR('H', 2, struct o_ioctl_type)
#define	O_HPIB_SEND_CMD	_IOWR('H', 3, struct o_ioctl_cmd_type)

#ifdef __hp9000s300
#define	O_HPIB_MAP	_IOWR('I', 5, struct o_fd_info)
#define	O_HPIB_UNMAP	_IOWR('I', 6, struct o_fd_info)
#define	O_HPIB_REMAP	_IOWR('I', 7, struct o_fd_info)
#define	O_HPIB_UNMAP_MARK	_IOWR('I', 8, struct o_fd_info)

#define	D515_HPIB_MAP	_IOWR('I', 5, struct D515_fd_info)
#define	D515_HPIB_UNMAP	_IOWR('I', 6, struct D515_fd_info)
#define	D515_HPIB_REMAP	_IOWR('I', 7, struct D515_fd_info)
#define	D515_HPIB_UNMAP_MARK	_IOWR('I', 8, struct D515_fd_info)

#define	D5141LIB	0
#define	D515LIB		1
#endif /* _hp9000s300 */
#define	D52LIB		2

#endif /* O_DIL_INCLUDED */
#endif /* _WSIO */
