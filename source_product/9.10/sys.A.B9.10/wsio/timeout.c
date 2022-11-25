/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/timeout.c,v $
 * $Revision: 1.3.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:34:50 $
 */

/* HPUX_ID: @(#)timeout.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


/* #define TEMPORARY_DEBUG_AID	1 */


/*
 * Provide Kleenix-compatible timeout calls
 * on top of 4.2's timeout() and untimeout().
 * Timeout.h does a #define of timeout, in
 * order to divert all Kleenix driver calls
 * here.  We must undo that to access the 4.2
 * routine.
 *
 * We may want to re-do this mechanism for
 * either of two reasons:
 *
 * 1) Efficiency.
 *
 * 2) Correctness:
 *    4.2's untimeout assumes that no two
 *    timeouts are set with the same function
 *    and argument.  This seems reasonable,
 *    but it has not yet been verified!!!
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../wsio/timeout.h"
#undef	timeout
#include "../h/callout.h"

/* The b_link field is used as a flag that the struct Ktimeout is in use */
#define IN_USE	(struct Ktimeout *)0x1010101

/*
 * timeout is called to arrange that fun(arg) is called in tim/HZ seconds.
 * Loc is a caller provided space for the linked list.
 * An entry is sorted into the linked list of timeout structures.
 * The time in each structure entry is the number of HZ's unitl 
 * the next entry should be done. In this way, decrementing the
 * first entry has the effect of updating all entries.
 */

#ifdef TEMPORARY_DEBUG_AID
int	disable_timeout	=	0;
#endif

deliver_timeout(loc)
register struct Ktimeout *loc;
{
	loc->b_link = NULL; /* no longer in use */
	(*loc->proc)(loc->arg);
}

#ifdef __hp9000s300  
/* S700 version of Ktimeout is in sys.800/kern_clock.c */

Ktimeout(fun, arg, tim, loc)
int (*fun)();
caddr_t arg;
register struct Ktimeout *loc;
{
#ifdef TEMPORARY_DEBUG_AID
switch (disable_timeout)
{
	case 0:	 break;	 /*normal timeout*/
	case 1:	 return; /*timeout disabled*/
	default: tim *= disable_timeout;
}
#endif

	if (loc != NULL) {
		if (loc->b_link == IN_USE) panic("timeout on queue");
		loc->proc = fun;
		loc->arg = arg;
		loc->b_link = IN_USE;
		timeout(deliver_timeout, (caddr_t)loc, tim);
	}
	else timeout(fun, arg, tim);
}
#endif /*  __hp9000s300 */

clear_timeout(loc)
register struct Ktimeout *loc;
{
	/* unless the caller provides a timeout structure, he
	   can't delete it. Thus we dont need to look for 'ours' */

	if (loc->b_link != IN_USE) return;
	loc->b_link = NULL; /* no longer in use */
	untimeout(deliver_timeout, (caddr_t)loc);
}

/* Not sure we need this for snakes so keep it to only s300 for now */
#ifdef __hp9000s300  

#define	PDELAY	(PZERO-1)
delay(ticks)
{
	int s;
	extern wakeup();

	if (ticks<=0)
		return;
	s = spl7();	/* needs to be the priority at which softclock() runs */
	timeout(wakeup, (caddr_t)u.u_procp+1, ticks);
	sleep((caddr_t)u.u_procp+1, PDELAY);
	splx(s);
}

#endif /*  __hp9000s300 */
