/* @(#) $Revision: 66.1 $ */

/* 

        ISINF OF A DOUBLE PRECSION ARGUMENT

        This routine returns a positive integer if the input argument
	is +INFINITY and a negative integer if the input argument is
	-INFINITY.  Otherwise it returns zero.

        Written by Liz Sanville
        Date: May 8, 1989

*/

#include	<math.h>

#ifdef _NAMESPACE_CLEAN
#undef 	isinf 	
#pragma _HP_SECONDARY_DEF	_isinf	isinf
#define isinf 	_isinf
#endif

int isinf(arg) double arg;
{
	union {
		unsigned int i[2];
		double d;
	} opnd;

	opnd.d = arg;
	if ( (((opnd.i[0] & 0x7FFFFFFF) == 0x7FF00000) &&
	     (opnd.i[1] == 0)) ) {
		if ( (opnd.i[0] & 0x80000000) == 0x0 )
			return(1);			/* arg is +INF */
		else
			return(-1);			/* arg is -INF */
	} else {
		return(0);				/* arg is not INF */
	}
}
