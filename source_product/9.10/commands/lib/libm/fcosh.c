/* @(#) $Revision: 64.5 $ */      
/*

	SINGLE PRECISION HYPERBOLIC COSINE

	Author: Mark McDowell
	Date:	December 19, 1984
	Source: Software Manual for the Elementary Functions
		by William James Cody, Jr. and William Waite (1980)

	This routine will work only for numbers in IEEE single precision
	floating point format. Of the numbers in this format, it checks for 
	and handles only normalized numbers and -0 and +0 -- not infinities, 
	denormalized numbers, nor NaN's.

*/
#ifdef _NAMESPACE_CLEAN
#define matherr _matherr 
#define fcosh _fcosh
#define rfcosh _rfcosh
#define fexp _fexp
#endif

#include	<math.h>
#include	<errno.h>

#define	YBAR		0x42AEAC50
#define	WMAX		0x42B1707A
#define	ONE		((float) 1.0)
#define	LNV		(*((float *) &hex_lnv))
#define	V2M1		(*((float *) &hex_v2m1))

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
#undef fcosh
#pragma _HP_SECONDARY_DEF _fcosh fcosh
#define fcosh _fcosh
#endif

#ifdef hp9000s200
#pragma OPT_LEVEL 1
#endif /* hp9000s200 */

float fcosh(arg) float arg; 
{
	float fexp();
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

#ifdef hp9000s200
#ifdef _NAMESPACE_CLEAN
#undef fcosh
#define _matherr __matherr
#endif /* _NAMESPACE_CLEAN */

	asm("	tst.w	flag_68881
		beq.w	cosh4
		lea	8(%a6),%a0");
	asm("cosh1:
		fmov.l	%fpcr,%d1		#save control register
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fcosh.s	(%a0),%fp0		#calculate cosh
		fmov.s	%fp0,%d0		#save result
		fmov.l	%d1,%fpcr		#restore control register
		fmov.l	%fpsr,%d1		#get status register
		btst	&25,%d1			#overflow?
		beq.w	cosh5");
	asm("cosh2:
		fmov.l	%fpcr,-(%sp)		#save control register
		mov.l	&0xE0000000,-(%sp)
		mov.l	&0x47EFFFFF,-(%sp)
		subq.w	&8,%sp
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.s	(%a0),%fp0
		fmov.d	%fp0,-(%sp)
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
		movq	&0,%d0
		fmov.l	%d0,%fpcr
		fmov.d	(%sp)+,%fp0		#get retval
		fmov.s	%fp0,%d0		#save result
		fmov.l	(%sp)+,%fpcr
		bra.w	cosh5

		data");
	asm("cosh_name:
		byte	102,99,111,115,104,0	#'fcosh'

		text");
	asm("cosh4:");

#ifdef _NAMESPACE_CLEAN
#undef _matherr
#define matherr _matherr
#define fcosh _fcosh
#endif /* _NAMESPACE_CLEAN */
#endif /* hp9000s200 */

	y.value = arg;
	y.ivalue &= 0x7FFFFFFF;			/* y = abs(arg) */

	if (y.ivalue <= YBAR) {
		f.value = fexp(y.value);		/* sinh(y)=(exp(y)+1/exp(y))/2 */
		f.value += ONE / f.value;
		f.ivalue -= 0x800000;
		return f.value;
	};

	y.value -= LNV;					/* y = y - log(2) */

	if (y.ivalue > WMAX) {				/* argument too big? */
		excpt.type = OVERFLOW;			/* error type */
		excpt.name = "fcosh";			/* routine name */
		excpt.arg1 = arg;			/* argument */
		f.ivalue = 0x7F7FFFFF;			/* return value = MAX */
		excpt.retval = (double) f.value;
		if (!matherr(&excpt)) errno = ERANGE;	/* user do something with error? */
		return excpt.retval;			/* return value */
	};

	f.value = fexp(y.value);			/* calculate cosh near MAX */
	f.value += V2M1 * f.value;
	return f.value;

#ifdef hp9000s200
	asm("cosh5:");
#endif

}

#ifdef hp9000s200
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef rfcosh
#pragma _HP_SECONDARY_DEF _rfcosh rfcosh
#define rfcosh _rfcosh
#endif

float rfcosh(arg) float *arg;
{
	float fcosh();
	return fcosh(*arg);
}
