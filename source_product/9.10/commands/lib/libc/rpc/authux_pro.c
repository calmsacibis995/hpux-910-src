/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/authux_pro.c,v $
 * $Revision: 12.1 $	$Author: dlr $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/03/20 10:25:39 $
 */

/* BEGIN_IMS_MOD   
******************************************************************************
****								
****	authux_pro - XDR for UNIX style authentication parameters
****								
******************************************************************************
* Description
*	This module contains the XDR routines for UNIX style authentication
*	parameters used by RPC.
*
* Externally Callable Routines
*	xdr_authunix_parms - XDR routine for UNIX authentication parameters
*	xdr_authkern - encode UNIX authentication information
*
* Test Module
*	$SCAFFOLD/nfs/*
*
* To Do List
*
* Notes
*
* Modification History
*
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: authux_pro.c,v 12.1 90/03/20 10:25:39 dlr Exp $ (Hewlett-Packard)";
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

#define xdr_array 		_xdr_array
#define xdr_authunix_parms 	_xdr_authunix_parms	/* In this file */
#define xdr_int 		_xdr_int
#define xdr_string 		_xdr_string
#define xdr_u_long 		_xdr_u_long

#endif /* _NAMESPACE_CLEAN */

#ifdef KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
#include "../h/utsname.h"
#else
#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#include <rpc/auth_unix.h>	/* <> */
#endif


/* BEGIN_IMS xdr_authunix_parms *
 ********************************************************************
 ****
 ****		xdr_authunix_parms(xdrs, p)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	p		pointer to authunix_parms structure
 *
 * Output Parameters
 *	p->		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	On the server size the initial handling of the credentials occurs
 *	in xdr_callmsg(), which simply copies the credential information
 *	into a raw data area. xdr_callmsg is called by svckudp_recv() 
 *	which is called by svc_getreq(). svc_getreq() finishes the processing
 *	of the credentials by calling _authenticate(), which simply looks
 *	up the appropriate service routine for the type of credentials.
 *	Since the credentials are usually the UNIX type, the routine
 *	normally called is _svcauth_unix(). This routine is basically the
 *	inverse of authkern_marshal(), using the xdr IXDR routines when
 *	possible, otherwise calling xdr_authunix_parms() to decode the
 *	information. xdr_authunix_parms calls lower level xdr routines 
 *	to handle the various fields in the authunix_parms structure.
 *
 * Algorithm
 *	{ call xdr_u_long to handle the update time
 *	       xdr_string to handle the machine name
 *	       xdr_int to handle the uid
 *	       xdr_int to handle the gid
 *	       xdr_array to handle the aup_gids array
 *	  return(value)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_authunix_parms */

#ifdef _NAMESPACE_CLEAN
#undef xdr_authunix_parms
#pragma _HP_SECONDARY_DEF _xdr_authunix_parms xdr_authunix_parms
#define xdr_authunix_parms _xdr_authunix_parms
#endif

bool_t
xdr_authunix_parms(xdrs, p)
	register XDR *xdrs;
	register struct authunix_parms *p;
{

	if (xdr_u_long(xdrs, &(p->aup_time))
	    && xdr_string(xdrs, &(p->aup_machname), MAX_MACHINE_NAME)
	    && xdr_int(xdrs, &(p->aup_uid))
	    && xdr_int(xdrs, &(p->aup_gid))
	    && xdr_array(xdrs, (caddr_t *)&(p->aup_gids),
		    &(p->aup_len), NGRPS, sizeof(int), xdr_int) ) {
		return (TRUE);
	}
	return (FALSE);
}


#ifdef KERNEL
/* BEGIN_IMS xdr_authkern *
 ********************************************************************
 ****
 ****		xdr_authkern(xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	XDR kernel unix auth parameters. Goes out of the u struct directly.
 *	This is called from authkern_marshal() only if the inline (IXDR)
 *	routines cannot be called. This is used to encode only.
 *
 * Algorithm
 *	{ calculate the number of groups the process belongs to
 *	  serialize the following fields of the u struct
 *		time 
 *		hostname
 *		process id
 *		group id
 *		groups the process belongs to	<< length + groups >>
 *	  return(value)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	This is an XDR_ENCODE only routine.
 *
 * Modification History
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_authkern */
xdr_authkern(xdrs)
	register XDR *xdrs;
{
	int	*gp;
	int	 uid = u.u_uid;
	int	 gid = u.u_gid;
	int	 len;
	caddr_t	groups;
	char	*name = utsname.nodename;

	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}

	for (gp = &u.u_groups[NGRPS]; gp > u.u_groups; gp--) {
		if (gp[-1] >= 0) {
			break;
		}
	}
	len = gp - u.u_groups;
	groups = (caddr_t)u.u_groups;
        if (xdr_u_long(xdrs, (u_long *)&time.tv_sec)
            && xdr_string(xdrs, &name, MAX_MACHINE_NAME)
            && xdr_int(xdrs, &uid)
            && xdr_int(xdrs, &gid)
	    && xdr_array(xdrs, &groups, (u_int *)&len, NGRPS, sizeof (int), xdr_int) ) {
                return (TRUE);
	}
	return (FALSE);
}
#endif
