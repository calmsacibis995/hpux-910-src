/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/auth_kern.c,v $
 * $Revision: 1.7.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:26:30 $
 * Modified for BSD4.3 8.0 
 */
/* BEGIN_IMS_MOD  
******************************************************************************
****								
****	auth_kern -  UNIX style authentication in the kernel 
****								
******************************************************************************
* Description
*	This module implements UNIX style authentications in the kernel. It
*       interfaces with svc_auth_unix on the server.  See auth_unix.c for
*       the user level implementation of UNIX authentication.
*
* Externally Callable Routines
*	authkern_create - initialize AUTH structure
*	authkern_nextverf - noop
*	authkern_marshal - insert user credentials into RPC packet
*	authkern_validate - validate the authentication in the RPC packet
*	authkern_refresh - noop
*	authkern_destroy - destroy the AUTH structure
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
static char rcsid[] = "@(#) $Header: auth_kern.c,v 1.7.83.3 93/09/17 19:26:30 kcs Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
#include "../netinet/in.h"
#include "../h/utsname.h"
#include "../h/subsys_id.h"
#include "../h/netdiag1.h" 
#include "../h/net_diag.h"
/*
 * Unix authenticator operations vector
 */
void	authkern_nextverf();
bool_t	authkern_marshal();
bool_t	authkern_validate();
bool_t	authkern_refresh();
void	authkern_destroy();

static struct auth_ops auth_kern_ops = {
	authkern_nextverf,
	authkern_marshal,
	authkern_validate,
	authkern_refresh,
	authkern_destroy
};

extern char hostname[MAXHOSTNAMELEN+1];


/* BEGIN_IMS authkern_create *
 ********************************************************************
 ****
 ****		authkern_create()
 ****
 ********************************************************************
 * Input Parameters
 *	none
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	Initialized AUTH structure
 *
 * Globals Referenced
 *	_null_auth
 *	auth_kern_ops
 *
 * Description
 *	This allocates memory for a kernel Unix style AUTH structure 
 *	and initializes the function pointer to point to the various 
 *	AUTH handling routines.
 *
 * Algorithm
 *	{ allocated memory for the AUTH structure
 *	  initialize the AUTH structure to point to the AUTH handling
 *		routines
 *	  return(pointer to the AUTH structure)
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
 * END_IMS authkern_create */
AUTH *
authkern_create()
{
	register AUTH *auth;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)kmem_alloc((u_int)sizeof(*auth));
	auth->ah_ops = &auth_kern_ops;
	auth->ah_verf = _null_auth;
	return (auth);
}

/* authkern operations */


/* BEGIN_IMS authkern_nextverf *
 ********************************************************************
 ****
 ****		authkern_nextverf(auth)
 ****
 ********************************************************************
 * Description
 *	This is a noop that holds a place in the auth_kern_ops
 *	function switch table.
 *
 ********************************************************************
 * END_IMS authkern_nextverf */
/*ARGSUSED*/
void
authkern_nextverf(auth)
	AUTH *auth;
{

	/* no action necessary */
}


/* BEGIN_IMS authkern_marshal *
 ********************************************************************
 ****
 ****		authkern_marshal(auth, xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	auth		pointer to the AUTH structure
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
 *	This is called via the AUTH_MARSHALL macro from clntkudp_callit()
 *	to insert the current credentials into the RPC packet. It is
 *	actually an xdr routine, as it performs the action of getting the
 *	credential data from the user structure and calling XDR routines
 *	to translate that data. If possible, it will use xdrmbuf_inline()
 *	and the IXDR macros to do inline translations, otherwise it will
 *	call xdr_authkern() to do the XDR translations.
 *
 * Algorithm
 *	{ calculate the actual number of groups the process belongs to
 *	  calculate the hostname length
 *	  calculate the size of the authunix_parms structure
 *	  if (sufficient INLINE space is available for the cred flavor and 
 *	       the authunix_parms structure and the verf flavor) {
 *		use IXDR routines to xdr the following parameters on to
 *		the RPC packet
 *			cred flavor
 *			cred length
 *			current time
 *			hostname 		<< length + name >>
 *			process id
 *			group id
 *			number of groups process belongs to
 *			group ids
 *		return(TRUE)
 *	  }
 *	  allocate space to serialize u struct information
 *	  create a xdrmem stream with the above space
 *	  if (serialization of u struct info fails) {
 *		log message
 *		return space
 *		return(FALSE)
 *	  }
 *	  change ah_cred record in the AUTH structure to point to
 *		the allocated space which contains the serialized
 *		u struct info
 *	  if (serialization of cred flavor,length and verf flavor,length
 *		fails)
 *		return(FALSE)
 *	  else
 *		return(TRUE)
 *	}
 *
 * Concurrency
 *
 * To Do List
 *	. If we do not use IXDR routines, we are returning the allocated
 *	  space even though the ah_cred in the AUTH structure points to
 *	  it.
 *
 * Notes
 *	Because a contiguous buffer is allocated by the client code for
 *	this purpose, the inline routines should be the only ones that
 *	are called.
 *
 * Modification History
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS authkern_marshal */
bool_t
authkern_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	char	*sercred;
	XDR	xdrm;
	struct	opaque_auth *cred;
	bool_t	ret = FALSE;
	register int *gp, *gpend;
	register int gidlen, credsize;
	register long *ptr;
	register hostnamelen;
	register char *hostnamep;

	/*
	 * First we try a fast path to get through
	 * this very common operation.
	 */
	gp = u.u_groups;
	gpend = &u.u_groups[NGRPS];
	while (gpend > u.u_groups && gpend[-1] < 0)
		gpend--;
	gidlen = gpend - gp;
	hostnamep = hostname;
	hostnamelen = 0;
/* Can we make hostnamelen a global ? */
	while (*hostnamep != 0 && hostnamelen < MAXHOSTNAMELEN) {
		hostnamep++;
		hostnamelen++;
	}
	credsize = 4 + 4 + roundup(hostnamelen, 4) + 4 + 4 + 4 + gidlen * 4;
	ptr = XDR_INLINE(xdrs, 4 + 4 + credsize + 4 + 4);
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_UNIX);	/* cred flavor */
		IXDR_PUT_LONG(ptr, credsize);	/* cred len */
		IXDR_PUT_LONG(ptr, time.tv_sec);
		IXDR_PUT_LONG(ptr, hostnamelen);
		bcopy(hostname, (caddr_t)ptr, (u_int)hostnamelen);
		ptr += roundup(hostnamelen, 4) / 4;
/*		HP pre-7.0 code without long hostnames  */
		/*
		IXDR_PUT_LONG(ptr, hostnamelen);
		bcopy(utsname.nodename, (caddr_t)ptr, (u_int)hostnamelen);
		ptr += roundup(hostnamelen, 4) / 4;
		*/

		IXDR_PUT_LONG(ptr, u.u_uid);
		IXDR_PUT_LONG(ptr, u.u_gid);
		IXDR_PUT_LONG(ptr, gidlen);
		while (gp < gpend) {
			IXDR_PUT_LONG(ptr, *gp++);
		}
		IXDR_PUT_LONG(ptr, AUTH_NULL);	/* verf flavor */
		IXDR_PUT_LONG(ptr, 0);	/* verf len */
		return (TRUE);
	}
	sercred = (char *)kmem_alloc((u_int)MAX_AUTH_BYTES);
	/*
	 * serialize u struct stuff into sercred
	 */
	xdrmem_create(&xdrm, sercred, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authkern(&xdrm)) {
		NS_LOG(LE_NFS_MARSHAL_FAIL,NS_LC_WARNING,NS_LS_NFS,0);
		ret = FALSE;
		goto done;
	}

	/*
	 * Make opaque auth credentials that point at serialized u struct
	 */
	cred = &(auth->ah_cred);
	cred->oa_length = XDR_GETPOS(&xdrm);
	cred->oa_flavor = AUTH_UNIX;
	cred->oa_base = sercred;

	/*
	 * serialize credentials and verifiers (null)
	 */
	if ((xdr_opaque_auth(xdrs, &(auth->ah_cred)))
	    && (xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}
done:
	kmem_free((caddr_t)sercred, (u_int)MAX_AUTH_BYTES);
	return (ret);
}


/* BEGIN_IMS authkern_validate *
 ********************************************************************
 ****
 ****		authkern_validate(auth, verf)
 ****
 ********************************************************************
 * Input Parameters
 *	auth		pointer to the AUTH structure
 *	verf		opaque_auth structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	TRUE		always true
 *
 * Description
 * 	Validate authentication of of RPC packet.
 *
 * Algorithm
 * 	return (true)
 *
 * Called By 
 *	clntkudp_callit
 *
 ********************************************************************
 * END_IMS authkern_validate */
/*ARGSUSED*/
bool_t
authkern_validate(auth, verf)
	AUTH *auth;
	struct opaque_auth verf;
{

	return (TRUE);
}


/* BEGIN_IMS authkern_refresh *
 ********************************************************************
 ****
 ****		authkern_refresh(auth)
 ****
 ********************************************************************
 * Description
 *	This is a noop that holds a place in the auth_kern_ops
 *	function switch table.
 *
 ********************************************************************
 * END_IMS authkern_refresh */
/*ARGSUSED*/
bool_t
authkern_refresh(auth)
	AUTH *auth;
{
}


/* BEGIN_IMS authkern_destroy *
 ********************************************************************
 ****
 ****		authkern_destroy(auth)
 ****
 ********************************************************************
 * Input Parameters
 *	auth		pointer to the AUTH structure
 *
 * Output Parameters
 * 	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *
 * Description
 *	Frees the memory taken up by the auth structure via a call to
 *	kmem_free.
 *
 * Algorithm
 *
 * Concurrency
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS authkern_destroy */
void
authkern_destroy(auth)
	register AUTH *auth;
{
	kmem_free((caddr_t)auth, (u_int)sizeof(*auth));
}
