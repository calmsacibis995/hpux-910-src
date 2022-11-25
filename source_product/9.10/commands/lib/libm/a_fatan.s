# @(#) $Revision: 70.3 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION ARCTANGENT				  *
#							  *
# float fatan(arg) float arg;				  *
#							  *
# Errors: Underflow and inexact errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(fatan)

	fmove.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0
	fbun	atan2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   do_040
    fatan.x %fp0
    bra.b   do_040_done
do_040:
    bsr.l   __fp040_atan
do_040_done:
	',`
	fatan.x	%fp0			#calculate asin
	')
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr
	rts
atan2:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fatan,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
