/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 70.1 $	$Date: 94/10/04 11:03:34 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_rec.c,v $
 * $Revision: 70.1 $	$Author: hmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/04 11:03:34 $
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr_rec.c,v 70.1 94/10/04 11:03:34 hmgr Exp $ (Hewlett-Packard)";
#endif

 /*
  * xdr_rec.c, Implements TCP/IP based XDR streams with a "record marking"
  * layer above tcp (for rpc's use).
  *
  * Copyright (C) 1984, Sun Microsystems, Inc.
  *
  * These routines interface XDRSTREAMS to a tcp/ip connection.
  * There is a record marking layer between the xdr stream
  * and the tcp transport level.  A record is composed on one or more
  * record fragments.  A record fragment is a thirty-two bit header followed
  * by n bytes of data, where n is contained in the header.  The header
  * is represented as a htonl(u_long).  Thegh order bit encodes
  * whether or not the fragment is the last fragment of the record
  * (1 => fragment is last, 0 => more fragments to follow).
  * The other 31 bits encode the byte length of the fragment.
  */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define fprintf 		_fprintf
#define lseek 			_lseek
#define memcpy 			_memcpy
#define xdrrec_create 		_xdrrec_create		/* In this file */
#define xdrrec_endofrecord 	_xdrrec_endofrecord	/* In this file */
#define xdrrec_eof 		_xdrrec_eof		/* In this file */
#define xdrrec_skiprecord 	_xdrrec_skiprecord	/* In this file */
#define xdrrec_readbytes 	_xdrrec_readbytes	/* In this file */

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 19	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <time.h>
#include <netinet/in.h>
/*
**	Add TRACE stuff ...
*/
#include <arpa/trace.h>

char *mem_alloc();

static bool_t	xdrrec_getlong();
static bool_t	xdrrec_putlong();
static bool_t	xdrrec_getbytes();
static bool_t	xdrrec_putbytes();
static u_int	xdrrec_getpos();
static bool_t	xdrrec_setpos();
static long *	xdrrec_inline();
static void	xdrrec_destroy();

static struct  xdr_ops xdrrec_ops = {
	 xdrrec_getlong,
	 xdrrec_putlong,
	 xdrrec_getbytes,
	 xdrrec_putbytes,
	 xdrrec_getpos,
	 xdrrec_setpos,
	 xdrrec_inline,
	 xdrrec_destroy
};

/*	HPNFS	jad	87.08.04
**	define the minimum size of an XDR record stream to be 100;
**	define the default size of an XDR record stream to be 4000;
**	define macro expression which returns a valid size, no less
**	than MIN_SIZE and a multiple of 4.
**	NOTE: DEF_SIZE must be a multiple of 4.
**	In the NFS 3.2 code Sun changed the fix_buf_size function to 
**	round up the values it receives.  For that reason we have changed
**	the FIX_BUF_SIZE macro we use to round up also (instead of rounding
**	down as it used to do).  
*/
# define	MIN_SIZE	100
# define	DEF_SIZE	4000
# define	FIX_BUF_SIZE(x)	(((x) < MIN_SIZE) ? DEF_SIZE : (((x)+3)&(~03)))

/*
 * A record is composed of one or more record fragments.
 * A record fragment is a two-byte header followed by zero to
 * 2**32-1 bytes.  The header is treated as a long unsigned and is
 * encode/decoded to the network via htonl/ntohl.  The low order 31 bits
 * are a byte count of the fragment.  The highest order bit is a boolean:
 * 1 => this fragment is the last fragment of the record,
 * 0 => this fragment is followed by more fragment(s).
 *
 * The fragment/record machinery is not general;  it is constructed to
 * meet the needs of xdr and rpc based on tcp.
 */

#define LAST_FRAG ((u_long)(1 << 31))

/* HPNFS 
** The RPC3.9 code added an extra field to the RECSTREAM structure 
** (the_buffer).  That field is used to insure that the input and
** output buffers used are word-aligned.  We had already fixed that
** problem for the s800 in a different way.  So, since the structure
** RECSTREAM is not visible outside this source file anyway we do not
** need to make the changes that Sun did.
*/

typedef struct rec_strm {
	 caddr_t tcp_handle;
	 /*
	  * out-going bits
	  */
	 int (*writeit)();
	 caddr_t out_base;	/* output buffer (points to frag header) */
	 caddr_t out_finger;	/* next output position */
	 caddr_t out_boundry;	/* data cannot go past this address */
	 u_long *frag_header;	/* beginning of current fragment */
	 bool_t frag_sent;	/* true if buffer sent in middle of record */
	 /*
	  * in-coming bits
	  */
	 int (*readit)();
	 u_long in_size;	/* fixed size of the input buffer */
	 caddr_t in_base;
	 caddr_t in_finger;	/* location of next byte to be had */
	 caddr_t in_boundry;	/* can read up to this location */
	 long fbtbc;		/* fragment bytes to be consumed */
	 bool_t last_frag;
	 u_int sendsize;
	 u_int recvsize;
} RECSTREAM;


/*
 * Create an xdr handle for xdrrec
 * xdrrec_create fills in xdrs.  Sendsize and recvsize are
 * send and recv buffer sizes (0 => use default).
 * tcp_handle is an opaque handle that is passed as the first parameter to
 * the procedures readit and writeit.  Readit and writeit are read and
 * write respectively.   They are like the system
 * calls expect that they take an opaque handle rather than an fd.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdrrec_create
#pragma _HP_SECONDARY_DEF _xdrrec_create xdrrec_create
#define xdrrec_create _xdrrec_create
#endif

void
xdrrec_create(xdrs, sendsize, recvsize, tcp_handle, readit, writeit)
	 register XDR *xdrs;
	 u_int sendsize;
	 u_int recvsize;
	 caddr_t tcp_handle;
	 int (*readit)();  /* like read, but pass it a tcp_handle, not sock */
	 int (*writeit)();  /* like write, but pass it a tcp_handle, not sock */
{
	 register RECSTREAM *rstrm =
	     (RECSTREAM *)mem_alloc(sizeof(RECSTREAM));

	 nlmsg_fd = _nfs_nls_catopen();

	 TRACE("xdrrec_create SOP");
	 if (rstrm == NULL) {
		 fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "xdrrec_create: out of memory\n")));
		 /* 
		  *  This is bad.  Should rework xdrrec_create to 
		  *  return a handle, and in this case return NULL
		  */
		 return;
	 }
	 xdrs->x_ops = &xdrrec_ops;
	 xdrs->x_private = (caddr_t)rstrm;
	 rstrm->tcp_handle = tcp_handle;
	 rstrm->readit = readit;
	 rstrm->writeit = writeit;

	 /*	HPNFS	jad	87.08.04
	 **	make sure the send size is word-aligned; allocate memory
	 **	for the buffer (out_base); make frag_header point to the
	 **	beginning of the buffer, out_finger point to the next free
	 **	word, and out_boundary point to the end of the buffer.
	 */
	 sendsize = FIX_BUF_SIZE(sendsize);
	 TRACE2("xdrrec_create sendsize = %d", sendsize);
	 if ((rstrm->out_base = mem_alloc(sendsize)) == NULL) {
		 fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "xdrrec_create: out of memory\n")));
		 return;
	 }
	 rstrm->frag_header = (u_long *)rstrm->out_base;
	 rstrm->out_finger = rstrm->out_base + sizeof(u_long);
	 rstrm->out_boundry = rstrm->out_base + sendsize;
	 rstrm->frag_sent = FALSE;

	 /*	HPNFS	jad	87.08.04
	 **	make sure the recv size is word-aligned; allocate memory
	 **	for the buffer (in_base); make in_finger point to the end
	 **	of the buffer, along with in_boundary (it's empty).
	 */
	 recvsize = FIX_BUF_SIZE(recvsize);
	 TRACE2("xdrrec_create recvsize = %d", recvsize);
	 if ((rstrm->in_base = mem_alloc(recvsize)) == NULL) {
		 fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "xdrrec_create: out of memory\n")));
		 return;
	 }
	 rstrm->in_boundry = rstrm->in_base + recvsize;
	 rstrm->in_finger = rstrm->in_boundry;
	 rstrm->last_frag = TRUE;
	 rstrm->fbtbc = 0;

	 rstrm->sendsize = sendsize;
	 rstrm->in_size = rstrm->recvsize = recvsize;
	 TRACE("xdrrec_create returning after setting up rstrm");
}


/*
 * The routines defined below are the xdr ops which will go into the
 * xdr handle filled in by xdrrec_create.
 */

static bool_t
xdrrec_getlong(xdrs, lp)
	 XDR *xdrs;
	 long *lp;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	 register long *buflp = (long *)(rstrm->in_finger);
	 long mylong;

	 TRACE("xdrrec_getlong SOP");
	 /* first try the inline, fast case */
	 if ((rstrm->fbtbc >= sizeof(long)) &&
	     (((int)rstrm->in_boundry - (int)buflp) >= sizeof(long))) {
		 TRACE("xdrrec_getlong inline (fast) case");
		 /*	HPNFS	jad	87.09.11
		 **	possible alignment problems if either of the
		 **	following (long *)addresses are not aligned.
		 **	see the code and comments in fill_input_buf.
		 */
		 *lp = ntohl(*buflp);
		 rstrm->fbtbc -= sizeof(long);
		 rstrm->in_finger += sizeof(long);
	 } else {
		 TRACE("xdrrec_getlong not inline case");
		 if (! xdrrec_getbytes(xdrs, (caddr_t)&mylong, (u_int)sizeof(long)))
			 return (FALSE);
		 *lp = ntohl(mylong);
	 }
	 TRACE("xdrrec_getlong returns TRUE");
	 return (TRUE);
}

static bool_t
xdrrec_putlong(xdrs, lp)
	 XDR *xdrs;
	 long *lp;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	 register long *dest_lp = ((long *)(rstrm->out_finger));

	 TRACE("xdrrec_putlong SOP");
	 if ((rstrm->out_finger += sizeof(long)) > rstrm->out_boundry) {
		 /*
		  * this case should almost never happen so the code is
		  * inefficient			(what an excuse, eh?:-)
		  */
		 TRACE("xdrrec_putlong should RARELY happen code");
		 rstrm->out_finger -= sizeof(long);
		 rstrm->frag_sent = TRUE;
		 if (! flush_out(rstrm, FALSE))
			 return (FALSE);
		 dest_lp = ((long *)(rstrm->out_finger));
		 rstrm->out_finger += sizeof(long);
	 }
	 /*	HPNFS	jad	87.09.11
	 **	possible alignment problems if either of the
	 **	following (long *)addresses are not aligned.
	 **	see the code and comments in fill_input_buf.
	 */
	 *dest_lp = htonl(*lp);
	 TRACE("xdrrec_putlong returns TRUE");
	 return (TRUE);
}

static bool_t  /* must manage buffers, fragments, and records */
xdrrec_getbytes(xdrs, addr, len)
	 XDR *xdrs;
	 register caddr_t addr;
	 register u_int len;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	 register int current;

	 TRACE("xdrrec_getbytes SOP");
	 while (len > 0) {
		 current = rstrm->fbtbc;
		 if (current == 0) {
			 if (rstrm->last_frag)
				 return (FALSE);
			 if (! set_input_fragment(rstrm))
				 return (FALSE);
			 continue;
		 }
		 current = (len < current) ? len : current;
		 if (! get_input_bytes(rstrm, addr, current))
			 return (FALSE);
		 addr += current; 
		 rstrm->fbtbc -= current;
		 len -= current;
	 }
	 TRACE("xdrrec_getbytes returns TRUE");
	 return (TRUE);
}

static bool_t
xdrrec_putbytes(xdrs, addr, len)
	 XDR *xdrs;
	 register caddr_t addr;
	 register u_int len;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	 register int current;

	 TRACE("xdrrec_putbytes SOP");
	 while (len > 0) {
		 current = (u_int)rstrm->out_boundry - (u_int)rstrm->out_finger;
		 current = (len < current) ? len : current;
		 memcpy(rstrm->out_finger, addr, current);
		 rstrm->out_finger += current;
		 addr += current;
		 len -= current;
		 if (rstrm->out_finger == rstrm->out_boundry) {
			 rstrm->frag_sent = TRUE;
			 if (! flush_out(rstrm, FALSE))
				 return (FALSE);
		 }
	 }
	 TRACE("xdrrec_putbytes returns TRUE");
	 return (TRUE);
}

static u_int
xdrrec_getpos(xdrs)
	 register XDR *xdrs;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	 register u_int pos;

	 TRACE("xdrrec_getpos SOP");
	 /*
	 **	THIS IS WEIRD!  We are doing an lseek on a struct
	 **	... which makes no sense whatsoever.  Even if we
	 **	gave it a socket, lseek() is not defined ...
	 */
	 pos = lseek((int)rstrm->tcp_handle, 0, 1);
	 TRACE2("xdrrec_getpos pos=%d", pos);
	 if ((int)pos != -1)
		 switch (xdrs->x_op) {

		 case XDR_ENCODE:
			 pos += rstrm->out_finger - rstrm->out_base;
			 TRACE2("xdrrec_getpos ENCODE pos=%d", pos);
			 break;

		 case XDR_DECODE:
			 pos -= rstrm->in_boundry - rstrm->in_finger;
			 TRACE2("xdrrec_getpos DECODE pos=%d", pos);
			 break;

		 default:
			 pos = (u_int) -1;
			 TRACE2("xdrrec_getpos default pos=%d", pos);
			 break;
		 }
	 TRACE2("xdrrec_getpos return pos = %d", pos);
	 return (pos);
}

static bool_t
xdrrec_setpos(xdrs, pos)
	 register XDR *xdrs;
	 u_int pos;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	 u_int currpos = xdrrec_getpos(xdrs);
	 int delta = currpos - pos;
	 caddr_t newpos;

	 TRACE2("xdrrec_setpos SOP, currpos = %d", currpos);
	 if ((int)currpos != -1)
		 switch (xdrs->x_op) {

		 case XDR_ENCODE:
			 newpos = rstrm->out_finger - delta;
			 TRACE4("xdrrec_setpos ENCODE finger=0x%x, delta=%d, newpos=%d", rstrm->out_finger, delta, newpos);
			 if ((newpos > (caddr_t)(rstrm->frag_header)) &&
			     (newpos < rstrm->out_boundry)) {
				 rstrm->out_finger = newpos;
				 TRACE("xdrrec_setpos ENCODE returns TRUE");
				 return (TRUE);
			 }
			 break;

		 case XDR_DECODE:
			 newpos = rstrm->in_finger - delta;
			 TRACE4("xdrrec_setpos DECODE finger=0x%x, delta=%d, newpos=%d", rstrm->in_finger, delta, newpos);
			 if ((delta < (int)(rstrm->fbtbc)) &&
			     (newpos <= rstrm->in_boundry) &&
			     (newpos >= rstrm->in_base)) {
				 rstrm->in_finger = newpos;
				 rstrm->fbtbc -= delta;
				 TRACE("xdrrec_setpos DECODE returns TRUE");
				 return (TRUE);
			 }
			 break;
		 }
	 TRACE("xdrrec_setpos returns FALSE");
	 return (FALSE);
}

static long *
xdrrec_inline(xdrs, len)
	 register XDR *xdrs;
	 int len;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;
	 long * buf = NULL;

	 TRACE("xdrrec_inline SOP");
	 switch (xdrs->x_op) {

	 case XDR_ENCODE:
		 if ((rstrm->out_finger + len) <= rstrm->out_boundry) {
			 buf = (long *) rstrm->out_finger;
			 rstrm->out_finger += len;
		 }
		 break;

	 case XDR_DECODE:
#ifdef	hp9000s800
		 /* NOT aligned!  do it the hard way ... */
		 /* otherwise we're non-aligned and die! */
		 if (((int)(rstrm->in_finger) & 03) || (len & 03))
			 return(FALSE);
#endif	/* hp9000s800 */
		 if ((len <= rstrm->fbtbc) &&
		     ((rstrm->in_finger + len) <= rstrm->in_boundry)) {
			 buf = (long *) rstrm->in_finger;
			 rstrm->fbtbc -= len;
			 rstrm->in_finger += len;
		 }
		 break;
	 }
	 TRACE2("xdrrec_inline returns 0x%x", buf);
	 return (buf);
}

static void
xdrrec_destroy(xdrs)
	 register XDR *xdrs;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)xdrs->x_private;

	 TRACE("xdrrec_destroy SOP"); 
	 mem_free(rstrm->out_base, rstrm->sendsize);
	 mem_free(rstrm->in_base, rstrm->recvsize);
	 mem_free((caddr_t)rstrm, sizeof(RECSTREAM));
}


/*
 * Exported routines to manage xdr records
 */

/*
 * Before reading (deserializing) from the stream, one should always call
 * this procedure to guarantee proper record alignment.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdrrec_skiprecord
#pragma _HP_SECONDARY_DEF _xdrrec_skiprecord xdrrec_skiprecord
#define xdrrec_skiprecord _xdrrec_skiprecord
#endif

bool_t
xdrrec_skiprecord(xdrs)
	 XDR *xdrs;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);

	 TRACE("xdrrec_skiprecord SOP");
	 while (rstrm->fbtbc > 0 || (! rstrm->last_frag)) {
		 if (! skip_input_bytes(rstrm, rstrm->fbtbc))
			 return (FALSE);
		 rstrm->fbtbc = 0;
		 if ((! rstrm->last_frag) && (! set_input_fragment(rstrm)))
			 return (FALSE);
	 }
	 rstrm->last_frag = FALSE;
	 TRACE("xdrrec_skiprecord returns TRUE");
	 return (TRUE);
}

/*
 * Look ahead fuction.
 * Returns TRUE iff there is no more input in the buffer 
 * after consuming the rest of the current record.
 */
/* Note: this function can return TRUE when we're not yet out of data */

#ifdef _NAMESPACE_CLEAN
#undef xdrrec_eof
#pragma _HP_SECONDARY_DEF _xdrrec_eof xdrrec_eof
#define xdrrec_eof _xdrrec_eof
#endif

bool_t
xdrrec_eof(xdrs)
	 XDR *xdrs;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);

	 TRACE("xdrrec_eof SOP");
	 while (rstrm->fbtbc > 0 || (! rstrm->last_frag)) {
		 if (! skip_input_bytes(rstrm, rstrm->fbtbc))
			 return (TRUE);
		 rstrm->fbtbc = 0;
		 if ((! rstrm->last_frag) && (! set_input_fragment(rstrm)))
			 return (TRUE);
	 }
	 if (rstrm->in_finger == rstrm->in_boundry) {
		 TRACE("xdrrec_eof returns TRUE");
		 return (TRUE);
	 }
	 TRACE("xdrrec_eof returns FALSE");
	 return (FALSE);
}

/*
 * The client must tell the package when an end-of-record has occurred.
 * The second paramters tells whether the record should be flushed to the
 * (output) tcp stream.  (This let's the package support batched or
 * pipelined procedure calls.)  TRUE => immmediate flush to tcp connection.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdrrec_endofrecord
#pragma _HP_SECONDARY_DEF _xdrrec_endofrecord xdrrec_endofrecord
#define xdrrec_endofrecord _xdrrec_endofrecord
#endif

bool_t
xdrrec_endofrecord(xdrs, sendnow)
	 XDR *xdrs;
	 bool_t sendnow;
{
	 register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	 register u_long len;  /* fragment length */

	 TRACE2("xdrrec_endofrecord SOP, sendnow=%d", sendnow);
	 if (sendnow || rstrm->frag_sent ||
	     ((u_long)rstrm->out_finger + sizeof(u_long) >=
	     (u_long)rstrm->out_boundry)) {
		 rstrm->frag_sent = FALSE;
		 return (flush_out(rstrm, TRUE));
	 }
	 len = (u_long)(rstrm->out_finger) - (u_long)(rstrm->frag_header) -
		 sizeof(u_long);
	 /*	HPNFS	jad	87.09.11
	 **	May have alignment problems here -- frag_header MUST
	 **	be properly aligned or we will dump core on s800.  See
	 **	fill_input_buf() for comments and the code change.
	 */
	 *(rstrm->frag_header) = htonl(len | LAST_FRAG);

	 rstrm->frag_header = (u_long *)rstrm->out_finger;
	 rstrm->out_finger += sizeof(u_long);
	 TRACE("xdrrec_endofrecord returns TRUE");
	 return (TRUE);
}

/*
 * This is just like the ops vector xdr_getbytes(), except that
 * instead of returning success or failure on getting a certain number
 * of bytes, it behaves much more like the read() system call against a
 * pipe -- it returns up to the number of bytes requested and a return of
 * zero indicates end-of-record.  A -1 means something very bad happened.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdrrec_readbytes
#pragma _HP_SECONDARY_DEF _xdrrec_readbytes xdrrec_readbytes
#define xdrrec_readbytes _xdrrec_readbytes
#endif

u_int /* must manage buffers, fragments, and records */
xdrrec_readbytes(xdrs, addr, l)
	XDR *xdrs;
	register caddr_t addr;
	u_int l;
{
	register RECSTREAM *rstrm = (RECSTREAM *)(xdrs->x_private);
	register int current, len;

	len = l;
	while (len > 0) {
		current = rstrm->fbtbc;
		if (current == 0) {
			if (rstrm->last_frag)
				return (l - len);
			if (! set_input_fragment(rstrm))
				return (-1);
			continue;
		}
		current = (len < current) ? len : current;
		if (! get_input_bytes(rstrm, addr, current))
			return (-1);
		addr += current;
		rstrm->fbtbc -= current;
		len -= current;
	}
	return (l - len);
}

/*
 * Internal useful routines
 */
static bool_t
flush_out(rstrm, eor)
	 register RECSTREAM *rstrm;
	 bool_t eor;
{
	 register u_long eormask = (eor == TRUE) ? LAST_FRAG : 0;
	 register u_long len = (u_long)(rstrm->out_finger) - 
	     (u_long)(rstrm->frag_header) - sizeof(u_long);

	 TRACE2("flush_out SOP, eor=%d", eor); 
	 /*	HPNFS	jad	87.09.11
	 **	May have alignment problems here -- frag_header MUST
	 **	be properly aligned or we will dump core on s800.  See
	 **	fill_input_buf() for comments and the code change.
	 */
	 *(rstrm->frag_header) = htonl(len | eormask);

	 len = (u_long)(rstrm->out_finger) - (u_long)(rstrm->out_base);
	 if ((*(rstrm->writeit))(rstrm->tcp_handle, rstrm->out_base, (int)len)
	     != (int)len)
		 return (FALSE);
	 rstrm->frag_header = (u_long *)rstrm->out_base;
	 rstrm->out_finger = (caddr_t)rstrm->out_base + sizeof(u_long);
	 TRACE("flush_out returns TRUE"); 
	 return (TRUE);
}

/* knows nothing about records!  Only about input buffers */
static bool_t
fill_input_buf(rstrm)
	 register RECSTREAM *rstrm;
{
	 register caddr_t where = rstrm->in_base;
	 register int len = rstrm->in_size;
	 int	align = (int)(rstrm->in_boundry) & 03;

	 TRACE("fill_input_buf SOP");
	 if (align != 0) {
		 /*	HPNFS	jad	87.09.10
		 **	TRICKY:  if our in_boundry is NOT ALIGNED in
		 **	memory, then we know that the remainder of an XDR
		 **	unit has been split between two TCP recv()s.  We
		 **	adjust the where pointer to be offset by the same
		 **	amount as we were off before -- so when we get the
		 **	new data, we will be able to pass back a few bytes
		 **	to fill in the previous XDR data unit (!always a
		 **	multiple of sizeof(long)!), and then be back on track.
		 **	NOTE:  in_boundry & 03 forces 4-byte alignment...
		 **	The RPC 3.9 code has a fix for this word-alignment
		 **	problem also.  Their fix is different from ours
		 **	and since ours has been proven to work and accomplishes
		 **	the same thing, we will keep our fix.
		 */
		 where += align;
		 len -= align;
	 }

	/*	HPNFS	jad	87.09.10
	**	now, if readit() returns a length not divisible by four,
	**	the end of our buffer will again be mis-aligned.  But not
	**	to worry -- the next time we get in here again we will do
	**	the same trick to make sure the buffer remains aligned!!
	*/
	if ((len = (*(rstrm->readit))(rstrm->tcp_handle, where, len)) == -1) {
		TRACE2("fill_input_buf, readit ERROR, errno=%d", errno);
		return (FALSE);
	}
	TRACE2("fill_input_buf readit returned %d bytes", len);
	rstrm->in_finger = where;
	where += len;
	rstrm->in_boundry = where;
	TRACE("fill_input_buf returns TRUE");
	return (TRUE);
}

/* knows nothing about records!  Only about input buffers */
static bool_t
get_input_bytes(rstrm, addr, len)
	register RECSTREAM *rstrm;
	register caddr_t addr;
	register int len;
{
	register int current;

	TRACE("get_input_bytes SOP");
	while (len > 0) {
		current = (int)rstrm->in_boundry - (int)rstrm->in_finger;
		if (current == 0) {
			if (! fill_input_buf(rstrm))
				return (FALSE);
			continue;
		}
		current = (len < current) ? len : current;
		memcpy(addr, rstrm->in_finger, current);
		rstrm->in_finger += current;
		addr += current;
		len -= current;
	}
	TRACE("get_input_bytes returns TRUE");
	return (TRUE);
}

/* next four bytes of the input stream are treated as a header */
static bool_t
set_input_fragment(rstrm)
	register RECSTREAM *rstrm;
{
	u_long header;

	TRACE("set_input_fragment SOP"); 
	if (! get_input_bytes(rstrm, (caddr_t)&header, sizeof(header)))
		return (FALSE);
	header = ntohl(header);
	rstrm->last_frag = ((header & LAST_FRAG) == 0) ? FALSE : TRUE;
	rstrm->fbtbc = header & (~LAST_FRAG);
	return (TRUE);
}

/* consumes input bytes; knows nothing about records! */
static bool_t
skip_input_bytes(rstrm, cnt)
	register RECSTREAM *rstrm;
	int cnt;
{
	register int current;

	TRACE("skip_input_bytes SOP");
	while (cnt > 0) {
		current = (int)rstrm->in_boundry - (int)rstrm->in_finger;
		if (current == 0) {
			if (! fill_input_buf(rstrm))
				return (FALSE);
			continue;
		}
		current = (cnt < current) ? cnt : current;
		rstrm->in_finger += current;
		cnt -= current;
	}
	TRACE("skip_input_bytes returns TRUE");
	return (TRUE);
}
