#include "float.h"
#include "sgl_float.h"
#include "dbl_float.h"
#include "quad_float.h"
#include "cnv_float.h"

static char rcsid[] = "$Header: qfcnvff.c,v 64.1 89/02/09 11:07:57 sje Exp $";
static char hpid[] = "HP Proprietary";

/*
 *  Single Floating-point to Quad Floating-point 
 */

_U_sqfcnvff(srcptr,nullptr,dstptr,status)

sgl_floating_point *srcptr;
quad_floating_point *dstptr;
unsigned int *nullptr, *status;
{
	register unsigned int src, resultp1, resultp2, resultp3, resultp4;
	register int src_exponent;

	src = *srcptr;
	src_exponent = Sgl_exponent(src);
	Quad_allp1(resultp1) = Sgl_all(src);  /* set sign of result */
	/* 
 	 * Test for NaN or infinity
 	 */
	if (src_exponent == SGL_INFINITY_EXPONENT) {
		/*
		 * determine if NaN or infinity
		 */
		if (Sgl_iszero_mantissa(src)) {
			/*
			 * is infinity; want to return double infinity
			 */
			Quad_setinfinity_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
		else {
			/* 
			 * is NaN; signaling or quiet?
			 */
			if (Sgl_isone_signaling(src)) {
				/* trap if INVALIDTRAP enabled */
				if (Is_invalidtrap_enabled())
					return(INVALIDEXCEPTION);
				/* make NaN quiet */
				else {
					Set_invalidflag();
					Sgl_set_quiet(src);
				}
			}
			/* 
			 * NaN is quiet, return as double NaN 
			 */
			Quad_setinfinity_exponent(resultp1);
			Sgl_to_quad_mantissa(src,resultp1,resultp2,resultp3,
			 resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
	}
	/* 
 	 * Test for zero or denormalized
 	 */
	if (src_exponent == 0) {
		/*
		 * determine if zero or denormalized
		 */
		if (Sgl_isnotzero_mantissa(src)) {
			/*
			 * is denormalized; want to normalize
			 */
			Sgl_clear_signexponent(src);
			Sgl_leftshiftby1(src);
			Sgl_normalize(src,src_exponent);
			Sgl_to_quad_exponent(src_exponent,resultp1);
			Sgl_to_quad_mantissa(src,resultp1,resultp2,resultp3,
			 resultp4);
		}
		else {
			Quad_setzero_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
		}
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		return(NOEXCEPTION);
	}
	/*
	 * No special cases, just complete the conversion
	 */
	Sgl_to_quad_exponent(src_exponent, resultp1);
	Sgl_to_quad_mantissa(Sgl_mantissa(src), resultp1,resultp2,resultp3,
	 resultp4);
	Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	return(NOEXCEPTION);
}

/*
 *  Double Floating-point to Quad Floating-point 
 */

_U_dqfcnvff(srcptr,nullptr,dstptr,status)

dbl_floating_point *srcptr;
quad_floating_point *dstptr;
unsigned int *nullptr, *status;
{
	register unsigned int srcp1, srcp2;
	register unsigned int resultp1, resultp2, resultp3, resultp4;
	register int src_exponent;

	Dbl_copyfromptr(srcptr,srcp1,srcp2);
	src_exponent = Dbl_exponent(srcp1);
	Quad_allp1(resultp1) = Dbl_allp1(srcp1);  /* set sign of result */
	/* 
 	 * Test for NaN or infinity
 	 */
	if (src_exponent == DBL_INFINITY_EXPONENT) {
		/*
		 * determine if NaN or infinity
		 */
		if (Dbl_iszero_mantissa(srcp1,srcp2)) {
			/*
			 * is infinity; want to return double infinity
			 */
			Quad_setinfinity_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
		else {
			/* 
			 * is NaN; signaling or quiet?
			 */
			if (Dbl_isone_signaling(srcp1)) {
				/* trap if INVALIDTRAP enabled */
				if (Is_invalidtrap_enabled())
					return(INVALIDEXCEPTION);
				/* make NaN quiet */
				else {
					Set_invalidflag();
					Dbl_set_quiet(srcp1);
				}
			}
			/* 
			 * NaN is quiet, return as double NaN 
			 */
			Quad_setinfinity_exponent(resultp1);
			Dbl_to_quad_mantissa(srcp1,srcp2,resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
	}
	/* 
 	 * Test for zero or denormalized
 	 */
	if (src_exponent == 0) {
		/*
		 * determine if zero or denormalized
		 */
		if (Dbl_isnotzero_mantissa(srcp1,srcp2)) {
			/*
			 * is denormalized; want to normalize
			 */
			Dbl_clear_signexponent(srcp1);
			Dbl_leftshiftby1(srcp1,srcp2);
			Dbl_normalize(srcp1,srcp2,src_exponent);
			Dbl_to_quad_exponent(src_exponent,resultp1);
			Dbl_to_quad_mantissa(srcp1,srcp2,resultp1,resultp2,
			 resultp3,resultp4);
		}
		else {
			Quad_setzero_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
		}
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		return(NOEXCEPTION);
	}
	/*
	 * No special cases, just complete the conversion
	 */
	Dbl_to_quad_exponent(src_exponent, resultp1);
	Dbl_to_quad_mantissa(Dbl_mantissap1(srcp1),srcp2,resultp1,resultp2,
	 resultp3,resultp4);
	Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	return(NOEXCEPTION);
}

/*
 *  Quad Floating-point to Single Floating-point 
 */

_U_qsfcnvff(srcptr,nullptr,dstptr,status)

quad_floating_point *srcptr;
sgl_floating_point *dstptr;
unsigned int *nullptr, *status;
{
        register unsigned int srcp1, srcp2, srcp3, srcp4, result;
        register int src_exponent, dest_exponent, dest_mantissa;
        register boolean inexact = FALSE, guardbit = FALSE, stickybit = FALSE;
	register boolean lsb_odd = FALSE;

	Quad_copyfromptr(srcptr,srcp1,srcp2,srcp3,srcp4);
        src_exponent = Quad_exponent(srcp1);
	Sgl_all(result) = Quad_allp1(srcp1);  /* set sign of result */
        /* 
         * Test for NaN or infinity
         */
        if (src_exponent == QUAD_INFINITY_EXPONENT) {
                /*
                 * determine if NaN or infinity
                 */
                if (Quad_iszero_mantissa(srcp1,srcp2,srcp3,srcp4)) {
                        /*
                         * is infinity; want to return single infinity
                         */
                        Sgl_setinfinity_exponentmantissa(result);
                        *dstptr = result;
                        return(NOEXCEPTION);
                }
                /* 
                 * is NaN; signaling or quiet?
                 */
                if (Quad_isone_signaling(srcp1)) {
                        /* trap if INVALIDTRAP enabled */
                        if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
                        else {
				Set_invalidflag();
                        	/* make NaN quiet */
                        	Quad_set_quiet(srcp1);
			}
                }
                /* 
                 * NaN is quiet, return as single NaN 
                 */
                Sgl_setinfinity_exponent(result);
		Sgl_set_mantissa(result,Qallp1(srcp1)<<7 | Qallp2(srcp2)>>25);
		if (Sgl_iszero_mantissa(result)) Sgl_set_quiet(result);
                *dstptr = result;
                return(NOEXCEPTION);
        }
        /*
         * Generate result
         */
        Quad_to_sgl_exponent(src_exponent,dest_exponent);
	if (dest_exponent > 0) {
        	Quad_to_sgl_mantissa(srcp1,srcp2,srcp3,srcp4,dest_mantissa,
		 inexact,guardbit,stickybit,lsb_odd);
	}
	else {
		if (Quad_iszero_exponentmantissa(srcp1,srcp2,srcp3,srcp4)){
			Sgl_setzero_exponentmantissa(result);
			*dstptr = result;
			return(NOEXCEPTION);
		}
                if (Is_underflowtrap_enabled()) {
			Quad_to_sgl_mantissa(srcp1,srcp2,srcp3,srcp4,
			 dest_mantissa,inexact,guardbit,stickybit,lsb_odd);
                }
		else {
			Quad_to_sgl_denormalized(srcp1,srcp2,srcp3,srcp4,
			 dest_exponent,dest_mantissa,inexact,guardbit,stickybit,
			 lsb_odd);
		}
	}
        /* 
         * Now round result if not exact
         */
        if (inexact) {
                switch (Rounding_mode()) {
                        case ROUNDPLUS: 
                                if (Sgl_iszero_sign(result)) dest_mantissa++;
                                break;
                        case ROUNDMINUS: 
                                if (Sgl_isone_sign(result)) dest_mantissa++;
                                break;
                        case ROUNDNEAREST:
                                if (guardbit) {
                                   if (stickybit || lsb_odd) dest_mantissa++;
                                   }
                }
        }
        Sgl_set_exponentmantissa(result,dest_mantissa);
        /*
         * check for mantissa overflow after rounding
         */
        if (Sgl_isone_hidden(result)) dest_exponent++;

        /* 
         * Test for overflow
         */
        if (dest_exponent >= SGL_INFINITY_EXPONENT) {
                /* trap if OVERFLOWTRAP enabled */
                if (Is_overflowtrap_enabled()) {
                        /* 
                         * Check for gross overflow
                         */
                        if (dest_exponent >= SGL_INFINITY_EXPONENT+SGL_WRAP) 
                        	return(UNIMPLEMENTEDEXCEPTION);
                        
                        /*
                         * Adjust bias of result
                         */
			Sgl_setwrapped_exponent(result,dest_exponent,ovfl);
			*dstptr = result;
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return(OVERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else Set_inexactflag();
                        return(OVERFLOWEXCEPTION);
                }
                Set_overflowflag();
		inexact = TRUE;
		/* set result to infinity or largest number */
		Sgl_setoverflow(result);
        }
        /* 
         * Test for underflow
         */
        else if (dest_exponent <= 0) {
                /* trap if UNDERFLOWTRAP enabled */
                if (Is_underflowtrap_enabled()) {
                        /* 
                         * Check for gross underflow
                         */
                        if (dest_exponent <= -(SGL_WRAP))
                        	return(UNIMPLEMENTEDEXCEPTION);
                        /*
                         * Adjust bias of result
                         */
			Sgl_setwrapped_exponent(result,dest_exponent,unfl);
			*dstptr = result;
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return(UNDERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else Set_inexactflag();
                        return(UNDERFLOWEXCEPTION);
                }
                /* 
                 * result is denormalized or signed zero
                 */
		if (inexact) Set_underflowflag();
        }
	else Sgl_set_exponent(result,dest_exponent);
	*dstptr = result;
        /* 
         * Trap if inexact trap is enabled
         */
        if (inexact)
        	if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
        	else Set_inexactflag();
        return(NOEXCEPTION);
}
 
/*
 *  Quad Floating-point to Double Floating-point 
 */

_U_qdfcnvff(srcptr,nullptr,dstptr,status)

quad_floating_point *srcptr;
dbl_floating_point *dstptr;
unsigned int *nullptr, *status;
{
        register unsigned int srcp1, srcp2, srcp3, srcp4, resultp1, resultp2;
        register int src_exponent, dest_exponent;
        register int dest_mantissap1, dest_mantissap2;
        register boolean inexact = FALSE, guardbit = FALSE, stickybit = FALSE;
	register boolean lsb_odd = FALSE;

	Quad_copyfromptr(srcptr,srcp1,srcp2,srcp3,srcp4);
        src_exponent = Quad_exponent(srcp1);
	Dbl_allp1(resultp1) = Quad_allp1(srcp1);  /* set sign of result */
        /* 
         * Test for NaN or infinity
         */
        if (src_exponent == QUAD_INFINITY_EXPONENT) {
                /*
                 * determine if NaN or infinity
                 */
                if (Quad_iszero_mantissa(srcp1,srcp2,srcp3,srcp4)) {
                        /*
                         * is infinity; want to return single infinity
                         */
                        Dbl_setinfinity_exponentmantissa(resultp1,resultp2);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
                        return(NOEXCEPTION);
                }
                /* 
                 * is NaN; signaling or quiet?
                 */
                if (Quad_isone_signaling(srcp1)) {
                        /* trap if INVALIDTRAP enabled */
                        if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
                        else {
				Set_invalidflag();
                        	/* make NaN quiet */
                        	Quad_set_quiet(srcp1);
			}
                }
                /* 
                 * NaN is quiet, return as single NaN 
                 */
                Dbl_setinfinity_exponent(resultp1);
		Dbl_set_mantissap1(resultp1,Qallp1(srcp1)<<4|Qallp2(srcp2)>>28);
		Dbl_set_mantissap2(resultp2,Qallp2(srcp2)<<4|Qallp3(srcp3)>>28);
		if (Dbl_iszero_mantissa(resultp1,resultp2)) 
			Dbl_set_quiet(resultp1);
		Dbl_copytoptr(resultp1,resultp2,dstptr);
                return(NOEXCEPTION);
        }
        /*
         * Generate result
         */
        Quad_to_dbl_exponent(src_exponent,dest_exponent);
	if (dest_exponent > 0) {
        	Quad_to_dbl_mantissa(srcp1,srcp2,srcp3,srcp4,dest_mantissap1,
		 dest_mantissap2,inexact,guardbit,stickybit,lsb_odd);
	}
	else {
		if (Quad_iszero_exponentmantissa(srcp1,srcp2,srcp3,srcp4)){
			Dbl_setzero_exponentmantissa(resultp1,resultp2);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
			return(NOEXCEPTION);
		}
                if (Is_underflowtrap_enabled()) {
			Quad_to_dbl_mantissa(srcp1,srcp2,srcp3,srcp4,
			 dest_mantissap1,dest_mantissap2,inexact,guardbit,
			 stickybit,lsb_odd);
                }
		else {
			Quad_to_dbl_denormalized(srcp1,srcp2,srcp3,srcp4,
			 dest_exponent,dest_mantissap1,dest_mantissap2,inexact,
			 guardbit,stickybit,lsb_odd);
		}
	}
        /* 
         * Now round result if not exact
         */
        if (inexact) {
                switch (Rounding_mode()) {
                        case ROUNDPLUS: 
                                if (Dbl_iszero_sign(resultp1))
				 Dbl_increment(dest_mantissap1,dest_mantissap2);
                                break;
                        case ROUNDMINUS: 
                                if (Dbl_isone_sign(resultp1))
				 Dbl_increment(dest_mantissap1,dest_mantissap2);
                                break;
                        case ROUNDNEAREST:
                                if (guardbit) {
                                   if (stickybit || lsb_odd)
				      Dbl_increment(dest_mantissap1,
				       dest_mantissap2);
                                   }
                }
        }
        Dbl_set_exponentmantissa(resultp1,resultp2,
	 dest_mantissap1,dest_mantissap2);
        /*
         * check for mantissa overflow after rounding
         */
        if (Dbl_isone_hidden(resultp1)) dest_exponent++;

        /* 
         * Test for overflow
         */
        if (dest_exponent >= DBL_INFINITY_EXPONENT) {
                /* trap if OVERFLOWTRAP enabled */
                if (Is_overflowtrap_enabled()) {
                        /* 
                         * Check for gross overflow
                         */
                        if (dest_exponent >= DBL_INFINITY_EXPONENT+DBL_WRAP) 
                        	return(UNIMPLEMENTEDEXCEPTION);
                        
                        /*
                         * Adjust bias of result
                         */
			Dbl_setwrapped_exponent(resultp1,dest_exponent,ovfl);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return(OVERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else Set_inexactflag();
                        return(OVERFLOWEXCEPTION);
                }
                Set_overflowflag();
		inexact = TRUE;
		/* set result to infinity or largest number */
		Dbl_setoverflow(resultp1,resultp2);
        }
        /* 
         * Test for underflow
         */
        else if (dest_exponent <= 0) {
                /* trap if UNDERFLOWTRAP enabled */
                if (Is_underflowtrap_enabled()) {
                        /* 
                         * Check for gross underflow
                         */
                        if (dest_exponent <= -(DBL_WRAP))
                        	return(UNIMPLEMENTEDEXCEPTION);
                        /*
                         * Adjust bias of result
                         */
			Dbl_setwrapped_exponent(resultp1,dest_exponent,unfl);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return(UNDERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else Set_inexactflag();
                        return(UNDERFLOWEXCEPTION);
                }
                /* 
                 * result is denormalized or signed zero
                 */
		if (inexact) Set_underflowflag();
        }
	else Dbl_set_exponent(resultp1,dest_exponent);
	Dbl_copytoptr(resultp1,resultp2,dstptr);
        /* 
         * Trap if inexact trap is enabled
         */
        if (inexact)
        	if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
        	else Set_inexactflag();
        return(NOEXCEPTION);
}
