# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION FMOD					  *
#							  *
# double fmod(arg1,arg2) double arg1,arg2;		  *
#							  *
# Errors: If (arg2==0) {errno = EDOM; return(arg1);}	  *
#							  *
# Author: Mark McDowell  6/6/85				  *
#							  *
#**********************************************************

	init_code(fmod)

	link	%a6,&0
	ftest.d	8(%a6)
	fbun.l	fmod5
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	fmod5
	ftest.d	16(%a6)
	fbun.l	fmod5
	fbeq.l	fmod9
	fmov.l	%fpsr,%d1
	btst	&25,%d1
	bne	fmod3
	ftest.d	8(%a6)
	fbeq.l	fmod4
        mov.w	&0x7FF0,%d0	# exponent mask
	mov.w	%d0,%d1		# save mask
	and.w	8(%a6),%d0	# isolate first exponent
	beq.w	fmod7		# special case zeroes and denorms
	cmp.w	%d0,%d1		# special case infinities and NaN's
	beq.w	fmod7
	mov.w	%d0,-(%sp)	# save exponent
	mov.w	%d1,%d0		# mask
	and.w	16(%a6),%d0	# isolate second exponent
	beq.w	fmod7		# special case zeroes and denorms
	cmp.w	%d0,%d1		# special case infinities and NaN's
	beq.w	fmod7
	fmov.l	%fpcr,%d0	# save control register
	fmov.l	&0x80,%fpcr	# double precision/no traps
	fmov.d	8(%a6),%fp0	# first argument
	fmov	%fp0,%fp1	# save argument
	fdiv.d	16(%a6),%fp0	# need to check for division overflow
	fmov.l	%fpsr,%d1
	fmov.l	%d0,%fpcr	# restore control register
	btst	&12,%d1		# overflow?
	ifdef(`libM',`
	bne.w	fmod4
	',`
	bne.w	fmod7
	')
	fmod.d	16(%a6),%fp1	# fmod function
	fmov.d	%fp1,-(%sp)
	movm.l	(%sp),%d0-%d1	# result
	unlk	%a6
	rts
fmod3:
	movm.l	8(%a6),%d0-%d1  # result is first number
	unlk	%a6
	rts
fmod4:
	movq	&0,%d0		# answer is 0.0
	movq	&0,%d1
	tst.b	4(%sp)		# check sign of first number
	bpl.b	fmod6
	bset	&31,%d0
	unlk	%a6
	rts
fmod5:
	lea	8(%a6),%a0
	lea	16(%a6),%a1
	call_error(fmod,%a0,%a1,DOUBLE_TYPE,NAN_RET,DOMAIN)
	unlk	%a6
	rts
fmod6:
	bclr	&31,%d0
	unlk	%a6
	rts
fmod7:
	movm.l	8(%a6),%d0-%d1  # result is first number
	lea	8(%a6),%a0
	lea	16(%a6),%a1
	call_error(fmod,%a0,%a1,DOUBLE_TYPE,OP1_RET,DOMAIN)
	unlk	%a6
	rts
fmod9:
	lea	8(%a6),%a0
	lea	16(%a6),%a1
ifdef(`libM',`
	call_error(fmod,%a0,%a1,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(fmod,%a0,%a1,DOUBLE_TYPE,OP1_RET,DOMAIN)
	')
	unlk	%a6
	rts
