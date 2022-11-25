# @(#) $Revision: 70.3 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION ARCSINE				  *
#							  *
# float fasin(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact and underflow errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(fasin)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0
	fbun	asin3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	asin3
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   do_040
    fasin.x %fp0
    bra.b   do_040_done
do_040:
    bsr.l   __fp040_asin
do_040_done:
	',`
	fasin.x	%fp0			#calculate asin
	')
	fmov.l	%fpsr,%d1		#get status register
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr
	andi.w	&0x2000,%d1		#domain error? (underflow is ignored
	bne.b	asin2			#	with default returned)
	rts
asin2:
	lea		4(%sp),%a0
ifdef(`libM',`
	call_error(fasin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(fasin,%a0,%a0,FLOAT_TYPE,ZERO_RET,DOMAIN)
	')
	rts
asin3:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fasin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCCOSINE				  *
#							  *
# float facos(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(facos)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0
	fbun	acos3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	acos3
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   da_040
    facos.x %fp0
    bra.b   da_040_done
da_040:
    bsr.l   __fp040_acos
da_040_done:
	',`
	facos.x	%fp0			#calculate acos
	')
	fmov.l	%fpsr,%d1		#get status register
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr	#restore control register
	andi.w	&0x2000,%d1		#domain error?
	bne.b	acos2
	rts
acos2:
	lea		4(%sp),%a0
ifdef(`libM',`
	call_error(facos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(facos,%a0,%a0,FLOAT_TYPE,ZERO_RET,DOMAIN)
	')
	rts
acos3:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(facos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
