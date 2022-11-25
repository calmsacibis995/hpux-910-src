/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 1.9.83.4 $	$Date: 93/12/06 22:48:41 $  */

/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/pmap_prot.c,v $
 * $Revision: 1.9.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 22:48:41 $
 *
 * Revision 1.4.109.1  91/08/26  14:08:07  14:08:07  kcs (Kernel Control System)
 * kernel90 (1.4) KERNEL 
 * 
 * Revision 1.4  91/08/26  14:08:07  14:08:07  kcs (Kernel Control System)
 * Place holder
 * 
 * Revision 1.4  91/07/22  11:46:29  11:46:29  kcs (Kernel Control System)
 * Merging differences between the designated ancestor Merge802_1(1.3)
 * and kernel806:Merge806_1(1.2.186.2) onto KERNEL(1.3).
 * 
 * Revision         
 * 
 * Revision         
 * 
 * Revision 1.1  90/04/25  10:25:16  10:25:16  kcs (Kernel Control System)
 * Place holder
 * 
 * Revision 1.1  90/04/24  15:11:16  15:11:16  kcs (Kernel Control System)
 * Initial revision
 * 
 * Revision 1.3.3.2  90/04/19  15:06:46  15:06:46  sandip
 * Merged with CND's commands branch obtained from Prabha and Dominique.
 * Files affected are: rpc/authux_pro.c rpc/pmap_prot.c rpc_prot.c svc.c
 * svc_auth.c svc_authux.c xdr.c xdr_array.c xdr_mem.c
 * 
 * Revision 12.1  90/03/20  10:28:27  10:28:27  dlr (Dominic Ruffatto)
 * Put comments around strings following #else and #endif lines (ANSII-C).
 * 
 * Revision 12.0  89/09/25  16:08:52  16:08:52  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  13:03:32  13:03:32  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.6  89/01/26  12:02:40  12:02:40  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.5  89/01/16  15:17:06  15:17:06  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:49  14:48:49  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.2  88/08/23  14:05:12  14:05:12  dds (Darren Smith)
 * Ifdef'd to include xdr_pmap() in the kernel. dds.
 * 
 * Revision 1.1.10.4  88/08/23  13:04:19  13:04:19  cmahon (Christina Mahon)
 * Ifdef'd to include xdr_pmap() in the kernel. dds.
 * 
 * Revision 10.1  88/02/29  13:52:46  13:52:46  nfsmgr (Nfsmanager)
 * Starting revision level for 6.2 and 3.0.
 * 
 * Revision 10.1  88/02/29  13:35:31  13:35:31  nfsmgr (Nfsmanager)
 * Starting point for 6.2 and 3.0.
 * 
 * Revision 8.2  88/02/04  10:34:57  10:34:57  chm (Cristina H. Mahon)
 * Changed SCCS strings to RCS revision and date strings.
 * 
 * Revision 8.1  87/11/30  13:54:53  13:54:53  nfsmgr (Nfsmanager)
 * revision levels for ic4b
 * 
 * Revision 1.1.10.3  87/08/07  14:55:50  14:55:50  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:13:04  22:13:04  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.3  86/09/25  14:20:58  14:20:58  vincent (Vincent Hsieh)
 * Included h/types.h fot NFS port.
 * 
 * Revision 1.2  86/07/28  11:41:00  11:41:00  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
#endif

/*
 * pmap_prot.c
 * Protocol for the local binder service, or pmap.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define xdr_bool 		_xdr_bool
#define xdr_pmap 		_xdr_pmap		/* In this file */
#define xdr_pmaplist 		_xdr_pmaplist		/* In this file */
#define xdr_reference 		_xdr_reference
#define xdr_u_long 		_xdr_u_long

#endif /* _NAMESPACE_CLEAN */

#ifdef _KERNEL
#include "../h/types.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/pmap_prot.h"
#else /* !_KERNEL */
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/pmap_prot.h>
#define NULL ((struct pmaplist *)0)

#endif /* _KERNEL */

#ifdef _NAMESPACE_CLEAN
#undef xdr_pmap
#pragma _HP_SECONDARY_DEF _xdr_pmap xdr_pmap
#define xdr_pmap _xdr_pmap
#endif

bool_t
xdr_pmap(xdrs, regs)
	XDR *xdrs;
	struct pmap *regs;
{

	if (xdr_u_long(xdrs, &regs->pm_prog) && 
		xdr_u_long(xdrs, &regs->pm_vers) && 
		xdr_u_long(xdrs, &regs->pm_prot))
		return (xdr_u_long(xdrs, &regs->pm_port));
	return (FALSE);
}

#ifndef _KERNEL

/* 
 * What is going on with linked lists? (!)
 * First recall the link list declaration from pmap_prot.h:
 *
 * struct pmaplist {
 *	struct pmap pml_map;
 *	struct pmaplist *pml_map;
 * };
 *
 * Compare that declaration with a corresponding xdr declaration that 
 * is (a) pointer-less, and (b) recursive:
 *
 * typedef union switch (bool_t) {
 * 
 *	case TRUE: struct {
 *		struct pmap;
 * 		pmaplist_t foo;
 *	};
 *
 *	case FALSE: struct {};
 * } pmaplist_t;
 *
 * Notice that the xdr declaration has no nxt pointer while
 * the C declaration has no bool_t variable.  The bool_t can be
 * interpreted as ``more data follows me''; if FALSE then nothing
 * follows this bool_t; if TRUE then the bool_t is followed by
 * an actual struct pmap, and then (recursively) by the 
 * xdr union, pamplist_t.  
 *
 * This could be implemented via the xdr_union primitive, though this
 * would cause a one recursive call per element in the list.  Rather than do
 * that we can ``unwind'' the recursion
 * into a while loop and do the union arms in-place.
 *
 * The head of the list is what the C programmer wishes to past around
 * the net, yet is the data that the pointer points to which is interesting;
 * this sounds like a job for xdr_reference!
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_pmaplist
#pragma _HP_SECONDARY_DEF _xdr_pmaplist xdr_pmaplist
#define xdr_pmaplist _xdr_pmaplist
#endif

bool_t
xdr_pmaplist(xdrs, rp)
	register XDR *xdrs;
	register struct pmaplist **rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct pmaplist **next;

	while (TRUE) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the next object
		 * before we free the current object ...
		 */
		if (freeing)
			next = &((*rp)->pml_next); 
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof(struct pmaplist), xdr_pmap))
			return (FALSE);
		rp = (freeing) ? next : &((*rp)->pml_next);
	}
}

#endif /* !_KERNEL */
