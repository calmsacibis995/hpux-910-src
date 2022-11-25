/* SCCSID strings for NFS/300 group */
/* %Z%%I%	[%E%  %U%] */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_float.c,v $
 * $Revision: 12.2 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:38 $
 *
 * Revision 12.1  90/03/21  10:47:31  10:47:31  dlr
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:09:36  16:09:36  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.1  89/01/26  13:22:12  13:22:12  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.5  89/01/26  12:21:18  12:21:18  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.4  87/08/07  15:11:48  15:11:48  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  11:18:43  11:18:43  cmahon (Christina Mahon)
 * Merged with 300 version.  Changed ifdefs to make 800 same as 300.
 * 
 * Revision 1.1.10.2  87/07/16  22:29:29  22:29:29  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  11:45:48  11:45:48  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr_float.c,v 12.2 92/02/07 17:08:38 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * xdr_float.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "floating point" xdr routines used to (de)serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define xdr_double 	_xdr_double	/* In this file */
#define xdr_float 	_xdr_float	/* In this file */

#endif /* _NAMESPACE_CLEAN */

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <stdio.h>
#include <arpa/trace.h>

/*
 * NB: Not portable.
 * This routine works on Suns (Sky / 68000's) and Vaxen.
**	HPNFS	jad	87.05.04
**	At first, it does not appear to work on HP-UX; some cases fail.
**	Adding TRACE() code to check out the problems ...
**	HPNFS	jad	87.07.30
**	Fixing xdr_float and xdr_double to work on s800 also (IEEE) --
**	define INTERNAL_IEEE for those who don't have to do conversion
*/

#if	defined(mc68000) || defined(hp9000s200) || defined(hp9000s800)
/*
**	then this hardware uses IEEE as its internal format --
**	the float conversion is essentially a no-op ... (how nice!)
*/
# define	INTERNAL_IEEE
#endif

#ifdef	vax
/*
**	HPNFS	jad	87.07.30
**	hide this junk on HP-UX -- we don't need this noise in our executables
*/

/* What IEEE single precision floating point looks like on a Vax */
struct  ieee_single {
	unsigned int	mantissa: 23;
	unsigned int	exp     : 8;
	unsigned int	sign    : 1;
};

/* Vax single precision floating point */
struct  vax_single {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;

    };

#define VAX_SNG_BIAS    0x81
#define IEEE_SNG_BIAS   0x7f

static struct sgl_limits {
	struct vax_single s;
	struct ieee_single ieee;
} sgl_limits[2] = {
	{{ 0x3f, 0xff, 0x0, 0xffff },	/* Max Vax */
	{ 0x0, 0xff, 0x0 }},		/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0 },	/* Min Vax */
	{ 0x0, 0x0, 0x0 }}		/* Min IEEE */
};

#endif	/* vax */


#ifdef _NAMESPACE_CLEAN
#undef xdr_float
#pragma _HP_SECONDARY_DEF _xdr_float xdr_float
#define xdr_float _xdr_float
#endif

bool_t
xdr_float(xdrs, fp)
	register XDR *xdrs;
	register float *fp;
{
#ifdef	vax
/*
**	HPNFS	jad	87.07.30
**	hide this junk on HP-UX -- we don't need this in our executables
*/
	struct ieee_single is;
	struct vax_single vs, *vsp;
	struct sgl_limits *lim;
	int i;
#endif	/* vax */

	TRACE("xdr_float SOP");
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		TRACE("xdr_float case XDR_ENCODE");
#ifdef	INTERNAL_IEEE
		TRACE("xdr_float IEEE code");
		return (XDR_PUTLONG(xdrs, (long *)fp));
#else
# ifdef	vax
		TRACE("xdr_float VAX code");
		vs = *((struct vax_single *)fp);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((vs.mantissa2 == lim->s.mantissa2) &&
				(vs.exp == lim->s.exp) &&
				(vs.mantissa1 == lim->s.mantissa1)) {
				is = lim->ieee;
				goto shipit;
			}
		}
		is.exp = vs.exp - VAX_SNG_BIAS + IEEE_SNG_BIAS;
		is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
	shipit:
		is.sign = vs.sign;
		return (XDR_PUTLONG(xdrs, (long *)&is));
#else

	Note: this is not a comment!
	THIS CASE IS AN ERROR and will not compile ...
	You have to make sure to either define INTERNAL_IEEE, or
	write the code that will do the internal --> IEEE float translation!

#endif
#endif

	case XDR_DECODE:
		TRACE("xdr_float case XDR_DECODE");
#ifdef	INTERNAL_IEEE
		TRACE("xdr_float mc68000 code");
		return (XDR_GETLONG(xdrs, (long *)fp));
#else
# ifdef	vax
		TRACE("xdr_float VAX code");
		vsp = (struct vax_single *)fp;
		if (!XDR_GETLONG(xdrs, (long *)&is))
			return (FALSE);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((is.exp == lim->ieee.exp) &&
				(is.mantissa = lim->ieee.mantissa)) {
				*vsp = lim->s;
				goto doneit;
			}
		}
		vsp->exp = is.exp - IEEE_SNG_BIAS + VAX_SNG_BIAS;
		vsp->mantissa2 = is.mantissa;
		vsp->mantissa1 = (is.mantissa >> 16);
	doneit:
		vsp->sign = is.sign;
		return (TRUE);
#else

	Note: this is not a comment!
	THIS CASE IS AN ERROR and will not compile ...
	You have to make sure to either define INTERNAL_IEEE, or
	write the code that will do the internal --> IEEE float translation!

#endif
#endif

	case XDR_FREE:
		TRACE("xdr_float case XDR_FREE");
		return (TRUE);
	}

	TRACE("xdr_float fell out of switch");
	return (FALSE);
}


#ifdef	vax
/*
**	HPNFS	jad	87.07.30
**	hide this junk on HP-UX -- we don't need this noise in our executables
*/

/* What IEEE double precision floating point looks like on a Vax */
struct  ieee_double {
	unsigned int	mantissa1 : 20;
	unsigned int	exp	  : 11;
	unsigned int	sign	  : 1;
	unsigned int	mantissa2 : 32;
};

/* Vax double precision floating point */
struct  vax_double {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;
	unsigned int	mantissa3 : 16;
	unsigned int	mantissa4 : 16;
};

#define VAX_DBL_BIAS    0x81
#define IEEE_DBL_BIAS   0x3ff
#define MASK(nbits)     ((1 << nbits) - 1)

static struct dbl_limits {
	struct  vax_double d;
	struct  ieee_double ieee;
} dbl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff, 0xffff, 0xffff },	/* Max Vax */
	{ 0x0, 0x7ff, 0x0, 0x0 }},			/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},		/* Min Vax */
	{ 0x0, 0x0, 0x0, 0x0 }}				/* Min IEEE */
};

#endif	/* vax */


#ifdef _NAMESPACE_CLEAN
#undef xdr_double
#pragma _HP_SECONDARY_DEF _xdr_double xdr_double
#define xdr_double _xdr_double
#endif

bool_t
xdr_double(xdrs, dp)
	register XDR *xdrs;
	double *dp;
{
	register long *lp;
#ifdef	vax
/*
**	HPNFS	jad	87.07.30
**	hide this junk on HP-UX -- we don't need this in our executables
*/
	struct  ieee_double id;
	struct  vax_double vd;
	register struct dbl_limits *lim;
	int i;
#endif	/* vax */

	TRACE("xdr_double SOP");
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		TRACE("xdr_double case XDR_ENCODE");
#ifdef	INTERNAL_IEEE
		TRACE("xdr_double mc68000 code");
		lp = (long *)dp;
#else
# ifdef	vax
		TRACE("xdr_double VAX code");
		vd = *((struct  vax_double *)dp);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((vd.mantissa4 == lim->d.mantissa4) &&
				(vd.mantissa3 == lim->d.mantissa3) &&
				(vd.mantissa2 == lim->d.mantissa2) &&
				(vd.mantissa1 == lim->d.mantissa1) &&
				(vd.exp == lim->d.exp)) {
				id = lim->ieee;
				goto shipit;
			}
		}
		id.exp = vd.exp - VAX_DBL_BIAS + IEEE_DBL_BIAS;
		id.mantissa1 = (vd.mantissa1 << 13) | (vd.mantissa2 >> 3);
		id.mantissa2 = ((vd.mantissa2 & MASK(3)) << 29) |
				(vd.mantissa3 << 13) |
				((vd.mantissa4 >> 3) & MASK(13));
	shipit:
		id.sign = vd.sign;
		lp = (long *)&id;
#else

	Note: this is not a comment!
	THIS CASE IS AN ERROR and will not compile ...
	You have to make sure to either define INTERNAL_IEEE, or
	write the code that will do the internal --> IEEE float translation!

#endif
#endif

		return (XDR_PUTLONG(xdrs, lp++) && XDR_PUTLONG(xdrs, lp));


	case XDR_DECODE:
		TRACE("xdr_double case XDR_DECODE");
#ifdef	INTERNAL_IEEE
		TRACE("xdr_double mc68000 code");
		lp = (long *)dp;
		return (XDR_GETLONG(xdrs, lp++) && XDR_GETLONG(xdrs, lp));
#else
# ifdef	vax
		TRACE("xdr_double VAX code");
		lp = (long *)&id;
		if (!XDR_GETLONG(xdrs, lp++) || !XDR_GETLONG(xdrs, lp))
			return (FALSE);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((id.mantissa2 == lim->ieee.mantissa2) &&
				(id.mantissa1 == lim->ieee.mantissa1) &&
				(id.exp == lim->ieee.exp)) {
				vd = lim->d;
				goto doneit;
			}
		}
		vd.exp = id.exp - IEEE_DBL_BIAS + VAX_DBL_BIAS;
		vd.mantissa1 = (id.mantissa1 >> 13);
		vd.mantissa2 = ((id.mantissa1 & MASK(13)) << 3) |
				(id.mantissa2 >> 29);
		vd.mantissa3 = (id.mantissa2 >> 13);
		vd.mantissa4 = (id.mantissa2 << 3);
	doneit:
		vd.sign = id.sign;
		*dp = *((double *)&vd);
		return (TRUE);
#else

	Note: this is not a comment!
	THIS CASE IS AN ERROR and will not compile ...
	You have to make sure to either define INTERNAL_IEEE, or
	write the code that will do the internal --> IEEE float translation!

#endif
#endif

	case XDR_FREE:
		TRACE("xdr_double case XDR_FREE");
		return (TRUE);
	}

	TRACE("xdr_double fell out of switch");
	return (FALSE);
}
