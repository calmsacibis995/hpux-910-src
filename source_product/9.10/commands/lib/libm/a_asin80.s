# @(#) $Revision: 70.1 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION ARCSINE				  *
#							  *
# double asin(arg) double arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact and underflow errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(asin)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	asin3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	asin3
	fasin.x	%fp1,%fp0		#calculate asin
	fmov.l	%fpsr,%d1		#get status register
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x2000,%d1		#domain error? (underflow is ignored
	bne.b	asin2			#	with default returned)
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
asin2:
	pop_double_value()
ifdef(`libM',`
	call_error(asin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(asin,%a0,%a0,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	rts
asin3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(asin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# DOUBLE PRECISION ARCCOSINE				  *
#							  *
# double acos(arg) double arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(acos)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	acos3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	acos3
	facos.x	%fp1,%fp0		#calculate acos
	fmov.l	%fpsr,%d1		#get status register
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	andi.w	&0x2000,%d1		#domain error?
	bne.b	acos2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
acos2:
	pop_double_value()
ifdef(`libM',`
	call_error(acos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(acos,%a0,%a0,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	rts
acos3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(acos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
