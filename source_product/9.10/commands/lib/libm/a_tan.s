# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION TANGENT				  *
#							  *
# double tan(arg) double arg;				  *
#							  *
# Errors: If |arg| > PI/2*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors, underflow errors, and quiet	  *
#		NaN's as inputs are ignored.		  *
#	  Overflow errors return HUGE.			  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(tan)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.w	c_tan3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f6.7465188522610178e+009 #total loss of precision?
	fbgt	c_tan2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   tan_040
    ftan.x  %fp0
    bra.b   tan_040_done
tan_040:
    bsr.l   __fp040_dtan
tan_040_done:
	',`
	ftan.x	%fp0			#calculate sin
	')
	fbun	c_tan3
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	btst	&25,%d1			#overflow?
	bne.w	c_tan4
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr
	rts
c_tan2:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(tan,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_tan3:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(tan,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
c_tan4:
	mov.w	(%sp),%d0
	addq.w	&8,%sp
	fmove.l	(%sp)+,%fpcr
	add.w	%d0,%d0
	mov.l	&0xFFDFFFFE,%d0
	roxr.l	&1,%d0
	movq	&-1,%d1
	rts
