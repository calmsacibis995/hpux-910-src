/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.2 $	$Date: 92/02/07 17:08:47 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_stdio.c,v $
 * $Revision: 12.2 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:47 $
 *
 * Revision 12.1  90/03/21  10:52:25  10:52:25  dlr
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:09:49  16:09:49  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/08  11:24:45  11:24:45  dlr (Dominic Ruffatto)
 * Added !defined (hp9000s800) condition around ntohl() and htonl() function
 * calls.
 * 
 * Revision 1.1.10.6  89/02/08  10:23:52  10:23:52  cmahon (Christina Mahon)
 * Added !defined (hp9000s800) condition around ntohl() and htonl() function
 * calls.
 * 
 * Revision 1.1.10.5  89/01/26  12:30:39  12:30:39  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.4  89/01/16  15:28:36  15:28:36  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:55:32  14:55:32  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.1  88/02/29  13:56:56  13:56:56  nfsmgr (Nfsmanager)
 * Starting revision level for 6.2 and 3.0.
 * 
 * Revision 10.1  88/02/29  13:36:01  13:36:01  nfsmgr (Nfsmanager)
 * Starting point for 6.2 and 3.0.
 * 
 * Revision 8.2  88/02/04  10:41:37  10:41:37  chm (Cristina H. Mahon)
 * Changed SCCS strings to RCS revision and date strings.
 * 
 * Revision 8.1  87/11/30  13:56:36  13:56:36  nfsmgr (Nfsmanager)
 * revision levels for ic4b
 * 
 * Revision 1.1.10.3  87/08/07  15:17:48  15:17:48  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:33:56  22:33:56  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:46:52  11:46:52  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
#endif

/*
 * xdr_stdio.c, XDR implementation on standard i/o file.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * This set of routines implements a XDR on a stdio stream.
 * XDR_ENCODE serializes onto the stream, XDR_DECODE de-serializes
 * from the stream.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define fflush 			_fflush
#define fread 			_fread
#define fseek 			_fseek
#define ftell 			_ftell
#define fwrite 			_fwrite
#define xdrstdio_create 	_xdrstdio_create	/* In this file */

#endif /* _NAME_SPACE_CLEAN */

#include <rpc/types.h>
#include <stdio.h>
#include <rpc/xdr.h>
/*	HPNFS	jad	87.05.04
**	include <netinet/in.h> for definitions of ntohl and htonl.
**	also add TRACE() macros!
*/
#include <netinet/in.h>
#include <arpa/trace.h>

static bool_t	xdrstdio_getlong();
static bool_t	xdrstdio_putlong();
static bool_t	xdrstdio_getbytes();
static bool_t	xdrstdio_putbytes();
static u_int	xdrstdio_getpos();
static bool_t	xdrstdio_setpos();
static long *	xdrstdio_inline();
static void	xdrstdio_destroy();

/*
 * Ops vector for stdio type XDR
 */
static struct xdr_ops	xdrstdio_ops = {
	xdrstdio_getlong,	/* deseraialize a long int */
	xdrstdio_putlong,	/* seraialize a long int */
	xdrstdio_getbytes,	/* deserialize counted bytes */
	xdrstdio_putbytes,	/* serialize counted bytes */
	xdrstdio_getpos,	/* get offset in the stream */
	xdrstdio_setpos,	/* set offset in the stream */
	xdrstdio_inline,	/* prime stream for inline macros */
	xdrstdio_destroy	/* destroy stream */
};

/*
 * Initialize a stdio xdr stream.
 * Sets the xdr stream handle xdrs for use on the stream file.
 * Operation flag is set to op.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdrstdio_create
#pragma _HP_SECONDARY_DEF _xdrstdio_create xdrstdio_create
#define xdrstdio_create _xdrstdio_create
#endif

void
xdrstdio_create(xdrs, file, op)
	register XDR *xdrs;
	FILE *file;
	enum xdr_op op;
{
	TRACE("xdrstdio_create SOP");
	xdrs->x_op = op;
	xdrs->x_ops = &xdrstdio_ops;
	xdrs->x_private = (caddr_t)file;
	xdrs->x_handy = 0;
	xdrs->x_base = 0;
}

/*
 * Destroy a stdio xdr stream.
 * Cleans up the xdr stream handle xdrs previously set up by xdrstdio_create.
 */
static void
xdrstdio_destroy(xdrs)
	register XDR *xdrs;
{
	TRACE("xdrstdio_destroy SOP, file NOT closed!");
	(void)fflush((FILE *)xdrs->x_private);
	/* xx should we close the file ?? */
};

static bool_t
xdrstdio_getlong(xdrs, lp)
	XDR *xdrs;
	register long *lp;
{

	TRACE("xdrstdio_getlong SOP");
	if (fread((caddr_t)lp, sizeof(long), 1, (FILE *)xdrs->x_private) != 1)
		return (FALSE);
#if	!defined(mc68000) && !defined(hp9000s200) && !defined(hp9000s800)
	TRACE("xdrstdio_getlong mc68000 case, translating lp");
	*lp = ntohl(*lp);
#endif
	TRACE2("xdrstdio_getlong returning TRUE with *lp=0x%lx", *lp);
	return (TRUE);
}

static bool_t
xdrstdio_putlong(xdrs, lp)
	XDR *xdrs;
	long *lp;
{
#if	!defined(mc68000) && !defined(hp9000s200) && !defined(hp9000s800)
	long mycopy = htonl(*lp);
	TRACE("xdrstdio_putlong SOP");
	lp = &mycopy;
	TRACE2("xdrstdio_putlong NOT mc68000 case, *lp=0x%lx", *lp);
#endif
	if (fwrite((caddr_t)lp, sizeof(long), 1, (FILE *)xdrs->x_private) != 1)
		return (FALSE);
	TRACE2("xdrstdio_putlong returning TRUE, *lp=0x%lx", *lp);
	return (TRUE);
}

static bool_t
xdrstdio_getbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{
	TRACE("xdrstdio_getbytes SOP");
	if ((len != 0) && (fread(addr, (int)len, 1, (FILE *)xdrs->x_private) != 1))
		return (FALSE);
	TRACE("xdrstdio_getbytes returning TRUE");
	return (TRUE);
}

static bool_t
xdrstdio_putbytes(xdrs, addr, len)
	XDR *xdrs;
	caddr_t addr;
	u_int len;
{
	TRACE("xdrstdio_putbytes SOP");
	if ((len != 0) && (fwrite(addr, (int)len, 1, (FILE *)xdrs->x_private) != 1))
		return (FALSE);
	TRACE("xdrstdio_putbytes returning TRUE");
	return (TRUE);
}

static u_int
xdrstdio_getpos(xdrs)
	XDR *xdrs;
{
	TRACE("xdrstdio_getpos SOP");
	return ((u_int) ftell((FILE *)xdrs->x_private));
}

static bool_t
xdrstdio_setpos(xdrs, pos) 
	XDR *xdrs;
	u_int pos;
{ 
	TRACE("xdrstdio_getbytes SOP");
	return ((fseek((FILE *)xdrs->x_private, (long)pos, 0) < 0) ?
		FALSE : TRUE);
}

static long *
xdrstdio_inline(xdrs, len)
	XDR *xdrs;
	u_int len;
{

	/*
	 * Must do some work to implement this: must insure
	 * enough data in the underlying stdio buffer,
	 * that the buffer is aligned so that we can indirect through a
	 * long *, and stuff this pointer in xdrs->x_buf.  Doing
	 * a fread or fwrite to a scratch buffer would defeat
	 * most of the gains to be had here and require storage
	 * management on this buffer, so we don't do this.
	 */
	TRACE("xdrstdio_inline SOP");
	return (NULL);
}
