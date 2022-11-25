/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/bsdtty.h,v $
 * $Revision: 1.12.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 17:02:26 $
 */
/* HPUX_ID: @(#) $Revision: 1.12.83.3 $  */
/* Ioctl request commands and structures for BSD job control */
#ifndef _SYS_BSDTTY_INCLUDED
#define _SYS_BSDTTY_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* BSD struct for local special characters */

struct ltchars {
	unsigned char	t_suspc;	/* Stop process character */
	unsigned char	t_dsuspc;	/* Delayed stop process character */
	unsigned char	t_rprntc;	/* Reserved */
	unsigned char	t_flushc;	/* Reserved */
	unsigned char	t_werasc;	/* Reserved */
	unsigned char	t_lnextc;	/* Reserved */
};

/* BSD ioctls */

#define	TIOCSLTC	_IOW('T', 23, struct ltchars)
					/* Set local special characters */
#define	TIOCGLTC	_IOR('T', 24, struct ltchars)
					/* Get local special characters */
#define	TIOCLBIS	_IOW('T', 25, int)
					/* Set local mode word bits */
#define	TIOCLBIC	_IOW('T', 26, int)
					/* Clear local mode word bits */
#define	TIOCLSET	_IOW('T', 27, int)
					/* Set local mode word */
#define	TIOCLGET	_IOR('T', 28, int)
					/* Get local mode word */
#define	TIOCSPGRP	_IOW('T', 29, int)
					/* Set TTY process group */
#define	TIOCGPGRP	_IOR('T', 30, int)
					/* Get TTY process group */

/* BSD local mode word bits */

#define	LTOSTOP	0000001	/* Send SIGTTOU for background write or ioctl */
#endif /* _SYS_BSDTTY_INCLUDED */
