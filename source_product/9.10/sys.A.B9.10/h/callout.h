/* $Header: callout.h,v 1.15.83.4 93/09/18 06:45:06 root Exp $ */

#ifndef _SYS_CALLOUT_INCLUDED /* allows multiple inclusion */
#define _SYS_CALLOUT_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_STDSYMS_INCLUDED  */

#ifndef _SYS_TYPES_INCLUDED
#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_TYPES_INCLUDED */

/*
 * The callout structure is for
 * a routine arranging
 * to be called by the clock interrupt
 * (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab
 * delays on typewriters.
 */

struct	callout {
	int	c_time;		/* incremental time */
	caddr_t	c_arg;		/* argument to routine */
	int	(*c_func)();	/* routine */
	struct	callout *c_next;
#if defined(__hp9000s800) && defined(_WSIO)
	int	flag;		/* driver or non-driver callout */
#endif /* _WSIO && __hp9000s800 */
};

#if (defined(__hp9000s800) && defined(_WSIO))
#define DRIVER_CALLOUT 1
#endif /* __hp9000s800 && _WSIO */

#ifdef _KERNEL
#ifdef __hp9000s800
extern	struct	callout *callfree, *callout, calltodo;	/* c.f. globals.h */
extern	int	ncallout;
#endif	/* __hp9000s800 */
#ifdef __hp9000s300
struct	callout *callfree, *callout, calltodo;		/* c.f. globals.h */
int	ncallout;

/* defines for flags argument to do_smart_poll() and set_smart_poll() */
#define SMART_POLL_CLOCK_TICK	0x01
#define SMART_POLL_IDLE		0x02
#endif /* __hp9000s300 */
#endif /* _KERNEL */

#endif /* _SYS_CALLOUT_INCLUDED */
