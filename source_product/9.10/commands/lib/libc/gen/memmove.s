# @(#) $Revision: 66.1 $
#
# ptr = memmove(dest, src, count);
#
# Copy memory from src to dest for count bytes.
# Returns dest.
#
# NOTE:  All timings are for an 9000/375 - 25Mhz 68040 SPU.
#   1     = 1 clock  ( 40 Nanoseconds)
#   2     = 2 clocks ( 80 Nanoseconds)
#   3     = 3 clocks (120 Nanoseconds)
#  2/3    = 2 clocks if branch taken, 3 clocks if not
#  2-6    = 2 clocks if no memory reference,
#           6 clock average with none in cache
#  1-4    = 1 clocks if no memory reference,
#           4 clock average with cache misses
# (21-66) = loop times in clocks for best and worst case.
#
	version 2

#
# move the least bytes of the count (backward copy)
#
Lr32:	mov.l	-(%a0),-(%a1)
Lr28:	mov.l	-(%a0),-(%a1)
Lr24:	mov.l	-(%a0),-(%a1)
Lr20:	mov.l	-(%a0),-(%a1)
Lr16:	mov.l	-(%a0),-(%a1)
Lr12:	mov.l	-(%a0),-(%a1)
Lr8:	mov.l	-(%a0),-(%a1)
Lr4:	mov.l	-(%a0),-(%a1)
	rts

Lr33:	mov.l	-(%a0),-(%a1)
Lr29:	mov.l	-(%a0),-(%a1)
Lr25:	mov.l	-(%a0),-(%a1)
Lr21:	mov.l	-(%a0),-(%a1)
Lr17:	mov.l	-(%a0),-(%a1)
Lr13:	mov.l	-(%a0),-(%a1)
Lr9:	mov.l	-(%a0),-(%a1)
Lr5:	mov.l	-(%a0),-(%a1)
Lr1:	mov.b	-(%a0),-(%a1)
	rts

Lr34:	mov.l	-(%a0),-(%a1)
Lr30:	mov.l	-(%a0),-(%a1)
Lr26:	mov.l	-(%a0),-(%a1)
Lr22:	mov.l	-(%a0),-(%a1)
Lr18:	mov.l	-(%a0),-(%a1)
Lr14:	mov.l	-(%a0),-(%a1)
Lr10:	mov.l	-(%a0),-(%a1)
Lr6:	mov.l	-(%a0),-(%a1)
Lr2:	mov.w	-(%a0),-(%a1)

Lswitch_table_rev:
	rts
	bra.b	Lr1
	bra.b	Lr2
	bra.b	Lr3
	bra.b	Lr4
	bra.b	Lr5
	bra.b	Lr6
	bra.b	Lr7
	bra.b	Lr8
	bra.b	Lr9
	bra.b	Lr10
	bra.b	Lr11
	bra.b	Lr12
	bra.b	Lr13
	bra.b	Lr14
	bra.b	Lr15
	bra.b	Lr16
	bra.b	Lr17
	bra.b	Lr18
	bra.b	Lr19
	bra.b	Lr20
	bra.b	Lr21
	bra.b	Lr22
	bra.b	Lr23
	bra.b	Lr24
	bra.b	Lr25
	bra.b	Lr26
	bra.b	Lr27
	bra.b	Lr28
	bra.b	Lr29
	bra.b	Lr30
	bra.b	Lr31
	bra.b	Lr32
	bra.b	Lr33
	bra.b	Lr34
Lr35:	mov.l	-(%a0),-(%a1)
Lr31:	mov.l	-(%a0),-(%a1)
Lr27:	mov.l	-(%a0),-(%a1)
Lr23:	mov.l	-(%a0),-(%a1)
Lr19:	mov.l	-(%a0),-(%a1)
Lr15:	mov.l	-(%a0),-(%a1)
Lr11:	mov.l	-(%a0),-(%a1)
Lr7:	mov.l	-(%a0),-(%a1)
Lr3:	mov.w	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	rts

#
# slow backward move loop, because dest > src, but within 4 bytes
#
Lbackslow_loop:
	mov.b	-(%a0),-(%a1)		#(19-22) move another 8 bytes
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
Lbackslow:
	subq.l	&8,%d1			# 1  check if another loop
	bcc.b   Lbackslow_loop		#2/3 try another 8 bytes

	neg.l	%d1			# 1  produce correct index
	jmp	0(%pc,%d1.w*2)		# 7  move remainder
	mov.b	-(%a0),-(%a1)		# 2  move 1 byte
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	mov.l	%a1,%d0			# 1  d0 = dst (s1)
	rts

#
# 4 byte align and backward move with unrolled
# mov.l loop 32 bytes at a time
#
Lbcopy_4b_align_rev:
	mov.l	%a1,%d0			# 1  low 2 bits of dest address
	neg.l	%d0			# 1  negate because of rev move
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	-(%a0),-(%a1)		#2-6 copy over four bytes
	add.l	%d0,%d1			# 1  adjust count to alignment
	add.l	%d0,%a1			# 1  decrement dest to alignment
	add.l	%d0,%a0			# 1  decrease src correct amount
	movq.l	&32,%d0			# 1  loop size counter

#
# 64 byte block move loop
#
Lrbcopy_loop_rev:
	mov.l	-(%a0),-(%a1)		#(23-51) move 32 bytes
	mov.l	-(%a0),-(%a1)		# 24 - 62 Nanosec per byte
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	sub.l	%d0,%d1			# 1  check if another loop
	bcc.b	Lrbcopy_loop_rev	#2/3 loop if more bytes

	mov.l	4(%sp),%d0		#(2) d0 = dst (s1)
	jmp	Lswitch_table_rev+32+32(%pc,%d1.w*2) # 7  small block

#
# Here we know dst > src and buffers overlap,
# so check if overlap is < 4 bytes
#
Lbackward:
	adda.l	%d1,%a1			# 1  dst_a1 = dst + count
	cmpi.l	%d0,&4			# 1  check at least < 4 bytes apart
	bcs.b	Lbackslow		#2/3 yes, move byte-at-time backward

	subi.l	&32+4,%d1		# 1  check if >= threshold
 	bcc.b	Lbcopy_4b_align_rev	#2/3 yes, move large block

	mov.l	4(%sp),%d0		#(2) d0 = dst (s1)
	jmp	Lswitch_table_rev+36+36(%pc,%d1.w*2) # 7  small block

#
# move the least bytes of the count (forward copy)
#
Lf32:	mov.l	(%a0)+,(%a1)+
Lf28:	mov.l	(%a0)+,(%a1)+
Lf24:	mov.l	(%a0)+,(%a1)+
Lf20:	mov.l	(%a0)+,(%a1)+
Lf16:	mov.l	(%a0)+,(%a1)+
Lf12:	mov.l	(%a0)+,(%a1)+
Lf8:	mov.l	(%a0)+,(%a1)+
Lf4:	mov.l	(%a0),(%a1)
	rts

Lf33:	mov.l	(%a0)+,(%a1)+
Lf29:	mov.l	(%a0)+,(%a1)+
Lf25:	mov.l	(%a0)+,(%a1)+
Lf21:	mov.l	(%a0)+,(%a1)+
Lf17:	mov.l	(%a0)+,(%a1)+
Lf13:	mov.l	(%a0)+,(%a1)+
Lf9:	mov.l	(%a0)+,(%a1)+
Lf5:	mov.l	(%a0)+,(%a1)+
Lf1:	mov.b	(%a0),(%a1)
	rts

Lf34:	mov.l	(%a0)+,(%a1)+
Lf30:	mov.l	(%a0)+,(%a1)+
Lf26:	mov.l	(%a0)+,(%a1)+
Lf22:	mov.l	(%a0)+,(%a1)+
Lf18:	mov.l	(%a0)+,(%a1)+
Lf14:	mov.l	(%a0)+,(%a1)+
Lf10:	mov.l	(%a0)+,(%a1)+
Lf6:	mov.l	(%a0)+,(%a1)+
Lf2:	mov.w	(%a0),(%a1)

Lswitch_table_fwd:
	rts
	bra.b	Lf1
	bra.b	Lf2
	bra.b	Lf3
	bra.b	Lf4
	bra.b	Lf5
	bra.b	Lf6
	bra.b	Lf7
	bra.b	Lf8
	bra.b	Lf9
	bra.b	Lf10
	bra.b	Lf11
	bra.b	Lf12
	bra.b	Lf13
	bra.b	Lf14
	bra.b	Lf15
	bra.b	Lf16
	bra.b	Lf17
	bra.b	Lf18
	bra.b	Lf19
	bra.b	Lf20
	bra.b	Lf21
	bra.b	Lf22
	bra.b	Lf23
	bra.b	Lf24
	bra.b	Lf25
	bra.b	Lf26
	bra.b	Lf27
	bra.b	Lf28
	bra.b	Lf29
	bra.b	Lf30
	bra.b	Lf31
	bra.b	Lf32
	bra.b	Lf33
	bra.b	Lf34
Lf35:	mov.l	(%a0)+,(%a1)+
Lf31:	mov.l	(%a0)+,(%a1)+
Lf27:	mov.l	(%a0)+,(%a1)+
Lf23:	mov.l	(%a0)+,(%a1)+
Lf19:	mov.l	(%a0)+,(%a1)+
Lf15:	mov.l	(%a0)+,(%a1)+
Lf11:	mov.l	(%a0)+,(%a1)+
Lf7:	mov.l	(%a0)+,(%a1)+
Lf3:	mov.w	(%a0)+,(%a1)+
	mov.b	(%a0),(%a1)
	rts

ifdef(`_NAMESPACE_CLEAN',`
	global	__memmove
	sglobal _memmove
__memmove:',`
	global	_memmove
')
_memmove:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memmove(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memmove,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a1		#(2) a1 = dst (s1)
	mov.l	8(%sp),%a0		#(2) a0 = src (s2)
	mov.l	12(%sp),%d1		#(2) d0 = count

#
# if (dst_a1 > src_a0) backwards copy may be necessary
#
	mov.l	%a1,%d0			# 1  d0 = dst
	sub.l	%a0,%d0			# 1  d0 = dst - src
	bls.b	Lsrc_ge_dst		#2/3 forward move, dst <= src

	adda.l	%d1,%a0			# 1  src_a0 = src + count
	cmp.l	%a0,%a1			# 1  check src+count > dst
	bhi.w	Lbackward		#2/3 yes, overlap, MUST backward copy

	suba.l	%d1,%a0			# 1  restore src_a0 = src
	bra.b	Lforward		# 2  forward move

Lsrc_ge_dst:				#    src >= dst (d0 zero or neg)
	beq.b	Lcexit			#2/3 src == dst, no moving to do
	cmpi.l	%d0,&-4			# 1  check if move < 4 bytes apart
	bhi.b	Lfwdslow		#2/3 yes, move byte-at-time forward

Lforward:
	mov.l	%a1,%d0			# 1  d0 = dst
	subi.l	&32+4,%d1		# 1  check if > threshold
	bcc.b	Lbcopy_4b_align_fwd	#2/3 yes, copy medium block

	jmp	Lswitch_table_fwd+36+36(%pc,%d1.w*2) # 7  small block
Lcexit:
	mov.l	%a1,%d0			# 1  d0 = dst
	rts

#
# 4 byte align and move with unrolled mov.l loop 32 bytes at a time
#
Lbcopy_4b_align_fwd:			# d1 = count-36, a1 = dest, a0 = src
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	(%a0)+,(%a1)+		#2-6 copy over four bytes
	add.l	%d0,%d1			# 1  adjust count for alignment
	sub.l	%d0,%a1			# 1  decrement to alignment
	sub.l	%d0,%a0			# 1  decrement correct amount
	movq.l	&32,%d0			# 1  loop size counter

#
# 32 byte block move loop
#
Lbfcopy_loop_fwd:
	mov.l	(%a0)+,(%a1)+		#(23-51) move 32 bytes
	mov.l	(%a0)+,(%a1)+		# 24 - 62 Nanosec per byte
	mov.l	(%a0)+,(%a1)+		#1-6
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	sub.l	%d0,%d1			# 1  check if another loop
	bcc.b	Lbfcopy_loop_fwd	#2/3 loop if more bytes

	mov.l	4(%sp),%d0		#(2) d0 = dst (s1)
	jmp	Lswitch_table_fwd+32+32(%pc,%d1.w*2) # 10 move small block

#
# slow forward move loop, because source and dest are within 4 bytes
#
Lfwdslow_loop:
	mov.b	(%a0)+,(%a1)+		#(19-22) move another 8 bytes
	mov.b	(%a0)+,(%a1)+		# 95 - 110 Nanosec per byte
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
Lfwdslow:
	subq.l	&8,%d1			# 1  check if another loop
	bcc.b   Lfwdslow_loop		#2/3 try another 4 bytes

	neg.l	%d1			# 1  make correct branch value
	jmp	0(%pc,%d1.w*2)		# 7  move remainder
	mov.b	(%a0)+,(%a1)+		# 2  move remaining bytes
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.b	(%a0)+,(%a1)+
	mov.l	4(%sp),%d0		#(2) d0 = dst (s1)
	rts

ifdef(`PROFILE',`
	data
p_memmove:
	long	0
')
