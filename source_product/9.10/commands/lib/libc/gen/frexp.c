/* @(#) $Revision: 70.1 $ */   
/*  source:  rewritten from System 5.2 routine for IEEE floating point  */
/*  format.                                                             */

/*  Purpose:  returns the mantissa of a double value as a double 	*/
/*	quantity, x, of magnitude less than 1, and stores an integer, 	*/
/*	n, such that (value = x*2^n) indirectly through eptr.		*/ 

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _frexp frexp
#define frexp _frexp
#endif

#include <math.h>
#include <values.h>
#include <errno.h>

#define _MANT_MASK 	0x000FFFFF
#define _EXP_MASK 	0x7FF00000

double
frexp(value, eptr)
double value;
int *eptr;

{
	/* structure to look at a double as 2 long integers */
	struct ivalues
	{
		long unsigned int ival1, ival2;
	};

	union 
	{
		double fval;		/* treat double as double or 	*/
		struct ivalues ival;	/* treat double as two longs	*/
	} number;

	/* special case for 0.0 */
	if (value == 0.0) 
	{
		*eptr = 0;
		return(0.0);
	}

	/* store double value in union structure */
	number.fval = value;

	if (!((number.ival.ival1) & _EXP_MASK))	/* denormalized */
	{
		register unsigned h_val = number.ival.ival1;
		register unsigned l_val = number.ival.ival2;
		unsigned sign = h_val & 0x80000000;
		register int exponent = -1021;
		register unsigned carry;

		/* Keep shifting mantissa until "hidden bit"	*/
		/* is shifted into the exponent field.		*/
		/* And keep bumping exponent... (we get one	*/
		/* "free" exponent bump in the while loop, so	*/
		/* we start off one shy...)			*/

		if (!h_val) {
			h_val = l_val >> 11;
			l_val = l_val << 21;
			exponent -= 21;
		}
		while (!(h_val & 0x00100000)) {
			carry = l_val & 0x80000000;
			l_val = l_val << 1;
			h_val = h_val << 1;
			if (carry) h_val |= 1;
			exponent--;
		}
		*eptr = exponent;
		h_val |= sign;
		number.ival.ival1 = h_val;
		number.ival.ival2 = l_val;
	}
	else
	{
		/* check for NaN and infinity				*/
		if ((number.ival.ival1 & _EXP_MASK) == _EXP_MASK)
		{
			/* If NaN or Inf, return NaN and set errno	*/
			number.ival.ival1 = (number.ival.ival1 | 0x7fffffff);
			number.ival.ival2 = 0xffffffff;
			errno = EDOM;
			return(number.fval);
		}
		else
		{
			/* extract exponent 				*/
			/* i.e. bits 1 to 11 of the 1st long 		*/
			/*	- 1023 (normalization factor)		*/
			/*	+ 1 (to account for mantissa being	*/
			/*		between	1.0 and 2.0)		*/
			*eptr = (( number.ival.ival1 >> 20) & 0x7ff) - 1022;
		}

	}
	/* extract mantissa 					*/
	/* i.e. bits 12 -31 of 1st word and all of 2nd word	*/
	/*	and set the exponent to effective value		*/
	/*	of 0 (bit pattern 1022)				*/
	number.ival.ival1 = (number.ival.ival1 & 0x800fffff) | 0x3fe00000;

	return(number.fval);
}
