/* @(#) $Revision: 64.1 $ */     
/*  source:  rewritten from System 5.2 routine for IEEE floating point  */
/*  format.                                                             */

/*  Purpose:  returns the positive fractional part of value and stores	*/
/*	the integer part indirectly through iptr			*/

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _modf modf
#define modf _modf
#endif /* _NAMESPACE_CLEAN */

double
modf(value,iptr)

double value, *iptr;

{
	/* structure to look at a double as 2 long integers */
	struct ivalues 
	{
		long unsigned int ival1, ival2;
	};

	union 
	{
		double fval;		/* treat double as double or	*/
		struct ivalues ival;	/* treat double as two longs	*/
	} number;

	long int expnt;		/* unbiased exponent */

	/* store double value in union structure */
	number.fval = value;

	/* determine the unbiased exponent 			*/
	/* i.e. bits 1 to 11 of the 1st long			*/
	/*	- 1023	(normalization factor)			*/
	expnt = ((number.ival.ival1 >> 20) & 0x7ff) - 1023;

        /* extract integer part of value			*/

	/* if exponent < 0 then integer part = 0		*/
	if (expnt < 0) *iptr = 0.0;
	
	/* if exponent <= 20 then significant digits for 	*/
	/*	integer are entirely in the first long.		*/
	/*	(second long contains only bits for the 	*/
	/*	fractional part of the double).			*/
	else if (expnt <= 20)
	{  
		/* shift off all fractional bits from the first long */
		number.ival.ival1 = (number.ival.ival1 >> (20 - expnt))
		   << (20 - expnt);
		number.ival.ival2 = 0;
		*iptr = number.fval;
	}

	/* if exponent <= 52 then significant digits for	*/
	/*	integer are in the first long and partly in the */
	/*	second long.					*/
	else if (expnt <= 52)
	{
		/* shift off all fractional bits from the second long */
		number.ival.ival2 = (number.ival.ival2 >> ( 52 - expnt))
		   << (52 - expnt);
		*iptr = number.fval;
	}

	/* if exponent > 52 then there are no bits for the 	*/
	/*	fractional part of the number			*/
	else *iptr = value;

	/* return fractional part of value	*/
	return(value - *iptr);
}

