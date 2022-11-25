# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION SINE					  *
#							  *
# double sin(arg) double arg;				  *
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

	init_code(sin)

	fmove.l	%fpcr,-(%sp)	#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0		#%fp0 <- arg
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
    bsr.l   __fp040_dsin
sin_040_done:
	',`
	fsin.x	%fp0			#calculate sin
	')
	fbun	c_sin3
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr	#restore control register
	rts
c_sin2:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(sin,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_sin3:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(sin,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# DOUBLE PRECISION COSINE				  *
#							  *
# double cos(arg) double arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	init_code(cos)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0		#%fp0 <- arg
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
    bsr.l   __fp040_dcos
cos_040_done:
	',`
	fcos.x	%fp0			#calculate cos
	')
	fbun	c_cos3
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr	#restore control register
	rts
c_cos2:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(cos,%a0,%a0,DOUBLE_TYPE,ZERO_RET,TLOSS)
	rts
c_cos3:
	fmove.l	(%sp)+,%fpcr	#restore control register
	lea		4(%sp),%a0
	call_error(cos,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
