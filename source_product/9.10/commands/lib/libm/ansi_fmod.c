/* @(#) $Revision: 64.1 $ */   
/*  source:  System III                                    */
/*     /usr/src/lib/libm/fmod.c                            */
/*     dsb - 3/8/82                                        */

#ifdef hp9000s200

#ifdef _NAMESPACE_CLEAN
#define     _fdiv ___fdiv
#define     _fmul ___fmul       
#define	    _fsub ___fsub
#endif

#include <errno.h>

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _fmod fmod
#define fmod _fmod
#endif /* _NAMESPACE_CLEAN */

#pragma OPT_LEVEL 1

double fmod (a, b) double a, b;

#ifdef _NAMESPACE_CLEAN
#undef fmod
#define _fmod __fmod
#endif /* _NAMESPACE_CLEAN */

{
   if (b == 0.0) {
	errno = EDOM;
	return(a);
   }
   asm("mov.w	&0x7FF0,%d0	# exponent mask
	mov.w	%d0,%d1		# save mask
	and.w	8(%a6),%d0	# isolate first exponent
	beq.w	fmod7		# special case zeroes and denorms
	cmp.w	%d0,%d1		# special case infinities and NaN's
	beq.w	fmod7
	mov.w	%d0,-(%sp)	# save exponent
	mov.w	%d1,%d0		# mask
	and.w	16(%a6),%d0	# isolate second exponent
	beq.w	fmod7		# special case zeroes and denorms
	cmp.w	%d0,%d1		# special case infinities and NaN's
	beq.w	fmod7
	tst.w	flag_68881	# 68881 present?
	beq.b	fmod1
	fmov.l	%fpcr,%d0	# save control register
	fmov.l	&0x80,%fpcr	# double precision/no traps
	fmov.d	8(%a6),%fp0	# first argument
	fmov	%fp0,%fp1	# save argument
	fdiv.d	16(%a6),%fp0	# need to check for division overflow
	fmov.l	%fpsr,%d1
	fmov.l	%d0,%fpcr	# restore control register
	btst	&12,%d1		# overflow?
	bne.w	fmod4
	fmod.d	16(%a6),%fp1	# fmod function
	fmov.d	%fp1,-(%sp)
	movm.l	(%sp),%d0-%d1	# result
	bra.w	fmod8");
    asm("fmod1:
	sub.w	%d0,(%sp)	# difference of exponents
	bmi.w	fmod7		# done if second number > first number
	not.w	%d1		# invert mask
	mov.w	&0x3FE0,%d0	# new exponent:
	lea	20(%a6),%a0	#    exponent will be fixed before
	mov.l	(%a0),-(%sp)	#    division to prevent overflow
	mov.l	-(%a0),-(%sp)   # get second number
	and.w	%d1,(%sp)	# mask out exponent
	or.w	%d0,(%sp)	# put in new exponent
	mov.l	-(%a0),-(%sp)	# get first number
	mov.l	-(%a0),-(%sp)
	and.w	%d1,(%sp)	# mask out exponent
	or.w	%d0,(%sp)	# put in new exponent
	jsr	_fdiv	        # first number/second number
	addq.w	&8,%sp
	movm.l	%d0-%d1,(%sp)   # move result to stack
	mov.w	&0x7FF0,%d0	# exponent mask
	mov.w	%d0,%d1		# save mask
	and.w	(%sp),%d0	# get exponent from dividend
	add.w	8(%sp),%d0	# exponent adjustment
	cmp.w	%d0,%d1		# special case overflow
	bhs.w	fmod7
	not.w	%d1		# invert mask
	and.w	%d1,(%sp)	# remove exponent from dividend
	or.w	%d0,(%sp)	# put in correct exponent
	lsr.w	&4,%d0		# normalize exponent
	mov.w	&1075,%d1	# bias + 52 bits (to locate binary pt.)
	sub.w	%d0,%d1		# bits to truncate
	ble.b	fmod4		# is there a fractional part?
	cmp.w	%d1,&52		# less than 1.0?
	bgt.b	fmod7
	movq	&-1,%d0		# bit mask
	cmp.b	%d1,&32		# mask more than 32 bits?
	bge.b	fmod2
	lsl.l	%d1,%d0		# create bit mask
	and.l	%d0,4(%sp)	# mask out bits
	bra.b	fmod3");
    asm("fmod2:
	clr.l	4(%sp)		# lower 32 bits are all 0's
	sub.b	&32,%d1		# number of bits to mask in top
	lsl.l	%d1,%d0		# create bit mask
	and.l	%d0,(%sp)       # mask out bits");
    asm("fmod3:
	mov.l	20(%a6),-(%sp)	# second number
	mov.l	16(%a6),-(%sp)
	jsr	_fmul		# integer * second number
	movm.l	%d0-%d1,(%sp)
	mov.l	12(%a6),-(%sp)	# first number
	mov.l	8(%a6),-(%sp)
	jsr	_fsub		# first number - integer * second number
	bra.b	fmod5");
    asm("fmod4:
	movq	&0,%d0		# answer is 0.0
	movq	&0,%d1");
    asm("fmod5:
	tst.b	8(%a6)		# check sign of first number
	bpl.b	fmod6
	bset	&31,%d0
	bra.b	fmod8");
    asm("fmod6:
	bclr	&31,%d0
	bra.b	fmod8");
    asm("fmod7:
	movm.l	8(%a6),%d0-%d1  # result is first number");
    asm("fmod8:");
}

#pragma OPT_LEVEL 2

#else /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#define modf _modf
#define fmod _fmod
#endif /* _NAMESPACE_CLEAN */

#ifdef _NAMESPACE_CLEAN
#undef fmod
#pragma _HP_SECONDARY_DEF _fmod fmod
#define fmod _fmod
#endif /* _NAMESPACE_CLEAN */

double fmod (a, b) double a, b;
{
	double d,t,modf(),frexp(),ldexp();
	int e1,e2;
	extern int errno;

	if (b == 0.0) {
	   errno = EDOM;
	   return(a);
	}
	t = frexp(a,&e1)/frexp(b,&e2);
	errno = 0;
	t = ldexp(t,e1-e2);
	if (errno) return 0.0;
	modf (t,&d);
	return a - d * b;
}

#endif /* hp9000s200 */
