# @(#) $Revision: 70.1 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION ARCTANGENT				  *
#							  *
# float fatan(arg) float arg;				  *
#							  *
# Errors: Underflow and inexact errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(fatan)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.s	(%a0),%fp1
	fbun	atan2
	fatan.x	%fp1,%fp0		#calculate atan
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
atan2:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fatan,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
