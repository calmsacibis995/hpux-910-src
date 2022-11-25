# @(#) $Revision: 70.3 $      
#

include(`a_include.h')

#**********************************************************
#							  *
# DOUBLE PRECISION NATURAL LOGARITHM			  *
#							  *
# double log(arg) double arg;				  *
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

	shlib_version   6,1992

	init_code(log)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0		#%fp0 <- arg
	fbun	log6
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   log_040
	fcmp.s	%fp0,&0f0.5		#work around bug in 68881
	fblt	log2
	fsub.w	&1,%fp0
	flognp1	%fp0			#calculate log((arg - 1) + 1)
	bra.b	log_040_done
log2:
	flogn	%fp0			#calculate log
    bra.b   log_040_done
log_040:
    bsr.l  	__fp040_dlogn
log_040_done:
	',`
	fcmp.s	%fp0,&0f0.5		#work around bug in 68881
	fblt	log2
	fsub.w	&1,%fp0
	flognp1	%fp0			#calculate log((arg - 1) + 1)
	bra.b	log3
log2:
	flogn	%fp0			#calculate log
log3:
	')
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x2400,%d1		#error?
	bne.b	log4
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr
	rts
log4:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	and.w	&0x2000,%d1		#domain or singularity error?
	bne.b	log5
	call_error(log,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,SING)
	rts
log5:
	ifdef(`libM',`
	call_error(log,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(log,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,DOMAIN)
	')
	rts
log6:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(log,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

#**********************************************************
#							  *
# DOUBLE PRECISION COMMON LOGARITHM			  *
#							  *
# double log10(arg) double arg;				  *
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

	init_code(log10)

	fmove.l	%fpcr,-(%sp)
	movq	&0,%d1
	fmov.l	%d1,%fpcr
	fmov.d	8(%sp),%fp0
	fbun	logt4
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
    bne.b   do_040
    flog10.x %fp0
    bra.b   do_040_done
do_040:
    bsr.l   __fp040_dlog10
do_040_done:
	',`
	flog10.x %fp0			#calculate log
	')
	fmov.l	%fpsr,%d1		#get status register
	andi.w	&0x2400,%d1		#error?
	bne.b	logt2
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	fmove.l	(%sp)+,%fpcr
	rts
logt2:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	and.w	&0x2000,%d1
	bne.b	logt3
	call_error(log10,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,SING)
	rts
logt3:
	ifdef(`libM',`
	call_error(log10,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	',`
	call_error(log10,%a0,%a0,DOUBLE_TYPE,NHUGE_RET,DOMAIN)
	')
	rts
logt4:
	fmove.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(log10,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
