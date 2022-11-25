/* $Header: termiox.h,v 1.4.83.4 93/09/17 18:36:16 kcs Exp $ */
#ifndef _SYS_TERMIOX_INCLUDED
#define _SYS_TERMIOX_INCLUDED


#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/* 
 *  I'm making up a value for NFF for now.  -jph 
 */

#define NFF	10

struct termiox {
	unsigned short          x_hflag;	/* hw flow cntl modes */
	unsigned short          x_cflag;	/* clock modes */
	unsigned short          x_rflag[NFF];	/* reserved modes */
	unsigned short          x_sflag;	/* spare local modes */
};

#define RTSXOFF  1
#define CTSXON   2

#define TCGETX  _IOR('X', 0, struct termiox)   /* get parameters */
#define TCSETX  _IOW('X', 1, struct termiox)   /* set parameters */
#define TCSETXW _IOW('X', 2, struct termiox)   /* set parameters after
						   output has drained */
#define TCSETXF _IOW('X', 3, struct termiox)   /* set parameters after
						   flushing output and
						   discard unread input */

#endif /* INCLUDE_HPUX_SOURCE */

#endif /* _SYS_TERMIOX_INCLUDED */
