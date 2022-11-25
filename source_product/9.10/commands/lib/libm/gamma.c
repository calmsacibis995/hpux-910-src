/* @(#) $Revision: 66.2 $ */      
/* 

        DOUBLE PRECISION LOG GAMMA FUNCTION OF DOUBLE PRECISION ARGUMENT

	gamma(x) computes the log of the absolute
	value of the gamma function.
	The sign of the gamma function is returned in the
	external quantity signgam.

	The coefficients for expansion around zero
	are #5243 from Hart & Cheney; for expansion
	around infinity they are #5404.

	Calls log and sin.

        Ported from HP9000 by Chih-Ming Hu
        Modified by Chih-Ming Hu to prevent unnecessary overflow during
                           computation, to fix the bad code for negative
                           argument and to meet the specification of 
                           UNIX System V - Release 2.0.
        Modification date: March 4, 1985.

*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define signgam _signgam
#define sin _sin
#define gamma _gamma
#define lgamma _lgamma
#define write _write
#define log _log
#define modf _modf
#endif

#include <errno.h>
#include <math.h>
#include "c_include.h"

#define M 6
#define N 8
#define ARG_MAX  2.556348163871689e305
#define SIN_MAX	 (*((double *) hex_sin_max))
#define	PI	 (*((double *) hex_pi))
#define BIG	 (*((double *) hex_big))
  
extern int errno;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef signgam
#pragma _HP_SECONDARY_DEF _signgam signgam
#define signgam _signgam
#endif

int	signgam = 0;

static int hex_sin_max[2] =	{0x41B1C583,0x1A000000};
static int hex_pi[2] =		{0x400921FB,0x54442D18};
static int hex_big[2] = 	{0x7FEFFFFF,0xFFFFFFFF};

static double goobie	= 0.9189385332046727417803297;

static double p1[] = {
	0.83333333333333101837e-1,
	-.277777777735865004e-2,
	0.793650576493454e-3,
	-.5951896861197e-3,
	0.83645878922e-3,
	-.1633436431e-2,
};
static double p2[] = {
	-.42353689509744089647e5,
	-.20886861789269887364e5,
	-.87627102978521489560e4,
	-.20085274013072791214e4,
	-.43933044406002567613e3,
	-.50108693752970953015e2,
	-.67449507245925289918e1,
	0.0,
};
static double q2[] = {
	-.42353689509744090010e5,
	-.29803853309256649932e4,
	0.99403074150827709015e4,
	-.15286072737795220248e4,
	-.49902852662143904834e3,
	0.18949823415702801641e3,
	-.23081551524580124562e2,
	0.10000000000000000000e1,
};

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define isnan _isnan
#define isinf _isinf
#undef gamma
#pragma _HP_SECONDARY_DEF _gamma gamma
#define gamma _gamma
#endif

double gamma(arg)  double arg;
{
	double temp, log(), pos(), neg(), asym();
        struct exception excpt;

	signgam = 1.0;
	if (isnan(arg)) {
		return(_error_handler("gamma",&arg,&arg,
					DOUBLE_TYPE,NAN_RET,DOMAIN));
	}
	if(arg <= 0.0) {                    /* if argument is non positive */ 
		if (!modf(arg, &temp)) {
			return(_error_handler("gamma",&arg,&arg,
					DOUBLE_TYPE,HUGE_RET,SING));
		}
		return(neg(arg));
        }
	if(arg > 8.0){                              /* for large argument */
		if (isinf(arg))
			return(_error_handler("gamma",&arg,&arg,DOUBLE_TYPE,INF_RET,DOMAIN));
		else if (arg >= ARG_MAX){                
			return(_error_handler("gamma",&arg,&arg,
					DOUBLE_TYPE,HUGE_RET,OVERFLOW));
                }
                return(asym(arg));    /* otherwise use asymptotic approach*/
        }
	return(log(pos(arg)));
}

static double asym(arg) double arg;
{
	double log();
	double n, argsq;
	int i;

	argsq = (1. / arg) * (1. / arg);
	for (n = 0, i = M-1; i >= 0; i--){
		n = n * argsq + p1[i];
	}
	return((arg - .5) * log(arg) - arg + goobie + n / arg);
}

static double neg(arg)  double arg;                /* for negative arg. */
{
	double temp;
	double log(), sin(), pos();

	arg = -arg;                                /* use absolute value */
	temp = sin(PI*arg);
	if(temp < 0.) temp = -temp;                /* use absolute value */
	else signgam = -1;
        if (arg > 8.0){              /* if arg > 8 use asymptotic approach */
                                /* if arg is so large as to cause sin to */
                if (arg > SIN_MAX / PI)            /* lose total precision */
                                /* neglect the small term containing sin: */
                         return -asym(arg);
                else return -( log(arg * temp / PI) + asym(arg) );
        }
                                     /* otherwise use regular approach */
	return -( log(arg * temp / PI) + log(pos(arg)) );
}

static double pos(arg)  double arg;            /* for small positive arg. */


{
	double n, d, s;
	register i;

	if(arg < 2.) return(pos(arg+1.)/arg);
	if(arg > 3.) return((arg-1.)*pos(arg-1.));

	s = arg - 2.;
	for(n=0,d=0,i=N-1; i>=0; i--){
		n = n*s + p2[i];
		d = d*s + q2[i];
	}
	return(n/d);
}

/* lgamma -
 * 	The lgamma routine performs the exact task as gamma().  The
 *	name was changed for XPG3.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef lgamma
#pragma _HP_SECONDARY_DEF _lgamma lgamma
#define lgamma _lgamma
#endif

double 
lgamma(arg) 
double arg;
{
	return(gamma(arg));
}
