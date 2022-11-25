# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#
#	DOUBLE PRECISION HYPERBOLIC SINE
#
#	Author: Mark McDowell
#	Date:	December 11, 1984
#	Source: Software Manual for the Elementary Functions
#		by William James Cody, Jr. and William Waite (1980)
#
#	This routine will work only for numbers in IEEE double precision
#	floating point format. Of the numbers in this format, it checks for 
#	and handles only normalized numbers and -0 and +0 -- not infinities, 
#	denormalized numbers, nor NaN's.
#

	init_code(sinh)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	4(%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	sinh4
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	sinh5
	fsinh.x	%fp1,%fp0		#calculate sinh
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	btst	&25,%d1			#Infinity?
	bne.b	sinh2
	btst	&12,%d1			#Overflow?
	bne.b	sinh2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
sinh2:
	pop_double_value()
	tst.b	(%a0)			#check sign of arg
	bpl.b	sinh3
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,OVERFLOW)
	rts
sinh3:
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	rts
sinh4:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
sinh5:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	fmov.d	%fp1,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
