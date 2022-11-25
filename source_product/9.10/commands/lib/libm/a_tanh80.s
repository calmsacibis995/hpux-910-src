# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#
#	DOUBLE PRECISION HYPERBOLIC TANGENT
#
#	Author: Mark McDowell
#	Date:	December 10, 1984
#	Source: Software Manual for the Elementary Functions
#		by William James Cody, Jr. and William Waite (1980)
#
#	This routine will work only for numbers in IEEE double precision
#	floating point format. Of the numbers in this format, it checks for 
#	and handles only normalized numbers and -0 and +0 -- not infinities, 
#	denormalized numbers, nor NaN's.
#

	init_code(tanh)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	4(%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	tanh2
	ftanh.x	%fp1,%fp0		#calculate tanh
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	mov.l	(%sp)+,%d0		#get result
	mov.l	(%sp)+,%d1
	rts
tanh2:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(tanh,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
