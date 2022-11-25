# @(#) $Revision: 70.1 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION ARCSINE				  *
#							  *
# float fasin(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact and underflow errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(fasin)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.s	(%a0),%fp1
	fbun	asin3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	asin3
	fasin.x	%fp1,%fp0		#calculate asin
	fmov.l	%fpsr,%d1		#get status register
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x2000,%d1		#domain error? (underflow is ignored
	bne.b	asin2			#	with default returned)
	rts
asin2:
ifdef(`libM',`
	call_error(fasin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(fasin,%a0,%a0,FLOAT_TYPE,ZERO_RET,DOMAIN)
	')
	rts
asin3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fasin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCCOSINE				  *
#							  *
# float facos(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(facos)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.s	(%a0),%fp1
	fbun	acos3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	acos3
	facos.x	%fp1,%fp0		#calculate acos
	fmov.l	%fpsr,%d1		#get status register
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x2000,%d1		#domain error?
	bne.b	acos2
	rts
acos2:
ifdef(`libM',`
	call_error(facos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(facos,%a0,%a0,FLOAT_TYPE,ZERO_RET,DOMAIN)
	')
	rts
acos3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(facos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
