/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/crt/qfadd.c,v $
 * $Revision: 64.1 $     $Author: sje $
 * $State: Exp $        $Locker:  $
 * $Date: 89/02/09 11:07:52 $
 *
 * $Log:	qfadd.c,v $
 * Revision 64.1  89/02/09  11:07:52  11:07:52  sje
 * Initial S300 version
 * 
 * Revision 1.2  88/01/21  17:10:57  17:10:57  liz (Liz Peters)
 * Changed entry point name.
 * 
 * Revision 1.1  87/10/08  13:36:41  13:36:41  liz (Liz Peters)
 * Initial revision
 * 
 * $Endlog$
 *
 *  Spectrum Simulator Assist Floating Point Coprocessor 
 *  Quad Precision Floating-point Add
 *  Liz Peters
 *
 *  *** HP Company Confidential ***
 *
 */
#include "float.h"
#include "quad_float.h"

static char rcsid[] = "$Header: qfadd.c,v 64.1 89/02/09 11:07:52 sje Exp $";
static char hpid[] = "HP Proprietary";

/*
 * Quad_add: add two quad precision values.
 */
_U_qfadd(leftptr, rightptr, dstptr, status)
    quad_floating_point *leftptr, *rightptr, *dstptr;
    unsigned int *status;
    {
    register unsigned int signless_upper_left, signless_upper_right, save;
    register unsigned int leftp1, leftp2, leftp3, leftp4, extent;
    register unsigned int rightp1, rightp2, rightp3, rightp4;
    register unsigned int resultp1=0, resultp2=0, resultp3=0, resultp4=0;
    
    register int result_exponent, right_exponent, diff_exponent;
    register int sign_save, jumpsize;
    register boolean inexact = FALSE, underflowtrap;
        
    /* Create local copies of the numbers */
    Quad_copyfromptr(leftptr,leftp1,leftp2,leftp3,leftp4);
    Quad_copyfromptr(rightptr,rightp1,rightp2,rightp3,rightp4);

    /* A zero "save" helps discover equal operands (for later),  *
     * and is used in swapping operands (if needed).             */
    Quad_xortointp1(leftp1,rightp1,/*to*/save);

    /*
     * check first operand for NaN's or infinity
     */
    if ((result_exponent = Quad_exponent(leftp1)) == QUAD_INFINITY_EXPONENT)
	{
	if (Quad_iszero_mantissa(leftp1,leftp2,leftp3,leftp4)) 
	    {
	    if (Quad_isnotnan(rightp1,rightp2,rightp3,rightp4)) 
		{
		if(Quad_isinfinity(rightp1,rightp2,rightp3,rightp4) && save!=0) 
		    {
		    /* 
		     * invalid since operands are opposite signed infinity's
		     */
		    if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
                    Set_invalidflag();
                    Quad_makequietnan(resultp1,resultp2,resultp3,resultp4);
		    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		    return(NOEXCEPTION);
		    }
		/*
	 	 * return infinity
	 	 */
		Quad_copytoptr(leftp1,leftp2,leftp3,leftp4,dstptr);
		return(NOEXCEPTION);
		}
	    }
	else 
	    {
            /*
             * is NaN; signaling or quiet?
             */
            if (Quad_isone_signaling(leftp1)) 
		{
               	/* trap if INVALIDTRAP enabled */
		if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
        	/* make NaN quiet */
        	Set_invalidflag();
        	Quad_set_quiet(leftp1);
        	}
	    /* 
	     * is second operand a signaling NaN? 
	     */
	    else if (Quad_is_signalingnan(rightp1) && 
                     Quad_isnotzero_mantissa(rightp1,rightp2,rightp3,rightp4))
		{
        	/* trap if INVALIDTRAP enabled */
               	if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
		/* make NaN quiet */
		Set_invalidflag();
		Quad_set_quiet(rightp1);
		Quad_copytoptr(rightp1,rightp2,rightp3,rightp4,dstptr);
		return(NOEXCEPTION);
		}
	    /*
 	     * return quiet NaN
 	     */
	    Quad_copytoptr(leftp1,leftp2,leftp3,leftp4,dstptr);
 	    return(NOEXCEPTION);
	    }
	} /* End left NaN or Infinity processing */
    /*
     * check second operand for NaN's or infinity
     */
    if (Quad_isinfinity_exponent(rightp1)) 
	{
	if (Quad_iszero_mantissa(rightp1,rightp2,rightp3,rightp4)) 
	    {
	    /* return infinity */
	    Quad_copytoptr(rightp1,rightp2,rightp3,rightp4,dstptr);
	    return(NOEXCEPTION);
	    }
        /*
         * is NaN; signaling or quiet?
         */
        if (Quad_isone_signaling(rightp1)) 
	    {
            /* trap if INVALIDTRAP enabled */
	    if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
	    /* make NaN quiet */
	    Set_invalidflag();
	    Quad_set_quiet(rightp1);
	    }
	/*
	 * return quiet NaN
 	 */
	Quad_copytoptr(rightp1,rightp2,rightp3,rightp4,dstptr);
	return(NOEXCEPTION);
    	} /* End right NaN or Infinity processing */

    /* Invariant: Must be dealing with finite numbers */

    /* Compare operands by removing the sign */
    Quad_copytoint_exponentmantissap1(leftp1,signless_upper_left);
    Quad_copytoint_exponentmantissap1(rightp1,signless_upper_right);

    /* sign difference selects add or sub operation. */
    if(Quad_ismagnitudeless(leftp2,leftp3,leftp4,rightp2,rightp3,rightp4,
       signless_upper_left,signless_upper_right))
	{
	/* Set the left operand to the larger one by XOR swap *
	 *  First finish the first word using "save"          */
	Quad_xorfromintp1(save,rightp1,/*to*/rightp1);
	Quad_xorfromintp1(save,leftp1,/*to*/leftp1);
     	Quad_swap_lower(leftp2,leftp3,leftp4,rightp2,rightp3,rightp4);
	result_exponent = Quad_exponent(leftp1);
	}
    /* Invariant:  left is not smaller than right. */ 

    if((right_exponent = Quad_exponent(rightp1)) == 0)
        {
	/* Denormalized operands.  First look for zeroes */
	if(Quad_iszero_mantissa(rightp1,rightp2,rightp3,rightp4)) 
	    {
	    /* right is zero */
	    if(Quad_iszero_exponentmantissa(leftp1,leftp2,leftp3,leftp4))
		{
		/* Both operands are zeros */
		if(Is_rounding_mode(ROUNDMINUS))
		    {
		    Quad_or_signs(leftp1,/*with*/rightp1);
		    }
		else
		    {
		    Quad_and_signs(leftp1,/*with*/rightp1);
		    }
		}
	    else 
		{
		/* Left is not a zero and must be the result.  Trapped
		 * underflows are signaled if left is denormalized.  Result
		 * is always exact. */
		if( (result_exponent == 0) && Is_underflowtrap_enabled() )
		    {
		    /* need to normalize results mantissa */
	    	    sign_save = Quad_signextendedsign(leftp1);
		    Quad_leftshiftby1(leftp1,leftp2,leftp3,leftp4);
		    Quad_normalize(leftp1,leftp2,leftp3,leftp4,result_exponent);
		    Quad_set_sign(leftp1,/*using*/sign_save);
                    Quad_setwrapped_exponent(leftp1,result_exponent,unfl);
		    Quad_copytoptr(leftp1,leftp2,leftp3,leftp4,dstptr);
		    /* inexact = FALSE */
		    return(UNDERFLOWEXCEPTION);
		    }
		}
	    Quad_copytoptr(leftp1,leftp2,leftp3,leftp4,dstptr);
	    return(NOEXCEPTION);
	    }

	/* Neither are zeroes */
	Quad_clear_sign(rightp1);	/* Exponent is already cleared */
	if(result_exponent == 0 )
	    {
	    /* Both operands are denormalized.  The result must be exact
	     * and is simply calculated.  A sum could become normalized and a
	     * difference could cancel to a true zero. */
	    if( (/*signed*/int) save < 0 )
		{
		Quad_subtract(leftp1,leftp2,leftp3,leftp4,
		 /*minus*/rightp1,rightp2,rightp3,rightp4,
		 /*into*/resultp1,resultp2,resultp3,resultp4);
		if(Quad_iszero_mantissa(resultp1,resultp2,resultp3,resultp4))
		    {
		    if(Is_rounding_mode(ROUNDMINUS))
			{
			Quad_setone_sign(resultp1);
			}
		    else
			{
			Quad_setzero_sign(resultp1);
			}
		    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		    return(NOEXCEPTION);
		    }
		}
	    else
		{
		Quad_addition(leftp1,leftp2,leftp3,leftp4,
		 rightp1,rightp2,rightp3,rightp4,
		 /*into*/resultp1,resultp2,resultp3,resultp4);
		if(Quad_isone_hidden(resultp1))
		    {
		    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		    return(NOEXCEPTION);
		    }
		}
	    if(Is_underflowtrap_enabled())
		{
		/* need to normalize result */
	    	sign_save = Quad_signextendedsign(resultp1);
		Quad_leftshiftby1(resultp1,resultp2,resultp3,resultp4);
		Quad_normalize(resultp1,resultp2,resultp3,resultp4,
		 result_exponent);
		Quad_set_sign(resultp1,/*using*/sign_save);
                Quad_setwrapped_exponent(resultp1,result_exponent,unfl);
	        Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		/* inexact = FALSE */
	        return(UNDERFLOWEXCEPTION);
		}
	    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	    return(NOEXCEPTION);
	    }
	right_exponent = 1;	/* Set exponent to reflect different bias
				 * with denomalized numbers. */
	}
    else
	{
	Quad_clear_signexponent_set_hidden(rightp1);
	}
    Quad_clear_exponent_set_hidden(leftp1);
    diff_exponent = result_exponent - right_exponent;

    /* 
     * Special case alignment of operands that would force alignment 
     * beyond the extent of the extension.  A further optimization
     * could special case this but only reduces the path length for this
     * infrequent case.
     */
    if(diff_exponent > QUAD_THRESHOLD)
	{
	diff_exponent = QUAD_THRESHOLD;
	}
    
    /* Align right operand by shifting to right */
    Quad_right_align(/*operand*/rightp1,rightp2,rightp3,rightp4,
     /*shifted by*/diff_exponent,/*and lower to*/extent);

    /* Treat sum and difference of the operands separately. */
    if( (/*signed*/int) save < 0 )
	{
	/*
	 * Difference of the two operands.  Their can be no overflow.  A
	 * borrow can occur out of the hidden bit and force a post
	 * normalization phase.
	 */
	Quad_subtract_withextension(leftp1,leftp2,leftp3,leftp4,
	 /*minus*/rightp1,rightp2,rightp3,rightp4,
	 /*with*/extent,/*into*/resultp1,resultp2,resultp3,resultp4);
	if(Quad_iszero_hidden(resultp1))
	    {
	    /* Handle normalization */
	    /* A straight foward algorithm would now shift the result
	     * and extension left until the hidden bit becomes one.  Not
	     * all of the extension bits need participate in the shift.
	     * Only the two most significant bits (round and guard) are
	     * needed.  If only a single shift is needed then the guard
	     * bit becomes a significant low order bit and the extension
	     * must participate in the rounding.  If more than a single 
	     * shift is needed, then all bits to the right of the guard 
	     * bit are zeros, and the guard bit may or may not be zero. */
	    sign_save = Quad_signextendedsign(resultp1);
            Quad_leftshiftby1_withextent(resultp1,resultp2,resultp3,resultp4,
	     extent,resultp1,resultp2,resultp3,resultp4);

            /* Need to check for a zero result.  The sign and exponent
	     * fields have already been zeroed.  The more efficient test
	     * of the full object can be used.
	     */
    	    if(Quad_iszero(resultp1,resultp2,resultp3,resultp4))
		/* Must have been "x-x" or "x+(-x)". */
		{
		if(Is_rounding_mode(ROUNDMINUS)) Quad_setone_sign(resultp1);
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		return(NOEXCEPTION);
		}
	    result_exponent--;
	    /* Look to see if normalization is finished. */
	    if(Quad_isone_hidden(resultp1))
		{
		if(result_exponent==0)
		    {
		    /* Denormalized, exponent should be zero.  Left operand *
		     * was normalized, so extent (guard, round) was zero    */
		    goto underflow;
		    }
		else
		    {
		    /* No further normalization is needed. */
		    Quad_set_sign(resultp1,/*using*/sign_save);
	    	    Ext_leftshiftby1(extent);
		    goto round;
		    }
		}

	    /* Check for denormalized, exponent should be zero.  Left    *
	     * operand was normalized, so extent (guard, round) was zero */
	    if(!(underflowtrap = Is_underflowtrap_enabled()) &&
	       result_exponent==0) goto underflow;

	    /* Shift extension to complete one bit of normalization and
	     * update exponent. */
	    Ext_leftshiftby1(extent);

	    /* Discover first one bit to determine shift amount.  Use a
	     * modified binary search.  We have already shifted the result
	     * one position right and still not found a one so the remainder
	     * of the extension must be zero and simplifies rounding. */
	    /* Scan bytes */
	    while(Quad_iszero_hiddenhigh7mantissa(resultp1))
		{
		Quad_leftshiftby8(resultp1,resultp2,resultp3,resultp4);
		if((result_exponent -= 8) <= 0  && !underflowtrap)
		    goto underflow;
		}
	    /* Now narrow it down to the nibble */
	    if(Quad_iszero_hiddenhigh3mantissa(resultp1))
		{
		/* The lower nibble contains the normalizing one */
		Quad_leftshiftby4(resultp1,resultp2,resultp3,resultp4);
		if((result_exponent -= 4) <= 0 && !underflowtrap)
		    goto underflow;
		}
	    /* Select case were first bit is set (already normalized)
	     * otherwise select the proper shift. */
	    if((jumpsize = Quad_hiddenhigh3mantissa(resultp1)) > 7)
		{
		/* Already normalized */
		if(result_exponent <= 0) goto underflow;
		Quad_set_sign(resultp1,/*using*/sign_save);
		Quad_set_exponent(resultp1,/*using*/result_exponent);
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		return(NOEXCEPTION);
		}
	    Quad_sethigh4bits(resultp1,/*using*/sign_save);
	    switch(jumpsize) 
		{
		case 1:
		    {
		    Quad_leftshiftby3(resultp1,resultp2,resultp3,resultp4);
		    result_exponent -= 3;
		    break;
		    }
		case 2:
		case 3:
		    {
		    Quad_leftshiftby2(resultp1,resultp2,resultp3,resultp4);
		    result_exponent -= 2;
		    break;
		    }
		case 4:
		case 5:
		case 6:
		case 7:
		    {
		    Quad_leftshiftby1(resultp1,resultp2,resultp3,resultp4);
		    result_exponent -= 1;
		    break;
		    }
		}
	    if(result_exponent > 0) 
		{
		Quad_set_exponent(resultp1,/*using*/result_exponent);
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		return(NOEXCEPTION); 	/* Sign bit is already set */
		}
	    /* Fixup potential underflows */
	  underflow:
	    if(Is_underflowtrap_enabled())
		{
		Quad_set_sign(resultp1,sign_save);
                Quad_setwrapped_exponent(resultp1,result_exponent,unfl);
		Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
		/* inexact = FALSE */
		return(UNDERFLOWEXCEPTION);
		}
	    /* 
	     * Since we cannot get an inexact denormalized result,
	     * we can now return.
	     */
	    Quad_fix_overshift(resultp1,resultp2,resultp3,resultp4,
	     (1-result_exponent),extent);
	    Quad_clear_signexponent(resultp1);
	    Quad_set_sign(resultp1,sign_save);
	    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	    return(NOEXCEPTION);
	    } /* end if(hidden...)... */
	/* Fall through and round */
	} /* end if(save < 0)... */
    else 
	{
	/* Add magnitudes */
	Quad_addition(leftp1,leftp2,leftp3,leftp4,
	 rightp1,rightp2,rightp3,rightp4,
	 /*to*/resultp1,resultp2,resultp3,resultp4);
	if(Quad_isone_hiddenoverflow(resultp1))
	    {
	    /* Prenormalization required. */
	    Quad_rightshiftby1_withextent(resultp4,extent,extent);
	    Quad_arithrightshiftby1(resultp1,resultp2,resultp3,resultp4);
	    result_exponent++;
	    } /* end if hiddenoverflow... */
	} /* end else ...add magnitudes... */
    
    /* Round the result.  If the extension is all zeros,then the result is
     * exact.  Otherwise round in the correct direction.  No underflow is
     * possible. If a postnormalization is necessary, then the mantissa is
     * all zeros so no shift is needed. */
  round:
    if(Ext_isnotzero(extent))
	{
	inexact = TRUE;
	switch(Rounding_mode())
	    {
	    case ROUNDNEAREST: /* The default. */
	    if(Ext_isone_sign(extent))
		{
		/* at least 1/2 ulp */
		if(Ext_isnotzero_lower(extent)  ||
		  Quad_isone_lowmantissap4(resultp4))
		    {
		    /* either exactly half way and odd or more than 1/2ulp */
		    Quad_increment(resultp1,resultp2,resultp3,resultp4);
		    }
		}
	    break;

	    case ROUNDPLUS:
	    if(Quad_iszero_sign(resultp1))
		{
		/* Round up positive results */
		Quad_increment(resultp1,resultp2,resultp3,resultp4);
		}
	    break;
	    
	    case ROUNDMINUS:
	    if(Quad_isone_sign(resultp1))
		{
		/* Round down negative results */
		Quad_increment(resultp1,resultp2,resultp3,resultp4);
		}
	    
	    case ROUNDZERO:;
	    /* truncate is simple */
	    } /* end switch... */
	if(Quad_isone_hiddenoverflow(resultp1)) result_exponent++;
	}
    if(result_exponent == QUAD_INFINITY_EXPONENT)
        {
        /* Overflow */
        if(Is_overflowtrap_enabled())
	    {
	    Quad_setwrapped_exponent(resultp1,result_exponent,ovfl);
	    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
	    if (inexact)
		if (Is_inexacttrap_enabled())
			return(OVERFLOWEXCEPTION | INEXACTEXCEPTION);
		else Set_inexactflag();
	    return(OVERFLOWEXCEPTION);
	    }
        else
	    {
	    inexact = TRUE;
	    Set_overflowflag();
	    Quad_setoverflow(resultp1,resultp2,resultp3,resultp4);
	    }
	}
    else Quad_set_exponent(resultp1,result_exponent);
    Quad_copytoptr(resultp1,resultp2,resultp3,resultp4,dstptr);
    if(inexact) 
	if(Is_inexacttrap_enabled()) return(INEXACTEXCEPTION);
	else Set_inexactflag();
    return(NOEXCEPTION);
    }
