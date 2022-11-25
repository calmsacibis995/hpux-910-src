# @(#) $Revision: 70.3 $     
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION ARCTANGENT2				  *
#							  *
# double atan2(arg1,arg2) double arg1,arg2;		  *
#							  *
# Errors: If arg1 = 0 and arg2 = 0, matherr() is called	  *
#		type = DOMAIN and retval = 0.0.		  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/18/85			  *
#							  *
#**********************************************************

	shlib_version   6,1992

	init_code(atan2)

	fmov.l	%fpcr,%d0		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
#
# put arg1 in %fp0 and arg2 in %fp1, if either is a NaN goto atan2_10
#
	lea	(4,%sp),%a0
	lea	(12,%sp),%a1
	fmov.d	(%a0),%fp0
	fbun	atan2_10		# atan2(x,NaN)
	fmov.d	(%a1),%fp1
	fbun	atan2_10		# atan2(x,NaN)
#
# If both parameters are infinity goto atan2_8
#
	fmovm.x	%fp2,-(%sp)
	ftest	%fp0
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	beq	atan2_a1
	ftest	%fp1
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	atan2_8			# atan2(Inf,Inf)
atan2_a1:
#
# Check for zero's
#
	ftest %fp0
	fbne	atan2_b1
	ftest %fp1
	fbne	atan2_b2
	# Both params are zero
	fmovm.x	(%sp)+,%fp2
	fmov.l	%d0,%fpcr		#restore control register
	call_error(atan2,%a0,%a1,DOUBLE_TYPE,ZERO_RET,DOMAIN)	# atan2(0,0)
	rts
atan2_b2:							# atan2(0,x)
	fmov.b	&0,%fp2
	ftest	%fp1
	fbgt	atan2_9
	fmove.x	&CR_PI_X,%fp2
	ftest	%fp0
	fmov.l	%fpsr,%d1
	btst	&27,%d1
	beq	atan2_9
	fneg	%fp2
	bra	atan2_9
atan2_b1:							# atan(x,?)
	ftest	%fp1
	fbne	atan2_b3
	fmove.x	&CR_PI_X,%fp2			# atan(x,0)
	fdiv.b	&2,%fp2
	ftest	%fp0
	fbgt	atan2_9
	fneg	%fp2
	bra	atan2_9
atan2_b3:							# atan(x,x)
#
# If first parameter is infinite, return  +/- PI/2
#
	ftest	%fp0
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	beq	atan2_c3
	fmove.x	&CR_PI_X,%fp2
	fdiv.b	&2,%fp2
	ftest	%fp0
	fbgt	atan2_9
	fneg	%fp2
	bra	atan2_9
atan2_c3:
#
# If second parameter is infinite, return 0 or PI
#
	ftest	%fp1
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	beq	atan2_c4
	fmov.b	&0,%fp2
	ftest	%fp1
	fbgt	atan2_9
	fmove.x	&CR_PI_X,%fp2
	bra	atan2_9
atan2_c4:
#
# Do the real work
#
	fdiv	%fp1,%fp0		#fp1 <- arg1/arg2
	fmov.l	%fpsr,%d1
	btst	&12,%d1
	bne	atan2_3
	btst	&11,%d1
	bne	atan2_4
	fabs	%fp0			#fp1 <- |arg1/arg2|
ifdef(`FP040',`
ifdef(`PIC',`
		mov.l	%a1,-(%sp)
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
		mov.l	(%sp)+,%a1
    ',` tst.w   flag_68040')
    bne.b   atan_040
    fatan.x %fp0
    bra.b   atan_040_done
atan_040:
	movem.l	%a0/%a1/%d0/%d1,-(%sp)
    bsr.l   __fp040_datan
	movem.l	(%sp)+,%a0/%a1/%d0/%d1
atan_040_done:
	',`
	fatan.x	%fp0			#calculate atan
	')
	tst.b	(%a1)
	bpl.b	atan2_1
	fmove.x	&CR_PI_X,%fp1
	fsub	%fp0,%fp1		#fp0 <- PI - result
	fmov	%fp1,%fp0
atan2_1:	
	tst.b	(%a0)
	bpl.b	atan2_2
	fneg	%fp0			#result = -result
atan2_2:	
	fmovm.x	(%sp)+,%fp2
	fmov.d	%fp0,-(%sp)
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
atan2_3:				#overflow
	fmove.x	&CR_PI_X,%fp2
	fdiv.b	&2,%fp2
	ftest	%fp0
	fbgt	atan2_9
	fneg	%fp2
	bra	atan2_9
atan2_4:
	fmov.b	&0,%fp2
	ftest	%fp0
	fbgt	atan2_9
	fmove.x	&CR_PI_X,%fp2
	bra	atan2_9
#
# Handle atan2(Inf,Inf)
#
atan2_8:
	fmove.x	&CR_PI_X,%fp2	# atan2(Inf,Inf)
	fdiv.b	&4,%fp2
	ftest	%fp1
	fblt	atan2_8a
	ftest	%fp0
	fbgt	atan2_9
	fneg	%fp2,%fp2
	bra	atan2_9
atan2_8a:
	fmul.b	&3,%fp2
	ftest	%fp0
	fbgt	atan2_9
	fneg	%fp2,%fp2
atan2_9:
	fmov.d	%fp2,-(%sp)
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	fmovm.x	(%sp)+,%fp2
	rts
#
# Handle Nan
#
atan2_10:
	fmov.l	%d0,%fpcr
	call_error(atan2,%a0,%a1,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
