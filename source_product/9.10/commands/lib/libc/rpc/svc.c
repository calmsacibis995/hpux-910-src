/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc.c,v $
 * $Revision: 12.7.1.2 $	$Author: hmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/13 10:49:08 $
 */

/* BEGIN_IMS_MOD  
******************************************************************************
****								
**** svc.c - rpc server routines
****								
******************************************************************************
* Description
*	This module contains somewhat generic server side routines.
*	These routine fall into two catagories. The routines to handle
*	transport handles, and the routines to get and send server
*	requests/replies.
*
* Externally Callable Routines
*
*	xprt-register 	- register a transport handle (user level only)
*	xprt-unregister	- unregister a handle (user level only)	 
*	svc_register	- add a service to the callout list
*	svc_unregister	- remove a service from the callout list
*	svc_find	- find a service in the callout list
*	svc_sendreply	- send a reply to an rpc request
*	svcerr_noproc	- send a "no such procedure error" reply
*	svcerr_decode	- send a "decode error" reply
*	svcerr_systemerr- send a "system error" reply
*	svcerr_auth	- send a "authentication error" reply
*	svcerr_weakauth - send a "weak authentication error" reply
*	svcerr_noprog	- send a "no such program error" reply
*	svcerr_progvers - send a "no such version error" reply
*	svc_getreq	- get a request and call the correct dispatch
*			  routine
*	svc_run		- wait for a request to arrive
*	svc_run_ms	- svc_run where mask size is specified
*
* Global Variables Declared 
*
*	rqcred_head	- list of available credential structures
*	svc_head	- callout list (prog/vers/dispatch triples)
*	xports[]	- array of (*)transport handles (user level only)
*	svc_fds		- bit map of registered sockets (user level only)
*
* Test Module
*	scaf/nfs/*
*
* To Do List
*
* Notes
*	
* Modification History
*	10/28/87	ew	added comments
*	10/28/87	ew	made xprt_register a non-kernel routine
*				rather than a null routine in the kernel.
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc.c,v 12.7.1.2 94/12/13 10:49:08 hmgr Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#ifdef _NAMESPACE_CLEAN
/* 
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define memcpy 			_memcpy
#define memset 			_memset
#define perror 			_perror
#define pmap_set 		_pmap_set
#define pmap_unset 		_pmap_unset
#define select 			_select

#define svc_fdset 		_svc_fdset		/* In this file */

#define svc_getreq 		_svc_getreq		/* In this file */
#define svc_getreqset 		_svc_getreqset		/* In this file */
#define svc_getreqset_ms 	_svc_getreqset_ms	/* In this file */
#define svc_register 		_svc_register		/* In this file */
#define svc_run 		_svc_run		/* In this file */
#define svc_run_ms 		_svc_run_ms		/* In this file */
#define svc_sendreply 		_svc_sendreply		/* In this file */
#define svc_unregister 		_svc_unregister		/* In this file */
#define svcerr_auth 		_svcerr_auth		/* In this file */
#define svcerr_decode 		_svcerr_decode		/* In this file */
#define svcerr_noproc 		_svcerr_noproc		/* In this file */
#define svcerr_noprog 		_svcerr_noprog		/* In this file */
#define svcerr_progvers 	_svcerr_progvers	/* In this file */
#define svcerr_systemerr 	_svcerr_systemerr	/* In this file */
#define svcerr_weakauth 	_svcerr_weakauth	/* In this file */
#define xdr_void 		_xdr_void
#define xprt_register 		_xprt_register		/* In this file */
#define xprt_unregister 	_xprt_unregister	/* In this file */

#endif /* _NAME_SPACE_CLEAN */


#ifdef KERNEL
#include "../h/param.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../netinet/in.h"
#include "../h/types.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/svc_auth.h"

char *kmem_alloc();
caddr_t rqcred_head;  /* head of cashed, free authentication parameters */
/*
 * HPNFS
 * added the following for the checking of the return value of
 * sbwait and subsequent recovery, dds, 12/5/86
 * NOTE: also removed the include of time.h, since it is also
 * 	included in user.h
 * HPNFS
 */
#include "../h/user.h"
#include "../h/errno.h"
#else /* not KERNEL */

#define NL_SETN 12	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <rpc/types.h>
#include <sys/errno.h>
#include <time.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <rpc/svc_auth.h>
#include <rpc/pmap_clnt.h>	/* <make kernel depend happy> */
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>

/* Active xprt handle structures:
 * svc_fds - a bit map used to indicate which file desciptors
 *           have interesting/registered socket associated with
 *	     them.
 * xports  - array of xprt handles, each element corresponds to
 *           a bit in svc_fds and therefore to an active socket.
 *
 * It should be noted that the define FD_SETSIZE determines the
 * number of active transport handles we may have.
 *
 * The NAMESPACE defines and initialization of svc_fdset and svc_fds stays
 * in this file since it we expect no one to be using svc_fds unless they
 * are also calling some of the functions in this file.
 */

static SVCXPRT **xports;

#ifdef _NAMESPACE_CLEAN
#undef svc_fdset
#pragma _HP_SECONDARY_DEF _svc_fdset svc_fdset
#define svc_fdset _svc_fdset
#endif /* _NAMESPACE_CLEAN */

/*
 *  THIS IS UGLY - Sun code expects svc_fdset to be initialized to zero
 *    automatically.  This is a bad assumption.  The only place that
 *    we could clear out this structure (using FD_ZERO) is in the
 *    application program.  This is unacceptable because it would break
 *    backward compatability.  So it looks like we have to force it
 *    here.  Sigh.
 *
 *    If FD_SETSIZE is ever changed (to a value other than 2048), then
 *    this initialization will have to change accordingly.
 *
 *    dlr 1/15/90
 */
fd_set svc_fdset =     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

extern errno;
char *malloc();
#endif /* not KERNEL */

/*
 * The services list (aka the callout list).
 * Each entry represents a set of procedures (an rpc "program").
 * The dispatch routine takes request structs and runs the
 * apropriate procedure.
 */
static struct svc_callout {
	struct svc_callout *sc_next;
	u_long		    sc_prog;
	u_long		    sc_vers;
	void		    (*sc_dispatch)();
} *svc_head;

static struct svc_callout *svc_find();

#define NULL_SVC ((struct svc_callout *)0)
#define	RQCRED_SIZE	400

#ifndef KERNEL

/* BEGIN_IMS xprt_register *
 ********************************************************************
 ****
 ****	void
 ****	xprt_register(xprt)
 ****		SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		xprt handle to register
 *	
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	xports		array of xprt handles for this process
 *	svc_fds		bit map of active (registered) sockets
 *
 * Description
 *	This routine is called only be user level rpc servers. It
 *	will add a new xprt handle to the array of active handles
 *	and set the corresponding bit in the bit map "svc_fds".
 *	This map is then used in svc_run to select the sockets
 *	upon which we will wait for requests.
 *
 * Algorithm
 *	get socket number out of xprt handle
 *	if sock is "selectable" (i.e. socket < FD_SETSIZE)
 *		add the xprt handle to "xports"
 *		set the corresponding bit in "svc_fds"
 *
 * Notes
 *	This is a user only routine.
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 ********************************************************************
 * END_IMS xprt_register */

#ifdef _NAMESPACE_CLEAN
#undef xprt_register
#pragma _HP_SECONDARY_DEF _xprt_register xprt_register
#define xprt_register _xprt_register
#endif

void
xprt_register(xprt)
	SVCXPRT *xprt;
{
	register int sock = xprt->xp_sock;

	if (xports == NULL) {
		xports = (SVCXPRT **)
			mem_alloc(FD_SETSIZE * sizeof(SVCXPRT *));
	}
	if (sock < FD_SETSIZE) {
		xports[sock] = xprt;
		FD_SET(sock, &svc_fdset);
	}
}

/* BEGIN_IMS xprt_unregister *
 ********************************************************************
 ****
 ****	void
 ****	xprt_unregister(xprt) 
 ****		SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- handle to unregister
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	xports		- array of active transport handles
 *	svc_fds		- bit map of active sockets	
 *
 * Description
 *	This routine removes the specified transport handle from the
 *	array of active handles and turns off the corresponding bit
 *	in svc_fds, the bit mask used for the select call in svc_run.
 *	This essentially disables the corresponding socket.
 *
 * Algorithm
 *	get socket number out of xprt handle
 *	if sock is within range (i.e. sock < FD_SETSIZE)
 *		clear corresponding entry in xports aray
 *		clear corresponding bit in svc_fds
 *
 * Notes
 *	This is a user only routine.
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 ********************************************************************
 * END_IMS xprt_unregister */

#ifdef _NAMESPACE_CLEAN
#undef xprt_unregister
#pragma _HP_SECONDARY_DEF _xprt_unregister xprt_unregister
#define xprt_unregister _xprt_unregister
#endif

void
xprt_unregister(xprt) 
	SVCXPRT *xprt;
{ 
	register int sock = xprt->xp_sock;

	if ((sock < FD_SETSIZE) && (xports[sock] == xprt)) {
		xports[sock] = (SVCXPRT *)0;
		FD_CLR(sock, &svc_fdset);
	}
} 
#endif /* not KERNEL */


/* BEGIN_IMS svc_register *
 ********************************************************************
 ****
 ****	bool_t
 ****	svc_register(xprt, prog, vers, dispatch, protocol)
 ****		SVCXPRT *xprt;
 ****		u_long prog;
 ****		u_long vers;
 ****		void (*dispatch)();
 ****		int protocol;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle so we can register the socket
 *			  with the port mapper (user space only)
 *	prog		- program number to add to the list
 *	vers		- version number of the prog
 *	disp		- address of the dispatch routine for this prog
 *	protocol	- protocol for this prog (for protmapper)
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	1 (TRUE)	- call ok
 *	0 (FALSE)	- call failed
 *
 * Globals Referenced
 *	svc_head	- head of the callout list
 *
 * Description
 *	This routine is called to add a service to the callout list.
 *	The list maps a program/version to a dispatch routine.
 *	For user processes, the service is also registered with the
 *	portmapper.
 *
 * Algorithm
 *	if program is already in the list
 *		just register the new socket with the protmapper
 *	else	
 *		add the new service to the callout list
 *		register the service/socket with the protmaper
 *
 * To Do List
 *	Why do we only check for memory allocation errors in user space?
 *
 * Notes
 *	While this routine will allow you to attempt to 
 * 	register a single program/version pair with two different
 *	ports, the portmapper will flag this as an error.
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	mem_alloc	- allocate a new callout structure
 *	svc_find	- see if the service is already in the list
 *	pmap_set	- register the service/port with the protmapper
 *
 ********************************************************************
 * END_IMS svc_register */

#ifdef _NAMESPACE_CLEAN
#undef svc_register
#pragma _HP_SECONDARY_DEF _svc_register svc_register
#define svc_register _svc_register
#endif

bool_t
svc_register(xprt, prog, vers, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)();
	int protocol;
{
	struct svc_callout *prev;
	register struct svc_callout *s;

	if ((s = svc_find(prog, vers, &prev)) != NULL_SVC) {
		if (s->sc_dispatch == dispatch)
			goto pmap_it;  /* he is registering another xptr */
		return (FALSE);
	}
	s = (struct svc_callout *)mem_alloc(sizeof(struct svc_callout));
#ifndef KERNEL
	if (s == (struct svc_callout *)0) {
		return (FALSE);
	}
#endif
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_next = svc_head;
	svc_head = s;
pmap_it:
#ifndef KERNEL
	/*
	 * Now register the information with the local binder service.
	 * For nfs, this is done once by the user space daemon code
	 * (nfsd.c) and therefore is not required here.
	 */
	if (protocol) {
		return (pmap_set(prog, vers, protocol, xprt->xp_port));
	}
#endif
	return (TRUE);
}


/* BEGIN_IMS svc_unregister *
 ********************************************************************
 ****
 ****	void
 ****	svc_unregister(prog, vers)
 ****		u_long prog;
 ****		u_long vers;
 ****
 ********************************************************************
 * Input Parameters
 *	prog		- program number to remove
 *	vers		- version number of the prog
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	svc_head	- head of the callout list
 *
 * Description
 *	This routine is called to remove a service from the callout list.
 *	For user processes, a call is also made to unregister the
 *	service with the protmapper.
 *
 * Algorithm
 *	if program is in the callout list
 *		remove the entry
 *		return the callout structure memory
 *		unregister the service with the protmapper
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	mem_free	- free memory used for the callout structure
 *	svc_find	- see if the service is in the list
 *	pmap_unset	- unregister the service with the protmapper
 *
 ********************************************************************
 * END_IMS svc_register */

#ifdef _NAMESPACE_CLEAN
#undef svc_unregister
#pragma _HP_SECONDARY_DEF _svc_unregister svc_unregister
#define svc_unregister _svc_unregister
#endif

void
svc_unregister(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	register struct svc_callout *s;

	if ((s = svc_find(prog, vers, &prev)) == NULL_SVC)
		return;
	if (prev == NULL_SVC) {
		svc_head = s->sc_next;
	} else {
		prev->sc_next = s->sc_next;
	}
	s->sc_next = NULL_SVC;
	mem_free((char *) s, (u_int) sizeof(struct svc_callout));
#ifndef KERNEL
	/* now unregister the information with the local binder service */
	(void)pmap_unset(prog, vers);
#endif
}


/* BEGIN_IMS svc_find *
 ********************************************************************
 ****
 ****	static struct svc_callout *
 ****	svc_find(prog, vers, prev)
 ****		u_long prog;
 ****		u_long vers;
 ****		struct svc_callout **prev;
 ****
 ********************************************************************
 * Input Parameters
 *	prog		- program number of interest
 *	vers		- version number of interest
 *
 * Output Parameters
 *	prev		- pointer to entry before the found entry
 *
 * Return Value
 *	0		- program/version pair not in callout list
 *	scv_callout	- pointer to matching callout structure
 *
 * Globals Referenced
 *	svc_head	- head of callout list
 *
 * Description
 *	This routine will search the callout list for an
 *	entry containing the specified program and version
 *	numbers. If found, it will return a pointer to the
 *	structure.
 *
 * Algorithm
 *	while not found and not end of list
 *		search for program/version pair
 *	if found return pointer to entry and previous entry
 *	else retrun null and pointer to last entry in the list
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	none
 *
 ********************************************************************
 * END_IMS svc_find */

static struct svc_callout *
svc_find(prog, vers, prev)
	u_long prog;
	u_long vers;
	struct svc_callout **prev;
{
	register struct svc_callout *s, *p;

	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if ((s->sc_prog == prog) && (s->sc_vers == vers))
			goto done;
		p = s;
	}
done:
	*prev = p;
	return (s);
}


/* BEGIN_IMS svc_reply *
 ********************************************************************
 ****
 ****	bool_t
 ****	svc_sendreply(xprt, xdr_results, xdr_location)
 ****		register SVCXPRT *xprt;
 ****		xdrproc_t xdr_results;
 ****		caddr_t xdr_location;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *	xdr_results	- xdr routine used to encode the results
 *	xdr_location	- address of the results for this reply
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	bool_t		- true is send ok, false otherwise
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svc_sendreply */

#ifdef _NAMESPACE_CLEAN
#undef svc_sendreply
#pragma _HP_SECONDARY_DEF _svc_sendreply svc_sendreply
#define svc_sendreply _svc_sendreply
#endif

bool_t
svc_sendreply(xprt, xdr_results, xdr_location)
	register SVCXPRT *xprt;
	xdrproc_t xdr_results;
	caddr_t xdr_location;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY;  
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf; 
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;
	return (SVC_REPLY(xprt, &rply)); 
}


/* BEGIN_IMS svcerr_noproc *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_noproc(xprt)
 ****		register SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_noproc */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_noproc
#pragma _HP_SECONDARY_DEF _svcerr_noproc svcerr_noproc
#define svcerr_noproc _svcerr_noproc
#endif

void
svcerr_noproc(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}


/* BEGIN_IMS svcerr_decode *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_decode(xprt)
 ****		register SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_decode */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_decode
#pragma _HP_SECONDARY_DEF _svcerr_decode svcerr_decode
#define svcerr_decode _svcerr_decode
#endif

void
svcerr_decode(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply); 
}


/* BEGIN_IMS svcerr_systemerr *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_systemerr(xprt)
 ****		register SVCXPRT *xprt;
  ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_systemerr */

#ifndef KERNEL

#ifdef _NAMESPACE_CLEAN
#undef svcerr_systemerr
#pragma _HP_SECONDARY_DEF _svcerr_systemerr svcerr_systemerr
#define svcerr_systemerr _svcerr_systemerr
#endif

void
svcerr_systemerr(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = SYSTEM_ERR;
	SVC_REPLY(xprt, &rply); 
}
#endif /* !KERNEL */


/* BEGIN_IMS svcerr_auth *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_auth(xprt, why)
 ****		SVCXPRT *xprt;
 ****		enum auth_stat why;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_auth */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_auth
#pragma _HP_SECONDARY_DEF _svcerr_auth svcerr_auth
#define svcerr_auth _svcerr_auth
#endif

void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
}


/* BEGIN_IMS svcerr_weakauth *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_weakauth(xprt)
 ****		register SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	call svcerr_auth(AUTH_TOOWEAK)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_weakauth */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_weakauth
#pragma _HP_SECONDARY_DEF _svcerr_weakauth svcerr_weakauth
#define svcerr_weakauth _svcerr_weakauth
#endif

void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
{

	svcerr_auth(xprt, AUTH_TOOWEAK);
}


/* BEGIN_IMS svc_noprog *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_noprog(xprt)
 ****		register SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_noprog */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_noprog
#pragma _HP_SECONDARY_DEF _svcerr_noprog svcerr_noprog
#define svcerr_noprog _svcerr_noprog
#endif

void 
svcerr_noprog(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;  

	rply.rm_direction = REPLY;   
	rply.rm_reply.rp_stat = MSG_ACCEPTED;  
	rply.acpted_rply.ar_verf = xprt->xp_verf;  
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}


/* BEGIN_IMS svcerr_progvers *
 ********************************************************************
 ****
 ****	void
 ****	svcerr_progvers(xprt)
 ****		register SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- xprt handle this reply will go out on
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine builds a basic reply structure then jumps
 *	through the xprt ops vector to the send routine for this
 *	transport handle.
 *
 * Algorithm
 *	build error rpc_msg
 *	call send routine (via xp_ops->xp_send)
 *
 * Modification History
 *	10/27/87	ew	added comments
 *
 * External Calls
 *	svckudp_send | ???
 *
 ********************************************************************
 * END_IMS svcerr_progvers */

#ifdef _NAMESPACE_CLEAN
#undef svcerr_progvers
#pragma _HP_SECONDARY_DEF _svcerr_progvers svcerr_progvers
#define svcerr_progvers _svcerr_progvers
#endif

void  
svcerr_progvers(xprt, low_vers, high_vers)
	register SVCXPRT *xprt; 
	u_long low_vers;
	u_long high_vers;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = low_vers;
	rply.acpted_rply.ar_vers.high = high_vers;
	SVC_REPLY(xprt, &rply);
}


/* BEGIN_IMS svc_getreqset *
 ********************************************************************
 ****
 ****	void
 ****	#ifdef KERNEL
 ****	svc_getreqset(xprt)
 ****		register SVCXPRT *xprt;
 ****	#else
 ****	svc_getreqset_ms(readfds, nfds)
 ****		fd_set readfds;
 ****		long	nfds;
 ****
 ****	svc_getreqset(readfds)
 ****		fd_set readfds;
 ****	#endif KERNEL
 ****
 ********************************************************************
 * Input Parameters
 *	xprt (kernel)	 transport handle upon which to receive msg.
 *	readfds (user)	 bit map of readable file descriptors.
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	svc_fds (user only)	- bit map of active rpc sockets
 *	xports (user only)	- array of active transport handles
 *	rqcred (kernel only)	- head of available credential storage list
 *	svc_head		- head of callout list
 *
 * Description
 *	This routine is the central fetch and dispatch for rpc servers.
 *	It will read a request from a socket into "msg", perform
 *	preliminary authentication, and call the correct dispatch
 *	routine for the request.
 *
 * Algorithm
 *	(if kernel) allocate authentication storage
 *	setup pointers to storage for authentication parameters
 *	if (SVC_RECV gets the next message & decodes the args)
 *		if (authenticate fails)
 *			free argument structures
 *			goto done
 *		if (program in callout list)
 *			call dispatch routine (which frees args)
 *			goto done
 *		else
 *			report error(bad prog or version)
 *			free argument structures
 *	else (SVC_RECV)
 * done:
 *		(if kernel) free authentication storage
 * To Do List
 *
 *	We may want to remove the do loop for kernel code. The 
 *	macro SVC_STAT will always return XPRT_IDLE in the kernel.
 *
 * Notes
 * Statement of authentication parameters management:
 * This function owns and manages all authentication parameters, specifically
 * the "raw" parameters (msg.rm_call.cb_cred and msg.rm_call.cb_verf) and
 * the "cooked" credentials (rqst->rq_clntcred).  However, this function
 * does not know the structure of the cooked credentials, so it make the
 * following two assumptions: a) the structure is contiguous (no pointers), and
 * b) the structure size does not exceed RQCRED_SIZE bytes. 
 * In all events, all three parameters are freed upon exit from this routine.
 * The storage is trivially management on the call stack in user land, but
 * is mallocated in kernel land.
 *
 * Modification History
 *	11/05/87	ew	added comments
 *
 * External Calls
 *	mem_alloc (kmem_alloc)	- allocate mem for credentials
 *	SVC_RECV		- receive message from transport
 *	_authenticate		- authenticate the message
 *	svcerr_auth		- report error in authentication
 *	svc_freeagrs		- free allocated argument structures
 *	*s->sc_dispatch		- call handler found in callout list
 *	svcerr_progvers		- return error if prog version not sup.
 *	svcerr_noprog		- return error if prog not found
 *	SVC_STAT		- report status of transport
 *	SVC_DESTORY		- destory a transport handle
 *
 * Called By 
 *	svc_run, svc_getreq
 *
 ********************************************************************
 * END_IMS svc_getreq */

#ifdef KERNEL

#ifdef _NAMESPACE_CLEAN
#undef svc_getreqset
#pragma _HP_SECONDARY_DEF _svc_getreqset svc_getreqset
#define svc_getreqset _svc_getreqset
#endif

void
svc_getreqset(xprt)
	register SVCXPRT *xprt;

#else	/* KERNEL */

#ifdef _NAMESPACE_CLEAN
#undef svc_getreqset_ms
#pragma _HP_SECONDARY_DEF _svc_getreqset_ms svc_getreqset_ms
#define svc_getreqset_ms _svc_getreqset_ms
#endif	/* KERNEL */

void
svc_getreqset_ms(readfds, nfds)
	fd_set *readfds;
	long	nfds;

#endif /* KERNEL */
{

	register enum xprt_stat stat;
	struct rpc_msg msg;
	int prog_found;
	u_long low_vers;
	u_long high_vers;
	struct svc_req r;
#ifndef KERNEL
	register int sock;
	register SVCXPRT *xprt;
	char cred_area[2*MAX_AUTH_BYTES + RQCRED_SIZE];

	nlmsg_fd = _nfs_nls_catopen();

#else /* KERNEL */
	char *cred_area;  /* too big to allocate on call stack */

	/*
	 * Firstly, allocate the authentication parameters' storage
	 */
	if (rqcred_head) {
		cred_area = rqcred_head;
		rqcred_head = *(caddr_t *)rqcred_head;
	} else {
		cred_area = mem_alloc(2*MAX_AUTH_BYTES + RQCRED_SIZE);
	}
#endif /* KERNEL */
	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r.rq_clntcred = &(cred_area[2*MAX_AUTH_BYTES]);

#ifndef KERNEL

        for (sock = 0; sock < nfds; sock++) {
            if (FD_ISSET(sock, readfds) && FD_ISSET(sock, &svc_fdset)) {
		/* sock has input waiting */
		xprt = xports[sock];
#endif /* KERNEL */
		/* now receive msgs from xprtprt (support batch calls) */
		do {
			if (SVC_RECV(xprt, &msg)) {

				/* now find the exported program and call it */
				register struct svc_callout *s;
				enum auth_stat why;

				r.rq_xprt = xprt;
				r.rq_prog = msg.rm_call.cb_prog;
				r.rq_vers = msg.rm_call.cb_vers;
				r.rq_proc = msg.rm_call.cb_proc;
				r.rq_cred = msg.rm_call.cb_cred;

				/* first authenticate the message */
				if ((why= _authenticate(&r, &msg)) != AUTH_OK) {
					svcerr_auth(xprt, why);
					/*
					 * Added call to svc_freeargs() to free
					 * up data structures allocated by
					 * svc_recv().  (e.g. kernel mbufs )
					 * dds 5/1/87
					 */
					(void) svc_freeargs( xprt, xdr_void, 0);
					goto call_done;
				}
				/* now match message with a registered service*/
				prog_found = FALSE;
				low_vers = 0 - 1;
				high_vers = 0;
				for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
					if (s->sc_prog == r.rq_prog) {
						if (s->sc_vers == r.rq_vers) {
							(*s->sc_dispatch)(&r, xprt);
							goto call_done;
						}  /* found correct version */
						prog_found = TRUE;
						if (s->sc_vers < low_vers)
							low_vers = s->sc_vers;
						if (s->sc_vers > high_vers)
							high_vers = s->sc_vers;
					}   /* found correct program */
				}
				/*
				 * if we got here, the program or version
				 * is not served ...
				 */
				if (prog_found)
					svcerr_progvers(xprt,
					low_vers, high_vers);
				else
					 svcerr_noprog(xprt);
				/*
				 * HPNFS
				 * If got here then need to be sure that any 
				 * resources used by the xdr transport are
				 * freed, e.g. kernel mbufs, etc.  However
				 * since we didn't do any decoding of any
				 * arguments, we don't need to free any
			 	 * other stuff, so use xdr_void. Normally
				 * this is done by the dispatch routine, but
				 * since we didn't call any dispatch routine
				 * we need to do it ourselves. dds 12/31/86
				 */
				(void) svc_freeargs( xprt, xdr_void, 0);

				/* Fall through to ... */
			}
		call_done:
			if ((stat = SVC_STAT(xprt)) == XPRT_DIED){
				SVC_DESTROY(xprt);
				break;
			}
		} while (stat == XPRT_MOREREQS);
#ifndef KERNEL
	    }
	}
#else
	/*
	 * free authentication parameters' storage
	 */
	*(caddr_t *)cred_area = rqcred_head;
	rqcred_head = cred_area;
#endif
}

#ifndef KERNEL
#ifdef _NAMESPACE_CLEAN
#undef svc_getreqset
#pragma _HP_SECONDARY_DEF _svc_getreqset svc_getreqset
#define svc_getreqset _svc_getreqset
#endif
void
svc_getreqset(readfds)
	fd_set *readfds;
{
	svc_getreqset_ms(readfds, FD_SETSIZE);
}
#endif


/* BEGIN_IMS svc_getreq *
 ********************************************************************
 ****
 ****	void
 ****	#ifdef KERNEL
 ****	svc_getreq(xprt)
 ****		register SVCXPRT *xprt;
 ****	#else
 ****	svc_getreq(rdfds)
 ****		int rdfds;
 ****	#endif
 ****
 ********************************************************************
 * Input Parameters
 *	xprt (kernel)	 transport handle upon which to receive msg.
 *	rdfds (user)	 bit map of readable file descriptors.
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine used to be the central fetch and dispatch for rpc servers.
 *	It still is, but now all that it does is call svc_getreqset which is
 *	a more capable routine.
 *	It is now here for backwards compatibility.
 *
 * Algorithm
 *	call svc_getreqset to do the work
 * To Do List
 *	We may want to remove the do loop for kernel code. The 
 *	macro SVC_STAT will always return XPRT_IDLE in the kernel.
 *
 * Modification History
 *	11/05/87	ew	added comments
 *	08/01/88	mds	pulled it into two routines
 *
 * External Calls
 *	svc_getreqset           - does the actual reception of a request
 *
 * Called By 
 *	svc_run
 *
 ********************************************************************
 * END_IMS svc_getreq */

#ifdef _NAMESPACE_CLEAN
#undef svc_getreq
#pragma _HP_SECONDARY_DEF _svc_getreq svc_getreq
#define svc_getreq _svc_getreq
#endif

void
#ifdef KERNEL
svc_getreq(xprt)
	register SVCXPRT *xprt;
#else
svc_getreq(rdfds)
	int rdfds;
#endif
{
#ifdef KERNEL
	svc_getreqset(xprt);
#else  /* KERNEL */
        fd_set readfds;

        FD_ZERO(&readfds);
        readfds.fds_bits[0] = rdfds;
        svc_getreqset(&readfds);

#endif /* KERNEL */
}


#ifdef KERNEL
/* BEGIN_IMS svc_run *
 ********************************************************************
 ****
 ****	void
 ****	svc_run(xprt)
 ****		SVCXPRT *xprt;
 ****
 ********************************************************************
 * Input Parameters
 *	xprt		- transport handle
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	Rpccnt		- number of times we have called svc_getreq
 *
 * Description
 *	This is the main server side idle loop. We will sleep here
 *	waiting for a request to be available on the socket
 *	associated with the passed transport handle. This routine will
 *	then call svc_getreq to do all the work.
 *
 * Algorithm
 *	repeat forever {
 *		while there is no data on the socket
 *			sleep (sbwait)
 *		if (we were interupted) longjump back to nfs_svc
 *		call svc_getreq
 *	}
 *
 * Modification History
 *	11/05/87	ew	added comments
 *
 * External Calls
 *	sbwait			wait for data to arrive on the socket
 *	longjunp		jump back to nfs_svc
 *	svc_getreq	        process an incomming request
 *
 * Called By
 *	nfs_svc	
 *
 ********************************************************************
 * END_IMS svc_run */

#ifdef _NAMESPACE_CLEAN
#undef svc_run
#pragma _HP_SECONDARY_DEF _svc_run svc_run
#define svc_run _svc_run
#endif

int Rpccnt;
void
svc_run(xprt)
	SVCXPRT *xprt;
{
	int	s;
/*
 * HPNFS  added check of retval of sbwait so that we can detect interrupts,
 * HPNFS  longjmp back to the calling routine, ie. nfs_svc.  dds, 12/4/86
 */
	int 	retval;

	while (TRUE) {
		s = splnet();
	 	while (xprt->xp_sock->so_rcv.sb_cc == 0) {
			retval = sbwait(&xprt->xp_sock->so_rcv);
			if ( retval == EINTR ) {
				(void) splx(s);
				longjmp(&u.u_qsave,EINTR);
			}
                }
		(void) splx(s);
		svc_getreq(xprt);
		Rpccnt++;
	}
}


#else /* not KERNEL */
/* BEGIN_IMS svc_run_ms (user) **************************************
 ********************************************************************
 ****
 ****	void
 ****	svc_run_ms()
 ****
 ********************************************************************
 * Input Parameters
 *	none
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	svc_fds		- bit mask of "active" file descriptors
 *
 * Description
 *	This routine will wait for any enabled file descriptors
 *	to be readable (select) and then pass a bit mask of
 *	readable descriptors to svc_getreq for processing.
 *
 * Algorithm
 *	If no file descriptors are enabled return
 *	wait for a request on any enabled file descriptor
 *	if (select failed)
 *		return
 *	else
 *		call svc_getreq to process the request(s)
 *
 * Notes
 *	This is a user level only routine
 *
 * Modification History
 *	11/05/87	ew	added comments
 *
 * External Calls
 *	select			wait for data to be available
 *	svc_getreq		process waiting request(s)
 *
 ********************************************************************
 * END_IMS svc_run_ms (user) */

#ifdef _NAMESPACE_CLEAN
#undef svc_run_ms
#pragma _HP_SECONDARY_DEF _svc_run_ms svc_run_ms
#define svc_run_ms _svc_run_ms
#endif

void
svc_run_ms(nfds)
register long nfds;
{
	fd_set readfds;
	register fd_set_len, or_mask, *mask_ptr,
		 fdsetlen_save = howmany(nfds, NFDBITS);/* words to hold nfds */
	int	 fd_size = (fdsetlen_save << 2);

	nlmsg_fd = _nfs_nls_catopen();

	while (TRUE) {
		/*	HPNFS	jad	87.09.10
		**	THIS IS AN ERROR!  svc_run_ms() will blindly
		**	select() on zero readfds and hang forever
		**	which is not behavior we like, so instead
		**	we just return to our caller (an error).
		*/

		mask_ptr = svc_fdset.fds_bits;
		or_mask = 0;
		fd_set_len = fdsetlen_save;
		do {
			or_mask |= *(mask_ptr++);
		} while (--fd_set_len);
		
		if (or_mask == 0)
			return;

		memcpy(&readfds,&svc_fdset, fd_size);

		switch (select(nfds, &readfds, (int *)0, (int *)0,
		    (struct timeval *)0)) {

		case -1:
			if (errno == EINTR)
				continue;
			else {
				perror((catgets(nlmsg_fd,NL_SETN,2, "svc.c: - Select failed")));
				return;
			}
		case 0:
			continue;
		default:
/*
 * This will probably need more work with the file descriptors
 */
			svc_getreqset_ms(&readfds, nfds);
		}
	}
}
/* BEGIN_IMS svc_run (user) *
 ********************************************************************
 ****
 ****	void
 ****	svc_run()
 ****
 ********************************************************************
 * Input Parameters
 *	none
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	svc_fds		- bit mask of "active" file descriptors
 *
 * Description
 *	This routine will wait for ANY enabled file descriptors
 *	to be readable (select) and then pass a bit mask of
 *	readable descriptors to svc_getreq for processing.
 *
 * Algorithm
 *		call svc_run_ms(FD_SETSIZE);
 * Notes
 *	This is a user level only routine
 *
 * Modification History
 *	11/05/87	ew	added comments
 *
 * External Calls
 *	select			wait for data to be available
 *	svc_getreq		process waiting request(s)
 *
 ********************************************************************
 * END_IMS svc_run (user) */

#ifdef _NAMESPACE_CLEAN
#undef svc_run
#pragma _HP_SECONDARY_DEF _svc_run svc_run
#define svc_run _svc_run
#endif

void
svc_run()
{
	svc_run_ms(FD_SETSIZE);
}
#endif /* KERNEL */
