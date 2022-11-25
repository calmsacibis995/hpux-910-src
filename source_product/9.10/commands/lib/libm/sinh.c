/* @(#) $Revision: 64.6 $ */   
/*

	DOUBLE PRECISION HYPERBOLIC SINE

	Author: Mark McDowell
	Date:	December 11, 1984
	Source: Software Manual for the Elementary Functions
		by William James Cody, Jr. and William Waite (1980)

	This routine will work only for numbers in IEEE double precision
	floating point format. Of the numbers in this format, it checks for 
	and handles only normalized numbers and -0 and +0 -- not infinities, 
	denormalized numbers, nor NaN's.

*/

#ifdef _NAMESPACE_CLEAN
#define exp _exp
#define matherr _matherr
#define rsinh _rsinh
#define sinh _sinh
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>
#include	<errno.h>

#define	ONE			1.0
#define	SINH_EPS		(*((double *) hex_sinh_eps))
#define	P0			(*((double *) hex_p0))
#define	P1			(*((double *) hex_p1))
#define	P2			(*((double *) hex_p2))
#define	P3			(*((double *) hex_p3))
#define	Q0			(*((double *) hex_q0))
#define	Q1			(*((double *) hex_q1))
#define	Q2			(*((double *) hex_q2))
#define	YBAR			(*((double *) hex_ybar))
#define	LNV			(*((double *) hex_lnv))
#define	V2M1			(*((double *) hex_v2m1))
#define	WMAX			(*((double *) hex_wmax))

#pragma FAST_ARRAY_ON
static int hex_sinh_eps[2] = 	{0x3E46A09E,0x667F3BCD};
static int hex_ybar[2] =	{0x4086232B,0xDD7ABCD2};
static int hex_p0[2] =		{0xC1157913,0x56533419};
static int hex_p1[2] =		{0xC0C695C2,0xB6941490};
static int hex_p2[2] =		{0xC0647841,0x6385BE4A};
static int hex_p3[2] =		{0xBFE944E7,0xB86FC81B};
static int hex_q0[2] =		{0xC1401ACE,0x80BE6713};
static int hex_q1[2] =		{0x40E1A857,0x23B65EC7};
static int hex_q2[2] =		{0xC0715BC3,0x81C97FF2};

static int hex_lnv[2] =		{0x3FE62E60,0x00000000};
static int hex_v2m1[2] =	{0x3EED0112,0xEB0202D6};
static int hex_wmax[2] =	{0x40862E3C,0x85B28BDB};
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
#undef sinh
#pragma _HP_SECONDARY_DEF _sinh sinh
#define sinh _sinh
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

double sinh(arg) double arg; 
{
	double exp();
	realnumber y,f;
	struct exception excpt;
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

#ifdef _NAMESPACE_CLEAN
#undef matherr
#define _matherr __matherr
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
	asm("	tst.w	flag_68881
		beq.w	sinh5
		lea	8(%a6),%a0");
	asm("sinh1:
		fmov.l	%fpcr,%d0		#save control register
		movq	&0,%d1
		fmov.l	%d1,%fpcr
		fsinh.d	(%a0),%fp0		#calculate sinh
		fmov.d	%fp0,-(%sp)		#save result
		fmov.l	%fpsr,%d1		#get status register
		fmov.l	%d0,%fpcr		#restore control register
		btst	&25,%d1			#Infinity?
		bne.b	sinh2
		btst	&12,%d1			#Overflow?
		bne.b	sinh2
		mov.l	(%sp)+,%d0		#move result
		mov.l	(%sp)+,%d1
		bra.w	sinh6");
	asm("sinh2:
		movq	&-1,%d1
		mov.l	%d1,4(%sp)
		mov.l	&0x7FEFFFFF,%d1		#assume retval = HUGE
		tst.b	(%a0)			#check sign of arg
		bpl.b	sinh3
		bchg	%d1,%d1			#retval = -HUGE");
	asm("sinh3:
		mov.l	%d1,(%sp)
		subq.w	&8,%sp
		mov.l	(%a0)+,%d0		#arg1 = arg
		mov.l	(%a0),-(%sp)
		mov.l	%d0,-(%sp)
		pea	sinh_name		#name = 'sinh'
		movq	&3,%d1
		mov.l	%d1,-(%sp)
		pea	(%sp)
		jsr	_matherr		#matherr(&excpt)
		lea	(28,%sp),%sp
		tst.l	%d0
		bne.b	sinh4
		movq	&34,%d0
		mov.l	%d0,_errno");
	asm("sinh4:	
		mov.l	(%sp)+,%d0		#get retval
		mov.l	(%sp)+,%d1
		bra.w	sinh6

		data");
	asm("sinh_name:
		byte	115,105,110,104,0	#'sinh'

		text");
	asm("sinh5:");
#endif /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#undef _matherr
#define matherr _matherr
#endif /* _NAMESPACE_CLEAN */

	y.value = arg;
	argsign = y.half.hi & 0x80000000;		/* save sign */
	y.half.hi ^= argsign;				/* y = abs(arg) */

	if (y.value <= ONE) {
		if (y.value < SINH_EPS) return arg;	/* sinh(x) = x for small x */
		f.value = arg * arg;			/* use polynomial for sinh */
		return arg + arg * (f.value * ((((P3 * f.value + P2) * f.value 
		    + P1) * f.value + P0) / (((f.value + Q2) * f.value + Q1) 
		    * f.value + Q0)));
	};

	if (y.value <= YBAR) {
		f.value = exp(y.value);			/* sinh(y) = (exp(y)-1/exp(y))/2 */
		f.value -= ONE / f.value;
		f.half.hi -= 0x100000;
		f.half.hi ^= argsign;			/* give proper sign */
		return f.value;
	};

	y.value -= LNV;					/* y = y - log(2) */

	if (y.value > WMAX) {				/* argument too big? */
		excpt.type = OVERFLOW;			/* error type */
		excpt.name = "sinh";			/* routine name */
		excpt.arg1 = arg;			/* argument */
		f.half.hi = argsign | 0x7FEFFFFF;	/* return value = +MAX or - MAX */
		f.half.lo = 0xFFFFFFFF;
		excpt.retval = f.value;
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return excpt.retval;			/* return value */
	};

	f.value = exp(y.value);				/* calculate sinh near MAX */
	f.value += V2M1 * f.value;
	f.half.hi ^= argsign;				/* fix sign */
	return f.value;

#ifdef hp9000s200
	asm("sinh6:");
#endif /* hp9000s200 */

}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#undef rsinh
#pragma _HP_SECONDARY_DEF _rsinh rsinh
#define rsinh _rsinh
#endif /* _NAMESPACE_CLEAN */

double rsinh(arg) double *arg;
{
	double sinh();
	return sinh(*arg);
}
