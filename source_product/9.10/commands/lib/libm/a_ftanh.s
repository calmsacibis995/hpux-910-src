# @(#) $Revision: 70.3 $
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

	shlib_version   6,1992

	init_code(ftanh)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0
	fbun	tanh2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   tanh_040
    ftanh.x  %fp0
    bra.b   tanh_040_done
tanh_040:
    bsr.l   __fp040_tanh
tanh_040_done:
	',`
	ftanh.x	%fp0			#calculate tanh
	')
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr
	rts
tanh2:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(ftanh,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts


