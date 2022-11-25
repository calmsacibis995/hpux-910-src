# HPUX_ID: @(#) $Revision: 70.1 $

#	bor.s
#
#	Series 300 dynamic loader
#
#	assembly hacking for bind on reference


	global	_shl_bor
	global	_dirty_sp

	text
_shl_bor:
	mov.l	%a0,-(%sp)		# save old %a0, %a1
	mov.l	%a1,-(%sp)
ifdef(`CHEAT_IN_ASM',`
ifdef(`PURGE_CACHE',`
',`
	mov.l	8(%sp),%a1
	mov.w	-6(%a1),%a0		# get BSR instruction
	cmpa.w	%a0,&0x60ff		# is it BRA?
	bne.b	cache_synced
	lea.l	-4(%a1),%a0		#	get "here"
	add.l	(%a0),%a0		#	target = displacement + here
	bra.w	got_target_in_a0	#	goto got_target_in_a0
cache_synced:
')
')
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	_dirty_sp(%a0),%a0	# get "dirty" area of stack
	mov.l	%sp,%a1
	cmp.l	%a1,(%a0)		# if current sp is above dirty area
	bls.b	moved_sp
	mov.l	(%a0),%sp		#	move sp to dirty area
moved_sp:
	mov.l	%a1,-(%sp)
	mov.l	%d0,-(%sp)		# save old scratch registers
	mov.l	%d1,-(%sp)
	mov.l	8(%a1),-(%sp)		# get plt entry address
	bsr.l	_bor			# call C routine to do heavy work
	addq.l	&4,%sp
	mov.l	%d0,%a0
	mov.l	(%sp)+,%d1		# restore scratch registers
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%a1		# restore %sp
	mov.l	%a1,%sp
got_target_in_a0:
	mov.l	%a0,8(%sp)		# store target address
	mov.l	(%sp)+,%a1
	mov.l	(%sp)+,%a0
	clr.l	-4(%sp)
	clr.l	-8(%sp)
	rts				# "return" to target

ifdef(`FLUSH_CACHE',`
	text
	global	_flush_cache
_flush_cache:
	mov.l	8(%sp),%d0		# what cpu are we on?
	cmpi.w	%d0,&0x020e
	blt.b	not040			# if >= 68040
	# The 68040 has a copy-back cache,
	# and it is conceivable that the displacement could have been flushed
	# before the opcode was written.
	# The move16 instruction has the side effect
	# of flushing cache entries while it writes,
	# so it should make sure the whole instruction makes it to memory.
	mov.l	4(%sp),%a0		#	get address
	mov.l	%a0,%a1
	move16	(%a0)+,(%a1)+		#	flush data cache line
not020:
	rts
not040:
	cmpi.w	%d0,&0x020c		# else if 68020
	bne.b	not020
	# The 68020 has only a 4 byte I-cache line,
	# so our patched PLT entry will span a line.
	# It is only 256 bytes total, however,
	# so we can easily purge the cache manually.
	# Note the Model 320 has a large external cache
	# with 4 byte cache lines as well,
	# but it is a write through cache,
	# so there is no problem.
	# The other Series 300 models all have 16 byte I-cache lines,
	# and since the PLT is itself 8 byte aligned,
	# every entry (8 bytes each) sits nicely in a half line.
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	tpf.l	&0
	rts
')
