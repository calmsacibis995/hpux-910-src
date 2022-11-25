# @(#) $Revision: 70.1 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION ARCTANGENT				  *
#							  *
# double atan(arg) double arg;				  *
#							  *
# Errors: Underflow and inexact errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(atan)
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	atan2
	fatan.x	%fp1,%fp0		#calculate atan
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
atan2:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(atan,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
