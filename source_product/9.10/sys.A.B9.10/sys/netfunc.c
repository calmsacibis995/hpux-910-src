/*
 * $Header: netfunc.c,v 1.4.83.3 93/09/17 20:08:57 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/netfunc.c,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:08:57 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) netfunc.c $Revision: 1.4.83.3 $";
#endif


#include "../h/errno.h"

static
int
noop()
{
	return(0);
}

/* enosys() : For callers who need to know if target is
** not re-defined by the *_init.c routines.  E.g., so they can
** free an mbuf chain, if the target of the NETCALL isn't genned
** into the kernel. NB: we don't want to use "nosys" because we don't
** want to (possibly) signal the user process -- i.e., this procedure
** is for cases where the absence of an entry point in the system should
** be transparent to user processes.
**/
enosys()
{
    return(ENOSYS);
}

int (*netproc[])() = {
	/* 0*/ noop,
	/* 1*/ noop,
	/* 2*/ noop,
	/* 3*/ noop,
	/* 4*/ noop,
	/* 5*/ noop,
	/* 6*/ noop,
	/* 7*/ noop,
	/* 8*/ noop,
	/* 9*/ noop,
	/*10*/ noop,
	/*11*/ noop,
	/*12*/ noop,
	/*13*/ noop,
	/*14*/ noop,
	/*15*/ noop,
	/*16*/ noop,
	/*17*/ noop,
	/*18*/ noop,
	/*19*/ noop,
	/*20*/ noop,
	/*21*/ noop,
	/*22*/ noop,
	/*23*/ noop,
	/*24*/ noop,
	/*25*/ noop,
	/*26*/ noop,		/* NETDDP_GETATNODE */
	/*27*/ enosys,		/* NETDDP_AARPINPUT */
	/*28*/ enosys,		/* NETDDP_ATALK_OUTPUT */
	/*29*/ noop,
	/*30*/ noop,
	/*31*/ noop,
	/*32*/ noop,
	/*33*/ noop,            /* NET_ISDNINTR */

        /* some extras for specials, etc. */
	/*34*/ noop,
	/*35*/ noop,
	/*36*/ noop,
	/*37*/ noop
};
