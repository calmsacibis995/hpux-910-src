/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_mbuf.c,v $
 * $Revision: 12.0 $	$Author: nfsmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/09/25 16:09:38 $
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr_mbuf.c,v 12.0 89/09/25 16:09:38 nfsmgr Exp $ (Hewlett-Packard)";
#endif

/*
 * xdr_mbuf.c, XDR implementation on kernel mbufs.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../netinet/in.h"
#include "../h/uio.h"
#include "../h/ns_diag.h"
#ifdef NTRIGGER
#include "../h/trigdef.h"		/* needed in order to use triggers */
#endif NTRIGGER
#ifdef hp9000s800
#include "../machine/vmparam.h"
#include "../h/user.h"
#include "../h/proc.h"
#endif hp9000s800

#ifdef hp9000s200
#define NFSBCOPY(A,B,C) bcopy(A,B,C)
#define XDR_UNIT_ALIGNED(p) (!((int)(p)&(int)1))
#else  !hp9000s200
#define NFSBCOPY(A,B,C) nfs_bcopy(A,B,C)
#define XDR_UNIT_ALIGNED(p) (!((int)(p)&(int)3))
#endif	hp9000s200
bool_t	xdrmbuf_getlong(), xdrmbuf_putlong();
bool_t	xdrmbuf_getbytes(), xdrmbuf_putbytes();
u_int	xdrmbuf_getpos();
bool_t	xdrmbuf_setpos();
long *	xdrmbuf_inline();
void	xdrmbuf_destroy();

/*
 * Xdr on mbufs operations vector.
 */
struct	xdr_ops xdrmbuf_ops = {
	xdrmbuf_getlong,
	xdrmbuf_putlong,
	xdrmbuf_getbytes,
	xdrmbuf_putbytes,
	xdrmbuf_getpos,
	xdrmbuf_setpos,
	xdrmbuf_inline,
	xdrmbuf_destroy
};


/* BEGIN_IMS xdrmbuf_init *
 ********************************************************************
 ****
 ****		xdrmbuf_init(xdrs, m, op)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	m		pointer to a mbuf structure
 *	op		kind of stream to be generated (XDR_ENCODE etc)
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
 *	This routine initializes a stream descriptor for a mbuf structure.
 *	It reflects the nature of the request in the fields of the xdr
 *	handle (xdrs). The x_ops field is now made to point to the
 *	appropriate routines for manipulating the mbuf structure, so that it
 *	looks like a xdr stream.
 *
 * Algorithm
 *	{ make x_ops point to manipulation routines
 *	       x_handy equal to the size of the mbuf block
 *	       x_base point to the mbuf start address
 *	       x_private point to where the data starts
 *	       x_public point to NULL
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
 * END_IMS xdrmbuf_init */
void
xdrmbuf_init(xdrs, m, op)
	register XDR		*xdrs;
	register struct mbuf	*m;
	enum xdr_op		 op;
{

	xdrs->x_op = op;
	xdrs->x_ops = &xdrmbuf_ops;
	xdrs->x_base = (caddr_t)m;
	xdrs->x_private = mtod(m, caddr_t);
	xdrs->x_public = (caddr_t)0;
	xdrs->x_handy = m->m_len;
}


/* BEGIN_IMS xdrmbuf_destroy *
 ********************************************************************
 ****
 ****		xdrmbuf_destroy(xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		pointer to xdr stream handle
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
 * END_IMS xdrmbuf_destroy */
void
/* ARGSUSED */
xdrmbuf_destroy(xdrs)
	XDR	*xdrs;
{
	/* do nothing */
}


/* BEGIN_IMS xdrmbuf_getlong *
 ********************************************************************
 ****
 ****		xdrmbuf_getlong(xdrs, lp)
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
 *	none
 *
 * Description
 *	This obtains a long from the stream, if there is any space 
 *	left. It uses the handle to get information about the stream.
 *	The different cases here that one has to take care of are:
 *		. long present wholly in current mbuf or next mbuf and 
 *			aligned
 *		. long present wholly in current mbuf or next mbuf but
 *			not aligned
 *		. long present partly in current mbuf and partly in next
 *		. not enough space avaliable for a long
 *	As one might well guess, the code is a mess!
 *
 * Algorithm
 *	{ if (not enough space in current mbuf) {
 *		if (long stradling two mbufs)
 *			copy bytes from current mbuf and store
 *		if (current mbuf not NULL) {
 *			if (there is a next_mbuf) {
 *				move to it
 *				update info in xdr handle
 *			} else {
 *				return(FALSE)
 *			}
 *			if (long was stradling two mbufs) {
 *				assemble rest of long
 *				update info in xdr handle
 *				return(TRUE)
 *			}
 *		} else {
 *			return(FALSE)	<< no mbufs >>
 *		}
 *	  }
 *	  << there is sufficient space in the current mbuf, but
 *	     alignment is not known >>
 *	  if (alignment is not rigth)
 *		assemble long from the stream
 *	  else
 *		get long from the stream
 *	  update information in xdr handle
 *	  return(TRUE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *	. mbufs used are returned by a higher entity
 *
 * Modification History
 *
 * External Calls
 *	ntohl
 *	bcopy
 *	sizeof
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmbuf_getlong */
bool_t
xdrmbuf_getlong(xdrs, lp)
	register XDR	*xdrs;
	long		*lp;
{
	char holdit[sizeof(long)];
	char more = '\0';
	short amt;
	/*
	 * temporaries in case of odd alignment
	 */
	register char *from;
	register char *to;

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMBUF_GETL, T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif NTRIGGER
	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long)) {
		/*
		 *HPNFS  This was removed by MDS  11/10/86
			printf("xdr_mbuf: long crosses mbufs!\n");
		 * 
		 *HPNFS  I need to pick up the remaining bytes in the
		 *HPNFS  m_buf, as opposed to SUN which just jumped to
		 *HPNFS  the next mbuf, and screwing everything up.
		 *HPNFS  Note: the fact that we saw this problem may
		 *HPNFS  indicate other networking architecture problems.
		 */
			amt = xdrs->x_handy + sizeof(long);
			bcopy(xdrs->x_private, holdit, amt);
			more = 'y';
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			if (more == 'y') {
				bcopy(xdrs->x_private, &(holdit[amt]), sizeof(long)-amt); 
				*lp = ntohl(*((long *)(holdit)));
				xdrs->x_private += sizeof(long)-amt;
				xdrs->x_handy = m->m_len - (sizeof(long)-amt);
				return (TRUE);
			} else {
				xdrs->x_handy = m->m_len - sizeof(long);
			}
		} else {
			return (FALSE);
		}
	}

	/*
	 * this code is ugly, but necessary
	 *	for the 310, in the case of odd-aligned data
	 *	for the 800, in the case of non-word-aligned data
	 * It is in-line to save a function
	 * call to bcopy.  It could be made faster with "asm"
	 * instructions.
	 * - jar (7/20/87)
	 */
	if (!XDR_UNIT_ALIGNED(xdrs->x_private)) {
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


/* BEGIN_IMS xdrmbuf_putlong *
 ********************************************************************
 ****
 ****		xdrmbuf_putlong(xdrs, lp)
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
 *	The different cases here that one has to take care of are:
 *		. long can be put wholly in current mbuf or next mbuf and 
 *			aligned
 *		. long can be put partly in current mbuf and partly in next
 *		. not enough space avaliable for a long
 *
 * Algorithm
 *	{ if (there isn't enough space in the current mbuf) {
 *		if (long is going to cross mbufs)
 *			log message
 *		if (current mbuf is not NULL) {
 *			if (next_mbuf is available) {
 *				move to next_mbuf
 *				update information in xdr handle
 *			} else 
 *				return(FALSE)
 *	  }
 *	  << there is sufficient space in the current mbuf >>
 *	  put long on the mbuf
 *	  update information in the xdr handle
 *	  return(TRUE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in getlong one takes care of alignment and when long crosses
 *	  mbufs. Here we just seem to ignore the problem.
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	htonl
 *	sizeof
 *	NS_LOG
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmbuf_putlong */
bool_t
xdrmbuf_putlong(xdrs, lp)
	register XDR	*xdrs;
	long		*lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long))
			NS_LOG(LE_NFS_OVERLAP,NS_LC_WARNING,NS_LS_NFS,0);
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len - sizeof(long);
		} else {
			return (FALSE);
		}
	}
	*(long *)xdrs->x_private = htonl(*lp);
	xdrs->x_private += sizeof(long);
	return (TRUE);
}


#ifdef hp9000s800
/* BEGIN_IMS nfs_bcopy *
 ********************************************************************
 ****
 ****		nfs_bcopy(from, to, len)
 ****
 ********************************************************************
 * Input Parameters
 *	from		address to copy from
 *	to		address to copy to
 *	len		length of the copy stream
 *
 * Output Parameters
 *	*to		len bytes are copied into this
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	u
 *
 * Description
 *	This routine copies len bytes over from from, to to !! If the
 *	data is in user space, it uses lbcopy instead of bcopy.
 *
 * Algorithm
 *	{ if ( user space ) 
 *		lbcopy
 *	  else
 *		bcopy
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
 *	lbcopy
 *	bcopy
 *
 * Called By (optional)
 *	xdrmbuf_getbytes
 *
 ********************************************************************
 * END_IMS nfs_bcopy */
nfs_bcopy(from, to, len)
	caddr_t from;
	caddr_t to;
	int     len;
{
	space_t     space;
	/* S2RPAGE flag is set in physstrat() when pagging in 
	 * remote text into user space
	 */
        if (u.u_procp->p_flag2 & S2RPAGE) {
		space = pvtospace(u.u_procp, to);
	        lbcopy(KERNELSPACE, from, space, to, len);
        } else  bcopy(from, to, len);
}

#endif hp9000s800


/* BEGIN_IMS xdrmbuf_getbytes *
 ********************************************************************
 ****
 ****		xdrmbuf_getbytes(xdrs, addr, len)
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
 *	This makes sure that there is sufficient space in the mbuf
 *	stream, and copies the appropriate number of bytes into the
 *	receiving area. Since the amount of data requested may not
 *	be present in one mbuf, we will have to traverse as many
 *	mbufs as necessary to fulfill the request.
 *
 * Algorithm
 *	{ while ( there aren't sufficient bytes in the current mbuf 
 *			to copy all of remaining data ) {
 *		if (there are any bytes remaining in current mbuf) 
 *			copy it from mbuf
 *		if (current mbuf is not NULL) {
 *			if (next_mbuf exists) {
 *				move to next_mbuf
 *				update xdr handle
 *			} else 
 *				return(FALSE)
 *		} else
 *			return(FALSE)
 *	  }
 *	  copy any remaining bytes over to target area
 *	  update xdr handle
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
 *	NFSBCOPY
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmbuf_getbytes */
bool_t
xdrmbuf_getbytes(xdrs, addr, len)
	register XDR	*xdrs;
	caddr_t		 addr;
	register u_int	 len;
{

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMBUF_GETB,T_NFS_PROC,NULL,NULL))
		return(FALSE);
#endif NTRIGGER
	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			NFSBCOPY(xdrs->x_private, addr, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len;
		} else {
			return (FALSE);
		}
	}
	NFSBCOPY(xdrs->x_private, addr, (u_int)len);
	xdrs->x_private += len;
	return (TRUE);
}


/* BEGIN_IMS xdrmbuf_putbytes *
 ********************************************************************
 ****
 ****		xdrmbuf_putbytes(xdrs, addr, len)
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
 *	This makes sure that there is sufficient space in the mbuf
 *	stream, and copies the appropriate number of bytes from the
 *	sending area into the xdr stream. Since the amount of data 
 *	requested may not fit into one mbuf, we will have to traverse
 *	as many	mbufs as necessary to fulfill the request.
 *
 * Algorithm
 *	{ while ( there aren't sufficient bytes in the current mbuf 
 *			to get all of remaining data ) {
 *		if (there are any bytes remaining in current mbuf) 
 *			copy it into target area from the mbuf
 *		if (current mbuf is not NULL) {
 *			if (next_mbuf exists) {
 *				move to next_mbuf
 *				update xdr handle
 *			} else 
 *				return(FALSE)
 *		} else
 *			return(FALSE)
 *	  }
 *	  copy any remaining bytes into mbuf
 *	  update xdr handle
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
 *	memcpy
 *	bcopy
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmbuf_putbytes */
bool_t
xdrmbuf_putbytes(xdrs, addr, len)
	register XDR	*xdrs;
	caddr_t		 addr;
	register u_int		 len;
{

#ifdef NTRIGGER
	if (utry_trigger(T_NFS_XDRMBUF_PUTB ,T_NFS_PROC, NULL, NULL))
		return(FALSE);
#endif NTRIGGER
	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			bcopy(addr, xdrs->x_private, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			register struct mbuf *m =
			    ((struct mbuf *)xdrs->x_base)->m_next;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = mtod(m, caddr_t);
			xdrs->x_handy = m->m_len;
		} else {
			return (FALSE);
		}
	}
	bcopy(addr, xdrs->x_private, len);
	xdrs->x_private += len;
	return (TRUE);
}


/* BEGIN_IMS xdrmbuf_putbuf *
 ********************************************************************
 ****
 ****		xdrmbuf_putbuf(xdrs, addr, len, func, arg)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	addr		address of the buffer which the new mbuf will
 *				be pointing to
 *	len		len of the above mentioned buffer
 *	func		function that is to be called after the new
 *				mbuf is released (!?)
 *	arg		additional information (!?)
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
 *	This routine was created out of the need for an efficient
 *	way to handle the results of a read request on the server side,
 *	since this is the most common request of the server. This routine
 *	is given a pointer to a data buffer and the length of the 
 *	buffer. An mbuf is allocated and made to point at the data 
 *	buffer, and the mbuf is appended to the end of the output
 *	mbuf chain. Thus, when a server receives a read request, and
 *	after the server has read the data into some internal buffer
 *	from the disk, a call is made to xdrmbuf_putbuf(). This is
 *	equivalent to an xdr_bytes() for encoding, but avoids the
 *	expense of copying a potentially large (8k) number of bytes.
 *
 * Algorithm
 *	{ put the size of the buffer on the xdr stream
 *	  calculate the necessary padding needed so that the total
 *		amount that gets sent is a integral number of 
 *		XDR_UNITS (4 bytes)
 *	  update the information in the current mbuf (accessing it
 *		through xdrs) so that the remainder of the buffer
 *		is not used
 *	  create a new mbuf whose data area will point to the addr buffer
 *	  if (not sucessful)
 *		return(FALSE)
 *	  link up the new mbuf into the chain
 *	  if (padding is necessary) {
 *		get a new mbuf
 *		if (not sucessful)
 *			return(FALSE)
 *		attach it to the chain and update information
 *	  }
 *	  return(TRUE)
 *	}
 *
 * Concurrency
 *	. none
 *
 * To Do List
 *	. When we initialize the xdr stream, we had attached a whole 
 *	  chain of mbufs. If we are in the middle of this mbuf chain,
 *	  and we call this routine, we append this new buffer by creating
 *	  a new mbuf, but do not relink the rest of the chain. Seems
 *	  to be a dangling buffer problem.
 *	. x_base and x_private do not seem to be updated properly. In
 *	  particular x_base should be pointing to the correct mbuf etc
 *
 * Notes
 *	This is like xxx_putbytes, except that we avoid the copy by
 *	pointing a type 2 mbuf at the buffer. This is not safe if the
 *	buffer goes away before the mbuf chain is deallocated.
 *
 *	This routine was changed to handle padding differently.
 *	Originally, Sun had the line below for doing padding:
 *	      len = (len + 3) & ~3;
 *	The idea being that we could just grab whatever padding bytes we 
 *	needed from the memory immediately after the buffer being sent. 
 *	However, if the buffer being sent was aligned on the end of a 
 *	page boundary, then those bytes might not be available. Thus, we
 *	are forced to grab another mbuf and use it for padding. We could
 *	try to be smart and do it only on page boundaries, but this seems 
 *	dangerous and doesn't save us that much.
 *
 * Modification History
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdrmbuf_putbuf */
bool_t
xdrmbuf_putbuf(xdrs, addr, len, func, arg)
	register XDR	*xdrs;
	caddr_t		 addr;
	u_int		 len;
	int		 (*func)();
	int		 arg;
{
	register struct mbuf *m;
	struct mbuf *mclgetx();
	long llen = len;
	long padding;
	register struct mbuf *m2;

	xdrmbuf_putlong(xdrs, &llen);
	/*
	 * Calculate the necessary padding needed to get to a
	 * multiple of four bytes. e.g. if len = 5, padding=3.
	 */
	padding = ((len + 3) & ~3) - len;
	((struct mbuf *)xdrs->x_base)->m_len -= xdrs->x_handy;
#ifdef NTRIGGER
	if (utry_trigger(T_NFS_MGET_6, T_NFS_PROC, NULL, NULL))
		m = NULL;
	else
#endif NTRIGGER
		m = mclgetx(func, arg, addr, (int)len, M_WAIT);
	if (m == NULL) {
		NS_LOG(LE_NFS_MCLGETX_FAIL,NS_LC_RESOURCELIM,NS_LS_NFS,0);
		return (FALSE);
	}
	((struct mbuf *)xdrs->x_base)->m_next = m;
	xdrs->x_handy = 0;
	if (padding) {
		/*
		 * Padding necessary, get an mbuf to chain on the end
		 * and just set the length to the pad value.  We don't
		 * care what the values are, so don't worry about zeroing.
		 */
		MGET( m2, M_WAIT, 0, MT_DATA);
#ifdef NTRIGGER
		if (utry_trigger(T_NFS_MGET_7, T_NFS_PROC, NULL, NULL)) {
			m_freem(m2);
			m2 = NULL;
		}
#endif NTRIGGER
		if ( m2 == NULL ) {
			NS_LOG(LE_NFS_MGET_FAIL,NS_LC_RESOURCELIM,NS_LS_NFS,0);
			return(FALSE);
		}
		m2->m_off = MMINOFF;
		m2->m_len = padding;
		m->m_next = m2;
		m2->m_next = NULL;
	}
	return (TRUE);
}


/* BEGIN_IMS xdrmbuf_getpos *
 ********************************************************************
 ****
 ****		xdrmbuf_getpos(xdrs)
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
 *	{ return(current position in the stream - base data address)
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
 * END_IMS xdrmbuf_getpos */
u_int
xdrmbuf_getpos(xdrs)
	register XDR	*xdrs;
{

	return (
	    (u_int)xdrs->x_private - mtod(((struct mbuf *)xdrs->x_base), u_int));
}


/* BEGIN_IMS xdrmbuf_setpos *
 ********************************************************************
 ****
 ****		xdrmbuf_setpos(xdrs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	pos		position to be set in the stream
 *
 * Output Parameters
 *	xdrs->private	points to the new position in the mbuf
 *	xdrs->x_handy	contains the number of bytes remaining in the
 *			mbuf
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
 *	  calculate end address of mbuf data block
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
 *	. there seems to be logical mismatch in the abstraction of the xdr
 *	  stream, depending upon whether it is implemented upon mbufs or
 *	  memory. We cannot set a position in the mbuf stream, if the
 *	  position exceeds the mbuf boundary, regardless of whether there
 *	  are more mbufs linked to the current mbuf or not!!
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
 * END_IMS xdrmbuf_setpos */
bool_t
xdrmbuf_setpos(xdrs, pos)
	register XDR	*xdrs;
	u_int		 pos;
{
	register caddr_t	newaddr =
	    mtod(((struct mbuf *)xdrs->x_base), caddr_t) + pos;
	register caddr_t	lastaddr =
	    xdrs->x_private + xdrs->x_handy;

	if ((int)newaddr > (int)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;
	return (TRUE);
}


/* BEGIN_IMS xdrmbuf_inline *
 ********************************************************************
 ****
 ****		xdrmbuf_inline(xdrs, len)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	len		length of data block
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
 *	sufficient space available in the stream. This checks for
 *	data alignment.
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
 * END_IMS xdrmbuf_inline */
long *
xdrmbuf_inline(xdrs, len)
	register XDR	*xdrs;
	int		 len;
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
 * xdrmem_inline() also.
 */
	static	int  flag = 0;

	if ((utry_trigger(T_NFS_XDRMBUF_INLINE,T_NFS_PROC,&flag,NULL)) || flag)
		return(buf);
#endif NTRIGGER
	if (xdrs->x_handy >= len && XDR_UNIT_ALIGNED(xdrs->x_private)) {
		xdrs->x_handy -= len;
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}
	return (buf);
}

