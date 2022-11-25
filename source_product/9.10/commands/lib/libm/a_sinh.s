# @(#) $Revision: 70.3 $
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

	shlib_version   6,1992

	init_code(sinh)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0
	fbun	sinh4
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	sinh5
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   sinh_040
    fsinh.x %fp0
    bra.b   sinh_040_done
sinh_040:
    bsr.l   __fp040_dsinh
sinh_040_done:
	',`
	fsinh.x	%fp0			#calculate sin
	')
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	btst	&25,%d1			#Infinity?
	bne.b	sinh2
	btst	&12,%d1			#Overflow?
	bne.b	sinh2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr
	rts
sinh2:
	pop_double_value()
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	tst.b	(%a0)			#check sign of arg
	bpl.b	sinh3
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,OVERFLOW)
	rts
sinh3:
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	rts
sinh4:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(sinh,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
sinh5:
	fmove.l	(%sp)+,%fpcr
	fmov.d	%fp0,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

