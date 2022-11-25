/* @(#) $Revision: 64.5 $ */   
/*

	DOUBLE PRECISION HYPERBOLIC COSINE

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
#define cosh _cosh
#define exp _exp
#define rcosh _rcosh
#define matherr _matherr
#endif /* _NAMESPACE_CLEAN */

#include	<math.h>
#include	<errno.h>

#define	ONE			1.0
#define	YBAR			(*((double *) hex_ybar))
#define	LNV			(*((double *) hex_lnv))
#define	V2M1			(*((double *) hex_v2m1))
#define	WMAX			(*((double *) hex_wmax))

#pragma FAST_ARRAY_ON
static int hex_ybar[2] =	{0x4086232B,0xDD7ABCD2};
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
#undef cosh
#pragma _HP_SECONDARY_DEF _cosh cosh
#define cosh _cosh
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

double cosh(arg) double arg; 
{
	double exp();
	realnumber y,f;
	struct exception excpt;

#ifdef hp9000s500
	/* ignore integer overflows */
	asm("
			pshr	4
			lni	0x800001
			and
			setr	4
		");
#endif

#ifdef _NAMESPACE_CLEAN
#undef matherr
#define _matherr __matherr
#endif /* _NAMESPACE_CLEAN */

#ifdef hp9000s200
	asm("	tst.w	flag_68881
		beq.w	cosh4
		lea	8(%a6),%a0");
	asm("cosh1:	
		fmov.l	%fpcr,%d0		#save control register
		movq	&0,%d1
		fmov.l	%d1,%fpcr
		fcosh.d	(%a0),%fp0		#calculate cosh
		fmov.d	%fp0,-(%sp)		#save result
		fmov.l	%fpsr,%d1		#get status register
		fmov.l	%d0,%fpcr		#restore control register
		btst	&25,%d1			#infinity?
		bne.b	cosh2
		btst	&12,%d1			#Overflow?
		bne.b	cosh2
		mov.l	(%sp)+,%d0		#move result
		mov.l	(%sp)+,%d1
		bra.w	cosh5");
	asm("cosh2:	
		movq	&-1,%d1
		mov.l	%d1,4(%sp)
		mov.l	&0x7FEFFFFF,(%sp)	#retval = HUGE
		subq.w	&8,%sp
		mov.l	(%a0)+,%d0		#arg1 = arg
		mov.l	(%a0),-(%sp)
		mov.l	%d0,-(%sp)
		pea	cosh_name
		movq	&3,%d0
		mov.l	%d0,-(%sp)
		pea	(%sp)
		jsr	_matherr		#matherr(&excpt)
		lea	(28,%sp),%sp
		tst.l	%d0
		bne.b	cosh3
		movq	&34,%d0
		mov.l	%d0,_errno");
	asm("cosh3:	
		mov.l	(%sp)+,%d0		#get retval
		mov.l	(%sp)+,%d1
		bra.w	cosh5

		data");
	asm("cosh_name:
		byte	99,111,115,104,0	#'cosh'
		
		text");
	asm("cosh4:");
#endif

#ifdef _NAMESPACE_CLEAN
#undef _matherr
#define matherr _matherr
#endif /* _NAMESPACE_CLEAN */

	y.value = arg;
	y.half.hi &= 0x7FFFFFFF;			/* y = abs(arg) */

	if (y.value <= YBAR) {
		f.value = exp(y.value);			/* sinh(y)=(exp(y)+1/exp(y))/2 */
		f.value += ONE / f.value;
		f.half.hi -= 0x100000;
		return f.value;
	};

	y.value -= LNV;					/* y = y - log(2) */

	if (y.value > WMAX) {				/* argument too big? */
		excpt.type = OVERFLOW;			/* error type */
		excpt.name = "cosh";			/* routine name */
		excpt.arg1 = arg;			/* argument */
		f.half.hi = 0x7FEFFFFF;			/* return value = MAX */
		f.half.lo = 0xFFFFFFFF;
		excpt.retval = f.value;
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return excpt.retval;			/* return value */
	};

	f.value = exp(y.value);				/* calculate cosh near MAX */
	f.value += V2M1 * f.value;
	return f.value;

#ifdef hp9000s200
	asm("cosh5:");
#endif

}

#ifdef hp9000s200
#pragma	OPT_LEVEL 2
#endif /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#undef rcosh
#pragma _HP_SECONDARY_DEF _rcosh rcosh
#define rcosh _rcosh
#endif /* _NAMESPACE_CLEAN */

double rcosh(arg) double *arg;
{
	double cosh();
	return cosh(*arg);
}
