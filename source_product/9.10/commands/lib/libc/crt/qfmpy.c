/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/crt/qfmpy.c,v $
 * $Revision: 64.1 $     $Author: sje $
 * $State: Exp $        $Locker:  $
 * $Date: 89/02/09 11:08:09 $
 *
 * $Log:	qfmpy.c,v $
 * Revision 64.1  89/02/09  11:08:09  11:08:09  sje
 * Initial S300 version
 * 
 * Revision 1.3  88/01/21  17:10:50  17:10:50  liz (Liz Peters)
 * Changed entry point name.
 * 
 * Revision 1.2  87/10/13  17:15:30  17:15:30  liz (Liz Peters)
 * Inexact was not being calculated correctly for denormalized numbers.
 * This change was made in quad_float.h, but is logged here to indicate
 * the bug fix.
 * 
 * Revision 1.1  87/10/08  13:36:51  13:36:51  liz (Liz Peters)
 * Initial revision
 * 
 * $Endlog$
 *
 *  Spectrum Simulator Assist Floating Point Coprocessor 
 *  Quad Precision Floating-point Multiply
 *  Liz Peters
 *
 *  *** HP Company Confidential ***
 * 
 */
#include "float.h"
#include "quad_float.h"

static char rcsid[] = "$Header: qfmpy.c,v 64.1 89/02/09 11:08:09 sje Exp $";
static char hpid[] = "HP Proprietary";

/*
 *  Quad Precision Floating-point Multiply
 */

_U_qfmpy(srcptr1,srcptr2,dstptr,status)

quad_floating_point *srcptr1, *srcptr2, *dstptr;
unsigned int *status;
{
	register unsigned int opnd1p1, opnd1p2, opnd1p3, opnd1p4;
	register unsigned int opnd2p1, opnd2p2, opnd2p3, opnd2p4;
	register unsigned int opnd3p1, opnd3p2, opnd3p3, opnd3p4;
	register unsigned int resultp1, resultp2, resultp3, resultp4;
	register int dest_exponent, count;
	register boolean inexact = FALSE, guardbit = FALSE, stickybit = FALSE;

	Quad_copyfromptr(srcptr1,opnd1p1,opnd1p2,opnd1p3,opnd1p4);
	Quad_copyfromptr(srcptr2,opnd2p1,opnd2p2,opnd2p3,opnd2p4);

	/* 
	 * set sign bit of result 
	 */
	if (Quad_sign(opnd1p1) ^ Quad_sign(opnd2p1)) 
		Quad_setnegativezerop1(resultp1); 
	else Quad_setzerop1(resultp1);
	/*
	 * check first operand for NaN's or infinity
	 */
	if (Quad_isinfinity_exponent(opnd1p1)) {
		if (Quad_iszero_mantissa(opnd1p1,opnd1p2,opnd1p3,opnd1p4)) {
			if (Quad_isnotnan(opnd2p1,opnd2p2,opnd2p3,opnd2p4)) {
				if (Quad_iszero_exponentmantissa(opnd2p1,
				 opnd2p2,opnd2p3,opnd2p4)) {
					/* 
					 * invalid since operands are infinity 
					 * and zero 
					 */
					if (Is_invalidtrap_enabled())
                                		return(INVALIDEXCEPTION);
                                	Set_invalidflag();
                                	Quad_makequietnan(resultp1,resultp2,
					 resultp3,resultp4);
					Quad_copytoptr(resultp1,resultp2,
					 resultp3,resultp4,dstptr);
					return(NOEXCEPTION);
				}
				/*
			 	 * return infinity
			 	 */
				Quad_setinfinity_exponentmantissa(resultp1,
				 resultp2,resultp3,resultp4);
				Quad_copytoptr(resultp1,resultp2,resultp3,
				 resultp4,dstptr);
				return(NOEXCEPTION);
			}
		}
		else {
                	/*
                 	 * is NaN; signaling or quiet?
                 	 */
                	if (Quad_isone_signaling(opnd1p1)) {
                        	/* trap if INVALIDTRAP enabled */
                        	if (Is_invalidtrap_enabled()) 
                            		return(INVALIDEXCEPTION);
                        	/* make NaN quiet */
                        	Set_invalidflag();
                        	Quad_set_quiet(opnd1p1);
                	}
			/* 
			 * is second operand a signaling NaN? 
			 */
                        else if (Quad_is_signalingnan(opnd2p1) &&
                   Quad_isnotzero_mantissa(opnd2p1,opnd2p2,opnd2p3,opnd2p4)) {
                        	/* trap if INVALIDTRAP enabled */
                        	if (Is_invalidtrap_enabled())
                            		return(INVALIDEXCEPTION);
                        	/* make NaN quiet */
                        	Set_invalidflag();
                        	Quad_set_quiet(opnd2p1);
				Quad_copytoptr(opnd2p1,opnd2p2,opnd2p3,opnd2p4,
				 dstptr);
                		return(NOEXCEPTION);
			}
                	/*
                 	 * return quiet NaN
                 	 */
			Quad_copytoptr(opnd1p1,opnd1p2,opnd1p3,opnd1p4,dstptr);
                	return(NOEXCEPTION);
		}
	}
	/*
	 * check second operand for NaN's or infinity
	 */
	if (Quad_isinfinity_exponent(opnd2p1)) {
		if (Quad_iszero_mantissa(opnd2p1,opnd2p2,opnd2p3,opnd2p4)) {
			if (Quad_iszero_exponentmantissa(opnd1p1,opnd1p2,
			 opnd1p3,opnd1p4)) {
				/* invalid since operands are zero & infinity */
				if (Is_invalidtrap_enabled())
                                	return(INVALIDEXCEPTION);
                                Set_invalidflag();
                                Quad_makequietnan(opnd2p1,opnd2p2,opnd2p3,
				 opnd2p4);
				Quad_copytoptr(opnd2p1,opnd2p2,opnd2p3,opnd2p4,
				 dstptr);
				return(NOEXCEPTION);
			}
			/*
			 * return infinity
			 */
			Quad_setinfinity_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
                /*
                 * is NaN; signaling or quiet?
                 */
                if (Quad_isone_signaling(opnd2p1)) {
                        /* trap if INVALIDTRAP enabled */
                        if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
                        /* make NaN quiet */
                        Set_invalidflag();
                        Quad_set_quiet(opnd2p1);
                }
                /*
                 * return quiet NaN
                 */
		Quad_copytoptr(opnd2p1,opnd2p2,opnd2p3,opnd2p4,dstptr);
                return(NOEXCEPTION);
	}
	/*
	 * Generate exponent 
	 */
	dest_exponent = Quad_exponent(opnd1p1) + Quad_exponent(opnd2p1) -QUAD_BIAS;

	/*
	 * Generate mantissa
	 */
	if (Quad_isnotzero_exponent(opnd1p1)) {
		/* set hidden bit */
		Quad_clear_signexponent_set_hidden(opnd1p1);
	}
	else {
		/* check for zero */
		if (Quad_iszero_mantissa(opnd1p1,opnd1p2,opnd1p3,opnd1p4)) {
			Quad_setzero_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
                /* is denormalized, adjust exponent */
                Quad_clear_signexponent(opnd1p1);
                Quad_leftshiftby1(opnd1p1,opnd1p2,opnd1p3,opnd1p4);
		Quad_normalize(opnd1p1,opnd1p2,opnd1p3,opnd1p4,dest_exponent);
	}
	/* opnd2 needs to have hidden bit set with msb in hidden bit */
	if (Quad_isnotzero_exponent(opnd2p1)) {
		Quad_clear_signexponent_set_hidden(opnd2p1);
	}
	else {
		/* check for zero */
		if (Quad_iszero_mantissa(opnd2p1,opnd2p2,opnd2p3,opnd2p4)) {
			Quad_setzero_exponentmantissa(resultp1,resultp2,
			 resultp3,resultp4);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			return(NOEXCEPTION);
		}
                /* is denormalized; want to normalize */
                Quad_clear_signexponent(opnd2p1);
                Quad_leftshiftby1(opnd2p1,opnd2p2,opnd2p3,opnd2p4);
		Quad_normalize(opnd2p1,opnd2p2,opnd2p3,opnd2p4,dest_exponent);
	}

	/* Multiply two source mantissas together */

	/* make room for guard bits */
	Quad_leftshiftby7(opnd2p1,opnd2p2,opnd2p3,opnd2p4);
	Quad_setzero(opnd3p1,opnd3p2,opnd3p3,opnd3p4);
        /* 
         * Four bits at a time are inspected in each loop, and a 
         * simple shift and add multiply algorithm is used. 
         */ 
	for (count = QUAD_P-1; count>=0; count-=4) {
		stickybit |= Qlow4p4(opnd3p4);
		Quad_rightshiftby4(opnd3p1,opnd3p2,opnd3p3,opnd3p4);
		if (Qbit28p4(opnd1p4)) {
	 		/* Fourword_add should be an ADD followed by 3 ADDC's */
                        Fourword_add(opnd3p1, opnd3p2, opnd3p3, opnd3p4, 
			 opnd2p1<<3 | opnd2p2>>29, opnd2p2<<3 | opnd2p3>>29,
			 opnd2p3<<3 | opnd2p4>>29, opnd2p4<<3);
		}
		if (Qbit29p4(opnd1p4)) {
                        Fourword_add(opnd3p1, opnd3p2, opnd3p3, opnd3p4,
			 opnd2p1<<2 | opnd2p2>>30, opnd2p2<<2 | opnd2p3>>30,
			 opnd2p3<<2 | opnd2p4>>30, opnd2p4<<2);
		}
		if (Qbit30p4(opnd1p4)) {
                        Fourword_add(opnd3p1, opnd3p2, opnd3p3, opnd3p4,
			 opnd2p1<<1 | opnd2p2>>31, opnd2p2<<1 | opnd2p3>>31,
			 opnd2p3<<1 | opnd2p4>>31, opnd2p4<<1);
		}
		if (Qbit31p4(opnd1p4)) {
                        Fourword_add(opnd3p1, opnd3p2, opnd3p3, opnd3p4,
			 opnd2p1, opnd2p2, opnd2p3, opnd2p4);
		}
		Quad_rightshiftby4(opnd1p1,opnd1p2,opnd1p3,opnd1p4);
	}
	if (Qbit7p1(opnd3p1)==0) {
		Quad_leftshiftby1(opnd3p1,opnd3p2,opnd3p3,opnd3p4);
	}
	else {
		/* result mantissa >= 2. */
		dest_exponent++;
	}
	/* check for denormalized result */
	while (Qbit7p1(opnd3p1)==0) {
		Quad_leftshiftby1(opnd3p1,opnd3p2,opnd3p3,opnd3p4);
		dest_exponent--;
	}
	/*
	 * check for guard, sticky and inexact bits 
	 */
	stickybit |= Qallp4(opnd3p4) << 25;
	guardbit = (Qallp4(opnd3p4) << 24) >> 31;
	inexact = guardbit | stickybit;

	/* align result mantissa */
	Quad_rightshiftby8(opnd3p1,opnd3p2,opnd3p3,opnd3p4);

	/* 
	 * round result 
	 */
	if (inexact && (dest_exponent>0 || Is_underflowtrap_enabled())) {
		Quad_clear_signexponent(opnd3p1);
		switch (Rounding_mode()) {
			case ROUNDPLUS: 
				if (Quad_iszero_sign(resultp1)) 
					Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				break;
			case ROUNDMINUS: 
				if (Quad_isone_sign(resultp1)) 
					Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				break;
			case ROUNDNEAREST:
				if (guardbit) {
			   	if (stickybit || 
				    Quad_isone_lowmantissap4(opnd3p4))
			      		Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				}
		}
		if (Quad_isone_hidden(opnd3p1)) dest_exponent++;
	}
	Quad_set_mantissa(resultp1,resultp2,resultp3,resultp4,opnd3p1,opnd3p2,
	 opnd3p3,opnd3p4);

        /* 
         * Test for overflow
         */
	if (dest_exponent >= QUAD_INFINITY_EXPONENT) {
                /* trap if OVERFLOWTRAP enabled */
                if (Is_overflowtrap_enabled()) {
                        /*
                         * Adjust bias of result
                         */
			Quad_setwrapped_exponent(resultp1,dest_exponent,ovfl);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return (OVERFLOWEXCEPTION | INEXACTEXCEPTION);
			    else Set_inexactflag();
			return (OVERFLOWEXCEPTION);
                }
		inexact = TRUE;
		Set_overflowflag();
                /* set result to infinity or largest number */
		Quad_setoverflow(resultp1,resultp2,resultp3,resultp4);
	}
        /* 
         * Test for underflow
         */
	else if (dest_exponent <= 0) {
                /* trap if UNDERFLOWTRAP enabled */
                if (Is_underflowtrap_enabled()) {
                        /*
                         * Adjust bias of result
                         */
			Quad_setwrapped_exponent(resultp1,dest_exponent,unfl);
			Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,
			 dstptr);
			if (inexact) 
			    if (Is_inexacttrap_enabled())
				return (UNDERFLOWEXCEPTION | INEXACTEXCEPTION);
			    else Set_inexactflag();
			return (UNDERFLOWEXCEPTION);
                }
		/*
		 * denormalize result or set to signed zero
		 */
		stickybit = inexact;
		Quad_denormalize(opnd3p1,opnd3p2,opnd3p3,opnd3p4,dest_exponent,
		 guardbit,stickybit,inexact);

		/* return zero or smallest number */
		if (inexact) {
			switch (Rounding_mode()) {
			case ROUNDPLUS: 
				if (Quad_iszero_sign(resultp1)) {
					Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				}
				break;
			case ROUNDMINUS: 
				if (Quad_isone_sign(resultp1)) {
					Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				}
				break;
			case ROUNDNEAREST:
				if (guardbit && (stickybit || 
				    Quad_isone_lowmantissap4(opnd3p4))) {
			      		Quad_increment(opnd3p1,opnd3p2,opnd3p3,
					 opnd3p4);
				}
				break;
			}
			if (Quad_iszero_hidden(opnd3p1)) Set_underflowflag();
		}
		Quad_set_exponentmantissa(resultp1,resultp2,resultp3,resultp4,
		 opnd3p1,opnd3p2,opnd3p3,opnd3p4);
	}
	else Quad_set_exponent(resultp1,dest_exponent);
	/* check for inexact */
	Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	if (inexact) {
		if (Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
		else Set_inexactflag();
	}
	return(NOEXCEPTION);
}
