/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/crt/qfcnvfxt.c,v $
 * $Revision: 64.1 $
 * $State: Exp $        $Locker:  $
 * $Date: 89/02/09 11:08:00 $
 *
 * $Log:	qfcnvfxt.c,v $
 * Revision 64.1  89/02/09  11:08:00  11:08:00  sje
 * Initial S300 version
 * 
 * Revision 1.3  88/01/21  17:10:34  17:10:34  liz (Liz Peters)
 * Changed entry point name.
 * 
 * Revision 1.2  87/10/12  17:45:40  17:45:40  liz (Liz Peters)
 * Changed routine names to new format.
 * 
 * Revision 1.1  87/10/08  13:36:48  13:36:48  liz (Liz Peters)
 * Initial revision
 * 
 * $Endlog$
 *
 *  Spectrum Simulator Assist Floating Point Coprocessor 
 *  Convert Floating-point to Fixed-point and Truncate
 *
 */
#include "float.h"
#include "sgl_float.h"
#include "dbl_float.h"
#include "quad_float.h"
#include "cnv_float.h"

static char rcsid[] = "$Header: qfcnvfxt.c,v 64.1 89/02/09 11:08:00 sje Exp $";
static char hpid[] = "HP Proprietary";

/*
 *  Quad Floating-point to Single Fixed-point 
 *  with truncated result
 */

_U_qsfcnvfxt(srcptr,nullptr,dstptr,status)

quad_floating_point *srcptr;
int *dstptr;
unsigned int *nullptr, *status;
{
	register unsigned int srcp1, srcp2, srcp3, srcp4;
	register unsigned int tempp1,tempp2, tempp3, tempp4;
	register int src_exponent, result;
	register boolean inexact = FALSE;

	Quad_copyfromptr(srcptr,srcp1,srcp2,srcp3,srcp4);
	src_exponent = Quad_exponent(srcp1) - QUAD_BIAS;

	/* 
	 * Test for overflow
	 */
	if (src_exponent > SGL_FX_MAX_EXP) {
		/* check for MININT */
		if (Quad_isoverflow_to_int(src_exponent,srcp1,srcp2)) {
			/* 
			 * Since source is a number which cannot be 
			 * represented in fixed-point format, do 
			 * unimplemented trap 
			 */
			return(UNIMPLEMENTEDEXCEPTION);
		}
	}
	/*
	 * Generate result
	 */
	if (src_exponent >= 0) {
		tempp1 = srcp1;
		tempp2 = srcp2;
		tempp3 = srcp3;
		tempp4 = srcp4;
		Quad_clear_signexponent_set_hidden(tempp1);
		Int_from_quad_mantissa(tempp1,tempp2,src_exponent);
		if (Quad_isone_sign(srcp1) && (src_exponent <= SGL_FX_MAX_EXP))
			result = -Quad_allp1(tempp1);
		else result = Quad_allp1(tempp1);
		*dstptr = result;

		/* check for inexact */
		if (Quad_isinexact_to_fix(srcp1,srcp2,srcp3,srcp4,
		  src_exponent)) {
			if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
			else Set_inexactflag();
                }
	}
	else {
		*dstptr = 0;

		/* check for inexact */
		if (Quad_isnotzero_exponentmantissa(srcp1,srcp2,srcp3,srcp4)) {
			if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
			else Set_inexactflag();
                }
	}
	return(NOEXCEPTION);
}

/*
 *  Quad Floating-point to Double Fixed-point 
 *  with truncated result
 */

_U_qdfcnvfxt(srcptr,nullptr,dstptr,status)

quad_floating_point *srcptr;
dbl_integer *dstptr;
unsigned int *nullptr, *status;
{
	register unsigned int srcp1, srcp2, srcp3, srcp4;
	register unsigned int tempp1,tempp2, tempp3, tempp4;
	register int src_exponent, resultp1;
	register unsigned int resultp2;
	register boolean inexact = FALSE;

	Quad_copyfromptr(srcptr,srcp1,srcp2,srcp3,srcp4);
	src_exponent = Quad_exponent(srcp1) - QUAD_BIAS;

	/* 
	 * Test for overflow
	 */
	if (src_exponent > DBL_FX_MAX_EXP) {
		/* check for MININT */
		if (Quad_isoverflow_to_dint(src_exponent,srcp1,srcp2,srcp3)) {
			/* 
			 * Since source is a number which cannot be 
			 * represented in fixed-point format, do 
			 * unimplemented trap 
			 */
			return(UNIMPLEMENTEDEXCEPTION);
		}
	}
	/*
	 * Generate result
	 */
	if (src_exponent >= 0) {
		tempp1 = srcp1;
		tempp2 = srcp2;
		tempp3 = srcp3;
		tempp4 = srcp4;
		Quad_clear_signexponent_set_hidden(tempp1);
		Dint_from_quad_mantissa(tempp1,tempp2,tempp3,src_exponent,
		  resultp1,resultp2);
		if (Quad_isone_sign(srcp1)) {
			Dint_setone_sign(resultp1,resultp2);
		}
		Dbl_copytoptr(resultp1,resultp2,dstptr);

		/* check for inexact */
		if (Quad_isinexact_to_fix(srcp1,srcp2,srcp3,srcp4,
		  src_exponent)) {
			if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
			else Set_inexactflag();
                }
	}
	else {
		Dint_setzero(resultp1,resultp2);
		Dbl_copytoptr(resultp1,resultp2,dstptr);

		/* check for inexact */
		if (Quad_isnotzero_exponentmantissa(srcp1,srcp2,srcp3,srcp4)) {
			if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
			else Set_inexactflag();
                }
	}
	return(NOEXCEPTION);
}
