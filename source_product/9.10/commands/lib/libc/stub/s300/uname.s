# @(#) $Revision: 66.1 $
# C library -- uname

include(defines.mh)
	set	UNAME,0
ifdef(`_NAMESPACE_CLEAN',`
	global	__uname
	sglobal	_uname',`
	global	_uname
 ')
	global	__cerror
ifdef(`_NAMESPACE_CLEAN',`
__uname:
')
_uname:
	ifdef(`PROFILE',`
        ifdef(`PIC',`
        mov.l   &DLT,%a0
        lea.l   -6(%pc,%a0.l),%a0
        mov.l   p_uname(%a0),%a0
        bsr.l   mcount',`
        mov.l   &p_uname,%a0
        jsr     mcount
        ')
	')
	move.l	&UNAME,-(%sp)
	move.l	&0,-(%sp)
	move.l	(12,%sp),-(%sp)
	ifdef(`PIC',`
	bsr	sys',`
	jsr	sys
	')
	add.l	&12,%sp
	rts

sys:
	movq	&SYS_utssys,%d0
	trap	&0
	bcc.b	noerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
noerror:
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_uname:	long	0
	')
