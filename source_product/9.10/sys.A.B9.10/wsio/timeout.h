/*
 * @(#)timeout.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 20:34:55 $
 * $Locker:  $
 */

#ifndef _SYS_TIMEOUT_INCLUDED /* allows multiple inclusion */
#define _SYS_TIMEOUT_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
 * WARNING: Including this header file on an S700/S800 system will
 *          cause timeout to be redefined to Ktimeout.  Ktimeout on
 *          S700/S800 systems will cause expired timers (timeouts)
 *          to execute at SPL5 and will interrupt other level 5
 *          (SPL5) processing.  Ktimeout provides S300/S400 compatible
 *          timers for drivers that are shared between S300/S400
 *          and S700 systems.  The normal S700/S800 timeout routine
 *          will cause timeouts to be executed at SPL2.  To use the
 *          normal S800 SPL2 timeouts either don't include timeout.h,
 *          or if timeout.h is needed for other reasons, undef timeout
 *          after its inclusion.
 */


#ifdef _WSIO
/* this defines the structure used for doing timeouts and delays.
   for KLEENIX compatibility on 4.2.
   The address of this structure is passed to timeout so
   that it can be used by untimeout.

   timeout is defined as Ktimeout to avoid conflict with the
   4.2 routine of the same name.
*/

#define timeout Ktimeout

struct timeout {
	long timeval;			/* timeout value */
	struct timeout *f_link;		/* forward link */
	struct timeout *b_link;		/* backward link */
	int (*proc)();			/* procedure to call if timed out */
	caddr_t arg;			/* argument to pass */
};


/* Software triggers usually are associated with timeouts */

/* This is used to provide 'software triggering' of interrupt service
   routines.  This structure is linked into a list of structures to be
   serviced on the way out of other ISR's.  This permits, for example,
   deferring the service of a timeout to a lower level, or having the
   top half of the character service stuff trigger the bottom half at
   a lower level */

struct sw_intloc {
	struct sw_intloc *link;		/* must be first */
	int (*proc)();			/* to call on trigger */
	caddr_t arg;			/* argument */
	char priority;			/* level of interrupt */
	char sub_priority;		/* sub-level of interrupt */
};

#endif 	/* _WSIO */
#endif /* ! _SYS_TIMEOUT_INCLUDED */
