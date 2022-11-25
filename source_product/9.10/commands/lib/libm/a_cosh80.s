# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#
#	DOUBLE PRECISION HYPERBOLIC COSINE
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


	init_code(cosh)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	4(%sp),%a0
	fmov.d	(%a0),%fp1
	fbun	cosh3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	cosh4
	fcosh.x	%fp1,%fp0		#calculate cosh
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	btst	&25,%d1			#infinity?
	bne.b	cosh2
	btst	&12,%d1			#Overflow?
	bne.b	cosh2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	rts
cosh2:	
	pop_double_value()
	call_error(cosh,%a0,%a0,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	rts
cosh3:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(cosh,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
cosh4:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	fmov.d	%fp1,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
