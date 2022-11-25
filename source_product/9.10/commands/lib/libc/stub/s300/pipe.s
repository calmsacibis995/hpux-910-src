# @(#) $Revision: 66.2 $
# pipe -- C library

#	pipe(f)
#	int f[2];

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__pipe
	sglobal	_pipe',`
	global	_pipe
 ')
	global	__cerror
 ifdef(`_NAMESPACE_CLEAN',`
__pipe:
')
_pipe:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_pipe(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_pipe,%a0
	jsr	mcount
	')
	')
	movq	&SYS_pipe,%d0
	trap	&0
	bcc.b	noerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
noerror:
	move.l	(4,%sp),%a1
	move.l	%d0,(%a1)+
	move.l	%d1,(%a1)
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_pipe:	long	0
	')
