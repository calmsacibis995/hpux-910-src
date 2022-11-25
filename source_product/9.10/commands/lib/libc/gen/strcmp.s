# @(#) $Revision: 66.2 $
# C library -- strcmp

#   strcmp(s1, s1)
#   unsigned char *s1;
#   unsigned char *s2;
#   {
#	for ( ; *s1 == *s2; s1++, s2++)
#	    if (*s1 == '\0')
#		return 0;
#	if (*s1 > *s2)
#	    return 1;
#	return -1;
#   }
#
ifdef(`_NAMESPACE_CLEAN',`
	global	__strcmp
	sglobal _strcmp',`
	global	_strcmp
 ')

ifdef(`_NAMESPACE_CLEAN',`
__strcmp:
')
_strcmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strcmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strcmp,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a0		# get s1
	mov.l	8(%sp),%a1		# get s2

	mov.b	(%a0),%d0		# check 1st byte a shorter way
	cmp.b	%d0,(%a1)		# compare 1st byte of s2
	beq.b	L4			# match ?
L1:
	bcc.b	L3			# no, check if s1 < s2 (unsigned char)
L2:
	movq	&-1,%d0			# yep, exit -1
	rts
L3:
	movq	&1,%d0			# nope, exit +1
	rts
L4:
	mov.w	%a0,%d0			# get least 2 bits s1
	mov.w	%a1,%d1			# get least 2 bits s2
	or.b	%d0,%d1			# merge them
	movq	&3,%d0			# get mask and clear upper 24 bits
	and.b	%d0,%d1			# are both long word aligned?
	bne.b	L7			# no, use slow algorithm
L5:
	mov.l	(%a0)+,%d0		# entry must be s1 & s2 long aligned
	mov.l	%d0,%d1			# begin test for 0 byte in d0
	sub.l	&0x01010101,%d1
	or.l	%d0,%d1
	eor.l	%d0,%d1			# And then a miracle happens
	and.l	&0x80808080,%d1		# about here
	bne.b	L6			# end	is there a 0 byte in d0?
	cmp.l	%d0,(%a1)+		# no,
	beq.b	L5			# continue if 4 bytes are equal
	bra.b	L1			# not equal, check for largest one
L6:
	subq	&4,%a0			# 0 byte in %d0, back up s1 4 bytes
	movq	&0,%d0			# check remaining one byte at a time
L7:
	mov.b	(%a0)+,%d0		# get s1 byte, d0 == 0 if null byte
	cmp.b	%d0,(%a1)+		# check if equal
	dbne	%d0,L7			# fall thru if equal -or- if d0 == null
	bne.b	L1			# not equal, check which one is bigger
	movq	&0,%d0			# strings match
	rts
	data
ifdef(`PROFILE',`
		data
p_strcmp:	long	0
')
