# @(#) $Revision: 70.1 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION SINE					  *
#							  *
# float fsin(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(fsin)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	(%a0),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_sin3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_sin2
	fsin	%fp0			#calculate sin
	fbun	c_sin3
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
c_sin2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(fsin,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_sin3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fsin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# SINGLE PRECISION COSINE				  *
#							  *
# float fcos(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(fcos)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	(%a0),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_cos3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_cos2
	fcos	%fp0			#calculate cos
	fbun	c_cos3
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
c_cos2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(fcos,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_cos3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(fcos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
