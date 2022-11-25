/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_array.c,v $
 * $Revision: 12.3 $	$Author: dlr $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/05/03 15:34:24 $
 */

/* BEGIN_IMS_MOD   
******************************************************************************
****								
****	xdr_array - XDR routine for an array
****								
******************************************************************************
* Description
*	XDR module for the array data type.
*
* Externally Callable Routines
*	xdr_array - encode/decode an array
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
static char rcsid[] = "@(#) $Header: xdr_array.c,v 12.3 90/05/03 15:34:24 dlr Exp $ (Hewlett-Packard)";
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
#define memset 		_memset
#define xdr_array 	_xdr_array	/* In this file */
#define xdr_u_int 	_xdr_u_int
#define xdr_vector 	_xdr_vector	/* In this file */

#endif /* _NAME_SPACE_CLEAN */

#ifdef KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"


#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#else /* not KERNEL */

#define NL_SETN 18	/* set number */
#include <nl_types.h>
static	nl_catd nlmsg_fd;

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <stdio.h>
char *mem_alloc();
#endif /* KERNEL */
#define LASTUNSIGNED	((u_int)0-1)


/* BEGIN_IMS xdr_array *
 ******************************************************************************
 ****
 ****		xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	addrp		pointer to the array
 *	sizep		pointer to the number of elements
 *	maxsize		maximum number of elements
 *	elsize		size in bytes of each element
 *	elproc		xdr routine to handle each element
 *
 * Output Parameters
 *	*sizep
 *	addrp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Like strings, arrays are really counted arrays. This routine
 *	will (depending upon the value of xdrs->x_op) get/put the
 *	size and array elements from/on the xdr stream. If 
 *	(xdrs->x_op == XDR_FREE) then it deallocates memory. For
 *	each array element it uses the procedure that has been passed
 *	to it as a parameter, to serialize/deserialize the array
 *	elemets.
 *
 *
 * Algorithm
 *	{ ifndef KERNEL
 *		open native language support catalog 
 *	  get/put array size from the xdr stream
 *	  if (xdrs->x_op != XDR_FREE) && (size > maxsize)
 *		return error
 *	  if (array pointer == NULL) {
 *		switch(xdrs->x_op) {
 *		case XDR_DECODE:	allocate memory
 *					print error if no memory available
 *					zero the allocated memory
 *					break
 *		case XDR_FREE:		return(TRUE)
 *		}
 *	  }
 *	  for (each array element) 
 *		call xdr_routine elproc to handle it
 *	  if (xdrs->x_op == XDR_FREE)
 *		return memory
 *	  return value
 *	}
 *
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. memory may be returned before trying to call the routine
 *	  for each array element. This is bound to speed up matters.
 *	  In fact, if xdrs->x_op is equal to XDR_FREE we needn't go into
 *	  all the processing. This may be checked right at the beginning.
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	getenv
 *	NS_LOG
 *	xdr_u_int
 *	mem_alloc
 *	memset/bzero
 *	mem_free
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_array */

#ifdef _NAMESPACE_CLEAN
#undef xdr_array
#pragma _HP_SECONDARY_DEF _xdr_array xdr_array
#define xdr_array _xdr_array
#endif

bool_t
xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
	register XDR *xdrs;
	caddr_t *addrp;		/* array pointer */
	u_int *sizep;		/* number of elements */
	u_int maxsize;		/* max numberof elements */
	u_int elsize;		/* size in bytes of each element */
	xdrproc_t elproc;	/* xdr routine to handle each element */
{
	register u_int i;
	register caddr_t target = *addrp;
	register u_int c;  /* the actual element count */
	register bool_t stat = TRUE;
	register int nodesize;

#ifndef KERNEL
	nlmsg_fd = _nfs_nls_catopen();
#endif /* not KERNEL */

	/* like strings, arrays are really counted arrays */
	if (! xdr_u_int(xdrs, sizep)) {
#ifdef KERNEL
		NS_LOG(LE_NFS_ARRAY_SIZE,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	c = *sizep;
	if ((c > maxsize) && (xdrs->x_op != XDR_FREE)) {
#ifdef KERNEL
		NS_LOG(LE_NFS_ARRAY_BAD,NS_LC_WARNING,NS_LS_NFS,0);
#endif
		return (FALSE);
	}
	nodesize = c * elsize;

	/*
	 * if we are deserializing, we may need to allocate an array.
	 * We also save time by checking for a null array if we are freeing.
	 */
	if (target == NULL)
		switch (xdrs->x_op) {
		case XDR_DECODE:
			if (c == 0)
				return (TRUE);
			*addrp = target = mem_alloc(nodesize);
#ifndef KERNEL
			if (target == NULL) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "xdr_array: out of memory\n")));
				return (FALSE);
			}
			memset(target, 0, (u_int)nodesize);
#else /* KERNEL */
			bzero(target, (u_int)nodesize);
#endif /* KERNEL */
			break;

		case XDR_FREE:
			return (TRUE);
	}
	
	/*
	 * now we xdr each element of array
	 */
	for (i = 0; (i < c) && stat; i++) {
		stat = (*elproc)(xdrs, target, LASTUNSIGNED);
		target += elsize;
	}

	/*
	 * the array may need freeing
	 */
	if (xdrs->x_op == XDR_FREE) {
		mem_free(*addrp, nodesize);
		*addrp = NULL;
	}
	return (stat);
}

#ifndef KERNEL

/* BEGIN_IMS xdr_vector *
 ******************************************************************************
 ****
 ****		xdr_vector(xdrs, basep, nelem, elemsize, xdr_elem)
 ****
 ******************************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	basep		base of the array   
 *	nelem		number of elements in the array  
 *	elemsize	size of an element of the array
 *	xdr_elem	xdr routine to handle each element
 *
 * Output Parameters
 *	none  
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	The routine goes through the array and calls the xdr_elem function
 *	for each element of the array
 *
 *
 * Algorithm
 *
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
 *	*xdr_elem
 *
 * Called By (optional)
 *
 ******************************************************************************
 * END_IMS xdr_array */

#ifdef _NAMESPACE_CLEAN
#undef xdr_vector
#pragma _HP_SECONDARY_DEF _xdr_vector xdr_vector
#define xdr_vector _xdr_vector
#endif

bool_t
xdr_vector(xdrs, basep, nelem, elemsize, xdr_elem)
        register XDR *xdrs;
        register char *basep;
        register u_int nelem;
        register u_int elemsize;
        register xdrproc_t xdr_elem;
{
        register u_int i;
        register char *elptr;

        elptr = basep;
        for (i = 0; i < nelem; i++) {
                if (! (*xdr_elem)(xdrs, elptr, LASTUNSIGNED)) {
                        return(FALSE);
                }
                elptr += elemsize;
        }
        return(TRUE);
}
#endif /* KERNEL */
