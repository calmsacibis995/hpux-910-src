# @(#) $Revision: 70.3 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION ARCSINE				  *
#							  *
# double asin(arg) double arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact and underflow errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(asin)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0
	fbun	asin3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	asin3
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
		tst.w	(%a1)
	',` tst.w   flag_68040')
    bne.b   asin_040
    fasin.x %fp0
    bra.b   asin_040_done
asin_040:
    bsr.l   __fp040_dasin
asin_040_done:
	',`
	fasin.x	%fp0			#calculate asin
	')
	fmov.l	%fpsr,%d1		#get status register
	fmov.d	%fp0,-(%sp)		#save result
	andi.w	&0x2000,%d1		#domain error? (underflow is ignored
	bne.b	asin2			#	with default returned)
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmov.l	(%sp)+,%fpcr	#restore control register
	rts
asin2:
	pop_double_value()
	fmov.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
ifdef(`libM',`
	call_error(asin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(asin,%a0,%a0,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	rts
asin3:
	fmov.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(asin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# DOUBLE PRECISION ARCCOSINE				  *
#							  *
# double acos(arg) double arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(acos)

	fmov.l	%fpcr,-(%sp)		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0
	fbun	acos3
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	acos3
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
		tst.w	(%a1)
	',` tst.w   flag_68040')
    bne.b   acos_040
    facos.x %fp0
    bra.b   acos_040_done
acos_040:
    bsr.l   __fp040_dacos
acos_040_done:
	',`
	facos.x	%fp0			#calculate acos
	')
	fmov.l	%fpsr,%d1		#get status register
	fmov.d	%fp0,-(%sp)		#save result
	andi.w	&0x2000,%d1		#domain error?
	bne.b	acos2
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmov.l	(%sp)+,%fpcr	#restore control register
	rts
acos2:
	pop_double_value()
	fmov.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
ifdef(`libM',`
	call_error(acos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(acos,%a0,%a0,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	rts
acos3:
	fmov.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(acos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
