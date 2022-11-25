# @(#) $Revision: 70.1 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION SINE					  *
#							  *
# double sin(arg) double arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(sin)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	(%a0),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_sin3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_sin2
	fsin	%fp0			#calculate sin
	fbun	c_sin3
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
c_sin2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(sin,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_sin3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(sin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# DOUBLE PRECISION COSINE				  *
#							  *
# double cos(arg) double arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(cos)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	(%a0),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_cos3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_cos2
	fcos	%fp0			#calculate cos
	fbun	c_cos3
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
c_cos2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(cos,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_cos3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(cos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
