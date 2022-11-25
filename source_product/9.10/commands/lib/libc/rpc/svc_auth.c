/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_auth.c,v $
 * $Revision: 12.2 $	$Author: dlr $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/05/03 15:21:05 $
 */

/* BEGIN_IMS_MOD    
******************************************************************************
****								
****	svc_auth.c - rpc server authentication interface
****								
******************************************************************************
* Description
*	This module contains the highest level server authntication
*	routine, _authenticate, as well as the null authentication
*	routine. The real decoding of authentication credentials is
*	is done by a specific routine depending upon the credential type.
*
* Externally Callable Routines
*	_authenticate		set up and call credential specific routine
*	_svc_auth_null		"no authentication" routine
*
* Test Module
*	$scaffold/nfs/*
*
* To Do List
*
* Notes
*
* Modification History
*	11/08/87	ew	added comments
*
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_auth.c,v 12.2 90/05/03 15:21:05 dlr Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#ifdef KERNEL
#include "../h/param.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/svc_auth.h"
#else
#include <rpc/types.h>	/* <> */
#include <netinet/in.h>
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#include <rpc/clnt.h>	/* <> */
#include <rpc/rpc_msg.h>	/* <> */
#include <rpc/svc.h>	/* <> */
#include <rpc/svc_auth.h>	/* <> */
#endif

/*
 * svcauthsw is the bdevsw of server side authentication. 
 * 
 * Server side authenticators are called from authenticate by
 * using the client auth struct flavor field to index into svcauthsw.
 * The server auth flavors must implement a routine that looks  
 * like: 
 * 
 *	enum auth_stat 
 *	flavorx_auth(rqst, msg)
 *		register struct svc_req *rqst; 
 *		register struct rpc_msg *msg;
 *  
 */

enum auth_stat _svcauth_null();		/* no authentication */
enum auth_stat _svcauth_unix();		/* unix style (uid, gids) */
enum auth_stat _svcauth_short();	/* short hand unix style */

static struct {
	enum auth_stat (*authenticator)();
} svcauthsw[] = {
	_svcauth_null,			/* AUTH_NULL */
	_svcauth_unix,			/* AUTH_UNIX */
	_svcauth_short			/* AUTH_SHORT */
};
#define	AUTH_MAX	2		/* HIGHEST AUTH NUMBER */


/* BEGIN_IMS _authenticate *
 ********************************************************************
 ****
 ****	enum auth_stat
 ****	authenticate(rqst, msg)
 ****		register struct svc_req *rqst;
 ****		struct rpc_msg *msg;
 ****
 ********************************************************************
 * Input Parameters
 *	rqst		rpc request being built
 *	msg		message from the wire
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	auth_stat	results of authentication
 *
 * Globals Referenced
 *	none
 *
 * Description
 * The call rpc message, msg has been obtained from the wire.  The msg contains
 * the raw form of credentials and verifiers.  authenticate returns AUTH_OK
 * if the msg is successfully authenticated.  If AUTH_OK then the routine also
 * does the following things:
 * set rqst->rq_xprt->verf to the appropriate response verifier;
 * sets rqst->rq_client_cred to the "cooked" form of the credentials.
 *
 * NB: rqst->rq_cxprt->verf must be pre-alloctaed;
 * its length is set appropriately.
 *
 * The caller still owns and is responsible for msg->u.cmb.cred and
 * msg->u.cmb.verf.  The authentication system retains ownership of
 * rqst->rq_client_cred, the cooked credentials.
 *
 * Algorithm
 *	set request authentication to null with length zero
 *	if (authentication flavor is one we cane handle)
 *		call corresponding routine
 *	else
 *		return error
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	11/08/97	ew	added comments
 *
 * External Calls
 *	credential specific routine
 *
 * Called By
 *	svc_getreq
 *
 ********************************************************************
 * END_IMS _authenticate */

enum auth_stat
_authenticate(rqst, msg)
	register struct svc_req *rqst;
	struct rpc_msg *msg;
{
	register int cred_flavor;

	rqst->rq_cred = msg->rm_call.cb_cred;
	rqst->rq_xprt->xp_verf.oa_flavor = _null_auth.oa_flavor;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	cred_flavor = rqst->rq_cred.oa_flavor;
	if ((cred_flavor <= AUTH_MAX)
	    && (cred_flavor >= AUTH_NULL))
	{
		return ((*(svcauthsw[cred_flavor].authenticator))(rqst, msg));
	}

	return (AUTH_REJECTEDCRED);
}


/* BEGIN_IMS _svcauth_null *
 ********************************************************************
 ****
 ****	enum auth_stat
 ****	_svcauth_null()
 ****
 ********************************************************************
 * Input Parameters
 *	none
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	auth_stat		always returns AUTH_OK
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Basically a place holder routine used when we don't care
 *	about authentication.
 *
 * Algorithm
 *	return AUTH_OK
 *
 ********************************************************************
 * END_IMS _svcauth_null */

enum auth_stat
_svcauth_null(/*rqst, msg*/)
	/*struct svc_req *rqst;
	struct rpc_msg *msg;*/
{

	return (AUTH_OK);
}
