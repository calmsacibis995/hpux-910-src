# @(#) $Revision: 70.3 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION ARCTANGENT				  *
#							  *
# double atan(arg) double arg;				  *
#							  *
# Errors: Underflow and inexact errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(atan)
	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0
	fbun	atan2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
		tst.w	(%a1)
	',` tst.w   flag_68040')
    bne.b   atan_040
    fatan.x %fp0
    bra.b   atan_040_done
atan_040:
    bsr.l   __fp040_datan
atan_040_done:
	',`
	fatan.x	%fp0			#calculate atan
	')
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	fmov.l	(%sp)+,%fpcr	#restore control register
	rts
atan2:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(atan,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
