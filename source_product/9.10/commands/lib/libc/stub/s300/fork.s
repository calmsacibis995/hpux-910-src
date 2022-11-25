# @(#) $Revision: 66.1 $
# C library -- fork

# pid = fork();

# d1 == 0 in parent process, d1 == 1 in child process.
# d0 == pid of child in parent, d0 == pid of parent in child.

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__fork
	sglobal	_fork',`
	global	_fork
 ')
	global	__cerror

ifdef(`_NAMESPACE_CLEAN',`
__fork:
')
_fork:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fork(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fork,%a0
	jsr	mcount
	')
	')
	movq	&SYS_fork,%d0
	trap	&0
	bcc.b	forkok
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
forkok:
	tst.l	%d1
	beq.b	parent
	movq	&0,%d0
parent:
	rts

ifdef(`PROFILE',`
		data
p_fork:	long	0
	')
