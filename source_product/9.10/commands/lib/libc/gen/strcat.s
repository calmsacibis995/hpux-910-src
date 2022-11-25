# @(#) $Revision: 66.1 $
#
# C library -- strcat
#
# strcat(s1, s2) copy s2 to the end of s1 -- return s1 in d0
#
ifdef(`_NAMESPACE_CLEAN',`
	global	__strcat
	sglobal _strcat',`
	global	_strcat
')

ifdef(`_NAMESPACE_CLEAN',`
__strcat:
')
_strcat:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strcat(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strcat,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a0		#(2)   get s1
	mov.l	8(%sp),%a1		#(2)   get s2
	mov.l	%a0,%d0			# 1    return s1 in all cases
ifdef(`NO_NULL_DEREFERENCE',`
#
# There is not a 0 byte at address 0, so we must explictly check
# for a NULL pointer.  This is not required in the current s300
# implementation, so this code is currently disabled.
#
	mov.l   %a1,%d1         	# 1  if (a1 != NULL)
	bne.b	L1			#2/3     do strcat
	rts                     	#    else return s1
')
L1:	
	tst.b	(%a0)+			#(1.3) find end of s1
	bne.b	L1			#2/3   null byte?
	subq.l	&1,%a0			# 1    yes, back up to null byte
L2:		
	mov.b	(%a1)+,(%a0)+		#(2)   move s2 at end of s1 
	bne.b	L2			#2/3   until null byte
	rts

ifdef(`PROFILE',`
	data
p_strcat:
	long	0
')
