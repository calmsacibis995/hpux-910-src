/* @(#) $Revision: 64.3 $ */   
/*

	SINGLE PRECISION POWER

	Author: Mark McDowell
	Date:	January 4, 1985
	Source: Software Manual for the Elementary Functions
		by William James Cody, Jr. and William Waite (1980)

	This routine will work only for numbers in IEEE single precision
	floating point format. Of the numbers in this format, it checks for 
	and handles only normalized numbers and -0 and +0 -- not infinities, 
	denormalized numbers, nor NaN's.

*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fpow _fpow
#define rfpow _rfpow
#define matherr _matherr
#define write _write
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>
#include	<errno.h>

#define	MINUSZERO			0x80000000
#define	C0				0x3F3504F3
#define EXP_BIAS			126
#define	ZERO				0.0
#define	HALF				0.5
#define	ONE				1.0
#define	ONE_OVER_LN2			(*((double *) hex_one_over_ln2))
#define	LN2_OVER_TWO			(*((double *) hex_ln2_over_two))
#define	LN2				(*((double *) hex_ln2))
#define	POW_EPS				(*((double *) hex_pow_eps))
#define	C1				(*((double *) hex_c1))
#define	C2				(*((double *) hex_c2))
#define	POW_MAX				(*((double *) hex_pow_max))
#define	POW_MIN				(*((double *) hex_pow_min))
#define	A0				(*((double *) hex_a0))
#define	B0				(*((double *) hex_b0))
#define	P0				(*((double *) hex_p0))
#define	P1				(*((double *) hex_p1))
#define	Q0				0.5
#define	Q1				(*((double *) hex_q1))

#pragma FAST_ARRAY_ON
static int hex_one_over_ln2[2] = 	{0x3FF71547,0x652B82FE};
static int hex_ln2_over_two[2] = 	{0x3FD62E42,0xFEFA39EF};
static int hex_ln2[2] = 		{0x3FE62E42,0xFEFA39EF};
static int hex_pow_eps[2] = 		{0x3E600000,0x00000000};
static int hex_c1[2] = 			{0x3FE63000,0x00000000};
static int hex_c2[2] = 			{0xBF2BD010,0x5C610CA8};
static int hex_pow_max[2] =		{0x40562E42,0xFEBA39EF};
static int hex_pow_min[2] =		{0xC055D589,0xF2FE5107};
static int hex_a0[2] =			{0xBFE1AFC7,0x9BCF42B8};
static int hex_b0[2] =			{0xC01A87E7,0x4892DAA7};
static int hex_p0[2] =			{0x3FCFFFFF,0xFEED1F41};
static int hex_p1[2] =			{0x3F710A60,0xF9812231};
static int hex_q1[2] =			{0x3FA997EB,0x642441E5};
#pragma FAST_ARRAY_OFF

/* The following unions are used so numbers may be accessed either as floats
   or doubles or as bit patterns.				    */

typedef union {
	float value;
	unsigned long ivalue;
} 
floatnumber;

typedef union {
	double value;
	struct {
		unsigned long hi,lo;
	} half;
} 
doublenumber;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fpow
#pragma _HP_SECONDARY_DEF _fpow fpow
#define fpow _fpow
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

float fpow(x_arg,y_arg) float x_arg,y_arg;
{
	floatnumber x,y;
	doublenumber t1,t2,g;
	struct exception excpt;
	double z,xn;
	long n;
	unsigned long temp;
	int write();

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
#define _write __write
#define _matherr __matherr
#endif /* _NAMESPACE_CLEAN */

	asm("	tst.w	flag_68881
		beq.w	pow11
		lea	8(%a6),%a0
		lea	12(%a6),%a1");
	asm("pow1:
		clr.w	-2(%a6)			#assume positive result
		fmov.l	%fpcr,%d0		#save control register
		movq	&0,%d1
		fmov.l	%d1,%fpcr		#ext precision
		fmov.s	(%a1),%fp1		#%fp1 <- y
		fmov.s	(%a0),%fp0		#%fp0 <- x
		fbneq	pow2
		ftest	%fp1
		fbngt	pow5			#0.0**y with y <= 0.0
		fmov.l	%d0,%fpcr		#restore control register
		movq	&0,%d0			#0.0**y = 0.0
		bra.w	pow12");
	asm("pow2:
		fbnlt	pow3			#is x < 0.0?
		fmovm.x %fp3,-(%sp)
		fint	%fp1,%fp3
		fcmp	%fp3,%fp1
		fbne	pow4a			#x**y with x < 0.0 and y not an integer
		fmovm.x %fp2,-(%sp)
		fscale.w &-1,%fp3
		fint	%fp3,%fp2
		fcmp	%fp3,%fp2
		fsne	-2(%a6)			#set if y is odd
		fmovm.x (%sp)+,%fp2-%fp3
		fabs	%fp0");
	asm("pow3:	
		flog10	%fp0			#log10(x)
		fmul	%fp1,%fp0		#y * log10(x)
		ftentox	%fp0			#10 ** (y * log10(x))
		fmov.l	%fpsr,%d1
		andi.w	&0x1800,%d1		#overflow or underflow?
		bne.w	pow7
		tst.w	-2(%a6)			#change sign?
		beq.b	pow4
		fneg	%fp0");
	asm("pow4:
		fmov.s	%fp0,(%sp)		#get result
		fmov.l	%fpsr,%d1
		andi.w	&0x1800,%d1		#overflow or underflow?
		bne.w	pow7
		fmov.l	%d0,%fpcr		#restore control register
		mov.l	(%sp),%d0		#move result
		bra.w	pow12");
	asm("pow4a:
		fmovm.x (%sp)+,%fp3");
	asm("pow5:
		mov.l	%d0,(%sp)		#save control register
		movq	&0,%d1
		mov.l	%d1,-(%sp)		#retval = 0.0
		mov.l	%d1,-(%sp)
		fmov.s	(%a1),%fp0
		fmov.d	%fp0,-(%sp)
		fmov.s	(%a0),%fp0
		fmov.d	%fp0,-(%sp)
		pea	pow_name
		movq	&1,%d0
		mov.l	%d0,-(%sp)		#type = DOMAIN
		pea	(%sp)
		jsr	_matherr		#matherr(&excpt)
		lea	(28,%sp),%sp
		tst.l	%d0
		bne.b	pow6
		movq	&33,%d0
		mov.l	%d0,_errno		#errno = EDOM
		pea	19.w
		pea	pow_msg
		pea	2.w
		jsr	_write			#write(2,'fpow: DOMAIN error\n',19)
		lea	(12,%sp),%sp");
	asm("pow6:	
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.d	(%sp)+,%fp0		#get retval
		fmov.s	%fp0,%d0
		fmov.l	(%sp)+,%fpcr
		bra.w	pow12");
	asm("pow7:
		mov.l	%d0,(%sp)		#save control register
		and.w	&0x1000,%d1		#overflow or underflow?
		bne.b	pow8			#jump if overflow
		movq	&4,%d1
		movq	&0,%d0			#retval = 0.0
		mov.l	%d0,-(%sp)
		mov.l	%d0,-(%sp)
		bra.b	pow9");
	asm("pow8:
		movq	&3,%d1
		mov.l	&0xE0000000,-(%sp)
		mov.l	&0x47EFFFFF,-(%sp)
		tst.w	-2(%a6)
		beq.b	pow9
		bchg	&7,(%sp)");
	asm("pow9:
		fmov.s	(%a1),%fp0
		fmov.d	%fp0,-(%sp)
		fmov.s	(%a0),%fp0
		fmov.d	%fp0,-(%sp)
		pea	pow_name		#name = 'fpow'
		mov.l	%d1,-(%sp)		#push type
		pea	(%sp)
		jsr	_matherr		#matherr(&excpt)
		lea	(28,%sp),%sp
		tst.l	%d0
		bne.b	pow10
		movq	&34,%d0
		mov.l	%d0,_errno		#errno = ERANGE");
	asm("pow10:	
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.d	(%sp)+,%fp0		#get retval
		fmov.s	%fp0,%d0
		fmov.l	(%sp),%fpcr
		bra.w	pow12

		data");
	asm("pow_name:
		byte	102,112,111,119,0		#'fpow'");
	asm("pow_msg:
		byte	102,112,111,119,58,32,68,79,77	#'fpow: DOMAIN error\n'
		byte	65,73,78,32,101,114,114,111
		byte	114,10,0

		text");
	asm("pow11:");

#ifdef _NAMESPACE_CLEAN
#undef _matherr
#undef _write
#endif /* _NAMESPACE_CLEAN */
#endif /* hp9000s200 */

	x.value = x_arg;
	if (x.ivalue == MINUSZERO) x.ivalue = 0;
	y.value = y_arg;
	if (y.ivalue == MINUSZERO) y.ivalue = 0;

	if (!x.ivalue) {
		if ((long) y.ivalue <= 0) goto domain_error; /* error for 0^non-pos */
		return (float) ZERO;			/* 0^pos = zero */
	};
	if (!y.ivalue) return (float) ONE;		/* x^0 = one */
	if ((long) x.ivalue < 0) {			/* is x negative? */
		/* now must determine if y is an integer */
		temp = y.ivalue & 0x7FFFFFFF;
		if (temp < 0x3F800000) goto domain_error;
		if (temp < 0x4B800000) {
			n = 1 << 150 - (temp >> 23);
			if (temp & n - 1) goto domain_error;
			n = temp & n ? 1 : 0;
		} else n = 0;	/* huge integer: LSB must be 0 */
		x.ivalue &= 0x7FFFFFFF;		/* abs(x) */
		x.value = fpow((float) x.value,(float) y_arg);
		if (n) x.ivalue ^= 0x80000000;	/* fix sign */
		return x.value;
	};

	n = (x.ivalue >> 23) - EXP_BIAS;		/* isolate exponent */
	x.ivalue &= 0x7FFFFF;				/* scale to between 0.5 and 1.0 */
	x.ivalue |= EXP_BIAS << 23;

	if (x.ivalue > C0) {				/* compare with sqrt(2)/2 */
		t1.value = t2.value = (double) x.value;	/* f = (f - 1) / (f * .5 + .5) */
		t1.half.hi -= 0x100000;
		t2.value = ((t2.value - HALF) - HALF) / (t1.value + HALF);
	}
	else {
		n--;
		t1.value = (double) x.value - HALF;	/* f = (f - .5) / ((f - .5) * .5 + .5) */
		t2.value = t1.value / (t1.value * HALF + HALF);
	};

	t1.value = t2.value * t2.value;		/* calculate log */
	xn = (double) n;			/* convert to double */
	t1.value = (double) y.value * (((xn * C2) + (t2.value 
		+ (t2.value * (t1.value * A0 / (t1.value + B0))))) + (xn * C1));

	if (t1.value > POW_MAX) {			/* overflow? */
		excpt.type = OVERFLOW;			/* error type */
		excpt.name = "fpow";			/* routine name */
		excpt.arg1 = (double) x_arg;		/* arguments */
		excpt.arg2 = (double) y_arg;
		t1.half.hi = 0x47EFFFFF;		/* return value = MAX */
		t1.half.lo = 0xE0000000;
		excpt.retval = t1.value;
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return (float) excpt.retval;		/* return value */
	};

	if (t1.value < POW_MIN) {			/* underflow? */
		excpt.type = UNDERFLOW;			/* error type */
		excpt.name = "fpow";			/* routine name */
		excpt.arg1 = (double) x_arg;		/* arguments */
		excpt.arg2 = (double) y_arg;
		excpt.retval = ZERO;			/* return value = 0.0 */
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return (float) excpt.retval;		/* return value */
	};

	temp = t1.half.hi & 0x80000000;		/* save sign of argument */
	t1.half.hi ^= temp;			/* t1 = abs(arg) */

	if (t1.value < POW_EPS) return (float) ONE; /* for small values, answer is 1 */

	if (t1.value > LN2_OVER_TWO) {		/* argument reduction needed? */
		n = (long) (t1.value * ONE_OVER_LN2 + HALF);  /* yes: divide by log(2) */
		xn = (double) n;		/* convert to double */
		g.value = t1.value - xn * LN2;	/* reduce */
		g.half.hi ^= temp;		/* adjust sign appropriately */
		if (temp) n = -n;		/* adjust multiple's sign */
	}
	else {					/* no reduction needed */
		n = 0;
		g.value = t1.value;
		g.half.hi |= temp;		/* restore sign */
	};

	z = g.value * g.value;			/* calculate exp */
	t1.value = (P1 * z + P0) * g.value;
	t2.value = Q1 * z + Q0;
	t1.value = HALF + (t1.value / (t2.value - t1.value));
	t1.half.hi += (n + 1) << 20;		/* adjust exponent */
	return (float) t1.value;

domain_error:						/* bad argument(s) */
	excpt.type = DOMAIN;				/* error type */
	excpt.name = "fpow";				/* routine name */
	excpt.arg1 = (double) x_arg;			/* arguments */
	excpt.arg2 = (double) y_arg;
	excpt.retval = ZERO;				/* return value = 0.0 */
	if (!matherr(&excpt)) {				/* user do something with error? */
		errno = EDOM;				/* no: domain error */
		(void) write(2,"fpow: DOMAIN error\n",19); /* print a message */
	};
	return (float) excpt.retval;			/* return value */

#ifdef hp9000s200
	asm("pow12:");
#endif /* hp9000s200 */

}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef rfpow
#pragma _HP_SECONDARY_DEF _rfpow rfpow
#define rfpow _rfpow
#endif /* _NAMESPACE_CLEAN */

float rfpow(x_arg,y_arg) float *x_arg,*y_arg;
{
	float fpow();
	return fpow((float) *x_arg,(float) *y_arg);
}
