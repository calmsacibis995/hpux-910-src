# @(#) $Revision: 66.2 $
#
# C library -- strcpy

# strcpy(s1, s2) copy s2 to s1 -- return s1 in d0

ifdef(`_NAMESPACE_CLEAN',`
	global	__strcpy
	sglobal _strcpy',`
	global	_strcpy
	')

ifdef(`_NAMESPACE_CLEAN',`
__strcpy:
	')
_strcpy:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strcpy(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strcpy,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a0		# get dest   (s1)
	mov.l	8(%sp),%a1		# get source (s2)
	mov.l	%a0,%d0			# return s1 in d0
L1:
	mov.b	(%a1)+,(%a0)+		# move until null byte
	bne.b	L1
	rts
	data
ifdef(`PROFILE',`
		data
p_strcpy:	long	0
')
