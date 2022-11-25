# @(#) $Revision: 66.5 $
#
#
# ptr = memccpy(dest, src, byte, limit)
#
# Returns 0 if limit expires
# Returns ptr to byte in dest *after* the last byte copied if
# the matching byte was found.
#
	version 2
ifdef(`_NAMESPACE_CLEAN',`
	global	__memccpy
	sglobal _memccpy
__memccpy:',`
	global	_memccpy
')
_memccpy:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memccpy(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memccpy,%a0
	jsr	mcount
    ')
')
	mov.l	16(%sp),%d0		#(2)   d0 = count
	beq.b	Lzero			# 3    do nothing
	mov.l	4(%sp),%a1		#(2)   a1 = dest
	subq.l	&1,%d0			# 1    adjust for dbeq
	mov.l	8(%sp),%a0		#(2)   a0 = src
	mov.b	15(%sp),%d1		#(2)   byte to search for
	beq.b	Lsearch1
#
	mov.l	%d2,-(%sp)		#(2)   save d2
#
# Find and move the desired byte
#
Lsearch:
	mov.b	(%a0)+,%d2		#(1.3) get a byte
	mov.b	%d2,(%a1)+		#(1.3) put a byte
	cmp.b	%d2,%d1			# 1    compare 1 byte
	dbeq	%d0,Lsearch		#3/4   keep searching
	bne.b	Lbig_string		#2/3   not found yet

	mov.l	(%sp)+,%d2		#(2)   restore d2
	mov.l	%a1,%d0			# 1    found it, return dest+1
Lzero:
	rts

Lbig_string:
	subi.l	&0x10000,%d0		# 1    decr hi bits, lo is -1
	bcc.b	Lsearch			#3/2   more bytes to check

	mov.l	(%sp)+,%d2		#(2)   restore d2
	movq	&0,%d0			# 1    not found, return 0
	rts

Lsearch1:
	mov.b	(%a0)+,(%a1)+		#(1.3) move a byte
	dbeq	%d0,Lsearch1		#3/4   keep searching
	bne.b	Lbig_string1		#2/3   not found yet

	mov.l	%a1,%d0			# 1    found it, return dest+1
	rts

Lbig_string1:
	subi.l	&0x10000,%d0		# 1    decr hi bits, lo is -1
	bcc.b	Lsearch1		#3/2   more bytes to check

	movq	&0,%d0			# 1    not found, return 0
	rts

ifdef(`PROFILE',`
	data
p_memccpy:
	long	0
')
