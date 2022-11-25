# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION X**Y					  *
#							  *
# double pow(x,y) double x,y;				  *
#							  *
# Errors: matherr() is called with type = DOMAIN and	  *
#		retval = 0.0 if: 1) x = 0.0 and y <= 0.0, *
#		or 2) x < 0.0 and y is not an integer	  *
#	  Overflow will invoke matherr with type = 	  *
#		OVERFLOW and retval = HUGE		  *
#	  Underflow will invoke matherr with type = 	  *
#		UNDERFLOW and retval = 0.0		  *
#	  Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/10/85			  *
#							  *
#**********************************************************

	init_code(pow)

	link	%a6,&-10
	lea	(8,%a6),%a0
	lea	(16,%a6),%a1
	clr.w	-2(%a6)			#assume positive result
	fmov.l	%fpcr,%d0		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr		#ext precision
	fmov.d	(%a1),%fp1		#%fp1 <- y
	fbun.l	c_pow6
	fmov.d	(%a0),%fp0		#%fp0 <- x
	fbun.l	c_pow6

	ftest	%fp1			#pow(x,0) = 1
ifdef(`libM',`
	fbeq	c_pow1
	',`
	fbneq	c_pow0
	ftest	%fp0
	fbeq	c_pow5
c_pow0:
	')

	fcmp.x	%fp0,&0f1.0		#pow(1,x) = 1
	fbeq	c_pow1

	ftest	%fp0
	fbneq	c_pow2
	ftest	%fp1
	fbngt	c_pow5			#0.0**y with y < 0.0
	fmov.l	%d0,%fpcr		#restore control register
	movq	&0,%d0			#0.0**y = 0.0
	movq	&0,%d1
	unlk	%a6
	rts	
c_pow1:
	fmov.l	%d0,%fpcr
	mov.l	&0x3FF00000,%d0		#0.0**0.0 = 1.0
	movq	&0,%d1
	unlk	%a6
	rts
c_pow2:
	fbnlt	c_pow3			#is x < 0.0?
	fmovm.x %fp3,-(%sp)
	fint	%fp1,%fp3
	fcmp	%fp3,%fp1
	fbne	c_pow4a			#x**y with x < 0.0 and y not an integer
	ftest	%fp1
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	c_pow4a
	fmovm.x %fp2,-(%sp)
	fscale.w &-1,%fp3
	fint	%fp3,%fp2
	fcmp	%fp3,%fp2
	fsne	-2(%a6)			#set if y is odd
	fabs	%fp0
	fmovm.x (%sp)+,%fp2-%fp3
c_pow3:	
	flog10	%fp0			#log10(x)
	fmul	%fp1,%fp0		#y * log10(x)
	ftentox	%fp0			#10 ** (y * log10(x))
	fmov.l	%fpsr,%d1
	andi.w	&0x1800,%d1		#overflow or underflow?
	bne.w	c_pow7
	tst.w	-2(%a6)			#change sign?
	beq.b	c_pow4
	fneg	%fp0
c_pow4:
	fmov.d	%fp0,(%sp)		#get result
	fmov.l	%fpsr,%d1
	andi.w	&0x1800,%d1		#overflow or underflow?
	bne.w	c_pow7
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	unlk 	%a6
	rts
c_pow4a:
	fmovm.x	(%sp)+,%fp3
	fmov.l	%d0,%fpcr		#restore control register
ifdef(`libM',`
	call_error(pow,%a0,%a1,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(pow,%a0,%a1,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	unlk	%a6
	rts
c_pow5:
	fmov.l	%d0,%fpcr		#restore control register
ifdef(`libM',`
	call_error(pow,%a0,%a1,DOUBLE_TYPE,NHUGE_RET,DOMAIN)
	',`
	call_error(pow,%a0,%a1,DOUBLE_TYPE,ZERO_RET,DOMAIN)
	')
	unlk	%a6
	rts
c_pow6:
	fmov.l	%d0,%fpcr
	call_error(pow,%a0,%a1,DOUBLE_TYPE,NAN_RET,DOMAIN)
	unlk	%a6
	rts
c_pow7:
	fmov.l	%d0,%fpcr		#restore control register
	and.w	&0x1000,%d1		#overflow or underflow?
	bne.b	c_pow8			#jump if overflow
	call_error(pow,%a0,%a1,DOUBLE_TYPE,ZERO_RET,UNDERFLOW)
	unlk	%a6
	rts
c_pow8:
	call_error(pow,%a0,%a1,DOUBLE_TYPE,HUGE_RET,OVERFLOW)
	unlk	%a6
	rts
