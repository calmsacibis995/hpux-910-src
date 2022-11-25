# @(#) $Revision: 70.1 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION EXP					  *
#							  *
# double exp(arg) double arg;				  *
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

	init_code(exp)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.l	%d1,%fpsr
	lea	(4,%sp),%a0
	fetox.d	(%a0),%fp0		#calculate exp
	fbun	exp4
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x60,%d1		#error?
	bne.b	exp2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
exp2:
	pop_double_value()
	andi.w	&0x20,%d1		#underflow error?
	bne.b	exp3
	call_error(exp,%a0,%a0,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	rts
exp3:
	call_error(exp,%a0,%a0,DOUBLE_TYPE,ZERO_RET,UNDERFLOW)
	rts
exp4:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(exp,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
