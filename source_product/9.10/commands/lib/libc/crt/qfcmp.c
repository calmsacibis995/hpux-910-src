/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/crt/qfcmp.c,v $
 * $Revision: 64.1 $     $Author: sje $
 * $State: Exp $        $Locker:  $
 * $Date: 89/02/09 11:07:55 $
 *
 * $Log:	qfcmp.c,v $
 * Revision 64.1  89/02/09  11:07:55  11:07:55  sje
 * Initial S300 version
 * 
 * Revision 1.2  85/02/22  22:41:03  22:41:03  jcm (J. C. Mercier)
 * Changed includes to be ../spmath relative.
 * 
 * Revision 1.1  85/02/21  16:49:47  peters
 * Initial revision
 * 
 * 
 * $Endlog$
 *
 *  Spectrum Simulator Assist Floating Point Coprocessor 
 *  Quad Precision Floating-point Compare
 *
 */
#include "float.h"
#include "quad_float.h"
    
/*
 * qfcmp: compare two values
 */
_U_qfcmp(leftptr, rightptr, cond, status)
    quad_floating_point *leftptr, *rightptr;
    unsigned int cond; /* The predicate to be tested */
    unsigned int *status;
    {
    register unsigned int leftp1, leftp2, leftp3, leftp4;
    register unsigned int rightp1, rightp2, rightp3, rightp4;
    register int xorresult;
        
    /* Create local copies of the numbers */
    Quad_copyfromptr(leftptr,leftp1,leftp2,leftp3,leftp4);
    Quad_copyfromptr(rightptr,rightp1,rightp2,rightp3,rightp4);
    /*
     * Test for NaN
     */
    if(    (Quad_exponent(leftp1) == QUAD_INFINITY_EXPONENT)
        || (Quad_exponent(rightp1) == QUAD_INFINITY_EXPONENT) )
	{
	/* Check if a NaN is involved.  Signal an invalid exception when 
	 * comparing a signaling NaN or when comparing quiet NaNs and the
	 * low bit of the condition is set */
        if( ((Quad_exponent(leftp1) == QUAD_INFINITY_EXPONENT)
	    && Quad_isnotzero_mantissa(leftp1,leftp2,leftp3,leftp4) 
	    && (Exception(cond) || Quad_isone_signaling(leftp1)))
	   ||
	    ((Quad_exponent(rightp1) == QUAD_INFINITY_EXPONENT)
	    && Quad_isnotzero_mantissa(rightp1,rightp2,rightp3,rightp4) 
	    && (Exception(cond) || Quad_isone_signaling(rightp1))) )
	    {
	    Set_status_cbit(Unordered(cond));
	    if( Is_invalidtrap_enabled() ) {
		return(INVALIDEXCEPTION);
	    }
	    else Set_invalidflag();
	    return(NOEXCEPTION);
	    }
	/* All the exceptional conditions are handled, now special case
	   NaN compares */
        else if( ((Quad_exponent(leftp1) == QUAD_INFINITY_EXPONENT)
	    && Quad_isnotzero_mantissa(leftp1,leftp2,leftp3,leftp4))
	   ||
	    ((Quad_exponent(rightp1) == QUAD_INFINITY_EXPONENT)
	    && Quad_isnotzero_mantissa(rightp1,rightp2,rightp3,rightp4)) )
	    {
	    /* NaNs always compare unordered. */
	    Set_status_cbit(Unordered(cond));
	    return(NOEXCEPTION);
	    }
	/* infinities will drop down to the normal compare mechanisms */
	}
    /* First compare for unequal signs => less or greater or
     * special equal case */
    Quad_xortointp1(leftp1,rightp1,xorresult);
    if( xorresult < 0 )
        {
        /* left negative => less, left positive => greater.
         * equal is possible if both operands are zeros. */
        if( Quad_iszero_exponentmantissa(leftp1,leftp2,leftp3,leftp4) 
	  && Quad_iszero_exponentmantissa(rightp1,rightp2,rightp3,rightp4) )
            {
	    Set_status_cbit(Equal(cond));
	    }
	else if( Quad_isone_sign(leftp1) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
        }
    /* Signs are the same.  Treat negative numbers separately
     * from the positives because of the reversed sense.  */
    else if( Quad_isequal(leftp1,leftp2,leftp3,leftp4,
			  rightp1,rightp2,rightp3,rightp4) )
        {
        Set_status_cbit(Equal(cond));
        }
    else if( Quad_iszero_sign(leftp1) )
        {
        /* Positive compare */
	if( Quad_allp1(leftp1) < Quad_allp1(rightp1) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else if( Quad_allp1(leftp1) > Quad_allp1(rightp1) )
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
	else
	    {
	    if( Quad_allp2(leftp2) < Quad_allp2(rightp2) )
	    	{
	    	Set_status_cbit(Lessthan(cond));
	    	}
	    else if( Quad_allp2(leftp2) > Quad_allp2(rightp2) )
	    	{
	    	Set_status_cbit(Greaterthan(cond));
	    	}
	    else
	    	{
		if( Quad_allp3(leftp3) < Quad_allp3(rightp3) )
		    {
		    Set_status_cbit(Lessthan(cond));
		    }
		else if( Quad_allp3(leftp3) > Quad_allp3(rightp3) )
		    {
		    Set_status_cbit(Greaterthan(cond));
		    }
		else
		    {
		    /* Equal first parts.  Now we must use unsigned compares to
		     * resolve the two possibilities. */
		    if( Quad_allp4(leftp4) < Quad_allp4(rightp4) )
			{
			Set_status_cbit(Lessthan(cond));
			}
		    else 
			{
			Set_status_cbit(Greaterthan(cond));
			}
		    }
		}
	    }
	}
    else
        {
        /* Negative compare.  Signed or unsigned compares
         * both work the same.  That distinction is only
         * important when the sign bits differ. */
	if( Quad_allp1(leftp1) > Quad_allp1(rightp1) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else if( Quad_allp1(leftp1) < Quad_allp1(rightp1) )
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
	else
	    {
	    if( Quad_allp2(leftp2) > Quad_allp2(rightp2) )
		{
		Set_status_cbit(Lessthan(cond));
		}
	    else if( Quad_allp2(leftp2) < Quad_allp2(rightp2) )
		{
		Set_status_cbit(Greaterthan(cond));
		}
	    else
		{
		if( Quad_allp3(leftp3) > Quad_allp3(rightp3) )
		    {
		    Set_status_cbit(Lessthan(cond));
		    }
		else if( Quad_allp3(leftp3) < Quad_allp3(rightp3) )
		    {
		    Set_status_cbit(Greaterthan(cond));
		    }
		else
		    {
		    /* Equal first parts.  Now we must use unsigned compares to
		     * resolve the two possibilities. */
		    if( Quad_allp4(leftp4) > Quad_allp4(rightp4) )
			{
			Set_status_cbit(Lessthan(cond));
			}
		    else 
			{
			Set_status_cbit(Greaterthan(cond));
			}
		    }
		}
	    }
        }
	return(NOEXCEPTION);
    }
