/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/dil.h,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:27:14 $
 */
/* @(#) $Revision: 1.4.83.3 $ */     
/*
**	dil.h	Device I/O Library header
*/
#ifndef _SYS_DIL_INCLUDED
#define _SYS_DIL_INCLUDED

/* dil.h - Device I/O Library header */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#if defined(_INCLUDE_HPUX_SOURCE)

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

struct sc_info {
	char mapped;
	char state;
	char card_type;
};

#define	TI9914_HOLDOFF	0x01
#define	PASS_IN		0x02
#define	PASS_OUT	0x04

/* bit field defines for general interface register access */
#define	READ_ACCESS	0x01
#define WORD_MODE	0x02

struct ioctl_type {
	char sc_state;
	int type;
	int data[3];
};

#define	CMDSIZE	122

struct ioctl_cmd_type {
	char sc_state;
	int length;
	unsigned char data[CMDSIZE];
};


#ifdef __cplusplus
    extern "C" {
#endif
struct ioctl_intr_type {
	int eid;
	int cause;
	int mask;
#ifdef __cplusplus
	int (*proc)(...);
#else
	int (*proc)();
#endif
};
#ifdef __cplusplus
}
#endif

/* this is the memory mapped structure */
struct fd_info {
	char state;
	char ba;
	char *cp;
	short pattern;
	char card_address;
	char reason;
	char addr;
	short temp;
	int  dev;
	char card_type;
	struct sc_info *d_sc;
};

/* ioctl's for all dil channels */

#define IO_CONTROL		_IOWR('I', 1, struct ioctl_type)
#define	IO_STATUS		_IOWR('I', 2, struct ioctl_type)
#define	IO_MAP			_IOWR('I', 3, struct fd_info)
#define	IO_UNMAP		_IOWR('I', 4, struct fd_info)
#define	IO_REMAP		_IOWR('I', 5, struct fd_info)
#define	IO_UNMAP_MARK		_IOWR('I', 6, struct fd_info)
	
#define HPIB_CONTROL		_IOWR('H', 1, struct ioctl_type)
#define	HPIB_STATUS		_IOWR('H', 2, struct ioctl_type)
#define	HPIB_SEND_CMD		_IOWR('H', 3, struct ioctl_cmd_type)
#define HPIB_INTERRUPT_GET	_IOWR('H', 5, struct ioctl_intr_type)
#define HPIB_INTERRUPT_SET	_IOWR('H', 6, struct ioctl_intr_type)
#define HPIB_INTERRUPT_ENABLE	_IOWR('H', 7, struct ioctl_intr_type)
#define HPIB_SET_DIL_SIG	_IOR('H', 8, struct ioctl_type)

#define	HPIB_SET_STATE   	_IO('I', 9)
#define	HPIB_CLEAR_STATE	_IO('I', 10)
#define	HPIB_GET_STATE		_IO('I', 11)

#define	GPIO_CONTROL		_IOWR('g', 1, struct ioctl_type)
#define	GPIO_STATUS		_IOWR('g', 2, struct ioctl_type)

/* ioctl definition for general interface register access */

#define	IO_REG_ACCESS	_IOWR('R', 1, struct ioctl_type)


/* IO types for all channels */
#define	IO_TIMEOUT		1
#define	IO_WIDTH		2
#define	IO_SPEED		3
#define	IO_READ_PATTERN		4
#define	IO_RESET		5
#define	IO_TERM_REASON		6
#define	IO_LOCK			7
#define	IO_UNLOCK		8
#define IO_RAW_DEV		9
#define	IO_INTERRUPT		10
#define	IO_DMA			11
#define IO_SET_MAP_ADDR		12

/* dma control functions */
#define	DMA_ACTIVE	1
#define	DMA_UNACTIVE	2
#define	DMA_RESERVE	3
#define	DMA_UNRESERVE	4
#define	DMA_LOCK	5
#define	DMA_UNLOCK	6

/* lock functions */
#define	DIL_LOCK	1
#define LOCK		DIL_LOCK 	/* backwards compatibility from
						when DIL_LOCK was LOCK */
#define	DIL_UNLOCK	2

/* channel type */
#define	CHANNEL_TYPE		22

/* termination reasons */
#define TR_ABNORMAL		0x00000000
#define TR_COUNT		0x00000001
#define TR_MATCH		0x00000002
#define TR_END			0x00000004

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_DIL_INCLUDED */
