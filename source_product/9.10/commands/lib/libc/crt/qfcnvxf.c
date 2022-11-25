#include "float.h"
#include "sgl_float.h"
#include "dbl_float.h"
#include "quad_float.h"
#include "cnv_float.h"

static char rcsid[] = "$Header: qfcnvxf.c,v 66.1 89/07/19 15:11:54 egeland Exp $";
static char hpid[] = "HP Proprietary";

/*
 *  Single Fixed-point to Quad Floating-point 
 */

_U_sqfcnvxf(srcptr,nullptr,dstptr,status)

int *srcptr;
quad_floating_point *dstptr;
unsigned int *nullptr, *status;
{
	register int src, dst_exponent;
	register unsigned int resultp1=0, resultp2, resultp3, resultp4;

	src = *srcptr;
	/* 
	 * set sign bit of result and get magnitude of source 
	 */
	if (src < 0) {
		Quad_setone_sign(resultp1);  
		Int_negate(src);
	}
	else {
		Quad_setzero_sign(resultp1);
        	/* Check for zero */
        	if (src == 0) {
                	Quad_copytoptr(0,0,0,0,dstptr);
                	return(NOEXCEPTION);
        	}
	}
	/*
	 * Generate exponent and normalized mantissa
	 */
	dst_exponent = 16;    /* initialize for normalization */
	/*
	 * Check word for most significant bit set.  Returns
	 * a value in dst_exponent indicating the bit position,
	 * between -1 and 30.
	 */
	Find_ms_one_bit(src,dst_exponent);
	/*  left justify source, with msb at bit position 1  */
	if (dst_exponent >= 0) src <<= dst_exponent;
	else src = 1 << 30;
	Quad_set_mantissap1(resultp1, src >> QUAD_EXP_LENGTH - 1);
	Quad_set_mantissap2(resultp2, src << (33-QUAD_EXP_LENGTH));
	Quad_set_mantissap3(resultp3, 0); 
	Quad_set_mantissap4(resultp4, 0);
	Quad_set_exponent(resultp1, (30+QUAD_BIAS) - dst_exponent);
	Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	return(NOEXCEPTION);
}

/*
 *  Double Fixed-point to Quad Floating-point 
 */

_U_dqfcnvxf(srcptr,nullptr,dstptr,status)

dbl_integer *srcptr;
quad_floating_point *dstptr;
unsigned int *nullptr, *status;
{
	register int srcp1, dst_exponent;
	register unsigned int srcp2, resultp1=0, resultp2, resultp3, resultp4;

	Dint_copyfromptr(srcptr,srcp1,srcp2);
	/* 
	 * set sign bit of result and get magnitude of source 
	 */
	if (srcp1 < 0) {
		Quad_setone_sign(resultp1);  
		Dint_negate(srcp1,srcp2);
	}
	else {
		Quad_setzero_sign(resultp1);
        	/* Check for zero */
        	if (srcp1 == 0 && srcp2 == 0) {
                	Quad_copytoptr(0,0,0,0,dstptr);
                	return(NOEXCEPTION);
        	}
	}
	/*
	 * Generate exponent and normalized mantissa
	 */
	dst_exponent = 16;    /* initialize for normalization */
	if (srcp1 == 0) {
		/*
		 * Check word for most significant bit set.  Returns
		 * a value in dst_exponent indicating the bit position,
		 * between -1 and 30.
		 */
		Find_ms_one_bit(srcp2,dst_exponent);
		/*  left justify source, with msb at bit position 1  */
		if (dst_exponent >= 0) {
			srcp1 = srcp2 << dst_exponent;    
			srcp2 = 0;
		}
		else {
			srcp1 = srcp2 >> 1;
			srcp2 <<= 31; 
		}
		/*
		 *  since msb set is in second word, need to 
		 *  adjust bit position count
		 */
		dst_exponent += 32;
	}
	else {
		Find_ms_one_bit(srcp1,dst_exponent);
		/*  left justify source, with msb at bit position 1  */
		if (dst_exponent >= 0) {
			Variableshiftdouble(srcp1,srcp2,32-dst_exponent,srcp1);
			srcp2 <<= dst_exponent;    
		}
		else {
			srcp1 = 1 << 30;	/* MININT */
			srcp2 = 0;
		}
	}
	Quad_set_mantissap1(resultp1, srcp1 >> QUAD_EXP_LENGTH - 1);
	Quad_set_mantissap2(resultp2, 
	  (srcp1 << (33 - QUAD_EXP_LENGTH) | srcp2 >> QUAD_EXP_LENGTH -1));
	Quad_set_mantissap3(resultp3, srcp2 << (33 - QUAD_EXP_LENGTH)); 
	Quad_set_mantissap4(resultp4, 0);
	Quad_set_exponent(resultp1, (62+QUAD_BIAS) - dst_exponent);
	Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	return(NOEXCEPTION);
}
