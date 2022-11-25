# @(#) $Revision: 66.2 $
# C library -- setsid, setpgrp, getpgrp

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__setsid
	sglobal _setsid
	global	__setpgrp
	sglobal _setpgrp
	global	__getpgrp
	sglobal _getpgrp',`
	global	_setsid
	global	_setpgrp
	global	_getpgrp
 ')
	global	__cerror
ifdef(`_NAMESPACE_CLEAN',`
__setsid:
')
_setsid:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_setsid(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_setsid,%a0
	jsr	mcount
	')
	')
	movq	&SYS_setpgrp,%d0
	move.l	&2,-(%sp)
	subq.l	&4,%sp
	trap	&0
	bcc.b	noerror
	addq.l	&8,%sp
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
noerror:
	addq.l	&8,%sp
	rts
ifdef(`_NAMESPACE_CLEAN',`
__setpgrp:
')
_setpgrp:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_setpgrp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_setpgrp,%a0
	jsr	mcount
	')
	')
	movq	&SYS_setpgrp,%d0
	move.l	&1,-(%sp)
	subq.l	&4,%sp
	trap	&0
	addq.l	&8,%sp
	rts
ifdef(`_NAMESPACE_CLEAN',`
__getpgrp:
 ')
_getpgrp:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_getpgrp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_getpgrp,%a0
	jsr	mcount
	')
	')
	movq	&SYS_setpgrp,%d0
	clr.l	-(%sp)
	subq.l	&4,%sp
	trap	&0
	addq.l	&8,%sp
	rts

ifdef(`PROFILE',`
		data
p_setsid:	long	0
p_setpgrp:	long	0
p_getpgrp:	long	0
	')
