# @(#) $Revision: 66.3 $
#
# val = memcmp(addr1, addr2, count);
#
# Compare memory at addr1 with memory at addr2 for for count bytes.
# Returns   0 if all bytes equal.
# Returns < 0 if the a byte in addr1 is < than a byte in addr2 (for the
#	      first mismatched byte).
# Returns > 0 if the a byte in addr1 is > than a byte in addr2 (for the
#	      first mismatched byte).
#
	version 2
ifdef(`_NAMESPACE_CLEAN',`
	global	__memcmp
	sglobal _memcmp
__memcmp:',`
	global	_memcmp
')
_memcmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memcmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memcmp,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a0		#(2) string1
	mov.l	8(%sp),%a1		#(2) string2
	mov.l	12(%sp),%d0		#(2) count
	mov.l	%d0,%d1			# 1  save count in d1
	lsr.l	&4,%d0			# 2  divide count by 16
	beq.b	Lsmallcnt		#2/3 count < 16

	subq.l	&1,%d0			# 1 decrement for dbne quirk
Loop1:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3 no, get out
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3 no, get out
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3 no, get out
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	dbne	%d0,Loop1		#3/4 continue if match or
					#    !end-of-count
	bne.b	Lnomatch		#2/3 no, get out

	subi.l	&0xFFFF,%d0		# 1  clears lo bits, tests if
					#    hi bits are zero.
	beq.b	Loopdone		#2/3 hi bits zero, do remainder

	subq.l	&1,%d0			# 1 decrement 1 from hi bits
					#   and restore low bits to 1s
	bra.b	Loop1			# 2 continue next 1 Mb

Loopdone:
	andi.w	&15,%d1			# 1 remainder bits
Lsmallcnt:
	jmp	Lswitch(%pc,%d1.w*2)

Lnomatch:
	bhi.b	Lgreater		#2/3 last greater
	movq	&-1,%d0			# 1 less than
	rts

Lgreater:
	movq	&1,%d0			# 1 greater than
	rts

L15:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L11:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L7:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L3:
	cmp.w	(%a0)+,(%a1)+		#(2) short matched?
	bne.b	Lnomatch		#2/3
L1:
	cmp.b	(%a0)+,(%a1)+		#(2) byte matched?
	bne.b	Lnomatch		#2/3
	rts

L14:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L10:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L6:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L2:
	cmp.w	(%a0)+,(%a1)+		#(2) short matched?
	bne.b	Lnomatch		#2/3
Lswitch:
	rts
	bra.b	L1
	bra.b	L2
	bra.b	L3
	bra.b	L4
	bra.b	L5
	bra.b	L6
	bra.b	L7
	bra.b	L8
	bra.b	L9
	bra.b	L10
	bra.b	L11
	bra.b	L12
	bra.b	L13
	bra.b	L14
	bra.b	L15
L16:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L12:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L8:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L4:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
	rts

L13:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L9:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
L5:
	cmp.l	(%a0)+,(%a1)+		#(2) long matched?
	bne.b	Lnomatch		#2/3
	cmp.b	(%a0)+,(%a1)+		#(2) byte matched?
	bne.b	Lnomatch		#2/3
	rts
ifdef(`PROFILE',`
	data
p_memcmp:
	long	0
')
