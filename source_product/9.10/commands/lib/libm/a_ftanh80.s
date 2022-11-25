# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#
#	SINGLE PRECISION HYPERBOLIC TANGENT
#
#	Author: Mark McDowell
#	Date:	December 10, 1984
#	Source: Software Manual for the Elementary Functions
#		by William James Cody, Jr. and William Waite (1980)
#
#	This routine will work only for numbers in IEEE single precision
#	floating point format. Of the numbers in this format, it checks for 
#	and handles only normalized numbers and -0 and +0 -- not infinities, 
#	denormalized numbers, nor NaN's.
#

	init_code(ftanh)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	4(%sp),%a0
	fmov.s	(%a0),%fp1
	fbun	tanh2
	ftanh.x	%fp1,%fp0		#calculate tanh
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
tanh2:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(ftanh,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
