# @(#) $Revision: 66.1 $
#
# n = abs(n);
#
# Returns the labsolute value of n.  labs(0-MAXINT-1) is undefined.
#
# int
# abs(n)
# int n;
# {
#     return n >= 0 ? n : -n;
# }
#
	version	2
ifdef(`_NAMESPACE_CLEAN',`
	global	__abs
	sglobal	_abs
__abs:',`
	global	_abs
')
_abs:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_abs(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_abs,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%d0
	bge.b	L0
	neg.l	%d0
L0:
	rts
ifdef(`PROFILE',`
	data
p_abs:
	long	0
')
