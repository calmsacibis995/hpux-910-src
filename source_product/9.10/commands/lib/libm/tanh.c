/* @(#) $Revision: 64.4 $ */   
/*

	DOUBLE PRECISION HYPERBOLIC TANGENT

	Author: Mark McDowell
	Date:	December 10, 1984
	Source: Software Manual for the Elementary Functions
		by William James Cody, Jr. and William Waite (1980)

	This routine will work only for numbers in IEEE double precision
	floating point format. Of the numbers in this format, it checks for 
	and handles only normalized numbers and -0 and +0 -- not infinities, 
	denormalized numbers, nor NaN's.

*/

#ifdef _NAMESPACE_CLEAN
#define exp _exp
#define rtanh _rtanh
#define tanh _tanh
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>

#define	HALF			0.5
#define	ONE			1.0
#define	XBIG			(*((double *) hex_xbig))
#define	LOG3_OVER_2		(*((double *) hex_log3_over_2))
#define	TANH_EPS		(*((double *) hex_tanh_eps))
#define	P0			(*((double *) hex_p0))
#define	P1			(*((double *) hex_p1))
#define	P2			(*((double *) hex_p2))
#define	Q0			(*((double *) hex_q0))
#define	Q1			(*((double *) hex_q1))
#define	Q2			(*((double *) hex_q2))

#pragma FAST_ARRAY_ON
static int hex_xbig[2] =	{0x40330FC1,0x931F09CA};
static int hex_log3_over_2[2] =	{0x3FE193EA,0x7AAD030B};
static int hex_tanh_eps[2] =	{0x3E46A09E,0x667F3BCD};
static int hex_p0[2] =		{0xC09935A5,0xC9BE1E18};
static int hex_p1[2] =		{0xC058CE75,0xA1BA5CCC};
static int hex_p2[2] =		{0xBFEEDC28,0xCEFBA77F};
static int hex_q0[2] =		{0x40B2E83C,0x574E9693};
static int hex_q1[2] =		{0x40A1738B,0x4D01F0F3};
static int hex_q2[2] =		{0x405C2FA9,0xE1EBF7FA};
#pragma FAST_ARRAY_OFF



/* The following union is used so numbers may be accessed either as doubles
   or as bit patterns. 							    */

typedef union {
	double value;
	struct {
		unsigned long hi,lo;
	} 
	half;
} 
realnumber;



#ifdef _NAMESPACE_CLEAN
#undef tanh
#pragma _HP_SECONDARY_DEF _tanh tanh
#define tanh _tanh
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

double tanh(arg) double arg; 
{
	double exp();
	realnumber f,g;
	unsigned long argsign;

#ifdef hp9000s500
	/* ignore integer overflows */
	asm("
			pshr	4
			lni	0x800001
			and
			setr	4
		");
#endif /* hp9000s500 */
#ifdef hp9000s200
	asm("	tst.w	flag_68881
		beq.b	tanh2
		lea	8(%a6),%a0");
	asm("tanh1:
		fmov.l	%fpcr,%d0		#save control register
		movq	&0,%d1
		fmov.l	%d1,%fpcr
		ftanh.d	(%a0),%fp0		#calculate tanh
		fmov.d	%fp0,-(%sp)		#save result
		fmov.l	%d0,%fpcr		#restore control register
		mov.l	(%sp)+,%d0		#get result
		mov.l	(%sp)+,%d1
		bra.w	tanh3");
	asm("tanh2:");
#endif /* hp9000s200 */

	f.value = arg;
	argsign = f.half.hi & 0x80000000;		/* save sign */
	f.half.hi ^= argsign;				/* f = abs(arg) */

	if (f.value > XBIG) {				/* tanh of large value is +1 or -1 */
		f.value = ONE;
		f.half.hi |= argsign;			/* give proper sign */
		return f.value;
	};

	if (f.value > LOG3_OVER_2) {			/* compare with log(3)/2 */
		f.half.hi += 0x100000;			/* f=1/2-1/(exp(f+f)+1) */
		f.value = HALF - ONE / (exp(f.value) + ONE);
		f.half.hi += 0x100000;
		f.half.hi ^= argsign;			/* give proper sign */
		return f.value;
	};

	if (f.value < TANH_EPS) return arg;		/* tanh(x) = x for small x */
	g.value = f.value * f.value;			/* calculate tanh */
	f.value += f.value * ((((P2 * g.value + P1) * g.value + P0) * g.value) /
	    (((g.value + Q2) * g.value + Q1) * g.value + Q0));
	f.half.hi ^= argsign;				/* give proper sign */
	return f.value;

#ifdef hp9000s200
	asm("tanh3:");
#endif /* hp9000s200 */

}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#undef rtanh
#pragma _HP_SECONDARY_DEF _rtanh rtanh
#define rtanh _rtanh
#endif /* _NAMESPACE_CLEAN */

double rtanh(arg) double *arg;
{
	double tanh();
	return tanh(*arg);
}
