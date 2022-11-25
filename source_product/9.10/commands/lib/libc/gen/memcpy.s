# @(#) $Revision: 66.9 $
#
# ptr = memcpy(dest, src, count);
#
# Copy memory from src to dest for count bytes.
# Returns dest.
#
	version 2
# Switch table for moving 0..36 bytes

Lm35:	mov.l	(%a0)+,(%a1)+		# 2-6 clocks per instruction
Lm31:	mov.l	(%a0)+,(%a1)+
Lm27:	mov.l	(%a0)+,(%a1)+
Lm23:	mov.l	(%a0)+,(%a1)+
Lm19:	mov.l	(%a0)+,(%a1)+
Lm15:	mov.l	(%a0)+,(%a1)+
Lm11:	mov.l	(%a0)+,(%a1)+
Lm7:	mov.l	(%a0)+,(%a1)+
Lm3:	mov.w	(%a0)+,(%a1)+
	mov.b	(%a0),(%a1)
	rts

Lm34:	mov.l	(%a0)+,(%a1)+		# 2-6 clocks per instruction
Lm30:	mov.l	(%a0)+,(%a1)+
Lm26:	mov.l	(%a0)+,(%a1)+
Lm22:	mov.l	(%a0)+,(%a1)+
Lm18:	mov.l	(%a0)+,(%a1)+
Lm14:	mov.l	(%a0)+,(%a1)+
Lm10:	mov.l	(%a0)+,(%a1)+
Lm6:	mov.l	(%a0)+,(%a1)+
Lm2:	mov.w	(%a0),(%a1)

Lmswitch:
	rts
	bra.b	Lm1
	bra.b	Lm2
	bra.b	Lm3
	bra.b	Lm4
	bra.b	Lm5
	bra.b	Lm6
	bra.b	Lm7
	bra.b	Lm8
	bra.b	Lm9
	bra.b	Lm10
	bra.b	Lm11
	bra.b	Lm12
	bra.b	Lm13
	bra.b	Lm14
	bra.b	Lm15
	bra.b	Lm16
	bra.b	Lm17
	bra.b	Lm18
	bra.b	Lm19
	bra.b	Lm20
	bra.b	Lm21
	bra.b	Lm22
	bra.b	Lm23
	bra.b	Lm24
	bra.b	Lm25
	bra.b	Lm26
	bra.b	Lm27
	bra.b	Lm28
	bra.b	Lm29
	bra.b	Lm30
	bra.b	Lm31
	bra.b	Lm32
	bra.b	Lm33
	bra.b	Lm34
	bra.b	Lm35
Lm36:	mov.l	(%a0)+,(%a1)+		# 2-6 clocks per instruction
Lm32:	mov.l	(%a0)+,(%a1)+
Lm28:	mov.l	(%a0)+,(%a1)+
Lm24:	mov.l	(%a0)+,(%a1)+
Lm20:	mov.l	(%a0)+,(%a1)+
Lm16:	mov.l	(%a0)+,(%a1)+
Lm12:	mov.l	(%a0)+,(%a1)+
Lm8:	mov.l	(%a0)+,(%a1)+
Lm4:	mov.l	(%a0),(%a1)
	rts

Lm33:	mov.l	(%a0)+,(%a1)+		# 2-6 clocks per instruction
Lm29:	mov.l	(%a0)+,(%a1)+
Lm25:	mov.l	(%a0)+,(%a1)+
Lm21:	mov.l	(%a0)+,(%a1)+
Lm17:	mov.l	(%a0)+,(%a1)+
Lm13:	mov.l	(%a0)+,(%a1)+
Lm9:	mov.l	(%a0)+,(%a1)+
Lm5:	mov.l	(%a0)+,(%a1)+
Lm1:	mov.b	(%a0),(%a1)
	rts

ifdef(`_NAMESPACE_CLEAN',`
	global	__memcpy
	sglobal _memcpy
__memcpy:',`
	global	_memcpy
')
_memcpy:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memcpy(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memcpy,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a1		#2-6 a1 = dst
	mov.l	8(%sp),%a0		#2-6 a0 = src
	mov.l	12(%sp),%d1		#2-6 d0 = count
	mov.l	%a1,%d0			# 1  d0 = dest
	subi.l	&36,%d1			# 1  check if > threshold
 	bhi.b	Lmemcpy_align		#2/3 yes, move large block
	jmp	Lmswitch+72(%pc,%d1.w*2)

# 36 byte or greater byte move, so 4 byte destination align before copy

Lmemcpy_align:
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	(%a0)+,(%a1)+		#2-6 copy over four bytes
	add.l	%d0,%d1			# 1  adjust count
	sub.l	%d0,%a1			# 1  decrement to alignment
	sub.l	%d0,%a0			# 1  decrement correct amount
	movq.l	&32,%d0			# 1  keep constant 32 in reg.

# 32 byte block move loop

Lmemcpy_loop:
	mov.l	(%a0)+,(%a1)+		#19-51 clocks to move 32 bytes
	mov.l	(%a0)+,(%a1)+		#  19 best case (in-cache)
	mov.l	(%a0)+,(%a1)+		#  51 worst case (memory)
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	sub.l	%d0,%d1			# 1  decrement count
	bcc.b	Lmemcpy_loop		#2/3 loop if more bytes

	mov.l	4(%sp),%d0		#2-6 return d0 = dst
	jmp	Lmswitch+64(%pc,%d1.w*2)

ifdef(`PROFILE',`
	data
p_memcpy:
	long	0
')
