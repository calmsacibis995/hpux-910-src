/* $Header: iomap.h,v 1.4.83.4 93/09/17 18:27:32 kcs Exp $ */

#ifndef _SYS_IOMAP_INCLUDED /* allows multiple inclusion */
#define _SYS_IOMAP_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* I/O mapping ioctl command defines */
#define	IOMAPMAP	_IOWR('M',1,int)
#define	IOMAPUNMAP	_IOWR('M',2,int)

/* I/O mapping minor number macros */
#ifdef	__hp9000s300
#define	IOMAP_SC(x)	(((x) >> 8) & 0xffff)
#define	IOMAP_SIZE(x)	((x) & 0xff)
#endif	/* __hp9000s300 */

#endif /* _SYS_IOMAP_INCLUDED */
