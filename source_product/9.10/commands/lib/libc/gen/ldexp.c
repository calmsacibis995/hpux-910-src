/* @(#) $Revision: 70.1 $ */   

/* Purpose:  returns the quantity value*2**exponent */
/* rewritten from System 5.2 routine for IEEE floating point format. */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define ldexp _ldexp
#endif

#include <math.h>
#include <values.h>
#include <errno.h>

#define _MANT_MASK      0x000FFFFF
#define _EXP_MASK       0x7FF00000

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ldexp
#pragma _HP_SECONDARY_DEF _ldexp ldexp
#define ldexp _ldexp
#endif

double
ldexp(value,exponent)
double value;
int exponent;
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

	long int expnt,		/* exponent of value		*/
	         new_expnt;	/* exponent of return value	*/

	/* special case for 0.0 */
	if (value == 0.0) return((double)0.0);

	/* store double value in union structure */
	number.fval = value;

	/* Check for NaN and Infinity		*/
	if ((number.ival.ival1 & _EXP_MASK) == _EXP_MASK)
	{
		if ((number.ival.ival1 & _MANT_MASK) ||
		    (number.ival.ival2))
		{
			/* Input is NaN */
			errno = EDOM;
			number.ival.ival1 = (number.ival.ival1 | 0x7fffffff);
			number.ival.ival2 = 0xffffffff;
			return(number.fval);
		}
		else
		{
			/* Input is Infinity */
			return(number.fval);
		}
	}

	/* determine exponent of value		*/
	/* i.e. bits 1 to 11 of the 1st long	*/
	expnt = ((number.ival.ival1 >> 20) & 0x7ff);


	/* determine exponent of double to be returned 		*/
	/* current value of exponent + exponent parameter	*/
	new_expnt = exponent + expnt;

	/* check for overflow -- either new_expnt or the double	*/
	if ((exponent > DMAXEXP-DMINEXP) ||
	    (new_expnt >= 2047))
	{
		errno = ERANGE;
		if (number.ival.ival1 & 0x80000000) 
			return(-HUGE);
		else 
			return(HUGE);
	}

	/*  check for underflow */
	if (new_expnt <= 0) 
	{
		errno = ERANGE;
		return((double)0.0);
	}

	/* change exponent of value to new exponent	*/
	/* i.e. bits 1 to 11 of the lst long		*/
	{
		number.ival.ival1 &= 0x800fffff;
		new_expnt = (unsigned) new_expnt << 20;
		number.ival.ival1 |= new_expnt;
		return (number.fval);
	}
}

