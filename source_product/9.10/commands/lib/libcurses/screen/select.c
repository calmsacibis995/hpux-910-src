/* @(#) $Revision: 51.1 $ */   
/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */


#include "curses.ext"
#include <signal.h>

#define NAPINTERVAL 100

/* From early specs - this may change by 4.2BSD */
struct _timeval {
	long tv_sec;
	long tv_usec;
};

/* removed Bell code that simulates the select() call */

#if !(defined hp9000s200 || defined hp9000s500 || defined hp9000s800)
int
select(nfds, prfds, pwfds, pefds, ms)
register int nfds;
int *prfds, *pwfds, *pefds;
struct _timeval *ms;
{
	/* Can't do it, but at least compile right */
	return ERR;
}
#endif
