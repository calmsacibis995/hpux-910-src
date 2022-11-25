#ifdef MODULE_ID
/*
 * @(#)xdr.h: $Revision: 1.13.83.3 $	$Date: 93/09/17 19:28:54 $
 * $Locker:  $
 */
#endif /* MODULE_ID */
#ifndef _RPC_XDR_INCLUDED
#define _RPC_XDR_INCLUDED
/*
 * REVISION: @(#)10.5
 */

/*
 * xdr.h, External Data Representation Serialization Routines.
 *
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../rpc/types.h"
#else  /* ! _KERNEL_BUILD */
#include <rpc/types.h>
#endif /* _KERNEL_BUILD */

#ifdef __cplusplus
#  define __o	... 		/* arguments to xdr functions */
   extern "C" {
#else /* not __cplusplus */
#  define __o			/* no arguments to xdr functions */
#endif /* not __cplusplus */

/*
 * XDR provides a conventional way for converting between C data
 * types and an external bit-string representation.  Library supplied
 * routines provide for the conversion on built-in C data types.  These
 * routines and utility routines defined here are used to help implement
 * a type encode/decode routine for each user-defined type.
 *
 * Each data type provides a single procedure which takes two arguments:
 *
 *	bool_t
 *	xdrproc(xdrs, argresp)
 *		XDR *xdrs;
 *		<type> *argresp;
 *
 * xdrs is an instance of a XDR handle, to which or from which the data
 * type is to be converted.  argresp is a pointer to the structure to be
 * converted.  The XDR handle contains an operation field which indicates
 * which of the operations (ENCODE, DECODE * or FREE) is to be performed.
 *
 * XDR_DECODE may allocate space if the pointer argresp is null.  This
 * data can be freed with the XDR_FREE operation.
 *
 * We write only one procedure per data type to make it easy
 * to keep the encode and decode procedures for a data type consistent.
 * In many cases the same code performs all operations on a user defined type,
 * because all the hard work is done in the component type routines.
 * decode as a series of calls on the nested data types.
 */

/*
 * Xdr operations.  XDR_ENCODE causes the type to be encoded into the
 * stream.  XDR_DECODE causes the type to be extracted from the stream.
 * XDR_FREE can be used to release the space allocated by an XDR_DECODE
 * request.
 */
enum xdr_op {
	XDR_ENCODE=0,
	XDR_DECODE=1,
	XDR_FREE=2
};

/*
 * This is the number of bytes per unit of external data.
 */
#define BYTES_PER_XDR_UNIT	(4)

#define RNDUP(x)  ((((x) + BYTES_PER_XDR_UNIT - 1) / BYTES_PER_XDR_UNIT) \
                   * BYTES_PER_XDR_UNIT)


/*
 * An xdrproc_t exists for each data type which is to be encoded or decoded.
 *
 * The second argument to the xdrproc_t is a pointer to an opaque pointer.
 * The opaque pointer generally points to a structure of the data type
 * to be decoded.  If this pointer is 0, then the type routines should
 * allocate dynamic storage of the appropriate size and return it.
 * bool_t	(*xdrproc_t)(XDR *, caddr_t *);
 */
typedef	bool_t (*xdrproc_t)(__o);

/*
 * The XDR handle.
 * Contains operation which is being applied to the stream,
 * an operations vector for the paticular implementation (e.g. see xdr_mem.c),
 * and two private fields for the use of the particular impelementation.
 */
typedef struct {
	enum xdr_op	x_op;		/* operation; fast additional param */
	struct xdr_ops {
	    bool_t  (*x_getlong)(__o);	/* get a long from underlying stream */
	    bool_t  (*x_putlong)(__o);	/* put a long to " */
	    bool_t  (*x_getbytes)(__o);	/* get some bytes from " */
	    bool_t  (*x_putbytes)(__o);	/* put some bytes to " */
	    u_int   (*x_getpostn)(__o);	/* returns bytes off from beginning */
	    bool_t  (*x_setpostn)(__o);	/* lets you reposition the stream */
	    long *  (*x_inline)(__o);	/* buf quick ptr to buffered data */
	    void    (*x_destroy)(__o);	/* free privates of this xdr_stream */
	} *x_ops;
	caddr_t 	x_public;	/* users' data */
	caddr_t		x_private;	/* pointer to private data */
	caddr_t 	x_base;		/* private used for position info */
	int		x_handy;	/* extra private word */
} XDR;

/*
 * Operations defined on a XDR handle
 *
 * XDR		*xdrs;
 * long		*longp;
 * caddr_t	 addr;
 * u_int	 len;
 * u_int	 pos;
 */
#define XDR_GETLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)
#define xdr_getlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)

#define XDR_PUTLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)
#define xdr_putlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)

#define XDR_GETBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)
#define xdr_getbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)

#define XDR_PUTBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)
#define xdr_putbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)

#define XDR_GETPOS(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)
#define xdr_getpos(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)

#define XDR_SETPOS(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)
#define xdr_setpos(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)

#define	XDR_INLINE(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)
#define	xdr_inline(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)

#define	XDR_DESTROY(xdrs)				\
	if ((xdrs)->x_ops->x_destroy) 			\
		(*(xdrs)->x_ops->x_destroy)(xdrs)
#define	xdr_destroy(xdrs)				\
	if ((xdrs)->x_ops->x_destroy) 			\
		(*(xdrs)->x_ops->x_destroy)(xdrs)

/*
 * Support struct for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * a entry with a null procedure pointer.  The xdr_union routine gets
 * the discriminant value and then searches the array of structures
 * for a matching value.  If a match is found the associated xdr routine
 * is called to handle that part of the union.  If there is
 * no match, then a default routine may be called.
 * If there is no match and no default routine it is an error.
 */
#define NULL_xdrproc_t ((xdrproc_t)0)
struct xdr_discrim {
	int	value;
	xdrproc_t proc;
};

/*
 * In-line routines for fast encode/decode of primitve data types.
 * Caveat emptor: these use single memory cycles to get the
 * data from the underlying buffer, and will fail to operate
 * properly if the data is not aligned.  The standard way to use these
 * is to say:
 *	if ((buf = XDR_INLINE(xdrs, count)) == NULL)
 *		return (FALSE);
 *	<<< macro calls >>>
 * where ``count'' is the number of bytes of data occupied
 * by the primitive data types.
 *
 * N.B. and frozen for all time: each data type here uses 4 bytes
 * of external representation.
 */
#define IXDR_GET_LONG(buf)		ntohl(*buf++)
#define IXDR_PUT_LONG(buf, v)		(*buf++ = htonl(v))

#define IXDR_GET_BOOL(buf)		((bool_t)IXDR_GET_LONG(buf))
#define IXDR_GET_ENUM(buf, t)		((t)IXDR_GET_LONG(buf))
#define IXDR_GET_U_LONG(buf)		((u_long)IXDR_GET_LONG(buf))
#define IXDR_GET_SHORT(buf)		((short)IXDR_GET_LONG(buf))
#define IXDR_GET_U_SHORT(buf)		((u_short)IXDR_GET_LONG(buf))

#define IXDR_PUT_BOOL(buf, v)		IXDR_PUT_LONG((buf), ((long)(v)))
#define IXDR_PUT_ENUM(buf, v)		IXDR_PUT_LONG((buf), ((long)(v)))
#define IXDR_PUT_U_LONG(buf, v)		IXDR_PUT_LONG((buf), ((long)(v)))
#define IXDR_PUT_SHORT(buf, v)		IXDR_PUT_LONG((buf), ((long)(v)))
#define IXDR_PUT_U_SHORT(buf, v)	IXDR_PUT_LONG((buf), ((long)(v)))

/*
 * These are the "generic" xdr routines.
 */
extern bool_t	xdr_void(__o);
extern bool_t	xdr_int(__o);
extern bool_t	xdr_u_int(__o);
extern bool_t	xdr_long(__o);
extern bool_t	xdr_u_long(__o);
extern bool_t	xdr_short(__o);
extern bool_t	xdr_u_short(__o);
extern bool_t	xdr_bool(__o);
extern bool_t	xdr_enum(__o);
extern bool_t	xdr_array(__o);
extern bool_t	xdr_bytes(__o);
extern bool_t	xdr_opaque(__o);
extern bool_t	xdr_string(__o);
extern bool_t	xdr_union(__o);
#ifndef _KERNEL
extern bool_t	xdr_float(__o);
extern bool_t	xdr_double(__o);
extern bool_t	xdr_reference(__o);
extern bool_t	xdr_wrapstring(__o);
extern bool_t 	xdr_char(__o);
extern bool_t 	xdr_u_char(__o);
extern bool_t 	xdr_vector(__o);
extern bool_t 	xdr_pointer(__o);
extern void 	xdr_free(__o);
#endif /* !_KERNEL */

/*
 * Common opaque bytes objects used by many rpc protocols;
 * declared here due to commonality.
 */
#define MAX_NETOBJ_SZ 1024 
struct netobj {
	u_int	n_len;
	char	*n_bytes;
};
typedef struct netobj netobj;
extern bool_t   xdr_netobj(__o);

/*
 * These are the public routines for the various implementations of
 * xdr streams.
 */
extern void   xdrmem_create(__o);	/* XDR using memory buffers */
#ifndef _KERNEL
extern void   xdrstdio_create(__o);	/* XDR using stdio library */
extern void   xdrrec_create(__o);	/* XDR pseudo records for tcp */
extern bool_t xdrrec_endofrecord(__o);	/* make end of xdr record */
extern bool_t xdrrec_skiprecord(__o);	/* move to begining of next record */
extern bool_t xdrrec_eof(__o);		/* true iff no more input */
#else
extern void xdrmbuf_init(__o);		/* XDR using kernel mbufs */
#endif /* !_KERNEL */

#undef __o

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _RPC_XDR_INCLUDED */
