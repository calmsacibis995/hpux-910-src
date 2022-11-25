# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION SINE					  *
#							  *
# float fsin(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(fsin)

	fmove.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_sin3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_sin2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   sin_040
    fsin.x 	%fp0
    bra.b   sin_040_done
sin_040:
    bsr.l   __fp040_sin
sin_040_done:
	',`
	fsin.x	%fp0			#calculate sin
	')
	fbun	c_sin3
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr	#restore control register
	rts
c_sin2:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(fsin,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_sin3:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(fsin,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# SINGLE PRECISION COSINE				  *
#							  *
# float fcos(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(fcos)

	fmove.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.b	c_cos3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f1.3493037704522036e+010 #total loss of precision?
	fbgt	c_cos2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   cos_040
    fcos.x 	%fp0
    bra.b   cos_040_done
cos_040:
    bsr.l   __fp040_cos
cos_040_done:
	',`
	fcos.x	%fp0			#calculate cos
	')
	fbun	c_cos3
	fmov.s	%fp0,%d0		#save result
	fmove.l	(%sp)+,%fpcr
	rts
c_cos2:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fcos,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_cos3:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fcos,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
