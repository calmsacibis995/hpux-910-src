
# HPUX_ID: @(#) $Revision: 66.4 $
ifdef(`_NAMESPACE_CLEAN',`
	global	__is_68010_present
	sglobal	_is_68010_present',`
	global	_is_68010_present
')
ifdef(`_NAMESPACE_CLEAN',`
__is_68010_present:')
_is_68010_present:
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_68010(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68010(%a1),%a0
	tst.w	(%a0)',`
	ifdef(`PROFILE',`
	mov.l	&p_68010,%a0
	jsr	mcount
	')
	tst.w	flag_68010
')
	beq.b	L1
	movq	&1,%d0
	rts
L1:
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_68010:	long	0
	')
