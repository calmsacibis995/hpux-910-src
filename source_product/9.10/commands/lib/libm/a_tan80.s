# @(#) $Revision: 70.1 $      
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

	init_code(tan)

	lea	(4,%sp),%a0
	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	(%a0),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_tan3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f6.7465188522610178e+009 #total loss of precision?
	fbgt	c_tan2
	ftan	%fp0			#calculate tan
	fbun	c_tan3
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	btst	&25,%d1			#overflow?
	bne.w	c_tan4
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
c_tan2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(tan,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_tan3:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	call_error(tan,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
c_tan4:
	mov.w	(%sp),%d0
	addq.w	&6,%sp
	add.w	%d0,%d0
	mov.l	&0xFFDFFFFE,%d0
	roxr.l	&1,%d0
	movq	&-1,%d1
	rts
