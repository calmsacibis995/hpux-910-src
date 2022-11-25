;#
;# algsum(buf, count, sum)
;# unsigned char *buf;
;# int count;
;# unsigned long sum;
;#
;# {
;#     /* Compute a 16 bit checksum in 'sum' */
;#     for (i = 0; i < count; i++)
;#         sum = 16bit_rotate_1(sum) + buf[i]
;#     return sum & 0xffff;
;# }
;#
;# Since C does not have a rotate operator, this code is written
;# here in assembly for efficiency.
;#

#if defined(__hp9000s300)
;#
;# MC-680x0 version
;#
;# Entry overhead is   13 clocks
;# Loop efficiency is  16/(16+3) [84.2 %]
;# Exit overhead is    10 clocks
	text
	global	_algsum
_algsum:
	mov.l	4(%sp),%a0		# (2) a0 = buf
	mov.l	8(%sp),%d1		# (2) get count
	mov.l	12(%sp),%d0		# (2) sum so far
	mov.l	%d2,%a1			# 1   save d2
	movq.l	&0,%d2			# 1   zero d2
	bra.b	Lloop_end		# 2

Lswitch:
	bra.b	Ldone			# 2   no bytes left
	bra.b	Lone_byte		# 2   one byte left
	bra.b	Ltwo_byte		# 2   two bytes left
Lthree_byte:
	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum
Ltwo_byte:
	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum
Lone_byte:
	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum
Ldone:
	mov.l	%a1,%d2			# 1   restore d2
	rts

Lloop:
	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum

	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum

	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum

	mov.b	(%a0)+,%d2		# 1   get next byte
	ror.w	&1,%d0			# 2   rotate sum right 1
	add.w	%d2,%d0			# 1   add byte to 16 bit sum
Lloop_end:
	subq.l	&4,%d1			# 1   count -= 4
	bcc.b	Lloop			#2/3  do 4 more bytes
	jmp	Lswitch+4+4(%pc,%d1.w*2) # 7 process 0..3 bytes

#else /* s700, s800 version */
;
; HP-PA version
;
; NOTE: The loop here is unrolled to reduce overhead and must always
;	process an even number of bytes.  We take 4 instructions to
;	process each byte, plus one instruction for the branch.
;	Two bytes per loop yields:      (8/(8+1)) or 88.9% efficiency.
;	Four bytes per loop yields:	(16/(16+1)) or 94.1% efficiency.
;	Eight bytes per loop yields:	(32/(32+1)) or 97.0% efficiency.
;
;	Currently, the code does 4 bytes/loop (94.1% efficient).
;
#include "/lib/pcc_prefix.s"

	.space	$TEXT$
	.subspa $CODE$
	.proc
	.export		algsum,entry
	.callinfo	no_calls
	.entry


#define buf	arg0
#define count	arg1
#define sumA	arg2
#define sumB	ret0
#define endp	r31
#define tempc	arg1	/* we reuse arg1 for temp storage */
#define remain	r1

algsum
	extru	  count,31,2,remain	; remain = count & 0x03
	add	  buf,count,endp	; endp = buf + count

	blr	  remain,r0		; switch (remain)
	ldbs,ma   1(0,buf),tempc	; tempc = *buf++    (delay slot)

	b	  Ldo_four		; do four bytes
	extru	  sumA,30,15,sumB	; sumB = (sumA >> 1) & 0x7fff

	b	  Ldo_one		; do one byte
	copy	  sumA,sumB		; sumB = sumA

	b	  Ldo_two		; do two bytes
	extru	  sumA,30,15,sumB	; sumB = (sumA >> 1) & 0x7fff

	b	  Ldo_three		; do three bytes
	copy	  sumA,sumB		; sumB = sumA

Lloop

	extru	  sumA,30,15,sumB	; sumB = (sumA >> 1) & 0x7fff
Ldo_four
	dep	  sumA,16,1,sumB	; sumB |= ((sumA<<15) & 0x8000)
	add	  sumB,tempc,sumB	; sumB += tempc

	ldbs,ma   1(0,buf),tempc	; tempc = *buf++
Ldo_three
	extru	  sumB,30,15,sumA	; sumA = (sumB >> 1) & 0x7fff
	dep	  sumB,16,1,sumA	; sumA |= ((sumB<<15) & 0x8000)
	add	  sumA,tempc,sumA	; sumA += tempc

	ldbs,ma   1(0,buf),tempc	; tempc = *buf++
	extru	  sumA,30,15,sumB	; sumB = (sumA >> 1) & 0x7fff
Ldo_two
	dep	  sumA,16,1,sumB	; sumB |= ((sumA<<15) & 0x8000)
	add	  sumB,tempc,sumB	; sumB += tempc

	ldbs,ma   1(0,buf),tempc	; tempc = *buf++
Ldo_one
	extru	  sumB,30,15,sumA	; sumA = (sumB >> 1) & 0x7fff
	dep	  sumB,16,1,sumA	; sumA |= ((sumB<<15) & 0x8000)
	add	  sumA,tempc,sumA	; sumA += tempc
	comb,<<,n buf,endp,Lloop	; while buf < endp
	ldbs,ma   1(0,buf),tempc	; tempc = *buf++

	bv	  0(2)			; return
	extru	  sumA,31,16,ret0	; sumA & 0xffff
	.procend
	.end
#endif
