# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION EXP					  *
#							  *
# double exp(arg) double arg;				  *
#							  *
# Errors: Overflow calls matherr() with type = OVERFLOW	  *
#		and retval = HUGE			  *
#         Underflow calls matherr() with type = UNDERFLOW *
#		and retval = 0.0			  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(exp)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.l	%d1,%fpsr
	fmove.d	8(%sp),%fp0
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   do_040
    fetox.x %fp0
    bra.b   do_040_done
do_040:
    bsr.l   __fp040_dexp
do_040_done:
	',`
	fetox.x	%fp0		#calculate exp
	')
	fbun	exp4
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x60,%d1		#error?
	bne.b	exp2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr
	rts
exp2:
	pop_double_value()
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	andi.w	&0x20,%d1		#underflow error?
	bne.b	exp3
	call_error(exp,%a0,%a0,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	rts
exp3:
	call_error(exp,%a0,%a0,DOUBLE_TYPE,ZERO_RET,UNDERFLOW)
	rts
exp4:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(exp,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
