# @(#) $Revision: 66.1 $
#
# n = labs(n);
#
# Returns the labsolute value of n.  labs(0-MAXLONG-1) is undefined.
#
# int
# labs(n)
# int n;
# {
#     return n >= 0 ? n : -n;
# }
#
	version	2
ifdef(`_NAMESPACE_CLEAN',`
	global	__labs
	sglobal	_labs
__labs:',`
	global	_labs
')
_labs:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_labs(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_labs,%a0
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
p_labs:
	long	0
')
