/* @(#) $Revision: 1.9.83.4 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/ptyio.h,v $
 * $Revision: 1.9.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:32:59 $
 */
#ifndef _SYS_PTYIO_INCLUDED /* allows multiple inclusion */
#define _SYS_PTYIO_INCLUDED

/* Ioctl Request Commands and Structures for PTY(4) */

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* Argument structure for inquiring about a slave side ioctl/open/close with the

/* Argument structure for inquiring about a slave side ioctl/open/close with the
   TIOCREQGET/TIOCREQSET ioctl requests. */
struct request_info {
	int request;		/* ioctl command received (read only) */
	int argget;		/* request to get argument trapped on
				   on slave side (read only) */
	int argset;		/* request to set argument to be returned
				   to slave side (read only) */
	short pgrp;		/* process group number of slave side process
				   doing the operation (read only) */
	short pid;		/* process id of slave side process 
				   doing the operation (read only) */
	int errno_error;	/* errno(2) error returned to be
				   returned to slave side (read/write) */
	int return_value;	/* return value for slave side (read/write) */
};

/* All Request Are For Application To Master Side Of PTY Only. */

/* pty: set/clear packet mode */
#define	TIOCPKT		_IOW('t', 112, int)

/* Read packet byte, nonzero header value definitions */
#define	TIOCPKT_DATA		0x00	/* data packet */
#define	TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define	TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define	TIOCPKT_STOP		0x04	/* stop output */
#define	TIOCPKT_START		0x08	/* start output */
#define	TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define	TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */

/* stop output, like ^S */
#define	TIOCSTOP	_IO('t', 111)

/* start output, like ^Q */
#define	TIOCSTART	_IO('t', 110)

/* remote input editing */
#define	TIOCREMOTE	_IO('t', 105)

/* enable/disable termio */
#define	TIOCTTY		_IOW('t', 104, int)

/* enable/disable ioctl, open, & close trapping */
#define	TIOCTRAP	_IOW('t', 103, int)

/* trap status */
#define	TIOCTRAPSTATUS	_IOR('t', 102, int)

/* get trapped ioctl/open/close header information */
#define	TIOCREQGET	_IOR('t', 101, struct request_info)

/* get trapped ioctl/open/close header information - non blocking */
#define TIOCREQCHECK    _IOR('t', 113, struct request_info)

/* set ioctl/open/close trap completion information */
#define	TIOCREQSET	_IOW('t', 100, struct request_info)

/* request returned when a slave side open is trapped */
#define	TIOCOPEN	_IO('t', 99)

/* request returned when a slave side close is trapped */
#define	TIOCCLOSE	_IO('t', 98)

/* request argget mask with size not filled in */
#define	TIOCARGGET	(_IOR('t', 97, int)& ~IOCSIZE_MASK)

/* request argset mask with size not filled in */
#define	TIOCARGSET	(_IOW('t', 96, int)& ~IOCSIZE_MASK)

/* enable/disable termio intercept trapping */
#define	TIOCMONITOR	_IOW('t', 95, int)

/* request to send a break (like pressing terminal break key) */
#define	TIOCBREAK	_IO('t', 94)

/* request to send a signal to slave tty process group */
#define	TIOCSIGSEND	_IO('t', 93)

/* set trapped slave-side signal handling characteristics */
#define TIOCSIGMODE	_IO('t', 92)

/* ioctl argument values for TIOCSIGMODE command */
#define TIOCSIGNORMAL	0	/* let signal handler choose to restart */
#define TIOCSIGABORT	1	/* do not allow signal handler to restart */
#define	TIOCSIGBLOCK	2	/* block signals caught by user */

#endif /* _SYS_PTYIO_INCLUDED */
