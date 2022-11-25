/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/xdr.c,v $
 * $Revision: 1.10.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 22:49:30 $
 */

/* BEGIN_IMS_MOD   
******************************************************************************
****								
****	xdr - the basic XDR encoding/decoding routines
****								
******************************************************************************
* Description
*	This module contains all the generic XDR (eXternal Data Representation) 
* 	routines for encoding and decoding the basic data types.
*
* Externally Callable Routines
*	xdr_void - null routine for absence of data type
*	xdr_int - encode/decode an int
*	xdr_u_int - encode/decode an unsigned int
*	xdr_long - encode/decode a long
*	xdr_u_long - encode/decode an unsigned long
*	xdr_short - encode/decode a short integer
*	xdr_u_short - encode/decode an unsigned short integer
*	xdr_bool - encode/decode a boolean type
*	xdr_enum - encode/decode an enumerated type
*	xdr_opaque - encode/decode an opaque type
*	xdr_bytes - encode/decode a sequence of bytes
*	xdr_union - encode/decode a union type
*	xdr_string - encode/decode a string
*	xdr_wrapstring - call xdr_string with maxsize set to LASTUNSIGNED
*
*	xdr_free - function to call a user's freeing function
*	xdr_char - encode/decode a char
*	xdr_u_char - encode/decode an unsigned char
*
* Test Routine
*	test_nfs - NS_QA test routine to hit various XDR code
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
*
******************************************************************************
* END_IMS_MOD */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr.c,v 1.10.83.4 93/12/06 22:49:30 marshall Exp $ (Hewlett-Packard)";
#endif

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
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

#define catgets 	_catgets
#define fprintf 	_fprintf
#define strlen 		_strlen
#define xdr_bool 	_xdr_bool		/* In this file */
#define xdr_bytes 	_xdr_bytes		/* In this file */
#define xdr_char 	_xdr_char		/* In this file */
#define xdr_enum 	_xdr_enum		/* In this file */
#define xdr_free 	_xdr_free		/* In this file */
#define xdr_int 	_xdr_int		/* In this file */
#define xdr_long 	_xdr_long		/* In this file */
#define xdr_netobj 	_xdr_netobj		/* In this file */
#define xdr_opaque 	_xdr_opaque		/* In this file */
#define xdr_short 	_xdr_short		/* In this file */
#define xdr_string 	_xdr_string		/* In this file */
#define xdr_u_char 	_xdr_u_char		/* In this file */
#define xdr_u_int 	_xdr_u_int		/* In this file */
#define xdr_u_long 	_xdr_u_long		/* In this file */
#define xdr_u_short 	_xdr_u_short		/* In this file */
#define xdr_union 	_xdr_union		/* In this file */
#define xdr_void 	_xdr_void		/* In this file */
#define xdr_wrapstring 	_xdr_wrapstring		/* In this file */

#endif _NAME_SPACE_CLEAN

#ifdef _KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"


#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#else /* not _KERNEL */

#define NL_SETN 17	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <stdio.h>
char *malloc();
#endif /* _KERNEL */

/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE	((long) 0)
#define XDR_TRUE	((long) 1)
#define LASTUNSIGNED	((u_int) 0-1)

/*
 * for unit alignment
 */
static char xdr_zero[BYTES_PER_XDR_UNIT] = { 0, 0, 0, 0 };



/* BEGIN_IMS xdr_void *
 ******************************************************************************
 ****
 ****		xdr_void()
 ****
 ******************************************************************************
 * Input Parameters
 *	none
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	TRUE		always
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	If only all routines were like this one !!
 *
 * Algorithm
 *	what algorithm??
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
 ******************************************************************************
 * END_IMS xdr_void */

#ifdef _NAMESPACE_CLEAN
#undef xdr_void
#pragma _HP_SECONDARY_DEF _xdr_void xdr_void
#define xdr_void _xdr_void
#endif

bool_t
xdr_void(/* xdrs, addr */)
	/* XDR *xdrs; */
	/* caddr_t addr; */
{

	return (TRUE);
}


/* BEGIN_IMS xdr_int *
 ******************************************************************************
 ****
 ****		xdr_int(xdrs,ip)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ip		pointer to an integer
 *
 * Output Parameters
 *	*ip		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an integer onto/from an xdr stream.
 *
 * Algorithm
 *	{ if (sizeof(int) == sizeof(long))
 *		call xdr_long
 *	 else
 *		call xdr_short
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
 *	xdr_long
 *	xdr_short
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_int */

#ifdef _NAMESPACE_CLEAN
#undef xdr_int
#pragma _HP_SECONDARY_DEF _xdr_int xdr_int
#define xdr_int _xdr_int
#endif

bool_t
xdr_int(xdrs, ip)
	XDR *xdrs;
	int *ip;
{

#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	return (xdr_long(xdrs, (long *)ip));
#else
	if (sizeof (int) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ip));
	} else {
		return (xdr_short(xdrs, (short *)ip));
	}
#endif
}


/* BEGIN_IMS xdr_u_int *
 ******************************************************************************
 ****
 ****		xdr_u_int(xdrs,up)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the  routines
 *	up		pointer to unsigned integer
 *
 * Output Parameters
 *	*up		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending on the success of the operation
 *
 * Globals Referenced	
 *	none
 *
 * Description
 *	depending upon the size of the unsigned interger this routine
 *	calls xdr_u_short or xdr_u_long.
 *
 * Algorithm
 *	{ if (sizeof(u_int) == sizeof(u_long))
 *		call xdr_u_long
 *	  else
 *		call xdr_u_short
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. xdr_short should be changed to xdr_u_short
 *	. ifdef lint code is wrong
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_u_short
 *	xdr_u_long
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_u_int */

#ifdef _NAMESPACE_CLEAN
#undef xdr_u_int
#pragma _HP_SECONDARY_DEF _xdr_u_int xdr_u_int
#define xdr_u_int _xdr_u_int
#endif

bool_t
xdr_u_int(xdrs, up)
	XDR *xdrs;
	u_int *up;
{

#ifdef lint
	(void) (xdr_short(xdrs, (short *)up));
	return (xdr_u_long(xdrs, (u_long *)up));
#else
	if (sizeof (u_int) == sizeof (u_long)) {
		return (xdr_u_long(xdrs, (u_long *)up));
	} else {
		return (xdr_short(xdrs, (short *)up));
	}
#endif
}


/* BEGIN_IMS xdr_long *
 ******************************************************************************
 ****
 ****		xdr_long(xdrs,lp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	lp		pointer to a long integer
 *
 * Output Parameters
 *	*lp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an long onto/from an xdr stream.
 *
 * Algorithm
 *	{ switch(xdrs->x_op) {
 *	  	case XDR_ENCODE: 	return(XDR_PUTLONG())
 *	  	case XDR_DECODE: 	return(XDR_GETLONG())
 *	  	case XDR_FREE:		return(TRUE)
 *	  	default:		return(FALSE)
 *	  }
 *	}
 *
 * Concurrency
 *	none
 
 * To Do List
 *
 * Notes
 *	XDR long integers same as xdr_u_long - open coded to save a proc call!
 *
 * Modification History
 *
 * External Calls
 *	XDR_PUTLONG
 *	XDR_GETLONG
 *	NS_LOG	
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_long */

#ifdef _NAMESPACE_CLEAN
#undef xdr_long
#pragma _HP_SECONDARY_DEF _xdr_long xdr_long
#define xdr_long _xdr_long
#endif

bool_t
xdr_long(xdrs, lp)
	register XDR *xdrs;
	long *lp;
{

	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, lp));

	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONG(xdrs, lp));

	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

#ifdef _KERNEL
	NS_LOG(LE_NFS_LONG_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_u_long *
 ******************************************************************************
 ****
 ****		xdr_u_long(xdrs,ulp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ulp		pointer to a unsigned long integer
 *
 * Output Parameters
 *	*ulp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an u_long onto/from an xdr stream.
 *	The xdr representation for u_long is a long.
 *
 * Algorithm
 *	{ switch(xdrs->x_op) {
 *		coerce u_long into long
 *	  	case XDR_ENCODE: 	return(XDR_PUTLONG())
 *	  	case XDR_DECODE: 	return(XDR_GETLONG())
 *	  	case XDR_FREE:		return(TRUE)
 *	  	default:		return(FALSE)
 *	  }
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	XDR unsigned long integers same as xdr_long - open coded to save a
 *	proc call!
 *
 * Modification History
 *
 * External Calls
 *	XDR_PUTLONG
 *	XDR_GETLONG
 *	NS_LOG	
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_u_long */

#ifdef _NAMESPACE_CLEAN
#undef xdr_u_long
#pragma _HP_SECONDARY_DEF _xdr_u_long xdr_u_long
#define xdr_u_long _xdr_u_long
#endif

bool_t
xdr_u_long(xdrs, ulp)
	register XDR *xdrs;
	u_long *ulp;
{

	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONG(xdrs, (long *)ulp));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, (long *)ulp));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);
#ifdef _KERNEL
	NS_LOG(LE_NFS_U_LONG_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_short *
 ******************************************************************************
 ****
 ****		xdr_short(xdrs,sp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	sp		pointer to a short integer
 *
 * Output Parameters
 *	*sp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an short onto/from an xdr stream.
 *	The xdr representation for short is a long.
 *
 * Algorithm
 *	{ switch(xdrs->x_op) {
 *		case XDR_ENCODE:	coerce into long
 *					return(XDR_PUTLONG())
 *		case XDR_DECODE:	if (XDR_GETLONG())
 *						coerce into short
 *						return(TRUE)
 *					else
 *						return(FALSE)
 *		case XDR_FREE:		return(TRUE)
 *		default:		return(FALSE)
 *	  }
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
 *	XDR_GETLONG
 *	XDR_PUTLONG
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_short */

#ifdef _NAMESPACE_CLEAN
#undef xdr_short
#pragma _HP_SECONDARY_DEF _xdr_short xdr_short
#define xdr_short _xdr_short
#endif

bool_t
xdr_short(xdrs, sp)
	register XDR *xdrs;
	short *sp;
{
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *sp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*sp = (short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}


/* BEGIN_IMS xdr_u_short *
 ******************************************************************************
 ****
 ****		xdr_u_short(xdrs,usp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	usp		pointer to a short integer
 *
 * Output Parameters
 *	*usp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an u_short onto/from an xdr stream.
 *	The xdr representation for u_short is a long.
 *
 * Algorithm
 *	{ switch(xdrs->x_op) {
 *		case XDR_ENCODE:	coerce into long
 *					return(XDR_PUTLONG())
 *		case XDR_DECODE:	if (XDR_GETLONG())
 *						coerce into u_short
 *						return(TRUE)
 *					else
 *						return(FALSE)
 *		case XDR_FREE:		return(TRUE)
 *		default:		return(FALSE)
 *	  }
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. coercion takes place to a u_long instead of long !!?
 *
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	XDR_GETLONG
 *	XDR_PUTLONG
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_u_short */

#ifdef _NAMESPACE_CLEAN
#undef xdr_u_short
#pragma _HP_SECONDARY_DEF _xdr_u_short xdr_u_short
#define xdr_u_short _xdr_u_short
#endif

bool_t
xdr_u_short(xdrs, usp)
	register XDR *xdrs;
	u_short *usp;
{
	u_long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *usp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
#ifdef _KERNEL
			NS_LOG(LE_NFS_U_SHORT_DECODE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
			return (FALSE);
		}
		*usp = (u_short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
#ifdef _KERNEL
	NS_LOG(LE_NFS_U_SHORT_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}



/* BEGIN_IMS xdr_bool *
 ******************************************************************************
 ****
 ****		xdr_bool(xdrs,bp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	bp		pointer to a bool_t 
 *
 * Output Parameters
 *	*bp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an bool onto/from an xdr stream.
 *	The xdr representation for bool is a long.
 *
 * Algorithm
 *	{ switch(xdrs->x_op) {
 *		case XDR_ENCODE:	coerce into long
 *					return(XDR_PUTLONG())
 *		case XDR_DECODE:	if (XDR_GETLONG())
 *						coerce into bool
 *						return(TRUE)
 *					else
 *						return(FALSE)
 *		case XDR_FREE:		return(TRUE)
 *		default:		return(FALSE)
 *	  }
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
 *	XDR_GETLONG
 *	XDR_PUTLONG
 *	NS_LOG
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_bool */

#ifdef _NAMESPACE_CLEAN
#undef xdr_bool
#pragma _HP_SECONDARY_DEF _xdr_bool xdr_bool
#define xdr_bool _xdr_bool
#endif

bool_t
xdr_bool(xdrs, bp)
	register XDR *xdrs;
	bool_t *bp;
{
	long lb;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		lb = *bp ? XDR_TRUE : XDR_FALSE;
		return (XDR_PUTLONG(xdrs, &lb));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &lb)) {
#ifdef _KERNEL
			NS_LOG(LE_NFS_BOOL_DECODE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
			return (FALSE);
		}
		*bp = (lb == XDR_FALSE) ? FALSE : TRUE;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
#ifdef _KERNEL
	NS_LOG(LE_NFS_BOOL_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_enum *
 ******************************************************************************
 ****
 ****		xdr_enum(xdrs,ep)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ep		pointer to a enum_t
 *
 * Output Parameters
 *	*ep		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an enum_t onto/from an xdr stream.
 *	The xdr representation for enum_t is a long.
 *
 * Algorithm
 *	{ if (sizeof(enum) == sizeof(long))
 *		coerce enum into long
 *		return(xdr_long())
 *	  else if (sizeof(enum) == sizeof(short))
 *		coerce enum into short
 *		return(xdr_short())
 *	  else
 *		return(FALSE)
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
 *	xdr_long
 *	xdr_short
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_enum */

#ifdef _NAMESPACE_CLEAN
#undef xdr_enum
#pragma _HP_SECONDARY_DEF _xdr_enum xdr_enum
#define xdr_enum _xdr_enum
#endif

bool_t
xdr_enum(xdrs, ep)
	XDR *xdrs;
	enum_t *ep;
{
#ifndef lint
	enum sizecheck { SIZEVAL };	/* used to find the size of an enum */
#endif

	/*
	 * enums are treated as ints
	 */
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ep));
	return (xdr_long(xdrs, (long *)ep));
#else
	if (sizeof (enum sizecheck) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ep));
	} else if (sizeof (enum sizecheck) == sizeof (short)) {
		return (xdr_short(xdrs, (short *)ep));
	} else {
		return (FALSE);
	}
#endif
}


/* BEGIN_IMS xdr_opaque *
 ******************************************************************************
 ****
 ****		xdr_opaque(xdrs,cp,cnt)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cp		pointer to the opaque object
 *	cnt		actual number of bytes
 *
 * Output Parameters
 *	*cp		this might change depending upon the value of
 *			xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	BYTES_PER_XDR_UNIT
 *
 * Description
 *	This allows for the specification of a fixed size sequence of
 *	opaque bytes. Since this has to sent as an integral quantity
 *	of xdr units, some padding/unpadding might be necessary.
 *
 * Algorithm
 *	{ return(TRUE) if there is no data (ie cnt == 0)
 *	  calculate how much padding/depadding is necessary
 *	  switch(xdrs->x_op) {
 *	  case XDR_DECODE:	get cnt bytes off stream
 *				make sure about depadding
 *				return value
 *	  case XDR_ENCODE:	put cnt bytes on stream
 *				make sure about padding
 *				return value
 *	  }
 *	  return(FALSE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. declaration of crud
 *	. move decleration of xdr_zero into this routine
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	NS_LOG
 *	XDR_GETBYTES
 *	XDR_PUTBYTES
 *
 * Called By (optional)
 *	xdr_bytes
 *	xdr_string
 *
 ******************************************************************************
 * END_IMS xdr_opaque */

#ifdef _NAMESPACE_CLEAN
#undef xdr_opaque
#pragma _HP_SECONDARY_DEF _xdr_opaque xdr_opaque
#define xdr_opaque _xdr_opaque
#endif

bool_t
xdr_opaque(xdrs, cp, cnt)
	register XDR *xdrs;
	caddr_t cp;
	register u_int cnt;
{
	register u_int rndup;
	static crud[BYTES_PER_XDR_UNIT];

	/*
	 * if no data we are done
	 */
	if (cnt == 0)
		return (TRUE);

	/*
	 * round byte count to full xdr units
	 */
	rndup = cnt % BYTES_PER_XDR_UNIT;
	if ((int)rndup > 0)
		rndup = BYTES_PER_XDR_UNIT - rndup;

	if (xdrs->x_op == XDR_DECODE) {
		if (!XDR_GETBYTES(xdrs, cp, cnt)) {
#ifdef _KERNEL
			NS_LOG(LE_NFS_OPAQUE_DECODE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_GETBYTES(xdrs, crud, rndup));
	}

	if (xdrs->x_op == XDR_ENCODE) {
		if (!XDR_PUTBYTES(xdrs, cp, cnt)) {
#ifdef _KERNEL
			NS_LOG(LE_NFS_OPAQUE_ENCODE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_PUTBYTES(xdrs, xdr_zero, rndup));
	}

	if (xdrs->x_op == XDR_FREE) {
		return (TRUE);
	}

#ifdef _KERNEL
	NS_LOG(LE_NFS_OPAQUE_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_bytes *
 ******************************************************************************
 ****
 ****		xdr_bytes(xdrs,cpp,sizep,maxsize)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cpp		pointer to the string (ie pointer to a char pointer)
 *	sizep		pointer to the length of the byte string
 *	maxsize		maximum permissible size of the string
 *
 * Output Parameters
 *	*sizep		
 *	cpp		if xdrs->x_op is XDR_DECODE or XDR_FREE then these 
 *			values may be changed
 *
 * Return Value
 *	TRUE/FALSE	depending on sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This is very similar to the routine xdr_string.The only difference
 *	is that instead of using strlen to obtain the length of the string,
 *	we now use *sizep. 
 *	Depending upon the value of xdr->x_op the encoding/decoding/freeing
 *	of space takes place. In the first two cases, the first integer on
 *	the byte stream is the size of the string.
 *
 * Algorithm
 *	{ if (xdrs->x_op != XDR_FREE)
 *		report error if (size > maxsize)
 *	  get/put size from/on the stream
 *	  switch(xdrs->x_op) {
 *	  case XDR_DECODE:	allocate space for string if necessary
 *				fall through
 *	  case XDR_ENCODE:	get/put string from/on the stream
 *				return value
 *	  case XDR_FREE:	free memory if pointer is not null
 *				return TRUE
 *	  }
 *	  return error
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. If (xdrs->x_op == XDR_ENCODE) && (size > maxsize) then
 *	  failure currently occurs after the size has been put on the
 *	  stream.
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	NS_LOG
 *	getenv
 *	mem_alloc
 *	mem_free
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_bytes */

#ifdef _NAMESPACE_CLEAN
#undef xdr_bytes
#pragma _HP_SECONDARY_DEF _xdr_bytes xdr_bytes
#define xdr_bytes _xdr_bytes
#endif

bool_t
xdr_bytes(xdrs, cpp, sizep, maxsize)
	register XDR *xdrs;
	char **cpp;
	register u_int *sizep;
	u_int maxsize;
{
	register char *sp = *cpp;  /* sp is the actual string pointer */
	register u_int nodesize;

#ifndef _KERNEL
	nlmsg_fd = _nfs_nls_catopen();
#endif not _KERNEL

	/*
	 * first deal with the length since xdr bytes are counted
	 */
	if (! xdr_u_int(xdrs, sizep)) {
#ifdef _KERNEL
		NS_LOG(LE_NFS_BYTES_SIZE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	nodesize = *sizep;
	if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE)) {
#ifdef _KERNEL
		NS_LOG(LE_NFS_BYTES_BAD,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0)
			return(TRUE);
		if (sp == NULL) {
			*cpp = sp = (char *)mem_alloc(nodesize);
		}
#ifndef _KERNEL
		if (sp == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "xdr_bytes: out of memory\n")));
			return (FALSE);
		}
#endif
		/* fall into ... */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, nodesize));

	case XDR_FREE:
		if (sp != NULL) {
			mem_free(sp, nodesize);
			*cpp = NULL;
		}
		return (TRUE);
	}
#ifdef _KERNEL
	NS_LOG(LE_NFS_BYTES_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


/*
 * Implemented here due to commonality of the object.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_netobj
#pragma _HP_SECONDARY_DEF _xdr_netobj xdr_netobj
#define xdr_netobj _xdr_netobj
#endif

bool_t
xdr_netobj(xdrs, np)
	XDR *xdrs;
	struct netobj *np;
{

	return (xdr_bytes(xdrs, &np->n_bytes, &np->n_len, MAX_NETOBJ_SZ));
}


/* BEGIN_IMS xdr_union *
 ******************************************************************************
 ****
 ****		xdr_union(xdrs, dscmp, unp, choices, dfault)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	dscmp		enum to decide which arm to work on
 *	unp		the union itself
 *	choices		[value, xdr_proc()] for each arm
 *	dfault		default xdr routine
 *
 * Output Parameters
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	NULL_xdrproc_t
 *
 * Description
 *	This routine goes through all choices comparing *dscmp to 
 *	choices->value and on a match, calls the appropriate routine.
 *	If there is no match, it calls the default routine, if any, else
 *	returns false. The array of xdr_discrim structures has to be 
 *	created by the user and is terminated with a null procedure
 *	pointer.
 *
 * Algorithm
 *	get/put the value of the discriminant (*dscmp) 
 *		on/to the xdr stream 
 *	loop until ((choices->proc == NULL_PROCEDURE) || MATCH_OCCURS)
 *	if MATCH_OCCURS 
 *		call the appropriate procedure
 *	else
 *		if (default_routine != NULL)
 *			call it
 *		else
 *			return(FALSE)
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. a potential infinite loop/data segmentation fault may occur 
 *	  if there is no null procedure !!
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	NS_LOG
 *	the appropriate routine as given in the xdr_discrim structure
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_union */

#ifdef _NAMESPACE_CLEAN
#undef xdr_union
#pragma _HP_SECONDARY_DEF _xdr_union xdr_union
#define xdr_union _xdr_union
#endif

bool_t
xdr_union(xdrs, dscmp, unp, choices, dfault)
	register XDR *xdrs;
	enum_t *dscmp;		/* enum to decide which arm to work on */
	caddr_t unp;		/* the union itself */
	struct xdr_discrim *choices;	/* [value, xdr proc] for each arm */
	xdrproc_t dfault;	/* default xdr routine */
{
	register enum_t dscm;

	/*
	 * we deal with the discriminator;  it's an enum
	 */
	if (! xdr_enum(xdrs, dscmp)) {
#ifdef _KERNEL
		NS_LOG(LE_NFS_ENUM_DSCMP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	dscm = *dscmp;

	/*
	 * search choices for a value that matches the discriminator.
	 * if we find one, execute the xdr routine for that value.
	 */
	for (; choices->proc != NULL_xdrproc_t; choices++) {
		if (choices->value == dscm)
			return ((*(choices->proc))(xdrs, unp, LASTUNSIGNED));
	}

	/*
	 * no match - execute the default xdr routine if there is one
	 */
	return ((dfault == NULL_xdrproc_t) ? FALSE :
	    (*dfault)(xdrs, unp, LASTUNSIGNED));
}

/*
 * Non-portable xdr primitives.
 * Care should be taken when moving these routines to new architectures.
 */

#ifdef _KERNEL
#define MINSIZ_XDR 16
#endif


/* BEGIN_IMS xdr_string *
 ******************************************************************************
 ****
 ****		xdr_string(xdrs,cpp,maxsize)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cpp		pointer to the string (ie pointer to a char pointer)
 *	maxsize		maximum permissible size of the string
 *
 * Output Parameters
 *	*cpp		if xdrs->x_op is XDR_DECODE or XDR_FREE then this 
 *			value may be changed
 *
 * Return Value
 *	TRUE/FALSE	depending on sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	xdr_string deals with "C strings" - arrays of bytes that are
 *	terminated by a NULL character. If the pointer is null, then
 *	the necessary storage is allocated. Depending upon the value
 *	of xdr->x_op the encoding/decoding/freeing of space takes place.
 *	In the first two cases, the first integer on the byte stream 
 *	is the size of the string.
 *
 * Algorithm
 *	{ open message catalog ifndef _KERNEL
 *	  switch(xdrs->x_op) {
 *	  case XDR_DECODE:	get length from the stream
 *				if size greater than maxsize
 *					return(FALSE)
 *				break
 *	  case XDR_ENCODE:	if string pointer is NULL
 *					return(FALSE)
 *				calculate string length
 *				if size greater than maxsize
 *					return(FALSE)
 *				put length on stream
 *				break
 *	  case XDR_FREE:	if pointer is NULL
 *					return(TRUE)
 *	  }
 *	  Since we always allocate/deallocate a minimum size of memory
 *		calculate the size
 *	  switch(xdrs->x_op) {
 *	  case XDR_DECODE:	allocate space for string if necessary
 *				put 0 at the end of string
 *				fall through
 *	  case XDR_ENCODE:	get/put string from/on the stream
 *				return value
 *	  case XDR_FREE:	free memory if pointer is not null
 *				return TRUE
 *	  }
 *	  return error
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. Fix strlen(null ptr) problem
 *	. If (xdrs->x_op == XDR_FREE) then maxsize should be ignored
 *	. If (xdrs->x_op == XDR_ENCODE) && (size > maxsize) then
 *	  failure occurs after the size has been put on the stream.
 *
 * Notes
 *	(gmf) At this time (2/18/87) we are not supporting long
 *	file names on the local file system; the ufs* functions
 *	will arbitrarily set the path[DIRSIZ] to NULL (DIRSIZ==14).
 *	To help out, we will allocate a minimum of 16 bytes of
 *	memory, so the ufs* functions need not check the string
 *	length.  The byte overwritten will be restored after use
 *	by the local system (UGLY!!!), so the strlen call here will
 *	work for deallocation.  16 is a good power of two that is
 *	larger than DIRSIZ.
 *
 * Modification History
 *
 * External Calls
 *	NS_LOG
 *	strlen
 *	getenv
 *	mem_alloc
 *	mem_free
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_string */

#ifdef _NAMESPACE_CLEAN
#undef xdr_string
#pragma _HP_SECONDARY_DEF _xdr_string xdr_string
#define xdr_string _xdr_string
#endif

bool_t
xdr_string(xdrs, cpp, maxsize)
	register XDR *xdrs;
	char **cpp;
	u_int maxsize;
{
	register char *sp = *cpp;  /* sp is the actual string pointer */
	u_int size = 0;
	u_int nodesize;

#ifndef _KERNEL
	nlmsg_fd = _nfs_nls_catopen();
#endif not _KERNEL

	/*
	 * first deal with the length since xdr strings are counted-strings
	 */
	if (((xdrs->x_op) != XDR_DECODE) && sp)
		size = strlen(sp);
	if (! xdr_u_int(xdrs, &size)) {
#ifdef _KERNEL
		NS_LOG(LE_NFS_STRING_SIZE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	if (size > maxsize) {
#ifdef _KERNEL
		NS_LOG(LE_NFS_STRING_BAD,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	nodesize = size + 1;

#ifdef _KERNEL
	/*
	 * guarantee minimum size of memory allocated
	 */
	if (nodesize < MINSIZ_XDR)
		nodesize = MINSIZ_XDR;
#endif

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 1) /* fix provided by Prabha (CND)
				      3/29/90 
				      This will only affect the user
				      space code because nodesize is
				      set to MINSIZ_XDR above, if we
				      are in the kernel. -- Sandip */
			return(TRUE);
		if (sp == NULL)
			*cpp = sp = (char *)mem_alloc(nodesize);
#ifndef _KERNEL
		if (sp == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "xdr_string: out of memory\n")));
			return (FALSE);
		}
#endif
		sp[size] = 0;
		/* fall into ... */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, size));

	case XDR_FREE:
		if (sp != NULL) {
			mem_free(sp, nodesize);
			*cpp = NULL;
		}
		return (TRUE);
	}
#ifdef _KERNEL
	NS_LOG(LE_NFS_STRING_BADOP,NS_LC_WARNING,NS_LS_NFS,0);
#endif
	return (FALSE);
}


#ifndef _KERNEL
/* BEGIN_IMS xdr_wrapstring *
 ******************************************************************************
 ****
 ****		xdr_wrapstring(xdrs, cpp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cpp		pointer to the string (ie pointer to a char pointer)
 *
 * Output Parameters
 *	*cpp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	LASTUNSIGNED 
 *
 * Description
 *	This only calls xdr_string, with the maximum size set to
 *	LASTUNSIGNED.
 *	After talking to John Dilley, I decided to put in LASTUNSIGNED
 *	instead of BUFSIZ (as it was for NFS 3.0).  This is because if 
 *	xdr_wrapstring is called to handle a string longer than 1024 (BUFSIZ), 
 *	it would return FALSE and it should probably not do that.
 *
 * Algorithm
 *	{ return(xdr_string(xdrs,cpp,LASTUNSIGNED))
 *	}
 *
 * Concurrency
 *	none
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_string
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_wrapstring */

#ifdef _NAMESPACE_CLEAN
#undef xdr_wrapstring
#pragma _HP_SECONDARY_DEF _xdr_wrapstring xdr_wrapstring
#define xdr_wrapstring _xdr_wrapstring
#endif

bool_t
xdr_wrapstring(xdrs, cpp)
	XDR *xdrs;
	char **cpp;
{
	return(xdr_string(xdrs, cpp, LASTUNSIGNED));
}



/* BEGIN_IMS xdr_free *
 ******************************************************************************
 ****
 ****		xdr_free(proc, objp)
 ****
 ******************************************************************************
 * Input Parameters
 *	proc		pointer to the function that will do the freeing
 *	objp		pointer to the object being freed
 *
 * Output Parameters
 *	*objp		should be deallocated
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Just calls the function pointed to by proc
 *
 * Algorithm
 *	See Description
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	none
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	the function pointed to by proc
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_free */

#ifdef _NAMESPACE_CLEAN
#undef xdr_free
#pragma _HP_SECONDARY_DEF _xdr_free xdr_free
#define xdr_free _xdr_free
#endif

void
xdr_free(proc, objp)
        xdrproc_t proc;
        char *objp;
{
        XDR x;

        x.x_op = XDR_FREE;
        (*proc)(&x, objp);
}



/* BEGIN_IMS xdr_char *
 ******************************************************************************
 ****
 ****		xdr_char(xdrs,cp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cp		pointer to a char
 *
 * Output Parameters
 *	*cp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes a char onto/from an xdr stream.
 *
 * Algorithm
 *	{ Just put *cp into an int variable and call xdr_int and let
 *	  it do the work.
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
 *	xdr_int
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_char */

#ifdef _NAMESPACE_CLEAN
#undef xdr_char
#pragma _HP_SECONDARY_DEF _xdr_char xdr_char
#define xdr_char _xdr_char
#endif

bool_t
xdr_char(xdrs, cp)
        XDR *xdrs;
        char *cp;
{
        int i;

        i = (*cp);
        if (!xdr_int(xdrs, &i)) {
                return (FALSE);
        }
        *cp = i;
        return (TRUE);
}



/* BEGIN_IMS xdr_u_char *
 ******************************************************************************
 ****
 ****		xdr_u_char(xdrs,cp)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	cp		pointer to a char
 *
 * Output Parameters
 *	*cp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Serializes/Deserializes an unsigned char onto/from an xdr stream.
 *
 * Algorithm
 *	{ Just put *cp into an unsigned int variable and call xdr_u_int and let
 *	  it do the work.
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
 *	xdr_u_int
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_char */

#ifdef _NAMESPACE_CLEAN
#undef xdr_u_char
#pragma _HP_SECONDARY_DEF _xdr_u_char xdr_u_char
#define xdr_u_char _xdr_u_char
#endif

bool_t
xdr_u_char(xdrs, cp)
        XDR *xdrs;
        char *cp;
{
        u_int u;

        u = (*cp);
        if (!xdr_u_int(xdrs, &u)) {
                return (FALSE);
        }
        *cp = u;
        return (TRUE);
}

#endif _KERNEL

#ifdef NS_QA
/* BEGIN_IMS test_nfs *
 ******************************************************************************
 ****
 ****		test_nfs(nfs_fail_count)
 ****
 ******************************************************************************
 * Input Parameters
 *
 * Output Parameters
 *	*nfs_fail_count		number of failed tests
 *
 * Return Value
 *
 * Globals Referenced
 *
 * Description
 *	This routine calls various basic xdr routines with a bad xdrs.x_op
 *	and with xdrs.x_op set to XDR_FREE so as to execite various
 *	portions of the code which are otherwise never called.
 *	This should be called by making an ioctl call only. This routine
 *	will therefore be called only from the test scaffold and is essentially
 *	present to do a more thorough test job.	This is NS_QA code only.
 *
 * Algorithm
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	This routine is called using the ioctl call SIONFSICA.
 *
 * Modification History
 *
 * External Calls
 *	xdr_long
 *	xdr_u_long
 *	xdr_short
 *	xdr_u_short
 *	xdr_bool
 *	xdr_opaque
 *	xdr_bytes
 *	xdrmem_create
 *	xdrmbuf_init
 *	xdrmbuf_putlong
 *	xdr_array
 *
 * Called By 
 *
 ******************************************************************************
 * END_IMS test_nfs */

#include "../h/mbuf.h"
#define	XDR_BAD_OPCODE		100
#define	LARGE_SIZE		1000
#define write_error(str) NS_LOG_STR(LE_NFS_TEST_ERROR, NS_LC_ERROR, NS_LS_NFS, 0, str)

test_nfs(nfs_fail_count)
int	*nfs_fail_count;
{
  	XDR	xdrs_nfs;
  	char	*buffer;

   	long lp = 0;
   	short sp = 0;
   	char cp[10];
   	char *ptr;
    	char *cpp = NULL;

  	(*nfs_fail_count) = 0;
  	buffer = (char *)kmem_alloc((u_int)LARGE_SIZE);
  	xdrmem_create(&xdrs_nfs,buffer,LARGE_SIZE,XDR_BAD_OPCODE);

	/* In this case we call the routines with x_op set to an 
	   illegal value */

	if (xdr_long(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_long: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	if (xdr_u_long(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_u_long: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	if (xdr_short(&xdrs_nfs,&sp)) {
		write_error("test_nfs-xdr_short: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	if (xdr_u_short(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_u_short: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	if (xdr_bool(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_bool: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	ptr = cp;
	if (xdr_opaque(&xdrs_nfs,(caddr_t)ptr,(u_int)100)) {
		write_error("test_nfs-xdr_opaque: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	lp = 5;
	if (xdr_bytes(&xdrs_nfs,&ptr,(u_int)&lp,(u_int)10)) {
		write_error("test_nfs-xdr_bytes: bad opcode failed.");
		(*nfs_fail_count)++;
	}
	lp = 20;
	xdrs_nfs.x_op = XDR_ENCODE;
	if (xdr_bytes(&xdrs_nfs,&ptr,(u_int)&lp,(u_int)10)) {
		write_error("test_nfs-xdr_bytes: size greater that maxsize failed.");
		(*nfs_fail_count)++;
	}

	/* Here x_op is set to XDR_FREE */

 	lp = 0;
    	sp = 0;
    	cpp = NULL;
  	xdrs_nfs.x_op = XDR_FREE;
	if (!xdr_long(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_long: free operation failed.");
		(*nfs_fail_count)++;
	}
	if (!xdr_u_long(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_u_long: free operation failed.");
		(*nfs_fail_count)++;
	}
	if (!xdr_short(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_short: free operation failed.");
		(*nfs_fail_count)++;
	}
	if (!xdr_u_short(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_u_short: free operation failed.");
		(*nfs_fail_count)++;
	}
	if (!xdr_bool(&xdrs_nfs,&lp)) {
		write_error("test_nfs-xdr_bool: free operation failed.");
		(*nfs_fail_count)++;
	}
	if (!xdr_opaque(&xdrs_nfs,(caddr_t)cpp,(u_int)100)) {
		write_error("test_nfs-xdr_opaque: free operation failed.");
		(*nfs_fail_count)++;
	}
	lp = 5;
	cpp = NULL;
	if (!xdr_bytes(&xdrs_nfs,&cpp,(u_int)&lp,(u_int)10)) {
		write_error("test_nfs-xdr_bytes: free operation failed.");
		(*nfs_fail_count)++;
	}

   {
	/* Here we call xdrmbuf_putlong with various different conditions */

	struct mbuf *m, *m1;
    	long lp;

/*++++++++++++++++++++++++++++++++++++++++++++++
	MGET(m, M_WAIT, 0, MT_DATA);
 */
	MGET(m, M_WAIT, MT_DATA);
	if(m == NULL){
		write_error("test_nfs- MGET failed.");
		(*nfs_fail_count)++;
	}
	else {
		xdrmbuf_init(&xdrs_nfs, m, XDR_ENCODE);
		xdrs_nfs.x_handy = 0;
		m->m_next = NULL;
		if (xdrmbuf_putlong(&xdrs_nfs,&lp)) {
			write_error("test_nfs-xdrmbuf_putlong: check for null chain failed.");
			(*nfs_fail_count)++;
		}
/*+++++++++++++++++++++++++++++++++++++++++++++++++
		MGET(m1,M_WAIT, 0, MT_DATA);
 */
		MGET(m1,M_WAIT, MT_DATA);
		if(m1 == NULL ){
			write_error("test_nfs- MGET failed.");
			(*nfs_fail_count)++;
			m_freem(m);
		}
		else {
			xdrs_nfs.x_handy = 0;
			xdrs_nfs.x_base = (caddr_t)m;
			m1->m_len = 100;
			m->m_next = m1;
			if (!xdrmbuf_putlong(&xdrs_nfs,&lp)) {
				write_error("test_nfs-xdrmbuf_putlong: chaining failed.");
				(*nfs_fail_count)++;
			}
			m_freem(m);
			xdrs_nfs.x_base = (caddr_t)NULL;
			xdrs_nfs.x_handy = 0;
			if (xdrmbuf_putlong(&xdrs_nfs,&lp)) {
				write_error("test_nfs-xdrmbuf_putlong:null mbuf check failed.");
				(*nfs_fail_count)++;
			}
		}
	}
   }

   {
	/* Here xdr_array is called with various error situations */

  	int	list[10];
   	int	size = 5;
   	caddr_t ptr = NULL;

  	xdrmem_create(&xdrs_nfs,buffer,LARGE_SIZE,XDR_BAD_OPCODE);
	ptr = (caddr_t)list;
	if (xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,10,sizeof(int),xdr_int)) {
		write_error("test_nfs-xdr_array:xdr_u_int should have failed.");
		(*nfs_fail_count)++;
	}
	xdrs_nfs.x_op = XDR_DECODE;
	if (xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,1,sizeof(int),xdr_int)) {
		write_error("test_nfs-xdr_array:size comparison failed.");
		(*nfs_fail_count)++;
	}
  	xdrmem_create(&xdrs_nfs,buffer,LARGE_SIZE,XDR_DECODE);
	*(int *)buffer = 0;
	ptr = NULL;
	if (!xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,10,sizeof(int),xdr_int)) {
		write_error("test_nfs-xdr_array:no memory should be allocated.");
		(*nfs_fail_count)++;
	}
	xdrs_nfs.x_op = XDR_FREE;
	if (!xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,10,sizeof(int),xdr_int)) {
		write_error("test_nfs-xdr_array:xdr_free, should just return.");
		(*nfs_fail_count)++;
	}
	ptr = NULL;
  	xdrmem_create(&xdrs_nfs,buffer,LARGE_SIZE,XDR_DECODE);
	*(int *)buffer = 5;
	if (xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,10,sizeof(int),xdr_int)) {
	    xdrs_nfs.x_op = XDR_FREE;
	    if (!xdr_array(&xdrs_nfs,&ptr,(u_int *)&size,10,sizeof(int),xdr_int)) {
		    write_error("test_nfs-xdr_array:memory should be returned.");
		    (*nfs_fail_count)++;
	    }
	} else {
		write_error("test_nfs-xdr_array:memory should be allocated.");
		(*nfs_fail_count)++;
	}
  }
  kmem_free((caddr_t)buffer,(u_int)LARGE_SIZE);
}
#endif NS_QA
