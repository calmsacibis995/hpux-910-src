/* @(#) $Revision: 64.3 $ */      
/*

	SINGLE PRECISION HYPERBOLIC TANGENT

	Author: Mark McDowell
	Date:	December 19, 1984
	Source: Software Manual for the Elementary Functions
		by William James Cody, Jr. and William Waite (1980)

	This routine will work only for numbers in IEEE single precision
	floating point format. Of the numbers in this format, it checks for 
	and handles only normalized numbers and -0 and +0 -- not infinities, 
	denormalized numbers, nor NaN's.

*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fexp _fexp
#define ftanh _ftanh
#define rftanh _rftanh
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>

#define	XBIG		0x41102CB3
#define	LOG3_OVER_2	0x3F0C9F54
#define	TANH_EPS	0x39800000
#define	HALF		((float) 0.5)
#define	ONE		((float) 1.0)
#define	P0		(*((float *) &hex_p0))
#define	P1		(*((float *) &hex_p1))
#define	Q0		(*((float *) &hex_q0))

static int hex_p0 =	0xBF52E2C6;
static int hex_p1 =	0xBB7B11B2;
static int hex_q0 =	0x401E2A1A;


/* The following union is used so numbers may be accessed either as floats
   or as bit patterns. 							    */

typedef union {
	float value;
	unsigned long ivalue;
} 
realnumber;


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ftanh
#pragma _HP_SECONDARY_DEF _ftanh ftanh
#define ftanh _ftanh
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

float ftanh(arg) float arg; 
{
	float fexp();
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

#ifdef _NAMESPACE_CLEAN
#undef ftanh             /*  undef'd here so asm code will assemble */
#endif  /*  _NAMESPACE_CLEAN  */

	asm("	tst.w	flag_68881
		beq.b	tanh1
		lea	8(%a6),%a0
		fmov.l	%fpcr,%d1		#save control register
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		ftanh.s	(%a0),%fp0		#calculate tanh
		fmov.s	%fp0,%d0		#save result
		fmov.l	%d1,%fpcr		#restore control register
		bra.w	tanh2");
	asm("tanh1:");

#ifdef _NAMESPACE_CLEAN
#define ftanh _ftanh
#endif   /*  _NAMESPACE_CLEAN  */

#endif  /* hp9000s200  */

	f.value = arg;
	argsign = f.ivalue & 0x80000000;		/* save sign */
	f.ivalue ^= argsign;				/* f = abs(arg) */

	if (f.ivalue > XBIG) {				/* tanh of large value is +1 or -1 */
		f.value = ONE;
		f.ivalue |= argsign;			/* give proper sign */
		return f.value;
	};

	if (f.ivalue > LOG3_OVER_2) {			/* compare with log(3)/2 */
		f.ivalue += 0x800000;			/* f=1/2-1/(exp(f+f)+1) */
		f.value = HALF - ONE / (fexp(f.value) + ONE);
		f.ivalue += 0x800000;
		f.ivalue ^= argsign;			/* give proper sign */
		return f.value;
	};

	if (f.ivalue < TANH_EPS) return arg;		/* tanh(x) = x for small x */
	g.value = f.value * f.value;			/* calculate tanh */
	f.value += f.value * (((P1 * g.value + P0) * g.value) / (g.value + Q0));
	f.ivalue ^= argsign;				/* give proper sign */
	return f.value;

#ifdef hp9000s200
	asm("tanh2:");
#endif /* hp9000s200 */

}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef rftanh
#pragma _HP_SECONDARY_DEF _rftanh rftanh
#define rftanh _rftanh
#endif /* _NAMESPACE_CLEAN */

float rftanh(arg) float *arg;
{
	float ftanh();
	return ftanh(*arg);
}
