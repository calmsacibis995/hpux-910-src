# @(#) $Revision: 66.1 $
# ptrace -- C library

#	result = ptrace(req, pid, addr, data);

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__ptrace
	sglobal	_ptrace',`
	global	_ptrace
 ')
	global	__cerror
	global	_errno
ifdef(`_NAMESPACE_CLEAN',`
__ptrace:
 ')
_ptrace:
	ifdef(`PIC',`
	mov.l	&DLT,%a1
	lea.l	-6(%pc,%a1.l),%a1
	')
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	p_ptrace(%a1),%a0
	bsr.l	mcount',`
	mov.l	&p_ptrace,%a0
	jsr	mcount
	')
	')
	movq	&SYS_ptrace,%d0
	ifdef(`PIC',`
	mov.l	_errno(%a1),%a0
	clr.l	(%a0)',`
	clr.l	_errno
	')
	trap	&0
	bcc.b	noerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
noerror:
	rts

ifdef(`PROFILE',`
		data
p_ptrace:	long	0
	')
