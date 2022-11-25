
# HPUX_ID: @(#) $Revision: 66.3 $
ifdef(`_NAMESPACE_CLEAN',`
	global	__is_98635A_present
	sglobal	_is_98635A_present',`
	global	_is_98635A_present ')
ifdef(`_NAMESPACE_CLEAN',`
__is_98635A_present:')
_is_98635A_present:
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_98635A(%a1),%a0
	bsr.l	mcount
	')
	mov.l	float_soft(%a1),%a0
	tst.w	(%a0)',`
	ifdef(`PROFILE',`
	mov.l	&p_98635A,%a0
	jsr	mcount
	')
	tst.w	float_soft
')
	beq.b	L1
	movq	&0,%d0
	rts
L1:
	movq	&1,%d0
	rts

ifdef(`PROFILE',`
		data
p_98635A:	long	0
	')
