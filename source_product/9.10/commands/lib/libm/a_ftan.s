# @(#) $Revision: 70.4 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION TANGENT				  *
#							  *
# float ftan(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI/2*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors, underflow errors, and quiet	  *
#		NaN's as inputs are ignored.		  *
#	  Overflow errors return HUGE.			  *
#							  *
# Author: Mark McDowell  6/7/85				  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(ftan)

	fmov.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.s	8(%sp),%fp0		#%fp0 <- arg
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne.w	c_tan3
	fabs	%fp0,%fp1
	fcmp.d	%fp1,&0f6.7465188522610178e+009 #total loss of precision?
	fbgt	c_tan2
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   tan_040
    ftan.x  %fp0
    bra.b   tan_040_done
tan_040:
    bsr.l   __fp040_tan
tan_040_done:
	',`
	ftan.x	%fp0			#calculate sin
	')
	fbun	c_tan3
	fmov.s	%fp0,%d0		#save result
	fmov.l	%fpsr,%d1		#get status register
	fmov.l	(%sp)+,%fpcr
	btst	&25,%d1			#overflow?
	bne.w	c_tan4
	rts
c_tan2:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(ftan,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_tan3:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(ftan,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
c_tan4:
	add.l	%d0,%d0
	mov.l	&0xFEFFFFFE,%d0
	roxr.l	&1,%d0
	rts

#**********************************************************
#							  *
# SINGLE PRECISION COTANGENT				  *
#							  *
# float fcot(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI/2*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#	  If arg = 0.0, matherr() is called with type =   *
#		SING and retval = MAXFLOAT.		  *
#         Inexact errors and underflows are ignored.	  *
#							  *
# Author: Mark McDowell  6/26/85			  *
#							  *
#**********************************************************

	init_code(fcot)

	fmov.l	%fpcr,-(%sp)
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.s	8(%sp),%fp0		#fp0 <- arg
	fbeq	c_cot2
	fabs	%fp0,%fp1
	fcmp.s	%fp1,&0f6.746519E9	#total loss of precision?
	fbgt	c_cot3
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   ctan_040
    ftan.x %fp0
    bra.b   ctan_040_done
ctan_040:
    bsr.l   __fp040_tan
ctan_040_done:
	',`
	ftan.x	%fp0			#calculate sin
	')
	fmov.w	&1,%fp1
	fdiv	%fp0,%fp1
	fmov.s	%fp1,%d0		#save result
	fmov.l	%fpsr,%d1
	fmov.l	(%sp)+,%fpcr
	btst	&25,%d1			#overflow?
	bne.w	c_cot8
	rts
c_cot2:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fcot,%a0,%a0,FLOAT_TYPE,MAXFLOAT_RET,SING)
	rts
c_cot3:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(fcot,%a0,%a0,FLOAT_TYPE,ZERO_RET,TLOSS)
	rts
c_cot8:
	add.l	%d0,%d0
	mov.l	&0xFEFFFFFE,%d0
	roxr.l	&1,%d0
	rts
