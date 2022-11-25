# @(#) $Revision: 66.6 $
#
# C return sequence which sets errno, returns -1.
#
	global	__cerror
	global	_errno

__cerror:
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	_errno(%a0),%a0
	mov.l	%d0,(%a0)',`
	mov.l	%d0,_errno
')
	movq.l	&-1,%d0
	rts

	bss
	lalign	4
_errno: space	4*(1)
