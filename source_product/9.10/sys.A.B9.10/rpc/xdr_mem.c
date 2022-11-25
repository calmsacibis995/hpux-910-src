/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/rpc/RCS/xdr_mem.c,v $
 * $Revision: 1.8.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 22:49:50 $
 */

/* BEGIN_IMS_MOD  
******************************************************************************
****								
****	xdr_mem - XDR implementation using memory buffers
****								
******************************************************************************
* Description
* 	The XDR memory routines provide the functions necessary for getting
*       XDR data to and from memory buffers.
*
* Externally Callable Routines
*	xdrmem_create - initialize an XDR stream for a memory buffer
*	xdrmem_destroy - null routine
*	xdrmem_getlong - get a long from the memory XDR stream
*	xdrmem_putlong - put a long into the memory XDR stream
*	xdrmem_getbytes - get bytes from the memory XDR stream
*	xdrmem_putbytes - put bytes into the memory XDR stream
*	xdrmem_getpos - determine the current position in the XDR stream
*	xdrmem_setpos - set the current position in the XDR stream
*	xdrmem_inline - ask for a block of contiguous data in the input stream
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
static char rcsid[] = "@(#) $Header: xdr_mem.c,v 1.8.83.4 93/12/06 22:49:50 marshall Exp $ (Hewlett-Packard)";
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

#define memcpy 		_memcpy
#define xdrmem_create 	_xdrmem_create		/* In this file */

#endif /* _NAMESPACE_CLEAN */

#ifdef _KERNEL
#include "../h/param.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../netinet/in.h"
#ifdef NTRIGGER
#include "../h/trigdef.h"		/* needed in order to use triggers */
#endif /* NTRIGGER */
#else
#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <netinet/in.h>
#endif

#ifdef __hp9000s800
#define XDR_UNIT_ALIGNED(p) (!((int)(p)&(int)3))
#else  /* not hp9000s800 */
#define XDR_UNIT_ALIGNED(p) (!((int)(p)&(int)1))
#endif	/* hp9000s800 */

static bool_t	xdrmem_getlong();
static bool_t	xdrmem_putlong();
static bool_t	xdrmem_getbytes();
static bool_t	xdrmem_putbytes();
static u_int	xdrmem_getpos();
static bool_t	xdrmem_setpos();
static long *	xdrmem_inline();
static void	xdrmem_destroy();

static struct	xdr_ops xdrmem_ops = {
	xdrmem_getlong,
	xdrmem_putlong,
	xdrmem_getbytes,
	xdrmem_putbytes,
	xdrmem_getpos,
	xdrmem_setpos,
	xdrmem_inline,
	xdrmem_destroy
};


/* BEGIN_IMS xdrmem_create *
 ********************************************************************
 ****
 ****		xdrmem_create(xdrs, addr, size, op)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	addr		start address of the memory block
 *	size		size of the memory block
 *	op		what kind of operation (XDR_ENCODE etc) is desired
 *
 * Output Parameters
 *	xdrs->		fields modified to reflect the nature of the request
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This routine initializes a stream descriptor for a memory buffer.
 *	It reflects the nature of the request in the fields of the xdr
 *	handle (xdrs). The x_ops field is now made to point to the
 *	appropriate routines for manipulating the memory block, so that it
 *	looks like a xdr stream.
 *
 * Algorithm
 *	{ make x_ops point to manipulation routines
 *	       x_handy equal to the size of the memory block
 *	       x_private and x_base point to the start address
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
 *	none
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_create */

#ifdef _NAMESPACE_CLEAN
#undef xdrmem_create
#pragma _HP_SECONDARY_DEF _xdrmem_create xdrmem_create
#define xdrmem_create _xdrmem_create
#endif

void
xdrmem_create(xdrs, addr, size, op)
	register XDR *xdrs;
	caddr_t addr;
	u_int size;
	enum xdr_op op;
{

	xdrs->x_op = op;
	xdrs->x_ops = &xdrmem_ops;
	xdrs->x_private = xdrs->x_base = addr;
	xdrs->x_handy = size;
}


/* BEGIN_IMS xdrmem_destroy *
 ********************************************************************
 ****
 ****		xdrmem_destroy(xdrs)
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
 *	none
 *
 * Description
 *	Does nothing! (at the moment)
 *
 * Algorithm
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	none
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_destroy */
static void
xdrmem_destroy(/*xdrs*/)
	/*XDR *xdrs;*/
{
}


/* BEGIN_IMS xdrmem_getlong *
 ********************************************************************
 ****
 ****		xdrmem_getlong(xdrs, lp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	lp		pointer to long
 *
 * Output Parameters
 *	*lp		contains the value of the long from the stream if
 *			the operation was sucessful
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This obtains a long from the stream, if there is any space 
 *	left. It uses the handle to get information about the stream.
 *
 * Algorithm
 *	{ if (no more space left in the memory block)
 *		return(FALSE)
 *	  get long from the block after conversion from XDR 
 *		format to machine format
 *	  update pointer into data area
 *	  update the stream size left
 *	  return(TRUE)
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
 *	ntohl
 *	sizeof
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_getlong */
static bool_t
xdrmem_getlong(xdrs, lp)
	register XDR *xdrs;
	long *lp;
{
	/*
	 * temporaries in case of odd alignment
	 */
	register char *from;
	register char *to;

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMEM_GETL, T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif /* NTRIGGER */
	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	/*
	 *  This code makes sure that only aligned data is passed to 	
	 *  other routines.  This catches odd-aligned data in the case 
	 *  of the s310 and non-word-aligned data in the case of the s800.
	 *  It is in-line to save a function call to bcopy.  
	 */
	if (!XDR_UNIT_ALIGNED(xdrs->x_private)) 
	{
		/*
		 * Missaligned pointer
		 */
		from = (char *) (xdrs->x_private);
		to = (char *) lp;
		*to++ = *from++;
		*to++ = *from++;
		*to++ = *from++;		
		*to++ = *from++;
	}
	else
		*lp = ntohl(*((long *)(xdrs->x_private)));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}


/* BEGIN_IMS xdrmem_putlong *
 ********************************************************************
 ****
 ****		xdrmem_putlong(xdrs, lp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	lp		pointer to long
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
 *	This puts a long on the stream, if there is any space 
 *	left. It uses the handle to get information about the stream.
 *
 * Algorithm
 *	{ if (no more space left in the memory block)
 *		return(FALSE)
 *	  put long on the block after conversion to XDR 
 *		format from machine format
 *	  update pointer into data area
 *	  return(TRUE)
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
 *	ntohl
 *	sizeof
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_putlong */
static bool_t
xdrmem_putlong(xdrs, lp)
	register XDR *xdrs;
	long *lp;
{
#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMEM_PUTL, T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif /* NTRIGGER */
	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
/*A BUG!!, if x_private is not on the word boundary.  */
	*(long *)xdrs->x_private = htonl(*lp);
	xdrs->x_private += sizeof(long);
	return (TRUE);
}


/* BEGIN_IMS xdrmem_getbytes *
 ********************************************************************
 ****
 ****		xdrmem_getbytes(xdrs, addr, len)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	addr		pointer to the receiving area
 *	len		number of bytes to be received
 *
 * Output Parameters
 *	*addr		data is copied into this area
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This makes sure that there is sufficient space in the memory
 *	stream, and copies the appropriate number of bytes into the
 *	receiving area.
 *
 * Algorithm
 *	{ if (sufficient data/space is not available)
 *		return(FALSE)
 *	  copy the appropriate number of bytes into the specified area
 *	  update relevant information in the xdr handle
 *	  return(TRUE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. there doesn't seem to be any copying done in the kernel !!
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	memcpy
 *	bcopy
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_getbytes */
static bool_t
xdrmem_getbytes(xdrs, addr, len)
	register XDR *xdrs;
	caddr_t addr;
	register u_int len;
{
#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMEM_GETB, T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif /* NTRIGGER */
	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
#if ! defined _KERNEL
	memcpy(addr, xdrs->x_private, len);
#else /* defined hpux && ! defined _KERNEL */
	bcopy(xdrs->x_private, addr, len);
#endif /* defined hpux && ! defined _KERNEL */
	xdrs->x_private += len;
	return (TRUE);
}


/* BEGIN_IMS xdrmem_putbytes *
 ********************************************************************
 ****
 ****		xdrmem_putbytes(xdrs, addr, len)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	addr		pointer to the sending area
 *	len		number of bytes to be received
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
 *	This makes sure that there is sufficient space in the memory
 *	stream, and copies the appropriate number of bytes from the
 *	sending area into the xdr stream.
 *
 * Algorithm
 *	{ if (sufficient data/space is not available)
 *		return(FALSE)
 *	  copy the appropriate number of bytes from the specified area
 *	  update relevant information in the xdr handle
 *	  return(TRUE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. there doesn't seem to be any copying done in the kernel !!
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	memcpy
 *	bcopy
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_putbytes */
static bool_t
xdrmem_putbytes(xdrs, addr, len)
	register XDR *xdrs;
	caddr_t addr;
	register u_int len;
{
#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMEM_PUTB, T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif /* NTRIGGER */
	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
#if ! defined _KERNEL
	memcpy(xdrs->x_private, addr, len);
#else /* defined hpux && ! defined _KERNEL */
	bcopy(addr, xdrs->x_private, len);
#endif /* defined hpux && ! defined _KERNEL */
	xdrs->x_private += len;
	return (TRUE);
}


/* BEGIN_IMS xdrmem_getpos *
 ********************************************************************
 ****
 ****		xdrmem_getpos(xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	current position in the xdr stream
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Calculates the current position in the xdr stream from the
 *	data available in xdrs.
 *
 * Algorithm
 *	{ return(current position in the stream - base address)
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
 *	none
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_getpos */
static u_int
xdrmem_getpos(xdrs)
	register XDR *xdrs;
{

	return ((u_int)xdrs->x_private - (u_int)xdrs->x_base);
}


/* BEGIN_IMS xdrmem_setpos *
 ********************************************************************
 ****
 ****		xdrmem_setpos(xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *
 * Output Parameters
 *	xdrs->private	points to the new position in the stream
 *	xdrs->x_handy	contains the number of bytes remaining in the
 *			stream
 *
 * Return Value
 *	TRUE/FALSE	depending upon the success of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	Sets the current position in the xdr stream as desired.
 *
 * Algorithm
 *	{ calculate position to be set
 *	  calculate end address of memory block
 *	  if (position to be set > end address)
 *		return(FALSE)
 *	  update information in xdrs
 *	  return(TRUE)
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
 *	none
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_setpos */
static bool_t
xdrmem_setpos(xdrs, pos)
	register XDR *xdrs;
	u_int pos;
{
	register caddr_t newaddr = xdrs->x_base + pos;
	register caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

	if ((long)newaddr > (long)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;
	return (TRUE);
}


/* BEGIN_IMS xdrmem_inline *
 ********************************************************************
 ****
 ****		xdrmem_inline(xdrs, len)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *
 * Output Parameters
 *	xdrs fields are updated accordingly
 *
 * Return Value
 *	buffer address, which is NULL if the operation fails
 *
 * Globals Referenced
 *
 * Description
 *	This gives the address of a block len bytes long, if there is
 *	sufficient space available in the stream.
 *
 * Algorithm
 *	{ set buffer address to NULL
 *	  if (sufficient space is available) {
 *		update statistics in xdrs
 *		calculate buffer address
 *	  }
 *	  return(buffer address)
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
 *	none
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmem_inline */
static long *
xdrmem_inline(xdrs, len)
	register XDR *xdrs;
	int len;
{
	long *buf = 0;
#ifdef NTRIGGER
/*
 * If at any time the trigger pops with the value of flag set to one,
 * then this would be equivalent to the trigger popping every time, until
 * the flag is reset to zero. This strategy is adopted to execute the parts
 * of the code which is only called if inline fails. Popping this trigger 
 * surgically is a very tricky proposition as inline is called from a
 * number of places and this is the best way to pop the trigger every time,
 * rather than using some magic number. A similar strategy is adopted in
 * xdrmbuf_inline() also.
 */
static	int  flag = 0;
	if ((utry_trigger(T_NFS_XDRMEM_INLINE, T_NFS_PROC,&flag,NULL)) || flag)
		return(buf);
#endif /* NTRIGGER */
	if (xdrs->x_handy >= len && XDR_UNIT_ALIGNED(xdrs->x_private)) {
		xdrs->x_handy -= len;
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}
	return (buf);
}
