/* @(#) $Revision: 70.2 $ */     
/*

        DOUBLE PRECISION BESSEL FUNCTION OF FIRST & SECOND KIND OF
                    ORDER INTEGER N OF DOUBLE PRECISION ARGUMENT

	jn(x) returns the value of Jn(x) for all
	integer values of n and all real values
	of x.

	There are no error returns.
	Calls j0, j1.

	For n=0, j0(x) is called,
	for n=1, j1(x) is called,
	for n<|x|, forward recursion is used starting
	from values of j0(x) and j1(x).
	for n>|x|, a continued fraction approximation to
	j(n,x)/j(n-1,x) is evaluated and then backward
	recursion is used starting from a supposed value
	for j(n,x). The resulting value of j(0,x) is
	compared with the actual value to correct the
	supposed value of j(n,x).
  
	yn(n,x) is similar in all respects, except
	that forward recursion is used for all
	values of n>1.

        Ported from HP9000 by Chih-Ming Hu
        Modified by Chih-Ming Hu to fix the bad code for negative argument
                     and to meet the specification of UNIX V - Release 2.0.
        Modification date: March 11, 1985.
*/
#ifdef _NAMESPACE_CLEAN
#define jn _jn
#define j0 _j0
#define j1 _j1
#define yn _yn
#define y0 _y0
#define y1 _y1
#define write _write
#define isnan _isnan
#define isinf _isinf
#endif

#include <math.h>
#include <errno.h>
#include "c_include.h"

#define	ZERO		0.0
#define	SIN_MAX		(*((double *) hex_sin_max))
#define	COS_MAX		(*((double *) hex_cos_max))
#define	NEGBIG		(*((double *) hex_negbig))

static int hex_sin_max[2] =	{0x41B1C583,0x1A000000};
static int hex_cos_max[2] =	{0x41B1C583,0x186DE04B};
static int hex_negbig[2] =	{0xFFEFFFFF,0xFFFFFFFF};

#ifdef _NAMESPACE_CLEAN
#undef jn
#pragma _HP_SECONDARY_DEF _jn jn
#define jn _jn
#endif
double jn(n,x)   int n; double x;
{
	int i;
	double a, b, temp, xsq, t;
	double j0(), j1(), error();

	if (isnan(x) || isinf(x)) {
		temp = (double) n;
		return(_error_handler("jn",&temp,&x,DOUBLE_TYPE,NAN_RET,DOMAIN));
		}
	if(n<0){
		n = -n;
		x = -x;
	}
	if(x == 0.0) {
		if (n == 0)
			return(1.0);
		else
			return(0.0);
	}

                  /* if |x| is so large as to cause total loss of precision, */
        if ((x > 0 ? x : -x) > ((SIN_MAX - COS_MAX) > 0 ? COS_MAX : SIN_MAX))
                 return error(n, x, 1);      /* go to error handling routine */

	if(n == 0) return(j0(x));
	if(n == 1) return(j1(x));

                       /* if n <= |x|, use recurrence formula to compute jn */ 
	if(n <= ((x > 0.) ? x : - x)){                       /* for n >= 2 */
	         a = j0(x);           
	         b = j1(x);    
	         for (i = 1; i < n; i++){
		         temp = b;
		         b = ((i + i) / x) * b - a;
		         a = temp;
                 }
	         return b;
        }
                                   /* if n > |x|, use different approach : */
	xsq = x * x;
	for (t = 0, i = n + 16; i > n; i--)
		t = xsq / ((i + i) - t);
	a = t = x / (2. * n - t);
	b = 1;
	for (i = n - 1; i > 0; i--){
		temp = b;
		b = ((i + i) / x) * b - a;
		a = temp;
	}
	return t * j0(x) / b;
}

#ifdef _NAMESPACE_CLEAN
#undef yn
#pragma _HP_SECONDARY_DEF _yn yn
#define yn _yn
#endif
double yn(n,x)   int n; double x;
{
	int i, sign;
	double a, b, temp;
	double y0(), y1();
        struct exception excpt;

	if (isnan(x) || isinf(x)) {
		temp = (double) n;
		return(_error_handler("yn",&temp,&x,DOUBLE_TYPE,NAN_RET,DOMAIN));
		}
	if (x <= 0) {          /* if x is non-positive, we have an error */
		temp = (double) n;
#ifdef libM
		return(_error_handler("yn",&temp,&x,DOUBLE_TYPE,NAN_RET,DOMAIN));
#else
		return(_error_handler("yn",&temp,&x,DOUBLE_TYPE,NHUGE_RET,DOMAIN));
#endif /* libM */
	}

                  /* if x is so large as to cause total loss of precision, */
        if (x > ((SIN_MAX - COS_MAX) > 0. ? COS_MAX : SIN_MAX)) 
                 return error(n, x, 0);      /* go to error handling routine */

	sign = 1;
	if (n < 0){
		n = -n;
		if (n % 2 == 1) sign = -1;
	}
	if (n == 0) return(y0(x));
	if (n == 1) return(y1(x));

	a = y0(x);                      /* use recurrence formula to compute */
	b = y1(x);                                         /* yn for n >= 2: */
	for (i = 1; i < n; i++){
		temp = b;
		b = ((i + i) / x) * b - a;
		a = temp;
	}
	return sign * b;
}
 
static double error(n, x, jnflag)  int n, jnflag; double x;
{
	double temp;

	temp = (double) n;
	return(_error_handler(jnflag? "jn" : "yn",&temp,&x,DOUBLE_TYPE,ZERO_RET,TLOSS));
}
