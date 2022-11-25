# @(#) $Revision: 66.5 $
#
# len = strlen(str)
#
	version 2
ifdef(`_NAMESPACE_CLEAN',`
	global	__strlen
	sglobal _strlen
__strlen:',`
	global	_strlen
')
_strlen:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strlen(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strlen,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%d0	#(2) d0 = str
ifdef(`NO_NULL_DEREFERENCE',`
#
# There is not a 0 byte at address 0, so we must explictly check
# for a NULL pointer.  This is not required in the current s300
# implementation, so this code is currently disabled.
#
	bne.b	Lnonnull	# if (str == NULL)
	rts			#     return 0
Lnonnull:
')
	mov.l	%d0,%a0		# 1  a0 = str, set flags
	not.l	%d0		# 0  d0 =  -src -1
L0:
	tst.b	(%a0)+		#(1.3) while (*a0++ != '\0')
	bne.b	L0		#2/3	   continue
	add.l	%a0,%d0		# 1    return a0 - str - 1
	rts
ifdef(`PROFILE',`
	data
p_strlen:
	long	0
')
