# @(#) $Revision: 70.1 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION EXP					  *
#							  *
# float fexp(arg) float arg;				  *
#							  *
# Errors: Overflow calls matherr() with type = OVERFLOW	  *
#		and retval = HUGE			  *
#         Underflow calls matherr() with type = UNDERFLOW *
#		and retval = 0.0			  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(fexp)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.l	%d1,%fpsr
	lea	(4,%sp),%a0
	fetox.s	(%a0),%fp0		#calculate exp
	fbun	exp4
	fmov.s	%fp0,%d0		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x60,%d1		#error?
	bne.b	exp2
	rts
exp2:
	andi.w	&0x20,%d1		#underflow error?
	bne.b	exp3
	call_error(fexp,%a0,%a0,FLOAT_TYPE,HUGE_RET,OVERFLOW)
	rts
exp3:
	call_error(fexp,%a0,%a0,FLOAT_TYPE,ZERO_RET,UNDERFLOW)
	rts
exp4:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fexp,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
