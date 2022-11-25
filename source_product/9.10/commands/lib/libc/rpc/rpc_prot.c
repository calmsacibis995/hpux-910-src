/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/rpc_prot.c,v $
 * $Revision: 12.2 $	$Author: dlr $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/05/03 14:56:36 $
 */

/* BEGIN_IMS_MOD   
******************************************************************************
****								
****		rpc_prot - XDR routines for RPC protocol packets
****								
******************************************************************************
* Description
* 	This set of routines implements the rpc message definition,
* 	its serializer, and some common rpc utility routines. These
*	routines are meant for various implementations of rpc - they
*	are NOT for the rpc client or rpc service implementations!
* 	Because authentication stuff is easy and is part of rpc, the
*	opaque routines are also in this module.
*
* Externally Callable Routines
*	xdr_opaque_auth	    - encode/decode opaque authentication structure
*	xdr_accepted_reply  - encode/decode accepted reply message
*	xdr_rejected_reply  - encode/decode rejected reply message
*	xdr_replymsg        - encode/decode reply message
*	xdr_callmsg         - encode/decode call message
*	xdr_callhdr         - encode/decode header of call message
*	_seterr_reply       - get return status of RPC call
*
*	xdr_deskey          - non-kernel routine to xdr DES key
*	rpc_badpacket       - trigger routine to send bad rpc packet
*	
* Internal Routines
*	accepted            - fill in return status of accepted RPC call
*	rejected            - fill in return status of rejected RPC call
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
static char rcsid[] = "@(#) $Header: rpc_prot.c,v 12.2 90/05/03 14:56:36 dlr Exp $ (Hewlett-Packard)";
#endif

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define memcpy 			_memcpy
#define xdr_accepted_reply 	_xdr_accepted_reply	/* In this file */
#define xdr_bytes 		_xdr_bytes
#define xdr_callhdr 		_xdr_callhdr		/* In this file */
#define xdr_callmsg 		_xdr_callmsg		/* In this file */
#define xdr_deskey 		_xdr_deskey		/* In this file */
#define xdr_enum 		_xdr_enum
#define xdr_opaque 		_xdr_opaque
#define xdr_opaque_auth 	_xdr_opaque_auth	/* In this file */
#define xdr_rejected_reply 	_xdr_rejected_reply	/* In this file */
#define xdr_replymsg 		_xdr_replymsg		/* In this file */
#define xdr_u_int 		_xdr_u_int
#define xdr_u_long 		_xdr_u_long
#define xdr_union 		_xdr_union

#endif /* _NAMESPACE_CLEAN */

#ifdef KERNEL
#include "../h/param.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../netinet/in.h"
#ifdef NTRIGGER
#include "../h/trigdef.h"
#endif /* NTRIGGER */
#else
#include <sys/param.h>
#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#include <rpc/clnt.h>	/* <> */
#include <rpc/rpc_msg.h>	/* <> */
#include <netinet/in.h>
#endif


/* * * * * * * * * * * * * * XDR Authentication * * * * * * * * * * * */

struct opaque_auth _null_auth;

/* BEGIN_IMS xdr_opaque_auth *
 ********************************************************************
 ****
 **** 			xdr_opaque_auth(xdrs, ap)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	ap	pointer to opaque authentication structure
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	XDR an opaque authentication struct (see auth.h).
 *
 * Algorithm
 *	if (xdr'ing the enum giving authentication flavor succeeds)
 *		return (xdr the bytes of authentication info)
 *	return (FALSE)
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	xdr_enum
 *	xdr_bytes
 *
 * Called By
 *	authkern_marshal, clntkudp_callit
 *	xdr_callmsg, xdr_accepted_reply
 *
 ********************************************************************
 * END_IMS xdr_opaque_auth */

#ifdef _NAMESPACE_CLEAN
#undef xdr_opaque_auth
#pragma _HP_SECONDARY_DEF _xdr_opaque_auth xdr_opaque_auth
#define xdr_opaque_auth _xdr_opaque_auth
#endif

bool_t
xdr_opaque_auth(xdrs, ap)
	register XDR *xdrs;
	register struct opaque_auth *ap;
{

	if (xdr_enum(xdrs, (enum_t *)&(ap->oa_flavor)))
		return (xdr_bytes(xdrs, &ap->oa_base,
			&ap->oa_length, MAX_AUTH_BYTES));
	return (FALSE);
}


/* BEGIN_IMS xdr_deskey *
 ********************************************************************
 ****
 **** 			xdr_deskey(xdrs, blkp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	blkp	pointer to DES block
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	XDR a DES key.
 *
 * Algorithm
 *	ifndef KERNEL
 *	if (xdr'ing the high part of key fails)
 *		return (FALSE)
 *	return (xdr the low part of key)
 *      endif !KERNEL
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	xdr_u_long
 *
 * Called By
 *
 ********************************************************************
 ********************************************************************
 * END_IMS xdr_deskey */

#ifdef _NAMESPACE_CLEAN
#undef xdr_deskey
#pragma _HP_SECONDARY_DEF _xdr_deskey xdr_deskey
#define xdr_deskey _xdr_deskey
#endif

#ifndef KERNEL
bool_t
xdr_deskey(xdrs, blkp)
	register XDR *xdrs;
	register union des_block *blkp;
{

	if (! xdr_u_long(xdrs, &(blkp->key.high)))
		return (FALSE);
	return (xdr_u_long(xdrs, &(blkp->key.low)));
}
#endif /* !KERNEL */


/* * * * * * * * * * * * * * XDR RPC MESSAGE * * * * * * * * * * * * * * * */

/* BEGIN_IMS xdr_accepted_reply *
 ********************************************************************
 ****
 **** 			xdr_accepted_reply(xdrs, ar)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	ar	pointer to accepted reply structure
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	XDR the MSG_ACCEPTED part of a reply message union.
 *
 * Algorithm
 *	if (xdr'ing the opaque authentication fails)
 *		return (FALSE)
 *	if (xdr'ing the enum giving return status fails)
 *		return (FALSE)
 *	switch (return status) {
 *		case SUCCESS:
 *			return(xdr the results of the RPC call)
 *		case PROG_MISMATCH:
 *			if (xdr'ing the low version fails)
 *				return (FALSE)
 *			return (xdr the high version)
 *	}
 *	return (TRUE)
 *
 * Concurrency
 *	none
 *
 * Notes
 *	MSG_ACCEPTED branch of reply_dscrm structure
 *
 * Modification History
 *
 * External Calls
 *	xdr_opaque_auth
 *	xdr_enum
 *	(*results.proc)
 *	xdr_u_long
 *
 * Called By
 *	xdr_replymsg via xdr_union
 *
 ********************************************************************
 ********************************************************************
 * END_IMS xdr_accepted_reply */

#ifdef _NAMESPACE_CLEAN
#undef xdr_accepted_reply
#pragma _HP_SECONDARY_DEF _xdr_accepted_reply xdr_accepted_reply
#define xdr_accepted_reply _xdr_accepted_reply
#endif

bool_t 
xdr_accepted_reply(xdrs, ar)
	register XDR *xdrs;   
	register struct accepted_reply *ar;
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_opaque_auth(xdrs, &(ar->ar_verf)))
		return (FALSE);
	if (! xdr_enum(xdrs, (enum_t *)&(ar->ar_stat)))
		return (FALSE);
	switch (ar->ar_stat) {

	case SUCCESS:
		return ((*(ar->ar_results.proc))(xdrs, ar->ar_results.where));
	
	case PROG_MISMATCH:
		if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
	}
	return (TRUE);  /* TRUE => open ended set of problems */
}


/* BEGIN_IMS xdr_rejected_reply *
 ********************************************************************
 ****
 **** 			xdr_rejected_reply(xdrs, rr)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	rr	pointer to rejected reply structure
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	XDR the MSG_DENIED part of a reply message union.
 *
 * Algorithm
 *	if (xdr'ing the enum giving return status fails)
 *		return (FALSE)
 *	switch (return status) {
 *		case RPC_MISMATCH:
 *			if (xdr'ing the low version fails)
 *				return (FALSE)
 *			return (xdr the high version)
 *		case AUTH_ERROR:
 *			return(xdr the reason)
 *	}
 *	return (FALSE)
 *
 * Concurrency
 *	none
 *
 * Notes
 *	MSG_DENIED branch of reply_dscrm structure
 *
 * Modification History
 *
 * External Calls
 *	xdr_enum
 *	xdr_u_long
 *
 * Called By
 *	xdr_replymsg via xdr_union
 *
 ********************************************************************
 * END_IMS xdr_rejected_reply */

#ifdef _NAMESPACE_CLEAN
#undef xdr_rejected_reply
#pragma _HP_SECONDARY_DEF _xdr_rejected_reply xdr_rejected_reply
#define xdr_rejected_reply _xdr_rejected_reply
#endif

bool_t 
xdr_rejected_reply(xdrs, rr)
	register XDR *xdrs;
	register struct rejected_reply *rr;
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_enum(xdrs, (enum_t *)&(rr->rj_stat)))
		return (FALSE);
	switch (rr->rj_stat) {

	case RPC_MISMATCH:
		if (! xdr_u_long(xdrs, &(rr->rj_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(rr->rj_vers.high)));

	case AUTH_ERROR:
		return (xdr_enum(xdrs, (enum_t *)&(rr->rj_why)));
	}
	return (FALSE);
}


/* BEGIN_IMS xdr_replymsg *
 ********************************************************************
 ****
 **** 			xdr_replymsg(xdrs, rmsg)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	rmsg	pointer to RPC reply messasge
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	Encode/decode a reply message.
 *
 * Algorithm
 *	if (op == XDR_ENCODE and reply == MSG_ACCEPTED 
 *	    and direction == REPLY && XDR_INLINE != NULL) {
 *		put xid, direction, status directly onto stream
 *		put verifier info directly onto stream
 *		put accepted reply status on stream
 *		switch (accepted reply status) {
 *			case SUCCESS:
 *				return (xdr results)
 *			case PROG_MISMATCH:
 *				return (xdr version info)
 *		}
 *		return (TRUE)
 *	}
 *	if (op == XDR_DECODE and XDR_INLINE != NULL) {
 *		get xid, direction from stream
 *		if (direction != REPLY)
 *			return(FALSE)
 *		get reply status
 *		if (reply status == MSG_DENIED)
 *			return(xdr_rejected_reply)
 *		get verifier info from stream
 *		get accepted reply status from stream
 *		switch (accepted reply status) {
 *			case SUCCESS:
 *				return (xdr results)
 *			case PROG_MISMATCH:
 *				return (xdr version info)
 *		}
 *		return (TRUE)
 *	}
 *	call xdr routines for each of the reply message fields to
 *		encode/decode the hard way
 *	if (all xdr routines succeed)
 *		return (TRUE)
 *	return (FALSE)
 *
 * Concurrency
 *	none
 *
 * Modification History
 *	11/19/87	lmb	added trigger
 *
 * External Calls
 *	XDR_INLINE
 *	IXDR_PUT_LONG, IXDR_GET_LONG
 *	IXDR_PUT_ENUM, IXDR_GET_ENUM
 *	bcopy, mem_alloc
 *	(*results.proc)
 *	xdr_u_long, xdr_enum, xdr_opaque, xdr_union
 *	xdr_rejected_reply
 *
 * Called By
 *	clntkudp_callit
 *	svckudp_send
 *
 ********************************************************************
 ********************************************************************
 * END_IMS xdr_replymsg */

static struct xdr_discrim reply_dscrm[3] = {
	{ (int)MSG_ACCEPTED, xdr_accepted_reply },
	{ (int)MSG_DENIED, xdr_rejected_reply },
	{ __dontcare__, NULL_xdrproc_t } };

#ifdef _NAMESPACE_CLEAN
#undef xdr_replymsg
#pragma _HP_SECONDARY_DEF _xdr_replymsg xdr_replymsg
#define xdr_replymsg _xdr_replymsg
#endif

bool_t
xdr_replymsg(xdrs, rmsg)
	register XDR *xdrs;
	register struct rpc_msg *rmsg;
{
	register long *buf;
	register struct accepted_reply *ar;
	register struct opaque_auth *oa;

	if (xdrs->x_op == XDR_ENCODE &&
	    rmsg->rm_reply.rp_stat == MSG_ACCEPTED &&
	    rmsg->rm_direction == REPLY &&
	    (buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT +
	    rmsg->rm_reply.rp_acpt.ar_verf.oa_length)) != NULL) {
		IXDR_PUT_LONG(buf, rmsg->rm_xid);
		IXDR_PUT_ENUM(buf, rmsg->rm_direction);
		IXDR_PUT_ENUM(buf, rmsg->rm_reply.rp_stat);
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		if (oa->oa_length) {
#if ! defined(KERNEL)
			memcpy(buf, oa->oa_base, oa->oa_length);
#else /* hpux */
			bcopy((caddr_t)oa->oa_base, (caddr_t)buf, oa->oa_length);
#endif /* hpux */
			buf += (oa->oa_length +
				BYTES_PER_XDR_UNIT - 1) /
				sizeof (long);
		}
		/*
		 * stat and rest of reply, copied from xdr_accepted_reply
		 */
		IXDR_PUT_ENUM(buf, ar->ar_stat);
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where));
	
		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
	if (xdrs->x_op == XDR_DECODE &&
	    (buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT)) != NULL) {
		rmsg->rm_xid = IXDR_GET_LONG(buf);
		rmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
		if (rmsg->rm_direction != REPLY) {
			return (FALSE);
		}
		rmsg->rm_reply.rp_stat = IXDR_GET_ENUM(buf, enum reply_stat);
		if (rmsg->rm_reply.rp_stat != MSG_ACCEPTED) {
			if (rmsg->rm_reply.rp_stat == MSG_DENIED) {
				return (xdr_rejected_reply(xdrs,
					&rmsg->rm_reply.rp_rjct));
			}
			return (FALSE);
		}
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
#ifdef NTRIGGER
		if (utry_trigger(T_NFS_SET_INLINE, T_NFS_PROC, 0, 0)) {
			set_trigger(T_NFS_XDRMBUF_INLINE, T_NFS_PROC,1,1,0,0);
		}
#endif /* NTRIGGER */
		buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
		} else {
			if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
			    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
				return (FALSE);
			}
		}
		if (oa->oa_length) {
			if (oa->oa_length > MAX_AUTH_BYTES) {
				return (FALSE);
			}
			if (oa->oa_base == NULL) {
				oa->oa_base = (caddr_t)
					mem_alloc(oa->oa_length);
			}
			buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
			if (buf == NULL) {
				if (xdr_opaque(xdrs, oa->oa_base,
				    oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
#if ! defined(KERNEL)
				memcpy(oa->oa_base, buf, oa->oa_length);
#else /* hpux */
				bcopy((caddr_t)buf, (caddr_t)oa->oa_base, oa->oa_length);
#endif /* hpux */
				/* no real need....
				buf += RNDUP(oa->oa_length) / sizeof (long);
				*/
			}
		}
		/*
		 * stat and rest of reply, copied from
		 * xdr_accepted_reply
		 */
		xdr_enum(xdrs, (enum_t *)&ar->ar_stat);
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where));

		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
	if (
    	    xdr_u_long(xdrs, &(rmsg->rm_xid)) && 
	    xdr_enum(xdrs, (enum_t *)&(rmsg->rm_direction)) &&
	    (rmsg->rm_direction == REPLY) )
		return (xdr_union(xdrs, (enum_t *)&(rmsg->rm_reply.rp_stat),
		    (caddr_t)&(rmsg->rm_reply.ru), reply_dscrm, NULL_xdrproc_t));
	return (FALSE);
}


/* BEGIN_IMS xdr_callmsg *
 ********************************************************************
 ****
 **** 			xdr_callmsg(xdrs, cmsg)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	cmsg	pointer to RPC call messasge
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	Encode/decode a call message.
 *
 * Algorithm
 *	if (op == XDR_ENCODE and XDR_INLINE != NULL) {
 *		put xid, direction, etc. directly onto stream
 *		put program, procedure directly onto stream
 *		put authentication and verifier onto stream
 *		return (TRUE)
 *	}
 *	if (op == XDR_DECODE and XDR_INLINE != NULL) {
 *		get xid, direction, rpc version from stream
 *		if (direction != CALL)
 *			return(FALSE)
 *		if (rpc version != RPC_MSG_VERSION)
 *			return(FALSE)
 *		get program, version, procedure
 *		get authentication and verifier from stream
 *		return (TRUE)
 *	}
 *	call xdr routines for each of the call message fields to
 *		encode/decode the hard way
 *	if (all xdr routines succeed)
 *		return (TRUE)
 *	return (FALSE)
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	XDR_INLINE
 *	RNDUP
 *	IXDR_PUT_LONG, IXDR_GET_LONG
 *	IXDR_PUT_ENUM, IXDR_GET_ENUM
 *	bcopy, mem_alloc
 *	xdr_u_long, xdr_enum, xdr_opaque, xdr_opaque_auth
 *
 * Called By
 *	svckudp_recv
 *
 ********************************************************************
 * END_IMS xdr_callmsg */

#ifdef _NAMESPACE_CLEAN
#undef xdr_callmsg
#pragma _HP_SECONDARY_DEF _xdr_callmsg xdr_callmsg
#define xdr_callmsg _xdr_callmsg
#endif

bool_t
xdr_callmsg(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{
	register long *buf;
	register struct opaque_auth *oa;

#ifndef KERNEL
	if (xdrs->x_op == XDR_ENCODE) {
		if (cmsg->rm_call.cb_cred.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		if (cmsg->rm_call.cb_verf.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_cred.oa_length)
			+ 2 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_verf.oa_length));
		if (buf != NULL) {
			IXDR_PUT_LONG(buf, cmsg->rm_xid);
			IXDR_PUT_ENUM(buf, cmsg->rm_direction);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_rpcvers);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_prog);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_vers);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_proc);
			oa = &cmsg->rm_call.cb_cred;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
#if ! defined(KERNEL)
				memcpy(buf, oa->oa_base, oa->oa_length);
#else /* hpux */
				bcopy(oa->oa_base, buf, oa->oa_length);
#endif /* hpux */
				buf += RNDUP(oa->oa_length) / sizeof (long);
			}
			oa = &cmsg->rm_call.cb_verf;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
#if ! defined(KERNEL)
				memcpy(buf, oa->oa_base, oa->oa_length);
#else /* hpux */
				bcopy(oa->oa_base, buf, oa->oa_length);
#endif /* hpux */
				/* no real need....
				buf += RNDUP(oa->oa_length) / sizeof (long);
				*/
			}
			return (TRUE);
		}
	}
#endif /* !KERNEL */
	if (xdrs->x_op == XDR_DECODE) { 
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			cmsg->rm_xid = IXDR_GET_LONG(buf);
			cmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			cmsg->rm_call.cb_rpcvers = IXDR_GET_LONG(buf);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			cmsg->rm_call.cb_prog = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_vers = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_proc = IXDR_GET_LONG(buf);
			oa = &cmsg->rm_call.cb_cred;
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = (caddr_t)
						mem_alloc(oa->oa_length);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
#if ! defined(KERNEL)
					memcpy(oa->oa_base, buf, oa->oa_length);
#else /* hpux */
					bcopy((caddr_t)buf, (caddr_t)oa->oa_base, oa->oa_length);
#endif /* hpux */

					/* no real need....
					buf += RNDUP(oa->oa_length) /
						sizeof (long);
					*/
				}
			}
			oa = &cmsg->rm_call.cb_verf;
			buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
			if (buf == NULL) {
				if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
				    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
				oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
				oa->oa_length = IXDR_GET_LONG(buf);
			}
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = (caddr_t)
						mem_alloc(oa->oa_length);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
#if ! defined(KERNEL)
					memcpy(oa->oa_base, buf, oa->oa_length);
#else /* hpux */
					bcopy((caddr_t)buf, (caddr_t)oa->oa_base, oa->oa_length);
#endif /* hpux */
					/* no real need...
					buf += RNDUP(oa->oa_length) /
						sizeof (long);
					*/
				}
			}
			return (TRUE);
		}
	}
	if (
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    (cmsg->rm_direction == CALL) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    (cmsg->rm_call.cb_rpcvers == RPC_MSG_VERSION) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_proc)) &&
	    xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_cred)) )
	    return (xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_verf)));
	return (FALSE);
}


/* BEGIN_IMS xdr_callhdr *
 ********************************************************************
 ****
 **** 			xdr_callhdr(xdrs, cmsg)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs	pointer to XDR handle
 *	cmsg	pointer to RPC call message
 *
 * Output Parameters
 * 	none 
 *
 * Return Value
 *	TRUE	success
 *	FALSE	failure
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	xdr_callhdr serializes the "static part" of a call message header.
 * 	The fields include: rm_xid, rm_direction, rpcvers, prog, and vers.
 * 	The rm_xid is not really static, but the user can easily munge on
 *	the fly.
 *
 * Algorithm
 *	message direction = CALL
 *	RPC version = RPC_MSG_VERSION
 *	if (op == ENCODE and
 *	    putting xid on stream succeeds and
 *	    putting direction on stream succeeds and
 *	    putting rpc_version on stream succeeds and
 *	    putting program number on stream succeeds)
 *		return (xdr program version)
 *	return (FALSE)
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	xdr_u_long
 *	xdr_enum
 *
 * Called By
 *	clntkudp_create
 *
 ********************************************************************
 * END_IMS xdr_callhdr */

#ifdef _NAMESPACE_CLEAN
#undef xdr_callhdr
#pragma _HP_SECONDARY_DEF _xdr_callhdr xdr_callhdr
#define xdr_callhdr _xdr_callhdr
#endif

bool_t
xdr_callhdr(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{

	cmsg->rm_direction = CALL;
	cmsg->rm_call.cb_rpcvers = RPC_MSG_VERSION;
	if (
	    (xdrs->x_op == XDR_ENCODE) &&
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) )
	    return (xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)));
	return (FALSE);
}


/* ************************** Client utility routine ************* */

/* BEGIN_IMS accepted *
 ********************************************************************
 ****
 **** 			accepted(acpt_stat, error)
 ****
 ********************************************************************
 * Input Parameters
 *	acpt_stat	accepted call return status
 *	error		pointer to RPC error structure
 *
 * Output Parameters
 * 	status field of RPC error structure 
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	accepted is a utility routine that fills in the status field
 *	of the RPC error structure for an accepted RPC call when a
 *	reply message is received.
 *
 * Algorithm
 *	switch (accept status) {
 *		case PROG_UNAVAIL:
 *			status = RPC_PROGUNAVAIL; return
 *		case PROG_MISMATCH:
 *			status = RPC_PROGVERSMISMATCH; return
 *		case PROC_UNAVAIL:
 *			status = RPC_PROCUNAVAIL; return
 *		case PROC_UNAVAIL:
 *			status = RPC_PROCUNAVAIL; return
 *		case GARBAGE_ARGS:
 *			status = RPC_CANTDECODEARGS; return
 *		case SYSTEM_ERR:
 *			status = SYSTEM_ERR; return
 *		case SUCCESS:
 *			status = RPC_SUCCESS; return
 *	}
 *	status = RPC_FAILED
 *	give MSG_ACCEPTED and accept status
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	none
 *
 * Called By
 *	_seterr_reply
 *
 ********************************************************************
 * END_IMS accepted */

static void
accepted(acpt_stat, error)
	register enum accept_stat acpt_stat;
	register struct rpc_err *error;
{

	switch (acpt_stat) {

	case PROG_UNAVAIL:
		error->re_status = RPC_PROGUNAVAIL;
		return;

	case PROG_MISMATCH:
		error->re_status = RPC_PROGVERSMISMATCH;
		return;

	case PROC_UNAVAIL:
		error->re_status = RPC_PROCUNAVAIL;
		return;

	case GARBAGE_ARGS:
		error->re_status = RPC_CANTDECODEARGS;
		return;

	case SYSTEM_ERR:
		error->re_status = RPC_SYSTEMERROR;
		return;

	case SUCCESS:
		error->re_status = RPC_SUCCESS;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_ACCEPTED;
	error->re_lb.s2 = (long)acpt_stat;
}


/* BEGIN_IMS rejected *
 ********************************************************************
 ****
 **** 			rejected(rjct_stat, error)
 ****
 ********************************************************************
 * Input Parameters
 *	rjct_stat	rejected call return status
 *	error		pointer to RPC error structure
 *
 * Output Parameters
 * 	status field of RPC error structure 
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	rejected is a utility routine that fills in the status field
 *	of the RPC error structure for an rejected RPC call when a
 *	reply message is received.
 *
 * Algorithm
 *	switch (rejected status) {
 *		case RPC_MISMATCH:
 *			status = RPC_VERSMISMATCH; return
 *		case AUTH_ERROR:
 *			status = RPC_AUTHERROR; return
 *	}
 *	status = RPC_FAILED
 *	give MSG_DENIED and rejected status
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	none
 *
 * Called By
 *	_seterr_reply
 ********************************************************************
 * END_IMS rejected */

static void 
rejected(rjct_stat, error)
	register enum reject_stat rjct_stat;
	register struct rpc_err *error;
{

	switch (rjct_stat) {

	case RPC_MISMATCH:
		error->re_status = RPC_VERSMISMATCH;
		return;

	case AUTH_ERROR:
		error->re_status = RPC_AUTHERROR;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_DENIED;
	error->re_lb.s2 = (long)rjct_stat;
}


/* BEGIN_IMS _seterr_reply *
 ********************************************************************
 ****
 **** 			_seterr_reply(msg, error)
 ****
 ********************************************************************
 * Input Parameters
 *	msg		pointer to RPC message
 *	error		pointer to RPC error structure
 *
 * Output Parameters
 * 	fields of RPC error structure 
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	Given a reply message, _seterr_reply gets the return status
 *	for accepted and denied RPC calls, plus any additional error
 *	information that may be provided.
 *
 * Algorithm
 *	switch (reply status) {
 *		case MSG_ACCEPTED:
 *			if (status == SUCCESS) {
 *				status = RPC_SUCCESS
 *				return
 *			}
 *			get accepted status
 *			break
 *		case MSG_DENIED:
 *			get rejected status
 *			break
 *		default:
 *			status = RPC_FAILED
 *	}
 *	switch (status) {
 *		case RPC_VERSMISMATCH:
 *			get more version 
 *		case RPC_AUTHERROR:
 *			get reason
 *		case RPC_PROGVERSMISMATCH:
 *			get more version 
 *	}
 *
 * Concurrency
 *	none
 *
 * Modification History
 *
 * External Calls
 *	accepted
 *	rejected
 *
 * Called By
 *	clntkudp_callit
 *
 ********************************************************************
 * END_IMS _seterr_reply */

void
_seterr_reply(msg, error)
	register struct rpc_msg *msg;
	register struct rpc_err *error;
{

	/* optimized for normal, SUCCESSful case */
	switch (msg->rm_reply.rp_stat) {

	case MSG_ACCEPTED:
		if (msg->acpted_rply.ar_stat == SUCCESS) {
			error->re_status = RPC_SUCCESS;
			return;
		};
		accepted(msg->acpted_rply.ar_stat, error);
		break;

	case MSG_DENIED:
		rejected(msg->rjcted_rply.rj_stat, error);
		break;

	default:
		error->re_status = RPC_FAILED;
		error->re_lb.s1 = (long)(msg->rm_reply.rp_stat);
		break;
	}
	switch (error->re_status) {

	case RPC_VERSMISMATCH:
		error->re_vers.low = msg->rjcted_rply.rj_vers.low;
		error->re_vers.high = msg->rjcted_rply.rj_vers.high;
		break;

	case RPC_AUTHERROR:
		error->re_why = msg->rjcted_rply.rj_why;
		break;

	case RPC_PROGVERSMISMATCH:
		error->re_vers.low = msg->acpted_rply.ar_vers.low;
		error->re_vers.high = msg->acpted_rply.ar_vers.high;
		break;
	}
}




/* BEGIN_IMS rpc_badpacket *
 ********************************************************************
 ****
 **** 		rpc_badpacket(which, procnum, xid, auth)
 ****
 ********************************************************************
 * Input Parameters
 *	which		error case to trigger
 *	procnum		NFS procedure number to use
 *	xid		transaction id to use for packet
 *	auth		user authentication information
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	NULL		failure to produce a bad packet
 *	address		mbuf chain containing encoded packet
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This is a TRIGGER only routine. It trashes an outgoing call
 *	packet in a well-defined way. It is primarily intended to 
 *	increase code coverage on the client side. It uses the ENCODE
 *	case of xdr_callmsg, a path that is never hit otherwise.
 *
 * Algorithm
 *	get an mbuf for rpc call message
 *	initialize XDR stream
 *	initialize fields of call message
 *	switch (which) {
 *		case 1:	 bad xid
 *		case 2:	 RPC_PROGUNAVAIL
 *		case 3:	 RPC_PROGVERSMISMATCH
 *		case 4:	 RPC_PROCUNAVAIL
 *		case 5:  direction = REPLY
 *		case 6:	 RPC_MISMATCH 
 *		case 7:	 RPC_AUTHERROR
 *	}
 *	XDR call message
 *	return (mbuf)
 *
 * Modification History
 *	11/6/87		lmb		wrote
 *
 * External Calls
 *	xdrmbuf_init
 *	MGET
 *	xdr_callmsg
 *
 * Called By
 *	clntkudp_callit
 *
 ********************************************************************
 * END_IMS rpc_badpacket */

#ifdef NTRIGGER
#include "../h/mbuf.h"

struct mbuf*
rpc_badpacket(which, procnum, xid, auth)
	int which;
	u_long procnum;
	u_long xid;
	AUTH *auth;
{
	struct mbuf *m;
	XDR xdrs;
	struct rpc_msg call_msg;
	struct opaque_auth *oa;
	long *buf;

/*-----------------------------------------------------------------------------
	MGET(m, M_WAIT, 0, MT_DATA);
	 * Changed interface
-----------------------------------------------------------------------------*/
	MGET(m, M_WAIT, MT_DATA);
	if (m == NULL) 
		return(m);
	m->m_off = MMINOFF;
	m->m_len = sizeof(struct rpc_msg) + 4;	/* a little data */
	xdrmbuf_init(&xdrs, m, XDR_ENCODE);

	call_msg.rm_xid = xid;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = ((u_long)100003); 	/* NFS_PROGRAM */
	call_msg.rm_call.cb_vers = ((u_long) 2);	/* NFS_VERSION */
	call_msg.rm_call.cb_proc = procnum;
	call_msg.rm_call.cb_cred = _null_auth;
	call_msg.rm_call.cb_verf = auth->ah_verf;

	switch (which) {
		case 1:		/* bad xid */
			call_msg.rm_xid = 0;	
			break;
		case 2:		/* RPC_PROGUNAVAIL */
			call_msg.rm_call.cb_prog = 105; 
			break;
		case 3:		/* RPC_PROGVERSMISMATCH */	
			call_msg.rm_call.cb_vers = 5;
			break;
		case 4:		/* RPC_PROCUNAVAIL */
			call_msg.rm_call.cb_proc = 999;
			break;
		case 5:	
			call_msg.rm_direction = REPLY;
			break;
		case 6:		/* RPC_MISMATCH */
			call_msg.rm_call.cb_rpcvers = 0;
			break;
		case 7:		/* RPC_AUTHERROR */
			call_msg.rm_call.cb_cred.oa_flavor = 5;
			break;
		default:
			break;
	}
	buf = XDR_INLINE(&xdrs, 11 * BYTES_PER_XDR_UNIT);
	if (buf != NULL) {
		IXDR_PUT_LONG(buf, call_msg.rm_xid);
		IXDR_PUT_ENUM(buf, call_msg.rm_direction);
		IXDR_PUT_LONG(buf, call_msg.rm_call.cb_rpcvers);
		IXDR_PUT_LONG(buf, call_msg.rm_call.cb_prog);
		IXDR_PUT_LONG(buf, call_msg.rm_call.cb_vers);
		IXDR_PUT_LONG(buf, call_msg.rm_call.cb_proc);
		oa = &call_msg.rm_call.cb_cred;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		oa = &call_msg.rm_call.cb_verf;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		IXDR_PUT_LONG(buf, 0);			/* some data */
	}
	else {
		m_freem(m);
		m = NULL;
	}
	return(m);
}
#endif /* NTRIGGER */
