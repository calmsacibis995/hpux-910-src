# @(#) $Revision: 70.3 $
#

include(`a_include.h')

#
#  floor and ceil-- greatest integer <= arg
#  (resp least >=)
#

	shlib_version   6,1992

	init_code(floor)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0x20,%d1
	fmov.l	%d1,%fpcr
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
	bne.b	floor_040
	fint.d	8(%sp),%fp0	
	bra.b	floor_040_done
floor_040:
	fmove.d	8(%sp),%fp0
	bsr.l	__fp040_dint
floor_040_done:
	',`
	fint.d	8(%sp),%fp0
	')
	fbun	floor2
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	fmov.l	(%sp)+,%fpcr
	rts
floor2:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(floor,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

	init_code(ceil)

	fmov.l	%fpcr,-(%sp)	#save control register
	movq	&0x30,%d1
	fmov.l	%d1,%fpcr
ifdef(`FP040',`
ifdef(`PIC',`
        mov.l  &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
        move.l  flag_68040(%a1),%a1
        tst.w   (%a1)
    ',` tst.w   flag_68040')
	bne.b	ceil_040
	fint.d	8(%sp),%fp0	
	bra.b	ceil_040_done
ceil_040:
	fmove.d	8(%sp),%fp0
	bsr.l	__fp040_dint
ceil_040_done:
	',`
	fint.d	8(%sp),%fp0
	')
	fbun	ceil2
	fmov.d	%fp0,-(%sp)		#save result
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	fmov.l	(%sp)+,%fpcr
	rts
ceil2:
	fmov.l	(%sp)+,%fpcr
	lea		4(%sp),%a0
	call_error(ceil,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
