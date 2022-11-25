# @(#) $Revision: 70.1 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# SINGLE PRECISION NATURAL LOGARITHM			  *
#							  *
# float flog(arg) float arg;				  *
#							  *
# Errors: If arg is 0.0, matherr() is called with type =  *
#		SING and retval = -HUGE			  *
#	  If arg < 0.0, matherr() is called with type =	  *
#		DOMAIN and retval = -HUGE		  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/10/85			  *
#							  *
#**********************************************************

	init_code(flog)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.s	(%a0),%fp0		#%fp0 <- arg
	fbun	log6
	fcmp.s	%fp0,&0f0.5		#work around bug in 68881
	fblt	log2
	fsub.w	&1,%fp0
	flognp1	%fp0			#calculate log((arg - 1) + 1)
	bra.b	log3
log2:
	flogn	%fp0			#calculate log
log3:
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x2400,%d1		#error?
	bne.b	log4
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
log4:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	and.w	&0x2000,%d1		#domain or singularity error?
	bne.b	log5
	call_error(flog,%a0,%a0,FLOAT_TYPE,NHUGE_RET,SING)
	rts
log5:
	ifdef(`libM',`
	call_error(flog,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(flog,%a0,%a0,FLOAT_TYPE,NHUGE_RET,DOMAIN)
	')
	rts
log6:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(flog,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# SINGLE PRECISION COMMON LOGARITHM			  *
#							  *
# float flog10(arg) float arg;				  *
#							  *
# Errors: If arg is 0.0, matherr() is called with type =  *
#		SING and retval = -HUGE			  *
#	  If arg < 0.0, matherr() is called with type =	  *
#		DOMAIN and retval = -HUGE		  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/10/85			  *
#							  *
#**********************************************************

	init_code(flog10)

	mov.l	%d2,%a1
	fmov.l	%fpcr,%d2		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	lea	(4,%sp),%a0
	fmov.s	(%a0),%fp1
	fbun	logt4
	flog10.x %fp1,%fp0		#calculate log
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x2400,%d1		#error?
	bne.b	logt2
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	rts
logt2:
	fmov.l	%d2,%fpcr		#restore control register
	mov.l	%a1,%d2
	and.w	&0x2000,%d1
	bne.b	logt3
	call_error(flog10,%a0,%a0,FLOAT_TYPE,NHUGE_RET,SING)
	rts
logt3:
	ifdef(`libM',`
	call_error(flog10,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(flog10,%a0,%a0,FLOAT_TYPE,NHUGE_RET,DOMAIN)
	')
	rts
logt4:
	fmov.l	%d2,%fpcr
	mov.l	%a1,%d2
	call_error(flog10,%a0,%a0,FLOAT_TYPE,NAN_RET,DOMAIN)
	rts
