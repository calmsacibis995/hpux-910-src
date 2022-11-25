# @(#) $Revision: 70.2 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION SQUARE ROOT				  *
#							  *
# float fsqrt(arg) float arg;				  *
#							  *
# Errors: If arg < 0, matherr() is called	 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(fsqrt)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fsqrt.s	(%a0),%fp0		#calculate sqrt
	fbun	sqrt3
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x2000,%d1		#sqrt(neg number)?
	bne.b	sqrt2
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
sqrt2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(fsqrt,%a0,%a0,FLOAT_TYPE,ZERO_RET,DOMAIN)
	rts
sqrt3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fsqrt,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
