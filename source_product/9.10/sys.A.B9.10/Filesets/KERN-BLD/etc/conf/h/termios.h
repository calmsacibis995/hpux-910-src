/* $Header: termios.h,v 1.6.83.4 93/09/17 18:36:11 kcs Exp $ */
#ifndef _SYS_TERMIOS_INCLUDED
#define _SYS_TERMIOS_INCLUDED

#define _TERMIOS_INCLUDED

#ifdef _KERNEL_BUILD
#    include "../h/termio.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/termio.h>
#endif /* _KERNEL_BUILD */

#endif /* _SYS_TERMIOS_INCLUDED */
