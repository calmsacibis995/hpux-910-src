# @(#) $Revision: 70.3 $
#

include(`a_include.h')

#
#	SINGLE PRECISION HYPERBOLIC COSINE
#
#	Author: Mark McDowell
#	Date:	December 11, 1984
#	Source: Software Manual for the Elementary Functions
#		by William James Cody, Jr. and William Waite (1980)
#
#	This routine will work only for numbers in IEEE single precision
#	floating point format. Of the numbers in this format, it checks for 
#	and handles only normalized numbers and -0 and +0 -- not infinities, 
#	denormalized numbers, nor NaN's.
#

	shlib_version   6,1992

	init_code(fcosh)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0
	fbun	cosh3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	cosh4
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   cosh_040
    fcosh.x %fp0
    bra.b   cosh_040_done
cosh_040:
    bsr.l   	__fp040_cosh
cosh_040_done:
	',`
	fcosh.x	%fp0			#calculate cosh
	')
	fmov.s	%fp0,%d0		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmove.l	(%sp)+,%fpcr	#restore control register
	btst	&25,%d1			#infinity?
	bne.b	cosh2
	btst	&12,%d1			#Overflow?
	bne.b	cosh2
	rts
cosh2:	
	lea		4(%sp),%a0
	call_error(fcosh,%a0,%a0,FLOAT_TYPE,HUGE_RET,OVERFLOW)
	rts
cosh3:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fcosh,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
cosh4:
	fmove.l	(%sp)+,%fpcr
	fmov.s	%fp0,%d0
	rts

