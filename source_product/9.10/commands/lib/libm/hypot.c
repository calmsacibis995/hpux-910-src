/* @(#) $Revision: 66.3 $ */      
/*
	DOUBLE PRECISION SQRT(A^2 + B^2) OF DOUBLE PRECISION A & B

	Ported from HP9000 by Chih-Ming Hu
	Modified by Chih-Ming Hu to prevent both overflow and
		    underflow during computation.
	Date: February 8, 1985

	Modified by Mark McDowell to call matherr on overflow
	Date: March 14, 1985
*/
#ifdef _NAMESPACE_CLEAN
#define sqrt _sqrt
#define hypot _hypot
#define cabs _cabs
#define isnan _isnan
#define isinf _isinf
#endif

#include <errno.h>
#include <math.h>
#include "c_include.h"

#define	HUGE_OVER_SQRT2	(*((double *) hex_huge_over_sqrt2))
#define	HUGE_OVER_2	(*((double *) hex_huge_over_2))
#define INFINITY	(*((double *) hex_inf))

static int hex_huge_over_sqrt2[2] =	{0x7FE6A09E,0x667F3BCD};
static int hex_huge_over_2[2] =		{0x7FDFFFFF,0xFFFFFFFF};
static int hex_hugenum[2] =		{0x7FEFFFFF,0xFFFFFFFF};
static int hex_inf[2] =			{0x7FF00000,0x00000000};

#ifdef _NAMESPACE_CLEAN
#undef hypot
#pragma _HP_SECONDARY_DEF _hypot hypot
#define hypot _hypot
#endif /* _NAMESPACE_CLEAN */

double hypot(a,b) 
double a,b;
{
	struct exception excpt;
	double t;

	if (isnan(a) || isnan(b))
		return(_error_handler("hypot",&a,&b,DOUBLE_TYPE,NAN_RET,DOMAIN));

	if (isinf(a) || isinf(b))
		return(INFINITY);

	if ((excpt.arg1 = a) < 0.0) a = -a;
	if ((excpt.arg2 = b) < 0.0) b = -b;

	if (a > b) {				/* make sure a <= b */
		t = a;
		a = b;
		b = t;
	}

	/* if a is too small compared to b, simply neglect it */
	if (b <= HUGE_OVER_2) {
		if (b + a == b) return b;
	} else if (b - a == b) return b;

	/* Now it's safe to divide without causing underflow */
	a /= b;

	a = sqrt(1.0 + a * a);

	if (b < HUGE_OVER_SQRT2) return a * b;

	if ((a *= 0.5 * b) > HUGE_OVER_2) {
		return(_error_handler("hypot",&excpt.arg1,&excpt.arg2,DOUBLE_TYPE,HUGE_RET,OVERFLOW));
	} 
	else return a + a;
}

struct	complex
{
	double	r;
	double	i;
};

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef cabs
#pragma _HP_SECONDARY_DEF _cabs cabs
#define cabs _cabs
#endif

double
cabs(arg)
struct complex arg;
{
	double hypot();

	return(hypot(arg.r, arg.i));
}
