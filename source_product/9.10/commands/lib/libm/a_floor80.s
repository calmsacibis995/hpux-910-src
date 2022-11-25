# @(#) $Revision: 70.1 $
#

include(`a_include.h')

#
#  floor and ceil-- greatest integer <= arg
#  (resp least >=)
#

	init_code(floor)

	fmov.l	%fpcr,%d0		#save control register
	movq	&0x20,%d1
	fmov.l	%d1,%fpcr
	lea	4(%sp),%a0
	fint.d	(%a0),%fp0		#fp0 <- int(arg)
	fbun	floor2
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
floor2:
	fmov.l	%d0,%fpcr
	call_error(floor,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts

	init_code(ceil)

	fmov.l	%fpcr,%d0		#save control register
	movq	&0x30,%d1
	fmov.l	%d1,%fpcr
	fint.d	4(%sp),%fp0		#fp0 <- int(arg)
	fbun	ceil2
	fmov.d	%fp0,-(%sp)		#save result
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts
ceil2:
	fmov.l	%d0,%fpcr
	lea	4(%sp),%a0
	call_error(ceil,%a0,%a0,DOUBLE_TYPE,NAN_RET,DOMAIN)
	rts
