/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/netmp.c,v $
 * $Revision: 1.4.83.5 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 11:23:55 $
 */

/*
 * $Header: netmp.c,v 1.4.83.5 93/10/20 11:23:55 rpc Exp $
 */
#if	defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) $Header: netmp.c,v 1.4.83.5 93/10/20 11:23:55 rpc Exp $";
#endif


/*****************************************************************
/* **** (from the IMS ***********************************************
/* 
/* LANLINK/MP Protection Models
/* 
/* There are two protection models to be implemented.  One (called THO)
/* protects only against race conditions among different processes; it
/* explicitly does not protect against race conditions on the ICS.  This
/* model is used when there can be no interactions from the interrupt
/* level, e.g., when netisr is running as a process.  In THO, the drivers
/* can still interrupt the system, but the only structures they can affect
/* are protected by splimp (== spl5).  When processes (other than netisr)
/* access these structures, they also use splimp protection.  The path
/* lengths for these are quite short, a few hundred instructions at most,
/* and thus the impact upon system thruput is expected to be negligible.
/* 
/* The other protection model (THICS) protects everything the
/* "top-half-only" model protects, and also against interactions from the
/* ICS.  It is intended for use when netisr is running on the ICS.  It may
/* also be used if there are other software modules in the system which
/* have similar characteristics, e.g., Falcon.
/* 
/* THICS is the more restrictive model.  Anything that can run under THO
/* would still run under THICS, albeit system performance suffers.
/* 
/* By locking the network semaphore first, and in both models, we provide
/* slightly better parallelism.  If a second processor should happen to
/* execute any of the system-calls dealing with sockets while another has
/* the sema locked, then the second processor can be put to other use.  If
/* in THICS we didn't lock the net-sema first, then the second processor
/* would simply spinlock, waiting for the spl_lock, and it couldn't get any
/* useful work done.
/* 
/* We could use NETCALLs for choosing which set of protection model procedures
/* to call, but the machine-code for doing so invokes $dyncall, and is
/* approximately 25% higher in overhead than in-line "if" statements, i.e.,
/*
/* if (protection_model == THO)
/*	protection_model_tho();
/* else	/* protection model must be THICS */
/*	protection_model_thics();
/*
/* is the faster way to do it.  However, rather than "clutter up" every-
/* body's code with this, we define macroes to do it for us (see netmp.h).
/*
/* If netisr is running as a process, the protection model will be THO
/* (but can be bumped up to THICS by external subsystems).  If netisr is
/* running on the ICS, the model will be THICS.
/* 
/* For THO, the macro "NETMP_GO_EXCLUSIVE" will vector to a routine which
/* locks the network semaphore (but does not call splnet).  The macro
/* "NETMP_GO_UNEXCLUSIVE" will vector to a routine which will unlock the
/* network semaphore, but not call splx().
/* 
/* For THICS, the macro NETMP_GO_EXCLUSIVE will invoke a routine which
/* will lock the network semaphore, and then call splnet().  The macro
/* NETMP_GO_UNEXCLUSIVE will invoke a routine which will call splx() and
/* then unlock network semaphore.
/* 
/* For all of these, the previous state of the network semaphore and/or
/* interrupt level will be saved/retrieved in arguments to the routine.
/* 
/* The netisr() routine will access a third pair of macroes.  Only netisr()
/* will access them.  Netisr needs its own routines, because for THICS,
/* its protection sequence is different.  The macroes are:
/* NETISR_MP_GO_EXCLUSIVE and NETISR_MP_GO_UNEXCLUSIVE.
/*
/* The S300 does not support MP, so some code will be no-ops
/* (as noted below).
/* 
/* Protection	NETISR_MP_GO_EXCLUSIVE	NETISR_MP_GO_UNEXCLUSIVE
/* Model
/* -----------
/* THO		lock network semaphore	unlock network semaphore
/* THICS		splnet			splx
/* 
/* 
/* Notice that even if netisr is running as a process, if the chosen
/* protection model is THICS, then it will go to splnet during its main
/* loop, restoring interrupt mask to the original level only at the very
/* bottom of the main loop, after exhausting all the work it has to do.  It
/* won't lock the network semaphore, though, if the model is THICS, even if
/* it is running as a process.  This means that, while netisr is executing,
/* access collisions will result in spin-waits (a bad thing), rather than
/* process-sleeps (a good thing).
/* 
/* One of the implications of all this is how important it will be for some
/* performance measurements to be taken.  It will be necessary for HP's
/* field people, not to mention customers, to be able to determine what the
/* tradeoffs are.  Measurements should be taken with netisr as a process
/* and using THO, as a process using THICS, and running off the ICS, using
/* THICS (THO isn't possible, in this case).  These measurements should
/* cover not only network thruput and delay, but also various measures of
/* system thruput.
/* 
**********************************************************************/

#include "../h/kern_sem.h"
#include "../h/netfunc.h"
#include "../net/netmp.h"

int netmp_go_excl_thics();
void netmp_go_unexcl_thics();
void netmp_go_excl_tho();
void netmp_go_unexcl_tho();
int netisr_go_excl_thics();
void netisr_go_unexcl_thics();
void netmp_bad_call();

int netmp_current_pmodel = 0;
static int netmp_pmodel_chosen  = 0;

/*
** The MP protection model is now set.
** Nobody else will be allowed to change it.
*/
netmp_pmodelischosen()
{
    netmp_pmodel_chosen = 1;
    if ((netmp_current_pmodel != NETMP_THO) &&
	(netmp_current_pmodel != NETMP_THICS)){
	MP_ASSERT(0,"netmp_pmodelischosen: illegal parameter");
	printf("netmp_pmodelischosen: defaulting to NETMP_THO\n");
	netmp_current_pmodel = NETMP_THO;/* Set default protection model*/
	return(-1);
    }
    return(netmp_current_pmodel);
}
/*
** Caller wants to set the MP protection model.
** If the requested model is less restrictive than the one we're
** at, return zero (leaving the model unchanged).  Otherwise, if the
** the drop-dead point has already been reached, then return
** an error (with model unchanged).
** Otherwise, set the model, and return zero.
*/
netmp_choose_pmodel(pmodel)
int pmodel;
{
    if (pmodel <= netmp_current_pmodel) return(netmp_current_pmodel);
    if (netmp_pmodel_chosen) 
    /* Error! More restrictive protection model, but too late! */
        return(netmp_current_pmodel);

    /*
    ** Set the protection model, according to the request.
    */
    switch(pmodel){
    case NETMP_THO:	/* Top-half only protection */
        netmp_current_pmodel = pmodel;
	break;
    case NETMP_THICS: /* Top-half + ICS protection */
        netmp_current_pmodel = pmodel;
	break;
    default:
	printf("netmp:illegal protection model parameter\n");
	MP_ASSERT(0,"netmp_choose_pmodel: illegal parameter");
	return(-1);
    }
    return(netmp_current_pmodel);
}

#ifdef MP
void
netmp_go_excl_tho(netmp_savestate)
sv_sema_t *netmp_savestate;	/* MPNET: Local save state */
{
    /* Some QA-only assertions, used only for testing, not compiled
    ** into released code.
    */

#ifdef __hp9000s800
    /* We should never be called on the ICS w/ this model.  */
    MP_ASSERT(! ON_ICS, "netmp_go_excl_tho called from ICS");
#endif /* __hp9000s800 */
    NET_PXSEMA(netmp_savestate);/* MPNET: General Rule VII */
				/* Does nothing in S300. */
    /* We should be holding the network semaphore at this point. */
#ifdef MP
    MP_ASSERT_SEMA(OWN_NET_SEMA,
	"netmp_go_excl_thics: not holding networking sema when should be");
#endif /* MP */
    return;
}
void
netmp_go_unexcl_tho(netmp_savestate)
sv_sema_t *netmp_savestate;	/* MPNET: Local save state */
{
    /* Some QA-only assertions, used only for testing, not compiled 
    ** into released code.
    /* We should never be called on the ICS w/ this model.  */
#ifdef MP
    MP_ASSERT(! ON_ICS, "netmp_go_unexcl_tho called from ICS");
#endif /* MP */
    NET_VXSEMA(netmp_savestate); /* MPNET: Closure for
				   ** General Rule VII.
				   ** Does nothing on S300.
				   */
    /* Cannot test that we don't hold netsema any more -- because of
    ** stuttering.
    */
    return;
}
/*
** NB: The THICS protection model does not make use of netsema, so we
** cannot use the MP_ASSERTs to test when we should and shouldn't be
** holding it.
*/
int
netmp_go_excl_thics(netmp_savestate)
sv_sema_t *netmp_savestate;	/* MPNET: Local save state */
{
    NET_PXSEMA(netmp_savestate);/* MPNET: General Rule VII */
				/* Does nothing in S300. */
    return(splnet());
}
void
netmp_go_unexcl_thics(oldlevel, netmp_savestate)
int oldlevel;
sv_sema_t *netmp_savestate;	/* MPNET: Local save state */
{
    splx(oldlevel);
    NET_VXSEMA(netmp_savestate); /* MPNET: Closure for
				   ** General Rule VII.
				   ** Does nothing on S300.
				   */
    return;
}

int
netisr_go_excl_thics()
{
    return(splnet());
}
void
netisr_go_unexcl_thics(oldlevel)
int oldlevel;
{
    splx(oldlevel);
    return;
}
#endif /* MP */

/*
** The purpose of this routine is only to trap instances where routines
** are used that are undefined for the currently-in-force protection
** model.
*/ 
#ifdef MP
int netmp_badcall_printf = 1;	/* adb this to zero when you're tired of
				/* getting the messages. */
void 
netmp_bad_call()
{
#ifdef NS_QA
    panic(
    "NetMP: use of procedure not allowed in current MP protection model\n");
#else /* !  NS_QA */
    if ( netmp_badcall_printf ) 
    printf(
    "NetMP: use of procedure not allowed in current MP protection model\n");
    (void) splnet();
#endif /* !  NS_QA */
}
#endif /* MP */
