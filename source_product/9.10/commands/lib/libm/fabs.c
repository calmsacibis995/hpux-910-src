/* @(#) $Revision: 66.2 $ */    
/*  source:  System III                                    */
/*     /usr/src/lib/libm/fabs.c                            */
/*     dsb - 3/8/82                                        */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _fabs fabs
#define isnan _isnan
#define fabs _fabs
#endif /* _NAMESPACE_CLEAN */

#include <math.h>
#include "c_include.h"

double
fabs(arg)
double arg;
{
	if (isnan(arg))
		_error_handler("fabs",&arg,&arg,DOUBLE_TYPE,NAN_RET,DOMAIN);
	else
		return((arg < 0)? -arg : arg);
}
