	global	_scksum
_scksum:
	mov.l	4(%sp),%a0		# buffer to sum
	mov.l	8(%sp),%d1		# of bytes to sum 2097152 MAX
	mov.l	%d2, %a1                # save d2 in a1
	mov.l	%d1,%d2			# number of
	and.l	&0x1c,%d2		# 4 byte adds in
	ror.l	&5,%d1			# loop
	neg.l	%d2			#
	sub.l	%d0,%d0			# 1 clear sum register & X
	jmp	Ldbf(%pc,%d2.w*1)	# jump into loop

Lbigloop:
	mov.l	(%a0)+,%d2		#(2) yes, do 32 byte
	addx.l	%d2,%d0			# 1 accumulate 
	mov.l	(%a0)+,%d2		#(2) 
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
	mov.l	(%a0)+,%d2		#(2)
	addx.l	%d2,%d0			# 1
Ldbf:
	dbf	%d1,Lbigloop		#3/4 another 32 bytes?

 # Now we have 0 to 3 bytes left to do
	clr.w	%d1			# 1 prepare count
	rol.l	&5,%d1			# 2 restore count
	andi.l	&0x3, %d1		# 0 - 3 bytes to go
	movq	&0, %d2			# prepare temp
	ror.l	&2, %d1			# get 2nd bit
	bcc.w	L1			# branch if 0 or 1
	mov.w	(%a0)+, %d2		# else add
	addx.w	%d2,%d0			# next word
L1:	rol.l	&2, %d1			# get 1st bit
	bcc.w	L0			# branch if 0
	movq	&0, %d2			# prepare temp
	mov.b	(%a0), %d2		# add last byte
	rol.w	&8, %d2			# shifted by 8
	addx.w	%d2, %d0		#
L0:	mov.l	%d0,%d1			# 1
	swap	%d1			# 2
	addx.w	%d0,%d1			# 1
	movq	&0,%d0			# 1
	addx.w	%d1,%d0			# 1
	mov.l	%a1, %d2		# restore d2
	rts
