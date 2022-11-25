# @(#) $Revision: 66.5 $
#
# ptr = memset(mem, c, n);
#
# Sets memory pointed to by 'mem' to the byte 'c' for 'n' bytes.
# Returns 'mem'.
#
	version 2
#
# Switch table for setting 0..35 bytes
#
Ls32:	mov.l	%d1,(%a0)+		# 1-4 clocks per instruction
Ls28:	mov.l	%d1,(%a0)+
Ls24:	mov.l	%d1,(%a0)+
Ls20:	mov.l	%d1,(%a0)+
Ls16:	mov.l	%d1,(%a0)+
Ls12:	mov.l	%d1,(%a0)+
Ls8:	mov.l	%d1,(%a0)+
Ls4:	mov.l	%d1,(%a0)
	rts

Ls33:	mov.l	%d1,(%a0)+		# 1-4 clocks per instruction
Ls29:	mov.l	%d1,(%a0)+
Ls25:	mov.l	%d1,(%a0)+
Ls21:	mov.l	%d1,(%a0)+
Ls17:	mov.l	%d1,(%a0)+
Ls13:	mov.l	%d1,(%a0)+
Ls9:	mov.l	%d1,(%a0)+
Ls5:	mov.l	%d1,(%a0)+
Ls1:	mov.b	%d1,(%a0)
	rts

Ls34:	mov.l	%d1,(%a0)+		# 1-4 clocks per instruction
Ls30:	mov.l	%d1,(%a0)+
Ls26:	mov.l	%d1,(%a0)+
Ls22:	mov.l	%d1,(%a0)+
Ls18:	mov.l	%d1,(%a0)+
Ls14:	mov.l	%d1,(%a0)+
Ls10:	mov.l	%d1,(%a0)+
Ls6:	mov.l	%d1,(%a0)+
Ls2:	mov.w	%d1,(%a0)
Lsswitch:
	rts
	bra.b	Ls1			# 2
	bra.b	Ls2
	bra.b	Ls3
	bra.b	Ls4
	bra.b	Ls5
	bra.b	Ls6
	bra.b	Ls7
	bra.b	Ls8
	bra.b	Ls9
	bra.b	Ls10
	bra.b	Ls11
	bra.b	Ls12
	bra.b	Ls13
	bra.b	Ls14
	bra.b	Ls15
	bra.b	Ls16
	bra.b	Ls17
	bra.b	Ls18
	bra.b	Ls19
	bra.b	Ls20
	bra.b	Ls21
	bra.b	Ls22
	bra.b	Ls23
	bra.b	Ls24
	bra.b	Ls25
	bra.b	Ls26
	bra.b	Ls27
	bra.b	Ls28
	bra.b	Ls29
	bra.b	Ls30
	bra.b	Ls31
	bra.b	Ls32
	bra.b	Ls33
	bra.b	Ls34
Ls35:	mov.l	%d1,(%a0)+		# 1-4 clocks per instruction
Ls31:	mov.l	%d1,(%a0)+
Ls27:	mov.l	%d1,(%a0)+
Ls23:	mov.l	%d1,(%a0)+
Ls19:	mov.l	%d1,(%a0)+
Ls15:	mov.l	%d1,(%a0)+
Ls11:	mov.l	%d1,(%a0)+
Ls7:	mov.l	%d1,(%a0)+
Ls3:	mov.w	%d1,(%a0)+
	mov.b	%d1,(%a0)
	rts

ifdef(`_NAMESPACE_CLEAN',`
	global	__memset
	sglobal _memset
__memset:',`
	global	_memset
')
_memset:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memset(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memset,%a0
	jsr	mcount
    ')
')
#
# replicate the byte 4 times into d1
#
	mov.b	11(%sp),%d1		# d1 = value
	mov.b	%d1,%d0			# 1 make copy in d0
	lsl.w	&8,%d0			# 2 slide up to next byte
	add.b	%d1,%d0			# 1 replicate in 2 high bytes
	mov.w	%d0,%d1			# 1 make copy in d1
	swap	%d1			# 2
	mov.w	%d0,%d1			# 1 d1 = byte replicated 4 times

#
# get remaining parameters
#
	mov.l	12(%sp),%a1		#(2) a1 = count
	mov.l	4(%sp),%d0		#(2) d0 = dest
	mov.l	%d0,%a0			# 1  a0 = dest
	cmpa.l	%a1,&35			# 1  check if > threshold
	bhi.b	Lmemset_align		#2/3 yes, set large blocks
#
# Short move, setup destination address and return value and
# jump to the appropriate code fragment for 'count' number of
# bytes.
#
	jmp	Lsswitch(%pc,%a1.w*2)	 # 7

#
# 36 bytes or greater to set, so 4 byte align before starting loop
#
# a1 = count
# d1 = value (duplicated in all four bytes of register)
# a0 = address (but not yet)
#
Lmemset_align:
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	%d1,(%a0)+		#(2) copy over four bytes
	sub.l	%d0,%a0			# 1  decrement correct amount
	add.l	%a1,%d0			# 1  adjust count
	subi.l	&36,%d0			# 1  decr count (32+4 for dbra)
	mov.w	&32,%a1			# 1  loop size
	
#
# 32 byte block move loop
#
Lmemset_loop:
	mov.l	%d1,(%a0)+		#1-4 move 32 bytes
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	mov.l	%d1,(%a0)+
	sub.l	%a1,%d0			# 1  decrement count 
	bcc.b   Lmemset_loop		#2/3 another 32 bytes?

	mov.l	%d0,%a1			# 1  a1 = remainder
	mov.l	4(%sp),%d0		#(2) return value (dest)
	jmp	Lsswitch+32+32(%pc,%a1.w*2) # 7

ifdef(`PROFILE',`
	data
p_memset:
	long	0
')
