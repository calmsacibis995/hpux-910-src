/*
 * $Header: netmp.h,v 1.4.83.4 93/09/17 19:01:06 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/netmp.h,v $
 * $Revision: 1.4.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:01:06 $
 */

#ifndef _SYS_NETMP_INCLUDED
#define _SYS_NETMP_INCLUDED

extern int netmp_current_pmodel;
/*
** Define symbols for LANLINK/MP netmp.c routine, and callers:
*/
#define NETMP_THO 	1
#define NETMP_THICS	2

/*
** Define macroes for protection model, according to machine type.
** Cases:
** 1. S800
** 2. S300
** 3. Preprocessor symbol "MP" not defined (i.e., MP is "turned off")
*/

#ifdef __hp9000s800
#ifdef MP
#define NETMP_GO_EXCLUSIVE(oldlevel, savestate) 	\
    if (netmp_current_pmodel == NETMP_THO) 		\
	netmp_go_excl_tho(&savestate);	\
    else    						\
	oldlevel = netmp_go_excl_thics(&savestate)

#define NETMP_GO_UNEXCLUSIVE(oldlevel,savestate)         \
    if (netmp_current_pmodel == NETMP_THO) 		 \
	netmp_go_unexcl_tho(&savestate);	 \
    else    						 \
	netmp_go_unexcl_thics(oldlevel, &savestate)

/*
** Note that the netisr exclusive/unexclusive pair is
** vectored to the same place as other users for THO,
** but to a different place than for other users for THICS.
** When the protection is THICS, but netisr is running as a
** process, then we also take the network semaphore.  This
** helps to avoid spin-locking (on the spl_lock).
*/

#define NETISR_MP_GO_EXCLUSIVE(oldlevel, savestate) 	\
    if (netmp_current_pmodel == NETMP_THO) 		\
	netmp_go_excl_tho(&savestate);			\
    else {						\
	extern int netisr_priority;			\
	if (netisr_priority != -1) 		 	\
	    netmp_go_excl_tho(&savestate);		\
	oldlevel = netisr_go_excl_thics();           	\
    }

#define NETISR_MP_GO_UNEXCLUSIVE(oldlevel, savestate) 	\
    if (netmp_current_pmodel == NETMP_THO) 		\
	netmp_go_unexcl_tho(&savestate);		\
    else {  						\
	extern int netisr_priority;			\
	netisr_go_unexcl_thics(oldlevel);	  	\
	if (netisr_priority != -1) 		 	\
	    netmp_go_unexcl_tho(&savestate);		\
    }

#else /* !MP */
#define NETMP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETMP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#define NETISR_MP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETISR_MP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#define NETICS_MP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETICS_MP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#endif /* !MP */
#else /* __hp9000s300 */
#define NETMP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETMP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#define NETISR_MP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETISR_MP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#define NETICS_MP_GO_EXCLUSIVE(oldlevel, savestate) oldlevel = splnet()
#define NETICS_MP_GO_UNEXCLUSIVE(oldlevel, savestate) splx(oldlevel)
#endif /* __hp9000s300 */
/*
** NETICS_* macroes are for the use of drivers and other cases
** where the protection has to be callable from the ICS, hence they
** cannot lock the network semaphore.  Since the protection model
** is defined only for "top-half+ICS" and not "top-half only"
** there is code to verify that this is the case.
*/

#define NETICS_GO_EXCLU(oldlevel, savestate) 		\
    if (netmp_current_pmodel == NETMP_THO)		\
	netmp_bad_call();				\
    else    						\
	oldlevel = netisr_go_excl_thics()

#define NETICS_GO_UNEXCLU(oldlevel, savestate) 		\
    if (netmp_current_pmodel == NETMP_THO)		\
	netmp_bad_call();				\
    else    						\
	netisr_go_unexcl_thics(oldlevel)

/*  Debug stuff for network semaphore -- for making sure he who should own
**  	the sema actually DOES own it, etc.
*/
#define MP_ASSERT_SEMA(a, m)

/* Define macroes for use in re-applying splnet() calls, for non-MP
** situations.  At the present time, s800's are either MP or non-MP,
** and 300s are non-MP only.  s300's run netisr as a process
** or off the ICS.
*/
#ifdef MP
#define NET_SPLNET(s)		/* define to "nothing" */
#define NET_SPLX(s)		/* define to "nothing" */
#else /* !MP */
#define NET_SPLNET(s) s = splnet()
#define NET_SPLX(s) splx(s)
#endif /* !MP */
#endif /* _SYS_NETMP_INCLUDED */
