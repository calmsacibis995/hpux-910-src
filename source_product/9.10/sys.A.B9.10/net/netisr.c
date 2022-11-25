/*
 * $Header: netisr.c,v 1.40.83.8 93/11/11 12:25:43 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/netisr.c,v $
 * $Revision: 1.40.83.8 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:25:43 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) netisr.c $Revision: 1.40.83.8 $";
#endif

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/callout.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/netfunc.h"
#include "../h/malloc.h"
#include "../h/vmmac.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../net/netisr.h"
#ifdef __hp9000s800
#include "../machine/int.h"
#include "../machine/spl.h"
extern struct eirrswitch eirr_switch[];
#endif /* __hp9000s800 */

#include "../h/spinlock.h"
#include "../net/netmp.h"
#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	/* FSD_KI */
#ifdef	__hp9000s300
#include "../wsio/timeout.h"
#undef timeout
#endif

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#define	STATIC	/* nothing  */

extern int netisr_priority;

#ifdef  __hp9000s800
#pragma align 16
#endif  /* __hp9000s800 */
STATIC int netirr = 0;			/* interrupt request register */
STATIC int netisr_initialized = FALSE;
STATIC int netisr_daemon_running = FALSE;	/* netisr daemon semaphore */
STATIC int net_timo_initialized = FALSE;	/* sanity check */

STATIC struct proc *netisr_procp = NULL;	/* daemon proc table pointer */

#ifdef MP
static sync_t netisr_sync;
#endif /* MP */

#ifdef notdef
STATIC int netisr_kill_event;			/* kill netisr event logged */
#endif 

struct ntimo {
	int		(*nt_func)();
	caddr_t		nt_arg;
	struct ntimo *	nt_prev;
	struct ntimo *	nt_next;
};

/*
** MPNET Note: net_timo_free_list, net_timo_sched_queue, and
** net_timo_ready_queue are global data structures, and either you or
** your caller must have exclusive access to them (via spinlocking
** ntimo_lock).  Protection is NOT explicit in the timer-manipulation
** routines.
*/

STATIC struct ntimo net_timo_free_list;
STATIC struct ntimo net_timo_sched_queue;
STATIC struct ntimo net_timo_ready_queue;

#ifdef	__hp9000s300
#define	spl_ntimo	spl6
#endif	/* __hp9000s300 */

#ifdef	__hp9000s800
#define	spl_ntimo	splimp
#endif	/* __hp9000s800 */

#ifdef __hp9000s300
struct sw_intloc net_intloc;
#endif

#ifdef NET_QA				/* for devel purposes */
struct netisr_stats netisr_stats;	/* defined in netisr.h */
#define	NETISR_STATS(field)		netisr_stats.field++
#define NET_ASSERT(cond, string)	if(!(cond)) panic((string))
#else
#define	NETISR_STATS(field)		/* nothing */
#define	NET_ASSERT(cond, string)	/* nothing */
#endif

/*
** MPNET:  No MP protection is provided to this routine, since it is
** only called during boot-time initialization, when only one processor
** is running anyway.
*/
netisr_init()
{
	int oldlevel;

	/*
 	 * Set/save processor priority level.
 	 */
 
 	oldlevel = spl_ntimo();
 
 	/*
	 * Guard against multiple calls.  Convention states that netisr
	 * should be initialized only once, at the time multitasking is
	 * started in main()
	 */

	NET_ASSERT(!netisr_initialized, "netisr: initialized more than once!");

	/*
	 * Initialize Interrupt Vector, and set MP protection model.
	 */

	if (netisr_priority == -1) {
#ifdef	__hp9000s800
		eirrswitch_assign(eirr_switch, SOFTNET_INT_BIT, netisr,
			SPLNET);
#endif 

#ifdef	__hp9000s300
		net_intloc.link = 0;
		net_intloc.proc = 0;
		net_intloc.arg = 0;
		net_intloc.priority = 2;
		net_intloc.sub_priority = 0;
#endif
	    /*choose MP protection model: TopHalf+ICS (THICS). */
	    netmp_choose_pmodel(NETMP_THICS);

	} else {
	    /* Choose MP protection model: TopHalf Only (THO). */
	    netmp_choose_pmodel(NETMP_THO);
	}

#ifdef MP
	initsync(&netisr_sync);
#endif /* MP */
	netmp_pmodelischosen(); /* Protection model now chosen. */

#ifdef NET_QA
	netisr_stats.events_sched = 0;
	netisr_stats.netisr_called = 0;
	netisr_stats.net_timeouts = 0;
	netisr_stats.net_untimeouts = 0;
	netisr_stats.net_timo_expired = 0;
	netisr_stats.net_callouts = 0;
#endif

	/*
	 * Get netisr ready to run
	 */
	
	netisr_initialized = TRUE;

	/*
	 * Reset the processor priority level.
	 */

	(void) splx(oldlevel);
}

netisr_daemon()
{
	int i = 0, oldlevel;
	char *process_name = "netisr";
	int rtpri;
	struct proc *pp, *allocproc();
	extern void pstat_cmd();

 	NET_ASSERT(!netisr_initialized, 
 		"netisr_daemon: netisr has not been initialized");
	if (netisr_priority == -1) {
		return;
	}

	NET_ASSERT(!netisr_daemon_running, 
		"netisr_daemon: netisr already running");

	if ((rtpri = netisr_priority) < 0 || rtpri > 127) {
		rtpri = NETISR_RTPRIO;
		printf("netisr real-time priority reset to %d\n", rtpri);
	}

	pp = allocproc(S_DONTCARE);
	proc_hash(pp, 0, PID_NETISR, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_DAEMON, pp)) {
		u.u_procp->p_rtpri = u.u_procp->p_pri = rtpri;
		u.u_procp->p_flag |= SRTPROC;

		/*
		 * Give a limited csp its name for pstat.
		 */
		pstat_cmd(u.u_procp, process_name, 1, process_name);
#ifdef	FSD_KI
		u.u_syscall = KI_NETISR;
#endif	/* FSD_KI */

		/*
		 * In order for schednetisr() to wake us up, it needs a pointer
		 * to the proc structure for the netisr daemon process.
		 * This is faster than a wakeup.
		 */

		netisr_procp = u.u_procp;

		netisr_daemon_running = TRUE;

		/*
		 * Loop forever.  While nothing to do, sleep.  When one or more
		 * events are enqueued, process them, and loop until we
		 * get dizzy...
		 */

		for (;;) {
		 	/* Protect ourselves against changes to netirr
			** while we're testing it.  We must protect
			** for MP as well as UP cases, 800 & 300 cases.
			*/
 			oldlevel = sleep_lock();
			if (netirr == 0) {
#ifdef MP
			    psync(&netisr_sync);
#else	/* !MP */
 			    /* Note: after the next statement: 
 			    **   MP:	the sleep_lock is released 
 			    **	 UP:	the spl level is restored
			    */
 			    sleep_then_unlock((caddr_t) &netirr, 
 				PZERO - 1, oldlevel);
#endif /* !MP */
			} else {
 			    sleep_unlock(oldlevel);
			}
			netisr();
		}
	}
}

/*
** Any callers who have work for netisr post that fact by calling
** schednetisr.  When netisr runs off the ICS, this results in a software
** interrupt being set in the interrupt mask register.  Netisr and the
** routines that it calls to handle this work are very sensitive to re-
** entrancy, and so netisr guards against this condition by elevating the
** spl level to splnet.  If there happens to be a bug in any software 
** anywhere in the system which sets an spl level **lower** than splnet,
** we can have a reentrancy problem.  These bugs exist elsewhere in the
** system, but they can cause crashes with netisr.  It is a simple matter
** to check to see if netisr has been re-entered, and if so, simply to
** return.  This doesn't help to "catch" the offending software, but it
** does help the customer by avoiding crashes.  We use the variable
** netisr_called_flag for this purpose.
*/

STATIC int netisr_called_flag = 0; /* Protect against bozo spl levels */
 
netisr()
{
 	register int *lock = &netirr;
  	register int saved_netirr;
  	int oldlevel;
	sv_sema_t netmp_savestate; /* MPNET: Local save state */
	
	NETISR_STATS(netisr_called);

	if (++netisr_called_flag != 1)
	    return; /*Protect ourselves against bozo spl levels.*/

 	NETISR_MP_GO_EXCLUSIVE(oldlevel, netmp_savestate); /* MPNET */
	while (TRUE) {			/* process events */

		/* Protect ourselves against changes to netirr while we
		** are moving it into saved_netirr, and then clearing it.
  		** For S800's, we can use the load-and-clear-word
  		** instruction.  For s300, we use spl's.
		** If any driver calls schednetisr() after this point,
		** we will process that work, too, and race conditions
		** are eliminated.
		*/
#ifdef  __hp9000s800
                _LDCWS(0, 0, lock, saved_netirr);
#endif  /* __hp9000s800 */
#ifdef  __hp9000s300
		oldlevel = sleep_lock();
		saved_netirr = netirr;
		netirr = 0;
		sleep_unlock(oldlevel);

#endif /*  __hp9000s300 */

		/* Now we have exclusive access to n/w data structs.
		** Exception:  data structures shared by top-half code and
		** driver interrupt service routines.  Potential race condi-
		** tions in accessing these data must still be prevented, by
		** going to the same interrupt level as the driver, e.g.,
		** splimp()).
		**/

		if (saved_netirr & (1 << NETISR_RAW))
			NETCALL(NET_RAWINTR)();
		if (saved_netirr & (1 << NETISR_IP))
			NETCALL(NET_IPINTR)();
#if 0 /* obsolete! */
		if (saved_netirr & (1 << NETISR_NS))
			NETCALL(NET_NSINTR)();
                if (saved_netirr & (1 << NETISR_DDP))
                        NETCALL(NET_DDPINTR)();
#endif
		if (saved_netirr & (1 << NETISR_ARP))
			NETCALL(NET_ARP)();
                if (saved_netirr & (1 << NETISR_MAP))
                        NETCALL(NET_MAPINTR)();
		if (saved_netirr & (1 << NETISR_PROBE))
			NETCALL(NET_PROBE)();
                if (saved_netirr & (1 << NETISR_X25))
                        NETCALL(NET_X25INTR)();
#ifdef	__hp9000s800
                if (saved_netirr & (1 << NETISR_DUX))
                        NETCALL(NET_DUXINTR)();
#endif
		if (saved_netirr & (1 << NETISR_NIT))
			NETCALL(NET_NITINTR)();
		if (saved_netirr & (1 << NETISR_ISDN))
			NETCALL(NET_ISDNINTR)();

                /* always last */
		if (saved_netirr & (1 << NETISR_NET_TIMEOUT)) 
			net_callout();

		if (netirr == 0) break;
	}
 	NETISR_MP_GO_UNEXCLUSIVE(oldlevel, netmp_savestate); /* MPNET */
 	/* We no longer have exclusive access to n/w data structs */
	netisr_called_flag = 0;
	return;
}

schednetisr(anisr)
int 	anisr;
{
	int oldlevel;

	NETISR_STATS(events_sched);

	/* 
	 * Set the relevant bit so netisr picks up the right event 
	 */

 	oldlevel = sleep_lock();
	netirr |= 1<<anisr;

	/* 
	 * let netisr know work's to be done 
	 */
	if (netisr_priority == -1) {
#ifdef __hp9000s800
		setsoftnet();		
#endif /*__hp9000s800 */
#ifdef __hp9000s300
		sw_trigger(&net_intloc, netisr, 0, 2, 0);
#endif /*__hp9000s300 */
	} else
	{
#ifdef MP
	    cvsync(&netisr_sync);
#else
	    if (netisr_procp && netisr_procp->p_stat != SRUN)
		    setrun(netisr_procp);
#endif /* MP */

	}
 	sleep_unlock(oldlevel);
}

/*
** MPNET: No need to provide MP protection here.  This routine is
** only called at boot-time initialization.  Only one processor is
** running then.  Another MPNET change: deleted the optimization wherein
** if netisr_priority == -1, then net_timeout() called timeout() directly,
** and this included bypassing the buildup of the timeout list in that
** case.  Now, we do this the same way in either case.  The change was
** necessary in order to avoid having to solve the problem of a timer-called
** routine needing to lock the network semaphore off of the ICS, i.e., the
** need to solve the same problem as with netisr-on-the-ics.
*/
ntimo_init()
{
	int n;
	struct ntimo *ntimo, *prev, *next;
	int npg = ncallout * sizeof(struct ntimo);

 	NET_ASSERT(!net_timo_initialized,
		"ntimo_init: initialized more than once!");
	/*
  	 * Allocate memory for the network timeout free list.
	 */

	/*
	 *	Changed MALLOC call to kmalloc to save space. When
	 *	MALLOC is called with a variable size, the text is
	 *	large. When size is a constant, text is smaller due to
	 *	optimization by the compiler. (RPC, 11/11/93)
	 */
	ntimo = (struct ntimo *) kmalloc(npg, M_NTIMO, M_NOWAIT);
	if (ntimo == 0)
		panic("init_ntimo: no ntimo memory");

	/*
	 * Link the free list together
	 */
 	prev = ntimo;
 	next = ntimo + 1;

	for (n = 0; n < (ncallout - 1); n++) {
		prev->nt_next = next;
		prev = next;
		next++;
	}

	prev->nt_next = (struct ntimo *) 0;

	/*
 	 * Initialize the network timeout queues.
	 */

	net_timo_free_list.nt_next = (struct ntimo *) ntimo;

	net_timo_sched_queue.nt_prev = &net_timo_sched_queue;
	net_timo_sched_queue.nt_next = &net_timo_sched_queue;

	net_timo_ready_queue.nt_prev = &net_timo_ready_queue;
	net_timo_ready_queue.nt_next = &net_timo_ready_queue;

	net_timo_initialized = TRUE;
}
/*
** MPNET: this routine is not callable outside of netisr because it
** doesn't provide explicit MP protection itself.  Its callers do.
** It would be preferable to declare it "static", except that then
** adb thinks that instructions in this routine are in the one
** above it, which is confusing when we have to look at a stack
** trace.  So we'll use comments to say all this, hoping people
** will read them.
*/
/* SHOULD BE STATIC -- DO NOT CALL FROM OUTSIDE NETISR.C */

net_enqueue_timo(prev, next)
	struct ntimo *prev, *next;
{
	/*
	 * Q.E.D.
	 */

	prev->nt_prev = next->nt_prev;
	prev->nt_next = next;
	prev->nt_prev->nt_next = prev;
	next->nt_prev = prev;

}

/*
** MPNET: this routine is not callable outside of netisr because it
** doesn't provide explicit MP protection itself.  Its callers do.
** It would be preferable to declare it "static", except that then
** adb thinks that instructions in this routine are in the one
** above it, which is confusing when we have to look at a stack
** trace.  So we'll use comments to say all this, hoping people
** will read them.
*/
/* SHOULD BE STATIC -- DO NOT CALL FROM OUTSIDE NETISR.C */

net_dequeue_timo(ntimo)
	struct ntimo *ntimo;
{
	/*
	 * Q.E.D.
	 */

	ntimo->nt_prev->nt_next = ntimo->nt_next;
	ntimo->nt_next->nt_prev = ntimo->nt_prev;
/* 
 * This is needed for MP protection. The timeout lists are protected 
 * by the proper lock, however there are references to these list from 
 * outside (when net_ready_timo is called by timeout).
 * Setting nt_prev to -1 (INVALID) will cause net_ready_timo routine 
 * to ignore the  request.   DTS#INDaa12269  
*/
  	ntimo->nt_prev = -1;

}

/*
** MPNET: this routine is not callable outside of netisr because it
** doesn't provide explicit MP protection itself.  Its callers do.
** It would be preferable to declare it "static", except that then
** adb thinks that instructions in this routine are in the one
** above it, which is confusing when we have to look at a stack
** trace.  So we'll use comments to say all this, hoping people
** will read them.
*/
/* SHOULD BE STATIC -- DO NOT CALL FROM OUTSIDE NETISR.C */

struct ntimo *
net_alloc_timo()
{
	struct ntimo *ntimo;

 	NET_ASSERT(!net_timo_initialized, 
 		"netisr_daemon: netisr timers have not been initialized");
	/*
	 * Q.E.D.
	 */

	ntimo = net_timo_free_list.nt_next;

	if (ntimo == (struct ntimo *) 0)
		panic("net_alloc_timo: empty net_timo_free_list");

	net_timo_free_list.nt_next = ntimo->nt_next;

	return(ntimo);
}

/*
** MPNET: this routine is not callable outside of netisr because it
** doesn't provide explicit MP protection itself.  Its callers do.
** It would be preferable to declare it "static", except that then
** adb thinks that instructions in this routine are in the one
** above it, which is confusing when we have to look at a stack
** trace.  So we'll use comments to say all this, hoping people
** will read them.
*/
/* SHOULD BE STATIC -- DO NOT CALL FROM OUTSIDE NETISR.C */

net_free_timo(ntimo)
	struct ntimo *ntimo;
{
	/*
	 * Q.E.D.
	 */

	ntimo->nt_next = net_timo_free_list.nt_next;
	net_timo_free_list.nt_next = ntimo;
}

/*
** MPNET: This routine derives its own MP protection from its caller,
** but it uses ntimo_lock to protect the timer queues from ISR activity.
*/

/* SHOULD BE STATIC -- DO NOT CALL FROM OUTSIDE NETISR.C */
net_callout()
{
	int oldlevel, (*func)();
	caddr_t arg;
	struct ntimo *ntimo;

	/*
	 * Loop through the network timeout ready list.
	 */
	while (TRUE) {

		NETISR_STATS(net_callouts);
		SPL_REPLACEMENT(ntimo_lock, spl_ntimo, oldlevel);
		/*
		 * Dequeue the next element from the network timeout
		 * ready queue.  If it is empty, then there is no more
		 * work to do.
		 */

		ntimo = net_timo_ready_queue.nt_next;

		if (ntimo == &net_timo_ready_queue) {
			SPLX_REPLACEMENT(ntimo_lock, oldlevel);
			break;
		}

		/*
		 * Copy the function pointer and argument values from
		 * the ntimo structure to local variables.  There is a
		 * potential problem here, (do you see it coming)?  When
		 * we drop to splnet() a device driver could interrupt
		 * us and net_untimeout() the routine we have in "func".
		 * In that case, the ntimo struct is no longer attached
		 * to either the sched queue or the ready queue.  (Now,
		 * you have something to think about, eh?)
		 */

		func = ntimo->nt_func;
		arg = ntimo->nt_arg;

		/*
		 * Dequeue ntimo from the ready queue.
		 */

		net_dequeue_timo(ntimo);

		/*
		 * Replace the ntimo struct into the free list.
		 */

		net_free_timo(ntimo);
		SPLX_REPLACEMENT(ntimo_lock, oldlevel);

		/*
		 * Call (*func)(arg) with MP exclusion protection.
		 */

		(*func)(arg);
	}
}

net_ready_timo(ntimo)
	struct ntimo *ntimo;
{
	int oldlevel;

	NETISR_STATS(net_timo_expired);

	/*
	 * Save the processor priority level and go to splimp().
	 */

	SPL_REPLACEMENT(ntimo_lock, spl_ntimo, oldlevel);

	/*
	 * Dequeue an ntimo struct from the sched queue, and re-attach
	 * in to the ready queue.  We assume that the pointer passed to
	 * us is a valid ntimo struct pointer.  If not, hell isn't far
	 * away... 
	 */

        if (ntimo->nt_prev == -1) {
                SPLX_REPLACEMENT(ntimo_lock, oldlevel);
                return;
        }

	net_dequeue_timo(ntimo);
	net_enqueue_timo(ntimo, &net_timo_ready_queue);


	/*
	 * Restore the processor priority level.
	 */
	SPLX_REPLACEMENT(ntimo_lock, oldlevel);
	/*
	 * Schedule a network timeout event.
	 */
	schednetisr(NETISR_NET_TIMEOUT);
}

/*
** MPNET: this procedure provides its own MP protection (ntimo_lock).
*/
net_timeout(func, arg, t)
	int (*func)();
	caddr_t arg;
	int t;
{
	int oldlevel;
	struct ntimo *free, *ntimo;

	NET_ASSERT(net_timo_initialized, 
		"net_timeout: net_timo not initialized!");

	NETISR_STATS(net_timeouts);

	/*
	 * Raise the processor priority level to block interface and
	 * timeout-generated events.
	 */

	SPL_REPLACEMENT(ntimo_lock, spl_ntimo, oldlevel);

	/*
	 * Allocate an ntimo struct from the free list.
	 */

	free = net_alloc_timo();

	/*
	 * Save the function pointer and argument in the ntimo struct.
	 */

	free->nt_func = func;
	free->nt_arg = arg;

	if (t) {
		/*
	 	* Loop through the sched queue, searching for the appropriate
	 	* place to insert the ntimo struct.  The queue itself is sorted
	 	* by argument value.
	 	*/

		ntimo = net_timo_sched_queue.nt_next;

		while (ntimo != &net_timo_sched_queue) {
			if (arg <= ntimo->nt_arg)
				break;

			ntimo = ntimo->nt_next;
		}

		/*
	 	* Insert it into the queue.
	 	*/
	
		net_enqueue_timo(free, ntimo);

		/*
	 	* Finally, call timeout for real.
	 	*/

		timeout(net_ready_timo, free, t);
		SPLX_REPLACEMENT(ntimo_lock, oldlevel);

	} else {
		/*
		 * If timeout value is zero, don't schedule it.  Simply put it
		 * into the ready queue and call schednetisr.
		 */
		net_enqueue_timo(free, &net_timo_ready_queue);

		SPLX_REPLACEMENT(ntimo_lock, oldlevel);/* MPNET: unlock now
						** to avoid any possibility
						** of a deadlock situation.
						*/
		/*
		 * Schedule a network timeout event.
		 */

		schednetisr(NETISR_NET_TIMEOUT);
	}
	/*
	 * We already restored the processor priority level.
	 */
}

void
net_untimeout(func, arg)
	int (*func)();
	caddr_t arg;
{
	int oldlevel;
	struct ntimo *ntimo;


	NET_ASSERT(net_timo_initialized,
		"net_untimeout: net_timeo not initialized!");

	NETISR_STATS(net_untimeouts);

	/*
	 * UPNET: Raise the processor priority level to block interface and
	 * timeout-generated events.
	 * MPNET: spin-lock ntimo_lock, to achieve same purpose.
	 */

	SPL_REPLACEMENT(ntimo_lock, spl_ntimo, oldlevel);

	/*
	 * Search for the ntimo struct that contains func/arg in the
	 * network timeout scheduled queue.
	 */

	ntimo = net_timo_sched_queue.nt_next;

	while (ntimo != &net_timo_sched_queue) {
		if (arg < ntimo->nt_arg)
			break;

		if (arg == ntimo->nt_arg && func == ntimo->nt_func)
			goto dequeue_ntimo;

		ntimo = ntimo->nt_next;
	}

	/*
	 * Search for the ntimo struct that contains func/arg in the
	 * network timeout ready queue.  (Just in time!)
	 */

	ntimo = net_timo_ready_queue.nt_next;

	while (ntimo != &net_timo_ready_queue) {
		if (arg == ntimo->nt_arg && func == ntimo->nt_func)
			goto dequeue_ntimo;

		ntimo = ntimo->nt_next;
	}

	/*
	 * Oh well, we tried, just give up.
	 */
	SPLX_REPLACEMENT(ntimo_lock, oldlevel); /* MPNET */
	return;

dequeue_ntimo:

	/*
	 * Dequeue the ntimo struct from the appropriate queue.
	 */

	net_dequeue_timo(ntimo);
	/*
	 * Replace the ntimo struct onto the free list.
	 */

	net_free_timo(ntimo);

	/*
	 * Having found the ntimo struct for the func/arg, call
	 * untimeout() to unschedule the timeout. 
	 * MPNET note: we still hold the ntimo_lock;  untimeout permits
	 * us to be called with this spin-lock held.
	 */

	untimeout(net_ready_timo, ntimo);
	/*
	 * Restore the processor priority level.
	 */
	SPLX_REPLACEMENT(ntimo_lock, oldlevel); /* MPNET */
	return;
}
