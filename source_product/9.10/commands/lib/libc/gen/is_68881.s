
# HPUX_ID: @(#) $Revision: 66.3 $
ifdef(`_NAMESPACE_CLEAN',`
	global	__is_68881_present
	sglobal	_is_68881_present',`
	global	_is_68881_present
')
ifdef(`_NAMESPACE_CLEAN',`
__is_68881_present:')
_is_68881_present:
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_68881(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)',`
	ifdef(`PROFILE',`
	mov.l	&p_68881,%a0
	jsr	mcount
	')
	tst.w	flag_68881
')
	beq.b	L1
	movq	&1,%d0
	rts
L1:
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_68881:	long	0
	')
