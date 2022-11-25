/* @(#) $Revision: 64.4 $ */      
/*

	SINGLE PRECISION HYPERBOLIC SINE

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
#define rfsinh _rfsinh
#define fsinh _fsinh
#define matherr _matherr
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>
#include	<errno.h>

#define	SINH_EPS	0x39800000
#define	YBAR		0x42AEAC50
#define	WMAX		0x42B1707A
#define	HEX_ONE		0x3F800000
#define	ONE		((float) 1.0)
#define P0		(*((float *) &hex_p0))
#define P1		(*((float *) &hex_p1))
#define Q0		(*((float *) &hex_q0))
#define	LNV		(*((float *) &hex_lnv))
#define	V2M1		(*((float *) &hex_v2m1))

static int hex_p0 =	0xC0E469F0;
static int hex_p1 =	0xBE42E6C2;
static int hex_q0 =	0xC22B4F93;
static int hex_lnv =	0x3F317300;
static int hex_v2m1 =	0x37680897;


/* The following union is used so numbers may be accessed either as floats
   or as bit patterns. 							    */

typedef union {
	float value;
	unsigned long ivalue;
} 
realnumber;


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fsinh
#pragma _HP_SECONDARY_DEF _fsinh fsinh
#define fsinh _fsinh
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

float fsinh(arg) float arg; 
{
	float fexp();
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

#ifdef hp9000s200

#ifdef _NAMESPACE_CLEAN
#undef fsinh              /*  _fsinh will interfere with assembling  */
#define _matherr __matherr
#endif /* _NAMESPACE_CLEAN */

	asm("	tst.w	flag_68881
		beq.w	sinh5
		lea	8(%a6),%a0");
	asm("sinh1:
		fmov.l	%fpcr,%d1		#save control register
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fsinh.s	(%a0),%fp0		#calculate sinh
		fmov.s	%fp0,%d0		#save result
		fmov.l	%d1,%fpcr		#restore control register
		fmov.l	%fpsr,%d1		#get status register
		btst	&25,%d1
		beq.w	sinh6");
	asm("sinh2:
		fmov.l	%fpcr,-(%sp)		#save control register
		mov.l	&0xE0000000,-(%sp)
		mov.l	&0x47EFFFFF,%d0
		tst.b	(%a0)			#check sign of arg
		bpl.b	sinh3
		bchg	%d0,%d0			#retval = -HUGE");
	asm("sinh3:
		mov.l	%d0,-(%sp)
		subq.w	&8,%sp
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.s	(%a0),%fp0
		fmov.d	%fp0,-(%sp)
		pea	sinh_name
		movq	&3,%d0
		mov.l	%d0,-(%sp)
		pea	(%sp)
		jsr	_matherr		#matherr(&excpt)
		lea	(28,%sp),%sp
		tst.l	%d0
		bne.b	sinh4
		movq	&34,%d0
		mov.l	%d0,_errno");
	asm("sinh4:	
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.d	(%sp)+,%fp0		#get retval
		fmov.s	%fp0,%d0		#save result
		fmov.l	(%sp)+,%fpcr		#restore control register
		bra.w	sinh6

		data");
	asm("sinh_name:
		byte	102,115,105,110,104,0	#'fsinh'

		text");
	asm("sinh5:");

#ifdef _NAMESPACE_CLEAN
#define fsinh _fsinh
#undef _matherr
#endif  /* _NAMESPACE_CLEAN */

#endif    /*  hp9000s200  */

	y.value = arg;
	argsign = y.ivalue & 0x80000000;		/* save sign */
	y.ivalue ^= argsign;				/* y = abs(arg) */

	if (y.ivalue <= HEX_ONE) {
		if (y.ivalue < SINH_EPS) return arg;	/* sinh(x) = x for small x */
		f.value = arg * arg;			/* use polynomial for sinh */
		return arg + arg * (f.value * ((P1 * f.value + P0) 
			/ (f.value + Q0)));
	};

	if (y.ivalue <= YBAR) {
		f.value = fexp(y.value);		/* sinh(y) = (exp(y)-1/exp(y))/2 */
		f.value -= ONE / f.value;
		f.ivalue -= 0x800000;
		f.ivalue ^= argsign;			/* give proper sign */
		return f.value;
	};

	y.value -= LNV;					/* y = y - log(2) */

	if (y.ivalue > WMAX) {				/* argument too big? */
		excpt.type = OVERFLOW;			/* error type */
		excpt.name = "fsinh";			/* routine name */
		excpt.arg1 = (double) arg;		/* argument */
		f.ivalue = argsign | 0x7F7FFFFF;	/* return value = +MAX or - MAX */
		excpt.retval = f.value;
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return excpt.retval;			/* return value */
	};

	f.value = fexp(y.value);			/* calculate sinh near MAX */
	f.value += V2M1 * f.value;
	f.ivalue ^= argsign;				/* fix sign */
	return f.value;

#ifdef hp9000s200
	asm("sinh6:");
#endif /* hp9000s200 */
}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef rfsinh
#pragma _HP_SECONDARY_DEF _rfsinh rfsinh
#define rfsinh _rfsinh
#endif

float rfsinh(arg) float *arg;
{
	float fsinh();
	return fsinh(*arg);
}
